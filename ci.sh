#!/bin/bash -ex

COMPILER=${COMPILER:-gcc}

CFLAGS="-Werror"
if [ x$BUILTIN == xyes ]; then
    CFLAGS+=" -DBUILTIN_MODULE=libev"
fi

if [ -f /etc/debian_version ]; then
    apt-get update
    apt-get -y install autoconf build-essential libtool $COMPILER \
            lib{ev,event,glib2.0}-dev
elif [ -f /etc/fedora-release ]; then
    # dnf has no update-only verb
    dnf -y install autoconf automake libtool make which $COMPILER \
        {glib2,libevent,libev}-devel --nogpgcheck --skip-broken
elif [ -f /etc/redhat-release ]; then
    # rhel/centos
    yum -y install autoconf automake libtool make which $COMPILER \
        {glib2,libevent}-devel

    # rhel doesn't have libev anyway
    CFLAGS+=" -std=c89"
else
    echo "Distro not found!"
    false
fi

autoreconf -fiv
./configure CFLAGS="$CFLAGS" CC=$(which $COMPILER)
make

if [ x$BUILTIN != xyes ]; then
    make check
fi
