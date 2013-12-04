/* attrutils.c */

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
#endif                  /* HAVE_STRING_H */
#include <ctype.h>

#include "conf.h"
#include "teeny.h"
#include "attrs.h"
#include "externs.h"

/* mud-level attribute utilities. */

int attr_copy(source, dest, match)
    int source, dest;
    char *match;
{
  struct attr *curr;
  struct asearch srch;

  if (!exists_object(source) || !exists_object(dest) || (source == dest))
    return (-1);

  for (curr = attr_first(source, &srch); curr != (struct attr *)NULL;
       curr = attr_next(source, &srch)) {
    if (curr->type == ATTR_STRING) {
      if(((match != (char *)NULL) && quick_wild_prefix(match, curr->name))
         || (match == (char *)NULL)) {
        if (attr_add(dest, curr->name, (curr->dat).str, curr->flags) != 0)
	  return (-1);
      }
    }
  }
  return (0);
}

char *attr_match(obj, str)
    int obj;
    char *str;
{
  struct attr *curr;
  struct asearch srch;
  char *match = (char *) NULL;

  if (!exists_object(obj))
    return ((char *) NULL);

  for (curr = attr_first(obj, &srch); curr != (struct attr *)NULL;
       curr = attr_next(obj, &srch)) {
    if (quick_wild_prefix(str, curr->name)) {
      if ((match == (char *) NULL) || (strlen(curr->name) > strlen(match)))
	match = curr->name;
    }
  }
  return (match);
}

static int attr_wildparse _ANSI_ARGS_((int, int, bool, char, char *,
				       char **, char **, int *, int,
				       int *, char *[]));

/* This code needs to be refined a bit more. */
static int attr_wildparse(object, cause, multi, token, string,
			  aname, adata, aflags, limit, argc, argv)
    int object, cause;
    bool multi;
    char token, *string, **aname, **adata;
    int *aflags, limit, *argc;
    char *argv[];
{
  char **eargv;
  register int idx, i;
  int count = 0, depth = 0;
  struct asearch srch;
  register struct attr *curr;
  char buff[MAXATTRNAME + 1];

  if(!exists_object(object))
    return(0);

  do {
    for(curr = attr_first(object, &srch); curr != (struct attr *)NULL;
	curr = attr_next(object, &srch)) {
      if((curr->type == ATTR_STRING) && (((curr->dat).str)[0] == token)) {
	/* Find the colon. */
	for(i = 1; (((curr->dat).str)[i] != ':') && (i < sizeof(buff)); i++)
	  buff[i - 1] = ((curr->dat).str)[i];
	buff[i - 1] = '\0';

	/* If it matches, do it. */
	if(wild(buff, string, limit, argc, argv)) {
	  /* Set these up for later. */
	  *aname = curr->name;
	  *adata = (curr->dat).str;
	  *aflags = curr->flags;

          /* Parse and shove in the queue. */
    	  while((*adata)[0] && ((*adata)[0] != ':'))
      	    (*adata)++;
    	  (*adata)++;
	  while((*adata)[0] && isspace((*adata)[0]))
	    (*adata)++;

    	  /* dup off the arguments */
    	  if(*argc > 0) {
      	    eargv = (char **)ty_malloc(sizeof(char *) * (*argc),
				       "attr_wildparse");
      	    for(idx = 0; idx < *argc; idx++)
              eargv[idx] = argv[idx];
    	  } else
      	    eargv = (char **)NULL;
    	  queue_add(object, cause, 1, -1, *adata, *argc, eargv);

    	  count++;
	  if(!multi)	/* Return now. */
	    return(count);
	}
      }
    }

    if(get_int_elt(object, PARENT, &object) == -1) {
      logfile(LOG_ERROR, "attr_wildparse: bas parent ref on object #%d\n",
	      object);
      return(count);
    }
    depth++;
  } while((object != -1) && (depth <= mudconf.parent_depth));

  return(count);
}

/* parse out listening attributes. */
int attr_listen(player, cause, token, str)
    int player, cause;
    char token;
    char *str;
{
  char *aname, *adata;
  int aflags, argc;
  char *argv[MAXARGS];

  if((player == cause) || islocked(cause, cause, player, ULOCK))
    return(0);

  if(attr_wildparse(player, cause, 1, token, str, &aname, &adata, &aflags,
                    MAXARGS, &argc, argv))
    return(1);
  return(0);
}

/*
 * Parse out attribute defined commands.
 *
 * Unlike MUSH, we do this in almost the order as we do exits.
 * (I like it better this way.)
 */
