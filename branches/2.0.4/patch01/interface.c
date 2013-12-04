/* interface.c */

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
 * the Free Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,
 * MA 02111-1307, USA.
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
#ifdef HAVE_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif			/* HAVE_STRING_H */
#include <ctype.h>
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#else
#include <time.h>
#endif			/* HAVE_SYS_TIME_H */
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif			/* HAVE_STDLIB_H */

#include "conf.h"
#include "teeny.h"
#include "commands.h"
#include "fcache.h"
#include "conn.h"
#include "externs.h"

enum who_code { LIST_WHO, LIST_WHERE, LIST_SESSION };
enum str_code { PREFIX, SUFFIX };

static char poll_str[41];		/* the poll */

static void conn_updsite _ANSI_ARGS_((struct conn *));
static void set_output_str _ANSI_ARGS_((struct conn *, char *, enum str_code));
static void do_wholine _ANSI_ARGS_((struct conn *, struct conn *,
				    char *, enum who_code, int, int *, int *));
static void do_wholist _ANSI_ARGS_((struct conn *, char *, enum who_code));
static void do_stty _ANSI_ARGS_((struct conn *, char *));
static void log_create _ANSI_ARGS_((struct conn *));
static void log_badconnect _ANSI_ARGS_((struct conn *, char *));
static void log_badcreate _ANSI_ARGS_((struct conn *, char *));

void interface_init()
{
  time_t now;

  time(&now);

  mudstat.now = now;
  mudstat.lastdump = now;
  mudstat.startup = now;
  mudstat.lasttime = now;
  poll_str[0] = '\0';
}

void timers()
{
  register struct conn *cp;

  /* run the queue */
  queue_run(mudconf.queue_slice);

  /* timeout connections. */
  if (mudstat.now > mudstat.lasttime) {
    for(cp = connlist; cp != (struct conn *)NULL; cp = cp->next) {
      if((cp->player == -1) && mudconf.login_timeout) {
	if((cp->last - mudstat.now) > mudconf.login_timeout) {
	  conn_dump_file(cp, mudconf.timeout_file);
	  conn_logoff(cp);
	}
      }
      if((cp->player != -1) && mudconf.player_timeout
         && !isWIZARD(cp->player)) {
        if((cp->last - mudstat.now) > mudconf.player_timeout) {
	  conn_dump_file(cp, mudconf.timeout_file);
	  conn_logoff(cp);
	}
      }

      cp->cc = 0;
    }
  }

  /* dump the database. */
  if ((mudstat.now - mudstat.lastdump) > mudconf.dump_interval) {
    mudstat.lastdump = mudstat.now;
    dump_db();
  }

  /* ping rwho */
  if (mudconf.enable_rwho
      && ((mudstat.now - mudstat.lastrwho) > mudconf.rwho_interval)) {
    mudstat.lastrwho = mudstat.now;
    rwho_ping();
  }

  mudstat.lasttime = mudstat.now;
}

#define SKIP_WORD(x, y)		for(y = x; y[0] && !isspace(y[0]); y++); \
				for(; y[0] && isspace(y[0]); y++)

