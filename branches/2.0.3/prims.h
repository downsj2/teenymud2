/* prims.h */

#include "config.h"

/*
 *		       This file is part of TeenyMUD II.
 *		 Copyright(C) 1993, 1994, 1995 by Jason Downs.
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

#ifndef __PRIMS_H

#include "hash/hash.h"

typedef struct Prim {
  char *name;
  VOID (*func)();

  int nargs;
  int flags;
  int perms;
} Prim;

/* function flags */
#define PRIM_VARARGS	0x0001

/* function permissions */
#define PRIM_GOD	0x0001		/* must be god */
#define PRIM_WIZ	0x0002		/* must be wiz */

extern Hash_Table Prim_Table;

#ifdef __STDC__
#define PRIM(_p)        VOID _p( \
                                int player, int cause, int source, int argc, \
                                char *argv[], char *buffer, size_t blen)
#else
#define PRIM(_p)        VOID _p(player, cause, source, argc, argv, buffer, \
								blen) \
                                int player, cause, source, argc; \
                                char *argv[], *buffer; \
				size_t blen;
#endif

#define __PRIM_H
#endif			/* __PRIM_H */
