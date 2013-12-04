/* expr.c */

#include "config.h"

/*
 *		       This file is part of TeenyMUD II.
 *	    Copyright(C) 1995 by Jason Downs.  All rights reserved.
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
#include "expr.h"
#include "externs.h"

/*
 * The TeenyMUD II math expression evaluator, version 2.
 */

static int isfloat _ANSI_ARGS_((char *));
static struct expr_val expr_sub _ANSI_ARGS_((char *, char **));

static int isfloat(str)
    char *str;
{
  while(*str && isspace(*str))
    str++;
  if((*str == '.') && isdigit(str[1]))
    return(1);

  while(*str && isdigit(*str))
    str++;
  if((*str == '.') && isdigit(str[1]))
    return(1);
  return(0);
}

/* expression evaluator */
static struct expr_val expr_sub(str, nptr)
    char *str;
    char **nptr;
{
  struct expr_val val;
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
	  if((val = expr_parse(str, &error))) {
	    switch(val.type) {
	    case EXPRVAL_INT:
	      (val.val).ival = -(val.val).ival;
	      break;
	    case EXPRVAL_FLOAT:
	      (val.val).fval = -(val.val).fval;
	      break;
	    }
	    if(error)
	      *nptr = (char *)NULL;
	  }
	}
      } else {
	if(isdouble(str)) {
	  (val.val).fval = strtod(str, nptr);
	  if(*nptr == str)
	    *nptr = (char *)NULL;
	  else {
	    val.type = EXPRVAL_FLOAT;
	    (val.val).fval = -(val.val).fval;
	  }
	} else {
	  (val.val).ival = (int)strtol(str, nptr, 10);
	  if(*nptr == str)
	    *nptr = (char *)NULL;
	  else {
	    val.type = EXPRVAL_INT;
	    (val.val).ival = -(val.val).ival;
	  }
	}
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
	  if((val = expr_parse(str, &error))) {
	    switch(val.type) {
	    case EXPRVAL_INT:
	      (val.val).ival = ~(val.val).ival;
	      break;
	    case EXPRVAL_FLOAT:
	      error++;
	      break;
	    }
	    if(error)
	      *nptr = (char *)NULL;
	  }
        }
      } else {
	if(isdouble(str))
	  *nptr = (char *)NULL;
	else {
	  (val.val).ival = (int)strtol(str, nptr, 10);
	  if(*nptr == str)
	    *nptr = (char *)NULL;
	  else {
	    val.type = EXPRVAL_INT;
	    (val.val).ival = ~(val.val).ival;
	  }
	}
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
	  if((val = expr_parse(str, &error))) {
	    switch(val.type) {
	    case EXPRVAL_INT:
	      (val.val).ival = !(val.val).ival;
	      break;
	    case EXPRVAL_FLOAT:
	      error++;
	      break;
	    }
	    if(error)
	      *nptr = (char *)NULL;
	  }
        }
      } else {
	if(isdouble(str))
	  *nptr = (char *)NULL;
	else {
	  (val.val).ival = (int)strtol(str, nptr, 10);
	  if(*nptr == str)
	    *nptr = (char *)NULL;
	  else {
	    val.type = EXPRVAL_INT;
	    (val.val).ival = !(val.val).ival;
	  }
	}
      }
    }
    break;
  default:
    if(isdouble(str)) {
      (val.val).fval = strtod(str, nptr);
      if(*nptr == str)
	*nptr = (char *)NULL;
      else
	val.type = EXPRVAL_FLOAT;
    } else if(isdigit(str[0])) {
      (val.val).ival = strtol(str, nptr, 10);
      if(*nptr == str)
	*nptr = (char *)NULL;
      else
	val.type = EXPRVAL_INT;
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
      left = expr_sub(str, &nptr);
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

    right = expr_sub(str, &nptr);
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