VOID handle_input(cp, input)
    struct conn *cp;
    char *input;
{
  switch(input[0]) {
  case 'W':
  case 'w':	/* WHO or WHERE */
    if(!strncasecmp(input, "WHO", 3)) {
      if(cp->outputprefix != (char *)NULL)
	conn_put(cp, cp->outputprefix);
      do_wholist(cp, input + 3, LIST_WHO);
      if(cp->outputsuffix != (char *)NULL)
	conn_put(cp, cp->outputsuffix);
      return;
    } else if(!strncasecmp(input, "WHERE", 5)) {
      if(cp->outputprefix != (char *)NULL)
	conn_put(cp, cp->outputprefix);
      do_wholist(cp, input + 5, LIST_WHERE);
      if(cp->outputsuffix != (char *)NULL)
	conn_put(cp, cp->outputsuffix);
      return;
    }
    break;
  case 'S':
  case 's':	/* SESSION or STTY */
    if(!strncasecmp(input, "SESSION", 7)) {
      if(cp->outputprefix != (char *)NULL)
	conn_put(cp, cp->outputprefix);
      do_wholist(cp, input + 7, LIST_SESSION);
      if(cp->outputsuffix != (char *)NULL)
	conn_put(cp, cp->outputsuffix);
      return;
    } else if(!strncasecmp(input, "STTY", 4)) {
      if(cp->outputprefix != (char *)NULL)
        conn_put(cp, cp->outputprefix);
      do_stty(cp, input + 4);
      if(cp->outputsuffix != (char *)NULL)
        conn_put(cp, cp->outputsuffix);
      return;
    }
    break;
  case 'Q':
  case 'q':
    if(!strcasecmp(input, "QUIT")) {
      conn_dump_file(cp, mudconf.bye_file);
      conn_logoff(cp);
      return;
    }
    break;
  case 'O':	/* OUPUT*X */
    if(!strncmp(input, "OUTPUTPREFIX", 12)) {
      if(isROBOT(cp->player)) {
        set_output_str(cp, input + 12, PREFIX);
      } else {
        conn_put(cp, "Only robots may use OUTPUTPREFIX.");
      }
      return;
    } else if(!strncmp(input, "OUTPUTSUFFIX", 12)) {
      if(isROBOT(cp->player)) {
        set_output_str(cp, input + 12, SUFFIX);
      } else {
        conn_put(cp, "Only robots may use OUTPUTSUFFIX.");
      }
      return;
    }
    break;
  }

  /* still here? */
  if(cp->outputprefix != (char *)NULL)
    conn_put(cp, cp->outputprefix);
  handle_cmd(cp->player, cp->player, input, 0, (char **)NULL);
  if(cp->outputsuffix != (char *)NULL)
    conn_put(cp, cp->outputsuffix);
}

