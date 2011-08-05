dnl Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)
AC_INIT(libverto, 0.1)


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

PKG_CHECK_MODULES(glib, glib-2.0, have_glib=yes, have_glib=no)
AC_ARG_WITH(glib,
            [AS_HELP_STRING([--without-glib],
              [build the glib library @<:@automatic@:>@])],
            [with_glib=no],
            [test x$have_glib = xyes && with_glib=yes])
if test x$with_glib = xyes; then
	AC_SUBST(glib_CFLAGS)
	AC_SUBST(glib_LIBS)
	if test x$have_glib = xno; then
		echo "glib library requires glib-2.0!"
		exit 1
	fi
else
	with_glib=no
fi
AM_CONDITIONAL([WITH_GLIB], [test x$with_glib = xyes])

PKG_CHECK_MODULES(libev, libev, have_libev=yes, have_libev=no)
AC_ARG_WITH(libev,
            [AS_HELP_STRING([--without-libev],
              [build the libev library @<:@automatic@:>@])],
            [with_libev=no],
            [test x$have_libev = xyes && with_libev=yes])
if test x$with_libev = xyes; then
	AC_SUBST(libev_CFLAGS)
	AC_SUBST(libev_LIBS)
	if test x$have_libev = xno; then
		echo "libev library requires libev!"
		exit 1
	fi
else
	with_libev=no
fi
AM_CONDITIONAL([WITH_LIBEV], [test x$with_libev = xyes])

PKG_CHECK_MODULES(libevent, libevent >= 2.0, have_libevent=yes, have_libevent=no)
AC_ARG_WITH(libevent,
            [AS_HELP_STRING([--without-libevent],
              [build the libevent library @<:@automatic@:>@])],
            [with_libevent=no],
            [test x$have_libevent = xyes && with_libevent=yes])
if test x$with_libevent = xyes; then
	AC_SUBST(libevent_CFLAGS)
	AC_SUBST(libevent_LIBS)
	if test x$have_libevent = xno; then
		echo "libevent library requires libevent >= 2.0!"
		exit 1
	fi
else
	with_libevent=no
fi
AM_CONDITIONAL([WITH_LIBEVENT], [test x$with_libevent = xyes])

PKG_CHECK_MODULES(tevent, tevent, have_tevent=yes, have_tevent=no)
AC_ARG_WITH(tevent,
            [AS_HELP_STRING([--without-tevent],
              [build the tevent library @<:@automatic@:>@])],
            [with_tevent=no],
            [test x$have_tevent = xyes && with_tevent=yes])
if test x$with_tevent = xyes; then
	AC_SUBST(tevent_CFLAGS)
	AC_SUBST(tevent_LIBS)
	if test x$have_tevent = xno; then
		echo "tevent library requires tevent!"
		exit 1
	fi
else
	with_tevent=no
fi
AM_CONDITIONAL([WITH_TEVENT], [test x$with_tevent = xyes])


AC_MSG_NOTICE()
AC_MSG_NOTICE([LIBRARIES TO BE BUILT])
AC_MSG_NOTICE(AS_HELP_STRING([libverto], [yes]))
AC_MSG_NOTICE(AS_HELP_STRING([libverto-glib], [$with_glib]))
AC_MSG_NOTICE(AS_HELP_STRING([libverto-libev], [$with_libev]))
AC_MSG_NOTICE(AS_HELP_STRING([libverto-libevent], [$with_libevent]))
AC_MSG_NOTICE(AS_HELP_STRING([libverto-tevent], [$with_tevent]))
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

