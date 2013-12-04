/* boolexp.c */

#include "config.h"

/*
 *		       This file is part of TeenyMUD II.
 *	      Portions Copyright(C) 1993, 1994, 1995 by Jason Downs.
 *			      All rights reserved.
 *
 * The following specifies terms and conditions of redistribution,
 * and limitations of warranty.
 */

/* -*-C-*-

Portions of this file are:

Copyright (c) 1989, 1990 by David Applegate, James Aspnes, Timothy Freeman,
		        and Bennet Yee.

This material was developed by the above-mentioned authors.
Permission to copy this software, to redistribute it, and to use it
for any purpose is granted, subject to the following restrictions and
understandings.

1. Any copy made of this software must include this copyright notice
in full.

2. Users of this software agree to make their best efforts (a) to
return to the above-mentioned authors any improvements or extensions
that they make, so that these may be included in future releases; and
(b) to inform the authors of noteworthy uses of this software.

3. All materials developed as a consequence of the use of this
software shall duly acknowledge such use, in accordance with the usual
standards of acknowledging credit in academic research.

4. The authors have made no warrantee or representation that the
operation of this software will be error-free, and the authors are
under no obligation to provide any services, by way of maintenance,
update, or otherwise.

5. In conjunction with products arising from the use of this material,
there shall be no use of the names of the authors, of Carnegie-Mellon
University, nor of any adaptation thereof in any advertising,
promotional, or sales literature without prior written consent from
the authors and Carnegie-Mellon University in each case.
*/

#include <stdio.h>
#include <sys/types.h>
#ifdef HAVE_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif			/* HAVE_STRING_H */
#include <ctype.h>
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif			/* HAVE_STDLIB_H */

#include "conf.h"
#include "teeny.h"
#include "flaglist.h"
#include "externs.h"

#define ATTR_NAME 0
#define ATTR_CONT 1

static int eval_const_lock _ANSI_ARGS_((int, int, int));
static int eval_flag_lock _ANSI_ARGS_((int, int, int *));
static int eval_attr_lock _ANSI_ARGS_((int, int, char *, char *));

static int eval_const_lock(player, cause, obj)
    int player, cause, obj;
{
  int contents;

  if(player == obj)
    return(1);
  if(get_int_elt(player, CONTENTS, &contents) == -1) {
    logfile(LOG_ERROR, "eval_boolexp: bad contents list on #%d\n", player);
    return(0);
  }
  return(member(obj, contents));
}

static int eval_flag_lock(player, cause, flags)
    int player, cause, *flags;
{
  int contents;

  /* there are some holes in the following. */
  if(Flags(player, flags, "eval_boolexp"))
    return(1);
  if(get_int_elt(player, CONTENTS, &contents) == -1) {
    logfile(LOG_ERROR, "eval_boolexp: bad contents list on #%d\n", player);
    return(0);
  }

  while(contents != -1) {
    if(Flags(contents, flags, "eval_boolexp"))
      return(1);
    if(get_int_elt(contents, NEXT, &contents) == -1) {
      logfile(LOG_ERROR, "eval_boolexp: bad next element on #%d\n", contents);
      return(0);
    }
  }
  return(0);
}

static int eval_attr_lock(player, cause, name, cont)
    int player, cause;
    char *name, *cont;
{
  int contents, ret, aflags, asrc;
  char *adata;

  ret = attr_get_parent(player, name, &adata, &aflags, &asrc);
  if(ret == -1) {
    logfile(LOG_ERROR, "eval_boolexp: bad attribute on #%d\n", player);
    return(0);
  } else if(adata && quick_wild(cont, adata)) {
    return(1);
  }

  if(get_int_elt(player, CONTENTS, &contents) == -1) {
    logfile(LOG_ERROR, "eval_boolexp: bad contents list on #%d\n", player);
    return(0);
  }
  while(contents != -1) {
    ret = attr_get_parent(contents, name, &adata, &aflags, &asrc);
    if(ret == -1) {
      logfile(LOG_ERROR, "eval_boolexp: bad attribute on #%d\n", contents);
      return(0);
    } else if(adata && quick_wild(cont, adata)) {
      return(1);
    }

    if(get_int_elt(contents, NEXT, &contents) == -1) {
      logfile(LOG_ERROR, "eval_boolexp: bad next element on #%d\n", contents);
      return(0);
    }
  }
  return(0);
}