VOID handle_login(cp, input)
    struct conn *cp;
    char *input;
{
  char *ptr, *name, *pwd, *mesg;
  int player;

  switch(input[0]) {
  case 'W':
  case 'w':	/* WHO or WHERE */
    if(!strncasecmp(input, "WHO", 3)) {
      if(cp->outputprefix != (char *)NULL)
	conn_put(cp, cp->outputprefix);
      do_wholist(cp, input + 3, LIST_WHO);
      if(cp->outputsuffix != (char *)NULL)
	conn_put(cp, cp->outputsuffix);
    } else if(!strncasecmp(input, "WHERE", 5)) {
      if(cp->outputprefix != (char *)NULL)
	conn_put(cp, cp->outputprefix);
      do_wholist(cp, input + 5, LIST_WHERE);
      if(cp->outputsuffix != (char *)NULL)
	conn_put(cp, cp->outputsuffix);
    }
    return;
  case 'S':
  case 's':	/* SESSION or STTY */
    if(!strncasecmp(input, "SESSION", 7)) {
      if(cp->outputprefix != (char *)NULL)
	conn_put(cp, cp->outputprefix);
      do_wholist(cp, input + 7, LIST_SESSION);
      if(cp->outputsuffix != (char *)NULL)
	conn_put(cp, cp->outputsuffix);
    } else if(!strncasecmp(input, "STTY", 4)) {
      if(cp->outputprefix != (char *)NULL)
        conn_put(cp, cp->outputprefix);
      do_stty(cp, input + 4);
      if(cp->outputsuffix != (char *)NULL)
        conn_put(cp, cp->outputsuffix);
    }
    return;
  case 'Q':
  case 'q':	/* QUIT */
    if(!strcasecmp(input, "QUIT"))
      conn_logoff(cp);
    return;
  case 'O':	/* OUTPUT*X */
    if(!strncmp(input, "OUTPUTPREFIX", 12))
      set_output_str(cp, input + 12, PREFIX);
    else if(!strncmp(input, "OUTPUTSUFFIX", 12))
      set_output_str(cp, input + 12, SUFFIX);
    return;
  }

  /* still here? */
  if(!strncasecmp(input, "co", 2)) {
    /* login attempt */
    SKIP_WORD(input, ptr);

    if(parse_name_pwd(ptr, &name, &pwd) != 0)
      conn_put(cp, "Error parsing connection sequence.");
    else {
      player = connect_player(name, pwd);
      if(player == -1) {
	conn_dump_file(cp, mudconf.badconn_file);
	log_badconnect(cp, name);
      } else {
	char *hatr;
	int haflgs;

	if((mudstat.logins != LOGINS_DISABLED) || isWIZARD(player)) {
          announce_connect(player, cp->user, cp->host);	/* first */
	  animate(player);
	  cp->player = player;
	  cp->handler = handle_input;
	  log_connect(cp);
	  if(mudconf.enable_rwho)
	    rwho_login(player);

	  if(!isGUEST(player)) {
	    if(attr_get(player, SITE, &hatr, &haflgs) != -1) {
	      if((hatr == (char *)NULL) || (hatr[0] == '\0'))
	        conn_dump_file(cp, mudconf.newp_file);
	    } else
	      logfile(LOG_ERROR,
		      "handle_login: couldn't get host on player #%d\n",
		      player);
	    conn_dump_file(cp, mudconf.motd_file);
	    if(mudstat.exmotd != (char *)NULL)
	      conn_put(cp, mudstat.exmotd);
	    if(isWIZARD(player)) {
	      conn_dump_file(cp, mudconf.wizn_file);
	      if(mudstat.exwiz != (char *)NULL)
	        conn_put(cp, mudstat.exwiz);
	    }
	  } else
	    conn_dump_file(cp, mudconf.guest_file);
	  check_last(player);
	  check_news(player);
	  if(mudconf.enable_money)
	    check_paycheck(player);

	  conn_updsite(cp);
	  stamp(player, STAMP_LOGIN);
	  if(!isROBOT(player)
	     && ((cp->outputprefix != (char *)NULL)
	 	  || (cp->outputsuffix != (char *)NULL))) {
	    conn_put(cp,
		     "Your OUTPUTREFIX and OUTPUTSUFFIX strings have been cleared.");
	    if(cp->outputprefix != (char *)NULL) {
	      ty_free((VOID *)cp->outputprefix);
	      cp->outputprefix = (char *)NULL;
	    }
	    if(cp->outputsuffix != (char *)NULL) {
	      ty_free((VOID *)cp->outputsuffix);
	      cp->outputsuffix = (char *)NULL;
	    }
	  }
	  stty_restore(cp);
	  look_location(player, player, 0);
	} else
	  conn_put(cp, "Sorry, logins are disabled.");
      }
    }
  } else if(!strncasecmp(input, "cr", 2)) {
    /* creation attempt */
    SKIP_WORD(input, ptr);

    if(mudstat.logins != LOGINS_DISABLED) {
      if(!mudconf.registration) {
        if(check_host(cp->host, LOCK_REGISTER, &mesg)) {
	  if(mesg && *mesg) {
	    conn_put(cp, mesg);
	  } else {
	    conn_dump_file(cp, mudconf.register_file);
	  }
	  return;
        }
        if(parse_name_pwd(ptr, &name, &pwd) != 0)
	  conn_put(cp, "Error parsing creation sequence.");
        else {
	  player = create_player(name, pwd, -1);
	  if(player == -1) {
	    conn_dump_file(cp, mudconf.badcrt_file);
	    log_badcreate(cp, name);
	  } else {
	    announce_connect(player, cp->user, cp->host);	/* first */
	    animate(player);
	    cp->player = player;
	    cp->handler = handle_input;
	    log_create(cp);
	    if(mudconf.enable_rwho)
	      rwho_login(player);

	    conn_dump_file(cp, mudconf.newp_file);
	    conn_dump_file(cp, mudconf.motd_file);
	    if(mudstat.exmotd != (char *)NULL)
	      conn_put(cp, mudstat.exmotd);
	    check_news(player);
	    conn_updsite(cp);
	    stamp(player, STAMP_LOGIN);
	    if(!isROBOT(player)
	       && ((cp->outputprefix != (char *)NULL)
		   || (cp->outputsuffix != (char *)NULL))) {
	      conn_put(cp,
		       "Your OUTPUTPREFIX and OUTPUTSUFFIX strings have been cleared.");
	      if(cp->outputprefix != (char *)NULL) {
	        ty_free((VOID *)cp->outputprefix);
	        cp->outputprefix = (char *)NULL;
	      }
	      if(cp->outputsuffix != (char *)NULL) {
	        ty_free((VOID *)cp->outputsuffix);
	        cp->outputsuffix = (char *)NULL;
	      }
	    }
	    stty_restore(cp);
	    look_location(player, player, 0);
	  }
        }
      } else
        conn_dump_file(cp, mudconf.register_file);
    } else
      conn_put(cp, "Sorry, logins are disabled.");
  } else
    conn_greet(cp);
}

