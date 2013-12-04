/* buildcmds.c */

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
#include <ctype.h>
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif			/* HAVE_STDLIB_H */

#include "conf.h"
#include "teeny.h"
#include "commands.h"
#include "ptable.h"
#include "sha/sha_wrap.h"
#include "externs.h"

/* creation commands. */

static int copy_locks _ANSI_ARGS_((int, int));
static void link_object _ANSI_ARGS_((int, int, int, char *, int));
static int *link_exit _ANSI_ARGS_((int, int, int, int, char *));

static int copy_locks(obj, new)
    int obj, new;
{
  struct boolexp *lock;
  int aflags, idx;

  for(idx = 0; idx < LTABLE_TOTAL; idx++) {
    if(attr_getlock(obj, Ltable[idx].name, &lock, &aflags) == -1)
      return(-1);

    /* Don't clone NULL locks! */
    if(lock != (struct boolexp *)NULL) {
      if(attr_addlock(new, Ltable[idx].name, copy_boolexp(lock), aflags) == -1)
        return(-1);
    }
  }
  return(0);
}

VOID do_clone(player, cause, switches, argone, argtwo)
    int player, cause, switches;
    char *argone, *argtwo;
{
  int obj, new, num, home, here, owner, flags[FLAGS_LEN], nflags[FLAGS_LEN];
  char *match, buf[MEDBUFFSIZ];

  if (mudconf.enable_quota && no_quota(player, cause, (switches & CMD_QUIET)))
    return;
  if ((argone == (char *)NULL) || (argone[0] == '\0')) {
    if(!(switches & CMD_QUIET))
      notify_player(player, cause, player, "What do you want to clone?", 
                    NOT_QUIET);
    return;
  }
  for (match = argone; *match && *match != '/'; match++);
  if (*match == '/')
    *match++ = '\0';
  else
    match = (char *) NULL;
  obj = resolve_object(player, cause, argone,
  		       (!(switches & CMD_QUIET) ? RSLV_NOISY : 0));
  if(obj == -1)
    return;
  if (!isTHING(obj) || (!controls(player, cause, obj) && !isVISUAL(obj))) {
    if(!(switches & CMD_QUIET))
      notify_player(player, cause, player, "You can't clone that!", NOT_QUIET);
    return;
  }
  if ((argtwo != (char *)NULL) && argtwo[0] && !ok_name(argtwo)) {
    if(!(switches & CMD_QUIET))
      notify_player(player, cause, player,
		    "That's a silly name for an object!", NOT_QUIET);
    return;
  }
  if (!can_afford(player, cause, mudconf.thing_cost, (switches & CMD_QUIET)))
    return;
  new = create_obj(TYP_THING);
  if ((argtwo == (char *)NULL) || (argtwo[0] == '\0')) {
    if (get_str_elt(obj, NAME, &argtwo) == -1) {
      notify_bad(player);
      return;
    }
  }
  if ((set_str_elt(new, NAME, argtwo) == -1) ||
      (get_int_elt(player, OWNER, &owner) == -1) ||
      (set_int_elt(new, OWNER, owner) == -1) ||
      (get_int_elt(obj, PARENT, &num) == -1) ||
      (set_int_elt(new, PARENT, num) == -1) ||
      (get_int_elt(obj, HOME, &home) == -1) ||
      (set_int_elt(new, LOC, player) == -1)) {
    notify_bad(player);
    return;
  }
  if (!controls(player, cause, home) && !isABODE(home)) {
    if ((get_int_elt(player, LOC, &here) == -1) ||
	(get_int_elt(player, HOME, &num) == -1)) {
      notify_bad(player);
      return;
    }
    if (controls(player, cause, here) || isABODE(here))
      home = here;
    else
      home = num;
  }
  if ((set_int_elt(new, HOME, home) == -1) ||
      (get_flags_elt(obj, FLAGS, nflags) == -1) ||
      (get_flags_elt(new, FLAGS, flags) == -1)) {
    notify_bad(player);
    return;
  }

  /* process the new flags */
  nflags[0] &= ~TYPE_MASK;
  nflags[1] &= ~(INTERNAL_FLAGS | WIZARD | GOD);
  flags[0] |= nflags[0];
  flags[1] |= nflags[1];
  if (set_flags_elt(new, FLAGS, flags) == -1) {
    notify_bad(player);
    return;
  }

  if (copy_locks(obj, new) != 0) {
    notify_bad(player);
    return;
  }
  if (attr_copy(obj, new, match) != 0) {
    notify_bad(player);
    return;
  }
  list_add(new, player, CONTENTS);
  stamp(new, STAMP_MODIFIED);

  /* Noisy message. */
  if(!(switches & CMD_QUIET)) {
    snprintf(buf, sizeof(buf), "Object %s created with number #%d.",
	     argtwo, new);
    notify_player(player, cause, player, buf, NOT_QUIET);
  }

  snprintf(buf, sizeof(buf), "#%d", new);
  var_set(owner, "CREATE-RESULT", buf);

  hear_alert(cause, 0, new);
}

