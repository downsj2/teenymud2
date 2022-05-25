/* utils.c */

#include "config.h"

/*
 *		       This file is part of TeenyMUD II.
 *		 Copyright(C) 1993, 1994, 1995, 2013, 2022 by Jason Downs.
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

#ifdef HAVE_ALLOCA_H
#include <alloca.h>
#endif	/* HAVE_ALLOCA_H */

#include <stdio.h>
#include <sys/types.h>
#ifdef HAVE_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif				/* HAVE_STRING_H */
#include <ctype.h>
#ifdef TM_IN_SYS_TIME
#include <sys/time.h>
#else
#include <time.h>
#endif				/* TM_IN_SYS_TIME */
#include <sys/stat.h>
#ifdef HAVE_MALLOC_H
#include <malloc.h>
#endif				/* HAVE_MALLOC_H */
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif				/* HAVE_STDLIB_H */

#include "conf.h"
#include "teeny.h"
#include "commands.h"
#include "ptable.h"
#include "sha/sha_wrap.h"
#include "externs.h"

/* Some generic utility functions for TeenyMUD */

int create_player(name, pwd, code)
    char *name, *pwd;
    int code;
{
  int player, ret = 0;
  int flags[FLAGS_LEN];

  /* No guests can be created from the greet screen. */
  if ((code == -1) 
      && (!ok_player_name(name) || (strncasecmp(name, "guest", 5) == 0)
	  || (pwd == (char *)NULL)))
    return(-1);

  /* Create a player in object number STARTING_LOC */
  player = create_obj(TYP_PLAYER);
  ret = set_str_elt(player, NAME, name);
  if ((ret != -1) && (pwd != (char *)NULL) && pwd[0])
    ret = attr_add(player, PASSWORD, cryptpwd(pwd), PASSWORD_FLGS);
  if (code == -1) {
    if (ret != -1) {
      list_add(player, mudconf.starting_loc, CONTENTS);
      ret = set_int_elt(player, LOC, mudconf.starting_loc);
    }
  } else {
    if (ret != -1) {
      list_add(player, code, CONTENTS);
      ret = set_int_elt(player, LOC, code);
    }
  }
  if (ret != -1)
    ret = set_int_elt(player, HOME, mudconf.starting_loc);
  if (ret != -1)
    ret = set_int_elt(player, OWNER, player);
  if (ret != -1)
    ret = get_flags_elt(player, FLAGS, flags);
  if (ret != -1) {
    flags[0] |= (mudconf.newp_flags)[0];
    flags[1] |= (mudconf.newp_flags)[1];
    ret = set_flags_elt(player, FLAGS, flags);
  }
  if (ret != -1)
    ret = set_int_elt(player, QUOTA, mudconf.starting_quota);
  if (ret != -1)
    ret = set_int_elt(player, PENNIES, mudconf.starting_money);

  if (ret == -1) {
    destroy_obj(player);
    return (-1);
  }
  return (player);
}

int connect_player(name, pwd)
    char *name, *pwd;
{
  char *realpwd;
  int player, aflags;

  /*
   * Special goo for multiple guest character support.
   */
  if(strcasecmp(name, "guest") == 0) {	/* They're connecting to guest. */
    player = match_player(name);
    if(player == -1) {		/* No "guest", look for "guestN". */
      int gn;
      char sbuf[10];

      for(gn = 1; gn < 256; gn++) {
	snprintf(sbuf, sizeof(sbuf), "guest%d", gn);

	player = match_player(sbuf);
	if(player == -1)
	  return(-1);
	
	/* Is it already connected? */
	if(isALIVE(player))
	  continue;

	/* Otherwise, we found the right one. */
	break;
      }
    }
  } else {
    char *sptr;

    player = match_player(name);
    if (player == -1) {
      if((name[0] == '#') && isdigit(name[1])) {
        player = (int)strtol(&name[1], &sptr, 10);
	if((sptr != &name[1]) && exists_object(player) && isPLAYER(player))
	  return(player);
      }
      return (-1);
    }
  }

  /* Check the password */
  if (attr_get(player, PASSWORD, &realpwd, &aflags) == -1) {
    logfile(LOG_ERROR,
	    "connect_player: bad attribute reference on object #%d\n", player);
    return (-1);
  }

  if (((realpwd == (char *)NULL) || (realpwd[0] == '\0'))
      && (pwd == (char *)NULL))
    return (player);
  if ((realpwd != (char *)NULL) && (realpwd[0] != '\0')
      && (pwd != (char *)NULL) && comp_password(realpwd, pwd))
    return (player);
  return (-1);
}