static void conn_updsite(cp)
    struct conn *cp;
{
  if(cp->player == -1)
    return;

  if(cp->user != (char *)NULL) {
    char *buff = (char *)alloca(strlen(cp->user) + strlen(cp->host) + 2);

    if(buff == (char *)NULL) {
      panic("conn_updsite: stack allocation failed.\n");
    }
    strcpy(buff, cp->user);
    strcat(buff, "@");
    strcat(buff, cp->host);

    if(attr_add(cp->player, SITE, buff, SITE_FLGS) == -1)
      logfile(LOG_ERROR,
              "conn_updsite: couldn't set host on player #%d\n", cp->player);
  } else if((cp->host != (char *)NULL) && (cp->host[0] != '\0')) {
    if(attr_add(cp->player, SITE, cp->host, SITE_FLGS) == -1)
      logfile(LOG_ERROR,
              "conn_updsite: couldn't set host on player #%d\n", cp->player);
  }
}

static void set_output_str(cp, string, code)
    struct conn *cp;
    char *string;
    enum str_code code;
{
  while (string[0] && isspace(string[0]))
    string++;

  switch(code) {
  case PREFIX:
    if (!string || !*string) {
      ty_free((VOID *)cp->outputprefix);
      cp->outputprefix = (char *)NULL;
    } else {
      ty_free((VOID *)cp->outputprefix);
      cp->outputprefix = (char *)ty_strdup(string,
      					"set_output_str.outputprefix");
    }
    break;
  case SUFFIX:
    if (!string || !*string) {
      ty_free((VOID *)cp->outputsuffix);
      cp->outputsuffix = (char *)NULL;
    } else {
      ty_free((VOID *)cp->outputsuffix);
      cp->outputsuffix = (char *)ty_strdup(string,
      					"set_output_str.outputprefix");
    }
    break;
  }
}

void conn_greet(cp)
    struct conn *cp;
{
  if (cp->player == -1)
    conn_dump_file(cp, mudconf.greet_file);
}

int conn_dump_file(cp, filename)
    struct conn *cp;
    char *filename;
{
  FCACHE *fc;
  off_t size;

  /* problem: fcache'd files of zero lenght return NULL. */
  fc = fcache_open(filename, &size);
  if ((fc == (FCACHE *)NULL) && (size == -1)) {
    char buff[256];

    snprintf(buff, sizeof(buff), "I'm sorry, %s seems to be broken.",
	     filename);
    conn_put(cp, buff);
    return(-1);
  }

  while(fc != (FCACHE *)NULL) {
    conn_put(cp, fcache_read(fc));

    fc = fcache_next(fc);
  }
  return(1);
}

/* Display the WHO list to a connection. */
INLINE static char *convtime1(wtime)
    time_t wtime;
{
  register time_t g, h, i, j;
  static char ret[40];

  g = wtime;
  h = g / 86400;
  g %= 86400;
  i = g / 3600;
  j = g / 60 % 60;

  if (h)
    snprintf(ret, sizeof(ret), "%ldd %02ld:%02ld", h, i, j);
  else
    snprintf(ret, sizeof(ret), "%02ld:%02ld", i, j);
  return (ret);
}

INLINE static char *convtime2(idtime)
    time_t idtime;
{
  register time_t k;
  register char idletype;
  static char ret[40];

  k = idtime;
  idletype = 's';
  if (k > 59) {
    idletype = 'm';
    k /= 60;
    if (k > 59) {
      idletype = 'h';
      k /= 60;
      if (k > 23) {
	idletype = 'd';
	k /= 24;
      }
    }
  }
  snprintf(ret, sizeof(ret), "%2ld%c", k, idletype);
  return (ret);
}

