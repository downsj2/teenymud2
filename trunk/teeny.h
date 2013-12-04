/* teeny.h */

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

#ifndef __TEENY_H

/* element code numbers. */

#define FLAGS           0
#define QUOTA           1
#define LOC             2

/* HOME and DROPTO are the same */
#define HOME            3
#define DROPTO          3

#define OWNER           4
#define CONTENTS        5
#define EXITS           6

/* ROOMS and PENNIES are the same */
#define ROOMS		7
#define PENNIES		7

#define NEXT            8
#define TIMESTAMP       9 /* time_t */
#define CREATESTAMP	10 /* time_t */
#define USES		11
#define PARENT		12
#define CHARGES		13
#define SEMAPHORES	14
#define COST		15
#define QUEUE		16

/* integer arrays. */

#define DESTS		25

/* strings. */

#define NAME		50

/* names of special attributes */
/* these reflect the tables in globals.c */

typedef struct lockslist {
  char *name;
  short matlen;
} LockList;

extern LockList Ltable[];
extern int Ltable_count;

#define LOCK            (Ltable[0].name)
#define ENLOCK          (Ltable[1].name)
#define PGLOCK          (Ltable[2].name)
#define TELINLOCK       (Ltable[3].name)
#define TELOUTLOCK      (Ltable[4].name)
#define ULOCK           (Ltable[5].name)

typedef struct strings {
  char *name;
  char *calias;
  short flags;
  bool command;
} StringList;

extern StringList Atable[];
extern int Atable_count;

#define DESC		(Atable[0].name)
#define IDESC		(Atable[1].name)
#define ODESC		(Atable[2].name)
#define ADESC		(Atable[3].name)
#define SUC		(Atable[4].name)
#define OSUC		(Atable[5].name)
#define ASUC		(Atable[6].name)
#define FAIL		(Atable[7].name)
#define OFAIL		(Atable[8].name)
#define AFAIL		(Atable[9].name)
#define DROP		(Atable[10].name)
#define ODROP		(Atable[11].name)
#define ADROP		(Atable[12].name)
#define ENTER		(Atable[13].name)
#define OENTER		(Atable[14].name)
#define OXENTER		(Atable[15].name)
#define AENTER		(Atable[16].name)
#define LEAVE		(Atable[17].name)
#define OLEAVE		(Atable[18].name)
#define OXLEAVE		(Atable[19].name)
#define ALEAVE		(Atable[20].name)
#define KILL		(Atable[21].name)
#define OKILL		(Atable[22].name)
#define AKILL		(Atable[23].name)
#define OTELEPORT	(Atable[24].name)
#define OXTELEPORT	(Atable[25].name)
#define ATELEPORT	(Atable[26].name)
#define SEX		(Atable[27].name)
#define ENFAIL		(Atable[28].name)
#define OENFAIL		(Atable[29].name)
#define AENFAIL		(Atable[30].name)
#define SITE		(Atable[31].name)
#define PASSWORD	(Atable[32].name)
#define APREFIX		(Atable[33].name)
#define AFILTER		(Atable[34].name)
#define USE		(Atable[35].name)
#define OUSE		(Atable[36].name)
#define AUSE		(Atable[37].name)
#define RUNOUT		(Atable[38].name)
#define PAY		(Atable[39].name)
#define OPAY		(Atable[40].name)
#define APAY		(Atable[41].name)
#define STARTUP		(Atable[42].name)
#define STTY		(Atable[43].name)
#define KILLFAIL	(Atable[44].name)
#define OKILLFAIL	(Atable[45].name)
#define AKILLFAIL	(Atable[46].name)
#define ALIAS		(Atable[47].name)
#define VRML_URL	(Atable[48].name)
#define HTDESC		(Atable[49].name)

#define SITE_FLGS	(Atable[31].flags)
#define PASSWORD_FLGS	(Atable[32].flags)
#define STTY_FLGS	(Atable[43].flags)
#define ALIAS_FLGS	(Atable[47].flags)

/* Object flags are 64bits long, broken into two 32bit words. */
#ifndef FLAGS_LEN
#define FLAGS_LEN	2	/* number of words */
#endif
#ifndef FLAGS_MAX
#define FLAGS_MAX	64	/* Maximum number of flags. */
#endif

#define sizeof_flags()	(sizeof(int) * FLAGS_LEN)

/* Object type is in the first word. */
#define TYPE_MASK       0x00000007
#define TYPE_TOTAL	4

#define TYP_PLAYER      0
#define TYP_THING       1
#define TYP_ROOM        2
#define TYP_EXIT        3

#define TYP_BAD		6

#define typeof_flags(_x)	(_x[0] & TYPE_MASK)

