#!/bin/sh
# Generated file, master is Makefile.am
. ${srcdir:-.}/common.sh
infile="$srcdir/images/logluv-3c-16b.tiff"
outfile="o-tiffcrop-extract-logluv-3c-16b.tiff"
f_test_convert "$TIFFCROP -U px -E top -X 100 -Y 100" $infile $outfile
f_tiffinfo_validate $outfile
