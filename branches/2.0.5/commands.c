/* commands.c */

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
#include <ctype.h>
#include <sys/types.h>
#ifdef HAVE_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif			/* HAVE_STRING_H */
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif			/* HAVE_STDLIB_H */

#include "conf.h"
#include "teeny.h"
#include "commands.h"
#include "externs.h"

#include "hash/hash.h"

/* all new main command parser. */

/* all command "switch" arguments are of this type. */
typedef struct cmd_switch {
  char *name;
  int matlen;
  int code;
  int flags;
} SWITCH;

/* all commands are of this type. */
typedef struct commands {
  char *name;
  VOID (*function) ();
  SWITCH *switches;
  int nargs;
  int perms;
} CMDS;

/* flag definitions - permissions. */
#define CMDS_NORMAL     0x00000000
#define CMDS_GOD        0x00000001
#define CMDS_WIZARD     0x00000002
#define CMDS_BUILDER    0x00000003
#define CMDS_MASK       0x00000007
#define CMDS_NGUEST     0x00000010
#define CMDS_PLAYER     0x00000020
#define CMDS_NOSED	0x00000040
#define CMDS_SPEECH	0x00000080		/* speech command */

/* switch types. */
#define SWITCH_MULTIPLE 0x00001000
#define SWITCH_SINGULAR 0x00002000

/* misc. flags. */
#define CMDS_NOEXEC	0x00100000
#define CMDS_VARSLASH	0x00200000
#define CMDS_LOCREQ	0x00400000

#define CMDS_VARARGS	-1

/* "switch" declarations. */

/*
 * First come the ``global'' switches-- ones that are parsed for in
 * every command.  The command itself may or may not actually support
 * them, of course.
 */

static SWITCH globalcmd_switches[] =
{
  {"quiet", 5, CMD_QUIET, CMDS_NORMAL | SWITCH_MULTIPLE},
  {(char *)NULL, 0, 0, 0}
};

static SWITCH ex_switches[] =
{
  {"rooms", 1, EXAMINE_ROOMS, CMDS_NORMAL | SWITCH_MULTIPLE},
  {"parents", 1, EXAMINE_PARENT, CMDS_NORMAL | SWITCH_MULTIPLE},
  {"attrs", 1, EXAMINE_ATTRS, CMDS_NORMAL | SWITCH_MULTIPLE},
  {"sort", 1, EXAMINE_SORT, CMDS_NORMAL | SWITCH_MULTIPLE},
  {(char *) NULL, 0, 0, 0}
};

static SWITCH recycle_switches[] =
{
  {"override", 4, RECYCLE_OVERRIDE, CMDS_WIZARD | SWITCH_SINGULAR},
  {(char *) NULL, 0, 0, 0}
};

static SWITCH link_switches[] =
{
  {"no_chown", 2, LINK_NOCHOWN, CMDS_WIZARD | SWITCH_SINGULAR},
  {(char *) NULL, 0, 0, 0}
};

static SWITCH stats_switches[] =
{
  {"full", 3, STATS_FULL, CMDS_NORMAL | SWITCH_SINGULAR},
  {(char *) NULL, 0, 0, 0}
};

static SWITCH find_switches[] =
{
  {"owned", 3, FIND_OWNED, CMDS_WIZARD | SWITCH_SINGULAR},
  {(char *) NULL, 0, 0, 0}
};

static SWITCH kill_switches[] =
{
  {"slay", 4, KILL_SLAY, CMDS_WIZARD | SWITCH_SINGULAR},
  {(char *) NULL, 0, 0, 0}
};

static SWITCH wall_switches[] =
{
  {"god", 1, WALL_GOD, CMDS_GOD | SWITCH_SINGULAR},
  {"wiz", 1, WALL_WIZ, CMDS_WIZARD | SWITCH_SINGULAR},
  {"pose", 1, WALL_POSE, CMDS_WIZARD | SWITCH_MULTIPLE},
  {"no_space", 1, WALL_NOSPACE, CMDS_WIZARD | SWITCH_MULTIPLE},
  {"emit", 1, WALL_EMIT, CMDS_WIZARD | SWITCH_MULTIPLE},
  {(char *) NULL, 0, 0, 0}
};

static SWITCH halt_switches[] =
{
  {"all", 3, HALT_ALL, CMDS_WIZARD | SWITCH_MULTIPLE},
/*{"quiet", 1, HALT_QUIET, CMDS_WIZARD | SWITCH_MULTIPLE},*/
  {(char *)NULL, 0, 0, 0}
};

static SWITCH ps_switches[] =
{
  {"all", 3, PS_ALL, CMDS_WIZARD | SWITCH_SINGULAR},
  {"cause", 1, PS_CAUSE, CMDS_NORMAL | SWITCH_MULTIPLE},
  {"owned", 3, PS_OWNED, CMDS_NORMAL | SWITCH_MULTIPLE},
  {(char *)NULL, 0, 0, 0}
};

static SWITCH pose_switches[] =
{
  {"no_space", 1, POSE_NOSPACE, CMDS_NORMAL | SWITCH_SINGULAR},
  {(char *) NULL, 0, 0, 0}
};

static SWITCH version_switches[] = 
{
  {"full", 1, VERSION_FULL, CMDS_NORMAL | SWITCH_MULTIPLE},
  {(char *) NULL, 0, 0, 0}
};

static SWITCH config_switches[] =
{
  {"set", 1, CONFIG_SET, CMDS_GOD | SWITCH_SINGULAR},
  {"alias", 1, CONFIG_ALIAS, CMDS_GOD | SWITCH_SINGULAR},
  {"unalias", 1, CONFIG_UNALIAS, CMDS_GOD | SWITCH_SINGULAR},
  {"expand", 1, CONFIG_EXPAND, CMDS_GOD | SWITCH_SINGULAR},
  {(char *)NULL, 0, 0, 0}
};

