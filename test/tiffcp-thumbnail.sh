#!/bin/sh
#
# Basic sanity check for thumbnail
#
. ${srcdir}/common.sh

outfile1=deleteme-in-$$.tif
outfile2=deleteme-out-$$.tif
operation=tiffcp
${TIFFCP} -c g3:1d ${IMG_MINISWHITE_1C_1B} ${outfile1}
status=$?

if test $status -eq 0
then
  operation=thumbnail
  ${THUMBNAIL} ${outfile1} ${outfile2} 2> /dev/null 
  status=$?
fi

if test $status -eq 0
then
  rm -f ${outfile1} ${outfile2}
else
  echo "Test failed (${operation} returned ${status}).  Please inspect these output files:"
  echo "  " ${outfile1} ${outfile2}
fi

exit $status
