/* db.c */

#include "config.h"

/*
 *		       This file is part of TeenyMUD II.
 *		 Copyright(C) 1993, 1994, 1995, 2013 by Jason Downs.
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
#include <errno.h>

#include "conf.h"
#include "teeny.h"
#include "teenydb.h"
#include "ptable.h"
#include "externs.h"

/* main database access routines. based on andrew's old code. */

static struct dsc *dsc_alloc _ANSI_ARGS_((void));
static void dsc_free _ANSI_ARGS_((struct dsc *));
static void grow_index _ANSI_ARGS_((void));

/* the actual database. */
struct dsc **main_index = NULL;

/* does the object exist? */
int exists_object(obj)
    int obj;
{
  return(_exists_object(obj));
}

/* retrieve a string from an object. */
int get_str_elt(obj, code, ret)
    int obj, code;
    char **ret;
{
  struct obj_data *theobj;

  if(!_exists_object(obj))
    return(-1);

  if((code == NAME) && (DSC_TYPE(main_index[obj]) == TYP_PLAYER)) {
    *ret = ptable_lookup(obj);
    if(*ret != (char *)NULL)
      return(0);
  }

  theobj = lookup_obj(obj);
  if(theobj == (struct obj_data *)NULL)
    return(-1);

  if(code == NAME) {
    *ret = theobj->name;
  } else {
    logfile(LOG_ERROR, "get_str_elt: invalid element code (%d)\n", code);
    return(-1);
  }
  return(0);
}

/* retrieve an integer array from an object. */
int get_array_elt(obj, code, ret)
    int obj, code, **ret;
{
  if(!_exists_object(obj))
    return(-1);

  if(code != DESTS) {
    logfile(LOG_ERROR, "get_array_elt: bad element code (%d)\n", code);
    return(-1);
  }

  /* only exits have dests! */
  if(DSC_TYPE(main_index[obj]) == TYP_EXIT) {
    *ret = DSC_DESTS(main_index[obj]);
    return(0);
  } else
    return(-1);
}

/* retrieve flags from an object. */
int get_flags_elt(obj, code, ret)
    int obj, code, *ret;
{
  int i;

  if(!_exists_object(obj))
    return(-1);

  if(code != FLAGS) {
    logfile(LOG_ERROR, "get_flags_elt: bad element code (%d)\n", code);
    return(-1);
  }

  for(i = 0; i < FLAGS_LEN; i++)
    ret[i] = (DSC_FLAGS(main_index[obj]))[i];
  return(0);
}

/* retrieve an integer from an object. */
int get_int_elt(obj, code, ret)
    int obj, code;
    int *ret;
{
  struct obj_data *theobj;

  if(!_exists_object(obj))
    return(-1);

  /* check the index first. */
  switch(code) {
  case NEXT:
    *ret = DSC_NEXT(main_index[obj]);
    break;
  case OWNER:
    *ret = DSC_OWNER(main_index[obj]);
    break;
  case PARENT:
    *ret = DSC_PARENT(main_index[obj]);
    break;
  case HOME:
    /* exits do not have homes! */
    if(DSC_TYPE(main_index[obj]) == TYP_EXIT)
      return(-1);
    *ret = DSC_HOME(main_index[obj]);
    break;
  default:
    /* now lookup the object. */
    theobj = lookup_obj(obj);
    if(theobj == (struct obj_data *)NULL)
      return(-1);

    switch(code) {
    case QUOTA:
      *ret = theobj->quota;
      break;
    case LOC:
      *ret = theobj->loc;
      break;
    case CONTENTS:
      *ret = theobj->contents;
      break;
    case EXITS:
      *ret = theobj->exits;
      break;
    case ROOMS:
      *ret = theobj->rooms;
      break;
    case USES:
      *ret = theobj->usecnt;
      break;
    case CHARGES:
      *ret = theobj->charges;
      break;
    case SEMAPHORES:
      *ret = theobj->semaphores;
      break;
    case COST:
      *ret = theobj->cost;
      break;
    case QUEUE:
      *ret = theobj->queue;
      break;
    default:
      logfile(LOG_ERROR, "get_int_elt: invalid element code (%d)\n", code);
      return(-1);
    }
  }
  return(0);
}

