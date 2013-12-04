/* misc.c */

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
 * the Free Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,
 * MA 02111-1307, USA.
 *
 */

/* Returns non-NULL if one string is contained within the other. */
#ifndef HAVE_STRCASESTR
char *strcasestr(str, key)
    char *str, *key;
{
  char *curr = key;
  char *holder = str;

  while (str && *str && *curr) {
    if (to_lower(*str) == to_lower(*curr))
      curr++;
    else {
      curr = key;
      holder = str + 1;
    }
    str++;
  }
  if (*curr == '\0')
    return (holder);
  else
    return ((char *) NULL);
}
#endif

