/* gdbm.c */

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
#ifdef HAVE_MALLOC_H
#include <malloc.h>
#endif			/* HAVE_MALLOC_H */
#ifdef HAVE_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif			/* HAVE_STRING_H */
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif			/* HAVE_STDLIB_H */
#include <gdbm.h>

#include "conf.h"
#include "teeny.h"
#include "teenydb.h"
#include "externs.h"

/* TeenyMUD GNU dbm interface. */

static GDBM_FILE gfd;
static datum key, cont;
static int *obj_work = (int *)NULL;
static int work_siz = 0;

static void gdbm_panic _ANSI_ARGS_((char *));
INLINE static char *gdbm_pack_data _ANSI_ARGS_((char *, char *, int));
static char *gdbm_pack_lock _ANSI_ARGS_((char *, struct boolexp *));
INLINE static char *gdbm_pack_string _ANSI_ARGS_((char *, char *));
static void gdbm_emergency_growbuf _ANSI_ARGS_((void));
INLINE static char *gdbm_unpack_data _ANSI_ARGS_((char *, char *, int));
static char *gdbm_unpack_lock _ANSI_ARGS_((struct boolexp **, char *));
INLINE static char *gdbm_unpack_string _ANSI_ARGS_((char **, char *));

extern gdbm_error gdbm_errno;

#define ALIGN(p)        (((int)p) % sizeof(int) == 0 ? ((int)p) : \
                         ((int)p) + sizeof(int) - ((int)p) % sizeof(int))

/* This is simply a wrapper around panic(). */
static void gdbm_panic(mesg)
    char *mesg;
{
  panic("GDBM panic: %s\n", mesg);
}

int dbmfile_open(name, flags)
    char *name;
    int flags;
{
  int cache = 10;
  int gflags = 0;

  if (flags & TEENY_DBMFAST)
    gflags |= GDBM_FAST;
  if (flags & TEENY_DBMREAD) {
    gflags |= GDBM_READER;
  } else {
    gflags |= GDBM_WRITER;
  }

  if((gfd = gdbm_open(name, MEDBUFFSIZ, gflags, 0600, gdbm_panic)) == NULL) {
    logfile(LOG_ERROR, "dbmfile_open: couldn't open %s, %s.\n", name, 
    	    gdbm_strerror(gdbm_errno));
    return(-1);
  }
  if(gdbm_setopt(gfd, GDBM_CACHESIZE, &cache, sizeof(int)) == -1) {
    logfile(LOG_ERROR, "dbmfile_open: couldn't set cache size, %s.\n",
    	    gdbm_strerror(gdbm_errno));
    return(-1);
  }
  return(0);
}

int dbmfile_init(name, flags)
    char *name;
    int flags;
{
  int cache = 10;
  int gflags = GDBM_NEWDB;

  if (flags & TEENY_DBMFAST)
    gflags |= GDBM_FAST;

  if((gfd = gdbm_open(name, MEDBUFFSIZ, gflags, 0600, gdbm_panic)) == NULL) {
    logfile(LOG_ERROR, "dbmfile_init: couldn't open %s, %s.\n", name,
    	    gdbm_strerror(gdbm_errno));
    return(-1);
  }
  if(gdbm_setopt(gfd, GDBM_CACHESIZE, &cache, sizeof(int)) == -1) {
    logfile(LOG_ERROR, "dbmfile_init: couldn't set cache size, %s.\n",
    	    gdbm_strerror(gdbm_errno));
    return(-1);
  }
  return(0);
}

void dbmfile_close()
{
  gdbm_close(gfd);
}

INLINE static char *gdbm_pack_data(dest, source, size)
    char *dest, *source;
    int size;
{
  bcopy((VOID *)source, (VOID *)dest, size);
  return((char *)(dest + size));
}

static char *gdbm_pack_lock(dest, src)
    char *dest;
    struct boolexp *src;
{
  if(src == (struct boolexp *)NULL) {
    short type = BOOLEXP_END;		/* i just *had* to use short... */

    return(gdbm_pack_data(dest, (char *)&type, sizeof(short)));
  }

