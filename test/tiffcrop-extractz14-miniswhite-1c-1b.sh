#!/bin/sh
. ${srcdir:-.}/common.sh
f_test_convert "$TIFFCROP -E left -Z1:4,2:4" "$srcdir/images/miniswhite-1c-1b.tiff" "deleteme-tiffcrop-extractz14-miniswhite-1c-1b.tiff"