/* Retrieve a time_t from an object. */
int get_time_elt(obj, code, value)
    int obj, code;
    time_t *value;
{
  struct obj_data *theobj;

  if(!_exists_object(obj))
    return(-1);

  /* Look up the object. */
  theobj = lookup_obj(obj);
  if(theobj == (struct obj_data *)NULL)
    return(-1);

  switch(code) {
  case TIMESTAMP:
    *value = theobj->timestamp;
    break;
  case CREATESTAMP:
    *value = theobj->created;
    break;
  default:
    logfile(LOG_ERROR, "get_time_elt: invalid element code (%d)\n", code);
    return(-1);
  }
  return(0);
}

/* set a string on an object. */
int set_str_elt(obj, code, value)
    int obj, code;
    char *value;
{
  struct obj_data *theobj;
  int newsize, valsize;
  char **ptr;

  if(!_exists_object(obj))
    return(-1);
  theobj = lookup_obj(obj);
  if(theobj == (struct obj_data *)NULL)
    return(-1);

  if(code == NAME) {
    ptr = &(theobj->name);
  } else {
    logfile(LOG_ERROR, "set_str_elt: invalid element code (%d)\n", code);
    return(-1);
  }

  valsize = (value == (char *)NULL) ? 0 : strlen(value);
  if(*ptr == (char *)NULL) {
    newsize = DSC_SIZE(main_index[obj]) + valsize;
  } else {
    newsize = (DSC_SIZE(main_index[obj]) - strlen(*ptr)) + valsize;
  }
  mudstat.cache_usage =
  	(mudstat.cache_usage - DSC_SIZE(main_index[obj])) + newsize;
  DSC_SIZE(main_index[obj]) = newsize;
  DSC_FLAG2(main_index[obj]) |= DIRTY;

  /* is this a player? */
  if ((code == NAME) && (DSC_TYPE(main_index[obj]) == TYP_PLAYER)) {
    if((value != (char *)NULL) && value[0]) {
      ptable_delete(obj);
      ptable_add(obj, value);
    } else {
      ptable_delete(obj);
    }
  }

  ty_free((VOID *) (*ptr));
  if(value == (char *)NULL) {
    *ptr = (char *)NULL;
  } else {
    *ptr = (char *)ty_strdup(value, "set_str_elt.ptr");
  }
  return(0);
}

/* set an integer array on an object. */
int set_array_elt(obj, code, value)
    int obj, code, *value;
{
  int *newval;

  if(!_exists_object(obj))
    return(-1);

  if(code != DESTS) {
    logfile(LOG_ERROR, "set_array_elt: invalid element code (%d)\n", code);
    return(-1);
  }

  /* only exits have dests! */
  if(DSC_TYPE(main_index[obj]) != TYP_EXIT)
    return(-1);

  /* we handle memory management here, but assume value is formatted correct */
  if(value != (int *)NULL) {
    newval = (int *)ty_malloc((value[0] + 1) * sizeof(int),
    		              "set_array_elt.newval");
    bcopy((VOID *) value, (VOID *) newval, (value[0] + 1) * sizeof(int));
  } else
    newval = (int *)NULL;

  ty_free((VOID *) DSC_DESTS(main_index[obj]));
  DSC_DESTS(main_index[obj]) = newval;

  return(0);
}

/* set the flags on an object. */
int set_flags_elt(obj, code, value)
    int obj, code, *value;
{
  int i;

  if(!_exists_object(obj))
    return(-1);

  if(code != FLAGS) {
    logfile(LOG_ERROR, "set_flags_elt: invalid element code (%d)\n", code);
    return(-1);
  }

  for(i = 0; i < FLAGS_LEN; i++)
    (DSC_FLAGS(main_index[obj]))[i] = value[i];
  return(0);
}

