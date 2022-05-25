/* parse.c */

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

#ifdef HAVE_ALLOCA_H
#include <alloca.h>
#endif	/* HAVE_ALLOCA_H */

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif			/* HAVE_STDLIB_H */
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
#include "prims.h"
#include "externs.h"

static char *exec_primitive _ANSI_ARGS_((int, int, int, char *, int, int,
					 char *[]));
static int can_exec _ANSI_ARGS_((int, int));
static long expr_val _ANSI_ARGS_((char *, char **));

char *exec(player, cause, source, string, flags, argc, argv)
    int player, cause, source;
    char *string;
    int flags, argc;
    char *argv[];
{
  char buffer[MEDBUFFSIZ+2];
  char *ptr = buffer;
  char *tptr, *nptr, *eptr;
  char *attr, anm[3], nbuf[MAXPNAMELEN + 3], selfbuf[MAXPNAMELEN + 6], *name;
  char cbuf[64], pbuf[64], sbuf[64], *wstr;
  int aflags, asource, gender, powner;
  int increment = 1;

  if (!*string)
    return((char *)NULL);

  /* make a copy of string to work on */
  wstr = (char *)alloca(strlen(string)+1);
  if(wstr == (char *)NULL)
    panic("exec(): stack allocation failed\n");
  strcpy(wstr, string);

  if(get_int_elt(player, OWNER, &powner) == -1)
    powner = -1;

  /* set up pronoun strings, just in case */
  gender = genderof(cause);
  if(get_str_elt(cause, NAME, &name) == -1)
    name = "???";		/* XXX */
  snprintf(nbuf, sizeof(nbuf), "%s's", name);
  snprintf(selfbuf, sizeof(selfbuf), "%s self", name);
  snprintf(cbuf, sizeof(cbuf), "#%d", cause);
  snprintf(pbuf, sizeof(pbuf), "#%d", player);
  snprintf(sbuf, sizeof(sbuf), "#%d", source);

  char *subjective[4] = {name, "they", "she", "he"};
  char *objective[4] = {name, "them", "her", "him"};
  char *possessive[4] = {nbuf, "theirs", "her", "his"};
  char *reflexive[4] = {selfbuf, "themself", "herself", "himself"};
  char *absolute[4] = {nbuf, "theirs", "hers", "his"};

  while(*wstr && (ptr - buffer) < MEDBUFFSIZ) {
    switch(*wstr) {
    case '\\':	/* single char escape */
      if (wstr[1]) {
	wstr++;
      }
      *ptr++ = *wstr++;
      break;
    case '{':	/* full scale quoted */
      wstr++;
      if ((tptr = parse_to(wstr, '{', '}'))) {
        if(flags & EXEC_PRIM)		/* Put the brace back in. */
	  *ptr++ = '{';
	while(*wstr && (ptr - buffer) < MEDBUFFSIZ) {
	  if (*wstr == '\\' && wstr[1])
	    wstr++;
	  *ptr++ = *wstr++;
	}
	if(flags & EXEC_PRIM)		/* Put the brace back in. */
	  *ptr++ = '}';
	wstr = tptr;
      } else
	*ptr++ = '{';
      break;
    case '[':	/* oh oh, primitive */
      wstr++;
      if ((nptr = parse_to(wstr, '[', ']'))) {
	if ((eptr = exec(player, cause, source, wstr, (flags|EXEC_PRIM),
	    argc, argv))) {
	  for(tptr = eptr; *tptr && (ptr - buffer) < MEDBUFFSIZ;
	      *ptr++ = *tptr++);
	  ty_free((VOID *)eptr);
	} else {
	  *ptr++ = '[';
	  if ((ptr - buffer) < MEDBUFFSIZ)
	    *ptr++ = ']';
	}
	wstr = nptr;
      } else
	*ptr++ = '[';
      break;
    case '$':	/* variable sub */
      wstr++;
      if(*wstr == '(') {
        wstr++;

        for(nptr = wstr; (*nptr != '\0') && (*nptr != ')'); nptr++);
        if(*nptr == ')') {
          *nptr++ = '\0';
	
	  eptr = (powner == -1) ? (char *)NULL : var_get(powner, wstr);
	  if(eptr != (char *)NULL) {
	    for(tptr = eptr; *tptr && (ptr - buffer) < MEDBUFFSIZ;
	        *ptr++ = *tptr++);
	  }
	  wstr = nptr;
        } else {
	  if ((ptr - buffer) < MEDBUFFSIZ)
            *ptr++ = '$';
	  if ((ptr - buffer) < MEDBUFFSIZ)
	    *ptr++ = '(';
	}
      } else 
        *ptr++ = '$';
      break;
    case '%':	/* 'pronoun' sub */
      if (wstr[1] && (wstr[1] != '%')) {
        int i = 0;

	wstr++;
	tptr = (char *)NULL;
        if(!isdigit(*wstr)) {
	  anm[0] = '%';
	  anm[1] = *wstr;
	  anm[2] = '\0';
	  if ((attr_get_parent(cause, anm, &attr, &aflags, &asource) != -1)
	      && (attr != (char *)NULL) &&
	      can_see_attr(player, cause, player, aflags)) { /* override */
	    tptr = attr;
	  }
	}
	if(tptr == (char *)NULL) {
	  switch(*wstr) {
	  case 'l':
	    tptr = "\r\n";
	    break;
	  case 't':
	    tptr = "\t";
	    break;
	  case 'n':
	  case 'N':
	    tptr = name;
	    break;
	  case 's':
	  case 'S':
	    tptr = subjective[gender];
	    break;
	  case 'o':
	  case 'O':
	    tptr = objective[gender];
	    break;
	  case 'p':
	  case 'P':
	    tptr = possessive[gender];
	    break;
	  case 'a':
	  case 'A':
	    tptr = absolute[gender];
	    break;
	  case 'r':
	  case 'R':
	    tptr = reflexive[gender];
	    break;
	  case '#':
	    tptr = cbuf;
	    break;
	  case '!':
	    tptr = pbuf;
	    break;
	  case '@':
	    tptr = sbuf;
	    break;
	  case '\0':
	    tptr = "";
	    increment = 0;
	    break;
	  default:
	    if(isdigit(*wstr) && argc && (argv != (char **)NULL)) {
	      int argnum = (int)strtol(wstr, &wstr, 10);
	      increment = 0;
	      if((argnum >= 0) && (argnum < argc) && 
	      	 (argv[argnum] != (char *)NULL)) {
	        tptr = argv[argnum];
	      } else
	        tptr = "";
	    } else
	      tptr = "";
	    break;
	  }
	}

	while (tptr[i] && (ptr - buffer) < MEDBUFFSIZ) {
	  if ((i == 0) && isupper(*wstr)) {
	    *ptr++ = to_upper(tptr[i]);
	  } else {
	    *ptr++ = tptr[i];
	  }
	  i++;
	}
	if(increment)
	  wstr++;
	else
	  increment = 1;
      } else {
	if (wstr[1] == '%')
	  wstr++;
	*ptr++ = *wstr++;
      }
      break;
    default:		/* just copy it */
      *ptr++ = *wstr++;
    }
  }
  *ptr = '\0';

  if(flags & EXEC_PRIM) {
    ptr = (char *)ty_strdup(exec_primitive(player, cause, source, buffer,
    					   flags, argc, argv), "exec.return");
  } else {
    ptr = (char *)ty_strdup(buffer, "exec.return");
  }

  return(ptr);
}
  
