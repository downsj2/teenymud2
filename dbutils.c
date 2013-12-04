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
#endif			/* HAVE_STDIB_H */

#include "conf.h"
#include "teeny.h"
#include "commands.h"
#include "teenydb.h"
#include "externs.h"

static char elist[] = "***End of list***";

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
  register int indx;

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

int inherit_wizard(object)
    int object;
{
  if(!_exists_object(object) || !_exists_object(DSC_OWNER(main_index[object]))
     || (!isINHERIT(object) && !isINHERIT(DSC_OWNER(main_index[object]))))
    return(0);

  return(isWIZARD(DSC_OWNER(main_index[object])));
}

VOID do_owned(player, cause, switches, argone, argtwo)
    int player, cause, switches;
    char *argone, *argtwo;
{
  int matchfor, i, type, flags[FLAGS_LEN], not;
  char *p;

  if (!argone || !*argone) {
    notify_player(player, cause, player, "I need a player name.", 0);
    return;
  }
  matchfor = resolve_player(player, cause, argone, RSLV_NOISY);
  if (matchfor == -1)
    return;

  type = -1;
  not = 0;
  bzero((VOID *)flags, sizeof(flags));
  if (argtwo && *argtwo) {
    if(*argtwo == '!') {
      not++;
      argtwo++;
    }

    for (p = argtwo; *p && *p != '/'; p++);
    if (*p == '/') {
      *p++ = '\0';
      parse_flags(p, flags);
      if ((flags[0] == -1) && (flags[1] == -1)) {
	notify_player(player, cause, player, "Illegal flags.", 0);
	return;
      }
    }
    if (*argtwo) {
      parse_type(argtwo, &type);
      if (type == -1) {
	notify_player(player, cause, player, "Illegal type.", 0);
	return;
      }
    }
  }
  for (i = 0; i < mudstat.total_objects; i++) {
    if (main_index[i] == NULL)
      continue;
    if (DSC_OWNER(main_index[i]) != DSC_OWNER(main_index[matchfor]))
      continue;

    /* they own this object */
    if (not) {
      if ((type != -1) && (DSC_TYPE(main_index[i]) == type))
	continue;
      if (((flags[0] != 0) && (DSC_FLAG1(main_index[i]) & flags[0]))
	  || ((flags[1] != 0) && (DSC_FLAG2(main_index[i]) & flags[1])))
	continue;
    } else {
      if ((type != -1) && (DSC_TYPE(main_index[i]) != type))
        continue;
      if (((flags[0] != 0) && (!(DSC_FLAG1(main_index[i]) & flags[0])))
	  || ((flags[1] != 0) && (!(DSC_FLAG2(main_index[i]) & flags[1]))))
        continue;
    }

    notify_player(player, cause, player, display_name(player, cause, i), 0);
    cache_trim();
  }
  notify_player(player, cause, player, elist, 0);
}

VOID do_entrances(player, cause, switches, argone)
    int player, cause, switches;
    char *argone;
{
  register int indx;
  int search_for;
  int done, obj;
  char buf[MEDBUFFSIZ];

  if (!argone || !*argone) {
    if (get_int_elt(player, LOC, &search_for) == -1) {
      notify_bad(player);
      return;
    }
  } else {
    search_for = resolve_object(player, cause, argone, RSLV_NOISY);
    if(search_for == -1)
      return;
    if (DSC_TYPE(main_index[search_for]) == TYP_EXIT) {
      notify_player(player, cause, player,
		    "That won't have anything linked to it.", 0);
      return;
    }
  }
  if (!controls(player, cause, search_for)) {
    notify_player(player, cause, player, "Permission denied.", 0);
    return;
  }

  for (obj = 0; obj < mudstat.total_objects; obj++) {
    if (main_index[obj] == NULL)
      continue;

    if(DSC_TYPE(main_index[obj]) == TYP_EXIT) {
      if (DSC_DESTS(main_index[obj]) != (int *)NULL) {
        for(indx = 1, done = 0;
	    !done && (indx <= (DSC_DESTS(main_index[obj]))[0]); indx++) {
	  if((DSC_DESTS(main_index[obj]))[indx] == search_for) {
	    strcpy(buf, "Exit: ");
	    strcat(buf, display_name(player, cause, obj));
	    notify_player(player, cause, player, buf, 0);

	    done++;
	  }
	}
      }
    } else {
      if(DSC_HOME(main_index[obj]) != search_for)
        continue;

      if(DSC_TYPE(main_index[obj]) == TYP_ROOM) {
        strcpy(buf, "Drop to: ");
      } else {
        strcpy(buf, "Home: ");
      }
      strcat(buf, display_name(player, cause, obj));
      notify_player(player, cause, player, buf, 0);
    }
    cache_trim();
  }
  notify_player(player, cause, player, elist, 0);
}