static SWITCH notify_switches[] = 
{
  {"all", 1, NOTIFY_ALL, CMDS_NORMAL | SWITCH_MULTIPLE},
  {"drain", 1, NOTIFY_DRAIN, CMDS_NORMAL | SWITCH_MULTIPLE},
  {(char *)NULL, 0, 0, 0}
};

static SWITCH motd_switches[] =
{
  {"set", 1, MOTD_SET, CMDS_WIZARD | SWITCH_SINGULAR},
  {(char *)NULL, 0, 0, 0}
};

static SWITCH boot_switches[] =
{
  {"nomesg", 2, BOOT_NOMESG, CMDS_WIZARD | SWITCH_SINGULAR},
  {(char *)NULL, 0, 0, 0}
};

static SWITCH case_switches[] =
{
  {"first", 2, CASE_FIRST, CMDS_NORMAL | SWITCH_SINGULAR},
  {(char *)NULL, 0, 0, 0}
};

static SWITCH group_switches[] =
{
  {"delete", 6, GROUP_DISOLVE, CMDS_NGUEST | SWITCH_SINGULAR},
  {"open", 4, GROUP_OPEN, CMDS_NGUEST | SWITCH_SINGULAR},
  {"close", 5, GROUP_CLOSE, CMDS_NGUEST | SWITCH_SINGULAR},
  {"boot", 4, GROUP_BOOT, CMDS_NGUEST | SWITCH_SINGULAR},
  {"say", 1, GROUP_SAY, CMDS_NORMAL | SWITCH_SINGULAR},
  {"emote", 1, GROUP_EMOTE, CMDS_NORMAL | SWITCH_SINGULAR},
  {"display", 3, GROUP_DISPLAY, CMDS_NORMAL | SWITCH_SINGULAR},
  {"all", 3, GROUP_ALL, CMDS_NORMAL | SWITCH_MULTIPLE},
  {"follow", 3, GROUP_FOLLOW, CMDS_NORMAL | SWITCH_SINGULAR},
  {"join", 1, GROUP_JOIN, CMDS_NORMAL | SWITCH_SINGULAR},
  {"silence", 3, GROUP_SILENCE, CMDS_NORMAL | SWITCH_SINGULAR},
  {"noisy", 3, GROUP_NOISY, CMDS_NORMAL | SWITCH_SINGULAR},
  {(char *)NULL, 0, 0, 0}
};

static SWITCH emit_switches[] =
{
  {"others", 1, EMIT_OTHERS, CMDS_NORMAL | SWITCH_SINGULAR},
  {"html", 4, EMIT_HTML, CMDS_NORMAL | SWITCH_MULTIPLE},
  {(char *)NULL, 0, 0, 0}
};

static SWITCH pemit_switches[] =
{
  {"html", 4, PEMIT_HTML, CMDS_NORMAL | SWITCH_MULTIPLE},
  {(char *)NULL, 0, 0, 0}
};

static SWITCH edit_switches[] =
{
  {"all", 1, EDIT_ALL, CMDS_NORMAL | SWITCH_MULTIPLE},
  {"no_space", 2, EDIT_NOSPACE, CMDS_NORMAL | SWITCH_MULTIPLE},
  {(char *)NULL, 0, 0, 0}
};

static SWITCH lockout_switches[] =
{
  {"all", 3, LOCKOUT_ALL, CMDS_WIZARD | SWITCH_SINGULAR},
  {"allow", 5, LOCKOUT_ALLOW, CMDS_WIZARD | SWITCH_SINGULAR},
  {"clear", 5, LOCKOUT_CLEAR, CMDS_WIZARD | SWITCH_SINGULAR},
  {"enable", 2, LOCKOUT_ENABLE, CMDS_WIZARD | SWITCH_SINGULAR},
  {"dump", 3, LOCKOUT_DUMP, CMDS_WIZARD | SWITCH_SINGULAR},
  {"register", 2, LOCKOUT_REGISTER, CMDS_WIZARD | SWITCH_SINGULAR},
  {(char *)NULL, 0, 0, 0}
};

static SWITCH copy_switches[] =
{
  {"move", 1, COPY_MOVE, CMDS_NORMAL | SWITCH_SINGULAR},
  {(char *)NULL, 0, 0, 0}
};

static SWITCH savestty_switches[] =
{
  {"clear", 1, SAVESTTY_CLEAR, CMDS_NORMAL | SWITCH_SINGULAR},
  {"restore", 1, SAVESTTY_RESTORE, CMDS_NORMAL | SWITCH_SINGULAR},
  {(char *)NULL, 0, 0, 0}
};

static SWITCH help_switches[] =
{
  {"reload", 1, HELP_RELOAD, CMDS_WIZARD | SWITCH_SINGULAR},
  {"nohtml", 3, HELP_NOHTML, CMDS_NORMAL | SWITCH_MULTIPLE},
  {(char *)NULL, 0, 0, 0}
};

static SWITCH news_switches[] =
{
  {"reload", 1, NEWS_RELOAD, CMDS_WIZARD | SWITCH_SINGULAR},
  {"nohtml", 3, NEWS_NOHTML, CMDS_NORMAL | SWITCH_MULTIPLE},
  {(char *)NULL, 0, 0, 0}
};

