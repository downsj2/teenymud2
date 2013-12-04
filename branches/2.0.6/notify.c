/* notify.c */

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
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif			/* HAVE_STDLIB_H */

#include "conf.h"
#include "teeny.h"
#include "teenydb.h"
#include "externs.h"

/* routines for sending strings to players. */

static void nospoof_mesg _ANSI_ARGS_((char *, int, int, int));

void notify_bad(player)
    int player;
{
  notify_player(player, player, player,
  		"Something truly horrid just happened.  Good luck.", 0);
}

/* Emulate MUSH-style NOSPOOF output. */
static void nospoof_mesg(buf, player, cause, sender)
    char *buf;
    int player, cause, sender;
{
  char *sname, *soname;
  int sowner;

  if(get_str_elt(sender, NAME, &sname) != -1) {
    sprintf(buf, "[%s(#%d)", sname, sender);
  } else {
    sprintf(buf, "[(#%d", sender);
  }

  if(get_int_elt(sender, OWNER, &sowner) != -1) {
    if(sender != sowner) {
      if(get_str_elt(sowner, NAME, &soname) != -1)
	sprintf(&buf[strlen(buf)], "{%s}", soname);
    }
  }

  if(sender != cause)
    sprintf(&buf[strlen(buf)], "<-(#%d)", cause);

  strcat(buf, "]");
}

/* Ugh. */
void notify_player(player, cause, sender, str, nflags)
    int player, cause, sender;
    char *str;
    int nflags;
{
  char buf[MEDBUFFSIZ];
  char *lbuf;
  int owner, loc, oloc;
  int i, j;
  char *nstr, *cstr;
  char *name;

  if ((str == (char *)NULL) || (str[0] == '\0'))
    return;
  if (!can_hear(player))
    return;
  if ((nflags & NOT_QUIET) && (isQUIET(player) || isQUIET(sender)))
    return;

  /* we don't want to modify str. */
  lbuf = (char *)alloca(strlen(str)+1);
  if(lbuf == (char *)NULL)
    panic("notify_player: stack allocation failed.\n");
  strcpy(lbuf, str);
  nstr = lbuf;
  cstr = lbuf;

  while (nstr[0]) {
    for (; nstr[0] && (nstr[0] != '\n') && (nstr[0] != '\r'); nstr++);
    if ((nstr[0] == '\n') || (nstr[0] == '\r')) {
      nstr[0] = '\0';
      for (nstr++; nstr[0] &&
	   ((nstr[0] == '\n') || (nstr[0] == '\r')); nstr++);
    }
    if (isALIVE(player)) {
      if ((nflags & NOT_NOSPOOF) && isNOSPOOF(player)) {
	nospoof_mesg(buf, player, cause, sender);
	i = strlen(buf);
      } else
	i = 0;
      for (j = 0; (i < sizeof(buf)-2) && cstr[j]; i++, j++) {
	buf[i] = cstr[j];
      }
      buf[i] = '\0';

      if (nflags & NOT_RAW)
	tcp_notify_raw(player, buf);
      else
        tcp_notify(player, buf);
    }
    if (isPUPPET(player)) {
      switch (Typeof(player)) {
      case TYP_THING:
	if ((get_str_elt(player, NAME, &name) == -1)
	    || (get_int_elt(player, OWNER, &owner) == -1)
	    || (get_int_elt(player, LOC, &loc) == -1)
	    || (get_int_elt(owner, LOC, &oloc) == -1))
	  return;
	if (((nflags & NOT_NOPUPPET) && (loc == oloc)) || !isALIVE(owner))
	  break;
	if ((nflags & NOT_NOSPOOF) && isNOSPOOF(player)) {
	  nospoof_mesg(buf, player, cause, sender);
	  i = strlen(buf);
	} else
	  i = 0;

	for (j = 0; (i < sizeof(buf)-3) && name[j]; i++, j++) {
	  buf[i] = name[j];
	}

	buf[i++] = '>';
	buf[i++] = ' ';
	for (j = 0; (i < sizeof(buf)-2) && cstr[j]; i++, j++) {
	  buf[i] = cstr[j];
	}
	buf[i] = '\0';

	if (nflags & NOT_RAW)
	  tcp_notify_raw(owner, buf);
	else
	  tcp_notify(owner, buf);
	break;
      }
    }

    if (isLISTENER(player)) {
      if(player != sender) {
        attr_listen(player, cause, '^', cstr);
      } else {
	attr_listen(player, cause, '+', cstr);
      }
    }

    if (isAUDIBLE(player) && !isROOM(player) && !(nflags & NOT_NOAUDIBLE))
      notify_audible(player, cause, sender, cstr);

    cstr = nstr;
  }
}

