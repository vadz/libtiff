#!/bin/sh
. ${srcdir:-.}/common.sh
f_test_convert "$TIFFCROP -E left -Z1:4,2:4" "$srcdir/images/palette-1c-8b.tiff" "deleteme-tiffcrop-extractz14-palette-1c-8b.tiff"