/* static commands table. */
static CMDS cmddefs[] =
{
 /* drop */
  {"drop", do_drop, NULL, 1, CMDS_NORMAL|CMDS_NOSED},
 /* enter */
  {"enter", do_enter, NULL, 1, CMDS_NORMAL|CMDS_NOSED},
 /* examine */
  {"examine", do_examine, ex_switches, 2, CMDS_NORMAL},
 /* give */
  {"give", do_give, NULL, 2, CMDS_NGUEST|CMDS_NOSED},
 /* go */
  {"go", do_go, NULL, 1, CMDS_NORMAL|CMDS_NOSED},
 /* gripe */
  {"gripe", do_gripe, NULL, 1, CMDS_NORMAL},
 /* hand */
  {"hand", do_hand, NULL, 2, CMDS_NORMAL},
 /* help */
  {"help", do_help, help_switches, 1, CMDS_NORMAL},
 /* home */
  {"home", do_home, NULL, 0, CMDS_NORMAL|CMDS_NOSED},
 /* inventory */
  {"inventory", do_inventory, NULL, 0, CMDS_NORMAL},
 /* kill */
  {"kill", do_kill, kill_switches, 2, CMDS_NGUEST},
 /* leave */
  {"leave", do_leave, NULL, 0, CMDS_NORMAL|CMDS_NOSED},
 /* look */
  {"look", do_look, NULL, 1, CMDS_NORMAL},
 /* news */
  {"news", do_news, news_switches, 1, CMDS_NORMAL},
 /* page */
  {"page", do_page, NULL, 2, CMDS_NORMAL|CMDS_SPEECH},
 /* pose */
  {"pose", do_pose, pose_switches, 1, CMDS_NORMAL|CMDS_SPEECH},
 /* say */
  {"say", do_say, NULL, 1, CMDS_NORMAL|CMDS_SPEECH},
 /* score */
  {"score", do_score, NULL, 0, CMDS_NORMAL},
 /* take */
  {"take", do_take, NULL, 1, CMDS_NORMAL|CMDS_NOSED},
 /* use */
  {"use", do_use, NULL, 1, CMDS_NORMAL},
 /* whisper */
  {"whisper", do_whisper, NULL, 2, CMDS_NORMAL|CMDS_SPEECH},
 /* @attach */
  {"@attach", do_attach, NULL, 2, CMDS_NGUEST},
 /* @boot */
  {"@boot", do_boot, boot_switches, 2, CMDS_WIZARD|CMDS_NGUEST},
 /* @charge */
  {"@charge", do_charge, NULL, 2, CMDS_NGUEST},
 /* @case */
  {"@case", do_case, case_switches, CMDS_VARARGS, CMDS_NORMAL},
 /* @children */
  {"@children", do_children, NULL, 2, CMDS_NGUEST},
 /* @chownall */
  {"@chownall", do_chownall, NULL, 2, CMDS_WIZARD|CMDS_NGUEST},
 /* @chown */
  {"@chown", do_chown, NULL, 2, CMDS_NGUEST},
 /* @clone */
  {"@clone", do_clone, NULL, 2, CMDS_BUILDER|CMDS_NGUEST|CMDS_NOSED},
 /* @config */
  {"@config", do_config, config_switches, 2, CMDS_NORMAL},
 /* @copy */
  {"@copy", do_copy, copy_switches, 2, CMDS_NGUEST},
 /* @cost */
  {"@cost", do_cost, NULL, 2, CMDS_NGUEST},
 /* @create */
  {"@create", do_create, NULL, 1, CMDS_BUILDER|CMDS_NGUEST|CMDS_NOSED},
 /* @dig */
  {"@dig", do_dig, NULL, 2, CMDS_BUILDER|CMDS_NGUEST|CMDS_NOSED},
 /* @doing */
  {"@doing", do_doing, NULL, 2, CMDS_PLAYER|CMDS_SPEECH},
 /* @dump */
  {"@dump", do_dump, NULL, 0, CMDS_WIZARD},
 /* @edit */
  {"@edit", do_edit, edit_switches, CMDS_VARARGS,
   CMDS_NGUEST|CMDS_VARSLASH|CMDS_NOEXEC},
 /* @emit */
  {"@emit", do_emit, emit_switches, 1, CMDS_NORMAL|CMDS_SPEECH},
 /* @entrances */
  {"@entrances", do_entrances, NULL, 1, CMDS_NGUEST},
 /* @foreach */
  {"@foreach", do_foreach, NULL, 2, CMDS_NGUEST|CMDS_NOEXEC},
 /* @force */
  {"@force", do_force, NULL, 2, CMDS_NGUEST},
 /* @find */
  {"@find", do_find, find_switches, 2, CMDS_NGUEST},
 /* @group */
  {"@group", do_group, group_switches, 1, CMDS_NORMAL},
 /* @halt */
  {"@halt", do_halt, halt_switches, 1, CMDS_NGUEST},
 /* @lock */
  {"@lock", do_lock, NULL, 2, CMDS_NGUEST},
 /* @lockout */
  {"@lockout", do_lockout, lockout_switches, 2, CMDS_WIZARD},
 /* @link */
  {"@link", do_link, link_switches, 2, CMDS_NGUEST},
 /* @motd */
  {"@motd", do_motd, motd_switches, 2, CMDS_NORMAL},
 /* @name */
  {"@name", do_name, NULL, 2, CMDS_NGUEST|CMDS_NOEXEC},
 /* @newpassword */
  {"@newpassword", do_newpassword, NULL, 2,
		CMDS_WIZARD|CMDS_NGUEST|CMDS_NOEXEC},
 /* @notify */
  {"@notify", do_notify, notify_switches, 2, CMDS_NGUEST},
 /* @open */
  {"@open", do_open, NULL, 2, CMDS_BUILDER|CMDS_NGUEST|CMDS_NOSED},
 /* @owned */
  {"@owned", do_owned, NULL, 2, CMDS_WIZARD|CMDS_NGUEST},
 /* @palias */
  {"@palias", do_palias, NULL, 2, CMDS_NGUEST},
 /* @parent */
  {"@parent", do_parent, NULL, 2, CMDS_BUILDER|CMDS_NGUEST},
 /* @password */
  {"@password", do_password, NULL, 2, CMDS_NGUEST|CMDS_NOEXEC},
 /* @pemit */
  {"@pemit", do_pemit, pemit_switches, 2, CMDS_NORMAL|CMDS_SPEECH},
 /* @pcreate */
  {"@pcreate", do_pcreate, NULL, 2, CMDS_GOD|CMDS_NGUEST|CMDS_NOSED},
 /* @poll */
  {"@poll", do_poll, NULL, 1, CMDS_WIZARD|CMDS_NGUEST},
 /* @poor */
  {"@poor", do_poor, NULL, 1, CMDS_GOD|CMDS_NGUEST},
 /* @ps */
  {"@ps", do_ps, ps_switches, 1, CMDS_NGUEST},
 /* @purge */
  {"@purge", do_purge, NULL, 1, CMDS_WIZARD | CMDS_NGUEST},
 /* @quota */
  {"@quota", do_quota, NULL, 2, CMDS_NGUEST},
 /* @recycle */
  {"@recycle", do_recycle, recycle_switches, 1, CMDS_NGUEST},
 /* @savestty */
  {"@savestty", do_savestty, savestty_switches, 1, CMDS_NGUEST},
 /* @semaphore */
  {"@semaphore", do_semaphore, NULL, 2, CMDS_NGUEST|CMDS_NOEXEC},
 /* @set */
  {"@set", do_set, NULL, 2, CMDS_NGUEST|CMDS_NOEXEC},
 /* @shutdown */
  {"@shutdown", do_shutdown, NULL, 0, CMDS_WIZARD|CMDS_NGUEST},
 /* @stats */
  {"@stats", do_stats, stats_switches, 1, CMDS_NGUEST},
 /* @sweep */
  {"@sweep", do_sweep, NULL, 1, CMDS_NORMAL},
 /* @systat */
  {"@systat", do_systat, NULL, 0, CMDS_WIZARD|CMDS_NGUEST},
 /* @teleport */
  {"@teleport", do_teleport, NULL, 2, CMDS_NGUEST},
 /* @toad */
  {"@toad", do_toad, NULL, 2, CMDS_WIZARD|CMDS_NGUEST},
 /* @trace */
  {"@trace", do_trace, NULL, 2, CMDS_NGUEST},
 /* @trigger */
  {"@trigger", do_trigger, NULL, CMDS_VARARGS, CMDS_NGUEST|CMDS_VARSLASH},
 /* @unlink */
  {"@unlink", do_unlink, NULL, 1, CMDS_NGUEST},
 /* @unlock */
  {"@unlock", do_unlock, NULL, 1, CMDS_NGUEST},
 /* @version */
  {"@version", do_version, version_switches, 0, CMDS_NORMAL},
 /* @wait */
  {"@wait", do_wait, NULL, 2, CMDS_NGUEST|CMDS_NOEXEC},
 /* @wall */
  {"@wall", do_wall, wall_switches, 2, CMDS_WIZARD|CMDS_NGUEST},
 /* @wipe */
  {"@wipe", do_wipe, NULL, CMDS_VARARGS, CMDS_NGUEST|CMDS_VARSLASH|CMDS_NOEXEC},
  {NULL, NULL, NULL, 0, 0}
};