VOID do_create(player, cause, switches, argone)
    int player, cause, switches;
    char *argone;
{
  int obj, home, here, owner;
  char buf[MEDBUFFSIZ];

  if (mudconf.enable_quota && no_quota(player, cause, (switches & CMD_QUIET)))
    return;
  if ((argone == (char *)NULL) || (argone[0] == '\0')) {
    if(!(switches & CMD_QUIET))
      notify_player(player, cause, player, "You need to specify a name.", 
      		    NOT_QUIET);
    return;
  }
  if (!ok_name(argone)) {
    if(!(switches & CMD_QUIET))
      notify_player(player, cause, player,
		    "That's a silly name for an object!", NOT_QUIET);
    return;
  }
  /* Go to it */
  if (!can_afford(player, cause, mudconf.thing_cost, (switches & CMD_QUIET)))
    return;
  obj = create_obj(TYP_THING);
  if ((set_str_elt(obj, NAME, argone) == -1) ||
      (get_int_elt(player, OWNER, &owner) == -1) ||
      (set_int_elt(obj, OWNER, owner) == -1) ||
      (set_int_elt(obj, LOC, player) == -1) ||
      (get_int_elt(player, LOC, &here) == -1)) {
    notify_bad(player);
    return;
  }
  if (controls(player, cause, here) || isABODE(here)) {
    home = here;
  } else {
    if (get_int_elt(player, HOME, &home) == -1) {
      notify_bad(player);
      return;
    }
  }
  if ((set_int_elt(obj, HOME, home) == -1) ||
      (set_int_elt(obj, LOC, player) == -1)) {
    notify_bad(player);
    return;
  }
  list_add(obj, player, CONTENTS);
  stamp(obj, STAMP_MODIFIED);

  /* Noisy message. */
  if(!(switches & CMD_QUIET)) {
    snprintf(buf, sizeof(buf), "Object %s created with number #%d.",
	     argone, obj);
    notify_player(player, cause, player, buf, NOT_QUIET);
  }

  snprintf(buf, sizeof(buf), "#%d", obj);
  var_set(owner, "CREATE-RESULT", buf);
}

VOID do_dig(player, cause, switches, argone, argtwo)
    int player, cause, switches;
    char *argone, *argtwo;
{
  int room, loc, owner;
  int check_depth;
  char *ptr, buf[MEDBUFFSIZ];

  if (mudconf.enable_quota && no_quota(player, cause, (switches & CMD_QUIET)))
    return;
  if ((argone == (char *)NULL) || (argone[0] == '\0')) {
    if(!(switches & CMD_QUIET))
      notify_player(player, cause, player, "You need to specify a name.", 
      		    NOT_QUIET);
    return;
  }
  if (!ok_name(argone)) {
    if(!(switches & CMD_QUIET))
      notify_player(player, cause, player, "That's a silly name for a room!", 
      		    NOT_QUIET);
    return;
  }
  if (!argtwo || !*argtwo) {
    loc = mudconf.root_location;
  } else {
    if (!strcasecmp(argtwo, "root"))
      loc = mudconf.root_location;
    else {
      if (argtwo[0] == '#')
	argtwo++;
      loc = (int)strtol(argtwo, &ptr, 10);
      if(ptr == argtwo) {
        if(!(switches & CMD_QUIET))
	  notify_player(player, cause, player,
		        "You must specify the location by number.", NOT_QUIET);
	return;
      }
      if (!exists_object(loc)) {
        if(!(switches & CMD_QUIET))
	  notify_player(player, cause, player,
		        "That location does not exist.", NOT_QUIET);
	return;
      }
      if (!isROOM(loc)) {
        if(!(switches & CMD_QUIET))
	  notify_player(player, cause, player, "Illegal location.", NOT_QUIET);
	return;
      }
      if (!controls(player, cause, loc) && (loc != mudconf.root_location) &&
          !isABODE(loc)) {
	if(!(switches & CMD_QUIET))
	  notify_player(player, cause, player, "Permission denied.", NOT_QUIET);
	return;
      }
      if (get_int_elt(loc, LOC, &room) == -1) {
	notify_bad(player);
	return;
      }
      check_depth = 0;		/* for legal_roomloc_check */
      if (!legal_roomloc_check(loc, room, &check_depth)) {
        if(!(switches & CMD_QUIET))
	  notify_player(player, cause, player, "Illegal location.", NOT_QUIET);
	return;
      }
    }
  }
  if (!can_afford(player, cause, mudconf.room_cost, (switches & CMD_QUIET)))
    return;
  room = create_obj(TYP_ROOM);
  if ((get_int_elt(player, OWNER, &owner) == -1) ||
      (set_int_elt(room, OWNER, owner) == -1) ||
      (set_str_elt(room, NAME, argone) == -1) ||
      (set_int_elt(room, LOC, loc) == -1)) {
    notify_bad(player);
    return;
  }
  list_add(room, loc, ROOMS);

  stamp(room, STAMP_MODIFIED);

  /* Noisy message. */
  if(!(switches & CMD_QUIET)) {
    snprintf(buf, sizeof(buf), "Room %s created with number #%d.",
             argone, room);
    notify_player(player, cause, player, buf, NOT_QUIET);
  }

  snprintf(buf, sizeof(buf), "#%d", room);
  var_set(owner, "CREATE-RESULT", buf);
}

