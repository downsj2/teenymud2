/* move.c */

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

#include "conf.h"
#include "teeny.h"
#include "commands.h"
#include "externs.h"

/* all the code dealing with moving objects around */

static void flush_room _ANSI_ARGS_((int, int));

static char noloc[] = "You have no location!";

VOID do_home(player, cause, switches)
    int player, cause, switches;
{
  int here, home, list, next;
  char buf[MEDBUFFSIZ];
  char *name;

  if (!has_home(player))
    return;
  if ((get_int_elt(player, HOME, &home) == -1) || !exists_object(home)) {
    if(!(switches & CMD_QUIET))
      notify_player(player, cause, player, "Your home does not exist!",
      		    NOT_QUIET);
    home = mudconf.starting_loc;
  }
  if ((get_int_elt(player, LOC, &here) == -1) || !exists_object(here)) {
    if(!(switches & CMD_QUIET))
      notify_player(player, cause, player, noloc, NOT_QUIET);
    return;
  }
  /* Tell people here. */
  if (get_str_elt(player, NAME, &name) == -1) {
    notify_bad(player);
    return;
  }
  if (!isDARK(here) && !isDARK(player)) {
    snprintf(buf, sizeof(buf), "%s goes home.", name);
    notify_oall(player, cause, player, buf, NOT_NOPUPPET);
  }

  if(!(switches & CMD_QUIET)) {
    notify_player(player, cause, player,
    		  "There's no place like home...", NOT_QUIET);
    notify_player(player, cause, player,
    		  "There's no place like home...", NOT_QUIET);
    notify_player(player, cause, player,
    		  "There's no place like home...", NOT_QUIET);
    notify_player(player, cause, player,
  		  "You wake up back home, without your possessions.",
		  NOT_QUIET);
  }

  send_home(player, cause, here);
  flush_room(here, cause);

  /* Send all the player's stuff home too */

  if (get_int_elt(player, CONTENTS, &list) == -1) {
    notify_bad(player);
    return;
  }
  while (list != -1) {
    /* send_home() will change the NEXT element of list */

    if (get_int_elt(list, NEXT, &next) == -1)
      break;
    send_home(list, cause, player);
    list = next;
  }
}

VOID do_go(player, cause, switches, argone)
    int player, cause, switches;
    char *argone;
{
  int here, theex;

  if ((argone == (char *)NULL) || (argone[0] == '\0')) {
    if(!(switches & CMD_QUIET))
      notify_player(player, cause, player, "You can't go that way.", NOT_QUIET);
    return;
  }
  if (strcasecmp(argone, "home") == 0) {
    do_home(player, cause, switches);
    return;
  }
  if ((get_int_elt(player, LOC, &here) == -1) || !exists_object(here)) {
    if(!(switches & CMD_QUIET))
      notify_player(player, cause, player, noloc, NOT_QUIET);
    return;
  }
  theex = match_exit_command(player, cause, argone);
  if(theex != -1) {
    do_go_attempt(player, cause, here, theex);
  } else {
    if(!(switches & CMD_QUIET))
      notify_player(player, cause, player, "You can't go that way.", NOT_QUIET);
  }
}

VOID do_hand(player, cause, switches, argone, argtwo)
    int player, cause, switches;
    char *argone, *argtwo;
{
  int obj, here, destination, oloc;
  char *oname, *dname, *pname;
  char buffer1[MEDBUFFSIZ];

  if ((argone == (char *)NULL) || (argone[0] == '\0')
      || (argtwo == (char *)NULL) || (argtwo[0] == '\0')) {
    if(!(switches & CMD_QUIET))
      notify_player(player, cause, player, "Hand what to who?", NOT_QUIET);
    return;
  }

  if((get_int_elt(player, LOC, &here) == -1) || !exists_object(here)) {
    if(!(switches & CMD_QUIET))
      notify_player(player, cause, player, noloc, NOT_QUIET);
    return;
  }

  obj = match_here(player, cause, player, argone, MAT_THINGS | MAT_PLAYERS);
  if(obj == -1) {
    if(!(switches & CMD_QUIET))
      notify_player(player, cause, player, "You don't have that.", NOT_QUIET);
    return;
  }
  if(obj == -2) {
    if(!(switches & CMD_QUIET))
      notify_player(player, cause, player, "I can't tell which one you mean.",
      		    NOT_QUIET);
    return;
  }
  if(obj == player) {
    if(!(switches & CMD_QUIET))
      notify_player(player, cause, player, "You wouldn't fit!", NOT_QUIET);
    return;
  }
  if((get_int_elt(obj, LOC, &oloc) == -1) || !exists_object(oloc)) {
    notify_bad(player);
    return;
  }

  destination = match_here(player, cause, here, argtwo,
			   MAT_THINGS | MAT_PLAYERS);
  if(destination == -1) {
    if(!(switches & CMD_QUIET))
      notify_player(player, cause, player, "I don't see that here.",
      		    NOT_QUIET);
    return;
  }
  if(destination == -2) {
    if(!(switches & CMD_QUIET))
      notify_player(player, cause, player, "I can't tell which one you mean.",
      		    NOT_QUIET);
    return;
  }
  if(!legal_thingloc_check(obj, destination)) {
    if(!(switches & CMD_QUIET))
      notify_player(player, cause, player, "It wouldn't fit!", NOT_QUIET);
    return;
  }

  if((!controls(player, cause, destination) && !isENTER_OK(destination))
     || islocked(player, cause, destination, ENLOCK)) {
    act_object(obj, cause, destination, ENFAIL, OENFAIL, AENFAIL, -1,
	       "It won't quite fit.", (char *)NULL);
    return;
  }

  if(get_str_elt(obj, NAME, &oname) == -1)
    oname = "???";
  if(get_str_elt(destination, NAME, &dname) == -1)
    dname = "???";
  if(get_str_elt(player, NAME, &pname) == -1)
    pname = "???";

  if(!(switches & CMD_QUIET)) {
    snprintf(buffer1, sizeof(buffer1), "You hand %s to %s.", oname, dname);
    notify_player(player, cause, player, buffer1, NOT_QUIET);
  }
  snprintf(buffer1, sizeof(buffer1), "%s hands %s to you.", pname, oname);
  notify_player(destination, cause, player, buffer1, 0);

  act_object(obj, cause, destination, ENTER, OENTER, AENTER, -1,
	     (char *)NULL, (char *)NULL);
  act_object(obj, cause, destination, (char *)NULL, OXENTER, (char *)NULL,
	     destination, (char *)NULL, (char *)NULL);
  move_player_leave(obj, cause, oloc, 0, (char *)NULL);
  move_player_arrive(obj, cause, destination, 0);

  look_location(obj, cause, 0);
  if(has_html(obj))
    html_anchor_location(obj, cause);
}

