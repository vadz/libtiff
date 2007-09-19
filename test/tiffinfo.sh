#!/bin/sh
#
# A sample test script which demonstrates executing a libtiff tool
# on an image file.  It might be sufficient that the tool executes
# without reporting an error, or perhaps the output of the tool
# should be validated somehow.  Regardless, the test is considered
# to have passed if the script returns 0.  This could be done by
# executing a test program as the last step, or using 'exit 0' or
# 'exit 1' to return explicit test status.
#
# In this case, we direct the normal tool output to /dev/null with
# the hope that when an error is reported, the errors are reported
# to stderr.  An alternative would be to collect the output to a
# temporary file and inspect the output for expected values.
#
set -e # Exit on any error
#set -x # Trace execution
. ${srcdir}/common.sh
"${TOOLS}/tiffinfo" "${IMAGES}/minisblack-1c-16b.tiff" > /dev/null
