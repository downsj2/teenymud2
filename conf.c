/* conf.c */

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

#include <stdio.h>
#include <sys/types.h>
#ifdef HAVE_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif			/* HAVE_STRING_H */
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif			/* HAVE_STDLIB_H */
#include <ctype.h>

#include "conf.h"
#include "teeny.h"
#include "commands.h"
#include "externs.h"

#include "hash/hash.h"

/* dynamic configuration system, command aliases, etc. */

typedef enum {
    CONF_BOOL,
    CONF_FLAGS,
    CONF_SHORT,
    CONF_INT,
    CONF_TIME,
    CONF_STRING
} cf_typ;

#define CONF_NORM	0x00
#define CONF_INITONLY	0x01

extern int errno;

static int ConfHash_Inited = 0;
static Hash_Table ConfHash;

static int conf_parse _ANSI_ARGS_((char *));
INLINE static void parse_bool _ANSI_ARGS_((char *, bool *));
INLINE static void parse_short _ANSI_ARGS_((char *, short *));
INLINE static void parse_int _ANSI_ARGS_((char *, int *));
INLINE static void parse_time _ANSI_ARGS_((char *, time_t *));
#ifndef INTERNAL
static void unparse_time _ANSI_ARGS_((char *, size_t, char *, time_t));
#endif			/* INTERNAL */

/* keep in sync with struct conf */
struct conftab {
  char *key;
  VOID *ptr;
  cf_typ typ;
  short flags;
};

#ifndef INTERNAL
static void conf_dumpopt _ANSI_ARGS_((int, int, struct conftab *, char *,
				      size_t));
#endif			/* INTERNAL */

/* parse keyword table. */
static struct conftab ctable[] = {
  {"mudname", (VOID *) mudconf.name, CONF_STRING, CONF_INITONLY},

  {"hostnames", (VOID *) &mudconf.hostnames, CONF_BOOL, CONF_NORM},
  {"registration", (VOID *) &mudconf.registration, CONF_BOOL, CONF_NORM},
  {"logcommands", (VOID *) &mudconf.logcommands, CONF_BOOL, CONF_NORM},
  {"enable_quota", (VOID *) &mudconf.enable_quota, CONF_BOOL, CONF_NORM},
  {"enable_money", (VOID *) &mudconf.enable_money, CONF_BOOL, CONF_NORM},
  {"enable_groups", (VOID *) &mudconf.enable_groups, CONF_BOOL, CONF_NORM},
  {"enable_autotoad", (VOID *) &mudconf.enable_autotoad, CONF_BOOL, CONF_NORM},
#ifdef notyet
  {"enable_compress", (VOID *) &mudconf.enable_compress, CONF_BOOL,
							 CONF_INITONLY},
#endif
  {"wizwhoall", (VOID *) &mudconf.wizwhoall, CONF_BOOL, CONF_NORM},
  {"dark_sleep", (VOID *) &mudconf.dark_sleep, CONF_BOOL, CONF_NORM},
  {"file_access", (VOID *) &mudconf.file_access, CONF_BOOL, CONF_INITONLY},
  {"file_exec", (VOID *) &mudconf.file_exec, CONF_BOOL, CONF_INITONLY},
  {"enable_rwho", (VOID *) &mudconf.enable_rwho, CONF_BOOL, CONF_INITONLY},

  {"default_port", (VOID *) &mudconf.default_port, CONF_INT, CONF_INITONLY},
  {"rwho_port", (VOID *) &mudconf.rwho_port, CONF_INT, CONF_INITONLY},
  {"starting_loc", (VOID *) &mudconf.starting_loc, CONF_INT, CONF_NORM},
  {"root_location", (VOID *) &mudconf.root_location, CONF_INT, CONF_NORM},
  {"player_god", (VOID *) &mudconf.player_god, CONF_INT, CONF_INITONLY},

  {"parent_depth", (VOID *) &mudconf.parent_depth, CONF_SHORT, CONF_NORM},
  {"exit_depth", (VOID *) &mudconf.exit_depth, CONF_SHORT, CONF_NORM},
  {"room_depth", (VOID *) &mudconf.room_depth, CONF_SHORT, CONF_NORM},

  {"growth_increment", (VOID *) &mudconf.growth_increment, CONF_INT,
   							   CONF_INITONLY},
  {"slack", (VOID *) &mudconf.slack, CONF_INT, CONF_INITONLY},

