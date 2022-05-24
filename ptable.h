/* ptable.h */

#include "config.h"

/*
 *		       This file is part of TeenyMUD II.
 *	    Copyright(C) 1995, 2013, 2022 by Jason Downs.  All rights reserved.
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

#ifndef __PTABLE_H

enum palias_flags { PTABLE_ALIAS, PTABLE_UNALIAS };

extern void ptable_add _ANSI_ARGS_((int, char *));
extern void ptable_alias _ANSI_ARGS_((int, char *, enum palias_flags));
extern void ptable_delete _ANSI_ARGS_((int));
extern char *ptable_lookup _ANSI_ARGS_((int));
extern int match_player _ANSI_ARGS_((char *));
extern void ptable_init _ANSI_ARGS_((void));

#define __PTABLE_H
#endif			/* __PTABLE_H */
