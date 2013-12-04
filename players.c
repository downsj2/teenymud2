/* players.c */

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

#include "conf.h"
#include "teeny.h"
#include "teenydb.h"
#include "externs.h"

#include "hash/hash.h"

/* internal player name lookups */

static Hash_Table phash_num;
static Hash_Table phash_name;
static int phash_inited = 0;

void ptable_add(num, name)
    int num;
    char *name;
{
  register Hash_Entry *new1, *new2;

  if (name == (char *) NULL)
    return;			/* fuck you, too */

  if (!phash_inited) {
    Hash_InitTable(&phash_name, 0, HASH_STRCASE_KEYS);
    Hash_InitTable(&phash_num, 0, HASH_ONE_WORD_KEYS);
    phash_inited = 1;
  }

  /* add to name table */
  new1 = Hash_CreateEntry(&phash_name, name, NULL);
  Hash_SetValue(new1, (int *) num);

  /* add to num table */
  new2 = Hash_CreateEntry(&phash_num, (char *) num, NULL);
  Hash_SetValue(new2, (int *) Hash_GetKey(&phash_name, new1));
}

void ptable_delete(num)
    int num;
{
  register Hash_Entry *entry1, *entry2;
  register char *name;

  if (!phash_inited)
    return;

  if ((entry1 = Hash_FindEntry(&phash_num, (char *) num))) {
    name = (char *)Hash_GetValue(entry1);
    if ((entry2 = Hash_FindEntry(&phash_name, name)))
      Hash_DeleteEntry(&phash_name, entry2);
    Hash_DeleteEntry(&phash_num, entry1);
  }
}

char *ptable_lookup(num)
    int num;
{
  register Hash_Entry *entry;

  if (phash_inited) {
    if ((entry = Hash_FindEntry(&phash_num, (char *) num)))
      return ((char *) Hash_GetValue(entry));
  }
  return ((char *) NULL);
}

int match_player(name)
    char *name;
{
  register Hash_Entry *entry;

  if (phash_inited) {
    if ((entry = Hash_FindEntry(&phash_name, name)))
      return ((int) Hash_GetValue(entry));
  }
  return(-1);
}

void ptable_init()
{
  register struct obj_data *obj;
  register int i;
  register int total;

  if (phash_inited)
    return;

  for (i = 0, total = 0; i < mudstat.total_objects; i++) {
    if (main_index[i] == (struct dsc *) NULL)
      continue;

    if (DSC_TYPE(main_index[i]) == TYP_PLAYER)
      total++;
  }
  if (total == 0)		/* BUH? */
    return;

  Hash_InitTable(&phash_name, total, HASH_STRCASE_KEYS);
  Hash_InitTable(&phash_num, total, HASH_ONE_WORD_KEYS);
  phash_inited = 1;
  
  for (i = 0; i < mudstat.total_objects; i++) {
    if (main_index[i] == (struct dsc *) NULL)
      continue;

    if (DSC_TYPE(main_index[i]) == TYP_PLAYER) {
      if ((obj = lookup_obj(i)) != (struct obj_data *) NULL) {
	ptable_add(i, obj->name);
	cache_trim();
      }
    }
  }
}
