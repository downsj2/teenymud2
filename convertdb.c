/* convertdb.c */

#include "config.h"

/*
 *		       This file is part of TeenyMUD II.
 *		 Copyright(C) 1993, 1994, 1995, 2013, 2022 by Jason Downs.
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
#include <time.h>
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif			/* HAVE_STDLIB_H */
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif			/* HAVE_UNISTD_H */
#include <errno.h>
#if defined(HAVE_GETOPT_LONG) && defined(HAVE_GETOPT_H)
#include <getopt.h>
#endif

#include "conf.h"
#include "teeny.h"
#include "teenydb.h"
#include "textdb.h"
#include "ptable.h"
#include "sha/sha_wrap.h"
#include "externs.h"
#ifndef HAVE_GETOPT_LONG
#include "compat/getopt.h"
#endif

static void print_version _ANSI_ARGS_((void));
static void print_help _ANSI_ARGS_((void));
static void upgrade_database _ANSI_ARGS_((int, int, int));
static void create_startup _ANSI_ARGS_((void));

enum text_state { TEXT_NONE, TEXT_READ, TEXT_WRITE };

static char *progname;

static struct option const getopt_options[] =
{
  { "configuration", required_argument, 0, 'c' },
  { "filter-garbage", no_argument, 0, 'G' },
  { "input-file", required_argument, 0, 'i' },
  { "initialize-db", no_argument, 0, 's' },
  { "output-file", required_argument, 0, 'o' },
  { "nonstandard", no_argument, 0, 'C' },
  { "xibo-compat", no_argument, 0, 'X' },
  { "use-existing-root", no_argument, 0, 'e' },
  { "help", no_argument, 0, 'h' },
  { "version", no_argument, 0, 'v' },
  { NULL, 0, 0, '\0' }
};
 
extern const char teenymud_version[];

static void print_version()
{
  fprintf(stderr,
  	  "This is the TeenyMUD Database Conversion System, version %s\n",
	  teenymud_version);
}

static void print_help()
{
  print_version();

  fputs("Choose from the following:\n", stderr);
  fputs("-c, --configuration\tspecify configuration file\n", stderr);
  fputs("-e, --use-existing-root\tforce use of existing root room\n", stderr);
  fputs("-G, --filter-garbage\tdo not write garbage objects\n", stderr);
  fputs("-i, --input-file\tspecify input file\n", stderr);
  fputs("-o, --output-file\tspecify output file\n", stderr);
  fputs("-s, --initialze-db\tcreate a new database\n", stderr);
  fputs("-C, --nonstandard\tenable nonstandard compatibility\n", stderr);
  fputs("-X, --xibo-compat\tenable 1.3 compatibility\n", stderr);
  fputs("-h, --help\t\tprint this help message\n", stderr);
  fputs("-v, --version\t\tprint the version number\n", stderr);
}

