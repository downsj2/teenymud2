/* flaglist.h */

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

#ifndef __FLAGLIST_H

/* flag table defines */
typedef struct flaglist {
  char *name;			/* full name */
  short matlen;
  int code;			/* flag value */
  char chr;			/* single letter 'name' */
  short word;			/* which word is this flag in */
  short perm;			/* permissions for setting */
} FlagList;

/* object flag permissions */

#define PERM_GOD	0x01	/* only settable by god */
#define PERM_WIZ	0x02	/* only settable by wiz */
#define PERM_VISIBLE	0x08	/* visible to all */
#define PERM_DARK	0x10	/* special */
#define PERM_INTERNAL	0x20	/* not changeable (by users) at all */
#define PERM_PLAYER	0x40	/* only settable by a player */

/* Attribute flags */
typedef struct aflaglist {
  char *name;
  short matlen;
  int code;
  char chr;
} AFlagList;

extern FlagList PlayerFlags[];
extern int PlayerFlags_count;

extern FlagList RoomFlags[];
extern int RoomFlags_count;

extern FlagList ThingFlags[];
extern int ThingFlags_count;

extern FlagList ExitFlags[];
extern int ExitFlags_count;

extern FlagList GenFlags[];
extern int GenFlags_count;

extern AFlagList AFlags[];
extern int AFlags_count;

extern int match_flaglist_name _ANSI_ARGS_((const char *, FlagList[], int));
extern int match_flaglist_chr _ANSI_ARGS_((const char, FlagList[], int));

#define __FLAGLIST_H
#endif			/* __FLAGLIST_H */
