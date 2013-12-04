/* display.c */

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
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif			/* HAVE_STDLIB_H */

#include "conf.h"
#include "teeny.h"
#include "flaglist.h"
#include "attrs.h"
#include "externs.h"

/* routines for formatting object-related data for human viewing. */

char *display_name(player, cause, thing)
    int player, cause, thing;
{
  static char buff[MEDBUFFSIZ];
  char *name, *ptr;
  int flags[FLAGS_LEN], type;
  register FlagList *flist;
  int control, visible;

  /* do a couple of sanity checks. */
  if(thing == -2) {
    strcpy(buff, "*HOME*");
    return(buff);
  } else if(!exists_object(thing)) {
    strcpy(buff, "*NOTHING*");
    return(buff);
  }

  if((get_str_elt(thing, NAME, &name) == -1)
     || (get_flags_elt(thing, FLAGS, flags) == -1)) {
    logfile(LOG_ERROR,
	    "display_name: couldn't get name or flags of object #%d\n", thing);
    strcpy(buff, "*BAD NAME*");
    return(buff);
  }

  for(ptr = buff; name && *name
      && ((ptr - buff) < (MEDBUFFSIZ-64)); *ptr++ = *name++);
  *ptr = '\0';

  type = (flags[0] & TYPE_MASK);
  control = controls(player, cause, thing);
  visible = 0;

  if(!control) {
    for (flist = GenFlags; flist->name; flist++) {
      if ((flags[flist->word] & flist->code)
	  && (flist->perm & PERM_VISIBLE))
        visible++;
    }
    switch (type) {
    case TYP_ROOM:
      for(flist = RoomFlags; flist->name; flist++) {
        if ((flags[flist->word] & flist->code)
	    && (flist->perm & PERM_VISIBLE))
	  visible++;
      }
      break;
    case TYP_PLAYER:
      for(flist = PlayerFlags; flist->name; flist++) {
        if ((flags[flist->word] & flist->code)
	    && (flist->perm & PERM_VISIBLE))
	  visible++;
      }
      break;
    case TYP_EXIT:
      for(flist = ExitFlags; flist->name; flist++) {
        if ((flags[flist->word] & flist->code)
	    && (flist->perm & PERM_VISIBLE))
	  visible++;
      }
      break;
    case TYP_THING:
      for(flist = ThingFlags; flist->name; flist++) {
        if ((flags[flist->word] & flist->code)
	    && (flist->perm & PERM_VISIBLE))
	  visible++;
      }
      break;
    }
  }

  if(control || visible) {
    sprintf(ptr, "(#%d%s%s)", thing,
    	    (type == TYP_PLAYER) ? "P" : (type == TYP_ROOM) ? "R" :
	    (type == TYP_EXIT) ? "E" : "", display_objflags(flags, control));
  }

  return(buff);
}

static int sort_flags(chr1, chr2)
    register char *chr1, *chr2;
{
  return(*chr1 - *chr2);
}

char *display_objflags(flags, showall)
    int *flags, showall;
{
  static char buff[FLAGS_MAX+1];
  register char *ptr = buff;
  register FlagList *flist;

  for (flist = GenFlags; flist->name; flist++) {
    if (flags[flist->word] & flist->code) {
      if(showall || (flist->perm & PERM_VISIBLE))
        *ptr++ = flist->chr;
    }
  }
  switch(flags[0] & TYPE_MASK) {
  case TYP_ROOM:
    for(flist = RoomFlags; flist->name; flist++) {
      if(flags[flist->word] & flist->code) {
	if(showall || (flist->perm & PERM_VISIBLE))
	  *ptr++ = flist->chr;
      }
    }
    break;
  case TYP_PLAYER:
    for(flist = PlayerFlags; flist->name; flist++) {
      if(flags[flist->word] & flist->code) {
	if(showall || (flist->perm & PERM_VISIBLE))
	  *ptr++ = flist->chr;
      }
    }
    break;
  case TYP_EXIT:
    for(flist = ExitFlags; flist->name; flist++) {
      if(flags[flist->word] & flist->code) {
	if(showall || (flist->perm & PERM_VISIBLE))
	  *ptr++ = flist->chr;
      }
    }
    break;
  case TYP_THING:
    for(flist = ThingFlags; flist->name; flist++) {
      if(flags[flist->word] & flist->code) {
	if(showall || (flist->perm & PERM_VISIBLE))
	  *ptr++ = flist->chr;
      }
    }
    break;
  }
  *ptr = '\0';

  /* make it look pretty */
  qsort(buff, (ptr - buff), sizeof(char), sort_flags);

  return(buff);
}