int main(argc, argv)
    int argc;
    char **argv;
{
  int ch, read_version, read_flags, read_total;
  int dash_c = 0, one_three = 0, use_existing = 0;
  int startup = 0;
  int output_flags = OUTPUT_FLAGS;
  enum text_state state = TEXT_NONE;
  char *conf_file = (char *) NULL;
  char *text_in = (char *) NULL;
  char *text_out = (char *) NULL;
  char buffer[BUFFSIZ];
  FILE *text;

  progname = ty_basename(argv[0]); 

  opterr = 0;
  while ((ch = getopt_long(argc, argv, "c:i:o:esCXGhv",
  			   getopt_options, (int *)0)) != -1) {
    switch (ch) {
    case 'h':
      print_help();
      exit(0);
    case 'v':
      print_version();
      exit(0);
    case 's':
      startup++;
      break;
    case 'e':
      use_existing++;
      break;
    case 'c':			/* config file */
      conf_file = optarg;
      break;
    case 'i':
      if (text_out != (char *) NULL) {
	fprintf(stderr, "%s: You may only specify one of -i or -o.\n",
		progname);
	fprintf(stderr, "%s: Use '%s --help' for a complete list of options.\n",
		progname, progname);
	exit(0);
      }
      text_in = optarg;
      state = TEXT_READ;
      break;
    case 'o':
      if (text_in != (char *) NULL) {
	fprintf(stderr, "%s: You may only specify one of -i or -o.\n",
		progname);
	fprintf(stderr, "%s: Use '%s --help' for a complete list of options.\n",
		progname, progname);
	exit(0);
      }
      text_out = optarg;
      state = TEXT_WRITE;
      break;
    case 'C':			/* ancient -C database format */
      dash_c++;
      break;
    case 'X':			/* Xibo database format(tm) */
      one_three++;
      break;
    case 'G':
      output_flags |= DB_GFILTER;
      break;
    default:
      fprintf(stderr, "%s: bad option -- %c\n", progname, ch);
      fprintf(stderr, "%s: Use '%s --help' for a complete list of options.\n",
	      progname, progname);
      exit(0);
    }
  }
  if (startup && (state != TEXT_NONE)) {
    fprintf(stderr, "%s: You cannot initialize a text database!\n", progname);
    exit(0);
  }
  if (conf_file == (char *) NULL) {
    fprintf(stderr, "%s: You must specify a configuration file.\n", progname);
    exit(0);
  }
  if (dash_c && (state != TEXT_READ)) {
    fprintf(stderr,
	    "%s: You may only specify -C while reading a database.\n",
	    progname);
    fprintf(stderr, "%s: Use '%s --help' for a complete list of options.\n",
	    progname, progname);
    exit(0);
  }
  if (one_three && (state != TEXT_READ)) {
    fprintf(stderr,
	    "%s: You may only specify -X while reading a database.\n",
	    progname);
    fprintf(stderr, "%s: Use '%s --help' for a complete list of options.\n",
	    progname, progname);
    exit(0);
  }
  if ((output_flags & DB_GFILTER) && (state != TEXT_WRITE)) {
    fprintf(stderr,
	    "%s: You may only specify -G while writing a database.\n",
	    progname);
    fprintf(stderr, "%s: Use '%s --help' for a complete list of options.\n",
	    progname, progname);
    exit(0);
  }

  /* works just like the MUD, now. */
  mudstat.status = MUD_COLD;
  time(&mudstat.now);

  if(conf_init(conf_file) == -1)
    exit(-1);

  /* first things first. */
  if(chdir(mudconf.chdir_path) != 0) {
    fprintf(stderr, "%s: chdir [%s] failed: %s, continuing...\n", progname,
	    mudconf.chdir_path, strerror(errno));
  }

  cache_init();

  fprintf(stderr, "TeenyMUD %s database conversion system.\n",
  	  teenymud_version);
  fprintf(stdout, "TeenyMUD %s is Copyright(C) 1993, 1994, 1995, 2013, 2022 Jason Downs.\n",
          teenymud_version);
  fputs("TeenyMUD is free software and comes with absolutely no warranty.\n",
        stdout);

  fprintf(stderr, "Read configuration for %s.\n", mudconf.name);

  if(startup)
    create_startup();	/* This is all self-contained. */

  switch (state) {
  case TEXT_READ:
    if ((text = fopen(text_in, "r")) == NULL) {
      fprintf(stderr, "Couldn't open %s for read, %s\n", text_in,
	      strerror(errno));
      exit(-1);
    }
    if (dbmfile_init(mudconf.dbm_file, TEENY_DBMFAST) != 0) {
      fprintf(stderr, "Couldn't open dbm file %s.\n", mudconf.dbm_file);
      exit(-1);
    }

    fputs("Looking at your database...\n", stderr);

    if(text_version(text, &read_version, &read_flags, &read_total) == -1){
      fputs("Aborting.\n", stderr);
      exit(-1);
    }
    if(read_version >= 0) {	/* TeenyMUD */
      if(read_version == 0) {	/* might really be version 5 */
        if (!dash_c) {
          fputs("Hmmm. Your database appears to be a very old format.\n",
	  	stderr);
          fputs("Was it created by the old TeenyMUD 1.1-C server? [n] ",
	  	stderr);
          fflush(stderr);
          if(fgets(buffer, BUFFSIZ, stdin) == (char *)NULL) {
	    fputs("Aborting.\n", stderr);
	    exit(-1);
          }
          if((buffer[0] == 'y') || (buffer[0] == 'Y'))
	    read_version = 5;
        } else
          read_version = 5;
      } else if((read_version < 5) && (read_version > 0)){/* might be 1.3 */
        if (!one_three) {
          fputs("Hmmm. Your database appears to be an old format,\n", stderr);
          fputs("and I can't tell if it was created by TeenyMUD 1.2,\n",
	        stderr);
          fputs("or TeenyMUD 1.3.  Which one is it? [1.3] ", stderr);
          fflush(stderr);
          if(fgets(buffer, BUFFSIZ, stdin) == (char *)NULL) {
	    fputs("Aborting.\n", stderr);
	    exit(-1);
          }
          if(!strchr(buffer, '2')){
	    read_flags |= DB_SITE;
	    read_flags |= DB_ARRIVE;
          }
        } else {
          read_flags |= DB_SITE;
	  read_flags |= DB_ARRIVE;
        }
      }
      if((read_version > 9) && (read_version < 20)) {	/* 1.3 */
        read_version -= 10;
        read_flags |= DB_SITE;
        read_flags |= DB_ARRIVE;
      }

      if((read_version < 5) && !(read_flags & DB_SITE)) {
        fputs("Hmm, looks like a pre-1.3 TeenyMUD database.\n", stderr);
      } else if((read_version < 5) && (read_flags & DB_SITE)) {
        fputs("Hmm, looks like a TeenyMUD 1.3 database.\n", stderr);
      } else if(read_version == 5) {
        fputs("Hmm, looks like a TeenyMUD 1.1-C database.\n", stderr);
      } else if(read_version < 200) {
        fputs("Hmm, looks like a TeenyMUD 1.4 or TeenyMUSK database.\n",
	      stderr);
      } else if(read_version < 300) {
        fputs("Hmm, looks like a TeenyMUD 2.0a database.\n", stderr);
      } else {
        fputs("Hmm, looks like a TeenyMUD 2.0 database.\n", stderr);
      }
    } else {		/* Something else. */
      if(read_version == -1)
        fputs("Hmm, looks like a TinyMUD database.  Buh.\n", stderr);
    }

#if SIZEOF_TIME_T < 8
    /* Complain, but otherwise continue. */
    if (read_flags & DB_LARGETIME) {
      fputs("WARNING: Loading a large time_t database on a small time_t architecture!\n", stderr);
    }
#endif

    fputs("Loading text database", stderr);
    fflush(stderr);
    if(text_load(text, read_version, read_flags, read_total) == -1) {
      fputs("Load of text database failed. Aborting.\n", stderr);
      exit(-1);
    }
    fclose(text);
    fprintf(stderr, "Loaded text database version %d.", read_version);
    if (read_flags & DB_SITE)
      fputs("  [SITE]", stderr);
    if (read_flags & DB_ARRIVE)
      fputs("  [ARRIVE]", stderr);
    if (read_flags & DB_DESTARRAY)
      fputs("  [DESTARRAY]", stderr);
    if (read_flags & DB_PARENTS)
      fputs("  [PARENTS]", stderr);
    if (read_flags & DB_IMMUTATTRS)
      fputs("  [IMMUTATTRS]", stderr);
    if (read_flags & DB_LARGETIME)
      fputs("  [LARGETIME]", stderr);
    fputc('\n', stderr);

    if (read_version < 300)
      upgrade_database(read_version, read_flags, use_existing);
    break;
  case TEXT_WRITE:
    if ((text = fopen(text_out, "w")) == NULL) {
      fprintf(stderr, "Couldn't open %s for write.\n", text_out);
      exit(-1);
    }
    fputs("Writing text database", stderr);
    fflush(stderr);
    if (dbmfile_open(mudconf.dbm_file, TEENY_DBMFAST|TEENY_DBMREAD) != 0) {
      fprintf(stderr, "Couldn't open dbm file %s\n", mudconf.dbm_file);
      exit(-1);
    }
    if (database_read(mudconf.db_file) != 0) {
      fprintf(stderr, "Couldn't read database file %s\n", mudconf.db_file);
      exit(-1);
    }
    ptable_init();
    text_dump(text, OUTPUT_VERSION, output_flags);
    fclose(text);
    break;
  default:
    fprintf(stderr, "%s: You must specify one of either -i or -o.\n", progname);
    fprintf(stderr, "%s: Use '%s --help' for a complete list of options.\n",
    	    progname, progname);
    exit(0);
  }
  cache_flush();
  dbmfile_close();
  if (database_write(mudconf.db_file) == -1)
    fputs("Error writing database file.\n", stderr);

  return (0);
}

