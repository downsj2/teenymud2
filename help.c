
#include "config.h"

/*
 *		       This file is part of TeenyMUD II.
 *		 Copyright(C) 1993, 1994, 1995, 2013 by Jason Downs.
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
#define alloca  __builtin_alloca
#else   /* not __GNUC__ */
#ifdef HAVE_ALLOCA_H
#include <alloca.h>
#else   /* not HAVE_ALLOCA_H */
#ifdef _AIX
 #pragma alloca
#endif  /* not _AIX */
#endif  /* not HAVE_ALLOCA_H */
#endif  /* not __GNUC__ */

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
#endif
#include <time.h>
#include <sys/stat.h>

#include "conf.h"
#include "teeny.h"
#include "commands.h"
#include "externs.h"

#include "uthash/uthash.h"

/*
 * The TeenyMUD help system is implemented by scanning the file(s) and
 * building a hash table, resolving subjects to file offsets.
 *
 * The table(s) are rebuilt automatically, if the time stamp on the file(s)
 * change.
 *
 * I really dislike this code today.  It should be rewritten to just
 * mmap() the damn files.
 */

#define MAXSUBJECTLEN	80
struct file_index {
  char subject[MAXSUBJECTLEN];
  off_t offset;

  UT_hash_handle hh;
};
static struct file_index *file_index_help = NULL;
static struct file_index *file_index_news = NULL;
 
static time_t file_mod_help = 0;
static time_t file_mod_news = 0;

static char buffer[BUFFSIZ];
static struct stat sbuf;

static void init_table _ANSI_ARGS_((FILE *, struct file_index **));
static void free_help _ANSI_ARGS_((struct file_index **));
static void display_topic _ANSI_ARGS_((int, int, const char *, const char *, struct file_index **, time_t *, const char *, int));

static void init_table(fd, tp)
    FILE *fd;
    struct file_index **tp;	/* Pointer to one of the hashes. */
{
  char *p, *q;
  struct file_index *fp;
  int lineno = 0;

  while (!feof(fd) && (fgets(buffer, sizeof(buffer), fd) != NULL)) {
    lineno++;
    if ((buffer[0] == '&') && buffer[1] && isspace(buffer[1])) {
      remove_newline(buffer);
      for (p = buffer; p[0] && !isspace(p[0]); p++);
      for (; p[0] && isspace(p[0]); p++);
      if (!p[0])
	continue;

      /* See if it already exists. */
      HASH_FIND_STR(*tp, p, fp);
      if (fp) {
	logfile(LOG_ERROR, "init_table: Skipping duplicate subject %s at line #%d\n", p, lineno);
	continue;
      }

      fp = ty_malloc(sizeof(struct file_index), "init_table");

      /* Lower case copy of the subject. */
      q = fp->subject;
      while(*p && (q - fp->subject) < (MAXSUBJECTLEN - 1)) {
	*q++ = to_lower(*p++);
      }
      *q = '\0';
      if (strlen(p) >= MAXSUBJECTLEN)
	logfile(LOG_ERROR, "init_table: Subject %s truncated to %s at line #%d\n", p, fp->subject, lineno);
      fp->offset = ftell(fd);

      HASH_ADD_STR(*tp, subject, fp);
    }
  }
}

void help_init()
{
  FILE *fd;

  if (file_index_help == NULL) {
    if ((stat(mudconf.help_file, &sbuf) != 0) ||
	((fd = fopen(mudconf.help_file, "r")) == NULL)) {
      logfile(LOG_ERROR, "help_init: couldn't open help file, %s\n",
	      mudconf.help_file);
    } else {
      init_table(fd, &file_index_help);
      fclose(fd);
      file_mod_help = sbuf.st_mtime;
    }
  }
  if (file_index_news == NULL) {
    if ((stat(mudconf.news_file, &sbuf) != 0) ||
	((fd = fopen(mudconf.news_file, "r")) == NULL)) {
      logfile(LOG_ERROR, "help_init: couldn't open news file, %s\n",
	      mudconf.news_file);
    } else {
      init_table(fd, &file_index_news);
      fclose(fd);
      file_mod_news = sbuf.st_mtime;
    }
  }
}

static void free_help(tp)
    struct file_index **tp;
{
  struct file_index *fp, *t;

  HASH_ITER(hh, *tp, fp, t) {
    HASH_DEL(*tp, fp);
    ty_free(fp);
  }
  /* Make sure it's clear. */
  HASH_CLEAR(hh, *tp);
}