VOID do_palias(player, cause, switches, argone, argtwo)
    int player, cause, switches;
    char *argone, *argtwo;
{
  int obj;

  if((argone == (char *)NULL) || (argone[0] == '\0')) {
    if(!(switches & CMD_QUIET))
      notify_player(player, cause, player, "Yes, and?", NOT_QUIET);
    return;
  }

  obj = resolve_player(player, cause, argone,
  		       (!(switches & CMD_QUIET) ? RSLV_NOISY : 0));
  if(obj == -1)
    return;

  if(!controls(player, cause, obj) || !isPLAYER(obj)) {
    if(!(switches & CMD_QUIET))
      notify_player(player, cause, player, "You can't alias that.", NOT_QUIET);
    return;
  }

  if((argtwo == (char *)NULL) || (argtwo[0] == '\0')) {
    char *alias;
    int aflags;

    if(attr_get(obj, ALIAS, &alias, &aflags) == -1) {
      notify_bad(player);
      return;
    }
    if((alias != (char *)NULL) && alias[0]) {
      ptable_alias(obj, alias, 1);

      if(attr_delete(obj, ALIAS) == -1) {
        notify_bad(player);
        return;
      }
      if(!(switches & CMD_QUIET))
        notify_player(player, cause, player, "Player alias removed.",
		      NOT_QUIET);
    } else
      if(!(switches & CMD_QUIET))
	notify_player(player, cause, player, "But you don't have an alias!",
		      NOT_QUIET);
  } else {
    if(!ok_player_name(argtwo)) {
      if(!(switches & CMD_QUIET))
	notify_player(player, cause, player, "That's not a good alias.",
		      NOT_QUIET);
      return;
    }
    if(attr_add(obj, ALIAS, argtwo, ALIAS_FLGS) == -1) {
      notify_bad(player);
      return;
    }
    ptable_alias(obj, argtwo, 0);
    if(!(switches & CMD_QUIET))
      notify_player(player, cause, player, "Player alias set.", NOT_QUIET);
  }
}

VOID do_name(player, cause, switches, argone, argtwo)
    int player, cause, switches;
    char *argone, *argtwo;
{
  int obj, aflags;
  char *name = (char *)NULL, *pwd = (char *)NULL;
  char *realpwd, *oldname, buf[MEDBUFFSIZ];

  if ((argone == (char *)NULL) || (argone[0] == '\0')) {
    if(!(switches & CMD_QUIET))
      notify_player(player, cause, player, "Change the name of what?",
      		    NOT_QUIET);
    return;
  }
  if ((argtwo == (char *)NULL) || (argone[0] == '\0')) {
    if(!(switches & CMD_QUIET))
      notify_player(player, cause, player, "You must specify a new name.",
    		    NOT_QUIET);
    return;
  }
  obj = resolve_object(player, cause, argone,
  		       (!(switches & CMD_QUIET) ? RSLV_EXITS|RSLV_NOISY
		       			        : RSLV_EXITS));
  if(obj == -1)
    return;
  if (!controls(player, cause, obj)) {
    if(!(switches & CMD_QUIET))
      notify_player(player, cause, player, "You can't do that!", NOT_QUIET);
    return;
  }

  switch (Typeof(obj)) {
  case TYP_EXIT:
    if (!ok_exit_name(argtwo)) {
      if(!(switches & CMD_QUIET))
        notify_player(player, cause, player,
		      "That's a silly name for an exit!", NOT_QUIET);
      return;
    }
    break;
  case TYP_PLAYER:
    if (mudconf.registration && mudconf.registername && !isWIZARD(player)) {
      if(!(switches & CMD_QUIET))
        notify_player(player, cause, player,
		      "Player names may not be changed right now.", NOT_QUIET);
      return;
    }
    if (parse_name_pwd(argtwo, &name, &pwd) != -1) {
      if (attr_get(obj, PASSWORD, &realpwd, &aflags) == -1) {
        notify_bad(player);
        return;
      }
      if ((realpwd == (char *)NULL) || (realpwd[0] == '\0')) {
        if(!(switches & CMD_QUIET))
          notify_player(player, cause, player,
		        "That player's name may not be changed.", NOT_QUIET);
        return;
      }
      if (((pwd == (char *)NULL) || (pwd[0] == '\0')) && !isGOD(player)) {
        if(!(switches & CMD_QUIET))
          notify_player(player, cause, player,
		        "You must specify a password.", NOT_QUIET);
        return;
      }
      if ((pwd != (char *)NULL) && (pwd[0] != '\0')
	  && !comp_password(realpwd, pwd)) {
        if(!(switches & CMD_QUIET))
          notify_player(player, cause, player, "Incorrect password.",
	  		NOT_QUIET);
        return;
      }
      if (get_str_elt(obj, NAME, &oldname) == -1) {
        notify_bad(player);
        return;
      }
      if (strcasecmp(oldname, name) && !ok_player_name(name)) {
        if(!(switches & CMD_QUIET))
          notify_player(player, cause, player,
		        "You can't give a player that name.", NOT_QUIET);
        return;
      }

      logfile(LOG_STATUS, "NAME CHANGE: %s(#%d) is now known as %s.\n",
	      oldname, obj, name);
      argtwo = name;
    } else {
      if(!(switches & CMD_QUIET))
        notify_player(player, cause, player, 
		      "Couldn't parse that name and password.", NOT_QUIET);
      return;
    }
    break;
  default:
    if (!ok_name(argtwo)) {
      if(!(switches & CMD_QUIET))
        notify_player(player, cause, player, "That's a silly name!", NOT_QUIET);
      return;
    }
  }
  if (set_str_elt(obj, NAME, argtwo) == -1)
    notify_bad(player);
  else {
    /* Noisy message. */
    if(!(switches & CMD_QUIET))
      notify_player(player, cause, player, "Name changed.", NOT_QUIET);

    stamp(obj, STAMP_MODIFIED);
  }

  /* Noisy message. */
  if (can_hear(obj) && (obj != player) && !(switches & CMD_QUIET)) {
    snprintf(buf, sizeof(buf), "Your name has been changed to \"%s\".",
	     argtwo);
    notify_player(obj, cause, player, buf, NOT_QUIET);
  }
}

