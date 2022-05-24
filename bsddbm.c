/* bsddbm.c */

#error Don't use this-- it hasn't been tested or updated.

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
#include <fcntl.h>
#ifdef HAVE_MALLOC_H
#include <malloc.h>
#endif			/* HAVE_MALLOC_H */
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif			/* HAVE_STDLIB_H */
#include <db.h>
#include <errno.h>

#include "conf.h"
#include "teeny.h"
#include "teenydb.h"
#include "externs.h"

/* TeenyMUD 4.4BSD database library interface. */

static DB *dbp = (DB *)NULL;
static DBT key, cont;
static int *obj_work = (int *)NULL;
static int work_siz = 0;
static int fast = 0;

static char *bsd_pack_data _ANSI_ARGS_((char *, char *, int));
static char *bsd_pack_lock _ANSI_ARGS_((char *, struct boolexp *));
static char *bsd_pack_string _ANSI_ARGS_((char *, char *));
static void bsd_emergency_growbuf _ANSI_ARGS_((void));
static char *bsd_unpack_data _ANSI_ARGS_((char *, char *, int));
static char *bsd_unpack_lock _ANSI_ARGS_((struct boolexp **, char *));
static char *bsd_unpack_string _ANSI_ARGS_((char **, char *));

#if SIZEOF_INT_P == SIZEOF_LONG
#define ALIGN(p)        (((long)p) % SIZEOF_LONG == 0 ? ((long)p) : \
                         ((long)p) + SIZEOF_LONG - ((long)p) % SIZEOF_LONG)
#elif SIZEOF_INT_P == SIZEOF_INT
#define ALIGN(p)        (((int)p) % SIZEOF_INT == 0 ? ((int)p) : \
                         ((int)p) + SIZEOF_INT - ((int)p) % SIZEOF_INT)
#else
#error You need an ALIGN macro.
#endif

int dbmfile_open(name, flags)
    char *name;
    int flags;
{
  HASHINFO hinfo;

  hinfo.bsize = 8128;		/* bucket size */
  hinfo.cachesize = 10240L;	/* small cache */
  hinfo.ffactor = 36;
  hinfo.hash = NULL;
  hinfo.lorder = 0;
  hinfo.nelem = mudstat.total_objects;	/* heh. */

  if((dbp = dbopen(name, O_RDWR, 0600, DB_HASH, &hinfo)) == NULL) {
    logfile(LOG_ERROR, "dbmfile_open: couldn't open %s, %s.\n", name,
	    strerror(errno));
    return(-1);
  }

  if (flags & TEENY_DBMFAST) {
    fast = 1;
  } else
    fast = 0;
  return(0);
}

int dbmfile_init(name, flags)
    char *name;
    int flags;
{
  HASHINFO hinfo;

  hinfo.bsize = 8128;           /* bucket size */
  hinfo.cachesize = 10240L;     /* small cache */
  hinfo.ffactor = 36;
  hinfo.hash = NULL;
  hinfo.lorder = 0;
  hinfo.nelem = mudstat.total_objects;  /* heh. */

  dbp = dbopen(name, O_RDWR|O_CREAT|O_TRUNC, 0600, DB_HASH, &hinfo);
  if(dbp == NULL) {
    logfile(LOG_ERROR, "dbmfile_init: couldn't open %s, %s\n", name,
	    strerror(errno));
    return(-1);
  }

  if (flags & TEENY_DBMFAST) {
    fast = 1;
  } else
    fast = 0;
  return(0);
}

void dbmfile_close()
{
  (dbp->close)(dbp);
}

static char *bsd_pack_data(dest, source, size)
    char *dest, *source;
    int size;
{
  bcopy((VOID *)source, (VOID *)dest, size);
  return((char *)(dest + size));
}

static char *bsd_pack_lock(dest, src)
    char *dest;
    struct boolexp *src;
{
  if(src == (struct boolexp *)NULL) {
    short type = BOOLEXP_END;		/* i just *had* to use short... */

    return(bsd_pack_data(dest, (char *)&type, sizeof(short)));
  }

