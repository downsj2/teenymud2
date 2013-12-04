/* teenydbm.c */

#include "config.h"

/*
 *		       This file is part of TeenyMUD II.
 *		    Copyright(C) 1994, 1995 by Jason Downs.
 *                              All rights reserved.
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
#include <signal.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif			/* HAVE_UNISTD_H */
#include <ctype.h>

#include "conf.h"
#include "teeny.h"
#include "teenydb.h"
#include "gnu/getopt.h"
#include "readline/readline.h"
#include "externs.h"

extern int errno;

extern int screenheight;		/* from readline */

static void print_version _ANSI_ARGS_((void));
static void print_help _ANSI_ARGS_((void));
static void print_greet _ANSI_ARGS_((void));
static void print_warranty _ANSI_ARGS_((void));
static void init_signals _ANSI_ARGS_((void));
static void page_start _ANSI_ARGS_((void));
static int page_append _ANSI_ARGS_((const char *));
static int cmd_loop _ANSI_ARGS_((void));
static void cmd_clear _ANSI_ARGS_((void));
static void cmd_status _ANSI_ARGS_((void));
static int strtoobj _ANSI_ARGS_((char *));
static void cmd_mark _ANSI_ARGS_((char *));

static char *progname;
static char *prompt;

static struct option const getopt_options[] =
{
  { "configuration", required_argument, 0, 'c' },
  { "help", no_argument, 0, 'h' },
  { "version", no_argument, 0, 'v' },
  { NULL, 0, 0, '\0' }
};

extern const char teenymud_version[];

static void print_version()
{
  fprintf(stderr,
  	  "This is the TeenyMUD Database Manager, version %s\n",
	  teenymud_version);
}

static void print_help()
{
  print_version();

  fputs("Choose from the following:\n", stderr);
  fputs("-c, --configuration\tspecify configuration file\n", stderr);
  fputs("-h, --help\t\tprint this help message\n", stderr);
  fputs("-v, --version\t\tprint the version number\n", stderr);
}

static void print_greet()
{
  fprintf(stdout,
  	  "TeenyMUD Database Manager (teenydbm), %s.\n", teenymud_version);
  fputs("Copyright (C) 1994, 1995, Jason Downs.\n", stdout);
  fputs("This is free software with ABSOLUTELY NO WARRANTY.\n", stdout);
  fputs("Type 'warranty' for further details.\n", stdout);
  fputs("Type 'license' for distribution information.\n", stdout);
}

static void print_warranty()
{
  fputs("This program is distributed in the hope that it will be useful,\n",
  	stdout);
  fputs("but WITHOUT ANY WARRANTY; without even the implied warranty of\n",
  	stdout);
  fputs("MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n",
  	stdout);
  fputs("GNU General Public License for more details.\n", stdout);
}

/* The dbm doesn't care so much about signals. */
static RETSIGTYPE handle_intr()
{
  fputs("\nUse 'quit' from the command line to exit, or ^D from the pager.\n",
        stdout);

#ifdef HAVE_SYSV_SIGNALS
  init_signals();
#endif			/* HAVE_SYSV_SIGNALS */
}

static void init_signals()
{
  signal(SIGINT, handle_intr);
}

