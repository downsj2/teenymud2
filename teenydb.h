/* teenydb.h */

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

#ifndef __DB_H

/* database structures and such. */

/* user-def'd attribute struct. keep in sync with attrs.h! */

#define ATTR_STRING	0
#define ATTR_LOCK	1

struct attr {
  short type;

  char *name;
  union {
    char *str;
    struct boolexp *lock;
  } dat;

  int flags;

  /* these should always be last. */
  struct attr *prev;
  struct attr *next;
};

/* attribute search struct. keep in sync with attrs.h! */
struct asearch {
  int elem;
  struct attr *curr;
};

struct obj_data {

  /* integer fields */

  int quota;
  int loc;
  int contents;
  int exits;
  int rooms;
  int timestamp;
  int created;
  int usecnt;
  int charges;
  int semaphores;
  int cost;
  int queue;

  /* zero terminated strings. */

  char *name;

  /* user def'd attributes */

  int attr_total;		/* total number of user-def'd attributes */
#define ATTR_WIDTH	12	/* width of the attribute hash */
  struct attr *attributes[ATTR_WIDTH];

  /* for use by the caching system */

  int objnum;
  struct obj_data *fwd;
  struct obj_data *back;
};

/* A typical memory resident descriptor. If it describes an actual object, it
 * will be referenced by the main object index array, and the name field of
 * the ptr union will be meaningful. If it describes a free chunk, the next
 * field will be. We do it this way so objects can conveniently be destroyed.
 */

#ifndef FLAGS_LEN
#define FLAGS_LEN	2	/* number of words */
#endif

struct dsc {
  int flags[FLAGS_LEN];
  int owner;

  union {
    int home_dropto;		/* home for things, dropto for rooms */
    int *dests;			/* destinations for exits. */
  } dst;

  int parent;			/* object's attribute parent */
  int size;			/* Size on disk. */
  int list_next;		/* exits/contents/rooms lists. */

  /* these should always go last. */
  union {
    struct obj_data *data;
    struct dsc *next;		/* free descriptor list */
  } ptr;
};

/* the database. */
extern struct dsc **main_index;

#define DSC_DATA(_x)	(((_x)->ptr).data)
#define DSC_SIZE(_x)	((_x)->size)
#define DSC_TYPE(_x)	(((_x)->flags[0]) & TYPE_MASK)
#define DSC_FLAG1(_x)	((_x)->flags[0])
#define DSC_FLAG2(_x)	((_x)->flags[1])
#define DSC_FLAGS(_x)	((_x)->flags)
#define DSC_OWNER(_x)	((_x)->owner)
#define DSC_PARENT(_x)	((_x)->parent)
#define DSC_HOME(_x)	(((_x)->dst).home_dropto)
#define DSC_DROPTO(_x)	(((_x)->dst).home_dropto)
#define DSC_DESTS(_x)	(((_x)->dst).dests)
#define DSC_NEXT(_x)	((_x)->list_next)

/* other internal macros */
#define _exists_object(_x)	((_x >= 0) && (_x < mudstat.total_objects) && \
				 (main_index[_x] != (struct dsc *)NULL))

/* dbm database layer flags. */
#define TEENY_DBMFAST	0x01
#define TEENY_DBMREAD	0x02

/* prototypes for the db layer */
/* from cache.c */
extern void cache_delete _ANSI_ARGS_((struct obj_data *));
extern void cache_touch _ANSI_ARGS_((struct obj_data *));
extern void cache_insert _ANSI_ARGS_((struct obj_data *));

/* from db.c */
extern struct obj_data *lookup_obj _ANSI_ARGS_((int));
extern void free_obj _ANSI_ARGS_((struct obj_data *));

/* from [bsd|g]dbm.c */
extern int disk_freeze _ANSI_ARGS_((struct obj_data *));
extern struct obj_data *disk_thaw _ANSI_ARGS_((int));
extern void disk_delete _ANSI_ARGS_((struct obj_data *));

/* from attributes.c */
extern int attr_hash _ANSI_ARGS_((char *));
extern struct attr *attr_first _ANSI_ARGS_((int, struct asearch *));
extern struct attr *attr_next _ANSI_ARGS_((int, struct asearch *));
extern void free_attributes _ANSI_ARGS_((struct attr *[]));

#define __DB_H
#endif				/* __DB_H */
