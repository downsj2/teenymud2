/* queue.c */

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
#include <time.h>
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif			/* HAVE_STDLIB_H */
#ifdef HAVE_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif			/* HAVE_STRING_H */
#include <ctype.h>

#include "conf.h"
#include "teeny.h"
#include "commands.h"
#include "externs.h"

/* The TeenyMUD command queue system, inspired by MUSH. */

/* command queue structure. */
struct cque {
  int player;		/* object running command. */
  int cause;		/* object causing the command. */
  int flags;		/* special operators. */

  time_t when;		/* when to run command. */
  int obj;		/* object to use as a semaphore. */

  char *cmd;		/* what to run. */

  int eargc;		/* exec argument count. */
  char **eargv;		/* exec arguments. */

  struct cque *next;
  struct cque *prev;
};

/* iterative queue structure. */
struct ique {
  int player;		/* object running command. */
  int cause;		/* object causing the command. */
  
  char *var;		/* variable to update. */
  char *val;		/* value of variable. */
  char *cmd;		/* what to run. */

  int eargc;		/* exec argument count. */
  char **eargv;		/* exec arguments. */

  struct ique *next;
  struct ique *prev;
};

/* the queue list */
static struct cque *gqueue = (struct cque *)NULL;
static struct ique *iqueue = (struct ique *)NULL;

/* add something to the iterative queue. */
void queue_iadd(player, cause, var, val, command, eargc, eargv)
    int player, cause;
    char *var, *val, *command;
    int eargc;
    char *eargv[];
{
  struct ique *new;
  int owner, check;

  if(mudconf.enable_money) {
    if((get_int_elt(player, OWNER, &owner) == -1)
       || (get_int_elt(owner, QUEUE, &check) == -1)) {
      logfile(LOG_ERROR, "queue_iadd: bad database reference.\n");
      return;
    }
    if(check >= mudconf.queue_commands) {
      if(!can_afford(owner, cause, mudconf.queue_cost, 1))
	return;
      if(set_int_elt(owner, QUEUE, 0) == -1) {
	logfile(LOG_ERROR, "queue_iadd: couldn't reset queue count on #%d\n",
	        owner);
	return;
      }
    } else {
      if(set_int_elt(owner, QUEUE, ++check) == -1) {
	logfile(LOG_ERROR, "queue_iadd: couldn't reset queue count on #%d\n",
	        owner);
	return;
      }
    }
  }

  /* make the entry. */
  new = (struct ique *)ty_malloc(sizeof(struct ique), "queue_iadd");
  new->player = player;
  new->cause = cause;

  new->var = (char *)ty_strdup(var, "queue_iadd");
  new->val = (char *)ty_strdup(val, "queue_iadd");
  new->cmd = (char *)ty_strdup(command, "queue_iadd");

  new->eargc = eargc;
  new->eargv = eargv;

  new->next = (struct ique *)NULL;

  /* add it to the queue. */
  if(iqueue == (struct ique *)NULL) {
    new->prev = new;
    iqueue = new;
  } else {
    /* add it to the tail. */
    new->prev = iqueue->prev;
    (iqueue->prev)->next = new;
    iqueue->prev = new;
  }
}

