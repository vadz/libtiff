/* $Id$ */

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

/*
 * TIFF Library UNIX-specific Routines. These are should also work with the
 * Windows Common RunTime Library.
 */

#define _LARGE_FILE_API

#include "tif_config.h"

#ifdef HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif

#include <stdarg.h>
#include <stdlib.h>
#include <sys/stat.h>

#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

#ifdef HAVE_FCNTL_H
# include <fcntl.h>
#endif

#ifdef HAVE_IO_H
# include <io.h>
#endif

#include "tiffiop.h"

static tmsize_t
_tiffReadProc(thandle_t fd, void* buf, tmsize_t size)
{
	/* tmsize_t is 64bit on 64bit systems, but the standard library read takes
	 * signed 32bit sizes, so we loop through the data in suitable 32bit sized
	 * chunks */
	uint8* ma;
	uint64 mb;
	size_t n;
	ssize_t o;
	tmsize_t p;
	ma=(uint8*)buf;
	mb=size;
	p=0;
	while (mb>0)
	{
		n=SSIZE_MAX;
		if ((uint64)n>mb)
			n=(size_t)mb;
		o=read((int)fd,(void*)ma,n);
		if (o<0)
			return(0);
		ma+=o;
		mb-=o;
		p+=o;
		if ((size_t)o!=n)
			break;
	}
	return(p);
}

static tmsize_t
_tiffWriteProc(thandle_t fd, void* buf, tmsize_t size)
{
	/* tmsize_t is 64bit on 64bit systems, but the standard library write takes
	 * signed 32bit sizes, so we loop through the data in suitable 32bit sized
	 * chunks */
	uint8* ma;
	uint64 mb;
	size_t n;
	ssize_t o;
	tmsize_t p;
	ma=(uint8*)buf;
	mb=size;
	p=0;
	while (mb>0)
	{
		n=SSIZE_MAX;
		if ((uint64)n>mb)
			n=(size_t)mb;
		o=write((int)fd,(void*)ma,n);
		if (o<0)
			return(0);
		ma+=o;
		mb-=o;
		p+=o;
		if ((size_t)o!=n)
			break;
	}
	return(p);
}

static uint64
_tiffSeekProc(thandle_t fd, uint64 off, int whence)
{
	return((uint64)lseek64((int)fd,(off64_t)off,whence));
}

static int
_tiffCloseProc(thandle_t fd)
{
	return(close((int)fd));
}

static uint64
_tiffSizeProc(thandle_t fd)
{
#ifdef _AM29K
	off64_t fsize;
	fsize=lseek64((int)fd,0,SEEK_END);
	if (fsize==(off64_t)(-1))
		return(0);
	else
		return((uint64)fsize);
#else
	struct stat64 sb;
	if (fstat64((int)fd,&sb)<0)
		return(0);
	else
		return((uint64)sb.st_size);
#endif
}

#ifdef HAVE_MMAP
#include <sys/mman.h>

static int
_tiffMapProc(thandle_t fd, void** pbase, tmsize_t* psize)
{
	uint64 size64 = _tiffSizeProc(fd);
	tmsize_t sizem = (tmsize_t)size64;
	if ((uint64)sizem==size64) {
		*pbase = (void*)
		    mmap(0, (size_t)sizem, PROT_READ, MAP_SHARED, (int) fd, 0);
		if (*pbase != (void*) -1) {
			*psize = (tmsize_t)sizem;
			return (1);
		}
	}
	return (0);
}

static void
_tiffUnmapProc(thandle_t fd, void* base, tmsize_t size)
{
	(void) fd;
	(void) munmap(base, (off_t) size);
}
#else /* !HAVE_MMAP */
static int
_tiffMapProc(thandle_t fd, void** pbase, tmsize_t* psize)
{
	(void) fd; (void) pbase; (void) psize;
	return (0);
}

static void
_tiffUnmapProc(thandle_t fd, void* base, tmsize_t size)
{
	(void) fd; (void) base; (void) size;
}
#endif /* !HAVE_MMAP */

/*
 * Open a TIFF file descriptor for read/writing.
 */
