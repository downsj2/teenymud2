/* help.c */

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
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#else
#include <time.h>
#endif			/* HAVE_SYS_TIME_H */
#include <sys/stat.h>

#include "conf.h"
#include "teeny.h"
#include "commands.h"
#include "externs.h"

#include "hash/hash.h"

/*
 * The TeenyMUD help system is implemented by scanning the file(s) and
 * building an associative array (hash table), resolving subjects to
 * file offsets.  It hadles both ``old fashioned'' 32bit offsets, and
 * newer, large, file offsets.
 *
 * The table(s) are rebuilt automatically, if the time stamp on the file(s)
 * change.
 */

static Hash_Table HelpTable;
static Hash_Table NewsTable;
static int HelpTableInited = 0;
static int NewsTableInited = 0;
static char buffer[128];
static struct stat sbuf;
static char *Mod_Entry = "FMODTIME";

static void init_table _ANSI_ARGS_((FILE *, Hash_Table *));
static void free_help _ANSI_ARGS_((Hash_Table *));

static void init_table(fd, table)
    FILE *fd;
    Hash_Table *table;
{
  register char *p;
  Hash_Entry *hash;
  int new;
#if (SIZEOF_OFF_T > SIZEOF_INT_P)
  register int *offset;
  off_t where;
#endif

  Hash_InitTable(table, 0, HASH_STRCASE_KEYS);
  while (!feof(fd) && (fgets(buffer, sizeof(buffer), fd) != NULL)) {
    if ((buffer[0] == '&') && buffer[1]) {
      remove_newline(buffer);
      for (p = buffer; p[0] && !isspace(p[0]); p++);
      for (; p[0] && isspace(p[0]); p++);
      if (!p[0])
	continue;
      hash = Hash_CreateEntry(table, p, &new);

#if (SIZEOF_OFF_T > SIZEOF_INT_P)
      offset = (int *)ty_malloc(sizeof(off_t), "init_table");
      where = ftell(fd);
      bcopy((VOID *)&where, (VOID *)offset, sizeof(off_t));
      Hash_SetValue(hash, offset);
#else
      Hash_SetValue(hash, (int *) ftell(fd));
#endif
    }
  }
}

void help_init()
{
  Hash_Entry *hash;
  int new;
  FILE *fd;

  if (!HelpTableInited) {
    if ((stat(mudconf.help_file, &sbuf) != 0) ||
	((fd = fopen(mudconf.help_file, "r")) == NULL)) {
      logfile(LOG_ERROR, "help_init: couldn't open help file, %s\n",
	      mudconf.help_file);
    } else {
      init_table(fd, &HelpTable);
      fclose(fd);
      HelpTableInited = 1;
      hash = Hash_CreateEntry(&HelpTable, Mod_Entry, &new);
      Hash_SetValue(hash, (int *) sbuf.st_mtime);
    }
  }
  if (!NewsTableInited) {
    if ((stat(mudconf.news_file, &sbuf) != 0) ||
	((fd = fopen(mudconf.news_file, "r")) == NULL)) {
      logfile(LOG_ERROR, "help_init: couldn't open news file, %s\n",
	      mudconf.news_file);
    } else {
      init_table(fd, &NewsTable);
      fclose(fd);
      NewsTableInited = 1;
      hash = Hash_CreateEntry(&NewsTable, Mod_Entry, &new);
      Hash_SetValue(hash, (int *) sbuf.st_mtime);
    }
  }
}

static void free_help(table)
    Hash_Table *table;
{
#if (SIZEOF_OFF_T > SIZEOF_INT_P)
  Hash_Entry *entry;
  Hash_Search srch;

  for(entry = Hash_EnumFirst(table, &srch); entry != (Hash_Entry *)NULL;
      entry = Hash_EnumNext(&srch)) {
    ty_free((VOID *)Hash_GetValue(entry));
  }
#endif
  Hash_DeleteTable(table);
}

