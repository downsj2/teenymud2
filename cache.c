/* cache.c */

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
#ifdef HAVE_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif			/* HAVE_STRING_H */

#include "conf.h"
#include "teeny.h"
#include "teenydb.h"
#include "externs.h"

/* new, hashed, cache. */

static struct obj_data **main_cache = NULL;
static int cache_locked = 0;

static void cache_resize _ANSI_ARGS_((int));

#define cache_hash(x)	((x->objnum) % mudconf.cache_width)

/* initialize the cache state. */
void cache_init()
{
  /* set variables */
  mudstat.cache_usage = 0;
  mudstat.cache_hits = 0;
  mudstat.cache_errors = 0;
  mudstat.cache_misses = 0;

  /* set up the hash table. */
  main_cache = (struct obj_data **)
  	ty_malloc(sizeof(struct obj_data *) * mudconf.cache_width,
		  "cache_init");
  bzero((VOID *)main_cache, sizeof(struct obj_data *) * mudconf.cache_width);
}

/* grow new buckets. ONLY CALL WHEN THE CACHE IS EMPTY OR DIE. */
static void cache_resize(nbuckets)
    int nbuckets;
{
  mudconf.cache_width = nbuckets;

  ty_free((VOID *)main_cache);
  main_cache = (struct obj_data **)
  	ty_malloc(sizeof(struct obj_data *) * mudconf.cache_width,
		  "cache_resize");
  bzero((VOID *)main_cache, sizeof(struct obj_data *) * mudconf.cache_width);
}

/* "lock" the cache. */
void cache_lock()
{
  cache_locked = 1;
}

/* "unlock" the cache. */
void cache_unlock()
{
  cache_locked = 0;
}

/* delete an object from cache. */
void cache_delete(obj)
    struct obj_data *obj;
{
  int bucket = cache_hash(obj);

  /* only thing in this bucket? */
  if((obj == main_cache[bucket]) && (main_cache[bucket]->fwd == obj)) {
    main_cache[bucket] = (struct obj_data *)NULL;
  } else {
    if (main_cache[bucket] == obj)
      main_cache[bucket] = obj->fwd;
    (obj->back)->fwd = obj->fwd;
    (obj->fwd)->back = obj->back;
  }

  mudstat.cache_usage -= DSC_SIZE(main_index[obj->objnum]);
}

/* move the obj to the head of it's bucket. */
void cache_touch(obj)
    struct obj_data *obj;
{
  int bucket = cache_hash(obj);

  /* already at the top? */
  if(obj != main_cache[bucket]) {
    (obj->back)->fwd = obj->fwd;
    (obj->fwd)->back = obj->back;

    obj->fwd = main_cache[bucket];
    obj->back = (main_cache[bucket])->back;
    ((main_cache[bucket])->back)->fwd = obj;
    (main_cache[bucket])->back = obj;

    main_cache[bucket] = obj;
  }
}

/* insert an object into the cache. trim if needed. */
void cache_insert(obj)
    struct obj_data *obj;
{
  int bucket = cache_hash(obj);

  /* bucket empty? */
  if(main_cache[bucket] == (struct obj_data *)NULL) {
    main_cache[bucket] = obj->back = obj->fwd = obj;
  } else {
    /* insert at top. */
    obj->fwd = main_cache[bucket];
    obj->back = (main_cache[bucket])->back;
    ((main_cache[bucket])->back)->fwd = obj;
    (main_cache[bucket])->back = obj;
    main_cache[bucket] = obj;
  }

  mudstat.cache_usage += DSC_SIZE(main_index[obj->objnum]);
  cache_trim();		/* trim it. */
}

/* trim the cache down to size. */
void cache_trim()
{
  struct obj_data *obj;
  int ret, bucket, bdone, count;

  /* loop through all of the buckets repeatedly, trimming the last object */
  /* in each, until the cache comes under control. */

  count = 1;	/* infinite loop prevention. */
  while(count && (mudstat.cache_usage > mudconf.cache_size)) {
    for(bucket = 0, count = 0; bucket < mudconf.cache_width; bucket++) {
      /* if the cache is locked, and the object is dirty, we don't do */
      /* anything to it, we wait until the cache is unlocked. */

      if (main_cache[bucket] != (struct obj_data *)NULL) {
        for (obj = (main_cache[bucket])->back, bdone = 0;
	     obj != main_cache[bucket] && !bdone; obj = obj->back) {
	  if (!(DSC_FLAG2(main_index[obj->objnum]) & DIRTY) || !cache_locked) {

	    /* if disk_freeze() fails, keep the object around. */
	    ret = 0;
	    if (DSC_FLAG2(main_index[obj->objnum]) & DIRTY)
	      ret = disk_freeze(obj);
	    if(ret == 0) {  /* If disk_freeze() was successful, or the object is not DIRTY. */
	      DSC_FLAG2(main_index[obj->objnum]) &= ~IN_MEMORY;

	      (obj->back)->fwd = obj->fwd;
	      (obj->fwd)->back = obj->back;

	      mudstat.cache_usage -= DSC_SIZE(main_index[obj->objnum]);
	      /* done with this bucket, for now. */
	      bdone++;
	      count++;

	      /* And finally, free the data. */
	      free_obj(obj);
	    }
	  }
	}
      }
    }
  }
}

/* flush the entire cache to disk. */
void cache_flush()
{
  struct obj_data *obj, *next;
  int bucket;
  int count = 0;
  int nbuckets = 0;

  for(bucket = 0; bucket < mudconf.cache_width; bucket++) {
    if(main_cache[bucket] != (struct obj_data *)NULL) {
      obj = main_cache[bucket];
      do {
	next = obj->fwd;

	/* only try to save it if it's dirty. */
	if (DSC_FLAG2(main_index[obj->objnum]) & DIRTY)
	  /* if disk_freeze() fails here, we lose data! */
	  disk_freeze(obj);
	DSC_FLAG2(main_index[obj->objnum]) &= ~IN_MEMORY;
	free_obj(obj);

	obj = next;
	count++;
      } while(obj != main_cache[bucket]);

      if(count > mudconf.cache_depth)
        nbuckets++;
    }
  }

  /* reset the cache. */
  if(nbuckets) {
    cache_resize(mudconf.cache_width + nbuckets);
  } else {
    bzero((VOID *)main_cache, sizeof(struct obj_data *) * mudconf.cache_width);
  }
  mudstat.cache_usage = 0;
  mudstat.cache_hits = 0;
  mudstat.cache_misses = 0;
}
