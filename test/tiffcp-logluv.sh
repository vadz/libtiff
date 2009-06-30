#!/bin/sh
#
# Basic sanity check for tiffcp with G3 compression
#
. ${srcdir:-.}/common.sh
outfile=deleteme-raw-$$.tif
outfile2=deleteme-sgilog-$$.tif
operation=tiffcp
${TIFFCP} -c none images/logluv-3c-16b.tiff ${outfile}
status=$?

if test $status -eq 0 ; then
  ${TIFFCP} -c sgilog ${outfile} ${outfile2}
fi

if test $status -eq 0 ; then
    rm -f ${outfile} ${outfile2}
else
  echo "Test failed (${operation} returns ${status}).  Please inspect these output files:"
  echo "  " ${outfile} ${outfile2}
fi

exit $status