VOID do_enter(player, cause, switches, argone)
    int player, cause, switches;
    char *argone;
{
  int here, obj;

  if ((argone == (char *)NULL) || (argone[0] == '\0')) {
    if(!(switches & CMD_QUIET))
      notify_player(player, cause, player, "Enter what?", NOT_QUIET);
    return;
  }
  if ((get_int_elt(player, LOC, &here) == -1) || !exists_object(here)) {
    if(!(switches & CMD_QUIET))
      notify_player(player, cause, player, noloc, NOT_QUIET);
    return;
  }
  obj = match_here(player, cause, here, argone, MAT_THINGS | MAT_PLAYERS);
  if(obj == -1) {
    if(!(switches & CMD_QUIET))
      notify_player(player, cause, player, "I don't see that here.", NOT_QUIET);
    return;
  }
  if (obj == -2) {
    if(!(switches & CMD_QUIET))
      notify_player(player, cause, player, "I can't tell which one you mean.",
      		    NOT_QUIET);
    return;
  }
  if (!legal_thingloc_check(player, obj)) {
    if(!(switches & CMD_QUIET))
      notify_player(player, cause, player, "You wouldn't fit!", NOT_QUIET);
    return;
  }
  if ((!controls(player, cause, obj) && !isENTER_OK(obj))
      || islocked(player, cause, obj, ENLOCK)) {
    act_object(player, cause, obj, ENFAIL, OENFAIL, AENFAIL, -1,
    	       "You don't quite fit.", (char *) NULL);
    return;
  }
  act_object(player, cause, obj, ENTER, OENTER, AENTER, -1, (char *)NULL,
  	     (char *)NULL);
  act_object(player, cause, obj, (char *) NULL, OXENTER, (char *)NULL, obj,
  	     (char *)NULL, (char *)NULL);
  move_player_leave(player, cause, here, 0, (char *)NULL);
  move_player_arrive(player, cause, obj, 0);
  flush_room(here, cause);
  look_location(player, cause, 0);
  if(has_html(player))
    html_anchor_location(player, cause);
}

VOID do_leave(player, cause, switches)
    int player, cause, switches;
{
  int here, hereloc;

  if ((get_int_elt(player, LOC, &here) == -1) || !exists_object(here)) {
    if(!(switches & CMD_QUIET))
      notify_player(player, cause, player, noloc, NOT_QUIET);
    return;
  }
  if (isROOM(here) || isEXIT(here)) {
    if(!(switches & CMD_QUIET))
      notify_player(player, cause, player, 
		    "You're not inside of anything to leave!", NOT_QUIET);
    return;
  }
  if ((get_int_elt(here, LOC, &hereloc) == -1) || !exists_object(hereloc)) {
    notify_bad(player);
    return;
  }
  if (!legal_thingloc_check(player, hereloc)) {
    if (!(switches & CMD_QUIET))
      notify_player(player, cause, player, "You wouldn't fit!", NOT_QUIET);
    return;
  }

  act_object(player, cause, here, LEAVE, OLEAVE, ALEAVE, -1, (char *)NULL,
  	     (char *)NULL);
  act_object(player, cause, here, (char *) NULL, OXLEAVE, (char *)NULL,
  	     hereloc, (char *)NULL, (char *)NULL);
  move_player_leave(player, cause, here, 0, (char *)NULL);
  move_player_arrive(player, cause, hereloc, 0);
  stamp(here, STAMP_USED);

  look_location(player, cause, 0);
  if(has_html(player))
    html_anchor_location(player, cause);
}

