/* externs.h */

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

#ifndef __EXTERNS_H

#if defined(HAVE_STDARG_H) && defined(__STDC__)
#include <stdarg.h>
#endif				/* HAVE_STDARG_H && __STDC__ */

/* from cache.c */
extern void cache_init _ANSI_ARGS_((void));
extern void cache_lock _ANSI_ARGS_((void));
extern void cache_unlock _ANSI_ARGS_((void));
extern void cache_trim _ANSI_ARGS_((void));
extern void cache_flush _ANSI_ARGS_((void));

/* from conf.c */
extern int conf_init _ANSI_ARGS_((char *));

/* from lockout.c */
extern int lockout_add _ANSI_ARGS_((char *, char *, char *));
extern int check_host _ANSI_ARGS_((char *, int, char **));

/* from db.c */
extern int get_str_elt _ANSI_ARGS_((int, int, char **));
extern int get_array_elt _ANSI_ARGS_((int, int, int **));
extern int get_flags_elt _ANSI_ARGS_((int, int, int *));
extern int get_int_elt _ANSI_ARGS_((int, int, int *));
extern int set_str_elt _ANSI_ARGS_((int, int, char *));
extern int set_array_elt _ANSI_ARGS_((int, int, int *));
extern int set_flags_elt _ANSI_ARGS_((int, int, int *));
extern int set_int_elt _ANSI_ARGS_((int, int, int));
extern void destroy_obj _ANSI_ARGS_((int));
extern int create_obj _ANSI_ARGS_((int));
extern int exists_object _ANSI_ARGS_((int));
extern void initialize_db _ANSI_ARGS_((int));
extern int database_write _ANSI_ARGS_((char *));
extern int database_read _ANSI_ARGS_((char *));
extern INLINE int setFlag _ANSI_ARGS_((int, int, int, char *));
extern INLINE int unsetFlag _ANSI_ARGS_((int, int, int, char *));
extern INLINE int Flags _ANSI_ARGS_((int, int *, char *));
extern INLINE int Flag1 _ANSI_ARGS_((int, int, const char *));
extern INLINE int Flag2 _ANSI_ARGS_((int, int, const char *));
extern INLINE int Typeof _ANSI_ARGS_((int));
extern INLINE void animate _ANSI_ARGS_((int));
extern INLINE void deanimate _ANSI_ARGS_((int));
extern INLINE int getowner _ANSI_ARGS_((int));

/* from {g,bsd,n}dbm.c */
extern int dbmfile_open _ANSI_ARGS_((char *, int));
extern int dbmfile_init _ANSI_ARGS_((char *, int));
extern void dbmfile_close _ANSI_ARGS_((void));

/* from players.c */
extern int match_player _ANSI_ARGS_((char *));
extern void ptable_add _ANSI_ARGS_((int, char *));
extern void ptable_delete _ANSI_ARGS_((int));
extern char *ptable_lookup _ANSI_ARGS_((int));
extern void ptable_init _ANSI_ARGS_((void));

/* from attributes.c */
extern int attr_set _ANSI_ARGS_((int, char *, int));
extern int attr_addlock _ANSI_ARGS_((int, char *, struct boolexp *, int));
extern int attr_add _ANSI_ARGS_((int, char *, char *, int));
extern int attr_delete _ANSI_ARGS_((int, char *));
extern int attr_source _ANSI_ARGS_((int, char *));
extern int attr_getlock_parent _ANSI_ARGS_((int, char *, struct boolexp **,
                                            int *, int *));
extern int attr_get_parent _ANSI_ARGS_((int, char *, char **, int *, int *));
extern int attr_getlock _ANSI_ARGS_((int, char *, struct boolexp **, int *));
extern int attr_get _ANSI_ARGS_((int, char *, char **, int *));