void notify_audible(object, cause, sender, str)
    int object, cause, sender;
    char *str;
{
  char buf[MEDBUFFSIZ], *prefix, *filter;
  int aflags, source;
  int i, j;

  if ((attr_get_parent(object, APREFIX, &prefix, &aflags, &source) == -1) ||
   (attr_get_parent(object, AFILTER, &filter, &aflags, &source) == -1))
    return;
  if ((filter != (char *) NULL) && filter_match(str, filter))
    return;
  if ((prefix == (char *) NULL) && isEXIT(object))
    prefix = "From a distance,";
  if (prefix && prefix[0]) {
    strcpy(buf, prefix);
    strcat(buf, " ");
  } else
    buf[0] = '\0';
  for (i = strlen(buf), j = 0; (i < MEDBUFFSIZ) && str[j]; i++, j++)
    buf[i] = str[j];
  buf[i] = '\0';

  if (isEXIT(object)) {
    int *dests, loc;

    if ((get_int_elt(object, LOC, &loc) == -1) ||
	(get_array_elt(object, DESTS, &dests) == -1) || (dests == (int *)NULL)
	|| (loc == dests[1]) || !exists_object(dests[1]) || isACTION(object))
      return;
    notify_contents(dests[1], cause, sender, buf, NOT_NOAUDIBLE);
  } else
    notify_contents(object, cause, sender, buf, NOT_NOAUDIBLE);
}

void notify_except2(player, cause, sender, string, except1, except2, nflags)
    int player, cause, sender;
    char *string;
    int except1, except2, nflags;
{
  int loc, list;

  if (!string || !string[0])
    return;

  if (get_int_elt(player, LOC, &loc) == -1) {
    logfile(LOG_ERROR, "notify_except2: bad location reference on object #%d\n",
	    player);
    return;
  }
  if (get_int_elt(loc, CONTENTS, &list) == -1) {
    logfile(LOG_ERROR, "notify_except2: bad contents reference on object #%d\n",
	    loc);
    return;
  }
  while (list != -1) {
    if ((list != except1) && (list != except2)) {
      notify_player(list, cause, sender, string, nflags);
    }
    if (get_int_elt(list, NEXT, &list) == -1) {
      logfile(LOG_ERROR, "notify_except2: bad next reference on object #%d\n",
	      list);
      return;
    }
  }
  if (!(nflags & NOT_NOAUDIBLE)) {
    if (get_int_elt(loc, EXITS, &list) == -1) {
      logfile(LOG_ERROR, "notify_except2: bad exits list on object #%d\n", loc);
      return;
    }
    while (list != -1) {
      if (isAUDIBLE(list) && (list != except1) && (list != except2))
	notify_audible(list, cause, sender, string);
      if (get_int_elt(list, NEXT, &list) == -1) {
	logfile(LOG_ERROR, "notify_except2: bad next reference on object #%d\n",
	        list);
	return;
      }
    }
  }
}

void notify_contents_except(loc, cause, sender, str, except, nflags)
    int loc, cause, sender;
    char *str;
    int except, nflags;
{
  int list;

  if (!has_contents(loc) || !str || !str[0])
    return;

  if (get_int_elt(loc, CONTENTS, &list) == -1) {
    logfile(LOG_ERROR,
	    "notify_contents_except: bad contents list on object #%d\n", list);
    return;
  }
  while (list != -1) {
    if ((except == -1) || (list != except)) {
      notify_player(list, cause, sender, str, nflags);
    }
    if (get_int_elt(list, NEXT, &list) == -1) {
      logfile(LOG_ERROR,
	      "notify_contents_except: bad next reference on object #%d\n",
	      list);
      return;
    }
  }
  if (!(nflags & NOT_NOAUDIBLE)) {
    if (get_int_elt(loc, EXITS, &list) == -1) {
      logfile(LOG_ERROR, "notify_contents_except: bad exits on object #%d\n",
	      loc);
      return;
    }
    while (list != -1) {
      if (isAUDIBLE(list) && (list != except))
	notify_audible(list, cause, sender, str);
      if (get_int_elt(list, NEXT, &list) == -1) {
	logfile(LOG_ERROR,
	        "notify_contents_except: bad next reference on object #%d\n",
	        list);
	return;
      }
    }
  }
}

void notify_contents_loc(loc, cause, sender, str, except, nflags)
    int loc, cause, sender;
    char *str;
    int except, nflags;
{
  int list;

  if (!has_contents(loc) || !str || !str[0])
    return;

  do {
    if (get_int_elt(loc, CONTENTS, &list) == -1) {
      logfile(LOG_ERROR,
	      "notify_contents_loc: bad contents list on object #%d\n",
	      list);
      return;
    }
    while (list != -1) {
      if ((except == -1) || (list != except)) {
	notify_player(list, cause, sender, str, nflags);
      }
      if (get_int_elt(list, NEXT, &list) == -1) {
	logfile(LOG_ERROR,
	        "notify_contents_loc: bad next reference on object #%d\n",
		list);
	return;
      }
    }
    if (!(nflags & NOT_NOAUDIBLE)) {
      if (get_int_elt(loc, EXITS, &list) == -1) {
	logfile(LOG_ERROR,
		"notify_contents_loc: bad exits list on object #%d\n", loc);
	return;
      }
      while (list != -1) {
	if (isAUDIBLE(list) && (list != except))
	  notify_audible(list, cause, sender, str);
	if (get_int_elt(list, NEXT, &list) == -1) {
	  logfile(LOG_ERROR,
	          "notify_contents_loc: bad next reference on object #%d\n",
		  list);
	  return;
	}
      }
    }
  }
  while (!isROOM(loc) && (get_int_elt(loc, LOC, &loc) != -1));
}