int islocked(player, cause, thing, name)
    int player, cause, thing;
    char *name;
{
  struct boolexp *lock;
  int aflags, asource;

  if(attr_getlock_parent(thing, name, &lock, &aflags, &asource) == -1) {
    logfile(LOG_ERROR, "islocked: bad lock element on #%d\n", thing);
    return(1);
  }
  return(!eval_boolexp(player, cause, lock));
}

int eval_boolexp(player, cause, b)
    int player, cause;
    struct boolexp *b;
{
  if (b == (struct boolexp *) NULL) {
    return(1);
  } else {
    switch (b->type) {
    case BOOLEXP_AND:
      return (eval_boolexp(player, cause, b->sub1)
	      && eval_boolexp(player, cause, b->sub2));
    case BOOLEXP_OR:
      return (eval_boolexp(player, cause, b->sub1)
	      || eval_boolexp(player, cause, b->sub2));
    case BOOLEXP_NOT:
      return(!eval_boolexp(player, cause, b->sub1));
    case BOOLEXP_CONST:
      return (eval_const_lock(player, cause, (b->dat).thing));
    case BOOLEXP_FLAG:
      return (eval_flag_lock(player, cause, (b->dat).flags));
    case BOOLEXP_ATTR:
      return (eval_attr_lock(player, cause, (b->dat).atr[ATTR_NAME],
      			     (b->dat).atr[ATTR_CONT]));
    default:
      logfile(LOG_ERROR, "eval_boolexp: bad boolexp type (%d)\n", b->type);
      return(0);
    }
  }
}

/* If the parser returns (struct boolexp *)NULL, you lose */
static char *parsebuf;
static int parse_player, parse_cause;

#define skip_whitespace()	while(*parsebuf && isspace(*parsebuf)) \
					parsebuf++;

static struct boolexp *parse_boolexp_E _ANSI_ARGS_((void));

static struct boolexp *parse_boolexp_D()
{
  struct boolexp *b;
  char buf[MEDBUFFSIZ];
  char *ptr;

  skip_whitespace();
  /* this could, literally, be anything. */

  ptr = buf;
  /* copy the current 'token' into our buffer. */
  while(*parsebuf && (*parsebuf != '&') && (*parsebuf != '|') &&
        (*parsebuf != ')') && ((ptr - buf) < MEDBUFFSIZ)) {
    *ptr++ = *parsebuf++;
  }
  /* remove any trailing whitespace. */
  *ptr-- = '\0';
  while(isspace(*ptr))
    *ptr-- = '\0';

  /* set up the return value. */
  b = boolexp_alloc();
  b->sub1 = b->sub2 = (struct boolexp *)NULL;

  /* figure out what they're trying to do. */
  if(strncasecmp(buf, "flag:", 5) == 0) {
    register FlagList *flist;

    b->type = BOOLEXP_FLAG;
    for(flist = GenFlags; flist->name; flist++) {
      if(strncasecmp(buf+5, flist->name, flist->matlen) == 0) {
        ((b->dat).flags)[flist->word] = flist->code;
	return(b);
      }
    }
    for(flist = PlayerFlags; flist->name; flist++) {
      if(strncasecmp(buf+5, flist->name, flist->matlen) == 0) {
        ((b->dat).flags)[flist->word] = flist->code;
	return(b);
      }
    }
    for(flist = ThingFlags; flist->name; flist++) {
      if(strncasecmp(buf+5, flist->name, flist->matlen) == 0) {
        ((b->dat).flags)[flist->word] = flist->code;
	return(b);
      }
    }
    for(flist = RoomFlags; flist->name; flist++) {
      if(strncasecmp(buf+5, flist->name, flist->matlen) == 0) {
        ((b->dat).flags)[flist->word] = flist->code;
	return(b);
      }
    }
    for(flist = ExitFlags; flist->name; flist++) {
      if(strncasecmp(buf+5, flist->name, flist->matlen) == 0) {
        ((b->dat).flags)[flist->word] = flist->code;
	return(b);
      }
    }

    notify_player(parse_player, parse_cause, parse_player,
		  "No such flag.", 0);
    boolexp_free(b);
    return((struct boolexp *)NULL);
  } else if((buf[0] != ':') && (ptr = strchr(buf, ':')) != (char *)NULL) {
    b->type = BOOLEXP_ATTR;

    *ptr++ = '\0';
    if(*ptr == '\0') {
      boolexp_free(b);
      return((struct boolexp *)NULL);
    }

    (b->dat).atr[ATTR_NAME] = (char *)ty_strdup(buf, "parse_boolexp");
    (b->dat).atr[ATTR_CONT] = (char *)ty_strdup(ptr, "parse_boolexp");
    return(b);
  }