  {"starting_quota", (VOID *) &mudconf.starting_quota, CONF_SHORT, CONF_NORM},
  {"starting_money", (VOID *) &mudconf.starting_money, CONF_SHORT, CONF_NORM},
  {"daily_paycheck", (VOID *) &mudconf.daily_paycheck, CONF_SHORT, CONF_NORM},
  {"queue_commands", (VOID *) &mudconf.queue_commands, CONF_SHORT, CONF_NORM},
  {"queue_cost", (VOID *) &mudconf.queue_cost, CONF_SHORT, CONF_NORM},
  {"find_cost", (VOID *) &mudconf.find_cost, CONF_SHORT, CONF_NORM},
  {"limfind_cost", (VOID *) &mudconf.limfind_cost, CONF_SHORT, CONF_NORM},
  {"stat_cost", (VOID *) &mudconf.stat_cost, CONF_SHORT, CONF_NORM},
  {"fullstat_cost", (VOID *) &mudconf.fullstat_cost, CONF_SHORT, CONF_NORM},
  {"room_cost", (VOID *) &mudconf.room_cost, CONF_SHORT, CONF_NORM},
  {"thing_cost", (VOID *) &mudconf.thing_cost, CONF_SHORT, CONF_NORM},
  {"exit_cost", (VOID *) &mudconf.exit_cost, CONF_SHORT, CONF_NORM},
  {"minkill_cost", (VOID *) &mudconf.minkill_cost, CONF_SHORT, CONF_NORM},
  {"maxkill_cost", (VOID *) &mudconf.maxkill_cost, CONF_SHORT, CONF_NORM},

  {"max_list", (VOID *) &mudconf.max_list, CONF_SHORT, CONF_NORM},
  {"max_commands", (VOID *) &mudconf.max_commands, CONF_SHORT, CONF_NORM},
  {"max_pennies", (VOID *) &mudconf.max_pennies, CONF_INT, CONF_NORM},

  {"cache_size", (VOID *) &mudconf.cache_size, CONF_INT, CONF_INITONLY},
  {"cache_width", (VOID *) &mudconf.cache_width, CONF_INT, CONF_INITONLY},
  {"cache_depth", (VOID *) &mudconf.cache_depth, CONF_INT, CONF_INITONLY},
 
  {"queue_slice", (VOID *) &mudconf.queue_slice, CONF_SHORT, CONF_NORM},

  {"build_flags", (VOID *) mudconf.build_flags, CONF_FLAGS, CONF_NORM},
  {"robot_flags", (VOID *) mudconf.robot_flags, CONF_FLAGS, CONF_NORM},
  {"wizard_flags", (VOID *) mudconf.wizard_flags, CONF_FLAGS, CONF_INITONLY},
  {"god_flags", (VOID *) mudconf.god_flags, CONF_FLAGS, CONF_INITONLY},
  {"guest_flags", (VOID *) mudconf.guest_flags, CONF_FLAGS, CONF_NORM},
  {"player_flags", (VOID *) mudconf.newp_flags, CONF_FLAGS, CONF_NORM},

  {"dump_interval", (VOID *) &mudconf.dump_interval, CONF_TIME, CONF_NORM},
  {"login_timeout", (VOID *) &mudconf.login_timeout, CONF_TIME, CONF_NORM},
  {"player_timeout", (VOID *) &mudconf.player_timeout, CONF_TIME, CONF_NORM},
  {"rwho_interval", (VOID *) &mudconf.rwho_interval, CONF_TIME, CONF_NORM},

  {"rwho_passwd", (VOID *) mudconf.rwho_passwd, CONF_STRING, CONF_INITONLY},
  {"rwho_host", (VOID *) mudconf.rwho_host, CONF_STRING, CONF_INITONLY},

