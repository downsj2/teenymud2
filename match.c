/* match.c */

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
#include "teeny.h"
#include "externs.h"

/* new TeenyMUD matching library. */

static int match_token _ANSI_ARGS_((char *, char *));
static int match_exname _ANSI_ARGS_((char *, char *));
static int choose_match _ANSI_ARGS_((int, int, int, int, int));

/* match list struct. */
struct mat_list {
  int obj;

  struct mat_list *next;
};

static struct mat_list *mlist = (struct mat_list *)NULL;
static struct mat_list *mcurr = (struct mat_list *)NULL;

#define MATVAL_EXACT	8192

/* match for tokens within a string. */
/* returns the 'value' of the match. */
static int match_token(substr, str)
    register char *substr, *str;
{
  register int mat_len;

  if(*substr) {
    /* special exact match check. */
    if(strcasecmp(substr, str) == 0)
      return(MATVAL_EXACT);

    while(*str) {
      if((mat_len = stringprefix(str, substr)))
        return(mat_len);
      while(*str && !isspace(*str))
        str++;
      while(*str && isspace(*str))
        str++;
    }
  }
  return(0);
}

/* match for tokens within a string, exit wise. */
/* returns the 'value' of the match. */
static int match_exname(substr, str)
    char *substr;
    register char *str;
{
  register int mat_len;
  register char *ptr;

  while(*str) {
    ptr = substr;
    mat_len = 0;

    /* trim leading spaces first. */
    while(*str && isspace(*str))
      str++;
    /* if we run out here, we loose. */
    if(*str == '\0')
      break;

    while(*ptr && (to_lower(*str) == to_lower(*ptr)) && (*str != ';')) {
      str++;
      ptr++;
      mat_len++;
    }
    /* did we match? */
    if(*ptr == '\0') {
      while(*str && isspace(*str))
        str++;
      if((*str == '\0') || (*str == ';'))
        return(mat_len);
    }
    /* nope, skip to next word. */
    while(*str && (*str != ';'))
      str++;
    while(*str && (*str == ';'))
      str++;
  }
  return(0);
}

/* choose between two equal matches. */
/* returns the better match, or picks at random. */
static int choose_match(player, cause, match1, match2, flags)
    int player, cause, match1, match2, flags;
{
  int lock1, lock2;
  int type1, type2;

  /* make sure they're both valid. */
  if(!exists_object(match1)) {
    return(match2);
  } else if(!exists_object(match2)) {
    return(match1);
  }

  type1 = Typeof(match1);
  type2 = Typeof(match2);
  /* check types. */
  if(!(flags & MAT_ANYTHING)) {
    if(Typeof(match1) != Typeof(match2)){
      /* see if there's a prefered type. */
      if(flags & MAT_THINGS) {
        if((type1 == TYP_THING) && (type2 != TYP_THING)) {
	  return(match1);
	} else if((type2 == TYP_THING) && (type1 != TYP_THING)) {
	  return(match2);
	}
      } else if(flags & MAT_PLAYERS) {
        if((type1 == TYP_PLAYER) && (type2 != TYP_PLAYER)) {
	  return(match1);
	} else if((type2 == TYP_PLAYER) && (type1 != TYP_PLAYER)) {
	  return(match2);
	}
      } else if(flags & MAT_EXITS) {
        if((type1 == TYP_EXIT) && (type2 != TYP_EXIT)) {
	  return(match1);
	} else if((type2 == TYP_EXIT) && (type1 != TYP_EXIT)) {
	  return(match2);
	}
      }
    }
  }

  /* check locks. */
  if(flags & MAT_LOCKS) {
    lock1 = islocked(player, cause, match1, LOCK);
    lock2 = islocked(player, cause, match2, LOCK);

    if(!lock1 && lock2) {
      return(match1);
    } else if(!lock2 && lock1) {
      return(match2);
    }
  }

  /* if all else fails... */
  return((random() % 2) ? match1 : match2);
}

static int best_match, best_value;

