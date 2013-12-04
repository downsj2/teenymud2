#include "config.h"

/* 
 * Hash_CreateEntry.c --
 *
 *	Source code for the Hash_CreateEntry library procedure.
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
#endif				/* HAVE_STDLIB_H */
#ifdef HAVE_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif				/* HAVE_STRING_H */
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
 * The following defines the ratio of # entries to # buckets
 * at which we rebuild the table to make it larger.
 */

static rebuildLimit = 3;

/*
 *---------------------------------------------------------
 *
 * RebuildTable --
 *	This local routine makes a new hash table that
 *	is larger than the old one.
 *
 * Results:	
 * 	None.
 *
 * Side Effects:
 *	The entire hash table is moved, so any bucket numbers
 *	from the old table are invalid.
 *
 *---------------------------------------------------------
 */

static void
RebuildTable(tablePtr)
    Hash_Table 	*tablePtr;		/* Table to be enlarged. */
{
    int 		 oldSize;
    int 		 bucket;
    List_Links		 *oldBucketPtr;
    register Hash_Entry  *hashEntryPtr;
    register List_Links	 *oldHashList;

    oldBucketPtr = tablePtr->bucketPtr;
    oldSize = tablePtr->size;

    /* 
     * Build a new table 4 times as large as the old one. 
     */

    Hash_InitTable(tablePtr, tablePtr->size * 4, tablePtr->keyType);

    for (oldHashList = oldBucketPtr; oldSize > 0; oldSize--, oldHashList++) {
	while (!List_IsEmpty(oldHashList)) {
	    hashEntryPtr = (Hash_Entry *) List_First(oldHashList);
	    List_Remove((List_Links *) hashEntryPtr);
	    switch (tablePtr->keyType) {
		case HASH_STRING_KEYS:
		case HASH_STRCASE_KEYS:
		    bucket = Hash(tablePtr, (char *) hashEntryPtr->key.name);
		    break;
		case HASH_ONE_WORD_KEYS:
		    bucket = Hash(tablePtr, (char *) hashEntryPtr->key.ptr);
		    break;
		default:
		    bucket = Hash(tablePtr, (char *) hashEntryPtr->key.words);
		    break;
	    }
	    List_Insert((List_Links *) hashEntryPtr, 
		LIST_ATFRONT(&(tablePtr->bucketPtr[bucket])));
	    tablePtr->numEntries++;
	}
    }

    free((VOID *) oldBucketPtr);
}

/*
 *---------------------------------------------------------
 *
 * Hash_CreateEntry --
 *
 *	Searches a hash table for an entry corresponding to
 *	key.  If no entry is found, then one is created.
 *
 * Results:
 *	The return value is a pointer to the entry.  If *newPtr
 *	isn't NULL, then *newPtr is filled in with TRUE if a
 *	new entry was created, and FALSE if an entry already existed
 *	with the given key.
 *
 * Side Effects:
 *	Memory may be allocated, and the hash buckets may be modified.
 *---------------------------------------------------------
 */

Hash_Entry *
Hash_CreateEntry(tablePtr, key, newPtr)
    register Hash_Table *tablePtr;	/* Hash table to search. */
    char *key;			/* A hash key. */
    int *newPtr;			/* Filled in with TRUE if new entry
    					 * created, FALSE otherwise. */
{
    register Hash_Entry *hashEntryPtr;
    register int 	*hashKeyPtr;
    register int 	*keyPtr;
    List_Links 		*hashList;
    register int	 j;

    keyPtr = (int *) key;

    hashList = &(tablePtr->bucketPtr[Hash(tablePtr, (char *) keyPtr)]);
    hashEntryPtr = HashChainSearch(tablePtr, (char *) keyPtr, hashList);

    if (hashEntryPtr != (Hash_Entry *) NULL) {
	if (newPtr != NULL) {
	    *newPtr = FALSE;
	}
    	return hashEntryPtr;
    }

    /* 
     * The desired entry isn't there.  Before allocating a new entry,
     * see if we're overloading the buckets.  If so, then make a
     * bigger table.
     */

    if (tablePtr->numEntries >= rebuildLimit * tablePtr->size) {
	RebuildTable(tablePtr);
	hashList = &(tablePtr->bucketPtr[Hash(tablePtr, (char *) keyPtr)]);
    }
    tablePtr->numEntries += 1;

    /*
     * Not there, we have to allocate.  If the string is longer
     * than 3 bytes, then we have to allocate extra space in the
     * entry.
     */

    switch (tablePtr->keyType) {
	case HASH_STRCASE_KEYS:
	    hashEntryPtr = (Hash_Entry *) malloc((unsigned)
		    (sizeof(Hash_Entry) + strlen((char *) keyPtr) - 3));
	    for(j = 0; ((char *)keyPtr)[j];
		    hashEntryPtr->key.name[j] = ((char *)keyPtr)[j], j++);
	    hashEntryPtr->key.name[j] = 0;
	    break;
	case HASH_STRING_KEYS:
	    hashEntryPtr = (Hash_Entry *) malloc((unsigned)
		    (sizeof(Hash_Entry) + strlen((char *) keyPtr) - 3));
	    strcpy(hashEntryPtr->key.name, (char *) keyPtr);
	    break;
	case HASH_ONE_WORD_KEYS:
	    hashEntryPtr = (Hash_Entry *) malloc(sizeof(Hash_Entry));
	    hashEntryPtr->key.ptr = (char *) keyPtr;
	    break;
	case 2:
	    hashEntryPtr = 
		(Hash_Entry *) malloc(sizeof(Hash_Entry) + sizeof(int));
	    hashKeyPtr = hashEntryPtr->key.words;
	    *hashKeyPtr++ = *keyPtr++;
	    *hashKeyPtr = *keyPtr;
	    break;
	default: {
	    register int n;

	    n = tablePtr->keyType;
	    hashEntryPtr = (Hash_Entry *) 
		    malloc((unsigned) (sizeof(Hash_Entry)
		    + (n - 1) * sizeof(int)));
	    hashKeyPtr = hashEntryPtr->key.words;
	    do { 
		*hashKeyPtr++ = *keyPtr++; 
	    } while (--n);
	    break;
	}
    }

    hashEntryPtr->clientData = (int *) NULL;
    List_Insert((List_Links *) hashEntryPtr, LIST_ATFRONT(hashList));

    if (newPtr != NULL) {
	*newPtr = TRUE;
    }
    return hashEntryPtr;
}
