#!/bin/sh
#
# Basic sanity check for tiffinfo.
#
. ${srcdir:-.}/common.sh
outfile=deleteme-$$.txt
operation=tiffinfo
${TIFFINFO} -c -D -d -j -s ${IMG_MINISBLACK_1C_16B} > $outfile
status=$?

if test $status -eq 0
then
  rm -f ${outfile}
else
  echo "Test failed (${operation} returns ${status}).  Please inspect these output files:"
  echo "  " ${outfile}
fi

exit $status