static void do_wholine(cp, who, arg, code, wizard, total, utotal)
    register struct conn *cp, *who;
    char *arg;
    enum who_code code;
    int wizard, *total, *utotal;
{
  char *whonm;
  char line[256], namebuf[40], locbuf[60];
  int loc;

  if (wizard || ((who->player != -1) && (!isDARK(who->player)))) {
    if(who->player != -1) {
      (*total)++;

      if (get_str_elt(who->player, NAME, &whonm) != -1)
	strcpy(namebuf, whonm);
      else
	strcpy(namebuf, "???");
      if (strlen(arg) && !stringprefix(namebuf, arg))
	return;
      if (wizard) {
	snprintf(&namebuf[strlen(namebuf)], sizeof(namebuf) - strlen(namebuf),
		 "(#%d%s%s%s%s)", who->player,
		 (isWIZARD(who->player) ? "W" : ""),
		 (isDARK(who->player) ? "D" : ""),
		 (isHIDDEN(who->player) ? "h" : ""),
		 (isGUEST(who->player) ? "g" : ""));
      }
    } else {
      (*utotal)++;
      strcpy(namebuf, "***");
    }

    switch (code) {
    case LIST_WHO:
      snprintf(line, sizeof(line), "%-19s%9s%7s %s", namebuf,
	      convtime1(mudstat.now - who->connect),
	      convtime2(mudstat.now - who->last),
	      (who->doing != (char *)NULL) ? who->doing : "");
      break;
    case LIST_WHERE:
      if ((who->player == -1) ||
	  (get_int_elt(who->player, LOC, &loc) == -1) || !exists_object(loc))
	loc = -1;
      if (!wizard) {
	if (!isHIDDEN(who->player) && (loc != -1) && !isHIDDEN(loc) &&
	    (get_str_elt(loc, NAME, &whonm) != -1)) {
	  ty_strncpy(locbuf, whonm, 50);
	} else
	  strcpy(locbuf, "???");
	if ((strlen(locbuf) < 45) && exists_object(loc) && (isJUMP_OK(loc) ||
	    ((cp->player != -1) && controls(cp->player, cp->player, loc)))) {
	  snprintf(&locbuf[strlen(locbuf)], sizeof(locbuf) - strlen(locbuf),
		   "(#%d)", loc);
	}
	snprintf(line, sizeof(line), "%-20s%s", namebuf, locbuf);
      } else {
	if (loc == -1) {
	  strcpy(locbuf, "???");
	} else {
	  snprintf(locbuf, sizeof(locbuf), "(#%d)", loc);
	}

	if (who->user != (char *)NULL) {
	  snprintf(line, sizeof(line), "%-20s%8s  %-2d %-4d %s@%s", namebuf,
	  	  locbuf, who->fd, who->port, who->user, who->host);
	} else {
	  snprintf(line, sizeof(line), "%-20s%8s  %-2d %-4d %s", namebuf,
		   locbuf, who->fd, who->port, who->host);
	}
      }
      break;
    case LIST_SESSION:
      snprintf(line, sizeof(line), "%-20s %-10d %-15d %-15d", namebuf,
	      who->bcc, who->ibyte, who->obyte);
      break;
    }
    conn_put(cp, line);
  }
}

static void do_wholist(cp, arg, code)
    struct conn *cp;
    char *arg;
    enum who_code code;
{
  register struct conn *who;
  int total = 0, utotal = 0;
  int wizard = 0;
  int reverse = 0;
  char line[128];

  while (isspace(arg[0]))
    arg++;

  if (mudconf.wizwhoall || ((cp->player != -1) && isWIZARD(cp->player)))
    wizard = 1;
  if((cp->player != -1) && isREVERSED_WHO(cp->player))
    reverse = 1;

  if(!wizard && (code == LIST_SESSION))
    code = LIST_WHO;

  switch(code) {
  case LIST_WHO:
    snprintf(line, sizeof(line),
    	     "Player Name           On For   Idle %s", 
	     (poll_str[0] ? poll_str : "Doing"));
    break;
  case LIST_WHERE:
    strcpy(line, "Player Name         Location");
    if(wizard)
      strcat(line, "  Fd Port User/Site");
    break;
  case LIST_SESSION:
    strcpy(line,
    	"Player Name          Blown      Input Count     Output Count");
    break;
  }
  conn_put(cp, line);

  /* dump the list. */
  if(reverse) {
    who = connlist->prev;
    do {
      do_wholine(cp, who, arg, code, wizard, &total, &utotal);

      who = who->prev;
    } while(who != connlist->prev);
  } else {
    for(who = connlist; who != (struct conn *)NULL; who = who->next)
      do_wholine(cp, who, arg, code, wizard, &total, &utotal);
  }

