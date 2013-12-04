/* autoconf.h.  Generated automatically by configure.  */
/* autoconf.h.in.  Generated automatically from configure.in by autoheader.  */
/* autoconf.h */

#ifndef __AUTOCONF_H

/* Define if on AIX 3.
   System headers sometimes define this.
   We just want to avoid a redefinition error message.  */
#ifndef _ALL_SOURCE
/* #undef _ALL_SOURCE */
#endif

/* Define if using alloca.c.  */
/* #undef C_ALLOCA */

/* Define to empty if the keyword does not work.  */
/* #undef const */

/* Define to one of _getb67, GETB67, getb67 for Cray-2 and Cray-YMP systems.
   This function is required for alloca.c support on those systems.  */
/* #undef CRAY_STACKSEG_END */

/* Define if you have alloca, as a function or macro.  */
#define HAVE_ALLOCA 1

/* Define if you have <alloca.h> and it should be used (not on Ultrix).  */
/* #undef HAVE_ALLOCA_H */

/* Define if you don't have vprintf but do have _doprnt.  */
/* #undef HAVE_DOPRNT */

/* Define if you have the strftime function.  */
#define HAVE_STRFTIME 1

/* Define if your struct tm has tm_zone.  */
#define HAVE_TM_ZONE 1

/* Define if you don't have tm_zone but do have the external array
   tzname.  */
/* #undef HAVE_TZNAME */

/* Define if you have the vprintf function.  */
#define HAVE_VPRINTF 1

/* Define if you have the wait3 system call.  */
#define HAVE_WAIT3 1

/* Define as __inline if that's what the C compiler calls it.  */
/* #undef inline */

/* Define if on MINIX.  */
/* #undef _MINIX */

/* Define to `long' if <sys/types.h> doesn't define.  */
/* #undef off_t */

/* Define if the system does not provide POSIX.1 features except
   with this defined.  */
/* #undef _POSIX_1_SOURCE */

/* Define if you need to in order for stat and other things to work.  */
/* #undef _POSIX_SOURCE */

/* Define as the return type of signal handlers (int or void).  */
#define RETSIGTYPE void

/* Define to `unsigned' if <sys/types.h> doesn't define.  */
/* #undef size_t */

/* If using the C implementation of alloca, define if you know the
   direction of stack growth for your system; otherwise it will be
   automatically deduced at run-time.
	STACK_DIRECTION > 0 => grows toward higher addresses
	STACK_DIRECTION < 0 => grows toward lower addresses
	STACK_DIRECTION = 0 => direction of growth unknown
 */
/* #undef STACK_DIRECTION */

/* Define if you have the ANSI C header files.  */
#define STDC_HEADERS 1

/* Define if you can safely include both <sys/time.h> and <time.h>.  */
#define TIME_WITH_SYS_TIME 1

/* Define if your <sys/time.h> declares struct tm.  */
/* #undef TM_IN_SYS_TIME */

/* Define if your processor stores words with the most significant
   byte first (like Motorola and SPARC, unlike Intel and VAX).  */
/* #undef WORDS_BIGENDIAN */

/* Define if your stdlib.h (or another header) have prototypes for
   the getpwent suite. */
/* #undef HAVE_GETPW_DECLS */

/* Define if you have SystemV style signals.  */
/* #undef HAVE_SYSV_SIGNALS */

/* Define if `union wait' is the type of the first arg to wait functions.  */
#define HAVE_UNION_WAIT 1

/* Define if your compiler supports void pointers.    */
#define HAVE_VOID_PTR 1

/* Define this if you have no <sys/file.h>. */
/* #undef NO_SYS_FILE */

/* Define this if you really don't want to use <sys/select.h>. */
/* #undef NO_SELECT_H */

/* Define this if TIOCGWINSZ is defined in <ioctl.h>. */
#define GWINSZ_IN_SYS_IOCTL 1

/* Define this if SO_LINGER is defined, but not fully supported. */
#define NO_STRUCT_LINGER 1

/* Define this if you have a real struct in_addr. */
/* #undef HAVE_ANCIENT_TCP */

/* The number of bytes in a off_t.  */
#define SIZEOF_OFF_T 8 

/* The number of bytes in a int.  */
#define SIZEOF_INT 4

/* The number of bytes in a int *.  */
#define SIZEOF_INT_P 4

/* The number of bytes in a long.  */
#define SIZEOF_LONG 4

/* Define if you have the atexit function.  */
#define HAVE_ATEXIT 1

/* Define if you have the bcopy function.  */
#define HAVE_BCOPY 1

/* Define if you have the fmod function.  */
#define HAVE_FMOD 1

/* Define if you have the fsync function.  */
#define HAVE_FSYNC 1

/* Define if you have the ftime function.  */
/* #undef HAVE_FTIME */

/* Define if you have the getdtablesize function.  */
#define HAVE_GETDTABLESIZE 1