void announce_connect(player, user, host)
    int player;
    char *user, *host;
{
  int loc;
  char *name;
  char buf[256];
  int wflags[FLAGS_LEN];

  if (get_int_elt(player, LOC, &loc) != -1) {
    if (get_str_elt(player, NAME, &name) != -1) {
      if (!isDARK(player) && !isDARK(loc)) {
	snprintf(buf, sizeof(buf), "%s has %s.", name, isALIVE(player) ?
		 "reconnected" : "connected");
	notify_all(player, player, player, buf, NOT_NOPUPPET);
      }

      if(user != (char *)NULL) {
        snprintf(buf, sizeof(buf), "[ %s has connected from %s@%s. ]", name,
		user, host);
      } else {
        snprintf(buf, sizeof(buf), "[ %s has connected from %s. ]", name, host);
      }

      wflags[0] = RETENTIVE;
      wflags[1] = 0;
      tcp_wall(wflags, buf, -1);
    }
  }
}

void announce_disconnect(player)
    int player;
{
  int loc;
  char *name;
  char buf[80];
  int wflags[FLAGS_LEN];

  if (get_int_elt(player, LOC, &loc) != -1) {
    if (get_str_elt(player, NAME, &name) != -1) {
      if (!isDARK(player) && !isDARK(loc)) {
	snprintf(buf, sizeof(buf), "%s has %s.", name, isALIVE(player) ?
		 "partially disconnected" : "disconnected");
	notify_all(player, player, player, buf, NOT_NOPUPPET);
      }
      snprintf(buf, sizeof(buf), "[ %s has disconnected. ]", name);

      wflags[0] = RETENTIVE;
      wflags[1] = 0;
      tcp_wall(wflags, buf, -1);
    }
  }
  if (isGUEST(player) && !isALIVE(player)) {
    /* send "GUEST" players to their home */
    do_home(player, player, 0);
  }
}

/* Hack name/pwd pair up. */
int parse_name_pwd(str, name, pwd)
    char *str, **name, **pwd;
{
  static char nbuf[MAXPNAMELEN + 1], pbuf[MAXPWDLEN + 1];
  char *p;

  if (str == (char *)NULL)
    return(-1);

  while (*str && isspace(*str))
    str++;
  if (*str == '\0')
    return (-1);

  if (*str != '\"') {		/* the name isn't quoted, fuck */
    /* assume the first word is name, all other is pword */
    for (p = nbuf; *str && !isspace(*str) && (p - nbuf) < sizeof(nbuf);
	 *p++ = *str++);
    *p = '\0';

    while (isspace(*str) && *str)
      str++;
    if (*str == '\0')
      *pwd = (char *) NULL;
    else {
      for (p = pbuf; *str && (p - pbuf) < sizeof(pbuf); *p++ = *str++);
      *p = '\0';
      *pwd = pbuf;		/* Assume no trailing whitespace. */
    }
    *name = nbuf;
    return (0);
  } else {			/* quoted name! YAY! */
    while (*str && *str == '\"')
      str++;
    while (*str && isspace(*str))
      str++;
    p = nbuf;
    *name = nbuf;
    while (*str && *str != '\"' && (p - nbuf) < sizeof(nbuf)) {
      if (!isspace(*str)) {
	*p++ = *str++;
	continue;
      }
      for (; *str && isspace(*str); str++);
      if (*str && (*str != '\"'))
	*p++ = ' ';
    }
    *p = '\0';

    while (*str && (*str == '\"' || isspace(*str)))
      str++;
    if (*str == '\0')
      *pwd = (char *) NULL;
    else {
      for (p = pbuf; *str && (p - pbuf) < sizeof(pbuf); *p++ = *str++);
      *p = '\0';
      *pwd = pbuf;
    }
    return (0);
  }
}

void parse_type(str, ret)
    char *str;
    int *ret;
{
  switch(str[0]) {
  case 'p':
  case 'P':
    *ret = TYP_PLAYER;
    break;
  case 'r':
  case 'R':
    *ret = TYP_ROOM;
    break;
  case 't':
  case 'T':
    *ret = TYP_THING;
    break;
  case 'e':
  case 'E':
    *ret = TYP_EXIT;
    break;
  default:
    *ret = -1;
  }
}

int ok_attr_name(name)
    char *name;
{
  return (name && *name && (strlen(name) < MAXATTRNAME) &&
	  !strchr(name, '*') && !strchr(name, '?') &&
	  !strchr(name, '\\') && !strchr(name, '[') &&
	  !strchr(name, ']'));
}

