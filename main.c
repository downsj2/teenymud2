/* main.c */

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
#ifdef HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif			/* HAVE_SYS_WAIT_H */
#include <sys/ioctl.h>
#include <sys/file.h>
#include <fcntl.h>
#include <ctype.h>
#include <signal.h>
#ifdef HAVE_SETRLIMIT
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif			/* HAVE_SYS_TIME_H */
#include <sys/resource.h>
#endif				/* HAVE_SETRLIMIT */
#if !defined(HAVE_SETRLIMIT) || !defined(HAVE_SYS_TIME_H) || \
	defined(TIME_WITH_SYS_TIME)
#include <time.h>
#endif
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif			/* HAVE_STDLIB_H */
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif			/* HAVE_UNISTD_H */

#ifndef SIGUSR1
/*
 * Sigh.  I tried building on an ancient 4.2BSD system that lacks a SIGUSR1
 * define.  It appears to support the full 32 signals, though, and the last
 * few appear to be free to use.  This might break on other systems, but
 * I hope to God there aren't any others that don't have SIGUSR1!
 */
#ifndef NSIG
 #error You need to figure out how to replace SIGUSR1.  Sorry.
#else
#define SIGUSR1	(NSIG-1)		/* XXX */
#endif
#endif			/* no SIGUSR1 */

#include "conf.h"
#include "teeny.h"
#include "ptable.h"
#include "sha/sha_wrap.h"
#include "gnu/getopt.h"
#include "externs.h"

static void print_version _ANSI_ARGS_((void));
static void print_help _ANSI_ARGS_((void));
static void ty_detach _ANSI_ARGS_((void));
#if defined(HAVE_SETRLIMIT) && defined(RLIMIT_NOFILE)
static void rlimit_init _ANSI_ARGS_((void));
#endif				/* HAVE_RLIMIT */
static void signals_init _ANSI_ARGS_((void));

extern int errno;

static int dump_pid;

static char *progname;

static struct option const getopt_options[] =
{
  { "detach", no_argument, 0, 'D' },
  { "configuration", required_argument, 0, 'c' },
  { "force", no_argument, 0, 'f' },
  { "help", no_argument, 0, 'h' },
  { "version", no_argument, 0, 'v' },
  { NULL, 0, 0, '\0' }
};

extern const char teenymud_version[];

static void print_version()
{
  fprintf(stderr, "This is TeenyMUD version %s.\n", teenymud_version);
}

static void print_help()
{
  print_version();

  fputs("Choose from the following:\n", stderr);
  fputs("-D, --detach\t\tdetach server from controlling terminal\n", stderr);
  fputs("-c, --configuration\tspecify configuration file\n", stderr);
  fputs("-f, --force\t\tforce boot\n", stderr);
  fputs("-h, --help\t\tdisplay this information\n", stderr);
  fputs("-v, --version\t\tdisplay program version information\n", stderr);
}

int main(argc, argv)
    int argc;
    char **argv;
{
  char *config = (char *)NULL;
  int detach = 0;
  int force = 0;
  int ch;

  progname = ty_basename(argv[0]);

  opterr = 0;
  while ((ch = getopt_long(argc, argv, "c:fDhv", getopt_options,
  			   (int *) 0)) != -1) {
    switch (ch) {
    case 'h':
      print_help();
      exit(0);
    case 'v':
      print_version();
      exit(0);
    case 'D':			/* detach */
      detach++;
      break;
    case 'c':			/* config file */
      config = optarg;
      break;
    case 'f':			/* force a boot */
      force++;
      break;
    default:
      fprintf(stderr, "%s: bad option -- %c\n", progname, ch);
      fprintf(stderr, "%s: Use '%s --help' for a complete list of options.\n",
      	      progname, progname);
      exit(0);
    }
  }
  if (config == (char *)NULL) {
    fprintf(stderr, "%s: You must specify a configuration file.\n", progname);
    fprintf(stderr, "%s: Use '%s --help' for a complete list of options.\n",
    	    progname, progname);
    exit(0);
  }

  mudstat.status = MUD_COLD;
  time(&mudstat.now);

  /* this prints it's own error messages. */
  if((conf_init(config) == -1) && !force)
    exit(-1);

  /* first things first. */
  if(chdir(mudconf.chdir_path) != 0) {
    if(!force) {
      fprintf(stderr, "%s: chdir [%s] failed: %s\n", progname, 
	      mudconf.chdir_path, strerror(errno));
      exit(0);
    } else
      logfile(LOG_ERROR, "chdir() failed: %s, continuing...\n",
	      strerror(errno));
  }

  /* do various initializations */
#if defined(HAVE_SETRLIMIT) && defined(RLIMIT_NOFILE)
  rlimit_init();
#endif				/* HAVE_RLIMIT */
  ty_malloc_init();
  signals_init();
  cache_init();
  interface_init();
  help_init();
  prims_init();
  srandom((int) getpid());
#ifdef HAVE_TZSET
  tzset();
#endif				/* HAVE_TZSET */

  if (dbmfile_open(mudconf.dbm_file, 0) != 0)
    panic("main: couldn't open dbm file %s\n", mudconf.dbm_file);
  if (database_read(mudconf.db_file) != 0)
    panic("main: couldn't open database file %s\n", mudconf.db_file);
  ptable_init();

  /* let the world know we live! */
  fprintf(stdout, "TeenyMUD %s is Copyright(C) 1993, 1994, 1995 Jason Downs.\n",
  	  teenymud_version);
  fputs("TeenyMUD is free software and comes with absolutely no warranty.\n",
  	stdout);

  if(detach)
    ty_detach();

  fflush(stdout);

  /* Set out state, and enable logins. */
  mudstat.status = MUD_RUNNING;
  mudstat.logins = LOGINS_ENABLED;

  /* things to do before actually starting the server... */
  handle_startup();		/* queue up STARTUP attributes */

  /* run the server */
  tcp_loop();			/* this is what actually does it */

  mud_shutdown(0);
  return (0);
}

