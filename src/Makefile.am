LIBVERTO_CURRENT=1
LIBVERTO_REVISION=0
LIBVERTO_AGE=0

AM_CFLAGS = -Wall -Wdeclaration-after-statement
AM_LDFLAGS = -export-symbols-regex '^verto_' -version-info \
               $(LIBVERTO_CURRENT):$(LIBVERTO_REVISION):$(LIBVERTO_AGE)

include_HEADERS     = verto.h verto-module.h
noinst_HEADERS      = module.h
lib_LTLIBRARIES     = libverto.la

libverto_la_SOURCES = verto.c module.c verto.h
libverto_la_CFLAGS  = $(AM_CFLAGS) $(PTHREAD_CFLAGS)
libverto_la_LIBADD  = $(AM_LIBS) $(LIBS) $(PTHREAD_LIBS)

# glib
if BUILD_GLIB
lib_LTLIBRARIES += libverto-glib.la
include_HEADERS += verto-glib.h
libverto_glib_la_SOURCES  = verto-glib.c
libverto_glib_la_CFLAGS   = $(AM_CFLAGS)  $(GLIB_CFLAGS)
libverto_glib_la_LDFLAGS  = $(AM_LDFLAGS) $(GLIB_LIBS)
libverto_glib_la_LIBADD   = libverto.la
endif

# libev
if BUILD_LIBEV
lib_LTLIBRARIES += libverto-libev.la
include_HEADERS += verto-libev.h
libverto_libev_la_SOURCES = verto-libev.c
libverto_libev_la_CFLAGS  = $(AM_CFLAGS)  $(LIBEV_CFLAGS)
libverto_libev_la_LDFLAGS = $(AM_LDFLAGS) $(LIBEV_LIBS)
libverto_libev_la_LIBADD  = libverto.la
endif

# libevent
if BUILD_LIBEVENT
lib_LTLIBRARIES += libverto-libevent.la
include_HEADERS += verto-libevent.h
libverto_libevent_la_SOURCES = verto-libevent.c
libverto_libevent_la_CFLAGS  = $(AM_CFLAGS)  $(LIBEVENT_CFLAGS)
libverto_libevent_la_LDFLAGS = $(AM_LDFLAGS) $(LIBEVENT_LIBS)
libverto_libevent_la_LIBADD  = libverto.la
endif

# tevent
if BUILD_TEVENT
lib_LTLIBRARIES += libverto-tevent.la
include_HEADERS += verto-tevent.h
libverto_tevent_la_SOURCES = verto-tevent.c
libverto_tevent_la_CFLAGS  = $(AM_CFLAGS)  $(TEVENT_CFLAGS)
libverto_tevent_la_LDFLAGS = $(AM_LDFLAGS) $(TEVENT_LIBS)
libverto_tevent_la_LIBADD  = libverto.la
endif
