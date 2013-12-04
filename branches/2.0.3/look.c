/* look.c */

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
#ifdef TM_IN_SYS_TIME
#include <sys/time.h>
#else
#include <time.h>
#endif			/* TM_IN_SYS_TIME */
#ifdef HAVE_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif			/* HAVE_STRING_H */

#include "conf.h"
#include "teeny.h"
#include "commands.h"
#include "externs.h"

/* routines dealing with looking at things */

static char noloc[] = "You have no location!";

VOID do_look(player, cause, switches, argone)
    int player, cause, switches;
    char *argone;
{
  int loc, here, obj;
  register int isfar;

  if (!argone || !*argone || !strcasecmp(argone, "here"))
    look_location(player, cause);
  else {
    if ((get_int_elt(player, LOC, &here) == -1) || !exists_object(here)) {
      notify_player(player, cause, player, noloc, 0);
      return;
    }
    obj = resolve_object(player, cause, argone, RSLV_EXITS | RSLV_NOISY);
    if(obj == -1)
      return;
    if ((get_int_elt(obj, LOC, &loc) != -1) && exists_object(loc)) {
      isfar = ((loc != here) && (loc != player));
      if ((isfar || isROOM(obj)) && !controls(player, cause, obj)) {
	notify_player(player, cause, player,
		      "That's much too far away to see.", 0);
      } else {
	look_thing(player, cause, obj, isfar);
      }
    } else
      notify_bad(player);
  }
}