/* from attrutils.c */
extern int attr_copy _ANSI_ARGS_((int, int, char *));
extern char *attr_match _ANSI_ARGS_((int, char *));
extern int attr_listen _ANSI_ARGS_((int, int, char, char *));
extern int attr_command _ANSI_ARGS_((int, int, char, char *));

/* from misc.c */
extern void stamp _ANSI_ARGS_((int, int));
extern char *ty_basename _ANSI_ARGS_((char *));
extern char to_lower _ANSI_ARGS_((char));
extern char to_upper _ANSI_ARGS_((char));
extern int nearby _ANSI_ARGS_((int, int));
extern int member _ANSI_ARGS_((int, int));
extern void list_drop _ANSI_ARGS_((int, int, int));
extern void list_add _ANSI_ARGS_((int, int, int));
extern void parse_flags _ANSI_ARGS_((char *, int *));
extern char *strcasestr _ANSI_ARGS_((char *, char *));
extern int stringprefix _ANSI_ARGS_((char *, char *));
extern void ty_malloc_init _ANSI_ARGS_((void));
extern VOID *ty_malloc _ANSI_ARGS_((int, char *));
extern VOID *ty_realloc _ANSI_ARGS_((VOID *, int, char *));
extern void ty_free _ANSI_ARGS_((VOID *));
#ifdef MALLOC_DEBUG
extern RETSIGTYPE report_malloc _ANSI_ARGS_((void));
#endif			/* MALLOC_DEBUG */
extern char *ty_strdup _ANSI_ARGS_((char *, char *));
extern void ty_strncpy _ANSI_ARGS_((char [], char *, int));
extern void copy_file _ANSI_ARGS_((char *, char *));

/* from getdate.c */
extern time_t get_date _ANSI_ARGS_((char *, time_t));

/* from boolexp.c */
extern int islocked _ANSI_ARGS_((int, int, int, char *));
extern int eval_boolexp _ANSI_ARGS_((int, int, struct boolexp *));
extern struct boolexp *parse_boolexp _ANSI_ARGS_((int, int, char *));
extern char *unparse_boolexp _ANSI_ARGS_((int, int, struct boolexp *));

/* from boolutils.c */
extern struct boolexp *boolexp_alloc _ANSI_ARGS_((void));
extern void boolexp_free _ANSI_ARGS_((struct boolexp *));
extern struct boolexp *copy_boolexp _ANSI_ARGS_((struct boolexp *));

/* from commands.c */
extern int command_alias _ANSI_ARGS_((char *, char *, char *));
extern int command_unalias _ANSI_ARGS_((char *));
extern void handle_cmd _ANSI_ARGS_((int, int, char *, int, char *[]));
extern void handle_cmds _ANSI_ARGS_((int, int, char *, int, char *[]));

/* from act.c */
extern void act_object _ANSI_ARGS_((int, int, int, char *, char *, char *,
				    int, char *, char *));
extern int can_hear _ANSI_ARGS_((int));
extern void hear_alert _ANSI_ARGS_((int, int, int));

/* from dbutils.c */
extern int controls _ANSI_ARGS_((int, int, int));
extern int econtrols _ANSI_ARGS_((int, int, int));
extern int inherit_wizard _ANSI_ARGS_((int));
extern int chownall _ANSI_ARGS_((int, int));
extern int no_quota _ANSI_ARGS_((int, int));
extern void handle_startup _ANSI_ARGS_((void));
extern void handle_autotoad _ANSI_ARGS_((int));

/* from display.c */
extern char *display_name _ANSI_ARGS_((int, int, int));
extern char *display_objflags _ANSI_ARGS_((int *, int));
extern char *display_attrflags _ANSI_ARGS_((int));
extern void display_flags _ANSI_ARGS_((int, int, int));
extern void display_attributes _ANSI_ARGS_((int, int, int, char *));
extern void display_attributes_parent _ANSI_ARGS_((int, int, int, char *));

/* from help.c */
extern void help_init _ANSI_ARGS_((void));
extern void check_news _ANSI_ARGS_((int));