VOID do_attach(player, cause, switches, argone, argtwo)
    int player, cause, switches;
    char *argone, *argtwo;
{
  int dest, action, loc;

  if ((argone == (char *)NULL) || (argone[0] == '\0')
      || (argtwo == (char *)NULL) || (argtwo[0] == '\0')) {
    if(!(switches & CMD_QUIET))
      notify_player(player, cause, player, "Attach what to what?", NOT_QUIET);
    return;
  }
  action = resolve_exit(player, cause, argone,
  			(!(switches & CMD_QUIET) ? RSLV_NOISY : 0));
  if(action == -1)
    return;
  dest = resolve_object(player, cause, argtwo,
  			(!(switches & CMD_QUIET) ? RSLV_NOISY : 0));
  if(dest == -1)
    return;
  if (!controls(player, cause, action)) {
    if(!(switches & CMD_QUIET))
      notify_player(player, cause, player, "Permission denied.", NOT_QUIET);
    return;
  }
  if (!controls(player, cause, dest) || !has_exits(dest)) {
    if(!(switches & CMD_QUIET))
      notify_player(player, cause, player, 
		    "You can't attach anything to that!", NOT_QUIET);
    return;
  }
  if (get_int_elt(action, LOC, &loc) == -1) {
    notify_bad(player);
    return;
  }
  list_drop(action, loc, EXITS);
  list_add(action, dest, EXITS);
  if (set_int_elt(action, LOC, dest) == -1) {
    notify_bad(player);
    return;
  }
  stamp(action, STAMP_MODIFIED);

  if(!(switches & CMD_QUIET))
    notify_player(player, cause, player, "Action reattached.", NOT_QUIET);
}

VOID do_take(player, cause, switches, argone)
    int player, cause, switches;
    char *argone;
{
  int here, obj;
  char *pname, *oname, buf[MEDBUFFSIZ];

  if ((argone == (char *)NULL) || (argone[0] == '\0')) {
    if(!(switches & CMD_QUIET))
      notify_player(player, cause, player, "Take what?", NOT_QUIET);
    return;
  }
  if ((get_int_elt(player, LOC, &here) == -1) || !exists_object(here)) {
    notify_player(player, cause, player, noloc, 0);
    return;
  }
  obj = match_here(player, cause, here, argone, MAT_PLAYERS | MAT_THINGS);
  if(obj == -1) {
    if(!(switches & CMD_QUIET))
      notify_player(player, cause, player, "I don't see that here.", NOT_QUIET);
    return;
  }
  if (obj == -2) {
    if(!(switches & CMD_QUIET))
      notify_player(player, cause, player, "I can't tell which one you mean.",
      		    NOT_QUIET);
    return;
  }
  switch (Typeof(obj)) {
  case TYP_EXIT:
    if(!(switches & CMD_QUIET)) {
      snprintf(buf, sizeof(buf), 
	       "You can't pick up exits.  Use \"@attach %s = me\", instead.",
    	      argone);
      notify_player(player, cause, player, buf, NOT_QUIET);
    }
    return;
  case TYP_THING:
  case TYP_PLAYER:
    if (!legal_thingloc_check(obj, player)) {
      if(!(switches & CMD_QUIET))
        notify_player(player, cause, player, "It wouldn't fit!", NOT_QUIET);
      return;
    }

    if ((get_str_elt(obj, NAME, &oname) == -1) ||
	(get_str_elt(player, NAME, &pname) == -1)) {
      notify_bad(player);
      return;
    }

    if (islocked(player, cause, obj, LOCK)) {
      act_object(player, cause, obj, FAIL, OFAIL, AFAIL, -1,
      		 "You can't take that.", (char *) NULL);
      return;
    }

    snprintf(buf, sizeof(buf), "takes %s.", oname);
    act_object(player, cause, obj, SUC, OSUC, ASUC, -1, (char *) NULL, buf);
    snprintf(buf, sizeof(buf), "%s picks you up.", pname);
    move_player_leave(obj, cause, here, 0, buf);
    move_player_arrive(obj, cause, player, 0);
    look_location(obj, cause, 0);
    if(has_html(obj))
      html_anchor_location(obj, cause);
    stamp(obj, STAMP_USED);
    break;
  default:
    if(!(switches & CMD_QUIET))
      notify_player(player, cause, player, "You can't take that!", NOT_QUIET);
    return;
  }
  if(!(switches & CMD_QUIET))
    notify_player(player, cause, player, "Taken.", NOT_QUIET);
}