/* Type specific flags are in the first word */

#define ABODE		0x00000010	/* everything, 'cept exits */
#define OBVIOUS		0x00000010	/* for exits */
#define ENTER_OK	0x00000020	/* everything, 'cept exits */
#define EXTERNAL	0x00000020	/* for exits */
#define ROBOT		0x00000040	/* for players */
#define ACTION		0x00000040	/* for exits */
#define GUEST		0x00000080	/* for players */
#define CHOWN_OK	0x00000080	/* everything, 'cept players */
#define RETENTIVE	0x00000100	/* for players */
#define DESTROY_OK	0x00000100	/* everything, 'cept players */
#define PUPPET		0x00000200	/* for things */
#define REVERSED_WHO	0x00000200	/* for players */
#define TRANSPARENT	0x00000200	/* for exits */
#define BUILDER		0x00000400	/* for players */
#define BUILDING_OK	0x00000400	/* for things and rooms */
#define AUDIBLE		0x00000800	/* everything, cept rooms */
#define LIGHT		0x00001000	/* everything, cept rooms */
#define ELOQUENT	0x00002000	/* for players and things */
#define OPAQUE		0x00004000	/* for players and things */

/* Non-type specific flags are in the second word */

#define GOD		0x00000001
#define WIZARD          0x00000002
#define LINK_OK         0x00000004
#define DARK            0x00000008
#define HAVEN           0x00000010
#define JUMP_OK         0x00000020
#define STICKY		0x00000040
#define PARENT_OK	0x00000080
#define VISUAL		0x00000100
#define NOSPOOF		0x00000200
#define HIDDEN		0x00000400
#define HALT		0x00000800
#define HAS_STARTUP	0x00001000
#define LISTENER	0x00002000
#define PICKY		0x00004000
#define FILE_OK		0x00008000
#define QUIET		0x00010000
#define NOCHECK		0x00020000
#define INHERIT		0x00040000
#define CONTROL_OK	0x00080000

/* Internal, database related, flags are in the second word. */
#define ALIVE		0x10000000
#define IN_MEMORY       0x20000000
#define DIRTY		0x40000000
#define MARKED		0x80000000

/* A mask for the internal use flags */

#define INTERNAL_FLAGS  0xF0000000

/* attribute flags */
#define A_PRIVATE       0x00000100
#define A_VISIBLE       0x00000200
#define A_PICKY		0x00000400
#define A_IMMUTABLE	0x00000800
#define A_INTERNAL      0x00001000

#define A_FLAGS_MAX	32

#define DEFATTR_FLGS	0
#define DEFLOCK_FLGS	0

/* boolexp locks */
struct boolexp {
  short type;

  struct boolexp *sub1;
  struct boolexp *sub2;	

  union {
    int thing;			/* for object locks */
    int flags[FLAGS_LEN];	/* for flag locks */
    char *atr[2];		/* for attribute locks */
  } dat;
};

/* boolexp types */
#define BOOLEXP_AND	0
#define BOOLEXP_OR	1
#define BOOLEXP_NOT	2
#define BOOLEXP_CONST	3
#define BOOLEXP_FLAG	4
#define BOOLEXP_ATTR	5
#define BOOLEXP_END	9

/* matching flags */
#define MAT_THINGS		0x0001
#define MAT_PLAYERS		0x0002
#define MAT_EXITS		0x0004
#define MAT_ANYTHING		0x0007
#define MAT_EXTERNAL		0x0100	/* for exits */
#define MAT_INTERNAL		0x0200	/* for exits */
#define MAT_LOCKS		0x0400	/* check locks */

/* Notify exception flags */
#define NOT_NOSPOOF	0x01
#define NOT_NOPUPPET	0x02
#define NOT_NOAUDIBLE	0x04
#define NOT_DARKOK	0x08
#define NOT_QUIET	0x10
#define NOT_RAW		0x20

/* Resolver flags */
#define RSLV_NOISY	0x01
#define RSLV_EXITS	0x02
#define RSLV_SPLATOK	0x04
#define RSLV_HOME	0x08
#define RSLV_ATOK	0x10
#define RSLV_ALLOK	0x20
#define RSLV_RECURSE	0x40

/* exec flags */
#define EXEC_PRIM	0x01

/* Misc. defines */
#ifndef TRUE
#define TRUE		(1)
#endif
#ifndef FALSE
#define FALSE  		(0)
#endif
#define BUFFSIZ		512
#define MEDBUFFSIZ	4096
#define LARGEBUFFSIZ	16384
#define STAMP_LOGIN	0
#define STAMP_CREATED	1
#define STAMP_USED	2
#define STAMP_MODIFIED	2
#define LOCK_REGISTER	1
#define LOCK_DISCONNECT	2
#define LOCK_ALLOWCRE	3
#define LOCK_ALLOWCON	4
#ifndef HTML_NONE
#define HTML_NONE       0
#endif
#ifndef HTML_PUEBLO
#define HTML_PUEBLO     1
#endif