void notify_exits(player, cause, sender, list)
    int player, cause, sender, list;
{
  char *name, buffer[BUFFSIZ + 4], *p;
  int any = 0, bufsiz, dohtml;

  dohtml = has_html(player);

  p = buffer;
  bufsiz = sizeof(buffer) - 4;
  while (list != -1) {
    if (!isOBVIOUS(list)) {
      if (get_int_elt(list, NEXT, &list) == -1) {
	logfile(LOG_ERROR, "notify_exits: bad next reference on object #%d\n",
	        list);
	return;
      }
      continue;
    }
    any++;

    if (get_str_elt(list, NAME, &name) == -1) {
      logfile(LOG_ERROR, "notify_exits: bad name reference on object #%d\n",
	      list);
      return;
    }
    while ((name[0] == ' ') || (name[0] == ';'))
      name++;

    if (dohtml)
      name = html_anchor_exit(name);

    for (; *name && (*name != ';') && ((p - buffer) < bufsiz);
	 *p++ = *name++);
    if ((p - buffer) > bufsiz) {
      *p = '\0';
      break;
    }
    *p++ = ',';
    *p++ = ' ';

    if (get_int_elt(list, NEXT, &list) == -1) {
      logfile(LOG_ERROR, "notify_exits: bad next reference on object #%d\n",
	      list);
      return;
    }
  }
  if (any) {
    notify_player(player, cause, sender, "Obvious exits:", 0);

    if(p[-2] == ',')
      p[-2] = '\0';

    if(dohtml)
      notify_player(player, cause, sender, buffer, NOT_RAW);
    else
      notify_player(player, cause, sender, buffer, 0);
  }
}

void notify_list(player, cause, sender, list, flags)
    int player, cause, sender, list, flags;
{
  int count;
  int dohtml = has_html(player);

  if (dohtml)
    flags |= NOT_RAW;

  for (count = 0; (list != -1) && (count < mudconf.max_list); count++) {
    if (!(flags & NOT_DARKOK)) {
      if (!can_see(player, cause, list)) {	/* Skip this name */
	if (get_int_elt(list, NEXT, &list) == -1) {
	  logfile(LOG_ERROR, "notify_list: bad next reference on object #%d\n",
	          list);
	  break;
	}
	continue;
      }
    }

    if ((flags & NOT_DARKOK) || (list != player)) {
      if (dohtml) {
        char *name;

        if(get_str_elt(list, NAME, &name) == -1)
	  name = "???";

	notify_player(player, cause, sender,
		      html_anchor_contents(name,
					   display_name(player, cause, list)),
		      flags);
      } else {
        notify_player(player, cause, sender, display_name(player, cause, list),
		      flags);
      }
    }
    if (get_int_elt(list, NEXT, &list) == -1) {
      logfile(LOG_ERROR, "notify_list: bad next reference on object #%d\n",
	      list);
      break;
    }
  }
}

/* a combination of notify_exits() and notify_list(). */
void notify_list2(player, cause, sender, list, flags)
    int player, cause, sender, list, flags;
{
  char *name, buffer[BUFFSIZ + 4], *ptr;
  int count, bufsiz;
  int dohtml = has_html(player);

  if (dohtml)
    flags |= NOT_RAW;

  bufsiz = sizeof(buffer) - 4;
  ptr = buffer;
  for (count = 0; (list != -1) && (count < mudconf.max_list); count++) {
    if (!(flags & NOT_DARKOK)) {
      if (!can_see(player, cause, list)) {	/* Skip this name */
	if (get_int_elt(list, NEXT, &list) == -1) {
	  logfile(LOG_ERROR, "notify_list2: bad next reference on object #%d\n",
	          list);
	  break;
	}
	continue;
      }
    }

    if ((flags & NOT_DARKOK) || (list != player)) {
      if (dohtml) {
	char *sname;

        if(get_str_elt(list, NAME, &sname) == -1)
	  sname = "???";
	name = html_anchor_contents(sname, display_name(player, cause, list));
      } else
	name = display_name(player, cause, list);

      if (((ptr - buffer) + strlen(name)) > bufsiz) {
	/* Display what we have and continue. */
	if (ptr[-2] == ',')
	  ptr[-2] = '\0';
	notify_player(player, cause, sender, buffer, flags);

	ptr = buffer;
	goto nextlist;
      }

      for (; *name && ((ptr - buffer) < bufsiz); *ptr++ = *name++);
      if ((ptr - buffer) > bufsiz) {
	/* Opps, we overran it... */
        *ptr = '\0';
        break;
      }
      *ptr++ = ',';
      *ptr++ = ' ';
    }

nextlist:
    if (get_int_elt(list, NEXT, &list) == -1) {
      logfile(LOG_ERROR, "notify_list2: bad next reference on object #%d\n",
	      list);
      break;
    }
  }

  if (ptr > buffer) {
    if(ptr[-2] == ',')
      ptr[-2] = '\0';
    notify_player(player, cause, sender, buffer, flags);
  }
}