static void upgrade_database(read_version, read_flags, use_existing)
    int read_version, read_flags, use_existing;
{
  int i;
  int default_loc;
  int newloc = 0;

  default_loc = mudconf.root_location;
  if ((read_version < 100) && !use_existing) {
    char ibuff[16];

    fputs("Create a new room to be the root? [Ny] ", stderr);
    fflush(stderr);
    
    if(fgets(ibuff, sizeof(ibuff), stdin) != (char *)NULL) {
      if((ibuff[0] == 'y') || (ibuff[0] == 'Y')) {
        default_loc = create_obj(TYP_ROOM);
	if(default_loc == -1) {
	  fprintf(stderr, "Couldn't create root room.  Aborting!\n");
	  return;
	}
	if((set_str_elt(default_loc, NAME, "Root Room") == -1)
	   || (set_int_elt(default_loc, OWNER, mudconf.player_god) == -1)) {
	  fprintf(stderr, "Couldn't set root room name or owner.  Aborting!\n");
	  return;
	}
	stamp(default_loc, STAMP_MODIFIED);
	newloc++;

	fprintf(stderr, "Created a root room with object #%d.\n",
		default_loc);
	fputs("(Be sure to put this object number in your configuration.)\n",
	      stderr);
      }
    }

    if((read_version < 100) && !newloc) {
      if (!exists_object(default_loc) || !isROOM(default_loc)) {
        fprintf(stderr,
		"Illegal default room location (#%d), reseting to #0.\n",
	        default_loc);
        default_loc = 0;
      }
      if (!exists_object(default_loc) || !isROOM(default_loc)) {
        fprintf(stderr, "Illegal default room location (#%d). Fatal error.\n",
	        default_loc);
        exit(-1);
      }
      fprintf(stderr, "Using object #%d as the root room.\n", default_loc);
    }

    fputs("Upgrading the database...", stderr);
    fflush(stderr);

    if(read_version < 0) {		/* TinyMUD. */
      /* Figure out exit locations...   Augh. */
      int list, next;

      for (i = 0; i < mudstat.total_objects; i++) {
        if ((main_index[i] == (struct dsc *)NULL)
	    || (DSC_TYPE(main_index[i]) == TYP_EXIT))
	  continue;
	
	/* Check the contents list. */
	if(get_int_elt(i, CONTENTS, &list) == -1) {
	  fprintf(stderr, "Database error on #%d.\n", i);
	  return;
	}
	while(list != -1) {
	  if(_exists_object(list)) {
	    next = DSC_NEXT(main_index[list]);
	    if(Typeof(list) == TYP_EXIT) {
	      list_drop(list, i, CONTENTS);
	      list_add(list, i, EXITS);
	    }
	    list = next;
	  } else {
	    fprintf(stderr, "Database error on #%d.\n", i);
	    return;
	  }
	}

	/* Check the exits list. */
	if(get_int_elt(i, EXITS, &list) == -1) {
	  fprintf(stderr, "Database error on #%d.\n", i);
	  return;
	}
	while(list != -1) {
	  if(_exists_object(list)) {
	    if(set_int_elt(list, LOC, i) == -1) {
	      fprintf(stderr, "Database error on #%d.\n", list);
	      return;
	    }
	    list = DSC_NEXT(main_index[list]);
	  } else {
	    fprintf(stderr, "Database error on #%d.\n", i);
	    return;
	  }
	}
      }
    }

    if (read_version < 100) {
      for (i = 0; i < mudstat.total_objects; i++) {
        if ((main_index[i] == (struct dsc *)NULL)
	    || (DSC_TYPE(main_index[i]) != TYP_ROOM))
	  continue;

        DSC_NEXT(main_index[i]) = -1;
	list_add(i, default_loc, ROOMS);
	if(set_int_elt(i, LOC, default_loc) == -1) {
	  fprintf(stderr, "Database error on #%d.\n", i);
	  return;
	}
      }
    }

    fputs("Done.\n", stderr);
    fflush(stderr);
  } else if((read_version == 200) || (read_version < 0)) {
    int loc;

    fprintf(stderr, "Recycling %s...",
    	    ((read_version == 200) ? "programs" : "garbage"));
    fflush(stderr);

    for (i = 0; i < mudstat.total_objects; i++) {
      if (main_index[i] == NULL)
	continue;
      switch(DSC_TYPE(main_index[i])){
      case TYP_PLAYER:
      case TYP_EXIT:
      case TYP_THING:
      case TYP_ROOM:
        break;
      default:
        if(get_int_elt(i, LOC, &loc) != -1) {
	  if(_exists_object(loc))
	    list_drop(i, loc, CONTENTS);
	  destroy_obj(i);
	}
      }
    }

    fputs("Done.\n", stderr);
  }
}