VOID do_parent(player, cause, switches, argone, argtwo)
    int player, cause, switches;
    char *argone, *argtwo;
{
  int obj, parent;
  int depth = 0;

  if ((argone == (char *)NULL) || (argone[0] == '\0')) {
    if(!(switches & CMD_QUIET))
      notify_player(player, cause, player, "Set the parent of what?",
      		    NOT_QUIET);
    return;
  }
  obj = resolve_object(player, cause, argone,
  		       (!(switches & CMD_QUIET) ? RSLV_EXITS|RSLV_NOISY
		       				: RSLV_EXITS));
  if(obj == -1)
    return;
  if (!controls(player, cause, obj)) {
    if(!(switches & CMD_QUIET))
      notify_player(player, cause, player, "Permission denied.", NOT_QUIET);
    return;
  }
  if ((argtwo == (char *)NULL) || (argtwo[0] == '\0'))
    parent = -1;
  else {
    parent = resolve_object(player, cause, argtwo,
    			    (!(switches & CMD_QUIET) ? RSLV_EXITS|RSLV_NOISY
			    			     : RSLV_EXITS));
    if(parent == -1)
      return;
    if (!controls(player, cause, parent) && !isPARENT_OK(parent)) {
      if(!(switches & CMD_QUIET))
        notify_player(player, cause, player, "Permission denied.", 0);
      return;
    }
    if (!legal_parent_check(obj, parent, &depth)) {
      if(!(switches & CMD_QUIET))
        notify_player(player, cause, player, "That's not a legal parent.", 
		      NOT_QUIET);
      return;
    }
  }
  if (set_int_elt(obj, PARENT, parent) == -1) {
    notify_bad(player);
    return;
  }

  /* Noisy message. */
  if(!(switches & CMD_QUIET))
    notify_player(player, cause, player,
  		  (parent == -1) ? "Parent unset." : "Parent set.", NOT_QUIET);
}

