# Tag Image File Format (TIFF) Software
#
# Copyright (C) 2004, Andrey Kiselev <dron@remotesensing.org>
#
# Permission to use, copy, modify, distribute, and sell this software and 
# its documentation for any purpose is hereby granted without fee, provided
# that (i) the above copyright notices and this permission notice appear in
# all copies of the software and related documentation, and (ii) the names of
# Sam Leffler and Silicon Graphics may not be used in any advertising or
# publicity relating to the software without the specific, prior written
# permission of Sam Leffler and Silicon Graphics.
# 
# THE SOFTWARE IS PROVIDED "AS-IS" AND WITHOUT WARRANTY OF ANY KIND, 
# EXPRESS, IMPLIED OR OTHERWISE, INCLUDING WITHOUT LIMITATION, ANY 
# WARRANTY OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE.  
# 
# IN NO EVENT SHALL SAM LEFFLER OR SILICON GRAPHICS BE LIABLE FOR
# ANY SPECIAL, INCIDENTAL, INDIRECT OR CONSEQUENTIAL DAMAGES OF ANY KIND,
# OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
# WHETHER OR NOT ADVISED OF THE POSSIBILITY OF DAMAGE, AND ON ANY THEORY OF 
# LIABILITY, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE 
# OF THIS SOFTWARE.

# Process this file with automake to produce Makefile.in.

LIBTIFF = $(top_srcdir)/libtiff/libtiff.la

EXTRA_DIST = Makefile.vc

if HAVE_OPENGL
BIN_TIFFGT = tiffgt
else
BIN_TIFFGT =
endif

bin_PROGRAMS = \
	fax2ps \
	fax2tiff \
	gif2tiff \
	pal2rgb \
	ppm2tiff \
	ras2tiff \
	raw2tiff \
	rgb2ycbcr \
	thumbnail \
	tiff2bw \
	tiff2pdf \
	tiff2ps \
	tiff2rgba \
	tiffcmp \
	tiffcp \
	tiffdither \
	tiffdump \
	tiffgt \
	tiffinfo \
	tiffmedian \
	tiffset \
	tiffsplit \
	$(BIN_TIFFGT)
EXTRA_PROGRAMS = $(BIN_TIFFGT) sgi2tiff sgisv ycbcr

fax2ps_SOURCES = fax2ps.c
fax2ps_LDADD = $(LIBTIFF)
fax2tiff_SOURCES = fax2tiff.c
fax2tiff_LDADD = $(LIBTIFF)
gif2tiff_SOURCES = gif2tiff.c
gif2tiff_LDADD = $(LIBTIFF)
pal2rgb_SOURCES = pal2rgb.c
pal2rgb_LDADD = $(LIBTIFF)
ppm2tiff_SOURCES = ppm2tiff.c
ppm2tiff_LDADD = $(LIBTIFF)
ras2tiff_SOURCES = ras2tiff.c
ras2tiff_LDADD = $(LIBTIFF)
raw2tiff_SOURCES = raw2tiff.c
raw2tiff_LDADD = $(LIBTIFF)
rgb2ycbcr_SOURCES = rgb2ycbcr.c
rgb2ycbcr_LDADD = $(LIBTIFF)
thumbnail_SOURCES = thumbnail.c
thumbnail_LDADD = $(LIBTIFF)
tiff2bw_SOURCES = tiff2bw.c
tiff2bw_LDADD = $(LIBTIFF)
tiff2pdf_SOURCES = tiff2pdf.c
tiff2pdf_LDADD = $(LIBTIFF)
tiff2ps_SOURCES = tiff2ps.c
tiff2ps_LDADD = $(LIBTIFF)
tiff2rgba_SOURCES = tiff2rgba.c
tiff2rgba_LDADD = $(LIBTIFF)
tiffcmp_SOURCES = tiffcmp.c
tiffcmp_LDADD = $(LIBTIFF)
tiffcp_SOURCES = tiffcp.c
tiffcp_LDADD = $(LIBTIFF)
tiffdither_SOURCES = tiffdither.c
tiffdither_LDADD = $(LIBTIFF)
tiffdump_SOURCES = tiffdump.c
tiffdump_LDADD = $(LIBTIFF)
tiffinfo_SOURCES = tiffinfo.c
tiffinfo_LDADD = $(LIBTIFF)
tiffmedian_SOURCES = tiffmedian.c
tiffmedian_LDADD = $(LIBTIFF)
tiffset_SOURCES = tiffset.c
tiffset_LDADD = $(LIBTIFF)
tiffsplit_SOURCES = tiffsplit.c
tiffsplit_LDADD = $(LIBTIFF)
tiffgt_SOURCES = tiffgt.c
tiffgt_LDADD = $(LIBTIFF) $(OPENGL_LIBS)

INCLUDES = -I$(top_srcdir)/libtiff

