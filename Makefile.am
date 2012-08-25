pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libverto.pc

if MODULE_GLIB
pkgconfig_DATA += libverto-glib.pc
endif

if MODULE_LIBEV
pkgconfig_DATA += libverto-libev.pc
endif

if MODULE_LIBEVENT
pkgconfig_DATA += libverto-libevent.pc
endif

if MODULE_TEVENT
pkgconfig_DATA += libverto-tevent.pc
endif

ACLOCAL_AMFLAGS=-I m4
SUBDIRS=src tests
