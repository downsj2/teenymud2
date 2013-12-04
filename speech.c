/* speech.c */

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

#include <stdio.h>
#include <sys/types.h>
#ifdef HAVE_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif			/* HAVE_STRING_H */
#include <ctype.h>
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif			/* HAVE_STDLIB_H */

#include "conf.h"
#include "teeny.h"
#include "commands.h"
#include "externs.h"

/* Basic player commands pertaining to communication. */

VOID do_pose(player, cause, switches, argone)
    int player, cause, switches;
    char *argone;
{
  char *name;
  char buf[MEDBUFFSIZ];

  if (get_str_elt(player, NAME, &name) != -1) {
    while (argone && *argone && isspace(*argone))
      argone++;

    if (argone && *argone) {
      snprintf(buf, sizeof(buf), "%s%s%s", name,
	       ((argone[0] == '\'') || (argone[0] == ',')
	       || (switches & POSE_NOSPACE)) ? "" : " ",
	       (argone && *argone) ? argone : "");
      } else {
	strcpy(buf, name);
      }
    notify_all(player, cause, player, buf, NOT_NOPUPPET | NOT_NOSPOOF);
  } else
    notify_bad(player);
}

VOID do_emit(player, cause, switches, argone)
    int player, cause, switches;
    char *argone;
{
  if((argone == (char *)NULL) || (argone[0] == '\0')) {
    notify_player(player, cause, player,
		  "But what do you want with the @emit?", 0);
    return;
  }

  if(switches & EMIT_OTHERS) {
    notify_oall(player, cause, player, argone, NOT_NOPUPPET | NOT_NOSPOOF);
    notify_player(player, cause, player, "Ok.", 0);
  } else {
    notify_all(player, cause, player, argone, NOT_NOPUPPET | NOT_NOSPOOF);
  }
}

VOID do_pemit(player, cause, switches, argone, argtwo)
    int player, cause, switches;
    char *argone, *argtwo;
{
  int victim;

  if((argone == (char *)NULL) || (argone[0] == '\0')
     || (argtwo == (char *)NULL) || (argtwo[0] == '\0')) {
    notify_player(player, cause, player,
		  "But what do you want with the @pemit?", 0);
    return;
  }

  victim = resolve_object(player, cause, argone, RSLV_NOISY);
  if(victim == -1)
    return;

  if(islocked(player, cause, victim, PGLOCK)
     || islocked(victim, cause, player, PGLOCK)) {
    notify_player(player, cause, player, "Permission denied.", 0);
    return;
  }

  notify_player(victim, cause, player, argtwo, NOT_NOSPOOF);
  if(victim != player)
    notify_player(player, cause, player, "Ok.", 0);
}

VOID do_page(player, cause, switches, argone, argtwo)
    int player, cause, switches;
    char *argone, *argtwo;
{
  int pagee;
  char *name, buf[MEDBUFFSIZ];
  int location;

  if ((argone == (char *)NULL) || (argone[0] == '\0')) {
    notify_player(player, cause, player, "Page whom?", 0);
    return;
  }
  pagee = resolve_player(player, cause, argone, RSLV_NOISY | RSLV_SPLATOK);
  if(pagee == -1)
    return;
  if (!isALIVE(pagee)) {
    notify_player(player, cause, player, "That player is not connected.", 0);
    return;
  }
  if (islocked(pagee, cause, player, PGLOCK)) {
    notify_player(player, cause, player,
	  "You can't send pages to that person while ignoring them.", 0);
    return;
  }
  if (islocked(player, cause, pagee, PGLOCK)) {
    notify_player(player, cause, player,
    		  "That player does not wish you to disturb them.", 0);
    return;
  }
  /* OK. Send the message. */
  if (!argtwo || !*argtwo) {
    if (isHIDDEN(player)) {
      notify_player(player, cause, player,
	"You may only page messages, since your location is hidden.", 0);
      return;
    }
    if ((get_str_elt(player, NAME, &name) != -1) &&
	(get_int_elt(player, LOC, &location) != -1)) {
      snprintf(buf, sizeof(buf), "You sense that %s is looking for you in %s.",
	       name, display_name(pagee, cause, location));
      notify_player(pagee, cause, player, buf, NOT_NOSPOOF);

      if(get_str_elt(pagee, NAME, &name) != -1) {
        snprintf(buf, sizeof(buf), "%s has been paged.", name);
        notify_player(player, cause, player, buf, 0);
      } else
	notify_bad(player);
    } else
      notify_bad(player);
  } else {			/* message page */
    if (get_str_elt(player, NAME, &name) != -1) {
      if (argtwo && *argtwo && *argtwo == ':' && argtwo[1]) {
	argtwo++;
	snprintf(buf, sizeof(buf), "From afar, %s%s%s", name,
		 (argtwo[0] != '\'' && argtwo[0] != ',') ? " " : "", argtwo);
      } else
	snprintf(buf, sizeof(buf), "%s pages: %s", name,
		 (argtwo && *argtwo) ? argtwo : "");
      notify_player(pagee, cause, player, buf, NOT_NOSPOOF);

      if(get_str_elt(pagee, NAME, &name) != -1) {
        snprintf(buf, sizeof(buf), "You paged %s: %s", name, argtwo);
        notify_player(player, cause, player, buf, 0);
      } else
	notify_bad(player);
    } else
      notify_bad(player);
  }
}

