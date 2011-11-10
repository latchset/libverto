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
if WITH_GLIB
lib_LTLIBRARIES += libverto-glib.la
include_HEADERS += verto-glib.h
libverto_glib_la_SOURCES  = verto-glib.c
libverto_glib_la_CFLAGS   = $(AM_CFLAGS)  @glib_CFLAGS@
libverto_glib_la_LDFLAGS  = $(AM_LDFLAGS) @glib_LIBS@
libverto_glib_la_LIBADD   = libverto.la
endif

# libev
if WITH_LIBEV
lib_LTLIBRARIES += libverto-libev.la
include_HEADERS += verto-libev.h
libverto_libev_la_SOURCES = verto-libev.c
libverto_libev_la_CFLAGS  = $(AM_CFLAGS)  @libev_CFLAGS@
libverto_libev_la_LDFLAGS = $(AM_LDFLAGS) @libev_LIBS@
libverto_libev_la_LIBADD  = libverto.la
endif

# libevent
if WITH_LIBEVENT
lib_LTLIBRARIES += libverto-libevent.la
include_HEADERS += verto-libevent.h
libverto_libevent_la_SOURCES = verto-libevent.c
libverto_libevent_la_CFLAGS  = $(AM_CFLAGS)  @libevent_CFLAGS@
libverto_libevent_la_LDFLAGS = $(AM_LDFLAGS) @libevent_LIBS@
libverto_libevent_la_LIBADD  = libverto.la
endif

# tevent
if WITH_TEVENT
lib_LTLIBRARIES += libverto-tevent.la
include_HEADERS += verto-tevent.h
libverto_tevent_la_SOURCES = verto-tevent.c
libverto_tevent_la_CFLAGS  = $(AM_CFLAGS)  @tevent_CFLAGS@
libverto_tevent_la_LDFLAGS = $(AM_LDFLAGS) @tevent_LIBS@
libverto_tevent_la_LIBADD  = libverto.la
endif
