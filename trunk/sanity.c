/* sanity.c */

#include "config.h"

/*
 *		       This file is part of TeenyMUD II.
 *		    Copyright(C) 1993, 1994 by Jason Downs.
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
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif			/* HAVE_STDLIB_H */
#include <varargs.h>

#include "conf.h"
#include "teeny.h"
#include "teenydb.h"
#include "gnu/getopt.h"
#include "externs.h"

#if defined(HAVE_DOPRNT) && !defined(HAVE_VPRINTF)
#define vfprintf(_s, _f, _a)    _doprnt(_f, _a, _s)
#endif

static char usage[] =
"Usage: %s -g <gdbm file> -d <db file>\n";
#define USAGE(s)	(fprintf(stderr, usage, s))

void check_location(object)
    int object;
{
int loc, list;

  if (get_int_elt(object, LOC, &loc) == -1) {
    printf("#%d has no location element.\n", object);
    return;
  }
  if (!exists_object(loc) || !has_contents(loc) ||
      (isroom(object) && !isroom(loc))) {
    printf("#%d has an invalid location (#%d).\n", object, loc);
    return;
  }
  if (object == loc) {
    printf("#%d has a looping location.\n", object);
    return;
  }
  switch (Typeof(object)) {
  case TYP_ROOM:
    if (get_int_elt(loc, ROOMS, &list) == -1) {
      printf("#%d has no rooms list.\n", loc);
      return;
    }
    break;
  case TYP_EXIT:
    if (get_int_elt(loc, EXITS, &list) == -1) {
      printf("#%d has no exits list.\n", loc);
      return;
    }
    break;
  default:
    if (get_int_elt(loc, CONTENTS, &list) == -1) {
      printf("#%d has no contents list.\n", loc);
      return;
    }
  }
  if (!member(object, list)) {
    printf("#%d has location #%d, but isn't in contents/exits/rooms list.\n",
	   object, loc);
    return;
  }
}
void check_contents(object)
    int object;
{
int list;
int depth = 10000;

  if (get_int_elt(object, CONTENTS, &list) == -1) {
    printf("#%d has no contents list.\n", object);
    return;
  }
  if (!has_contents(object) && (list != -1)) {
    printf("#%d shouldn't have a contents list, but does.\n", object);
    return;
  }
  while (list != -1) {
    if (isroom(list) || isexit(list)) {
      printf("#%d has #%d (a %s) in its contents list.\n", object, list,
	     isroom(list) ? "room" : "exit");
    }
    if (get_int_elt(list, NEXT, &list) == -1) {
      printf("#%d has no next reference.\n", list);
      return;
    }
    depth--;
    if (depth < 0) {
      printf("#%d has a looping contents list.\n", object);
      return;
    }
  }
}
void check_exits(object)
    int object;
{
int list;
int depth = 10000;

  if (get_int_elt(object, EXITS, &list) == -1) {
    printf("#%d has no exits list.\n", object);
    return;
  }
  if (!has_exits(object) && (list != -1)) {
    printf("#%d shouldn't have an exits list, but does.\n", object);
    return;
  }
  while (list != -1) {
    if (!isexit(list)) {
      printf("#%d has #%d (which isn't an exit) in its exits list.\n", object,
	     list);
    }
    if (get_int_elt(list, NEXT, &list) == -1) {
      printf("#%d has no next reference.\n", list);
      return;
    }
    depth--;
    if (depth < 0) {
      printf("#%d has a looping exits list.\n", object);
      return;
    }
  }
}
void check_rooms(object)
    int object;
{
int list;
int depth = 10000;

  if (has_rooms(object)) {
    if (get_int_elt(object, ROOMS, &list) == -1) {
      printf("#%d has no rooms list.\n", object);
      return;
    }
    while (list != -1) {
      if (!isroom(list)) {
	printf("#%d has #%d (which isn't a room) in its rooms list.\n", object,
	       list);
      }
      if (get_int_elt(list, NEXT, &list) == -1) {
	printf("#%d has no next reference.\n", list);
	return;
      }
      depth--;
      if (depth < 0) {
	printf("#%d has a looping rooms list.\n", object);
	return;
      }
    }
  }
}

void check_homedropto(object)
    int object;
{
int home;

  if (!has_home(object) && !has_dropto(object))
    return;

  if (get_int_elt(object, HOME, &home) == -1) {
    printf("#%d has no home/dropto element.\n", object);
    return;
  }
  if (((home != -1) && (home != -3) && !exists_object(home)) ||
      (exists_object(home) && isexit(home))) {
    printf("#%d has an invalid home/dropto (#%d).\n", object, home);
    return;
  }
  if ((home == -3) && !isroom(object) && !isexit(object)) {
    printf("#%d has an invalid home (-3).\n", object);
    return;
  }
  if (home == object) {
    printf("#%d has a looping home/dropto.\n", object);
    return;
  }
}
void check_owner(object)
    int object;
{
int owner;

  if (get_int_elt(object, OWNER, &owner) == -1) {
    printf("#%d has no owner element.\n", object);
    return;
  }
  if ((!exists_object(owner) || !isplayer(owner)) ||
      (isplayer(object) && !isplayer(owner))) {
    printf("#%d has an invalud owner (#%d).\n", object, owner);
    return;
  }
}
void check_parent(object)
    int object;
{
int parent;

  if (get_int_elt(object, PARENT, &parent) == -1) {
    printf("#%d has no parent element.\n", object);
    return;
  }
  if ((parent != -1) && !exists_object(parent)) {
    printf("#%d has an invalud parent (#%d).\n", object, parent);
    return;
  }
}
void check_name(object)
    int object;
{
char *name;

  if (get_str_elt(object, NAME, &name) == -1) {
    printf("#%d has no name element.\n", object);
    return;
  }
  if (!name || !*name) {
    printf("#%d has a null name.\n", object);
    return;
  }
}