int main(argc, argv)
    int argc;
    char **argv;
{
  char *conf_file = (char *)NULL;
  int ch;

  progname = ty_basename(argv[0]);

  opterr = 0;
  while ((ch = getopt_long(argc, argv, "c:hv",
  			   getopt_options, (int *)0)) != -1) {
    switch (ch) {
    case 'h':
      print_help();
      exit(0);
    case 'v':
      print_version();
      exit(0);
    case 'c':
      conf_file = optarg;
      break;
    default:
      fprintf(stderr, "%s: bad option -- %c\n", progname, ch);
      fprintf(stderr, "%s: Use '%s --help' for complete list of options.\n",
      	      progname, progname);
      exit(0);
    }
  }

  if(conf_file == (char *)NULL) {
    fprintf(stderr, "%s: You must specify a configuration file.\n", progname);
    exit(0);
  }

  /* works just like the MUD. */
  mudstat.status = MUD_COLD;
  time(&mudstat.now);

  print_greet();

  fputs("Initializing...\n", stdout);

  init_signals();

  if(conf_init(conf_file) == -1)
    exit(-1);

  /* first things first. */
  if(chdir(mudconf.chdir_path) != 0) {
    fprintf(stderr, "%s: chdir [%s] failed: %s, continuing...\n", progname,
	    mudconf.chdir_path, strerror(errno));
  }

  cache_init();

  if((dbmfile_open(mudconf.dbm_file, 0) != 0) 
     || (database_read(mudconf.db_file) != 0)) {
    fputs("Error opening database file.\n", stderr);
    exit(-1);
  }

  fprintf(stdout, "Read configuration for %s.\n", mudconf.name);

  prompt = (char *)ty_malloc(strlen(progname) + 2, "main");
  sprintf(prompt, "%s> ", progname);

  while(!cmd_loop());	/* loop forever */

  cache_flush();
  dbmfile_close();
  if(database_write(mudconf.db_file) == -1)
    fputs("Error writing database file.\n", stderr);

  ty_free(prompt);

  return(0);
}

static int lines_max = 0;
static int lines_cur = 0;

static void page_start()
{
  /* initialize page length. */
  if(lines_max == 0) {
    /* Ask readline. */
    lines_max = screenheight;

    if(lines_max)
      lines_max -= 2;
  }
  if(lines_max == 0)
    lines_max = 22;

  lines_cur = 0;
}

static int page_append(str)
    const char *str;
{
  if (lines_cur < lines_max) {
    fputs(str, stdout);
    fputc('\n', stdout);

    lines_cur++;
  } else {
    char buf[2];

    fputs("---press <return> to continue---", stdout);
    fflush(stdout);
    if(fgets(buf, sizeof(buf), stdin) == (char *)NULL) {
      clearerr(stdin);
      fputc('\n', stdout);
      return(0);
    }
    fputc('\n', stdout);

    fputs(str, stdout);
    fputc('\n', stdout);
    lines_cur = 1;
  }

  return(1);
}

/* the main input loop and parser. */
static int cmd_loop()
{
  char *line;
  int success = 0;

  line = readline(prompt);
  if(line == (char *)NULL) {	/* EOF */
    fputs("Goodbye.\n", stdout);
    return(1);
  }
  if(*line == '\0') {		/* empty line */
    free(line);
    return(0);
  }

  add_history(line);

  /* simple parser. */
  switch(line[0]) {
  case 'c':
  case 'C':
    if(strncasecmp(line, "clear", 5) == 0) {
      cmd_clear();
      success++;
    }
    break;
  case 'l':
  case 'L':
    if(strncasecmp(line, "license", 7) == 0) {
      FILE *gfp;
      char gbuf[128];

      if((gfp = fopen("license.txt", "r")) == (FILE *)NULL) {
        if((gfp = fopen("text/license.txt", "r")) == (FILE *)NULL) {
	  gfp = fopen("files/license.txt", "r");
	}
      }

      if(gfp == (FILE *)NULL)
        fputs("Can't open license.txt\n", stdout);
      else {
        page_start();
        while(!feof(gfp) && (fgets(gbuf, sizeof(gbuf), gfp) != (char *)NULL)) {
	  remove_newline(gbuf);
          if(!page_append(gbuf))
	    break;
        }
      }

      success++;
    }
    break;
  case 'm':
  case 'M':
    if(strncasecmp(line, "mark", 4) == 0) {
      cmd_mark(line + 4);
      success++;
    }
    break;
  case 's':
  case 'S':
    if(strncasecmp(line, "status", 6) == 0) {
      cmd_status();
      success++;
    }
    break;
  case 'q':
  case 'Q':
    if(strncasecmp(line, "quit", 4) == 0) {
      fputs("Goodbye.\n", stdout);

      free(line);
      return(1);
    }
    break;
  case 'w':
  case 'W':
    if(strncasecmp(line, "warranty", 8) == 0) {
      print_warranty();
      success++;
    }
    break;
  }

  if(!success)
    fputs("Unknown command.  Type 'help' for help.\n", stdout);

  free(line);
  return(0);
}

