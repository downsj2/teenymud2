/* textin.c */

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
#include <netinet/in.h>
#ifdef HAVE_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif				/* HAVE_STRING_H */
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif				/* HAVE_STDLIB_H */
#include <ctype.h>

#include "conf.h"
#include "teeny.h"
#include "teenydb.h"
#include "textdb.h"
#include "sha/sha_wrap.h"
#include "externs.h"

static char gender[16];

/*
 * We have to read TinyMUD objects all in one gulp, since their
 * field order is so incompatible.
 */
static struct {
  char name[512];
  char description[512];
  char fail[512];
  char succ[512];
  char ofail[512];
  char osucc[512];
  char password[512];

  struct boolexp *lock;

  int location;
  int contents;
  int exits;
  int next;
  int owner;
  int pennies;
  int flags;
} tinyobj;

static int getnumber _ANSI_ARGS_((FILE *, int, int, int, int));
static struct boolexp *unparse_oldlock _ANSI_ARGS_((int *));
static int getarray _ANSI_ARGS_((FILE *, int, int, int, int));
static struct boolexp *negate_bool _ANSI_ARGS_((struct boolexp *));
static struct boolexp *getbool_sub _ANSI_ARGS_((int, int, int));
static int getbool _ANSI_ARGS_((FILE *, int, char *, int, int));
static int getstring _ANSI_ARGS_((FILE *, int, int, int, int));
static int getstrings _ANSI_ARGS_((FILE *, int, int, int));
static int getattr _ANSI_ARGS_((FILE *, int, char *, int, int));
static int convert_aflags _ANSI_ARGS_((char *, int, int, int));
static int getattributes _ANSI_ARGS_((FILE *, int, int, int));
static int *convert_flags _ANSI_ARGS_((int, int, int));

/* ARGSUSED */
static int getnumber(in, obj, code, read_version, read_flags)
    FILE *in;
    int obj, code;
    int read_version, read_flags;
{
  int num, ret;
  time_t tval;
  char *ptr;

  if(fgets(txt_buffer, TXTBUFFSIZ, in) == (char *)NULL){
    fprintf(stderr, txt_eoferr, obj);
    return(-1);
  }
  if((code != TIMESTAMP) && (code != CREATESTAMP))
    num = (int)strtol(txt_buffer, &ptr, 10);
  else {
#if SIZEOF_TIME_T > SIZEOF_LONG
    tval = (time_t)strtoll(txt_buffer, &ptr, 10);
#else
    tval = (time_t)strtol(txt_buffer, &ptr, 10);
#endif
  }
  if(ptr == txt_buffer) {
    fprintf(stderr, txt_numerr, obj);
    return(-1);
  }
  if((read_version < 100) && (code == TIMESTAMP))
    tval *= 60;
  if((code != TIMESTAMP) && (code != CREATESTAMP))
    ret = set_int_elt(obj, code, num);
  else
    ret = set_time_elt(obj, code, tval);
  if(ret == -1) {
    fprintf(stderr, txt_dberr, obj);
    return(-1);
  }
  return(0);
}

/* old RPN lock emulation code */

struct tree {
  int dat;
  short left, right;
};

static struct boolexp *to_bool _ANSI_ARGS_((struct tree *, int));

static struct boolexp *unparse_oldlock(lock)
    int *lock;
{
  int stack[64];
  struct tree trees[64];
  int tp;
  int sp;
  int count;

  tp = sp = 0;
  count = *lock++;

  while ((*lock != -1) && count) {
    switch (*lock) {
    case -2:
    case -3:
      if (sp < 2 || tp > 63)
	return ((struct boolexp *)NULL);
      trees[tp].dat = *lock;
      trees[tp].left = (short) (stack[--sp]);
      trees[tp].right = (short) (stack[--sp]);
      stack[sp++] = tp++;
      break;
    case -4:
      if (sp < 1 || tp > 63)
	return ((struct boolexp *)NULL);
      trees[tp].dat = -4;
      trees[tp].right = (short) (stack[--sp]);
      stack[sp++] = tp++;
      break;
    default:
      if (sp > 255 || tp > 63)
	return ((struct boolexp *)NULL);
      trees[tp].dat = *lock;
      if (sp < 63)
	stack[sp++] = tp++;
      else
	return ((struct boolexp *)NULL);
      break;
    }
    lock++;
    count--;
  }
  if (sp != 1)
    return ((struct boolexp *)NULL);

  return(to_bool(trees, tp -1));
}

static struct boolexp *to_bool(trees, t)
    struct tree *trees;
    int t;
{
  struct boolexp *ret = boolexp_alloc();

  switch (trees[t].dat) {
  case -2:
    ret->type = BOOLEXP_AND;
    ret->sub1 = to_bool(trees, trees[t].left);
    ret->sub2 = to_bool(trees, trees[t].right);
    if((ret->sub1 == (struct boolexp *)NULL) 
       || (ret->sub2 == (struct boolexp *)NULL)) {
      boolexp_free(ret);
      return((struct boolexp *)NULL);
    }
    break;
  case -3:
    ret->type = BOOLEXP_OR;
    ret->sub1 = to_bool(trees, trees[t].left);
    ret->sub2 = to_bool(trees, trees[t].right);
    if((ret->sub1 == (struct boolexp *)NULL)
       || (ret->sub2 == (struct boolexp *)NULL)) {
      boolexp_free(ret);
      return((struct boolexp *)NULL);
    }
    break;
  case -4:
    ret->type = BOOLEXP_NOT;
    ret->sub1 = to_bool(trees, trees[t].right);
    if(ret->sub1 == (struct boolexp *)NULL) {
      boolexp_free(ret);
      return((struct boolexp *)NULL);
    }
    break;
  case -100:
    ret->type = BOOLEXP_ATTR;
    ((ret->dat).atr)[0] = ty_strdup("sex", "to_bool");
    ((ret->dat).atr)[1] = ty_strdup("male", "to_bool");
    break;
  case -101:
    ret->type = BOOLEXP_ATTR;
    ((ret->dat).atr)[0] = ty_strdup("sex", "to_bool");
    ((ret->dat).atr)[1] = ty_strdup("female", "to_bool");
    break;
  case -102:
    ret->type = BOOLEXP_ATTR;
    ((ret->dat).atr)[0] = ty_strdup("sex", "to_bool");
    ((ret->dat).atr)[1] = ty_strdup("it", "to_bool");
    break;
  default:
    if(trees[t].dat > -1) {
      ret->type = BOOLEXP_CONST;
      (ret->dat).thing = trees[t].dat;
    } else {
      boolexp_free(ret);
      return((struct boolexp *)NULL);
    }
    break;
  }
  return (ret);
}