void check_quota(object)
    int object;
{
int quota;

  if (get_int_elt(object, QUOTA, &quota) == -1) {
    printf("#%d has a bad quota element.\n", object);
    return;
  }
  if ((quota != 0) && !has_quota(object)) {
    printf("#%d has a quota of %d, but isn't a player.\n", object, quota);
    return;
  }
  if ((quota != STARTING_QUOTA) && has_quota(object))
    printf("#%d has a quota of %d objects.\n", object, quota);
}

void check_pennies(object)
    int object;
{
int pennies;

  if (has_pennies(object)) {
    if (get_int_elt(object, PENNIES, &pennies) == -1) {
      printf("#%d has a bad pennies element.\n", object);
      return;
    }
    if ((pennies < 0) || (pennies > mudconf.max_pennies))
      printf("#%d has %d pennies.\n", object, pennies);
  }
}

int main(argc, argv)
    int argc;
    char **argv;
{
int ch, obj, loc;
char *gdbm_file = (char *) 0;
char *db_file = (char *) 0;

extern char *optarg;
extern int opterr;

  opterr = 0;
  while ((ch = getopt(argc, argv, "g:d:")) != -1) {
    switch (ch) {
    case 'g':			/* gdbm */
      gdbm_file = optarg;
      break;
    case 'd':			/* db */
      db_file = optarg;
      break;
    default:
      USAGE(argv[0]);
      exit(-1);
    }
  }
  if ((gdbm_file == (char *) 0) || (db_file == (char *) 0)) {
    fputs("You must specify both -g and -d arguments.\n", stderr);
    USAGE(argv[0]);
    exit(-1);
  }
#ifdef SUNMALLOC
  init_malloc();
#endif				/* SUNMALLOC */
  cache_init(40960L, 0, 0);

  if (open_dbmfile(gdbm_file, TEENY_DBMFAST) != 0) {
    (void) fprintf(stderr, "Couldn't open gdbm file %s\n", gdbm_file);
    exit(-1);
  }
  if (database_read(db_file) != 0) {
    (void) fprintf(stderr, "Couldn't read database file %s\n", db_file);
    exit(-1);
  }
  init_ptable();

  /* do it to it */
  for (obj = 0; obj < mudstat.total_objects; obj++) {
    if (main_index[obj] == (struct dsc *) 0) {
      printf("#%d is garbage.\n", obj);
      continue;
    }
    check_location(obj);
    check_exits(obj);
    check_contents(obj);
    check_rooms(obj);
    check_homedropto(obj);	/* does home and dropto */
    check_owner(obj);
    check_parent(obj);
    check_quota(obj);
    check_pennies(obj);
    check_name(obj);

    DSC_FLAGS(main_index[obj]) &= ~MARKED;
    cache_trim();
  }

#if 0
  /* scan for rooms with no entrances */
  for (obj = 0; obj < mudstat.total_objects; obj++) {
    if ((main_index[obj] == (struct dsc *) 0) || !ExitP(main_index[obj]) ||
	(DSC_DESTINATION(main_index[obj]) < 0) ||
	(get_int_elt(obj, LOC, &loc) == -1) ||
	(loc == DSC_DESTINATION(main_index[obj])))
      continue;
    DSC_FLAGS(main_index[DSC_DESTINATION(main_index[obj])]) |= MARKED;
  }
  for (obj = 0; obj < mudstat.total_objects; obj++) {
    if ((main_index[obj] == (struct dsc *) 0) || !RoomP(main_index[obj]))
      continue;
    if (!(DSC_FLAGS(main_index[obj]) & MARKED))
      printf("#%d is a room with no entrances.\n", obj);
  }
#endif
  fflush(stdout);

  cache_flush();
  close_dbmfile();
  if (database_write(db_file) == -1)
    fputs("Error writing database file.\n", stderr);

  return (0);
}
/* extra routines */
/* VARARGS */
void log_error(va_alist)
    va_dcl
{
char *format;
va_list args;

  va_start(args);
format = (char *) va_arg(args, char *);

  (void) vfprintf(stderr, format, args);
  va_end(args);
}
/* VARARGS */
void panic(va_alist)
    va_dcl
{
char *format;
va_list args;

  va_start(args);
format = (char *) va_arg(args, char *);

  (void) vfprintf(stderr, format, args);
  va_end(args);
  exit(1);
}