/* clear all MARKED objects. */
static void cmd_clear()
{
  register int indx;
  int count = 0;

  for(indx = 0; indx < mudstat.total_objects; indx++) {
    if(main_index[indx] != (struct dsc *)NULL) {
      if(DSC_FLAG2(main_index[indx]) & MARKED) {
        DSC_FLAG2(main_index[indx]) &= ~MARKED;
	count++;
      }
    }
  }

  fprintf(stdout, "Unmarked %d objects.\n", count);
}

/* print (hopefully) useful database statistics. */
static void cmd_status()
{
  register int indx;
  int total_players = 0;
  int total_rooms = 0;
  int total_exits = 0;
  int total_things = 0;
  int total_garbage = 0;
  int total_marked = 0;

  for(indx = 0; indx < mudstat.total_objects; indx++) {
    if(main_index[indx] != (struct dsc *)NULL) {
      if(DSC_FLAG2(main_index[indx]) & MARKED)
        total_marked++;
      switch(DSC_TYPE(main_index[indx])) {
      case TYP_PLAYER:
        total_players++;
	break;
      case TYP_ROOM:
        total_rooms++;
	break;
      case TYP_EXIT:
        total_exits++;
	break;
      case TYP_THING:
        total_things++;
	break;
      }
    } else
      total_garbage++;
  }

  fputs("Current Statistics:\n", stdout);
  fprintf(stdout, "\t%d marked of %d total objects.\n", total_marked,
  	  mudstat.total_objects);
  fprintf(stdout, "\t%d players, %d rooms, %d exits, %d things (%d garbage).\n",
  	  total_players, total_rooms, total_exits, total_things,
	  total_garbage);
}

static int strtoobj(str)
    char *str;
{
  char *ptr;
  int obj;

  if(*str != '#')
    return(-1);

  str++;
  obj = (int)strtol(str, &ptr, 10);
  if(ptr == str)
    return(-1);

  return(obj);
}

static void cmd_mark(args)
    char *args;
{
  char *arg1, *arg2;
  int obj, indx;
  int display = 1;
  char buf[BUFSIZ];

  /* parse out arguments */
  arg1 = args;
  while((*arg1 != '\0') && isspace(*arg1))
    arg1++;
  arg2 = arg1;
  while((*arg2 != '\0') && !isspace(*arg2))
    arg2++;
  while((*arg2 != '\0') && isspace(*arg2))
    arg2++;
  
  if(*arg1 == '\0') {
    fputs("The 'mark' command requires at least one argument.\n", stdout);
    return;
  }

  /* easy one, first. */
  if((*arg2 == '\0') && ((obj = strtoobj(arg1)) != -1)) {
    if(!_exists_object(obj)) {
      fputs("No such object.\n", stdout);
      return;
    }
    DSC_FLAG2(main_index[obj]) |= MARKED;

    fprintf(stdout, "Marked object #%d.\n", obj);
    return;
  }

  /* parse first argument */
  switch(arg1[0]) {
  case 'o':
  case 'O':
    /* mark matching owners. */
    obj = strtoobj(arg2);
    if(obj == -1) {
      fputs("Illegal object.\n", stdout);
      return;
    }

    page_start();
    for(indx = 0; indx < mudstat.total_objects; indx++) {
      if(main_index[indx] != (struct dsc *)NULL) {
        if(DSC_OWNER(main_index[indx]) == obj) {
	  if(display) {
	    sprintf(buf, "Marked object #%d.", indx);
	    display = page_append(buf);
	  }
	  DSC_FLAG2(main_index[indx]) |= MARKED;
	}
      }
    }
    break;
  default:
    fputs("No such keyword.\n", stdout);
  }

}