int chownall(oldowner, newowner)
    register int oldowner, newowner;
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

VOID do_find(player, cause, switches, argone, argtwo)
    int player, cause, switches;
    char *argone, *argtwo;
{
  char *p, *name;
  int type, flags[FLAGS_LEN], not;
  register int i;
  register int cost = mudconf.find_cost;

  type = -1;
  not = 0;
  bzero((VOID *)flags, sizeof(flags));
  if (argtwo && *argtwo) {
    if (*argtwo == '!') {
      not++;
      argtwo++;
    }

    for (p = argtwo; *p && (*p != '/'); p++);
    if (*p == '/') {
      *p++ = '\0';
      parse_flags(p, flags);
      if ((flags[0] == -1) && (flags[1] == -1)) {
	notify_player(player, cause, player, "Illegal flag.", 0);
	return;
      }
    }
    if (*argtwo) {
      parse_type(argtwo, &type);
      if (type == -1) {
	notify_player(player, cause, player, "Illegal type.", 0);
	return;
      }
    }
    cost = mudconf.limfind_cost;
  }

  if (!can_afford(player, cause, cost))
    return;

  for (i = 0; i < mudstat.total_objects; i++) {
    if (main_index[i] == (struct dsc *)NULL)
      continue;

    if (not) {
      if ((type != -1) && (DSC_TYPE(main_index[i]) == type))
	continue;
      if (((flags[0] != 0) && (DSC_FLAG1(main_index[i]) & flags[0]))
	  || ((flags[1] != 0) && (DSC_FLAG2(main_index[i]) & flags[1])))
	continue;
    } else {
      if ((type != -1) && (DSC_TYPE(main_index[i]) != type))
        continue;
      if (((flags[0] != 0) && (!(DSC_FLAG1(main_index[i]) & flags[0])))
	  || ((flags[1] != 0) && (!(DSC_FLAG2(main_index[i]) & flags[1]))))
        continue;
    }

    if ((switches & FIND_OWNED) && (DSC_OWNER(main_index[i]) != player))
      continue;
    if (!controls(player, cause, i))
      continue;
    if ((get_str_elt(i, NAME, &name) == -1) || !name || !name[0]) {
      logfile(LOG_ERROR, "do_find: object #%d has a bad name.\n");
      continue;
    }

    if ((argone == (char *)NULL) || (argone[0] == '\0')
    	|| quick_wild_prefix(argone, name))
      notify_player(player, cause, player, display_name(player, cause, i), 0);
    cache_trim();
  }
  notify_player(player, cause, player, elist, 0);
}

VOID do_trace(player, cause, switches, argone, argtwo)
    int player, cause, switches;
    char *argone, *argtwo;
{
  int obj, max_depth, loc, depth;
  char *ptr;

  if ((get_int_elt(player, LOC, &loc) == -1) || !_exists_object(loc)) {
    notify_player(player, cause, player, "You have no location!", 0);
    return;
  }
  if (!argone || !*argone) {
    obj = loc;
  } else {
    obj = resolve_object(player, cause, argone, RSLV_NOISY);
    if(obj == -1)
      return;
  }
  if (!argtwo || !*argtwo) {
    max_depth = mudconf.max_list;
  } else {
    max_depth = (int)strtol(argtwo, &ptr, 10);
    if ((ptr == argtwo) || (max_depth < 1)){
      notify_player(player, cause, player,
		    "That's not a reasonable depth.", 0);
      return;
    }
  }
  depth = 0;
  do {
    notify_player(player, cause, player, display_name(player, cause, obj), 0);

    depth++;
    cache_trim();
  }
  while ((depth < max_depth) && (obj != mudconf.root_location)
	 && (get_int_elt(obj, LOC, &obj) != -1) && _exists_object(obj));
  notify_player(player, cause, player, elist, 0);
}