  dest = gdbm_pack_data(dest, (char *)&(src->type), sizeof(short));
  switch(src->type) {
  case BOOLEXP_AND:
  case BOOLEXP_OR:
    dest = gdbm_pack_lock((char *)ALIGN(dest), src->sub2);
  case BOOLEXP_NOT:
    dest = gdbm_pack_lock((char *)ALIGN(dest), src->sub1);
    break;
  case BOOLEXP_CONST:
    dest = gdbm_pack_data((char *)ALIGN(dest), (char *)&((src->dat).thing),
    			  sizeof(int));
    break;
  case BOOLEXP_FLAG:
    dest = gdbm_pack_data((char *)ALIGN(dest), (char *)((src->dat).flags),
    			  sizeof(int) * FLAGS_LEN);
    break;
  case BOOLEXP_ATTR:
    dest = gdbm_pack_string((char *)ALIGN(dest), (src->dat).atr[0]);
    dest = gdbm_pack_string((char *)ALIGN(dest), (src->dat).atr[1]);
    break;
  default:
    logfile(LOG_ERROR, "gdbm_pack_lock: bad boolexp type (%d)\n", src->type);
    dest[-1] = BOOLEXP_END;
  }
  return(dest);
}

INLINE static char *gdbm_pack_string(dest, src)
    register char *dest, *src;
{
  register int len;

  if(src == (char *)NULL){
    dest[0] = '\0';
    return((char *)(dest + 1));
  }

#ifdef notyet
  if(mudconf.enable_compress) {
    register char *csrc;

    csrc = compress(src);
    len = strlen(csrc)+1;
    bcopy((VOID *)csrc, (VOID *)dest, len);
    ty_free(csrc);
  } else {
    len = strlen(src)+1;
    bcopy((VOID *)src, (VOID *)dest, len);
  }
#else
  len = strlen(src)+1;
  bcopy((VOID *)src, (VOID *)dest, len);
#endif
  return((char *)(dest + len));
}

static void gdbm_emergency_growbuf()
{
  obj_work = (int *) ty_realloc(obj_work, work_siz + MEDBUFFSIZ,
  			        "gdbm_emergency_growbuf");
  work_siz += MEDBUFFSIZ;
}

int disk_freeze(data)
    struct obj_data *data;
{
  char *ptr;
  int *iptr;
  struct attr *attrs;
  int obj;
  register int idx;

  obj = data->objnum;

  if (!(DSC_FLAG2(main_index[obj]) & IN_MEMORY)) {
    logfile(LOG_ERROR,
	    "disk_freeze: attempt to freeze non-resident object #%d\n", obj);
    return(-1);
  }

  if ((work_siz == 0) && DSC_SIZE(main_index[obj]) < 10240) {
    /* initialize our buffer */
    obj_work = (int *) ty_malloc(10240L, "disk_freeze.obj_work");
    work_siz = 10240L;
  }
  if (DSC_SIZE(main_index[obj]) > work_siz) {   /* grow it */
    ty_free((VOID *) obj_work);
    obj_work = (int *) ty_malloc(DSC_SIZE(main_index[obj]) + MEDBUFFSIZ,
                                  "disk_freeze.obj_work");
    work_siz = DSC_SIZE(main_index[obj]) + MEDBUFFSIZ;
  }
  bzero((VOID *)obj_work, work_siz);

  /* malloc'd memory should always be aligned, damnit. */
  iptr = obj_work;
  *iptr++ = data->quota;
  *iptr++ = data->loc;
  *iptr++ = data->contents;
  *iptr++ = data->exits;
  *iptr++ = data->rooms;
  *iptr++ = data->timestamp;
  *iptr++ = data->created;
  *iptr++ = data->usecnt;
  *iptr++ = data->charges;
  *iptr++ = data->semaphores;
  *iptr++ = data->cost;
  *iptr++ = data->queue;