char *display_attrflags(aflags)
    int aflags;
{
  static char buff[A_FLAGS_MAX+1];
  register char *ptr;
  AFlagList *Aflags;

  for (ptr = buff, Aflags = AFlags; Aflags->name; Aflags++) {
    if (aflags & Aflags->code) 
      *ptr++ = Aflags->chr;
  }
  *ptr = '\0';
  return (buff);
}

/*
 * Format, sort, and display the long list of object flags.
 *
 * XXX: Too much of this is hard coded.
 */

struct sortbuf {
  char *name;	/* Long flag name */
  char chr;	/* Short flag name. */
};

static int sort_withbuf(s1, s2)
    register struct sortbuf *s1, *s2;
{
  return(s1->chr - s2->chr);
}

void display_flags(player, cause, obj)
    int player, cause, obj;
{
  char buffer[MEDBUFFSIZ];
  struct sortbuf bufptr[FLAGS_MAX];
  int flags[FLAGS_LEN], pindex = 0, bindex;
  register FlagList *flist;

  if (get_flags_elt(obj, FLAGS, flags) == -1) {
    notify_player(player, cause, player, "<Spammed flags>", 0);
  } else {
    strcpy(buffer, "Type: ");
    switch (flags[0] & TYPE_MASK) {
    case TYP_ROOM:
      strcat(buffer, "Room");
      break;
    case TYP_EXIT:
      strcat(buffer, "Exit");
      break;
    case TYP_PLAYER:
      strcat(buffer, "Player");
      break;
    case TYP_THING:
      strcat(buffer, "Thing");
      break;
    default:
      strcat(buffer, "*UNKNOWN TYPE*");
    }

    /*
     * We now assign pointers, so that we can sort them ala the short
     * flags display.
     */
    for (flist = GenFlags; flist->name; flist++) {
      if (flags[flist->word] & flist->code) {
	bufptr[pindex].chr = flist->chr;
	bufptr[pindex++].name = flist->name;
      }
    }
    switch(Typeof(obj)) {
    case TYP_ROOM:
      for(flist = RoomFlags; flist->name; flist++) {
	if(flags[flist->word] & flist->code) {
	  bufptr[pindex].chr = flist->chr;
	  bufptr[pindex++].name = flist->name;
	}
      }
      break;
    case TYP_PLAYER:
      for(flist = PlayerFlags; flist->name; flist++) {
	if(flags[flist->word] & flist->code) {
	  bufptr[pindex].chr = flist->chr;
	  bufptr[pindex++].name = flist->name;
	}
      }
      break;
    case TYP_THING:
      for(flist = ThingFlags; flist->name; flist++) {
	if(flags[flist->word] & flist->code) {
	  bufptr[pindex].chr = flist->chr;
	  bufptr[pindex++].name = flist->name;
	}
      }
      break;
    case TYP_EXIT:
      for(flist = ExitFlags; flist->name; flist++) {
	if(flags[flist->word] & flist->code) {
	  bufptr[pindex].chr = flist->chr;
	  bufptr[pindex++].name = flist->name;
	}
      }
      break;
    }

    if(pindex > 0) {		/* Found some flags... */
      strcat(buffer, "  Flags: ");

      /* Sort bufptr. */
      qsort(bufptr, (pindex - 1), sizeof(struct sortbuf), sort_withbuf);

      /* ..And now finish building the message. */
      for(bindex = 0; bindex < pindex; bindex++) {
        /* Space check. */
        if((strlen(buffer) + strlen(bufptr[bindex].name)) >= sizeof(buffer) - 3)
          break;

        strcat(buffer, bufptr[bindex].name);
        strcat(buffer, " ");
      }

      if (buffer[strlen(buffer) - 1] == ' ')
        buffer[strlen(buffer) - 1] = '\0';
    }
    
    /* Tell them about it. */
    notify_player(player, cause, player, buffer, 0);
  }
}