/* add something to the command queue. */
void queue_add(player, cause, delay, sema, command, eargc, eargv)
    int player, cause, delay, sema;
    char *command;
    int eargc;
    char *eargv[];
{
  struct cque *new;
  int owner, check;

  if((command == (char *)NULL) || (command[0] == '\0'))
    return;

  if(mudconf.enable_money) {
    if((get_int_elt(player, OWNER, &owner) == -1)
       || (get_int_elt(owner, QUEUE, &check) == -1)) {
      logfile(LOG_ERROR, "queue_add: bad database reference.\n");
      return;
    }
    if(check >= mudconf.queue_commands) {
      if(!can_afford(owner, cause, mudconf.queue_cost, 1))
        return;
      if(set_int_elt(owner, QUEUE, 0) == -1) {
        logfile(LOG_ERROR, "queue_add: couldn't reset queue count on #%d\n",
		owner);
        return;
      }
    } else {
      if(set_int_elt(owner, QUEUE, ++check) == -1) {
        logfile(LOG_ERROR, "queue_add: couldn't reset queue count on #%d\n",
		owner);
	return;
      }
    }
  }

  /* make the entry. */
  new = (struct cque *)ty_malloc(sizeof(struct cque), "queue_add");
  new->player = player;
  new->cause = cause;
  new->flags = 0;

  new->when = (delay >= 0) ? mudstat.now + delay : -1;
  new->obj = sema;

  new->cmd = (char *)ty_strdup(command, "queue_add");

  new->eargc = eargc;
  new->eargv = eargv;

  new->next = (struct cque *)NULL;

  /* add it to the queue */
  if(gqueue == (struct cque *)NULL) {
    new->prev = new;
    gqueue = new;
  } else {
    /* add it to the tail. */
    new->prev = gqueue->prev;
    (gqueue->prev)->next = new;
    gqueue->prev = new;
  }
}

/* do count things off the top of the queue(s). */
void queue_run(count)
    int count;
{
  register struct cque *curr, *next;
  register struct ique *icurr, *inext;
  int sema, owner;

  /* optimization. */
  if((gqueue == (struct cque *)NULL) && (iqueue == (struct ique *)NULL)) {
    return;
  }

  /* command queue. */
  curr = gqueue;
  while(count && (curr != (struct cque *)NULL)) {
    if((curr->when >= 0) && (curr->when <= mudstat.now)) {	/* run it */
      /* if it's went HALT or no longer exists, just drain it. */
      if(!isHALT(curr->player) && exists_object(curr->player))
        handle_cmds(curr->player, curr->cause, curr->cmd, curr->eargc,
		    curr->eargv);

      /* update semaphore count */
      if(curr->obj != -1) {
        if((get_int_elt(curr->obj, SEMAPHORES, &sema) == -1)
	   || (set_int_elt(curr->obj, SEMAPHORES, --sema) == -1)) {
	  logfile(LOG_ERROR,
		  "queue_run: failed to update semaphore count on #%d\n",
	          curr->obj);
	}
      }

      /* remove it. */
      if(curr == gqueue) {
        if(curr->next == (struct cque *)NULL) {	/* only thing here. */
	  gqueue = (struct cque *)NULL;
	  next = (struct cque *)NULL;
	} else {
	  (curr->next)->prev = curr->prev;
	  gqueue = curr->next;
	  next = curr->next;
	}
      } else {
        if(curr->next == (struct cque *)NULL) {	/* last thing here. */
	  (curr->prev)->next = (struct cque *)NULL;
	  gqueue->prev = curr->prev;
	  next = (struct cque *)NULL;
	} else {
          (curr->next)->prev = curr->prev;
	  (curr->prev)->next = curr->next;
	  next = curr->next;
	}
      }
      ty_free((VOID *)curr->cmd);
      free_argv(curr->eargc, curr->eargv);
      ty_free((VOID *)curr->eargv);
      ty_free((VOID *)curr);

      count--;
      curr = next;
    } else
      curr = curr->next;
  }

  /* iterative queue. */
  icurr = iqueue;
  while(count && (icurr != (struct ique *)NULL)) {
    /* if it's went HALT, or no longer exists, just drain it. */
    if(!isHALT(icurr->player) && !exists_object(icurr->player)) {
      /* set the variable. */
      if(icurr->var != (char *)NULL) {
	if(get_int_elt(icurr->player, OWNER, &owner) == -1) {
	  logfile(LOG_ERROR, "queue_run: couldn't get owner of #%d.\n",
	          icurr->player);
	  break;
	}
	if(icurr->val != (char *)NULL) {
	  var_set(owner, icurr->var, icurr->val);
	} else {
	  var_delete(owner, icurr->var);
	}
      }

      /* do the command. */
      if(icurr->cmd != (char *)NULL) {
	handle_cmds(icurr->player, icurr->cause, icurr->cmd, icurr->eargc,
		    icurr->eargv);
      }
    }

    if(icurr == iqueue) {			/* only thing here. */
      if(icurr->next == (struct ique *)NULL) {
	iqueue = (struct ique *)NULL;
	inext = (struct ique *)NULL;
      } else {
	(icurr->next)->prev = icurr->prev;
	iqueue = icurr->next;
	inext = icurr->next;
      }
    } else {
      if(icurr->next == (struct ique *)NULL) {	/* last thing here. */
	(icurr->prev)->next = (struct ique *)NULL;
	iqueue->prev = icurr->prev;
	inext = (struct ique *)NULL;
      } else {
	(icurr->next)->prev = icurr->prev;
	(icurr->prev)->next = icurr->next;
	inext = icurr->next;
      }
    }
    ty_free((VOID *)icurr->var);
    ty_free((VOID *)icurr->val);
    ty_free((VOID *)icurr->cmd);
    free_argv(icurr->eargc, icurr->eargv);
    ty_free((VOID *)icurr->eargv);
    ty_free((VOID *)icurr);

    count--;
    icurr = inext;
  }
}