/* ARGSUSED */
static int getarray(in, obj, code, read_version, read_flags)
    FILE *in;
    int obj, code;
    int read_version, read_flags;
{
  int *array, count;
  int i;
  char *ptr, *ptr2;

  if(fgets(txt_buffer, TXTBUFFSIZ, in) == (char *)NULL) {
    fprintf(stderr, txt_eoferr, obj);
    return(-1);
  }

  /* regardless of the database type, if it's -1, we go NULL. */
  count = (int)strtol(txt_buffer, &ptr, 10);
  if(ptr == txt_buffer) {
    fprintf(stderr, txt_numerr, obj);
    return(-1);
  }

  if(count == -1) {
    array = (int *)NULL;
  } else {
    if(read_flags & DB_DESTARRAY) {	/* new style */
      array = (int *)alloca((count + 1) * sizeof(int));
      if(array == (int *)NULL)
        panic("getarray(): stack allocation failed\n");
      array[0] = count;

      for(i = 1; i <= count; i++) {
        array[i] = (int)strtol(ptr, &ptr2, 10);
	if(ptr2 == ptr) {
	  fprintf(stderr, txt_numerr, obj);
	  return(-1);
	}
	ptr = ptr2;
      }
    } else {				/* count is really a single dest */
      array = (int *)alloca(sizeof(int) * 2);
      if(array == (int *)NULL)
        panic("getarray(): stack allocation failed\n");
      array[0] = 1;
      array[1] = count;
    }
  }

  if(set_array_elt(obj, code, array) == -1) {
    fprintf(stderr, txt_dberr, obj);
    return(-1);
  }
  return(0);
}

static struct boolexp *negate_bool(curr)
    struct boolexp *curr;
{
  struct boolexp *new;

  if(curr == (struct boolexp *)NULL)
    return((struct boolexp *)NULL);

  new = boolexp_alloc();
  new->type = BOOLEXP_NOT;
  new->sub1 = curr;

  return(new);
}

static char *getbool_ptr = (char *)NULL;

/* ARGSUSED */
static struct boolexp *getbool_sub(obj, read_version, read_flags)
    int obj, read_version, read_flags;
{
  struct boolexp *ret = (struct boolexp *)NULL;
  char *ptr, *nptr;

  if(getbool_ptr == (char *)NULL)
    return((struct boolexp *)NULL);

  switch(*getbool_ptr) {
  case '\0':
  case '\n':
    return((struct boolexp *)NULL);
  case '(':
    ret = boolexp_alloc();

    switch(*++getbool_ptr) {
    case '!':
      ret->type = BOOLEXP_NOT;

      getbool_ptr++;
      ret->sub1 = getbool_sub(obj, read_version, read_flags);
      if((ret->sub1 == (struct boolexp *)NULL) || (*getbool_ptr != ')')) {
        fprintf(stderr, txt_boolerr, obj);
	boolexp_free(ret);
	return((struct boolexp *)NULL);
      }
      getbool_ptr++;
      return(ret);
    default:
      ret->sub1 = getbool_sub(obj, read_version, read_flags);

      switch(*getbool_ptr) {
      case '&':
        ret->type = BOOLEXP_AND;
	break;
      case '|':
        ret->type = BOOLEXP_OR;
	break;
      default:
        ret->type = BOOLEXP_NOT;

        fprintf(stderr, txt_boolerr, obj);
	boolexp_free(ret);
	return((struct boolexp *)NULL);
      }

      getbool_ptr++;
      ret->sub2 = getbool_sub(obj, read_version, read_flags);
      if((ret->sub1 == (struct boolexp *)NULL)
         || (ret->sub2 == (struct boolexp *)NULL) || (*getbool_ptr != ')')) {
        fprintf(stderr, txt_boolerr, obj);
	boolexp_free(ret);
	return((struct boolexp *)NULL);
      }
      getbool_ptr++;
      return(ret);
    }
    break;
  case '#':		/* TeenyMUD const token */
  case '0':		/* TinyMUD const numbers */
  case '1':
  case '2':
  case '3':
  case '4':
  case '5':
  case '6':
  case '7':
  case '8':
  case '9':
    ret = boolexp_alloc();
    ret->type = BOOLEXP_CONST;

    if(*getbool_ptr == '#')
      getbool_ptr++;

    (ret->dat).thing = (int)strtol(getbool_ptr, &ptr, 10);
    if(ptr == getbool_ptr) {
      fprintf(stderr, txt_boolerr, obj);
      boolexp_free(ret);
      return((struct boolexp *)NULL);
    }
    getbool_ptr = ptr;
    break;
  case '~':
    ret = boolexp_alloc();
    ret->type = BOOLEXP_FLAG;

    getbool_ptr++;
    ((ret->dat).flags)[0] = (int)strtol(getbool_ptr, &ptr, 10);
    if((ptr == getbool_ptr) || (*ptr != '/')) {
      fprintf(stderr, txt_boolerr, obj);
      boolexp_free(ret);
      return((struct boolexp *)NULL);
    }
    ptr++;
    ((ret->dat).flags)[1] = (int)strtol(ptr, &nptr, 10);
    if(nptr == ptr) {
      fprintf(stderr, txt_boolerr, obj);
      boolexp_free(ret);
      return((struct boolexp *)NULL);
    }

    getbool_ptr = nptr;
    break;
  case '{':
    ret = boolexp_alloc();
    ret->type = BOOLEXP_ATTR;

    ptr = ++getbool_ptr;
    for(nptr = getbool_ptr; *nptr && (*nptr != ':'); nptr++);
    if(*nptr == ':')
      *nptr++ = '\0';

    for(getbool_ptr = nptr; *getbool_ptr && (*getbool_ptr != '}');
        getbool_ptr++);
    if(*getbool_ptr == '}')
      *getbool_ptr++ = '\0';

    ((ret->dat).atr)[0] = ty_strdup(ptr, "getbool_sub");
    ((ret->dat).atr)[1] = ty_strdup(nptr, "getbool_sub");
    break;
  }

  return(ret);
}

