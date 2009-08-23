#!/bin/sh
. ${srcdir:-.}/common.sh
infile="/images/rgb-3c-8b.tiff"
outfile="o-tiff2rgba-rgb-3c-8b.tiff"
f_test_convert "$TIFF2RGBA" $infile $outfile
f_tiffinfo_validate $outfile
