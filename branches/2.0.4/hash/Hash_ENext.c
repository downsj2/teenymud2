#include "config.h"

/* 
 * Hash_EnumNext.c --
 *
 *	Source code for the Hash_EnumNext library procedure.
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

#include "hash.h"
#include "list.h"

/*
 *---------------------------------------------------------
 *
 * Hash_EnumNext --
 *    This procedure returns successive entries in the hash table.
 *
 * Results:
 *    The return value is a pointer to the next HashEntry
 *    in the table, or NULL when the end of the table is
 *    reached.
 *
 * Side Effects:
 *    The information in hashSearchPtr is modified to advance to the
 *    next entry.
 *
 *---------------------------------------------------------
 */

Hash_Entry *
Hash_EnumNext(hashSearchPtr)
    register Hash_Search *hashSearchPtr; /* Area used to keep state about 
					    search. */
{
    register List_Links *hashList;
    register Hash_Entry *hashEntryPtr;

    hashEntryPtr = hashSearchPtr->hashEntryPtr;
    while (hashEntryPtr == (Hash_Entry *) NULL ||
	   List_IsAtEnd(hashSearchPtr->hashList,
	   (List_Links *) hashEntryPtr)) {
	if (hashSearchPtr->nextIndex >= hashSearchPtr->tablePtr->size) {
	    return((Hash_Entry *) NULL);
	}
	hashList = &(hashSearchPtr->tablePtr->bucketPtr[
		hashSearchPtr->nextIndex]);
	hashSearchPtr->nextIndex++;
	if (!List_IsEmpty(hashList)) {
	    hashEntryPtr = (Hash_Entry *) List_First(hashList);
	    hashSearchPtr->hashList = hashList;
	    break;
	}
    }

    hashSearchPtr->hashEntryPtr = 
		(Hash_Entry *) List_Next((List_Links *) hashEntryPtr);

    return(hashEntryPtr);
}
