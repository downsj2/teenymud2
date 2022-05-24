/* ptable.c */

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

/* AIX requires this to be the first thing in the file. */
#ifdef __GNUC__
#define alloca  __builtin_alloca
#else   /* not __GNUC__ */
#ifdef HAVE_ALLOCA_H
#include <alloca.h>
#else   /* not HAVE_ALLOCA_H */
#ifdef _AIX
 #pragma alloca
#endif  /* not _AIX */
#endif  /* not HAVE_ALLOCA_H */
#endif  /* not __GNUC__ */

#include <stdio.h>
#include <sys/types.h>

#include "conf.h"
#include "teeny.h"
#include "teenydb.h"
#include "ptable.h"
#include "externs.h"

#include "uthash/uthash.h"

/* Internal player name and alias lookups. */

struct ptable {
  int objnum;
  char *name; /* lower case key */
  char *alias; /* lower case key */
  char *rname; /* original name */

  UT_hash_handle hh1, hh2, hh3; /* hh1: objnum, hh2: name, hh3: alias */
};
static struct ptable *ptable_hash_objnum = NULL;
static struct ptable *ptable_hash_name = NULL;
static struct ptable *ptable_hash_alias = NULL;

/* Add just the name and number to the hash tables. */
void ptable_add(num, pname)
    int num;
    char *pname;
{
  struct ptable *new;
  char *name, *p, *q;

  if ((pname == (char *) NULL) || !pname[0])
    return;

  /* Check to see if we already have it. */
  HASH_FIND(hh1, ptable_hash_objnum, &num, sizeof(int), new);
  if (new)
    return;	/* Don't do anything. */

  /* Lower case copy. */
  name = alloca(strlen(pname) + 1);
  if (name == NULL)
    panic("ptable_add(): stack allocation failed.\n");
  for(p = pname, q = name; *p; *q++ = to_lower(*p++));
  *q = '\0';

  new = (struct ptable *)ty_malloc(sizeof(struct ptable), "ptable_add");
  bzero(new, sizeof(struct ptable));

  new->objnum = num;
  new->name = ty_strdup(name, "ptable_add");
  new->rname = ty_strdup(pname, "ptable_add");  /* Leave alias NULL for now. */
  
  /* Add to the tables. */
  HASH_ADD(hh1, ptable_hash_objnum, objnum, sizeof(int), new);
  HASH_ADD_KEYPTR(hh2, ptable_hash_name, new->name, strlen(new->name), new);
}

/* Add to the alias hash table. */
#if defined(__STDC__)
void ptable_alias(int num, char *palias, enum palias_flags flag)
#else
void ptable_alias(num, alias, flag)
    int num;
    char *palias;
    enum palias_flags flag;
#endif
{
  struct ptable *hp;
  char *alias, *p, *q;

  /* Find the existing struct. */
  HASH_FIND(hh1, ptable_hash_objnum, &num, sizeof(int), hp);
  if (!hp) {	/* Bite me. */
    logfile(LOG_ERROR, "ptable_alias: Alias called on non-hashed object #%d\n", num);
    return;
  }

  /* Lower case copy. */
  alias = alloca(strlen(palias) + 1);
  if (alias == NULL)
    panic("ptable_alias(): stack allocation failed.\n");
  for(p = palias, q = alias; *p; *q++ = to_lower(*p++));
  *q = '\0';

  if(flag == PTABLE_UNALIAS) {
    /* Just remove it if it's here. */
    HASH_FIND(hh3, ptable_hash_alias, alias, strlen(alias), hp);
    if (!hp) {
      logfile(LOG_ERROR, "ptable_alias: Attempt to delete a non-aliased object #%d\n", num);
      return;
    }

    HASH_DELETE(hh3, ptable_hash_alias, hp);
    ty_free(hp->alias);
    hp->alias = NULL;
  } else {
    /* Add it in. */
    hp->alias = ty_strdup(alias, "ptable_alias");
    HASH_ADD_KEYPTR(hh3, ptable_hash_alias, hp->alias, strlen(hp->alias), hp);
  }
}

void ptable_delete(num)
    int num;
{
  struct ptable *hp;

  /* Find it. */
  HASH_FIND(hh1, ptable_hash_objnum, &num, sizeof(int), hp);
  if (!hp)
    return;

  /* Delete it from all the hashes. */
  HASH_DELETE(hh1, ptable_hash_objnum, hp);
  HASH_DELETE(hh2, ptable_hash_name, hp);
  if (hp->alias) {
    HASH_DELETE(hh3, ptable_hash_alias, hp);
    ty_free(hp->alias);
  }
  ty_free(hp->name);
  ty_free(hp->rname);
  ty_free(hp);
}

/* Lookup a player name by number. */
char *ptable_lookup(num)
    int num;
{
  struct ptable *hp;

  HASH_FIND(hh1, ptable_hash_objnum, &num, sizeof(int), hp);
  if (hp)
    return(hp->rname);
  return ((char *) NULL);
}

/* Lookup a player by name or alias. */
int match_player(pname)
    char *pname;
{
  struct ptable *hp;
  char *name, *p, *q;

  /* Lower case copy. */
  name = alloca(strlen(pname) + 1);
  if (name == NULL)
    panic("match_player(): stack allocation failed.\n");
  for(p = pname, q = name; *p; *q++ = to_lower(*p++));
  *q = '\0';

  HASH_FIND(hh2, ptable_hash_name, name, strlen(name), hp);
  if (hp)
    return(hp->objnum);
  HASH_FIND(hh3, ptable_hash_alias, name, strlen(name), hp);
  if (hp)
    return(hp->objnum);
  return(-1); /* No match. */
}

/* Initialize all of this crap. */
void ptable_init()
{
  struct obj_data *obj;
  int i;
  int aflags;
  char *alias;

  if (ptable_hash_objnum) /* Make sure this only runs once. */
    return;

  /* Just loop through all of the players, calling the above routines. */
  for (i = 0; i < mudstat.total_objects; i++) {
    if (main_index[i] == (struct dsc *) NULL)
      continue;

    if (DSC_TYPE(main_index[i]) == TYP_PLAYER) {
      if ((obj = lookup_obj(i)) != (struct obj_data *) NULL) {
	ptable_add(i, obj->name);

        if(attr_get(i, ALIAS, &alias, &aflags) != -1) {
	  if((alias != (char *)NULL) && alias[0])
	    ptable_alias(i, alias, 0);
	}
      }
    }
  }
  cache_trim();  /* Trim it at the end. */
}
