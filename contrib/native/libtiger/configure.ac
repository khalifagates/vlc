AC_INIT([libtiger],[0.3.4])

AC_CONFIG_SRCDIR([include/tiger/tiger.h.in])
AC_CONFIG_AUX_DIR([misc/autotools])

AC_PREREQ([2.69])

AC_CANONICAL_TARGET

AM_INIT_AUTOMAKE([-Wall -Werror foreign subdir-objects])
AC_CONFIG_HEADERS([include/config.h])

ifdef([AM_SILENT_RULES],[AM_SILENT_RULES])

dnl  3. If the library source code has changed at all since the last
dnl     update, then increment REVISION (`C:R:A' becomes `C:r+1:A').
dnl
dnl  4. If any interfaces have been added, removed, or changed since the
dnl     last update, increment CURRENT, and set REVISION to 0.
dnl
dnl  5. If any interfaces have been added since the last public release,
dnl     then increment AGE.
dnl
dnl  6. If any interfaces have been removed since the last public release,
dnl     then set AGE to 0.

LIBTIGER_SHARED_VERSION="5:2:0"

AC_PROG_CC_C_O
AC_PROG_LIBTOOL
AC_PROG_GREP
AC_PROG_NM
AC_PROG_SED
AM_PROG_AR

AC_C_CONST
AC_C_INLINE


# headers

AC_HEADER_STDC
AC_CHECK_HEADERS([unistd.h time.h limits.h sys/types.h sys/select.h])
AC_CHECK_HEADERS([stdint.h])

AC_CHECK_FUNCS([select nanosleep usleep])

AC_TYPE_SIZE_T

AC_CHECK_PROG(HAVE_PKG_CONFIG,pkg-config,yes)

AC_ARG_ENABLE(doc, [  --disable-doc           Disable building documentation (default enabled)])
if test "x$enable_doc" != "xno"
then
  AC_CHECK_PROG(HAVE_DOXYGEN,doxygen,yes)
else
  HAVE_DOXYGEN="no"
fi
AM_CONDITIONAL(HAVE_DOXYGEN,test "${HAVE_DOXYGEN}" = "yes")

if test "x$HAVE_PKG_CONFIG" = "xyes"
then
  PKG_CHECK_MODULES(KATE,kate >= 0.2.0)
  PKG_CHECK_MODULES(PANGOCAIRO,pangocairo >= 1.16)
else
  AC_CHECK_LIB(kate,kate_get_version)
  AC_CHECK_HEADER([kate/kate.h])
  AC_CHECK_LIB(pango,pango_version)
  AC_CHECK_HEADER([pango/pango.h])
  AC_CHECK_LIB(cairo,cairo_version)
  AC_CHECK_HEADER([cairo/cairo.h])
  AC_CHECK_LIB(pangocairo,pango_cairo_update_context)
  AC_CHECK_HEADER([pango/pangocairo.h])
fi
 
AC_ARG_ENABLE(pthread, [  --disable-pthread       Builds single threaded library (default enabled)])

if test "x$enable_pthread" != "xno"
then
  AC_CHECK_LIB(
    pthread,
    pthread_create,
    [
      AC_CHECK_HEADER(
        [pthread.h],
        [HAVE_PTHREAD=yes],
        [HAVE_PTHREAD=no],
      )
    ],
    [HAVE_PTHREAD=no]
  )
fi
if test "x$HAVE_PTHREAD" = "xyes"
then
  AC_DEFINE(HAVE_PTHREAD, [1], [Define to 1 if pthread is available.])
  PTHREAD_CFLAGS=""
  PTHREAD_LIBS="-lpthread"
fi

#AC_ARG_ENABLE(valgrind, [  --enable-valgrind       Run the tests with Valgrind (default autodetect)])
#if test "x$enable_valgrind" != "xno"
#then
#  AC_PATH_PROG(VALGRIND,valgrind,)
#  if test "x$VALGRIND" != "x"
#  then
#    VALGRIND="$VALGRIND -q --leak-check=full --show-reachable=yes"
#  fi
#fi

VERSION_MAJOR=`echo $VERSION | $SED 's/\([[0-9]]*\)\.\([[0-9]]*\)\.\([[0-9]]*\)/\1/'`
VERSION_MINOR=`echo $VERSION | $SED 's/\([[0-9]]*\)\.\([[0-9]]*\)\.\([[0-9]]*\)/\2/'`
VERSION_PATCH=`echo $VERSION | $SED 's/\([[0-9]]*\)\.\([[0-9]]*\)\.\([[0-9]]*\)/\3/'`

CWARNFLAGS_LIGHT="-Wall -W"
CWARNFLAGS_FULL="$CWARNFLAGS_LIGHT"

AC_ARG_ENABLE(more_warnings, [  --enable-more-warnings  Enable more warnings (default disabled)])
if test "x$enable_more_warnings" = "xyes"
then
  CWARNFLAGS_LIGHT="$CWARNFLAGS_LIGHT -Wdeclaration-after-statement -Wcast-qual -Wcast-align"
  CWARNFLAGS_LIGHT="$CWARNFLAGS_LIGHT -Winit-self -Wcast-align -pedantic -Wformat=2 -Wunused"
  CWARNFLAGS_LIGHT="$CWARNFLAGS_LIGHT -Wstrict-aliasing=2 -Wpointer-arith -Wbad-function-cast -Waggregate-return"

  CWARNFLAGS_FULL="$CWARNFLAGS_LIGHT"
  CWARNFLAGS_FULL="$CWARNFLAGS_FULL -Wshadow -Wsign-compare -Wredundant-decls -Wmissing-prototypes -Wundef -Wmissing-declarations"
fi

#ifeq ($(SPAMMY_WARNINGS),1)
#CWARNFLAGS_FULL="$CWARNFLAGS_FULL -Wconversion"
#CWARNFLAGS_LIGHT="$CWARNFLAGS_FULL"
#endif

AC_ARG_ENABLE(fortify_source, [  --enable-fortify-source Build with -D_FORTIFY_SOURCE=2 (default disabled)])
if test "x$enable_fortify_source" = "xyes"
then
  CFLAGS_FORTIFY_SOURCE="-D_FORTIFY_SOURCE=2"
fi

AC_ARG_ENABLE(debug, [  --enable-debug          Enable debug code and logs (default disabled)])
if test "x$enable_debug" = "xyes"
then
  CFLAGS_DEBUG="-DDEBUG"
fi

AC_ARG_ENABLE(profile, [  --enable-profile        Enable profile code (default disabled)])
if test "x$enable_profile" = "xyes"
then
  CFLAGS_PROFILE="-DPROFILE"
fi


AC_SUBST([LIBTIGER_SHARED_VERSION])
AC_SUBST([VERSION_MAJOR])
AC_SUBST([VERSION_MINOR])
AC_SUBST([VERSION_PATCH])
AC_SUBST([CONST])
AC_SUBST([CWARNFLAGS_LIGHT])
AC_SUBST([CWARNFLAGS_FULL])
AC_SUBST([CFLAGS_FORTIFY_SOURCE])
AC_SUBST([CFLAGS_DEBUG])
AC_SUBST([CFLAGS_PROFILE])
AC_SUBST([NM])
AC_SUBST([PTHREAD_CFLAGS])
AC_SUBST([PTHREAD_LIBS])

AC_CONFIG_FILES(
  [Makefile]
  [doc/Makefile]
  [include/tiger/tiger.h]
  [misc/pkgconfig/tiger.pc]
  [misc/pkgconfig/tiger-uninstalled.pc]
  [doc/tiger.doxygen]
)
AC_OUTPUT

