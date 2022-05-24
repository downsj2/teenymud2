/* textout.c */

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

#include <stdio.h>
#include <sys/types.h>
#include <netinet/in.h>
#ifdef HAVE_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif				/* HAVE_STRING_H */
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif				/* HAVE_STDLIB_H */
#include <ctype.h>

#include "conf.h"
#include "teeny.h"
#include "teenydb.h"
#include "textdb.h"
#include "externs.h"

static void putarray _ANSI_ARGS_((FILE *, int *));
static void putbool_sub _ANSI_ARGS_((FILE *, struct boolexp *));
static void putbool _ANSI_ARGS_((FILE *, struct boolexp *));
static void gfilter_init _ANSI_ARGS_((void));
static void gfilter_free _ANSI_ARGS_((void));
static int gfilter _ANSI_ARGS_((int));

static int *gfilt = (int *)NULL;
static int gcount = 0;

static void gfilter_init()
{
  int indx, findx;

  gcount = mudstat.total_objects - mudstat.actual_objects;
  if(gcount > 0) {
    if(gfilt != (int *)NULL)
      ty_free((VOID *)gfilt);
    gfilt = (int *)ty_malloc((gcount * sizeof(int)), "gfilt");

    for(indx = 0, findx = 0; indx < mudstat.total_objects; indx++) {
      if(!_exists_object(indx))
        gfilt[findx++] = indx;
    }
  }
}

static void gfilter_free()
{
  if(gfilt != (int *)NULL) {
    ty_free((VOID *)gfilt);
    gfilt = (int *)NULL;
    gcount = 0;
  }
}

static int gfilter(objnum)
    int objnum;
{
  int indx;

  if((gfilt != (int *)NULL) && (objnum >= 0)) {
    for(indx = 0; indx < gcount; indx++) {
      if(gfilt[indx] > objnum)
        break;
    }
    indx--;

    return(objnum - indx);
  } else
    return(objnum);
}

static void putarray(out, array)
    FILE *out;
    int *array;
{
  int i;

  if(array != (int *)NULL) {
    for(i = 0; i < array[0]; i++)
      fprintf(out, "%d ", gfilter(array[i]));
    fprintf(out, "%d\n", gfilter(array[i]));
  } else
    fputs("-1\n", out);
}

static void putbool_sub(out, lock)
    FILE *out;
    struct boolexp *lock;
{
  switch(lock->type) {
  case BOOLEXP_AND:
    fputc('(', out);
    putbool_sub(out, lock->sub1);
    fputc('&', out);
    putbool_sub(out, lock->sub2);
    fputc(')', out);
    break;
  case BOOLEXP_OR:
    fputc('(', out);
    putbool_sub(out, lock->sub1);
    fputc('|', out);
    putbool_sub(out, lock->sub2);
    fputc(')', out);
    break;
  case BOOLEXP_NOT:
    fputc('(', out);
    fputc('!', out);
    putbool_sub(out, lock->sub1);
    fputc(')', out);
    break;
  case BOOLEXP_CONST:
    fprintf(out, "#%d", gfilter((lock->dat).thing));
    break;
  case BOOLEXP_FLAG:
    fprintf(out, "~%d/%d", ((lock->dat).flags)[0], ((lock->dat).flags)[1]);
    break;
  case BOOLEXP_ATTR:
    fputc('{', out);
    fputs(((lock->dat).atr)[0], out);
    fputc(':', out);
    fputs(((lock->dat).atr)[1], out);
    fputc('}', out);
    break;
  }
}

static void putbool(out, lock)
    FILE *out;
    struct boolexp *lock;
{
  if(lock != (struct boolexp *)NULL)
    putbool_sub(out, lock);
}

extern const char teenymud_version[];

