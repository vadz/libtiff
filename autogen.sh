#!/bin/sh
set -x
aclocal
autoheader
libtoolize --automake --copy
automake --foreign --add-missing --copy
autoconf