  {"dbm_file", (VOID *) mudconf.dbm_file, CONF_STRING, CONF_INITONLY},
  {"db_file", (VOID *) mudconf.db_file, CONF_STRING, CONF_INITONLY},
  {"help_file", (VOID *) mudconf.help_file, CONF_STRING, CONF_NORM},
  {"news_file", (VOID *) mudconf.news_file, CONF_STRING, CONF_NORM},
  {"motd_file", (VOID *) mudconf.motd_file, CONF_STRING, CONF_NORM},
  {"wizn_file", (VOID *) mudconf.wizn_file, CONF_STRING, CONF_NORM},
  {"greet_file", (VOID *) mudconf.greet_file, CONF_STRING, CONF_NORM},
  {"newp_file", (VOID *) mudconf.newp_file, CONF_STRING, CONF_NORM},
  {"register_file", (VOID *) mudconf.register_file, CONF_STRING, CONF_NORM},
  {"bye_file", (VOID *) mudconf.bye_file, CONF_STRING, CONF_NORM},
  {"shtd_file", (VOID *) mudconf.shtd_file, CONF_STRING, CONF_NORM},
  {"guest_file", (VOID *) mudconf.guest_file, CONF_STRING, CONF_NORM},
  {"timeout_file", (VOID *) mudconf.timeout_file, CONF_STRING, CONF_NORM},
  {"badconn_file", (VOID *) mudconf.badconn_file, CONF_STRING, CONF_NORM},
  {"badcrt_file", (VOID *) mudconf.badcrt_file, CONF_STRING, CONF_NORM},

  {"status_file", (VOID *) mudconf.logstatus_file, CONF_STRING, CONF_NORM},
  {"gripe_file", (VOID *) mudconf.loggripe_file, CONF_STRING, CONF_NORM},
  {"command_file", (VOID *) mudconf.logcommand_file, CONF_STRING, CONF_NORM},
  {"error_file", (VOID *) mudconf.logerror_file, CONF_STRING, CONF_NORM},
  {"panic_file", (VOID *) mudconf.logpanic_file, CONF_STRING, CONF_NORM},

  {"chdir_path", (VOID *) mudconf.chdir_path, CONF_STRING, CONF_INITONLY},
  {"files_path", (VOID *) mudconf.files_path, CONF_STRING, CONF_INITONLY},

  {(char *)NULL, (VOID *)NULL, CONF_BOOL, CONF_NORM}
};

