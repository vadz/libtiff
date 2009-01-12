#!/bin/sh
#
# Basic sanity check for tiffcrop
#
# Created by Richard Nolde
#
. ${srcdir}/common.sh

TCSTATUS=0
TMPPREFIX=deleteme-
# Test rotations
for FILE in ${IMAGES}/*.tiff
  do
   BASE=`basename ${FILE} .tiff`
     "${TIFFCROP}" -R90 ${FILE} ${TMPPREFIX}${BASE}-R90.tiff
   RESULT=$?
   if [ "${RESULT}" != "0" ]
   then 
     echo "Failed to rotate ${FILE} by 90 degrees"
     echo "Check ${TMPPREFIX}${BASE}-R90.tiff"
     TCSTATUS=${RESULT}
   else
     :
     #echo "Successfully rotated ${FILE} by 90 degrees"
     rm  ${TMPPREFIX}${BASE}-R90.tiff
   fi
   done

# Test flip (mirror)
for FILE in ${IMAGES}/*.tiff
  do
   BASE=`basename ${FILE} .tiff`
     "${TIFFCROP}" -F both ${FILE} ${TMPPREFIX}${BASE}-doubleflip.tiff
   RESULT=$?
   if [ "${RESULT}" != "0" ]
   then 
     echo "Failed to flip ${FILE} horizontally and vertically"
     echo "Check ${TMPPREFIX}${BASE}-doubleflip.tiff"
     TCSTATUS=${RESULT}
   else
     :
     #echo "Successfully flipped ${FILE} horizontally and vertically"
     rm  ${TMPPREFIX}${BASE}-doubleflip.tiff
   fi
   done

# Test extracting a section 100 pixels wide and 100 pixels high
for FILE in ${IMAGES}/*.tiff
  do
   BASE=`basename ${FILE} .tiff`
     "${TIFFCROP}" -U px -E top -X 100 -Y 100 ${FILE} ${TMPPREFIX}${BASE}-100x100.tiff
   RESULT=$?
   if [ "${RESULT}" != "0" ]
   then 
     echo "Failed to extract 100 pixel by  100 pixel region from ${FILE}"
     echo "Check ${TMPPREFIX}${BASE}-100x100.tiff"
     TCSTATUS=${RESULT}
   else
     :
     #echo "Successfully flipped ${FILE} horizontally and vertically"
     rm  ${TMPPREFIX}${BASE}-100x100.tiff
   fi
   done

# Test extracting the first and fourth quarters from the left side.
for FILE in ${IMAGES}/*.tiff
  do
   BASE=`basename ${FILE} .tiff`
     "${TIFFCROP}" -E left -Z1:4,2:4 ${FILE} ${TMPPREFIX}${BASE}-Zones1and4FromLeftEdge.tiff
   RESULT=$?
   if [ "${RESULT}" != "0" ]
   then 
     echo "Failed to extract and composite first and fourth quarters of image from left side of ${FILE}"
     echo "Check ${TMPPREFIX}${BASE}-Zones1and4FromLeftEdge.tiff"
     TCSTATUS=${RESULT}
   else
     :
     #echo "Successfull extracted and composited first and fourth quarters of image from left side of ${FILE}"
     rm  ${TMPPREFIX}${BASE}-Zones1and4FromLeftEdge.tiff
   fi
   done



exit ${TCSTATUS}