VOID do_drop(player, cause, switches, argone)
    int player, cause, switches;
    char *argone;
{
  int here, obj, dest;
  char *pname, *oname, buf[MEDBUFFSIZ];

  if ((argone == (char *)NULL) || (argone[0] == '\0')) {
    if(!(switches & CMD_QUIET))
      notify_player(player, cause, player, "Drop what?", NOT_QUIET);
    return;
  }
  if ((get_int_elt(player, LOC, &here) == -1) || !exists_object(here)) {
    if(!(switches & CMD_QUIET))
      notify_player(player, cause, player, noloc, NOT_QUIET);
    return;
  }
  obj = match_here(player, cause, player, argone, MAT_PLAYERS | MAT_THINGS);
  if(obj == -1) {
    if(!(switches & CMD_QUIET))
      notify_player(player, cause, player, "You don't have that!", NOT_QUIET);
    return;
  }
  if (obj == -2) {
    if(!(switches & CMD_QUIET))
      notify_player(player, cause, player,
    		    "I can't tell which one you want to drop.", NOT_QUIET);
    return;
  }

  switch (Typeof(obj)) {
  case TYP_EXIT:
    if(!(switches & CMD_QUIET)) {
      snprintf(buf, sizeof(buf),
	       "You can't drop exits.  Use \"@attach %s = here\", instead.",
    	       argone);
      notify_player(player, cause, player, buf, NOT_QUIET);
    }
    return;
  case TYP_THING:
    if (isDARK(obj) && !controls(player, cause, here)) {
      if(!(switches & CMD_QUIET))
        notify_player(player, cause, player, "You can't drop that here.",
		      NOT_QUIET);
      return;
    }
  case TYP_PLAYER:
    /* Figure out where it's supposed to go */

    if (isSTICKY(obj)) {
      /* If the object is STICKY, send the object to home if you can.     */
      if (get_int_elt(obj, HOME, &dest) == -1) {
	notify_bad(player);
	return;
      }
    } else if(!isSTICKY(here)) {
      /* If this place is !STICKY, see if there's a dropto */
      if (get_int_elt(here, DROPTO, &dest) == -1) {
	notify_bad(player);
	return;
      }
      if ((dest == -1) || !isROOM(here)) {	/* No dropto */
	dest = here;
      } else if (dest == -3) {	/* Dropto home */
	if (get_int_elt(obj, HOME, &dest) == -1) {
	  notify_bad(player);
	  return;
	}
      }
    } else
      dest = here;

    if (!legal_thingloc_check(obj, dest)) {
      if(!(switches & CMD_QUIET))
	notify_player(player, cause, player, "It wouldn't fit!", NOT_QUIET);
      return;
    }

    stamp(obj, STAMP_USED);

    /* Tell the room */

    if(get_str_elt(player, NAME, &pname) == -1)
      pname = "???";
    if(get_str_elt(obj, NAME, &oname) == -1)
      oname = "???";

    snprintf(buf, sizeof(buf), "dropped %s.", oname);
    act_object(player, cause, obj, DROP, ODROP, ADROP, -1, (char *) NULL, buf);
    snprintf(buf, sizeof(buf), "%s dropped you.", pname);
    move_player_leave(obj, cause, player, 0, buf);

    if (dest != here)
      act_object(obj, cause, here, DROP, ODROP, ADROP, -1, (char *) NULL,
      		 (char *) NULL);

    move_player_arrive(obj, cause, dest, 0);
    look_location(obj, cause, 0);
    if(has_html(obj))
      html_anchor_location(obj, cause);
    break;
  default:
    if(!(switches & CMD_QUIET))
      notify_player(player, cause, player, 
		    "You have no business carrying that!", NOT_QUIET);
    return;
  }
  if(!(switches & CMD_QUIET))
    notify_player(player, cause, player, "Dropped.", NOT_QUIET);
}