/* match within a contents list. */
int match_contents(player, cause, str, list, flags)
    int player, cause;
    char *str;
    int list, flags;
{
  int curr_value;
  char *name;

  best_match = -1;
  best_value = 0;

  while(list != -1) {
    if(get_str_elt(list, NAME, &name) != -1) {
      if((curr_value = match_token(str, name))) {
        /* decide what to do with it */
	if(curr_value > best_value) {
	  best_match = list;
	  best_value = curr_value;
	} else if(curr_value == best_value) {
	  best_match = choose_match(player, cause, best_match, list, flags);
	}
      }
    } else
      logfile(LOG_ERROR, "match_contents: bad name on object #%d\n", list);
    if(get_int_elt(list, NEXT, &list) == -1) {
      logfile(LOG_ERROR, "match_contents: bad next element on object #%d\n",
	      list);
      break;
    }
  }
  return(best_match);
}

/* regular expression contents matcher. */
void match_regexp(player, cause, str, list, flags)
    int player, cause;
    char *str;
    int list, flags;
{
  char *name;
  struct mat_list *nmat;

  while(list != -1) {
    if(get_str_elt(list, NAME, &name) != -1) {
      if(quick_wild(str, name)) {
	nmat = (struct mat_list *)ty_malloc(sizeof(struct mat_list),
					    "match_regexp.nmat");
	nmat->obj = list;
	nmat->next = mlist;
      }
    }
  }
}

/* match within an exits list. */
int match_exits(player, cause, str, list, flags)
    int player, cause;
    char *str;
    int list, flags;
{
  int curr_value;
  char *name;

  best_match = -1;
  best_value = 0;

  while(list != -1) {
    if(get_str_elt(list, NAME, &name) != -1) {
      if(((flags & MAT_EXTERNAL) && isEXTERNAL(list)) ||
      	 ((flags & MAT_INTERNAL) && !isEXTERNAL(list))) {
        if((curr_value = match_exname(str, name))) {
          /* decide what to do with it */
          if(curr_value > best_value) {
            best_match = list;
            best_value = curr_value;
          } else if(curr_value == best_value) {
            best_match = choose_match(player, cause, best_match, list, flags);
          }
        }
      }
    } else
      logfile(LOG_ERROR, "match_exits: bad name on object #%d\n", list);
    if(get_int_elt(list, NEXT, &list) == -1) {
      logfile(LOG_ERROR, "match_exits: bad next element on object #%d\n", list);
      break;
    }
  }
  return(best_match);
}

/* regexp match utilities. */
int match_first()
{
  mcurr = mlist;

  if(mcurr == (struct mat_list *)NULL)
    return(-1);
  return(mcurr->obj);
}

int match_next()
{
  if((mcurr == (struct mat_list *)NULL)
     || (mcurr->next == (struct mat_list *)NULL))
    return(-1);

  mcurr = mcurr->next;
  return(mcurr->obj);
}

void match_free()
{
  struct mat_list *mnext;

  for(mcurr = mlist; mcurr != (struct mat_list *)NULL; mcurr = mnext) {
    mnext = mcurr->next;

    ty_free((VOID *)mcurr);
  }
  mlist = (struct mat_list *)NULL;
}

/* wrapper for all of the above. */
int match_here(player, cause, where, str, flags)
    int player, cause, where;
    char *str;
    int flags;
{
  int list, location;
  int match = -1;
  char *ptr;

  /* check 'me' and 'here' first. */
  if((get_int_elt(player, LOC, &location) != -1) && exists_object(location)){
    if((where == location) && (strcasecmp(str, "me") == 0)) {
      return(player);
    } else if(strcasecmp(str, "here") == 0) {
      return(location);
    }
  } else {
    logfile(LOG_ERROR, "match_here: bad location on object #%d\n", player);
    return(-1);
  }

  /* check absolute number next. */
  if(str[0] == '#') {
    match = (int)strtol(&str[1], &ptr, 10);
    if ((ptr != &str[1]) && exists_object(match)) {
      if(get_int_elt(match, LOC, &location) == -1) {
        logfile(LOG_ERROR, "match_here: bad location on object #%d\n", match);
	return(-1);
      }
      if(location == where) {
        switch(Typeof(match)) {
	case TYP_THING:
	  if(flags & MAT_THINGS)
	    return(match);
	case TYP_PLAYER:
	  if(flags & MAT_PLAYERS)
	    return(match);
	case TYP_EXIT:
	  if(flags & MAT_EXITS)
	    return(match);
	}
      }
    }
  }

  if(flags & MAT_EXITS) {
    if(get_int_elt(where, EXITS, &list) != -1) {
      if((match = match_exits(player, cause, str, list, flags)) != -1)
        return(match);
    } else {
      logfile(LOG_ERROR, "match_here: bad exits list on object #%d\n", where);
      return(-1);
    }
  }
  if((flags & MAT_THINGS) || (flags & MAT_PLAYERS)) {
    if(get_int_elt(where, CONTENTS, &list) != -1) {
      if((match = match_contents(player, cause, str, list, flags)) != -1)
        return(match);
    } else {
      logfile(LOG_ERROR, "match_here: bad contents list on object #%d\n",
	      where);
      return(-1);
    }
  }
  return(-1);
}