/* Logging. */
#define LOG_STATUS	0
#define LOG_GRIPE	1
#define LOG_COMMAND	2
#define LOG_ERROR	3

/* Macros for checking flags */

#define isGOD(_x)	((_x == mudconf.player_god) \
			 || Flags(_x,mudconf.god_flags,"isGOD"))
#define isWIZARD(_x)	((_x == mudconf.player_god) \
			 || Flags(_x,mudconf.god_flags,"isWIZARD") \
			 || Flags(_x,mudconf.wizard_flags,"isWIZARD") \
			 || inherit_wizard(_x))
#define isROBOT(_x)	((Typeof(_x) == TYP_PLAYER) \
			 && Flags(_x,mudconf.robot_flags,"isROBOT"))
#define isSTICKY(_x)	(Flag2(_x,STICKY,"isSTICKY"))
#define isLINK_OK(_x)	(Flag2(_x,LINK_OK,"isLINK_OK"))
#define isDARK(_x)	(Flag2(_x,DARK,"isDARK"))
#define isHAVEN(_x)	(Flag2(_x,HAVEN,"isHAVEN"))
#define isJUMP_OK(_x)	(Flag2(_x,JUMP_OK,"isJUMP_OK"))
#define isABODE(_x)	((Typeof(_x) != TYP_EXIT) && Flag1(_x,ABODE,"isABODE"))
#define isOBVIOUS(_x)	((Typeof(_x) == TYP_EXIT) \
			 && Flag1(_x,OBVIOUS,"isOBVIOUS"))
/* XXX */
#define isBUILDER(_x)	((Typeof(getowner(_x)) == TYP_PLAYER) \
			 && Flags(getowner(_x),mudconf.build_flags,"isBUILDER"))
#define isBUILDING_OK(_x)	((Typeof(_x) != TYP_PLAYER) \
			 && Flag1(_x,BUILDING_OK,"isBUILDING_OK"))
#define isENTER_OK(_x)	((Typeof(_x) != TYP_EXIT) \
			 && Flag1(_x,ENTER_OK,"isENTER_OK"))
#define isEXTERNAL(_x)	((Typeof(_x) == TYP_EXIT) \
			 && Flag1(_x,EXTERNAL,"isEXTERNAL"))
#define isACTION(_x)	((Typeof(_x) == TYP_EXIT) \
			 && Flag1(_x,ACTION,"isACTION"))
#define isGUEST(_x)	(Flags(_x,mudconf.guest_flags,"isGUEST"))
#define isLISTENER(_x)	(Flag2(_x,LISTENER,"isLISTENER"))
#define isDESTROY_OK(_x)	((Typeof(_x) != TYP_PLAYER) \
			 && Flag1(_x,DESTROY_OK,"isDESTROY_OK"))
#define isCHOWN_OK(_x)	((Typeof(_x) != TYP_PLAYER) \
			 && Flag1(_x,CHOWN_OK,"isCHOWN_OK"))
#define isPUPPET(_x)	((Typeof(_x) == TYP_THING) \
			 && Flag1(_x,PUPPET,"isPUPPET"))
#define isREVERSED_WHO(_x)	((Typeof(_x) == TYP_PLAYER) \
			 && Flag1(_x,REVERSED_WHO,"isREVERSED_WHO"))
#define isPARENT_OK(_x)	(Flag2(_x,PARENT_OK,"isPARENT_OK"))
#define isVISUAL(_x)	(Flag2(_x,VISUAL,"isVISUAL"))
#define isAUDIBLE(_x)	((Typeof(_x) != TYP_ROOM) \
			 && Flag1(_x,AUDIBLE,"isAUDIBLE"))
#define isOPAQUE(_x)	(((Typeof(_x) == TYP_THING) \
			  || (Typeof(_x) == TYP_PLAYER)) \
			 && Flag1(_x,OPAQUE,"isOPAQUE"))
#define isLIGHT(_x)	((Typeof(_x) != TYP_ROOM) \
			 && Flag1(_x,LIGHT,"isLIGHT"))
#define isELOQUENT(_x)	(((Typeof(_x) == TYP_PLAYER) \
			  || (Typeof(_x) == TYP_THING)) \
			 && Flag1(_x,ELOQUENT,"isELOQUENT"))
#define isTRANSPARENT(_x)	((Typeof(_x) == TYP_EXIT) \
				 && Flag1(_x,TRANSPARENT,"isTRANSPARENT"))