/* set an integer on an object. */
int set_int_elt(obj, code, value)
    int obj, code, value;
{
  struct obj_data *theobj;

  if(!_exists_object(obj))
    return(-1);

  /* check the index first. */
  switch(code) {
  case NEXT:
    DSC_NEXT(main_index[obj]) = value;
    return(0);
  case OWNER:
    DSC_OWNER(main_index[obj]) = value;
    return(0);
  case PARENT:
    DSC_PARENT(main_index[obj]) = value;
    return(0);
  case HOME:
    /* exits do not have homes! */
    if(DSC_TYPE(main_index[obj]) == TYP_EXIT)
      return(-1);
    DSC_HOME(main_index[obj]) = value;
    return(0);
  default:
    /* look up the object. */
    theobj = lookup_obj(obj);
    if(theobj == (struct obj_data *)NULL)
      return(-1);

    switch(code) {
    case QUOTA:
      theobj->quota = value;
      break;
    case LOC:
      theobj->loc = value;
      break;
    case CONTENTS:
      theobj->contents = value;
      break;
    case EXITS:
      theobj->exits = value;
      break;
    case ROOMS:
      theobj->rooms = value;
      break;
    case USES:
      theobj->usecnt = value;
      break;
    case CHARGES:
      theobj->charges = value;
      break;
    case SEMAPHORES:
      theobj->semaphores = value;
      break;
    case COST:
      theobj->cost = value;
      break;
    case QUEUE:
      theobj->queue = value;
      break;
    default:
      logfile(LOG_ERROR, "set_int_elt: invalid element code (%d)\n", code);
      return(-1);
    }
  }

  DSC_FLAG2(main_index[obj]) |= DIRTY;
  return(0);
}

/* Set a time value on an object. */
int set_time_elt(obj, code, value)
    int obj, code;
    time_t value;
{
  struct obj_data *theobj;

  if(!_exists_object(obj))
    return(-1);

  /* Look up the object. */
  theobj = lookup_obj(obj);
  if(theobj == (struct obj_data *)NULL)
    return(-1);

  switch(code) {
  case TIMESTAMP:
    theobj->timestamp = value;
    break;
  case CREATESTAMP:
    theobj->created = value;
    break;
  default:
    logfile(LOG_ERROR, "set_time_elt: invalid element code (%d)\n", code);
    return(-1);
  }

  DSC_FLAG2(main_index[obj]) |= DIRTY;
  return(0);
}

/* destroy an object. */
void destroy_obj(obj)
    int obj;
{
  struct obj_data *theobj;

  if(!_exists_object(obj))
    return;
  theobj = lookup_obj(obj);
  if(theobj == (struct obj_data *) NULL)
    return;

  /* remove from cache and disk. */
  cache_delete(theobj);
  disk_delete(theobj);

  /* free everything. */
  free_obj(theobj);		/* BOOM! */

  /* type specific stuff */
  switch(DSC_TYPE(main_index[obj])) {
  case TYP_PLAYER:
    ptable_delete(obj);
    break;
  case TYP_EXIT:
    ty_free((VOID *) DSC_DESTS(main_index[obj]));
    break;
  }

  dsc_free(main_index[obj]);
  mudstat.actual_objects--;
  mudstat.garbage_count++;
  main_index[obj] = (struct dsc *)NULL;
}

