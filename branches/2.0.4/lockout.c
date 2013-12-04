/* lockout.c */

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

#include <stdio.h>
#include <sys/types.h>
#ifdef HAVE_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif			/* HAVE_STRING_H */
#ifdef TM_IN_SYS_TIME
#include <sys/time.h>
#else
#include <time.h>
#endif			/* TM_IN_SYS_TIME */
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif			/* HAVE_STDLIB_H */

#include "conf.h"
#include "teeny.h"
#include "commands.h"
#include "externs.h"

/* lockout routines are here, since they're wholly config options */
struct locklist {
  int type;

  char *host;
  char *mesg;

  short shour;
  short ehour;
  short limit;

  struct locklist *next;
};

static struct locklist *lockout = (struct locklist *)NULL;
static struct locklist *allow = (struct locklist *)NULL;

int lockout_add(cmd, host, mesg)
    char *cmd, *host, *mesg;
{
  struct locklist *new;
  int add = 1;
  int type;
  register char *hptr, *lptr;

  if(*cmd == '-') {
    add = 0;
    cmd++;
  } else if(*cmd == '+')
    cmd++;

  if(!*cmd)
    return(-1);
  if(strcasecmp(cmd, "lockout") == 0) {
    type = LOCK_DISCONNECT;
  } else if(strcasecmp(cmd, "register") == 0) {
    type = LOCK_REGISTER;
  } else
    return(-1);

  for(hptr = host; *hptr && (*hptr != ':'); hptr++);
  if(*hptr == ':') {
    *hptr++ = '\0';

    for(lptr = hptr; *lptr && (*lptr != ':'); lptr++);
    if(*lptr == ':')
      *lptr++ = '\0';
    else
      lptr = (char *)NULL;
  } else {
    hptr = (char *)NULL;
    lptr = (char *)NULL;
  }

  new = (struct locklist *)ty_malloc(sizeof(struct locklist), "lockout_add");
  new->type = type;
  new->host = (char *)ty_strdup(host, "lockout_add");
  new->mesg = (char *)ty_strdup(mesg, "lockout_add");

  if((hptr != (char *)NULL) && *hptr) {
    char *yptr, *zptr;

    new->shour = (short)strtol(hptr, &yptr, 10);
    if((yptr == hptr) || (*yptr != '-')) {
      new->shour = -1;
      new->ehour = -1;
    } else {
      if(new->shour >= 100)
        new->shour /= 100;

      yptr++;
      new->ehour = (short)strtol(yptr, &zptr, 10);
      if(zptr == yptr) {
        new->shour = -1;
	new->ehour = -1;
      } else {
        if(new->ehour >= 100)
	  new->ehour /= 100;
      }
    }
  } else {
    new->shour = -1;
    new->ehour = -1;
  }

  if((lptr != (char *)NULL) && *lptr) {
    char *zptr;

    new->limit = (short)strtol(lptr, &zptr, 10);
    if(zptr == lptr)
      new->limit = -1;
  } else
    new->limit = -1;

  /* add it to the right list */
  if(add) {
    new->next = lockout;
    lockout = new;
  } else {
    new->next = allow;
    allow = new;
  }

  return(0);
}

