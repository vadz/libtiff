#!/bin/sh
. ${srcdir:-.}/common.sh
f_test_convert "$TIFFCROP -F both" "$srcdir/images/miniswhite-1c-1b.tiff" "deleteme-tiffcrop-doubleflip-miniswhite-1c-1b.tiff"
