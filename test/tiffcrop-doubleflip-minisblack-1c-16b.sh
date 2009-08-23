#!/bin/sh
. ${srcdir:-.}/common.sh
f_test_convert "$TIFFCROP -F both" "$srcdir/images/minisblack-1c-16b.tiff" "deleteme-tiffcrop-doubleflip-minisblack-1c-16b.tiff"
