dnl Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)
AC_INIT(libverto, 0.2.4)

AC_CANONICAL_SYSTEM
AM_INIT_AUTOMAKE([1.11])

AC_PROG_CC
AC_PROG_LIBTOOL
AC_CONFIG_MACRO_DIR([m4])

m4_ifdef([AM_SILENT_RULES], [
  AM_SILENT_RULES([yes])
], [
  AM_DEFAULT_VERBOSITY=1
  AC_SUBST(AM_DEFAULT_VERBOSITY)
])

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
if test "x$WITH_PTHREAD" != xno; then
  AX_PTHREAD([PTHREAD_CFLAGS="$PTHREAD_CFLAGS -DHAVE_PTHREAD=1";
              CC="$PTHREAD_CC";
              BUILD_PTHREAD=yes],
             [test "x$WITH_PTHREAD" = xyes && AC_MSG_ERROR("pthread not found")])
fi

AC_ARG_WITH([glib],
            [AS_HELP_STRING([--with-glib],
                            [build the glib library @<:@default: automatic@:>@])],
            [case $withval in
                 no) WITH_GLIB=no;;
                yes) WITH_GLIB=yes;;
                  *) WITH_GLIB=auto;;
             esac], [WITH_GLIB=auto])
BUILD_GLIB=no
if test "x$WITH_GLIB" != xno; then
  PKG_CHECK_MODULES([GLIB], [glib-2.0], [BUILD_GLIB=yes],
                    [test "x$WITH_GLIB" = xyes && AC_MSG_ERROR("glib not found")])
fi
AM_CONDITIONAL([BUILD_GLIB], [test "x$BUILD_GLIB" = xyes])

AC_ARG_WITH([libev],
            [AS_HELP_STRING([--with-libev],
                            [build the libev library @<:@default: automatic@:>@])],
            [case $withval in
                 no) WITH_LIBEV=no;;
                yes) WITH_LIBEV=yes;;
                  *) WITH_LIBEV=auto;;
             esac], [WITH_LIBEV=auto])
BUILD_LIBEV=no
if test "x$WITH_LIBEV" != xno; then
  PKG_CHECK_MODULES([LIBEV], [libev], [BUILD_LIBEV=yes],
                    [test "x$WITH_LIBEV" = xyes && AC_MSG_ERROR("libev not found")])
fi
AM_CONDITIONAL([BUILD_LIBEV], [test "x$BUILD_LIBEV" = xyes])

AC_ARG_WITH([libevent],
            [AS_HELP_STRING([--with-libevent],
                            [build the libevent library @<:@default: automatic@:>@])],
            [case $withval in
                 no) WITH_LIBEVENT=no;;
                yes) WITH_LIBEVENT=yes;;
                  *) WITH_LIBEVENT=auto;;
             esac], [WITH_LIBEVENT=auto])
BUILD_LIBEVENT=no
if test "x$WITH_LIBEVENT" != xno; then
  PKG_CHECK_MODULES([LIBEVENT], [libevent >= 2.0], [BUILD_LIBEVENT=yes],
                    [test "x$WITH_LIBEVENT" = xyes && AC_MSG_ERROR("libevent not found")])
fi
AM_CONDITIONAL([BUILD_LIBEVENT], [test "x$BUILD_LIBEVENT" = xyes])

AC_ARG_WITH([tevent],
            [AS_HELP_STRING([--with-tevent],
                            [build the tevent library @<:@default: automatic@:>@])],
            [case $withval in
                 no) WITH_TEVENT=no;;
                yes) WITH_TEVENT=yes;;
                  *) WITH_TEVENT=auto;;
             esac], [WITH_TEVENT=auto])
BUILD_TEVENT=no
if test "x$WITH_TEVENT" != xno; then
  PKG_CHECK_MODULES([TEVENT], [tevent], [BUILD_TEVENT=yes],
                    [test "x$WITH_TEVENT" = xyes && AC_MSG_ERROR("tevent not found")])
fi
AM_CONDITIONAL([BUILD_TEVENT], [test "x$BUILD_TEVENT" = xyes])

AC_MSG_NOTICE()
AC_MSG_NOTICE([LIBRARIES TO BE BUILT])
AC_MSG_NOTICE(AS_HELP_STRING([libverto], [yes (pthread: $BUILD_PTHREAD)]))
AC_MSG_NOTICE(AS_HELP_STRING([libverto-glib], [$BUILD_GLIB]))
AC_MSG_NOTICE(AS_HELP_STRING([libverto-libev], [$BUILD_LIBEV]))
AC_MSG_NOTICE(AS_HELP_STRING([libverto-libevent], [$BUILD_LIBEVENT]))
AC_MSG_NOTICE(AS_HELP_STRING([libverto-tevent], [$BUILD_TEVENT]))
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

