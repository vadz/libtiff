#!/bin/sh
#
# Basic sanity check for tiffcp + tiffsplit + tiffcp
#
# First we use tiffcp to join our test files into a multi-frame TIFF
# then we use tiffsplit to split them out again, and then we use
# tiffcp to recombine again.

. ${srcdir:-.}/common.sh
conjoined=deleteme-conjoined-$$.tif
reconjoined=deleteme-reconjoined-$$.tif
splitfile=deleteme-split-$$

operation=tiffcp
${TIFFCP} ${IMG_UNCOMPRESSED} ${conjoined}
status=$?
if test $status -eq 0
then
  operation=tiffsplit
  ${TIFFSPLIT} ${conjoined} ${splitfile}
  status=$?
  if test $status -eq 0
  then
    operation=tiffcp
    ${TIFFCP} ${splitfile}* ${reconjoined}
    status=$?
  fi
fi

if test $status -eq 0
then
  rm -f ${conjoined} ${splitfile}* ${reconjoined}
else
  echo "Test failed (${operation} returned ${status}).  Please inspect these output files:"
  echo "  " ${conjoined} ${splitfile}* ${reconjoined}
fi

exit $status
