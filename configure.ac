dnl Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)
AC_INIT(libverto, 0.3.0)

m4_ifdef([AC_USE_SYSTEM_EXTENSIONS],
    [AC_USE_SYSTEM_EXTENSIONS],
    [AC_GNU_SOURCE])

AC_PROG_CC_C99
for flag in -Wall -Wextra; do
  AC_TRY_COMPILE([], [return 0;], [CFLAGS="$CFLAGS $flag"],)
done

AC_CANONICAL_SYSTEM
AM_INIT_AUTOMAKE([1.11])

AC_PROG_LIBTOOL
AC_CONFIG_MACRO_DIR([m4])

m4_ifdef([AM_SILENT_RULES], [
  AM_SILENT_RULES([yes])
], [
  AM_DEFAULT_VERBOSITY=1
  AC_SUBST(AM_DEFAULT_VERBOSITY)
])

PKG_PROG_PKG_CONFIG
AC_CHECK_LIB([dl],[dlopen])

AC_ARG_WITH([pthread],
            [AS_HELP_STRING([--with-pthread],
                            [build pthread support @<:@default: automatic@:>@])],
            [case $withval in
                 no) WITH_PTHREAD=no;;
                yes) WITH_PTHREAD=yes;;
                  *) WITH_PTHREAD=auto;;
             esac], [WITH_PTHREAD=auto])
BUILD_PTHREAD=no
if test x$WITH_PTHREAD != xno; then
  AX_PTHREAD([PTHREAD_CFLAGS="$PTHREAD_CFLAGS -DHAVE_PTHREAD=1";
              CC="$PTHREAD_CC";
              BUILD_PTHREAD=yes],
             [test x$WITH_PTHREAD = xyes && AC_MSG_ERROR("pthread not found")])
fi

AC_ARG_WITH([glib],
            [AS_HELP_STRING([--with-glib],
                            [build the glib library @<:@default: automatic@:>@])],
            [case $withval in
                   no) WITH_GLIB=no;;
                  yes) WITH_GLIB=yes;;
              default) WITH_GLIB=default;;
              builtin) WITH_GLIB=builtin;;
                    *) WITH_GLIB=auto;;
             esac], [WITH_GLIB=auto])

AC_ARG_WITH([libev],
            [AS_HELP_STRING([--with-libev],
                            [build the libev library @<:@default: automatic@:>@])],
            [case $withval in
                   no) WITH_LIBEV=no;;
                  yes) WITH_LIBEV=yes;;
              default) WITH_LIBEV=default;;
              builtin) WITH_LIBEV=builtin;;
                    *) WITH_LIBEV=auto;;
             esac], [WITH_LIBEV=auto])

AC_ARG_WITH([libevent],
            [AS_HELP_STRING([--with-libevent],
                            [build the libevent library @<:@default: automatic@:>@])],
            [case $withval in
                   no) WITH_LIBEVENT=no;;
                  yes) WITH_LIBEVENT=yes;;
              default) WITH_LIBEVENT=default;;
              builtin) WITH_LIBEVENT=builtin;;
                    *) WITH_LIBEVENT=auto;;
             esac], [WITH_LIBEVENT=auto])

AC_ARG_WITH([tevent],
            [AS_HELP_STRING([--with-tevent],
                            [build the tevent library @<:@default: automatic@:>@])],
            [case $withval in
                 no) WITH_TEVENT=no;;
                yes) WITH_TEVENT=yes;;
                  *) WITH_TEVENT=auto;;
             esac], [WITH_TEVENT=auto])

# Ensure that if a builtin is chosen only one is built
BUILTIN_MODULE=
if test x$WITH_GLIB == xbuiltin; then
  BUILTIN_MODULE=glib
  WITH_LIBEV=no
  WITH_LIBEVENT=no
  WITH_TEVENT=no
elif test x$WITH_LIBEV == xbuiltin; then
  BUILTIN_MODULE=libev
  WITH_LIBGLIB=no
  WITH_LIBEVENT=no
  WITH_TEVENT=no
elif test x$WITH_LIBEVENT == xbuiltin; then
  BUILTIN_MODULE=libevent
  WITH_LIBGLIB=no
  WITH_LIBEV=no
  WITH_TEVENT=no
elif test x$WITH_TEVENT == xbuiltin; then
  BUILTIN_MODULE=tevent
  WITH_GLIB=no
  WITH_LIBEV=no
  WITH_LIBEVENT=no
fi
AC_SUBST([BUILTIN_MODULE], $BUILTIN_MODULE)
if test x$BUILTIN_MODULE != x; then
  AC_DEFINE_UNQUOTED([BUILTIN_MODULE], $BUILTIN_MODULE)
fi

# Ensure that there is only one default (convert duplicate default to yes)
if test x$WITH_GLIB == xdefault; then
  AC_DEFINE([DEFUALT_MODULE], [glib])
  test x$WITH_LIBEV    == xdefault && WITH_LIBEV=yes
  test x$WITH_LIBEVENT == xdefault && WITH_LIBEVENT=yes
  test x$WITH_TEVENT   == xdefault && WITH_TEVENT=yes