static int CmdHash_Inited = 0;
static Hash_Table CmdHash;

static int AliasSw_Inited = 0;
static Hash_Table AliasSw;

static int fexec = 0;

static void cmdhash_init _ANSI_ARGS_((void));
static int cmdaccess _ANSI_ARGS_((int, int));
static void notify_huh _ANSI_ARGS_((int, int));
static void notify_halt _ANSI_ARGS_((int, int));
static void parse_argtwo _ANSI_ARGS_((int, int, char *, char **, char **, int,
				      int, char *[]));
static void parse_argvee _ANSI_ARGS_((int, int, char *, int *, char *[], int,
				      int, char *[]));
static int handle_exit_cmd _ANSI_ARGS_((int, int, char *));
static int parse_switches _ANSI_ARGS_((int, int, CMDS *, char *));

int command_alias(cmd, alias, switches)
    char *cmd, *alias, *switches;
{
  Hash_Entry *find, *entry;
  int new;

  if((cmd == (char *)NULL) || (*cmd == '\0')
     || (alias == (char *)NULL) || (*alias == '\0'))
    return(-1);

  if(!CmdHash_Inited)
    cmdhash_init();

  if((find = Hash_FindEntry(&CmdHash, cmd)) == NULL)
    return(-1);
  if((entry = Hash_FindEntry(&CmdHash, alias)) == NULL) {
    entry = Hash_CreateEntry(&CmdHash, alias, (int *)NULL);
    Hash_SetValue(entry, Hash_GetValue(find));

    if(switches != (char *)NULL) {
      if(!AliasSw_Inited) {
        Hash_InitTable(&AliasSw, 20, HASH_STRCASE_KEYS);
	AliasSw_Inited++;
      }

      entry = Hash_CreateEntry(&AliasSw, alias, &new);
      if(!new)
        ty_free((VOID *)Hash_GetValue(entry));
      Hash_SetValue(entry, (int *)ty_strdup(switches, "command_alias"));
    }
  } else
    return(-1);
  return(0);
}

