include_HEADERS      = verto.h
lib_LTLIBRARIES      = libverto.la

libverto_la_SOURCES  = verto.c verto.h
libverto_la_CFLAGS   = $(AM_CFLAGS) -Wall -DMODSUFFIX='".so"'
libverto_la_LDFLAGS  = $(AM_LDFLAGS) -ldl

# glib
if WITH_GLIB
lib_LTLIBRARIES += libverto-glib.la
include_HEADERS += verto-glib.h
libverto_glib_la_SOURCES  = verto-glib.c
libverto_glib_la_CFLAGS   = $(AM_CFLAGS)  @glib_CFLAGS@ -Wall
libverto_glib_la_LDFLAGS  = $(AM_LDFLAGS) @glib_LIBS@
libverto_glib_la_LIBADD   = libverto.la
endif

# libev
if WITH_EV
lib_LTLIBRARIES += libverto-ev.la
include_HEADERS += verto-ev.h
libverto_ev_la_SOURCES = verto-ev.c
libverto_ev_la_CFLAGS  = $(AM_CFLAGS)  @ev_CFLAGS@ -Wall
libverto_ev_la_LDFLAGS = $(AM_LDFLAGS) @ev_LIBS@
libverto_ev_la_LIBADD  = libverto.la
endif

# libevent
if WITH_EVENT
lib_LTLIBRARIES += libverto-event.la
include_HEADERS += verto-event.h
libverto_event_la_SOURCES = verto-event.c
libverto_event_la_CFLAGS  = $(AM_CFLAGS)  @event_CFLAGS@ -Wall
libverto_event_la_LDFLAGS = $(AM_LDFLAGS) @event_LIBS@
libverto_event_la_LIBADD  = libverto.la
endif

# tevent
if WITH_TEVENT
lib_LTLIBRARIES += libverto-tevent.la
include_HEADERS += verto-tevent.h
libverto_tevent_la_SOURCES = verto-tevent.c
libverto_tevent_la_CFLAGS  = $(AM_CFLAGS)  @tevent_CFLAGS@ -Wall
libverto_tevent_la_LDFLAGS = $(AM_LDFLAGS) @tevent_LIBS@
libverto_tevent_la_LIBADD  = libverto.la
endif