/* ARGSUSED */
static int getbool(in, obj, name, read_version, read_flags)
    FILE *in;
    int obj;
    char *name;
    int read_version, read_flags;
{
  struct boolexp *ret = (struct boolexp *)NULL;

  if(fgets(txt_buffer, TXTBUFFSIZ, in) == (char *)NULL) {
    fprintf(stderr, txt_eoferr, obj);
    return(-1);
  }

  if(read_version > 299) {
    getbool_ptr = txt_buffer;
    ret = getbool_sub(obj, read_version, read_flags);
  } else {
    if(txt_buffer[0] != '\n') {
      int work[64], size = 64;
      int i;
      char *ptr, *optr;

      remove_newline(txt_buffer);

      for(i = 0, optr = txt_buffer; i <= size; i++) {
	work[i] = (int)strtol(optr, &ptr, 10);
	if(ptr == optr) {
	  fprintf(stderr, txt_numerr, obj);
	  return(-1);
	}
	if(i == 0)
	  size = work[i];

	optr = ptr;
      }
      ret = unparse_oldlock(work);
    }
  }

  if(ret != (struct boolexp *)NULL) {
    if(attr_addlock(obj, name, ret, DEFLOCK_FLGS) == -1) {
      fprintf(stderr, txt_dberr, obj);
      return(-1);
    }
  }
  return(0);
}

/* ARGSUSED */
static int getstring(in, obj, code, read_version, read_flags)
    FILE *in;
    int obj, code;
    int read_version, read_flags;
{
  if(fgets(txt_buffer, TXTBUFFSIZ, in) == (char *)NULL){
    fprintf(stderr, txt_eoferr, obj);
    return(-1);
  }
  remove_newline(txt_buffer);

  if(set_str_elt(obj, code, txt_buffer[0] ? txt_buffer : (char *)NULL) == -1) {
    fprintf(stderr, txt_dberr, obj);
    return(-1);
  }
  return(0);
}

static char *convert_attr(obj, code, aflags)
    int obj, code;
    int *aflags;
{
  static char tmp[27];

  *aflags = DEFATTR_FLGS;
  switch (code) {
  case 14:
    return (SUC);
  case 15:
    return (OSUC);
  case 16:
    return (FAIL);
  case 17:
    return (OFAIL);
  case 18:
    return (DESC);
  case 19:
    return (SEX);
  case 20:
    return (DROP);
  case 21:
    return (ODROP);
  case 22:
    return (ENTER);
  case 23:
    return (OENTER);
  case 24:
    return (LEAVE);
  case 25:
    return (OLEAVE);
  case 26:
    return (IDESC);
  case 27:
    return (ODESC);
  case 28:
    *aflags = SITE_FLGS;
    return (SITE);
  case 29:
    return (KILL);
  case 30:
    return (OKILL);
  case 31:
    *aflags = PASSWORD_FLGS;
    return (PASSWORD);
  case 32:
    return (OTELEPORT);
  case 33:
    return (OXTELEPORT);
  case 34:
    return (OXENTER);
  case 35:
    return (OXLEAVE);
  case 36:
    return (ENFAIL);
  case 37:
    return (OENFAIL);
  default:
    (void) snprintf(tmp, sizeof(tmp), "Unknown%d", code);
    return (tmp);
  }
}

/* ARGSUSED */
static int getstrings(in, obj, read_version, read_flags)
    FILE *in;
    int obj, read_version, read_flags;
{
  int ret, aflags, code = 0;
  char *p, *attr;

  do {
    if (fgets(txt_buffer, TXTBUFFSIZ, in) == (char *)NULL) {
      (void) fprintf(stderr, txt_eoferr, obj);
      return(-1);
    }
    remove_newline(txt_buffer);

    if (txt_buffer[0] && txt_buffer[0] != '<') {
      code = (int)strtol(txt_buffer, &p, 10);
      if((p == txt_buffer) || (*p != ':')) {
	fprintf(stderr, txt_numerr, obj);
	return(-1);
      }
      p++;
      if (code == 1) {	/* name */
	ret = set_str_elt(obj, NAME, p);
       } else {
	attr = convert_attr(obj, code, &aflags);
	ret = attr_add(obj, attr, p, aflags);
      }
      if (ret == -1) {
	fprintf(stderr, txt_dberr, obj);
	return(-1);
      }
    }
  } while (txt_buffer[0] != '<');
  return(0);
}

/* ARGSUSED */
static int getattr(in, obj, attr, read_version, read_flags)
    FILE *in;
    int obj;
    char *attr;
    int read_version, read_flags;
{
  if(fgets(txt_buffer, TXTBUFFSIZ, in) == (char *)NULL){
    fprintf(stderr, txt_eoferr, obj);
    return(-1);
  }
  remove_newline(txt_buffer);

  if(txt_buffer[0]) {
    if(attr_add(obj, attr, txt_buffer, DEFATTR_FLGS) == -1){
      fprintf(stderr, txt_dberr, obj);
      return(-1);
    }
  }
  return(0);
}

/* ARGSUSED */
static int convert_aflags(str, flags, read_version, read_flags)
    char *str;
    int flags, read_version, read_flags;
{
  int nflags = flags;

  if (read_version < 300) {
    if(strcmp(str, SITE) == 0) {
      nflags = SITE_FLGS;
    } else if(strcmp(str, PASSWORD) == 0) {
      nflags = PASSWORD_FLGS;
    } else
      nflags = 0;

#if 0
    /* This seems to be bogus... */
    if(flags & 0x00000100)
      nflags |= A_PRIVATE;
#endif
  } else if (!(read_flags & DB_IMMUTATTRS)) {
    /* SITE_FLAGS changed. */
    if(strcmp(str, SITE) == 0)
      nflags = SITE_FLGS;
  }
  return(nflags);
}