/* create a new object. */
int create_obj(type)
    int type;
{
  struct obj_data *theobj;
  struct dsc *thedsc;
  int num;

  thedsc = dsc_alloc();
  theobj = (struct obj_data *)ty_malloc(sizeof(struct obj_data),
  					"create_obj.theobj");
  bzero((VOID *)theobj, sizeof(struct obj_data));
  
  /* set it up. */
  DSC_DATA(thedsc) = theobj;
  DSC_SIZE(thedsc) = (14 * sizeof(int)) + 1;
  DSC_FLAG1(thedsc) = type;
  DSC_FLAG2(thedsc) = IN_MEMORY | DIRTY;
  DSC_OWNER(thedsc) = -1;
  DSC_PARENT(thedsc) = -1;
  DSC_NEXT(thedsc) = -1;

  /* exit? */
  if(type != TYP_EXIT) {
    DSC_HOME(thedsc) = -1;
  } else {
    DSC_DESTS(thedsc) = (int *)NULL;
  }

  /* bzero((VOID *)theobj->attributes, sizeof(struct attr *) * ATTR_WIDTH); */
  theobj->attr_total = 0;
  theobj->name = (char *)NULL;
  theobj->contents = -1;
  theobj->exits = -1;
  theobj->rooms = (type == TYP_PLAYER) ? 0 : -1;
  theobj->quota = 0;
  theobj->loc = 0;
  theobj->timestamp = 0;
  theobj->created = 0;
  theobj->usecnt = 0;
  theobj->charges = -1;
  theobj->semaphores = 0;
  theobj->cost = 0;
  theobj->queue = 0;

  if(mudstat.garbage_count == 0) {
    if(mudstat.slack == 0)
      grow_index();
    mudstat.slack--;
    main_index[mudstat.total_objects] = thedsc;
    num = mudstat.total_objects;
    mudstat.total_objects++;
  } else {
    for(num = mudstat.total_objects - 1; num > 0; num--) {
      if(main_index[num] == (struct dsc *)NULL)
        break;
    }
    if((num == 0) || (main_index[num] != (struct dsc *)NULL)) {
      logfile(LOG_ERROR, "create_obj: garbage count out of sync\n");
      return(-1);
    }
    main_index[num] = thedsc;
    mudstat.garbage_count--;
  }
  mudstat.actual_objects++;

  theobj->objnum = num;
  cache_insert(theobj);

  stamp(num, STAMP_CREATED);
  return(num);
}

/* look an object up. */
struct obj_data *lookup_obj(obj)
    int obj;
{
  struct obj_data *theobj;

  /* non-existance is an error. */
  if(!_exists_object(obj)) {
    mudstat.cache_errors++;
    return((struct obj_data *)NULL);
  }
  if(!(DSC_FLAG2(main_index[obj]) & IN_MEMORY)) {
    mudstat.cache_misses++;
    
    theobj = disk_thaw(obj);
    if(theobj == (struct obj_data *)NULL) {
      logfile(LOG_ERROR, "lookup_obj: thaw of #%d failed.\n", obj);
      mudstat.cache_errors++;
      return((struct obj_data *)NULL);
    }
    cache_insert(theobj);
  } else {
    mudstat.cache_hits++;
    theobj = DSC_DATA(main_index[obj]);
    cache_touch(theobj);
  }
  return(theobj);
}

/* write out the database file, new style. */
int database_write(name)
    char *name;
{
  FILE *fp;
  struct dsc *thedsc;
  int i;
  int work[6 + FLAGS_LEN];		/* minimal write buffer */

  fp = fopen(name, "w");
  if (fp == (FILE *)NULL) {
    logfile(LOG_ERROR, "database_write: couldn't open %s for writing.\n", name);
    return(-1);
  }

  /* first things first. */ 
  work[0] = mudstat.actual_objects;
  work[1] = mudstat.garbage_count;
  work[2] = mudstat.total_objects;

  if(fwrite((char *)work, sizeof(int), 3, fp) < 3) {
    logfile(LOG_ERROR, "database_write: error writing file.\n");
    fclose(fp);
    return(-1);
  }

  /* write out the objects. */
  for (i = 0; i < mudstat.total_objects; i++) {
    if (_exists_object(i)) {
      thedsc = main_index[i];

      work[0] = i;              /* Obj # */
      work[1] = DSC_FLAG1(thedsc);
      work[2] = (DSC_FLAG2(thedsc) & ~INTERNAL_FLAGS);
      work[3] = DSC_SIZE(thedsc);
      work[4] = DSC_NEXT(thedsc);
      work[5] = DSC_OWNER(thedsc);
      work[6] = DSC_PARENT(thedsc);

      /* extra glue for destinations */
      if(DSC_TYPE(thedsc) != TYP_EXIT) {
        work[7] = DSC_HOME(thedsc);

	if(fwrite((char *)work, sizeof(work), 1, fp) < 1) {
	  logfile(LOG_ERROR, "database_write: error writing file.\n");
	  fclose(fp);
          return(-1);
        }
      } else {
        if(DSC_DESTS(thedsc) == (int *)NULL) {
	  work[7] = -1;

	  if(fwrite((char *)work, sizeof(work), 1, fp) < 1) {
	    logfile(LOG_ERROR, "database_write: error writing file.\n");
	    fclose(fp);
	    return(-1);
	  }
	} else {
          if(fwrite((char *)work, sizeof(int), 7, fp) < 7) {
	    logfile(LOG_ERROR, "database_write: error writing file.\n");
	    fclose(fp);
	    return(-1);
	  }

	  /* the first element of dests is how many follow. */
	  if(fwrite((char *)DSC_DESTS(thedsc), sizeof(int),
	  	(DSC_DESTS(thedsc)[0])+1, fp) < (DSC_DESTS(thedsc)[0])+1) {
	    logfile(LOG_ERROR, "database_write: error writing file.\n");
	    fclose(fp);
	    return(-1);
	  }
	}
      }    
    }
  }

  fclose(fp);
  return(0);
}

