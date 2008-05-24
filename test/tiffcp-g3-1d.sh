#!/bin/sh
#
# Basic sanity check for tiffcp with G3 compression and 1 dimensional encoding.
#
. ${srcdir}/common.sh
outfile=deleteme-$$.tif
operation=tiffcp
${TIFFCP} -c g3:1d ${IMG_MINISWHITE_1C_1B} $outfile
status=$?

if test $status -eq 0
then
  rm -f ${outfile}
else
  echo "Test failed (${operation} returns ${status}).  Please inspect these output files:"
  echo "  " ${outfile}
fi

exit $status
