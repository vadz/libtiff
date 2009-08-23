#!/bin/sh
. ${srcdir:-.}/common.sh
f_test_convert "$TIFFCROP -U px -E top -X 100 -Y 100" "$srcdir/images/miniswhite-1c-1b.tiff" "deleteme-tiffcrop-extract-miniswhite-1c-1b.tiff"