/* this is what we have to do to match an exit 'command'. */
int match_exit_command(player, cause, cmd)
    int player, cause;
    char *cmd;
{
  int match, here, list, contents, parent;
  int last_match, last_value;

  /* check for EXTERNAL exits on player. */
  if(get_int_elt(player, EXITS, &list) != -1){
    match = match_exits(player, cause, cmd, list, MAT_EXTERNAL|MAT_LOCKS);
    if(match != -1)
      return(match);
  } else {
    logfile(LOG_ERROR, "match_exit_command: bad exits list on object #%d\n",
	    player);
    return(-1);
  }

  /* check for EXTERNAL exits on inventory. */
  if(get_int_elt(player, CONTENTS, &contents) != -1) {
    last_match = -1;
    last_value = 0;

    while(contents != -1){
      if(!isPLAYER(contents) && (contents != player)){
        if(get_int_elt(contents, EXITS, &list) == -1) {
	  logfile(LOG_ERROR,
		  "match_exit_command: bad exits list on object #%d\n",
	  	  contents);
	  return(-1);
	}
        match = match_exits(player, cause, cmd, list, 
			    MAT_EXTERNAL|MAT_LOCKS|MAT_EXITS);
	if(match != -1) {
	  if(best_value > last_value) {
	    last_value = best_value;
	    last_match = match;
	  } else if(best_value == last_value) {
	    last_match = choose_match(player, cause, last_match, match,
	    			      MAT_EXTERNAL|MAT_LOCKS|MAT_EXITS);
	  }
	}
      }
      if(get_int_elt(contents, NEXT, &contents) == -1) {
        logfile(LOG_ERROR,
		"match_exit_command: bad next element on object #%d\n",
	    	contents);
	return(-1);
      }
    }
    if(last_match != -1)
      return(last_match);
  } else {
    logfile(LOG_ERROR, "match_exit_command: bad contents list on object #%d\n",
	    player);
    return(-1);
  }

  /* check for exits on location. */
  if((get_int_elt(player, LOC, &here) != -1) && exists_object(here)){
    if(get_int_elt(here, EXITS, &list) == -1) {
      logfile(LOG_ERROR, "match_exit_command: bad exits list on object #%d\n",
	      here);
      return(-1);
    }
    match = match_exits(player, cause, cmd, list,
    			isROOM(here) ? MAT_EXTERNAL|MAT_INTERNAL|MAT_LOCKS :
			MAT_INTERNAL|MAT_LOCKS);
    if(match != -1)
      return(match);
  } else {
    logfile(LOG_ERROR, "match_exit_command: bad location on object #%d\n",
	    player);
    return(-1);
  }

  /* check for EXTERNAL exits on contents. */
  if(get_int_elt(here, CONTENTS, &contents) == -1) {
    logfile(LOG_ERROR, "match_exit_command: bad contents list on object #%d\n",
	    player);
    return(-1);
  }
  last_match = -1;
  last_value = 0;
  while(contents != -1) {
    if(!isPLAYER(contents) && (contents != player)) {
      if(get_int_elt(contents, EXITS, &list) == -1) {
        logfile(LOG_ERROR, "match_exit_command: bad exits list on object #%d\n",
		contents);
	return(-1);
      }
      match = match_exits(player, cause, cmd, list,
                	  MAT_EXTERNAL|MAT_LOCKS|MAT_EXITS);
      if(match != -1) {
        if(best_value > last_value) {
          last_value = best_value;
          last_match = match;
        } else if(best_value == last_value) {
          last_match = choose_match(player, cause, last_match, match,
                                    MAT_EXTERNAL|MAT_LOCKS|MAT_EXITS);
        }
      }
    }
    if(get_int_elt(contents, NEXT, &contents) == -1) {
      logfile(LOG_ERROR, "match_exit_command: bad next element on object #%d\n",
      	      contents);
      return(-1);
    }
  }
  if(last_match != -1)
    return(last_match);

  /* check room locations. */
  if(isROOM(here)) {
    register int depth = 0;

    parent = here;
    do {
      if (get_int_elt(parent, LOC, &parent) == -1) {
        logfile(LOG_ERROR, "match_exit_command: bad parent ref on #%d\n",
		parent);
        return (-1);
      }
      if (get_int_elt(parent, EXITS, &list) == -1) {
        logfile(LOG_ERROR, "match_exit_command: bad exit ref on #%d\n", parent);
        return (-1);
      }
      match = match_exits(player, cause, cmd, list,
			  MAT_INTERNAL|MAT_LOCKS);
      if(match != -1)
	return(match);
      depth++;
    } while((depth <= mudconf.room_depth)
    	    && (parent != mudconf.root_location));
  } else {
    if(get_int_elt(mudconf.root_location, EXITS, &list) == -1) {
      logfile(LOG_ERROR, "match_exit_command: bad exits list on ROOT!\n");
      return(-1);
    }
    match = match_exits(player, cause, cmd, list,
			MAT_INTERNAL|MAT_LOCKS);
    if(match != -1)
      return(match);
  }
  return(-1);
}

