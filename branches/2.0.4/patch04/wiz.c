/* wiz.c */

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

/* AIX requires this to be the first thing in the file. */
#ifdef __GNUC__
#define alloca	__builtin_alloca
#else	/* not __GNUC__ */
#ifdef HAVE_ALLOCA_H
#include <alloca.h>
#else	/* not HAVE_ALLOCA_H */
#ifdef _AIX
 #pragma alloca
#endif	/* not _AIX */
#endif	/* not HAVE_ALLOCA_H */
#endif 	/* not __GNUC__ */

#include <stdio.h>
#include <sys/types.h>
#ifdef HAVE_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif				/* HAVE_STRING_H */
#include <ctype.h>
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif				/* HAVE_STDLIB_H */
#ifdef HAVE_GETRUSAGE
#include <sys/time.h>
#include <sys/resource.h>
#endif				/* HAVE_GETRUSAGE */

#include "conf.h"
#include "teeny.h"
#include "commands.h"
#include "ptable.h"
#include "sha/sha_wrap.h"
#include "externs.h"

/* Admin only commands. */

VOID do_systat(player, cause, switches)
    int player, cause, switches;
{
#ifndef HAVE_GETRUSAGE
  if(!(switches & CMD_QUIET))
    notify_player(player, cause, player,
  		  "Sorry, that command is not supported on this system.",
		  NOT_QUIET);
#else
  struct rusage rbuf;
  char buf[MEDBUFFSIZ];
#ifdef HAVE_GETLOADAVG
  double load[3];
#endif

  if(getrusage(RUSAGE_SELF, &rbuf) == -1) {
    if(!(switches & CMD_QUIET))
      notify_player(player, cause, player, "Sorry, something is broken.",
      		    NOT_QUIET);
    return;
  }

  snprintf(buf, sizeof(buf), 
	   "User time: %ld seconds, system time: %ld seconds.",
  	   rbuf.ru_utime.tv_sec, rbuf.ru_stime.tv_sec);
  notify_player(player, cause, player, buf, 0);
  snprintf(buf, sizeof(buf),
	   "Max. RSS: %d, minor faults: %d, major faults: %d, swaps: %d",
  	   rbuf.ru_maxrss, rbuf.ru_minflt, rbuf.ru_majflt, rbuf.ru_nswap);
  notify_player(player, cause, player, buf, 0);
  snprintf(buf, sizeof(buf),
	   "Input block I/O: %d, output block I/O: %d", rbuf.ru_inblock,
  	   rbuf.ru_oublock);
  notify_player(player, cause, player, buf, 0);
  snprintf(buf, sizeof(buf),
	   "Messages sent: %d, messages recvd: %d", rbuf.ru_msgsnd,
  	   rbuf.ru_msgrcv);
  notify_player(player, cause, player, buf, 0);
  snprintf(buf, sizeof(buf),
    "Signals: %d, vol. context switches: %d, forced context switches: %d",
    	   rbuf.ru_nsignals, rbuf.ru_nvcsw, rbuf.ru_nivcsw);
  notify_player(player, cause, player, buf, 0);

#ifdef HAVE_GETLOADAVG
  if(getloadavg(load, 3) == 3) {
    snprintf(buf, sizeof(buf), "System Load: %.2f, %.2f, %.2f", load[0],
    	     load[1], load[2]);
    notify_player(player, cause, player, buf, 0);
  }
#endif		/* HAVE_GETLOADAVG */
#endif		/* HAVE_GETRUSAGE */
}