int command_unalias(cmd)
    char *cmd;
{
  Hash_Entry *entry;

  if((cmd == (char *)NULL) || (*cmd == '\0'))
    return(-1);

  if(!CmdHash_Inited)
    cmdhash_init();

  if((entry = Hash_FindEntry(&CmdHash, cmd)) == NULL)
    return(-1);
  Hash_DeleteEntry(&CmdHash, entry);

  if(AliasSw_Inited) {
    entry = Hash_FindEntry(&AliasSw, cmd);
    if(entry != NULL) {
      ty_free((VOID *)Hash_GetValue(entry));
      Hash_DeleteEntry(&AliasSw, entry);
    }
  }
  return(0);
}

int command_aliasexp(cmd, buf, bufsiz)
    char *cmd, *buf;
    int bufsiz;
{
  register Hash_Entry *centry, *sentry;
  register char *bptr, *iptr;

  if((cmd == (char *)NULL) || (cmd[0] == '\0') || !CmdHash_Inited)
    return(-1);

  if((centry = Hash_FindEntry(&CmdHash, cmd)) == (Hash_Entry *)NULL)
    return(-1);
  if(AliasSw_Inited) {
    sentry = Hash_FindEntry(&AliasSw, cmd);
  } else
    sentry = (Hash_Entry *)NULL;

  /* Copy it in. */
  bptr = buf;
  iptr = (char *)Hash_GetValue(centry);
  while(((bptr - buf) < (bufsiz - 1)) && *iptr)
    *bptr++ = *iptr++;

  if((bptr - buf) < (bufsiz - 1))
    *bptr++ = '/';

  if(sentry != (Hash_Entry *)NULL) {
    iptr = (char *)Hash_GetValue(sentry);
    while(((bptr - buf) < (bufsiz - 1)) && *iptr)
      *bptr++ = *iptr++;
  }
  *bptr = '\0';

  return(0);
}

static void cmdhash_init()
{
  register CMDS *cptr;
  register Hash_Entry *entry;

  Hash_InitTable(&CmdHash, sizeof(cmddefs) / sizeof(CMDS), HASH_STRCASE_KEYS);
  for(cptr = cmddefs; cptr->name != NULL; cptr++){
    entry = Hash_CreateEntry(&CmdHash, cptr->name, NULL);
    Hash_SetValue(entry, (int *)cptr);
  }
  CmdHash_Inited = 1;
}

static int cmdaccess(player, perms)
    int player, perms;
{
  if ((perms & CMDS_NGUEST) && isGUEST(player))
    return (0);
  if ((perms & CMDS_PLAYER) && !isPLAYER(player))
    return (0);
  if ((perms & CMDS_NOSED) && (isROOM(player) || isEXIT(player)))
    return (0);

  switch (perms & CMDS_MASK) {
  case CMDS_NORMAL:
    return (1);
  case CMDS_GOD:
    return (isGOD(player));
  case CMDS_WIZARD:
    return (isWIZARD(player));
  case CMDS_BUILDER:
    return (isBUILDER(player) || isWIZARD(player));
  }
  return (0);
}

static void notify_huh(player, cause)
    int player, cause;
{
  notify_player(player, cause, player, "Huh?  (Type \"help\" for help.)", 0);
}

static void notify_halt(player, cause)
    int player, cause;
{
  char buff[BUFFSIZ];
  int owner;

  if(get_int_elt(player, OWNER, &owner) != -1) {
    snprintf(buff, sizeof(buff),
	     "Attempt to execute command by halted object #%d.", player);
    notify_player(owner, cause, player, buff, 0);
  }
}

#define noexec(_f, _p)	((_f & CMDS_NOEXEC) \
			 || ((_f & CMDS_SPEECH) && isELOQUENT(_p)))

static void parse_argtwo(player, cause, str, argone, argtwo, flags,
			 eargc, eargv)
    int player, cause;
    char *str, **argone, **argtwo;
    int flags, eargc;
    char *eargv[];
{
  register char *ptr1, *ptr2;
  register int len;

  if(!*str) {
    *argone = (char *)NULL;
    *argtwo = (char *)NULL;
    return;
  }

  /* find argtwo, if there is one. */
  ptr1 = parse_to(str, '\0', '=');

  /* eat trailing whitespace of argone, if there is one. */
  if(*str) {
    for(ptr2 = &str[strlen(str)]; (ptr2 > str) &&
  	((*ptr2 == '\0') || isspace(*ptr2)); ptr2--);
    ptr2[1] = '\0';
  }

  /* set up return values. */
  if(!fexec && noexec(flags, player)) {
    /* there isn't always an argone. */
    if(*str) {
      len = strlen(str)+1;
      *argone = (char *)ty_malloc(len, "parse_argtwo.argone");
      parse_copy(*argone, str, len);
    } else
      *argone = (char *)NULL;

    if(ptr1) {	/* there is an argtwo. */
      /* eat leading whitespace of argtwo. */
      while(*ptr1 && isspace(*ptr1))
        ptr1++;
      /* if we ran out, we loose. */
      if(ptr1) {
        len = strlen(ptr1)+1;
        *argtwo = (char *)ty_malloc(len, "parse_argtwo.argtwo");
        parse_copy(*argtwo, ptr1, len);
      } else
        *argtwo = NULL;
    } else
      *argtwo = NULL;
  } else {	/* exec stuff. */
    /* there isn't always an argone. */
    if(*str) {
      *argone = exec(player, cause, player, str, 0, eargc, eargv);
    } else {
      *argone = (char *)NULL;
    }

    /* eat leading whitespace of argtwo. */
    while(ptr1 && *ptr1 && isspace(*ptr1))
      ptr1++;

    if(ptr1) {	/* there is an argtwo. */
      *argtwo = exec(player, cause, player, ptr1, 0, eargc, eargv);
    } else {
      *argtwo = NULL;
    }
  }
}