static void resolve_mesg _ANSI_ARGS_((int, int, int, int));

/* display the appropiate message. */
static void resolve_mesg(player, cause, match, type)
    int player, cause, match, type;
{
  switch(type){
  case TYP_PLAYER:
    switch(match){
    case -1:
      notify_player(player, cause, player, "I don't see that player here.", 0);
      break;
    case -2:
      notify_player(player, cause, player, 
		    "I can't tell which player you mean.", 0);
      break;
    }
    break;
  case TYP_EXIT:
    switch(match){
    case -1:
      notify_player(player, cause, player, "I don't see that exit here.", 0);
      break;
    case -2:
      notify_player(player, cause, player,
		    "I can't tell which exit you mean.", 0);
      break;
    }
    break;
  case TYP_ROOM:
    switch(match){
    case -1:
      notify_player(player, cause, player, "I don't see that room here.", 0);
      break;
    case -2:
      notify_player(player, cause, player,
		    "I can't tell which room you mean.", 0);
      break;
    }
    break;
  default:
    switch(match){
    case -1:
      notify_player(player, cause, player, "I don't see that here.", 0);
      break;
    case -2:
      notify_player(player, cause, player, 
		    "I can't tell which one you mean.", 0);
      break;
    }
    break;
  }
}

int resolve_object(player, cause, name, flags)
    int player, cause;
    char *name;
    int flags;
{
  int location, list;
  register int matched;
  register int curr_match = -1;
  register int curr_value = 0;
  char *ptr;

  if (!name || !*name) {
    if (flags & RSLV_NOISY)
      notify_player(player, cause, player, "I don't see that here.", 0);
    return (-1);		/* no match */
  }
  if ((*name == '#') && isdigit(name[1])) {
    name++;
    matched = (int)strtol(name, &ptr, 10);
    if(ptr == name)
      return(-1);

    if (exists_object(matched) && ((!(flags & RSLV_EXITS)
			 && !isEXIT(matched)) || (flags & RSLV_EXITS))) {
      return (matched);
    } else {
      if (flags & RSLV_NOISY)
	notify_player(player, cause, player, "I don't see that here.", 0);
      return (-1);
    }
  }
  /* Check these first.  Don't want to confuse these. */
  if (strcasecmp("me", name) == 0)
    return (player);
  if ((get_int_elt(player, LOC, &location) == -1)
      || !exists_object(location)) {
    logfile(LOG_ERROR, "resolve_object: couldn't get location of player #%d\n",
	    player);
    if (flags & RSLV_NOISY)
      notify_player(player, cause, player, "You have no location!", 0);
    return (-1);
  }
  if (strcasecmp("here", name) == 0)
    return (location);
  if ((flags & RSLV_HOME) && (strcasecmp("home", name) == 0))
    return (-3);

  /* match player's inventory first. objects now take precedence over exits. */
  if(get_int_elt(player, CONTENTS, &list) == -1) {
    logfile(LOG_ERROR, "resolve_object: bad contents list on object #%d\n",
	    player);
    if(flags & RSLV_NOISY)
      resolve_mesg(player, cause, curr_match, -1);
    return(curr_match);
  }
  matched = match_contents(player, cause, name, list, MAT_THINGS|MAT_PLAYERS);
  if(best_value > curr_value) {
    curr_match = matched;
    curr_value = best_value;
  }
  if(get_int_elt(player, EXITS, &list) == -1) {
    logfile(LOG_ERROR, "resolve_object: bad exits list on object #%d\n",
	    player);
    if(flags & RSLV_NOISY)
      resolve_mesg(player, cause, curr_match, -1);
    return(curr_match);
  }
  matched = match_exits(player, cause, name, list,
  			MAT_EXITS|MAT_INTERNAL|MAT_EXTERNAL);
  if(best_value > curr_value) {
    curr_match = matched;
    curr_value = best_value;
  }

  /* match location things. */
  if(get_int_elt(location, CONTENTS, &list) == -1) {
    logfile(LOG_ERROR, "resolve_object: bad contents list on object #%d\n",
	    location);
    if(flags & RSLV_NOISY)
      resolve_mesg(player, cause, curr_match, -1);
    return(curr_match);
  }
  matched = match_contents(player, cause, name, list, MAT_THINGS);
  if(best_value > curr_value) {
    curr_match = matched;
    curr_value = best_value;
  }

  /* match location players. */
  matched = match_contents(player, cause, name, list, MAT_PLAYERS);
  if(best_value > curr_value) {
    curr_match = matched;
    curr_value = best_value;
  }

  /* match location exits. */
  if(flags & RSLV_EXITS) {
    if(get_int_elt(location, EXITS, &list) == -1) {
      logfile(LOG_ERROR, "resolve_object: bad exits list on object #%d\n",
	      location);
      if(flags & RSLV_NOISY)
        resolve_mesg(player, cause, curr_match, -1);
      return(curr_match);
    }
    matched = match_exits(player, cause, name, list,
    			  MAT_EXITS|MAT_INTERNAL|MAT_EXTERNAL);
    if(best_value > curr_value){
      curr_match = matched;
      curr_value = best_value;
    }
  }

  if (curr_match == -1) {
    if (name[0] == '*') {
      if ((flags & RSLV_SPLATOK) || isWIZARD(player)) {
	name++;
	if (exists_object((matched = match_active_player(name)))) {
	  curr_match = matched;
	} else {
	  curr_match = match_player(name);
	}
      } else
	curr_match = -1;
    } else
      curr_match = -1;
  }

  if (flags & RSLV_NOISY)
    resolve_mesg(player, cause, curr_match, -1);
  return (curr_match);
}

