#!/bin/sh
. ${srcdir:-.}/common.sh
infile="/images/minisblack-2c-8b-alpha.tiff"
outfile="o-tiff2rgba-minisblack-2c-8b-alpha.tiff"
f_test_convert "$TIFF2RGBA" $infile $outfile
f_tiffinfo_validate $outfile
