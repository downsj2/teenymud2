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
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif			/* HAVE_STDLIB_H */
#include <ctype.h>
#include <errno.h>

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
    CONF_STRING,
    CONF_SARRAY
} cf_typ;

#define CONF_NORM	0x00
#define CONF_INITONLY	0x01
#define CONF_PRIV	0x02	/* Requires GOD privs to even see. */

static int ConfHash_Inited = 0;
static Hash_Table ConfHash;

static int conf_load _ANSI_ARGS_((char *));
static int conf_parse _ANSI_ARGS_((char *, char *));
static void parse_bool _ANSI_ARGS_((char *, bool *));
static void parse_short _ANSI_ARGS_((char *, short *));
static void parse_int _ANSI_ARGS_((char *, int *));
static void parse_string _ANSI_ARGS_((char *, char **));
static int parse_sarray _ANSI_ARGS_((char *, char *[]));
#ifndef INTERNAL
static void unparse_time _ANSI_ARGS_((char *, size_t, char *, time_t));
static void unparse_sarray _ANSI_ARGS_((char *, size_t, char *, char *[]));
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
  {"mudname", (VOID *) &mudconf.name, CONF_STRING, CONF_INITONLY},

  {"hostnames", (VOID *) &mudconf.hostnames, CONF_BOOL, CONF_NORM},
  {"registration", (VOID *) &mudconf.registration, CONF_BOOL, CONF_NORM},
  {"registername", (VOID *) &mudconf.registername, CONF_BOOL, CONF_NORM},
  {"logcommands", (VOID *) &mudconf.logcommands, CONF_BOOL, CONF_PRIV},
  {"enable_quota", (VOID *) &mudconf.enable_quota, CONF_BOOL, CONF_NORM},
  {"enable_money", (VOID *) &mudconf.enable_money, CONF_BOOL, CONF_NORM},
  {"enable_groups", (VOID *) &mudconf.enable_groups, CONF_BOOL, CONF_NORM},
  {"enable_autotoad", (VOID *) &mudconf.enable_autotoad, CONF_BOOL, CONF_NORM},
  {"enable_autohome", (VOID *) &mudconf.enable_autohome, CONF_BOOL, CONF_NORM},
  {"enable_pueblo", (VOID *) &mudconf.enable_pueblo, CONF_BOOL, CONF_NORM},
#ifdef notyet
  {"enable_compress", (VOID *) &mudconf.enable_compress, CONF_BOOL,
							 CONF_INITONLY},