void display_attributes(player, cause, obj, match)
    int player, cause, obj;
    char *match;
{
  struct attr *atptr;
  struct asearch srch;
  char buf[MEDBUFFSIZ*2];

  for (atptr = attr_first(obj, &srch); atptr != (struct attr *)NULL;
       atptr = attr_next(obj, &srch)) {
    if (!can_see_attr(player, cause, obj, atptr->flags))
      continue;
    if ((match != (char *)NULL) && (match[0] != '\0')
        && !quick_wild_prefix(match, atptr->name))
      continue;
    if (atptr->flags != (DEFATTR_FLGS)) {
      sprintf(buf, "%s(%s): ", atptr->name, display_attrflags(atptr->flags));
      switch(atptr->type) {
      case ATTR_STRING:
        strcat(buf, (atptr->dat).str);
	break;
      case ATTR_LOCK:
        strcat(buf, unparse_boolexp(player, cause, (atptr->dat).lock));
	break;
      }
    } else {
      strcpy(buf, atptr->name);
      strcat(buf, ": ");
      switch(atptr->type) {
      case ATTR_STRING:
        strcat(buf, (atptr->dat).str);
	break;
      case ATTR_LOCK:
        strcat(buf, unparse_boolexp(player, cause, (atptr->dat).lock));
	break;
      }
    }
    notify_player(player, cause, player, buf, 0);
  }
}

void display_attributes_parent(player, cause, obj, match)
    int player, cause, obj;
    char *match;
{
  int parent, depth;
  struct attr *atptr;
  struct asearch srch;
  char buf[MEDBUFFSIZ*2];

  depth = 0;
  parent = obj;
  do {
    for (atptr = attr_first(parent, &srch); atptr != (struct attr *)NULL;
         atptr = attr_next(parent, &srch)) {
      if ((atptr->flags & A_PRIVATE) && (parent != obj))
	continue;
      if ((atptr->flags & A_PICKY) && (Typeof(parent) != Typeof(obj)))
        continue;
      if (isPICKY(parent) && (Typeof(parent) != Typeof(obj)))
        continue;
      if (!can_see_attr(player, cause, obj, atptr->flags))
	continue;
      if ((match != (char *)NULL) && (match[0] != '\0')
          && !quick_wild_prefix(match, atptr->name))
	continue;
      if (attr_source(obj, atptr->name) != parent)
	continue;

      if (atptr->flags != (DEFATTR_FLGS)) {
	sprintf(buf, "%s(%s): ", atptr->name, display_attrflags(atptr->flags));
	switch(atptr->type) {
	case ATTR_STRING:
	  strcat(buf, (atptr->dat).str);
	  break;
	case ATTR_LOCK:
	  strcat(buf, unparse_boolexp(player, cause, (atptr->dat).lock));
	  break;
	}
      } else {
        strcpy(buf, atptr->name);
	strcat(buf, ": ");
	switch(atptr->type) {
	case ATTR_STRING:
	  strcat(buf, (atptr->dat).str);
	  break;
	case ATTR_LOCK:
	  strcat(buf, unparse_boolexp(player, cause, (atptr->dat).lock));
	  break;
	}
      }
      notify_player(player, cause, player, buf, 0);
    }

    if (get_int_elt(parent, PARENT, &parent) == -1)
      break;
    depth++;
  } while ((parent != -1) && (depth <= mudconf.parent_depth));
}
