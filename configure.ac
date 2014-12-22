dnl configure.ac for pwld

AC_INIT([pwld], [0.1], [joakim.jalap@gmail.com])
AM_INIT_AUTOMAKE([-Wall -Werror foreign])
AC_PROG_CXX
AC_CONFIG_HEADERS([config.h])
dnl pidfile is in libutil in FreeBSD, libbsd in Arch Linux
AC_SEARCH_LIBS([pidfile_open], [bsd util], [], [
  AC_MSG_ERROR([unable to find pidfile_open()])
])
AC_SEARCH_LIBS([XkbOpenDisplay], [X11], [], [
  AC_MSG_ERROR([unable to find XkbOpenDisplay()])
])
AC_CONFIG_FILES([Makefile src/Makefile doc/Makefile])
AC_OUTPUT
