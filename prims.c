/* prims.c */

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
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#include <time.h>
#include <ctype.h>
#include <math.h>
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif			/* HAVE_STDLIB_H */

#include "conf.h"
#include "teeny.h"
#include "prims.h"
#include "ptable.h"
#include "sha/sha_wrap.h"
#include "externs.h"

static const char *errors[] = {
  "#-1 ILLEGAL OBJECT", "#-1 DATABASE ERROR",
  "#-1 PERMISSION DENIED", "#-1 TOO FEW ARGUMENTS",
  "#-1 DIVIDE BY ZERO", "#-1 VARIABLE NAME TOO LONG",
  "#-1 NO SUCH VARIABLE", "#-1 NUMERICAL ARGUMENT EXPECTED",
  "#-1 OBJECT ARGUMENT EXPECTED", "#-1 TOO MANY ARGUMENTS"
};
#define ERR_ILLOBJ	errors[0]
#define ERR_DB		errors[1]
#define ERR_PERM	errors[2]
#define ERR_FEWARGS	errors[3]
#define ERR_DIVZERO	errors[4]
#define ERR_VARLONG	errors[5]
#define ERR_NOVAR	errors[6]
#define ERR_NUM		errors[7]
#define ERR_OBJ		errors[8]
#define ERR_TOOARGS	errors[9]

/* local util prototypes. */
static int strtonum _ANSI_ARGS_((char *, char **));
static void fval _ANSI_ARGS_((char *, double));

/* local utility functions. */

static int strtonum(str, ptr)
    char *str, **ptr;
{
  int ret;

  if((strlen(str) > 1) && (str[0] == '#')) {
    str++;
    ret = (int)strtol(str, ptr, 10);
    if(*ptr == str) {	/* return the old value. */
      *ptr = &str[-1];
      return(0);
    }
    return(ret);
  }
  *ptr = str;
  return(0);
}

static void fval(buffer, val)
    char *buffer;
    double val;
{
  char *ptr;

  if (val == 0.0)
    strcpy(buffer, "0");
  else {
    sprintf(buffer, "%.6f", val);

    for(ptr = buffer; (*ptr != '\0') && (*ptr != '.'); ptr++);
    if(*ptr == '.') {
      while((*ptr != '\0') && (*ptr != '0'))
        ptr++;
      if(*ptr == '0') {
	if(ptr[-1] == '.')
	  ptr--;
        *ptr = '\0';
      }
    }
  }
}

/* primitives. */

static PRIM(prim_getattr)
{
  int obj;
  char *attr, *ptr;
  int aflags, asource;

  obj = strtonum(argv[1], &ptr);
  if(ptr == argv[1]) {
    strcpy(buffer, ERR_OBJ);
    return;
  }
  if(!exists_object(obj)) {
    strcpy(buffer, ERR_ILLOBJ);
    return;
  }
  if (attr_get_parent(obj, argv[2], &attr, &aflags, &asource) == -1) {
    strcpy(buffer, ERR_DB);
    return;
  }
  if(!can_see_attr(player, cause, obj, aflags)) {
    strcpy(buffer, ERR_PERM);
    return;
  }
  if (attr == (char *)NULL)
    buffer[0] = '\0';
  else
    strlcpy(buffer, attr, MEDBUFFSIZ);
}

static PRIM(prim_getlock)
{
  int obj;
  struct boolexp *lock;
  char *ptr;
  int aflags, asource;

  obj = strtonum(argv[1], &ptr);
  if(ptr == argv[1]) {
    strcpy(buffer, ERR_OBJ);
    return;
  }
  if(!exists_object(obj)) {
    strcpy(buffer, ERR_ILLOBJ);
    return;
  }
  if(attr_getlock_parent(obj, argv[2], &lock, &aflags, &asource) == -1) {
    strcpy(buffer, ERR_DB);
    return;
  }
  if(!can_see_attr(player, cause, obj, aflags)) {
    strcpy(buffer, ERR_PERM);
    return;
  }
  if(lock == (struct boolexp *)NULL)
    buffer[0] = '\0';
  else
    strlcpy(buffer, unparse_boolexp(player, cause, lock), MEDBUFFSIZ);
}

static PRIM(prim_contents)
{
  int obj;
  int list;
  int count = 0;
  char *ptr, sbuff[32];

  obj = strtonum(argv[1], &ptr);
  if(ptr == argv[1]) {
    strcpy(buffer, ERR_OBJ);
    return;
  }
  if(!exists_object(obj) || !has_contents(obj)){
    strcpy(buffer, ERR_ILLOBJ);
    return;
  }
  if(!controls(player, cause, obj) && !isVISUAL(obj) && !nearby(player, obj)
     &&	(cause != obj)) {
    strcpy(buffer, ERR_PERM);
    return;
  }
  if (get_int_elt(obj, CONTENTS, &list) == -1) {
    strcpy(buffer, ERR_DB);
    return;
  }

  if(list == -1) {
    strcpy(buffer, "{}");	/* fake out the list */
    return;
  }
  while(list != -1) {
    if ((list == player) || can_see(player, cause, list)) {
      snprintf(sbuff, sizeof(sbuff), "#%d", list);
      slist_add(sbuff, buffer, MEDBUFFSIZ);

      count++;
    }

    if (get_int_elt(list, NEXT, &list) == -1) {
      strcpy(buffer, ERR_DB);
      return;
    }
  }
  if(!count)
    strcpy(buffer, "{}");	/* fake out the list */
}

static PRIM(prim_exits)
{
  int obj;
  int list;
  int count = 0;
  char *ptr, sbuff[32];

  obj = strtonum(argv[1], &ptr);
  if(ptr == argv[1]) {
    strcpy(buffer, ERR_OBJ);
    return;
  }
  if(!exists_object(obj) || !has_exits(obj)){
    strcpy(buffer, ERR_ILLOBJ);
    return;
  }
  if(!controls(player, cause, obj) && !isVISUAL(obj) && !nearby(player, obj)
     &&	(cause != obj)){
    strcpy(buffer, ERR_PERM);
    return;
  }
  if (get_int_elt(obj, EXITS, &list) == -1) {
    strcpy(buffer, ERR_DB);
    return;
  }

  if(list == -1) {
    strcpy(buffer, "{}");	/* fake out the list */
    return;
  }
  while(list != -1) {
    if(isOBVIOUS(list) || controls(player, cause, list)) {
      snprintf(sbuff, sizeof(sbuff), "#%d", list);
      slist_add(sbuff, buffer, MEDBUFFSIZ);

      count++;
    }
    if(get_int_elt(list, NEXT, &list) == -1) {
      strcpy(buffer, ERR_DB);
      return;
    }
  }
  if(!count)
    strcpy(buffer, "{}");	/* fake out the list */
}

