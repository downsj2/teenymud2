/* autoconf.h */

#ifndef __AUTOCONF_H
@TOP@

/* Define if your stdlib.h (or another header) has a prototype for
   alloca. */
#undef HAVE_ALLOCA_DECL

/* Define if your stdlib.h (or another header) have prototypes for
   the getpwent suite. */
#undef HAVE_GETPW_DECLS
  
/* Define if you have <sys/mtio.h>.  */
#undef HAVE_SYS_MTIO_H

/* Define if you have SystemV style signals.  */
#undef HAVE_SYSV_SIGNALS

/* Define if `union wait' is the type of the first arg to wait functions.  */
#undef HAVE_UNION_WAIT

/* Define if your compiler supports void pointers.    */
#undef HAVE_VOID_PTR

/* Define if you have the POSIX.1 `waitpid' function.  */
#undef HAVE_WAITPID

/* Define this if you have no <sys/file.h>. */
#undef NO_SYS_FILE

/* Define this if TIOCGWINSZ is defined in <ioctl.h>. */
#undef GWINSZ_IN_SYS_IOCTL
 
/* The number of bytes in a off_t.  */
#undef SIZEOF_OFF_T 

@BOTTOM@
#define __AUTOCONF_H
#endif		/* __AUTOCONF_H */