/* ARGSUSED */
static int getattributes(in, obj, read_version, read_flags)
    FILE *in;
    int obj;
    int read_version, read_flags;
{
  char *ptr, *nptr, *data;
  char name[MAXATTRNAME+1];
  int aflags, done = 0;
  short atype;

  while (!done) {
    if(fgets(txt_buffer, TXTBUFFSIZ, in) == (char *)NULL){
      fprintf(stderr, txt_eoferr, obj);
      return(-1);
    }
    remove_newline(txt_buffer);

    if(txt_buffer[0] && (txt_buffer[0] != '>')) {
      if(read_version < 300) {	/* pathetic format */
	for(ptr = txt_buffer; *ptr && (*ptr != ':'); ptr++);
	*ptr++ = '\0';

	for(data = ptr; *data && (*data != ':'); data++);
	*data++ = '\0';

	aflags = (int)strtol(data, &nptr, 10);
	if((nptr == data) || (*nptr != ':')){
	  fprintf(stderr, txt_numerr, obj);
	  return(-1);
	}
	aflags = convert_aflags(txt_buffer, aflags, read_version, read_flags);
	nptr++;
	if(attr_add(obj, txt_buffer, nptr, aflags) == -1) {
	  fprintf(stderr, txt_dberr, obj);
	  return(-1);
	}
      } else {			/* studly format */
	strlcpy(name, txt_buffer, MAXATTRNAME);

	if(fgets(txt_buffer, TXTBUFFSIZ, in) == (char *)NULL){
	  fprintf(stderr, txt_eoferr, obj);
	  return(-1);
	}
	aflags = (int)strtol(txt_buffer, &ptr, 10);
	if(ptr == txt_buffer){
	  fprintf(stderr, txt_numerr, obj);
	  return(-1);
	}
	aflags = convert_aflags(name, aflags, read_version, read_flags);

	if(read_version > 300) {	/* attr type */
	  char *mptr;

	  ptr++;			/* skip space */
	  atype = (short)strtol(ptr, &mptr, 10);
	  if(mptr == ptr) {
	    fprintf(stderr, txt_numerr, obj);
	    return(-1);
	  }
	} else
	  atype = ATTR_STRING;

	if(fgets(txt_buffer, TXTBUFFSIZ, in) == (char *)NULL){
	  fprintf(stderr, txt_eoferr, obj);
	  return(-1);
	}
	remove_newline(txt_buffer);

	switch(atype) {
	case ATTR_STRING:
	  if(attr_add(obj, name, txt_buffer, aflags) == -1){
	    fprintf(stderr, txt_dberr, obj);
	    return(-1);
	  }
	  break;
	case ATTR_LOCK:
	  getbool_ptr = txt_buffer;
	  if(attr_addlock(obj, name,
	  		  getbool_sub(obj, read_version, read_flags),
			  aflags) == -1) {
	    fprintf(stderr, txt_dberr, obj);
	    return(-1);
	  }
	  break;
	}
      }
    } else
      done++;
  }
  return(0);
}

/* ARGSUSED */
static int *convert_flags(oflags, read_version, read_flags)
    int oflags, read_version, read_flags;
{
  static int nflags[FLAGS_LEN];

  bzero((VOID *)nflags, sizeof(nflags));

  /* convert first generation flags to 64bits */

  if (read_version < 0) {	/* TinyMUD */
    switch(oflags & 0x7) {
    case 0:
      nflags[0] = TYP_ROOM;
      break;
    case 1:
      nflags[0] = TYP_THING;
      break;
    case 2:
      nflags[0] = TYP_EXIT;
      break;
    case 3:
      nflags[0] = TYP_PLAYER;
      break;
    case 6:
      nflags[0] = TYP_BAD;
      break;
    }

    if(oflags & 0x10)
      nflags[1] |= WIZARD;
    if(oflags & 0x20)
      nflags[1] |= LINK_OK;
    if(oflags & 0x40)
      nflags[1] |= DARK;
    if(oflags & 0x100)
      nflags[1] |= STICKY;
    if(oflags & 0x200)
      nflags[0] |= BUILDER;
    if(oflags & 0x400)
      nflags[1] |= HAVEN;
    if(oflags & 0x800)
      nflags[1] |= ABODE;
    if(oflags & 0x4000)
      nflags[0] |= ROBOT;
    if(oflags & 0x8000)
      nflags[0] |= CHOWN_OK;
    if(oflags & 0x20000)
      nflags[0] |= REVERSED_WHO;
    if(oflags & 0x40000)
      nflags[1] |= GOD;

    switch((oflags & 0x3000) >> 12) {
    case 0:
      gender[0] = '\0';
      break;
    case 1:
      strcpy(gender, "NEUTER");
      break;
    case 2:
      strcpy(gender, "FEMALE");
      break;
    case 3:
      strcpy(gender, "MALE");
      break;
    }
    return(nflags);
  }
  
  if (read_version < 100) {	/* 1.0 - 1.3 */
    switch (oflags & 0x0003) {
    case 0:
      nflags[0] = TYP_PLAYER;
      break;
    case 1:
      nflags[0] = TYP_THING;
      break;
    case 2:
      nflags[0] = TYP_ROOM;
      break;
    case 3:
      nflags[0] = TYP_EXIT;
      break;
    }
    if (oflags & 0x0004)
      nflags[1] |= STICKY;
    if (oflags & 0x0008)
      nflags[1] |= WIZARD;
    if (oflags & 0x0010)
      nflags[0] |= ROBOT;
    if (oflags & 0x0020)
      nflags[1] |= LINK_OK;
    if (read_version == 5) {	/* a -C database?? */
      if ((oflags & 0x0040) && ((oflags & 0x003) != 3))	/* !exit */
	nflags[1] |= DARK;
      else if (!(oflags & 0x0040))	/* exit */
	nflags[0] |= OBVIOUS;
      if (oflags & 0x0080)
	nflags[1] |= GOD;
      if (oflags & 0x0100)
	nflags[1] |= HAVEN;
      if (oflags & 0x0200)
	nflags[1] |= JUMP_OK;
      if (oflags & 0x0400)
	nflags[1] |= ABODE;
    } else {
      if (oflags & 0x0040)
	nflags[1] |= DARK;
      if (oflags & 0x0080)
	nflags[1] |= HAVEN;
      if (oflags & 0x0100)
	nflags[1] |= JUMP_OK;
      if (oflags & 0x0200)
	nflags[1] |= ABODE;
      if (oflags & 0x0400)
	nflags[0] |= BUILDER;
    }
  }

  /* convert second generation flags to 64bits */
  /* i no longer have flag maps for anything before version 300, so */
  /* there may be a few problems. */

  if ((read_version >= 100) && (read_version < 301)) {
    /* do types and type specific flags */
    switch(oflags & 0x00000007) {
    case 0:		/* player */
      nflags[0] = TYP_PLAYER;
      if(oflags & 0x00000200)
        nflags[0] |= ABODE;
      if(oflags & 0x00000400)
        nflags[0] |= BUILDER;
      if(oflags & 0x00000800)
        nflags[0] |= ENTER_OK;
      if(oflags & 0x00002000)
        nflags[0] |= ROBOT;
      if(oflags & 0x00004000)
      	nflags[0] |= GUEST;
      if(oflags & 0x00008000)
        nflags[0] |= RETENTIVE;
      if(oflags & 0x00010000)
        nflags[0] |= REVERSED_WHO;
      break;
    case 1:		/* thing */
      nflags[0] = TYP_THING;
      if(oflags & 0x00000200)
        nflags[0] |= ABODE;
      if(oflags & 0x00000400)
        nflags[0] |= BUILDING_OK;
      if(oflags & 0x00000800)
        nflags[0] |= ENTER_OK;
      if(oflags & 0x00004000)
      	nflags[0] |= CHOWN_OK;
      if(oflags & 0x00008000)
        nflags[0] |= DESTROY_OK;
      if(oflags & 0x00010000)
        nflags[0] |= PUPPET;
      break;
    case 2:		/* room */
      nflags[0] = TYP_ROOM;
      if(oflags & 0x00000200)
        nflags[0] |= ABODE;
      if(oflags & 0x00000400)
        nflags[0] |= BUILDING_OK;
      if(oflags & 0x00000800)
        nflags[0] |= ENTER_OK;
      if(oflags & 0x00004000)
      	nflags[0] |= CHOWN_OK;
      if(oflags & 0x00008000)
        nflags[0] |= DESTROY_OK;
      break;
    case 3:		/* exit */
      nflags[0] = TYP_EXIT;
      if(oflags & 0x00000200)
        nflags[0] |= OBVIOUS;
      if(oflags & 0x00000800)
        nflags[0] |= EXTERNAL;
      if(oflags & 0x00002000)
        nflags[0] |= ACTION;
      if(oflags & 0x00004000)
        nflags[0] |= CHOWN_OK;
      if(oflags & 0x00008000)
        nflags[0] |= DESTROY_OK;
      break;
    default:		/* program, or something */
      nflags[0] = TYP_BAD;
      break;
    }
    if(oflags & 0x00000008)
      nflags[1] |= WIZARD;
    if(oflags & 0x00000010)
      nflags[1] |= GOD;
    if(oflags & 0x00000020)
      nflags[1] |= LINK_OK;
    if(oflags & 0x00000080)
      nflags[1] |= HAVEN;
    if(oflags & 0x00000100)
      nflags[1] |= JUMP_OK;
    if(oflags & 0x00001000)
      nflags[1] |= STICKY;
    if(oflags & 0x00020000)
      nflags[1] |= PARENT_OK;
    if(oflags & 0x00040000)
      nflags[1] |= VISUAL;
    if(oflags & 0x00080000)
      nflags[1] |= AUDIBLE;
    if(oflags & 0x00100000)
      nflags[1] |= NOSPOOF;
    if(oflags & 0x00200000)
      nflags[1] |= HIDDEN;
    if(oflags & 0x00400000)
      nflags[1] |= HALT;
    if(oflags & 0x00800000)
      nflags[1] |= HAS_STARTUP;
    if(oflags & 0x01000000)
      nflags[1] |= LISTENER;
  }

  if (read_version == 105) {	/* gender */
    if (oflags & 0x01000000) {	/* male */
      strcpy(gender, "MALE");
    }
    if (oflags & 0x02000000) {	/* female */
      strcpy(gender, "FEMALE");
    }
    if (oflags & 0x04000000) {	/* it */
      strcpy(gender, "NEUTER");
    }
  }
  return (nflags);
}

