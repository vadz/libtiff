#!/bin/sh
. ${srcdir:-.}/common.sh
infile="/images/logluv-3c-16b.tiff"
outfile="o-tiff2rgba-logluv-3c-16b.tiff"
f_test_convert "$TIFF2RGBA" $infile $outfile
f_tiffinfo_validate $outfile
