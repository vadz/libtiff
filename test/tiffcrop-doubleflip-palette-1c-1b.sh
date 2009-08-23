#!/bin/sh
. ${srcdir:-.}/common.sh
f_test_convert "$TIFFCROP -F both" "$srcdir/images/palette-1c-1b.tiff" "deleteme-tiffcrop-doubleflip-palette-1c-1b.tiff"