void parse_copy(dest, src, len)
    char *dest, *src;
    int len;
{
  char *ptr = dest;

  while(*src && isspace(*src))
    src++;
  while(*src && (ptr - dest) < len) {
    if ((*src == '\\') && src[1])
      src++;
    *ptr++ = *src++;
  }
  if((ptr > dest) && isspace(ptr[-1])) {
    while((ptr >= dest) && isspace(ptr[-1]))
      ptr--;
  }
  *ptr = '\0';
}

/* a simpler argv parser, for exec() arguments and such. */
void parse_sargv(str, limit, argc, argv)
    char *str;
    int limit, *argc;
    char *argv[];
{
  char *ptr, *qtr;

  *argc = 0;
  if((str == (char *)NULL) || (str[0] == '\0'))
    return;

  while(limit) {
    /* i don't want to overwrite str, so... */
    for(ptr = str; *ptr && !isspace(*ptr); ptr++);
    qtr = (char *)ty_malloc((ptr - str) + 1, "parse_sargv");
    bcopy((VOID *)str, (VOID *)qtr, (ptr - str));
    qtr[(ptr - str)] = '\0';

    argv[*argc++] = qtr;
    while(*ptr && isspace(*ptr))
      ptr++;
    if(*ptr == '\0')
      break;

    str = ptr;
    limit--;
  }
}

void free_argv(argc, argv)
    int argc;
    char *argv[];
{
  int i;

  for(i = 0; i < argc; i++) {
    ty_free((VOID *)argv[i]);
  }
}