static PRIM(prim_rooms)
{
  int obj;
  int list;
  int count = 0;
  char *ptr, sbuff[32];

  obj = strtonum(argv[1], &ptr);
  if(ptr == argv[1]) {
    strcpy(buffer, ERR_OBJ);
    return;
  }
  if(!exists_object(obj) || !has_rooms(obj)){
    strcpy(buffer, ERR_ILLOBJ);
    return;
  }
  if(!isABODE(obj) && !controls(player, cause, obj)){
    strcpy(buffer, ERR_PERM);
    return;
  }
  if (get_int_elt(obj, ROOMS, &list) == -1) {
    strcpy(buffer, ERR_DB);
    return;
  }

  if(list == -1) {
    strcpy(buffer, "{}");	/* fake out the list */
    return;
  }
  while(list != -1) {
    if(isABODE(list) || controls(player, cause, list)) {
      snprintf(sbuff, sizeof(sbuff), "#%d", list);
      slist_add(sbuff, buffer, MEDBUFFSIZ);

      count++;
    }
    if(get_int_elt(list, NEXT, &list) == -1) {
      strcpy(buffer, ERR_DB);
      return;
    }
  }
  if(!count)
    strcpy(buffer, "{}");	/* fake out the list */
}

static PRIM(prim_owner)
{
  int obj;
  int dat;
  char *ptr;

  obj = strtonum(argv[1], &ptr);
  if(ptr == argv[1]){
    strcpy(buffer, ERR_OBJ);
    return;
  }
  if(!exists_object(obj)) {
    strcpy(buffer, ERR_ILLOBJ);
    return;
  }
  if(get_int_elt(obj, OWNER, &dat) == -1){
    strcpy(buffer, ERR_DB);
    return;
  }
  sprintf(buffer, "#%d", dat);
}

static PRIM(prim_quota)
{
  int obj;
  int dat;
  char *ptr;

  if (mudconf.enable_quota) {
    obj = strtonum(argv[1], &ptr);
    if(ptr == argv[1]) {
      strcpy(buffer, ERR_OBJ);
      return;
    }
    if(!exists_object(obj) || !has_quota(obj)) {
      strcpy(buffer, ERR_ILLOBJ);
      return;
    }
    if(!controls(player, cause, obj) && !isVISUAL(obj)) {
      strcpy(buffer, ERR_PERM);
      return;
    }
    if(get_int_elt(obj, QUOTA, &dat) == -1){
      strcpy(buffer, ERR_DB);
      return;
    }
    sprintf(buffer, "%d", dat);
  } else
    strcpy(buffer, "#-1 QUOTA NOT SUPPORTED");
}

static PRIM(prim_pennies)
{
  int obj;
  int dat;
  char *ptr;

  if(mudconf.enable_money) {
    obj = strtonum(argv[1], &ptr);
    if(ptr == argv[1]) {
      strcpy(buffer, ERR_OBJ);
      return;
    }
    if(!exists_object(obj) || !has_pennies(obj)){
      strcpy(buffer, ERR_ILLOBJ);
      return;
    }
    if(!controls(player, cause, obj) && !isVISUAL(obj)) {
      strcpy(buffer, ERR_PERM);
      return;
    }
    if(get_int_elt(obj, PENNIES, &dat) == -1){
      strcpy(buffer, ERR_DB);
      return;
    }
    sprintf(buffer, "%d", dat);
  } else
    strcpy(buffer, "#-1 PENNIES NOT SUPPORTED");
}

static PRIM(prim_loc)
{
  int obj;
  int dat;
  char *ptr;

  obj = strtonum(argv[1], &ptr);
  if(ptr == argv[1]){
    strcpy(buffer, ERR_OBJ);
    return;
  }
  if(!exists_object(obj)) {
    strcpy(buffer, ERR_ILLOBJ);
    return;
  }
  if(isHIDDEN(obj) && !controls(player, cause, obj) && !nearby(player, obj)) {
    strcpy(buffer, ERR_PERM);
    return;
  }
  if(get_int_elt(obj, LOC, &dat) == -1){
    strcpy(buffer, ERR_DB);
    return;
  }
  sprintf(buffer, "#%d", dat);
}

static PRIM(prim_rloc)
{
  int i;
  int levels, obj, prev;
  char *ptr;

  obj = strtonum(argv[1], &ptr);
  if(ptr == argv[1]){
    strcpy(buffer, ERR_OBJ);
    return;
  }
  if(!exists_object(obj)) {
    strcpy(buffer, ERR_ILLOBJ);
    return;
  }
  levels = (int)strtol(argv[2], &ptr, 10);
  if(ptr == argv[2]){
    strcpy(buffer, ERR_NUM);
    return;
  }
  if(levels > mudconf.room_depth)	/* seems logical enough... */
    levels = mudconf.room_depth;

  prev = -1;
  if(!isHIDDEN(obj) || controls(player, cause, obj) || nearby(player, obj)){
    for(i = 0; i < levels; i++) {
      if(!exists_object(obj) || isROOM(obj))
	break;
      prev = obj;
      if(get_int_elt(prev, LOC, &obj) == -1){
	strcpy(buffer, ERR_DB);
	return;
      }
    }
  }
  sprintf(buffer, "#%d", prev);
}

static PRIM(prim_controls)
{
  int obj1, obj2;
  char *ptr;

  obj1 = strtonum(argv[1], &ptr);
  if(ptr == argv[1]) {
    strcpy(buffer, ERR_OBJ);
    return;
  }
  obj2 = strtonum(argv[2], &ptr);
  if(ptr == argv[2]) {
    strcpy(buffer, ERR_OBJ);
    return;
  }
  if(!exists_object(obj1) || !exists_object(obj2)){
    strcpy(buffer, ERR_ILLOBJ);
    return;
  }

  sprintf(buffer, "%d", controls(obj1, cause, obj2));
}

static PRIM(prim_dropto)
{
  int obj;
  int dropto;
  char *ptr;

  obj = strtonum(argv[1], &ptr);
  if(ptr == argv[1]) {
    strcpy(buffer, ERR_OBJ);
    return;
  }
  if(!exists_object(obj) || !has_dropto(obj)) {
    strcpy(buffer, ERR_ILLOBJ);
    return;
  }
  if(!controls(player, cause, obj) && !isVISUAL(obj)) {
    strcpy(buffer, ERR_PERM);
    return;
  }
  if(get_int_elt(obj, DROPTO, &dropto) == -1) {
    strcpy(buffer, ERR_DB);
    return;
  }
  sprintf(buffer, "#%d", dropto);
}