static void parse_argvee(player, cause, str, argc, argv, flags,
			 eargc, eargv)
    int player, cause;
    register char *str;
    int *argc;
    char *argv[];
    int flags, eargc;
    char *eargv[];
{
  register char *ptr1, *ptr2;
  register int count = 0, len;
  int fixhack = 0;

  *argc = 0;
  if(*str) {
    /* first argument: up to first '=', or '/'. */
    if((flags & CMDS_VARSLASH) && (ptr1 = parse_to(str, '\0', '/'))) {
      if(*str) {
        for(ptr2 = &str[strlen(str)]; (ptr2 > str) &&
      	  ((*ptr2 == '\0') || isspace(*ptr2)); ptr2--);
        ptr2[1] = '\0';
      }

      if(!fexec && noexec(flags, player)) {
        len = strlen(str)+1;
	argv[count] = (char *)ty_malloc(len, "parse_argvee.argv");
	parse_copy(argv[count++], str, len);
      } else
	argv[count++] = exec(player, cause, player, str, 0, eargc, eargv);
      str = ptr1;
    } else if((flags & CMDS_VARSLASH) && (ptr1 == NULL))
      fixhack++;

    ptr1 = parse_to(str, '\0', '=');
    if(*str) {
      for(ptr2 = &str[strlen(str)]; (ptr2 > str) &&
    	  ((*ptr2 == '\0') || isspace(*ptr2)); ptr2--);
      ptr2[1] = '\0';
    }

    if(!fexec && noexec(flags, player)) {
      len = strlen(str)+1;
      argv[count] = (char *)ty_malloc(len, "parse_argvee.argv");
      parse_copy(argv[count++], str, len);
    } else {
      argv[count++] = exec(player, cause, player, str, 0, eargc, eargv);
    }

    /* eat any leading whitespace. */
    while(ptr1 && *ptr1 && isspace(*ptr1))
      ptr1++;

    if(ptr1 && *ptr1) {
      /* skip one, if need be. */
      count += fixhack;

      /* now parse up to ','. */
      while(count < MAXARGS){
        for(str = ptr1; *str && isspace(*str); str++);
        if(!*str)
          break;	/* we're done. */

        ptr1 = parse_to(str, '\0', ',');
	if(*str) {
          for(ptr2 = &str[strlen(str)]; (ptr2 > str) &&
      	      ((*ptr2 == '\0') || isspace(*ptr2)); ptr2--);
          ptr2[1] = '\0';   
	}

        if(!fexec && noexec(flags, player)) {
          len = strlen(str)+1;
          argv[count] = (char *)ty_malloc(len, "parse_argvee.argv");
	  parse_copy(argv[count++], str, len);
        } else {
          argv[count++] = exec(player, cause, player, str, 0, eargc, eargv);
        }

        if(!ptr1 || !*ptr1)
          break;	/* we're done. */
      }
    }
  }
  *argc = count;
}

static int handle_exit_cmd(player, cause, str)
    int player, cause;
    char *str;
{
  int here, theex;

  if((get_int_elt(player, LOC, &here) == -1) || !exists_object(here)) {
    logfile(LOG_ERROR, "handle_exit_cmd: bad location of player #%d\n",
	    player);
    return(0);
  }
  theex = match_exit_command(player, cause, str);
  if(theex != -1) {
    do_go_attempt(player, cause, here, theex);
    return(1);
  }
  return(0);
}

/*
 * Switch parser, broken out for versatility's sake.
 *
 * This way, everything sees at least global switches.
 */
static int parse_switches(player, cause, cptr, swptr)
    int player, cause;
    CMDS *cptr;
    char *swptr;
{
  register int overtotal = 0;	/* overall total. */
  register int intotal = 0;	/* inner total. */
  register int doglob = 0;	/* doing globals. */
  register SWITCH *sptr;
  int switches;

  switches = 0;
  if (swptr != NULL) {
    if((cptr != NULL) && (cptr->switches != NULL))
      sptr = cptr->switches;
    else {
      doglob++;
      sptr = globalcmd_switches;
    }

    while (*swptr) {
      while(*swptr && (sptr->name != NULL)) {
	if (!strncasecmp(sptr->name, swptr, sptr->matlen)) {
	  if (!cmdaccess(player, sptr->flags)) {
	    notify_player(player, cause, player, "Permission denied.",
			  NOT_QUIET);
	    return(-1);
	  }
	  if (overtotal && (sptr->flags & SWITCH_SINGULAR)) {
	    notify_player(player, cause, player,
			  "Illegal combination of command switches.",
			  NOT_QUIET);
	    return(-1);
	  } else {
	    switches |= sptr->code;
	    if (sptr->flags & SWITCH_SINGULAR) {
	      overtotal++;
	    }
	  }
	  intotal++;
	}
	sptr++;
      }

      /*
       * If we've not checked globals yet, do it now.
       */
      if (!doglob) {
	doglob++;

	sptr = globalcmd_switches;
	continue;  /* Go back to the top of the while. */
      } else
	doglob = 0;

      if (!intotal) {
	notify_player(player, cause, player, "Illegal command switch.",
		      NOT_QUIET);
	return(-1);
      }
      for (; *swptr && (*swptr != '/'); swptr++);
      if (*swptr)
	swptr++;
    }
    if (!switches) {
      notify_player(player, cause, player, "Illegal switches.", NOT_QUIET);
      return(-1);
    }
  }
  return(switches);
}