  if (utotal) {
    snprintf(line, sizeof(line),
	     "There are %d connections to the twilight zone.", utotal);
    conn_put(cp, line);
  }
  if (total) {
    snprintf(line, sizeof(line), "%d user%s %s connected.  %sUptime: %s", total,
	     ((total == 1) ? "" : "s"), ((total == 1) ? "is" : "are"),
	     ((mudstat.logins == LOGINS_DISABLED) ? "Logins disabled.  " : ""),
	     convtime1(mudstat.now - mudstat.startup));
  } else {
    snprintf(line, sizeof(line), "No users are connected. %sUptime: %s",
    	     ((mudstat.logins == LOGINS_DISABLED) ? "Logins disabled.  " : ""),
	     convtime1(mudstat.now - mudstat.startup));
  }
  conn_put(cp, line);
}

/* Count how many people are logged in from the host, or all. */
int count_logins(host)
    char *host;
{
  register int count = 0;
  register struct conn *who;

  for(who = connlist; who != (struct conn *)NULL; who = who->next) {
    if((host == (char *)NULL) || quick_wild(who->host, host))
      count++;
  }
  return(count);
}

/* IMPORTANT: These all must be in the same order! */
const char *const stty_nlmodekey[] = {"crnl", "cr", "nl", "nlcr"};
const int stty_nlmodeval[] = {CONN_CRNL, CONN_CR, CONN_NL, CONN_NLCR};
const char *const stty_nlmodes[] = {"\r\n", "\r", "\n", "\n\r"};
const char *const stty_nltab[] = {"CONN_CRNL", "CONN_CR", "CONN_NL",
				  "CONN_NLCR" };

static void do_stty(cp, input)
    struct conn *cp;
    char *input;
{
  int i;
  char *ptr;
  char buf[BUFFSIZ];

  while((*input != '\0') && isspace(*input))
    input++;

  switch(*input) {
  case 'N':
  case 'n':
    if(!strncasecmp(input, "nlmode", 6)) {
      while((*input != '\0') && isspace(*input))
        input++;
      if(*input == '\0')
        break;

      for(i = 0; i < 4; i++) {
        if(!strcasecmp(input, stty_nlmodekey[i])) {
	  cp->lcrnl = stty_nlmodeval[i];
	  conn_put(cp, "Newline mode changed.");
	  return;
	}
      }
    }
    break;
  case 'W':
  case 'w':
    if(!strncasecmp(input, "wrap", 4)) {
      while((*input != '\0') && !isspace(*input))
        input++;
      if(*input == '\0')
        break;

      i = (int)strtol(input, &ptr, 10);
      if(ptr == input)
        break;
      if(i < 0)
        i = 0;
      cp->lwrap = i;
      if(i == 0)
        conn_put(cp, "Line wrap disabled.");
      else
        conn_put(cp, "Line wrap enabled.");
      return;
    }
    break;
  }

  conn_put(cp, "Current stty settings:");
  if(cp->lwrap)
    snprintf(buf, sizeof(buf), "Line wrap: %d  Newline mode: ", cp->lwrap);
  else
    strcpy(buf, "Line wrap: *DISABLED*  Newline mode: ");
  strcat(buf, stty_nlmodekey[cp->lcrnl]);
  conn_put(cp, buf);
}

void log_disconnect(cp)
    struct conn *cp;
{
  char *name;

  if (cp->player != -1) {
    if (get_str_elt(cp->player, NAME, &name) != -1) {
      logfile(LOG_STATUS, "DISCONNECT: %s(#%d) on fd %d.\n", name, cp->player,
	      cp->fd);
    } else
      logfile(LOG_STATUS, "DISCONNECT: ???(#%d) on fd %d.\n", cp->player,
	      cp->fd);
  } else
    logfile(LOG_STATUS, "DISCONNECT: fd %d never connected.\n", cp->fd);
}

void log_connect(cp)
    struct conn *cp;
{
  char *name;