  dest = bsd_pack_data(dest, (char *)&(src->type), sizeof(short));
  switch(src->type) {
  case BOOLEXP_AND:
  case BOOLEXP_OR:
    dest = bsd_pack_lock((char *)ALIGN(dest), src->sub2);
  case BOOLEXP_NOT:
    dest = bsd_pack_lock((char *)ALIGN(dest), src->sub1);
    break;
  case BOOLEXP_CONST:
    dest = bsd_pack_data((char *)ALIGN(dest), (char *)&((src->dat).thing),
    			  sizeof(int));
    break;
  case BOOLEXP_FLAG:
    dest = bsd_pack_data((char *)ALIGN(dest), (char *)((src->dat).flags),
    			  sizeof(int) * FLAGS_LEN);
    break;
  case BOOLEXP_ATTR:
    dest = bsd_pack_string((char *)ALIGN(dest), (src->dat).atr[0]);
    dest = bsd_pack_string((char *)ALIGN(dest), (src->dat).atr[1]);
    break;
  default:
    logfile(LOG_ERROR, "bsd_pack_lock: bad boolexp type (%d)\n", src->type);
    dest[-1] = BOOLEXP_END;
  }
  return(dest);
}

static char *bsd_pack_string(dest, src)
    char *dest, *src;
{
  int len;

  if(src == (char *)NULL){
    dest[0] = '\0';
    return((char *)(dest + 1));
  }

#ifdef notyet
  if(mudconf.enable_compress) {
    char *csrc;

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

static void bsd_emergency_growbuf()
{
  obj_work = (int *) ty_realloc(obj_work, work_siz + MEDBUFFSIZ,
  				"bsd_emergency_growbuf");
  work_siz += MEDBUFFSIZ;
}

int disk_freeze(data)
    struct obj_data *data;
{
  char *ptr;
  int *iptr;
  struct attr *attrs;
  int obj;
  int idx;

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
      ptr = bsd_pack_data((char *)ALIGN(ptr), (char *)&attrs->type,
      			  sizeof(short));
      ptr = bsd_pack_string((char *)ALIGN(ptr), attrs->name);
      if ((ptr - (char *)obj_work) > (work_siz - MEDBUFFSIZ))
        bsd_emergency_growbuf();
      switch(attrs->type) {
      case ATTR_STRING:
        ptr = bsd_pack_string((char *)ALIGN(ptr), (attrs->dat).str);
	break;
      case ATTR_LOCK:
        ptr = bsd_pack_lock((char *)ALIGN(ptr), (attrs->dat).lock);
	break;
      }
      ptr = bsd_pack_data((char *)ALIGN(ptr), (char *)&attrs->flags,
      			  sizeof(int));
      if ((ptr - (char *)obj_work) > (work_siz - MEDBUFFSIZ))
        bsd_emergency_growbuf();
    }
  }

  /* strings and such. */
  ptr = bsd_pack_string((char *)ALIGN(ptr), data->name);

  /* set it up. */
  key.data = (VOID *) &obj;
  key.size = sizeof(int);
  cont.data = (VOID *) obj_work;
  cont.size = ptr - (char *)obj_work;

  /* store it. */
  if((dbp->put)(dbp, &key, &cont, 0) != 0){
    logfile(LOG_ERROR, "disk_freeze: failed to store object #%d, %s.\n", obj,
    	    strerror(errno));
    mudstat.cache_errors++;
    return(-1);
  }

  /* sync it. */
  if(!fast) {
    if((dbp->sync)(dbp, 0) == -1) {
      logfile(LOG_ERROR, "disk_freeze: failed to sync the database.\n");
    }
  }

  /* all done! */
  DSC_FLAG2(main_index[obj]) &= ~IN_MEMORY;
  free_obj(data);

  return(0);
}

static char *bsd_unpack_data(dest, source, size)
    char *dest, *source;
    int size;
{
  bcopy((VOID *)source, (VOID *)dest, size);
  return((char *)(source + size));
}

