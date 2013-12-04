/* group.c */

#include "config.h"

/*
 *		       This file is part of TeenyMUD II.
 *	    Copyright(C) 1995 by Jason Downs.  All rights reserved.
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

#include "conf.h"
#include "teeny.h"
#include "commands.h"
#include "externs.h"

/* User-level support for ``groups''. */

/*
 * ``Groups'' are maintained seperate from the database, in memory, so
 * they don't last between boots or logins.
 */

struct glist {
  int player;		/* Current player, or leader. */
  int flags;
#define GRP_FOLLOW	0x01	/* Follow the leader. */
#define GRP_SILENT	0x02	/* Don't hear group messages. */
#define GRP_CLOSE	0x04	/* No new members. */

  /* If top of the list, use these two: */
  int count;		/* Total number of members. */
  char *str;		/* Name of group. */

  struct glist *next;	/* Next in list. NULL if bottom. */
  struct glist *prev;	/* Previous. NULL if top. */
};

#ifndef MAXPGROUPS
#define MAXPGROUPS	128		/* Should be more than enough. */
#endif			/* MAXPGROUPS */

static struct glist *garray[MAXPGROUPS];
static int gtop = 0;

static struct glist *glist_alloc _ANSI_ARGS_((void));
static void glist_free _ANSI_ARGS_((struct glist *));
static int group_find _ANSI_ARGS_((int, struct glist **, struct glist **));
static void group_zap _ANSI_ARGS_((struct glist *, int));
static void group_notify _ANSI_ARGS_((struct glist *, int, char *, int));
static void group_dump _ANSI_ARGS_((int, int, int));
static void group_sdump _ANSI_ARGS_((int, int, struct glist *));

#define can_follow(_x)	((Typeof(_x) == TYP_PLAYER) || \
			 (Typeof(_x) == TYP_THING))

/*
 * glist_alloc()
 *
 * Allocate a glist struct.
 */
static struct glist *glist_alloc()
{
  register struct glist *ret;

  ret = (struct glist *)ty_malloc(sizeof(struct glist), "glist_alloc.ret");
  bzero((VOID *)ret, sizeof(struct glist));

  return(ret);
}

/*
 * glist_free()
 *
 * Destroy a glist struct.
 */
static void glist_free(ptr)
    register struct glist *ptr;
{
  if(ptr != (struct glist *)NULL) {
    if(ptr->str != (char *)NULL) {
      /* free this, too. */
      ty_free(ptr->str);
    }
    ty_free(ptr);
  }
}

/*
 * group_find()
 *
 * Return the appropiate glist pointers.
 */
static int group_find(player, actual, leader)
    register int player;
    struct glist **actual, **leader;
{
  register struct glist *gcur;
  register int acur;

  for(acur = 0; acur < gtop; acur++) {
    if(garray[acur] != (struct glist *)NULL) {
      for(gcur = garray[acur]; gcur != (struct glist *)NULL;
	  gcur = gcur->next) {
	if(gcur->player == player) {
	  if(actual != (struct glist **)NULL)
	    *actual = gcur;
	  if(leader != (struct glist **)NULL)
	    *leader = garray[acur];

	  return(1);
	}
      }
    }
  }

  return(0);
}

/*
 * group_ismem()
 *
 * Return true if player is a member (or leader) of a group.
 */
int group_ismem(player)
    int player;
{
  return(group_find(player, (struct glist **)NULL, (struct glist **)NULL));
}

/*
 * group_isleader()
 *
 * Return true if player is a leader of a group.
 */
int group_isleader(player)
    int player;
{
  struct glist *leader;

  return(group_find(player, (struct glist **)NULL, &leader)
	 && (leader->player == player));
}

/*
 * group_remove()
 *
 * Remove a player from a group.  If the player is a leader, disolve the
 * group.
 */
void group_remove(player, cause)
    int player, cause;
{
  struct glist *actual, *leader;
  char *name, buffer[MEDBUFFSIZ];

  if(group_find(player, &actual, &leader)) {
    if(actual == leader)
      group_zap(leader, cause);
    else {
      /* prev should never be NULL. */
      (actual->prev)->next = actual->next;
      if(actual->next != (struct glist *)NULL)
	(actual->next)->prev = actual->prev;
      glist_free(actual);
 
      /* Decrement leader's count. */
      leader->count--;

      /* Notify everyone. */
      if(get_str_elt(player, NAME, &name) == -1)
	name = "???";
      snprintf(buffer, sizeof(buffer), "**] %s has left the group.", name);

      group_notify(leader, cause, buffer, 0);
    }
  }
}

