#!/bin/sh
#
# Basic sanity check for tiffcp with G4 compression
#
. ${srcdir:-.}/common.sh
outfile=deleteme-$$.tif
operation=tiffcp
${TIFFCP} -c g4 ${IMG_MINISWHITE_1C_1B} $outfile
status=$?

if test $status -eq 0
then
  rm -f ${outfile}
else
  echo "Test failed (${operation} returns ${status}).  Please inspect these output files:"
  echo "  " ${outfile}
fi

exit $status