/* run 'semaphores' off the queue. */
VOID do_notify(player, cause, switches, argone, argtwo)
    int player, cause, switches;
    char *argone, *argtwo;
{
  int obj, count, ocount, ran, sema;
  register struct cque *curr, *next;
  char *ptr;

  if((argone == (char *)NULL) || (argone[0] == '\0')) {
    if(!(switches & CMD_QUIET))
      notify_player(player, cause, player, "Notify what?", NOT_QUIET);
    return;
  }
  if((argtwo == (char *)NULL) || (argtwo[0] == '\0')) {
    count = 1;
    ocount = 1;
  } else {
    count = (int)strtol(argtwo, &ptr, 10);
    if(ptr == argtwo) {
      if(!(switches & CMD_QUIET))
        notify_player(player, cause, player, "I don't understand that count.",
		      NOT_QUIET);
      return;
    }
    if(count == 0) {
      if(!(switches & CMD_QUIET))
        notify_player(player, cause, player, "Nothing to do.", NOT_QUIET);
      return;
    }
    ocount = count;	/* save a copy */
  }

  obj = resolve_object(player, cause, argone,
  		       (!(switches & CMD_QUIET) ? RSLV_EXITS|RSLV_NOISY
		       				: RSLV_EXITS));
  if(obj == -1)
    return;

  curr = gqueue;
  ran = 0;
  while(((switches & NOTIFY_ALL) || count) && (curr != (struct cque *)NULL)) {
    if(curr->obj == obj) {		/* run */
      /* if it's went HALT, or no longer exists, just drain it. */
      if(!(switches & NOTIFY_DRAIN) && !isHALT(curr->player)
	 && exists_object(curr->player)) {
        handle_cmds(curr->player, curr->cause, curr->cmd, curr->eargc,
                    curr->eargv);
	ran++;
      }

      /* remove it. */
      if(curr == gqueue) {
        if(curr->next == (struct cque *)NULL) { /* only thing here. */
          gqueue = (struct cque *)NULL;
          next = (struct cque *)NULL;
        } else {
          curr->next->prev = curr->prev;
          gqueue = curr->next;
          next = curr->next;
        }
      } else {
        if(curr->next == (struct cque *)NULL) { /* last thing here. */
          curr->prev->next = (struct cque *)NULL;
          gqueue->prev = curr->prev;
          next = (struct cque *)NULL;
        } else {
          curr->next->prev = curr->prev;
          curr->prev->next = curr->next;
          next = curr->next;
        }
      }
      ty_free((VOID *)curr->cmd);
      free_argv(curr->eargc, curr->eargv);
      ty_free((VOID *)curr->eargv);
      ty_free((VOID *)curr);

      count--;
      curr = next;
    } else
      curr = curr->next;
  }
  if((ran == 0) && !(switches & NOTIFY_DRAIN) && !(switches & NOTIFY_ALL)) {
    /* fix up the semaphore count, this was a 'future' notify */
    ran = ocount;
  }

  /* update semaphore count */
  if(get_int_elt(obj, SEMAPHORES, &sema) == -1) {
    notify_bad(player);
    return;
  }
  sema = (switches & NOTIFY_DRAIN) ? 0 : sema - ran;
  if(set_int_elt(obj, SEMAPHORES, sema) == -1) {
    notify_bad(player);
    return;
  }

  if(!(switches & CMD_QUIET))
    notify_player(player, cause, player, "Notified.", NOT_QUIET);
}

