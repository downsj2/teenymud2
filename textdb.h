/* textdb.h */

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

#ifndef _TEXTDB_H

/* database flags */
#define DB_SITE		0x0001	/* for compatibility */
#define DB_ARRIVE	0x0002	/* for compatibility */
#define DB_DESTARRAY	0x0004	/* exits have a destination array */
#define DB_PARENTS	0x0008	/* objects have multiple parents */
#define DB_IMMUTATTRS	0x0010	/* attributes have immutable flag */
#define DB_LARGETIME	0x0020	/* time_t is larger than 32bits */

#define DB_GFILTER	0x1000	/* don't output garbage */

#define OUTPUT_VERSION	301
#if SIZEOF_TIME_T > 4
#define OUTPUT_FLAGS	(DB_DESTARRAY|DB_IMMUTATTRS|DB_LARGETIME)
#else
#define OUTPUT_FLAGS	(DB_DESTARRAY|DB_IMMUTATTRS)
#endif

#define TXTBUFFSIZ	LARGEBUFFSIZ

/* externs */
extern char txt_buffer[];
extern char txt_eoferr[];
extern char txt_numerr[];
extern char txt_dberr[];
extern char txt_boolerr[];

/* from textdb.c */
extern int text_version _ANSI_ARGS_((FILE *, int *, int *, int *));
extern int skip_line _ANSI_ARGS_((FILE *, int));

/* from textin.c */
extern int text_load _ANSI_ARGS_((FILE *, int, int, int));

/* from textout.c */
extern void text_dump _ANSI_ARGS_((FILE *, int, int));

#define _TEXTDB_H
#endif				/* _TEXTDB_H */
