#include "config.h"

/* 
 * HashChainSearch.c --
 *
 *	Source code for HashChainSearch, a utility procedure used by
 *	the hash table library.
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
#ifdef HAVE_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif			/* HAVE_STRING_H */

#include "hash.h"
#include "list.h"

/*
 *---------------------------------------------------------
 *
 * HashChainSearch --
 *
 * 	Search the hash table for the entry in the hash chain.
 *
 * Results:
 *	Pointer to entry in hash chain, NULL if none found.
 *
 * Side Effects:
 *	None.
 *
 *---------------------------------------------------------
 */

Hash_Entry *
HashChainSearch(tablePtr, key, hashList)
    Hash_Table 		*tablePtr;	/* Hash table to search. */
    register char	*key;	/* A hash key. */
    register List_Links *hashList;
{
    register Hash_Entry *hashEntryPtr;
    register int 	*hashKeyPtr;
    int 		*keyPtr;
    register int	numKeys;

    numKeys = tablePtr->keyType;
#if defined(_AIX) && !defined(__GNUC__)
    LIST_FORALL(hashList, hashEntryPtr) {	/* XLC bug. */
#else
    LIST_FORALL(hashList, (List_Links *) hashEntryPtr) {
#endif
	switch (numKeys) {
	    case HASH_STRING_KEYS:
		if (strcmp(hashEntryPtr->key.name, key) == 0) {
		    return(hashEntryPtr);
		}
		break;
	    case HASH_STRCASE_KEYS:
		if (strcasecmp(hashEntryPtr->key.name, key) == 0) {
		    return(hashEntryPtr);
		}
		break;
	    case HASH_ONE_WORD_KEYS:
		if (hashEntryPtr->key.ptr == key) {
		    return(hashEntryPtr);
		}
		break;
	    case 2:
		hashKeyPtr = hashEntryPtr->key.words;
		keyPtr = (int *) key;
		if (*hashKeyPtr++ == *keyPtr++ && *hashKeyPtr == *keyPtr) {
		    return(hashEntryPtr);
		}
		break;
	    default:
		if (!bcmp((char *) hashEntryPtr->key.words,
			    (char *) key, numKeys * sizeof(int))) {
		    return(hashEntryPtr);
		}
		break;
	}
    }

    /* 
     * The desired entry isn't there 
     */

    return ((Hash_Entry *) NULL);
}
