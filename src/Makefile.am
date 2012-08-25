AM_CFLAGS = -Wall -Wdeclaration-after-statement
AM_LDFLAGS = -version-info 1:0:0
EXTRA_DIST = libverto.symbols libverto-glib.symbols libverto-libev.symbols \
             libverto-libevent.symbols libverto-tevent.symbols

include_HEADERS     = verto.h verto-module.h
noinst_HEADERS      = module.h
lib_LTLIBRARIES     = libverto.la

libverto_la_SOURCES = verto.c module.c verto.h
libverto_la_CFLAGS  = $(AM_CFLAGS) $($(BUILTIN_MODULE)_CFLAGS) $(PTHREAD_CFLAGS)
libverto_la_LDFLAGS = $(AM_LDFLAGS) $($(BUILTIN_MODULE)_LIBS) $(PTHREAD_LIBS) $(LIBS) \
                      -export-symbols $(srcdir)/libverto.symbols

if BUILTIN_GLIB
libverto_la_SOURCES += verto-glib.c
endif

if BUILTIN_LIBEV
libverto_la_SOURCES += verto-libev.c
endif

if BUILTIN_LIBEVENT
libverto_la_SOURCES += verto-libevent.c
endif

if BUILTIN_TEVENT
libverto_la_SOURCES += verto-tevent.c
endif

if MODULE_GLIB
lib_LTLIBRARIES += libverto-glib.la
include_HEADERS += verto-glib.h
libverto_glib_la_SOURCES  = verto-glib.c
libverto_glib_la_LIBADD   = libverto.la
libverto_glib_la_CFLAGS   = $(AM_CFLAGS) $(glib_CFLAGS)
libverto_glib_la_LDFLAGS  = $(AM_LDFLAGS) $(glib_LIBS) \
                            -export-symbols $(srcdir)/libverto-glib.symbols
endif

if MODULE_LIBEV
lib_LTLIBRARIES += libverto-libev.la
include_HEADERS += verto-libev.h
libverto_libev_la_SOURCES = verto-libev.c
libverto_libev_la_LIBADD  = libverto.la
libverto_libev_la_CFLAGS  = $(AM_CFLAGS) $(libev_CFLAGS)
libverto_libev_la_LDFLAGS = $(AM_LDFLAGS) $(libev_LIBS) \
                            -export-symbols $(srcdir)/libverto-libev.symbols
endif

if MODULE_LIBEVENT
lib_LTLIBRARIES += libverto-libevent.la
include_HEADERS += verto-libevent.h
libverto_libevent_la_SOURCES = verto-libevent.c
libverto_libevent_la_LIBADD  = libverto.la
libverto_libevent_la_CFLAGS  = $(AM_CFLAGS) $(libevent_CFLAGS)
libverto_libevent_la_LDFLAGS = $(AM_LDFLAGS) $(libevent_LIBS) \
                               -export-symbols $(srcdir)/libverto-libevent.symbols
endif

if MODULE_TEVENT
lib_LTLIBRARIES += libverto-tevent.la
include_HEADERS += verto-tevent.h
libverto_tevent_la_SOURCES = verto-tevent.c
libverto_tevent_la_LIBADD  = libverto.la
libverto_tevent_la_CFLAGS  = $(AM_CFLAGS) $(tevent_CFLAGS)
libverto_tevent_la_LDFLAGS = $(AM_LDFLAGS) $(tevent_LIBS) \
                             -export-symbols $(srcdir)/libverto-tevent.symbols
endif