  if (cp->player != -1) {
    if (get_str_elt(cp->player, NAME, &name) != -1) {
      logfile(LOG_STATUS, "CONNECTED: %s(#%d) on fd %d.\n", name, cp->player,
	      cp->fd);
    } else
      logfile(LOG_STATUS, "CONNECTED: ???(#%d) on fd %d.\n", cp->player,
	      cp->fd);
  } else {
    if(cp->user != (char *)NULL) {
      logfile(LOG_STATUS,
	      "CONNECTION: fd %d connected from [%s@%s], remort port %d.\n",
      	      cp->fd, cp->user, cp->host, cp->port);
    } else { 
      logfile(LOG_STATUS,
	      "CONNECTION: fd %d connected from [%s], remote port %d.\n",
	      cp->fd, cp->host, cp->port);
    }
  }
}

static void log_create(cp)
    struct conn *cp;
{
  char *name;

  if (get_str_elt(cp->player, NAME, &name) != -1) {
    logfile(LOG_STATUS, "CREATED: %s(#%d) on fd %d.\n", name, cp->player,
	    cp->fd);
  } else
    logfile(LOG_STATUS, "CREATED: ???(#%d) on fd %d.\n", cp->player, cp->fd);
}

static void log_badconnect(cp, s)
    struct conn *cp;
    char *s;
{
  logfile(LOG_STATUS, "FAILED CONNECT: fd %d failed connection to \"%s\".\n",
          cp->fd, s);
}

static void log_badcreate(cp, s)
    struct conn *cp;
    char *s;
{
  logfile(LOG_STATUS, "FAILED CREATE: fd %d failed to create player \"%s\".\n",
          cp->fd, s);
}

int numconnected(player)
    int player;
{
  register struct conn *cp;
  register int ret = 0;

  for (cp = connlist; cp != (struct conn *) NULL; cp = cp->next) {
    if ((cp->player != -1) && (cp->player == player))
      ret++;
  }
  return (ret);
}

int fd_conn(player)
    int player;
{
  register struct conn *cp;

  for (cp = connlist; cp != (struct conn *) NULL; cp = cp->next) {
    if ((cp->player != -1) && (cp->player == player))
      return (cp->fd);
  }
  return (-1);
}

int match_active_player(arg)
    char *arg;
{
  register struct conn *cp;
  int number = 0;
  int player = -1;
  int aflags;
  char *name, *alias;

  for (cp = connlist; cp != (struct conn *) NULL; cp = cp->next) {
    if(cp->player == -1)
      continue;

    if (get_str_elt(cp->player, NAME, &name) == -1) {
      logfile(LOG_ERROR, "match_active_player: bad player name reference #%d\n",
	      cp->player);
      return (-1);
    }
    if (attr_get(cp->player, ALIAS, &alias, &aflags) == -1) {
      logfile(LOG_ERROR, "match_active_player: bad player alias ref #%d\n",
	      cp->player);
      return(-1);
    }

    if (arg[0] && stringprefix(name, arg)) {
      if (strlen(arg) == strlen(name))
	return (cp->player);
      number++;
      if (number == 1) {
	player = cp->player;
      } else {
	if (cp->player != player)
	  return (-2);
	else
	  number = 1;
      }
    }
    if (arg[0] && (alias != (char *)NULL) && alias[0]
	&& stringprefix(alias, arg)) {
      if (strlen(arg) == strlen(name))
	return (cp->player);
      number++;
      if (number == 1) {
	player = cp->player;
      } else {
	if (cp->player != player)
	  return (-2);
	else
	  number = 1;
      }
    }
  }
  if (number)
    return (player);
  else
    return (-1);
}

