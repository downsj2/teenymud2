#include "config.h"

#if !defined(HAVE_VPRINTF) || !defined(HAVE_SNPRINTF)

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

#include <sys/types.h>
#include <stdio.h>
#ifdef HAVE_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif
#include <ctype.h>

/* Portable vfprintf  by Robert A. Larson <blarson@skat.usc.edu> */
/*          Hacked up by Jason Downs <downsj@xenotropic.com> */

/* Copyright 1989 Robert A. Larson.
 * Distribution in any form is allowed as long as the author
 * retains credit, changes are noted by their author and the
 * copyright message remains intact.  This program comes as-is
 * with no warentee of fitness for any purpouse.
 *
 * Thanks to Doug Gwen, Chris Torek, and others who helped clarify
 * the ansi printf specs.
 *
 * Please send any bug fixes and improvments to blarson@skat.usc.edu .
 * The use of goto is NOT a bug.
 */

/* Feb	7, 1989		blarson		First usenet release */

/* This code implements the vsprintf function, without relying on
 * the existance of _doprint or other system specific code.
 *
 * Define NOVOID if void * is not a supported type.
 *
 * Two compile options are available for efficency:
 *	INTSPRINTF	should be defined if sprintf is int and returns
 *			the number of chacters formated.
 *	LONGINT		should be defined if sizeof(long) == sizeof(int)
 *
 *	They only make the code smaller and faster, they need not be
 *	defined.
 *
 * UNSIGNEDSPECIAL should be defined if unsigned is treated differently
 * than int in argument passing.  If this is definded, and LONGINT is not,
 * the compiler must support the type unsingned long.
 *
 * Most quirks and bugs of the available sprintf fuction are duplicated,
 * however * in the width and precision fields will work correctly
 * even if sprintf does not support this, as will the n format.
 *
 * Bad format strings, or those with very long width and precision
 * fields (including expanded * fields) will cause undesired results.
 */

#if (SIZEOF_INT == SIZEOF_LONG)
#define LONGINT
#else
#undef LONGINT
#endif
#if defined(aiws)
#define INTSPRINTF
#else
#undef INTSPRINTF
#endif

#define UNSIGNEDSPECIAL

#ifdef	INTSPRINTF
#define Sprintf(string,format,arg)	(sprintf((string),(format),(arg)))
#else
#define Sprintf(string,format,arg)	(\
	sprintf((string),(format),(arg)),\
	strlen(string)\
)
#endif

#if defined(HAVE_STDARG_H) && defined(__STDC__)
#include <stdarg.h>
#else
#include <varargs.h>
#endif

typedef int *intp;

#if !defined(HAVE_VPRINTF)

#if defined(HAVE_STDARG_H) && defined(__STDC__)
int vsprintf(char *dest, const char *format, ...)
#else
int vsprintf(dest, format, va_alist)
char *dest;
const char *format;
va_dcl
#endif
{
    register char *dp = dest;
    register char c;
    register char *tp;
    char tempfmt[64];
#ifndef LONGINT
    int longflag;
#endif
    va_list args;

#if defined(HAVE_STDARG_H) && defined(__STDC__)
    va_start(args, format);
#else
    va_start(args);
#endif

    tempfmt[0] = '%';
    while((c = *format++)) {
	if(c=='%') {
	    tp = &tempfmt[1];
#ifndef LONGINT
	    longflag = 0;
#endif
continue_format:
	    switch((c = *format++)) {
		case 's':
		    *tp++ = c;
		    *tp = '\0';
		    dp += Sprintf(dp, tempfmt, va_arg(args, char *));
		    break;
		case 'u':
		case 'x':
		case 'o':
		case 'X':
#ifdef UNSIGNEDSPECIAL
		    *tp++ = c;
		    *tp = '\0';
#ifndef LONGINT
		    if(longflag)
			dp += Sprintf(dp, tempfmt, va_arg(args, unsigned long));
		    else
#endif
			dp += Sprintf(dp, tempfmt, va_arg(args, unsigned));
		    break;
#endif
		case 'd':
		case 'c':
		case 'i':
		    *tp++ = c;
		    *tp = '\0';
#ifndef LONGINT
		    if(longflag)
			dp += Sprintf(dp, tempfmt, va_arg(args, long));
		    else
#endif
			dp += Sprintf(dp, tempfmt, va_arg(args, int));
		    break;
		case 'f':
		case 'e':
		case 'E':
		case 'g':
		case 'G':
		    *tp++ = c;
		    *tp = '\0';
		    dp += Sprintf(dp, tempfmt, va_arg(args, double));
		    break;
		case 'p':
		    *tp++ = c;
		    *tp = '\0';
		    dp += Sprintf(dp, tempfmt, va_arg(args, VOID *));
		    break;
		case '-':
		case '+':
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
		case '.':
		case ' ':
		case '#':
		case 'h':
		    *tp++ = c;
		    goto continue_format;
		case 'l':
#ifndef LONGINT
		    longflag = 1;
		    *tp++ = c;
#endif
		    goto continue_format;
		case '*':
		    tp += Sprintf(tp, "%d", va_arg(args, int));
		    goto continue_format;
		case 'n':
		    *va_arg(args, intp) = dp - dest;
		    break;
		case '%':
		default:
		    *dp++ = c;
		    break;
	    }
	} else *dp++ = c;
    }
    *dp = '\0';

    va_end(args);

    return(dp - dest);
}