/* read in the new style database file. */
int database_read(name)
    char *name;
{
  FILE *fp;
  int i;
  int objnum;
  struct dsc *thedsc;
  int work[6 + FLAGS_LEN];
  int *dests;

  fp = fopen(name, "r");
  if (fp == (FILE *)NULL) {
    logfile(LOG_ERROR, "database_read: couldn't open descriptor file %s.\n",
	    name);
    return(-1);
  }

  /* Read in the initial stuff */
  if(feof(fp) || (fread((char *)work, sizeof(int), 3, fp) < 3)) {
    logfile(LOG_ERROR, "database_read: error reading descriptor file.\n");
    fclose(fp);
    return (-1);
  }
  mudstat.actual_objects = work[0];
  mudstat.garbage_count = work[1];
  mudstat.total_objects = work[2];

  /* initialize the database. */
  initialize_db(mudstat.total_objects + mudconf.slack);
  mudstat.slack = mudconf.slack;

  /* read in the objects, being smart about exit destinations. */
  for (i = mudstat.actual_objects; i > 0; i--) {
    thedsc = dsc_alloc();
    if (feof(fp) || (fread((char *)work, sizeof(work), 1, fp) < 1)) {
      logfile(LOG_ERROR, "database_read: error reading descriptor file.\n");
      fclose(fp);
      return (-1);
    }
    objnum = work[0];
    main_index[objnum] = thedsc;
    DSC_FLAG1(thedsc) = work[1];
    DSC_FLAG2(thedsc) = work[2];
    DSC_SIZE(thedsc) = work[3];
    DSC_NEXT(thedsc) = work[4];
    DSC_OWNER(thedsc) = work[5];
    DSC_PARENT(thedsc) = work[6];

    /* the might be an exit... */
    if(DSC_TYPE(thedsc) != TYP_EXIT) {
      DSC_HOME(thedsc) = work[7];
    } else {
      if(work[7] != -1) {
        /* first element is how many follow. */
        dests = (int *)ty_malloc((work[7] + 1) * sizeof(int),
				 "read_database.dests");
	dests[0] = work[7];
	if (feof(fp) || (fread((char *)dests + sizeof(int), sizeof(int),
			 dests[0], fp) < dests[0])) {
	  logfile(LOG_ERROR, "database_read: error reading descriptor file.\n");
	  fclose(fp);
	  ty_free((VOID *)dests);
	  return(-1);
	}

	DSC_DESTS(thedsc) = dests;
      } else {
        DSC_DESTS(thedsc) = (int *)NULL;
      }
    }
  }

  fclose(fp);
  return (0);
}

/* This grows the index by the specified increment. */
static void grow_index()
{
  int newsize =
	(mudstat.total_objects + mudconf.growth_increment + mudstat.slack);

  main_index = (struct dsc **)ty_realloc((VOID *)main_index,
		(size_t)(sizeof(struct dsc *) * newsize),
		"grow_index.main_index");
  while(newsize > mudstat.total_objects)
    main_index[--newsize] = (struct dsc *)NULL;
}

/* descriptor management code. */
static struct dsc *free_dscs = (struct dsc *)NULL;

void initialize_db(size)
    int size;
{
  int idx = 0;

  free_dscs = (struct dsc *)ty_malloc(sizeof(struct dsc) * size,
				      "initialize_db.free_dscs");

  /* This loop is written like this for a *reason*. */
  while(idx < (size - 1)) {
    (free_dscs[idx].ptr).next = &free_dscs[idx + 1];
    idx++;
  }
  (free_dscs[idx].ptr).next = (struct dsc *)NULL;

  main_index = (struct dsc **)ty_malloc(sizeof(struct dsc *) * size,
					"initialize_db.main_index");
  bzero((VOID *)main_index, sizeof(struct dsc *) * size);
}

