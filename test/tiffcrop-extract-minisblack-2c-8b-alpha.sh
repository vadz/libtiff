#!/bin/sh
. ${srcdir:-.}/common.sh
f_test_convert "$TIFFCROP -U px -E top -X 100 -Y 100" "$srcdir/images/minisblack-2c-8b-alpha.tiff" "deleteme-tiffcrop-extract-minisblack-2c-8b-alpha.tiff"
