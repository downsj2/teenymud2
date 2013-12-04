/* commands.h */

#include "config.h"

/*
 *		       This file is part of TeenyMUD II.
 *		    Copyright(C) 1994, 1995, 2013 by Jason Downs.
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

#ifndef __COMMANDS_H

#define MAX_ALIAS_LEN 32
#define MAX_ALIAS_SWITCHES 64

/* Command switches */
#define EXAMINE_ROOMS	0x0001
#define EXAMINE_PARENT	0x0002
#define EXAMINE_ATTRS	0x0004
#define EXAMINE_SORT	0x0008
#define RECYCLE_OVERRIDE	0x0001
#define LINK_NOCHOWN	0x0001
#define STATS_FULL	0x0001
#define KILL_SLAY	0x0001
#define POSE_NOSPACE	0x0001
#define EMIT_OTHERS	0x0001
#define EMIT_HTML	0x0002
#define PEMIT_HTML	0x0001
#define WALL_GOD	0x0001
#define WALL_WIZ	0x0002
#define WALL_POSE	0x0004
#define WALL_NOSPACE	0x0008
#define WALL_EMIT	0x0010
#define FIND_OWNED	0x0001
#define HALT_ALL	0x0001
#define PS_OWNED	0x0001
#define PS_CAUSE	0x0002
#define PS_ALL		0x0004
#define VERSION_FULL	0x0001
#define CONFIG_SET	0x0001
#define CONFIG_ALIAS	0x0002
#define CONFIG_UNALIAS	0x0004
#define CONFIG_EXPAND	0x0008
#define NOTIFY_ALL	0x0001
#define NOTIFY_DRAIN	0x0002
#define MOTD_SET	0x0001
#define BOOT_NOMESG	0x0001
#define CASE_FIRST	0x0001
#define GROUP_DISOLVE	0x0001
#define GROUP_CLOSE	0x0002
#define GROUP_OPEN	0x0004
#define GROUP_BOOT	0x0008
#define GROUP_SAY	0x0010
#define GROUP_EMOTE	0x0020
#define GROUP_DISPLAY	0x0040
#define GROUP_ALL	0x0080
#define GROUP_FOLLOW	0x0100
#define GROUP_JOIN	0x0200
#define GROUP_SILENCE	0x0400
#define GROUP_NOISY	0x0800
#define EDIT_ALL	0x0001
#define EDIT_NOSPACE	0x0002
#define LOCKOUT_REGISTER	0x0001
#define LOCKOUT_ALL	0x0002
#define LOCKOUT_ENABLE	0x0004
#define LOCKOUT_DUMP	0x0008
#define LOCKOUT_ALLOW	0x0010
#define LOCKOUT_CLEAR	0x0020
#define COPY_MOVE	0x0001
#define SAVESTTY_CLEAR	0x0001
#define SAVESTTY_RESTORE	0x0002
#define HELP_RELOAD	0x0001
#define HELP_NOHTML	0x0002
#define NEWS_RELOAD	0x0001
#define NEWS_NOHTML	0x0002

/* Special switches. */
#define ARG_FEXEC	0x1000
#define CMD_QUIET	0x2000

/* from conf.c */
extern VOID do_config _ANSI_ARGS_((int, int, int, char *, char *));

/* from buildcmds.c */
extern VOID do_clone _ANSI_ARGS_((int, int, int, char *, char *));
extern VOID do_create _ANSI_ARGS_((int, int, int, char *));
extern VOID do_dig _ANSI_ARGS_((int, int, int, char *, char *));
extern VOID do_palias _ANSI_ARGS_((int, int, int, char *, char *));
extern VOID do_name _ANSI_ARGS_((int, int, int, char *, char *));
extern VOID do_parent _ANSI_ARGS_((int, int, int, char *, char *));
extern VOID do_open _ANSI_ARGS_((int, int, int, char *, char *));
extern VOID do_link _ANSI_ARGS_((int, int, int, char *, char *));
extern VOID do_unlink _ANSI_ARGS_((int, int, int, char *));
extern VOID do_unlock _ANSI_ARGS_((int, int, int, char *));
extern VOID do_lock _ANSI_ARGS_((int, int, int, char *, char *));
extern VOID do_copy _ANSI_ARGS_((int, int, int, char *, char *));

/* from dbcmds.c */
extern VOID do_owned _ANSI_ARGS_((int, int, int, char *, char *));
extern VOID do_entrances _ANSI_ARGS_((int, int, int, char *));
extern VOID do_find _ANSI_ARGS_((int, int, int, char *, char *));
extern VOID do_trace _ANSI_ARGS_((int, int, int, char *, char *));
extern VOID do_stats _ANSI_ARGS_((int, int, int, char *));
extern VOID do_children _ANSI_ARGS_((int, int, int, char *));
extern VOID do_wipe _ANSI_ARGS_((int, int, int, int, char *[]));
extern VOID do_edit _ANSI_ARGS_((int, int, int, int, char *[]));
extern VOID do_sweep _ANSI_ARGS_((int, int, int, char *));