/* lockout command. */
VOID do_lockout(player, cause, switches, argone, argtwo)
    int player, cause, switches;
    char *argone, *argtwo;
{
  struct locklist *new, *curr;
  int type = LOCK_DISCONNECT;
  char buffer[MEDBUFFSIZ];
  register char *hptr, *lptr;
  
  if(switches & LOCKOUT_CLEAR) {
    for(curr = lockout; curr != (struct locklist *)NULL; curr = new) {
      new = curr->next;

      ty_free((VOID *)curr);
    }
    for(curr = allow; curr != (struct locklist *)NULL; curr = new) {
      new = curr->next;

      ty_free((VOID *)curr);
    }
    lockout = (struct locklist *)NULL;
    allow = (struct locklist *)NULL;

    if(!(switches & CMD_QUIET))
      notify_player(player, cause, player, "Lockout lists cleared.",
      		    NOT_QUIET);
    return;
  }

  if(switches & LOCKOUT_REGISTER)
    type = LOCK_REGISTER;

  if(switches & LOCKOUT_ALL) {
    mudstat.logins = LOGINS_DISABLED;

    if(!(switches & CMD_QUIET))
      notify_player(player, cause, player, "All logins disabled.", NOT_QUIET);
    return;
  }

  if(switches & LOCKOUT_ENABLE) {
    mudstat.logins = LOGINS_ENABLED;

    if(!(switches & CMD_QUIET))
      notify_player(player, cause, player, "Logins enabled.", NOT_QUIET);
    return;
  }

  if(switches & LOCKOUT_DUMP) {
    notify_player(player, cause, player, "Lockout list:", 0);
    for(curr = lockout; curr != (struct locklist *)NULL; curr = curr->next) {
      if(((curr->shour == -1) || (curr->ehour == -1)) && (curr->limit == -1)) {
        snprintf(buffer, sizeof(buffer), "%s: %s \"%s\"",
      	         ((curr->type == LOCK_DISCONNECT) ? "lockout" : "register"),
	         curr->host, ((curr->mesg == (char *)NULL) ? "" : curr->mesg));
      } else if(curr->limit == -1) {
        snprintf(buffer, sizeof(buffer), "%s: %s [%d-%d] \"%s\"",
		 ((curr->type == LOCK_DISCONNECT) ? "lockout" : "register"),
		 curr->host, curr->shour, curr->ehour,
		 ((curr->mesg == (char *)NULL) ? "" : curr->mesg));
      } else if((curr->shour == -1) || (curr->ehour == -1)) {
        snprintf(buffer, sizeof(buffer), "%s: %s {%d} \"%s\"",
		 ((curr->type == LOCK_DISCONNECT) ? "lockout" : "register"),
		 curr->host, curr->limit,
		 ((curr->mesg == (char *)NULL) ? "" : curr->mesg));
      } else {
        snprintf(buffer, sizeof(buffer), "%s: %s [%d-%d]{%d} \"%s\"",
		 ((curr->type == LOCK_DISCONNECT) ? "lockout" : "register"),
		 curr->host, curr->shour, curr->ehour, curr->limit,
		 ((curr->mesg == (char *)NULL) ? "" : curr->mesg));
      }
      notify_player(player, cause, player, buffer, 0);
    }

    notify_player(player, cause, player, "Allow list:", 0);
    for(curr = lockout; curr != (struct locklist *)NULL; curr = curr->next) {
      if(((curr->shour == -1) || (curr->ehour == -1)) && (curr->limit == -1)) {
        snprintf(buffer, sizeof(buffer), "%s: %s \"%s\"",
      	         ((curr->type == LOCK_DISCONNECT) ? "lockout" : "register"),
	         curr->host, ((curr->mesg == (char *)NULL) ? "" : curr->mesg));
      } else if(curr->limit == -1) {
        snprintf(buffer, sizeof(buffer), "%s: %s [%d-%d] \"%s\"",
		 ((curr->type == LOCK_DISCONNECT) ? "lockout" : "register"),
		 curr->host, curr->shour, curr->ehour,
		 ((curr->mesg == (char *)NULL) ? "" : curr->mesg));
      } else if((curr->shour == -1) || (curr->ehour == -1)) {
        snprintf(buffer, sizeof(buffer), "%s: %s {%d} \"%s\"",
		 ((curr->type == LOCK_DISCONNECT) ? "lockout" : "register"),
		 curr->host, curr->limit,
		 ((curr->mesg == (char *)NULL) ? "" : curr->mesg));
      } else {
        snprintf(buffer, sizeof(buffer), "%s: %s [%d-%d]{%d} \"%s\"",
		 ((curr->type == LOCK_DISCONNECT) ? "lockout" : "register"),
		 curr->host, curr->shour, curr->ehour, curr->limit,
		 ((curr->mesg == (char *)NULL) ? "" : curr->mesg));
      }
      notify_player(player, cause, player, buffer, 0);
    }
    notify_player(player, cause, player, "***End of lists***", 0);

    return;
  }

  if((argone == (char *)NULL) || (argone[0] == '\0')) {
    if(!(switches & CMD_QUIET))
      notify_player(player, cause, player, "But who do you wish to lockout?",
      		    NOT_QUIET);
    return;
  }

  for(hptr = argone; *hptr && (*hptr != ':'); hptr++);
  if(*hptr == ':') {
    *hptr++ = '\0';

    for(lptr = hptr; *lptr && (*lptr != ':'); lptr++);
    if(*lptr == ':')
      *lptr++ = '\0';
    else
      lptr = (char *)NULL;
  } else {
    hptr = (char *)NULL;
    lptr = (char *)NULL;
  }

  new = (struct locklist *)ty_malloc(sizeof(struct locklist), "do_lockout.new");
  new->type = type;
  new->host = (char *)ty_strdup(argone, "do_lockout.new");

  if((hptr != (char *)NULL) && *hptr) {
    char *yptr, *zptr;

    new->shour = (short)strtol(hptr, &yptr, 10);
    if((yptr == hptr) || (*yptr != '-')) {
      new->shour = -1;
      new->ehour = -1;
    } else {
      yptr++;

      new->ehour = (short)strtol(yptr, &zptr, 10);
      if(zptr == yptr) {
        new->shour = -1;
	new->ehour = -1;
      }
    }
  } else {
    new->shour = -1;
    new->ehour = -1;
  }

  if((lptr != (char *)NULL) && *lptr) {
    char *zptr;

    new->limit = (short)strtol(lptr, &zptr, 10);
    if(zptr == lptr)
      new->limit = -1;
  } else
    new->limit = -1;

  if((argtwo != (char *)NULL) && argtwo[0])
    new->mesg = (char *)ty_strdup(argtwo, "do_lockout.mesg");
  else
    new->mesg = (char *)NULL;

  if(switches & LOCKOUT_ALLOW) {
    new->next = allow;
    allow = new;
  } else {
    new->next = lockout;
    lockout = new;
  }

  if(!(switches & CMD_QUIET))
    notify_player(player, cause, player, "Entry added.", NOT_QUIET);
}