VOID do_open(player, cause, switches, argone, argtwo)
    int player, cause, switches;
    char *argone, *argtwo;
{
  int here, exit;
  int source, owner;
  char *p, buf[MEDBUFFSIZ];

  if ((get_int_elt(player, LOC, &here) == -1) || !exists_object(here)) {
    if(!(switches & CMD_QUIET))
      notify_player(player, cause, player, "You have no location!", NOT_QUIET);
    return;
  }
  if (mudconf.enable_quota && no_quota(player, cause, (switches & CMD_QUIET)))
    return;
  if ((argone == (char *)NULL) || (argone[0] == '\0')) {
    notify_player(player, cause, player, "You must specify a name.", 0);
    return;
  }
  if (!ok_exit_name(argone) && !isGOD(player)) {
    if(!(switches & CMD_QUIET))
      notify_player(player, cause, player,
		    "That's a silly name for an exit!", NOT_QUIET);
    return;
  }
  /* parse for a source object */
  for (p = argone; *p && *p != '/'; p++);
  if (*p == '/') {		/* found one */
    *p++ = 0;			/* terminate argone */
    if (*p) {
      source = resolve_object(player, cause, p, 0);
      if(source == -1) {
        if(!(switches & CMD_QUIET))
	  notify_player(player, cause, player, "I can't find that source.", 
	  		NOT_QUIET);
	return;
      }
      if (source == -2) {
        if(!(switches & CMD_QUIET))
	  notify_player(player, cause, player,
		        "I can't tell which source you mean.", NOT_QUIET);
	return;
      }
      if (!exists_object(source) || !controls(player, cause, source) ||
	  !has_exits(source)) {
	if(!(switches & CMD_QUIET))
	  notify_player(player, cause, player, 
		        "You can't open an exit on that!", NOT_QUIET);
	return;
      }
    } else {
      if(!(switches & CMD_QUIET))
        notify_player(player, cause, player, "You need to specify a source.", 
		      NOT_QUIET);
      return;
    }
  } else {
    source = here;
    if ((!controls(player, cause, source) && !isBUILDING_OK(source)) ||
	!has_exits(source)) {
      if(!(switches & CMD_QUIET))
        notify_player(player, cause, player, "You can't open an exit here!", 
		      NOT_QUIET);
      return;
    }
  }
  if (!can_afford(player, cause, mudconf.exit_cost, (switches & CMD_QUIET)))
    return;
  exit = create_obj(TYP_EXIT);
  if ((set_str_elt(exit, NAME, argone) == -1) ||
      (get_int_elt(player, OWNER, &owner) == -1) ||
      (set_int_elt(exit, OWNER, owner) == -1) ||
      (set_int_elt(exit, LOC, source) == -1)) {
    notify_bad(player);
    return;
  }
  list_add(exit, source, EXITS);

  stamp(exit, STAMP_MODIFIED);

  /* Noisy message. */
  if(!(switches & CMD_QUIET)) {
    snprintf(buf, sizeof(buf), "Exit %s opened with number #%d.",
             argone, exit);
    notify_player(player, cause, player, buf, NOT_QUIET);
  }

  snprintf(buf, sizeof(buf), "#%d", exit);
  var_set(owner, "CREATE-RESULT", buf);

  if ((argtwo != (char *)NULL) && argtwo[0]) {	/* Try to link this */
    link_object(player, cause, exit, argtwo, switches);
  }
}

VOID do_link(player, cause, switches, argone, argtwo)
    int player, cause, switches;
    char *argone, *argtwo;
{
  int thing;

  if ((argone == (char *)NULL) || (argone[0] == '\0')
      || (argtwo == (char *)NULL) || (argtwo[0] == '\0')) {
    if(!(switches & CMD_QUIET))
      notify_player(player, cause, player, "Link what to where?", NOT_QUIET);
    return;
  }

  thing = resolve_object(player, cause, argone,
  			 (!(switches & CMD_QUIET) ? RSLV_EXITS|RSLV_NOISY
			 			  : RSLV_EXITS));
  if(thing == -1)
    return;

  /* this does everything else. */
  link_object(player, cause, thing, argtwo, switches);
}

/* common code for do_open() and do_link(). */
static void link_object(player, cause, thing, str, switches)
    int player, cause, thing, switches;
    char *str;
{
  char *msg;
  int dest, *dest2, exowner, code, owner;

  switch (Typeof(thing)) {
  case TYP_PLAYER:
  case TYP_THING:
    if(!controls(player, cause, thing)) {
      if(!(switches & CMD_QUIET))
        notify_player(player, cause, player, "You can't link that!", NOT_QUIET);
      return;
    }

    dest = resolve_object(player, cause, str,
    			  (!(switches & CMD_QUIET) ? RSLV_NOISY : 0));
    if (dest == -1)
      return;

    if ((dest == -3) || (thing == dest)) {
      if(!(switches & CMD_QUIET))
        notify_player(player, cause, player, "Paradox in link of object.", 
		      NOT_QUIET);
      return;
    }
    if (!controls(player, cause, dest) && !isABODE(dest)) {
      if(!(switches & CMD_QUIET))
        notify_player(player, cause, player, "You can't link that there!", 
		      NOT_QUIET);
      return;
    }
    msg = "Home set.";
    code = HOME;
    break;
  case TYP_ROOM:
    if(!controls(player, cause, thing)) {
      if(!(switches & CMD_QUIET))
        notify_player(player, cause, player, "You can't link that!", NOT_QUIET);
      return;
    }

    dest = resolve_object(player, cause, str,
    			  (!(switches & CMD_QUIET) ? RSLV_NOISY|RSLV_HOME
			  			   : RSLV_HOME));
    if(dest == -1)
      return;

    if ((dest != -3) && !controls(player, cause, dest) && !isLINK_OK(dest)) {
      if(!(switches & CMD_QUIET))
        notify_player(player, cause, player, "You can't link that there!", 
		      NOT_QUIET);
      return;
    }
    msg = "Dropto set.";
    code = DROPTO;
    break;
  case TYP_EXIT:
    if(!(switches & CMD_QUIET))
      notify_player(player, cause, player, "Trying to link...", NOT_QUIET);

    /* Check that the exit is unlinked */
    if (get_array_elt(thing, DESTS, &dest2) == -1) {
      notify_bad(player);
      return;
    }
    if ((dest2 != (int *)NULL) && !controls(player, cause, thing)) {
      if(!(switches & CMD_QUIET))
        notify_player(player, cause, player, "That exit is linked.", NOT_QUIET);
      return;
    } else {
      if((dest2 == (int *)NULL) && !controls(player, cause, thing)
	 && islocked(player, cause, thing, LOCK)) {
	if(!(switches & CMD_QUIET))
	  notify_player(player, cause, player, "You can't link that!",
			NOT_QUIET);
	return;
      }

      if (get_int_elt(thing, OWNER, &exowner) != -1) {
	if (exowner != player) {

	  if (!isBUILDER(player) && !isWIZARD(player)) {
	    if(!(switches & CMD_QUIET))
	      notify_player(player, cause, player,
	    		    "Only authorized builders may do that.", NOT_QUIET);
	    return;
	  }
	  if (!(switches & LINK_NOCHOWN)) {
	    if (mudconf.enable_quota
	        && no_quota(player, cause, (switches & CMD_QUIET)))
	      return;
	    if (!can_afford(player, cause, mudconf.exit_cost,
	    		    (switches & CMD_QUIET)))
	      return;
	    reward_money(exowner, mudconf.exit_cost);
	  }
	}
      }
    }
    if (switches != LINK_NOCHOWN) {
      if ((get_int_elt(player, OWNER, &owner) == -1) ||
	  (set_int_elt(thing, OWNER, owner) == -1)) {
	notify_bad(player);
	return;
      }
    }

    dest2 = link_exit(player, cause, thing, switches, str);
    if(set_array_elt(thing, DESTS, dest2) == -1)
      notify_bad(player);
    return;
  default:
    if(!(switches & CMD_QUIET))
      notify_player(player, cause, player, "I don't understand that.",
      		    NOT_QUIET);
    logfile(LOG_ERROR, "do_link: bad type on object #%d\n", thing);
    return;
  }

  /* Link it up. */
  if (set_int_elt(thing, code, dest) != -1) {
    /* Noisy message. */
    if(!(switches & CMD_QUIET))
      notify_player(player, cause, player, msg, NOT_QUIET);
  } else
    notify_bad(player);
}