  /* attributes */
  *iptr++ = data->attr_total;
  ptr = (char *)iptr;
  for(idx = 0; idx < ATTR_WIDTH; idx++) {
    for(attrs = data->attributes[idx]; attrs != (struct attr *)NULL;
  	attrs = attrs->next) {
      ptr = gdbm_pack_data((char *)ALIGN(ptr), (char *)&attrs->type,
      			   sizeof(int));
      ptr = gdbm_pack_string((char *)ALIGN(ptr), attrs->name);
      if ((ptr - (char *)obj_work) > (work_siz - MEDBUFFSIZ))
        gdbm_emergency_growbuf();
      switch(attrs->type) {
      case ATTR_STRING:
        ptr = gdbm_pack_string((char *)ALIGN(ptr), (attrs->dat).str);
	break;
      case ATTR_LOCK:
        ptr = gdbm_pack_lock((char *)ALIGN(ptr), (attrs->dat).lock);
	break;
      }
      ptr = gdbm_pack_data((char *)ALIGN(ptr), (char *)&attrs->flags,
    			   sizeof(int));
      if ((ptr - (char *)obj_work) > (work_siz = MEDBUFFSIZ))
        gdbm_emergency_growbuf();
    }
  }

  /* strings and such. */
  ptr = gdbm_pack_string((char *)ALIGN(ptr), data->name);

  /* set it up. */
  key.dptr = (char *) &obj;
  key.dsize = sizeof(int);
  cont.dptr = (char *) obj_work;
  cont.dsize = ptr - (char *)obj_work;

  /* store it. */
  if(gdbm_store(gfd, key, cont, GDBM_REPLACE) != 0) {
    logfile(LOG_ERROR, "disk_freeze: failed to store object #%d, %s.\n", obj,
    	    gdbm_strerror(gdbm_errno));
    mudstat.cache_errors++;
    return(-1);
  }

  /* all done! */
  DSC_FLAG2(main_index[obj]) &= ~IN_MEMORY;
  free_obj(data);

  return(0);
}

INLINE static char *gdbm_unpack_data(dest, source, size)
    char *dest, *source;
    int size;
{
  bcopy((VOID *)source, (VOID *)dest, size);
  return((char *)(source + size));
}

static char *gdbm_unpack_lock(dest, source)
    struct boolexp **dest;
    char *source;
{
  short type;

  source = gdbm_unpack_data((char *)&type, source, sizeof(short));
  switch(type) {
  case BOOLEXP_END:
    *dest = (struct boolexp *)NULL;
    return(source);
  case BOOLEXP_AND:
  case BOOLEXP_OR:
    *dest = boolexp_alloc();
    (*dest)->type = type;

    source = gdbm_unpack_lock(&(*dest)->sub2, (char *)ALIGN(source));
    return(gdbm_unpack_lock(&(*dest)->sub1, (char *)ALIGN(source)));
  case BOOLEXP_NOT:
    *dest = boolexp_alloc();
    (*dest)->type = type;

    return(gdbm_unpack_lock(&(*dest)->sub1, (char *)ALIGN(source)));
  case BOOLEXP_CONST:
    *dest = boolexp_alloc();
    (*dest)->type = type;

    return(gdbm_unpack_data((char *)&((*dest)->dat).thing,
    	   (char *)ALIGN(source), sizeof(int)));
  case BOOLEXP_FLAG:
    *dest = boolexp_alloc();
    (*dest)->type = type;

    return(gdbm_unpack_data((char *)((*dest)->dat).flags, (char *)ALIGN(source),
    	   sizeof(int) * FLAGS_LEN));
  case BOOLEXP_ATTR:
    *dest = boolexp_alloc();
    (*dest)->type = type;

    source = gdbm_unpack_string(&(((*dest)->dat).atr[0]),
    				(char *)ALIGN(source));
    return(gdbm_unpack_string(&(((*dest)->dat).atr[1]), (char *)ALIGN(source)));
  default:
    logfile(LOG_ERROR, "gdbm_unpack_lock: bad boolexp type (%c)\n", *source);
    *dest = (struct boolexp *)NULL;
  }
}

