#!/bin/sh
. ${srcdir:-.}/common.sh
f_test_convert "$TIFFCROP -E left -Z1:4,2:4" "$srcdir/images/logluv-3c-16b.tiff" "deleteme-tiffcrop-extractz14-logluv-3c-16b.tiff"
