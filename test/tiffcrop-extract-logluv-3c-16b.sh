#!/bin/sh
. ${srcdir:-.}/common.sh
f_test_convert "$TIFFCROP -U px -E top -X 100 -Y 100" "$srcdir/images/logluv-3c-16b.tiff" "deleteme-tiffcrop-extract-logluv-3c-16b.tiff"