VOID do_stats(player, cause, switches, argone)
    int player, cause, switches;
    char *argone;
{
  int matchfor;
  char buf[MEDBUFFSIZ];
  static char stats_frmt[] =
"%d object%s = %d room%s, %d exit%s, %d thing%s, %d player%s (%d garbage).";

  if (!argone || !*argone) {
    matchfor = -2;
  } else {
    matchfor = resolve_player(player, cause, argone, RSLV_NOISY);
    if(matchfor == -1)
      return;
    if ((matchfor != player) && !isWIZARD(player)) {
      notify_player(player, cause, player, "You can't stat other players!", 0);
      return;
    }
    /* matchfor should now contain a valid player number */
  }
  if (!isWIZARD(player) && (matchfor == -2) && !(switches & STATS_FULL)) {
    snprintf(buf, sizeof(buf), "The universe contains %d objects.",
	    mudstat.actual_objects);
    notify_player(player, cause, player, buf, 0);
  } else {
    int totals[TYPE_TOTAL];
    register int i, total = 0;
    float ratio = 0.0;

    if (!can_afford(player, cause, (switches == STATS_FULL) ?
				   mudconf.fullstat_cost :
				   mudconf.stat_cost))
      return;

    bzero((VOID *)totals, sizeof(int) * TYPE_TOTAL);
    for (i = 0; i < mudstat.total_objects; i++) {
      if (main_index[i] == (struct dsc *)NULL)
	continue;
      if ((matchfor == -2) || (DSC_OWNER(main_index[i]) == matchfor)) {
	total++;
	totals[DSC_TYPE(main_index[i])]++;
      }
    }

    if(totals[TYP_ROOM] > 0)
      ratio = (float)totals[TYP_EXIT] / (float)totals[TYP_ROOM];
    snprintf(buf, sizeof(buf), stats_frmt, total, (total == 1) ? "" : "s",
	    totals[TYP_ROOM], (totals[TYP_ROOM] == 1) ? "" : "s",
	    totals[TYP_EXIT], (totals[TYP_EXIT] == 1) ? "" : "s",
	    totals[TYP_THING], (totals[TYP_THING] == 1) ? "" : "s",
	    totals[TYP_PLAYER], (totals[TYP_PLAYER] == 1) ? "" : "s", 
	    (matchfor == -2) ? mudstat.garbage_count : 0);
    notify_player(player, cause, player, buf, 0);
    snprintf(buf, sizeof(buf), "Exit/Room Ratio: %.2f", ratio);
    notify_player(player, cause, player, buf, 0);

    if (isWIZARD(player) && (matchfor == -2)) {
      snprintf(buf, sizeof(buf), "Cache stats: %d bytes in use of %d max. %d cache hits with %d misses. %d database errors.", mudstat.cache_usage, mudconf.cache_size,
	      mudstat.cache_hits, mudstat.cache_misses, mudstat.cache_errors);
      notify_player(player, cause, player, buf, 0);
    }
  }
}

int no_quota(player, cause)
    int player, cause;
{
  int quota;
  register int i, total;

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
    notify_player(player, cause, player,
		  "Sorry, you're over your building limit.", 0);
    return (1);
  }
  return (0);
}

VOID do_children(player, cause, switches, argone)
    int player, cause, switches;
    char *argone;
{
  int obj;
  register int i;

  if (!argone || !*argone) {
    notify_player(player, cause, player, "List the children of what?", 0);
    return;
  }
  obj = resolve_object(player, cause, argone, RSLV_EXITS | RSLV_NOISY);
  if(obj == -1)
    return;
  if (!controls(player, cause, obj) && !isPARENT_OK(obj)) {
    notify_player(player, cause, player, "Permission denied.", 0);
    return;
  }
  for (i = 0; i < mudstat.total_objects; i++) {
    if (main_index[i] == (struct dsc *)NULL)
      continue;
    if (DSC_PARENT(main_index[i]) == obj) {
      notify_player(player, cause, player, display_name(player, cause, i), 0);
      cache_trim();
    }
  }
  notify_player(player, cause, player, elist, 0);
}