#if defined(HAVE_STDARG_H) && defined(__STDC__)
int vfprintf(FILE *dest, const char *format, ...)
#else
int vfprintf(dest, format, va_alist)
FILE *dest;
const char *format;
va_dcl
#endif
{
    register char c;
    register char *tp;
    register int count = 0;
    char tempfmt[64];
#ifndef LONGINT
    int longflag;
#endif
    va_list args;

#if defined(HAVE_STDARG_H) && defined(__STDC__)
    va_start(args, format);
#else
    va_start(args);
#endif

    tempfmt[0] = '%';
    while((c = *format++)) {
	if(c=='%') {
	    tp = &tempfmt[1];
#ifndef LONGINT
	    longflag = 0;
#endif
continue_format:
	    switch((c = *format++)) {
		case 's':
		    *tp++ = c;
		    *tp = '\0';
		    count += fprintf(dest, tempfmt, va_arg(args, char *));
		    break;
		case 'u':
		case 'x':
		case 'o':
		case 'X':
#ifdef UNSIGNEDSPECIAL
		    *tp++ = c;
		    *tp = '\0';
#ifndef LONGINT
		    if(longflag)
			count += fprintf(dest, tempfmt, va_arg(args, unsigned long));
		    else
#endif
			count += fprintf(dest, tempfmt, va_arg(args, unsigned));
		    break;
#endif
		case 'd':
		case 'c':
		case 'i':
		    *tp++ = c;
		    *tp = '\0';
#ifndef LONGINT
		    if(longflag)
			count += fprintf(dest, tempfmt, va_arg(args, long));
		    else
#endif
			count += fprintf(dest, tempfmt, va_arg(args, int));
		    break;
		case 'f':
		case 'e':
		case 'E':
		case 'g':
		case 'G':
		    *tp++ = c;
		    *tp = '\0';
		    count += fprintf(dest, tempfmt, va_arg(args, double));
		    break;
		case 'p':
		    *tp++ = c;
		    *tp = '\0';
		    count += fprintf(dest, tempfmt, va_arg(args, VOID *));
		    break;
		case '-':
		case '+':
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
		case '.':
		case ' ':
		case '#':
		case 'h':
		    *tp++ = c;
		    goto continue_format;
		case 'l':
#ifndef LONGINT
		    longflag = 1;
		    *tp++ = c;
#endif
		    goto continue_format;
		case '*':
		    tp += Sprintf(tp, "%d", va_arg(args, int));
		    goto continue_format;
		case 'n':
		    *va_arg(args, intp) = count;
		    break;
		case '%':
		default:
		    putc(c, dest);
		    count++;
		    break;
	    }
	} else {
	    putc(c, dest);
	    count++;
	}
    }
    va_end(args);

    return(count);
}

#if defined(HAVE_STDARG_H) && defined(__STDC__)
int vprintf(char *format, ...)
#else
int vprintf(format, va_alist)
char *format;
va_dcl
#endif
{
    va_list args;

    return(vfprintf(stdout, format, args));
}
#endif

