/* attrs.h */

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

#ifndef __ATTRS_H_

/* how the outside world accesses the entire attribute list. */

/* the attribute struct. note the difference from teenydb.h! */

#define ATTR_STRING	0
#define ATTR_LOCK	1

struct attr {
  short type;

  char *name;
  union {
    char *str;
    struct boolexp *lock;
  } dat;

  int flags;
};

/* the attribute search tracker struct. */
struct asearch {
  int elem;
  struct attr *curr;
};

/* prototypes from attributes.c */
extern struct attr *attr_first _ANSI_ARGS_((int, struct asearch *));
extern struct attr *attr_next _ANSI_ARGS_((int, struct asearch *));

#define __ATTRS_H_
#endif			/* __ATTRS_H_ */