static void ty_detach()
{
  int pid;
#ifndef HAVE_SETSID
  int fd;
#endif			/* HAVE_SETSID */

  if ((pid = fork()) == -1)
    logfile(LOG_ERROR, "detach: couldn't fork() to detach the server.\n");
  else if (pid != 0) {
    printf("[Detaching process %d.]\n", pid);
    exit(0);
  }

  close(0);
  close(1);
#ifdef HAVE_SETSID
  if (setsid() == -1)
    panic("detach: setsid() failed, %s\n", strerror(errno));
#else
  if ((fd = open("/dev/tty", O_RDWR, 0600)) == -1)
    panic("detach: can't open /dev/tty, %s\n", strerror(errno));
  if (ioctl(fd, TIOCNOTTY, 0) == -1)
    panic("detach: ioctl() failed, %s\n", strerror(errno));
  close(fd);

#if defined(aiws)
  setpgrp();
#endif
#endif                          /* HAVE_SETSID */
}

void mud_shutdown(status)
    int status;
{
  char buff[128];
  FILE *fd;
  int wflags[FLAGS_LEN];

  /* just for you, xibo */
  if ((fd = fopen(mudconf.shtd_file, "r")) != (FILE *) NULL) {
    bzero((VOID *)wflags, sizeof(wflags));
    while (fgets(buff, 128, fd) != (char *) NULL) {
      remove_newline(buff);
      tcp_wall(wflags, buff, -1);
    }
    fclose(fd);
  }
  tcp_shutdown();

  /* Wait for any dumps in progress to terminate */
  waitpid(dump_pid, NULL, 0);

  /* Dump the DB */

  cache_flush();
  dbmfile_close();
  if (database_write(mudconf.db_file) == -1)
    logfile(LOG_ERROR, "mud_shutdown: final database dump failed.\n");

  logfile(LOG_STATUS, "%s done.\n", mudconf.name);

  exit(status);
}

/* Resets the MUD status when a dump completes. */
static RETSIGTYPE dump_finished(dummy)
    int dummy;
{
  waitpid(dump_pid, NULL, 0);
  cache_unlock();
  mudstat.status = MUD_RUNNING;

#ifdef HAVE_SYSV_SIGNALS
  signals_init();
#endif				/* HAVE_SYSV_SIGNALS */
}

