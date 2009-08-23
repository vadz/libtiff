#!/bin/sh
. ${srcdir:-.}/common.sh
infile="$srcdir/images/miniswhite-1c-1b.tiff"
outfile="o-tiffcrop-extract-miniswhite-1c-1b.tiff"
f_test_convert "$TIFFCROP -U px -E top -X 100 -Y 100" $infile $outfile
f_tiffinfo_validate $outfile
