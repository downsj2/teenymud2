/* misc.c */

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

/* AIX requires this to be the first thing in the file. */
#ifdef __GNUC__
#define alloca	__builtin_alloca
#else	/* not __GNUC__ */
#ifdef HAVE_ALLOCA_H
#include <alloca.h>
#else	/* not HAVE_ALLOCA_H */
#ifdef _AIX
 #pragma alloca
#endif	/* not _AIX */
#endif	/* not HAVE_ALLOCA_H */
#endif 	/* not __GNUC__ */

#include <stdio.h>
#include <sys/types.h>
#ifdef HAVE_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif			/* HAVE_STRING_H */
#ifdef HAVE_MALLOC_H
#include <malloc.h>
#endif				/* HAVE_MALLOC_H */
#include <ctype.h>
#include <time.h>
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif			/* HAVE_STDLIB_H */

#include "conf.h"
#include "flaglist.h"
#include "teeny.h"
#include "externs.h"

/* Some generic utility functions for TeenyMUD */

/* Write a timestamp on an object. */

void stamp(obj, code)
    int obj, code;
{
  int uses;

  switch (code) {
  case STAMP_USED:
    /* case STAMP_MODIFIED: */
    if (!isPLAYER(obj)) {
      if (set_int_elt(obj, TIMESTAMP, (int) mudstat.now) == -1)
	logfile(LOG_ERROR, "stamp: failed to update object #%d\n", obj);
      if (get_int_elt(obj, USES, &uses) != -1) {
	uses++;
	if (set_int_elt(obj, USES, uses) == -1)
	  logfile(LOG_ERROR, "stamp: couldn't update use count of object #%d\n",
	      	  obj);
      }
    }
    break;
  case STAMP_LOGIN:
    if (isPLAYER(obj)) {
      if (set_int_elt(obj, TIMESTAMP, (int) mudstat.now) == -1)
	logfile(LOG_ERROR, "stamp: failed to update object #%d\n", obj);
      if (get_int_elt(obj, USES, &uses) != -1) {
	uses++;
	if (set_int_elt(obj, USES, uses) == -1)
	  logfile(LOG_ERROR, "stamp: couldn't update use count of object #%d\n",
	          obj);
      }
    }
    break;
  case STAMP_CREATED:
    if (set_int_elt(obj, CREATESTAMP, (int) mudstat.now) == -1)
      logfile(LOG_ERROR, "stamp: failed to update object #%d\n", obj);
    break;
  }
}

/* assuming argument is a pathname, return last component */
char *ty_basename(pname)
    char *pname;
{
  register char *ptr;

  if(pname != (char *)NULL) {
    ptr = &pname[strlen(pname)];

    while((*ptr != '/') && (ptr > pname))
      ptr--;
    if(*ptr == '/')
      ptr++;

    return(ptr);
  } else
    return(pname);
}

struct alphatable {
  char upcase;
  char lowcase;
};

static struct alphatable alphatab[26] = {
  {'A', 'a'}, {'B', 'b'}, {'C', 'c'}, {'D', 'd'}, {'E', 'e'},
  {'F', 'f'}, {'G', 'g'}, {'H', 'h'}, {'I', 'i'}, {'J', 'j'},
  {'K', 'k'}, {'L', 'l'}, {'M', 'm'}, {'N', 'n'}, {'O', 'o'},
  {'P', 'p'}, {'Q', 'q'}, {'R', 'r'}, {'S', 's'}, {'T', 't'},
  {'U', 'u'}, {'V', 'v'}, {'W', 'w'}, {'X', 'x'}, {'Y', 'y'},
  {'Z', 'z'}
};

/* fast, safe, ANSI compliant tolower() */
char to_lower(c)
    register char c;
{
  register short i;

  for(i = 0; i < 26; i++) {
    if(alphatab[i].upcase == c)
      return(alphatab[i].lowcase);
  }
  return (c);
}

