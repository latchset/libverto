pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libverto.pc

if BUILD_GLIB
pkgconfig_DATA += libverto-glib.pc
endif

if BUILD_LIBEV
pkgconfig_DATA += libverto-libev.pc
endif

if BUILD_LIBEVENT
pkgconfig_DATA += libverto-libevent.pc
endif

if BUILD_TEVENT
pkgconfig_DATA += libverto-tevent.pc
endif

ACLOCAL_AMFLAGS=-I m4
SUBDIRS=src tests
