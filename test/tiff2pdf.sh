#!/bin/sh
#
# Basic sanity check for tiff2pdf
#
. ${srcdir:-.}/common.sh
f_test_convert "${TIFF2PDF}" "${IMG_MINISWHITE_1C_1B}" "> o-tiff2pdf.pdf"