/* read in the configuration file. */
int conf_init(fname)
    char *fname;
{
  register struct conftab *cptr;
  register Hash_Entry *entry;
  int line = 1;
  char buffer[BUFFSIZ];
  FILE *fp;

  /* set up parse table */
  if (!ConfHash_Inited) {
    Hash_InitTable(&ConfHash, 0, HASH_STRCASE_KEYS);
    for(cptr = ctable; cptr->key != (char *)NULL; cptr++){
      entry = Hash_CreateEntry(&ConfHash, cptr->key, NULL);
      Hash_SetValue(entry, (char *)cptr);
    }
    ConfHash_Inited = 1;
  }

  /* initialize suitable defaults -- keep in sync with struct conf */
  strcpy(mudconf.name, "TeenyMUD");

  mudconf.hostnames = 1;
  mudconf.registration = 0;
  mudconf.logcommands = 0;
  mudconf.enable_quota = 1;
  mudconf.enable_money = 1;
  mudconf.enable_groups = 1;
  mudconf.enable_autotoad = 0;
#ifdef notyet
  mudconf.enable_compress = 0;
#endif
  mudconf.wizwhoall = 0;
  mudconf.dark_sleep = 0;
  mudconf.file_access = 0;
  mudconf.file_exec = 0;
  mudconf.enable_rwho = 0;

  mudconf.default_port = 4201;
  mudconf.rwho_port = 6888;
  mudconf.starting_loc = 0;
  mudconf.root_location = 0;
  mudconf.player_god = 1;

  mudconf.parent_depth = 50;
  mudconf.exit_depth = 50;
  mudconf.room_depth = 50;

  mudconf.growth_increment = 1024;
  mudconf.slack = 512;

  mudconf.starting_quota = 250;
  mudconf.starting_money = 100;
  mudconf.daily_paycheck = 50;
  mudconf.queue_commands = 36;
  mudconf.queue_cost = 1;
  mudconf.find_cost = 100;
  mudconf.limfind_cost = 25;
  mudconf.stat_cost = 100;
  mudconf.fullstat_cost = 25;
  mudconf.room_cost = 10;
  mudconf.thing_cost = 10;
  mudconf.exit_cost = 1;
  mudconf.minkill_cost = 10;
  mudconf.maxkill_cost = 100;

  mudconf.max_list = 512;
  mudconf.max_commands = 10;
  mudconf.max_pennies = 10000;

  mudconf.cache_size = 20480;
  mudconf.cache_width = 10;
  mudconf.cache_depth = 10;
 
  mudconf.queue_slice = 10;

  mudconf.build_flags[0] = BUILDER;
  mudconf.build_flags[1] = 0;

  mudconf.robot_flags[0] = ROBOT;
  mudconf.robot_flags[1] = 0;

  mudconf.wizard_flags[0] = 0;
  mudconf.wizard_flags[1] = WIZARD;

  mudconf.god_flags[0] = 0;
  mudconf.god_flags[1] = GOD;

  mudconf.guest_flags[0] = GUEST;
  mudconf.guest_flags[1] = 0;

  mudconf.newp_flags[0] = 0;
  mudconf.newp_flags[1] = 0;

  mudconf.dump_interval = 108000;
  mudconf.login_timeout = 18000;
  mudconf.player_timeout = 216000;
  mudconf.rwho_interval = 200;

  strcpy(mudconf.rwho_passwd, "none");
  strcpy(mudconf.rwho_host, "none");

  strcpy(mudconf.dbm_file, "teeny.dbm");
  strcpy(mudconf.db_file, "teeny.db");
  strcpy(mudconf.help_file, "help.txt");
  strcpy(mudconf.news_file, "news.txt");
  strcpy(mudconf.motd_file, "motd.txt");
  strcpy(mudconf.wizn_file, "wiznews.txt");
  strcpy(mudconf.greet_file, "greet.txt");
  strcpy(mudconf.newp_file, "welcome.txt");
  strcpy(mudconf.register_file, "register.txt");
  strcpy(mudconf.bye_file, "goodbye.txt");
  strcpy(mudconf.shtd_file, "shutdown.txt");
  strcpy(mudconf.guest_file, "guest.txt");
  strcpy(mudconf.timeout_file, "timeout.txt");
  strcpy(mudconf.badconn_file, "badconn.txt");
  strcpy(mudconf.badcrt_file, "badcrt.txt");

  strcpy(mudconf.logstatus_file, "status.log");
  strcpy(mudconf.loggripe_file, "gripe.log");
  strcpy(mudconf.logcommand_file, "commands.log");
  strcpy(mudconf.logerror_file, "errors.log");
  strcpy(mudconf.logpanic_file, "panic.log");

  strcpy(mudconf.chdir_path, ".");
  strcpy(mudconf.files_path, "files/");

  /* these aren't conf options, but need to be initialized here. */
  mudstat.exmotd = (char *)NULL;
  mudstat.exwiz = (char *)NULL;

  /* read in the file */
  fp = fopen(fname, "r");
  if(fp == (FILE *)NULL) {
    logfile(LOG_ERROR, "Unable to load configuration file '%s', %s.\n", fname,
    	    strerror(errno));
    return(-1);
  }
  while(!feof(fp) && (fgets(buffer, BUFFSIZ, fp) != (char *)NULL)) {
    if((buffer[0] == '\n') || (buffer[0] == '#')) {
      line++;
      continue;
    }

    if(conf_parse(buffer) == -1) {
      logfile(LOG_ERROR, "Configuration error at line %d.\n", line);
      return(-1);
    }
    line++;
  }
  fclose(fp);

  return(0);
}

INLINE static void parse_bool(str, ptr)
    register char *str;
    register bool *ptr;
{
  /* default to false */
  if((strcasecmp(str, "yes") == 0)
     || ((str[0] == 'y') && (str[1] == '\0'))
     || (strcasecmp(str, "on") == 0)
     || (strcasecmp(str, "true") == 0)
     || ((str[0] == '1') && (str[1] == '\0'))) {
    *ptr = 1;
  } else {
    *ptr = 0;
  }
}

INLINE static void parse_short(str, ptr)
    register char *str;
    register short *ptr;
{
  *ptr = (short)strtol(str, NULL, 10);
}

INLINE static void parse_int(str, ptr)
    register char *str;
    register int *ptr;
{
  *ptr = (int)strtol(str, NULL, 10);
}

INLINE static void parse_time(str, ptr)
    register char *str;
    register time_t *ptr;
{
  char *sptr;

  *ptr = (time_t)strtol(str, &sptr, 10);
  switch(*sptr) {
  case 'm':
    *ptr = (*ptr) * 60;
    break;
  case 'h':
    *ptr = (*ptr) * 3600;
    break;
  case 'd':
    *ptr = (*ptr) * 86400;
    break;
  }
}