VOID do_examine(player, cause, switches, argone, argtwo)
    int player, cause, switches;
    char *argone, *argtwo;
{
  int obj, num, *dests;
  char *msg = (char *)NULL;
  char *name;
  time_t timestamp;
  char buf[MEDBUFFSIZ];

  if ((argone == (char *)NULL) || (argone[0] == '\0')) {
    if ((get_int_elt(player, LOC, &obj) == -1) || !exists_object(obj)) {
      notify_player(player, cause, player, noloc, 0);
      return;
    }
  } else {
    if((argtwo == (char *)NULL) || (argtwo[0] == '\0')) {
      /* Fish it out of the first argument, instead. */
      for(argtwo = argone; *argtwo && (*argtwo != '/'); argtwo++);
      if(*argtwo == '/')
	*argtwo++ = '\0';
    }
    obj = resolve_object(player, cause, argone, RSLV_EXITS | RSLV_NOISY);
    if(obj == -1)
      return;
  }
  if (!exists_object(obj)) {
    notify_player(player, cause, player, "No such object.", 0);
    return;
  }
  if (!isVISUAL(obj) && !controls(player, cause, obj)
      && !econtrols(player, cause, obj)) {
    if(!(switches & EXAMINE_ATTRS)) {
      look_thing(player, cause, obj, 1);
      /* Owner */
      if ((get_int_elt(obj, OWNER, &num) != -1) &&
	  (get_str_elt(num, NAME, &name) != -1)) {
        snprintf(buf, sizeof(buf), "Owner: %s", name);
        notify_player(player, cause, player, buf, 0);
      } else
        notify_player(player, cause, player, "Owner: ???", 0);
    } else
      notify_player(player, cause, player, "Permission denied.", 0);
    return;
  }

  /* name */
  notify_player(player, cause, player, display_name(player, cause, obj), 0);

  if(switches & EXAMINE_ATTRS) {
    /* attributes */
    if (!(switches & EXAMINE_PARENT)) {
      display_attributes(player, cause, obj, argtwo);
    } else {
      display_attributes_parent(player, cause, obj, argtwo);
    }
    return;
  }

  /* flags */
  display_flags(player, cause, obj);

  /* Owner */
  if ((get_int_elt(obj, OWNER, &num) != -1) &&
      (get_str_elt(num, NAME, &name) != -1)) {
    snprintf(buf, sizeof(buf), "Owner: %s ", name);
  } else {
    strcpy(buf, "Owner: ??? ");
  }

  /* pennies */
  if (mudconf.enable_money && has_pennies(obj)) {
    if (get_int_elt(obj, PENNIES, &num) != -1) {
      snprintf(&buf[strlen(buf)], sizeof(buf) - strlen(buf),
	       "  Pennies: %d", num);
    } else {
      strcat(buf, "  Pennies: ???");
    }
  }
  notify_player(player, cause, player, buf, 0);

  /* charges, semaphores, cost, queue */
  if(get_int_elt(obj, CHARGES, &num) != -1) {
    if(num >= 0) {
      snprintf(buf, sizeof(buf), "Charges: %d", num);
    } else {
      strcpy(buf, "Charges: *NOT CHARGED*");
    }
  } else {
    strcpy(buf, "Charges: ???");
  }
  if(get_int_elt(obj, SEMAPHORES, &num) != -1) {
    snprintf(&buf[strlen(buf)], sizeof(buf) - strlen(buf),
	     "  Semaphores: %d", num);
  } else {
    strcat(buf, "  Semaphores: ???");
  }
  if(mudconf.enable_money) {
    if(get_int_elt(obj, COST, &num) != -1) {
      snprintf(&buf[strlen(buf)], sizeof(buf) - strlen(buf), "  Cost: %d", num);
    } else {
      strcat(buf, "  Cost: ???");
    }
  }
  if(isWIZARD(player) && isPLAYER(obj)) {
    if(get_int_elt(obj, QUEUE, &num) != -1) {
      snprintf(&buf[strlen(buf)], sizeof(buf) - strlen(buf),
	       "  Queue: %d", num);
    } else {
      strcat(buf, "  Queue: ???");
    }
  }
  notify_player(player, cause, player, buf, 0);

  if (get_int_elt(obj, TIMESTAMP, (int *) &timestamp) != -1) {
    if (isPLAYER(obj)) {
      strftime(buf, sizeof(buf),
	       "Last login: %a %b %e %H:%M:%S %Z %Y", localtime(&timestamp));
    } else {
      strftime(buf, sizeof(buf),
	       "Timestamp: %a %b %e %H:%M:%S %Z %Y", localtime(&timestamp));
    }
    notify_player(player, cause, player, buf, 0);
  }
  if (get_int_elt(obj, CREATESTAMP, (int *) &timestamp) != -1) {
    strftime(buf, sizeof(buf),
	     "Created: %a %b %e %H:%M:%S %Z %Y", localtime(&timestamp));
    if (get_int_elt(obj, USES, &num) != -1) {
      snprintf(&buf[strlen(buf)], sizeof(buf) - strlen(buf),
	       "  Use count: %d", num);
      notify_player(player, cause, player, buf, 0);
    }
  }

  /* attributes */
  if (!(switches & EXAMINE_PARENT)) {
    display_attributes(player, cause, obj, argtwo);
  } else {
    display_attributes_parent(player, cause, obj, argtwo);
  }

  if (Typeof(obj) != TYP_EXIT) {
    /* home and dropto are the same. */
    if (get_int_elt(obj, HOME, &num) != -1) {
      switch (Typeof(obj)) {
      case TYP_ROOM:
        strcpy(buf, "Dropto: ");
        msg = (num == -3) ? "*HOME*" : "None.";
        break;
      case TYP_THING:
      case TYP_PLAYER:
        strcpy(buf, "Home: ");
        msg = "Does not exist!";
        break;
      }
      if (exists_object(num)) {
        strcat(buf, display_name(player, cause, num));
      } else {
        strcat(buf, msg);
      }
      notify_player(player, cause, player, buf, 0);
    }
  } else {			/* exit destinations */
    if(get_array_elt(obj, DESTS, &dests) != -1) {
      register int indx;

      if(dests == (int *)NULL) {
        notify_player(player, cause, player, "Destination: *UNLINKED*", 0);
      } if(dests[0] == 1) {
        if(exists_object(dests[1])) {
	  snprintf(buf, sizeof(buf), "Destination: %s", 
	  	   display_name(player, cause, dests[1]));
	  notify_player(player, cause, player, buf, 0);
	} else if(dests[1] == -3) {
	  snprintf(buf, sizeof(buf), "Destination: *HOME*", dests[1]);
	} else {
	  snprintf(buf, sizeof(buf), "Destination: *BAD OBJECT #%d*",
	           dests[1]);
	}
      } else {
        for(indx = 1; indx <= dests[0]; indx++) {
	  if(exists_object(dests[indx])) {
	    snprintf(buf, sizeof(buf), "Destination %d: %s", indx,
	    	     display_name(player, cause, dests[indx]));

	    notify_player(player, cause, player, buf, 0);
	  } else if(dests[indx] == -3) {
	    snprintf(buf, sizeof(buf), "Destination %d: *HOME*", indx);
	  } else {
	    snprintf(buf, sizeof(buf), "Destination %d: *BAD OBJECT #%d*",
	    	     indx, dests[indx]);
	  }
	}
      }
    }
  }

  /* location */
  if (get_int_elt(obj, LOC, &num) != -1) {
    if (exists_object(num)) {
      snprintf(buf, sizeof(buf), "Location: %s",
	       display_name(player, cause, num));
    } else {
      snprintf(buf, sizeof(buf), "Location: *BAD OBJECT #%d*", num);
    }
    notify_player(player, cause, player, buf, 0);
  }
  /* parent */
  if (get_int_elt(obj, PARENT, &num) != -1) {
    if (num != -1) {
      if (exists_object(num)) {
	snprintf(buf, sizeof(buf), "Parent: %s",
		 display_name(player, cause, num));
      } else {
	snprintf(buf, sizeof(buf), "Parent: *BAD OBJECT #%d*", num);
      }
      notify_player(player, cause, player, buf, 0);
    }
  }
  if (has_contents(obj)) {
    /* contents list */
    if (get_int_elt(obj, CONTENTS, &num) != -1) {
      if (num != -1) {
	switch(Typeof(obj)) {
	case TYP_ROOM:
	  notify_player(player, cause, player, "Contents:", 0);
	  break;
	case TYP_PLAYER:
	  notify_player(player, cause, player, "Carrying:", 0);
	  break;
	default:
	  notify_player(player, cause, player, "Containing:", 0);
	  break;
	}
	notify_list(player, cause, player, num, NOT_DARKOK);
      } else {
	notify_player(player, cause, player, "No contents.", 0);
      }
    }
  }
  if (has_exits(obj)) {
    /* exits list */
    if (get_int_elt(obj, EXITS, &num) != -1) {
      if (num != -1) {
	notify_player(player, cause, player, "Exits:", 0);
	notify_list(player, cause, player, num, NOT_DARKOK);
      } else {
	notify_player(player, cause, player, "No exits.", 0);
      }
    }
  }
  if (has_rooms(obj) && (switches & EXAMINE_ROOMS) &&
      (get_int_elt(obj, ROOMS, &num) != -1)) {
    if (num != -1) {
      notify_player(player, cause, player, "Rooms:", 0);
      notify_list(player, cause, player, num, NOT_DARKOK);
    } else {
      notify_player(player, cause, player, "No rooms.", 0);
    }
  }
}