int ok_attr_value(data)
    char *data;
{
  int i;

  for (i = 0; data[i] && (i < 4097); i++) {
    if (!isprint(data[i]) || (data[i] == '\r') || (data[i] == '\n'))
      return (0);
  }
  return (i < 4097);
}

int ok_name(name)
    char *name;
{
  int i;

  if (!name || !*name || (name[0] == '#') || (name[0] == '!'))
    return (0);
  for (i = 0; name[i]; i++) {
    if ((name[i] == '=') || (name[i] == '&') || (name[i] == '|')
	|| (name[i] == '*') || (name[i] == '?') || (name[i] == '\\')
	|| (name[i] == '[') || (name[i] == ']') || (name[i] == ':')
	|| (name[i] == '<') || (name[i] == '>') || (name[i] == '&')
	|| (name[i] == '\"') || !isprint(name[i]))
      return (0);
  }

  /* Hard coded bad object names. */
  return (strcmp(name, "A") && strcmp(name, "An") && strcmp(name, "The")
	  && strcmp(name, "You") && strcmp(name, "Your")
          && strcmp(name, "Going") && strcmp(name, "Huh?")
	  && strcmp(name, "me") && strcmp(name, "home")
	  && strcmp(name, "here"));
}

int ok_player_name(name)
    char *name;
{
  /* Do the obvious checkeds, first. */
  if (!ok_name(name) || (strlen(name) > MAXPNAMELEN) ||
      (match_player(name) != -1))
    return (0);
  else {
    int index;

    /* Check for configured bad player names. */
    for(index = 0; index < 64; index++) {
      if(mudconf.bad_pnames[index] != (char *)NULL) {
        if(strcmp(mudconf.bad_pnames[index], name) == 0)
	  return(0);
      } else
        break;
    }
    return (1);
  }
}

int ok_exit_name(name)
    char *name;
{
  char *p, *q, *buf;

  if (!ok_name(name))
    return (0);

  buf = (char *) alloca(strlen(name) + 1);
  if(buf == (char *)NULL)
    panic("ok_exit_name(): stack allocation failed\n");
  strcpy(buf, name);

  /* we have to check each sub string... */
  q = buf;
  while (q && *q) {
    for (p = q; *p && *p != ';'; p++);
    if (*p)
      *p++ = '\0';
    if (!ok_name(q))
      return (0);
    q = p;
  }
  return (1);
}

void reward_money(player, amount)
    int player, amount;
{
  int pennies, owner;

  if(!mudconf.enable_money)
    return;

  if (get_int_elt(player, OWNER, &owner) == -1) {
    logfile(LOG_ERROR, "reward_money: bad owner reference on #%d\n", player);
    return;
  }
  if (isWIZARD(owner))
    return;

  if (get_int_elt(owner, PENNIES, &pennies) == -1) {
    logfile(LOG_ERROR, "reward_money: bad pennies reference on #%d\n", owner);
    return;
  }
  if (set_int_elt(owner, PENNIES, pennies + amount) == -1) {
    logfile(LOG_ERROR, "reward_money: bad pennies reference on #%d\n", owner);
    return;
  }
}

int can_afford(player, cause, amount, quiet)
    int player, cause, amount, quiet;
{
  int pennies, owner;

  if(!mudconf.enable_money)
    return(1);

  if (get_int_elt(player, OWNER, &owner) == -1) {
    logfile(LOG_ERROR, "can_afford: bad owner reference on #%d\n", player);
    return (0);
  }
  if (isWIZARD(owner))
    return (1);

  if (get_int_elt(owner, PENNIES, &pennies) == -1) {
    logfile(LOG_ERROR, "can_afford: bad pennies reference on #%d\n", owner);
    return (0);
  }
  if (pennies > amount) {
    if (set_int_elt(owner, PENNIES, pennies - amount) == -1) {
      logfile(LOG_ERROR, "can_afford: bad pennies reference on #%d\n", owner);
      return (0);
    }
    return (1);
  }
  if(!quiet)
    notify_player(player, cause, player, "You don't have enough pennies.",
    		  NOT_QUIET);
  return (0);
}

void check_paycheck(player)
    int player;
{
  time_t last;
  int pennies;

  if(!mudconf.enable_money)
    return;

  if (isWIZARD(player) ||
      (get_time_elt(player, TIMESTAMP, &last) == -1) ||
      (get_int_elt(player, PENNIES, &pennies) == -1))
    return;
  if (last <= 0)
    return;
  if ((pennies < mudconf.max_pennies) &&
      ((localtime(&mudstat.now))->tm_yday > (localtime(&last))->tm_yday)) {
    if (set_int_elt(player, PENNIES, pennies + mudconf.daily_paycheck) == -1)
      return;
  }
}