/* from log.c */
#if defined(HAVE_STDARG_H) && defined(__STDC__)
extern void logfile _ANSI_ARGS_((int, char *, ...));
extern NORETURN panic _ANSI_ARGS_((char *, ...));
#else
extern void logfile(), panic();
#endif				/* HAVE_STDARG_H && __STDC__ */

/* from look.c */
extern void look_thing _ANSI_ARGS_((int, int, int, int));
extern void look_location _ANSI_ARGS_((int, int));
extern int can_see_anything _ANSI_ARGS_((int, int, int));
extern int can_see _ANSI_ARGS_((int, int, int));

/* from main.c */
extern void mud_shutdown _ANSI_ARGS_((int));
extern void dump_db _ANSI_ARGS_((void));

/* from match.c */
extern int match_contents _ANSI_ARGS_((int, int, char *, int, int));
extern void match_regexp _ANSI_ARGS_((int, int, char *, int, int));
extern int match_exits _ANSI_ARGS_((int, int, char *, int, int));
extern int match_first _ANSI_ARGS_((void));
extern int match_next _ANSI_ARGS_((void));
extern void match_free _ANSI_ARGS_((void));
extern int match_here _ANSI_ARGS_((int, int, int, char *, int));
extern int match_exit_command _ANSI_ARGS_((int, int, char *));
extern int resolve_object _ANSI_ARGS_((int, int, char *, int));
extern int resolve_player _ANSI_ARGS_((int, int, char *, int));
extern int resolve_exit _ANSI_ARGS_((int, int, char *, int));

/* from move.c */
extern void do_go_attempt _ANSI_ARGS_((int, int, int, int));
extern void move_object _ANSI_ARGS_((int, int, int, int));
extern void move_player_room _ANSI_ARGS_((int, int, int, int, int));
extern void move_player_leave _ANSI_ARGS_((int, int, int, int, char *));
extern void move_player_arrive _ANSI_ARGS_((int, int, int, int));
extern void send_home _ANSI_ARGS_((int, int, int));

/* from interface.c */
extern void interface_init _ANSI_ARGS_((void));
extern void timers _ANSI_ARGS_((void));
extern int count_logins _ANSI_ARGS_((char *));
extern int numconnected _ANSI_ARGS_((int));
extern int fd_conn _ANSI_ARGS_((int));
extern int match_active_player _ANSI_ARGS_((char *));

/* from set.c */
extern void do_set_string _ANSI_ARGS_((int, int, char *, char *,
					StringList *));
extern void set_attrflags _ANSI_ARGS_((int, int, int, char *, char *));
extern void set_attribute _ANSI_ARGS_((int, int, int, char *, char *));

/* from tcpip.c */
extern void tcp_loop _ANSI_ARGS_((void));
extern void tcp_shutdown _ANSI_ARGS_((void));
extern int tcp_notify _ANSI_ARGS_((int, char *));
extern int tcp_wall _ANSI_ARGS_((int *, char *, int));
extern int tcp_logoff _ANSI_ARGS_((int));
extern int fd_logoff _ANSI_ARGS_((int));

/* from notify.c */
extern void notify_bad _ANSI_ARGS_((int));
extern void notify_player _ANSI_ARGS_((int, int, int, char *, int));
extern void notify_audible _ANSI_ARGS_((int, int, int, char *));
extern void notify_list _ANSI_ARGS_((int, int, int, int, int));
extern void notify_except2 _ANSI_ARGS_((int, int, int, char *, int, int, int));
extern void notify_contents_except _ANSI_ARGS_((int, int, int, char *, int,
						int));
extern void notify_contents_loc _ANSI_ARGS_((int, int, int, char *, int, int));
extern void notify_exits _ANSI_ARGS_((int, int, int, int));