VOID do_boot(player, cause, switches, argone, argtwo)
    int player, cause, switches;
    char *argone, *argtwo;
{
  int victim, fd, total;
  char *ptr, buf[MEDBUFFSIZ];

  if ((argone == (char *)NULL) || (argone[0] == '\0')) {
    if(!(switches & CMD_QUIET))
      notify_player(player, cause, player, "Boot whom?", NOT_QUIET);
    return;
  }
  victim = resolve_player(player, cause, argone,
  			  (!(switches & CMD_QUIET) ? RSLV_NOISY : 0));
  if(victim == -1)
    return;
  if (!isALIVE(victim)) {
    if(!(switches & CMD_QUIET))
      notify_player(player, cause, player, "That player is not connected.",
      		    NOT_QUIET);
    return;
  }
  if (isGOD(victim) && !isGOD(player)) {
    if(!(switches & CMD_QUIET))
      notify_player(player, cause, player, "You can't boot God!", NOT_QUIET);
    return;
  }
  if ((argtwo != (char *)NULL) && argtwo[0]) {
    fd = (int)strtol(argtwo, &ptr, 10);
    if(ptr == argtwo) {
      if(!(switches & CMD_QUIET))
        notify_player(player, cause, player,
		      "You must specify an fd by number.", NOT_QUIET);
      return;
    }
    if (fd_conn(victim) != fd) {
      if(!(switches & CMD_QUIET))
        notify_player(player, cause, player,
     		      "That player isn't connected on that fd.", NOT_QUIET);
      return;
    }

    if(!(switches & BOOT_NOMESG)) {
      snprintf(buf, sizeof(buf), "You have been %s off the game.",
	       (numconnected(victim) > 1) ? "partially booted" : "booted");
      notify_player(victim, cause, player, buf, 0);
    }
    total = fd_logoff(fd);
  } else {
    if(!(switches & BOOT_NOMESG))
      notify_player(victim, cause, player,
		    "You have been booted off the game.", 0);
    total = tcp_logoff(victim);
  }

  if(!(switches & CMD_QUIET)) {
    snprintf(buf, sizeof(buf), "Booted.  Closed %d connections.", total);
    notify_player(player, cause, player, buf, NOT_QUIET);
  }
}

VOID do_chown(player, cause, switches, argone, argtwo)
    int player, cause, switches;
    char *argone, *argtwo;
{
  int owner, obj, exowner;

  if ((argone == (char *)NULL) || (argone[0] == '\0')
      || (argtwo == (char *)NULL) || (argtwo[0] == '\0')) {
    if(!(switches & CMD_QUIET))
      notify_player(player, cause, player, "Chown what to who?", NOT_QUIET);
    return;
  }
  owner = resolve_player(player, cause, argtwo,
  			 (!(switches & CMD_QUIET) ? RSLV_NOISY : 0));
  if(owner == -1)
    return;
  obj = resolve_object(player, cause, argone,
  		       (!(switches & CMD_QUIET) ? RSLV_NOISY|RSLV_EXITS
		       				: RSLV_EXITS));
  if(obj == -1)
    return;
  if (!isWIZARD(player) && (owner != player)) {
    if(!(switches & CMD_QUIET))
      notify_player(player, cause, player,
		    "You may only chown things to yourself.", NOT_QUIET);
    return;
  }
  if (isPLAYER(obj) && !isGOD(player)) {
    if(!(switches & CMD_QUIET))
      notify_player(player, cause, player, "Players always own themselves.",
      		    NOT_QUIET);
    return;
  }
  if (!isWIZARD(player) && !isCHOWN_OK(obj)) {
    if(!(switches & CMD_QUIET))
      notify_player(player, cause, player, "Permission denied.", NOT_QUIET);
    return;
  }
  if (mudconf.enable_money && !isWIZARD(player)
      && !can_afford(owner, cause, cost_of(obj), (switches & CMD_QUIET)))
    return;
  if (mudconf.enable_quota && !isWIZARD(player)
      && no_quota(owner, cause, (switches & CMD_QUIET)))
    return;

  if ((get_int_elt(owner, OWNER, &owner) == -1) ||
      (get_int_elt(obj, OWNER, &exowner) == -1) ||
      (set_int_elt(obj, OWNER, owner) == -1)) {
    if(!(switches & CMD_QUIET))
      notify_player(player, cause, player, "Couldn't set owner.", NOT_QUIET);
  } else {
    if(!(switches & CMD_QUIET))
      notify_player(player, cause, player, "Owner changed.", NOT_QUIET);
    reward_money(exowner, cost_of(obj));
    stamp(obj, STAMP_MODIFIED);
  }
}

