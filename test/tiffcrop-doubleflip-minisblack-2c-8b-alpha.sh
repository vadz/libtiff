#!/bin/sh
. ${srcdir:-.}/common.sh
f_test_convert "$TIFFCROP -F both" "$srcdir/images/minisblack-2c-8b-alpha.tiff" "deleteme-tiffcrop-doubleflip-minisblack-2c-8b-alpha.tiff"