#endif
  {"wizwhoall", (VOID *) &mudconf.wizwhoall, CONF_BOOL, CONF_NORM},
  {"dark_sleep", (VOID *) &mudconf.dark_sleep, CONF_BOOL, CONF_NORM},
  {"compact_contents", (VOID *) &mudconf.compact_contents, CONF_BOOL,
							   CONF_NORM},
  {"file_access", (VOID *) &mudconf.file_access, CONF_BOOL, CONF_INITONLY},
  {"file_exec", (VOID *) &mudconf.file_exec, CONF_BOOL, CONF_INITONLY},
  {"enable_rwho", (VOID *) &mudconf.enable_rwho, CONF_BOOL, CONF_INITONLY},
  {"hfile_autoload", (VOID *) &mudconf.hfile_autoload, CONF_BOOL, CONF_NORM},

  {"default_port", (VOID *) &mudconf.default_port, CONF_INT, CONF_INITONLY},
  {"rwho_port", (VOID *) &mudconf.rwho_port, CONF_INT,
  					     CONF_INITONLY|CONF_PRIV},
  {"starting_loc", (VOID *) &mudconf.starting_loc, CONF_INT, CONF_NORM},
  {"root_location", (VOID *) &mudconf.root_location, CONF_INT, CONF_NORM},
  {"player_god", (VOID *) &mudconf.player_god, CONF_INT,
  					       CONF_INITONLY|CONF_PRIV},

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

  {"bad_pnames", (VOID *) mudconf.bad_pnames, CONF_SARRAY, CONF_NORM},

  {"dump_interval", (VOID *) &mudconf.dump_interval, CONF_TIME, CONF_NORM},
  {"login_timeout", (VOID *) &mudconf.login_timeout, CONF_TIME, CONF_NORM},
  {"player_timeout", (VOID *) &mudconf.player_timeout, CONF_TIME, CONF_NORM},
  {"rwho_interval", (VOID *) &mudconf.rwho_interval, CONF_TIME, CONF_NORM},

  {"money_penny", (VOID *) &mudconf.money_penny, CONF_STRING, CONF_NORM},
  {"money_nickle", (VOID *) &mudconf.money_nickle, CONF_STRING, CONF_NORM},
  {"money_dime", (VOID *) &mudconf.money_dime, CONF_STRING, CONF_NORM},
  {"money_quarter", (VOID *) &mudconf.money_quarter, CONF_STRING, CONF_NORM},
  {"money_dollar", (VOID *) &mudconf.money_dollar, CONF_STRING, CONF_NORM},

  {"money_pennies", (VOID *) &mudconf.money_pennies, CONF_STRING, CONF_NORM},
  {"money_nickles", (VOID *) &mudconf.money_nickles, CONF_STRING, CONF_NORM},
  {"money_dimes", (VOID *) &mudconf.money_dimes, CONF_STRING, CONF_NORM},
  {"money_quarters", (VOID *) &mudconf.money_quarters, CONF_STRING, CONF_NORM},
  {"money_dollars", (VOID *) &mudconf.money_dollars, CONF_STRING, CONF_NORM},

  {"rwho_passwd", (VOID *) &mudconf.rwho_passwd, CONF_STRING,
  					         CONF_INITONLY|CONF_PRIV},
  {"rwho_host", (VOID *) &mudconf.rwho_host, CONF_STRING,
  					     CONF_INITONLY|CONF_PRIV},

  {"dbm_file", (VOID *) &mudconf.dbm_file, CONF_STRING, CONF_INITONLY},
  {"db_file", (VOID *) &mudconf.db_file, CONF_STRING, CONF_INITONLY},
  {"help_file", (VOID *) &mudconf.help_file, CONF_STRING, CONF_NORM},
  {"news_file", (VOID *) &mudconf.news_file, CONF_STRING, CONF_NORM},
  {"motd_file", (VOID *) &mudconf.motd_file, CONF_STRING, CONF_NORM},
  {"wizn_file", (VOID *) &mudconf.wizn_file, CONF_STRING, CONF_NORM},
  {"greet_file", (VOID *) &mudconf.greet_file, CONF_STRING, CONF_NORM},
  {"newp_file", (VOID *) &mudconf.newp_file, CONF_STRING, CONF_NORM},
  {"register_file", (VOID *) &mudconf.register_file, CONF_STRING, CONF_NORM},
  {"bye_file", (VOID *) &mudconf.bye_file, CONF_STRING, CONF_NORM},
  {"shtd_file", (VOID *) &mudconf.shtd_file, CONF_STRING, CONF_NORM},
  {"guest_file", (VOID *) &mudconf.guest_file, CONF_STRING, CONF_NORM},
  {"timeout_file", (VOID *) &mudconf.timeout_file, CONF_STRING, CONF_NORM},
  {"badconn_file", (VOID *) &mudconf.badconn_file, CONF_STRING, CONF_NORM},
  {"badcrt_file", (VOID *) &mudconf.badcrt_file, CONF_STRING, CONF_NORM},

  {"help_url", (VOID *) &mudconf.help_url, CONF_STRING, CONF_NORM},
  {"news_url", (VOID *) &mudconf.news_url, CONF_STRING, CONF_NORM},

  {"status_file", (VOID *) &mudconf.logstatus_file, CONF_STRING, CONF_NORM},
  {"gripe_file", (VOID *) &mudconf.loggripe_file, CONF_STRING, CONF_NORM},
  {"command_file", (VOID *) &mudconf.logcommand_file, CONF_STRING, CONF_NORM},
  {"error_file", (VOID *) &mudconf.logerror_file, CONF_STRING, CONF_NORM},
  {"panic_file", (VOID *) &mudconf.logpanic_file, CONF_STRING, CONF_NORM},

  {"chdir_path", (VOID *) &mudconf.chdir_path, CONF_STRING,
  					       CONF_INITONLY|CONF_PRIV},
  {"files_path", (VOID *) &mudconf.files_path, CONF_STRING,
  					       CONF_INITONLY|CONF_PRIV},

  {(char *)NULL, (VOID *)NULL, CONF_BOOL, CONF_NORM}
};