TIFF*
TIFFFdOpen(int fd, const char* name, const char* mode)
{
	TIFF* tif;

	tif = TIFFClientOpen(name, mode,
	    (thandle_t) fd,
	    _tiffReadProc, _tiffWriteProc,
	    _tiffSeekProc, _tiffCloseProc, _tiffSizeProc,
	    _tiffMapProc, _tiffUnmapProc);
	if (tif)
		tif->tif_fd = fd;
	return (tif);
}

/*
 * Open a TIFF file for read/writing.
 */
TIFF*
TIFFOpen(const char* name, const char* mode)
{
	static const char module[] = "TIFFOpen";
	int m, fd;
	TIFF* tif;

	m = _TIFFgetMode(mode, module);
	if (m == -1)
		return ((TIFF*)0);

/* for cygwin and mingw */
#ifdef O_BINARY
	m |= O_BINARY;
#endif

	m |= O_LARGEFILE;

#ifdef _AM29K
	fd = open(name, m);
#else
	fd = open(name, m, 0666);
#endif
	if (fd < 0) {
		TIFFErrorExt(0, module, "%s: Cannot open", name);
		return ((TIFF *)0);
	}

	tif = TIFFFdOpen((int)fd, name, mode);
	if(!tif)
		close(fd);
	return tif;
}

#ifdef __WIN32__
#include <windows.h>
/*
 * Open a TIFF file with a Unicode filename, for read/writing.
 */
TIFF*
TIFFOpenW(const wchar_t* name, const char* mode)
{
	static const char module[] = "TIFFOpenW";
	int m, fd;
	int mbsize;
	char *mbname;
	TIFF* tif;

	m = _TIFFgetMode(mode, module);
	if (m == -1)
		return ((TIFF*)0);

/* for cygwin and mingw */
#ifdef O_BINARY
	m |= O_BINARY;
#endif


	m |= O_LARGEFILE;

	fd = _wopen(name, m, 0666);
	if (fd < 0) {
		TIFFErrorExt(0, module, "%s: Cannot open", name);
		return ((TIFF *)0);
	}

	mbname = NULL;
	mbsize = WideCharToMultiByte(CP_ACP, 0, name, -1, NULL, 0, NULL, NULL);
	if (mbsize > 0) {
		mbname = _TIFFmalloc(mbsize);
		if (!mbname) {
			TIFFErrorExt(0, module,
			"Can't allocate space for filename conversion buffer");
			return ((TIFF*)0);
		}

		WideCharToMultiByte(CP_ACP, 0, name, -1, mbname, mbsize,
				    NULL, NULL);
	}

	tif = TIFFFdOpen((int)fd, (mbname != NULL) ? mbname : "<unknown>",
			 mode);
	
	_TIFFfree(mbname);
	
	if(!tif)
		close(fd);
	return tif;
}
#endif

void*
_TIFFmalloc(tmsize_t s)
{
	return (malloc((size_t) s));
}

void
_TIFFfree(void* p)
{
	free(p);
}

void*
_TIFFrealloc(void* p, tmsize_t s)
{
	return (realloc(p, (size_t) s));
}

void
_TIFFmemset(void* p, int v, tmsize_t c)
{
	memset(p, v, (size_t) c);
}

void
_TIFFmemcpy(void* d, const void* s, tmsize_t c)
{
	memcpy(d, s, (size_t) c);
}

int
_TIFFmemcmp(const void* p1, const void* p2, tmsize_t c)
{
	return (memcmp(p1, p2, (size_t) c));
}

static void
unixWarningHandler(const char* module, const char* fmt, va_list ap)
{
	if (module != NULL)
		fprintf(stderr, "%s: ", module);
	fprintf(stderr, "Warning, ");
	vfprintf(stderr, fmt, ap);
	fprintf(stderr, ".\n");
}
TIFFErrorHandler _TIFFwarningHandler = unixWarningHandler;

static void
unixErrorHandler(const char* module, const char* fmt, va_list ap)
{
	if (module != NULL)
		fprintf(stderr, "%s: ", module);
	vfprintf(stderr, fmt, ap);
	fprintf(stderr, ".\n");
}
TIFFErrorHandler _TIFFerrorHandler = unixErrorHandler;
