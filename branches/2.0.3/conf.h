/* conf.h */

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

#ifndef __CONF_H

/* dynamic configuration system. */

typedef unsigned char bool;

struct mconf {
  char name[128];		/* server name */

  bool hostnames;		/* look up hostnames */
  bool registration;		/* disable all player creation */
  bool logcommands;		/* log all commands */
  bool enable_quota;		/* enable object quota */
  bool enable_money;		/* enable pennies */
  bool enable_groups;		/* enable groups */
  bool enable_autotoad;		/* enable AutoToad(tm) */
#ifdef notyet
  bool enable_compress;		/* enable compression */
#endif
  bool wizwhoall;		/* show the wiz who list to all */
  bool dark_sleep;		/* make sleeping players invis */
  bool file_access;		/* allow objects to access files */
  bool file_exec;		/* allow objects to exec files */
  bool enable_rwho;		/* talk to MJR Rwho servers */

  int default_port;		/* default player port number */
  int rwho_port;		/* Rwho server port number */
  int starting_loc;		/* starting location for new players */
  int root_location;		/* root room location */
  int player_god;		/* default god */

  short parent_depth;		/* the maximum depth of a parent chain */
  short exit_depth;		/* the maximum nesting level of exits */
  short room_depth;		/* the maximum nesting of rooms */

  int growth_increment;		/* db array growth increment */
  int slack;			/* db array slack objects */

  short starting_quota;		/* starting quota for new players */
  short starting_money;		/* starting money for new players */
  short daily_paycheck;		/* the daily paycheck for players */
  short queue_commands;		/* rate of charging for queue use */
  short queue_cost;		/* cost of running queue_commands */
  short find_cost;		/* cost of a @find */
  short limfind_cost;		/* cost of a limited @find */
  short stat_cost;		/* cost of a @stat */
  short fullstat_cost;		/* cost of a global @stat */
  short room_cost;		/* cost to create a room */
  short thing_cost;		/* cost to create an object */
  short exit_cost;		/* cost to create an exit */
  short minkill_cost;		/* minimum cost to kill */
  short maxkill_cost;		/* maximum cost to kill */

  short max_list;		/* maximum length of a (displayed) list */
  short max_commands;		/* maximum commands from a connection/second */
  int max_pennies;		/* maximum wealth of a player */

  int cache_size;		/* maximum size of the object cache */
  int cache_width;		/* default width of the cache */
  int cache_depth;		/* depth of the cache */
  
  short queue_slice;		/* maximum nuber of cmds run from queue/sec */

  int build_flags[2];		/* flags required to build */
  int robot_flags[2];		/* flags required to use OUTPUT*X */
  int wizard_flags[2];		/* flags required to be a wizard */
  int god_flags[2];		/* flags required to be a god */
  int guest_flags[2];		/* flags required to be a guest */
  int newp_flags[2];		/* flags given to new players */

  time_t dump_interval;		/* time between database backups */
  time_t login_timeout;		/* time before login is timed out */
  time_t player_timeout;	/* time before connected player is timed out */
  time_t rwho_interval;		/* time between rwho pings */

  char rwho_passwd[128];	/* Rwho password */
  char rwho_host[128];		/* Rwho hostname */
  
  char dbm_file[128];		/* dbm file name */
  char db_file[128];		/* db file name */
  char help_file[128];		/* help file name */
  char news_file[128];		/* news file name */
  char motd_file[128];		/* motd file name */
  char wizn_file[128];		/* wizard motd file name */
  char greet_file[128];		/* greet file for primary port */
  char newp_file[128];		/* new player motd file name */
  char register_file[128];	/* registration required file name */
  char bye_file[128];		/* goodbye message file name */
  char shtd_file[128];		/* shutdown message file name */
  char guest_file[128];		/* guest motd file name */
  char timeout_file[128];	/* timeout message file name */
  char badconn_file[128];	/* bad connection message file name */
  char badcrt_file[128];	/* bad creation message file name */

  char logstatus_file[128];	/* status log file name */
  char loggripe_file[128];	/* gripe log file name */
  char logcommand_file[128];	/* command log file name */
  char logerror_file[128];	/* error log file name */
  char logpanic_file[128];	/* panic log file name */

  char chdir_path[128];		/* somewhere to chdir upon startup */
  char files_path[128];		/* leading path for object files */
};

struct mstat {
  time_t now;			/* current time */
  time_t lasttime;		/* last time we did timers */
  time_t startup;		/* boot time */
  time_t lastdump;		/* last time we dumped */
  time_t lastrwho;		/* last time we pinged rwho */

  /* server status */
  enum { MUD_COLD, MUD_RUNNING, MUD_STOPPED, MUD_DUMPING } status;

  /* logins */
  enum { LOGINS_ENABLED, LOGINS_DISABLED } logins;

  int cache_usage;		/* amount of data in the cache */
  int cache_hits;		/* number of cache hits */
  int cache_misses;		/* number of cache misses */
  int cache_errors;		/* number of database errors */

  int total_objects;            /* the total number of objects */
  int actual_objects;           /* the total number of real objects */
  int garbage_count;            /* the total number of garbage objects */
  int slack;                    /* empty slots in the index */

  char *exmotd;			/* extra motd string. */
  char *exwiz;			/* extra wizard motd string. */
};

/* our configuration */
extern struct mconf mudconf;

/* our stats */
extern struct mstat mudstat;

#define __CONF_H
#endif				/* __CONF_H */