#ifndef INTERNAL
static void unparse_time(buffer, blen, key, val)
    char *buffer;
    size_t blen;
    char *key;
    time_t val;
{
  if((val % 86400) == 0) {		/* days */
    snprintf(buffer, blen, "[%s] = %ldd", key, (val / 86400));
  } else if((val % 3600) == 0) {	/* hours */
    snprintf(buffer, blen, "[%s] = %ldh", key, (val / 3600));
  } else if((val % 60) == 0) {		/* minutes */
    snprintf(buffer, blen, "[%s] = %ldm", key, (val / 60));
  } else
    snprintf(buffer, blen, "[%s] = %ld", key, val);
}

VOID do_config(player, cause, switches, argone, argtwo)
    int player, cause, switches;
    char *argone, *argtwo;
{
  static char err[] = "But what do you want to do with the config?";
  char buff[MEDBUFFSIZ];
  register Hash_Entry *entry;
  register struct conftab *cptr;

  if(switches & CONFIG_SET) {
    if((argone == (char *)NULL) || (argone[0] == '\0')
       || (argtwo == (char *)NULL) || (argtwo[0] == '\0')) {
      notify_player(player, cause, player, err, 0);
      return;
    }

    entry = Hash_FindEntry(&ConfHash, argone);
    if(entry != (Hash_Entry *)NULL) {
      cptr = (struct conftab *)Hash_GetValue(entry);

      if(cptr->flags & CONF_INITONLY) {		/* security */
        notify_player(player, cause, player,
		      "That may not be changed now.", 0);
	return;
      }

      switch(cptr->typ) {
      case CONF_BOOL:
	parse_bool(argtwo, (bool *)cptr->ptr);
	break;
      case CONF_FLAGS:
	parse_flags(argtwo, (int *)cptr->ptr);
	break;
      case CONF_SHORT:
	parse_short(argtwo, (short *)cptr->ptr);
	break;
      case CONF_INT:
	parse_int(argtwo, (int *)cptr->ptr);
	break;
      case CONF_TIME:
	parse_time(argtwo, (time_t *)cptr->ptr);
	break;
      case CONF_STRING:
	if(strlen(argtwo) > 127)
	  argtwo[127] = '\0';
	strcpy((char *)cptr->ptr, argtwo);
	break;
      default:
	notify_bad(player);
	return;
      }
      notify_player(player, cause, player, "Configuration set.", 0);
      return;
    } else {
      notify_player(player, cause, player, "No such configuration option.", 0);
      return;
    }
  } else if(switches & CONFIG_ALIAS) {
    register char *swptr;

    if((argone == (char *)NULL) || (argone[0] == '\0')
       || (argtwo == (char *)NULL) || (argtwo[0] == '\0')) {
      notify_player(player, cause, player, err, 0);
      return;
    }

    for(swptr = argone; *swptr && (*swptr != '/'); swptr++);
    if(*swptr == '/') {
      *swptr++ = '\0';
    } else {
      swptr = (char *)NULL;
    }
    if(command_alias(argone, argtwo, swptr) == -1) {
      notify_player(player, cause, player, "Error aliasing command.", 0);
    } else {
      notify_player(player, cause, player, "Command aliased.", 0);
    }
    return;
  } else if(switches & CONFIG_UNALIAS) {
    if((argone == (char *)NULL) || (argone[0] == '\0')) {
      notify_player(player, cause, player, err, 0);
      return;
    }

    if(command_unalias(argone) == -1)
      notify_player(player, cause, player, "Error removing command alias.", 0);
    else
      notify_player(player, cause, player, "Command alias removed.", 0);
    return;
  } else { /* display configuration */
    if((argone == (char *)NULL) || (argone[0] == '\0')) {
      for(cptr = ctable; cptr->key != (char *)NULL; cptr++) {
	conf_dumpopt(player, cause, cptr, buff, sizeof(buff));
      }
    } else {
      entry = Hash_FindEntry(&ConfHash, argone);
      if(entry != (Hash_Entry *)NULL) {
	conf_dumpopt(player, cause, (struct conftab *)Hash_GetValue(entry),
		     buff, sizeof(buff));
      }
    }
    notify_player(player, cause, player, "***End of list***", 0);
  }
}

