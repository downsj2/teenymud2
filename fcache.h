/* fcache.h */

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

#ifndef __FCACHE_H

#define MAX_FCACHE_NAME	64

typedef struct fcache_line {
  char *ptr;

  char *line;			/* line */
  char str[1];		/* zero length line */
  
  struct fcache_line *next;	/* next line */
} FCACHE;

typedef struct fcache_head {
  time_t stamp;			/* modification time */

  struct fcache_line *lines;	/* lines */
} FHEAD;

/* macros */
#define fcache_next(x)	(x->next)
#define fcache_read(x)	(x->ptr)

/* prototypes */
extern FCACHE *fcache_open _ANSI_ARGS_((char *, off_t *));

#define __FCACHE_H
#endif				/* __FCACHE_H */