VOID do_kill(player, cause, switches, argone, argtwo)
    int player, cause, switches;
    char *argone, *argtwo;
{
  int here, victim, cost, owner, pennies;
  int list, next;
  char buf[MEDBUFFSIZ], *ptr, *vname, *pname;

  if ((argone == (char *)NULL) || (argone[0] == '\0')) {
    if(!(switches & CMD_QUIET))
      notify_player(player, cause, player, "Who would you like to kill?",
      		    NOT_QUIET);
    return;
  }

  if(mudconf.enable_money) {
    if ((argtwo == (char *)NULL) || (argtwo[0] == '\0')) {
      cost = mudconf.minkill_cost;
    } else {
      cost = (int)strtol(argtwo, &ptr, 10);
      if(ptr == argtwo){
        if(!(switches & CMD_QUIET))
          notify_player(player, cause, player,
		        "You must specify cost by number.", NOT_QUIET);
        return;
      }
      if (cost < mudconf.minkill_cost) {
        cost = mudconf.minkill_cost;
      } else if (cost > mudconf.maxkill_cost) {
        cost = mudconf.maxkill_cost;
      }
    }
  } else
    cost = 0;

  /* Find who we are supposed to kill */
  if ((get_int_elt(player, LOC, &here) == -1) || !exists_object(here)) {
    if(!(switches & CMD_QUIET))
      notify_player(player, cause, player, noloc, NOT_QUIET);
    return;
  }
  if (isHAVEN(here) && !isWIZARD(player)) {
    act_object(player, cause, here, KILLFAIL, OKILLFAIL, AKILLFAIL, here,
    	       "It's too peaceful here.", (char *)NULL);
    return;
  }
  victim = match_here(player, cause, here, argone, MAT_PLAYERS | MAT_THINGS);
  if(victim == -1) {
    if(!(switches & CMD_QUIET))
      notify_player(player, cause, player,
      		    "I don't see that here.", NOT_QUIET);
    return;
  }
  if (victim == -2) {
    if(!(switches & CMD_QUIET))
      notify_player(player, cause, player,
    		    "I can't figure out what you want to kill.", NOT_QUIET);
    return;
  }

  if ((get_str_elt(player, NAME, &pname) == -1) ||
	(get_str_elt(victim, NAME, &vname) == -1)) {
    notify_bad(player);
    return;
  }

  /* Have a whack at it. */
  if (!can_afford(player, cause, cost, (switches & CMD_QUIET)))
    return;

  switch (Typeof(victim)) {
  case TYP_PLAYER:
  case TYP_THING:
    if (!isWIZARD(player)
	&& ((mudconf.enable_money && ((random() % mudconf.maxkill_cost) >=cost))
	    || isWIZARD(victim))) {
      /* Fail the kill */

      snprintf(buf, sizeof(buf), "%s tried to kill you!", pname);
      notify_player(victim, cause, player, buf, 0);
      act_object(player, cause, victim, KILLFAIL, OKILLFAIL, AKILLFAIL, -1,
      		 "Your murder attempt failed.", (char *)NULL);
    } else {
      snprintf(buf, sizeof(buf), "%s killed you!", pname);
      notify_player(victim, cause, player, buf, 0);
      snprintf(buf, sizeof(buf), "You killed %s!", vname);
      act_object(player, cause, victim, KILL, OKILL, AKILL, -1, buf, buf + 4);

      /* give reward */
      if (mudconf.enable_money
          && !isWIZARD(victim) && (get_int_elt(victim, OWNER, &owner) != -1)
	  && (get_int_elt(owner, PENNIES, &pennies) != -1)) {
	if ((pennies < mudconf.max_pennies) && (player != owner) &&
	    !(switches & KILL_SLAY)) {
	  int reward;

	  reward = cost / 2;
	  if (set_int_elt(owner, PENNIES, pennies + reward) != -1) {
	    snprintf(buf, sizeof(buf),
		     "You insurance policy pays you %d pennies.", reward);
	    notify_player(victim, cause, player, buf, 0);
	  }
	} else {
	  notify_player(victim, cause, player,
	  		"Your insurance policy has been revoked.", 0);
	}
      }
      send_home(victim, cause, here);
      flush_room(here, cause);

      /* Send all the player's stuff home too */

      if (get_int_elt(victim, CONTENTS, &list) != -1) {
	while (list != -1) {
	  /* send_home() will change the NEXT element of list */
	  if (get_int_elt(list, NEXT, &next) == -1)
	    break;
	  send_home(list, cause, victim);
	  list = next;
	}
      }
    }
    break;
  default:
    if(!(switches & CMD_QUIET))
      notify_player(player, cause, player, "You can't kill that!", NOT_QUIET);
  }
}