int resolve_player(player, cause, name, flags)
    int player, cause;
    char *name;
    int flags;
{
  int loc, list;
  char *ptr;
  register int matched;
  register curr_match = -1;
  register curr_value = 0;

  if (!name || !*name) {
    if (flags & RSLV_NOISY)
      notify_player(player, cause, player, "I don't see that here.", 0);
    return (-1);		/* no match */
  }
  if ((get_int_elt(player, LOC, &loc) == -1) || !exists_object(loc)) {
    logfile(LOG_ERROR, "resolve_player: couldn't get location of player #%d\n",
	    player);
    if (flags & RSLV_NOISY)
      notify_player(player, cause, player, "You have no location!", 0);
    return (-1);
  }

  if(get_int_elt(player, CONTENTS, &list) == -1) {
    logfile(LOG_ERROR, "resolve_player: bad contents list on object #%d\n",
	    player);
    if(flags & RSLV_NOISY)
      resolve_mesg(player, cause, curr_match, TYP_PLAYER);
    return(curr_match);
  }
  matched = match_contents(player, cause, name, list, MAT_PLAYERS);
  if(best_value > curr_value){
    curr_match = matched;
    curr_value = best_value;
  }

  if(get_int_elt(loc, CONTENTS, &list) == -1) {
    logfile(LOG_ERROR, "resolve_player: bad contents list on object #%d\n",
	    loc);
    if(flags & RSLV_NOISY)
      resolve_mesg(player, cause, curr_match, TYP_PLAYER);
    return(curr_match);
  }
  matched = match_contents(player, cause, name, list, MAT_PLAYERS);
  if(best_value > curr_value){
    curr_match = matched;
    curr_value = best_value;
  }

  if ((curr_match == -1) || isPLAYER(curr_match)) {
    if (!strcasecmp(name, "me")) {
      curr_match = player; 
    } else if ((flags & RSLV_SPLATOK) || isWIZARD(player)) {
      if (name[0] == '*')
        name++;
      if (name[0] == '#') {
        name++;

        matched = (int)strtol(name, &ptr, 10);
        if ((ptr != name) && exists_object(matched) && isPLAYER(matched)) {
	  curr_match = matched;
        } else {
	  curr_match = -1;
        }
      } else {
        matched = match_active_player(name);
        if (exists_object(matched)) {
	  curr_match = matched;
        } else {
	  curr_match = match_player(name);
        }
      }
    }
  }

  if (flags & RSLV_NOISY)
    resolve_mesg(player, cause, curr_match, TYP_PLAYER);
  return (curr_match);
}