#define isNOSPOOF(_x)	(Flag2(_x,NOSPOOF,"isNOSPOOF"))
#define isHIDDEN(_x)	(Flag2(_x,HIDDEN,"isHIDDEN"))
#define isHALT(_x)	(Flag2(_x,HALT,"isHALT"))
#define isHAS_STARTUP(_x)	(Flag2(_x,HAS_STARTUP,"isHAS_STARTUP"))
#define isRETENTIVE(_x)	((Typeof(_x) == TYP_PLAYER) \
			 && Flag1(_x,RETENTIVE,"isRETENTIVE"))
#define isPICKY(_x)	(Flag2(_x,PICKY,"isPICKY"))
#define isFILE_OK(_x)	(Flag2(_x,FILE_OK,"isFILE_OK"))
#define isQUIET(_x)	(Flag2(_x,QUIET,"isQUIET"))
#define isNOCHECK(_x)	(Flag2(_x,NOCHECK,"isNOCHECK"))
#define isINHERIT(_x)	(Flag2(_x,INHERIT,"isINHERIT"))
#define isCONTROL_OK(_x)	(Flag2(_x,CONTROL_OK,"isCONTROL_OK"))

#define isALIVE(_x)	(Flag2(_x,ALIVE,"isALIVE"))
#define isMARKED(_x)	(Flag2(_x,MARKED,"isMARKED"))

#define isPLAYER(_x)	(Typeof(_x) == TYP_PLAYER)
#define isROOM(_x)	(Typeof(_x) == TYP_ROOM)
#define isEXIT(_x)	(Typeof(_x) == TYP_EXIT)
#define isTHING(_x)	(Typeof(_x) == TYP_THING)

#define isGARBAGE(_x)	((_x >= 0) && (_x < mudstat.total_objects) \
			 && !exists_object(_x))

/* flag set macros */
#define setHALT(_x)	(setFlag(_x,HALT,1,"setHALT"))
#define setHAS_STARTUP(_x)	(setFlag(_x,HAS_STARTUP,1,"setHAS_STARTUP"))
#define unsetHAS_STARTUP(_x)	\
			(unsetFlag(_x,HAS_STARTUP,1,"unsetHAS_STARTUP"))

/* more macros */

#define has_contents(_x)	(Typeof(_x) != TYP_EXIT)
#define has_exits(_x)	(Typeof(_x) != TYP_EXIT)
#define has_rooms(_x)	(Typeof(_x) == TYP_ROOM)
#define has_home(_x)	((Typeof(_x) != TYP_EXIT) \
			 && (Typeof(_x) != TYP_ROOM))
#define has_dests(_x)	(Typeof(_x) == TYP_EXIT)
#define has_dropto(_x)	(Typeof(_x) == TYP_ROOM)
#define has_quota(_x)	(Typeof(_x) == TYP_PLAYER)
#define has_pennies(_x)	(Typeof(_x) == TYP_PLAYER)

#define cost_of(_x)	(isROOM(_x) ? mudconf.room_cost : \
			 (isEXIT(_x) ? mudconf.exit_cost : \
				(isTHING(_x) ? mudconf.thing_cost : 0)))

#define could_hear(_x)	(isPLAYER(_x) || isTHING(_x))

#define is_list(_x)	((_x == CONTENTS) || (_x == EXITS) || (_x == ROOMS))

#define notify_oall(_p,_c,_s,_st,_f)	notify_except2(_p,_c,_s,_st,_p,-1,_f)
#define notify_all(_p,_c,_s,_st,_f)	notify_except2(_p,_c,_s,_st,-1,-1,_f)
#define notify_except(_p,_c,_s,_st,_e,_f) \
			notify_except2(_p,_c,_s,_st,_e,-1,_f)
#define notify_contents(_l,_c,_s,_st,_f) \
			notify_contents_except(_l,_c,_s,_st,-1,_f)

#define remove_newline(_zz)	{int _yy = strlen(_zz)-1; \
				 if(_zz[_yy] == '\n') _zz[_yy] = '\0';}

#define can_see_attr(_p,_c,_o,_f)	((isVISUAL(_o) || (_f & A_VISIBLE) || \
				  	 controls(_p, _c, _o)) && \
				 	(!(_f & A_INTERNAL) || isGOD(_p)))
#define can_set_attr(_p,_c,_o,_f)	(controls(_p, _c, _o) \
					 && (!(_f & A_INTERNAL) \
					     || isGOD(_p)) \
					 && (!(_f & A_IMMUTABLE) \
					     || isWIZARD(_p)))

#define __TEENY_H
#endif				/* __TEENY_H */
