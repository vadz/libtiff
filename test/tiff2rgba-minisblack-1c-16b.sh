#!/bin/sh
. ${srcdir:-.}/common.sh
infile="/images/minisblack-1c-16b.tiff"
outfile="o-tiff2rgba-minisblack-1c-16b.tiff"
f_test_convert "$TIFF2RGBA" $infile $outfile
f_tiffinfo_validate $outfile