static struct dsc *dsc_alloc()
{
  struct dsc *ret;

  if(free_dscs == (struct dsc *)NULL) {
    int idx = 0;

    free_dscs = (struct dsc *)ty_malloc(sizeof(struct dsc) * 64,
					"dsc_alloc.free_dscs");
    while(idx < 63) {
      (free_dscs[idx].ptr).next = &free_dscs[idx + 1];
      idx++;
    }
    (free_dscs[idx].ptr).next = (struct dsc *)NULL;
  }

  ret = free_dscs;
  free_dscs = (free_dscs->ptr).next;

  bzero((VOID *)ret, sizeof(struct dsc));
  return(ret);
}

static void dsc_free(ptr)
    struct dsc *ptr;
{
  if(ptr != (struct dsc *)NULL) {
    (ptr->ptr).next = free_dscs;
    free_dscs = ptr;
  }
}

/* free all the data associated with the object. */
void free_obj(obj)
    struct obj_data *obj;
{
  ty_free((VOID *)obj->name);

  free_attributes(obj->attributes);
  ty_free((VOID *)obj);
}

/* Misc. flag checkers and such. */
static char _flagerr[] = "%s: bad obj passed as an argument, #%d\n";

int setFlag(obj, flag, word, macro)
    int obj, flag, word;
    char *macro;
{
  if(!_exists_object(obj)) {
    logfile(LOG_ERROR, _flagerr, macro, obj);
    return(-1);
  }
  (DSC_FLAGS(main_index[obj]))[word] |= flag;
  return(0);
}

int unsetFlag(obj, flag, word, macro)
    int obj, flag, word;
    char *macro;
{
  if(!_exists_object(obj)) {
    logfile(LOG_ERROR, _flagerr, macro, obj);
    return(-1);
  }
  (DSC_FLAGS(main_index[obj]))[word] &= ~flag;
  return(0);
}

int Flags(obj, flags, macro)
    int obj, *flags;
    char *macro;
{
  if(!_exists_object(obj)) {
    logfile(LOG_ERROR, _flagerr, macro, obj);
    return(0);
  }
  return(((flags[0] == 0) || (DSC_FLAG1(main_index[obj]) & flags[0]))
	 && ((flags[1] == 0) || (DSC_FLAG2(main_index[obj]) & flags[1])));
}

int Flag1(obj, flag, macro)
    int obj, flag;
    const char *macro;
{
  if(!_exists_object(obj)) {
    logfile(LOG_ERROR, _flagerr, macro, obj);
    return(0);
  }
  return(DSC_FLAG1(main_index[obj]) & flag);
}

int Flag2(obj, flag, macro)
    int obj, flag;
    const char *macro;
{
  if(!_exists_object(obj)) {
    logfile(LOG_ERROR, _flagerr, macro, obj);
    return(0);
  }
  return(DSC_FLAG2(main_index[obj]) & flag);
}

int Typeof(obj)
    int obj;
{
  if (!_exists_object(obj)) {
    logfile(LOG_ERROR, _flagerr, "Typeof", obj);
    return (0);
  }
  return (DSC_TYPE(main_index[obj]));
}

/* yes, these are back. ARGH. */
void animate(obj)
    int obj;
{
  if(!_exists_object(obj)) {
    logfile(LOG_ERROR, _flagerr, "animate", obj);
  } else {
    DSC_FLAG2(main_index[obj]) |= ALIVE;
  }
}

void deanimate(obj)
    int obj;
{
  if(!_exists_object(obj)) {
    logfile(LOG_ERROR, _flagerr, "deanimate", obj);
  } else {
    DSC_FLAG2(main_index[obj]) &= ~ALIVE;
  }
}

/* this routine shouldn't be called very much, it has no sanity checking. */
int getowner(obj)
    int obj;
{
  return(DSC_OWNER(main_index[obj]));
}