static PRIM(prim_home)
{
  int obj;
  int home;
  char *ptr;

  obj = strtonum(argv[1], &ptr);
  if(ptr == argv[1]) {
    strcpy(buffer, ERR_OBJ);
    return;
  }
  if(!exists_object(obj) || !has_home(obj)) {
    strcpy(buffer, ERR_ILLOBJ);
    return;
  }
  if(!controls(player, cause, obj) && !isVISUAL(obj)) {
    strcpy(buffer, ERR_PERM);
    return;
  }
  if(get_int_elt(obj, HOME, &home) == -1) {
    strcpy(buffer, ERR_DB);
    return;
  }
  sprintf(buffer, "#%d", home);
}

static PRIM(prim_dests)
{
  int obj, indx;
  int count = 0;
  int *dests;
  char *ptr, sbuff[32];

  obj = strtonum(argv[1], &ptr);
  if(ptr == argv[1]){
    strcpy(buffer, ERR_OBJ);
    return;
  }
  if(!exists_object(obj) || !has_dests(obj)) {
    strcpy(buffer, ERR_ILLOBJ);
    return;
  }
  if(!controls(player, cause, obj) && !isVISUAL(obj)) {
    strcpy(buffer, ERR_PERM);
    return;
  }
  if(get_array_elt(obj, DESTS, &dests) == -1){
    strcpy(buffer, ERR_DB);
    return;
  }

  if (dests != (int *)NULL) {
    for(indx = 1; indx <= dests[0]; indx++) {
      snprintf(sbuff, sizeof(sbuff), "#%d", dests[indx]);
      slist_add(sbuff, buffer, MEDBUFFSIZ);

      count++;
    }
  }
  if(!count)
    strcpy(buffer, "{}");	/* fake out the list */
}

static PRIM(prim_timestamp)
{
  int obj;
  time_t dat;
  char *ptr;

  obj = strtonum(argv[1], &ptr);
  if(ptr == argv[1]){
    strcpy(buffer, ERR_OBJ);
    return;
  }
  if(!exists_object(obj)){
    strcpy(buffer, ERR_ILLOBJ);
    return;
  }
  if(!controls(player, cause, obj) && !isVISUAL(obj)) {
    strcpy(buffer, ERR_PERM);
    return;
  }
  if(get_time_elt(obj, TIMESTAMP, &dat) == -1){
    strcpy(buffer, ERR_DB);
    return;
  }
#if SIZEOF_TIME_T > SIZEOF_LONG
  sprintf(buffer, "%lld", dat);
#elif SIZEOF_TIME_T == SIZEOF_LONG
  sprintf(buffer, "%ld", dat);
#else	/* Good luck */
  sprintf(buffer, "%d", dat);
#endif
}

static PRIM(prim_createstamp)
{
  int obj;
  time_t dat;
  char *ptr;

  obj = strtonum(argv[1], &ptr);
  if(ptr == argv[1]){
    strcpy(buffer, ERR_OBJ);
    return;
  }
  if(!exists_object(obj)){
    strcpy(buffer, ERR_ILLOBJ);
    return;
  }
  if(!controls(player, cause, obj) && !isVISUAL(obj)) {
    strcpy(buffer, ERR_PERM);
    return;
  }
  if(get_time_elt(obj, CREATESTAMP, &dat) == -1){
    strcpy(buffer, ERR_DB);
    return;
  }
#if SIZEOF_TIME_T > SIZEOF_LONG
  sprintf(buffer, "%lld", dat);
#elif SIZEOF_TIME_T == SIZEOF_LONG
  sprintf(buffer, "%ld", dat);
#else	/* Good luck */
  sprintf(buffer, "%d", dat);
#endif
}

static PRIM(prim_parent)
{
  int obj;
  int dat;
  char *ptr;

  obj = strtonum(argv[1], &ptr);
  if(ptr == argv[1]){
    strcpy(buffer, ERR_OBJ);
    return;
  }
  if(!exists_object(obj)){
    strcpy(buffer, ERR_ILLOBJ);
    return;
  }
  if(!controls(player, cause, obj) && !isVISUAL(obj)) {
    strcpy(buffer, ERR_PERM);
    return;
  }
  if(get_int_elt(obj, PARENT, &dat) == -1){
    strcpy(buffer, ERR_DB);
    return;
  }
  sprintf(buffer, "#%d", dat);
}

static PRIM(prim_uses)
{
  int obj;
  int dat;
  char *ptr;

  obj = strtonum(argv[1], &ptr);
  if(ptr == argv[1]){
    strcpy(buffer, ERR_OBJ);
    return;
  }
  if(!exists_object(obj)){
    strcpy(buffer, ERR_ILLOBJ);
    return;
  }
  if(!controls(player, cause, obj) && !isVISUAL(obj)) {
    strcpy(buffer, ERR_PERM);
    return;
  }
  if(get_int_elt(obj, USES, &dat) == -1){
    strcpy(buffer, ERR_DB);
    return;
  }
  sprintf(buffer, "%d", dat);
}

static PRIM(prim_name)
{
  int obj;
  char *name, *ptr;

  obj = strtonum(argv[1], &ptr);
  if(ptr == argv[1]){
    strcpy(buffer, ERR_OBJ);
    return;
  }
  if(!exists_object(obj)){
    strcpy(buffer, ERR_ILLOBJ);
    return;
  }
  if(get_str_elt(obj, NAME, &name) == -1) {
    strcpy(buffer, ERR_DB);
    return;
  }
  strcpy(buffer, (name == (char *)NULL) ? "" : name);
}

static PRIM(prim_flags)
{
  int obj;
  int flags[FLAGS_LEN];
  char *ptr;

  obj = strtonum(argv[1], &ptr);
  if(ptr == argv[1]){
    strcpy(buffer, ERR_OBJ);
    return;
  }
  if(!exists_object(obj)) {
    strcpy(buffer, ERR_ILLOBJ);
    return;
  }

  if(get_flags_elt(obj, FLAGS, flags) == -1) {
    strcpy(buffer, ERR_DB);
    return;
  }
  strcpy(buffer, display_objflags(flags, controls(player, cause, obj)));
}