INLINE static char *gdbm_unpack_string(dest, source)
    char **dest, *source;
{
  register char *ptr = source;

  if(ptr[0] == '\0') {
    *dest = (char *)NULL;
    return((char *)(source + 1));
  }

  while(ptr[0] != '\0')
    ptr++;
  ptr++;

#ifdef notyet
  *dest = uncompress(source);
#else
  *dest = ty_strdup(source, "gdbm_unpack_string.ret");
#endif
  return(ptr);
}

struct obj_data *disk_thaw(obj)
    int obj;
{
  struct obj_data *ret;
  int *iptr;
  char *ptr;
  register int attr_total, hashval;
  struct attr *curr;

  if (DSC_FLAG2(main_index[obj]) & IN_MEMORY) {
    logfile(LOG_ERROR, "disk_thaw: attempt to re-thaw thawed object #%d\n",
	    obj);
    return (DSC_DATA(main_index[obj]));
  }

  /* set it up. */
  key.dptr = (char *) &obj;
  key.dsize = sizeof(int);

  /* fetch it. */
  cont = gdbm_fetch(gfd, key);
  if(cont.dptr == (char *)NULL) {
    logfile(LOG_ERROR, "disk_thaw: couldn't fetch object #%d, %s.\n", obj,
    	    gdbm_strerror(gdbm_errno));
    return((struct obj_data *)NULL);
  }

  ret = (struct obj_data *) ty_malloc(sizeof(struct obj_data),
  					"disk_thaw.ret");
  ret->objnum = obj;

  /* gdbm data should be aligned. */
  iptr = (int *)cont.dptr;
  ret->quota = *iptr++;
  ret->loc = *iptr++;
  ret->contents = *iptr++;
  ret->exits = *iptr++;
  ret->rooms = *iptr++;
  ret->timestamp = *iptr++;
  ret->created = *iptr++;
  ret->usecnt = *iptr++;
  ret->charges = *iptr++;
  ret->semaphores = *iptr++;
  ret->cost = *iptr++;
  ret->queue = *iptr++;

  /* attributes. */
  bzero((VOID *)ret->attributes, sizeof(struct attr *) * ATTR_WIDTH);
  ret->attr_total = *iptr++;
  ptr = (char *)iptr;
  attr_total = ret->attr_total;

  while(attr_total) {
    curr = (struct attr *)ty_malloc(sizeof(struct attr), "disk_thaw.curr");

    ptr = gdbm_unpack_data((char *)&curr->type, (char *)ALIGN(ptr),
    			   sizeof(short));
    ptr = gdbm_unpack_string(&curr->name, (char *)ALIGN(ptr));
    switch(curr->type) {
    case ATTR_STRING:
      ptr = gdbm_unpack_string(&(curr->dat).str, (char *)ALIGN(ptr));
      break;
    case ATTR_LOCK:
      ptr = gdbm_unpack_lock(&(curr->dat).lock, (char *)ALIGN(ptr));
      break;
    }
    ptr = gdbm_unpack_data((char *)&curr->flags, (char *)ALIGN(ptr),
      			   sizeof(int));

    hashval = attr_hash(curr->name);
    curr->next = ret->attributes[hashval];
    if(ret->attributes[hashval] == (struct attr *)NULL)
      curr->prev = curr;
    else {
      curr->prev = (ret->attributes[hashval])->prev;
      (ret->attributes[hashval])->prev = curr;
    }
    ret->attributes[hashval] = curr;

    attr_total--;
  }

  /* strings and such. */
  ptr = gdbm_unpack_string(&ret->name, (char *)ALIGN(ptr));

  DSC_DATA(main_index[obj]) = ret;
  DSC_FLAG2(main_index[obj]) |= IN_MEMORY;
  DSC_FLAG2(main_index[obj]) &= ~DIRTY;

  /* don't make any assumptions about free() */
  free(cont.dptr);
  return(ret);
}

void disk_delete(data)
    struct obj_data *data;
{
  key.dptr = (char *) &data->objnum;
  key.dsize = sizeof(int);

  if(gdbm_exists(gfd, key)) {
    if(gdbm_delete(gfd, key) != 0) {
      logfile(LOG_ERROR, "disk_delete: couldn't delete object #%d, %s.\n",
	      data->objnum, gdbm_strerror(gdbm_errno));
    }
  }
}
