/* set.c */

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

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif			/* HAVE_STDLIB_H */
#include <stdio.h>
#include <sys/types.h>
#ifdef HAVE_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif			/* HAVE_STRING_H */
#include <ctype.h>

#include "conf.h"
#include "flaglist.h"
#include "teeny.h"
#include "commands.h"
#include "sha/sha_wrap.h"
#include "externs.h"

/* commands dealing with setting things. */

static void set_attrflags _ANSI_ARGS_((int player, int cause, int switches,
				       int obj, char *name, char *flags));

VOID do_charge(player, cause, switches, argone, argtwo)
    int player, cause, switches;
    char *argone, *argtwo;
{
  int obj, charges;
  char *ptr;

  if((argone == (char *)NULL) || (argone[0] == '\0')) {
    if(!(switches & CMD_QUIET))
      notify_player(player, cause, player, "Charge what?", NOT_QUIET);
    return;
  }
  obj = resolve_object(player, cause, argone,
  		       (!(switches & CMD_QUIET) ? RSLV_EXITS|RSLV_NOISY
		       				: RSLV_EXITS));
  if(obj == -1)
    return;
  if(!controls(player, cause, obj)) {
    if(!(switches & CMD_QUIET))
      notify_player(player, cause, player, "Permission denied.", NOT_QUIET);
    return;
  }

  if((argtwo == (char *)NULL) || (argtwo[0] == '\0')) {
    charges = -1;
  } else {
    charges = (int)strtol(argtwo, &ptr, 10);
    if(ptr == argtwo) {
      if(!(switches & CMD_QUIET))
        notify_player(player, cause, player, "Numerical argument required.",
		      NOT_QUIET);
      return;
    }
    if(charges < -1)
      charges = -1;
  }

  if(set_int_elt(obj, CHARGES, charges) == -1)
    notify_bad(player);
  else if(!(switches & CMD_QUIET)) {
    switch(charges) {
    case -1:
      notify_player(player, cause, player, "Charged removed.", NOT_QUIET);
      break;
    case 0:
      notify_player(player, cause, player, "Charge emptied.", NOT_QUIET);
      break;
    default:
      notify_player(player, cause, player, "Charge set.", NOT_QUIET);
      break;
    }
  }
}

