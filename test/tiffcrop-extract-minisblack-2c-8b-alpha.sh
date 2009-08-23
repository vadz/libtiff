#!/bin/sh
. ${srcdir:-.}/common.sh
infile="$srcdir/images/minisblack-2c-8b-alpha.tiff"
outfile="o-tiffcrop-extract-minisblack-2c-8b-alpha.tiff"
f_test_convert "$TIFFCROP -U px -E top -X 100 -Y 100" $infile $outfile
f_tiffinfo_validate $outfile