/* lockout/registration checker for the rest of the system */
int check_host(host, type, mesg)
    char *host;
    int type;
    char **mesg;
{
  register struct locklist *entry;
  struct tm *now;

  *mesg = (char *)NULL;
  now = localtime(&mudstat.now);

  /* check allow list first */
  for(entry = allow; entry != (struct locklist *)NULL; entry = entry->next) {
    if((entry->type == type) && quick_wild(entry->host, host)) {
      register int ret = 0;

      if((entry->shour != -1) && (entry->ehour != -1)) {
        ret = !((now->tm_hour >= entry->shour)
	        && (now->tm_hour <= entry->ehour));
      }
      if(entry->limit != -1)
        ret = (count_logins(host) <= entry->limit);

      *mesg = entry->mesg;
      return(ret);
    }
  }

  for(entry = lockout; entry != (struct locklist *)NULL; entry = entry->next) {
    if((entry->type == type) && quick_wild(entry->host, host)) {
      register int ret = 1;

      if((entry->shour != -1) && (entry->ehour != -1)) {
        ret = ((now->tm_hour >= entry->shour)
	       && (now->tm_hour <= entry->ehour));
      }
      if(entry->limit != -1)
        ret = (count_logins(host) > entry->limit);

      *mesg = entry->mesg;
      return(ret);
    }
  }
  return(0);
}
