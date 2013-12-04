/* misc.c */

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
      if (set_time_elt(obj, TIMESTAMP, mudstat.now) == -1)
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
      if (set_time_elt(obj, TIMESTAMP, mudstat.now) == -1)
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
    if (set_time_elt(obj, CREATESTAMP, mudstat.now) == -1)
      logfile(LOG_ERROR, "stamp: failed to update object #%d\n", obj);
    break;
  }
}

/* assuming argument is a pathname, return last component */
char *ty_basename(pname)
    char *pname;
{
  char *ptr;

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
#if defined(__STDC__)
char to_lower(char c)
#else
char to_lower(c)
    char c;
#endif
{
  int i;

  for(i = 0; i < 26; i++) {
    if(alphatab[i].upcase == c)
      return(alphatab[i].lowcase);
  }
  return (c);
}

/* fast, safe, ANSI compliant toupper() */
#if defined(__STDC__)
char to_upper(char c)
#else
char to_upper(c)
    char c;
#endif
{
  int i;

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

#ifdef __STDC__
int match_flaglist_name(const char *str, FlagList flaglist[], int count)
#else
int match_flaglist_name(str, flaglist, count)
    const char *str;
    FlagList flaglist[];
    int count;
#endif
{
  int idx;

  for (idx = 0; idx < count; idx++) {
    if(strncasecmp(str, flaglist[idx].name, flaglist[idx].matlen) == 0)
      return(idx);
  }
  return(-1);
}

#ifdef __STDC__
int match_flaglist_chr(const char chr, FlagList flaglist[], int count)
#else
int match_flaglist_chr(chr, flaglist, count)
    const char chr;
    FlagList flaglist[];
    int count;
#endif
{
  int idx;

  for (idx = 0; idx < count; idx++) {
    if(flaglist[idx].chr == chr)
      return(idx);
  }
  return(-1);
}

void parse_flags(str, ret)
    char *str;
    int *ret;
{
  int idx, matched;

  /* Special key(s) for no flags. */
  if((strcmp(str, "0") == 0) || (strcasecmp(str, "none") == 0)) {
    ret[0] = 0;
    ret[1] = 0;
    return;
  }

  while (*str) {
    matched = 0;

    /* GenFlags */
    idx = match_flaglist_chr(*str, GenFlags, GenFlags_count);
    if (idx >= 0) {
      ret[GenFlags[idx].word] |= GenFlags[idx].code;
      matched = 1;
    }

    /* PlayerFlags */
    idx = match_flaglist_chr(*str, PlayerFlags, PlayerFlags_count);
    if (idx >= 0) {
      ret[PlayerFlags[idx].word] |= PlayerFlags[idx].code;
      matched = 1;
    }

    /* RoomFlags */
    idx = match_flaglist_chr(*str, RoomFlags, RoomFlags_count);
    if (idx >= 0) {
      ret[RoomFlags[idx].word] |= RoomFlags[idx].code;
      matched = 1;
    }

    /* ThingFlags */
    idx = match_flaglist_chr(*str, ThingFlags, ThingFlags_count);
    if (idx >= 0) {
      ret[ThingFlags[idx].word] |= ThingFlags[idx].code;
      matched = 1;
    }

    /* ExitFlags */
    idx = match_flaglist_chr(*str, ExitFlags, ExitFlags_count);
    if (idx >= 0) {
      ret[ExitFlags[idx].word] |= ExitFlags[idx].code;
      matched = 1;
    }

    if (!matched) {
      ret[0] = -1;
      ret[1] = -1;
      return;
    }
    str++;
  }
}

void parse_time(str, ptr)
    char *str;
    time_t *ptr;
{
  char *sptr;

  *ptr = (time_t)strtol(str, &sptr, 10);
  switch(*sptr) {
  case 'm':
    *ptr = (*ptr) * 60;
    break;
  case 'h':
    *ptr = (*ptr) * 3600;
    break;
  case 'd':
    *ptr = (*ptr) * 86400;
    break;
  }
}

/* parse a string until you hit a slash, stripping whitespace. */
/* string gets destroyed. */
char *parse_slash(first, second)
    char *first, **second;
{
  char *ptr, *optr;

  if((first == (char *)NULL) || (first[0] == '\0'))
    return((char *)NULL);

  for(ptr = first; *ptr && (*ptr != '/'); ptr++);
  if(*ptr == '/') {
    optr = ptr;
    *ptr++ = '\0';

    /* skip whitespace. */
    while(*ptr && isspace(*ptr))
      ptr++;
    *second = ptr;

    /* eat trailing whitespace. */
    while((optr > first) && isspace(optr[-1]))
      optr--;
    if(isspace(*optr))
      *optr = '\0';
  } else
    *second = (char *)NULL;
  return(first);
}

/* parse a string until you hit delim, following standard quoting rules. */
/* destroys string, and returns a pointer to the character after delim. */
#if defined(__STDC__)
char *parse_to(char *string, char sdelim, char delim)
#else
char *parse_to(string, sdelim, delim)
    char *string;
    char sdelim, delim;
#endif
{
  int depth = 1;

  while(*string && depth) {
    if (*string == '\\' && string[1]) {
      string++;
    } else if(*string == sdelim) {
      depth++;
    } else if(*string == delim) {
      depth--;
    }
    if (depth)
      string++;
  }
  if (*string) {
    *string++ = '\0';
    return(string);
  } else
    return((char *)NULL);
}

int stringprefix(str, prefix)
    char *str, *prefix;
{
  int count = 0;

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
static bool malloc_inited = 0;

#ifdef MEMTRACKING
static void ty_malloc_report()
{
  struct mmem *curr;

  for(curr = mudstat.memlist; curr != (struct mmem *)NULL; curr = curr->next) {
    logfile(LOG_STATUS, "ty_malloc_report: %d bytes still malloc'd from %s.\n",
	    curr->msize, curr->mfunc);
  }
  logfile(LOG_STATUS, "ty_malloc_report: %ld bytes total still malloc'd.\n",
	  mudstat.memtotal);
}
#endif			/* MEMTRACKING */

void ty_malloc_init()
{
  if(!malloc_inited) {
#if defined(sun) && defined(M_MXFAST) && defined(HAVE_MALLOPT)
    mallopt(M_MXFAST, 128);
    mallopt(M_NLBLKS, 50);
    mallopt(M_GRAIN, 16);
#endif
#ifdef MEMTRACKING
    mudstat.memlist = (struct mmem *)NULL;
    mudstat.memtotal = 0;

    /* some C libraries (i.e., GNU) have both on_exit() and atexit(). */
#if defined(MALLOC_DEBUG) && defined(HAVE_ON_EXIT) && !defined(HAVE_ATEXIT)
    on_exit(ty_malloc_report, NULL);
#endif
#if defined(MALLOC_DEBUG) && defined(HAVE_ATEXIT)
    atexit(ty_malloc_report);
#endif
#endif			/* MEMTRACKING */
    malloc_inited = TRUE;
  }
}

VOID *ty_malloc(size, where)
    int size;
    char *where;
{
  VOID *ret;
#ifdef MEMTRACKING
  mmem *newmem;
#endif			/* MEMTRACKING */

  if (!malloc_inited)
    ty_malloc_init();

  if ((where == (char *)NULL) || (where[0] == '\0'))
    where = "unknown";

  if (size <= 0) {
    logfile(LOG_ERROR, "ty_malloc: got a %d size request from %s.\n", size,
	    where);
    return ((VOID *) NULL);
  }
  ret = (VOID *)malloc((size_t)size);
  if (ret == (VOID *)NULL) {
    panic("%s: malloc() failed (%d bytes)\n", where, size);
  }

#ifdef MEMTRACKING
  newmem = (struct mmem *)malloc(sizeof(struct mmem));
  if (newmem == (struct mmem *)NULL) {
    panic("%s: malloc() failed (%d bytes)\n", where, size);
  }
  newmem->mfunc = (char *)malloc(srtlen(where)+1);
  if (newmem->mfunc == (char *)NULL) {
    panic("%s: malloc() failed (%d bytes)\n", where, size);
  }

  newmem->msize = size;
  newmem->mptr = ret;
  strcpy(newmem->mfunc, where);
  newmem->next = mudstat->memlist;
  mudstat->memlist = newmem;
  mudstat->memtotal += size;
#if MEMTRACKING > 1
  logfile(LOG_STATUS, "ty_malloc: allocation of %d bytes for %s, %x\n", size,
          where, ret);
#endif
#endif			/* MEMTRACKING */

  return (ret);
}

VOID *ty_realloc(ptr, size, where)
    VOID *ptr;
    int size;
    char *where;
{
  VOID *ret;
#ifndef __STDC__
  VOID *realloc();
#endif
#ifdef MEMTRACKING
  struct mmem *curr;
#endif			/* MEMTRACKING */

  if(!malloc_inited)
    ty_malloc_init();

  if((where == (char *)NULL) || (where[0] == '\0'))
    where = "unknown";

#ifdef MEMTRACKING
  /* This is *slow* */
  for(curr = mudstat.memlist; curr != (struct mmem *)NULL; curr = curr->next) {
    if(curr->mptr == ptr)
      break;
  }
  if(curr != (struct mmem *)NULL) {
#if MEMTRACKING > 1
    logfile(LOG_STATUS,
	    "ty_realloc: realloc of %d bytes, was %d bytes from %s\n",
	    size, curr->size, curr->mfunc);
#endif
    mudstat.memtotal -= curr->msize;
    mudstat.memtotal += size;
    curr->msize = size;
    if(curr->mfunc != (char *)NULL)
      free((VOID *)curr->mfunc);
    curr->mfunc = (char *)malloc(strlen(where) + 1);
    if(curr->mfunc == (char *)NULL) {
      panic("%s: malloc() failed (%d bytes)\n", where, size);
    }
    strcpy(curr->mfunc, where);
#endif			/* MEMTRACKING */

  ret = realloc(ptr, size);
  if(ret == (VOID *)NULL) {
    panic("%s: realloc() failed (%d bytes)\n", where, size);
  }
  return(ret);
}

void ty_free(ptr)
    VOID *ptr;
{
#ifdef MEMTRACKING
  struct mmem *curr, *prev;
#endif			/* MEMTRACKING */

  if (ptr == (VOID *)NULL)
    return;			/* Ok */

  if(!malloc_inited)
    ty_malloc_init();

#ifdef MEMTRACKING
  /* This is *slow*. */
  for(prev = mudstat.memlist, curr = mudstat.memlist;
      curr != (struct mmem *)NULL; prev = curr, curr = curr->next) {
    if(curr->mptr == ptr)
      break;
  }
  if(curr != (struct mmem *)NULL) {
    /* Unlink and free. */
    mudstat.memtotal -= curr->msize;
    prev->next = curr->next;

    if(curr->mfunc != (chr *)NULL)
      free((VOID *)curr->mfunc);
    free((VOID *)curr);
  }
#endif			/* MEMTRACKING */
  free(ptr);
}

char *ty_strdup(str, where)
    char *str, *where;
{
  char *ret = (char *)NULL;
  int idx, len;

  if (str != (char *)NULL) {
    len = strlen(str) + 1;
    ret = (char *) ty_malloc(len, where);
    for (idx = 0; idx < len; idx++)
      ret[idx] = str[idx];
  }
  return (ret);
}

/* copy a file, using the stdio library. */
/* returns -1 on failure, or 0 upon success. */
void copy_file(source, dest)
    char *source, *dest;
{
  FILE *sfp, *dfp;
  char buf[512];
  int ret;

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