VOID do_dump(player, cause, switches)
    int player, cause, switches;
{
  if (mudstat.status == MUD_DUMPING) {
    if(!(switches & CMD_QUIET))
      notify_player(player, cause, player,
		    "Wait for the first one to finish, eh?", NOT_QUIET);
  } else {
    if(!(switches & CMD_QUIET))
      notify_player(player, cause, player, "Dumping...", NOT_QUIET);
    dump_db();
  }
}

VOID do_newpassword(player, cause, switches, argone, argtwo)
    int player, cause, switches;
    char *argone, *argtwo;
{
  int victim;
  char buf[MEDBUFFSIZ];
  char *pname, *vname;

  if ((argone == (char *)NULL) || (argone[0] == '\0')) {
    if(!(switches & CMD_QUIET))
      notify_player(player, cause, player, "Change who's password to what?",
      		    NOT_QUIET);
    return;
  }
  victim = resolve_player(player, cause, argone,
  			  (!(switches & CMD_QUIET) ? RSLV_NOISY : 0));
  if(victim == -1)
    return;
  if (isGOD(victim) && !isGOD(player)) {
    if(!(switches & CMD_QUIET))
      notify_player(player, cause, player, "You can't change God's password!",
      		    NOT_QUIET);
    return;
  }
  if ((argtwo == (char *)NULL) || (argtwo[0] == '\0')) {
    if(!(switches & CMD_QUIET))
      notify_player(player, cause, player, "Change their password to what?",
      		    NOT_QUIET);
    return;
  }

  /* make sure we can get their names before we try to modify them. */
  if ((get_str_elt(player, NAME, &pname) == -1) ||
	(get_str_elt(victim, NAME, &vname) == -1)) {
    notify_bad(player);
    return;
  }

  if (attr_add(victim, PASSWORD, cryptpwd(argtwo), PASSWORD_FLGS) == -1) {
    notify_bad(player);
    return;
  }
  if ((argtwo != (char *)NULL) && argtwo[0] && (player != victim)) {
    snprintf(buf, sizeof(buf),
	     "Your password has been changed to \"%s\".", argtwo);
    notify_player(victim, player, cause, buf, 0);
  }
  logfile(LOG_STATUS,
	  "NEW PASSWORD: %s(#%d) changed the password of %s(#%d).\n", pname,
	  player, vname, victim);

  if(!(switches & CMD_QUIET))
    notify_player(player, cause, player, "Password changed.", NOT_QUIET);
}

