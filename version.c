/* version.c */

#include "config.h"

/*
 *		       This file is part of TeenyMUD II.
 *	    Copyright(C) 1995 by Jason Downs.  All rights reserved.
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

#include <stdio.h>
#include <sys/types.h>
#ifdef HAVE_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif			/* HAVE_STRING_H */

#include "conf.h"
#include "teeny.h"
#include "commands.h"
#include "externs.h"

/*
 * The version user command.
 */

extern const char teenymud_id[];

VOID do_version(player, cause, switches)
    int player, cause, switches;
{
  char buf[BUFFSIZ];

  snprintf(buf, sizeof(buf), "This is %s.", &teenymud_id[4]);
  notify_player(player, cause, player, buf, 0);
  notify_player(player, cause, player,
	"Copyright(C) 1993-1995, Jason Downs.  All rights reserved.", 0);

  if(switches & VERSION_FULL) {
    notify_player(player, cause, player, "Compile time options:", 0);

    buf[0] = '\0';
#ifdef USE_NDBM
    strcat(buf, "[NDBM] ");
#endif		/* USE_NDBM */
#ifdef USE_GDBM
    strcat(buf, "[GDBM] ");
#endif		/* USE_GDBM */
#ifdef USE_BSDDBM
    strcat(buf, "[BSD DBM] ");
#endif		/* USE_BSDDBM */
#ifdef USE_TERM
    strcat(buf, "[TERM] ");
#endif		/* USE_TERM */
#ifdef USE_TLI
    strcat(buf, "[TLI] ");
#endif		/* USE_TLI */
    
    buf[strlen(buf)-1] = '\0';
    notify_player(player, cause, player, buf, 0);
  }
}
