#!/bin/sh
#
# Basic sanity check for tiffdump
#
. ${srcdir:-.}/common.sh
outfile=deleteme-$$.txt
operation=tiffdump
${TIFFDUMP} ${IMG_MINISWHITE_1C_1B} > $outfile
status=$?

if test $status -eq 0
then
  rm -f ${outfile}
else
  echo "Test failed (${operation} returns ${status}).  Please inspect these output files:"
  echo "  " ${outfile}
fi

exit $status
