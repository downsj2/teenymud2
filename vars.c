/* vars.c */

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
#include <ctype.h>

#include "conf.h"
#include "teeny.h"
#include "externs.h"

#include "uthash/uthash.h"

/* 'variable' handlers / 'list' creation and handlers */

struct variable {
    char *name;
    char *value;

    UT_hash_handle hh;
};

struct variable *variable_hash = NULL;

char *var_get(owner, key)
    int owner;
    char *key;
{
  char sbuff[32], *buff;
  struct variable *entry;

  if((key == (char *)NULL) || (key[0] == '\0'))
    return((char *)NULL);

  snprintf(sbuff, sizeof(sbuff), "%d-", owner);
  buff = (char *)alloca(strlen(sbuff) + strlen(key) + 1);
  if(buff == (char *)NULL)
    panic("var_get: stack allocation failed.\n");
  strcpy(buff, sbuff);
  strcat(buff, key);

  HASH_FIND_STR(variable_hash, buff, entry);
  if(entry == NULL)
    return((char *)NULL);
  return(entry->value);
}

void var_delete(owner, key)
    int owner;
    char *key;
{
  char sbuff[32], *buff;
  struct variable *entry;

  if((key == (char *)NULL) || (key[0] == '\0'))
    return;

  snprintf(sbuff, sizeof(sbuff), "%d-", owner);
  buff = (char *)alloca(strlen(sbuff) + strlen(key) + 1);
  if(buff == (char *)NULL)
    panic("var_get: stack allocation failed.\n");
  strcpy(buff, sbuff);
  strcat(buff, key);

  HASH_FIND_STR(variable_hash, buff, entry);
  if(entry == NULL)
    return;
  HASH_DEL(variable_hash, entry);
  ty_free(entry->name);
  ty_free(entry->value);
  ty_free(entry);
}

void var_set(owner, key, value)
    int owner;
    char *key, *value;
{
  char sbuff[32], *buff;
  struct variable *entry;

  if((key == (char *)NULL) || (key[0] == '\0')
     || (value == (char *)NULL) || (value[0] == '\0'))
    return;

  snprintf(sbuff, sizeof(sbuff), "%d-", owner);
  buff = (char *)alloca(strlen(sbuff) + strlen(key) + 1);
  if(buff == (char *)NULL)
    panic("var_set: stack allocation failed.\n");
  strcpy(buff, sbuff);
  strcat(buff, key);

  entry = ty_malloc(sizeof(struct variable), "var_set");
  entry->name = ty_strdup(key, "var_set");
  entry->value = ty_strdup(value, "var_set");
  HASH_ADD_KEYPTR(hh, variable_hash, entry->name, strlen(entry->name), entry);
}

/* add an element to a list, doing proper quoting and all. */

static char slist_ret[BUFFSIZ];	/* elem size limit */

void slist_add(elem, buffer, bufsiz)
    char *elem, *buffer;
    int bufsiz;
{
  int quote = 0;
  int first = 0;
  char *ptr, *bptr;

  if((elem == (char *)NULL) || (buffer == (char *)NULL))
    return;

  /* is the buffer empty? */
  if((buffer[0] == '\0') && bufsiz) {
    strcpy(buffer, "{}");
    first++;
  } else if((buffer[0] == '{') && (buffer[1] == '}')) {
    first++;
  }
  bufsiz -= strlen(buffer);
  if(bufsiz <= 0)
    return;

  /* should we quote it? */
  ptr = elem;
  while(*ptr != '\0') {
    if(isspace(*ptr))
      quote++;

    ptr++;
  }

  /* add as much of the element in as we can. */
  bptr = &buffer[strlen(buffer)-1];	/* remove trailing brace */
  ptr = elem;

  /* insert any leading space and any needing quote. */
  if(!first) {
    *bptr++ = ' ';
    bufsiz--;
  }
  if(quote) {
    *bptr++ = '\"';
    bufsiz--;
  }

  while((*ptr != '\0') && (bufsiz > 2)
	 && ((ptr - elem) < sizeof(slist_ret))) {
    if((*ptr == '\"') || (*ptr == '{') || (*ptr == '}')
       || (*ptr == '[') || (*ptr == ']')) {
      *bptr++ = '\\';
      bufsiz--;
    }
    *bptr++ = *ptr++;
    bufsiz--;
  }
  if(quote) {
    *bptr++ = '\"';
    bufsiz--;
  }
  *bptr++ = '}';
  *bptr = '\0';
}

/* return the next (or first) element of a list. */
char *slist_next(list, nptr)
    char *list, **nptr;
{
  char *ptr, *bptr;

  if(list == (char *)NULL)
    return((char *)NULL);
  if(*nptr == (char *)NULL) {
    for(ptr = list; (*ptr != '\0') &&
	((*ptr == '{') || isspace(*ptr)); ptr++);
    if(*ptr == '\0')
      return((char *)NULL);
  } else
    ptr = *nptr;

  if((*ptr == '\0') || (*ptr == '}'))
    return((char *)NULL);

  while((*ptr != '\0') && isspace(*ptr))
    ptr++;
  if(*ptr == '\0')
    return((char *)NULL);

  bptr = slist_ret;
  if(*ptr == '\"') {	/* quoted element */
    /* eat leading quote. */
    ptr++;

    while((*ptr != '\0') && (*ptr != '\"') && (*ptr != '}')
    	  && ((bptr - slist_ret) < sizeof(slist_ret))) {
      if(*ptr == '\\') {
        ptr++;

	if(*ptr != '\0')
	  *bptr++ = *ptr++;
      } else
        *bptr++ = *ptr++;
    }

    if((bptr - slist_ret) >= sizeof(slist_ret)) {
      /* try to find the end of this element */
      while((*ptr != '\0') && (*ptr != '\"') && (ptr[-1] != '\\'))
	ptr++;
    }

    /* find the beginning of the next element. */
    if(*ptr == '\"')
      ptr++;
    while((*ptr != '\0') && (*ptr != '\"') && isspace(*ptr))
      ptr++;
  } else {
    while((*ptr != '\0') && !isspace(*ptr) && (*ptr != '}')
	  && ((bptr - slist_ret) < sizeof(slist_ret))) {
      if(*ptr == '\\') {
        ptr++;

        if(*ptr != '\0')
	  *bptr++ = *ptr++;
      } else
        *bptr++ = *ptr++;
    }

    if((bptr - slist_ret) >= sizeof(slist_ret)) {
      /* try to find the end of this element */
      while((*ptr != '\0') && !isspace(*ptr) && (ptr[-1] != '\\'))
	ptr++;
    }
  }
  *bptr = '\0';

  *nptr = ptr; /* point at terminating quote or space! */
  return(slist_ret);
}
