/* globals.c */

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

#include "conf.h"
#include "teeny.h"
#include "flaglist.h"

LockList Ltable[LTABLE_TOTAL] =
{
  {"Lock", 1},
  {"EnterLock", 1},
  {"PageLock", 1},
  {"TelInLock", 4},
  {"TelOutLock", 4},
  {"UseLock", 1}
};

StringList Atable[ATABLE_TOTAL] =
{
  {"Description", "describe", DEFATTR_FLGS, 1},
  {"Idescription", "idescribe", DEFATTR_FLGS, 1},
  {"Odescription", "odescribe", DEFATTR_FLGS, 1},
  {"Adescription", "adescribe", DEFATTR_FLGS, 1},
  {"Success", (char *) NULL, DEFATTR_FLGS, 1},
  {"Osuccess", (char *) NULL, DEFATTR_FLGS, 1},
  {"Asuccess", (char *) NULL, DEFATTR_FLGS, 1},
  {"Fail", (char *) NULL, DEFATTR_FLGS, 1},
  {"Ofail", (char *) NULL, DEFATTR_FLGS, 1},
  {"Afail", (char *) NULL, DEFATTR_FLGS, 1},
  {"Drop", (char *) NULL, DEFATTR_FLGS, 1},
  {"Odrop", (char *) NULL, DEFATTR_FLGS, 1},
  {"Adrop", (char *) NULL, DEFATTR_FLGS, 1},
  {"Enter", (char *) NULL, DEFATTR_FLGS, 1},
  {"Oenter", (char *) NULL, DEFATTR_FLGS, 1},
  {"Oxenter", (char *) NULL, DEFATTR_FLGS, 1},
  {"Aenter", (char *) NULL, DEFATTR_FLGS, 1},
  {"Leave", (char *) NULL, DEFATTR_FLGS, 1},
  {"Oleave", (char *) NULL, DEFATTR_FLGS, 1},
  {"Oxleave", (char *) NULL, DEFATTR_FLGS, 1},
  {"Aleave", (char *) NULL, DEFATTR_FLGS, 1},
  {"Kill", (char *) NULL, DEFATTR_FLGS, 1},
  {"Okill", (char *) NULL, DEFATTR_FLGS, 1},
  {"Akill", (char *) NULL, DEFATTR_FLGS, 1},
  {"Oteleport", (char *) NULL, DEFATTR_FLGS, 1},
  {"Oxteleport", (char *) NULL, DEFATTR_FLGS, 1},
  {"Ateleport", (char *) NULL, DEFATTR_FLGS, 1},
  {"Sex", "gender", DEFATTR_FLGS, 1},
  {"EnterFail", "efail", DEFATTR_FLGS, 1},
  {"OenterFail", "oefail", DEFATTR_FLGS, 1},
  {"AenterFail", "aefail", DEFATTR_FLGS, 1},
  {"Hostname", (char *) NULL, A_IMMUTABLE, 0},
  {"XPassword", (char *) NULL, A_INTERNAL, 0},
  {"AudiblePrefix", "aprefix", DEFATTR_FLGS, 1},
  {"AudibleFilter", "afilter", DEFATTR_FLGS, 1},
  {"Use", (char *) NULL, DEFATTR_FLGS, 1},
  {"Ouse", (char *) NULL, DEFATTR_FLGS, 1},
  {"Ause", (char *) NULL, DEFATTR_FLGS, 1},
  {"Runout", (char *) NULL, DEFATTR_FLGS, 1},
  {"Pay", (char *) NULL, DEFATTR_FLGS, 1},
  {"Opay", (char *) NULL, DEFATTR_FLGS, 1},
  {"Apay", (char *) NULL, DEFATTR_FLGS, 1},
  {"Startup", (char *) NULL, DEFATTR_FLGS, 1},
  {"Stty", (char *) NULL, A_INTERNAL, 0},
  {"KillFail", (char *) NULL, DEFATTR_FLGS, 1},
  {"OKillFail", (char *) NULL, DEFATTR_FLGS, 1},
  {"AKillFail", (char *) NULL, DEFATTR_FLGS, 1},
  {"Alias", (char *) NULL, A_IMMUTABLE, 0},
  {"Vrml_url", (char *) NULL, DEFATTR_FLGS, 1},
  {"HtDescription", "htdescribe", DEFATTR_FLGS, 1}
};

AFlagList AFlags[] =
{
  {"PRIVATE", 3, A_PRIVATE, 'P'},
  {"VISIBLE", 1, A_VISIBLE, 'V'},
  {"INTERNAL", 7, A_INTERNAL, 'I'},
  {"PICKY", 3, A_PICKY, 'p'},
  {"IMMUTABLE", 2, A_IMMUTABLE, 'i'},
  {(char *) NULL, 0, 0, '\0'}
};