void check_last(player)
    int player;
{
  char *host;
  time_t last;
  int aflags;
  char buf[MEDBUFFSIZ];

  if ((get_time_elt(player, TIMESTAMP, &last) == -1)
      || (attr_get(player, SITE, &host, &aflags) == -1))
    return;
  if (last <= 0)
    return;

  strftime(buf, MEDBUFFSIZ, "Last connection: %a %b %e %H:%M:%S %Z %Y from ",
  	   localtime(&last));
  strcat(buf, (host == NULL) ? "???" : host);
  notify_player(player, player, player, buf, 0);
}

/* spew a file to a player, following special permissions. */
void dump_file(player, cause, thing, fname)
    int player, cause, thing;
    char *fname;
{
  FILE *fp;
  char fbuf[128], mbuf[BUFFSIZ];
#ifndef __STDC__
  char *strstr();
#endif

  /* If there is a leading slash, object must be set FILE_OK. */
  if(fname[0] == '/') {
    if(!isFILE_OK(thing)) {
      snprintf(mbuf, sizeof(mbuf), "%s: Permission denied.", fname);
      notify_player(player, cause, thing, mbuf, 0);
      return;
    }
    snprintf(mbuf, sizeof(mbuf), "%s/%s", mudconf.files_path, fname);
  } else
    snprintf(mbuf, sizeof(mbuf), "%s/#%d/%s", mudconf.files_path, thing, fname);

  /* If it has a "../" in it, fail. */
  if(strstr(fname, "../") != (char *)NULL) {
    snprintf(mbuf, sizeof(mbuf), "%s: Permission denied.", fname);
    notify_player(player, cause, thing, mbuf, 0);
    return;
  }

  fp = fopen(mbuf, "r");
  if(fp == (FILE *)NULL) {
    snprintf(mbuf, sizeof(mbuf), "%s: Can't open file.", fname);
    notify_player(player, cause, thing, mbuf, 0);
    return;
  }
  while(!feof(fp) && (fgets(fbuf, sizeof(fbuf), fp) != (char *)NULL)) {
    remove_newline(fbuf);
    notify_player(player, cause, thing, fbuf, 0);
  }
  fclose(fp);
}

/* spew a shell command at the player, following special permissions. */
void dump_cmdoutput(player, cause, thing, fname)
    int player, cause, thing;
    char *fname;
{
  FILE *fp;
  char fbuf[128], mbuf[BUFFSIZ];
#ifndef __STDC__
  char *strstr();
#endif


  /* If there is a leading slash, object must be set FILE_OK. */
  if(fname[0] == '/') {
    if(!isFILE_OK(thing)) {
      snprintf(mbuf, sizeof(mbuf), "%s: Permission denied.", fname);
      notify_player(player, cause, thing, mbuf, 0);
      return;
    }
    snprintf(mbuf, sizeof(mbuf), "%s/%s", mudconf.files_path, fname);
  } else
    snprintf(mbuf, sizeof(mbuf), "%s/#%d/%s", mudconf.files_path, thing, fname);

  /* If it has a "../" in it, fail. */
  if(strstr(fname, "../") != (char *)NULL) {
    snprintf(mbuf, sizeof(mbuf), "%s: Permission denied.", fname);
    notify_player(player, cause, thing, mbuf, 0);
    return;
  }

  fp = popen(mbuf, "r");
  if(fp == (FILE *)NULL) {
    snprintf(mbuf, sizeof(mbuf), "%s: Can not execute.", fname);
    notify_player(player, cause, thing, mbuf, 0);
    return;
  }
  while(!feof(fp) && (fgets(fbuf, sizeof(fbuf), fp) != (char *)NULL)) {
    remove_newline(fbuf);
    notify_player(player, cause, thing, fbuf, 0);
  }
  pclose(fp);
}

int legal_roomloc_check(source, test, check_depth)
    int source, test;
    int *check_depth;
{
  int loc;

  (*check_depth)++;

  if (source == test)
    return (0);
  if (!exists_object(test) || !isROOM(test))
    return (0);
  if (test == mudconf.root_location)
    return (1);
  if ((*check_depth) > mudconf.room_depth)
    return (0);
  if (get_int_elt(test, LOC, &loc) == -1)
    return (0);
  return (legal_roomloc_check(source, loc, check_depth));
}

int legal_thingloc_check(obj, dest)
    int obj, dest;
{
  int loc, depth;

  depth = 0;

  if (isROOM(obj) || isEXIT(obj))
    return(1);
  if (obj == dest)
    return(0);

  loc = dest;
  while(!isROOM(loc) && (depth < mudconf.room_depth)) {
    if ((loc == obj) || ((depth > 0) && (loc == dest)))
      return(0);

    if (get_int_elt(loc, LOC, &loc) == -1)
      return(0);

    depth++;
  }

  if(isROOM(loc))
    return(1);
  return(0);
}