static char *bsd_unpack_lock(dest, source)
    struct boolexp **dest;
    char *source;
{
  short type;

  source = bsd_unpack_data((char *)&type, source, sizeof(short));
  switch(type) {
  case BOOLEXP_END:
    *dest = (struct boolexp *)NULL;
    return(source);
  case BOOLEXP_AND:
  case BOOLEXP_OR:
    *dest = boolexp_alloc();
    (*dest)->type = type;

    source = bsd_unpack_lock(&(*dest)->sub2, (char *)ALIGN(source));
    return(bsd_unpack_lock(&(*dest)->sub1, (char *)ALIGN(source)));
  case BOOLEXP_NOT:
    *dest = boolexp_alloc();
    (*dest)->type = type;

    return(bsd_unpack_lock(&(*dest)->sub1, (char *)ALIGN(source)));
  case BOOLEXP_CONST:
    *dest = boolexp_alloc();
    (*dest)->type = type;

    return(bsd_unpack_data((char *)&((*dest)->dat).thing,
    	   (char *)ALIGN(source), sizeof(int)));
  case BOOLEXP_FLAG:
    *dest = boolexp_alloc();
    (*dest)->type = type;

    return(bsd_unpack_data((char *)((*dest)->dat).flags, (char *)ALIGN(source),
    	   sizeof(int) * FLAGS_LEN));
  case BOOLEXP_ATTR:
    *dest = boolexp_alloc();
    (*dest)->type = type;

    source = bsd_unpack_string(&(((*dest)->dat).atr[0]),
    				(char *)ALIGN(source));
    return(bsd_unpack_string(&(((*dest)->dat).atr[1]), (char *)ALIGN(source)));
  default:
    logfile(LOG_ERROR, "bsd_unpack_lock: bad boolexp type (%c)\n", *source);
    *dest = (struct boolexp *)NULL;
    return(source);
  }
}

static char *bsd_unpack_string(dest, source)
    char **dest, *source;
{
  char *ptr = source;

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
  *dest = ty_strdup(source, "bsddbm_unpack_string.ret");
#endif
  return(ptr);
}

struct obj_data *disk_thaw(obj)
    int obj;
{
  struct obj_data *ret;
  int *iptr, *data;
  char *ptr;
  int attr_total, hashval;
  struct attr *curr;

  if (DSC_FLAG2(main_index[obj]) & IN_MEMORY) {
    logfile(LOG_ERROR, "disk_thaw: attempt to re-thaw thawed object #%d\n",
	    obj);
    return (DSC_DATA(main_index[obj]));
  }

  /* set it up. */
  key.data = (VOID *) &obj;
  key.size = sizeof(int);

  /* fetch it. */
  if((dbp->get)(dbp, &key, &cont, 0) != 0) {
    logfile(LOG_ERROR, "disk_thaw: couldn't fetch object #%d, %s.\n", obj,
    	    strerror(errno));
    return((struct obj_data *)NULL);
  }

  ret = (struct obj_data *) ty_malloc(sizeof(struct obj_data),
  					"disk_thaw.ret");
  ret->objnum = obj;

  /* this is lame. */
  if(cont.size <= work_siz) {
    data = obj_work;
  } else {
    data = (int *)ty_malloc(cont.size, "disk_thaw.data");
  }
  /* bsd does not align it's return data. */
  bcopy((VOID *)cont.data, (VOID *)data, cont.size);

  iptr = data;
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
  ptr = (char *)iptr;

  /* attributes. */
  bzero((VOID *)ret->attributes, sizeof(struct attr *) * ATTR_WIDTH);
  ptr = bsd_unpack_data((char *)&ret->attr_total, ptr, sizeof(int));
  attr_total = ret->attr_total;

  while(attr_total) {
    curr = (struct attr *)ty_malloc(sizeof(struct attr), "disk_thaw.curr");

    ptr = bsd_unpack_data((char *)&curr->type, (char *)ALIGN(ptr),
    			  sizeof(short));
    ptr = bsd_unpack_string(&curr->name, (char *)ALIGN(ptr));
    switch(curr->type) {
    case ATTR_STRING:
      ptr = bsd_unpack_string(&(curr->dat).str, (char *)ALIGN(ptr));
      break;
    case ATTR_LOCK:
      ptr = bsd_unpack_lock(&(curr->dat).lock, (char *)ALIGN(ptr));
      break;
    }
    ptr = bsd_unpack_data((char *)&curr->flags, (char *)ALIGN(ptr),
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
  ptr = bsd_unpack_string(&ret->name, (char *)ALIGN(ptr));

  DSC_DATA(main_index[obj]) = ret;
  DSC_FLAG2(main_index[obj]) |= IN_MEMORY;
  DSC_FLAG2(main_index[obj]) &= ~DIRTY;

  if(data != obj_work)
    ty_free((VOID *)data);
  return(ret);
}

void disk_delete(data)
    struct obj_data *data;
{
  key.data = (VOID *) &data->objnum;
  key.size = sizeof(int);

  if((dbp->del)(dbp, &key, 0) != 0) {
    logfile(LOG_ERROR, "disk_delete: couldn't delete object #%d, %s.\n",
	    data->objnum, strerror(errno));
  }
}
