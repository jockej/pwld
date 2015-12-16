dnl configure.ac for pwld

AC_INIT([pwld], [0.2], [joakim.jalap@gmail.com])
AM_INIT_AUTOMAKE([-Wall -Werror foreign])

AC_CONFIG_SRCDIR([config.h.in])
AC_CONFIG_HEADERS([config.h])

AC_CANONICAL_HOST

dnl Prefer clang on *BSD
case $host_os in
  *linux*)
    CCOMPS="gcc clang"
    CXXCOMPS="g++ clang++"
    ;;  
  *freebsd*)
    CCOMPS="clang gcc"
    CXXCOMPS="clang++ g++"
    ;;
  *openbsd*)
    CCOMPS="clang gcc"
    CXXCOMPS="clang++ g++"
    dnl LDFLAGS="$LDFLAGS -L/usr/X11R6/lib"
    dnl CPPFLAGS="$CPPFLAGS -I/usr/X11R6/include"
    ;;
  *)
    #Default Case
    AC_MSG_ERROR([Your platform is not currently supported])
    ;;
esac

AC_PROG_CXX([$CXXCOMPS])
AC_PROG_CC([$CCOMPS])

AC_PROG_INSTALL

# Checks for header files.
AC_CHECK_HEADERS([fcntl.h stdlib.h string.h syslog.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_CHECK_HEADER_STDBOOL
AC_C_INLINE
AC_TYPE_SIZE_T

# Checks for library functions.
AC_CHECK_FUNCS([atexit ftruncate select])

dnl Check for some variant of pidfile handling
AC_SEARCH_LIBS([pidfile_open], [bsd util], [
  AC_DEFINE([FREEBSD_PIDFILE], [],
    [Defined if we use the FreeBSD version of pidfile_open(3).])], [])

AC_SEARCH_LIBS([XkbOpenDisplay], [X11], [], [
  AC_MSG_ERROR([unable to find XkbOpenDisplay(3)])
])

AC_CONFIG_FILES([Makefile src/Makefile doc/Makefile])
AC_OUTPUT