int legal_parent_check(obj, test, check_depth)
    int obj, test;
    int *check_depth;
{
  int parent;

  (*check_depth)++;

  if (test == -1)
    return (1);
  if ((obj == test) || !exists_object(test))
    return (0);
  if ((*check_depth) > mudconf.parent_depth)
    return (0);
  if (get_int_elt(test, PARENT, &parent) == -1)
    return (0);
  return (legal_parent_check(obj, parent, check_depth));
}

int legal_recursive_exit(exit, test, check_depth)
    int exit, test, *check_depth;
{
  int *dests;
  int i;

  (*check_depth)++;

  if ((test == -1) || (test == -3) || (exists_object(test) && !isEXIT(test)))
    return (1);
  if ((exit == test) || !exists_object(test))
    return (0);
  if ((*check_depth) > mudconf.exit_depth)
    return (0);
  if (get_array_elt(test, DESTS, &dests) == -1)
    return (0);

  if (dests != (int *)NULL) {
    for(i = 1; i <= dests[0]; i++) {
      if(dests[i] == exit) {
        return(0);
      }
      if (Typeof(dests[i]) == TYP_EXIT) {
        if (!legal_recursive_exit(exit, dests[i], check_depth))
	  return(0);
      }
    }
  }
  return(1);
}

/*
 * HTML support code.
 *
 * html_anchor_exit() takes an exit name and returns an anchor for it.
 *
 * html_anchor_contents() takes an element from the contents list and
 * returns an anchor for it.
 *
 * html_anchor_location() checks the player's location for a URL, and sends
 * it to them.
 *
 * html_desc() checks for an Htdesc, and sends it.
 */
char *html_anchor_exit(name)
    char *name;
{
  static char ret[2048];
  char *sname, *ptr;

  for(ptr = name; *ptr && (*ptr != ';'); ptr++);
  if(((*ptr == '\0') || (*ptr == ';')) && ((ptr - name) > 0)) {
    sname = (char *)alloca((ptr - name)+1);
    if(name == (char *)NULL) {
      panic("html_anchor_exit: stack allocation failed!");
    }
    strlcpy(sname, name, (ptr - name));
  
    snprintf(ret, sizeof(ret), "<a xch_cmd=\"%s\" xch_hint=\"Go %s\">%s</a>",
	     sname, sname, sname);
    return(ret);
  } else
    return(name);
}

char *html_anchor_contents(sname, name)
    char *sname, *name;
{
  static char ret[2048];

  snprintf(ret, sizeof(ret),
	   "<a xch_cmd=\"look %s\" xch_hint=\"Look at %s\">%s</a>", sname,
	   sname, name);
  return(ret);
}

void html_anchor_location(player, cause)
    int player, cause;
{
  int loc, aflags, source;
  char *attr, *buf;

  if((get_int_elt(player, LOC, &loc) == -1) || !exists_object(loc))
    return;

  if((attr_get_parent(loc, VRML_URL, &attr, &aflags, &source) == 0)
     && (attr != (char *)NULL)) {
    buf = (char *)alloca(strlen(attr)+30);
    if(buf == (char *)NULL) {
      panic("html_anchor_location: stack allocation failed!");
    }

    strcpy(buf, "<img xch_graph=load href=\"<");
    strcat(buf, attr);
    strcat(buf, "\">");

    notify_player(player, cause, player, buf, NOT_RAW);
  }
}

void html_desc(player, cause, thing)
    int player, cause, thing;
{
  int aflags, source;
  char *attr;

  if((attr_get_parent(thing, HTDESC, &attr, &aflags, &source) == 0)
     && (attr != (char *)NULL)) {
    notify_player(player, cause, player, attr, NOT_RAW);
  }
}

#ifdef UNIX_CRYPT

/*
 * Extra GOO to invisibly support both crypt(3) and SHA passwords, for
 * old databases.
 */

int comp_password(encrypt, string)
    char *encrypt, *string;
{
  /* If the encrypted string is 13 characters long, assume it's crypt(3). */
  if(strlen(encrypt) == 13) {
    char salt[3];

    salt[0] = encrypt[0];
    salt[1] = encrypt[1];
    salt[2] = '\0';
    return(strcmp((char *)crypt(string, salt), encrypt) == 0);
  }
  return(strcmp(sha_crypt(string), encrypt) == 0);
}
#endif			/* UNIX_CRYPT */
