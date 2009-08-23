#!/bin/sh
. ${srcdir:-.}/common.sh
f_test_convert "$TIFFCROP -F both" "$srcdir/images/rgb-3c-8b.tiff" "deleteme-tiffcrop-doubleflip-rgb-3c-8b.tiff"
