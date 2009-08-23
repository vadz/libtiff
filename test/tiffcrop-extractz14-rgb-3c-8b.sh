#!/bin/sh
. ${srcdir:-.}/common.sh
f_test_convert "$TIFFCROP -E left -Z1:4,2:4" "$srcdir/images/rgb-3c-8b.tiff" "deleteme-tiffcrop-extractz14-rgb-3c-8b.tiff"