VOID do_edit(player, cause, switches, argc, argv)
    int player, cause, switches, argc;
    char *argv[];
{
  int thing;
  int d, len, doneone = 0;
  char *r, *val, *s;
  struct attr *atptr;
  struct asearch srch;
  char dest[MEDBUFFSIZ], buf[MEDBUFFSIZ];
  int (*mfunc) _ANSI_ARGS_((char *, char *));

  if (argc == 0) {
    notify_player(player, cause, player, "Edit what?", 0);
    return;
  }
  if ((argv[1] == NULL) || (argv[1][0] == '\0')) {
    notify_player(player, cause, player, "You must specify an attribute.", 0);
    return;
  }
  if (argc < 3) {
    notify_player(player, cause, player, "Nothing to do.", 0);
    return;
  }

  thing = resolve_object(player, cause, argv[0], RSLV_EXITS | RSLV_NOISY);
  if(thing == -1)
    return;
  val = argv[2];
  if(argv[3] == NULL)
    r = "";
  else
    r = argv[3];

  /*
   * Set the mfunc.  If /all was given, use quick_wild(), otherwise use
   * quick_wild_prefix().
   */
  if (switches & EDIT_ALL)
    mfunc = quick_wild;
  else
    mfunc = quick_wild_prefix;

  for (atptr = attr_first(thing, &srch); atptr != (struct attr *)NULL;
       atptr = attr_next(thing, &srch)) {
    if (atptr->type != ATTR_STRING)
      continue;
    
    if (!(mfunc)(argv[1], atptr->name)
        || ((atptr->dat).str == (char *)NULL)
	|| (((atptr->dat).str)[0] == '\0'))
      continue;

    if (!can_see_attr(player, cause, thing, atptr->flags)
	|| !can_set_attr(player, cause, thing, atptr->flags)) {
      snprintf(buf, sizeof(buf), "%s(#%d): Permission denied.",
	       atptr->name, thing);
      notify_player(player, cause, player, buf, 0);
      continue;
    }

    /*
     * Decide what to do.
     */
    if((val[0] == '$') && (val[1] == '\0')) {	/* Append */
      s = (atptr->dat).str;
      d = 0;
      while((d < sizeof(dest)-2) && *s)
	dest[d++] = *s++;
      if(!(switches & EDIT_NOSPACE))
	dest[d++] = ' ';
      s = r;
      while((d < sizeof(dest)-1) && *s)
	dest[d++] = *s++;
    } else if((val[0] == '^') && (val[1] == '\0')) {	/* Prepend */
      s = r;
      d = 0;
      while((d < sizeof(dest)-2) && *s)
	dest[d++] = *s++;
      if(!(switches & EDIT_NOSPACE))
	dest[d++] = ' ';
      s = (atptr->dat).str;
      while((d < sizeof(dest)-1) && *s)
	dest[d++] = *s++;
    } else {				 /* Replacement. */
      len = strlen(val);
      s = (atptr->dat).str;
      d = 0;
      while((d < sizeof(dest)) && *s) {
        if (strncmp(val, s, len) == 0) {
	  if ((d + strlen(r)) < sizeof(dest)-1) {
	    strcpy(dest + d, r);
	    d += strlen(r);
	    s += len;
	  } else
	    dest[d++] = *s++;
        } else
	  dest[d++] = *s++;
      }
    }
    dest[d] = '\0';

    if (!ok_attr_value(dest)) {
      notify_player(player, cause, player,
		    "You can't set an attribute to that!", 0);
      return;
    }

    /* XXX - this only works because the attribute already exists! */
    if (attr_add(thing, atptr->name, dest, atptr->flags) == -1) {
      notify_bad(player);
      return;
    }
    snprintf(buf, sizeof(buf), "%s set.", atptr->name);
    notify_player(player, cause, player, buf, 0);

    doneone++;

    /* If /all wasn't given, we're done. */
    if(!(switches & EDIT_ALL))
      break;
  }

  if (doneone) {
    stamp(thing, STAMP_MODIFIED);
  } else {
    notify_player(player, cause, player, "No matching attributes.", 0);
  }
}

VOID do_sweep(player, cause, switches, argone)
    int player, cause, switches;
    char *argone;
{
  int here, list, obj, owner;
  char *name, buf[MEDBUFFSIZ];

  if ((get_int_elt(player, LOC, &here) == -1) || !_exists_object(here)) {
    notify_player(player, cause, player, "You have no location!", 0);
    return;
  }
  if (argone && *argone) {
    obj = resolve_object(player, cause, argone, RSLV_NOISY);
    if(obj == -1)
      return;
  } else
    obj = here;
  if ((obj != here) && !controls(player, cause, obj)) {
    notify_player(player, cause, player, "Permission denied.", 0);
    return;
  }
  if (get_int_elt(obj, CONTENTS, &list) != -1) {
    while (list != -1) {
      if((get_str_elt(list, NAME, &name) != -1)
	 && (get_int_elt(list, OWNER, &owner) != -1)) {
        if (isALIVE(list) || (isPUPPET(list) && isALIVE(owner))
	    || isLISTENER(list)) {
	  snprintf(buf, sizeof(buf), "%s is listening.", name);
	  notify_player(player, cause, player, buf, 0);
        }
        if (isAUDIBLE(list)) {
	  snprintf(buf, sizeof(buf), "%s will echo things.", name);
	  notify_player(player, cause, player, buf, 0);
        }
      }
      if (get_int_elt(list, NEXT, &list) == -1)
	break;
    }
  } else {
    notify_bad(player);
    return;
  }
  if (get_int_elt(obj, EXITS, &list) != -1) {
    while (list != -1) {
      if (isAUDIBLE(list) && (get_str_elt(list, NAME, &name) != -1)) {
	snprintf(buf, sizeof(buf), "%s will echo things.", name);
	notify_player(player, cause, player, buf, 0);
      }
      if (get_int_elt(list, NEXT, &list) == -1)
	break;
    }
  }
  notify_player(player, cause, player, elist, 0);
}

/* shove all the startup attributes into the queue. */
void handle_startup()
{
  register int idx;
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
    register int idx;
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
    notify_oall(player, mudconf.player_god, player, buf, 0);

    if(recycle_obj(mudconf.player_god, mudconf.player_god, player))
      /* log in the commands area to match @toad. */
      logfile(LOG_COMMAND, "AUTOTOAD: Deleted %s(#%d).\n", dstr, player);
  }
}
