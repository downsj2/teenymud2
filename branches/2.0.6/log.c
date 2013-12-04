/* log.c */

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
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#include <time.h>
/* Too many people have broken varargs headers.  Hi, HP and IBM. */
#if (!defined(HAVE_STDARG_H) && defined(HAVE_VARARGS_H)) || !defined(__STDC__)
#include <varargs.h>
#endif			/* !HAVE_STDARG_H */
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif			/* HAVE_STDLIB_H */

#include "conf.h"
#include "teeny.h"
#include "externs.h"

/* In-server logging routines. */

static char tformat[] = "%a %b %e %H:%M";
static char tbuf[26];

static FILE *lfp[4] = { NULL, NULL, NULL, NULL };

#if defined(HAVE_STDARG_H) && defined(__STDC__)
void logfile(int which, char *format, ...)
#else
void logfile(which, format, va_alist)
    int which;
    char *format;
    va_dcl
#endif			/* HAVE_STDARG_H && __STDC__ */
{
  va_list args;

  strftime(tbuf, sizeof(tbuf), tformat, localtime(&mudstat.now));

#if defined(HAVE_STDARG_H) && defined(__STDC__)
  va_start(args, format);
#else
  va_start(args);
#endif			/* HAVE_STDARG_H && __STDC__ */

  if((which < LOG_STATUS) || (which > LOG_ERROR))
    return;		/* XXX */

  if(format == (char *)NULL) {
    if(lfp[which] != (FILE *)NULL)
      fclose(lfp[which]);
    lfp[which] = (FILE *)NULL;
    return;
  }

  if((lfp[which] == (FILE *)NULL) && (mudstat.status != MUD_COLD)) {
    switch(which) {
    case LOG_STATUS:
      lfp[which] = fopen(mudconf.logstatus_file, "a");
      break;
    case LOG_GRIPE:
      lfp[which] = fopen(mudconf.loggripe_file, "a");
      break;
    case LOG_COMMAND:
      lfp[which] = fopen(mudconf.logcommand_file, "a");
      break;
    case LOG_ERROR:
      lfp[which] = fopen(mudconf.logerror_file, "a");
      break;
    }
  }
  if (lfp[which] == (FILE *)NULL) {
    fprintf(stderr, "[%s/%s] ", mudconf.name, tbuf);
    vfprintf(stderr, format, args);
    fflush(stderr);
  } else {
    fprintf(lfp[which], "[%s/%s] ", mudconf.name, tbuf);
    vfprintf(lfp[which], format, args);
    fflush(lfp[which]);
  }
  va_end(args);
}

#if defined(HAVE_STDARG_H) && defined(__STDC__)
void panic(char *format, ...)
#else
void panic(format, va_alist)
    char *format;
    va_dcl
#endif			/* HAVE_STDARG_H && __STDC__ */
{
  FILE *fp = (FILE *)NULL;
  va_list args;

  strftime(tbuf, sizeof(tbuf), tformat, localtime(&mudstat.now));

#if defined(HAVE_STDARG_H) && defined(__STDC__)
  va_start(args, format);
#else
  va_start(args);
#endif			/* HAVE_STDARG_H && __STDC__ */

  if(((mudconf.logpanic_file)[0] != '-') && (mudstat.status != MUD_COLD))
    fp = fopen(mudconf.logpanic_file, "a");
  if (fp == (FILE *) NULL) {
    fprintf(stderr, "[%s/%s] ", mudconf.name, tbuf);
    vfprintf(stderr, format, args);
    fflush(stderr);
  } else {
    fprintf(fp, "[%s/%s] ", mudconf.name, tbuf);
    vfprintf(fp, format, args);
    fclose(fp);
  }
  va_end(args);
  exit(1);
}