/* read in the configuration file. */
int conf_init(fname)
    char *fname;
{
  struct conftab *cptr;
  Hash_Entry *entry;

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
  mudconf.name = ty_strdup("TeenyMUD", "conf_init");

#ifdef notyet
  mudconf.peers = (struct mpeer *)NULL;	/* linked list. */
#endif

  mudconf.hostnames = 1;
  mudconf.registration = 0;
  mudconf.registername = 1;
  mudconf.logcommands = 0;
  mudconf.enable_quota = 1;
  mudconf.enable_money = 1;
  mudconf.enable_groups = 1;
  mudconf.enable_autotoad = 0;
  mudconf.enable_autohome = 0;
  mudconf.enable_pueblo = 0;
  mudconf.wizwhoall = 0;
  mudconf.dark_sleep = 0;
  mudconf.compact_contents = 0;
  mudconf.file_access = 0;
  mudconf.file_exec = 0;
  mudconf.enable_rwho = 0;
  mudconf.hfile_autoload = 1;

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

  bzero((VOID *)mudconf.bad_pnames, sizeof(mudconf.bad_pnames));

  mudconf.dump_interval = 108000;
  mudconf.login_timeout = 18000;
  mudconf.player_timeout = 216000;
  mudconf.rwho_interval = 200;

  mudconf.money_penny = ty_strdup("penny", "conf_init");
  mudconf.money_nickle = ty_strdup("nickle", "conf_init");
  mudconf.money_dime = ty_strdup("dime", "conf_init");
  mudconf.money_quarter = ty_strdup("quarter", "conf_init");
  mudconf.money_dollar = ty_strdup("dollar", "conf_init");

  mudconf.money_pennies = ty_strdup("pennies", "conf_init");
  mudconf.money_nickles = ty_strdup("nickles", "conf_init");
  mudconf.money_dimes = ty_strdup("dimes", "conf_init");
  mudconf.money_quarters = ty_strdup("quarters", "conf_init");
  mudconf.money_dollars = ty_strdup("dollars", "conf_init");

  mudconf.rwho_passwd = ty_strdup("none", "conf_init");
  mudconf.rwho_host = ty_strdup("none", "conf_init");

  mudconf.dbm_file = ty_strdup("teeny.dbm", "conf_init");
  mudconf.db_file = ty_strdup("teeny.db", "conf_init");
  mudconf.help_file = ty_strdup("help.txt", "conf_init");
  mudconf.news_file = ty_strdup("news.txt", "conf_init");
  mudconf.motd_file = ty_strdup("motd.txt", "conf_init");
  mudconf.wizn_file = ty_strdup("wiznews.txt", "conf_init");
  mudconf.greet_file = ty_strdup("greet.txt", "conf_init");
  mudconf.newp_file = ty_strdup("welcome.txt", "conf_init");
  mudconf.register_file = ty_strdup("register.txt", "conf_init");
  mudconf.bye_file = ty_strdup("goodbye.txt", "conf_init");
  mudconf.shtd_file = ty_strdup("shutdown.txt", "conf_init");
  mudconf.guest_file = ty_strdup("guest.txt", "conf_init");
  mudconf.timeout_file = ty_strdup("timeout.txt", "conf_init");
  mudconf.badconn_file = ty_strdup("badconn.txt", "conf_init");
  mudconf.badcrt_file = ty_strdup("badcrt.txt", "conf_init");

  mudconf.help_url =
	ty_strdup("<A HREF=\"http://www.teeny.org/help/\">Help@teeny.org</A>",
		  "conf_init");
  mudconf.news_url =
	ty_strdup("<A HREF=\"http://www.teeny.org/news/\">News@teeny.org</A>",
		  "conf_init");

  mudconf.logstatus_file = ty_strdup("status.log", "conf_init");
  mudconf.loggripe_file = ty_strdup("gripe.log", "conf_init");
  mudconf.logcommand_file = ty_strdup("commands.log", "conf_init");
  mudconf.logerror_file = ty_strdup("errors.log", "conf_init");
  mudconf.logpanic_file = ty_strdup("panic.log", "conf_init");

  mudconf.chdir_path = ty_strdup(".", "conf_init");
  mudconf.files_path = ty_strdup("files/", "conf_init");

  /* these aren't conf options, but need to be initialized here. */
  mudstat.exmotd = (char *)NULL;
  mudstat.exwiz = (char *)NULL;

  return(conf_load(fname));
}