static int *link_exit(player, cause, exit, switches, str)
    int player, cause, exit, switches;
    char *str;
{
  static int array[MAXLINKS+1];
  register char *ptr;
  char buf[MEDBUFFSIZ];
  register int indx = 0;
  int dest, prdest = 0, exit_depth;

  while(str && *str) {
    while(*str && isspace(*str))
      str++;
    for(ptr = str; *ptr && (*ptr != ';'); ptr++);
    if(*ptr != '\0')
      *ptr++ = '\0';

    dest = resolve_object(player, cause, str,
    			  (!(switches & CMD_QUIET)
			   ? RSLV_NOISY|RSLV_EXITS|RSLV_HOME
			   : RSLV_EXITS|RSLV_HOME));
    if(dest == -1) {
      str = ptr;
      continue;
    }

    if(dest != -3) {
      if (!controls(player, cause, dest) && !isLINK_OK(dest)) {
        if(!(switches & CMD_QUIET))
        notify_player(player, cause, player, "You can't link to that!",
		      NOT_QUIET);
        str = ptr;
	continue;
      }

      switch(Typeof(dest)) {
      case TYP_PLAYER:
      case TYP_ROOM:
        if (prdest) {
	  /* Noisy message. */
	  if(!(switches & CMD_QUIET)) {
            snprintf(buf, sizeof(buf),
   "Only one player or room destination allowed.  Destination \"%s\" ignored.",
 		     str);
	    notify_player(player, cause, player, buf, NOT_QUIET);
	  }

	  str = ptr;
	  continue;
        } else {
          array[++indx] = dest;
          prdest = 1;
	}
        break;
      case TYP_THING:
        array[++indx] = dest;
        break;
      case TYP_EXIT:
        exit_depth = 0;

        if (!legal_recursive_exit(exit, dest, &exit_depth)) {
	  /* Noisy message. */
	  if(!(switches & CMD_QUIET)) {
            snprintf(buf, sizeof(buf),
		     "Destination \"%s\" would create a loop, ignored.", str);
	    notify_player(player, cause, player, buf, NOT_QUIET);
	  }

	  str = ptr;
	  continue;
        } else
          array[++indx] = dest;
        break;
      }
    } else {		/* HOME */
      if (prdest) {
        /* Noisy message. */
	if(!(switches & CMD_QUIET)) {
          snprintf(buf, sizeof(buf),
  "Only one player or room destination allowed.  Destination \"%s\" ignored.",
                   str);
          notify_player(player, cause, player, buf, NOT_QUIET);
	}

	str = ptr;
        continue;                          
      } else {
        array[++indx] = dest;
        prdest = 1;        
      }
    }
    
    /* Noisy messages. */
    if (!(switches & CMD_QUIET)) {
      if (dest == -3) {
        notify_player(player, cause, player, "Linked to *HOME*.", NOT_QUIET);
      } else {
        snprintf(buf, sizeof(buf), "Linked to %s.",
	         display_name(player, cause, dest));
        notify_player(player, cause, player, buf, NOT_QUIET);
      }
    }
    if (indx >= MAXLINKS) {
      if(!(switches & CMD_QUIET))
        notify_player(player, cause, player,
		      "Too many destinations, rest ignored.", NOT_QUIET);
      break;
    }

    str = ptr;
  }

  if(indx == 0) {
    return((int *)NULL);
  }

  array[0] = indx;
  return(array);
}

