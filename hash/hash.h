/* hash.h --
 *
 * 	This file contains definitions used by the hash module,
 * 	which maintains hash tables.
 *
 * Copyright 1986, 1988 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 * $Header: /sprite/src/lib/include/RCS/hash.h,v 1.3 89/06/23 11:29:46 rab Exp Locker: mendel $ SPRITE (Berkeley)
 */


#ifndef	_HASH
#define	_HASH

#ifndef _LIST
#include "list.h"
#endif

#ifndef TRUE
#define TRUE	1
#endif
#ifndef FALSE
#define FALSE	0
#endif

/* 
 * The following defines one entry in the hash table.
 */

typedef struct Hash_Entry {
    List_Links	      links;		/* Used to link together all the
    					 * entries associated with the same
					 * bucket. */
    int		     *clientData;	/* Arbitrary piece of data associated
    					 * with key. */
    union {
	char	 *ptr;			/* One-word key value to identify
					 * entry. */
	int words[1];			/* N-word key value.  Note: the actual
					 * size may be longer if necessary to
					 * hold the entire key. */
	char 	 name[4];		/* Text name of this entry.  Note: the
					 * actual size may be longer if
					 * necessary to hold the whole string.
					 * This MUST be the last entry in the
					 * structure!!! */
    } key;
} Hash_Entry;

/* 
 * A hash table consists of an array of pointers to hash
 * lists.  Tables can be organized in either of three ways, depending
 * on the type of comparison keys:
 *
 *	Strings:	  these are NULL-terminated; their address
 *			  is passed to HashFind as a (char *).
 *	Single-word keys: these may be anything, but must be passed
 *			  to Hash_Find as an (char *).
 *	Multi-word keys:  these may also be anything; their address
 *			  is passed to HashFind as an (char *).
 *
 *	Single-word keys are fastest, but most restrictive.
 */

#define HASH_STRING_KEYS	-1
#define HASH_STRCASE_KEYS	0
#define HASH_ONE_WORD_KEYS	1

typedef struct Hash_Table {
    List_Links 	*bucketPtr;	/* Pointer to array of List_Links, one
    				 * for each bucket in the table. */
    int 	size;		/* Actual size of array. */
    int 	numEntries;	/* Number of entries in the table. */
    int 	downShift;	/* Shift count, used in hashing function. */
    int 	mask;		/* Used to select bits for hashing. */
    int 	keyType;	/* Type of keys used in table:
    				 * HASH_STRING_KEYS, HASH_ONE-WORD_KEYS,
				 * or >1 menas keyType gives number of words
				 * in keys.
				 */
} Hash_Table;

/* 
 * The following structure is used by the searching routines
 * to record where we are in the search.
 */

typedef struct Hash_Search {
    Hash_Table  *tablePtr;	/* Table being searched. */
    int 	nextIndex;	/* Next bucket to check (after current). */
    Hash_Entry 	*hashEntryPtr;	/* Next entry to check in current bucket. */
    List_Links	*hashList;	/* Hash chain currently being checked. */
} Hash_Search;

/*
 * Macros.
 */

/*
 * int *Hash_GetValue(h) 
 *     Hash_Entry *h; 
 */

#define Hash_GetValue(h) ((h)->clientData)

/* 
 * Hash_SetValue(h, val); 
 *     HashEntry *h; 
 *     char *val; 
 */

#define Hash_SetValue(h, val) ((h)->clientData = (int *) (val))

/*
 * Hash_GetKey(tablePtr, h);
 *	Hash_Table *tablePtr;
 *	Hash_Entry *h;
 */

#define Hash_GetKey(tablePtr, h) \
    ((char *) (((tablePtr)->keyType == HASH_ONE_WORD_KEYS) ? (h)->key.ptr \
						    : (h)->key.name))

/* 
 * Hash_Size(n) returns the number of words in an object of n bytes 
 */

#define	Hash_Size(n)	(((n) + sizeof (int) - 1) / sizeof (int))

/*
 * The following procedure declarations and macros
 * are the only things that should be needed outside
 * the implementation code.
 */

extern Hash_Entry *	Hash_CreateEntry _ANSI_ARGS_((Hash_Table *,
						      char *, int *));
extern void		Hash_DeleteTable _ANSI_ARGS_((Hash_Table *));
extern void		Hash_DeleteEntry _ANSI_ARGS_((Hash_Table *,
						      Hash_Entry *));
extern Hash_Entry *	Hash_EnumFirst _ANSI_ARGS_((Hash_Table *,
						    Hash_Search *));
extern Hash_Entry *	Hash_EnumNext _ANSI_ARGS_((Hash_Search *));
extern Hash_Entry *	Hash_FindEntry _ANSI_ARGS_((Hash_Table *, char *));
extern void		Hash_InitTable _ANSI_ARGS_((Hash_Table *,
						    int, int));
extern void		Hash_PrintStats _ANSI_ARGS_((Hash_Table *,
						     void (*)(),
						     int *));

#endif /* _HASH */
