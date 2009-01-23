#!/bin/sh
#
# Basic sanity check for tiff2pdf
#
. ${srcdir:-.}/common.sh
outfile=deleteme-$$.pdf

operation=tiff2pdf
${TIFF2PDF} -o $outfile ${IMG_MINISWHITE_1C_1B}
status=$?

if test $status -eq 0
then
  rm -f ${outfile}
else
  echo "Test failed (${operation} return $status).  Please inspect these output files:"
  echo "  " ${outfile}
fi

exit $status
