/* tcpip.c */

#include "config.h"

/*
 *		       This file is part of TeenyMUD II.
 *		 Copyright(C) 1993, 1994, 1995 by Jason Downs.
 *                           All rights reserved.
 * 
 * TeenyMUD II is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * TeenyMUD II is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program (see the file 'COPYING'); if not, write to
 * the Free Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139,
 * USA.
 *
 */

/* AIX requires this to be the first thing in the file. */
#ifdef __GNUC__
#define alloca	__builtin_alloca
#else	/* not __GNUC__ */
#ifdef HAVE_ALLOCA_H
#include <alloca.h>
#else	/* not HAVE_ALLOCA_H */
#ifdef _AIX
 #pragma alloca
#endif	/* not _AIX */
#endif	/* not HAVE_ALLOCA_H */
#endif 	/* not __GNUC__ */

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#else
#include <time.h>
#endif			/* HAVE_SYS_TIME_H */
#ifdef HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif			/* HAVE_SYS_SELECT_H */
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif			/* HAVE_UNISTD_H */
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#ifdef HAVE_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif			/* HAVE_STRING_H */
#include <netdb.h>
#include <ctype.h>
#include <errno.h>
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif			/* HAVE_STDLIB_H */

#include "conf.h"
#include "teeny.h"
#include "externs.h"

/* dynamic tcpip interface. */

extern int errno;

/* a typical dynamic buffer-struct. */
struct buffer {
  char *ptr;
  char full;

  int len;

  struct buffer *prev;
  struct buffer *next;
};

/* pull in the tcpip version of conn.h. oh, to have C++. */
#define __TCPIP_C
#include "conn.h"

struct conn *connlist = (struct conn *)NULL;

static int topfd, servfd, rwhofd;
static fd_set live;
static char *localhost;

static void tcp_flush _ANSI_ARGS_((void));
static char *conn_addr _ANSI_ARGS_((struct in_addr));
static void buffer_output _ANSI_ARGS_((struct conn *, char *));
static void buffer_input _ANSI_ARGS_((struct conn *));
static void buffer_addinput _ANSI_ARGS_((struct conn *, char *, int));
static void buffer_flush _ANSI_ARGS_((struct conn *));
static void buffer_command _ANSI_ARGS_((struct conn *));
static void buffer_free _ANSI_ARGS_((struct buffer *));
static struct buffer *buffer_alloc _ANSI_ARGS_((void));
static char *tcp_authuser _ANSI_ARGS_((unsigned long, unsigned short,
				       unsigned short));

#ifndef PF_INET
#define PF_INET	AF_INET
#endif

#ifndef AGGREGATE_LEN
#define AGGREGATE_LEN	128
#endif

extern const char teenymud_version[];

