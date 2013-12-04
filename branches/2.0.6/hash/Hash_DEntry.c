#include "config.h"

/* 
 * Hash_DeleteEntry.c --
 *
 *	Source code for the Hash_DeleteEntry library procedure.
 *
 * Copyright 1988 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */

#include <stdio.h>
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif			/* HAVE_STDLIB_H */
#ifdef HAVE_MALLOC_H
#include <malloc.h>
#endif			/* HAVE_MALLOC_H */

#include "hash.h"
#include "list.h"

/*
 * Utility procedures defined in other files:
 */

extern Hash_Entry *	HashChainSearch _ANSI_ARGS_((Hash_Table *,
						     char *,
						     List_Links *));
extern int		Hash _ANSI_ARGS_((Hash_Table *, char *));

/*
 *---------------------------------------------------------
 *
 * Hash_DeleteEntry --
 *
 * 	Delete the given hash table entry and free memory associated with
 *	it.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	Hash chain that entry lives in is modified and memory is freed.
 *
 *---------------------------------------------------------
 */

void
Hash_DeleteEntry(tablePtr, hashEntryPtr)
    Hash_Table			*tablePtr;
    register	Hash_Entry	*hashEntryPtr;
{
    if (hashEntryPtr != (Hash_Entry *) NULL) {
	List_Remove((List_Links *) hashEntryPtr);
	free((VOID *) hashEntryPtr);
	tablePtr->numEntries--;
    }
}
