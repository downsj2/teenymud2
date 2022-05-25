AC_INIT
AC_CONFIG_SRCDIR([conf.c])
AC_CONFIG_HEADERS([autoconf.h])
AC_REVISION(: Configure for TeenyMUD 2.x)
AC_PREREQ([2.71])
dnl System stuff
AC_USE_SYSTEM_EXTENSIONS
dnl Specials
AC_SEARCH_LIBS([strerror],[cposix])
dnl Compiler
AC_PROG_CC
AC_PROG_CPP
dnl Cross Compiling

if test $cross_compiling = yes; then
	echo "Cross compiling is NOT supported by this package.  Sorry."
	exit 0
fi
dnl Programs
AC_CHECK_PROG(LORDER, lorder, lorder, echo)
AC_PROG_YACC
AC_PROG_RANLIB
AC_PROG_MAKE_SET
dnl Compiler stuff
AC_PROG_GCC_TRADITIONAL
AC_C_CONST
AC_C_INLINE
AC_C_BIGENDIAN
AC_MSG_CHECKING(for void pointer support)
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <stdio.h>
void test_func1() { /* innocent whistle */ }
void test_func2() { /* innocent whistle */ }

struct table {
  void (*function)();
};

struct table Table[] = {
  test_func1, test_func2, NULL
};]], [[
  register void *ptr;	/* register pointer. */
  char buf[80];

  ptr = (void *)malloc(1);	/* cast. */
  free(ptr);

  /* assign. */
  ptr = (void *)&buf[0];

  /* void function pointers. */
  (Table[0].function)();
  (Table[1].function)();
]])],[AC_DEFINE(HAVE_VOID_PTR) AC_MSG_RESULT(sure)],[AC_MSG_RESULT(nada)])
dnl Headers
AC_CHECK_HEADERS(alloca.h stdlib.h string.h unistd.h sys/select.h malloc.h \
		sys/wait.h limits.h sys/time.h sys/params.h varargs.h \
		dirent.h sys/ptem.h sys/pte.h sys/stream.h stdarg.h getopt.h)
AC_CHECK_HEADER(sys/file.h, ,AC_DEFINE(NO_SYS_FILE))
AC_MSG_CHECKING(for TIOCGWINSZ in sys/ioctl.h)
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <sys/types.h>
#include <sys/ioctl.h>]], [[int x = TIOCGWINSZ;]])],[AC_DEFINE(GWINSZ_IN_SYS_IOCTL) AC_MSG_RESULT(found)],[AC_MSG_RESULT(not found)])
AC_MSG_CHECKING(for sys/time.h and sys/select.h clash)
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <sys/types.h>]], [[#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#ifdef HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif]])],[AC_MSG_RESULT(none)],[AC_DEFINE(NO_SELECT_H) AC_MSG_RESULT(yes)])
dnl Types
AC_TYPE_SIZE_T
AC_TYPE_OFF_T
AC_CHECK_SIZEOF(off_t)
AC_CHECK_SIZEOF(long)
AC_CHECK_SIZEOF(int)
AC_CHECK_SIZEOF(int *)
AC_CHECK_SIZEOF(time_t)
dnl Libraries
AC_CHECK_LIB(m, main)
AC_CHECK_LIB(net, main)
dnl (this is for DYNIX/ptx...)
AC_CHECK_LIB(seq, main)
AC_CHECK_LIB(socket, connect,
	     AC_CHECK_LIB(socket, connect) \
	     AC_CHECK_LIB(inet, main) \
	     AC_CHECK_LIB(nsl, main), , -linet -lnsl -lc)
dnl (...end DYNIX/ptx)
dnl (this is for Solaris...)
unset ac_cv_lib_socket
AC_CHECK_LIB(socket, connect,
     	     AC_CHECK_LIB(socket, connect) \
     	     AC_CHECK_LIB(nsl, main), , -lnsl -lc)
dnl (...end Solaris)
dnl (this is for SCO...)
unset ac_cv_lib_socket
AC_CHECK_LIB(socket, socket, , , -lc)
dnl (...SCO)
AC_CHECK_LIB(nsl_s, main)
AC_CHECK_LIB(bsd, main)
AC_CHECK_LIB(resolv, gethostbyaddr, , , -lc)
dnl Library routines
AC_CHECK_FUNCS(fmod gettimeofday setrlimit tzset setsid \
	       on_exit atexit getrusage getloadavg \
	       gethostname uname getopt_long strlcpy strlcat)
dnl Structures
AC_STRUCT_TM
AC_STRUCT_TIMEZONE
AC_MSG_CHECKING(for struct linger)
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <stdio.h>
#include <sys/signal.h>]], [[struct linger ll;]])],[AC_MSG_RESULT(found)],[AC_DEFINE(NO_STRUCT_LINGER) AC_MSG_RESULT(not found)])
dnl AC_MSG_CHECKING(for ancient in_addr)
dnl AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <stdio.h>
dnl #include <netinet/in.h>
dnl #include <arpa/inet.h>]], [[dnl	       struct in_addr ina; ina.S_un.S_addr = 0x00000000;]])],[dnl	       AC_MSG_RESULT(you poor soul) AC_DEFINE(HAVE_ANCIENT_TCP)],[dnl	       AC_MSG_RESULT(no)])
dnl Extras
AC_MSG_CHECKING(whether you have SystemV or BSD signals)
AC_RUN_IFELSE([AC_LANG_SOURCE([[#include <signal.h>
int count = 0;
RETSIGTYPE sighandle() {
  count++;
  if(count == 2)
    exit(0);			/* BSD */
}
main() {
  signal(SIGINT, sighandle);
  kill(getpid(), SIGINT);	/* definately ignored. */
  kill(getpid(), SIGINT);	/* terminated under SysV */
  exit(1);
}]])],[AC_MSG_RESULT(bsd)],[AC_DEFINE(HAVE_SYSV_SIGNALS) AC_MSG_RESULT(sysv)],[])
dnl Database configuration
unset ac_cv_lib_gdbm
AC_CHECK_LIB(gdbm, gdbm_open,
	     [dbm_lib="-lgdbm"
	      xdbm_lib="gdbm"], dbm_lib="-lgdbm")
if test "${ac_cv_lib_gdbm}" = no; then
	AC_MSG_WARN(no working GDBM library found.)
	xdbm_lib="NONE"
fi
if test "${xdbm_lib}" != "NONE"; then
	unset ac_cv_lib_gdbm
	AC_CHECK_LIB($xdbm_lib, gdbm_setopt, true,
	     	     AC_MSG_WARN(this package requires at least GDBM version 1.7.1.))
fi
dnl Done
AC_CONFIG_FILES([Makefile compat/Makefile sha/Makefile])
AC_OUTPUT