VOID do_set(player, cause, switches, argone, argtwo)
    int player, cause, switches;
    char *argone, *argtwo;
{
  int obj, unset, objflags[FLAGS_LEN], ohear, idx;
  char *msg, *name, *data;
  FlagList *match = NULL;

  if ((argtwo == (char *)NULL) || (argtwo[0] == '\0')) {
    if(!(switches & CMD_QUIET))
      notify_player(player, cause, player,"Set what?", NOT_QUIET);
    return;
  }
  obj = resolve_object(player, cause, argone,
  		       (!(switches & CMD_QUIET) ? RSLV_EXITS|RSLV_NOISY
		       				: RSLV_EXITS));
  if(obj == -1)
    return;
  if (!exists_object(obj) || !controls(player, cause, obj)) {
    if(!(switches & CMD_QUIET))
      notify_player(player, cause, player, "You can't set anything on that!",
      		    NOT_QUIET);
    return;
  }
  /* Okie. What's this doof want to set? */
  /* attributes */
  if ((data = strchr(argtwo, ':')) != (char *) NULL) {
    for (name = (data - 1); name >= argtwo && isspace(*name); name--);
    name[1] = 0;
    data++;

    while (*data && isspace(*data))
      data++;
    set_attribute(player, cause, switches, obj, argtwo, data);
    return;
  } else if ((data = strchr(argtwo, '/')) != (char *) NULL) {
    for (name = (data - 1); name >= argtwo && isspace(*name); name--);
    name[1] = 0;
    data++;

    while (*data && isspace(*data))
      data++;
    set_attrflags(player, cause, switches, obj, argtwo, data);
    return;
  }
  /* object flags */
  if (*argtwo == '!') {
    unset = 1;
    argtwo++;
  } else
    unset = 0;

  /* GenFlags */
  idx = match_flaglist_name(argtwo, GenFlags, GenFlags_count);
  if (idx < 0) {
    switch(Typeof(obj)) {
    case TYP_PLAYER:
      /* PlayerFlags */
      idx = match_flaglist_name(argtwo, PlayerFlags, PlayerFlags_count);
      if (idx >= 0)
	match = &PlayerFlags[idx];
      break;
    case TYP_ROOM:
      /* RoomFlags */
      idx = match_flaglist_name(argtwo, RoomFlags, RoomFlags_count);
      if (idx >= 0)
	match = &RoomFlags[idx];
      break;
    case TYP_THING:
      /* ThingFlags */
      idx = match_flaglist_name(argtwo, ThingFlags, ThingFlags_count);
      if (idx >= 0)
	match = &ThingFlags[idx];
      break;
    case TYP_EXIT:
      /* ExitFlags */
      idx = match_flaglist_name(argtwo, ExitFlags, ExitFlags_count);
      if (idx >= 0)
	match = &ExitFlags[idx];
      break;
    }
  } else
    match = &GenFlags[idx];

  if (match == NULL) {
    if(!(switches & CMD_QUIET))
      notify_player(player, cause, player, "I don't understand that flag.",
      		    NOT_QUIET);
    return;
  }

  /* permission checks */
  if (((match->perm & PERM_GOD) && !isGOD(player))
      || ((match->perm & PERM_WIZ) && !isWIZARD(player))
      || (match->perm & PERM_INTERNAL)) {
    if(!(switches & CMD_QUIET))
      notify_player(player, cause, player, "Permission denied.", NOT_QUIET);
    return;
  }
  /* special permissions */
  if ((obj == mudconf.player_god) && unset && (match->code == GOD)
      && (match->word == 1)) {
    if(!(switches & CMD_QUIET))
      notify_player(player, cause, player, "That player is always God.",
      		    NOT_QUIET);
    return;
  }
  if ((match->perm & PERM_DARK) && !isWIZARD(player) && isTHING(obj)) {
    int loc;

    if (get_int_elt(obj, LOC, &loc) == -1) {
      notify_bad(player);
      return;
    }
    if (!controls(player, cause, loc)) {
      if(!(switches & CMD_QUIET))
        notify_player(player, cause, player, "You must pick it up, first.",
		      NOT_QUIET);
      return;
    }
  }
  if (match->perm & PERM_PLAYER) {
    int powner;

    if(get_int_elt(player, OWNER, &powner) == -1) {
      notify_bad(player);
      return;
    }
    if(!isPLAYER(player) || (player != powner)) {
      if(!(switches & CMD_QUIET))
        notify_player(player, cause, player,
		      "Only a player may set that flag.", NOT_QUIET);
      return;
    }
  }

  /* Set the flags */
  ohear = can_hear(obj);
  stamp(obj, STAMP_MODIFIED);

  if (get_flags_elt(obj, FLAGS, objflags) == -1) {
    notify_bad(player);
    return;
  }
  if (unset) {
    objflags[match->word] &= ~(match->code);
    msg = "Flag unset.";
  } else {
    objflags[match->word] |= match->code;
    msg = "Flag set.";
  }
  if (set_flags_elt(obj, FLAGS, objflags) == -1) {
    notify_bad(player);
    return;
  }

  if(!(switches & CMD_QUIET))
    notify_player(player, cause, player, msg, NOT_QUIET);

  hear_alert(cause, ohear, obj);
}		

VOID do_password(player, cause, switches, argone, argtwo)
    int player, cause, switches;
    char *argone, *argtwo;
{
  char *pwd;
  int aflags;

  if ((argone == (char *)NULL) || (argone[0] == '\0')) {
    if(!(switches & CMD_QUIET))
      notify_player(player, cause, player,
		    "Usage: @password <oldpassword> = <newpassword>",
		    NOT_QUIET);
    return;
  }
  if (attr_get(player, PASSWORD, &pwd, &aflags) == -1) {
    notify_bad(player);
    return;
  }
  if ((pwd == (char *)NULL) || (pwd[0] == '\0')) {
    if(!(switches & CMD_QUIET))
      notify_player(player, cause, player,
      		    "You can't change your password.", NOT_QUIET);
    return;
  }
  if (!comp_password(pwd, argone)) {
    if(!(switches & CMD_QUIET))
      notify_player(player, cause, player, "Incorrect password.", NOT_QUIET);
    return;
  }
  if (attr_add(player, PASSWORD, cryptpwd(argtwo), aflags) == -1) {
    notify_bad(player);
    return;
  }

  if(!(switches & CMD_QUIET))
    notify_player(player, cause, player, "Password changed.", NOT_QUIET);
}