/* fast, safe, ANSI compliant toupper() */
char to_upper(c)
    register char c;
{
  register short i;

  for(i = 0; i < 26; i++) {
    if(alphatab[i].lowcase == c)
      return(alphatab[i].upcase);
  }
  return (c);
}

/* checks to see if object is near player. */
int nearby(player, object)
    int player, object;
{
  int ploc, oloc;

  if(!exists_object(player) || !exists_object(object))
    return(0);
  if((get_int_elt(object, LOC, &oloc) == -1) || !exists_object(oloc))
    return(0);
  if(oloc == player)
    return(1);
  if((get_int_elt(player, LOC, &ploc) == -1) || !exists_object(ploc))
    return(0);
  return((ploc == oloc) || (object == ploc));
}

/* checks to see if object is in the list */
int member(object, list)
    int object, list;
{
  while (list != -1) {
    if (list == object)
      return (1);
    if (get_int_elt(list, NEXT, &list) == -1) {
      logfile(LOG_ERROR, "member: bad next reference on object #%d\n", list);
      return (0);
    }
  }
  return (0);
}

/* drop something from a list, my style. */
void list_drop(what, where, code)
    int what, where, code;
{
  int list, prev, next;

  if(!is_list(code)){
    logfile(LOG_ERROR, "list_drop: passed a bad list code (%d)\n", code);
    return;
  }

  if(get_int_elt(where, code, &list) == -1) {
    logfile(LOG_ERROR, "list_drop: bad list on object #%d\n", where);
    return;
  }

  for(prev = -1; list != -1; prev = list, list = next) {
    if(get_int_elt(list, NEXT, &next) == -1) {
      logfile(LOG_ERROR, "list_drop: bad next reference on object #%d\n", list);
      return;
    }
    if(list == what) {		/* got it */
      if(prev == -1) {		/* head of list */
	if(set_int_elt(where, code, next) == -1) {
	  logfile(LOG_ERROR, "list_drop: couldn't set list on object #%d\n",
	      	  where);
	  return;
	}
      } else {
	if(set_int_elt(prev, NEXT, next) == -1) {
	  logfile(LOG_ERROR, "list_drop: couldn't set next on object #%d\n",
		  prev);
	  return;
	}
      }
      break;			/* done */
    }
  }
}

/* add an object to a list, my style. */
void list_add(what, where, code)
    int what, where, code;
{
  int list;

  if(!is_list(code)){
    logfile(LOG_ERROR, "list_add: passed a bad list code (%d)\n", code);
    return;
  }

  if(get_int_elt(where, code, &list) == -1){
    logfile(LOG_ERROR, "list_add: bad list on object #%d\n", where);
    return;
  }
  if(member(what, list))
    return;

  if(set_int_elt(what, NEXT, list) == -1){
    logfile(LOG_ERROR, "list_add: couldn't set next on object #%d\n", what);
    return;
  }
  if(set_int_elt(where, code, what) == -1){
    logfile(LOG_ERROR, "list_add: couldn't set list on object #%d\n", where);
    return;
  }
}

void parse_flags(str, ret)
    char *str;
    int *ret;
{
  register FlagList *flags;
  register int matched;

  /* Special key for no flags. */
  if(strcmp(str, "0") == 0) {
    ret[0] = 0;
    ret[1] = 0;
    return;
  }

  while (*str) {
    matched = 0;

    for (flags = GenFlags; flags->name; flags++) {
      if (flags->chr == *str) {
	ret[flags->word] |= flags->code;
	matched = 1;
	break;
      }
    }
    for (flags = PlayerFlags; flags->name; flags++) {
      if(flags->chr == *str) {
        ret[flags->word] |= flags->code;
	matched = 1;
	break;
      }
    }
    for (flags = RoomFlags; flags->name; flags++) {
      if(flags->chr == *str) {
        ret[flags->word] |= flags->code;
	matched = 1;
	break;
      }
    }
    for (flags = ThingFlags; flags->name; flags++) {
      if(flags->chr == *str) {
        ret[flags->word] |= flags->code;
	matched = 1;
	break;
      }
    }
    for (flags = ExitFlags; flags->name; flags++) {
      if(flags->chr == *str) {
        ret[flags->word] |= flags->code;
	matched = 1;
	break;
      }
    }

    if (!matched) {
      ret[0] = -1;
      ret[1] = -1;
      return;
    }
    str++;
  }
}