VOID do_help(player, cause, switches, argone)
    int player, cause, switches;
    char *argone;
{
  FILE *fd;
  Hash_Entry *hash;
  time_t last;
  char buf[MEDBUFFSIZ];
  char *topic;
#if (SIZEOF_OFF_T > SIZEOF_INT_P)
  register off_t *optr;
#endif
  int err;

  if (switches & HELP_RELOAD) {
    HelpTableInited = 0;
    free_help(&HelpTable);
    help_init();

    if (!(switches & CMD_QUIET))
      notify_player(player, cause, player, "Help file reloaded.", NOT_QUIET);
    return;
  }

  if ((argone == (char *)NULL) || (argone[0] == '\0')
      || strcmp(argone, Mod_Entry) == 0)
    topic = "summary";
  else
    topic = argone;

  if (!HelpTableInited || mudconf.hfile_autoload) {
    err = ((stat(mudconf.help_file, &sbuf) != 0) ||
    	   (fd = fopen(mudconf.help_file, "r")) == NULL);
  } else {
    err = ((fd = fopen(mudconf.help_file, "r")) == NULL);
  }

  if (err) {
    if(!(switches & CMD_QUIET)) {
      snprintf(buf, sizeof(buf), "Sorry, %s seems to be broken.",
	       mudconf.help_file);
      notify_player(player, cause, player, buf, NOT_QUIET);
    }
    return;
  }
  if (!HelpTableInited
      || ((hash = Hash_FindEntry(&HelpTable, Mod_Entry)) == NULL)) {
    help_init();
    last = sbuf.st_mtime;
  } else {
    last = (time_t) Hash_GetValue(hash);
  }
  if (mudconf.hfile_autoload) {
    if (sbuf.st_mtime > last) {
      HelpTableInited = 0;
      free_help(&HelpTable);
      help_init();
    }
  }

  if (strcasecmp(topic, "-summary") == 0) {
    Hash_Search srch;

    for(hash = Hash_EnumFirst(&HelpTable, &srch); hash != NULL;
        hash = Hash_EnumNext(&srch))
      notify_player(player, cause, player, Hash_GetKey(&HelpTable, hash), 0);
    return;
  }
  if ((hash = Hash_FindEntry(&HelpTable, topic)) == NULL) {
    if(!(switches & CMD_QUIET)) {
      snprintf(buf, sizeof(buf), "No help available for %s.", topic);
      notify_player(player, cause, player, buf, NOT_QUIET);
    }
    fclose(fd);
    return;
  }

#if (SIZEOF_OFF_T > SIZEOF_INT_P)
  optr = (off_t *)Hash_GetValue(hash);
  fseek(fd, *optr, 0);
#else
  fseek(fd, (off_t) Hash_GetValue(hash), 0);
#endif
  while (!feof(fd) && (fgets(buffer, sizeof(buffer), fd) != NULL)) {
    if (!strncmp(buffer, "@@@@", 4))
      break;
    if (buffer[0] != '&') {
      remove_newline(buffer);
      notify_player(player, cause, player, (buffer[0]) ? buffer : " ", 0);
    }
  }
  fclose(fd);
}

VOID do_news(player, cause, switches, argone)
    int player, cause, switches;
    char *argone;
{
  FILE *fd;
  Hash_Entry *hash;
  time_t last;
  char buf[MEDBUFFSIZ];
  char *topic;
#if (SIZEOF_OFF_T > SIZEOF_INT_P)
  register off_t *optr;
#endif
  int err;

  if (switches & NEWS_RELOAD) {
    NewsTableInited = 0;
    free_help(&NewsTable);
    help_init();

    if (!(switches & CMD_QUIET))
      notify_player(player, cause, player, "News file reloaded.", NOT_QUIET);
    return;
  }

  if ((argone == (char *)NULL) || (argone[0] == '\0')
      || strcmp(argone, Mod_Entry) == 0)
    topic = "summary";
  else
    topic = argone;

  if (!NewsTableInited || mudconf.hfile_autoload) {
    err = ((stat(mudconf.news_file, &sbuf) != 0)
    	   || (fd = fopen(mudconf.news_file, "r")) == NULL);
  } else {
    err = ((fd = fopen(mudconf.news_file, "r")) == NULL);
  }

  if (err) {
    if(!(switches & CMD_QUIET)) {
      snprintf(buf, sizeof(buf), "Sorry, %s seems to be broken.",
	       mudconf.news_file);
      notify_player(player, cause, player, buf, NOT_QUIET);
    }
    return;
  }
  if (!NewsTableInited
      || ((hash = Hash_FindEntry(&NewsTable, Mod_Entry)) == NULL)) {
    help_init();
    last = sbuf.st_mtime;
  } else {
    last = (time_t) Hash_GetValue(hash);
  }
  if (mudconf.hfile_autoload) {
    if (sbuf.st_mtime > last) {
      NewsTableInited = 0;
      free_help(&NewsTable);
      help_init();
    }
  }

  if (strcasecmp(topic, "-summary") == 0) {
    Hash_Search srch;

    for(hash = Hash_EnumFirst(&NewsTable, &srch); hash != NULL;
        hash = Hash_EnumNext(&srch))
      notify_player(player, cause, player, Hash_GetKey(&NewsTable, hash), 0);
    return;
  }
  if ((hash = Hash_FindEntry(&NewsTable, topic)) == NULL) {
    if(!(switches & CMD_QUIET)) {
      snprintf(buf, sizeof(buf), "No news available for %s.", topic);
      notify_player(player, cause, player, buf, NOT_QUIET);
    }
    fclose(fd);
    return;
  }

#if (SIZEOF_OFF_T > SIZEOF_INT_P)
  optr = (off_t *)Hash_GetValue(hash);
  fseek(fd, *optr, 0);
#else
  fseek(fd, (off_t) Hash_GetValue(hash), 0);
#endif
  while (!feof(fd) && (fgets(buffer, sizeof(buffer), fd) != NULL)) {
    if (!strncmp(buffer, "@@@@", 4))
      break;
    if (buffer[0] != '&') {
      remove_newline(buffer);
      notify_player(player, cause, player, (buffer[0]) ? buffer : " ", 0);
    }
  }
  fclose(fd);
}

/*
 * Display the news file last modification time to a player.
 */
void check_news(player)
    int player;
{
  char buf[MEDBUFFSIZ];

  if (stat(mudconf.news_file, &sbuf) == 0) {
    strftime(buf, BUFFSIZ,
	     "News file last updated: %a %b %e %H:%M:%S %Z %Y",
	     localtime(&sbuf.st_mtime));

    notify_player(player, player, player, buf, 0);
  }
}
