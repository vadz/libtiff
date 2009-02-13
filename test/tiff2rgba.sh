#!/bin/sh
#
# Test a few case of the RGBA interface.
#
. ${srcdir:-.}/common.sh


outfile=deleteme-$$.tif
operation=tiff2rgba

for FILE in \
    minisblack-2c-8b-alpha.tiff \
    palette-1c-1b.tiff \
    palette-1c-8b.tiff \
    rgb-3c-8b.tiff \
    minisblack-1c-8b.tiff ; do 

  ${TIFF2RGBA} images/$FILE tiff2rgba-$FILE
  status=$?

  if test $status -eq 0 ; then
    rm -f tiff2rgba-$FILE
  else
    echo "Test failed (${operation} returns ${status}).  Please inspect these output files:"
    echo "  " tiff2rgba-$FILE
  fi
done

exit $status