static PRIM(prim_type)
{
  int obj;
  char *ptr;

  obj = strtonum(argv[1], &ptr);
  if(ptr == argv[1]) {
    strcpy(buffer, ERR_OBJ);
    return;
  }
  if(!exists_object(obj)) {
    strcpy(buffer, ERR_ILLOBJ);
    return;
  }

  switch(Typeof(obj)) {
  case TYP_PLAYER:
    strcpy(buffer, "Player");
    break;
  case TYP_ROOM:
    strcpy(buffer, "Room");
    break;
  case TYP_EXIT:
    strcpy(buffer, "Exit");
    break;
  case TYP_THING:
    strcpy(buffer, "Thing");
    break;
  default:
    strcpy(buffer, ERR_ILLOBJ);
  }
}

static PRIM(prim_match)
{
  int obj;

  obj = resolve_object(player, cause, argv[1], RSLV_EXITS);
  if ((obj == -1) || (obj == -2)) {
    strcpy(buffer, "#-1 NO MATCH");
    return;
  }
  sprintf(buffer, "#%d", obj);
}

static PRIM(prim_pmatch)
{
  int obj;

  if (strcasecmp(argv[1], "me") == 0)
    obj = player;
  else {
    obj = match_active_player(argv[1]);
    if (!exists_object(obj))
      obj = match_player(argv[1]);
    if (!exists_object(obj)) {
      strcpy(buffer, "#-1 NO MATCH");
      return;
    }
  }
  sprintf(buffer, "#%d", obj);
}

static PRIM(prim_wholist)
{
  who_strlist(buffer, MEDBUFFSIZ);
}

static PRIM(prim_rand)
{
  int num;
  char *ptr;

  num = (int)strtol(argv[1], &ptr, 10);
  if(ptr == argv[1]) {
    strcpy(buffer, ERR_NUM);
  } else {
    if(num <= 0)
      strcpy(buffer, "0");
    else
      sprintf(buffer, "%ld", random() % num);
  }
}

static PRIM(prim_abs)
{
  double num;
  char *ptr;

  num = strtod(argv[1], &ptr);
  if(ptr != argv[1]) {
    if (num == 0.0) {
      strcpy(buffer, "0.0");
    } else if (num < 0.0) {
      fval(buffer, -num);
    } else {
      fval(buffer, num);
    }
  } else
    strcpy(buffer, ERR_NUM);
}

static PRIM(prim_sqrt)
{
  double num;
  char *ptr;

  num = strtod(argv[1], &ptr);
  if(ptr != argv[1]) {
    if (num < 0.0) {
      strcpy(buffer, "#-1 SQUARE ROOT OF NEGATIVE");
    } else if(num == 0.0) {
      strcpy(buffer, "0");
    } else {
      fval(buffer, sqrt(num));
    }
  } else
    strcpy(buffer, ERR_NUM);
}

static PRIM(prim_add)
{
  double num = 0.0;
  char *ptr;
  int i, got_one;

  for(i = 1,got_one = 0; i < argc; i++) {
    num += strtod(argv[i], &ptr);
    if(ptr == argv[i]) {
      strcpy(buffer, ERR_NUM);
      return;
    }
    if(i > 1)
      got_one++;
  }
  if(!got_one)
    strcpy(buffer, "#-1 TOO FEW ARGUMENTS");
  else
    fval(buffer, num);
}

static PRIM(prim_sub)
{
  double num1, num2;
  char *ptr;

  num1 = strtod(argv[1], &ptr);
  if(ptr == argv[1]) {
    strcpy(buffer, ERR_NUM);
    return;
  }
  num2 = strtod(argv[2], &ptr);
  if(ptr == argv[2]) {
    strcpy(buffer, ERR_NUM);
    return;
  }
  fval(buffer, num1 - num2);
}

static PRIM(prim_mul)
{
  double num, val;
  char *ptr;
  int i, got_one;

  for(i = 1,got_one = 0,num = 0.0; i < argc; i++) {
    val = strtod(argv[i], &ptr);
    if(ptr == argv[i]) {
      strcpy(buffer, ERR_NUM);
      return;
    }
    if(i > 1) {
      num *= val;
      got_one++;
    } else
      num = val;
  }
  if(!got_one)
    strcpy(buffer, "#-1 TOO FEW ARGUMENTS");
  else
    fval(buffer, num);
}

static PRIM(prim_floor)
{
  double num;
  char *ptr;

  num = strtod(argv[1], &ptr);
  if(ptr == argv[1])
    strcpy(buffer, ERR_NUM);
  else
    sprintf(buffer, "%.0f", floor(num));
}

static PRIM(prim_ceil)
{
  double num;
  char *ptr;

  num = strtod(argv[1], &ptr);
  if(ptr == argv[1])
    strcpy(buffer, ERR_NUM);
  else
    sprintf(buffer, "%.0f", ceil(num));
}

static PRIM(prim_round)
{
  const char *fstr;
  double num;
  int digits;
  char *ptr;

  digits = (int)strtol(argv[2], &ptr, 10);
  if(ptr == argv[2]){
    strcpy(buffer, ERR_NUM);
    return;
  }

  switch (digits) {
  case 1:         fstr = "%.1f"; break;
  case 2:         fstr = "%.2f"; break;
  case 3:         fstr = "%.3f"; break;
  case 4:         fstr = "%.4f"; break;
  case 5:         fstr = "%.5f"; break;
  case 6:         fstr = "%.6f"; break;
  default:        fstr = "%.0f"; break;
  }
  num = strtod(argv[1], &ptr);
  if(ptr == argv[1]){
    strcpy(buffer, ERR_NUM);
    return;
  }
  sprintf(buffer, fstr, num);

  if (!strcmp(buffer, "-0")) {
    strcpy(buffer, "0");
  }
}

static PRIM(prim_trunc)
{
  sprintf(buffer, "%d", atoi(argv[1]));
}

static PRIM(prim_div)
{
  double val1, val2;
  char *ptr;

  val1 = strtod(argv[1], &ptr);
  if(ptr == argv[1]) {
    strcpy(buffer, ERR_NUM);
    return;
  }
  val2 = strtod(argv[2], &ptr);
  if(ptr == argv[2]) {
    strcpy(buffer, ERR_NUM);
    return;
  }
  if(val2 == 0) {
    strcpy(buffer, ERR_DIVZERO);
  } else {
    fval(buffer, (val1 / val2));
  }
}

