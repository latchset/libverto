#!/bin/bash -ex

if [ -f /etc/debian_version ]; then
    apt-get update
    apt-get -y install autoconf build-essential libtool $COMPILER \
            lib{ev,event,glib2.0}-dev
elif [ -f /etc/fedora-release ]; then
    # dnf has no update-only verb
    dnf -y install autoconf automake libtool make which $COMPILER \
        {glib2,libevent,libev}-devel
elif [ -f /etc/redhat-release ]; then
    # rhel/centos
    yum -y install autoconf automake libtool make which $COMPILER \
        {glib2,libevent}-devel
else
    echo "Distro not found!"
    false
fi

autoreconf -fiv
./configure CFLAGS=-Werror CC=$(which $COMPILER)
make
make check
