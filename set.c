/* set.c */

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

VOID do_charge(player, cause, switches, argone, argtwo)
    int player, cause, switches;
    char *argone, *argtwo;
{
  int obj, charges;
  char *ptr;

  if((argone == (char *)NULL) || (argone[0] == '\0')) {
    notify_player(player, cause, player, "Charge what?", 0);
    return;
  }
  obj = resolve_object(player, cause, argone, RSLV_EXITS|RSLV_NOISY);
  if(obj == -1)
    return;
  if(!controls(player, cause, obj)) {
    notify_player(player, cause, player, "Permission denied.", 0);
    return;
  }

  if((argtwo == (char *)NULL) || (argtwo[0] == '\0')) {
    charges = -1;
  } else {
    charges = (int)strtol(argtwo, &ptr, 10);
    if(ptr == argtwo) {
      notify_player(player, cause, player, "Numerical argument required.", 0);
      return;
    }
    if(charges < -1)
      charges = -1;
  }

  if(set_int_elt(obj, CHARGES, charges) == -1)
    notify_bad(player);
  else {
    switch(charges) {
    case -1:
      notify_player(player, cause, player, "Charged removed.", 0);
      break;
    case 0:
      notify_player(player, cause, player, "Charge emptied.", 0);
      break;
    default:
      notify_player(player, cause, player, "Charge set.", 0);
      break;
    }
  }
}