static char *exec_primitive(player, cause, source, string, flags, eargc, eargv)
    int player, cause, source;
    char *string;
    int flags, eargc;
    char *eargv[];
{
  int argc;
  char *argv[MAXARGS];
  static char buffer[MEDBUFFSIZ];
  char *wstr, *nptr;
  Prim *fp, t;

  if (!string || !*string)
    return((char *)NULL);

  wstr = string;

  for(argc = 0; argc < MAXARGS; argc++) {
    argv[argc] = (char *)NULL;
  }
  argc = 0;

  while(*wstr && argc < MAXARGS) {
    /* kill leading spaces */
    while(*wstr && isspace(*wstr))
      wstr++;
    if(!*wstr) {
      argc--;
      break;
    }

    switch(*wstr) {
    case '{':
      wstr++;
      if((nptr = parse_to(wstr, '{', '}'))) {
	argv[argc] = (char *)ty_malloc(strlen(wstr)+1, "exec_primitive.argv");
	parse_copy(argv[argc], wstr, strlen(wstr));
      } else {		/* syntax error */
	argc--;
	goto error;
      }
      wstr = nptr;
      break;
    case '"':
      wstr++;
      if((nptr = parse_to(wstr, '\0', '"')))
	argv[argc] = exec(player, cause, source, wstr, (flags & ~EXEC_PRIM),
			  eargc, eargv);
      else {		/* syntax error */
	argc--;
	goto error;
      }
      wstr = nptr;
      break;
    default:
      for(nptr = wstr; *nptr && !isspace(*nptr); nptr++);
      if(*nptr)
        *nptr++ = '\0';
      argv[argc] = (char *)ty_malloc(strlen(wstr)+1, "exec_primitive.argv");
      parse_copy(argv[argc], wstr, strlen(wstr));
      wstr = nptr;
      break;
    }
    argc++;
  }

  /* If we've reached here, we've parsed the arg list. Yay. */
  t.name = argv[0];
  fp = bsearch(&t, PTable, PTable_count, sizeof(Prim), prims_cmp);
  if (fp == NULL) {
    snprintf(buffer, sizeof(buffer), "#-1 NO SUCH PRIMITIVE \"%s\"", argv[0]);
    free_argv(argc, argv);
    return(buffer);
  }
  if (((fp->nargs+1) != argc) && !(fp->flags & PRIM_VARARGS)) {
    snprintf(buffer, sizeof(buffer),
	     "#-1 PRIMITIVE \"%s\" EXPECTS %d ARGUMENTS", argv[0], fp->nargs);
    free_argv(argc, argv);
    return(buffer);
  } else if((fp->nargs != -1) && (argc <= fp->nargs) &&
  		(fp->flags & PRIM_VARARGS)) {
    snprintf(buffer, sizeof(buffer),
	     "#-1 PRIMITIVE \"%s\" EXPECTS AT LEAST %d ARGUMENTS",
    	     argv[0], fp->nargs);
    free_argv(argc, argv);
    return(buffer);
  }
  if(!can_exec(player, fp->perms)) {
    snprintf(buffer, sizeof(buffer), "#-1 %s: PERMISSION DENIED", argv[0]);
    free_argv(argc, argv);
    return(buffer);
  }

  buffer[0] = '\0';
  (fp->func)(player, cause, source, argc, argv, buffer, sizeof(buffer));
  return(buffer);

error:
  if(argv[0]) {		/* we have a primitive name, yay. */
    snprintf(buffer, sizeof(buffer), "#-1 SYNTAX ERROR NEAR \"%s\"", argv[0]);
  } else
    strcpy(buffer, "#-1 SYNTAX ERROR");
  free_argv(argc, argv);
  return(buffer);
}

static int can_exec(player, perms)
    int player, perms;
{
  if((perms & PRIM_GOD) && !isGOD(player))
    return(0);
  if((perms & PRIM_WIZ) && !isWIZARD(player))
    return(0);
  return(1);
}

int genderof(player)
    int player;
{
  char *str;
  int aflags, source;

  if ((attr_get_parent(player, SEX, &str, &aflags, &source) == -1)
      || (str == (char *)NULL))
    return (GENDER_UNSPECIFIED);

  switch (to_lower(str[0])) {
  case 'f':
    return (GENDER_FEMALE);
  case 'm':
    return (GENDER_MALE);
  default:
    return (GENDER_NONBINARY);
  }
}