FlagList PlayerFlags[] =
{
  {"ABODE", 2, ABODE, 'A', 0, PERM_VISIBLE},
  {"BUILDER", 6, BUILDER, 'B', 0, PERM_WIZ},
  {"DARK", 2, DARK, 'D', 1, PERM_WIZ},
  {"REVERSED_WHO", 3, REVERSED_WHO, 'b', 0, 0},
  {"ENTER_OK", 2, ENTER_OK, 'e', 0, 0},
  {"GUEST", 2, GUEST, 'g', 0, PERM_WIZ},
  {"OPAQUE", 2, OPAQUE, 'o', 0, 0},
  {"ELOQUENT", 2, ELOQUENT, 'q', 0, 0},
  {"ROBOT", 2, ROBOT, 'r', 0, PERM_WIZ},
  {"LIGHT", 3, LIGHT, 't', 0, 0},
  {"AUDIBLE", 2, AUDIBLE, 'v', 0, 0},
  {"RETENTIVE", 3, RETENTIVE, 'w', 0, PERM_WIZ},
  {(char *) NULL, 0, '\0', 0, 0}
};

FlagList RoomFlags[] =
{
  {"ABODE", 2, ABODE, 'A', 0, PERM_VISIBLE},
  {"BUILDING_OK", 6, BUILDING_OK, 'B', 0, PERM_VISIBLE},
  {"CHOWN_OK", 2, CHOWN_OK, 'C', 0, PERM_VISIBLE},
  {"DARK", 2, DARK, 'D', 1, PERM_DARK},
  {"DESTROY_OK", 2, DESTROY_OK, 'd', 0, 0},
  {(char *) NULL, 0, '\0', 0, 0}
};

FlagList ThingFlags[] =
{
  {"ABODE", 2, ABODE, 'A', 0, PERM_VISIBLE},
  {"BUILDING_OK", 6, BUILDING_OK, 'B', 0, PERM_VISIBLE},
  {"CHOWN_OK", 2, CHOWN_OK, 'C', 0, PERM_VISIBLE},
  {"DARK", 2, DARK, 'D', 1, PERM_DARK},
  {"DESTROY_OK", 2, DESTROY_OK, 'd', 0, 0},
  {"ENTER_OK", 2, ENTER_OK, 'e', 0, 0},
  {"OPAQUE", 2, OPAQUE, 'o', 0, 0},
  {"PUPPET", 2, PUPPET, 'p', 0, 0},
  {"ELOQUENT", 2, ELOQUENT, 'q', 0, 0},
  {"LIGHT", 3, LIGHT, 't', 0, 0},
  {"AUDIBLE", 2, AUDIBLE, 'v', 0, 0},
  {(char *) NULL, 0, '\0', 0, 0}
};

FlagList ExitFlags[] =
{
  {"CHOWN_OK", 2, CHOWN_OK, 'C', 0, PERM_VISIBLE},
  {"DARK", 2, DARK, 'D', 1, PERM_DARK},
  {"OBVIOUS", 2, OBVIOUS, 'O', 0, 0},
  {"ACTION", 2, ACTION, 'a', 0, 0},
  {"DESTROY_OK", 2, DESTROY_OK, 'd', 0, 0},
  {"TRANSPARENT", 3, TRANSPARENT, 'p', 0, 0},
  {"LIGHT", 3, LIGHT, 't', 0, 0},
  {"AUDIBLE", 2, AUDIBLE, 'v', 0, 0},
  {"EXTERNAL", 2, EXTERNAL, 'x', 0, 0},
  {(char *) NULL, 0, '\0', 0, 0}
};

FlagList GenFlags[] =
{
  {"FILE_OK", 4, FILE_OK, 'F', 1, PERM_GOD},
  {"GOD", 3, GOD, 'G', 1, PERM_GOD},
  {"HAVEN", 3, HAVEN, 'H', 1, 0},
  {"INHERIT", 3, INHERIT, 'I', 1, PERM_PLAYER},
  {"JUMP_OK", 2, JUMP_OK, 'J', 1, PERM_VISIBLE},
  {"LINK_OK", 3, LINK_OK, 'L', 1, PERM_VISIBLE},
  {"NOCHECK", 3, NOCHECK, 'N', 1, 0},
  {"QUIET", 2, QUIET, 'Q', 1, 0},
  {"STICKY", 2, STICKY, 'S', 1, 0},
  {"VISUAL", 2, VISUAL, 'V', 1, PERM_VISIBLE},
  {"WIZARD", 3, WIZARD, 'W', 1, PERM_GOD},
  {"PARENT_OK", 2, PARENT_OK, 'X', 1, PERM_VISIBLE},
  {"CONNECTED", 8, ALIVE, 'c', 1, PERM_INTERNAL},
  {"HALT", 3, HALT, 'h', 1, 0},
  {"HIDDEN", 2, HIDDEN, 'i', 1, 0},
  {"LISTENER", 3, LISTENER, 'l', 1, 0},
  {"NOSPOOF", 3, NOSPOOF, 'n', 1, 0},
  {"STARTUP", 7, HAS_STARTUP, 's', 1, PERM_INTERNAL},
  {"PICKY", 3, PICKY, 'y', 1, 0},
  {"CONTROL_OK", 3, CONTROL_OK, 'z', 1, 0},
  {(char *) NULL, 0, '\0', 0, 0}
};

struct mconf mudconf;           /* global config struct */
struct mstat mudstat;		/* global stat struct */


/* version string, make it look like an ID. */
#if defined(__STDC__) && defined(__DATE__)
const char teenymud_id[] = "@(#)TeenyMUD v2.0.5 PL#2 (Built " __DATE__ ")";
#else
const char teenymud_id[] = "@(#)TeenyMUD v2.0.5 PL#2";
#endif
const char teenymud_version[] = "2.0.5p2";