VOID do_teleport(player, cause, switches, argone, argtwo)
    int player, cause, switches;
    char *argone, *argtwo;
{
  int object, loc, dest;
  int check_depth;

  if ((argone == (char *)NULL) || (argone[0] == '\0')) {
    if(!(switches & CMD_QUIET))
      notify_player(player, cause, player,
      		    "Teleport what to where?", NOT_QUIET);
    return;
  }
  if ((argtwo == (char *)NULL) || (argtwo[0] == '\0')) {
    object = player;
    dest = resolve_object(player, cause, argone,
    			  (!(switches & CMD_QUIET)
    			   ? RSLV_NOISY|RSLV_HOME|RSLV_ATOK
			   : RSLV_HOME|RSLV_ATOK));
    if(dest == -1)
      return;
  } else {
    object = resolve_object(player, cause, argone,
    			    (!(switches & CMD_QUIET) ? RSLV_NOISY : 0));
    if(object == -1)
      return;
    dest = resolve_object(player, cause, argtwo,
    			  (!(switches & CMD_QUIET)
    			   ? RSLV_NOISY|RSLV_HOME|RSLV_ATOK
			   : RSLV_HOME|RSLV_ATOK));
    if(dest == -1)
      return;
  }
  /* do it */
  if ((get_int_elt(object, LOC, &loc) == -1) || !exists_object(loc)) {
    if(!(switches & CMD_QUIET))
      notify_player(player, cause, player, "That object has no location!",
      		    NOT_QUIET);
    return;
  }

  switch (Typeof(object)) {
  case TYP_ROOM:		/* parent */
    if (dest != mudconf.root_location) {
      if (!exists_object(dest) || (object == dest)) {
        if(!(switches & CMD_QUIET))
	  notify_player(player, cause, player, "Illegal location.", NOT_QUIET);
	return;
      }
      if (!isROOM(dest)) {
        if(!(switches & CMD_QUIET))
	  notify_player(player, cause, player, "That's not a room!", NOT_QUIET);
	return;
      }
      if (!controls(player, cause, dest) && (dest != mudconf.root_location)
          && !isABODE(dest)) {
	if(!(switches & CMD_QUIET))
	  notify_player(player, cause, player, "Permission denied.", NOT_QUIET);
	return;
      }
      check_depth = 0;		/* for legal_roomloc_check */
      if (!legal_roomloc_check(object, dest, &check_depth)) {
        if(!(switches & CMD_QUIET))
	  notify_player(player, cause, player, "Illegal location.", NOT_QUIET);
	return;
      }
    }
    if (set_int_elt(object, LOC, dest) == -1) {
      notify_bad(player);
      return;
    }
    list_drop(object, loc, ROOMS);
    list_add(object, dest, ROOMS);
    if(!(switches & CMD_QUIET))
      notify_player(player, cause, player, "Location set.", NOT_QUIET);
    return;
  case TYP_EXIT:		/* no go */
    if(!(switches & CMD_QUIET))
      notify_player(player, cause, player,
    		    "You can't teleport exits.  Use @attach, instead.",
		    NOT_QUIET);
    return;
  case TYP_THING:
  case TYP_PLAYER:		/* our most complex */
    if (dest == -3) {		/* home */
      if ((get_int_elt(object, HOME, &dest) == -1) || !exists_object(dest)) {
	dest = mudconf.starting_loc;
      }
    }
    if (((Typeof(object) == TYP_PLAYER) && !controls(player, cause, loc)
	 && !isJUMP_OK(loc)) || islocked(player, cause, loc, TELOUTLOCK)
	|| (!controls(player, cause, dest) && (!isJUMP_OK(dest)
		 || islocked(player, cause, dest, TELINLOCK)))
	|| (!controls(player, cause, object) && !isJUMP_OK(object))) {
      if(!(switches & CMD_QUIET))
        notify_player(player, cause, player, "Permission denied.", NOT_QUIET);
      return;
    }
    if (Typeof(dest) == TYP_EXIT) {
      if(!(switches & CMD_QUIET))
        notify_player(player, cause, player,
		      "You can't teleport anything to that!", NOT_QUIET);
      return;
    }
    if (!legal_thingloc_check(object, dest)) {
      if(!(switches & CMD_QUIET))
        notify_player(player, cause, player,
		      ((player == object) ? "You wouldn't fit!"
					  : "It wouldn't fit!"), NOT_QUIET);
      return;
    }

    act_object(object, cause, object, (char *) NULL,
    		(!isDARK(object)) ? OTELEPORT : (char *) NULL, ATELEPORT,
		-1, (char *) NULL, (char *) NULL);
    move_player_leave(object, cause, loc, 0,
    		      "You feel a wrenching sensation...");
    act_object(object, cause, object, (char *) NULL,
	       (!isDARK(object)) ? OXTELEPORT : (char *) NULL, (char *)NULL,
	       dest, (char *) NULL, (char *) NULL);
    move_player_arrive(object, cause, dest, 0);
    flush_room(loc, cause);
    look_location(object, cause, 0);
    if(has_html(object))
      html_anchor_location(object, cause);

    break;
  default:
    if(!(switches & CMD_QUIET))
      notify_player(player, cause, player,
		    "You have no business teleporting that!", NOT_QUIET);
    return;
  }
  if(!(switches & CMD_QUIET))
    notify_player(player, cause, player, "Teleported.", NOT_QUIET);
}

/* Given an exit, this will try to shove the player through it, or whatever. */

static int go_depth = 0;	/* recursive do_go_attempt() */

