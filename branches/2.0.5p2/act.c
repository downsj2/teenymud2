/* act.c */

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

#include <stdio.h>
#include <sys/types.h>
#ifdef HAVE_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif			/* HAVE_STRING_H */
#include <ctype.h>
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif			/* HAVE_STLDIB_H */

#include "conf.h"
#include "teeny.h"
#include "externs.h"

/* object actions. */

void act_object(player, cause, thing, atr, oatr, aatr, oloc, def, odef)
    int player, cause, thing;
    char *atr, *oatr, *aatr;
    int oloc;
    char *def, *odef;
{
  char *str, *ostr, *astr, *name;
  char buf[MEDBUFFSIZ];
  int aflags, source;
  register char *eptr;

  if (atr != (char *) NULL) {
    if (attr_get_parent(thing, atr, &str, &aflags, &source) == -1) {
      logfile(LOG_ERROR, "act_object: bad attribute list on #%d\n", thing);
      return;
    }
    if (str == (char *) NULL)
      str = def;
    if (str != (char *) NULL) {
      if(mudconf.file_access || mudconf.file_exec) {
        if(mudconf.file_exec && (str[0] == '!')) {
	  dump_cmdoutput(player, cause, thing, &str[1]);
	} else if(mudconf.file_access && (str[0] == '@')) {
	  dump_file(player, cause, thing, &str[1]);
	} else {
          eptr = exec(player, cause, thing, str, 0, 0, (char **)NULL);
	  notify_player(player, cause, thing, eptr, 0);
	  ty_free((VOID *) eptr);
        }
      } else {
        eptr = exec(player, cause, thing, str, 0, 0, (char **)NULL);
        notify_player(player, cause, thing, eptr, 0);
        ty_free((VOID *)eptr);
      }
    }
  }
  if (oatr != (char *) NULL) {
    if (attr_get_parent(thing, oatr, &ostr, &aflags, &source) == -1) {
      logfile(LOG_ERROR, "act_object: bad attribute list on #%d\n", thing);
      return;
    }
    if (ostr == (char *) NULL)
      ostr = odef;
    if ((ostr != (char *) NULL) && !isDARK(player)) {
      if (get_str_elt(player, NAME, &name) == -1) {
	logfile(LOG_ERROR, "act_object: bad name reference on object #%d\n",
	        player);
	return;
      }
      eptr = exec(player, cause, thing, ostr, 0, 0, (char **)NULL);
      snprintf(buf, sizeof(buf), "%s%s%s", name,
	      ((ostr[0] == '\'' || ostr[0] == ',') ? "" : " "), eptr);
      ty_free((VOID *)eptr);
      if (oloc == -1) {
	notify_except2(player, cause, thing, buf, player, thing, NOT_NOPUPPET);
      } else {
	notify_contents_except(oloc, cause, thing, buf, player, NOT_NOPUPPET);
      }
    }
  }
  if(aatr != (char *)NULL) {
    int charges;

    if(get_int_elt(thing, CHARGES, &charges) == -1) {
      logfile(LOG_ERROR, "act_object: bad charges reference on #%d\n", thing);
      return;
    }
    if(charges > 0) {
      if(set_int_elt(thing, CHARGES, --charges) == -1) {
        logfile(LOG_ERROR, "act_object: bad charges reference on #%d\n", thing);
	return;
      }
    }
    if(charges == 0) {
      if(attr_get_parent(thing, RUNOUT, &astr, &aflags, &source) == -1) {
	logfile(LOG_ERROR, "act_object: bad attribute list on #%d\n", thing);
	return;
      }
    } else {
      if(attr_get_parent(thing, aatr, &astr, &aflags, &source) == -1) {
        logfile(LOG_ERROR, "act_object: bad attribute list on #%d\n", thing);
        return;
      }
    }

    if(astr != (char *)NULL)
      queue_add(thing, cause, 1, -1, astr, 0, (char **)NULL);
  }
}

int can_hear(obj)
    int obj;
{
  int owner;

  if (get_int_elt(obj, OWNER, &owner) == -1)
    return (0);

  if (isPLAYER(obj) || isLISTENER(obj) || (isPUPPET(obj) && isALIVE(owner))
      || isAUDIBLE(obj))
    return(1);

  return (0);
}

void hear_alert(cause, chear, obj)
    int cause, chear, obj;
{
  int loc, gender, nhear;
  char *name;
  char buff[BUFFSIZ];

  if((get_int_elt(obj, LOC, &loc) == -1)
     || (get_str_elt(obj, NAME, &name) == -1)) {
    logfile(LOG_ERROR, "hear_alert: bad loc or name reference on object #%d\n",
            obj);
    return;
  }

  nhear = can_hear(obj);
  if(!chear && nhear) {
    snprintf(buff, sizeof(buff), "%s grows ears and can now hear.", name);
    notify_contents_loc(loc, cause, obj, buff, obj, NOT_NOPUPPET);
  } else if(chear && !nhear) {
    gender = genderof(obj);

    snprintf(buff, sizeof(buff), "%s loses %s ears and becomes deaf.", name,
            (((gender == 0) || (gender == 1)) ? "its" :
	    ((gender == 2) ? "her" : "his")));
    notify_contents_loc(loc, cause, obj, buff, obj, 0);
  }
}
