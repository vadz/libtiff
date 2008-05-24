#!/bin/sh
#
# Basic sanity check for tiffps with PostScript Level 1 output
#
. ${srcdir}/common.sh
outfile=deleteme-$$.ps
operation=tiff2ps
${TIFF2PS} -a -1 ${IMG_MINISWHITE_1C_1B} > $outfile
status=$?

if test $status -eq 0
then
  rm -f ${outfile}
else
  echo "Test failed (${operation} returns ${status}).  Please inspect these output files:"
  echo "  " ${outfile}
fi

exit $status