VOID do_inventory(player, cause, switches)
    int player, cause, switches;
{
  register int header = 0;
  int list, owner, pennies;
  char buf[MEDBUFFSIZ];

  if (get_int_elt(player, CONTENTS, &list) != -1) {
    if (list != -1) {
      notify_player(player, cause, player, "You are carrying:", 0);
      header++;
      notify_list(player, cause, player, list, NOT_DARKOK);
    }
  }
  if (get_int_elt(player, EXITS, &list) != -1) {
    if (list != -1) {
      if (!header) {
	notify_player(player, cause, player, "You are carrying:", 0);
	header++;
      }
      notify_list(player, cause, player, list, NOT_DARKOK);
    }
  }
  if (!header)
    notify_player(player, cause, player, "You aren't carrying anything.", 0);

  if (mudconf.enable_money && (get_int_elt(player, OWNER, &owner) != -1) &&
      (get_int_elt(owner, PENNIES, &pennies) != -1)) {
    snprintf(buf, sizeof(buf), "You have %d pennies.", pennies);
    notify_player(player, cause, player, buf, 0);
  }
}

VOID do_score(player, cause, switches)
    int player, cause, switches;
{
  int owner, pennies;
  char buf[MEDBUFFSIZ];

  if (mudconf.enable_money) {
    if ((get_int_elt(player, OWNER, &owner) != -1) &&
        (get_int_elt(owner, PENNIES, &pennies) != -1)) {
      snprintf(buf, sizeof(buf), "You have %d pennies.", pennies);
      notify_player(player, cause, player, buf, 0);
    } else {
      notify_bad(player);
    }
  } else
    notify_player(player, cause, player, "Pennies are not enabled.", 0);
}

/* look at a thing */
void look_thing(player, cause, thing, quiet)
    int player, cause, thing, quiet;
{
  int contents;

  if (!can_hear(player))
    return;

  stamp(thing, STAMP_USED);
  if(controls(player, cause, thing) || isVISUAL(thing))
    notify_player(player, cause, player,
		  display_name(player, cause, thing), 0);
  act_object(player, cause, thing, DESC, (quiet) ? (char *) NULL : ODESC,
  	     ADESC, -1, "You see nothing special.", (char *) NULL);

  if (!has_contents(thing) || isOPAQUE(thing)
      || (get_int_elt(thing, CONTENTS, &contents) == -1))
    return;
  if (contents != -1) {
    if (can_see_anything(player, cause, thing)) {
      switch (Typeof(thing)) {
      case TYP_ROOM:
	notify_player(player, cause, player, "Contents:", 0);
	break;
      case TYP_PLAYER:
	notify_player(player, cause, player, "Carrying:", 0);
	break;
      default:
	notify_player(player, cause, player, "Containing:", 0);
	break;
      }
      notify_list(player, cause, player, contents, 0);
    }
  }
}