VOID do_unlink(player, cause, switches, argone)
    int player, cause, switches;
    char *argone;
{
  int obj;

  if ((argone == (char *)NULL) || (argone[0] == '\0')) {
    if (!(switches & CMD_QUIET))
      notify_player(player, cause, player, "Unlink what?", NOT_QUIET);
    return;
  }
  obj = resolve_object(player, cause, argone,
  		       (!(switches & CMD_QUIET) ? RSLV_EXITS|RSLV_NOISY
		       				: RSLV_EXITS));
  if(obj == -1)
    return;
  if (!controls(player, cause, obj) && !econtrols(player, cause, obj)) {
    if(!(switches & CMD_QUIET))
      notify_player(player, cause, player, "You can't unlink that!", NOT_QUIET);
    return;
  }

  switch (Typeof(obj)) {
  case TYP_PLAYER:
  case TYP_THING:
    if(!(switches & CMD_QUIET))
      notify_player(player, cause, player,
		    "You can't unset an object's home!", NOT_QUIET);
    return;
  case TYP_EXIT:
    if((set_array_elt(obj, DESTS, (int *)NULL) != -1)
       && !(switches & CMD_QUIET)) {
      notify_player(player, cause, player, "Exit unlinked.", NOT_QUIET);
    } else
      notify_bad(player);
    return;
  case TYP_ROOM:
    if((set_int_elt(obj, DROPTO, -1) != -1) && !(switches & CMD_QUIET)) {
      notify_player(player, cause, player, "Dropto unset.", NOT_QUIET);
    } else
      notify_bad(player);
    return;
  default:
    if(!(switches & CMD_QUIET))
      notify_player(player, cause, player, "I don't understand that.", 
      		    NOT_QUIET);
    return;
  }
}

VOID do_unlock(player, cause, switches, argone)
    int player, cause, switches;
    char *argone;
{
  int obj;
  char *ptr, *attr;
  struct boolexp *rattr;
  int rflags;
  register int idx;

  if ((argone == (char *)NULL) || (argone[0] == '\0')) {
    if(!(switches & CMD_QUIET))
      notify_player(player, cause, player, "Unlock what?", NOT_QUIET);
    return;
  }

  attr = (char *)NULL;
  for(ptr = argone; (*ptr != '\0') && (*ptr != '/'); ptr++);
  if(*ptr == '/') {
    *ptr++ = '\0';

    for(idx = 0; idx < LTABLE_TOTAL; idx++) {
      if(!strncasecmp(Ltable[idx].name, ptr, Ltable[idx].matlen)) {
        attr = Ltable[idx].name;
	break;
      }
    }
  } else
    attr = LOCK;

  if (attr == (char *)NULL) {
    if(!(switches & CMD_QUIET))
      notify_player(player, cause, player, "No such lock.", NOT_QUIET);
    return;
  }
  obj = resolve_object(player, cause, argone,
  		       (!(switches & CMD_QUIET) ? RSLV_EXITS|RSLV_NOISY
		       				: RSLV_EXITS));
  if(obj == -1)
    return;
  if (attr_getlock(obj, attr, &rattr, &rflags) == -1) {
    notify_bad(player);
    return;
  }
  if(rattr != (struct boolexp *)NULL) {
    if (!can_set_attr(player, cause, obj, rflags)) {
      if(!(switches & CMD_QUIET))
      notify_player(player, cause, player, "You can't unlock that!", NOT_QUIET);
      return;
    }
    if (attr_delete(obj, attr) == -1) {
      notify_bad(player);
      return;
    }
  }

  /* Noisy message. */
  if(!(switches & CMD_QUIET))
    notify_player(player, cause, player, "Unlocked.", NOT_QUIET);
}

VOID do_lock(player, cause, switches, argone, argtwo)
    int player, cause, switches;
    char *argone, *argtwo;
{
  int obj;
  struct boolexp *exp;
  char *ptr, *attr;
  register int idx;

  if ((argone == (char *)NULL) || (argone[0] == '\0')) {
    if(!(switches & CMD_QUIET))
      notify_player(player, cause, player, "Lock what to what?", NOT_QUIET);
    return;
  }

  attr = (char *)NULL;
  for(ptr = argone; (*ptr != '\0') && (*ptr != '/'); ptr++);
  if(*ptr == '/') {
    *ptr++ = '\0';

    for(idx = 0; idx < LTABLE_TOTAL; idx++) {
      if(!strncasecmp(Ltable[idx].name, ptr, Ltable[idx].matlen)) {
        attr = Ltable[idx].name;
	break;
      }
    }
  } else
    attr = LOCK;

  if (attr == (char *)NULL) {
    if(!(switches & CMD_QUIET))
      notify_player(player, cause, player, "No such lock.", NOT_QUIET);
    return;
  }
  obj = resolve_object(player, cause, argone,
  		       (!(switches & CMD_QUIET) ? RSLV_EXITS|RSLV_NOISY
		       				: RSLV_EXITS));
  if(obj == -1)
    return;
  if(!can_set_attr(player, cause, obj, DEFLOCK_FLGS) == -1) {
    if(!(switches & CMD_QUIET))
      notify_player(player, cause, player, "You can't do that!", NOT_QUIET);
    return;
  }

  exp = parse_boolexp(player, cause, argtwo);
  if(exp == (struct boolexp *)NULL) {
    if(!(switches & CMD_QUIET))
      notify_player(player, cause, player, "I don't understand that lock.", 
      		    NOT_QUIET);
    return;
  }

  if (attr_addlock(obj, attr, exp, DEFLOCK_FLGS) == -1) {
    notify_bad(player);
    return;
  }

  /* Noisy message. */
  if(!(switches & CMD_QUIET))
    notify_player(player, cause, player, "Locked.", NOT_QUIET);
}