/* main command parser. */
void handle_cmd(player, cause, origcmd, eargc, eargv)
    int player, cause;
    char *origcmd;
    int eargc;
    char *eargv[];
{
  char *cmd, *argv[MAXARGS], *swptr, *argone, *argtwo;
  register char *ptr = (char *)NULL;
  register int i;
  register Hash_Entry *entry;
  CMDS *cptr;
  int argc = 0, switches = 0;
  char *name;

  if (mudconf.logcommands) {
    if(get_str_elt(player, NAME, &name) == -1)
      name = "???";	/* XXX */
    logfile(LOG_COMMAND, "COMMAND: %s(#%d): %s\n", name, player, origcmd);
  }

  fexec = 0;		/* Reset forced mode. */

  /* Kill any leading and trailing whitespace. */
  if((origcmd != (char *)NULL) && (origcmd[0] != '\0')) {
    while(*origcmd && isspace(*origcmd))
      origcmd++;
    if(*origcmd) {
      ptr = &origcmd[strlen(origcmd)-1];
      while(*ptr && isspace(*ptr) && (ptr >= origcmd))
	ptr--;
      ptr[1] = '\0';
      /* Leave ptr set for the exec check! */
    }

    /* Check if we want forced execution.  Use ptr from above. */
    if((origcmd[0] == '<') && (ptr != (char *)NULL) && (*ptr == '>')) {
      fexec++;

      origcmd++;	/* Skip past the leading token... */
      *ptr = '\0';	/* ...And eat the trailing one. */
    }
  }

  /* Check if we want forced execution.  Use ptr from above. */

  /* Perform sanity checks. */
  if((origcmd == (char *)NULL) || (origcmd[0] == '\0'))
    return;
  if(!exists_object(player) || !exists_object(cause)) {
    logfile(LOG_ERROR,
	    "handle_cmd: called with bad player (#%d) or cause (#%d)\n",
    	    player, cause);
    return;
  }

  /* Are they halted? */
  if(!isPLAYER(player) && isHALT(player)) {
    notify_halt(player, cause);
    return;
  }

  /* init the hash table, if need be. */
  if(!CmdHash_Inited)
    cmdhash_init();

  /* check these first. */
  if(origcmd[0] == '\"') {
    if(origcmd[1] && !fexec && !noexec(CMDS_SPEECH, player)) {
      ptr = exec(player, cause, player, &origcmd[1], 0, eargc, eargv);
      do_say(player, cause, switches, ptr);
      ty_free((VOID *)ptr);
    } else
      do_say(player, cause, switches, &origcmd[1]);
    return;
  } else if(origcmd[0] == ':') {
    if(origcmd[1] && !fexec && !noexec(CMDS_SPEECH, player)) {
      ptr = exec(player, cause, player, &origcmd[1], 0, eargc, eargv);
      do_pose(player, cause, switches, ptr);
      ty_free((VOID *)ptr);
    } else
      do_pose(player, cause, switches, &origcmd[1]);
    return;
  } else if((origcmd[0] == '@') && (origcmd[1] == '@')) {
    return;
  }

  /* lock the cache. */
  cache_lock();

  /* is this an exit? */
  if((origcmd[0] != '@') && (origcmd[0] != '&')) {
    if(handle_exit_cmd(player, cause, origcmd)) {
      if(mudstat.status != MUD_DUMPING)
        cache_unlock();
      return;
    }
  }

  /* save ourselves. */
  cmd = (char *)alloca(strlen(origcmd)+1);
  if(cmd == (char *)NULL)
    panic("handle_cmd(): stack allocation failed.\n");
  strcpy(cmd, origcmd);

  /* init argument array, for later. */
  for(i = 0; i < MAXARGS; i++)
    argv[i] = NULL;

  /* argument 0 is the first word, up to whitespace *or* slash. */
  for(ptr = cmd; *ptr && !isspace(*ptr) && (*ptr != '/'); ptr++);
  if(*ptr == '/') {
    /* parse switch, up to next whitespace. */
    for(*ptr++ = '\0', swptr = ptr; *ptr && !isspace(*ptr); ptr++);
  } else
    swptr = NULL;	/* no switches. */
  if(*ptr) {
    /* skip next whitespace. */
    for(*ptr++ = '\0'; *ptr && isspace(*ptr); ptr++);
  }

  /* check for aliased switches. */
  if(AliasSw_Inited) {
    entry = Hash_FindEntry(&AliasSw, cmd);
    if(entry != NULL){
      register char *tptr, *aptr;

      aptr = (char *)Hash_GetValue(entry);
      if(swptr != NULL) {
        tptr = (char *)alloca(strlen(aptr) + strlen(swptr) + 2);
        if(tptr == (char *)NULL)
          panic("handle_cmd: stack allocation failed.\n");
        strcpy(tptr, aptr);
	strcat(tptr, "/");
        strcat(tptr, swptr);
      } else {
        tptr = (char *)alloca(strlen(aptr) + 1);
        if(tptr == (char *)NULL)
          panic("handle_cmd: stack allocation failed.\n");
        strcpy(tptr, (char *)Hash_GetValue(entry));
      }
      swptr = tptr;
    }
  }

  /* try to find the command. */
  entry = Hash_FindEntry(&CmdHash, cmd);
  if(entry != NULL){
    cptr = (CMDS *)Hash_GetValue(entry);
    /* check access. */
    if(!cmdaccess(player, cptr->perms)) {
      notify_player(player, cause, player, "Permission denied.", 0);
      if(mudstat.status != MUD_DUMPING)
        cache_unlock();
      return;
    }

    /* parse switches. */
    switches = parse_switches(player, cause, cptr, swptr);
    if(switches == -1) {		/* abort */
      if (mudstat.status != MUD_DUMPING)
	cache_unlock();
      return;
    }

    /* Prevent excessive executing. */
    if(fexec)
      switches |= ARG_FEXEC;

    /* parse extra arguments if needed, and/or exec them, and run command. */
    switch(cptr->nargs){
    case 0:	/* simple case */
      (cptr->function) (player, cause, switches);
      break;
    case 1:	/* pretty simple case -- ptr is argument one. */
      if(*ptr && (fexec || !noexec(cptr->perms, player))) {
        register char *tptr = exec(player, cause, player, ptr, 0, eargc, eargv);
	(cptr->function) (player, cause, switches, tptr);
	ty_free((VOID *)tptr);
      } else
        (cptr->function) (player, cause, switches, ptr);
      break;
    case 2:	/* complex */
      /* ptr is at first argument, '=' seperates them. */
      parse_argtwo(player, cause, ptr, &argone, &argtwo, cptr->perms,
      		   eargc, eargv);
      (cptr->function) (player, cause, switches, argone, argtwo);
      ty_free((VOID *) argone);
      ty_free((VOID *) argtwo);
      break;
    case CMDS_VARARGS:	/* AUGH */
      /* ptr is at the start of the argument string. */
      parse_argvee(player, cause, ptr, &argc, argv, cptr->perms,
      		   eargc, eargv);
      (cptr->function) (player, cause, switches, argc, argv);
      free_argv(argc, argv);
      break;
    }
  } else {
    /* parse switches. */
    switches = parse_switches(player, cause, (CMDS *)NULL, swptr);
    if(switches == -1) {		/* abort */
      if (mudstat.status != MUD_DUMPING)
	cache_unlock();
      return;
    }

    switch(cmd[0]){
    case '@':		/* standard attribute, or exit. */
      for (i = 0; i < ATABLE_TOTAL; i++) {
        if (Atable[i].command && (stringprefix((Atable[i].calias) ?
                                               Atable[i].calias :
             Atable[i].name, cmd+1) || stringprefix(Atable[i].name, cmd+1)) &&
            cmd[1] && cmd[2] && cmd[2] != ' ') {
          parse_argtwo(player, cause, ptr, &argone, &argtwo, CMDS_NOEXEC,
	  	       eargc, eargv);
          do_set_string(player, cause, switches, argone, argtwo, &Atable[i]);
	  ty_free((VOID *)argone);
	  ty_free((VOID *)argtwo);
          if (mudstat.status != MUD_DUMPING)
            cache_unlock();
          cache_trim();
          return;
        }
      }

      /* check for an exit. */
      if(!handle_exit_cmd(player, cause, origcmd))
        notify_huh(player, cause);
      break;
    case '&':		/* attribute. */
      for (cmd++; *cmd && isspace(*cmd); cmd++);
      parse_argtwo(player, cause, ptr, &argone, &argtwo, CMDS_NOEXEC,
      		   eargc, eargv);
      i = resolve_object(player, cause, argone,
      			 (!(switches & CMD_QUIET) ? RSLV_EXITS|RSLV_NOISY
			 			  : RSLV_EXITS));
      if (i != -1) {
        if (cmd && *cmd) {
          set_attribute(player, cause, switches, i, cmd, argtwo);
        } else {
	  if(!(switches & CMD_QUIET))
            notify_player(player, cause, player, "Nothing to do.", NOT_QUIET);
        }
	ty_free((VOID *)argone);
	ty_free((VOID *)argtwo);
      }
      break;
    case '!':                   /* force */
      for (cmd++; *cmd && isspace(*cmd); cmd++);
      do_force(player, cause, switches, cmd, ptr);
      break;
    default:			/* try other things... */
      if(!attr_command(player, cause, '$', origcmd))
        notify_huh(player, cause);
      break;
    }
  }
  if(mudstat.status != MUD_DUMPING)
    cache_unlock();
}

