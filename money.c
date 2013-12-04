/* money.c */

#include "config.h"

/*
 *		       This file is part of TeenyMUD II.
 *	  Copyright(C) 1994, 1995 by Jason Downs.  All rights reserved.
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

#include <sys/types.h>
#include <stdio.h>
#ifdef HAVE_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif			/* HAVE_STRING_H */
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif			/* HAVE_STDLIB_H */

#include "conf.h"
#include "teeny.h"
#include "commands.h"
#include "externs.h"

/*
 * money.c -- Routines and commands dealing with money.
 */

static INLINE int isvowel(c)
    char c;
{
  if((c == 'a') || (c == 'e') || (c == 'i')
     || (c == 'o') || (c == 'u'))
    return(1);
  return(0);
}

/*
 * money_name(int amount)
 *
 * Return the best-fit name for the specified amount of currency.
 */

#define bufleft()	(sizeof(money_buffer) - (money_ptr - money_buffer))

static char *money_name(amount)
    int amount;
{
  static char money_buffer[BUFFSIZ];
  register char *money_ptr;
  int doneone = 0;

  if (amount == 0)
    return("no money");

  if (amount < 0) {
    strcpy(money_buffer, "negative ");
    money_ptr = (money_buffer + strlen(money_buffer));
    amount = (-amount);
  } else
    money_ptr = money_buffer;

  if (amount >= 100) {
    register int num = amount / 100;
    amount -= (num * 100);

    if (num == 1) {
      snprintf(money_ptr, bufleft(), "%s %s, ",
      	       (isvowel(mudconf.money_dollar[0]) ? "an" : "a"),
	       mudconf.money_dollar);
    } else {
      snprintf(money_ptr, bufleft(), "%d %s, ", num, mudconf.money_dollars);
    }
    money_ptr += strlen(money_ptr);

    doneone++;
  } 

  if (amount >= 25) {
    register int num = amount / 25;
    amount -= (num * 25);

    if (num == 1) {
      snprintf(money_ptr, bufleft(), "%s%s %s, ",
	       (((doneone > 0) && (amount == 0)) ? "and " : ""),
      	       (isvowel(mudconf.money_quarter[0]) ? "an" : "a"),
	       mudconf.money_quarter);
    } else {
      snprintf(money_ptr, bufleft(), "%s%d %s, ",
	       (((doneone > 0) && (amount == 0)) ? "and " : ""),
	       num, mudconf.money_quarters);
    }
    money_ptr += strlen(money_ptr);

    doneone++;
  }

  if (amount >= 10) {
    register int num = amount / 10;
    amount -= (num * 10);

    if (num == 1) {
      snprintf(money_ptr, bufleft(), "%s%s %s, ",
	       (((doneone > 0) && (amount == 0)) ? "and " : ""),
      	       (isvowel(mudconf.money_dime[0]) ? "an" : "a"),
	       mudconf.money_dime);
    } else {
      snprintf(money_ptr, bufleft(), "%s%d %s, ",
	       (((doneone > 0) && (amount == 0)) ? "and " : ""),
	       num, mudconf.money_dimes);
    }
    money_ptr += strlen(money_ptr);

    doneone++;
  }

  if (amount >= 5) {
    register int num = amount / 5;
    amount -= (num * 5);

    if (num == 1) {
      snprintf(money_ptr, bufleft(), "%s%s %s, ",
	       (((doneone > 0) && (amount == 0)) ? "and " : ""),
	       (isvowel(mudconf.money_nickle[0]) ? "an" : "a"),
	       mudconf.money_nickle);
    } else {
      snprintf(money_ptr, bufleft(), "%s%d %s, ",
	       (((doneone > 0) && (amount == 0)) ? "and " : ""),
	       num, mudconf.money_nickles);
    }
    money_ptr += strlen(money_ptr);

    doneone++;
  }

  if (amount > 0) {
    if (amount == 1) {
      snprintf(money_ptr, bufleft(), "%s%s %s",
	       ((doneone > 0) ? "and " : ""),
	       (isvowel(mudconf.money_penny[0]) ? "an" : "a"),
	       mudconf.money_penny);
    } else {
      snprintf(money_ptr, bufleft(), "%s%d %s",
	       ((doneone > 0) ? "and " : ""),
	       amount, mudconf.money_pennies);
    }
  } else {
    /* Fix termination. */
    if(((money_ptr - money_buffer) > 2) && (money_ptr[-2] == ','))
      money_ptr[-2] = '\0';
  }

  return(money_buffer);
}

/*
 * reward_move(int player, int cause, int loc)
 *
 * Reward the player a random amount of money, if they don't own loc.
 */
void reward_move(player, cause, loc)
    int player, cause, loc;
{
  if (mudconf.enable_money
      && !controls(player, cause, loc) && (random() % 10) == 0) {
    char buffer[BUFFSIZ];
    int amount;

    switch ((int)(random() % 20)) {
    case 0:
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
    case 7:
      amount = 1;
      break;
    case 8:
    case 9:
    case 10:
    case 11:
    case 12:
    case 13:
      amount = 5;
      break;
    case 14:
    case 15:
    case 16:
      amount = 10;
      break;
    case 17:
    case 18:
      amount = 25;
      break;
    default:
      amount = 100;
    }

    snprintf(buffer, sizeof(buffer), "You found %s!", money_name(amount));
    notify_player(player, cause, player, buffer, 0);
    reward_money(player, amount);
  }
}