VOID do_toad(player, cause, switches, argone, argtwo)
    int player, cause, switches;
    char *argone, *argtwo;
{
  int newowner, victim, total, flags[FLAGS_LEN], loc, dtotal;
  char *name, *pname, *oldname, buf[MEDBUFFSIZ];

  if ((argone == (char *)NULL) || (argone[0] == '\0')) {
    if(!(switches & CMD_QUIET))
      notify_player(player, cause, player, "Whom do you want to toad?",
      		    NOT_QUIET);
    return;
  }
  if ((argtwo == (char *)NULL) || (argtwo[0] == '\0')) {
    newowner = -1;
  } else {
    newowner = resolve_player(player, cause, argtwo,
    			      (!(switches & CMD_QUIET) ? RSLV_NOISY : 0));
    if(newowner == -1)
      return;
    if (get_int_elt(newowner, OWNER, &newowner) == -1) {
      notify_bad(player);
      return;
    }
  }
  victim = resolve_player(player, cause, argone,
  			  (!(switches & CMD_QUIET) ? RSLV_NOISY : 0));
  if(victim == -1)
    return;
  if (isGOD(victim)) {
    if(!(switches & CMD_QUIET))
      notify_player(player, cause, player, "God may not be toaded.", NOT_QUIET);
    return;
  }
  if (player == victim) {
    if(!(switches & CMD_QUIET))
      notify_player(player, cause, player,
    		    "Toading yourself would be quite stupid.", NOT_QUIET);
    return;
  }
  if (isWIZARD(victim) && !isGOD(player)) {
    if(!(switches & CMD_QUIET))
      notify_player(player, cause, player, "You can't toad another wizard!",
      		    NOT_QUIET);
    return;
  }
  notify_player(victim, cause, player,
  		"You sense that everything you own is vanishing...", 0);

  if (newowner != -1) {
    total = chownall(victim, newowner);
  } else {
    total = recycle_owned(player, cause, victim);
  }
  if (get_flags_elt(victim, FLAGS, flags) == -1) {
    notify_bad(player);
    return;
  }
  flags[0] = ((flags[0] & INTERNAL_FLAGS) | TYP_THING);
  if (set_flags_elt(victim, FLAGS, flags) == -1) {
    notify_bad(player);
    return;
  }
  notify_player(victim, cause, player,
		"You have been turned into a slimy toad!", 0);
  dtotal = tcp_logoff(victim);

  if ((get_int_elt(victim, LOC, &loc) == -1) ||
      (get_str_elt(victim, NAME, &name) == -1) ||
      (get_str_elt(player, NAME, &pname) == -1)) {
    notify_bad(player);
    return;
  }
  strcpy(buf, name);
  strcat(buf, " vanishes in a puff of smoke.");
  notify_oall(victim, cause, player, buf, NOT_NOPUPPET);
  if (exists_object(loc))
    move_object(victim, cause, loc, player);

  snprintf(buf, sizeof(buf), "A slimy toad named %s.", name);

  oldname = (char *)alloca(strlen(name) + 1);
  if(oldname == (char *)NULL)
    panic("do_toad(): stack allocation failed\n");
  strcpy(oldname, name);

  if ((set_str_elt(victim, NAME, buf) == -1) ||
      (set_int_elt(victim, OWNER, (newowner == -1) ?
		   player : newowner) == -1)) {
    notify_bad(player);
    return;
  }
  stamp(victim, STAMP_MODIFIED);

  if(!(switches & CMD_QUIET)) {
    snprintf(buf, sizeof(buf),
	     "Toaded.  %d objects %s.  %d connections closed.", total,
	     (newowner == -1) ? "purged" : "chowned", dtotal);
    notify_player(player, cause, player, buf, NOT_QUIET);
  }

  ptable_delete(victim);

  logfile(LOG_COMMAND, "TOAD: %s(#%d) toaded by %s(#%d).\n", oldname, victim,
          pname, player);
}

VOID do_shutdown(player, cause, switches)
    int player, cause, switches;
{
  char *name;

  if (mudstat.status == MUD_DUMPING) {
    if(!(switches & CMD_QUIET))
      notify_player(player, cause, player,
		    "Wait for it to finish dumping, eh?", NOT_QUIET);
    return;
  } else {
    if(get_str_elt(player, NAME, &name) != -1) {
      mudstat.status = MUD_STOPPED;
      logfile(LOG_COMMAND, "SHUTDOWN: by %s(#%d)\n", name, player);

      if(!(switches & CMD_QUIET))
        notify_player(player, cause, player, "Ok.", NOT_QUIET);
    } else
      notify_bad(player);
  }
}

