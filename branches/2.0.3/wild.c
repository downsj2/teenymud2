/* wild.c */

#include "config.h"

/*
 *		       This file is part of TeenyMUD II.
 *		Portions Copyright(C) 1994, 1995 by Jason Downs.
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

/* wild.c - wildcard routines
 *
 * Stolen from MUSH and hacked up, April 1994.
 * (Well, not really stolen. Supposedly, it was under the 'GNU copyleft'.)
 *
 * Written by T. Alexander Popiel, 24 June 1993
 * Last modified by T. Alexander Popiel, 19 August 1993
 *
 * Thanks go to Andrew Molitor for debugging
 * Thanks also go to Rich $alz for code to benchmark against
 *
 * Copyright (c) 1993 by T. Alexander Popiel
 * This code is hereby placed under GNU copyleft,
 * see copyright.h for details.
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

#define FIXCASE(a) (to_lower(a))
#define EQUAL(a,b) ((a == b) || (FIXCASE(a) == FIXCASE(b)))
#define NOTEQUAL(a,b) ((a != b) && (FIXCASE(a) != FIXCASE(b)))

static char **arglist;	/* argument return space */
static int numargs;	/* argument return maximum size */
static int totargs;	/* total number of parsed arguments */

static int wild1 _ANSI_ARGS_((char *, char *, int));

int quick_wild_prefix(tstr, dstr)
    char *tstr, *dstr;
{
  return((strncasecmp(tstr, dstr, strlen(tstr)) == 0)
         || quick_wild(tstr, dstr));
}

/* ---------------------------------------------------------------------------
 * quick_wild: do a wildcard match, without remembering the wild data.
 *
 * This routine will cause crashes if fed NULLs instead of strings.
 */
int quick_wild(tstr, dstr)
char	*tstr, *dstr;
{
	while (*tstr != '*') {
		switch (*tstr) {
		case '?':
			/* Single character match.  Return false if at
			 * end of data.
			 */
			if (!*dstr) return 0;
			break;
		case '\\':
			/* Escape character.  Move up, and force literal
			 * match of next character.
			 */
			tstr++;
			/* FALL THROUGH */
		default:
			/* Literal character.  Check for a match.
			 * If matching end of data, return true.
			 */
			if (NOTEQUAL(*dstr, *tstr)) return 0;
			if (!*dstr) return 1;
		}
		tstr++;
		dstr++;
	}

	/* Skip over '*'. */

	tstr++;

	/* Return true on trailing '*'. */

	if (!*tstr) return 1;

	/* Skip over wildcards. */

	while ((*tstr == '?') || (*tstr == '*')) {
		if (*tstr == '?') {
			if (!*dstr) return 0;
			dstr++;
		}
		tstr++;
	}

	/* Skip over a backslash in the pattern string if it is there. */

	if (*tstr == '\\') tstr++;

	/* Return true on trailing '*'. */

	if (!*tstr) return 1;

	/* Scan for possible matches. */

	while (*dstr) {
		if (EQUAL(*dstr, *tstr) &&
		    quick_wild(tstr+1, dstr+1)) return 1;
		dstr++;
	}
	return 0;
}

/* ---------------------------------------------------------------------------
 * wild1: INTERNAL: do a wildcard match, remembering the wild data.
 *
 * DO NOT CALL THIS FUNCTION DIRECTLY - DOING SO MAY RESULT IN
 * SERVER CRASHES AND IMPROPER ARGUMENT RETURN.
 *
 * Side Effect: this routine modifies the 'arglist' static global
 * variable.
 */