VOID do_set(player, cause, switches, argone, argtwo)
    int player, cause, switches;
    char *argone, *argtwo;
{
  int obj, unset, objflags[FLAGS_LEN], ohear;
  char *msg, *name, *data;
  register FlagList *flist;
  FlagList *match;

  if (!argtwo || !*argtwo) {
    notify_player(player, cause, player,"Set what?", 0);
    return;
  }
  obj = resolve_object(player, cause, argone, RSLV_EXITS | RSLV_NOISY);
  if(obj == -1)
    return;
  if (!exists_object(obj) || !controls(player, cause, obj)) {
    notify_player(player, cause, player, "You can't set anything on that!", 0);
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
    set_attribute(player, cause, obj, argtwo, data);
    return;
  } else if ((data = strchr(argtwo, '/')) != (char *) NULL) {
    for (name = (data - 1); name >= argtwo && isspace(*name); name--);
    name[1] = 0;
    data++;

    while (*data && isspace(*data))
      data++;
    set_attrflags(player, cause, obj, argtwo, data);
    return;
  }
  /* object flags */
  if (*argtwo == '!') {
    unset = 1;
    argtwo++;
  } else
    unset = 0;

  match = (FlagList *)NULL;
  for (flist = GenFlags; flist->name; flist++) {
    if (!strncasecmp(flist->name, argtwo, flist->matlen)) {
      match = flist;
      break;
    }
  }
  /* no match yet? */
  if (match == (FlagList *)NULL) {
    switch(Typeof(obj)) {
    case TYP_PLAYER:
      for(flist = PlayerFlags; flist->name; flist++) {
        if(!strncasecmp(flist->name, argtwo, flist->matlen)) {
	  match = flist;
	  break;
	}
      }
      break;
    case TYP_ROOM:
      for(flist = RoomFlags; flist->name; flist++) {
        if(!strncasecmp(flist->name, argtwo, flist->matlen)) {
	  match = flist;
	  break;
	}
      }
      break;
    case TYP_THING:
      for(flist = ThingFlags; flist->name; flist++) {
        if(!strncasecmp(flist->name, argtwo, flist->matlen)) {
	  match = flist;
	  break;
	}
      }
      break;
    case TYP_EXIT:
      for(flist = ExitFlags; flist->name; flist++) {
        if(!strncasecmp(flist->name, argtwo, flist->matlen)) {
	  match = flist;
	  break;
	}
      }
      break;
    }
  }

  if (match == (FlagList *)NULL) {
    notify_player(player, cause, player, "I don't understand that flag.", 0);
    return;
  }

  /* permission checks */
  if (((match->perm & PERM_GOD) && !isGOD(player))
      || ((match->perm & PERM_WIZ) && !isWIZARD(player))
      || (match->perm & PERM_INTERNAL)) {
    notify_player(player, cause, player, "Permission denied.", 0);
    return;
  }
  /* special permissions */
  if ((obj == mudconf.player_god) && unset && (match->code == GOD)
      && (match->word == 1)) {
    notify_player(player, cause, player, "That player is always God.", 0);
    return;
  }
  if ((match->perm & PERM_DARK) && !isWIZARD(player) && isTHING(obj)) {
    int loc;

    if (get_int_elt(obj, LOC, &loc) == -1) {
      notify_bad(player);
      return;
    }
    if (!controls(player, cause, loc)) {
      notify_player(player, cause, player, "You must pick it up, first.", 0);
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
      notify_player(player, cause, player,
		    "Only a player may set that flag.", 0);
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
  notify_player(player, cause, player, msg, 0);

  hear_alert(cause, ohear, obj);
}		

VOID do_password(player, cause, switches, argone, argtwo)
    int player, cause, switches;
    char *argone, *argtwo;
{
  char *pwd;
  int aflags;

  if (!argone || !*argone) {
    notify_player(player, cause, player,
		  "Usage: @password <oldpassword> = <newpassword>", 0);
    return;
  }
  if (attr_get(player, PASSWORD, &pwd, &aflags) == -1) {
    notify_bad(player);
    return;
  }
  if (!pwd || !*pwd) {
    notify_player(player, cause, player, "You can't change your password.", 0);
    return;
  }
  if (!comp_password(pwd, argone)) {
    notify_player(player, cause, player, "Incorrect password.", 0);
    return;
  }
  if (attr_add(player, PASSWORD, cryptpwd(argtwo), aflags) == -1) {
    notify_bad(player);
    return;
  }
  notify_player(player, cause, player, "Password changed.", 0);
  return;
}

VOID do_give(player, cause, switches, argone, argtwo)
    int player, cause, switches;
    char *argone, *argtwo;
{
  int victim, cost, expt;
  int owner, pennies, nval;
  char *ptr, buf[MEDBUFFSIZ], *pname, *vname;

  if(!mudconf.enable_money) {
    notify_player(player, cause, player, "Pennies are not enabled.", 0);
    return;
  }

  if (!argone || !*argone || !argtwo || !*argtwo) {
    notify_player(player, cause, player, "Give what to who?", 0);
    return;
  }
  victim = resolve_player(player, cause, argone, RSLV_NOISY);
  if(victim == -1)
    return;
  cost = (int)strtol(argtwo, &ptr, 10);
  if((ptr == argtwo) || ((cost < 1) && !isWIZARD(player))){
    notify_player(player, cause, player,
    		  "You must specify a postive number of pennies.", 0);
    return;
  }

  if(get_int_elt(victim, COST, &expt) == -1) {
    notify_bad(player);
    return;
  }
  if((expt > cost) && (cost > 0)) {
    notify_player(player, cause, player, "Being a cheapskate, eh?", 0);
    return;
  }
  if (!can_afford(player, cause,
      (((cost > expt) && (expt > 0)) ? expt : cost)))
    return;

  if ((get_int_elt(victim, OWNER, &owner) == -1) ||
      (get_int_elt(owner, PENNIES, &pennies) == -1) ||
      (get_str_elt(player, NAME, &pname) == -1) ||
      (get_str_elt(victim, NAME, &vname) == -1)) {
    notify_bad(player);
    return;
  }

  nval = pennies + (((cost > expt) && (expt > 0)) ? expt : cost);
  if (!isWIZARD(player) && (nval > mudconf.max_pennies)) {
    notify_player(player, cause, player, 
		  "They don't need that many pennies!", 0);
    return;
  }
  if (set_int_elt(owner, PENNIES, nval) == -1) {
    notify_bad(player);
    return;
  }

  if((cost > expt) && (expt > 0)) {
    snprintf(buf, sizeof(buf),
	     "You give %d %s to %s, and get %d %s in change.",
    	     cost, (cost == 1) ? "penny" : "pennies", vname,
	     (cost - expt), ((cost - expt) == 1) ? "penny" : "pennies");
  } else {
    snprintf(buf, sizeof(buf), "You give %d %s to %s.", cost,
	     (cost == 1) ? "penny" : "pennies", vname);
  }
  notify_player(player, cause, player, buf, 0);
  snprintf(buf, sizeof(buf), "%s gives you %d %s.", pname,
	   cost, (cost == 1) ? "penny" : "pennies");
  notify_player(victim, cause, player, buf, 0);

  act_object(player, cause, victim, PAY, OPAY, APAY, -1, (char *)NULL,
  	     (char *)NULL);
}

VOID do_cost(player, cause, switches, argone, argtwo)
    int player, cause, switches;
    char *argone, *argtwo;
{
  int dest, cost;
  char *ptr;

  if((argone == (char *)NULL) || (argone[0] == '\0')) {
    notify_player(player, cause, player, "Set the cost of what?", 0);
    return;
  }

  if((argtwo == (char *)NULL) || (argtwo[0] == '\0'))
    cost = 0;
  else {
    cost = (int)strtol(argtwo, &ptr, 10);
    if((ptr == argtwo) || (cost < 0)) {
      notify_player(player, cause, player,
		    "Cost must be a positive number.", 0);
      return;
    }
  }

  dest = resolve_player(player, cause, argone, RSLV_NOISY);
  if(dest == -1)
    return;

  if(!controls(player, cause, dest)) {
    notify_player(player, cause, player, "You can't set the cost of that!", 0);
    return;
  }

  if(set_int_elt(dest, COST, cost) == -1) {
    notify_bad(player);
    return;
  }

  notify_player(player, cause, player,
		((cost == 0) ? "Cost unset." : "Cost set."), 0);
}

void do_set_string(player, cause, argone, argtwo, str)
    int player, cause;
    char *argone, *argtwo;
    StringList *str;
{
  int obj, ret;
  char buf[MEDBUFFSIZ];

  /* Find the thing to set the string ON */

  if (!argone || !*argone) {
    snprintf(buf, sizeof(buf), "Set the %s string on what?", str->name);
    notify_player(player, cause, player, buf, 0);
    return;
  }
  obj = resolve_object(player, cause, argone, RSLV_EXITS | RSLV_NOISY);
  if(obj == -1)
    return;
  if (!controls(player, cause, obj)) {
    notify_player(player, cause, player, "You can't do that!", 0);
    return;
  }
  if (argtwo && *argtwo && !ok_attr_value(argtwo)) {
    notify_player(player, cause, player,
		  "You can't set an attribute to that!", 0);
    return;
  }
  stamp(obj, STAMP_MODIFIED);
  if (argtwo && *argtwo) {
    ret = attr_add(obj, str->name, argtwo, str->flags);
  } else {
    ret = attr_delete(obj, str->name);
  }
  if (ret == -1) {
    notify_bad(player);
    return;
  }
  snprintf(buf, sizeof(buf), "%s set.", str->name);
  notify_player(player, cause, player, buf, 0);
}

void set_attrflags(player, cause, obj, name, flags)
    int player, cause, obj;
    char *name, *flags;
{
  int aflags;
  int unset = 0;
  char *data, buf[MEDBUFFSIZ];
  AFlagList *Aflags;

  if (attr_get(obj, name, &data, &aflags) == -1) {
    notify_bad(player);
    return;
  }
  if (data == (char *) NULL) {
    notify_player(player, cause, player, "No such attribute.", 0);
    return;
  }
  if (flags[0] == '!') {
    unset = 1;
    flags++;
  }
  for (Aflags = AFlags; Aflags->name &&
       strncasecmp(Aflags->name, flags, Aflags->matlen); Aflags++);
  if (!Aflags->name) {
    notify_player(player, cause, player, "No such attribute flag.", 0);
    return;
  }
  if (!can_set_attr(player, cause, obj, Aflags->code)) {
    snprintf(buf, sizeof(buf), "%s(#%d): Permission denied.", name, obj);
    notify_player(player, cause, player, buf, 0);
    return;
  }
  if (unset) {
    aflags &= ~(Aflags->code);
  } else {
    aflags |= Aflags->code;
  }
  if (attr_set(obj, name, aflags) == -1) {
    notify_bad(player);
    return;
  }
  notify_player(player, cause, player,
		((unset) ? "Attribute flag unset." : "Attribute flag set."), 0);
}

void set_attribute(player, cause, obj, name, data)
    int player, cause, obj;
    char *name, *data;
{
  int aflags;
  char *p, buf[MEDBUFFSIZ];

  if (!ok_attr_name(name)) {
    notify_player(player, cause, player,
		  "That's a silly name for an attribute!", 0);
    return;
  }
  if (attr_get(obj, name, &p, &aflags) == -1) {
    notify_bad(player);
    return;
  }
  if (p && !can_set_attr(player, cause, obj, aflags)) {
    snprintf(buf, sizeof(buf), "%s(#%d): Permission denied.", name, obj);
    notify_player(player, cause, player, buf, 0);
    return;
  }
  if (!p)
    aflags = DEFATTR_FLGS;
  if (data && *data) {
    if (!ok_attr_value(data)) {
      notify_player(player, cause, player,
		    "You can't set an attribute to that!", 0);
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
    notify_player(player, cause, player, "Attribute set.", 0);
  } else {
    if (attr_delete(obj, name) == -1) {
      notify_player(player, cause, player, "No such attribute.", 0);
      return;
    }

    /* special case */
    if(strcasecmp(name, STARTUP) == 0) {
      if(unsetHAS_STARTUP(obj) == -1) {
	notify_bad(player);
	return;
      }
    }
    notify_player(player, cause, player, "Attribute removed.", 0);
  }
}