VOID do_pcreate(player, cause, switches, argone, argtwo)
    int player, cause, switches;
    char *argone, *argtwo;
{
  int newplayer, loc;
  char *name, *pwd, nbuf[MAXPNAMELEN + 1], pbuf[MAXPWDLEN + 1];
  char buf[MEDBUFFSIZ];
  char *newname, *pname;

  if ((argone == (char *)NULL) || (argone[0] == '\0')
      || (argtwo == (char *)NULL) || (argtwo[0] == '\0')) {
    if(!(switches & CMD_QUIET))
      notify_player(player, cause, player,
		    "Usage: @pcreate <name> = <password>", NOT_QUIET);
    return;
  }

  if(get_str_elt(player, NAME, &pname) == -1) {
    notify_bad(player);
    return;
  }

  for (name = nbuf; *argone && (isspace(*argone) || *argone == '\"'); argone++);
  while (*argone && *argone != '\"' && (name - nbuf) <= MAXPNAMELEN) {
    if (!isspace(*argone)) {
      *name++ = *argone++;
      continue;
    }
    for (; *argone && isspace(*argone); argone++);
    if (*argone && *argone != '\"')
      *name++ = ' ';
  }
  *name = '\0';

  if (match_player(nbuf) != -1) {
    if(!(switches & CMD_QUIET))
      notify_player(player, cause, player,
    		    "A player with that name already exists.", NOT_QUIET);
    return;
  }
  if (!ok_player_name(nbuf)) {
    if(!(switches & CMD_QUIET))
      notify_player(player, cause, player,
    		    "That's not a reasonable name for a player.", NOT_QUIET);
    return;
  }
  for (pwd = pbuf; *argtwo && isspace(*argtwo); argtwo++);
  for (; *argtwo && (pwd - pbuf) < sizeof(pbuf); *pwd++ = *argtwo++);
  *pwd = '\0';

  if(!mudconf.registration) {
    if ((get_int_elt(player, LOC, &loc) == -1) || !exists_object(loc)) {
      if(!(switches & CMD_QUIET))
        notify_player(player, cause, player,
		      "You have no location!", NOT_QUIET);
      return;
    }
    newplayer = create_player(nbuf, pbuf, loc);
  } else
    newplayer = create_player(nbuf, pbuf, mudconf.starting_loc);

  if(newplayer == -1) {
    if(!(switches & CMD_QUIET))
      notify_player(player, cause, player, "Pcreate failed.", NOT_QUIET);
  } else {
    int owner;

    stamp(newplayer, STAMP_MODIFIED);

    if(get_str_elt(newplayer, NAME, &newname) == -1)
      newname = "???";		/* XXX */

    if(!(switches & CMD_QUIET)) {
      snprintf(buf, sizeof(buf), 
	       "Player \"%s\" created with object #%d.", newname, newplayer);
      notify_player(player, cause, player, buf, NOT_QUIET);
    }

    if(get_int_elt(player, OWNER, &owner) != -1) {
      snprintf(buf, sizeof(buf), "#%d", newplayer);
      var_set(owner, "CREATE-RESULT", buf);
    }
    logfile(LOG_COMMAND, "PCREATED: %s(#%d) created by %s(#%d).\n",
	    newname, newplayer, pname, player);
  }
}

VOID do_purge(player, cause, switches, argone)
    int player, cause, switches;
    char *argone;
{
  int victim, ret;
  char buf[MEDBUFFSIZ];

  if ((argone == (char *)NULL) || (argone[0] == '\0')) {
    if(!(switches & CMD_QUIET))
      notify_player(player, cause, player,
    		    "But whose possessions do you want to purge?", NOT_QUIET);
    return;
  }
  victim = resolve_player(player, cause, argone,
  			  (!(switches & CMD_QUIET) ? RSLV_NOISY : 0));
  if(victim == -1)
    return;
  if (isGOD(victim)) {
    if(!(switches & CMD_QUIET))
      notify_player(player, cause, player,
		    "You may not purge God's possessions.", NOT_QUIET);
    return;
  }
  ret = recycle_owned(player, cause, victim);

  if(!(switches & CMD_QUIET)) {
    snprintf(buf, sizeof(buf), "Purged %d objects.", ret);
    notify_player(player, cause, player, buf, NOT_QUIET);
  }
}

VOID do_chownall(player, cause, switches, argone, argtwo)
    int player, cause, switches;
    char *argone, *argtwo;
{
  int oldowner, newowner, ret;
  char buf[MEDBUFFSIZ];

  if ((argone == (char *)NULL) || (argone[0] == '\0')
      || (argtwo == (char *)NULL) || (argtwo[0] == '\0')) {
    if(!(switches & CMD_QUIET))
      notify_player(player, cause, player,
    		    "Usage: @chownall <oldowner> = <newowner>", NOT_QUIET);
    return;
  }
  oldowner = resolve_player(player, cause, argone,
  			    (!(switches & CMD_QUIET) ? RSLV_NOISY : 0));
  if(oldowner == -1)
    return;
  newowner = resolve_player(player, cause, argtwo,
  			    (!(switches & CMD_QUIET) ? RSLV_NOISY : 0));
  if(newowner == -1)
    return;
  if (get_int_elt(newowner, OWNER, &newowner) == -1) {
    notify_bad(player);
    return;
  }
  if (isGOD(oldowner)) {
    if(!(switches & CMD_QUIET))
      notify_player(player, cause, player,
    		    "You may not chown all of God's possessions.", NOT_QUIET);
    return;
  }
  ret = chownall(oldowner, newowner);

  if(!(switches & CMD_QUIET)) {
    snprintf(buf, sizeof(buf), "Chowned %d objects to #%d.", ret, newowner);
    notify_player(player, cause, player, buf, NOT_QUIET);
  }
}