/* queue related command handlers. */
VOID do_halt(player, cause, switches, arg)
    int player, cause, switches;
    char *arg;
{
  int victim;
  char buff[MEDBUFFSIZ], *name;
  register struct cque *curr, *next;
  register struct ique *icurr, *inext;

  if(switches & HALT_ALL) {	/* expunge the queue(s). */
    if(!(switches & CMD_QUIET)) {
      int wflags[FLAGS_LEN];

      bzero((VOID *)wflags, sizeof(wflags));
      if(get_str_elt(player, NAME, &name) == -1)
        name = "???";
      snprintf(buff, sizeof(buff), "System halted by %s.", name);
      tcp_wall(wflags, buff, -1);
    }

    /* poof */
    for(curr = gqueue; curr != (struct cque *)NULL; curr = next) {
      next = curr->next;
      ty_free((VOID *)curr->cmd);
      free_argv(curr->eargc, curr->eargv);
      ty_free((VOID *)curr->eargv);
      ty_free((VOID *)curr);
    }
    gqueue = (struct cque *)NULL;

    for(icurr = iqueue; icurr != (struct ique *)NULL; icurr = inext) {
      inext = icurr->next;
      ty_free((VOID *)icurr->var);
      ty_free((VOID *)icurr->val);
      ty_free((VOID *)icurr->cmd);
      free_argv(icurr->eargc, icurr->eargv);
      ty_free((VOID *)icurr->eargv);
      ty_free((VOID *)icurr);
    }
    iqueue = (struct ique *)NULL;
  } else {
    /* we just do a flag set, yawn. */
    victim = resolve_object(player, cause, arg,
    			    (!(switches & CMD_QUIET) ? RSLV_NOISY|RSLV_EXITS
			    			     : RSLV_EXITS));
    if(victim == -1)
      return;

    if(!controls(player, cause, victim)) {
      if(!(switches & CMD_QUIET))
        notify_player(player, cause, player, "Permission denied.", NOT_QUIET);
      return;
    }

    if(setHALT(victim) == -1) {
      notify_bad(player);
      return;
    }
  }

  if(!(switches & CMD_QUIET))
    notify_player(player, cause, player, "Halted.", NOT_QUIET);
}

VOID do_semaphore(player, cause, switches, argone, argtwo)
    int player, cause, switches;
    char *argone, *argtwo;
{
  int delay, obj, sema;
  char *ptr, *nptr;

  if((argone == (char *)NULL) || (argone[0] == '\0')
     || (argtwo == (char *)NULL) || (argtwo[0] == '\0')) {
    if(!(switches & CMD_QUIET))
      notify_player(player, cause, player,
		    "You must specify an object and a command.", NOT_QUIET);
    return;
  }

  for(ptr = argone; *ptr && (*ptr != '/'); ptr++);
  if(*ptr == '/') {
    *ptr++ = '\0';

    delay = (int)strtol(ptr, &nptr, 10);
    if(nptr == ptr) {
      if(!(switches & CMD_QUIET))
        notify_player(player, cause, player, 
		      "You must specify delay by number.", NOT_QUIET);
      return;
    }
  } else
    delay = -1;

  obj = resolve_object(player, cause, argone,
  		       (!(switches & CMD_QUIET) ? RSLV_NOISY|RSLV_EXITS
		       				: RSLV_EXITS));
  if(obj == -1)
    return;
  if(!controls(player, cause, obj) && !isLINK_OK(obj)) {
    if(!(switches & CMD_QUIET))
      notify_player(player, cause, player, "Permission denied.", NOT_QUIET);
    return;
  }

  if(get_int_elt(obj, SEMAPHORES, &sema) == -1) {
    notify_bad(player);
    return;
  }
  if(sema < 0) {
    /* fake out the queue */
    queue_add(player, cause, 1, -1, argtwo, 0, (char **)NULL);
  } else {
    queue_add(player, cause, delay, obj, argtwo, 0, (char **)NULL);
  }
  sema++;
  if(set_int_elt(obj, SEMAPHORES, sema) == -1) {
    notify_bad(player);
    return;
  }

  if(!(switches & CMD_QUIET))
    notify_player(player, cause, player, "Queued.", NOT_QUIET);
}

