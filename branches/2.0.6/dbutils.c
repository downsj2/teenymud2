/* dbutils.c */

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

#include "conf.h"
#include "teeny.h"
#include "commands.h"
#include "teenydb.h"
#include "externs.h"


/*
 * Database intensive utility routines.
 */

/*
 * Returns true if player controls object.
 */
int controls(player, cause, obj)
    int player, cause, obj;
{
  if (obj == -3)
    return(1);			/* everyone controls HOME */

  if (!_exists_object(player) || !_exists_object(obj))
    return(0);

  if (isWIZARD(player))
    return(1);

  return(DSC_OWNER(main_index[obj]) == DSC_OWNER(main_index[player]));
}

/*
 * Returns true if object is an exit, and player controls it's
 * destination.
 */
int econtrols(player, cause, object)
    int player, cause, object;
{
  int indx;

  if(!_exists_object(player) || !_exists_object(object))
    return(0);

  if((DSC_TYPE(main_index[object]) == TYP_EXIT)
     && (DSC_DESTS(main_index[object]) != (int *)NULL)) {
    for(indx = 1; indx <= (DSC_DESTS(main_index[object])[0]); indx++) {
      if(DSC_OWNER(main_index[DSC_DESTS(main_index[object])[indx]])
	 		==  DSC_OWNER(main_index[player]))
        return(1);
    }
  }

  return(0);
}

/*
 * Returns true is object can inherit Wizard powers.
 */
int inherit_wizard(object)
    int object;
{
  if(!_exists_object(object) || !_exists_object(DSC_OWNER(main_index[object]))
     || (!isINHERIT(object) && !isINHERIT(DSC_OWNER(main_index[object]))))
    return(0);

  return(isWIZARD(DSC_OWNER(main_index[object])));
}

/*
 * Change the ownership of all objects owned by `oldowner' to `newowner'.
 */
int chownall(oldowner, newowner)
    int oldowner, newowner;
{
  int i, count = 0;

  if (!_exists_object(oldowner) || !_exists_object(newowner)) {
    logfile(LOG_ERROR, "chownall: called with bad object as an argument\n");
    return (0);
  }
  for (i = 0; i < mudstat.total_objects; i++) {
    if (main_index[i] == (struct dsc *)NULL)
      continue;
    if (DSC_OWNER(main_index[i]) == oldowner) {
      DSC_OWNER(main_index[i]) = newowner;
      count++;
    }
  }
  return count;
}

/*
 * Checks and returns true if player is over their quota.
 */
int no_quota(player, cause, quiet)
    int player, cause, quiet;
{
  int quota;
  int i, total;

  if (!_exists_object(player) || isWIZARD(DSC_OWNER(main_index[player])))
    return (0);

  for (i = 0, total = 0; i < mudstat.total_objects; i++) {
    if (main_index[i] == (struct dsc *) NULL)
      continue;
    if (DSC_OWNER(main_index[i]) == DSC_OWNER(main_index[player]))
      total++;
  }
  if (get_int_elt(DSC_OWNER(main_index[player]), QUOTA, &quota) == -1) {
    logfile(LOG_ERROR, "no_quota: player #%d has a bad quota ref.", player);
    return (1);
  }
  if (total >= quota) {
    if(!quiet)
      notify_player(player, cause, player,
		    "Sorry, you're over your building limit.", NOT_QUIET);
    return (1);
  }
  return (0);
}

/* shove all the startup attributes into the queue. */
void handle_startup()
{
  int idx;
  int aflags;
  char *attr;

  for(idx = 0; idx < mudstat.total_objects; idx++) {
    if(main_index[idx] == (struct dsc *)NULL)
      continue;
    if(DSC_FLAG2(main_index[idx]) & HAS_STARTUP) {
      if(attr_get(idx, STARTUP, &attr, &aflags) == -1) {
        logfile(LOG_ERROR, "handle_startup: bad attributes on object #%d\n",
		idx);
	continue;
      }
      if(attr != (char *)NULL)
        queue_add(idx, idx, 1, -1, attr, 0, (char **)NULL);
      cache_trim();
    }
  }
}

/* Check to see if they should be deleted. */
void handle_autotoad(player)
    int player;
{
  if(!isALIVE(player) && !isGUEST(player)) {
    int idx;
    char *dstr, buf[BUFFSIZ];
    int aflags, asource;

    if(attr_get_parent(player, DESC, &dstr, &aflags, &asource) == -1) {
      logfile(LOG_ERROR, "handle_autotoad: bad attributes on object #%d\n",
	  player);
      return;
    }
    if(dstr && *dstr && (asource == player))
      return;

    for(idx = 0; idx < mudstat.total_objects; idx++) {
      if(main_index[idx] == (struct dsc *)NULL)
	continue;
      if((DSC_OWNER(main_index[idx]) == player) && (idx != player))
	return;
    }

    /* If we got this far, nuke 'em 'til they glow. */
    if(get_str_elt(player, NAME, &dstr) == -1)
      dstr = "???";
    snprintf(buf, sizeof(buf), "%s self destructs... *BOOM*!", dstr);
    notify_oall(player, mudconf.player_god, player, buf, NOT_NOPUPPET);

    if(recycle_obj(mudconf.player_god, mudconf.player_god, player)) {
      /* log in the commands area to match @toad. */
      logfile(LOG_COMMAND, "AUTOTOAD: Deleted %s(#%d).\n", dstr, player);
    }
  }
}