static PRIM(prim_mod)
{
#ifdef HAVE_FMOD
  double val1, val2;
  char *ptr;

  val1 = strtod(argv[1], &ptr);
  if(ptr == argv[1]){
    strcpy(buffer, ERR_NUM);
    return;
  }
  val2 = strtod(argv[2], &ptr);
  if(ptr == argv[2]){
    strcpy(buffer, ERR_NUM);
    return;
  }
  if(val2 == 0.0)
    val2 = 1;
  fval(buffer, fmod(val1, val2));
#else
  int num;

  if((num = atoi(argv[2])) == 0)
    num = 1;
  sprintf(buffer, "%d", atoi(argv[1]) % num);
#endif			/* HAVE_FMOD */
}

static PRIM(prim_pi)
{
  strcpy(buffer, "3.141592654");
}

static PRIM(prim_e)
{
  strcpy(buffer, "2.718281828");
}

static PRIM(prim_sin)
{
  double num;
  char *ptr;

  num = strtod(argv[1], &ptr);
  if(ptr == argv[1]) {
    strcpy(buffer, ERR_NUM);
  } else {
    fval(buffer, sin(num));
  }
}

static PRIM(prim_cos)
{
  double num;
  char *ptr;

  num = strtod(argv[1], &ptr);
  if(ptr == argv[1]) {
    strcpy(buffer, ERR_NUM);
  } else {
    fval(buffer, cos(num));
  }
}

static PRIM(prim_tan)
{
  double num;
  char *ptr;

  num = strtod(argv[1], &ptr);
  if(ptr == argv[1]) {
    strcpy(buffer, ERR_NUM);
  } else {
    fval(buffer, tan(num));
  }
}

static PRIM(prim_exp)
{
  double num;
  char *ptr;

  num = strtod(argv[1], &ptr);
  if(ptr == argv[1]) {
    strcpy(buffer, ERR_NUM);
  } else {
    fval(buffer, exp(num));
  }
}

static PRIM(prim_power)
{
  double arg1, arg2;
  char *ptr;

  arg1 = strtod(argv[1], &ptr);
  if(ptr == argv[1]) {
    strcpy(buffer, ERR_NUM);
    return;
  }
  arg2 = strtod(argv[2], &ptr);
  if(ptr == argv[2]) {
    strcpy(buffer, ERR_NUM);
    return;
  }
  if (arg1 < 0)
    strcpy(buffer, "#-1 POWER OF NEGATIVE");
  else
    fval(buffer, pow(arg1, arg2));
}

static PRIM(prim_ln)
{
  double val;
  char *ptr;

  val = strtod(argv[1], &ptr);
  if(ptr == argv[1]) {
    strcpy(buffer, ERR_NUM);
    return;
  }
  if(val > 0)
    fval(buffer, log(val));
  else
    strcpy(buffer, "#-1 LN OF NEGATIVE OR ZERO");
}

static PRIM(prim_log)
{
  double val;
  char *ptr;

  val = strtod(argv[1], &ptr);
  if(ptr == argv[1]) {
    strcpy(buffer, ERR_NUM);
    return;
  }
  if(val > 0)
    fval(buffer, log10(val));
  else
    strcpy(buffer, "#-1 LOG OF NEGATIVE OR ZERO");
}

static PRIM(prim_asin)
{
  double val;
  char *ptr;

  val = strtod(argv[1], &ptr);
  if(ptr == argv[1]) {
    strcpy(buffer, ERR_NUM);
    return;
  }
  if((val < -1) || (val > 1))
    strcpy(buffer, "#-1 ASIN ARGUMENT OUT OF RANGE");
  else
    fval(buffer, asin(val));
}

static PRIM(prim_acos)
{
  double val;
  char *ptr;

  val = strtod(argv[1], &ptr);
  if(ptr == argv[1]) {
    strcpy(buffer, ERR_NUM);
    return;
  }
  if((val < -1) || (val > 1))
    strcpy(buffer, "#-1 ACOS ARGUMENT OUT OF RANGE");
  else
    fval(buffer, acos(val));
}

static PRIM(prim_atan)
{
  double val;
  char *ptr;

  val = strtod(argv[1], &ptr);
  if(ptr == argv[1])
    strcpy(buffer, ERR_NUM);
  else
    fval(buffer, atan(val));
}

static PRIM(prim_min)
{
  int i;
  int ret, num;
  char *ptr;

  for(i = 1,ret = 0; i < argc; i++) {
    num = (int)strtol(argv[i], &ptr, 10);
    if(ptr == argv[i]) {
      strcpy(buffer, ERR_NUM);
      return;
    }

    if((num < ret) || (i == 1))
      ret = num;
  }
  sprintf(buffer, "%d", ret);
}

static PRIM(prim_max)
{
  int i;
  int ret, num;
  char *ptr;

  for(i = 1,ret = 0; i < argc; i++) {
    num = (int)strtol(argv[i], &ptr, 10);
    if(ptr == argv[i]) {
      strcpy(buffer, ERR_NUM);
      return;
    }

    if((num > ret) || (i == 1))
      ret = num;
  }
  sprintf(buffer, "%d", ret);
}

static PRIM(prim_expr)
{
  long ret;
  int error;

  ret = expr_parse(argv[1], &error);
  if(error)
    strcpy(buffer, "#-1 BAD EXPRESSION SYNTAX");
  else
    sprintf(buffer, "%ld", ret);
}

static PRIM(prim_if)
{
  if((argc < 3) || (argc > 5) || ((argc == 5)
      				  && strcasecmp(argv[3], "else"))) {
    strcpy(buffer, "#-1 WRONG NUMBER OF ARGUMENTS");
    return;
  }
  if (atoi(argv[1])) {
    strcpy(buffer, argv[2]);
  } else if(argc == 4){
    strcpy(buffer, argv[3]);
  } else if(argc == 5){
    strcpy(buffer, argv[4]);
  } else
    buffer[0] = '\0';
}

static PRIM(prim_setvar)
{
  int owner;
  char *val;

  if((argc < 2) || (argc > 3)) {
    strcpy(buffer, "#-1 WRONG NUMBER OF ARGUMENTS");
    return;
  }

  if(get_int_elt(player, OWNER, &owner) == -1) {
    strcpy(buffer, ERR_DB);
    return;
  }

  if(argc == 3) {
    var_set(owner, argv[1], argv[2]);
    strcpy(buffer, argv[2]);
  } else {
    val = var_get(owner, argv[1]);
    if(val == (char *)NULL) {
      strcpy(buffer, ERR_NOVAR);
      return;
    }
    strcpy(buffer, val);
    var_delete(owner, argv[1]);
  }
}