/* expression evaluator */
static long expr_val(str, nptr)
    char *str;
    char **nptr;
{
  long val = 0;
  int error;

  switch(str[0]) {
  case '(':
    str++;
    if((*nptr = parse_to(str, '(', ')'))) {
      val = expr_parse(str, &error);
      if(error)
	*nptr = (char *)NULL;
    }
    break;
  case '-':
    str++;
    if(!*str)
      *nptr = (char *)NULL;
    else {
      if(*str == '(') {		/* awe, fuck */
	str++;
	if((*nptr = parse_to(str, '(', ')'))) {
	  if((val = expr_parse(str, &error)))
	    val = -val;
	  if(error)
	    *nptr = (char *)NULL;
	}
      } else {
	val = strtol(str, nptr, 10);
	if(*nptr == str)
	  *nptr = (char *)NULL;
	else
	  val = -val;
      }
    }
    break;
  case '~':
    str++;
    if(!*str)
      *nptr = (char *)NULL;
    else {
      if(*str == '(') {		/* argh */
	str++;
	if((*nptr = parse_to(str, '(', ')'))) {
	  if((val = expr_parse(str, &error)))
	    val = ~val;
	  if(error)
	    *nptr = (char *)NULL;
        }
      } else {
	val = strtol(str, nptr, 10);
	if(*nptr == str)
	  *nptr = (char *)NULL;
	else
	  val = ~val;
      }
    }
    break;
  case '!':
    str++;
    if(!*str)
      *nptr = (char *)NULL;
    else {
      if(*str == '(') {		/* die, scum */
	str++;
	if((*nptr = parse_to(str, '(', ')'))) {
	  if((val = expr_parse(str, &error)))
	    val = !val;
	  if(error)
	    *nptr = (char *)NULL;
        }
      } else {
	val = strtol(str, nptr, 10);
	if(*nptr == str)
	  *nptr = (char *)NULL;
	else
	  val = !val;
      }
    }
    break;
  default:
    if(isdigit(str[0])) {
      val = strtol(str, nptr, 10);
      if(*nptr == str)
	*nptr = (char *)NULL;
    } else {
      *nptr = (char *)NULL;
    }
    break;
  }

  return(val);
}

long expr_parse(str, error)
    char *str;
    int *error;
{
  long left = 0, right = 0;
  int first = 1;
  char *expr, *nptr;

  if (!str) {
    *error = 1;
    return(0);
  } else
    *error = 0;

  while(*str && !*error) {
    while(*str && isspace(*str))
      str++;
    if(!*str)
      break;
    
    if(first) {
      left = expr_val(str, &nptr);
      if(!nptr) {
	*error = 1;
	break;
      }
      for(str = nptr; *str && isspace(*str); str++);
      if(!*str)
	break;
      first = 0;
    }
    expr = str;
    while(*str && strchr("*/%+-<>=!&^|", *str))
      str++;
    while(*str && isspace(*str))
      str++;
    if(!*str || (!isdigit(*str) && !strchr("(-~!", *str))) {
      *error = 1;
      break;
    }

    right = expr_val(str, &nptr);
    if(!nptr) {
      *error = 1;
      break;
    }
    str = nptr;

    /* evaluate */
    switch(expr[0]) {
    case '*':	/* multiply */
      left *= right;
      break;
    case '/':	/* divide */
      left /= right;
      break;
    case '%':	/* modulo */
      left %= right;
      break;
    case '+':	/* addition */
      left += right;
      break;
    case '-':	/* substraction */
      left -= right;
      break;
    case '<':
      switch(expr[1]) {
      case '<':		/* shift */
	left = left << right;
	break;
      case '=':		/* lesser than or equal */
	left = (left <= right);
	break;
      default:		/* lesser than */
	left = (left < right);
        break;
      }
      break;
    case '>':
      switch(expr[1]) {
      case '>':		/* shift */
	left = left >> right;
	break;
      case '=':		/* greater than or equal */
	left = (left >= right);
	break;
      default:		/* greater than */
	left = (left > right);
	break;
      }
      break;
    case '=':
      if(expr[1] != '=') {
	*error = 1;
	break;
      }
      left = (left == right);
      break;
    case '!':
      if(expr[1] != '=') {
	*error = 1;
	break;
      }
      left = (left != right);
      break;
    case '^':
      left ^= right;
      break;
    case '&':
      switch(expr[1]) {
      case '&':
	left = (left && right);
	break;
      default:
	left &= right;
	break;
      }
      break;
    case '|':
      switch(expr[1]) {
      case '|':
	left = (left || right);
	break;
      default:
	left |= right;
	break;
      }
      break;
    default:
      *error = 1;
    }
  }

  return(left);
}