VOID do_copy(player, cause, switches, argone, argtwo)
    int player, cause, switches;
    char *argone, *argtwo;
{
  int source, dest, aflags;
  char *satr, *datr, *sdat;
  char *p, buf[MEDBUFFSIZ];

  if ((argone == (char *)NULL) || (argone[0] == '\0')
      || (argtwo == (char *)NULL) || (argtwo[0] == '\0')) {
    if(!(switches & CMD_QUIET))
      notify_player(player, cause, player, "Copy what to what?", NOT_QUIET);
    return;
  }
  /* parse the first two arguments */
  for (satr = argone; *satr && *satr != '/'; satr++);
  if (*satr == '/') {
    *satr++ = 0;
    source = resolve_object(player, cause, argone, RSLV_EXITS);
    if(source == -1) {
      notify_player(player, cause, player, "I can't find that source.", 0);
      return;
    } else if (source == -2) {
      if(!(switches & CMD_QUIET))
        notify_player(player, cause, player,
		      "I can't tell which source you mean.", NOT_QUIET);
      return;
    }
  } else {
    if(!(switches & CMD_QUIET))
      notify_player(player, cause, player, 
		    "You must specify a source attribute.", NOT_QUIET);
    return;
  }
  if (!controls(player, cause, source)) {
    if(!(switches & CMD_QUIET))
      notify_player(player, cause, player, "Permission denied.", NOT_QUIET);
    return;
  }
  /* parse the second two arguments */
  for (datr = argtwo; *datr && *datr != '/'; datr++);
  if (*datr == '/') {
    *datr++ = 0;
    dest = resolve_object(player, cause, argtwo, RSLV_EXITS);
    if(dest == -1) {
      if(!(switches & CMD_QUIET))
        notify_player(player, cause, player,
		      "I can't find that destination.", NOT_QUIET);
      return;
    } else if (dest == -2) {
      if(!(switches & CMD_QUIET))
        notify_player(player, cause, player,
      		      "I can't tell which destination you mean.", NOT_QUIET);
      return;
    }
  } else {
    if(!(switches & CMD_QUIET))
      notify_player(player, cause, player,
    		    "You must specify a destination attribute.", NOT_QUIET);
    return;
  }
  if (!controls(player, cause, dest)) {
    if(!(switches & CMD_QUIET))
      notify_player(player, cause, player, "Permission denied.", NOT_QUIET);
    return;
  }
  if (attr_get(dest, datr, &p, &aflags) == -1) {
    notify_bad(player);
    return;
  }
  if (p && *p && !can_set_attr(player, cause, dest, aflags)) {
    if(!(switches & CMD_QUIET)) {
      snprintf(buf, sizeof(buf), "%s(#%d): Permission denied.", datr, dest);
      notify_player(player, cause, player, buf, NOT_QUIET);
    }
    return;
  }
  if (attr_get(source, satr, &sdat, &aflags) == -1) {
    notify_bad(player);
    return;
  }
  if ((sdat == (char *)NULL) || (sdat[0] == '\0')) {
    if(!(switches & CMD_QUIET))
      notify_player(player, cause, player, "Nothing to do.", NOT_QUIET);
    return;
  }
  if (!can_see_attr(player, cause, source, aflags)
      || ((switches & COPY_MOVE)
          && !can_set_attr(player, cause, source, aflags))) {
    if(!(switches & CMD_QUIET)) {
      snprintf(buf, sizeof(buf), "%s(#%d): Permission denied.", satr, source);
      notify_player(player, cause, player, buf, NOT_QUIET);
    }
    return;
  }
  if (attr_add(dest, datr, sdat, aflags) == -1) {
    notify_bad(player);
    return;
  }
  if (switches & COPY_MOVE) {
    if (attr_delete(source, satr) == -1) {
      notify_bad(player);
      return;
    }
  }
  stamp(dest, STAMP_MODIFIED);

  /* Noisy message. */
  if(!(switches & CMD_QUIET)) {
    snprintf(buf, sizeof(buf), "%s set.", datr);
    notify_player(player, cause, player, buf, NOT_QUIET);
  }
}