static void conf_dumpopt(player, cause, cptr, buffer, blen)
    int player, cause;
    register struct conftab *cptr;
    register char *buffer;
    size_t blen;
{
  register short *sptr;		/* Compiler GOO. */
  register int *iptr;		/* Compiler GOO. */
  register bool *bptr;		/* Compiler GOO. */
  register time_t *tptr;	/* Compiler GOO. */

  switch(cptr->typ) {
  case CONF_BOOL:
    bptr = (bool *)cptr->ptr;
    snprintf(buffer, blen, "[%s] = %s", cptr->key, ((*bptr) ? "on" : "off"));
    break;
  case CONF_FLAGS:
    snprintf(buffer, blen, "[%s] = %s", cptr->key,
	    display_objflags((int *)cptr->ptr, 1));
    break;
  case CONF_SHORT:
    sptr = (short *)cptr->ptr;
    snprintf(buffer, blen, "[%s] = %d", cptr->key, (*sptr));
    break;
  case CONF_INT:
    iptr = (int *)cptr->ptr;
    snprintf(buffer, blen, "[%s] = %d", cptr->key, (*iptr));
    break;
  case CONF_TIME:
    tptr = (time_t *)cptr->ptr;
    unparse_time(buffer, blen, cptr->key, (*tptr));
    break;
  case CONF_STRING:
    snprintf(buffer, blen, "[%s] = %s", cptr->key, (char *)cptr->ptr);
    break;
  }
  notify_player(player, cause, player, buffer, 0);
}
#endif			/* INTERNAL */

static int conf_parse(line)
    char *line;
{
  register char *ptr, *eptr;
  register Hash_Entry *entry;
  register struct conftab *cptr;

  /* remove leading whitespace */
  while(*line && isspace(*line))
    line++;
  if(!*line)
    return(1);

  /* parse out first word */
  for(ptr = line; *ptr && !isspace(*ptr); ptr++);
  if(!*ptr)
    return(-1);
  for(*ptr++ = '\0'; *ptr && isspace(*ptr); ptr++);
  if(!*ptr)
    return(-1);
  /* remove trailing whitespace */
  for(eptr = &ptr[strlen(ptr)-1]; (eptr > ptr)
      && ((*eptr == '\n') || isspace(*eptr)); *eptr-- = '\0');

  entry = Hash_FindEntry(&ConfHash, line);
  if(entry != (Hash_Entry *)NULL) {
    cptr = (struct conftab *)Hash_GetValue(entry);

    switch(cptr->typ) {
    case CONF_BOOL:
      parse_bool(ptr, (bool *)cptr->ptr);
      break;
    case CONF_FLAGS:
      parse_flags(ptr, (int *)cptr->ptr);
      break;
    case CONF_SHORT:
      parse_short(ptr, (short *)cptr->ptr);
      break;
    case CONF_INT:
      parse_int(ptr, (int *)cptr->ptr);
      break;
    case CONF_TIME:
      parse_time(ptr, (time_t *)cptr->ptr);
      break;
    case CONF_STRING:
      if(strlen(ptr) > 127)
        ptr[127] = '\0';
      strcpy((char *)cptr->ptr, ptr);
      break;
    }

    return(0);
  } else {
#ifndef INTERNAL
    register char *swptr;

    /* try other hard coded keywords. */
    if(strcasecmp(line, "alias") == 0) {	/* command alias */
      /* parse out the next argument */
      for(eptr = ptr; *eptr && !isspace(*eptr); eptr++);
      if(!*eptr)
        return(-1);
      *eptr++ = '\0';
      while(*eptr && isspace(*eptr))
        eptr++;
      if(!*eptr)
        return(-1);

      for(swptr = ptr; *swptr && (*swptr != '/'); swptr++);
      if(*swptr == '/') {
	*swptr++ = '\0';
      } else {
	swptr = (char *)NULL;
      }
      /* ptr is the command, eptr is the alias, swptr is switches */
      return(command_alias(ptr, eptr, swptr));
    } else if(strcasecmp(line, "unalias") == 0) {
      /* only need one argument */
      return(command_unalias(ptr));
    } else {		/* default: site lockout/allow string */
      /* parse out the next argument */
      for(eptr = ptr; *eptr && !isspace(*eptr); eptr++);
      if(*eptr) {
        *eptr++ = '\0';
        while(*eptr && isspace(*eptr))
          eptr++;
      } else
        eptr = (char *)NULL;	/* messages can be null, damnit */

      /* line is the command, ptr is the host, and eptr is the message. */
      return(lockout_add(line, ptr, eptr));
    }
#else
    return(0);
#endif			/* INTERNAL */
  }
  /*return(-1);*/
}
