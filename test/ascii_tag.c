/* $Id$ */

/*
 * Copyright (c) 2004, Andrey Kiselev  <dron@remotesensing.org>
 *
 * Permission to use, copy, modify, distribute, and sell this software and 
 * its documentation for any purpose is hereby granted without fee, provided
 * that (i) the above copyright notices and this permission notice appear in
 * all copies of the software and related documentation, and (ii) the names of
 * Sam Leffler and Silicon Graphics may not be used in any advertising or
 * publicity relating to the software without the specific, prior written
 * permission of Sam Leffler and Silicon Graphics.
 * 
 * THE SOFTWARE IS PROVIDED "AS-IS" AND WITHOUT WARRANTY OF ANY KIND, 
 * EXPRESS, IMPLIED OR OTHERWISE, INCLUDING WITHOUT LIMITATION, ANY 
 * WARRANTY OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE.  
 * 
 * IN NO EVENT SHALL SAM LEFFLER OR SILICON GRAPHICS BE LIABLE FOR
 * ANY SPECIAL, INCIDENTAL, INDIRECT OR CONSEQUENTIAL DAMAGES OF ANY KIND,
 * OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
 * WHETHER OR NOT ADVISED OF THE POSSIBILITY OF DAMAGE, AND ON ANY THEORY OF 
 * LIABILITY, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE 
 * OF THIS SOFTWARE.
 */

/*
 * TIFF Library
 *
 * Module to test ASCII tags read/write functions.
 */

#include "tif_config.h"

#include <stdio.h>

#ifdef HAVE_UNISTD_H 
# include <unistd.h> 
#endif 

#include "tiffio.h"

const char	*filename = "ascii_test.tiff";

static struct Tags {
	ttag_t		tag;
	const char	*value;
} ascii_tags[] = {
	{ TIFFTAG_DOCUMENTNAME, "Test TIFF image." },
	{ TIFFTAG_IMAGEDESCRIPTION, "Temporary test image." },
	{ TIFFTAG_MAKE, "This is not scanned image." },
	{ TIFFTAG_MODEL, "No scanner." },
	{ TIFFTAG_PAGENAME, "Test page." },
	{ TIFFTAG_SOFTWARE, "Libtiff library." },
	{ TIFFTAG_DATETIME, "September,03 2004." },
	{ TIFFTAG_ARTIST, "Andrey V. Kiselev" },
	{ TIFFTAG_HOSTCOMPUTER, "Debian GNU/Linux (Sarge)." },
	{ TIFFTAG_TARGETPRINTER, "No printer." },
	{ TIFFTAG_PIXAR_TEXTUREFORMAT, "No texture." },
	{ TIFFTAG_PIXAR_WRAPMODES, "No wrap." },
	{ TIFFTAG_COPYRIGHT, "Copyright (c) 2004, Andrey Kiselev." }
};
#define NTAGS   (sizeof (ascii_tags) / sizeof (ascii_tags[0]))

int
main(int argc, char **argv)
{
	TIFF		*tif;
	unsigned char	buf[1] = { 255 };
	int		i;

	tif = TIFFOpen(filename, "w");
	if (!tif) {
		fprintf (stderr, "Can't create test TIFF file %s.\n", filename);
		return 1;
	}

	if (!TIFFSetField(tif, TIFFTAG_IMAGEWIDTH, 1)) {
		fprintf (stderr, "Can't set ImageWidth tag.\n");
		goto bad;
	}
	if (!TIFFSetField(tif, TIFFTAG_IMAGELENGTH, 1)) {
		fprintf (stderr, "Can't set ImageLength tag.\n");
		goto bad;
	}
	if (!TIFFSetField(tif, TIFFTAG_BITSPERSAMPLE, 8)) {
		fprintf (stderr, "Can't set BitsPerSample tag.\n");
		goto bad;
	}
	if (!TIFFSetField(tif, TIFFTAG_SAMPLESPERPIXEL, 1)) {
		fprintf (stderr, "Can't set SamplesPerPixel tag.\n");
		goto bad;
	}
	if (!TIFFSetField(tif, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG)) {
		fprintf (stderr, "Can't set PlanarConfiguration tag.\n");
		goto bad;
	}
	if (!TIFFSetField(tif, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_MINISBLACK)) {
		fprintf (stderr, "Can't set PhotometricInterpretation tag.\n");
		goto bad;
	}

	for (i = 0; i < NTAGS; i++) {
		if (!TIFFSetField(tif, ascii_tags[i].tag,
				  ascii_tags[i].value)) {
			fprintf (stderr, "Can't set tag %d.\n",
				 ascii_tags[i].tag);
			goto bad;
		}
	}

	if (!TIFFWriteScanline(tif, buf, 0, 0) < 0) {
		fprintf (stderr, "Can't write image data.\n");
		goto bad;
	}

	TIFFClose(tif);
	unlink(filename);
	return 0;

bad:
	TIFFClose(tif);
	unlink(filename);
	return 1;
}

/* vim: set ts=8 sts=8 sw=8 noet: */