void text_dump(out, write_version, write_flags)
    FILE *out;
    int write_version, write_flags;
{
  int obj, idx;
  struct obj_data *theobj;
  struct attr *attrs;

  if(write_flags & DB_GFILTER)
    gfilter_init();

  fprintf(out, "!\n! TeenyMUD %s textdump.\n! Actual objects: %d\n!\n",
	  teenymud_version, mudstat.actual_objects);
  fprintf(out, "$%d %d %d\n", write_version, (write_flags & ~DB_GFILTER),
  	  ((write_flags & DB_GFILTER) ? mudstat.actual_objects :
	   mudstat.total_objects));

  for(obj = 0; obj < mudstat.total_objects; obj++) {
    if((obj >= 1000) && ((obj % 1000) == 0)) {
      fputc('o', stderr);
      fflush(stderr);
    } else if((obj % 100) == 0) {
      fputc('.', stderr);
      fflush(stderr);
    }

    if(_exists_object(obj)) {
      if((theobj = lookup_obj(obj)) == (struct obj_data *)NULL) {
        fprintf(stderr,
	  "Warning: couldn't load object #%d! Writing it out as garbage.\n",
	  obj);
        fprintf(out, "@%d\n", obj);
      } else {
        fprintf(out, "#%d\n%d/%d\n", gfilter(obj), DSC_FLAG1(main_index[obj]),
		DSC_FLAG2(main_index[obj]));
        fprintf(out, "%d\n%d\n", theobj->quota, gfilter(theobj->loc));

        if(DSC_TYPE(main_index[obj]) != TYP_EXIT) {
          fprintf(out, "%d\n", gfilter(DSC_HOME(main_index[obj])));
        } else {
          putarray(out, DSC_DESTS(main_index[obj]));
	}

        fprintf(out, "%d\n", gfilter(DSC_OWNER(main_index[obj])));
        fprintf(out, "%d\n%d\n", gfilter(theobj->contents),
		gfilter(theobj->exits));
        fprintf(out, "%d\n%d\n", gfilter(theobj->rooms),
		gfilter(DSC_NEXT(main_index[obj])));
#if SIZEOF_TIME_T > SIZEOF_LONG
        fprintf(out, "%lld\n%lld\n", theobj->timestamp, theobj->created);
#elif SIZEOF_TIME_T == SIZEOF_LONG
        fprintf(out, "%ld\n%ld\n", theobj->timestamp, theobj->created);
#else	/* Good luck */
        fprintf(out, "%d\n%d\n", theobj->timestamp, theobj->created);
#endif
        fprintf(out, "%d\n%d\n", theobj->usecnt,
		gfilter(DSC_PARENT(main_index[obj])));
	fprintf(out, "%d\n%d\n", theobj->charges, theobj->semaphores);
	fprintf(out, "%d\n%d\n", theobj->cost, theobj->queue);

        if(theobj->name != (char *)NULL)
          fputs(theobj->name, out);
        fputc('\n', out);

	for (idx = 0; idx < ATTR_WIDTH; idx++) {
          for (attrs = theobj->attributes[idx]; attrs != (struct attr *)NULL;
	       attrs = attrs->next) {
	    if(attrs->name != (char *)NULL)
	      fputs(attrs->name, out);
	    fputc('\n', out);
	    fprintf(out, "%d %d\n", attrs->flags, attrs->type);
	    switch(attrs->type) {
	    case ATTR_STRING:
	      if((attrs->dat).str != (char *)NULL)
	        fputs((attrs->dat).str, out);
	      break;
	    case ATTR_LOCK:
	      putbool(out, (attrs->dat).lock);
	      break;
	    }
	    fputc('\n', out);
          }
	}
        fputs(">\n", out);
      }
    } else if(!(write_flags & DB_GFILTER)) {
      fprintf(out, "@%d\n", obj);
    }
  }
  fputs("**** END OF DUMP ****\n", out);
  fputc('\n', stderr);
  fflush(stderr);

  if(write_flags & DB_GFILTER)
    gfilter_free();
}