void do_set_string(player, cause, switches, argone, argtwo, str)
    int player, cause, switches;
    char *argone, *argtwo;
    StringList *str;
{
  int obj, ret;
  char buf[MEDBUFFSIZ];

  /* Find the thing to set the string ON */

  if ((argone == (char *)NULL) || (argone[0] == '\0')) {
    if(!(switches & CMD_QUIET)) {
      snprintf(buf, sizeof(buf), "Set the %s string on what?", str->name);
      notify_player(player, cause, player, buf, NOT_QUIET);
    }
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
  if ((argtwo != (char *)NULL) && argtwo[0] && !ok_attr_value(argtwo)) {
    if(!(switches & CMD_QUIET))
      notify_player(player, cause, player,
		    "You can't set an attribute to that!", NOT_QUIET);
    return;
  }
  stamp(obj, STAMP_MODIFIED);
  if ((argtwo != (char *)NULL) && argtwo[0]) {
    ret = attr_add(obj, str->name, argtwo, str->flags);
  } else {
    ret = attr_delete(obj, str->name);
  }
  if (ret == -1) {
    notify_bad(player);
    return;
  }

  if(!(switches & CMD_QUIET)) {
    snprintf(buf, sizeof(buf), "%s set.", str->name);
    notify_player(player, cause, player, buf, NOT_QUIET);
  }
}

static void set_attrflags(player, cause, switches, obj, name, flags)
    int player, cause, switches, obj;
    char *name, *flags;
{
  int aflags;
  int unset = 0;
  char *data, buf[MEDBUFFSIZ];
  int idx;

  if (attr_get(obj, name, &data, &aflags) == -1) {
    notify_bad(player);
    return;
  }
  if (data == (char *) NULL) {
    if(!(switches & CMD_QUIET))
      notify_player(player, cause, player, "No such attribute.", NOT_QUIET);
    return;
  }
  if (flags[0] == '!') {
    unset = 1;
    flags++;
  }
  for (idx = 0; idx < AFlags_count &&
       strncasecmp(AFlags[idx].name, flags, AFlags[idx].matlen); idx++);
  if (!AFlags[idx].name) {
    if(!(switches & CMD_QUIET))
      notify_player(player, cause, player, "No such attribute flag.",
      		    NOT_QUIET);
    return;
  }
  if (!can_set_attr(player, cause, obj, AFlags[idx].code)) {
    if(!(switches & CMD_QUIET)) {
      snprintf(buf, sizeof(buf), "%s(#%d): Permission denied.", name, obj);
      notify_player(player, cause, player, buf, NOT_QUIET);
    }
    return;
  }
  if (unset) {
    aflags &= ~(AFlags[idx].code);
  } else {
    aflags |= AFlags[idx].code;
  }
  if (attr_set(obj, name, aflags) == -1) {
    notify_bad(player);
    return;
  }

  if(!(switches & CMD_QUIET))
    notify_player(player, cause, player,
		  ((unset) ? "Attribute flag unset." : "Attribute flag set."),
		  NOT_QUIET);
}

void set_attribute(player, cause, switches, obj, name, data)
    int player, cause, switches, obj;
    char *name, *data;
{
  int aflags;
  char *p, buf[MEDBUFFSIZ];

  if (!ok_attr_name(name)) {
    if(!(switches & CMD_QUIET))
      notify_player(player, cause, player,
		    "That's a silly name for an attribute!", NOT_QUIET);
    return;
  }
  if (attr_get(obj, name, &p, &aflags) == -1) {
    notify_bad(player);
    return;
  }
  if ((p != (char *)NULL) && !can_set_attr(player, cause, obj, aflags)) {
    if(!(switches & CMD_QUIET)) {
      snprintf(buf, sizeof(buf), "%s(#%d): Permission denied.", name, obj);
      notify_player(player, cause, player, buf, NOT_QUIET);
    }
    return;
  }
  if (p == (char *)NULL)
    aflags = DEFATTR_FLGS;
  if ((data != (char *)NULL) && data[0]) {
    if (!ok_attr_value(data)) {
      if(!(switches & CMD_QUIET))
        notify_player(player, cause, player,
		      "You can't set an attribute to that!", NOT_QUIET);
      return;
    }
    if (attr_add(obj, name, data, aflags) == -1) {
      notify_bad(player);
      return;
    }

    /* special case */
    if(strcasecmp(name, STARTUP) == 0) {
      if(setHAS_STARTUP(obj) == -1) {
	notify_bad(player);
	return;
      }
    }

    if(!(switches & CMD_QUIET))
      notify_player(player, cause, player, "Attribute set.", NOT_QUIET);
  } else {
    if (attr_delete(obj, name) == -1) {
      if(!(switches & CMD_QUIET))
        notify_player(player, cause, player, "No such attribute.", NOT_QUIET);
      return;
    }

    /* special case */
    if(strcasecmp(name, STARTUP) == 0) {
      if(unsetHAS_STARTUP(obj) == -1) {
	notify_bad(player);
	return;
      }
    }

    if(!(switches & CMD_QUIET))
      notify_player(player, cause, player, "Attribute removed.", NOT_QUIET);
  }
}