/* the real main loop. */
void tcp_loop()
{
  int opt = 1, done = 0;
  int selr, addrlen, newfd;
  struct sockaddr_in addr;
  fd_set readset, excptset;
  struct timeval timeout;
  struct conn *newconn;
  register struct conn *curr;
  char lbuff[BUFFSIZ];
  struct hostent *h;
  unsigned long a;
  char *msg;

  /* set localhost */
  if(gethostname(lbuff, sizeof(lbuff)) != 0) {
    logfile(LOG_ERROR, "tcp_loop: gethostname() failed, %s\n",
	    strerror(errno));
    return;
  }
  h = gethostbyname(lbuff);
  if(h == (struct hostent *)NULL) {
    logfile(LOG_ERROR, "tcp_loop: gethostbyname() failed, %s\n",
	    strerror(errno));
    localhost = (char *)ty_strdup(lbuff, "tcp_loop.localhost");
  } else
    localhost = (char *)ty_strdup(h->h_name, "tcp_loop.localhost");

  /* set up the server. */
  servfd = socket(PF_INET, SOCK_STREAM, 0);
  if(servfd == -1) {
    logfile(LOG_ERROR, "tcp_loop: socket() failed, %s\n", strerror(errno));
    return;
  }
  if(setsockopt(servfd, SOL_SOCKET, SO_REUSEADDR,
                (char *)&opt, sizeof(opt)) == -1) {
    logfile(LOG_ERROR, "tcp_loop: setsockopt() failed, %s\n", strerror(errno));
    return;
  }
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = INADDR_ANY;
  addr.sin_port = htons(mudconf.default_port);
  if(bind(servfd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
    logfile(LOG_ERROR, "tcp_loop: bind() failed, %s\n", strerror(errno));
    return;
  }
  if(listen(servfd, 5) == -1) {
    logfile(LOG_ERROR, "tcp_loop: listen() failed, %s\n", strerror(errno));
    return;
  }

  FD_ZERO(&live);
  FD_SET(servfd, &live);

  /* connect to the rwho server */
  if(mudconf.enable_rwho) {
    h = gethostbyname(mudconf.rwho_host);
    if(h == (struct hostent *)NULL) {
      a = inet_addr(mudconf.rwho_host);
      if(a == -1) {
        logfile(LOG_ERROR, "tcp_loop: gethostbyname() failed, %s\n",
	        strerror(errno));
	rwhofd = -1;
	goto loop_start;
      }
      bcopy((VOID *)&a, (VOID *)&addr.sin_addr, sizeof(a));
    } else
      bcopy((VOID *)h->h_addr, (VOID *)&addr.sin_addr, h->h_length);
    addr.sin_port = htons(mudconf.rwho_port);
    addr.sin_family = AF_INET;

    rwhofd = socket(PF_INET, SOCK_DGRAM, 0);
    if(rwhofd == -1) {
      logfile(LOG_ERROR, "tcp_loop: socket() failed, %s\n", strerror(errno));
      goto loop_start;
    }

    if(connect(rwhofd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
      logfile(LOG_ERROR, "tcp_loop: connect() failed, %s\n", strerror(errno));
      close(rwhofd);
      rwhofd = -1;
      goto loop_start;
    }
    if (fcntl(rwhofd, F_SETFL, O_NDELAY) != 0)
      logfile(LOG_ERROR, "tcp_loop: fcntl() failed, %s\n", strerror(errno));

    /* send hello */
    snprintf(lbuff, sizeof(lbuff), "U\t%s\t%s\t%s\t%d\t0\tTeeny%s",
	     mudconf.name, mudconf.rwho_passwd, mudconf.name,
	     (int)mudstat.now, teenymud_version);
    if(send(rwhofd, lbuff, strlen(lbuff), 0) == -1)
      logfile(LOG_ERROR, "tcp_loop: send() failed, %s\n", strerror(errno));
    /* send a ping */
    lbuff[0] = 'M';	/* just change the first character. */
    if(send(rwhofd, lbuff, strlen(lbuff), 0) == -1)
      logfile(LOG_ERROR, "tcp_loop: send() failed, %s\n", strerror(errno));
  } else
    rwhofd = -1;

loop_start:
  topfd = ((rwhofd > servfd) ? rwhofd : servfd);

  /* loop away. */
  while(!done) {
    /* run one command line off the top of every connection */
    for(curr = connlist; curr != (struct conn *)NULL; curr = curr->next) {
      if((curr->ibuff != (struct buffer *)NULL)
         && !(curr->flags & CONN_ERROR) && (curr->ibuff)->full)
	buffer_command(curr);
    }

    /* set up for the main select() */
    bcopy((VOID *)&live, (VOID *)&readset, sizeof(readset));
    bcopy((VOID *)&live, (VOID *)&excptset, sizeof(excptset));
    timeout.tv_sec = 0;
    timeout.tv_usec = 800000;

    selr = select(topfd+1, &readset, (fd_set *)NULL, &excptset, &timeout);
    if(selr == -1) {
      if(errno != EINTR) {
        logfile(LOG_ERROR, "tcp_loop: select() failed, %s\n", strerror(errno));
        return;
      } else
        continue;
    }

    /* start the clock */
    time(&mudstat.now);
    timers();

    if(selr > 0) {

      /* check for new connections */
      if(FD_ISSET(servfd, &readset) && !FD_ISSET(servfd, &excptset)) {
        addrlen = sizeof(addr);
        newfd = accept(servfd, (struct sockaddr *)&addr, &addrlen);

        if(newfd != -1) {
          /* set up the connection. */
#ifdef  SO_LINGER
          struct linger ling;

          ling.l_onoff = 0;
          ling.l_linger = 0;
          setsockopt(newfd, SOL_SOCKET, SO_LINGER, (char *)&ling, sizeof(ling));
#endif
          if (fcntl(newfd, F_SETFL, O_NDELAY) == 0) {
	    newconn = (struct conn *)ty_malloc(sizeof(struct conn), "tcp_loop");
	    newconn->flags = 0;
	    newconn->fd = newfd;
	    newconn->player = -1;
	    newconn->port = ntohs(addr.sin_port);
	    newconn->lwrap = 0;
	    newconn->lcrnl = CONN_CRNL;
	    newconn->connect = mudstat.now;
	    newconn->last = mudstat.now;
	    newconn->cc = 0;
	    newconn->bcc = 0;
	    newconn->ibyte = 0;
	    newconn->obyte = 0;

	    newconn->host = conn_addr(addr.sin_addr);
	    newconn->user = tcp_authuser(addr.sin_addr.s_addr,
	    				 htons(mudconf.default_port),
					 addr.sin_port);
	    newconn->doing = (char *)NULL;
	    newconn->outputprefix = (char *)NULL;
	    newconn->outputsuffix = (char *)NULL;

	    newconn->handler = handle_login;

	    newconn->ibuff = (struct buffer *)NULL;
	    newconn->obuff = (struct buffer *)NULL;

	    /* enter it in our tables. */
	    FD_SET(newfd, &live);
	    if(newfd > topfd)
	      topfd = newfd;
	
	    if(connlist == (struct conn *)NULL) {
	      newconn->prev = newconn;
	      connlist = newconn;
	    } else {
	      /* link it in. */
	      newconn->prev = connlist->prev;
	      (connlist->prev)->next = newconn;
	      connlist->prev = newconn;
	    }
	    newconn->next = (struct conn *)NULL;

	    log_connect(newconn);

	    if(check_host(newconn->host, LOCK_DISCONNECT, &msg)) {
	      if(msg && *msg)
	        conn_put(newconn, msg);
	      newconn->flags = CONN_KILL;
	    } else {
	      /* send banner and greet. */
	      snprintf(lbuff, sizeof(lbuff), "[ Connected to %s, port %d. ]",
	              localhost, mudconf.default_port);
	      conn_put(newconn, lbuff);
	      buffer_flush(newconn);

	      conn_greet(newconn);
	    }
	  } else {
	    logfile(LOG_ERROR, "tcp_loop: fcntl() failed, %s\n",
		    strerror(errno));
	    /* drop it, hard. */
	    shutdown(newfd, 2);
	    close(newfd);
	  }
        } else
          logfile(LOG_ERROR, "tcp_loop: accept() failed, %s\n",
		  strerror(errno));

        selr--;		/* optimization */
      }

      if(FD_ISSET(servfd, &excptset)) {
        logfile(LOG_ERROR, "tcp_loop: server fd set exception.\n");
        return;
      }

      /* check for input or exceptions. */
      if(selr > 0) {
        for(curr = connlist; curr != (struct conn *)NULL; curr = curr->next) {
          if(FD_ISSET(curr->fd, &excptset))
	    curr->flags |= CONN_ERROR;
	  else {
	    if(FD_ISSET(curr->fd, &readset)) {
	      if(curr->cc < mudconf.max_commands)
	        buffer_input(curr);
	      else {
	      	/* only inc bcc once per run */
	      	if(curr->cc == mudconf.max_commands)
	          curr->bcc++;
	      }
	      curr->cc++;
	    }
	  }
        }
      }
    }

    /* flush output */
    tcp_flush();

    /* in case of shutdown... */
    if(mudstat.status == MUD_STOPPED)
      done++;

#ifdef C_ALLOCA
    alloca(0);			/* garbage collect */
#endif			/* C_ALLOCA */
  }
}

/* flush all the buffers/drop connections. */
static void tcp_flush()
{
  register struct conn *curr, *next;

  for(curr = connlist; curr != (struct conn *)NULL; curr = next) {
    next = curr->next;

    if((curr->obuff != (struct buffer *)NULL)
       && !(curr->flags & CONN_ERROR)) {
      buffer_flush(curr);
    } else if((curr->flags & CONN_ERROR) || (curr->flags & CONN_KILL)) {
      conn_close(curr);
    }
  }
}

/* shutdown the network interface. */
void tcp_shutdown()
{
  register struct conn *curr, *next;
  char rbuff[BUFFSIZ];

  /* flush and shutdown all connections */
  for(curr = connlist; curr != (struct conn *)NULL; curr = next) {
    next = curr->next;

    conn_close(curr);
  }

  /* shutdown the server. */
  shutdown(servfd, 2);
  close(servfd);

  /* shutdown rwho */
  if(rwhofd != -1) {
    snprintf(rbuff, sizeof(rbuff), "D\t%s\t%s\t%s", mudconf.name,
	     mudconf.rwho_passwd, mudconf.name);
    if(send(rwhofd, rbuff, strlen(rbuff), 0) == -1)
      logfile(LOG_ERROR, "tcp_shutdown: send() failed, %s\n", strerror(errno));
    close(rwhofd);
  }

  ty_free((VOID *)localhost);
}

static char *conn_addr(addr)
    struct in_addr addr;
{
  struct hostent *he;

  he = gethostbyaddr((char *) &addr.s_addr, sizeof(addr.s_addr), AF_INET);
  if (he) {
    he = gethostbyname(he->h_name);
    if (he)
      return (ty_strdup(he->h_name, "conn_addr.ret"));
  }
  return (ty_strdup(inet_ntoa(addr), "conn_addr.ret"));
}

/* close a connection, low level, flushing it. */
void conn_close(cp)
    struct conn *cp;
{
  register struct buffer *buff, *next;
  char rbuff[BUFFSIZ];

  /* notify, log. */
  if(cp->player != -1) {
    if(numconnected(cp->player) < 2) {
      deanimate(cp->player);

      /* Take care of groups. */
      group_remove(cp->player, cp->player);
    }
    announce_disconnect(cp->player);

    /* notify the rwho server */
    if(rwhofd != -1) {
      snprintf(rbuff, sizeof(rbuff), "Z\t%s\t%s\t%s\t#%d", mudconf.name,
      	       mudconf.rwho_passwd, mudconf.name, cp->player);
      if(send(rwhofd, rbuff, strlen(rbuff), 0) == -1)
	logfile(LOG_ERROR, "conn_close: send() failed, %s\n", strerror(errno));
    }
  }
  log_disconnect(cp);

  if(mudconf.enable_autotoad && (cp->player != -1))
    handle_autotoad(cp->player);

  if((cp->obuff != (struct buffer *)NULL) && !(cp->flags & CONN_ERROR))
    buffer_flush(cp);

  shutdown(cp->fd, 2);
  close(cp->fd);

  /* free it up. */
  for(buff = cp->ibuff; buff != (struct buffer *)NULL; buff = next) {
    next = buff->next;

    buffer_free(buff);
  }
  for(buff = cp->obuff; buff != (struct buffer *)NULL; buff = next) {
    next = buff->next;

    buffer_free(buff);
  }

  ty_free((VOID *)cp->host);
  ty_free((VOID *)cp->user);
  ty_free((VOID *)cp->doing);
  ty_free((VOID *)cp->outputprefix);
  ty_free((VOID *)cp->outputsuffix);

  /* blow it out of our tables. */
  FD_CLR(cp->fd, &live);
  if(topfd == cp->fd)
    topfd--;

  if(cp == connlist) {
    if(cp->next == (struct conn *)NULL)	/* only one */
      connlist = (struct conn *)NULL;
    else {
      (cp->next)->prev = connlist->prev;
      connlist = cp->next;
    }
  } else {
    (cp->prev)->next = cp->next; /* ok for tail */
    if(cp->next != (struct conn *)NULL)
      (cp->next)->prev = cp->prev;
    if(connlist->prev == cp)	/* tail? */
      connlist->prev = cp->prev;
  }

  ty_free((VOID *)cp);
}

/* shove text into the connections output queue */
static void buffer_output(cp, str)
    struct conn *cp;
    char *str;
{
  register struct buffer *buff;
  register int newline = 0, len;

  /* shouldn't ever happen, but... */
  if((str == (char *)NULL) || (str[0] == '\0'))
    return;

  len = strlen(str);
  /* if the tail is full, or there is no queue... */
  if((cp->obuff == (struct buffer *)NULL)
     || ((len > AGGREGATE_LEN) && (((cp->obuff)->prev)->full))) {
    buff = buffer_alloc();

    buff->ptr = (char *)NULL;
    buff->full = 0;

    /* link it in, it's the new tail. */
    if(cp->obuff == (struct buffer *)NULL) {
      cp->obuff = buff;
      buff->prev = buff;
    } else {
      buff->prev = (cp->obuff)->prev;
      ((cp->obuff)->prev)->next = buff;
      (cp->obuff)->prev = buff;
    }
    buff->next = (struct buffer *)NULL;
  } else
    buff = (cp->obuff)->prev;

  /* process the goo. */
  newline = ((str[len-1] == '\n') || (str[len-1] == '\r'));
  if(buff->ptr != (char *)NULL) {
    buff->ptr = (char *)ty_realloc((VOID *)buff->ptr, buff->len + len + 1,
				   "buffer_output");
    strcat(buff->ptr, str);
    buff->len += len;
  } else {
    buff->ptr = (char *)ty_malloc(len + 1, "buffer_output");
    strcpy(buff->ptr, str);
    buff->len = len;
  }
  buff->full = newline;
}

/* this is what actually reads from a descriptor. */
static void buffer_input(cp)
    struct conn *cp;
{
  /*
   * Although attributes and the like except (expect) 4k strings, we only
   * except input 1k at a time.
   */
  char input[NETINPUT_BUFFSIZ+1];
  int rd;
  register char *ptr, *optr;

  rd = read(cp->fd, input, sizeof(input)-1);
  if(rd == 0) {		/* EOF */
    cp->flags |= CONN_ERROR;
    return;
  }
  if(rd == -1) {
    if (errno != EWOULDBLOCK)
      cp->flags |= CONN_ERROR;
    return;
  }

  /*
   * In my test case, if the connection is from telnet in character
   * mode, the client will send \0 after a carriage return.
   */
  ptr = input;
  optr = input;
  while((ptr - input) < rd) {
    if((*ptr == '\n') || (*ptr == '\r')) {
      *ptr++ = '\0';

      buffer_addinput(cp, optr, 1);
      while(((*ptr == '\n') || (*ptr == '\r') || (*ptr == '\0'))
	    && (ptr - input) < rd)
        ptr++;
      optr = ptr;

      continue;
    } else if(!isprint(*ptr))
      *ptr = ' ';
    ptr++;
  }
  if(optr != ptr) {
    *ptr = '\0';

    buffer_addinput(cp, optr, (strlen(optr) == sizeof(input)-1));
  }

  cp->ibyte += rd;
}

/* add a zero-terminated, but not newline terminated string to the
   connection's input queue. */
static void buffer_addinput(cp, str, newline)
    struct conn *cp;
    char *str;
    int newline;
{
  register struct buffer *buff;
  register int len;

  if((cp->ibuff == (struct buffer *)NULL) || ((cp->ibuff)->prev)->full) {
    buff = buffer_alloc();

    buff->ptr = (char *)NULL;
    buff->full = 0;

    /* link it in, it's the new tail. */
    if(cp->ibuff == (struct buffer *)NULL) {
      cp->ibuff = buff;
      buff->prev = buff;
    } else {
      buff->prev = (cp->ibuff)->prev;
      ((cp->ibuff)->prev)->next = buff;
      (cp->ibuff)->prev = buff;
    }
    buff->next = (struct buffer *)NULL;
  } else
    buff = (cp->ibuff)->prev;

  /* process the goo. */
  len = strlen(str);
  if(buff->ptr != (char *)NULL) {
    buff->ptr = (char *)ty_realloc(buff->ptr, buff->len + len + 1,
				   "buffer_addinput");
    strcat(buff->ptr, str);
    buff->len += len;
  } else {
    buff->ptr = (char *)ty_malloc(len + 1, "buffer_addinput");
    strcpy(buff->ptr, str);
    buff->len = len;
  }
  buff->full = newline;
}

/* this is what actually writes data out to the descriptor. */
static void buffer_flush(cp)
    struct conn *cp;
{
  register int ret;
  register struct buffer *bptr, *next;
  fd_set writeset;
  struct timeval timeout;

  if(cp->flags & CONN_ERROR)
    return;

  /* argh. do a quick select()-- experience tells me that nonblocking */
  /* sockets don't always work on some operating systems. (they block) */
  FD_ZERO(&writeset);
  FD_SET(cp->fd, &writeset);
  timeout.tv_sec = 0;
  timeout.tv_usec = 800;

  ret = select(topfd+1, (fd_set *)NULL, &writeset, (fd_set *)NULL, &timeout);
  if(ret == -1) {
    logfile(LOG_ERROR, "buffer_flush: select() failed, %s\n", strerror(errno));
    return;
  }
  if((ret == 0) || !FD_ISSET(cp->fd, &writeset))
    return;

  /* go through the buffers, trying to write them out. */
  for(bptr = cp->obuff; bptr != (struct buffer *)NULL; bptr = next) {
    next = bptr->next;

    ret = write(cp->fd, bptr->ptr, bptr->len);
    if(ret != bptr->len) {	/* something happened */
      if(ret == -1) {
        if(errno != EWOULDBLOCK)
	  cp->flags |= CONN_ERROR;
	break;
      }

      if(ret > 0) {		/* partial write */
        /* i sure hope your bcopy() likes to overlap. */
        bcopy((VOID *)(bptr->ptr + ret), (VOID *)bptr->ptr, bptr->len - ret);
	bptr->len -= ret;
      } else
        /* wrote nothing? */
	break;
    } else {
      /* everything's ok. */
      buffer_free(bptr);
      bptr = (struct buffer *)NULL;
    }
  }

  /* if bptr is NULL, everything went ok. otherwise, we have a mess. */
  if(bptr == (struct buffer *)NULL) {
    /* everything has already been freed. */
    cp->obuff = (struct buffer *)NULL;
  } else {
    /* bptr should be the new top of the list. everything above it is
       already gone. */
    bptr->prev = (cp->obuff)->prev;
    cp->obuff = bptr;
  }

  cp->obyte += ret;
}

/* this is what actually does things. */
static void buffer_command(cp)
    struct conn *cp;
{
  register struct buffer *next;

  /* just handle the top command line, and drop it off the list. */
  if(cp->ibuff != (struct buffer *)NULL) {
    cp->last = mudstat.now;
    (cp->handler)(cp, (cp->ibuff)->ptr);

    /* drop it off the list. */
    if((cp->ibuff)->next)
      ((cp->ibuff)->next)->prev = (cp->ibuff)->prev;

    next = (cp->ibuff)->next;
    buffer_free(cp->ibuff);
    cp->ibuff = next;
  }
}

/* Rwho schtuff */
void rwho_login(player)
    int player;
{
  char rbuff[BUFFSIZ];
  char *pname;

  if(rwhofd != -1) {
    if(get_str_elt(player, NAME, &pname) == -1)
      pname = "???";

    snprintf(rbuff, sizeof(rbuff), "A\t%s\t%s\t%s\t#%d\t%d\t0\t%s",
	     mudconf.name, mudconf.rwho_passwd, mudconf.name, player,
	     (int)mudstat.now, pname);
    if(send(rwhofd, rbuff, strlen(rbuff), 0) == -1)
      logfile(LOG_ERROR, "rwho_login: send() failed, %s\n", strerror(errno));
  }
}

void rwho_ping()
{
  char rbuff[BUFFSIZ];

  if(rwhofd != -1) {
    snprintf(rbuff, sizeof(rbuff), "M\t%s\t%s\t%s\t%d\t0\tTeeny%s",
	     mudconf.name, mudconf.rwho_passwd, mudconf.name,
	     (int)mudstat.now, teenymud_version);
    if(send(rwhofd, rbuff, strlen(rbuff), 0) == -1)
      logfile(LOG_ERROR, "rwho_ping: send() failed, %s\n", strerror(errno));
  }
}

static struct buffer *freebuffs = (struct buffer *)NULL;

/* free a buffer struct. */
static void buffer_free(buff)
    struct buffer *buff;
{
  if(buff != (struct buffer *)NULL) {
    ty_free((VOID *)buff->ptr);

    buff->next = freebuffs;
    freebuffs = buff;
  }
}

/* allocate buffer structs in chunks. */
static struct buffer *buffer_alloc()
{
  register struct buffer *ret;

  if(freebuffs == (struct buffer *)NULL) {
    register int idx = 0;

    freebuffs = (struct buffer *)ty_malloc(sizeof(struct buffer)*256,
					   "buffer_alloc");
    while(idx < 255) {
      freebuffs[idx].next = &freebuffs[idx + 1];
      idx++;
    }
    freebuffs[idx].next = (struct buffer *)NULL;
  }

  ret = freebuffs;
  freebuffs = freebuffs->next;

  /* zero it. */
  bzero((VOID *)ret, sizeof(struct buffer));

  return(ret);
}

static char *tcp_authuser(in, local, remote)
    unsigned long in;
    unsigned short local, remote;
{
  int s, ret;
  struct sockaddr_in sa;
  fd_set writeset, readset, exceptset;
  struct timeval timeout;
  char buff[BUFFSIZ], *ptr, *eptr;
#ifndef __STDC__
  char *strstr();
#endif

  s = socket(PF_INET, SOCK_STREAM, 0);
  if(s == -1)
    return((char *)NULL);
  if (fcntl(s, F_SETFL, O_NDELAY) != 0)
    return((char *)NULL);

  sa.sin_family = AF_INET;
  sa.sin_port = htons(113);
  sa.sin_addr.s_addr = in;
  if((connect(s, (struct sockaddr *)&sa, sizeof(sa)) == -1)
     && (errno != EINPROGRESS)) {
    close(s);
    return((char *)NULL);
  }

  FD_ZERO(&writeset);
  FD_ZERO(&readset);
  FD_ZERO(&exceptset);
  FD_SET(s, &writeset);
  FD_SET(s, &readset);
  FD_SET(s, &exceptset);

  timeout.tv_sec = 5;
  timeout.tv_usec = 0;
  /* are we connected? */
  ret = select(s + 1, (fd_set *)NULL, &writeset, &exceptset, &timeout);
  if((ret == -1) || (ret == 0) || FD_ISSET(s, &exceptset)) {
    close(s);
    return((char *)NULL);
  }

  snprintf(buff, sizeof(buff), "%u , %u\r\n", (unsigned int)remote,
	   (unsigned int)local);
  ret = write(s, buff, strlen(buff));
  if(ret < strlen(buff)) {
    close(s);
    return((char *)NULL);
  }

  ret = select(s + 1, &readset, (fd_set *)NULL, &exceptset, &timeout);
  if((ret == -1) || (ret == 0) || FD_ISSET(s, &exceptset)) {
    close(s);
    return((char *)NULL);
  }
  ret = read(s, buff, sizeof(buff));
  if(((ret == -1) && (errno != EWOULDBLOCK)) || (ret == 0)) {
    close(s);
    return((char *)NULL);
  }

  ptr = strstr(buff, "USERID");
  if(ptr != (char *)NULL) {
    register int i;

    for(i = 0; i < 2; i++) {
      /* skip word */
      while(*ptr && ((ptr - buff) < ret) && !isspace(*ptr))
        ptr++;
      /* skip space */
      while(*ptr && ((ptr - buff) < ret) && isspace(*ptr))
        ptr++;
      /* skip colon */
      while(*ptr && ((ptr - buff) < ret) && !isspace(*ptr))
        ptr++;
      /* skip space */
      while(*ptr && ((ptr - buff) < ret) && isspace(*ptr))
        ptr++;
    }
    /* find the end */
    for(eptr = ptr; *eptr && ((eptr - buff) < ret) && !isspace(*eptr); eptr++);
    *eptr++ = '\0';

    close(s);
    return(ty_strdup(ptr, "tcp_authuser"));
  }

  close(s);
  return((char *)NULL);
}

static char *conn_crnlmodes[] = {
  "\r\n", "\r", "\n", "\n\r"
};

/* high level ouput routines. */
int tcp_notify(player, str)
    int player;
    char *str;
{
  register struct conn *cp;
  register int ret = 0;

  for(cp = connlist; cp != (struct conn *)NULL; cp = cp->next) {
    if(cp->player == player) {
      if((*str != '\0') && (cp->lwrap > 0)) {		/* line wrap! */
        register char *sptr, *eptr, *svptr;
	char sv;

	sptr = (char *)alloca(strlen(str)+1);
	if(sptr == (char *)NULL)
	  panic("tcp_notify: stack allocation failed.\n");
	strcpy(sptr, str);
	
	while(*sptr != '\0') {
	  eptr = sptr;
	  sv = '\0';

	  while(((eptr - sptr) < cp->lwrap) && (*eptr != '\0'))
	    eptr++;
	  if(*eptr != '\0') {
	    svptr = eptr;
	    while((eptr >= sptr) && !isspace(*eptr))
	      eptr--;
	    if((eptr == sptr) || !isspace(*eptr)) {	/* no spaces */
	      eptr = svptr;
	      sv = *eptr;
	      *eptr = '\0';
	    } else 					/* space */
	      *eptr++ = '\0';
	  }

	  buffer_output(cp, sptr);
	  buffer_output(cp, conn_crnlmodes[cp->lcrnl]);

	  if(sv)
	    *eptr = sv;
	  sptr = eptr;
	}
      } else {
        buffer_output(cp, str);
        buffer_output(cp, conn_crnlmodes[cp->lcrnl]);
      }

      ret++;
    }
  }
  return(ret);
}

int tcp_wall(flags, str, except)
    int *flags;
    char *str;
    int except;
{
  register struct conn *cp;
  register int ret = 0;
  int pflags[FLAGS_LEN];

  for(cp = connlist; cp != (struct conn *)NULL; cp = cp->next) {
    if((cp->player == -1) || (cp->player == except))
      continue;
    if(get_flags_elt(cp->player, FLAGS, pflags) == -1)
      continue;
    if(((flags[0] != 0) && !(pflags[0] & flags[0])) ||
       ((flags[1] != 0) && !(pflags[1] & flags[1])))
      continue;

    if((*str != '\0') && (cp->lwrap > 0)) {		/* line wrap! */
      register char *sptr, *eptr, *svptr;
      char sv;

      sptr = (char *)alloca(strlen(str)+1);
      if(sptr == (char *)NULL)
	panic("tcp_wall: stack allocation failed.\n");
      strcpy(sptr, str);
	
      while(*sptr != '\0') {
	eptr = sptr;
	sv = '\0';

	while(((eptr - sptr) < cp->lwrap) && (*eptr != '\0'))
	  eptr++;
	if(*eptr != '\0') {
	  svptr = eptr;
	  while((eptr >= sptr) && !isspace(*eptr))
	    eptr--;
	  if((eptr == sptr) || !isspace(*eptr)) {	/* no spaces */
	    eptr = svptr;
	    sv = *eptr;
	    *eptr = '\0';
	  } else 					/* space */
	    *eptr++ = '\0';
	}

	buffer_output(cp, sptr);
	buffer_output(cp, conn_crnlmodes[cp->lcrnl]);

	if(sv)
	  *eptr = sv;
	sptr = eptr;
      }
    } else {
      buffer_output(cp, str);
      buffer_output(cp, conn_crnlmodes[cp->lcrnl]);
    }

    ret++;
  }
  return(ret);
}

int conn_put(cp, str)
    struct conn *cp;
    char *str;
{
  if(cp != (struct conn *)NULL) {
    if((*str != '\0') && (cp->lwrap > 0)) {		/* line wrap! */
      register char *sptr, *eptr, *svptr;
      char sv;

      sptr = (char *)alloca(strlen(str)+1);
      if(sptr == (char *)NULL)
	panic("conn_put: stack allocation failed.\n");
      strcpy(sptr, str);
	
      while(*sptr != '\0') {
	eptr = sptr;
	sv = '\0';

	while(((eptr - sptr) < cp->lwrap) && (*eptr != '\0'))
	  eptr++;
	if(*eptr != '\0') {
	  svptr = eptr;
	  while((eptr >= sptr) && !isspace(*eptr))
	    eptr--;
	  if((eptr == sptr) || !isspace(*eptr)) {	/* no spaces */
	    eptr = svptr;
	    sv = *eptr;
	    *eptr = '\0';
	  } else 					/* space */
	    *eptr++ = '\0';
	}

	buffer_output(cp, sptr);
	buffer_output(cp, conn_crnlmodes[cp->lcrnl]);

	if(sv)
	  *eptr = sv;
	sptr = eptr;
      }
    } else {
      buffer_output(cp, str);
      buffer_output(cp, conn_crnlmodes[cp->lcrnl]);
    }

    return(1);
  }
  return(0);
}

int tcp_logoff(who)
    int who;
{
  register struct conn *cp;
  int total = 0;

  for(cp = connlist; cp != (struct conn *)NULL; cp = cp->next) {
    if(cp->player == who) {
      cp->flags |= CONN_KILL;
      total++;
    }
  }
  return(total);
}

int fd_logoff(who)
    int who;
{
  register struct conn *cp;
  int total = 0;

  for(cp = connlist; cp != (struct conn *)NULL; cp = cp->next) {
    if(cp->fd == who) {
      cp->flags |= CONN_KILL;
      total++;
    }
  }
  return(total);
}

int conn_logoff(cp)
    struct conn *cp;
{
  if(cp != (struct conn *)NULL) {
    cp->flags |= CONN_KILL;
    return(1);
  }
  return(0);
}