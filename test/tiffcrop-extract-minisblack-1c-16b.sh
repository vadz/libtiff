#!/bin/sh
. ${srcdir:-.}/common.sh
f_test_convert "$TIFFCROP -U px -E top -X 100 -Y 100" "$srcdir/images/minisblack-1c-16b.tiff" "deleteme-tiffcrop-extract-minisblack-1c-16b.tiff"