/* do the command thing, recursively. */
void handle_cmds(player, cause, str, eargc, eargv)
    int player, cause;
    char *str;
    int eargc;
    char *eargv[];
{
  register char *ptr, *cmds;

  if((str == NULL) || (*str == '\0'))
    return;

  /* we don't want to write into str- it might be an attribute. */
  cmds = (char *)alloca(strlen(str)+1);
  if(cmds == (char *)NULL)
    panic("handle_cmds(): stack allocation failed\n");
  strcpy(cmds, str);

  /* commands are seperated by an unquoted semi-colon, or enclosed in braces. */
  /* we strip braces, but *don't* do anything with backslashes. */
  while(1) {
    while(*cmds && isspace(*cmds))
      cmds++;
    if(!*cmds)
      break;	/* we're done. */

    if(*cmds == '{') {
      cmds++;
      ptr = parse_to(cmds, '{', '}');
      if(ptr == NULL) {	/* fuck you, too. */
        handle_cmd(player, cause, &cmds[-1], eargc, eargv);
	return;		/* we're done. */
      }
      handle_cmd(player, cause, cmds, eargc, eargv);
      cmds = ptr;
    } else {
      ptr = parse_to(cmds, '\0', ';');
      handle_cmd(player, cause, cmds, eargc, eargv);
      if(ptr && *ptr)
        cmds = ptr;
      else
        break;		/* we're done. */
    }
  }
}