int resolve_exit(player, cause, name, flags)
    int player, cause;
    char *name;
    int flags;
{
  int location, list;
  char *ptr;
  register int matched;
  register int curr_match = -1;
  register int curr_value = 0;

  if (!name || !*name) {
    if (flags & RSLV_NOISY)
      notify_player(player, cause, player, "I don't see that here.", 0);
    return (-1);		/* no match */
  }
  if ((get_int_elt(player, LOC, &location) == -1)
      || !exists_object(location)) {
    logfile(LOG_ERROR, "resolve_exit: couldn't get location of player #%d\n",
	    player);
    if (flags & RSLV_NOISY)
      notify_player(player, cause, player, "You have no location!", 0);
    return (-1);
  }

  if(get_int_elt(player, EXITS, &list) == -1) {
    logfile(LOG_ERROR, "resolve_exit: bad exits list on object #%d\n", player);
    if(flags & RSLV_NOISY)
      resolve_mesg(player, cause, curr_match, TYP_EXIT);
    return(curr_match);
  }
  matched = match_exits(player, cause, name, list,
  			MAT_EXITS|MAT_INTERNAL|MAT_EXTERNAL);
  if(best_value > curr_value) {
    curr_match = matched;
    curr_value = best_value;
  }

  if(get_int_elt(location, EXITS, &list) == -1) {
    logfile(LOG_ERROR, "resolve_exit: bad exits list on object #%d\n",
	    location);
    if(flags & RSLV_NOISY)
      resolve_mesg(player, cause, curr_match, TYP_EXIT);
    return(curr_match);
  }
  matched = match_exits(player, cause, name, list,
			MAT_EXITS|MAT_INTERNAL|MAT_EXTERNAL);
  if(best_value > curr_value) {
    curr_match = matched;
    curr_value = best_value;
  }

  if (curr_match == -1) {
    if (*name && (*name == '#')) {
      name++;

      curr_match = (int)strtol(name, &ptr, 10);
      if ((ptr == name) || !exists_object(curr_match) || !isEXIT(curr_match))
	curr_match = -1;
    }
  }

  if (flags & RSLV_NOISY)
    resolve_mesg(player, cause, curr_match, TYP_EXIT);
  return (curr_match);
}