/* Define if you have the gethostname function.  */
#define HAVE_GETHOSTNAME 1

/* Define if you have the getloadavg function.  */
#define HAVE_GETLOADAVG 1

/* Define if you have the getrusage function.  */
#define HAVE_GETRUSAGE 1

/* Define if you have the gettimeofday function.  */
#define HAVE_GETTIMEOFDAY 1

/* Define if you have the mallopt function.  */
/* #undef HAVE_MALLOPT */

/* Define if you have the on_exit function.  */
/* #undef HAVE_ON_EXIT */

/* Define if you have the random function.  */
#define HAVE_RANDOM 1

/* Define if you have the setrlimit function.  */
#define HAVE_SETRLIMIT 1

/* Define if you have the setsid function.  */
#define HAVE_SETSID 1

/* Define if you have the sighold function.  */
/* #undef HAVE_SIGHOLD */

/* Define if you have the snprintf function.  */
#define HAVE_SNPRINTF 1

/* Define if you have the strcasecmp function.  */
#define HAVE_STRCASECMP 1

/* Define if you have the strchr function.  */
#define HAVE_STRCHR 1

/* Define if you have the strerror function.  */
#define HAVE_STRERROR 1

/* Define if you have the strstr function.  */
#define HAVE_STRSTR 1

/* Define if you have the strtod function.  */
#define HAVE_STRTOD 1

/* Define if you have the strtok function.  */
#define HAVE_STRTOK 1

/* Define if you have the strtol function.  */
#define HAVE_STRTOL 1

/* Define if you have the strtoul function.  */
#define HAVE_STRTOUL 1

/* Define if you have the sys_errlist function.  */
#define HAVE_SYS_ERRLIST 1

/* Define if you have the timezone function.  */
#define HAVE_TIMEZONE 1

/* Define if you have the tzset function.  */
#define HAVE_TZSET 1

/* Define if you have the uname function.  */
#define HAVE_UNAME 1

/* Define if you have the waitpid function.  */
#define HAVE_WAITPID 1

/* Define if you have the <dirent.h> header file.  */
#define HAVE_DIRENT_H 1

/* Define if you have the <limits.h> header file.  */
#define HAVE_LIMITS_H 1

/* Define if you have the <malloc.h> header file.  */
/* #undef HAVE_MALLOC_H */

/* Define if you have the <stdarg.h> header file.  */
#define HAVE_STDARG_H 1

/* Define if you have the <stdlib.h> header file.  */
#define HAVE_STDLIB_H 1

/* Define if you have the <string.h> header file.  */
#define HAVE_STRING_H 1

/* Define if you have the <sys/params.h> header file.  */
/* #undef HAVE_SYS_PARAMS_H */

/* Define if you have the <sys/pte.h> header file.  */
/* #undef HAVE_SYS_PTE_H */

/* Define if you have the <sys/ptem.h> header file.  */
/* #undef HAVE_SYS_PTEM_H */

/* Define if you have the <sys/select.h> header file.  */
#define HAVE_SYS_SELECT_H 1

/* Define if you have the <sys/stream.h> header file.  */
/* #undef HAVE_SYS_STREAM_H */

/* Define if you have the <sys/time.h> header file.  */
#define HAVE_SYS_TIME_H 1

/* Define if you have the <sys/wait.h> header file.  */
#define HAVE_SYS_WAIT_H 1

/* Define if you have the <termcap.h> header file.  */
/* #undef HAVE_TERMCAP_H */

/* Define if you have the <termio.h> header file.  */
/* #undef HAVE_TERMIO_H */

/* Define if you have the <unistd.h> header file.  */
#define HAVE_UNISTD_H 1

/* Define if you have the <varargs.h> header file.  */
#define HAVE_VARARGS_H 1

/* Define if you have the bsd library (-lbsd).  */
/* #undef HAVE_LIBBSD */

/* Define if you have the inet library (-linet).  */
/* #undef HAVE_LIBINET */

/* Define if you have the m library (-lm).  */
#define HAVE_LIBM 1

/* Define if you have the malloc library (-lmalloc).  */
/* #undef HAVE_LIBMALLOC */

/* Define if you have the net library (-lnet).  */
/* #undef HAVE_LIBNET */

/* Define if you have the nsl library (-lnsl).  */
/* #undef HAVE_LIBNSL */

/* Define if you have the nsl_s library (-lnsl_s).  */
/* #undef HAVE_LIBNSL_S */

/* Define if you have the resolv library (-lresolv).  */
#define HAVE_LIBRESOLV 1

/* Define if you have the seq library (-lseq).  */
/* #undef HAVE_LIBSEQ */

/* Define if you have the socket library (-lsocket).  */
/* #undef HAVE_LIBSOCKET */
#define __AUTOCONF_H
#endif		/* __AUTOCONF_H */