void do_go_attempt(player, cause, here, theex)
    int player, cause, here, theex;
{
  int *dests, dest, homed;
  int source, dloc, sloc;
  int indx;

  if (go_depth > mudconf.exit_depth) {		/* this is bad. */
    notify_player(player, cause, player, 
		  "Recursion depth exceeded in move.", 0);
    return;
  }

  if (get_array_elt(theex, DESTS, &dests) == -1) {
    notify_bad(player);
    return;
  }
  if (dests == (int *)NULL) {	/* not linked */
    act_object(player, cause, theex, FAIL, OFAIL, AFAIL, -1,
    	       "You can't go that way.", (char *) NULL);
    stamp(theex, STAMP_USED);
    return;
  }

  for(indx = 1; indx <= dests[0]; indx++) {
    if (islocked(player, cause, theex, LOCK)
  	|| (!exists_object(dests[indx]) && (dests[indx] != -3))
	|| ((indx > 1) && !isACTION(theex))) {
      act_object(player, cause, theex, FAIL, OFAIL, AFAIL, -1,
      		 "You can't go that way.", (char *) NULL);
      stamp(theex, STAMP_USED);
      continue;		/* next dest. */
    }
    if (dests[indx] == -3) {		/* home */
      if ((get_int_elt(player, HOME, &dest) == -1) || !exists_object(dest)) {
        notify_player(player, cause, player, "Your home does not exist!", 0);
        dest = mudconf.starting_loc;
      }
      homed = 1;
    } else {
      dest = dests[indx];
      homed = 0;
    }

    if ((!isACTION(theex) && !isEXIT(dest)) || homed) {
      move_player_room(player, cause, theex, here, dest);
      reward_move(player, cause, dest);
      /* even if it's not a room, this will work. */
      continue;		/* next dest. */
    }
    /* now handle "actions" */
    if (get_int_elt(theex, LOC, &source) == -1) {
      notify_bad(player);
      return;
    }
    switch (Typeof(dest)) {
    case TYP_EXIT:		/* meta-exit */
      go_depth++;

      /* from move_player_room() */
      act_object(player, cause, theex, SUC,
                 (isDARK(player) || (isDARK(here) && !isLIGHT(theex))) ?
		 (char *) NULL : OSUC, ASUC, -1, (char *) NULL, (char *) NULL);
      stamp(theex, STAMP_USED);

      do_go_attempt(player, cause, here, dest);
      break;
    case TYP_ROOM:		/* destination is room */
      move_player_room(player, cause, theex, here, dest);
      reward_move(player, cause, dest);
      break;
    case TYP_PLAYER:		/* destination is player */
      if (isHIDDEN(dest) && !isWIZARD(player)) {
        notify_player(player, cause, player,
	     "That player doesn't wish to be disturbed at the moment.", 0);
        continue;		/* next dest */
      }
      if ((get_int_elt(dest, LOC, &dloc) == -1) || !exists_object(dloc)) {
        notify_bad(player);
        return;
      }
      move_player_room(player, cause, theex, here, dloc);
      reward_move(player, cause, dloc);
      break;
    case TYP_THING:		/* our most complex */
      switch (Typeof(source)) {
      case TYP_THING:		/* object -> object */
        if ((get_int_elt(dest, LOC, &dloc) == -1) || !exists_object(dloc)) {
	  notify_bad(player);
	  return;
        }
        move_object(dest, cause, dloc, here);
        if (!isSTICKY(theex)) {
	  /* sloc should be here, but might not be for some odd reason */
	  if (get_int_elt(source, LOC, &sloc) == -1) {
	    notify_bad(player);
	    return;
	  }
	  send_home(source, cause, sloc);
        }
        /* me made it, yay */
        act_object(player, cause, theex, SUC,
		   (isDARK(player) || (isDARK(here) && !isLIGHT(theex))) ?
		   (char *) NULL : OSUC, ASUC, -1, "Ok.", (char *) NULL);
        stamp(theex, STAMP_USED);
        break;
      case TYP_PLAYER:		/* player -> object */
        if ((get_int_elt(dest, LOC, &dloc) == -1) || !exists_object(dloc)) {
	  notify_bad(player);
	  return;
        }
        if (!isEXTERNAL(theex)) {	/* uh oh */
	  move_player_room(player, cause, theex, here, dloc);
	  reward_move(player, cause, dloc);
	  continue;		/* next dest */
        }
        move_object(dest, cause, dloc, player);
        act_object(player, cause, theex, SUC,
		   (isDARK(player) || (isDARK(here) && !isLIGHT(theex))) ?
		   (char *) NULL : OSUC, ASUC, -1, "Ok.", (char *) NULL);
        stamp(theex, STAMP_USED);
        break;
      case TYP_ROOM:		/* room -> object */
        if ((get_int_elt(dest, LOC, &dloc) == -1) || !exists_object(dloc)) {
	  notify_bad(player);
	  return;
        }
        move_object(dest, cause, dloc, here);
        act_object(player, cause, theex, SUC,
		   (isDARK(player) || (isDARK(here) && !isLIGHT(theex))) ?
		   (char *) NULL : OSUC, ASUC, -1, "Ok.", (char *) NULL);
        stamp(theex, STAMP_USED);
        break;
      default:			/* what the fuck? */
        act_object(player, cause, theex, FAIL, OFAIL, AFAIL, -1,
      			  "You can't go that way.", (char *) NULL);
        break;
      }
      break;
    default:			/* what the fuck? */
      act_object(player, cause, theex, FAIL, OFAIL, AFAIL, -1,
      		 "You can't go that way.", (char *) NULL);
      break;
    }
  }
  go_depth = 0;
}

void move_object(obj, cause, here, dest)
    int obj, cause, here, dest;
{
  int home;

  if (isROOM(obj) || isEXIT(obj))
    return;
  if (dest == -3) {
    if (get_int_elt(obj, HOME, &home) == -1)
      return;
    dest = home;
  }
  if (!exists_object(dest))
    return;
  if (dest != here) {
    move_player_leave(obj, cause, here, 0, (char *) NULL);
    move_player_arrive(obj, cause, dest, 0);
    flush_room(here, cause);
  }
  look_location(obj, cause, 0);
  if(has_html(obj))
    html_anchor_location(obj, cause);
}

