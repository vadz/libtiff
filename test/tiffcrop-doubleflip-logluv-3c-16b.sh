#!/bin/sh
. ${srcdir:-.}/common.sh
f_test_convert "$TIFFCROP -F both" "$srcdir/images/logluv-3c-16b.tiff" "deleteme-tiffcrop-doubleflip-logluv-3c-16b.tiff"