/* from parse.c */
extern char *parse_to _ANSI_ARGS_((char *, char, char));
extern char *exec _ANSI_ARGS_((int, int, int, char *, int, int, char *[]));
extern void parse_copy _ANSI_ARGS_((char *, char *, int));
extern void parse_sargv _ANSI_ARGS_((char *, int, int *, char *[]));
extern void free_argv _ANSI_ARGS_((int, char *[]));
extern int genderof _ANSI_ARGS_((int));
extern long expr_parse _ANSI_ARGS_((char *, int *));

/* from prims.c */
extern void prims_init _ANSI_ARGS_((void));

/* from vars.c */
extern char *var_get _ANSI_ARGS_((int, char *));
extern void var_delete _ANSI_ARGS_((int, char *));
extern void var_set _ANSI_ARGS_((int, char *, char *));
extern void slist_add _ANSI_ARGS_((char *, char *, int));
extern char *slist_next _ANSI_ARGS_((char *, char **));

/* from queue.c */
extern void queue_iadd _ANSI_ARGS_((int, int, char *, char *, char *, int,
				    char *[]));
extern void queue_add _ANSI_ARGS_((int, int, int, int, char *, int, char *[]));
extern void queue_run _ANSI_ARGS_((int));

/* from recycle.c */
extern int recycle_owned _ANSI_ARGS_((int, int, int));
extern int recycle_obj _ANSI_ARGS_((int, int, int));

/* from utils.c */
extern int create_player _ANSI_ARGS_((char *, char *, int));
extern int connect_player _ANSI_ARGS_((char *, char *));
extern void announce_connect _ANSI_ARGS_((int, char *, char *));
extern void announce_disconnect _ANSI_ARGS_((int));
extern int parse_name_pwd _ANSI_ARGS_((char *, char **, char **));
extern void parse_type _ANSI_ARGS_((char *, int *));
extern int ok_attr_name _ANSI_ARGS_((char *));
extern int ok_attr_value _ANSI_ARGS_((char *));
extern int ok_name _ANSI_ARGS_((char *));
extern int ok_player_name _ANSI_ARGS_((char *));
extern int ok_exit_name _ANSI_ARGS_((char *));
extern void reward_money _ANSI_ARGS_((int, int));
extern int can_afford _ANSI_ARGS_((int, int, int));
extern void check_paycheck _ANSI_ARGS_((int));
extern void check_last _ANSI_ARGS_((int));
extern void dump_file _ANSI_ARGS_((int, int, int, char *));
extern void dump_cmdoutput _ANSI_ARGS_((int, int, int, char *));
extern int legal_roomloc_check _ANSI_ARGS_((int, int, int *));
extern int legal_parent_check _ANSI_ARGS_((int, int, int *));
extern int legal_recursive_exit _ANSI_ARGS_((int, int, int *));
#ifdef UNIX_CRYPT
extern int comp_password _ANSI_ARGS_((char *, char *));
#endif			/* UNIX_CRYPT */

/* from wild.c */
extern int quick_wild_prefix _ANSI_ARGS_((char *, char *));
extern int quick_wild _ANSI_ARGS_((char *, char *));
extern int wild _ANSI_ARGS_((char *, char *, int, int *, char *[]));
extern int isregexp _ANSI_ARGS_((char *));
extern int filter_match _ANSI_ARGS_((char *, char *));

/* from group.c */
extern int group_ismem _ANSI_ARGS_((int));
extern int group_isleader _ANSI_ARGS_((int));
extern void group_remove _ANSI_ARGS_((int, int));
extern void group_create _ANSI_ARGS_((int, int, char *));
extern void group_join _ANSI_ARGS_((int, int, int));
extern void notify_group _ANSI_ARGS_((int, int, int, char *, int));
extern void group_follow _ANSI_ARGS_((int, int, int, int));

/* from compress.c */
extern int compressed _ANSI_ARGS_((char *));
extern char *compress _ANSI_ARGS_((char *));
extern char *uncompress _ANSI_ARGS_((char *));

#define __EXTERNS_H
#endif				/* __EXTERNS_H */
