/* boolutils.c */

#include "config.h"

/*
 *		       This file is part of TeenyMUD II.
 *	    Copyright(C) 1995 by Jason Downs.  All rights reserved.
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
#include "externs.h"

#define ATTR_NAME 0
#define ATTR_CONT 1

/* Boolexp utility routines. */

static struct boolexp *boolexp_pool = (struct boolexp *)NULL;

struct boolexp *boolexp_alloc()
{
  struct boolexp *ret;

  if(boolexp_pool == (struct boolexp *)NULL) {
    int idx = 0;

    boolexp_pool = (struct boolexp *)ty_malloc(sizeof(struct boolexp) * 256,
    					       "boolexp_alloc.boolexp_pool");

    /* This loop is written like this for a *reason*. */
    while(idx < 255) {
      boolexp_pool[idx].sub1 = &boolexp_pool[idx + 1];
      idx++;
    }
    boolexp_pool[idx].sub1 = (struct boolexp *)NULL;
  }

  ret = boolexp_pool;
  boolexp_pool = boolexp_pool->sub1;

  bzero((VOID *)ret, sizeof(struct boolexp));
  return(ret);
}

void boolexp_free(b)
    struct boolexp *b;
{
  if (b != (struct boolexp *) NULL) {
    switch (b->type) {
    case BOOLEXP_AND:
    case BOOLEXP_OR:
      boolexp_free(b->sub2);
    case BOOLEXP_NOT:
      boolexp_free(b->sub1);
      break;
    case BOOLEXP_ATTR:
      ty_free((VOID *) (b->dat).atr[ATTR_NAME]);
      ty_free((VOID *) (b->dat).atr[ATTR_CONT]);
      break;
    }

    b->sub1 = boolexp_pool;
    boolexp_pool = b;
  }
}

struct boolexp *copy_boolexp(b)
    struct boolexp *b;
{                   
  struct boolexp *r;
  
  if (b == (struct boolexp *)NULL)
    return((struct boolexp *)NULL);

  r = boolexp_alloc();
  r->type = b->type;

  switch(b->type) {
  case BOOLEXP_AND:
  case BOOLEXP_OR: 
    r->sub2 = copy_boolexp(b->sub2);
  case BOOLEXP_NOT:          
    r->sub1 = copy_boolexp(b->sub1);
    break;
  case BOOLEXP_CONST:        
    (r->dat).thing = (b->dat).thing;
    break;
  case BOOLEXP_FLAG:
    bcopy((VOID *)((b->dat).flags), (VOID *)((r->dat).flags),
	  sizeof(int)*FLAGS_LEN);
    break;
  case BOOLEXP_ATTR:
    (r->dat).atr[ATTR_NAME] = (char *)ty_strdup((b->dat).atr[ATTR_NAME],
    						"copy_boolexp");
    (r->dat).atr[ATTR_CONT] = (char *)ty_strdup((b->dat).atr[ATTR_CONT],
    						"copy_boolexp");
    break;
  default:
    logfile(LOG_ERROR, "copy_boolexp: bad bool type (%d)\n", b->type);
    boolexp_free(r);
    return((struct boolexp *)NULL);
  }
  return(r);
}