static void create_startup()
{
  int room, wiz;

  if (dbmfile_init(mudconf.dbm_file, 0) != 0) {
    fprintf(stderr, "%s: Couldn't upen dbm file %s\n", progname,
    	    mudconf.dbm_file);
    exit(0);
  }
  initialize_db(16);
  mudstat.slack = 16;

  room = create_obj(TYP_ROOM);
  wiz = create_obj(TYP_PLAYER);

  if ((set_str_elt(room, NAME, "Limbo") == -1)
      || (set_str_elt(wiz, NAME, "Wizard") == -1)
      || (attr_add(wiz, PASSWORD, cryptpwd("potrzebie"),
		   PASSWORD_FLGS) == -1)
      || (set_int_elt(wiz, NEXT, -1) == -1)
      || (set_int_elt(room, NEXT, -1) == -1)
      || (set_int_elt(wiz, LOC, room) == -1)
      || (set_int_elt(room, LOC, room) == -1)
      || (set_int_elt(room, CONTENTS, wiz) == -1)
      || (set_int_elt(wiz, HOME, room) == -1)
      || (set_int_elt(room, OWNER, wiz) == -1)
      || (set_int_elt(wiz, OWNER, wiz) == -1)) {
    fprintf(stderr, "%s: Something bad happened.\n", progname);
    exit(0);
  }
  stamp(room, STAMP_MODIFIED);
  stamp(wiz, STAMP_MODIFIED);

  cache_flush();
  dbmfile_close();
  if (database_write(mudconf.db_file) == -1)
    fprintf(stderr, "%s: Error writing the database.\n", progname);
  else
    fprintf(stderr, "Database initialized.\n");
  exit(0);
}
