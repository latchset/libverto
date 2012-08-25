VERSION_INFO = -version-info 1:0:0

AM_CFLAGS = -Wall -Wdeclaration-after-statement

include_HEADERS     = verto.h verto-module.h
noinst_HEADERS      = module.h
lib_LTLIBRARIES     = libverto.la

libverto_la_SOURCES = verto.c module.c verto.h
libverto_la_CFLAGS  = $(AM_CFLAGS) $(PTHREAD_CFLAGS)
libverto_la_LDFLAGS = $(VERSION_INFO) $(LIBS) $(PTHREAD_LIBS) \
                        -export-symbols $(srcdir)/libverto.symbols

# glib
if BUILD_GLIB
lib_LTLIBRARIES += libverto-glib.la
include_HEADERS += verto-glib.h
libverto_glib_la_SOURCES  = verto-glib.c
libverto_glib_la_LIBADD   = libverto.la
libverto_glib_la_CFLAGS   = $(AM_CFLAGS) $(GLIB_CFLAGS)
libverto_glib_la_LDFLAGS  = $(VERSION_INFO) $(GLIB_LIBS) \
                            -export-symbols $(srcdir)/libverto-glib.symbols
endif

# libev
if BUILD_LIBEV
lib_LTLIBRARIES += libverto-libev.la
include_HEADERS += verto-libev.h
libverto_libev_la_SOURCES = verto-libev.c
libverto_libev_la_LIBADD  = libverto.la
libverto_libev_la_CFLAGS  = $(AM_CFLAGS) $(LIBEV_CFLAGS)
libverto_libev_la_LDFLAGS = $(VERSION_INFO) $(LIBEV_LIBS) \
                            -export-symbols $(srcdir)/libverto-libev.symbols
endif

# libevent
if BUILD_LIBEVENT
lib_LTLIBRARIES += libverto-libevent.la
include_HEADERS += verto-libevent.h
libverto_libevent_la_SOURCES = verto-libevent.c
libverto_libevent_la_LIBADD  = libverto.la
libverto_libevent_la_CFLAGS  = $(AM_CFLAGS) $(LIBEVENT_CFLAGS)
libverto_libevent_la_LDFLAGS = $(VERSION_INFO) $(LIBEVENT_LIBS) \
                               -export-symbols $(srcdir)/libverto-libevent.symbols
endif

# tevent
if BUILD_TEVENT
lib_LTLIBRARIES += libverto-tevent.la
include_HEADERS += verto-tevent.h
libverto_tevent_la_SOURCES = verto-tevent.c
libverto_tevent_la_LIBADD  = libverto.la
libverto_tevent_la_CFLAGS  = $(AM_CFLAGS) $(TEVENT_CFLAGS)
libverto_tevent_la_LDFLAGS = $(VERSION_INFO) $(TEVENT_LIBS) \
                             -export-symbols $(srcdir)/libverto-tevent.symbols
endif