VOID do_wait(player, cause, switches, argone, argtwo)
    int player, cause, switches;
    char *argone, *argtwo;
{
  int obj, owner;
  time_t delay;
  char *ptr;

  if((argone == (char *)NULL) || (argone[0] == '\0')
     || (argtwo == (char *)NULL) || (argtwo[0] == '\0')) {
    if(!(switches & CMD_QUIET))
      notify_player(player, cause, player, 
		    "You must specify a delay and a command.", NOT_QUIET);
    return;
  }

  delay = (int)strtol(argone, &ptr, 10);
  if(!isdigit(argone[0])) {
    if(!(switches & CMD_QUIET))
      notify_player(player, cause, player,
		    "You must specify delay by number.", NOT_QUIET);
    return;
  }
  parse_time(argone, &delay);

  if(*ptr == '/') {	/* optional object. */
    ptr++;
    obj = resolve_object(player, cause, ptr,
    			 (!(switches & CMD_QUIET) ? RSLV_NOISY|RSLV_EXITS
			 			  : RSLV_EXITS));
    if(obj == -1)
      return;
    if(!controls(player, cause, obj)) {
      if(!(switches & CMD_QUIET))
        notify_player(player, cause, player, "Permission denied.", NOT_QUIET);
      return;
    }
    if(!isGOD(player) && isGOD(obj)) {
      if(!(switches & CMD_QUIET))
        notify_player(player, cause, player, "Permission denied.", NOT_QUIET);
      return;
    }
    if(get_int_elt(player, OWNER, &owner) == -1) {
      notify_bad(player);
      return;
    }
    if((obj == owner) && !isINHERIT(player) && !isINHERIT(owner)) {
      if(!(switches & CMD_QUIET))
        notify_player(player, cause, player, "Permission denied.", NOT_QUIET);
      return;
    }

    /* Swab the cause, just like @force. */
    queue_add(obj, obj, (int)delay, -1, argtwo, 0, (char **)NULL);
  } else
    queue_add(player, cause, (int)delay, -1, argtwo, 0, (char **)NULL);

  if(!(switches & CMD_QUIET))
    notify_player(player, cause, player, "Queued.", NOT_QUIET);
}