VOID do_quota(player, cause, switches, argone, argtwo)
    int player, cause, switches;
    char *argone, *argtwo;
{
  int quota, victim;
  char *ptr, buf[MEDBUFFSIZ];

  if(mudconf.enable_quota) {
    if ((argone == (char *)NULL) || (argone[0] == '\0')) {
      victim = player;
    } else {
      victim = resolve_player(player, cause, argone,
      			      (!(switches & CMD_QUIET) ? RSLV_NOISY : 0));
      if(victim == -1)
        return;
      if (get_int_elt(victim, OWNER, &victim) == -1) {
	notify_bad(player);
	return;
      }
      if (!controls(player, cause, victim) || !has_quota(victim)) {
        if(!(switches & CMD_QUIET))
          notify_player(player, cause, player,
		        "You're simply not perceptive enough.", NOT_QUIET);
        return;
      }
    }
    if ((argtwo == (char *)NULL) || (argtwo[0] == '\0')) {
      if (!isWIZARD(victim)) {
        if (get_int_elt(victim, QUOTA, &quota) == -1) {
	  notify_bad(player);
	  return;
        }
        snprintf(buf, sizeof(buf), "%s  Object Quota: %d",
		 display_name(player, cause, victim), quota);
      } else
        snprintf(buf, sizeof(buf), "%s  Object Quota: *UNRESTRICTED*",
		 display_name(player, cause, victim));
      notify_player(player, cause, player, buf, 0);
    } else {
      if (!isWIZARD(player)) {
        if(!(switches & CMD_QUIET))
          notify_player(player, cause, player,
      		        "Contact a Wizard if your quota needs updated.",
			NOT_QUIET);
        return;
      }
      quota = (int)strtol(argtwo, &ptr, 10);
      if((ptr == argtwo) || (quota < 0)) {
        if(!(switches & CMD_QUIET))
          notify_player(player, cause, player,
      		        "Please specify only a positive number.", NOT_QUIET);
        return;
      }
      if (set_int_elt(victim, QUOTA, quota) == -1) {
        if(!(switches & CMD_QUIET))
          notify_player(player, cause, player,
	  		"Couldn't set quota.", NOT_QUIET);
        return;
      }

      if(!(switches & CMD_QUIET)) {
        snprintf(buf, sizeof(buf), "Quota set to %d objects.", quota);
        notify_player(player, cause, player, buf, NOT_QUIET);
      }
    }
  } else {
    if(!(switches & CMD_QUIET))
      notify_player(player, cause, player,
      		    "Quotas are not enabled.", NOT_QUIET);
  }
}

VOID do_poor(player, cause, switches, argone)
    int player, cause, switches;
    char *argone;
{
  int amount;
  register int i, count;
  char *ptr, buf[MEDBUFFSIZ];

  if(!mudconf.enable_money) {
    if(!(switches & CMD_QUIET))
      notify_player(player, cause, player, "Pennies are not enabled.",
      		    NOT_QUIET);
    return;
  }

  if ((argone == (char *)NULL) || (argone[0] == '\0')) {
    if(!(switches & CMD_QUIET))
      notify_player(player, cause, player, "You must specify an amount.",
      		    NOT_QUIET);
    return;
  }
  amount = (int)strtol(argone, &ptr, 10);
  if(ptr == argone) {
    if(!(switches & CMD_QUIET))
      notify_player(player, cause, player,
		    "You must specify a numerical amount.", NOT_QUIET);
    return;
  }

  for (count = 0, i = 0; i < mudstat.total_objects; i++) {
    if (!exists_object(i) || !isPLAYER(i))
      continue;
    if (set_int_elt(i, PENNIES, amount) == -1) {
      logfile(LOG_ERROR, "poor: bad pennies reference on player #%d\n", i);
    } else {
      count++;
    }
  }

  if(!(switches & CMD_QUIET)) {
    snprintf(buf, sizeof(buf), "Set %d players to %d pennies.", count, amount);
    notify_player(player, cause, player,buf, NOT_QUIET);
  }
}

