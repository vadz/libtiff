#!/bin/sh
. ${srcdir:-.}/common.sh
f_test_convert "$TIFFCROP -U px -E top -X 100 -Y 100" "$srcdir/images/rgb-3c-16b.tiff" "deleteme-tiffcrop-extract-rgb-3c-16b.tiff"