/* Returns non-NULL if one string is contained within the other. */
char *strcasestr(str, key)
    register char *str, *key;
{
  register char *curr = key;
  register char *holder = str;

  while (str && *str && *curr) {
    if (to_lower(*str) == to_lower(*curr))
      curr++;
    else {
      curr = key;
      holder = str + 1;
    }
    str++;
  }
  if (*curr == '\0')
    return (holder);
  else
    return ((char *) NULL);
}

int stringprefix(str, prefix)
    register char *str, *prefix;
{
  register int count = 0;

  while(*str && *prefix && (to_lower(*prefix) == to_lower(*str))) {
    count++;
    str++;
    prefix++;
  }
  if(*prefix == '\0')
    return(count);
  return(0);
}

/* malloc stuff */

#ifdef MALLOC_DEBUG
#include "hash/hash.h"

static Hash_Table MallTable;
#endif				/* MALLOC_DEBUG */

void ty_malloc_init()
{
#ifdef MALLOC_DEBUG
  Hash_InitTable(&MallTable, 0, HASH_ONE_WORD_KEYS);
#endif				/* MALLOC_DEBUG */
#if defined(sun) && defined(M_MXFAST)
  mallopt(M_MXFAST, 128);
  mallopt(M_NLBLKS, 50);
  mallopt(M_GRAIN, 16);
#endif
}

VOID *ty_malloc(size, where)
    int size;
    char *where;
{
  register VOID *ret;
#ifdef MALLOC_DEBUG
  register Hash_Entry *entry;
  int new;
  register char *str;
#endif				/* MALLOC_DEBUG */

  if (size <= 0) {
    logfile(LOG_ERROR, "ty_malloc: got a %d size request from %s.\n", size,
	    where);
    return ((VOID *) NULL);
  }
  ret = (VOID *)malloc((size_t)size);
  if (ret == (VOID *)NULL) {
    panic("%s: malloc() failed (%d bytes)\n", where, size);
  }

#ifdef MALLOC_DEBUG
  entry = Hash_CreateEntry(&MallTable, ret, &new);
  if(!new)
    logfile(LOG_ERROR, "ty_malloc: got a duplicate pointer (%x)\n", ret);
  /* don't want to use ty_malloc() */
  str = (char *)malloc(strlen(where) + 1 + sizeof(int));
  if(str == (char *)NULL) {
    panic("%s: malloc() failed (%d bytes)\n", where,
	  strlen(where) + 1 + sizeof(int));
  }
  ((int *)str)[0] = size;
  strcpy(&str[sizeof(int)], where);
  Hash_SetValue(entry, str);
#if MALLOC_DEBUG > 1
  logfile(LOG_STATUS, "ty_malloc: allocation of %d bytes for %s, %x\n", size,
          where, ret);
#endif
#endif				/* MALLOC_DEBUG */

  return (ret);
}

VOID *ty_realloc(ptr, size, where)
    VOID *ptr;
    int size;
    char *where;
{
  register VOID *ret;
#ifndef __STDC__
  VOID *realloc();
#endif
#ifdef MALLOC_DEBUG
  register Hash_Entry *entry;
  register char *str;

  entry = Hash_FindEntry(&MallTable, ptr);
  if(entry == (Hash_Entry *)NULL)
    logfile(LOG_ERROR, "ty_realloc: got an unknown pointer (%x) from %s\n", ptr,
	    where);
  else {
    str = (char *)Hash_GetValue(entry);
#if MALLOC_DEBUG > 1
    logfile(LOG_STATUS,
	    "ty_realloc: realloc of %d bytes, was %d bytes from %s\n",
	    size, ((int *)str)[0], &str[sizeof(int)]);
#endif
    ((int *)str)[0] = size;
  }
#endif				/* MALLOC_DEBUG */

  ret = realloc(ptr, size);
  if(ret == (VOID *)NULL)
    panic("%s: realloc() failed (%d bytes)\n", where, size);
  return(ret);
}

