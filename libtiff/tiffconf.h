/* $Header$ */
/*
 * Copyright (c) 1988-1997 Sam Leffler
 * Copyright (c) 1991-1997 Silicon Graphics, Inc.
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

#ifndef _TIFFCONF_
#define	_TIFFCONF_
/*
 * Library Configuration Definitions.
 *
 * This file defines the default configuration for the library.
 * If the target system does not have make or a way to specify
 * #defines on the command line, this file can be edited to
 * configure the library.  Otherwise, one can override portability
 * and configuration-related definitions from a Makefile or command
 * line by defining COMPRESSION_SUPPORT (see below).
 */

/*
 * General portability-related defines:
 *
 * HAVE_IEEEFP		define as 0 or 1 according to the floating point
 *			format suported by the machine
 * HAVE_MMAP		enable support for memory mapping read-only files;
 *			this is typically deduced by the configure script
 */
#ifndef HAVE_IEEEFP
#define	HAVE_IEEEFP	1
#endif

/*
 * ``Orthogonal Features''
 *
 * STRIPCHOP_DEFAULT	default handling of strip chopping support (whether
 *			or not to convert single-strip uncompressed images
 *			to mutiple strips of ~8Kb--to reduce memory use)
 * DEFAULT_EXTRASAMPLE_AS_ALPHA
 *                      The RGBA interface will treat a fourth sample with
 *                      no EXTRASAMPLE_ value as being ASSOCALPHA.  Many
 *                      packages produce RGBA files but don't mark the alpha
 *                      properly.
 * CHECK_JPEG_YCBCR_SUBSAMPLING
 *                      Enable picking up YCbCr subsampling info from the
 *                      JPEG data stream to support files lacking the tag.
 *                      See Bug 168 in Bugzilla, and JPEGFixupTestSubsampling()
 *                      for details. 
 */
#ifndef STRIPCHOP_DEFAULT
#define	STRIPCHOP_DEFAULT	TIFF_STRIPCHOP	/* default is to enable */
#endif
#ifndef DEFAULT_EXTRASAMPLE_AS_ALPHA
#define DEFAULT_EXTRASAMPLE_AS_ALPHA 1
#endif
#ifndef CHECK_JPEG_YCBCR_SUBSAMPLING
#define CHECK_JPEG_YCBCR_SUBSAMPLING 1
#endif

/*
 * Feature support definitions.
 * XXX: These macros are obsoleted. Don't use them in your apps!
 * Macros stays here for backward compatibility and should be always defined.
 */
#define	SUBIFD_SUPPORT
#define	COLORIMETRY_SUPPORT
#define	YCBCR_SUPPORT
#define	CMYK_SUPPORT
#define	ICC_SUPPORT
#define PHOTOSHOP_SUPPORT
#define IPTC_SUPPORT

#endif /* _TIFFCONF_ */
