#!/bin/sh
. ${srcdir:-.}/common.sh
infile="$srcdir/images/rgb-3c-8b.tiff"
outfile="o-tiffcrop-doubleflip-rgb-3c-8b.tiff"
f_test_convert "$TIFFCROP -F both" $infile $outfile
f_tiffinfo_validate $outfile