/*
 * What actually reads the file.  Can be recursive.
 */
static int conf_load(fname)
    char *fname;
{
  FILE *fp;
  char buffer[BUFFSIZ];
  int line = 1;

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

    if(conf_parse(buffer, fname) == -1) {
      logfile(LOG_ERROR, "Configuration error in file '%s' at line %d.\n",
	      fname, line);
      return(-1);
    }
    line++;
  }
  fclose(fp);

  return(0);
}

static void parse_bool(str, ptr)
    char *str;
    bool *ptr;
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

static void parse_short(str, ptr)
    char *str;
    short *ptr;
{
  *ptr = (short)strtol(str, NULL, 10);
}

static void parse_int(str, ptr)
    char *str;
    int *ptr;
{
  *ptr = (int)strtol(str, NULL, 10);
}

static void parse_string(str, ptr)
    char *str, **ptr;
{
  ty_free(*ptr);
  *ptr = ty_strdup(str, "parse_string");
}

static int parse_sarray(sptr, ptr)
    char *sptr;
    char *ptr[];
{
  int index;
  char *qptr;

  for(index = 0; index < 64; index++) {
    if(ptr[index] != (char *)NULL) {
      ty_free(ptr[index]);
      ptr[index] = (char *)NULL;
    }
  }

  for(index = 0; index < 64; index++) {
    if(*sptr) {
      while(*sptr && isspace(*sptr))
        sptr++;
      if(*sptr && (*sptr == '{')) {
        sptr++;
	for(qptr = sptr; *qptr && (*qptr != '}'); qptr++);
	if(*qptr) {
	  *qptr++ = '\0';
	  ptr[index] = ty_strdup(sptr, "parse_sarray.ptr");
	  sptr = qptr;
	} else
	  goto sarray_fail;
      } else if(*sptr && (*sptr == '"')) {
        sptr++;
	for(qptr = sptr; *qptr && (*qptr != '"'); qptr++);
	if(*qptr) {
	  *qptr++ = '\0';
	  ptr[index] = ty_strdup(sptr, "parse_sarray.ptr");
	  sptr = qptr;
	} else
	  goto sarray_fail;
      } else if(*sptr) {
        for(qptr = sptr; *qptr && !isspace(*qptr); qptr++);
	if(*qptr)
	  *qptr++ = '\0';

	ptr[index] = ty_strdup(sptr, "parse_sarray.ptr");
	sptr = qptr;
      }
    } else
      break;
  }
  ptr[index] = (char *)NULL;

  return(0);

sarray_fail:

  for(index = 0; index < 64; index++) {
    if(ptr[index] != (char *)NULL) {
      ty_free(ptr[index]);
      ptr[index] = (char *)NULL;
    }
  }
  return(-1);
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

static void unparse_sarray(buffer, blen, key, val)
    char *buffer;
    size_t blen;
    char *key;
    char *val[];
{
  int index;
  int doneone = 0;

  snprintf(buffer, blen, "[%s] = ", key);
  if (val != (char **)NULL) {
    for(index = 0; index < 64; index++) {
      if(val[index] != (char *)NULL) {
        if(strchr(val[index], ' ') == (char *)NULL) {
          strncat(buffer, val[index], (blen - strlen(buffer) - 1));
	  strncat(buffer, ", ", (blen - strlen(buffer) - 1));
        } else {
          snprintf(&buffer[strlen(buffer)],
		   (blen - strlen(buffer) - 1), "\"%s\", ", val[index]);
        }
	doneone++;
      } else
        break;
    }
  }

  if (doneone)
    buffer[strlen(buffer) - 2] = '\0';
}

VOID do_config(player, cause, switches, argone, argtwo)
    int player, cause, switches;
    char *argone, *argtwo;
{
  static char err[] = "But what do you want to do with the config?";
  char buff[MEDBUFFSIZ];
  Hash_Entry *entry;
  struct conftab *cptr;

  if(switches & CONFIG_SET) {
    if((argone == (char *)NULL) || (argone[0] == '\0')
       || (argtwo == (char *)NULL) || (argtwo[0] == '\0')) {
      if(!(switches & CMD_QUIET))
        notify_player(player, cause, player, err, NOT_QUIET);
      return;
    }

    entry = Hash_FindEntry(&ConfHash, argone);
    if(entry != (Hash_Entry *)NULL) {
      cptr = (struct conftab *)Hash_GetValue(entry);

      if(cptr->flags & CONF_INITONLY) {		/* security */
        if(!(switches & CMD_QUIET))
          notify_player(player, cause, player,
		        "That may not be changed now.", NOT_QUIET);
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
        parse_string(argtwo, (char **)cptr->ptr);
	break;
      case CONF_SARRAY:
        parse_sarray(argtwo, (char **)cptr->ptr);
	break;
      default:
	notify_bad(player);
	return;
      }

      if(!(switches & CMD_QUIET))
        notify_player(player, cause, player, "Configuration set.", NOT_QUIET);
      return;
    } else {
      if(!(switches & CMD_QUIET))
        notify_player(player, cause, player, "No such configuration option.",
		      NOT_QUIET);
      return;
    }
  } else if(switches & CONFIG_ALIAS) {
    char *swptr;

    if((argone == (char *)NULL) || (argone[0] == '\0')
       || (argtwo == (char *)NULL) || (argtwo[0] == '\0')) {
      if(!(switches & CMD_QUIET))
        notify_player(player, cause, player, err, NOT_QUIET);
      return;
    }

    for(swptr = argone; *swptr && (*swptr != '/'); swptr++);
    if(*swptr == '/') {
      *swptr++ = '\0';
    } else {
      swptr = (char *)NULL;
    }
    if(command_alias(argone, argtwo, swptr) == -1) {
      if(!(switches & CMD_QUIET))
        notify_player(player, cause, player, "Error aliasing command.",
		      NOT_QUIET);
    } else if(!(switches & CMD_QUIET)) {
      notify_player(player, cause, player, "Command aliased.", NOT_QUIET);
    }
    return;
  } else if(switches & CONFIG_UNALIAS) {
    if((argone == (char *)NULL) || (argone[0] == '\0')) {
      if(!(switches & CMD_QUIET))
        notify_player(player, cause, player, err, NOT_QUIET);
      return;
    }

    if(command_unalias(argone) == -1) {
      if(!(switches & CMD_QUIET))
        notify_player(player, cause, player, "Error removing command alias.",
		      NOT_QUIET);
    } else if(!(switches & CMD_QUIET)) {
      notify_player(player, cause, player, "Command alias removed.", NOT_QUIET);
    }
    return;
  } else if(switches & CONFIG_EXPAND) {
    char retbuf[128];

    if((argone == (char *)NULL) || (argone[0] == '\0')) {
      if(!(switches & CMD_QUIET))
        notify_player(player, cause, player, err, NOT_QUIET);
      return;
    }

    if(command_aliasexp(argone, retbuf, sizeof(retbuf)) == -1) {
      if(!(switches & CMD_QUIET))
        notify_player(player, cause, player, "Error expanding alias.",
		      NOT_QUIET);
    } else {
      snprintf(buff, sizeof(buff), "[%s] = %s", argone, retbuf);
      notify_player(player, cause, player, buff, 0);
    }
  } else { /* display configuration */
    if((argone == (char *)NULL) || (argone[0] == '\0')) {
      for(cptr = ctable; cptr->key != (char *)NULL; cptr++) {
        if((cptr->flags & CONF_PRIV) && !isGOD(player))
	  continue;
	conf_dumpopt(player, cause, cptr, buff, sizeof(buff));
      }
      notify_player(player, cause, player, "***End of list***", 0);
    } else {
      entry = Hash_FindEntry(&ConfHash, argone);
      if(entry != (Hash_Entry *)NULL) {
        cptr = (struct conftab *)Hash_GetValue(entry);

        if((cptr->flags & CONF_PRIV) && !isGOD(player))
	  notify_player(player, cause, player, "Permission denied.", 0);
	else
	  conf_dumpopt(player, cause, cptr, buff, sizeof(buff));
      }
    }
  }
}

static void conf_dumpopt(player, cause, cptr, buffer, blen)
    int player, cause;
    struct conftab *cptr;
    char *buffer;
    size_t blen;
{
  short *sptr;		/* Compiler GOO. */
  int *iptr;		/* Compiler GOO. */
  bool *bptr;		/* Compiler GOO. */
  time_t *tptr;		/* Compiler GOO. */
  char **chptr;		/* Compiler GOO. */

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
    chptr = (char **)cptr->ptr;
    snprintf(buffer, blen, "[%s] = %s", cptr->key, (*chptr));
    break;
  case CONF_SARRAY:
    unparse_sarray(buffer, blen, cptr->key, (char **)cptr->ptr);
    break;
  }
  notify_player(player, cause, player, buffer, 0);
}
#endif			/* INTERNAL */