static PRIM(prim_getvar)
{
  int owner;
  char *val;

  if(get_int_elt(player, OWNER, &owner) == -1) {
    strcpy(buffer, ERR_DB);
    return;
  }

  val = var_get(owner, argv[1]);
  if(val != (char *)NULL) {
    strcpy(buffer, val);
  } else
    buffer[0] = '\0';
}

/* time primitives */

static PRIM(prim_time)
{
  sprintf(buffer, "%ld", mudstat.now);
}

static PRIM(prim_ctime)
{
  time_t when;
  char *ptr;

  when = (time_t)strtol(argv[1], &ptr, 10);
  if((when < 0) || (ptr == argv[1]))
    strcpy(buffer, "#-1 ILLEGAL TIME VALUE");
  else
    strftime(buffer, MEDBUFFSIZ, "%a %b %e %H:%M:%S %Z %Y", localtime(&when));
}

static PRIM(prim_cnvtime)
{
  extern char ty_getdate_err[];
  time_t ret;

  if((ret = get_date(argv[1], mudstat.now)) == -1)
    snprintf(buffer, blen, "#-1 %s", ty_getdate_err);
  else
    snprintf(buffer, blen, "%ld", ret);
}

/* string/list primitives */

/* format is loosely based on the public domain vsprintf(). */
static PRIM(prim_format)
{
  char *dp = buffer;
  char *format;
  char c;
  char *tp, *sa;
  char tempfmt[64], tempbuf[128], *sptr;
#if (SIZEOF_INT != SIZEOF_LONG)
  int longflag;
#endif
  int outspace = 0, curarg = 2;
  int nbase;
  const char *formaterr = "#-1 FORMAT ARGUMENT TYPE ERROR";

  format = argv[1];

  tempfmt[0] = '%';
  while((c = *format++) && !outspace) {
    if(c == '%') {
      tp = &tempfmt[1];
#if (SIZEOF_INT != SIZEOF_LONG)
      longflag = 0;
#endif

continue_format:
      nbase = 10;

      switch((c = *format++)) {
        /* String. */
	case 'c':
        case 's': {
	  int padding;
	  char *ptr, *buff;

	  *tp++ = c;
	  *tp = '\0';

	  /* Scan for padding. */
	  for(ptr = tempfmt; *ptr && !isdigit(*ptr); ptr++);
	  if(isdigit(*ptr)) {
	    padding = (int)strtol(ptr, NULL, 10);
	    if(padding < 0)
	      padding = abs(padding);
	  } else
	    padding = 0;

	  sa = argv[curarg++];

	  if(padding) {
	    padding = (padding > strlen(sa)) ? padding : strlen(sa);

	    /* Allocate a buffer to hold the return. */
	    buff = (char *)alloca(padding+1);
	    if(buff == (char *)NULL) {
	      strcpy(buffer, "#-1 STACK ALLOCATION FAILED");
	      return;
	    }

	    /* Call sprintf() in order to get padding right. */
	    sprintf(buff, tempfmt, sa);

	    if(strlen(buff) >= (blen - (dp - buffer))) {
	      buff[blen - ((dp - buffer)+1)] = '\0';
	      outspace++;
	    }
	    strcpy(dp, buff);
	    dp += strlen(buff);
	  } else {
	    ptr = sa;
	    while(*ptr && ((dp - buffer) < (blen-1)))
	      *dp++ = *ptr++;
	    /* We don't terminate, but we do check for overflow. */
	    if((dp - buffer) >= blen)
	      outspace++;
	  }
	} break;
	/* Unsigned values. */
	case 'p':
	case 'x':
	case 'X':
	  nbase = 16;
	case 'o':
	  nbase = 8;
	case 'u': {
	  *tp++ = c;
	  *tp = '\0';

	  sa = argv[curarg++];

#if (SIZEOF_INT != SIZEOF_LONG)
	  if(longflag) {
	    unsigned long val;

	    val = strtoul(sa, &sptr, nbase);
	    if(sptr == sa) {
	      strcpy(buffer, formaterr);
	      return;
	    }
	    sprintf(tempbuf, tempfmt, val);
	  } else
#endif
	  {
	    unsigned int val;

	    val = (int)strtoul(sa, &sptr, nbase);
	    if(sptr == sa) {
	      strcpy(buffer, formaterr);
	      return;
	    }
	    sprintf(tempbuf, tempfmt, val);
	  }
	  if(strlen(tempbuf) >= (blen - (dp - buffer))) {
	    tempbuf[blen - ((dp - buffer)+1)] = '\0';
	    outspace++;
	  }
	  strcpy(dp, tempbuf);
	  dp += strlen(tempbuf);
	} break;
	/* Signed values. */
	case 'd': {
	  *tp++ = c;
	  *tp = '\0';

	  sa = argv[curarg++];

#if (SIZEOF_INT != SIZEOF_LONG)
	  if(longflag) {
	    long val;

	    val = strtol(sa, &sptr, 10);
	    if(sptr == sa) {
	      strcpy(buffer, formaterr);
	      return;
	    }
	    sprintf(tempbuf, tempfmt, val);
	  } else
#endif
	  {
	    int val;

	    val = strtol(sa, &sptr, 10);
	    if(sptr == sa) {
	      strcpy(buffer, formaterr);
	      return;
	    }
	    sprintf(tempbuf, tempfmt, val);
	  }
	  if(strlen(tempbuf) >= (blen - (dp - buffer))) {
	    tempbuf[blen - ((dp - buffer)+1)] = '\0';
	    outspace++;
	  }
	  strcpy(dp, tempbuf);
	  dp += strlen(tempbuf);
	} break;
	case 'f':
	case 'e':
	case 'E':
	case 'g':
	case 'G': {
	  double val;

	  *tp++ = c;
	  *tp = '\0';

	  sa = argv[curarg++];

	  val = strtod(sa, &sptr);
	  if(sa == sptr) {
	    strcpy(buffer, formaterr);
	    return;
	  }

	  sprintf(tempbuf, tempfmt, val);
	  if(strlen(tempbuf) >= (blen - (dp - buffer))) {
	    tempbuf[blen - ((dp - buffer)+1)] = '\0';
	    outspace++;
	  }
	  strcpy(dp, tempbuf);
	  dp += strlen(tempbuf);
	} break;
	case '-':
	case '+':
	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':
	case '.':
	case ' ':
	case '#':
	case 'h':
	  *tp++ = c;
	  goto continue_format;
	case 'l':
#if (SIZEOF_INT != SIZEOF_LONG)
	  longflag = 1;
	  *tp++ = c;
#endif
	  goto continue_format;
	case '*': {
	  char *ptr;

	  sa = argv[curarg++];

	  for(ptr = sa; *ptr && isdigit(*ptr); ptr++);
	  if(*ptr) {
	    strcpy(buffer, formaterr);
	    return;
	  }
	  strcpy(tp, sa);
	  tp += strlen(sa);
	  goto continue_format;
	} break;
	case 'n':
	  strcpy(buffer, "#-1 FORMAT TYPE 'n' NOT SUPPORTED");
	  return;
	case '%':
	default:
	  if((blen - (dp - buffer)) > 1)
	    *dp++ = c;
	  else
	    outspace++;
	  break;
      }
    } else {
      if((blen - (dp - buffer)) > 1)
	*dp++ = c;
      else
	outspace++;
    }
  }
  *dp = '\0';
}