#if !defined(HAVE_SNPRINTF)
#if defined(HAVE_STDARG_H) && defined(__STDC__)
int snprintf(char *dest, size_t size, const char *format, ...)
#else
int snprintf(dest, size, format, va_alist)
char *dest;
size_t size;
const char *format;
va_dcl
#endif
{
    register char *dp = dest;
    register char c;
    register char *tp, *sa;
    char tempfmt[64], tempbuf[128];
#ifndef LONGINT
    int longflag;
#endif
    int outspace = 0;
    va_list args;

#if defined(HAVE_STDARG_H) && defined(__STDC__)
    va_start(args, format);
#else
    va_start(args);
#endif

    tempfmt[0] = '%';
    while((c = *format++) && !outspace) {
	if(c=='%') {
	    tp = &tempfmt[1];
#ifndef LONGINT
	    longflag = 0;
#endif
continue_format:
	    switch((c = *format++)) {
		case 's': {
		    int padding;
		    register char *ptr, *buff;

		    *tp++ = c;
		    *tp = '\0';

		    /* Scan for padding. */
		    for(ptr = tempfmt; *ptr && !isdigit(*ptr); ptr++);
		    if(isdigit(*ptr)) {
		        padding = (int)strtol(ptr, NULL, 10);
			if(padding < 0)
			    padding = abs(padding);
		    } else
			padding = 0;

		    sa = va_arg(args, char *);

		    if(padding) {
			padding = (padding > strlen(sa)) ? padding : strlen(sa);

		        /* Allocate a buffer to hold the return. */
		        buff = (char *)alloca(padding+1);
		        if(buff == (char *)NULL)
			    return(-1);	/* XXX */

		        /* Call sprintf() in order to get padding right. */
		        sprintf(buff, tempfmt, sa);

		        if(strlen(buff) >= (size - (dp - dest))) {
		            buff[size - ((dp - dest)+1)] = '\0';
		            outspace++;
		        }
		        strcpy(dp, buff);
			dp += strlen(buff);
		    } else {
			/* We can't write into sa! */
			ptr = sa;
			while(*ptr && ((dp - dest) < (size-1)))
			    *dp++ = *ptr++;
			/* We don't terminate, but we do check for overflow. */
			if((dp - dest) >= size)
			    outspace++;
		    }
		}   break;
		case 'u':
		case 'x':
		case 'o':
		case 'X':
#ifdef UNSIGNEDSPECIAL
		    *tp++ = c;
		    *tp = '\0';
#ifndef LONGINT
		    if(longflag)
			sprintf(tempbuf, tempfmt, va_arg(args, unsigned long));
		    else
#endif
			sprintf(tempbuf, tempfmt, va_arg(args, unsigned));
		    if(strlen(tempbuf) >= (size - (dp - dest))) {
			tempbuf[size - ((dp - dest)+1)] = '\0';
			outspace++;
		    }
		    strcpy(dp, tempbuf);
		    dp += strlen(tempbuf);
		    break;
#endif
		case 'd':
		case 'c':
		case 'i':
		    *tp++ = c;
		    *tp = '\0';
#ifndef LONGINT
		    if(longflag)
			sprintf(tempbuf, tempfmt, va_arg(args, long));
		    else
#endif
			sprintf(tempbuf, tempfmt, va_arg(args, int));
		    if(strlen(tempbuf) >= (size - (dp - dest))) {
			tempbuf[size - ((dp - dest)+1)] = '\0';
			outspace++;
		    }
		    strcpy(dp, tempbuf);
		    dp += strlen(tempbuf);
		    break;
		case 'f':
		case 'e':
		case 'E':
		case 'g':
		case 'G':
		    *tp++ = c;
		    *tp = '\0';

		    sprintf(tempbuf, tempfmt, va_arg(args, double));
		    if(strlen(tempbuf) >= (size - (dp - dest))) {
			tempbuf[size - ((dp - dest)+1)] = '\0';
			outspace++;
		    }
		    strcpy(dp, tempbuf);
		    dp += strlen(tempbuf);
		    break;
		case 'p':
		    *tp++ = c;
		    *tp = '\0';

		    sprintf(tempbuf, tempfmt, va_arg(args, VOID *));
		    if(strlen(tempbuf) >= (size - (dp - dest))) {
			tempbuf[size - ((dp - dest)+1)] = '\0';
			outspace++;
		    }
		    strcpy(dp, tempbuf);
		    dp += strlen(tempbuf);
		    break;
		case '-':
		case '+':
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
		case '.':
		case ' ':
		case '#':
		case 'h':
		    *tp++ = c;
		    goto continue_format;
		case 'l':
#ifndef LONGINT
		    longflag = 1;
		    *tp++ = c;
#endif
		    goto continue_format;
		case '*':
		    tp += Sprintf(tp, "%d", va_arg(args, int));
		    goto continue_format;
		case 'n':
		    *va_arg(args, intp) = dp - dest;
		    break;
		case '%':
		default:
		    if((size - (dp - dest)) > 1)
		        *dp++ = c;
		    else
			outspace++;
		    break;
	    }
	} else {
	    if((size - (dp - dest)) > 1)
	        *dp++ = c;
	    else
		outspace++;
	}
    }
    *dp = '\0';

    va_end(args);

    return(dp - dest);
}
#endif
#endif