void ty_free(ptr)
    VOID *ptr;
{
#ifdef MALLOC_DEBUG
  register Hash_Entry *entry;
  register char *str;
#endif				/* MALLOC_DEBUG */

  if (ptr == (VOID *)NULL)
    return;			/* Ok */

#ifdef MALLOC_DEBUG
  entry = Hash_FindEntry(&MallTable, ptr);
  if(entry == (Hash_Entry *)NULL)
    logfile(LOG_ERROR, "ty_free: got an unknown pointer (%x)\n", ptr);
  else {
    str = (char *)Hash_GetValue(entry);
#if MALLOC_DEBUG > 1
    logfile(LOG_STATUS, "ty_free: free of %d bytes (%x) allocated from %s\n",
	    ((int *)str)[0], ptr, &str[sizeof(int)]);
#endif
    free(str);
    Hash_DeleteEntry(&MallTable, entry);
  }
#endif				/* MALLOC_DEBUG */

  free(ptr);
}

#ifdef MALLOC_DEBUG
RETSIGTYPE report_malloc()
{
  register Hash_Entry *entry;
  Hash_Search search;
  register char *str;

  for(entry = Hash_EnumFirst(&MallTable, &search);
      entry != (Hash_Entry *)NULL;
      entry = Hash_EnumNext(&search)) {
    str = (char *)Hash_GetValue(entry);

    logfile(LOG_STATUS, "report_malloc: %d bytes still allocated from %s\n",
	    ((int *)str)[0], &str[sizeof(int)]);
    free(str);
  }
  Hash_DeleteTable(&MallTable);
}
#endif				/* MALLOC_DEBUG */

char *ty_strdup(str, where)
    char *str, *where;
{
  register char *ret = (char *)NULL;
  register int idx, len;

  if (str != (char *)NULL) {
    len = strlen(str) + 1;
    ret = (char *) ty_malloc(len, where);
    for (idx = 0; idx < len; idx++)
      ret[idx] = str[idx];
  }
  return (ret);
}

/* strncpy(), my way. */
void ty_strncpy(dest, source, len)
    char dest[], *source;
    int len;
{
  register int i;

  for(i = 0; (i < len) && (source[i] != '\0'); i++) {
    dest[i] = source[i];
  }
  dest[i] = '\0';
}

/* copy a file, using the stdio library. */
/* returns -1 on failure, or 0 upon success. */
void copy_file(source, dest)
    char *source, *dest;
{
  FILE *sfp, *dfp;
  char buf[512];
  register int ret;

  if((sfp = fopen(source, "r")) == (FILE *)NULL) {
    logfile(LOG_ERROR, "copy_file: couldn't open %s for read.\n", source);
    return;
  }
  if((dfp = fopen(dest, "w")) == (FILE *)NULL) {
    logfile(LOG_ERROR, "copy_file: couldn't open %s for write.\n", dest);
    return;
  }

  while(!feof(sfp)) {
    if(((ret = fread(buf, sizeof(char), 512, sfp)) < 512) && !feof(sfp)) {
      logfile(LOG_ERROR, "copy_file: failed read from %s.\n", source);
      goto copy_done;
    }
    if(fwrite(buf, sizeof(char), ret, dfp) < ret) {
      logfile(LOG_ERROR, "copy_file: failed write to %s.\n", dest);
      goto copy_done;
    }
  }

copy_done:

  fclose(sfp);
  fclose(dfp);
}
