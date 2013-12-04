/* config.h */

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

/*
 * This header file defines certain constants that can be changed,
 * as well as some system and compiler dependant settings.
 */

#ifndef __CONFIG_H

/* read in the autoconf settings. */
#include "autoconf.h"

#if (SIZEOF_OFF_T == 0)
#undef SIZEOF_OFF_T
#define SIZEOF_OFF_T SIZEOF_LONG
#endif

#if (SIZEOF_INT < 4)
 #error TeenyMUD requires at least 32bit sized integers.
#endif

#ifdef __STDC__
#define _ANSI_ARGS_(_x)	_x
#else
#define _ANSI_ARGS_(_x)	()
#endif	/* __STDC__ */

#ifdef HAVE_VOID_PTR
#define VOID	void
#else
#define VOID	char
#endif				/* HAVE_VOID_PTR */

/* version number. */
#define VERSION		"2.0.7p0"

/* maximum length of a player name */
#define MAXPNAMELEN	16
/* maximum length of a player password */
#define MAXPWDLEN	128
/* maximum length of an attribute name */
#define MAXATTRNAME	128
/* maximum arguments to a primitive and such */
#define MAXARGS		128
/* maximum number of destinations an exit may have */
#define MAXLINKS	32

#define BACKUP_SUFFIX ".BAK"

#define __CONFIG_H
#endif				/* __CONFIG_H */
