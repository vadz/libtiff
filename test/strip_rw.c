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
 * Numerical arrays used to test libtiff's read/write functions.
 */

#include "tif_config.h"

#include <stdio.h>

#ifdef HAVE_UNISTD_H 
# include <unistd.h> 
#endif 

#include "tiffio.h"
#include "test_arrays.h"

const char	*filename = "strip_test.tiff";

int
write_strips(TIFF *tif, tdata_t array, tsize_t size)
{
	tstrip_t    strip;
	tsize_t	    stripsize, offset;

	stripsize = TIFFStripSize(tif);
	if (!stripsize) {
		fprintf (stderr, "Wrong size of strip.\n");
			return -1;
	}

	for (offset = 0, strip = 0; offset < size; offset+=stripsize, strip++) {
		if (TIFFWriteEncodedStrip(tif, strip, (char *)array + offset,
					  stripsize) < 0) {
			fprintf (stderr, "Can't write strip %d.\n", (int)strip);
			return -1;
		}
        }

	return 0;
}

int
write_scanlines(TIFF *tif, tdata_t array, tsize_t size)
{
	uint32	    length, row;
	tsize_t	    scanlinesize, offset;

	if (!TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &length)) {
		fprintf (stderr, "Can't get tag %d.\n", TIFFTAG_IMAGELENGTH);
		return -1;
	}
	
	scanlinesize = TIFFScanlineSize(tif);
	if (!scanlinesize) {
		fprintf (stderr, "Wrong size of scanline.\n");
			return -1;
	}

	for (offset = 0, row = 0; row < length; offset+=scanlinesize, row++) {
		if (TIFFWriteScanline(tif, (char *)array + offset, row, 0) < 0) {
			fprintf (stderr,
				 "Can't write image data at row %ld.\n", row);
			return -1;
		}
        }

	return 0;
}

int
main(int argc, char **argv)
{
	TIFF	    *tif;
	uint16	    value;
	uint16	    spp = 1;
	uint16	    bps = 8;
	uint16	    photometric = PHOTOMETRIC_MINISBLACK;
	uint16	    rows_per_strip = 1;
	uint16	    planarconfig = PLANARCONFIG_CONTIG;

	/* Test whether we can write tags. */
	tif = TIFFOpen(filename, "w");
	if (!tif) {
		fprintf (stderr, "Can't create test TIFF file %s.\n", filename);
		return 1;
	}

	if (!TIFFSetField(tif, TIFFTAG_IMAGEWIDTH, XSIZE)) {
		fprintf (stderr, "Can't set ImageWidth tag.\n");
		goto failure;
	}
	if (!TIFFSetField(tif, TIFFTAG_IMAGELENGTH, YSIZE)) {
		fprintf (stderr, "Can't set ImageLength tag.\n");
		goto failure;
	}
	if (!TIFFSetField(tif, TIFFTAG_BITSPERSAMPLE, bps)) {
		fprintf (stderr, "Can't set BitsPerSample tag.\n");
		goto failure;
	}
	if (!TIFFSetField(tif, TIFFTAG_SAMPLESPERPIXEL, spp)) {
		fprintf (stderr, "Can't set SamplesPerPixel tag.\n");
		goto failure;
	}
	if (!TIFFSetField(tif, TIFFTAG_ROWSPERSTRIP, rows_per_strip)) {
		fprintf (stderr, "Can't set SamplesPerPixel tag.\n");
		goto failure;
	}
	if (!TIFFSetField(tif, TIFFTAG_PLANARCONFIG, planarconfig)) {
		fprintf (stderr, "Can't set PlanarConfiguration tag.\n");
		goto failure;
	}
	if (!TIFFSetField(tif, TIFFTAG_PHOTOMETRIC, photometric)) {
		fprintf (stderr, "Can't set PhotometricInterpretation tag.\n");
		goto failure;
	}

	if (write_strips(tif, (tdata_t) byte_array1, byte_array1_size) < 0) {
		fprintf (stderr, "Can't write image data.\n");
		goto failure;
	}

	TIFFClose(tif);
	
	/* Ok, now test whether we can read written values. */
	tif = TIFFOpen(filename, "r");
	if (!tif) {
		fprintf (stderr, "Can't open test TIFF file %s.\n", filename);
		return 1;
	}
	if (!TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &value)
	    || value != XSIZE) {
		fprintf (stderr, "Can't get tag %d.\n", TIFFTAG_IMAGEWIDTH);
		goto failure;
	}
	if (!TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &value)
	    || value != YSIZE) {
		fprintf (stderr, "Can't get tag %d.\n", TIFFTAG_IMAGELENGTH);
		goto failure;
	}
	if (!TIFFGetField(tif, TIFFTAG_BITSPERSAMPLE, &value)
	    || value != bps) {
		fprintf (stderr, "Can't get tag %d.\n", TIFFTAG_BITSPERSAMPLE);
		goto failure;
	}
	if (!TIFFGetField(tif, TIFFTAG_PHOTOMETRIC, &value)
	    || value != photometric) {
		fprintf (stderr, "Can't get tag %d.\n", TIFFTAG_PHOTOMETRIC);
		goto failure;
	}
	if (!TIFFGetField(tif, TIFFTAG_SAMPLESPERPIXEL, &value)
	    || value != spp) {
		fprintf (stderr, "Can't get tag %d.\n", TIFFTAG_SAMPLESPERPIXEL);
		goto failure;
	}
	if (!TIFFGetField(tif, TIFFTAG_ROWSPERSTRIP, &value)
	    || value != rows_per_strip) {
		fprintf (stderr, "Can't get tag %d.\n", TIFFTAG_ROWSPERSTRIP);
		goto failure;
	}
	if (!TIFFGetField(tif, TIFFTAG_PLANARCONFIG, &value)
	    || value != planarconfig) {
		fprintf (stderr, "Can't get tag %d.\n", TIFFTAG_PLANARCONFIG);
		goto failure;
	}

	TIFFClose(tif);
	
	/* All tests passed; delete file and exit with success status. */
	unlink(filename);
	return 0;

failure:
	/* Something goes wrong; close file and return unsuccessful status. */
	TIFFClose(tif);
	unlink(filename);
	return 1;
}

/* vim: set ts=8 sts=8 sw=8 noet: */