VOID do_doing(player, cause, switches, argone, argtwo)
    int player, cause, switches;
    char *argone, *argtwo;
{
  struct conn *cp;
  register char *ptr;
  int person;

  if ((argtwo != (char *)NULL) && argtwo[0]) {
    person = resolve_player(player, cause, argone,
    			    (!(switches & CMD_QUIET) ? RSLV_NOISY : 0));
    if(person == -1)
      return;
    if (!controls(player, cause, person)) {
      if(!(switches & CMD_QUIET))
        notify_player(player, cause, player, "You can't do that.", NOT_QUIET);
      return;
    }
  } else {
    person = player;
    argtwo = argone;
  }
  if (!isALIVE(person)) {	/* @force won't crash me */
    if(!(switches & CMD_QUIET))
      notify_player(player, cause, player, "That person is not connected.",
      		    NOT_QUIET);
    return;
  }

  if (argtwo && *argtwo && (strlen(argtwo) > (sizeof(poll_str)-1)))
    argtwo[sizeof(poll_str)] = '\0';

  for (cp = connlist; cp != (struct conn *) NULL; cp = cp->next) {
    if ((cp->player == -1) || (cp->player != person))
      continue;
    if (cp->doing != (char *)NULL) {
      ty_free((VOID *)cp->doing);
      cp->doing = (char *) NULL;
    }
    if (argtwo && *argtwo)
      cp->doing = (char *) ty_strdup(argtwo, "do_doing.doing");
    if (cp->doing != (char *)NULL) {
      for(ptr = cp->doing; *ptr != '\0'; ptr++) {
        if(!isprint(*ptr))
	  *ptr = ' ';
      }
    }
  }

  if(!(switches & CMD_QUIET))
    notify_player(player, cause, player,
  		  ((person == player) ? "Set.  Have fun doing it." : "Set."),
		  NOT_QUIET);
}

VOID do_poll(player, cause, switches, argone)
    int player, cause, switches;
    char *argone;
{
  if ((argone == (char *)NULL) || (argone[0] == '\0'))
    argone = "Doing";

  if (strlen(argone) > (sizeof(poll_str)-1))
    argone[sizeof(poll_str)-1] = '\0';
  strcpy(poll_str, argone);

  if(!(switches & CMD_QUIET))
    notify_player(player, cause, player, "Poll set.", NOT_QUIET);
}

void stty_restore(cp)
    struct conn *cp;
{
  int aflags;
  char *stty, *ptr;

  if((cp->player != -1) && exists_object(cp->player) && isPLAYER(cp->player)) {
    if(attr_get(cp->player, STTY, &stty, &aflags) != -1) {
      if(stty && *stty) {
        cp->lwrap = (short)strtol(stty, &ptr, 10);
	if(*ptr != ':') {
	  cp->lwrap = 0;
	  return;
	} else {
	  register int idx;

	  ptr++;
	  for(idx = 0; idx < 4; idx++) {
	    if(strncmp(stty_nltab[idx], ptr, strlen(stty_nltab[idx])) == 0) {
	      cp->lcrnl = idx;
	      break;
	    }
	  }
	}
	conn_put(cp, "Stty settings restored.");
      }
    }
  }
}

VOID do_savestty(player, cause, switches, argone)
    int player, cause, switches;
    char *argone;
{
  struct conn *cp;
  char buf[40];
  int person;

  if((argone != (char *)NULL) && argone[0]) {
    person = resolve_player(player, cause, argone,
    			    (!(switches & CMD_QUIET) ? RSLV_NOISY : 0));
    if(person == -1)
      return;
  } else
    person = player;

  if (!controls(player, cause, person)) {
    if(!(switches & CMD_QUIET))
      notify_player(player, cause, player, "You can't do that.", NOT_QUIET);
    return;
  }

  if (switches & SAVESTTY_RESTORE) {
    for (cp = connlist; cp != (struct conn *) NULL; cp = cp->next) {
      if ((cp->player == -1) || (cp->player != person))
        continue;

      stty_restore(cp);
    }
    return;
  } else if (!(switches & SAVESTTY_CLEAR)) {
    if (!isALIVE(person)) {       /* @force won't crash me */
      if(!(switches & CMD_QUIET))
        notify_player(player, cause, player, "That person is not connected.",
		      NOT_QUIET);
      return;
    }

    for (cp = connlist; cp != (struct conn *) NULL; cp = cp->next) {
      if ((cp->player == -1) || (cp->player != person))
        continue;

      snprintf(buf, sizeof(buf), "%d:%s", cp->lwrap, stty_nltab[cp->lcrnl]);
      if(attr_add(player, STTY, buf, STTY_FLGS) == -1) {
        notify_bad(player);
        return;
      }
      break;
    }

    if(!(switches & CMD_QUIET))
      notify_player(player, cause, player, "Stty settings saved.", NOT_QUIET);
  } else {
    if(attr_delete(player, STTY) == -1) {
      notify_bad(player);
      return;
    }

    if(!(switches & CMD_QUIET))
      notify_player(player, cause, player, "Saved stty settings cleared.",
      		    NOT_QUIET);
  }
}
