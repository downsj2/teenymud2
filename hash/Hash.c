#include "config.h"

/* 
 * Hash.c --
 *
 *	Source code for Hash, a utility procedure used by the hash
 *	table library.
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

/* AIX requires this to be the first thing in the file. */
#ifdef __GNUC__
#define alloca	__builtin_alloca
#else	/* not __GNUC__ */
#ifdef HAVE_ALLOCA_H
#include <alloca.h>
#else	/* not HAVE_ALLOCA_H */
#ifdef _AIX
 #pragma alloca
#endif	/* not _AIX */
#endif	/* not HAVE_ALLOCA_H */
#endif 	/* not __GNUC__ */

#include "hash.h"
#include "list.h"

extern char to_lower _ANSI_ARGS_((char));


/*
 *---------------------------------------------------------
 *
 * Hash --
 *	This is a local procedure to compute a hash table
 *	bucket address based on a string value.
 *
 * Results:
 *	The return value is an integer between 0 and size - 1.
 *
 * Side Effects:	
 *	None.
 *
 * Design:
 *	It is expected that most keys are decimal numbers,
 *	so the algorithm behaves accordingly.  The randomizing
 *	code is stolen straight from the rand library routine.
 *
 *---------------------------------------------------------
 */

int
Hash(tablePtr, key)
    register Hash_Table *tablePtr;
    register char 	*key;
{
    register int 	i = 0;
    register int 	j;
    register int 	*intPtr;
    register char	*ptr;

    switch (tablePtr->keyType) {
	case HASH_STRCASE_KEYS:
	    ptr = (char *)alloca(strlen(key)+1);
	    for(j = 0; key[j]; ptr[j] = to_lower(key[j]), j++);
	    ptr[j] = 0;
	    key = ptr;
	case HASH_STRING_KEYS:
	    while (*key != 0) {
		i = (i * 10) + (*key++ - '0');
	    }
	    break;
	case HASH_ONE_WORD_KEYS:
	    i = (int) key;
	    break;
	case 2:
	    i = ((int *) key)[0] + ((int *) key)[1];
	    break;
	default:
	    j = tablePtr->keyType;
	    intPtr = (int *) key;
	    do { 
		i += *intPtr++; 
		j--;
	    } while (j > 0);
	    break;
    }


    return(((i*1103515245 + 12345) >> tablePtr->downShift) & tablePtr->mask);
}