  /* default */
  b->type = BOOLEXP_CONST;

  /* do that match */
  (b->dat).thing = resolve_object(parse_player, parse_cause, buf,
				  RSLV_NOISY|RSLV_SPLATOK);
  if ((b->dat).thing == -1) {
    boolexp_free(b);
    return((struct boolexp *)NULL);
  } else
    return(b);
}

/* F -> (E); F -> !F; F -> D */
static struct boolexp *parse_boolexp_F()
{
  struct boolexp *b;

  skip_whitespace();
  switch (*parsebuf) {
  case '(':
    parsebuf++;
    b = parse_boolexp_E();
    skip_whitespace();
    if ((b == (struct boolexp *) NULL) || (*parsebuf++ != ')')) {
      boolexp_free(b);
      return((struct boolexp *) NULL);
    } else {
      return(b);
    }
    /* break; */
  case '!':
    parsebuf++;
    b = boolexp_alloc();
    b->type = BOOLEXP_NOT;
    b->sub2 = (struct boolexp *)NULL;

    b->sub1 = parse_boolexp_F();
    if (b->sub1 == (struct boolexp *) NULL) {
      boolexp_free(b);
      return((struct boolexp *) NULL);
    } else {
      return(b);
    }
    /* break */
  default:
    return(parse_boolexp_D());
    /* break */
  }
}

/* T -> F; T -> F & T */
static struct boolexp *parse_boolexp_T()
{
  struct boolexp *b;
  struct boolexp *b2;

  if ((b = parse_boolexp_F()) == (struct boolexp *) NULL) {
    return(b);
  } else {
    skip_whitespace();
    if (*parsebuf == '&') {
      parsebuf++;

      b2 = boolexp_alloc();
      b2->type = BOOLEXP_AND;
      b2->sub1 = b;
      if ((b2->sub2 = parse_boolexp_T()) == (struct boolexp *) NULL) {
	boolexp_free(b2);
	return((struct boolexp *) NULL);
      } else {
	return(b2);
      }
    } else {
      return(b);
    }
  }
}

/* E -> T; E -> T | E */
static struct boolexp *parse_boolexp_E()
{
  struct boolexp *b;
  struct boolexp *b2;

  if ((b = parse_boolexp_T()) == (struct boolexp *) NULL) {
    return(b);
  } else {
    skip_whitespace();
    if (*parsebuf == '|') {
      parsebuf++;

      b2 = boolexp_alloc();
      b2->type = BOOLEXP_OR;
      b2->sub1 = b;
      if ((b2->sub2 = parse_boolexp_E()) == (struct boolexp *) NULL) {
	boolexp_free(b2);
	return((struct boolexp *) NULL);
      } else {
	return(b2);
      }
    } else {
      return(b);
    }
  }
}

struct boolexp *parse_boolexp(player, cause, buf)
    int player, cause;
    char *buf;
{
  if ((buf == (char *)NULL) || (buf[0] == '\0'))
    return((struct boolexp *)NULL);

  parsebuf = buf;
  parse_player = player;
  parse_cause = cause;
  return(parse_boolexp_E());
}

static char boolexp_buf[MEDBUFFSIZ];
static char *buftop;

