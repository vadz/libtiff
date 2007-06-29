/* $Id$ */

/*
 * Copyright (c) 1988-1996 Sam Leffler
 * Copyright (c) 1991-1996 Silicon Graphics, Inc.
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
 * TIFF Library UNIX-specific Routines.
 */
#include "tiffiop.h"
#include <iostream>

#ifndef __VMS
using namespace std;
#endif

class tiffis_data
{
  public:

	istream	*myIS;
        tmsize_t myStreamStartPos;
};

class tiffos_data
{
  public:

	ostream	*myOS;
	tmsize_t myStreamStartPos;
};

static tmsize_t
_tiffosReadProc(thandle_t, void*, tmsize_t)
{
        return 0;
}

static tmsize_t
_tiffisReadProc(thandle_t fd, void* buf, tmsize_t size)
{
        tiffis_data	*data = (tiffis_data *) fd;

        data->myIS->read((char *) buf, (size_t) size);

        return data->myIS->gcount();
}

static tmsize_t
_tiffosWriteProc(thandle_t fd, void* buf, tmsize_t size)
{
	tiffos_data	*data = (tiffos_data *)fd;
	ostream		*os = data->myOS;
	off_t		pos = os->tellp();

	os->write((const char *) buf, (size_t) size);

	return ((off_t) os->tellp()) - pos;
}

static tmsize_t
_tiffisWriteProc(thandle_t, void*, tmsize_t)
{
	return 0;
}

static uint64
_tiffosSeekProc(thandle_t fd, uint64 off, int whence)
{
	tiffos_data	*data = (tiffos_data *) fd;
	ostream	*os = data->myOS;

	// if the stream has already failed, don't do anything
	if( os->fail() )
		return os->tellp();

	switch(whence) {
	case SEEK_SET:
	    	os->seekp(data->myStreamStartPos + off, ios::beg);
		break;
	case SEEK_CUR:
		os->seekp(off, ios::cur);
		break;
	case SEEK_END:
		os->seekp(off, ios::end);
		break;
	}

	// Attempt to workaround problems with seeking past the end of the
	// stream.  ofstream doesn't have a problem with this but
	// ostrstream/ostringstream does. In that situation, add intermediate
	// '\0' characters.
	if( os->fail() ) {
#ifdef __VMS
		int		old_state;
#else
		ios::iostate	old_state;
#endif
		toff_t		origin;

		old_state = os->rdstate();
		// reset the fail bit or else tellp() won't work below
		os->clear(os->rdstate() & ~ios::failbit);
		switch( whence ) {
			case SEEK_SET:
                        default:
				origin = data->myStreamStartPos;
				break;
			case SEEK_CUR:
				origin = os->tellp();
				break;
			case SEEK_END:
				os->seekp(0, ios::end);
				origin = os->tellp();
				break;
		}
		// restore original stream state
		os->clear(old_state);	

		// only do something if desired seek position is valid
		if( origin + off > (toff_t) data->myStreamStartPos ) {
			toff_t	num_fill;

			// clear the fail bit 
			os->clear(os->rdstate() & ~ios::failbit);

			// extend the stream to the expected size
			os->seekp(0, ios::end);
			num_fill = origin + off - (toff_t)os->tellp();
			for( toff_t i = 0; i < num_fill; i++ )
				os->put('\0');

			// retry the seek
			os->seekp(origin + off, ios::beg);
		}
	}

	return os->tellp();
}

static toff_t
_tiffisSeekProc(thandle_t fd, uint64 off, int whence)
{
	tiffis_data	*data = (tiffis_data *)fd;

	switch(whence) {
	case SEEK_SET:
		data->myIS->seekg(data->myStreamStartPos + off, ios::beg);
		break;
	case SEEK_CUR:
		data->myIS->seekg(off, ios::cur);
		break;
	case SEEK_END:
		data->myIS->seekg(off, ios::end);
		break;
	}

	return ((long)data->myIS->tellg()) - data->myStreamStartPos;
}

static uint64
_tiffosSizeProc(thandle_t fd)
{
	tiffos_data	*data = (tiffos_data *)fd;
	ostream		*os = data->myOS;
	off_t		pos = os->tellp();
	off_t		len;

	os->seekp(0, ios::end);
	len = os->tellp();
	os->seekp(pos);

	return (uint64) len;
}

static uint64
_tiffisSizeProc(thandle_t fd)
{
	tiffis_data	*data = (tiffis_data *)fd;
	off_t		pos = data->myIS->tellg();
	off_t		len;

	data->myIS->seekg(0, ios::end);
	len = data->myIS->tellg();
	data->myIS->seekg(pos);

	return (uint64) len;
}

static int
_tiffosCloseProc(thandle_t fd)
{
	// Our stream was not allocated by us, so it shouldn't be closed by us.
	delete (tiffos_data *) fd;
	return 0;
}

static int
_tiffisCloseProc(thandle_t fd)
{
	// Our stream was not allocated by us, so it shouldn't be closed by us.
	delete (tiffis_data *) fd;
	return 0;
}

static int
_tiffDummyMapProc(thandle_t , void** , tmsize_t* )
{
	return (0);
}

static void
_tiffDummyUnmapProc(thandle_t , void* , tmsize_t )
{
}

/*
 * Open a TIFF file descriptor for read/writing.
 */
static TIFF*
_tiffStreamOpen(const char* name, const char* mode, void *fd)
{
	TIFF*	tif;

	if( strchr(mode, 'w') ) {
		tiffos_data	*data = new tiffos_data;
		data->myOS = (ostream *)fd;
		data->myStreamStartPos = data->myOS->tellp();

		// Open for writing.
		tif = TIFFClientOpen(name, mode,
				(thandle_t) data,
				_tiffosReadProc,
                                _tiffosWriteProc,
				_tiffosSeekProc,
                                _tiffosCloseProc,
				_tiffosSizeProc,
				_tiffDummyMapProc,
                                _tiffDummyUnmapProc);
	} else {
		tiffis_data	*data = new tiffis_data;
		data->myIS = (istream *)fd;
		data->myStreamStartPos = data->myIS->tellg();
		// Open for reading.
		tif = TIFFClientOpen(name, mode,
				(thandle_t) data,
				_tiffisReadProc,
                                _tiffisWriteProc,
				_tiffisSeekProc,
                                _tiffisCloseProc,
				_tiffisSizeProc,
				_tiffDummyMapProc,
                                _tiffDummyUnmapProc);
	}

	return (tif);
}

TIFF*
TIFFStreamOpen(const char* name, ostream *os)
{
	// If os is either a ostrstream or ostringstream, and has no data
	// written to it yet, then tellp() will return -1 which will break us.
	// We workaround this by writing out a dummy character and
	// then seek back to the beginning.
	if( !os->fail() && (int)os->tellp() < 0 ) {
		*os << '\0';
		os->seekp(0);
	}

	// NB: We don't support mapped files with streams so add 'm'
	return _tiffStreamOpen(name, "wm", os);
}

TIFF*
TIFFStreamOpen(const char* name, istream *is)
{
	// NB: We don't support mapped files with streams so add 'm'
	return _tiffStreamOpen(name, "rm", is);
}

/* vim: set ts=8 sts=8 sw=8 noet: */