static int wild1(tstr, dstr, arg)
char	*tstr, *dstr;
int	arg;
{
char	*datapos;
int	argpos, numextra;

	while (*tstr != '*') {
		switch (*tstr) {
		case '?':
			/* Single character match.  Return false if at
			 * end of data.
			 */
			if (!*dstr) return 0;

			/* allocate the argument */
			arglist[arg] = (char *)ty_malloc((size_t)2,
							 "wild1.arglist");
			arglist[arg][0] = *dstr;
			arglist[arg][1] = '\0';
			arg++;
			totargs++;

			/* Jump to the fast routine if we can. */

			if (arg >= numargs) return quick_wild(tstr+1, dstr+1);
			break;
		case '\\':
			/* Escape character.  Move up, and force literal
			 * match of next character.
			 */
			tstr++;
			/* FALL THROUGH */
		default:
			/* Literal character.  Check for a match.
			 * If matching end of data, return true.
			 */
			if (NOTEQUAL(*dstr, *tstr)) return 0;
			if (!*dstr) return 1;
		}
		tstr++;
		dstr++;
	}

	/* If at end of pattern, slurp the rest, and leave. */

	if (!tstr[1]) {
		/* allocate the argument */
		arglist[arg] = ty_strdup(dstr, "wild1.arglist");
		totargs++;
		return 1;
	}

	/* Remember current position for filling in the '*' return. */

	datapos = dstr;
	argpos = arg;

	/* Scan forward until we find a non-wildcard. */

	do {
		if (argpos < arg) {
			/* Fill in arguments if someone put another '*'
			 * before a fixed string.
			 */
			arglist[argpos] = (char *)ty_strdup("",
							    "wild1.arglist");
			argpos++;
			totargs++;

			/* Jump to the fast routine if we can. */

			if (argpos >= numargs) return quick_wild(tstr, dstr);

			/* Fill in any intervening '?'s */

			while (argpos < arg) {
				/* allocate the argument */
				arglist[argpos] = (char *)ty_malloc((size_t)2,
							"wild1.arglist");
				arglist[argpos][0] = *datapos;
				arglist[argpos][1] = '\0';
				datapos++;
				argpos++;
				totargs++;

				/* Jump to the fast routine if we can. */

				if (argpos >= numargs)
					return quick_wild(tstr, dstr);
			}
		}

		/* Skip over the '*' for now... */

		tstr++;
		arg++;

		/* Skip over '?'s for now... */

		numextra = 0;
		while (*tstr == '?') {
			if (!*dstr) return 0;
			tstr++;
			dstr++;
			arg++;
			numextra++;
		}
	} while (*tstr == '*');

	/* Skip over a backslash in the pattern string if it is there. */

	if (*tstr == '\\') tstr++;

	/* Check for possible matches.  This loop terminates either at
	 * end of data (resulting in failure), or at a successful match.
	 */
	while (1) {

		/* Scan forward until first character matches. */

		if (*tstr)
			while (NOTEQUAL(*dstr, *tstr)) {
				if (!*dstr) return 0;
				dstr++;
			}
		else 
			while (*dstr) dstr++;

		/* The first character matches, now.  Check if the rest
		 * does, using the fastest method, as usual.
		 */
		if (!*dstr ||
		    ((arg<numargs) ? wild1(tstr+1, dstr+1, arg)
				   : quick_wild(tstr+1, dstr+1))) {

			/* Found a match!  Fill in all remaining arguments.
			 * First do the '*'...
			 */
			/* allocate the argument */
			arglist[argpos] = (char *)ty_malloc(
					((dstr - datapos) - numextra) + 1,
					"wild1.arglist");
			ty_strncpy(arglist[argpos], datapos,
				   (dstr - datapos) - numextra);
			datapos = dstr - numextra;
			argpos++;
			totargs++;

			/* Fill in any trailing '?'s that are left. */

			while (numextra) {
				if (argpos >= numargs) return 1;

				/* allocate the argument */
				arglist[argpos] = (char *)ty_malloc((size_t)2,
							"wild1.arglist");
				arglist[argpos][0] = *datapos;
				arglist[argpos][1] = '\0';
				datapos++;
				argpos++;
				totargs++;
				numextra--;
			}

			/* It's done! */

			return 1;
		} else {
			dstr++;
		}
	}
}

/* ---------------------------------------------------------------------------
 * wild: do a wildcard match, remembering the wild data.
 *
 * This routine will cause crashes if fed NULLs instead of strings.
 *
 * Side Effect: this routine modifies the 'arglist' and 'numargs'
 * static global variables.
 */
int wild(tstr, dstr, limit, argc, args)
char	*tstr, *dstr;
int	limit, *argc;
char	*args[];
{
int	i, value;

	/* Initialize the return array. */

	*argc = 0;
	for (i=0; i<limit; i++) args[i] = (char *)NULL;

	/* Do fast match. */

	while ((*tstr != '*') && (*tstr != '?')) {
		if (*tstr == '\\') tstr++;
		if (NOTEQUAL(*dstr, *tstr)) return 0;
		if (!*dstr) return 1;
		tstr++;
		dstr++;
	}

	/* Put stuff in globals for quick recursion. */

	arglist = args;
	numargs = limit;
	totargs = 0;

	/* Do the match. */

	value = wild1(tstr, dstr, 0);

	/* If we matched nothing, make sure the array is empty. */
	if(!value) {
		for(i = 0; i < limit; i++) {
			if(args[i] != (char *)NULL) {
				ty_free((VOID *)args[i]);
				args[i] = (char *)NULL;
			}
		}
	} else
		*argc = totargs;

	return value;
}

int filter_match(str, filter)
    register char *str, *filter;
{
  register char *buf, *p;

  buf = (char *) alloca(strlen(filter) + 1);
  if(buf == (char *)NULL)
    panic("filter_match(): stack allocation failed\n");
  strcpy(buf, filter);

  while (buf[0]) {
    for (p = buf; *p && (*p != '|'); p++);
    if (*p == '|')
      *p++ = '\0';
    if (quick_wild(buf, str))
      return (1);
    buf = p;
  }
  return (0);
}
