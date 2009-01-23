#!/bin/sh
#
# Basic sanity check for tiffcp + tiffsplit
#
# First we use tiffcp to join our test files into a multi-frame TIFF
# and then we use tiffsplit to split them out again.
#
. ${srcdir:-.}/common.sh
conjoined=deleteme-conjoined-$$.tif
splitfile=deleteme-split-$$

operation=tiffcp
${TIFFCP} ${IMG_UNCOMPRESSED} ${conjoined}
status=$?

if test $status -eq 0
then
  operation=tiffsplit
  ${TIFFSPLIT} ${conjoined} ${splitfile}
  status=$?
fi

if test $status -eq 0
then
  :
  rm -f ${conjoined} ${splitfile}*
else
  echo "Test failed (${operation} returned ${status}).  Please inspect these output files:"
  echo "  " ${conjoined} ${splitfile}* ${reconjoined}
fi

exit $status