/* Score user command. */
VOID do_score(player, cause, switches)
    int player, cause, switches;
{
  int owner, pennies;
  char buf[BUFFSIZ];

  if (mudconf.enable_money) {
    if ((get_int_elt(player, OWNER, &owner) != -1) &&
        (get_int_elt(owner, PENNIES, &pennies) != -1)) {
      snprintf(buf, sizeof(buf), "You have %s.", money_name(pennies));
      notify_player(player, cause, player, buf, 0);
    } else {
      notify_bad(player);
    }
  } else {
    if(!(switches & CMD_QUIET))
      notify_player(player, cause, player, "Money is not enabled.",
      		    NOT_QUIET);
  }
}

/* Give user command. */
VOID do_give(player, cause, switches, argone, argtwo)
    int player, cause, switches;
    char *argone, *argtwo;
{
  int victim, cost, expt;
  int owner, pennies, nval;
  char *ptr, buf[MEDBUFFSIZ], *pname, *vname;

  if(!mudconf.enable_money) {
    if(!(switches & CMD_QUIET))
      notify_player(player, cause, player, "Money is not enabled.", NOT_QUIET);
    return;
  }

  if ((argone == (char *)NULL) || (argone[0] == '\0')
      || (argtwo == (char *)NULL) || (argtwo[0] == '\0')) {
    if(!(switches & CMD_QUIET))
      notify_player(player, cause, player, "Give what to who?", NOT_QUIET);
    return;
  }
  victim = resolve_player(player, cause, argone,
  			  (!(switches & CMD_QUIET) ? RSLV_NOISY : 0));
  if(victim == -1)
    return;
  cost = (int)strtol(argtwo, &ptr, 10);
  if((ptr == argtwo) || ((cost < 1) && !isWIZARD(player))){
    if(!(switches & CMD_QUIET))
      notify_player(player, cause, player,
    		    "You must specify a postive amount of money.", NOT_QUIET);
    return;
  }

  if(get_int_elt(victim, COST, &expt) == -1) {
    notify_bad(player);
    return;
  }
  if((expt > cost) && (cost > 0)) {
    if(!(switches & CMD_QUIET))
      notify_player(player, cause, player,
      		    "Being a cheapskate, eh?", NOT_QUIET);
    return;
  }
  if (!can_afford(player, cause,
      (((cost > expt) && (expt > 0)) ? expt : cost), (switches & CMD_QUIET)))
    return;

  if ((get_int_elt(victim, OWNER, &owner) == -1) ||
      (get_int_elt(owner, PENNIES, &pennies) == -1) ||
      (get_str_elt(player, NAME, &pname) == -1) ||
      (get_str_elt(victim, NAME, &vname) == -1)) {
    notify_bad(player);
    return;
  }

  nval = pennies + (((cost > expt) && (expt > 0)) ? expt : cost);
  if (!isWIZARD(player) && (nval > mudconf.max_pennies)) {
    if(!(switches & CMD_QUIET))
      notify_player(player, cause, player, 
		    "They don't need that much money!", NOT_QUIET);
    return;
  }
  if (set_int_elt(owner, PENNIES, nval) == -1) {
    notify_bad(player);
    return;
  }

  if(!(switches & CMD_QUIET)) {
    if((cost > expt) && (expt > 0)) {
      snprintf(buf, sizeof(buf), "You give %s to %s and get %s in change.",
	       money_name(cost), vname, money_name(cost - expt));
    } else {
      snprintf(buf, sizeof(buf), "You give %s to %s.", money_name(cost),
	       vname);
    }
    notify_player(player, cause, player, buf, NOT_QUIET);
  }
  snprintf(buf, sizeof(buf), "%s gives you %s.", pname, money_name(cost));
  notify_player(victim, cause, player, buf, NOT_QUIET);

  act_object(player, cause, victim, PAY, OPAY, APAY, -1, (char *)NULL,
  	     (char *)NULL);
}

/* Cost user command. */
VOID do_cost(player, cause, switches, argone, argtwo)
    int player, cause, switches;
    char *argone, *argtwo;
{
  int dest, cost;
  char *ptr;

  if((argone == (char *)NULL) || (argone[0] == '\0')) {
    if(!(switches & CMD_QUIET))
      notify_player(player, cause, player, "Set the cost of what?", NOT_QUIET);
    return;
  }

  if((argtwo == (char *)NULL) || (argtwo[0] == '\0'))
    cost = 0;
  else {
    cost = (int)strtol(argtwo, &ptr, 10);
    if((ptr == argtwo) || (cost < 0)) {
      if(!(switches & CMD_QUIET))
        notify_player(player, cause, player,
		      "Cost must be a positive number.", NOT_QUIET);
      return;
    }
  }

  dest = resolve_object(player, cause, argone,
  			(!(switches & CMD_QUIET) ? RSLV_NOISY : 0));
  if(dest == -1)
    return;

  if(!controls(player, cause, dest)) {
    if(!(switches & CMD_QUIET))
      notify_player(player, cause, player,
      		    "You can't set the cost of that!", NOT_QUIET);
    return;
  }

  if(set_int_elt(dest, COST, cost) == -1) {
    notify_bad(player);
    return;
  }

  if(!(switches & CMD_QUIET))
    notify_player(player, cause, player,
		  ((cost == 0) ? "Cost unset." : "Cost set."), NOT_QUIET);
}