/*
 * group_create()
 *
 * Create a new group and make player the leader.
 */
void group_create(player, cause, switches, str)
    int player, cause;
    char *str;
{
  if(group_ismem(player)) {
    if(!(switches & CMD_QUIET))
      notify_player(player, cause, player, "You're already in a group!",
      		    NOT_QUIET);
  } else {
    if(gtop < MAXPGROUPS) {
      struct glist *newgrp;

      newgrp = glist_alloc();
      garray[gtop++] = newgrp;

      newgrp->player = player;

      /* Set the desc. */
      if(str != (char *)NULL)
	newgrp->str = ty_strdup(str, "group_create.str");

      if(!(switches & CMD_QUIET))
        notify_player(player, cause, player, "New group created.", NOT_QUIET);
    } else {
      if(!(switches & CMD_QUIET))
        notify_player(player, cause, player,
		      "Sorry, there are too many groups.", NOT_QUIET);
    }
  }
}

/*
 * group_join()
 *
 * Add player to the other's group.
 */
void group_join(player, cause, switches, friend)
    int player, cause, switches, friend;
{
  struct glist *actual, *leader;

  if(group_ismem(player)) {
    if(!(switches & CMD_QUIET))
      notify_player(player, cause, player,
		    "You can only be in one group at a time.", NOT_QUIET);
    return;
  }

  if(!group_find(friend, &actual, &leader)) {
    if(!(switches & CMD_QUIET))
      notify_player(player, cause, player, "But they aren't in a group!",
      		    NOT_QUIET);
  } else {
    struct glist *newmem;
    char *name, buf[MEDBUFFSIZ];

    if(leader->flags & GRP_CLOSE) {
      if(!(switches & CMD_QUIET))
        notify_player(player, cause, player, "Sorry, that group is closed.",
		      NOT_QUIET);
      return;
    }

    newmem = glist_alloc();
    newmem->player = player;

    /* Insert them forward of leader. */
    newmem->prev = leader;
    if(leader->next != (struct glist *)NULL)
      (leader->next)->prev = newmem;
    newmem->next = leader->next;
    leader->next = newmem;

    /* Increment leader's count. */
    leader->count++;

    /* Notify everyone. */
    if(get_str_elt(player, NAME, &name) == -1)
      name = "???";
    snprintf(buf, sizeof(buf), "**] %s has joined the group.", name);
    group_notify(leader, cause, buf, 0);
  }
}

/*
 * group_zap()
 *
 * Lower-level group killer.
 */
static void group_zap(grptr, cause)
    struct glist *grptr;
    int cause;
{
  register struct glist *curr, *next;
  register int idx;

  /* Inline notify_group(). */
  curr = grptr;
  while(curr != (struct glist *)NULL) {
    next = curr->next;		/* Save next. */

    if(!(curr->flags & GRP_SILENT))
      notify_player(curr->player, cause, curr->player,
		    "**] Your group has been dissolved.", NOT_QUIET);
    glist_free(curr);

    curr = next;
  }

  /* grptr is no longer valid, but we need it to fix garray. */
  for(idx = 0; idx < gtop; idx++) {
    if(garray[idx] == grptr) {
      /* Found it.  Sure hope your bcopy() likes to overlap! */
      bcopy((VOID *)&garray[idx+1], (VOID *)&garray[idx], gtop - (idx + 1));
      gtop--;	/* Important! */
      break;
    }
  }
}

/*
 * group_notify()
 *
 * Send a message to everyone in a group. Low level.
 */
static void group_notify(grptr, cause, str, flags)
    struct glist *grptr;
    int cause;
    char *str;
    int flags;
{
  register struct glist *curr;

  for(curr = grptr; curr != (struct glist *)NULL; curr = curr->next) {
    if(!(curr->flags & GRP_SILENT))
      notify_player(curr->player, cause, curr->player, str, flags);
  }
}

/*
 * notify_group()
 *
 * Send a message to everyone in a group. High level.
 */
void notify_group(player, cause, sender, str, flags)
    int player, cause, sender;
    char *str;
    int flags;
{
  struct glist *actual, *leader;
  register struct glist *curr;

  if(group_find(player, &actual, &leader)) {
    for(curr = leader; curr != (struct glist *)NULL; curr = curr->next) {
      notify_player(curr->player, cause, sender, str, flags);
    }
  }
}