static int conf_parse(line, fname)
    char *line, *fname;
{
  char *ptr, *eptr;
  Hash_Entry *entry;
  struct conftab *cptr;

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
      parse_string(ptr, (char **)cptr->ptr);
      break;
    case CONF_SARRAY:
      parse_sarray(ptr, (char **)cptr->ptr);
      break;
    }

    return(0);
  } else {
    /* try other hard coded keywords. */
    if(strcasecmp(line, "include") == 0) {	/* include file */
      /* ptr is the file name. */
      if(strcmp(fname, ptr) == 0)	/* can't include ourself. */
	return(-1);

      /* launch it. */
      return(conf_load(ptr));
    }
#ifndef INTERNAL
    else if(strcasecmp(line, "alias") == 0) {	/* command alias */
      char *swptr;
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
#ifdef notyet
    } else if(strcasecmp(line, "peer") == 0) {
      struct mpeer *newp;
      char *kptr;

      newp = (struct mpeer *)ty_malloc(sizeof(struct mpeer), "conf_parse.newp");

      /* parse out the next argument */
      for(eptr = ptr; *eptr && !isspace(*eptr); eptr++);
      if(!*eptr) {
	ty_free((VOID *)newp);
        return(-1);
      }
      *eptr++ = '\0';
      while(*eptr && isspace(*eptr))
        eptr++;
      if(!*eptr) {
	ty_free((VOID *)newp);
        return(-1);
      }

      /* ptr is hostname, eptr is port and password. */
      newp->hostname = ty_strdup(ptr, "conf_parse.newp");
      newp->port = (int)strtol(eptr, &kptr, 10);
      if(kptr == eptr) {
	ty_free((VOID *)newp->hostname);
	ty_free((VOID *)newp);
	return(-1);
      }
      
      /* parse out password. */
      while(*kptr && isspace(*kptr))
	kptr++;
      if(!*kptr)
	newp->pword = (char *)NULL;
      else
	newp->pword = ty_strdup(kptr, "conf_parse.newp");

      /* link it in. */
      newp->next = mudconf.peers;
      mudconf.peers = newp;
#endif
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
