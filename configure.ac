dnl Process this file with autoconf to produce a configure script.

AC_INIT([rtcom-eventlogger-plugins], [1.3])

LT_CURRENT=1
LT_REVISION=0
LT_AGE=0

AM_INIT_AUTOMAKE
AM_CONFIG_HEADER(config.h)

CFLAGS="$CFLAGS -Wall -Werror -Wcast-align -DDMALLOC"

PKG_CHECK_MODULES(
	RTCOM_EVENTLOGGER_PLUGINS,
	glib-2.0
	gobject-2.0
	gmodule-2.0
	rtcom-eventlogger >= 1.0,
	sqlite3 >= 3.3)
AC_SUBST(RTCOM_EVENTLOGGER_PLUGINS_CFLAGS)
AC_SUBST(RTCOM_EVENTLOGGER_PLUGINS_LIBS)

AC_ISC_POSIX
AC_PROG_CC
AM_PROG_CC_STDC
AC_HEADER_STDC
AC_PROG_INSTALL
AC_PROG_LIBTOOL

AC_ARG_ENABLE(debug,        [  --enable-debug           compile with DEBUG],,enable_debug=no)

if test "x$enable_debug" = "xyes"; then
        DEBUG_FLAGS="-DDEBUG"
fi

AC_ARG_ENABLE(cast-checks,      [  --disable-cast-checks         compile with GLIB cast checks disabled],[cchecks=${enableval}],cchecks=yes)
if test "x$cchecks" = "xno"; then
        DEBUG_FLAGS="$DEBUG_FLAGS -DG_DISABLE_CAST_CHECKS"
fi

AC_ARG_ENABLE(asserts,      [  --disable-asserts        compile with GLIB assertions disabled],[asserts=${enableval}],asserts=yes)
if test "x$asserts" = "xno"; then
        DEBUG_FLAGS="$DEBUG_FLAGS -DG_DISABLE_ASSERTS"
fi

AC_ARG_ENABLE(checks,       [  --disable-checks         compile with GLIB checks disabled],[checks=${enableval}],checks=yes)
if test "x$checks" = "xno"; then
        DEBUG_FLAGS="$DEBUG_FLAGS -DG_DISABLE_CHECKS"
fi

AC_ARG_ENABLE(debug,
              [ --enable-debug enable printing of debug messages],
              [ddebug=${enableval}],ddebug=no)
if test "x$ddebug" != "xyes"; then
    CFLAGS="$CFLAGS -DG_DEBUG_DISABLE"
fi

AC_SUBST([LT_CURRENT])
AC_SUBST([LT_REVISION])
AC_SUBST([LT_AGE])

AC_OUTPUT([
	rtcom-eventlogger-plugins.pc
	Makefile
	src/Makefile
])