int attr_command(player, cause, token, cmd)
    int player, cause;
    char token;
    char *cmd;
{
  char *aname, *adata;
  int loc, list, aflags, rloc;
  int argc;
  int depth = 0;
  char *argv[MAXARGS];

  /* Check player first. */
  if(!isHALT(player) && !isNOCHECK(player)
     && !islocked(cause, cause, player, ULOCK)) {
    if(attr_wildparse(player, cause, 0, token, cmd, &aname, &adata, &aflags,
		      MAXARGS, &argc, argv))
      return(1);
  }

  /* Next, check player's inventory. */
  if(get_int_elt(player, CONTENTS, &list) == -1) {
    logfile(LOG_ERROR, "attr_command: bad contents list on object #%d\n",
	    player);
    return(0);
  }
  while(list != -1) {
    if((list != player) && !isHALT(list) && !isNOCHECK(list)
       && !islocked(cause, cause, list, ULOCK)) {
      if(attr_wildparse(list, cause, 0, token, cmd, &aname, &adata, &aflags,
			MAXARGS, &argc, argv))
	return(1);
    }

    if(get_int_elt(list, NEXT, &list) == -1) {
      logfile(LOG_ERROR, "attr_command: bad next reference on object #%d\n",
	      list);
      return(0);
    }
  }

  /* Next, check location, *if* it's a room, then it's contents. */
  if((get_int_elt(player, LOC, &loc) == -1) || !exists_object(loc)) {
    logfile(LOG_ERROR, "attr_command: bad location on object #%d\n", player);
    return(0);
  }

  /* (Abort both checks if they're inside themselves.) */
  if(loc != player) {
    if(isROOM(loc) && !isHALT(loc) && !isNOCHECK(loc)
       && !islocked(cause, cause, player, ULOCK)) {
      if(attr_wildparse(loc, cause, 0, token, cmd, &aname, &adata, &aflags,
			MAXARGS, &argc, argv))
	return(1);
    }

    /* Now, locations contents. */
    if(get_int_elt(loc, CONTENTS, &list) == -1) {
      logfile(LOG_ERROR, "attr_command: bad contents on object #%d\n", loc);
      return(0);
    }
    while(list != -1) {
      if((list != player) && !isHALT(list) && !isNOCHECK(list)
         && !islocked(cause, cause, list, ULOCK)) {
        if(attr_wildparse(list, cause, 0, token, cmd, &aname, &adata, &aflags,
    		          MAXARGS, &argc, argv))
          return(1);
      }

      if(get_int_elt(list, NEXT, &list) == -1) {
        logfile(LOG_ERROR, "attr_command: bad next reference on object #%d\n",
	        list);
        return(0);
      }
    }
  }

  /*
   * If they're not in a room, find their real (room) location, and
   * then search up the room tree, just like for exits.  Only include
   * contents, as well.
   *
   * The is extremely inefficient, and just as insecure.  Sigh.
   */
  rloc = loc;
  while(!isROOM(rloc) && (depth <= mudconf.room_depth)) {
    if((get_int_elt(rloc, LOC, &rloc) == -1) || !exists_object(rloc)) {
      logfile(LOG_ERROR, "attr_command: bad location for object #%d\n", rloc);
      return(0);
    }
  }
  if(depth > mudconf.room_depth) {
    logfile(LOG_ERROR, "attr_command: exceeded room_depth in location #%d\n",
	    loc);
    return(0);
  }
  depth = 0;	/* Reset counter. */

  do {
    /* Current location was already checked, if it was a room. */
    if(rloc != loc) {
      if(get_int_elt(rloc, LOC, &rloc) == -1) {
        logfile(LOG_ERROR, "attr_command: bad location ref on object #%d\n",
	        rloc);
        return(0);
      }
    } else
      loc = -1;		/* Loop prevention. */

    /* Check room locations. Yow. */
    if(!isHALT(rloc) && !isNOCHECK(rloc)
       && !islocked(cause, cause, rloc, ULOCK)) {
      if(attr_wildparse(rloc, cause, 0, token, cmd, &aname, &adata, &aflags,
			MAXARGS, &argc, argv))
        return(1);
    }

    /* Check current room's contents. */
    if(get_int_elt(rloc, CONTENTS, &list) == -1) {
      logfile(LOG_ERROR, "attr_command: bad contents list on object #%d\n",
	      rloc);
      return(0);
    }
    while(list != -1) {
      if((list != player) && !isHALT(list) && !isNOCHECK(list)
         && !islocked(cause, cause, list, ULOCK)) {
        if(attr_wildparse(list, cause, 0, token, cmd, &aname, &adata, &aflags,
      			  MAXARGS, &argc, argv))
	  return(1);
      }
      if(get_int_elt(list, NEXT, &list) == -1) {
        logfile(LOG_ERROR, "attr_command: bad next reference on object #%d\n",
	        list);
        return(0);
      }
    }

    depth++;
  } while((depth <= mudconf.room_depth)
	  && (rloc != mudconf.root_location));
  
  /* Done! */
  return(0);
}
