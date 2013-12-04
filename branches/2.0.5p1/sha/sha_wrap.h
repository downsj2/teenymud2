/* sha_wrap.h */

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

#ifndef __SHA_WRAP_H

/* macros */
#define cryptpwd(_s)	sha_crypt(_s)
#ifndef UNIX_CRYPT
#define comp_password(_e,_s)	!strcmp(sha_crypt(_s), _e)
#endif			/* UNIX_CRYPT */

/* prototype for the SHA wrapper. */
extern char *sha_crypt _ANSI_ARGS_((char *));

#define __SHA_WRAP_H
#endif			/* __SHA_WRAP_H */