/* Display a topic from either of the indexes. */
#ifdef __STDC__
static void display_topic(int player, int cause, const char *utopic, const char *filename, struct file_index **index, time_t *index_mod, const char *index_type, int quiet)
#else
static void display_topic(player, cause, utopic, filename, index, index_mod, index_type, quiet)
    int player, cause;
    const char *utopic, *filename;
    struct file_index **index;
    time_t *index_mod;
    const char *index_type;
    int quiet;
#endif
{
  FILE *fd;
  struct file_index *fp;
  time_t last;
  char buf[MEDBUFFSIZ];
  char *topic;
  int err;
	
  if ((utopic == (char *)NULL) || (utopic[0] == '\0'))
    topic = "summary";
  else {
    const char *p;
    char *q;

    topic = alloca(strlen(utopic) + 1);
    if (topic == NULL)
      panic("do_help(): stack allocation failed.\n");
    for(p = utopic, q = topic; *p; *q++ = to_lower(*p++));
    *q = '\0';
  }

  if ((*index == NULL) || mudconf.hfile_autoload) {
    err = ((stat(filename, &sbuf) != 0) ||
    	   (fd = fopen(filename, "r")) == NULL);
  } else {
    err = ((fd = fopen(filename, "r")) == NULL);
  }

  if (err) {
    if(!quiet) {
      snprintf(buf, sizeof(buf), "Sorry, %s file seems to be broken.",
	       index_type);
      notify_player(player, cause, player, buf, NOT_QUIET);
    }
    return;
  }

  if ((*index == NULL) || (*index_mod == 0)) {
    help_init();
    last = sbuf.st_mtime;
  } else {
    last = *index_mod;
    if (mudconf.hfile_autoload && (sbuf.st_mtime > last)) {
      free_help(index);
      help_init();
    }
  }

  /* Iterate and display all subjects. */
  if (strcasecmp(topic, "-summary") == 0) {
    struct file_index *t;

    snprintf(buf, sizeof(buf), "There are %d total %s topics.", HASH_COUNT(*index), index_type);
    notify_player(player, cause, player, buf, 0);

    HASH_ITER(hh, *index, fp, t) {
      notify_player(player, cause, player, fp->subject, 0);
    }
    return;
  }

  /* Find the subject. */
  HASH_FIND_STR(*index, topic, fp);
  if (!fp) {
    if(!quiet) {
      snprintf(buf, sizeof(buf), "Sorry, no %s available for %s.", index_type, utopic);
      notify_player(player, cause, player, buf, NOT_QUIET);
    }
    fclose(fd);
    return;
  }

  fseek(fd, fp->offset, 0);
  while (!feof(fd) && (fgets(buffer, sizeof(buffer), fd) != NULL)) {
    if (!strncmp(buffer, "@@@@", 4))
      break;
    if ((buffer[0] != '&') || (buffer[1] && !isspace(buffer[1]))) {
      remove_newline(buffer);
      notify_player(player, cause, player, (buffer[0]) ? buffer : " ", 0);
    }
  }
  fclose(fd);
}

VOID do_help(player, cause, switches, argone)
    int player, cause, switches;
    char *argone;
{
  if (switches & HELP_RELOAD) {
    free_help(&file_index_help);
    help_init();

    if (!(switches & CMD_QUIET))
      notify_player(player, cause, player, "Help file reloaded.", NOT_QUIET);
    return;
  }

  if (!(switches & HELP_NOHTML) && has_html(player))
    notify_player(player, cause, player, mudconf.help_url, NOT_RAW);

  display_topic(player, cause, argone, mudconf.help_file, &file_index_help, &file_mod_help, "help", (switches & CMD_QUIET));
}

VOID do_news(player, cause, switches, argone)
    int player, cause, switches;
    char *argone;
{
  if (switches & NEWS_RELOAD) {
    free_help(&file_index_news);
    help_init();

    if (!(switches & CMD_QUIET))
      notify_player(player, cause, player, "News file reloaded.", NOT_QUIET);
    return;
  }

  if (!(switches & NEWS_NOHTML) && has_html(player))
    notify_player(player, cause, player, mudconf.news_url, NOT_RAW);

  display_topic(player, cause, argone, mudconf.news_file, &file_index_news, &file_mod_news, "news", (switches & CMD_QUIET));
}

/*
 * Display the news file last modification time to a player.
 */
void check_news(player)
    int player;
{
  char buf[BUFFSIZ];

  if (stat(mudconf.news_file, &sbuf) == 0) {
    strftime(buf, sizeof(buf),
	     "News file last updated: %a %b %e %H:%M:%S %Z %Y",
	     localtime(&sbuf.st_mtime));

    notify_player(player, player, player, buf, 0);
  }
}