VOID do_say(player, cause, switches, argone)
    int player, cause, switches;
    char *argone;
{
  char *name, buf[MEDBUFFSIZ];

  if (get_str_elt(player, NAME, &name) == -1) {
    notify_bad(player);
    return;
  }
  while (argone && *argone && isspace(*argone))
    argone++;

  snprintf(buf, sizeof(buf), "%s says, \"%s\"", name,
  	   (argone && *argone) ? argone : "");
  notify_oall(player, cause, player, buf, NOT_NOPUPPET | NOT_NOSPOOF);

  snprintf(buf, sizeof(buf), "You say, \"%s\"",
	   (argone && *argone) ? argone : "");
  notify_player(player, cause, player, buf, 0);
}

VOID do_whisper(player, cause, switches, argone, argtwo)
    int player, cause, switches;
    char *argone, *argtwo;
{
  int whisperee, here;
  char *name, buf[MEDBUFFSIZ];

  if ((get_int_elt(player, LOC, &here) == -1) || !exists_object(here)) {
    notify_player(player, cause, player, "You have no location!", 0);
    return;
  }
  if (!argone || !*argone) {
    notify_player(player, cause, player, "Whisper to whom?", 0);
    return;
  }
  whisperee = resolve_player(player, cause, argone, RSLV_NOISY);
  if(whisperee == -1)
    return;
  if (argtwo && *argtwo && (argtwo[0] == ':') && argtwo[1]) {
    argtwo++;

    if (get_str_elt(player, NAME, &name) == -1) {
      notify_bad(player);
      return;
    }
    snprintf(buf, sizeof(buf), "You sense that %s%s%s", name,
	     (argtwo[0] == '\'' || argtwo[0] == ',') ? "" : " ",
	     argtwo);
    notify_player(whisperee, cause, player, buf, NOT_NOSPOOF);
  } else {
    if (get_str_elt(player, NAME, &name) == -1) {
      notify_bad(player);
      return;
    }
    snprintf(buf, sizeof(buf), "%s whispers, \"%s\"", name,
	     (argtwo && *argtwo) ? argtwo : "");
    notify_player(whisperee, cause, player, buf, NOT_NOSPOOF);
  }
  if (get_str_elt(whisperee, NAME, &name) == -1) {
    notify_bad(player);
    return;
  }
  snprintf(buf, sizeof(buf), "You whisper, \"%s\" to %s.",
	   (argtwo && *argtwo) ? argtwo : "", name);
  notify_player(player, cause, player, buf, 0);
}

VOID do_wall(player, cause, switches, argone)
    int player, cause, switches;
    char *argone;
{
  char *name, *type, buf[MEDBUFFSIZ];
  int flags[FLAGS_LEN];

  if (!argone || !*argone) {
    notify_player(player, cause, player,
		  "But what do you want with the wall?", 0);
    return;
  }
  if (get_str_elt(player, NAME, &name) == -1) {
    notify_bad(player);
    return;
  }

  flags[0] = 0;
  if (switches & WALL_GOD) {
    type = "thinks,";
    flags[1] = GOD;
  } else if (switches & WALL_WIZ) {
    type = "broadcasts,";
    flags[1] = GOD | WIZARD;
  } else {
    type = "shouts,";
    flags[1] = 0;
  }
  if (switches & WALL_POSE) {
    snprintf(buf, sizeof(buf), "%s%s%s", name, ((argone[0] == ',') ||
	     (argone[0] == '\'') || (switches & WALL_NOSPACE)) ? "" : " ",
	     argone);
  } else if (switches & WALL_EMIT) {
    strcpy(buf, argone);
  } else {
    snprintf(buf, sizeof(buf), "%s %s \"%s\"", name, type, argone);
  }
  tcp_wall(flags, buf, -1);

  logfile(LOG_COMMAND, "WALL: %s(#%d)  \"%s\"\n", name, player, argone);
}

VOID do_gripe(player, cause, switches, argone)
    int player, cause, switches;
    char *argone;
{
  char *name, buf[MEDBUFFSIZ];
  int flags[FLAGS_LEN];

  if (!argone || !*argone) {
    notify_player(player, cause, player,
		  "What do you wish to gripe about?", 0);
    return;
  }

  if(get_str_elt(player, NAME, &name) == -1)
    name = "???";
  logfile(LOG_GRIPE, "GRIPE: From %s(#%d), \"%s\"\n", name, player, argone);

  snprintf(buf, sizeof(buf), "%s gripes, \"%s\"", name, argone);

  flags[0] = 0;
  flags[1] = WIZARD|GOD;
  tcp_wall(flags, buf, player);
  notify_player(player, cause, player,
		"Your complaint has been duly noted.", 0);
}

extern const char teenymud_version[];

VOID do_version(player, cause, switches)
    int player, cause, switches;
{
  char buf[BUFFSIZ];

  snprintf(buf, sizeof(buf), "This is TeenyMUD %s.", teenymud_version);
  notify_player(player, cause, player, buf, 0);
  notify_player(player, cause, player,
	"Copyright(C) 1993-1995, Jason Downs.  All rights reserved.", 0);

  if(switches & VERSION_FULL) {
    notify_player(player, cause, player, "Compile time options:", 0);

    buf[0] = '\0';
#ifdef USE_NDBM
    strcat(buf, "[NDBM] ");
#endif		/* USE_NDBM */
#ifdef USE_GDBM
    strcat(buf, "[GDBM] ");
#endif		/* USE_GDBM */
#ifdef USE_BSDDBM
    strcat(buf, "[BSD DBM] ");
#endif		/* USE_BSDDBM */
#ifdef USE_TERM
    strcat(buf, "[TERM] ");
#endif		/* USE_TERM */
    
    buf[strlen(buf)-1] = '\0';
    notify_player(player, cause, player, buf, 0);
  }
}