static PRIM(prim_mid)
{
  char *str;
  char *ptr;
  int pos, len;

  str = argv[1];
  pos = (int)strtol(argv[2], &ptr, 10);
  if(ptr == argv[2]) {
    strcpy(buffer, ERR_NUM);
    return;
  }
  len = (int)strtol(argv[3], &ptr, 10);
  if(ptr == argv[3]) {
    strcpy(buffer, ERR_NUM);
    return;
  }

  if((pos < 0) || (len < 0) || (pos > strlen(str))) {
    strcpy(buffer, "#-1 OUT OF RANGE");
    return;
  }
  strlcpy(buffer, &str[pos], len);
}

static PRIM(prim_first)
{
  char sep = ' ';
  char *ptr, *bptr;

  if(argc == 3) {
    sep = (argv[2])[0];		/* we only care about the fist char. */
  } else if(argc != 2) {
    strcpy(buffer, ERR_TOOARGS);
    return;
  }

  ptr = argv[1];
  while((*ptr != '\0') && (*ptr == sep))	/* trim leading sep */
    ptr++;

  bptr = buffer;
  while((*ptr != '\0') && (*ptr != sep))
    *bptr++ = *ptr++;
  *bptr = '\0';
}

static PRIM(prim_rest)
{
  char sep = ' ';
  char *ptr, *bptr;

  if(argc == 3) {
    sep = (argv[2])[0];		/* we only care about the first char. */
  } else if(argc != 2) {
    strcpy(buffer, ERR_TOOARGS);
    return;
  }

  ptr = argv[1];
  while((*ptr != '\0') && (*ptr == sep))	/* trim leading sep */
    ptr++;
  while((*ptr != '\0') && (*ptr != sep))	/* skip first word */
    ptr++;
  while((*ptr != '\0') && (*ptr == sep))	/* skip sep */
    ptr++;

  /* copy the rest */
  bptr = buffer;
  while(*ptr != '\0')
    *bptr++ = *ptr++;
  *bptr = '\0';
}

static PRIM(prim_last)
{
  char sep = ' ';
  char *ptr, *bptr;

  if(argc == 3) {
    sep = (argv[2])[0];		/* we only care about the first char. */
  } else if(argc != 2) {
    strcpy(buffer, ERR_TOOARGS);
    return;
  }

  ptr = argv[1];
  while(*ptr != '\0')
    ptr++;
  while((ptr > argv[1]) && (*ptr == sep))
    ptr--;
  while((ptr > argv[1]) && (*ptr != sep))
    ptr--;
  
  bptr = buffer;
  while((*ptr != '\0') && (*ptr != sep))
    *bptr++ = *ptr++;
  *bptr = '\0';
}

static PRIM(prim_words)
{
  char sep= ' ';
  char *ptr;
  int count = 0;

  if(argc == 3) {
    sep = (argv[2])[0];		/* we only care about the first char. */
  } else if(argc != 2) {
    strcpy(buffer, ERR_TOOARGS);
    return;
  }

  ptr = argv[1];
  while((*ptr != '\0') && (*ptr == sep))	/* trim leading sep */
    ptr++;

  while(*ptr != '\0') {
    if(*ptr == sep) {				/* update count */
      while((*ptr != '\0') && (*ptr == sep))
	ptr++;
      if(*ptr != '\0')
	count++;
    } else
      ptr++;
  }
  sprintf(buffer, "%d", count);
}

static PRIM(prim_strlen)
{
  sprintf(buffer, "%lu", strlen(argv[1]));
}

static PRIM(prim_strcmp)
{
  sprintf(buffer, "%d", !strcasecmp(argv[1], argv[2]));
}

static PRIM(prim_strmatch)
{
  sprintf(buffer, "%d", quick_wild(argv[1], argv[2]));
}

static PRIM(prim_strprefix)
{
  sprintf(buffer, "%d", !strncasecmp(argv[1], argv[2], strlen(argv[1])));
}

static PRIM(prim_strncmp)
{
  int len;
  char *ptr;

  len = (int)strtol(argv[3], &ptr, 10);
  if(ptr == argv[3]) {
    strcpy(buffer, ERR_NUM);
    return;
  }
  sprintf(buffer, "%d", !strncasecmp(argv[1], argv[2], len));
}

static PRIM(prim_wrdmatch)
{
  char sep = ' ';
  char *str, *ptr;
  int count = 1;

  if(argc == 4) {
    sep = (argv[3])[0];		/* we only care about the first char. */
  } else if(argc != 3) {
    strcpy(buffer, ERR_TOOARGS);
    return;
  }

  str = argv[2];
  while((*str != '\0') && (*str == sep))
    str++;
  do {
    /* find the next word */
    for(ptr = str; (*ptr != '\0') && (*ptr != sep); ptr++);
    if(*ptr != '\0') {
      *ptr++ = '\0';
      while((*ptr != '\0') && (*ptr == sep))
	ptr++;
    }

    if(quick_wild(argv[1], str)) {
      sprintf(buffer, "%d", count);
      return;
    }
    str = ptr;
    count++;
  } while(*str != '\0');
  strcpy(buffer, "0");
}

static PRIM(prim_strcat)
{
  int i;

  buffer[0] = '\0';
  for(i = 1; i < argc; i++) {
    if(i > 1)
      strcat(buffer, " ");
    strcat(buffer, argv[i]);
  }
}

static PRIM(prim_strdcat)
{
  int i;

  buffer[0] = '\0';
  for(i = 1; i < argc; i++)
    strcat(buffer, argv[i]);
}