VOID do_ps(player, cause, switches, arg)
    int player, cause, switches;
    char *arg;
{
  int searchfor, powner, cowner;
  register struct cque *curr;
  char buff[MEDBUFFSIZ];

  if(switches & PS_ALL) {
    searchfor = -1;
  } else {
    if((arg == (char *)NULL) || (arg[0] == '\0')) {
      searchfor = player;
    } else {
      searchfor = resolve_object(player, cause, arg,
      				 (!(switches & CMD_QUIET)
				  ? RSLV_NOISY|RSLV_EXITS : RSLV_EXITS));
      if(searchfor == -1)
        return;
    }
  }

  if((searchfor != -1) && !controls(player, cause, searchfor)) {
    if(!(switches & CMD_QUIET))
      notify_player(player, cause, player, "Permission denied.", NOT_QUIET);
    return;
  }
  if((switches & PS_OWNED)
     && (get_int_elt(searchfor, OWNER, &searchfor) == -1)) {
    notify_bad(player);
    return;
  }

  /* dump the appropiate queue. */
  notify_player(player, cause, player, "Command Queue:", 0);

  if((searchfor != -1) && !(switches & PS_OWNED)) {
    for(curr = gqueue; curr != (struct cque *)NULL; curr = curr->next) {
      if(((switches & PS_CAUSE) && (curr->cause == searchfor))
	 || (!(switches & PS_CAUSE) && (curr->player == searchfor))) {
	snprintf(buff, sizeof(buff), "%s[->#%d]: ",
	         display_name(player, cause, curr->player), curr->cause);
	strncat(buff, curr->cmd, sizeof(buff) - strlen(buff) - 1);
	buff[MEDBUFFSIZ-1] = '\0';
	notify_player(player, cause, player, buff, 0);
      }
    }
  } else {
    for(curr = gqueue; curr != (struct cque *)NULL; curr = curr->next) {
      if((get_int_elt(curr->cause, OWNER, &cowner) == -1)
	 || (get_int_elt(curr->player, OWNER, &powner) == -1))
	continue;

      if((searchfor == -1)
         || ((switches & PS_CAUSE) && (cowner == searchfor))
	 || (!(switches & PS_CAUSE) && (powner == searchfor))) {
	snprintf(buff, sizeof(buff), "%s[->#%d]: ",
		 display_name(player, cause, curr->player), curr->cause);
	strncat(buff, curr->cmd, sizeof(buff) - strlen(buff) - 1);
	buff[MEDBUFFSIZ - 1] = '\0';
	notify_player(player, cause, player, buff, 0);
      }
    }
  }

  notify_player(player, cause, player, "***End of list***", 0);
}

VOID do_force(player, cause, switches, argone, argtwo)
    int player, cause, switches;
    char *argone, *argtwo;
{
  int obj, owner;

  if ((argone == (char *)NULL) || (argone[0] == '\0')
      || (argtwo == (char *)NULL) || (argtwo[0] == '\0')) {
      notify_player(player, cause, player, "Force who to do what?", NOT_QUIET);
    return;
  }
  obj = resolve_object(player, cause, argone,
  		       (!(switches & CMD_QUIET) ? RSLV_NOISY : 0));
  if(obj == -1)
    return;
  if (!controls(player, cause, obj)) {
    if(!(switches & CMD_QUIET))
      notify_player(player, cause, player, "Permission denied.", NOT_QUIET);
    return;
  }
  if (isGOD(obj) && !isGOD(player)) {
    if(!(switches & CMD_QUIET))
      notify_player(player, cause, player,
		    "You can't force God to do anything!", NOT_QUIET);
    return;
  }

  /* You can't force your owner unless you're set INHERIT. */
  if(get_int_elt(player, OWNER, &owner) == -1) {
    notify_bad(player);
    return;
  }
  if((owner == obj) && !isINHERIT(player) && !isINHERIT(owner)) {
    if(!(switches & CMD_QUIET))
      notify_player(player, cause, player, "Permission denied.", NOT_QUIET);
    return;
  }

  if(isROBOT(obj)) {	/* deliver the TinyMUD message only to robots. */
    char *name, buf[MEDBUFFSIZ];

    if(get_str_elt(player, NAME, &name) == -1)
      name = "???";
    snprintf(buf, sizeof(buf), "You sense that %s is forcing you to %s",
	     name, argtwo);
    notify_player(obj, cause, player, buf, NOT_QUIET);
  }

  /* As per MUSH, this command resets the cause. */
  handle_cmds(obj, obj, argtwo, 0, (char **)NULL);

  if(!(switches & CMD_QUIET) && !isQUIET(obj))
    notify_player(player, cause, player, "Ok.", NOT_QUIET);
}

