dnl Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)
AC_INIT(libverto, 1.0)


AC_CANONICAL_SYSTEM
AM_INIT_AUTOMAKE()

AC_PROG_CC
AC_PROG_LIBTOOL
AC_CONFIG_MACRO_DIR([m4])

m4_ifdef([AM_SILENT_RULES], [
  AM_SILENT_RULES([yes])
], [
  AM_DEFAULT_VERBOSITY=1
  AC_SUBST(AM_DEFAULT_VERBOSITY)
])

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
fi
AM_CONDITIONAL([WITH_GLIB], [test x$with_glib = xyes])

PKG_CHECK_MODULES(ev, libev, have_ev=yes, have_ev=no)
AC_ARG_WITH(ev,
            [AS_HELP_STRING([--without-ev],
              [build the ev library @<:@automatic@:>@])],
            [with_ev=no],
            [test x$have_ev = xyes && with_ev=yes])
if test x$with_ev = xyes; then
	AC_SUBST(ev_CFLAGS)
	AC_SUBST(ev_LIBS)
	if test x$have_ev = xno; then
		echo "ev library requires libev!"
		exit 1
	fi
fi
AM_CONDITIONAL([WITH_EV], [test x$with_ev = xyes])

PKG_CHECK_MODULES(event, libevent >= 2.0, have_event=yes, have_event=no)
AC_ARG_WITH(event,
            [AS_HELP_STRING([--without-event],
              [build the event library @<:@automatic@:>@])],
            [with_event=no],
            [test x$have_event = xyes && with_event=yes])
if test x$with_event = xyes; then
	AC_SUBST(event_CFLAGS)
	AC_SUBST(event_LIBS)
	if test x$have_event = xno; then
		echo "event library requires libevent!"
		exit 1
	fi
fi
AM_CONDITIONAL([WITH_EVENT], [test x$with_event = xyes])

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
fi
AM_CONDITIONAL([WITH_TEVENT], [test x$with_tevent = xyes])


echo -e ""
echo -e "LIBRARIES TO BE BUILT"
echo -e "\tlibverto\t\tyes"
echo -e "\tlibverto-glib\t\t$with_glib"
echo -e "\tlibverto-ev\t\t$with_ev"
echo -e "\tlibverto-event\t\t$with_event"
echo -e "\tlibverto-tevent\t\t$with_tevent"
echo -e ""

AC_CONFIG_FILES(Makefile src/Makefile)
AC_OUTPUT