/* from help.c */
extern VOID do_help _ANSI_ARGS_((int, int, int, char *));
extern VOID do_news _ANSI_ARGS_((int, int, int, char *));

/* from look.c */
extern VOID do_look _ANSI_ARGS_((int, int, int, char *));
extern VOID do_examine _ANSI_ARGS_((int, int, int, char *, char *));
extern VOID do_inventory _ANSI_ARGS_((int, int, int));
extern VOID do_score _ANSI_ARGS_((int, int, int));
extern VOID do_use _ANSI_ARGS_((int, int, int, char *));

/* from move.c */
extern VOID do_home _ANSI_ARGS_((int, int, int));
extern VOID do_go _ANSI_ARGS_((int, int, int, char *));
extern VOID do_hand _ANSI_ARGS_((int, int, int, char *, char *));
extern VOID do_enter _ANSI_ARGS_((int, int, int, char *));
extern VOID do_leave _ANSI_ARGS_((int, int, int));
extern VOID do_attach _ANSI_ARGS_((int, int, int, char *, char *));
extern VOID do_take _ANSI_ARGS_((int, int, int, char *));
extern VOID do_drop _ANSI_ARGS_((int, int, int, char *));
extern VOID do_kill _ANSI_ARGS_((int, int, int, char *, char *));
extern VOID do_teleport _ANSI_ARGS_((int, int, int, char *, char *));

/* from interface.c */
extern VOID do_doing _ANSI_ARGS_((int, int, int, char *, char *));
extern VOID do_poll _ANSI_ARGS_((int, int, int, char *));
extern VOID do_savestty _ANSI_ARGS_((int, int, int, char *));

/* from speech.c */
extern VOID do_pose _ANSI_ARGS_((int, int, int, char *));
extern VOID do_emit _ANSI_ARGS_((int, int, int, char *));
extern VOID do_pemit _ANSI_ARGS_((int, int, int, char *, char *));
extern VOID do_page _ANSI_ARGS_((int, int, int, char *, char *));
extern VOID do_say _ANSI_ARGS_((int, int, int, char *));
extern VOID do_whisper _ANSI_ARGS_((int, int, int, char *, char *));
extern VOID do_wall _ANSI_ARGS_((int, int, int, char *));
extern VOID do_gripe _ANSI_ARGS_((int, int, int, char *));

/* from set.c */
extern VOID do_charge _ANSI_ARGS_((int, int, int, char *, char *));
extern VOID do_set _ANSI_ARGS_((int, int, int, char *, char *));
extern VOID do_password _ANSI_ARGS_((int, int, int, char *, char *));
extern VOID do_give _ANSI_ARGS_((int, int, int, char *, char *));
extern VOID do_cost _ANSI_ARGS_((int, int, int, char *, char *));

/* from queue.c */
extern VOID do_notify _ANSI_ARGS_((int, int, int, char *, char *));
extern VOID do_halt _ANSI_ARGS_((int, int, int, char *));
extern VOID do_semaphore _ANSI_ARGS_((int, int, int, char *, char *));
extern VOID do_wait _ANSI_ARGS_((int, int, int, char *, char *));
extern VOID do_ps _ANSI_ARGS_((int, int, int, char *));
extern VOID do_force _ANSI_ARGS_((int, int, int, char *, char *));
extern VOID do_trigger _ANSI_ARGS_((int, int, int, int, char *[]));
extern VOID do_foreach _ANSI_ARGS_((int, int, int, char *, char *));
extern VOID do_case _ANSI_ARGS_((int, int, int, int, char *[]));

/* from recycle.c */
extern VOID do_recycle _ANSI_ARGS_((int, int, int, char *));

/* from wiz.c */
extern VOID do_systat _ANSI_ARGS_((int, int, int));
extern VOID do_boot _ANSI_ARGS_((int, int, int, char *, char *));
extern VOID do_chown _ANSI_ARGS_((int, int, int, char *, char *));
extern VOID do_dump _ANSI_ARGS_((int, int, int));
extern VOID do_newpassword _ANSI_ARGS_((int, int, int, char *, char *));
extern VOID do_toad _ANSI_ARGS_((int, int, int, char *, char *));
extern VOID do_shutdown _ANSI_ARGS_((int, int, int));
extern VOID do_pcreate _ANSI_ARGS_((int, int, int, char *, char *));
extern VOID do_purge _ANSI_ARGS_((int, int, int, char *));
extern VOID do_chownall _ANSI_ARGS_((int, int, int, char *, char *));
extern VOID do_quota _ANSI_ARGS_((int, int, int, char *, char *));
extern VOID do_poor _ANSI_ARGS_((int, int, int, char *));
extern VOID do_motd _ANSI_ARGS_((int, int, int, char *, char *));

/* from group.c */
extern VOID do_group _ANSI_ARGS_((int, int, int, char *));

/* from lockout.c */
extern VOID do_lockout _ANSI_ARGS_((int, int, int, char *, char *));

/* from version.c */
extern VOID do_version _ANSI_ARGS_((int, int, int));

#define __COMMANDS_H
#endif			/* __COMMANDS_H */
