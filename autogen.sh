#!/bin/sh
set -x
libtoolize --force --install --copy
aclocal -I .
autoheader
automake --foreign --add-missing --copy
autoconf