int text_load(in, read_version, read_flags, read_total)
    FILE *in;
    int read_version, read_flags, read_total;
{
  int i, num, type, e;
  int *flags, rflags[FLAGS_LEN], ret, done = 0, garbage = 0;
  char *ptr;

  initialize_db(read_total + mudconf.slack);
  mudstat.slack = read_total + mudconf.slack;

  gender[0] = '\0';

  for(i = 0, num = 0; ((read_total < 0) || (i < read_total)) && !done; i++){
    if((i >= 1000) && ((i % 1000) == 0)) {
      fputc('o', stderr);
      fflush(stderr);
    } else if((i % 100) == 0) {
      fputc('.', stderr);
      fflush(stderr);
    }

    if(fgets(txt_buffer, TXTBUFFSIZ, in) == (char *)NULL) {
      fprintf(stderr, txt_eoferr, i);
      return(-1);
    }
    switch(txt_buffer[0]){
    case '#':
      num = (int)strtol(txt_buffer+1, &ptr, 10);
      if(ptr == (txt_buffer+1)){
	fprintf(stderr, txt_numerr, i);
	return(-1);
      }
      if(num != i) {
	fprintf(stderr, "Database out of sync at object #%d.\n", num);
	return(-1);
      }

      if(read_version >= 0) {		/* TeenyMUD */
        /* flags */
        if(fgets(txt_buffer, TXTBUFFSIZ, in) == (char *)NULL) {
	  fprintf(stderr, txt_eoferr, i);
	  return(-1);
        }
        rflags[0] = (int)strtol(txt_buffer, &ptr, 10);
        if(ptr == txt_buffer){
	  fprintf(stderr, txt_numerr, i);
	  return(-1);
        }
        if(read_version > 300) {
          char *nptr;

	  ptr++;
	  rflags[1] = (int)strtol(ptr, &nptr, 10);
	  if(nptr == ptr) {
	    fprintf(stderr, txt_numerr, i);
	    return(-1);
	  }
	  flags = rflags;
        } else
          flags = convert_flags(rflags[0], read_version, read_flags);

        type = typeof_flags(flags);
        ret = create_obj(type);
        if(ret != num){
	  fprintf(stderr, "Internal and input databases out of sync at #%d.\n",
		  num);
	  return(-1);
        }

        flags[1] = ((flags[1] & ~INTERNAL_FLAGS) |
		    (DSC_FLAG2(main_index[num]) & INTERNAL_FLAGS));
        for(e = 0; e < FLAGS_LEN; e++)
          (DSC_FLAGS(main_index[num]))[e] = flags[e];

        if(read_version > 299) {	/* sane database */
          /* quota */
          if(getnumber(in, num, QUOTA, read_version, read_flags) == -1)
	    return(-1);

          /* location */
          if(getnumber(in, num, LOC, read_version, read_flags) == -1)
	    return(-1);

          /* home/dropto/destinations */
	  if(type != TYP_EXIT) {
            if(getnumber(in, num, HOME, read_version, read_flags) == -1)
	      return(-1);
	  } else {
	    if(getarray(in, num, DESTS, read_version, read_flags) == -1)
	      return(-1);
	  }

          /* owner */
          if(getnumber(in, num, OWNER, read_version, read_flags) == -1)
	    return(-1);

          /* contents */
          if(getnumber(in, num, CONTENTS, read_version, read_flags) == -1)
	    return(-1);

          /* exits */
          if(getnumber(in, num, EXITS, read_version, read_flags) == -1)
	    return(-1);

          /* rooms/pennies */
          if(getnumber(in, num, ROOMS, read_version, read_flags) == -1)
	    return(-1);

          /* next */
          if(getnumber(in, num, NEXT, read_version, read_flags) == -1)
	    return(-1);

          /* timestamp */
          if(getnumber(in, num, TIMESTAMP, read_version, read_flags) == -1)
	    return(-1);

          /* createstamp */
          if(getnumber(in, num, CREATESTAMP, read_version, read_flags) == -1)
	    return(-1);

          /* uses */
          if(getnumber(in, num, USES, read_version, read_flags) == -1)
	    return(-1);

          /* parent */
          if(getnumber(in, num, PARENT, read_version, read_flags) == -1)
	    return(-1);

	  /* charges */
	  if(getnumber(in, num, CHARGES, read_version, read_flags) == -1)
	    return(-1);
	
	  /* semaphores */
	  if(getnumber(in, num, SEMAPHORES, read_version, read_flags) == -1)
	    return(-1);

	  /* cost */
	  if(getnumber(in, num, COST, read_version, read_flags) == -1)
	    return(-1);

	  /* queue */
	  if(getnumber(in, num, QUEUE, read_version, read_flags) == -1)
	    return(-1);

          /* name */
	  if(getstring(in, num, NAME, read_version, read_flags) == -1)
	    return(-1);

	  if(read_version < 301) {
            /* lock */
            if(getbool(in, num, LOCK, read_version, read_flags) == -1)
	      return(-1);

            /* enter lock */
            if(getbool(in, num, ENLOCK, read_version, read_flags) == -1)
	      return(-1);

            /* page lock */
            if(getbool(in, num, PGLOCK, read_version, read_flags) == -1)
	      return(-1);

            /* teleport in lock */
            if(getbool(in, num, TELINLOCK, read_version, read_flags) == -1)
	      return(-1);

            /* teleport out lock */
            if(getbool(in, num, TELOUTLOCK, read_version, read_flags) == -1)
	      return(-1);
	  }

	  /* attributes */
	  if(getattributes(in, num, read_version, read_flags) == -1)
	    return(-1);
        } else {		/* insane database */
	  /* name (except for 1.4) */
	  if(read_version > 199) {	/* 2.0a */
	    if(getstring(in, num, NAME, read_version, read_flags) == -1)
	      return(-1);
	  } else if(read_version < 100) {	/* 1.x */
	    if(fgets(txt_buffer, TXTBUFFSIZ, in) == (char *)NULL){
	      fprintf(stderr, txt_eoferr, i);
	      return(-1);
	    }
	    remove_newline(txt_buffer);

	    if(type == TYP_PLAYER) {
	      for(ptr = txt_buffer; *ptr && !isspace(*ptr); ptr++);
	      *ptr++ = '\0';

	      if((set_str_elt(num, NAME, txt_buffer) == -1) ||
	         (attr_add(num, PASSWORD, cryptpwd(ptr), PASSWORD_FLGS) == -1)){
	        fprintf(stderr, txt_dberr, i);
	        return(-1);
	      }
	    } else {
	      if(set_str_elt(num, NAME, txt_buffer) == -1) {
	        fprintf(stderr, txt_dberr, i);
	        return(-1);
	      }
	    }
	  }

	  /* next */
	  if(getnumber(in, num, NEXT, read_version, read_flags) == -1)
	    return(-1);

	  /* either site, quota, or pennies */
	  if((read_version < 100) && (read_flags & DB_SITE)
	     && (type == TYP_PLAYER)) {	/* site */
	    long addr;

	    if(fgets(txt_buffer, TXTBUFFSIZ, in) == (char *)NULL){
	      fprintf(stderr, txt_eoferr, i);
	      return(-1);
	    }
	    addr = htonl(strtol(txt_buffer, &ptr, 10));
	    if(ptr == txt_buffer) {
	      fprintf(stderr, txt_numerr, i);
	      return(-1);
	    }
	    snprintf(txt_buffer, TXTBUFFSIZ, "%ld.%ld.%ld.%ld",
		     (addr >> 24) & 0xff, (addr >> 16) & 0xff,
		     (addr >> 8) & 0xff, addr & 0xff);
	    if(attr_add(num, SITE, txt_buffer, SITE_FLGS) == -1) {
	      fprintf(stderr, txt_dberr, i);
	      return(-1);
	    }
	  } else if(read_version > 99) {		/* quota */
	    if(getnumber(in, num, QUOTA, read_version, read_flags) == -1)
	      return(-1);
	  } else if(read_version < 99) {		/* pennies */
	    if(type == TYP_PLAYER) {
	      if(getnumber(in, num, PENNIES, read_version, read_flags) == -1)
	        return(-1);
	    } else {
	      if(skip_line(in, num) == -1)
	        return(-1);
	    }
	  }

	  /* location */
	  if(getnumber(in, num, LOC, read_version, read_flags) == -1)
	    return(-1);

	  /* home/dropto/destinations */
	  if(type != TYP_EXIT) {
	    if(getnumber(in, num, HOME, read_version, read_flags) == -1)
	      return(-1);
	  } else {
	    if(getarray(in, num, DESTS, read_version, read_flags) == -1)
	      return(-1);
	  }

	  /* owner */
	  if(getnumber(in, num, OWNER, read_version, read_flags) == -1)
	    return(-1);

	  /* 2.0a parent */
	  if(read_version > 199) {
	    if(getnumber(in, num, PARENT, read_version, read_flags) == -1)
	      return(-1);
	  }

	  /* contents */
	  if(getnumber(in, num, CONTENTS, read_version, read_flags) == -1)
	    return(-1);

	  /* exits */
	  if(getnumber(in, num, EXITS, read_version, read_flags) == -1)
	    return(-1);

	  /* rooms or pennies */
	  if(read_version > 99){
	    if(getnumber(in, num, ROOMS, read_version, read_flags) == -1)
	      return(-1);
	  }

	  /* timestamp */
	  if((read_version == 2) || (read_version > 3)){
	    if(getnumber(in, num, TIMESTAMP, read_version, read_flags) == -1)
	      return(-1);
	  } else {
	    if(skip_line(in, num) == -1)
	      return(-1);
	  }

	  /* createstamp and use count */
	  if(read_version > 199){
	    if(getnumber(in, num, CREATESTAMP, read_version, read_flags) == -1)
	      return(-1);
	    if(getnumber(in, num, USES, read_version, read_flags) == -1)
	      return(-1);
	  }

	  /* lock */
	  if(getbool(in, num, LOCK, read_version, read_flags) == -1)
	    return(-1);

	  /* 1.4 and 2.0a locks */
	  if(read_version > 99){
	    if(getbool(in, num, ENLOCK, read_version, read_flags) == -1)
	      return(-1);
	    if(getbool(in, num, PGLOCK, read_version, read_flags) == -1)
	      return(-1);
	  }

	  /* 2.0a locks */
	  if(read_version > 199) {
	    if(getbool(in, num, TELINLOCK, read_version, read_flags) == -1)
	      return(-1);
	    if(getbool(in, num, TELOUTLOCK, read_version, read_flags) == -1)
	      return(-1);
	  }

	  /* 1.x strings */
	  if(read_version < 100) {
	    /* suc/osuc or oxteleport/oteleport */
	    if((read_flags & DB_ARRIVE) && (type == TYP_PLAYER)){
	      if(getattr(in, num, OXTELEPORT, read_version, read_flags) == -1)
	        return(-1);
	      if(getattr(in, num, OTELEPORT, read_version, read_flags) == -1)
	        return(-1);
	    } else {
	      if(getattr(in, num, SUC, read_version, read_flags) == -1)
	        return(-1);
	      if(getattr(in, num, OSUC, read_version, read_flags) == -1)
	        return(-1);
	    }

	    /* fail/ofail */
	    if(getattr(in, num, FAIL, read_version, read_flags) == -1)
	      return(-1);
	    if(getattr(in, num, OFAIL, read_version, read_flags) == -1)
	      return(-1);

	    /* drop/odrop or kill/okill */
	    if(read_version > 2) {
	      if(getattr(in, num, (type == TYP_PLAYER) ? KILL : DROP,
	    	         read_version, read_flags) == -1)
	        return(-1);
	      if(getattr(in, num, (type == TYP_PLAYER) ? OKILL : ODROP,
	    	         read_version, read_flags) == -1)
	        return(-1);
	    }

	    /* oxtel/otel */
	    if(read_version == 5) {
	      if(getattr(in, num, OXTELEPORT, read_version, read_flags) == -1)
	        return(-1);
	      if(getattr(in, num, OTELEPORT, read_version, read_flags) == -1)
	        return(-1);
	    }

	    /* desc */
	    if(getattr(in, num, DESC, read_version, read_flags) == -1)
	      return(-1);

	    /* sex */
	    if(getattr(in, num, SEX, read_version, read_flags) == -1)
	      return(-1);

	    /* address */
	    if(read_version == 5) {
	      if(getattr(in, num, "Address", read_version, read_flags) == -1)
	        return(-1);
	    }
	  } else if(read_version < 200) {	/* 1.4 strings */
	    if(getstrings(in, num, read_version, read_flags) == -1)
	      return(-1);
	  } else {			/* 2.0a attributes */
	    if(getattributes(in, num, read_version, read_flags) == -1)
	      return(-1);
	  }
        }
      } else if(read_version < 0) {		/* TinyMUD */
        char *nptr;

        /* Snarf up the object data. */

	/* Name. */
	if(fgets(tinyobj.name, sizeof(tinyobj.name), in) == (char *)NULL) {
	  fprintf(stderr, txt_eoferr, i);
	  return(-1);
	}
	remove_newline(tinyobj.name);

	/* Description. */
	if(fgets(tinyobj.description,
	   sizeof(tinyobj.description), in) == (char *)NULL) {
	  fprintf(stderr, txt_eoferr, i);
	  return(-1);
	}
	remove_newline(tinyobj.description);
	
	/* Location. */
	if(fgets(txt_buffer, TXTBUFFSIZ, in) == (char *)NULL) {
	  fprintf(stderr, txt_eoferr, i);
	  return(-1);
	}
	tinyobj.location = (int)strtol(txt_buffer, &nptr, 10);
	if(nptr == txt_buffer) {
	  fprintf(stderr, txt_numerr, i);
	  return(-1);
	}

	/* Contents. */
	if(fgets(txt_buffer, TXTBUFFSIZ, in) == (char *)NULL) {
	  fprintf(stderr, txt_eoferr, i);
	  return(-1);
	}
	tinyobj.contents = (int)strtol(txt_buffer, &nptr, 10);
	if(nptr == txt_buffer) {
	  fprintf(stderr, txt_numerr, i);
	  return(-1);
	}

	/* Exits. */
	if(fgets(txt_buffer, TXTBUFFSIZ, in) == (char *)NULL) {
	  fprintf(stderr, txt_eoferr, i);
	  return(-1);
	}
	tinyobj.exits = (int)strtol(txt_buffer, &nptr, 10);
	if(nptr == txt_buffer) {
	  fprintf(stderr, txt_numerr, i);
	  return(-1);
	}

	/* Next. */
	if(fgets(txt_buffer, TXTBUFFSIZ, in) == (char *)NULL) {
	  fprintf(stderr, txt_eoferr, i);
	  return(-1);
	}
	tinyobj.next = (int)strtol(txt_buffer, &nptr, 10);
	if(nptr == txt_buffer) {
	  fprintf(stderr, txt_numerr, i);
	  return(-1);
	}

	/* Lock. */
	if(fgets(txt_buffer, TXTBUFFSIZ, in) == (char *)NULL) {
	  fprintf(stderr, txt_eoferr, i);
	  return(-1);
	}
	getbool_ptr = txt_buffer;
	tinyobj.lock = getbool_sub(i, read_version, read_flags);

	/* Fail. */
	if(fgets(tinyobj.fail, sizeof(tinyobj.fail), in) == (char *)NULL) {
	  fprintf(stderr, txt_eoferr, i);
	  return(-1);
	}
	remove_newline(tinyobj.fail);

	/* Success. */
	if(fgets(tinyobj.succ, sizeof(tinyobj.succ), in) == (char *)NULL) {
	  fprintf(stderr, txt_eoferr, i);
	  return(-1);
	}
	remove_newline(tinyobj.succ);

	/* OFail. */
	if(fgets(tinyobj.ofail, sizeof(tinyobj.ofail), in) == (char *)NULL) {
	  fprintf(stderr, txt_eoferr, i);
	  return(-1);
	}
	remove_newline(tinyobj.ofail);

	/* OSuccess. */
	if(fgets(tinyobj.osucc, sizeof(tinyobj.osucc), in) == (char *)NULL) {
	  fprintf(stderr, txt_eoferr, i);
	  return(-1);
	}
	remove_newline(tinyobj.osucc);

	/* Owner. */
	if(fgets(txt_buffer, TXTBUFFSIZ, in) == (char *)NULL) {
	  fprintf(stderr, txt_eoferr, i);
	  return(-1);
	}
	tinyobj.owner = (int)strtol(txt_buffer, &nptr, 10);
	if(nptr == txt_buffer) {
	  fprintf(stderr, txt_numerr, i);
	  return(-1);
	}

	/* Pennies. */
	if(fgets(txt_buffer, TXTBUFFSIZ, in) == (char *)NULL) {
	  fprintf(stderr, txt_eoferr, i);
	  return(-1);
	}
	tinyobj.pennies = (int)strtol(txt_buffer, &nptr, 10);
	if(nptr == txt_buffer) {
	  fprintf(stderr, txt_numerr, i);
	  return(-1);
	}

	/* Flags. */
	if(fgets(txt_buffer, TXTBUFFSIZ, in) == (char *)NULL) {
	  fprintf(stderr, txt_eoferr, i);
	  return(-1);
	}
	tinyobj.flags = (int)strtol(txt_buffer, &nptr, 10);
	if(nptr == txt_buffer) {
	  fprintf(stderr, txt_numerr, i);
	  return(-1);
	}

	/* Password. */
	if(fgets(tinyobj.password,
	   sizeof(tinyobj.password), in) == (char *)NULL) {
	  fprintf(stderr, txt_eoferr, i);
	  return(-1);
	}
	remove_newline(tinyobj.password);

	/* Do some conversions. */
	if(tinyobj.flags & 0x8)
	  tinyobj.lock = negate_bool(tinyobj.lock);

	/* Process flags and create the object. */
        flags = convert_flags(tinyobj.flags, read_version, read_flags);

        type = typeof_flags(flags);
        ret = create_obj(type);
        if(ret != num){
	  fprintf(stderr, "Internal and input databases out of sync at #%d.\n",
		  num);
	  return(-1);
        }

        flags[1] = ((flags[1] & ~INTERNAL_FLAGS) |
		    (DSC_FLAG2(main_index[num]) & INTERNAL_FLAGS));
        for(e = 0; e < FLAGS_LEN; e++)
          (DSC_FLAGS(main_index[num]))[e] = flags[e];

	/* Set the data. */

	DSC_OWNER(main_index[num]) = tinyobj.owner;

	DSC_NEXT(main_index[num]) = tinyobj.next;

	switch(typeof_flags(flags)) {
	case TYP_PLAYER:
	case TYP_THING:
	  DSC_HOME(main_index[num]) = tinyobj.exits;

	  if((set_int_elt(num, LOC, tinyobj.location) == -1)
	     || (set_int_elt(num, PENNIES, tinyobj.pennies) == -1)) {
	    fprintf(stderr, txt_dberr, i);
	    return(-1);
	  }
	  break;
	case TYP_ROOM:
	  DSC_DROPTO(main_index[num]) = tinyobj.location;

	  if(set_int_elt(num, EXITS, tinyobj.exits) == -1) {
	    fprintf(stderr, txt_dberr, i);
	    return(-1);
	  }
	  break;
	case TYP_EXIT:
	  if(tinyobj.location != -1) {
	    int iptr[2];

	    iptr[0] = 1;
	    iptr[1] = tinyobj.location;

	    if(set_array_elt(num, DESTS, iptr) == -1) {
	      fprintf(stderr, txt_dberr, i);
	      return(-1);
	    }
	  }
	  break;
	}

	if(set_str_elt(num, NAME, tinyobj.name) == -1) {
	  fprintf(stderr, txt_dberr, i);
	  return(-1);
	}
	if(tinyobj.description[0]) {
	  if(attr_add(num, DESC, tinyobj.description, DEFATTR_FLGS) == -1) {
	    fprintf(stderr, txt_dberr, i);
	    return(-1);
	  }
	}
	if(tinyobj.fail[0]) {
	  if(attr_add(num, FAIL, tinyobj.fail, DEFATTR_FLGS) == -1) {
	    fprintf(stderr, txt_dberr, i);
	    return(-1);
	  }
	}
	if(tinyobj.ofail[0]) {
	  if(attr_add(num, OFAIL, tinyobj.ofail, DEFATTR_FLGS) == -1) {
	    fprintf(stderr, txt_dberr, i);
	    return(-1);
	  }
	}
	if(tinyobj.succ[0]) {
	  if(attr_add(num, SUC, tinyobj.succ, DEFATTR_FLGS) == -1) {
	    fprintf(stderr, txt_dberr, i);
	    return(-1);
	  }
	}
	if(tinyobj.osucc[0]) {
	  if(attr_add(num, OSUC, tinyobj.osucc, DEFATTR_FLGS) == -1) {
	    fprintf(stderr, txt_dberr, i);
	    return(-1);
	  }
	}
	if(tinyobj.password[0]) {
	  if(attr_add(num, PASSWORD, cryptpwd(tinyobj.password),
	  	      PASSWORD_FLGS) == -1) {
	    fprintf(stderr, txt_dberr, i);
	    return(-1);
	  }
	}
	if(tinyobj.lock != (struct boolexp *)NULL) {
	  if(attr_addlock(num, LOCK, tinyobj.lock, DEFATTR_FLGS) == -1) {
	    fprintf(stderr, txt_dberr, i);
	    return(-1);
	  }
	}
	stamp(num, STAMP_USED);
      }

      if(gender[0]) {
	if(attr_add(num, SEX, gender, DEFATTR_FLGS) == -1) {
	  fprintf(stderr, txt_dberr, i);
	  return(-1);
	}
	gender[0] = '\0';
      }
      break;
    case '@':		/* garbage */
      garbage++;
      mudstat.total_objects++;
      main_index[i] = (struct dsc *)NULL;
      break;
    case '*':		/* the end */
      done++;
      break;
    default:
      fprintf(stderr, "Database format error near object #%d.\n", i);
      return(-1);
    }

    if((i % 100) == 0)
      cache_trim();
  }

  mudstat.garbage_count += garbage;
  fputc('\n', stderr);
  fflush(stderr);

  return(0);
}
