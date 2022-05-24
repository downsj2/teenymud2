/* textdb.c */

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
#include <netinet/in.h>
#ifdef HAVE_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif				/* HAVE_STRING_H */
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif				/* HAVE_STDLIB_H */
#include <ctype.h>

#include "conf.h"
#include "teeny.h"
#include "teenydb.h"
#include "textdb.h"
#include "externs.h"

char txt_buffer[TXTBUFFSIZ];

char txt_eoferr[] = "Unexpected EOF near object #%d.\n";
char txt_numerr[] = "Failed to get integer from string near object #%d.\n";
char txt_dberr[] = "Internal database error at object #%d.\n";
char txt_boolerr[] = "Boolexp format error at object #%d.\n";

int text_version(in, read_version, read_flags, read_total)
    FILE *in;
    int *read_version, *read_flags, *read_total;
{
  int ch;
  char *ptr, *optr;
  int done = 0;

  while(!done) {
    ch = fgetc(in);

    /* parse out version number and such */
    switch(ch) {
    case '!':		/* Comments. */
      if(fgets(txt_buffer, sizeof(txt_buffer), in) == (char *)NULL) {
        fputs("Read failed in header.\n", stderr);
	return(-1);
      }
      break;
    case '%':		/* original TeenyMUD */
      if(fgets(txt_buffer, sizeof(txt_buffer), in) == (char *)NULL) {
        fputs("Read failed in header.\n", stderr);
	return(-1);
      }
      *read_total = (int)strtol(txt_buffer, &ptr, 10);
      if(ptr == txt_buffer){
        fputs("Failed to get object count from old style database.\n", stderr);
        return(-1);
      }
      if((ch = fgetc(in)) == '~'){	/* 1.2-1.3 version */
        if(fgets(txt_buffer, sizeof(txt_buffer), in) == (char *)NULL){
	  fputs("Read failed while trying to get database version number.\n",
		stderr);
	  return(-1);
        }
        *read_version = (int)strtol(txt_buffer, &ptr, 10);
        if(ptr == txt_buffer){
	  fputs("Failed to get version number from old style database.\n",
		stderr);
	  return(-1);
        }
      } else {				/* 1.0-1.1 */
        ungetc(ch, in);
        *read_version = 0;
      }
      *read_flags = 0;			/* no flags */

      done++;
      break;
    case '&':				/* 1.4 */
    case '+':				/* 2.0a */
      if(fgets(txt_buffer, sizeof(txt_buffer), in) == (char *)NULL) {
        fputs("Read failed in header.\n", stderr);
	return(-1);
      }
      *read_total = (int)strtol(txt_buffer, &optr, 10);
      if(optr == txt_buffer){
        fputs("Failed to get object count from old attribute database.\n",
	      stderr);
        return(-1);
      }
      *read_version = (int)strtol(optr, &ptr, 10);
      if(ptr == optr) {
        fputs("Failed to get version number from old attribute database.\n",
	      stderr);
        return(-1);
      }
      *read_flags = 0;			/* no flags */

      done++;
      break;
    case '$':				/* 1.5 */
      if(fgets(txt_buffer, sizeof(txt_buffer), in) == (char *)NULL) {
        fputs("Read failed in header.\n", stderr);
	return(-1);
      }
      *read_version = (int)strtol(txt_buffer, &optr, 10);
      if(optr == txt_buffer){
        fputs("Failed to get version number from new style database.\n",
	      stderr);
        return(-1);
      }
      *read_flags = (int)strtol(optr, &ptr, 10);
      if(ptr == optr){
        fputs("Failed to get database flags from new style database.\n",
	      stderr);
        return(-1);
      }
      *read_total = (int)strtol(ptr, &optr, 10);
      if(optr == ptr){
        fputs("Failed to get object count from new style database.\n", stderr);
        return(-1);
      }

      done++;
      break;
    case '#':	/* TinyMUD */
      *read_version = -1;
      *read_flags = 0;
      *read_total = -1;

      ungetc(ch, in);

      done++;
      break;
    default:
      fputs("Couldn't figure out database header.  You loose.\n", stderr);
      return(-1);
    }
  }
  return(0);
}

int skip_line(in, obj)
    FILE *in;
    int obj;
{
  if(fgets(txt_buffer, sizeof(txt_buffer), in) == (char *)NULL){
    fprintf(stderr, txt_eoferr, obj);
    return(-1);
  }
  return(0);
}