/* look at location */
void look_location(player, cause)
    int player, cause;
{
  int here, contents, exits;

  if (!can_hear(player))
    return;

  if ((get_int_elt(player, LOC, &here) == -1) || !exists_object(here)) {
    notify_player(player, cause, player, "You have no location!", 0);
    return;
  }
  stamp(here, STAMP_USED);
  notify_player(player, cause, player, display_name(player, cause, here), 0);

  act_object(player, cause, here, (isROOM(here)) ? DESC : IDESC, ODESC,
  	     ADESC, -1, (char *) NULL, (char *) NULL);
  if (isROOM(here)) {
    act_object(player, cause, here, IDESC, (char *) NULL, (char *) NULL,
    	       -1, (char *) NULL, (char *) NULL);
    if (islocked(player, cause, here, LOCK)) {
      act_object(player, cause, here, FAIL, OFAIL, AFAIL, -1, (char *) NULL,
      			(char *) NULL);
    } else {
      act_object(player, cause, here, SUC, OSUC, ASUC, -1, (char *) NULL,
      			(char *) NULL);
    }
  }
  if (has_contents(here) && can_see_anything(player, cause, here)) {
    if (get_int_elt(here, CONTENTS, &contents) == -1) {
      logfile(LOG_ERROR,
	      "look_location: bad contents reference on object #%d\n", here);
      return;
    }
    notify_player(player, cause, player, "Contents:", 0);
    notify_list(player, cause, player, contents, 0);
  }
  if (has_exits(here)) {
    if (get_int_elt(here, EXITS, &exits) == -1) {
      logfile(LOG_ERROR, "look_location: bad exits reference on object #%d\n",
	      here);
      return;
    }
    notify_exits(player, cause, player, exits);
  }
}

/* Returns 1 if the player can see anything, 0 otherwise. */
int can_see_anything(player, cause, location)
    int player, cause, location;
{
  int list, contents;

  if (location == -1) {
    /* get their location */
    if ((get_int_elt(player, LOC, &location) == -1) ||
	!exists_object(location)) {
      logfile(LOG_ERROR,
	      "can_see_anything: couldn't get location of player #%d\n",
	      player);
      notify_bad(player);
      return (0);
    }
  }
  if (get_int_elt(location, CONTENTS, &contents) == -1) {
    logfile(LOG_ERROR, "can_see_anything: bad contents reference on #%d\n",
	    location);
    notify_bad(player);
    return (0);
  }
  if (contents == player) {
    int foo;

    if (get_int_elt(player, NEXT, &foo) == -1) {
      logfile(LOG_ERROR, "can_see_anything: bad next reference on #%d\n",
	      player);
      notify_bad(player);
      return (0);
    }
    if (foo == -1)
      return (0);
  }
  /* still here... hmm... loop through list. */
  list = contents;
  while (list != -1) {
    if (can_see(player, cause, list))
      return (1);
    if (get_int_elt(list, NEXT, &list) == -1) {
      logfile(LOG_ERROR, "can_see_anything: bad next reference on #%d\n", list);
      notify_bad(player);
      return (0);
    }
  }
  /* they cannot see a damn thing! */
  return (0);
}

/* Returns a 1 if player can see that specific object, 0 otherwise. */
int can_see(player, cause, obj)
    int player, cause, obj;
{
  int loc, owner;

  if (player == obj)
    return (0);
  if (mudconf.dark_sleep && isPLAYER(obj) && !isALIVE(obj))
    return (0);
  if (isROOM(obj))
    return (0);			/* wee... */
  if ((get_int_elt(obj, LOC, &loc) == -1) || !exists_object(loc)) {
    logfile(LOG_ERROR, "can_see: couldn't get location of object #%d\n", obj);
    return (0);
  }
  if (isDARK(loc)) {
    if (isLIGHT(obj))
      return(1);
    if (get_int_elt(obj, OWNER, &owner) == -1) {
      logfile(LOG_ERROR, "can_see: bad owner reference on object #%d\n", obj);
      return (0);
    }
    /* this is so a wizz won't see so much junk in a dark room. */
    if (owner != player)
      return (0);
  }
  if (!isDARK(obj))
    return (1);
  if (isDARK(obj) && controls(player, cause, obj))
    return (1);

  return (0);
}

VOID do_use(player, cause, switches, argone)
    int player, cause, switches;
    char *argone;
{
  int obj;

  if((argone == (char *)NULL) || (argone[0] == '\0')) {
    notify_player(player, cause, player, "But what do you want to use?", 0);
    return;
  }

  obj = resolve_object(player, cause, argone, RSLV_EXITS | RSLV_NOISY);
  if(obj == -1)
    return;
  if(islocked(player, cause, obj, ULOCK)) {
    notify_player(player, cause, player, "You can't use that.", 0);
    return;
  }

  act_object(player, cause, obj, USE, OUSE, AUSE, -1, 
  	     "There is no obvious way to use that.", (char *)NULL);
}