fi
if test x$WITH_LIBEV == xdefault; then
  AC_DEFINE([DEFUALT_MODULE], [libev])
  test x$WITH_LIBGLIB  == xdefault && WITH_GLIB=yes
  test x$WITH_LIBEVENT == xdefault && WITH_LIBEVENT=yes
  test x$WITH_TEVENT   == xdefault && WITH_TEVENT=yes
fi
if test x$WITH_LIBEVENT == xdefault; then
  AC_DEFINE([DEFUALT_MODULE], [libevent])
  test x$WITH_GLIB     == xdefault && WITH_GLIB=yes
  test x$WITH_LIBEV    == xdefault && WITH_LIBEV=yes
  test x$WITH_TEVENT   == xdefault && WITH_TEVENT=yes
fi
if test x$WITH_TEVENT == xdefault; then
  AC_DEFINE([DEFUALT_MODULE], [tevent])
  test x$WITH_GLIB     == xdefault && WITH_GLIB=yes
  test x$WITH_LIBEV    == xdefault && WITH_LIBEV=yes
  test x$WITH_LIBEVENT == xdefault && WITH_LIBEVENT=yes
fi

BUILD_GLIB=no
BUILD_LIBEV=no
BUILD_LIBEVENT=no
BUILD_TEVENT=no

if test x$WITH_GLIB != xno; then
  PKG_CHECK_MODULES([glib], [glib-2.0], [BUILD_GLIB=$WITH_GLIB],
                    [test x$WITH_GLIB != xauto && AC_MSG_ERROR("glib not found")])
  if test x$BUILD_GLIB == xauto; then
    BUILD_GLIB=yes
  fi
fi

if test x$WITH_LIBEV != xno; then
  AC_CHECK_HEADER(
    [ev.h],
    AC_CHECK_LIB(
      [ev], [ev_loop_new], [BUILD_LIBEV=$WITH_LIBEV],
      [test x$WITH_LIBEV != xauto && AC_MSG_ERROR("libev not found")]
    ),
    [test x$WITH_LIBEV != xauto && AC_MSG_ERROR("ev.h not found")]
  )
  if test x$BUILD_LIBEV == xauto; then
    BUILD_LIBEV=yes
  fi
fi

if test x$WITH_LIBEVENT != xno; then
  PKG_CHECK_MODULES([libevent], [libevent >= 2.0], [BUILD_LIBEVENT=$WITH_LIBEVENT],
                    [test x$WITH_LIBEVENT != xauto && AC_MSG_ERROR("libevent not found")])
  if test x$BUILD_LIBEVENT == xauto; then
    BUILD_LIBEVENT=yes
  fi
fi

if test x$WITH_TEVENT != xno; then
  PKG_CHECK_MODULES([tevent], [tevent], [BUILD_TEVENT=$WITH_TEVENT],
                    [test x$WITH_TEVENT != xauto && AC_MSG_ERROR("tevent not found")])
  if test x$BUILD_TEVENT == xauto; then
    BUILD_TEVENT=yes
  fi
fi

AM_CONDITIONAL([MODULE_GLIB],      [test x$BUILTIN_MODULE == x && test x$BUILD_GLIB     != xno])
AM_CONDITIONAL([MODULE_LIBEV],     [test x$BUILTIN_MODULE == x && test x$BUILD_LIBEV    != xno])
AM_CONDITIONAL([MODULE_LIBEVENT],  [test x$BUILTIN_MODULE == x && test x$BUILD_LIBEVENT != xno])
AM_CONDITIONAL([MODULE_TEVENT],    [test x$BUILTIN_MODULE == x && test x$BUILD_TEVENT   != xno])
AM_CONDITIONAL([BUILTIN_GLIB],     [test x$BUILTIN_MODULE == xglib])
AM_CONDITIONAL([BUILTIN_LIBEV],    [test x$BUILTIN_MODULE == xlibev])
AM_CONDITIONAL([BUILTIN_LIBEVENT], [test x$BUILTIN_MODULE == xlibevent])
AM_CONDITIONAL([BUILTIN_TEVENT],   [test x$BUILTIN_MODULE == xtevent])

AC_MSG_NOTICE()
AC_MSG_NOTICE([BUILD CONFIGURATION])
AC_MSG_NOTICE(AS_HELP_STRING([pthread], [$BUILD_PTHREAD]))
AC_MSG_NOTICE(AS_HELP_STRING([glib], [$BUILD_GLIB]))
AC_MSG_NOTICE(AS_HELP_STRING([libev], [$BUILD_LIBEV]))
AC_MSG_NOTICE(AS_HELP_STRING([libevent], [$BUILD_LIBEVENT]))
AC_MSG_NOTICE(AS_HELP_STRING([tevent], [$BUILD_TEVENT]))
AC_MSG_NOTICE()

AC_CONFIG_FILES(Makefile
                src/Makefile
                tests/Makefile
                libverto-glib.pc
                libverto-libev.pc
                libverto-libevent.pc
                libverto-tevent.pc
                libverto.pc)
AC_OUTPUT