/*
 * group_follow()
 *
 * For everyone in player's group (if they lead) who is following, and are
 * in ``oldloc'', teleport to ``newloc''.  This is different from standard
 * teleport: it doesn't trigger teleport actions.
 */
void group_follow(leader, cause, oldloc, newloc)
    int leader, cause, oldloc, newloc;
{
  struct glist *actual, *rleader;
  register struct glist *curr;

  if(group_find(leader, &actual, &rleader) && (actual == rleader)) {
    for(curr = rleader->next; curr != (struct glist *)NULL; curr = curr->next) {
      if((curr->flags & GRP_FOLLOW) && can_follow(curr->player)) {
	int loc;

	if(get_int_elt(curr->player, LOC, &loc) == -1) {
	  logfile(LOG_ERROR, "group_follow: couldn't get location of #%d\n",
	          curr->player);
	  continue;
	}
	if(loc != oldloc)	/* They aren't with the group. */
	  continue;

        move_player_leave(curr->player, cause, oldloc, 0,
    		          "You feel a wrenching sensation...");
        move_player_arrive(curr->player, cause, newloc, 0);
        look_location(curr->player, cause, 0);
      }
    }
  }
}

/*
 * group_dump()
 *
 * Produce a pretty-formatted group listing.  High level.
 */
static void group_dump(player, cause, switches)
    int player, cause, switches;
{
  struct glist *actual, *leader;
  register int idx;
  
  if(switches & GROUP_ALL) {
    for(idx = 0; idx < gtop; idx++) {
      if(garray[idx] != (struct glist *)NULL)
	group_sdump(player, cause, garray[idx]);
    }
    notify_player(player, cause, player, "***End of list***", 0);
  } else {
    if(group_find(player, &actual, &leader)) {
      group_sdump(player, cause, leader);
      notify_player(player, cause, player, "***End of list***", 0);
    } else {
      if(!(switches & CMD_QUIET))
        notify_player(player, cause, player, "But you're not in a group!",
		      NOT_QUIET);
    }
  }
}

/*
 * group_sdump()
 *
 * Produce a pretty-formatted group listing.  Low level.
 */
static void group_sdump(player, cause, grptr)
    int player, cause;
    struct glist *grptr;
{
  char buf[MEDBUFFSIZ], *name;
  register struct glist *curr;

  snprintf(buf, sizeof(buf), "Group: %s",
	  ((grptr->str != (char *)NULL) ? grptr->str : "*NO NAME*"));
  notify_player(player, cause, player, buf, 0);

  strcpy(buf, "Members:");
  for(curr = grptr; curr != (struct glist *)NULL; curr = curr->next) {
    if(get_str_elt(curr->player, NAME, &name) == -1)
      name = "???";

    if((sizeof(buf) - strlen(buf)) < strlen(name)) {	/* Overflow */
      notify_player(player, cause, player, buf, 0);
      sprintf(buf, "Members: %s", name);
    } else
      sprintf(&buf[strlen(buf)], " %s", name);
  }
  notify_player(player, cause, player, buf, 0);
}

/* User commands. */

