/* conn.h */

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


#ifndef __CONN_H
#define __CONN_H

/* a typical connection. */
struct conn {
  short flags;			/* connection status flags */
#define CONN_ERROR	0x01	/* something bad has happened- kill it */
#define CONN_KILL	0x02	/* flush it, then kill it */

  int fd;			/* file descriptor */

  int player;			/* player number, if logged in */

  int port;			/* remote port number */

  short lwrap;			/* conn. screen width if wrap enabled */
  short lcrnl;			/* conn. screen cr/nl mode */
#define CONN_CRNL	0
#define CONN_CR		1
#define CONN_NL		2
#define CONN_NLCR	3
  short html_ext;		/* type of HTML extension */
#ifndef HTML_NONE
#define HTML_NONE	0
#endif
#ifndef HTML_PUEBLO
#define HTML_PUEBLO	1
#endif

  time_t connect;		/* when first connected */
  time_t last;			/* last input */

  int cc;			/* input command count this second */
  int bcc;			/* number of times blown command count */
  int ibyte;			/* input byte count, total */
  int obyte;			/* output byte count, total */

  VOID (*handler)();		/* input handler for this connection */

  char *host;			/* remote host name */
  char *user;			/* remote user name */
  char *doing;			/* for the who list */
  char *outputprefix;
  char *outputsuffix;

  struct conn *prev;
  struct conn *next;

#ifdef __TCPIP_C
  /* buffers come last! */
  struct buffer *ibuff;
  struct buffer *obuff;
#endif			/* __TCPIP_C */
};

extern struct conn *connlist;

/* from interface.c */
extern const char *const stty_nlmodekey[];
extern const int stty_nlmodeval[];
extern const char *const stty_nlmodes[];

/* This must always be smaller than medbuffsiz! */
#define NETINPUT_BUFFSIZ	MEDBUFFSIZ/3

/* prototypes. */
extern void conn_close _ANSI_ARGS_((struct conn *));
extern int conn_put _ANSI_ARGS_((struct conn *, char *));
extern int conn_put_raw _ANSI_ARGS_((struct conn *, char *));
extern int conn_logoff _ANSI_ARGS_((struct conn *));

extern void rwho_login _ANSI_ARGS_((int));
extern void rwho_ping _ANSI_ARGS_((void));

extern VOID handle_input _ANSI_ARGS_((struct conn *, char *));
extern VOID handle_login _ANSI_ARGS_((struct conn *, char *));
extern void conn_greet _ANSI_ARGS_((struct conn *));
extern int conn_dump_file _ANSI_ARGS_((struct conn *, char *));
extern void log_disconnect _ANSI_ARGS_((struct conn *));
extern void log_connect _ANSI_ARGS_((struct conn *));
extern void stty_restore _ANSI_ARGS_((struct conn *));

#endif			/* __CONN_H */
