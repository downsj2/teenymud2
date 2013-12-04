/* recycle.c */

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
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif			/* HAVE_UNISTD_H */

#include "conf.h"
#include "teeny.h"
#include "ptable.h"
#include "commands.h"
#include "externs.h"

#ifndef F_OK
#define F_OK	0
#endif

/* object recycling system */

static int recycle_clean _ANSI_ARGS_((int, int, int, int *));

VOID do_recycle(player, cause, switches, argone)
    int player, cause, switches;
    char *argone;
{
  int obj, ret;
  char buff[BUFFSIZ];

  if((argone == (char *)NULL) || (argone[0] == '\0')) {
    if(!(switches & CMD_QUIET))
      notify_player(player, cause, player, "Recycle what?", NOT_QUIET);
    return;
  }

  obj = resolve_object(player, cause, argone,
  		       (!(switches & CMD_QUIET) ? RSLV_EXITS|RSLV_NOISY
		       				: RSLV_EXITS));
  if(obj == -1)
    return;

  if(!isDESTROY_OK(obj)) {
    if(!(switches & RECYCLE_OVERRIDE)) {
      int plown, objown;
      char fbuf[152];

      if((get_int_elt(player, OWNER, &plown) == -1)
         || (get_int_elt(obj, OWNER, &objown) == -1)) {
        notify_bad(player);
        return;
      }
      if(objown != plown) {
        if(!(switches & CMD_QUIET))
          notify_player(player, cause, player, "You can't recycle that!",
	  		NOT_QUIET);
        return;
      }

      /* Very rudimentary protection. */
      snprintf(fbuf, sizeof(fbuf), "%s#%d", mudconf.files_path, obj);
      if(access(fbuf, F_OK) == 0) {
        if(!(switches & CMD_QUIET))
          notify_player(player, cause, player, "You can't recycle that!",
	  		NOT_QUIET);
	return;
      }
    } else {
      if(isEXIT(obj)) {
        int loc;

        if(get_int_elt(obj, LOC, &loc) == -1) {
	  notify_bad(player);
	  return;
        }
        if(!controls(player, cause, obj)
	   && !controls(player, cause, loc)) {
	  if(!(switches & CMD_QUIET))
	    notify_player(player, cause, player, "You can't recycle that!",
	    		  NOT_QUIET);
	  return;
        }
      } else {
        if(!controls(player, cause, obj)) {
	  if(!(switches & CMD_QUIET))
            notify_player(player, cause, player, "You can't recycle that!",
	    		  NOT_QUIET);
          return;
        }
      }
    }
  }

  if((obj == mudconf.starting_loc) || (obj == mudconf.root_location)) {
    if(!(switches & CMD_QUIET))
      notify_player(player, cause, player, "You may not recycle that.",
      		    NOT_QUIET);
    return;
  }
  if(isPLAYER(obj)) {
    if(!(switches & CMD_QUIET))
      notify_player(player, cause, player, "You may not recycle players.",
      		    NOT_QUIET);
    return;
  }

  ret = recycle_obj(player, cause, obj);
  switch(ret) {
  case -1:
    notify_bad(player);
    break;
  case 0:
    if(!(switches & CMD_QUIET))
      notify_player(player, cause, player, "Nothing to recycle?!", NOT_QUIET);
    break;
  case 1:
    if(!(switches & CMD_QUIET))
      notify_player(player, cause, player, "Thank you for recycling.",
      		    NOT_QUIET);
    break;
  default:
    if(!(switches & CMD_QUIET)) {
      snprintf(buff, sizeof(buff), "Thank you for recycling %d objects.", ret);
      notify_player(player, cause, player, buff, NOT_QUIET);
    }
  }
}

int recycle_owned(player, cause, object)
    int player, cause, object;
{
  register int index, ret;
  int owner, total = 0;

  for(index = 0; index < mudstat.total_objects; index++) {
    if(!exists_object(index) || (index == object))
      continue;
    if(get_int_elt(index, OWNER, &owner) == -1) {
      logfile(LOG_ERROR, "recycle_owned: bad owner reference on object #%d\n",
	      index);
      return(-1);
    }
    if(owner == object) {
      ret = recycle_obj(player, cause, index);
      if(ret == -1)
	return(-1);
      total += ret;
    }
  }
  return(total);
}

int recycle_obj(player, cause, object)
    int player, cause, object;
{
  int loc, owner;
  int total = 0;

  /* some sanity checking. */
  if(!exists_object(object)
     || (get_int_elt(object, LOC, &loc) == -1)
     || (get_int_elt(object, OWNER, &owner) == -1)) {
    logfile(LOG_ERROR, "recycle_obj: bad reference to object #%d\n", object);
    return(-1);
  }

  switch(Typeof(object)) {
  case TYP_PLAYER:
    notify_player(object, cause, object,
		  "You feel the universe collapsing around you...", 0);
    if(isALIVE(object))
      tcp_logoff(object);
    ptable_delete(object);
  case TYP_THING:
    move_player_leave(object, cause, loc, 0, "You've been recycled!");

    if(recycle_clean(player, cause, object, &total) == -1)
      return(-1);

    if(isTHING(object))
      reward_money(owner, mudconf.thing_cost);

    destroy_obj(object);
    total++;
    break;
  case TYP_EXIT:
    list_drop(object, loc, EXITS);

    reward_money(owner, mudconf.exit_cost);

    destroy_obj(object);
    total++;
    break;
  case TYP_ROOM:
    notify_contents(object, cause, object,
		    "The room shakes and begins to crumble...", 0);

    list_drop(object, loc, ROOMS);

    if(recycle_clean(player, cause, object, &total) == -1)
      return(-1);

    reward_money(owner, mudconf.room_cost);

    destroy_obj(object);
    total++;
  }

  return(total);
}