void move_player_room(player, cause, theex, here, dest)
    int player, cause, theex, here, dest;
{
  act_object(player, cause, theex, SUC,
  	     (isDARK(player) || (isDARK(here) && !isLIGHT(theex))) ?
	     (char *) NULL : OSUC, ASUC, -1, (char *) NULL, (char *) NULL);
  stamp(theex, STAMP_USED);

  if (dest != here) {
    move_player_leave(player, cause, here, isDARK(theex), (char *) NULL);
    act_object(player, cause, theex, DROP,
    	       (isDARK(player) || (isDARK(dest) && !isLIGHT(theex))) ?
	       (char *) NULL : ODROP, ADROP, dest, (char *) NULL,
	       (char *) NULL);
    move_player_arrive(player, cause, dest, isDARK(theex));

    /* Get the tag-alongs. */
    if(mudconf.enable_groups)
      group_follow(player, cause, here, dest);

    flush_room(here, cause);
  } else
    act_object(player, cause, theex, DROP,
    	       (isDARK(player) || (isDARK(dest) && !isLIGHT(theex))) ?
	       (char *) NULL : ODROP, ADROP, -1, (char *) NULL, (char *) NULL);
  look_location(player, cause, 0);
  if(has_html(player))
    html_anchor_location(player, cause);
}

void move_player_leave(player, cause, loc, darkexit, str)
    int player, cause, loc, darkexit;
    char *str;
{
  char work[MEDBUFFSIZ];
  char *name;

  if (str != (char *) NULL)
    notify_player(player, cause, player, str, 0);
  if (can_hear(player) && !isDARK(player) && !isDARK(loc) && !darkexit) {
    if(get_str_elt(player, NAME, &name) == -1)
      name = "???";
    snprintf(work, sizeof(work), "%s has left.", name);
    notify_oall(player, cause, player, work, NOT_NOPUPPET);
  }
  list_drop(player, loc, CONTENTS);
}

void move_player_arrive(player, cause, dest, darkexit)
    int player, cause, dest, darkexit;
{
  char work[BUFFSIZ];
  char *name;

  if (set_int_elt(player, LOC, dest) == -1)
    logfile(LOG_ERROR, "move_player: couldn't set location of player #%d\n",
	    player);
  list_add(player, dest, CONTENTS);
  if (can_hear(player) && !isDARK(player) && !isDARK(dest) && !darkexit) {
    if(get_str_elt(player, NAME, &name) == -1)
      name = "???";
    snprintf(work, sizeof(work), "%s has arrived.", name);
    notify_oall(player, cause, player, work, NOT_NOPUPPET);
  }
}

/* handle sticky droptos. */
static void flush_room(room, cause)
    int room, cause;
{
  int dest, list, next;

  if (has_dropto(room) && isSTICKY(room)) {
    if (get_int_elt(room, DROPTO, &dest) == -1) {
      logfile(LOG_ERROR, "flush_room: bad dropto reference on object #%d\n",
	      room);
      return;
    }
    /* If no dropto, or it's to somewhere that doesn't exist.. */
    if (dest == -1 || (dest != -3 && !exists_object(dest)))
      return;
    /* OK. See if there are any players here. */

    if (get_int_elt(room, CONTENTS, &list) == -1) {
      logfile(LOG_ERROR, "flush_room: bad contents reference on object #%d\n",
	      room);
      return;
    }
    while (list != -1) {
      if (can_hear(list))
	return;
      if (get_int_elt(list, NEXT, &list) == -1) {
	logfile(LOG_ERROR, "flush_room: bad next reference on object #%d\n",
		list);
	return;
      }
    }
    /* No players left, toss everything here down the dropto */
    if (get_int_elt(room, CONTENTS, &list) == -1) {
      logfile(LOG_ERROR, "flush_room: bad contents reference on object #%d\n",
	      room);
      return;
    }
    while (list != -1) {
      if (get_int_elt(list, NEXT, &next) == -1) {
	logfile(LOG_ERROR, "flush_room: bad next reference on object #%d\n",
		list);
	return;
      }
      move_object(list, cause, room, (isSTICKY(list)) ? -3 : dest);
      list = next;
    }
  }
}

void send_home(obj, cause, loc)
    int obj, cause, loc;
{
  int home;

  if (!has_home(obj))
    return;
  if (get_int_elt(obj, HOME, &home) == -1) {
    logfile(LOG_ERROR, "send_home: could not get home of #%d\n", obj);
    return;
  }
  move_player_leave(obj, cause, loc, 0, (char *) NULL);

  if (!exists_object(home))
    home = mudconf.starting_loc;
  move_player_arrive(obj, cause, home, 0);
  look_location(obj, cause, 0);
  if(has_html(obj))
    html_anchor_location(obj, cause);
}
