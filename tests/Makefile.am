AUTOMAKE_OPTIONS = parallel-tests color-tests
LDADD = $(abs_top_srcdir)/src/libverto.la
AM_LDFLAGS  = -rpath $(abs_top_builddir)/src/.libs
AM_CFLAGS = -Wall -I$(abs_top_srcdir)/src

if WITH_GLIB
AM_CFLAGS += -DHAVE_GLIB=1 
endif
if WITH_LIBEV
AM_CFLAGS += -DHAVE_LIBEV=1 
endif
if WITH_LIBEVENT
AM_CFLAGS += -DHAVE_LIBEVENT=1 
endif
if WITH_TEVENT
AM_CFLAGS += -DHAVE_TEVENT=1 
endif

TESTS = timeout idle child signal read
noinst_PROGRAMS = $(TESTS)
EXTRA_DIST      = test.h