static PRIM(prim_crypt)
{
  strcpy(buffer, sha_crypt(argv[1]));
}

static PRIM(prim_upcase)
{
  int i, j, k;
  char *ptr;

  k = 0;
  for(i = 1; i < argc; i++) {
    if(i > 1)
      buffer[k++] = ' ';
    ptr = argv[i];
    for(j = 0; ptr[j] != '\0'; j++) {
      buffer[k++] = to_upper(ptr[j]);
    }
  }
  buffer[k] = '\0';
}

static PRIM(prim_lowcase)
{
  int i, j, k;
  char *ptr;

  k = 0;
  for(i = 1; i < argc; i++) {
    if(i > 1)
      buffer[k++] = ' ';
    ptr = argv[i];
    for(j = 0; ptr[j] != '\0'; j++) {
      buffer[k++] = to_lower(ptr[j]);
    }
  }
  buffer[k] = '\0';
}

static PRIM(prim_swapcase)
{
  int i, j, k;
  char *ptr;

  k = 0;
  for(i = 1; i < argc; i++) {
    if(i > 1)
      buffer[k++] = ' ';
    ptr = argv[i];
    for(j = 0; ptr[j] != '\0'; j++) {
      buffer[k++] = (isupper(ptr[j]) ? to_lower(ptr[j]) : to_upper(ptr[j]));
    }
  }
  buffer[k] = '\0';
}

Prim PTable[] = {
  /* database primitives */
  {"getattr", prim_getattr, 2, 0, 0},
  {"getlock", prim_getlock, 2, 0, 0},
  {"contents", prim_contents, 1, 0, 0},
  {"exits", prim_exits, 1, 0, 0},
  {"rooms", prim_rooms, 1, 0, 0},
  {"owner", prim_owner, 1, 0, 0},
  {"quota", prim_quota, 1, 0, 0},
  {"pennies", prim_pennies, 1, 0, 0},
  {"loc", prim_loc, 1, 0, 0},
  {"rloc", prim_rloc, 2, 0, 0},
  {"controls", prim_controls, 2, 0, 0},
  {"home", prim_home, 1, 0, 0},
  {"dropto", prim_dropto, 1, 0, 0},
  {"dests", prim_dests, 1, 0, 0},
  {"timestamp", prim_timestamp, 1, 0, 0},
  {"createstamp", prim_createstamp, 1, 0, 0},
  {"parent", prim_parent, 1, 0, 0},
  {"uses", prim_uses, 1, 0, 0},
  {"name", prim_name, 1, 0, 0},
  {"flags", prim_flags, 1, 0, 0},
  {"type", prim_type, 1, 0, 0},
  {"match", prim_match, 1, 0, 0},
  {"pmatch", prim_pmatch, 1, 0, 0},
  {"wholist", prim_wholist, 0, 0, 0},
  /* math primitives */
  {"rand", prim_rand, 1, 0, 0},
  {"abs", prim_abs, 1, 0, 0},
  {"sqrt", prim_sqrt, 1, 0, 0},
  {"add", prim_add, -1, PRIM_VARARGS, 0},
  {"sub", prim_sub, 2, 0, 0},
  {"mul", prim_mul, -1, PRIM_VARARGS, 0},
  {"floor", prim_floor, 1, 0, 0},
  {"ceil", prim_ceil, 1, 0, 0},
  {"round", prim_round, 2, 0, 0},
  {"trunc", prim_trunc, 1, 0, 0},
  {"div", prim_div, 2, 0, 0},
  {"mod", prim_mod, 2, 0, 0},
  {"pi", prim_pi, 0, 0, 0},
  {"e", prim_e, 0, 0, 0},
  {"sin", prim_sin, 1, 0, 0},
  {"cos", prim_cos, 1, 0, 0},
  {"tan", prim_tan, 1, 0, 0},
  {"exp", prim_exp, 1, 0, 0},
  {"power", prim_power, 2, 0, 0},
  {"ln", prim_ln, 1, 0, 0},
  {"log", prim_log, 1, 0, 0},
  {"asin", prim_asin, 1, 0, 0},
  {"acos", prim_acos, 1, 0, 0},
  {"atan", prim_atan, 1, 0, 0},
  {"min", prim_min, 2, PRIM_VARARGS, 0},
  {"max", prim_max, 2, PRIM_VARARGS, 0},
  /* conditionals */
  {"expr", prim_expr, 1, 0, 0},
  {"if", prim_if, -1, PRIM_VARARGS, 0},
  /* variables */
  {"setvar", prim_setvar, -1, PRIM_VARARGS, 0},
  {"getvar", prim_getvar, 1, 0, 0},
  /* time prims */
  {"time", prim_time, 0, 0, 0},
  {"ctime", prim_ctime, 1, 0, 0},
  {"cnvtime", prim_cnvtime, 1, 0, 0},
  /* strings */
  {"format", prim_format, 1, PRIM_VARARGS, 0},
  {"mid", prim_mid, 3, 0, 0},
  {"first", prim_first, 1, PRIM_VARARGS, 0},
  {"rest", prim_rest, 1, PRIM_VARARGS, 0},
  {"last", prim_last, 1, PRIM_VARARGS, 0},
  {"words", prim_words, 1, PRIM_VARARGS, 0},
  {"strlen", prim_strlen, 1, 0, 0},
  {"strcmp", prim_strcmp, 2, 0, 0},
  {"strmatch", prim_strmatch, 2, 0, 0},
  {"strprefix", prim_strprefix, 2, 0, 0},
  {"strncmp", prim_strncmp, 3, 0, 0},
  {"wrdmatch", prim_wrdmatch, 2, PRIM_VARARGS, 0},
  {"strcat", prim_strcat, 1, PRIM_VARARGS, 0},
  {"strdcat", prim_strdcat, 1, PRIM_VARARGS, 0},
  {"crypt", prim_crypt, 1, 0, 0},
  {"upcase", prim_upcase, 1, PRIM_VARARGS, 0},
  {"lowcase", prim_lowcase, 1, PRIM_VARARGS, 0},
  {"swapcase", prim_swapcase, 1, PRIM_VARARGS, 0}
};

int PTable_count = sizeof(PTable) / sizeof(Prim);

int prims_cmp(s1, s2)
    const VOID *s1;
    const VOID *s2;
{
  const Prim *c1 = s1;
  const Prim *c2 = s2;

  return(strcasecmp(c1->name, c2->name));
}

/* Sort the table. */
void prims_init()
{
  qsort(PTable, PTable_count, sizeof(Prim), prims_cmp);
}