static void unparse_boolexp1 _ANSI_ARGS_((int, int, struct boolexp *, int));

static void unparse_boolexp1(player, cause, b, outer_type)
    int player, cause;
    struct boolexp *b;
    int outer_type;
{
  if (b == (struct boolexp *) NULL) {
    strcpy(buftop, "*UNLOCKED*");
    buftop += strlen(buftop);
  } else {
    switch (b->type) {
    case BOOLEXP_AND:
      if (outer_type == BOOLEXP_NOT) {
	*buftop++ = '(';
      }
      unparse_boolexp1(player, cause, b->sub1, b->type);
      *buftop++ = '&';
      unparse_boolexp1(player, cause, b->sub2, b->type);
      if (outer_type == BOOLEXP_NOT) {
	*buftop++ = ')';
      }
      break;
    case BOOLEXP_OR:
      if ((outer_type == BOOLEXP_NOT) || (outer_type == BOOLEXP_AND)) {
	*buftop++ = '(';
      }
      unparse_boolexp1(player, cause, b->sub1, b->type);
      *buftop++ = '|';
      unparse_boolexp1(player, cause, b->sub2, b->type);
      if ((outer_type == BOOLEXP_NOT) || (outer_type == BOOLEXP_AND)) {
	*buftop++ = ')';
      }
      break;
    case BOOLEXP_NOT:
      *buftop++ = '!';
      unparse_boolexp1(player, cause, b->sub1, b->type);
      break;
    case BOOLEXP_CONST:
      strcpy(buftop, display_name(player, cause, (b->dat).thing));
      buftop += strlen(buftop);
      break;
    case BOOLEXP_FLAG: {
      register FlagList *flist;
      register int matched = 0;

      for(flist = GenFlags; flist->name; flist++) {
	if(flist->code == ((b->dat).flags)[flist->word]) {
	  strcpy(buftop, "flag:");
	  strcat(buftop, flist->name);
	  matched++;
	}
      }
      if(!matched) {
	for(flist = PlayerFlags; flist->name; flist++) {
	  if(flist->code == ((b->dat).flags)[flist->word]) {
	    strcpy(buftop, "flag:");
	    strcat(buftop, flist->name);
	    matched++;
	  }
	}
      }
      if(!matched) {
	for(flist = RoomFlags; flist->name; flist++) {
	  if(flist->code == ((b->dat).flags)[flist->word]) {
	    strcpy(buftop, "flag:");
	    strcat(buftop, flist->name);
	    matched++;
	  }
	}
      }
      if(!matched) {
	for(flist = ThingFlags; flist->name; flist++) {
	  if(flist->code == ((b->dat).flags)[flist->word]) {
	    strcpy(buftop, "flag:");
	    strcat(buftop, flist->name);
	    matched++;
	  }
	}
      }
      if(!matched) {
	for(flist = ExitFlags; flist->name; flist++) {
	  if(flist->code == ((b->dat).flags)[flist->word]) {
	    strcpy(buftop, "flag:");
	    strcat(buftop, flist->name);
	    matched++;
	  }
	}
      }
      if(!matched)
	strcpy(buftop, "flag:*UNKNOWN*");
      buftop += strlen(buftop);
      }  break;
    case BOOLEXP_ATTR:
      strcpy(buftop, "(");
      strcat(buftop, (b->dat).atr[ATTR_NAME]);
      strcat(buftop, ":");
      strcat(buftop, (b->dat).atr[ATTR_CONT]);
      strcat(buftop, ")");
      buftop += strlen(buftop);
      break;
    default:
      strcpy(buftop, "*BAD LOCK*");
      buftop += strlen(buftop);
      logfile(LOG_ERROR, "unparse_boolexp: bad bool type (%d)\n", b->type);
      break;
    }
  }
}

char *unparse_boolexp(player, cause, b)
    int player, cause;
    struct boolexp *b;
{
  buftop = boolexp_buf;
  unparse_boolexp1(player, cause, b, BOOLEXP_CONST);	/* no outer type */
  *buftop++ = '\0';

  return(boolexp_buf);
}