VOID do_motd(player, cause, switches, argone, argtwo)
    int player, cause, switches;
    char *argone, *argtwo;
{
  FILE *fp;
  char buf[128];
  enum { MOTD_ANY, MOTD_NORM, MOTD_WIZ, MOTD_NEWP } wh;

  if(argone != (char *)NULL) {
    switch(argone[0]) {
    case 'w':
    case 'W':
      wh = MOTD_WIZ;
      break;
    case 'm':
    case 'M':
      wh = MOTD_NORM;
      break;
    case 'n':
    case 'N':
      wh = MOTD_NEWP;
      break;
    case '\0':
      wh = MOTD_ANY;
      break;
    default:
      if(!(switches & CMD_QUIET))
        notify_player(player, cause, player,
		      "I don't understand that argument.", NOT_QUIET);
      return;
    }
  } else
    wh = MOTD_ANY;

  if(switches & MOTD_SET) {
    if((argtwo != (char *)NULL) && (argtwo[0] == '\0'))
      argtwo = (char *)NULL;

    switch(wh) {
    case MOTD_WIZ:
      ty_free((VOID *)mudstat.exwiz);
      mudstat.exwiz = ty_strdup(argtwo, "do_motd");
      break;
    case MOTD_NORM:
      ty_free((VOID *)mudstat.exmotd);
      mudstat.exmotd = ty_strdup(argtwo, "do_motd");
      break;
    default:
      if(!(switches & CMD_QUIET))
        notify_player(player, cause, player,
		      "You can't set just any motd!", NOT_QUIET);
      return;
    }
    if(!(switches & CMD_QUIET))
      notify_player(player, cause, player, "Motd set.", NOT_QUIET);
  } else {
    if((wh == MOTD_NORM) || (wh == MOTD_ANY)) {
      if((fp = fopen(mudconf.motd_file, "r")) != (FILE *)NULL) {
	while(!feof(fp) && (fgets(buf, sizeof(buf), fp) != (char *)NULL)) {
	  remove_newline(buf);

	  notify_player(player, cause, player, buf, 0);
	}
	fclose(fp);
      }
      if(mudstat.exmotd != (char *)NULL)
        notify_player(player, cause, player, mudstat.exmotd, 0);
    }
    if((wh == MOTD_WIZ) || (wh == MOTD_ANY)) {
      if(isWIZARD(player)) {
	if((fp = fopen(mudconf.wizn_file, "r")) != (FILE *)NULL) {
	  while(!feof(fp)
		&& (fgets(buf, sizeof(buf), fp) != (char *)NULL)) {
	    remove_newline(buf);

	    notify_player(player, cause, player, buf, 0);
	  }
	  fclose(fp);
	}
	if(mudstat.exwiz != (char *)NULL)
	  notify_player(player, cause, player, mudstat.exwiz, 0);
      } else if(wh == MOTD_WIZ)
	notify_player(player, cause, player, "Permission denied.", 0);
    }
    if(wh == MOTD_NEWP) {
      if((fp = fopen(mudconf.newp_file, "r")) != (FILE *)NULL) {
	while(!feof(fp) && (fgets(buf, sizeof(buf), fp) != (char *)NULL)) {
	  remove_newline(buf);

	  notify_player(player, cause, player, buf, 0);
	}
      }
    }
    notify_player(player, cause, player, "***End of motd***", 0);
  }
}