static int recycle_clean(player, cause, object, total)
    int player, cause, object, *total;
{
  register int index;
  int home, *dests, parent, list, next;
  static char err[] =
	"recycle_clean: bad element reference on object #%d\n";

  /* fix references to recycled objects */
  for(index = 0; index < mudstat.total_objects; index++) {
    if(!exists_object(index))
      continue;

    if(!isEXIT(index)) {
      if(get_int_elt(index, HOME, &home) == -1) {
	logfile(LOG_ERROR, err, index); 
        return(-1);
      }
    }

    switch(Typeof(index)) {
    case TYP_PLAYER:
      if(home == object) {
	if(set_int_elt(index, HOME, mudconf.starting_loc) == -1) {
	  logfile(LOG_ERROR, err, index);
	  return(-1);
	}
      }
      break;
    case TYP_THING:
      if(home == object) {
	int owner, ownhome;

	if(get_int_elt(index, OWNER, &owner) == -1) {
	  logfile(LOG_ERROR, err, index);
	  return(-1);
	}
	if(get_int_elt(owner, HOME, &ownhome) == -1) {
	  logfile(LOG_ERROR, err, owner);
	  return(-1);
	}

	if(ownhome != object) {
	  if(set_int_elt(index, HOME, ownhome) == -1) {
	    logfile(LOG_ERROR, err, index);
	    return(-1);
	  }
	} else {
	  if(set_int_elt(index, HOME, mudconf.starting_loc) == -1) {
	    logfile(LOG_ERROR, err, index);
	    return(-1);
	  }
	}
      }
      break;
    case TYP_ROOM:
      /* really drop-to */
      if(home == object) {
	if(set_int_elt(index, DROPTO, -1) == -1) {
	  logfile(LOG_ERROR, err, index);
	  return(-1);
	}
      }
      break;
    case TYP_EXIT:
      if(get_array_elt(index, DESTS, &dests) == -1) {
	logfile(LOG_ERROR, err, index);
	return(-1);
      }

      if(dests != (int *)NULL) {
	register int d, t, change = 0;
	int new[MAXLINKS + 1];

	for(d = 1; d <= dests[0]; d++) {
	  if(dests[d] == object)
	    change++;
	}
	if(change) {
	  t = 0;

	  for(d = 1; d <= dests[0]; d++) {
	    if(dests[d] != object)
	      new[++t] = dests[d];
	  }
	  if(t) {
	    new[0] = t;
	    if(set_array_elt(index, DESTS, new) == -1) {
	      logfile(LOG_ERROR, err, index);
	      return(-1);
	    }
	  } else {
	    if(set_array_elt(index, DESTS, (int *)NULL) == -1) {
	      logfile(LOG_ERROR, err, index);
	      return(-1);
	    }
	  }
	}
      }
      break;
    }

    if(get_int_elt(index, PARENT, &parent) == -1) {
      logfile(LOG_ERROR, err, index);
      return(-1);
    }
    if(parent == object) {
      if(set_int_elt(index, PARENT, -1) == -1) {
	logfile(LOG_ERROR, err, index);
	return(-1);
      }
    }
  }

  /* clean out lists and such */
  if(get_int_elt(object, CONTENTS, &list) == -1) {
    logfile(LOG_ERROR, err, object);
    return(-1);
  }

  while(list != -1) {
    if(get_int_elt(list, NEXT, &next) == -1) {
      logfile(LOG_ERROR, err, list);
      return(-1);
    }
    notify_player(list, cause, list, "You feel a wrenching sensation...", 0);
    send_home(list, cause, object);

    list = next;
  }

  if(get_int_elt(object, EXITS, &list) == -1) {
    logfile(LOG_ERROR, err, object);
    return(-1);
  }

  while(list != -1) {
    if(get_int_elt(list, NEXT, &next) == -1) {
      logfile(LOG_ERROR, err, list);
      return(-1);
    }
    (*total) += recycle_obj(player, cause, list);
    list = next;
  }

  if(isROOM(object)) {
    if(get_int_elt(object, ROOMS, &list) == -1) {
      logfile(LOG_ERROR, err, object);
      return(-1);
    }

    while(list != -1) {
      if(get_int_elt(list, NEXT, &next) == -1) {
	logfile(LOG_ERROR, err, list);
	return(-1);
      }
      list_add(list, mudconf.root_location, ROOMS);
      if(set_int_elt(list, LOC, mudconf.root_location) == -1) {
	logfile(LOG_ERROR, err, list);
	return(-1);
      }
      list = next;
    }
  }
  return(0);
}