VOID do_group(player, cause, switches, argone)
    int player, cause, switches;
    char *argone;
{
  if(mudconf.enable_groups) {
    if(switches & GROUP_DISOLVE) {
      struct glist *actual, *leader;

      if(!group_find(player, &actual, &leader)) {
        if(!(switches & CMD_QUIET))
	  notify_player(player, cause, player, "You're not even in a group!",
	  		NOT_QUIET);
	return;
      }
      if((leader->player != player) && !isWIZARD(player)) {
        if(!(switches & CMD_QUIET))
	  notify_player(player, cause, player, "Permission denied.", NOT_QUIET);
	return;
      }
      group_zap(leader, cause);
    } else if((switches & GROUP_CLOSE) || (switches & GROUP_OPEN)
	      || (switches & GROUP_SILENCE) || (switches & GROUP_NOISY)) {
      struct glist *actual, *leader;
      char *msg;

      if(!group_find(player, &actual, &leader)) {
        if(!(switches & CMD_QUIET))
	  notify_player(player, cause, player, "You're not even in a group!",
	  		NOT_QUIET);
	return;
      }
      if((leader->player != player) && !isWIZARD(player)) {
        if(!(switches & CMD_QUIET))
	  notify_player(player, cause, player, "Permission denied.", NOT_QUIET);
	return;
      }

      if(switches & GROUP_CLOSE) {
	leader->flags |= GRP_CLOSE;
	msg = "Group closed.";
      } else if(switches & GROUP_OPEN) {
	leader->flags &= ~GRP_CLOSE;
	msg = "Group opened.";
      } else if(switches & GROUP_SILENCE) {
	actual->flags |= GRP_SILENT;
	msg = "Group silenced.";
      } else {
	actual->flags &= ~GRP_SILENT;
	msg = "Group unsilenced.";
      }
      if(!(switches & CMD_QUIET))
        notify_player(player, cause, player, msg, NOT_QUIET);
    } else if(switches & GROUP_BOOT) {
      struct glist *actual, *leader;
      int victim;
	
      if((argone == (char *)NULL) || (argone[0] == '\0')) {
        if(!(switches & CMD_QUIET))
	  notify_player(player, cause, player, "You must specify a player.", 
	  		NOT_QUIET);
	return;
      }
      victim = resolve_player(player, cause, argone,
      			      (!(switches & CMD_QUIET) ? RSLV_NOISY|RSLV_ALLOK
						       : RSLV_ALLOK));
      if(victim == -1)
	return;

      if(!group_find(victim, &actual, &leader)) {
        if(!(switches & CMD_QUIET))
	  notify_player(player, cause, player,
		        "They're not even in a group!", NOT_QUIET);
	return;
      }
      if((leader->player != player) && !isWIZARD(player)) {
        if(!(switches & CMD_QUIET))
	  notify_player(player, cause, player, "Permission denied.", NOT_QUIET);
	return;
      }
      group_remove(victim, cause);
    } else if((switches & GROUP_SAY) || (switches & GROUP_EMOTE)) {
      char *name, buf[MEDBUFFSIZ];

      if(get_str_elt(player, NAME, &name) == -1) {
	notify_bad(player);
	return;
      }
      if(switches & GROUP_SAY) {
	snprintf(buf, sizeof(buf), "**] %s says, \"%s\"", name, 
		((argone != (char *)NULL) ? argone : ""));
      } else {
	if((argone != (char *)NULL) &&
	   ((argone[0] == ',') || (argone[0] == '\''))) {
	  snprintf(buf, sizeof(buf), "**] %s%s", name, argone);
	} else
	  snprintf(buf, sizeof(buf), "**] %s %s", name,
		  ((argone != (char *)NULL) ? argone : ""));
      }

      notify_group(player, cause, player, buf, 0);
    } else if(switches & GROUP_DISPLAY) {
      group_dump(player, cause, switches);
    } else if(switches & GROUP_JOIN) {
      int friend;

      if((argone == (char *)NULL) || (argone[0] == '\0')) {
        if(!(switches & CMD_QUIET))
	  notify_player(player, cause, player,
		        "You must specify a player to join.", NOT_QUIET);
	return;
      }
      friend = resolve_player(player, cause, argone,
      			      (!(switches & CMD_QUIET) ? RSLV_NOISY|RSLV_ALLOK
						       : RSLV_ALLOK));
      if(friend == -1)
	return;
      group_join(player, cause, switches, friend);
    } else if(switches & GROUP_FOLLOW) {
      struct glist *actual, *leader;

      if(!group_find(player, &actual, &leader)) {
        if(!(switches & CMD_QUIET))
          notify_player(player, cause, player,
		        "You're not a member of a group!", NOT_QUIET);
      } else {
        if(actual == leader) {
	  if(!(switches & CMD_QUIET))
	    notify_player(player, cause, player,
		          "You wish to follow yourself?", NOT_QUIET);
	  return;
        }
        actual->flags |= GRP_FOLLOW;

	if(!(switches & CMD_QUIET))
          notify_player(player, cause, player, "Following activated.",
	  		NOT_QUIET);

        if(!(leader->flags & GRP_SILENT)) {
	  char *name, buf[MEDBUFFSIZ];

          if(get_str_elt(player, NAME, &name) == -1)
	    name = "???";
          snprintf(buf, sizeof(buf), "%s is now following you.", name);
          notify_player(leader->player, cause, player, buf, 0);
        }
      }
    } else {
      if(isGUEST(player)) {		/* XXX */
        if(!(switches & CMD_QUIET))
	  notify_player(player, cause, player, "Permission denied.", NOT_QUIET);
	return;
      }
      group_create(player, cause, switches, argone);
    }
  } else {
    if(!(switches & CMD_QUIET))
      notify_player(player, cause, player, "Sorry, groups are not enabled.",
      		    NOT_QUIET);
  }
}
