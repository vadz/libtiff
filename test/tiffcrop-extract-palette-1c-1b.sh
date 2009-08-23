#!/bin/sh
. ${srcdir:-.}/common.sh
f_test_convert "$TIFFCROP -U px -E top -X 100 -Y 100" "$srcdir/images/palette-1c-1b.tiff" "deleteme-tiffcrop-extract-palette-1c-1b.tiff"
