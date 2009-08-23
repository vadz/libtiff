#!/bin/sh
. ${srcdir:-.}/common.sh
f_test_convert "$TIFFCROP -E left -Z1:4,2:4" "$srcdir/images/minisblack-2c-8b-alpha.tiff" "deleteme-tiffcrop-extractz14-minisblack-2c-8b-alpha.tiff"