VOID do_trigger(player, cause, switches, argc, argv)
    int player, cause, switches, argc;
    char *argv[];
{
  int obj, aflags, asource, eargc;
  char *atr, **eargv;

  if(argc < 2) {
    if(!(switches & CMD_QUIET))
      notify_player(player, cause, player, "Trigger what?", NOT_QUIET);
    return;
  }

  obj = resolve_object(player, cause, argv[0],
  		       (!(switches & CMD_QUIET) ? RSLV_NOISY|RSLV_EXITS
		       				: RSLV_EXITS));
  if(obj == -1)
    return;
  if(!controls(player, cause, obj)) {
    if(!(switches & CMD_QUIET))
      notify_player(player, cause, player, "Permission denied.", NOT_QUIET);
    return;
  }

  if(attr_get_parent(obj, argv[1], &atr, &aflags, &asource) == -1) {
    notify_bad(player);
    return;
  }
  if(atr == (char *)NULL) {
    if(!(switches & CMD_QUIET))
      notify_player(player, cause, player, "No such attribute.", NOT_QUIET);
    return;
  }

  /* queue it up. */
  argc -= 2;
  argv += 2;
  if(argc > 0) {
    eargv = (char **)ty_malloc(sizeof(char *) * argc, "do_trigger");
    for(eargc = 0; eargc < argc; eargc++) {
      eargv[eargc] = ty_strdup(argv[eargc], "do_trigger");
    }
  } else {
    eargc = 0;
    eargv = (char **)NULL;
  }

  queue_add(obj, player, 1, -1, atr, eargc, eargv);

  if(!(switches & CMD_QUIET))
    notify_player(player, cause, player, "Triggered.", NOT_QUIET);
}

/* iterate over a list. */
VOID do_foreach(player, cause, switches, argone, argtwo)
    int player, cause, switches;
    char *argone, *argtwo;
{
  char *ptr, *var, *lptr, *val;

  if((argone == (char *)NULL) || (argone[0] == '\0')
     || (argtwo == (char *)NULL) || (argtwo[0] == '\0')) {
    if(!(switches & CMD_QUIET))
      notify_player(player, cause, player,
		    "You can't foreach without a list.", NOT_QUIET);
    return;
  }

  for(ptr = argone; (*ptr != '\0') && (*ptr != '/'); ptr++);
  if(*ptr == '/') {
    *ptr++ = '\0';

    var = ptr;
  } else {
    if(!(switches & CMD_QUIET))
      notify_player(player, cause, player,
    		    "You must specify a variable name.", NOT_QUIET);
    return;
  }

  /* the command parser doesn't parse this! */
  if(!(switches & ARG_FEXEC))
    lptr = exec(player, cause, player, argone, 0, 0, (char **)NULL);
  else
    lptr = argone;

  ptr = (char *)NULL;

  while((val = slist_next(lptr, &ptr)) != (char *)NULL)
    queue_iadd(player, cause, var, val, argtwo, 0, (char **)NULL);

  if(!(switches & ARG_FEXEC))
    ty_free((VOID *)lptr);
}

VOID do_case(player, cause, switches, argc, argv)
    int player, cause, switches, argc;
    char *argv[];
{
  register int idx = 1;
  register int done = 0;
  register int top = argc - 1;
  char *lptr;

  if(argc < 3) {
    if(!(switches & CMD_QUIET))
      notify_player(player, cause, player,
      		    "How can you case on that?", NOT_QUIET);
    return;
  }

  /* the command parser doesn't parse this! */
  if(!(switches & ARG_FEXEC))
    lptr = exec(player, cause, player, argv[0], 0, 0, (char **)NULL);
  else
    lptr = argv[0];

  while(1) {
    if((idx == top) && !done) {	/* default */
      queue_add(player, cause, 1, -1, argv[idx], 0, (char **)NULL);
      break;
    } else if(idx >= top)		/* simply done */
      break;

    if(quick_wild(argv[idx], lptr)) {
      /* queue this up */
      if(idx < top) {
        queue_add(player, cause, 1, -1, argv[idx+1], 0, (char **)NULL);
	done++;
      }
      if(switches & CASE_FIRST) /* all done */
	break;
    }

    idx += 2;
  }

  if(!(switches & ARG_FEXEC))
    ty_free((VOID *)lptr);
}