/* Dumps the database. Amazing, huh? */
void dump_db()
{
  static char *backdb = (char *)NULL;
#if defined(USE_BSDDBM) || defined(USE_GDBM)
  static char *backdbm = (char *)NULL;
#endif			/* USE_BSDDBM || USE_GDBM */
#ifdef USE_NDBM
  static char *dirname = (char *)NULL;
  static char *pagname = (char *)NULL;
  static char *backdir = (char *)NULL;
  static char *backpag = (char *)NULL;

  /* setup the dir and page file names */
  if (dirname == (char *)NULL) {
    dirname = (char *)ty_malloc(strlen(mudconf.dbm_file) + 5,
    				"dump_db.dirname");
    strcpy(dirname, mudconf.dbm_file);
    strcat(dirname, ".dir");
  }
  if (pagname == (char *)NULL) {
    pagname = (char *)ty_malloc(strlen(mudconf.dbm_file) + 5,
    				"dump_db.pagname");
    strcpy(pagname, mudconf.dbm_file);
    strcat(pagname, ".pag");
  }
#endif			/* USE_NDBM */

  /* set up the backup file names */
  if (backdb == (char *)NULL) {
    backdb = (char *)ty_malloc(strlen(mudconf.db_file) + 5, "dump_db.backdb");
    strcpy(backdb, mudconf.db_file);
    strcat(backdb, ".BAK");
  }
#if defined(USE_BSDDBM) || defined(USE_GDBM)
  if (backdbm == (char *)NULL) {
    backdbm = (char *)ty_malloc(strlen(mudconf.dbm_file) + 5,
    				"dump_db.backdbm");
    strcpy(backdbm, mudconf.dbm_file);
    strcat(backdbm, ".BAK");
  }
#endif			/* USE_BSDDBM || USE_GDBM */
#ifdef USE_NDBM
  if (backdir == (char *)NULL) {
    backdir = (char *)ty_malloc(strlen(dirname) + 5, "dump_db.backdir");
    strcpy(backdir, dirname);
    strcat(backdir, ".BAK");
  }
  if (backpag == (char *)NULL) {
    backpag = (char *)ty_malloc(strlen(pagname) + 5, "dump_db.backpag");
    strcpy(backpag, pagname);
    strcat(backpag, ".BAK");
  }
#endif			/* USE_NDBM */

  /* We damned well *better* not have any dumps still in progress. */
  /* This should wait until the last dump is done, on the off  */
  /* chance that there is one still in progress. */

  waitpid(dump_pid, NULL, 0);

  cache_flush();
  mudstat.status = MUD_DUMPING;
  cache_lock();

  if ((dump_pid = fork()) == 0) {	/* We're in the child */
    /* write out the descriptor file. */
    if (database_write(mudconf.db_file) == -1)
      logfile(LOG_ERROR,
	      "dump_db: database_write() failed, can't write db file.\n");
    /* back it up */
    copy_file(mudconf.db_file, backdb);

    /* back up the rest of the database */
#if defined(USE_BSDDBM) || defined(USE_GDBM)
    copy_file(mudconf.dbm_file, backdbm);
#endif			/* USE_BSDDBM || USE_GDBM */
#ifdef USE_NDBM
    copy_file(dirname, backdir);
    copy_file(pagname, backpag);
#endif			/* USE_NDBM */

    /* We're done. Signal the parent */

    kill(getppid(), SIGUSR1);
    _exit(0);

  } else if (dump_pid == -1) {	/* The fork() failed! */
    logfile(LOG_ERROR, "dump_db: could not fork(), %s\n", strerror(errno));
  }
}

static RETSIGTYPE signal_shutdown(dummy)
    int dummy;
{
  logfile(LOG_STATUS, "Caught a shutdown signal... Closing down.\n");
  mud_shutdown(0);
}

static RETSIGTYPE signal_dump(dummy)
    int dummy;
{
  if(mudstat.status != MUD_DUMPING) {
    logfile(LOG_STATUS, "Caught a dump signal... Dumping the database.\n");
    dump_db();
  } else
    logfile(LOG_STATUS, "Caught a dump signal, but already dumping.\n");
}

#ifndef NSIG
#define NSIG	32		/* XXX */
#endif

static void signals_init()
{
  register int snum;

  /* Reset all signals to ignore. */
  for(snum = 1; snum < NSIG; snum++)
      signal(snum, SIG_IGN);
  
  /* Set proper defaults for the rest. */
#ifdef SIGSEGV
  signal(SIGSEGV, SIG_DFL);
#endif
#ifdef SIGBUS
  signal(SIGBUS, SIG_DFL);
#endif
#ifdef SIGQUIT
  signal(SIGQUIT, SIG_DFL);
#endif
#ifdef SIGINT
  signal(SIGINT, signal_dump);
#endif
#ifdef SIGTERM
  signal(SIGTERM, signal_shutdown);
#endif
#ifdef SIGDANGER
  signal(SIGDANGER, signal_shutdown);
#endif
#ifdef SIGPWR
  signal(SIGPWR, signal_shutdown);
#endif
#ifdef SIGXFSZ
  signal(SIGXFSZ, signal_shutdown);
#endif
#ifdef SIGXCPU
  signal(SIGXCPU, signal_shutdown);
#endif
#ifdef SIGLOST
  signal(SIGLOST, signal_shutdown);
#endif

  /* SIGUSR1 and SIGUSR2 are required. */
  signal(SIGUSR1, dump_finished);
}

#if defined(HAVE_SETRLIMIT) && defined(RLIMIT_NOFILE)
static void rlimit_init()
{
  struct rlimit rlp;

  if (getrlimit(RLIMIT_NOFILE, &rlp) == 0) {
    rlp.rlim_cur = rlp.rlim_max;
    if (setrlimit(RLIMIT_NOFILE, &rlp) != 0)
      logfile(LOG_ERROR, "rlimit_init: setrlimit failed, %s\n",
	      strerror(errno));
  } else
    logfile(LOG_ERROR, "rlimit_init: getrlimit failed, %s\n", strerror(errno));
}
#endif				/* HAVE_RLIMIT */
