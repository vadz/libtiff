# Common code fragment for tests
#
BUILDDIR=`pwd`
SRCDIR=`dirname $0`
SRCDIR=`cd $SRCDIR && pwd`
TOPSRCDIR=`cd $srcdir/.. && pwd`
TOOLS=`cd ../tools && pwd`
IMAGES="${SRCDIR}/images"

# Aliases for built tools
BMP2TIFF=${TOOLS}/bmp2tiff
FAX2PS=${TOOLS}/fax2ps
FAX2TIFF=${TOOLS}/fax2tiff
GIF2TIFF=${TOOLS}/gif2tiff
PAL2RGB=${TOOLS}/pal2rgb
PPM2TIFF=${TOOLS}/ppm2tiff
RAS2TIFF=${TOOLS}/ras2tiff
RAW2TIFF=${TOOLS}/raw2tiff
RGB2YCBCR=${TOOLS}/rgb2ycbcr
THUMBNAIL=${TOOLS}/thumbnail
TIFF2BW=${TOOLS}/tiff2bw
TIFF2PDF=${TOOLS}/tiff2pdf
TIFF2PS=${TOOLS}/tiff2ps
TIFF2RGBA=${TOOLS}/tiff2rgba
TIFFCMP=${TOOLS}/tiffcmp
TIFFCP=${TOOLS}/tiffcp
TIFFCROP=${TOOLS}/tiffcrop
TIFFDITHER=${TOOLS}/tiffdither
TIFFDUMP=${TOOLS}/tiffdump
TIFFINFO=${TOOLS}/tiffinfo
TIFFMEDIAN=${TOOLS}/tiffmedian
TIFFSET=${TOOLS}/tiffset
TIFFSPLIT=${TOOLS}/tiffsplit

# Aliases for input test files
IMG_MINISBLACK_1C_16B=${IMAGES}/minisblack-1c-16b.tiff
IMG_MINISBLACK_1C_8B=${IMAGES}/minisblack-1c-8b.tiff
IMG_MINISWHITE_1C_1B=${IMAGES}/miniswhite-1c-1b.tiff
IMG_PALETTE_1C_1B=${IMAGES}/palette-1c-1b.tiff
IMG_PALETTE_1C_4B=${IMAGES}/palette-1c-4b.tiff
IMG_PALETTE_1C_8B=${IMAGES}/palette-1c-8b.tiff
IMG_RGB_3C_16B=${IMAGES}/rgb-3c-16b.tiff
IMG_RGB_3C_8B=${IMAGES}/rgb-3c-8b.tiff
IMG_MINISBLACK_2C_8B_ALPHA=${IMAGES}/minisblack-2c-8b-alpha.tiff

# All uncompressed image files
IMG_UNCOMPRESSED="${IMG_MINISBLACK_1C_16B} ${IMG_MINISBLACK_1C_8B} ${IMG_MINISWHITE_1C_1B} ${IMG_PALETTE_1C_1B} ${IMG_PALETTE_1C_4B} ${IMG_PALETTE_1C_4B} ${IMG_PALETTE_1C_8B} ${IMG_RGB_3C_8B}"

if test "$VERBOSE" = TRUE
then
  set -x
fi

