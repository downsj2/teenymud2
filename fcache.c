/* fcache.c */

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
#include <sys/stat.h>
#ifdef HAVE_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif                  /* HAVE_STRING_H */

#include "conf.h"
#include "fcache.h"
#include "teeny.h"
#include "externs.h"

#include "hash/hash.h"

/* file cache -- cache text files in memory, allowing for line-by-line ret. */

static Hash_Table CacheTable;
static int CacheInited = 0;

static FCACHE *fcache_load _ANSI_ARGS_((char *));
static void fcache_free _ANSI_ARGS_((FCACHE *));

FCACHE *fcache_open(fname, sptr)
    char *fname;
    off_t *sptr;
{
  FHEAD *ret;
  Hash_Entry *hp;
  struct stat sbuf;
  int new;

  if((fname == (char *)NULL) || (fname[0] == '\0')) {
    *sptr = -1;
    return((FCACHE *)NULL);
  }

  if(!CacheInited) {
    Hash_InitTable(&CacheTable, 0, HASH_STRING_KEYS);
    CacheInited++;
  }

  if(stat(fname, &sbuf) != 0) {
    *sptr = -1;
    return((FCACHE *)NULL);
  }

  hp = Hash_CreateEntry(&CacheTable, fname, &new);
  if (new) {
    ret = (FHEAD *)ty_malloc(sizeof(FHEAD), "fcache_open");
    ret->stamp = sbuf.st_mtime;
    ret->lines = fcache_load(fname);

    Hash_SetValue(hp, (int *)ret);
  } else {
    ret = (FHEAD *)Hash_GetValue(hp);
    if(sbuf.st_mtime > ret->stamp) {
      fcache_free(ret->lines);
      ret->lines = fcache_load(fname);
      ret->stamp = sbuf.st_mtime;

      Hash_SetValue(hp, (int *)ret);
    }
  }

  *sptr = sbuf.st_size;
  return(ret->lines);
}

static FCACHE *fcache_load(fname)
    char *fname;
{
  FILE *fp;
  FCACHE *new, *ptr;
  char buffer[128];

  fp = fopen(fname, "r");
  if(fp == (FILE *)NULL)
    return((FCACHE *)NULL);

  for(new = (FCACHE *)NULL, ptr = (FCACHE *)NULL;
      !feof(fp) && (fgets(buffer, 128, fp) != (char *)NULL);) {
    if(new == (FCACHE *)NULL) { /* first line */
      new = (FCACHE *)ty_malloc(sizeof(FCACHE), "fcache_open");
      new->next = (FCACHE *)NULL;
      ptr = new;
    } else {
      ptr->next = (FCACHE *)ty_malloc(sizeof(FCACHE), "fcache_open");
      ptr = ptr->next;
      ptr->next = (FCACHE *)NULL;
    }

    remove_newline(buffer);
    if(buffer[0] == '\0') {
      ptr->str[0] = buffer[0];
      ptr->ptr = ptr->str;
    } else {
      ptr->line = (char *)ty_strdup(buffer, "fcache_open");
      ptr->ptr = ptr->line;
    }
  }
  fclose(fp);
  return(new);
}

static void fcache_free(ptr)
    FCACHE *ptr;
{
  FCACHE *next;

  while(ptr != (FCACHE *)NULL) {
    next = ptr->next;

    if(ptr->ptr == ptr->line)
      ty_free((VOID *)ptr->line);
    ty_free((VOID *)ptr);
    
    ptr = next;
  }
}
