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
 * TIFF Library.
 *
 * Directory Write Support Routines.
 */
#include "tiffiop.h"

#ifdef NDEF

static	int TIFFWriteNormalTag(TIFF*, TIFFDirEntry*, const TIFFFieldInfo*);
static	void TIFFSetupShortLong(TIFF*, ttag_t, TIFFDirEntry*, uint32);
static	void TIFFSetupShort(TIFF*, ttag_t, TIFFDirEntry*, uint16);
static	int TIFFSetupShortPair(TIFF*, ttag_t, TIFFDirEntry*);
static	int TIFFWritePerSampleShorts(TIFF*, ttag_t, TIFFDirEntry*);
static	int TIFFWritePerSampleAnys(TIFF*, TIFFDataType, ttag_t, TIFFDirEntry*);
static	int TIFFWriteShortTable(TIFF*, ttag_t, TIFFDirEntry*, uint32, uint16**);
static	int TIFFWriteShortArray(TIFF*, TIFFDirEntry*, uint16*);
static	int TIFFWriteLongArray(TIFF *, TIFFDirEntry*, uint32*);
static	int TIFFWriteRationalArray(TIFF *, TIFFDirEntry*, float*);
static	int TIFFWriteFloatArray(TIFF *, TIFFDirEntry*, float*);
static	int TIFFWriteDoubleArray(TIFF *, TIFFDirEntry*, double*);
static	int TIFFWriteByteArray(TIFF*, TIFFDirEntry*, char*);
static	int TIFFWriteAnyArray(TIFF*,
	    TIFFDataType, ttag_t, TIFFDirEntry*, uint32, double*);
static	int TIFFWriteTransferFunction(TIFF*, TIFFDirEntry*);
static	int TIFFWriteInkNames(TIFF*, TIFFDirEntry*);
static	int TIFFWriteData(TIFF*, TIFFDirEntry*, char*);

#define	WriteRationalPair(type, tag1, v1, tag2, v2) {		\
	TIFFWriteRational((tif), (type), (tag1), (dir), (v1))	\
	TIFFWriteRational((tif), (type), (tag2), (dir)+1, (v2))	\
	(dir)++;						\
}
#define	TIFFWriteRational(tif, type, tag, dir, v)		\
	(dir)->tdir_tag = (tag);				\
	(dir)->tdir_type = (type);				\
	(dir)->tdir_count = 1;					\
	if (!TIFFWriteRationalArray((tif), (dir), &(v)))	\
		goto bad;

#endif

/* ddddddddddddddddddddddddddddddddddddddddddddddddddd */

#ifdef HAVE_IEEEFP
#define TIFFCvtNativeToIEEEFloat(tif, n, fp)
#define TIFFCvtNativeToIEEEDouble(tif, n, dp)
#else
extern void TIFFCvtNativeToIEEEFloat(TIFF* tif, uint32 n, float* fp);
extern void TIFFCvtNativeToIEEEDouble(TIFF* tif, uint32 n, double* dp);
#endif

static int TIFFWriteDirectoryTagSampleformatPerSample(TIFF* tif, uint32* ndir, TIFFDirEntry* dir, uint16 tag, double value);

static int TIFFWriteDirectoryTagAscii(TIFF* tif, uint32* ndir, TIFFDirEntry* dir, uint16 tag, uint32 count, char* value);
static int TIFFWriteDirectoryTagUndefinedArray(TIFF* tif, uint32* ndir, TIFFDirEntry* dir, uint16 tag, uint32 count, uint8* value);
static int TIFFWriteDirectoryTagByte(TIFF* tif, uint32* ndir, TIFFDirEntry* dir, uint16 tag, uint8 value);
static int TIFFWriteDirectoryTagByteArray(TIFF* tif, uint32* ndir, TIFFDirEntry* dir, uint16 tag, uint32 count, uint8* value);
static int TIFFWriteDirectoryTagBytePerSample(TIFF* tif, uint32* ndir, TIFFDirEntry* dir, uint16 tag, uint8 value);
static int TIFFWriteDirectoryTagSbyte(TIFF* tif, uint32* ndir, TIFFDirEntry* dir, uint16 tag, int8 value);
static int TIFFWriteDirectoryTagSbyteArray(TIFF* tif, uint32* ndir, TIFFDirEntry* dir, uint16 tag, uint32 count, int8* value);
static int TIFFWriteDirectoryTagSbytePerSample(TIFF* tif, uint32* ndir, TIFFDirEntry* dir, uint16 tag, int8 value);
static int TIFFWriteDirectoryTagShort(TIFF* tif, uint32* ndir, TIFFDirEntry* dir, uint16 tag, uint16 value);
static int TIFFWriteDirectoryTagShortArray(TIFF* tif, uint32* ndir, TIFFDirEntry* dir, uint16 tag, uint32 count, uint16* value);
static int TIFFWriteDirectoryTagShortPerSample(TIFF* tif, uint32* ndir, TIFFDirEntry* dir, uint16 tag, uint16 value);
static int TIFFWriteDirectoryTagSshort(TIFF* tif, uint32* ndir, TIFFDirEntry* dir, uint16 tag, int16 value);
static int TIFFWriteDirectoryTagSshortArray(TIFF* tif, uint32* ndir, TIFFDirEntry* dir, uint16 tag, uint32 count, int16* value);
static int TIFFWriteDirectoryTagSshortPerSample(TIFF* tif, uint32* ndir, TIFFDirEntry* dir, uint16 tag, int16 value);
static int TIFFWriteDirectoryTagLong(TIFF* tif, uint32* ndir, TIFFDirEntry* dir, uint16 tag, uint32 value);
static int TIFFWriteDirectoryTagLongArray(TIFF* tif, uint32* ndir, TIFFDirEntry* dir, uint16 tag, uint32 count, uint32* value);
static int TIFFWriteDirectoryTagLongPerSample(TIFF* tif, uint32* ndir, TIFFDirEntry* dir, uint16 tag, uint32 value);
static int TIFFWriteDirectoryTagSlong(TIFF* tif, uint32* ndir, TIFFDirEntry* dir, uint16 tag, int32 value);
static int TIFFWriteDirectoryTagSlongArray(TIFF* tif, uint32* ndir, TIFFDirEntry* dir, uint16 tag, uint32 count, int32* value);
static int TIFFWriteDirectoryTagSlongPerSample(TIFF* tif, uint32* ndir, TIFFDirEntry* dir, uint16 tag, int32 value);
static int TIFFWriteDirectoryTagLong8(TIFF* tif, uint32* ndir, TIFFDirEntry* dir, uint16 tag, uint64_new value);
static int TIFFWriteDirectoryTagLong8Array(TIFF* tif, uint32* ndir, TIFFDirEntry* dir, uint16 tag, uint32 count, uint64_new* value);
static int TIFFWriteDirectoryTagSlong8(TIFF* tif, uint32* ndir, TIFFDirEntry* dir, uint16 tag, int64_new value);
static int TIFFWriteDirectoryTagSlong8Array(TIFF* tif, uint32* ndir, TIFFDirEntry* dir, uint16 tag, uint32 count, int64_new* value);
static int TIFFWriteDirectoryTagRational(TIFF* tif, uint32* ndir, TIFFDirEntry* dir, uint16 tag, double value);
static int TIFFWriteDirectoryTagRationalArray(TIFF* tif, uint32* ndir, TIFFDirEntry* dir, uint16 tag, uint32 count, float* value);
static int TIFFWriteDirectoryTagSrationalArray(TIFF* tif, uint32* ndir, TIFFDirEntry* dir, uint16 tag, uint32 count, float* value);
static int TIFFWriteDirectoryTagFloat(TIFF* tif, uint32* ndir, TIFFDirEntry* dir, uint16 tag, float value);
static int TIFFWriteDirectoryTagFloatArray(TIFF* tif, uint32* ndir, TIFFDirEntry* dir, uint16 tag, uint32 count, float* value);
static int TIFFWriteDirectoryTagFloatPerSample(TIFF* tif, uint32* ndir, TIFFDirEntry* dir, uint16 tag, float value);
static int TIFFWriteDirectoryTagDouble(TIFF* tif, uint32* ndir, TIFFDirEntry* dir, uint16 tag, double value);
static int TIFFWriteDirectoryTagDoubleArray(TIFF* tif, uint32* ndir, TIFFDirEntry* dir, uint16 tag, uint32 count, double* value);
static int TIFFWriteDirectoryTagDoublePerSample(TIFF* tif, uint32* ndir, TIFFDirEntry* dir, uint16 tag, double value);
static int TIFFWriteDirectoryTagIfdArray(TIFF* tif, uint32* ndir, TIFFDirEntry* dir, uint16 tag, uint32 count, uint32* value);
static int TIFFWriteDirectoryTagIfd8Array(TIFF* tif, uint32* ndir, TIFFDirEntry* dir, uint16 tag, uint32 count, uint64_new* value);
static int TIFFWriteDirectoryTagShortLong(TIFF* tif, uint32* ndir, TIFFDirEntry* dir, uint16 tag, uint32 value);
static int TIFFWriteDirectoryTagShortLongLong8Array(TIFF* tif, uint32* ndir, TIFFDirEntry* dir, uint16 tag, uint32 count, uint64_new* value);
static int TIFFWriteDirectoryTagColormap(TIFF* tif, uint32* ndir, TIFFDirEntry* dir);
static int TIFFWriteDirectoryTagTransferfunction(TIFF* tif, uint32* ndir, TIFFDirEntry* dir);
static int TIFFWriteDirectoryTagSubifd(TIFF* tif, uint32* ndir, TIFFDirEntry* dir);

static int TIFFWriteDirectoryTagCheckedAscii(TIFF* tif, uint32* ndir, TIFFDirEntry* dir, uint16 tag, uint32 count, char* value);
static int TIFFWriteDirectoryTagCheckedUndefinedArray(TIFF* tif, uint32* ndir, TIFFDirEntry* dir, uint16 tag, uint32 count, uint8* value);
static int TIFFWriteDirectoryTagCheckedByte(TIFF* tif, uint32* ndir, TIFFDirEntry* dir, uint16 tag, uint8 value);
static int TIFFWriteDirectoryTagCheckedByteArray(TIFF* tif, uint32* ndir, TIFFDirEntry* dir, uint16 tag, uint32 count, uint8* value);
static int TIFFWriteDirectoryTagCheckedSbyte(TIFF* tif, uint32* ndir, TIFFDirEntry* dir, uint16 tag, int8 value);
static int TIFFWriteDirectoryTagCheckedSbyteArray(TIFF* tif, uint32* ndir, TIFFDirEntry* dir, uint16 tag, uint32 count, int8* value);
static int TIFFWriteDirectoryTagCheckedShort(TIFF* tif, uint32* ndir, TIFFDirEntry* dir, uint16 tag, uint16 value);
static int TIFFWriteDirectoryTagCheckedShortArray(TIFF* tif, uint32* ndir, TIFFDirEntry* dir, uint16 tag, uint32 count, uint16* value);
static int TIFFWriteDirectoryTagCheckedSshort(TIFF* tif, uint32* ndir, TIFFDirEntry* dir, uint16 tag, int16 value);
static int TIFFWriteDirectoryTagCheckedSshortArray(TIFF* tif, uint32* ndir, TIFFDirEntry* dir, uint16 tag, uint32 count, int16* value);
static int TIFFWriteDirectoryTagCheckedLong(TIFF* tif, uint32* ndir, TIFFDirEntry* dir, uint16 tag, uint32 value);
static int TIFFWriteDirectoryTagCheckedLongArray(TIFF* tif, uint32* ndir, TIFFDirEntry* dir, uint16 tag, uint32 count, uint32* value);
static int TIFFWriteDirectoryTagCheckedSlong(TIFF* tif, uint32* ndir, TIFFDirEntry* dir, uint16 tag, int32 value);
static int TIFFWriteDirectoryTagCheckedSlongArray(TIFF* tif, uint32* ndir, TIFFDirEntry* dir, uint16 tag, uint32 count, int32* value);
static int TIFFWriteDirectoryTagCheckedLong8(TIFF* tif, uint32* ndir, TIFFDirEntry* dir, uint16 tag, uint64_new value);
static int TIFFWriteDirectoryTagCheckedLong8Array(TIFF* tif, uint32* ndir, TIFFDirEntry* dir, uint16 tag, uint32 count, uint64_new* value);
static int TIFFWriteDirectoryTagCheckedSlong8(TIFF* tif, uint32* ndir, TIFFDirEntry* dir, uint16 tag, int64_new value);
static int TIFFWriteDirectoryTagCheckedSlong8Array(TIFF* tif, uint32* ndir, TIFFDirEntry* dir, uint16 tag, uint32 count, int64_new* value);
static int TIFFWriteDirectoryTagCheckedRational(TIFF* tif, uint32* ndir, TIFFDirEntry* dir, uint16 tag, double value);
static int TIFFWriteDirectoryTagCheckedRationalArray(TIFF* tif, uint32* ndir, TIFFDirEntry* dir, uint16 tag, uint32 count, float* value);
static int TIFFWriteDirectoryTagCheckedSrationalArray(TIFF* tif, uint32* ndir, TIFFDirEntry* dir, uint16 tag, uint32 count, float* value);
static int TIFFWriteDirectoryTagCheckedFloat(TIFF* tif, uint32* ndir, TIFFDirEntry* dir, uint16 tag, float value);
static int TIFFWriteDirectoryTagCheckedFloatArray(TIFF* tif, uint32* ndir, TIFFDirEntry* dir, uint16 tag, uint32 count, float* value);
static int TIFFWriteDirectoryTagCheckedDouble(TIFF* tif, uint32* ndir, TIFFDirEntry* dir, uint16 tag, double value);
static int TIFFWriteDirectoryTagCheckedDoubleArray(TIFF* tif, uint32* ndir, TIFFDirEntry* dir, uint16 tag, uint32 count, double* value);
static int TIFFWriteDirectoryTagCheckedIfdArray(TIFF* tif, uint32* ndir, TIFFDirEntry* dir, uint16 tag, uint32 count, uint32* value);
static int TIFFWriteDirectoryTagCheckedIfd8Array(TIFF* tif, uint32* ndir, TIFFDirEntry* dir, uint16 tag, uint32 count, uint64_new* value);

static int TIFFWriteDirectoryTagData(TIFF* tif, uint32* ndir, TIFFDirEntry* dir, uint16 tag, uint16 datatype, uint32 count, uint32 datalength, void* data);

static int TIFFLinkDirectory(TIFF*);

/*
 * Write the contents of the current directory                                                                              
 * to the specified file.  This routine doesn't
 * handle overwriting a directory with auxiliary
 * storage that's been changed.
 */
static int
_TIFFWriteDirectory(TIFF* tif, int done)
{
	static const char module[] = "_TIFFWriteDirectory";
	uint32 ndir;
	TIFFDirEntry* dir;
	uint32 dirsize;
	void* dirmem;
	uint32 m;
	if (tif->tif_mode == O_RDONLY)
		return (1);
	/*
	 * Clear write state so that subsequent images with
	 * different characteristics get the right buffers
	 * setup for them.
	 */
	if (done)
	{
		if (tif->tif_flags & TIFF_POSTENCODE)
		{
			tif->tif_flags &= ~TIFF_POSTENCODE;
			if (!(*tif->tif_postencode)(tif))
			{
				TIFFErrorExt(tif->tif_clientdata,module,
				    "Error post-encoding before directory write");
				return (0);
			}
		}
		(*tif->tif_close)(tif);       /* shutdown encoder */
		/*
		 * Flush any data that might have been written
		 * by the compression close+cleanup routines.
		 */
		if (tif->tif_rawcc > 0
		    && (tif->tif_flags & TIFF_BEENWRITING) != 0
		    && !TIFFFlushData1(tif))
		{
			TIFFErrorExt(tif->tif_clientdata, module,
			    "Error flushing data before directory write");
			return (0);
		}
		if ((tif->tif_flags & TIFF_MYBUFFER) && tif->tif_rawdata)
		{
			_TIFFfree(tif->tif_rawdata);
			tif->tif_rawdata = NULL;
			tif->tif_rawcc = 0;
			tif->tif_rawdatasize = 0;
		}
		tif->tif_flags &= ~(TIFF_BEENWRITING|TIFF_BUFFERSETUP);
	}
	dir=NULL;
	dirmem=NULL;
	while (1)
	{
		ndir=0;
		if (TIFFFieldSet(tif,FIELD_IMAGEDIMENSIONS))
		{
			if (!TIFFWriteDirectoryTagShortLong(tif,&ndir,dir,TIFFTAG_IMAGEWIDTH,tif->tif_dir.td_imagewidth))
				goto bad;
			if (!TIFFWriteDirectoryTagShortLong(tif,&ndir,dir,TIFFTAG_IMAGELENGTH,tif->tif_dir.td_imagelength))
				goto bad;
		}
		if (TIFFFieldSet(tif,FIELD_TILEDIMENSIONS))
		{
			if (!TIFFWriteDirectoryTagShortLong(tif,&ndir,dir,TIFFTAG_TILEWIDTH,tif->tif_dir.td_tilewidth))
				goto bad;
			if (!TIFFWriteDirectoryTagShortLong(tif,&ndir,dir,TIFFTAG_TILELENGTH,tif->tif_dir.td_tilelength))
				goto bad;
		}
		if (TIFFFieldSet(tif,FIELD_RESOLUTION))
		{
			if (!TIFFWriteDirectoryTagRational(tif,&ndir,dir,TIFFTAG_XRESOLUTION,tif->tif_dir.td_xresolution))
				goto bad;
			if (!TIFFWriteDirectoryTagRational(tif,&ndir,dir,TIFFTAG_YRESOLUTION,tif->tif_dir.td_yresolution))
				goto bad;
		}
		if (TIFFFieldSet(tif,FIELD_POSITION))
		{
			if (!TIFFWriteDirectoryTagRational(tif,&ndir,dir,TIFFTAG_XPOSITION,tif->tif_dir.td_xposition))
				goto bad;
			if (!TIFFWriteDirectoryTagRational(tif,&ndir,dir,TIFFTAG_YPOSITION,tif->tif_dir.td_yposition))
				goto bad;
		}
		if (TIFFFieldSet(tif,FIELD_SUBFILETYPE))
		{
			if (!TIFFWriteDirectoryTagLong(tif,&ndir,dir,TIFFTAG_SUBFILETYPE,tif->tif_dir.td_subfiletype))
				goto bad;
		}
		if (TIFFFieldSet(tif,FIELD_BITSPERSAMPLE))
		{
			if (!TIFFWriteDirectoryTagShortPerSample(tif,&ndir,dir,TIFFTAG_BITSPERSAMPLE,tif->tif_dir.td_bitspersample))
				goto bad;
		}
		if (TIFFFieldSet(tif,FIELD_COMPRESSION))
		{
			if (!TIFFWriteDirectoryTagShort(tif,&ndir,dir,TIFFTAG_COMPRESSION,tif->tif_dir.td_compression))
				goto bad;
		}
		if (TIFFFieldSet(tif,FIELD_PHOTOMETRIC))
		{
			if (!TIFFWriteDirectoryTagShort(tif,&ndir,dir,TIFFTAG_PHOTOMETRIC,tif->tif_dir.td_photometric))
				goto bad;
		}
		if (TIFFFieldSet(tif,FIELD_THRESHHOLDING))
		{
			if (!TIFFWriteDirectoryTagShort(tif,&ndir,dir,TIFFTAG_THRESHHOLDING,tif->tif_dir.td_threshholding))
				goto bad;
		}
		if (TIFFFieldSet(tif,FIELD_FILLORDER))
		{
			if (!TIFFWriteDirectoryTagShort(tif,&ndir,dir,TIFFTAG_FILLORDER,tif->tif_dir.td_fillorder))
				goto bad;
		}
		if (TIFFFieldSet(tif,FIELD_ORIENTATION))
		{
			if (!TIFFWriteDirectoryTagShort(tif,&ndir,dir,TIFFTAG_ORIENTATION,tif->tif_dir.td_orientation))
				goto bad;
		}
		if (TIFFFieldSet(tif,FIELD_SAMPLESPERPIXEL))
		{
			if (!TIFFWriteDirectoryTagShort(tif,&ndir,dir,TIFFTAG_SAMPLESPERPIXEL,tif->tif_dir.td_samplesperpixel))
				goto bad;
		}
		if (TIFFFieldSet(tif,FIELD_ROWSPERSTRIP))
		{
			if (!TIFFWriteDirectoryTagShortLong(tif,&ndir,dir,TIFFTAG_ROWSPERSTRIP,tif->tif_dir.td_rowsperstrip))
				goto bad;
		}
		if (TIFFFieldSet(tif,FIELD_MINSAMPLEVALUE))
		{
			if (!TIFFWriteDirectoryTagShortPerSample(tif,&ndir,dir,TIFFTAG_MINSAMPLEVALUE,tif->tif_dir.td_minsamplevalue))
				goto bad;
		}
		if (TIFFFieldSet(tif,FIELD_MAXSAMPLEVALUE))
		{
			if (!TIFFWriteDirectoryTagShortPerSample(tif,&ndir,dir,TIFFTAG_MAXSAMPLEVALUE,tif->tif_dir.td_maxsamplevalue))
				goto bad;
		}
		if (TIFFFieldSet(tif,FIELD_PLANARCONFIG))
		{
			if (!TIFFWriteDirectoryTagShort(tif,&ndir,dir,TIFFTAG_PLANARCONFIG,tif->tif_dir.td_planarconfig))
				goto bad;
		}
		if (TIFFFieldSet(tif,FIELD_RESOLUTIONUNIT))
		{
			if (!TIFFWriteDirectoryTagShort(tif,&ndir,dir,TIFFTAG_RESOLUTIONUNIT,tif->tif_dir.td_resolutionunit))
				goto bad;
		}
		if (TIFFFieldSet(tif,FIELD_PAGENUMBER))
		{
			if (!TIFFWriteDirectoryTagShortArray(tif,&ndir,dir,TIFFTAG_PAGENUMBER,2,&tif->tif_dir.td_pagenumber[0]))
				goto bad;
		}
		if (TIFFFieldSet(tif,FIELD_STRIPBYTECOUNTS))
		{
			if (!TIFFWriteDirectoryTagShortLongLong8Array(tif,&ndir,dir,isTiled(tif)?TIFFTAG_TILEBYTECOUNTS:TIFFTAG_STRIPBYTECOUNTS,tif->tif_dir.td_nstrips,tif->tif_dir.td_stripbytecount))
				goto bad;
		}
		if (TIFFFieldSet(tif,FIELD_STRIPOFFSETS))
		{
			if (!TIFFWriteDirectoryTagShortLongLong8Array(tif,&ndir,dir,isTiled(tif)?TIFFTAG_TILEOFFSETS:TIFFTAG_STRIPOFFSETS,tif->tif_dir.td_nstrips,tif->tif_dir.td_stripoffset))
				goto bad;
		}
		if (TIFFFieldSet(tif,FIELD_COLORMAP))
		{
			if (!TIFFWriteDirectoryTagColormap(tif,&ndir,dir))
				goto bad;
		}
		if (TIFFFieldSet(tif,FIELD_EXTRASAMPLES))
		{
			if (tif->tif_dir.td_extrasamples)
			{
				uint16 na;
				uint16* nb;
				TIFFGetFieldDefaulted(tif,TIFFTAG_EXTRASAMPLES,&na,&nb);
				if (!TIFFWriteDirectoryTagShortArray(tif,&ndir,dir,TIFFTAG_EXTRASAMPLES,na,nb))
					goto bad;
			}
		}
		if (TIFFFieldSet(tif,FIELD_SAMPLEFORMAT))
		{
			if (!TIFFWriteDirectoryTagShortPerSample(tif,&ndir,dir,TIFFTAG_SAMPLEFORMAT,tif->tif_dir.td_sampleformat))
				goto bad;
		}
		if (TIFFFieldSet(tif,FIELD_SMINSAMPLEVALUE))
		{
			if (!TIFFWriteDirectoryTagSampleformatPerSample(tif,&ndir,dir,TIFFTAG_SMINSAMPLEVALUE,tif->tif_dir.td_sminsamplevalue))
				goto bad;
		}
		if (TIFFFieldSet(tif,FIELD_SMAXSAMPLEVALUE))
		{
			if (!TIFFWriteDirectoryTagSampleformatPerSample(tif,&ndir,dir,TIFFTAG_SMAXSAMPLEVALUE,tif->tif_dir.td_smaxsamplevalue))
				goto bad;
		}
		if (TIFFFieldSet(tif,FIELD_IMAGEDEPTH))
		{
			if (!TIFFWriteDirectoryTagLong(tif,&ndir,dir,TIFFTAG_IMAGEDEPTH,tif->tif_dir.td_imagedepth))
				goto bad;
		}
		if (TIFFFieldSet(tif,FIELD_TILEDEPTH))
		{
			if (!TIFFWriteDirectoryTagLong(tif,&ndir,dir,TIFFTAG_TILEDEPTH,tif->tif_dir.td_tiledepth))
				goto bad;
		}
		if (TIFFFieldSet(tif,FIELD_HALFTONEHINTS))
		{
			if (!TIFFWriteDirectoryTagShortArray(tif,&ndir,dir,TIFFTAG_HALFTONEHINTS,2,&tif->tif_dir.td_halftonehints[0]))
				goto bad;
		}
		if (TIFFFieldSet(tif,FIELD_YCBCRSUBSAMPLING))
		{
			if (!TIFFWriteDirectoryTagShortArray(tif,&ndir,dir,TIFFTAG_YCBCRSUBSAMPLING,2,&tif->tif_dir.td_ycbcrsubsampling[0]))
				goto bad;
		}
		if (TIFFFieldSet(tif,FIELD_YCBCRPOSITIONING))
		{
			if (!TIFFWriteDirectoryTagShort(tif,&ndir,dir,TIFFTAG_YCBCRPOSITIONING,tif->tif_dir.td_ycbcrpositioning))
				goto bad;
		}
		if (TIFFFieldSet(tif,FIELD_TRANSFERFUNCTION))
		{
			if (!TIFFWriteDirectoryTagTransferfunction(tif,&ndir,dir))
				goto bad;
		}
		if (TIFFFieldSet(tif,FIELD_INKNAMES))
		{
			if (!TIFFWriteDirectoryTagAscii(tif,&ndir,dir,TIFFTAG_INKNAMES,tif->tif_dir.td_inknameslen,tif->tif_dir.td_inknames))
				goto bad;
		}
		if (TIFFFieldSet(tif,FIELD_SUBIFD))
		{
			if (!TIFFWriteDirectoryTagSubifd(tif,&ndir,dir))
				goto bad;
		}
		for (m=0; m<(uint32)(tif->tif_dir.td_customValueCount); m++)
		{
			switch (tif->tif_dir.td_customValues[m].info->field_type)
			{
				case TIFF_ASCII:
					if (!TIFFWriteDirectoryTagAscii(tif,&ndir,dir,tif->tif_dir.td_customValues[m].info->field_tag,tif->tif_dir.td_customValues[m].count,tif->tif_dir.td_customValues[m].value))
						goto bad;
					break;
				case TIFF_UNDEFINED:
					if (!TIFFWriteDirectoryTagUndefinedArray(tif,&ndir,dir,tif->tif_dir.td_customValues[m].info->field_tag,tif->tif_dir.td_customValues[m].count,tif->tif_dir.td_customValues[m].value))
						goto bad;
					break;
				case TIFF_BYTE:
					if (!TIFFWriteDirectoryTagByteArray(tif,&ndir,dir,tif->tif_dir.td_customValues[m].info->field_tag,tif->tif_dir.td_customValues[m].count,tif->tif_dir.td_customValues[m].value))
						goto bad;
					break;
				case TIFF_SBYTE:
					if (!TIFFWriteDirectoryTagSbyteArray(tif,&ndir,dir,tif->tif_dir.td_customValues[m].info->field_tag,tif->tif_dir.td_customValues[m].count,tif->tif_dir.td_customValues[m].value))
						goto bad;
					break;
				case TIFF_SHORT:
					if (!TIFFWriteDirectoryTagShortArray(tif,&ndir,dir,tif->tif_dir.td_customValues[m].info->field_tag,tif->tif_dir.td_customValues[m].count,tif->tif_dir.td_customValues[m].value))
						goto bad;
					break;
				case TIFF_SSHORT:
					if (!TIFFWriteDirectoryTagSshortArray(tif,&ndir,dir,tif->tif_dir.td_customValues[m].info->field_tag,tif->tif_dir.td_customValues[m].count,tif->tif_dir.td_customValues[m].value))
						goto bad;
					break;
				case TIFF_LONG:
					if (!TIFFWriteDirectoryTagLongArray(tif,&ndir,dir,tif->tif_dir.td_customValues[m].info->field_tag,tif->tif_dir.td_customValues[m].count,tif->tif_dir.td_customValues[m].value))
						goto bad;
					break;
				case TIFF_SLONG:
					if (!TIFFWriteDirectoryTagSlongArray(tif,&ndir,dir,tif->tif_dir.td_customValues[m].info->field_tag,tif->tif_dir.td_customValues[m].count,tif->tif_dir.td_customValues[m].value))
						goto bad;
					break;
				case TIFF_LONG8:
					if (!TIFFWriteDirectoryTagLong8Array(tif,&ndir,dir,tif->tif_dir.td_customValues[m].info->field_tag,tif->tif_dir.td_customValues[m].count,tif->tif_dir.td_customValues[m].value))
						goto bad;
					break;
				case TIFF_SLONG8:
					if (!TIFFWriteDirectoryTagSlong8Array(tif,&ndir,dir,tif->tif_dir.td_customValues[m].info->field_tag,tif->tif_dir.td_customValues[m].count,tif->tif_dir.td_customValues[m].value))
						goto bad;
					break;
				case TIFF_RATIONAL:
					if (!TIFFWriteDirectoryTagRationalArray(tif,&ndir,dir,tif->tif_dir.td_customValues[m].info->field_tag,tif->tif_dir.td_customValues[m].count,tif->tif_dir.td_customValues[m].value))
						goto bad;
					break;
				case TIFF_SRATIONAL:
					if (!TIFFWriteDirectoryTagSrationalArray(tif,&ndir,dir,tif->tif_dir.td_customValues[m].info->field_tag,tif->tif_dir.td_customValues[m].count,tif->tif_dir.td_customValues[m].value))
						goto bad;
					break;
				case TIFF_FLOAT:
					if (!TIFFWriteDirectoryTagFloatArray(tif,&ndir,dir,tif->tif_dir.td_customValues[m].info->field_tag,tif->tif_dir.td_customValues[m].count,tif->tif_dir.td_customValues[m].value))
						goto bad;
					break;
				case TIFF_DOUBLE:
					if (!TIFFWriteDirectoryTagDoubleArray(tif,&ndir,dir,tif->tif_dir.td_customValues[m].info->field_tag,tif->tif_dir.td_customValues[m].count,tif->tif_dir.td_customValues[m].value))
						goto bad;
					break;
				case TIFF_IFD:
					if (!TIFFWriteDirectoryTagIfdArray(tif,&ndir,dir,tif->tif_dir.td_customValues[m].info->field_tag,tif->tif_dir.td_customValues[m].count,tif->tif_dir.td_customValues[m].value))
						goto bad;
					break;
				case TIFF_IFD8:
					if (!TIFFWriteDirectoryTagIfd8Array(tif,&ndir,dir,tif->tif_dir.td_customValues[m].info->field_tag,tif->tif_dir.td_customValues[m].count,tif->tif_dir.td_customValues[m].value))
						goto bad;
					break;
			}
		}
		if (dir!=NULL)
			break;
		dir=_TIFFmalloc(ndir*sizeof(TIFFDirEntry));
		if (dir==NULL)
		{
			TIFFErrorExt(tif->tif_clientdata,module,"Out of memory");
			goto bad;
		}
		if ((tif->tif_diroff==0)&&(!TIFFLinkDirectory(tif)))
			goto bad;
		if (!(tif->tif_flags&TIFF_BIGTIFF))
			dirsize=2+ndir*12+4;
		else
			dirsize=8+ndir*20+8;
		tif->tif_dataoff=tif->tif_diroff+dirsize;
		if (!(tif->tif_flags&TIFF_BIGTIFF))
			tif->tif_dataoff=(uint32)tif->tif_dataoff;
		if ((tif->tif_dataoff<tif->tif_diroff)||(tif->tif_dataoff<(uint64_new)dirsize))
		{
			TIFFErrorExt(tif->tif_clientdata,module,"Maximum TIFF file size exceeded");
			goto bad;
		}
		if (tif->tif_dataoff&1)
			tif->tif_dataoff++;
	}
	if (TIFFFieldSet(tif,FIELD_SUBIFD)&&(tif->tif_subifdoff==0))
	{
		uint32 na;
		TIFFDirEntry* nb;
		for (na=0, nb=dir; ; na++, nb++)
		{
			assert(na<ndir);
			if (nb->tdir_tag==TIFFTAG_SUBIFD)
				break;
		}
		if (!(tif->tif_flags&TIFF_BIGTIFF))
			tif->tif_subifdoff=tif->tif_diroff+2+na*12+8;
		else
			tif->tif_subifdoff=tif->tif_diroff+8+na*20+12;
	}
	dirmem=_TIFFmalloc(dirsize);
	if (dirmem==NULL)
	{
		TIFFErrorExt(tif->tif_clientdata,module,"Out of memory");
		goto bad;
	}
	if (!(tif->tif_flags&TIFF_BIGTIFF))
	{
		uint8* n;
		TIFFDirEntry* o;
		n=dirmem;
		*(uint16*)n=ndir;
		if (tif->tif_flags&TIFF_SWAB)
			TIFFSwabShort((uint16*)n);
		n+=2;
		o=dir;
		for (m=0; m<ndir; m++)
		{
			*(uint16*)n=o->tdir_tag;
			if (tif->tif_flags&TIFF_SWAB)
				TIFFSwabShort((uint16*)n);
			n+=2;
			*(uint16*)n=o->tdir_type;
			if (tif->tif_flags&TIFF_SWAB)
				TIFFSwabShort((uint16*)n);
			n+=2;
			*(uint32*)n=(uint32)o->tdir_count;
			if (tif->tif_flags&TIFF_SWAB)
				TIFFSwabLong((uint32*)n);
			n+=4;
			_TIFFmemcpy(n,&o->tdir_offset,4);
			n+=4;
			o++;
		}
		*(uint32*)n=0;
	}
	else
	{
		uint8* n;
		TIFFDirEntry* o;
		n=dirmem;
		*(uint64_new*)n=ndir;
		if (tif->tif_flags&TIFF_SWAB)
			TIFFSwabLong8((uint64_new*)n);
		n+=8;
		o=dir;
		for (m=0; m<ndir; m++)
		{
			*(uint16*)n=o->tdir_tag;
			if (tif->tif_flags&TIFF_SWAB)
				TIFFSwabShort((uint16*)n);
			n+=2;
			*(uint16*)n=o->tdir_type;
			if (tif->tif_flags&TIFF_SWAB)
				TIFFSwabShort((uint16*)n);
			n+=2;
			*(uint64_new*)n=o->tdir_count;
			if (tif->tif_flags&TIFF_SWAB)
				TIFFSwabLong8((uint64_new*)n);
			n+=8;
			_TIFFmemcpy(n,&o->tdir_offset,8);
			n+=8;
			o++;
		}
		*(uint64_new*)n=0;
	}
	_TIFFfree(dir);
	dir=NULL;
	if (!SeekOK(tif,tif->tif_diroff))
	{
		TIFFErrorExt(tif->tif_clientdata,module,"IO error writing directory");
		goto bad;
	}
	if (!WriteOK(tif,dirmem,dirsize))
	{
		TIFFErrorExt(tif->tif_clientdata,module,"IO error writing directory");
		goto bad;
	}
	_TIFFfree(dirmem);
	return(1);
bad:
	if (dir!=NULL)
		_TIFFfree(dir);
	if (dirmem!=NULL)
		_TIFFfree(dirmem);
	return(0);
}

static int
TIFFWriteDirectoryTagSampleformatPerSample(TIFF* tif, uint32* ndir, TIFFDirEntry* dir, uint16 tag, double value)
{
	switch (tif->tif_dir.td_sampleformat)
	{
		case SAMPLEFORMAT_IEEEFP:
			if (tif->tif_dir.td_bitspersample<=32)
				return(TIFFWriteDirectoryTagFloatPerSample(tif,ndir,dir,tag,(float)value));
			else
				return(TIFFWriteDirectoryTagDoublePerSample(tif,ndir,dir,tag,value));
		case SAMPLEFORMAT_INT:
			if (tif->tif_dir.td_bitspersample<=8)
				return(TIFFWriteDirectoryTagSbytePerSample(tif,ndir,dir,tag,(int8)value));
			else if (tif->tif_dir.td_bitspersample<=16)
				return(TIFFWriteDirectoryTagSshortPerSample(tif,ndir,dir,tag,(int16)value));
			else
				return(TIFFWriteDirectoryTagSlongPerSample(tif,ndir,dir,tag,(int32)value));
		case SAMPLEFORMAT_UINT:
			if (tif->tif_dir.td_bitspersample<=8)
				return(TIFFWriteDirectoryTagBytePerSample(tif,ndir,dir,tag,(uint8)value));
			else if (tif->tif_dir.td_bitspersample<=16)
				return(TIFFWriteDirectoryTagShortPerSample(tif,ndir,dir,tag,(uint16)value));
			else
				return(TIFFWriteDirectoryTagLongPerSample(tif,ndir,dir,tag,(uint32)value));
		default:
			return(1);
	}
}

static int
TIFFWriteDirectoryTagAscii(TIFF* tif, uint32* ndir, TIFFDirEntry* dir, uint16 tag, uint32 count, char* value)
{
	if (dir==NULL)
	{
		(*ndir)++;
		return(1);
	}
	return(TIFFWriteDirectoryTagCheckedAscii(tif,ndir,dir,tag,count,value));
}

static int
TIFFWriteDirectoryTagUndefinedArray(TIFF* tif, uint32* ndir, TIFFDirEntry* dir, uint16 tag, uint32 count, uint8* value)
{
	if (dir==NULL)
	{
		(*ndir)++;
		return(1);
	}
	return(TIFFWriteDirectoryTagCheckedUndefinedArray(tif,ndir,dir,tag,count,value));
}

static int
TIFFWriteDirectoryTagByte(TIFF* tif, uint32* ndir, TIFFDirEntry* dir, uint16 tag, uint8 value)
{
	if (dir==NULL)
	{
		(*ndir)++;
		return(1);
	}
	return(TIFFWriteDirectoryTagCheckedByte(tif,ndir,dir,tag,value));
}

static int
TIFFWriteDirectoryTagByteArray(TIFF* tif, uint32* ndir, TIFFDirEntry* dir, uint16 tag, uint32 count, uint8* value)
{
	if (dir==NULL)
	{
		(*ndir)++;
		return(1);
	}
	return(TIFFWriteDirectoryTagCheckedByteArray(tif,ndir,dir,tag,count,value));
}

static int
TIFFWriteDirectoryTagBytePerSample(TIFF* tif, uint32* ndir, TIFFDirEntry* dir, uint16 tag, uint8 value)
{
	static const char module[] = "TIFFWriteDirectoryTagBytePerSample";
	uint8* m;
	uint8* na;
	uint16 nb;
	int o;
	if (dir==NULL)
	{
		(*ndir)++;
		return(1);
	}
	m=_TIFFmalloc(tif->tif_dir.td_samplesperpixel*sizeof(uint8));
	if (m==NULL)
	{
		TIFFErrorExt(tif->tif_clientdata,module,"Out of memory");
		return(0);
	}
	for (na=m, nb=0; nb<tif->tif_dir.td_samplesperpixel; na++, nb++)
		*na=value;
	o=TIFFWriteDirectoryTagCheckedByteArray(tif,ndir,dir,tag,tif->tif_dir.td_samplesperpixel,m);
	_TIFFfree(m);
	return(o);
}

static int
TIFFWriteDirectoryTagSbyte(TIFF* tif, uint32* ndir, TIFFDirEntry* dir, uint16 tag, int8 value)
{
	if (dir==NULL)
	{
		(*ndir)++;
		return(1);
	}
	return(TIFFWriteDirectoryTagCheckedSbyte(tif,ndir,dir,tag,value));
}

static int
TIFFWriteDirectoryTagSbyteArray(TIFF* tif, uint32* ndir, TIFFDirEntry* dir, uint16 tag, uint32 count, int8* value)
{
	if (dir==NULL)
	{
		(*ndir)++;
		return(1);
	}
	return(TIFFWriteDirectoryTagCheckedSbyteArray(tif,ndir,dir,tag,count,value));
}

static int
TIFFWriteDirectoryTagSbytePerSample(TIFF* tif, uint32* ndir, TIFFDirEntry* dir, uint16 tag, int8 value)
{
	static const char module[] = "TIFFWriteDirectoryTagSbytePerSample";
	int8* m;
	int8* na;
	uint16 nb;
	int o;
	if (dir==NULL)
	{
		(*ndir)++;
		return(1);
	}
	m=_TIFFmalloc(tif->tif_dir.td_samplesperpixel*sizeof(int8));
	if (m==NULL)
	{
		TIFFErrorExt(tif->tif_clientdata,module,"Out of memory");
		return(0);
	}
	for (na=m, nb=0; nb<tif->tif_dir.td_samplesperpixel; na++, nb++)
		*na=value;
	o=TIFFWriteDirectoryTagCheckedSbyteArray(tif,ndir,dir,tag,tif->tif_dir.td_samplesperpixel,m);
	_TIFFfree(m);
	return(o);
}

static int
TIFFWriteDirectoryTagShort(TIFF* tif, uint32* ndir, TIFFDirEntry* dir, uint16 tag, uint16 value)
{
	if (dir==NULL)
	{
		(*ndir)++;
		return(1);
	}
	return(TIFFWriteDirectoryTagCheckedShort(tif,ndir,dir,tag,value));
}

static int
TIFFWriteDirectoryTagShortArray(TIFF* tif, uint32* ndir, TIFFDirEntry* dir, uint16 tag, uint32 count, uint16* value)
{
	if (dir==NULL)
	{
		(*ndir)++;
		return(1);
	}
	return(TIFFWriteDirectoryTagCheckedShortArray(tif,ndir,dir,tag,count,value));
}

static int
TIFFWriteDirectoryTagShortPerSample(TIFF* tif, uint32* ndir, TIFFDirEntry* dir, uint16 tag, uint16 value)
{
	static const char module[] = "TIFFWriteDirectoryTagShortPerSample";
	uint16* m;
	uint16* na;
	uint16 nb;
	int o;
	if (dir==NULL)
	{
		(*ndir)++;
		return(1);
	}
	m=_TIFFmalloc(tif->tif_dir.td_samplesperpixel*sizeof(uint16));
	if (m==NULL)
	{
		TIFFErrorExt(tif->tif_clientdata,module,"Out of memory");
		return(0);
	}
	for (na=m, nb=0; nb<tif->tif_dir.td_samplesperpixel; na++, nb++)
		*na=value;
	o=TIFFWriteDirectoryTagCheckedShortArray(tif,ndir,dir,tag,tif->tif_dir.td_samplesperpixel,m);
	_TIFFfree(m);
	return(o);
}

static int
TIFFWriteDirectoryTagSshort(TIFF* tif, uint32* ndir, TIFFDirEntry* dir, uint16 tag, int16 value)
{
	if (dir==NULL)
	{
		(*ndir)++;
		return(1);
	}
	return(TIFFWriteDirectoryTagCheckedSshort(tif,ndir,dir,tag,value));
}

static int
TIFFWriteDirectoryTagSshortArray(TIFF* tif, uint32* ndir, TIFFDirEntry* dir, uint16 tag, uint32 count, int16* value)
{
	if (dir==NULL)
	{
		(*ndir)++;
		return(1);
	}
	return(TIFFWriteDirectoryTagCheckedSshortArray(tif,ndir,dir,tag,count,value));
}

static int
TIFFWriteDirectoryTagSshortPerSample(TIFF* tif, uint32* ndir, TIFFDirEntry* dir, uint16 tag, int16 value)
{
	static const char module[] = "TIFFWriteDirectoryTagSshortPerSample";
	int16* m;
	int16* na;
	uint16 nb;
	int o;
	if (dir==NULL)
	{
		(*ndir)++;
		return(1);
	}
	m=_TIFFmalloc(tif->tif_dir.td_samplesperpixel*sizeof(int16));
	if (m==NULL)
	{
		TIFFErrorExt(tif->tif_clientdata,module,"Out of memory");
		return(0);
	}
	for (na=m, nb=0; nb<tif->tif_dir.td_samplesperpixel; na++, nb++)
		*na=value;
	o=TIFFWriteDirectoryTagCheckedSshortArray(tif,ndir,dir,tag,tif->tif_dir.td_samplesperpixel,m);
	_TIFFfree(m);
	return(o);
}

static int
TIFFWriteDirectoryTagLong(TIFF* tif, uint32* ndir, TIFFDirEntry* dir, uint16 tag, uint32 value)
{
	if (dir==NULL)
	{
		(*ndir)++;
		return(1);
	}
	return(TIFFWriteDirectoryTagCheckedLong(tif,ndir,dir,tag,value));
}

static int
TIFFWriteDirectoryTagLongArray(TIFF* tif, uint32* ndir, TIFFDirEntry* dir, uint16 tag, uint32 count, uint32* value)
{
	if (dir==NULL)
	{
		(*ndir)++;
		return(1);
	}
	return(TIFFWriteDirectoryTagCheckedLongArray(tif,ndir,dir,tag,count,value));
}

static int
TIFFWriteDirectoryTagLongPerSample(TIFF* tif, uint32* ndir, TIFFDirEntry* dir, uint16 tag, uint32 value)
{
	static const char module[] = "TIFFWriteDirectoryTagLongPerSample";
	uint32* m;
	uint32* na;
	uint16 nb;
	int o;
	if (dir==NULL)
	{
		(*ndir)++;
		return(1);
	}
	m=_TIFFmalloc(tif->tif_dir.td_samplesperpixel*sizeof(uint32));
	if (m==NULL)
	{
		TIFFErrorExt(tif->tif_clientdata,module,"Out of memory");
		return(0);
	}
	for (na=m, nb=0; nb<tif->tif_dir.td_samplesperpixel; na++, nb++)
		*na=value;
	o=TIFFWriteDirectoryTagCheckedLongArray(tif,ndir,dir,tag,tif->tif_dir.td_samplesperpixel,m);
	_TIFFfree(m);
	return(o);
}

static int
TIFFWriteDirectoryTagSlong(TIFF* tif, uint32* ndir, TIFFDirEntry* dir, uint16 tag, int32 value)
{
	if (dir==NULL)
	{
		(*ndir)++;
		return(1);
	}
	return(TIFFWriteDirectoryTagCheckedSlong(tif,ndir,dir,tag,value));
}

static int
TIFFWriteDirectoryTagSlongArray(TIFF* tif, uint32* ndir, TIFFDirEntry* dir, uint16 tag, uint32 count, int32* value)
{
	if (dir==NULL)
	{
		(*ndir)++;
		return(1);
	}
	return(TIFFWriteDirectoryTagCheckedSlongArray(tif,ndir,dir,tag,count,value));
}

static int
TIFFWriteDirectoryTagSlongPerSample(TIFF* tif, uint32* ndir, TIFFDirEntry* dir, uint16 tag, int32 value)
{
	static const char module[] = "TIFFWriteDirectoryTagSlongPerSample";
	int32* m;
	int32* na;
	uint16 nb;
	int o;
	if (dir==NULL)
	{
		(*ndir)++;
		return(1);
	}
	m=_TIFFmalloc(tif->tif_dir.td_samplesperpixel*sizeof(int32));
	if (m==NULL)
	{
		TIFFErrorExt(tif->tif_clientdata,module,"Out of memory");
		return(0);
	}
	for (na=m, nb=0; nb<tif->tif_dir.td_samplesperpixel; na++, nb++)
		*na=value;
	o=TIFFWriteDirectoryTagCheckedSlongArray(tif,ndir,dir,tag,tif->tif_dir.td_samplesperpixel,m);
	_TIFFfree(m);
	return(o);
}

static int
TIFFWriteDirectoryTagLong8(TIFF* tif, uint32* ndir, TIFFDirEntry* dir, uint16 tag, uint64_new value)
{
	if (dir==NULL)
	{
		(*ndir)++;
		return(1);
	}
	return(TIFFWriteDirectoryTagCheckedLong8(tif,ndir,dir,tag,value));
}

static int
TIFFWriteDirectoryTagLong8Array(TIFF* tif, uint32* ndir, TIFFDirEntry* dir, uint16 tag, uint32 count, uint64_new* value)
{
	if (dir==NULL)
	{
		(*ndir)++;
		return(1);
	}
	return(TIFFWriteDirectoryTagCheckedLong8Array(tif,ndir,dir,tag,count,value));
}

static int
TIFFWriteDirectoryTagSlong8(TIFF* tif, uint32* ndir, TIFFDirEntry* dir, uint16 tag, int64_new value)
{
	if (dir==NULL)
	{
		(*ndir)++;
		return(1);
	}
	return(TIFFWriteDirectoryTagCheckedSlong8(tif,ndir,dir,tag,value));
}

static int
TIFFWriteDirectoryTagSlong8Array(TIFF* tif, uint32* ndir, TIFFDirEntry* dir, uint16 tag, uint32 count, int64_new* value)
{
	if (dir==NULL)
	{
		(*ndir)++;
		return(1);
	}
	return(TIFFWriteDirectoryTagCheckedSlong8Array(tif,ndir,dir,tag,count,value));
}

static int
TIFFWriteDirectoryTagRational(TIFF* tif, uint32* ndir, TIFFDirEntry* dir, uint16 tag, double value)
{
	if (dir==NULL)
	{
		(*ndir)++;
		return(1);
	}
	return(TIFFWriteDirectoryTagCheckedRational(tif,ndir,dir,tag,value));
}

static int
TIFFWriteDirectoryTagRationalArray(TIFF* tif, uint32* ndir, TIFFDirEntry* dir, uint16 tag, uint32 count, float* value)
{
	if (dir==NULL)
	{
		(*ndir)++;
		return(1);
	}
	return(TIFFWriteDirectoryTagCheckedRationalArray(tif,ndir,dir,tag,count,value));
}

static int
TIFFWriteDirectoryTagSrationalArray(TIFF* tif, uint32* ndir, TIFFDirEntry* dir, uint16 tag, uint32 count, float* value)
{
	if (dir==NULL)
	{
		(*ndir)++;
		return(1);
	}
	return(TIFFWriteDirectoryTagCheckedSrationalArray(tif,ndir,dir,tag,count,value));
}

static int TIFFWriteDirectoryTagFloat(TIFF* tif, uint32* ndir, TIFFDirEntry* dir, uint16 tag, float value)
{
	if (dir==NULL)
	{
		(*ndir)++;
		return(1);
	}
	return(TIFFWriteDirectoryTagCheckedFloat(tif,ndir,dir,tag,value));
}

static int TIFFWriteDirectoryTagFloatArray(TIFF* tif, uint32* ndir, TIFFDirEntry* dir, uint16 tag, uint32 count, float* value)
{
	if (dir==NULL)
	{
		(*ndir)++;
		return(1);
	}
	return(TIFFWriteDirectoryTagCheckedFloatArray(tif,ndir,dir,tag,count,value));
}

static int TIFFWriteDirectoryTagFloatPerSample(TIFF* tif, uint32* ndir, TIFFDirEntry* dir, uint16 tag, float value)
{
	static const char module[] = "TIFFWriteDirectoryTagFloatPerSample";
	float* m;
	float* na;
	uint16 nb;
	int o;
	if (dir==NULL)
	{
		(*ndir)++;
		return(1);
	}
	m=_TIFFmalloc(tif->tif_dir.td_samplesperpixel*sizeof(float));
	if (m==NULL)
	{
		TIFFErrorExt(tif->tif_clientdata,module,"Out of memory");
		return(0);
	}
	for (na=m, nb=0; nb<tif->tif_dir.td_samplesperpixel; na++, nb++)
		*na=value;
	o=TIFFWriteDirectoryTagCheckedFloatArray(tif,ndir,dir,tag,tif->tif_dir.td_samplesperpixel,m);
	_TIFFfree(m);
	return(o);
}

static int TIFFWriteDirectoryTagDouble(TIFF* tif, uint32* ndir, TIFFDirEntry* dir, uint16 tag, double value)
{
	if (dir==NULL)
	{
		(*ndir)++;
		return(1);
	}
	return(TIFFWriteDirectoryTagCheckedDouble(tif,ndir,dir,tag,value));
}

static int TIFFWriteDirectoryTagDoubleArray(TIFF* tif, uint32* ndir, TIFFDirEntry* dir, uint16 tag, uint32 count, double* value)
{
	if (dir==NULL)
	{
		(*ndir)++;
		return(1);
	}
	return(TIFFWriteDirectoryTagCheckedDoubleArray(tif,ndir,dir,tag,count,value));
}

static int TIFFWriteDirectoryTagDoublePerSample(TIFF* tif, uint32* ndir, TIFFDirEntry* dir, uint16 tag, double value)
{
	static const char module[] = "TIFFWriteDirectoryTagDoublePerSample";
	double* m;
	double* na;
	uint16 nb;
	int o;
	if (dir==NULL)
	{
		(*ndir)++;
		return(1);
	}
	m=_TIFFmalloc(tif->tif_dir.td_samplesperpixel*sizeof(double));
	if (m==NULL)
	{
		TIFFErrorExt(tif->tif_clientdata,module,"Out of memory");
		return(0);
	}
	for (na=m, nb=0; nb<tif->tif_dir.td_samplesperpixel; na++, nb++)
		*na=value;
	o=TIFFWriteDirectoryTagCheckedDoubleArray(tif,ndir,dir,tag,tif->tif_dir.td_samplesperpixel,m);
	_TIFFfree(m);
	return(o);
}

static int
TIFFWriteDirectoryTagIfdArray(TIFF* tif, uint32* ndir, TIFFDirEntry* dir, uint16 tag, uint32 count, uint32* value)
{
	if (dir==NULL)
	{
		(*ndir)++;
		return(1);
	}
	return(TIFFWriteDirectoryTagCheckedIfdArray(tif,ndir,dir,tag,count,value));
}

static int
TIFFWriteDirectoryTagIfd8Array(TIFF* tif, uint32* ndir, TIFFDirEntry* dir, uint16 tag, uint32 count, uint64_new* value)
{
	if (dir==NULL)
	{
		(*ndir)++;
		return(1);
	}
	return(TIFFWriteDirectoryTagCheckedIfd8Array(tif,ndir,dir,tag,count,value));
}

static int
TIFFWriteDirectoryTagShortLong(TIFF* tif, uint32* ndir, TIFFDirEntry* dir, uint16 tag, uint32 value)
{
	if (dir==NULL)
	{
		(*ndir)++;
		return(1);
	}
	if (value<=0xFFFF)
		return(TIFFWriteDirectoryTagCheckedShort(tif,ndir,dir,tag,(uint16)value));
	else
		return(TIFFWriteDirectoryTagCheckedLong(tif,ndir,dir,tag,value));
}

static int
TIFFWriteDirectoryTagShortLongLong8Array(TIFF* tif, uint32* ndir, TIFFDirEntry* dir, uint16 tag, uint32 count, uint64_new* value)
{
	static const char module[] = "TIFFWriteDirectoryTagShortLongLong8Array";
	uint64_new* ma;
	uint32 mb;
	uint8 n;
	int o;
	if (dir==NULL)
	{
		(*ndir)++;
		return(1);
	}
	n=0;
	for (ma=value, mb=0; mb<count; ma++, mb++)
	{
		if ((n==0)&&(*ma>0xFFFF))
			n=1;
		if ((n==1)&&(*ma>0xFFFFFFFF))
		{
			n=2;
			break;
		}
	}
	if (n==0)
	{
		uint16* p;
		uint16* q;
		p=_TIFFmalloc(count*2);
		if (p==NULL)
		{
			TIFFErrorExt(tif->tif_clientdata,module,"Out of memory");
			return(0);
		}
		for (ma=value, mb=0, q=p; mb<count; ma++, mb++, q++)
			*q=(uint16)(*ma);
		o=TIFFWriteDirectoryTagCheckedShortArray(tif,ndir,dir,tag,count,p);
		_TIFFfree(p);
	}
	else if (n==1)
	{
		uint32* p;
		uint32* q;
		p=_TIFFmalloc(count*4);
		if (p==NULL)
		{
			TIFFErrorExt(tif->tif_clientdata,module,"Out of memory");
			return(0);
		}
		for (ma=value, mb=0, q=p; mb<count; ma++, mb++, q++)
			*q=(uint32)(*ma);
		o=TIFFWriteDirectoryTagCheckedLongArray(tif,ndir,dir,tag,count,p);
		_TIFFfree(p);
	}
	else if (n==2)
		o=TIFFWriteDirectoryTagCheckedLong8Array(tif,ndir,dir,tag,count,value);
	return(o);
}

/* PODD */

static int
TIFFWriteDirectoryTagColormap(TIFF* tif, uint32* ndir, TIFFDirEntry* dir)
{
	uint32 m;
	uint16* n;
	int o;
	if (dir==NULL)
	{
		(*ndir)++;
		return(1);
	}
	m=(1<<tif->tif_dir.td_bitspersample);
	n=_TIFFmalloc(3*m*sizeof(uint16));
	if (n==NULL)
		assert(0);
	_TIFFmemcpy(&n[0],tif->tif_dir.td_colormap[0],m*sizeof(uint16));
	_TIFFmemcpy(&n[m],tif->tif_dir.td_colormap[1],m*sizeof(uint16));
	_TIFFmemcpy(&n[2*m],tif->tif_dir.td_colormap[2],m*sizeof(uint16));
	o=TIFFWriteDirectoryTagCheckedShortArray(tif,ndir,dir,TIFFTAG_COLORMAP,3*m,n);
	_TIFFfree(n);
	return(o);
}

static int
TIFFWriteDirectoryTagTransferfunction(TIFF* tif, uint32* ndir, TIFFDirEntry* dir)
{
	uint32 m;
	uint16 n;
	uint16* o;
	int p;
	if (dir==NULL)
	{
		(*ndir)++;
		return(1);
	}
	m=(1<<tif->tif_dir.td_bitspersample);
	n=tif->tif_dir.td_samplesperpixel-tif->tif_dir.td_extrasamples;
	if (n>3)
		n=3;
	if (n==3)
	{
		if (!_TIFFmemcmp(tif->tif_dir.td_transferfunction[0],tif->tif_dir.td_transferfunction[2],m*sizeof(uint16)))
			n=2;
	}
	if (n==2)
	{
		if (!_TIFFmemcmp(tif->tif_dir.td_transferfunction[0],tif->tif_dir.td_transferfunction[1],m*sizeof(uint16)))
			n=1;
	}
	if (n==0)
		n=1;
	o=_TIFFmalloc(n*m*sizeof(uint16));
	if (o==NULL)
		assert(0);
	_TIFFmemcpy(&o[0],tif->tif_dir.td_transferfunction[0],m*sizeof(uint16));
	if (n>1)
		_TIFFmemcpy(&o[m],tif->tif_dir.td_transferfunction[1],m*sizeof(uint16));
	if (n>2)
		_TIFFmemcpy(&o[2*m],tif->tif_dir.td_transferfunction[2],m*sizeof(uint16));
	p=TIFFWriteDirectoryTagCheckedShortArray(tif,ndir,dir,TIFFTAG_TRANSFERFUNCTION,n*m,o);
	_TIFFfree(o);
	return(p);
}

static int
TIFFWriteDirectoryTagSubifd(TIFF* tif, uint32* ndir, TIFFDirEntry* dir)
{
	uint64_new m;
	int n;
	if (tif->tif_dir.td_nsubifd==0)
		return(1);
	if (dir==NULL)
	{
		(*ndir)++;
		return(1);
	}
	m=tif->tif_dataoff;
	if (!(tif->tif_flags&TIFF_BIGTIFF))
	{
		uint32* o;
		uint64_new* pa;
		uint32* pb;
		uint16 p;
		o=_TIFFmalloc(tif->tif_dir.td_nsubifd*sizeof(uint32));
		pa=tif->tif_dir.td_subifd;
		pb=o;
		for (p=0; p<tif->tif_dir.td_nsubifd; p++)
		{
			assert(*pa<=0xFFFFFFFFUL);
			*pb++=*pa++;
		}
		n=TIFFWriteDirectoryTagCheckedIfdArray(tif,ndir,dir,TIFFTAG_SUBIFD,tif->tif_dir.td_nsubifd,o);
		_TIFFfree(o);
	}
	else
		n=TIFFWriteDirectoryTagCheckedIfd8Array(tif,ndir,dir,TIFFTAG_SUBIFD,tif->tif_dir.td_nsubifd,tif->tif_dir.td_subifd);
	if (!n)
		return(0);
	/*
	 * Total hack: if this directory includes a SubIFD
	 * tag then force the next <n> directories to be
	 * written as ``sub directories'' of this one.  This
	 * is used to write things like thumbnails and
	 * image masks that one wants to keep out of the
	 * normal directory linkage access mechanism.
	 */
	tif->tif_flags|=TIFF_INSUBIFD;
	tif->tif_nsubifd=tif->tif_dir.td_nsubifd;
	if (tif->tif_dir.td_nsubifd==1)
		tif->tif_subifdoff=0;
	else
		tif->tif_subifdoff=m;
	return(1);
}

static int
TIFFWriteDirectoryTagCheckedAscii(TIFF* tif, uint32* ndir, TIFFDirEntry* dir, uint16 tag, uint32 count, char* value)
{
	assert(sizeof(char)==1);
	return(TIFFWriteDirectoryTagData(tif,ndir,dir,tag,TIFF_ASCII,count,count,value));
}

static int
TIFFWriteDirectoryTagCheckedUndefinedArray(TIFF* tif, uint32* ndir, TIFFDirEntry* dir, uint16 tag, uint32 count, uint8* value)
{
	assert(sizeof(uint8)==1);
	return(TIFFWriteDirectoryTagData(tif,ndir,dir,tag,TIFF_UNDEFINED,count,count,value));
}

static int
TIFFWriteDirectoryTagCheckedByte(TIFF* tif, uint32* ndir, TIFFDirEntry* dir, uint16 tag, uint8 value)
{
	assert(sizeof(uint8)==1);
	return(TIFFWriteDirectoryTagData(tif,ndir,dir,tag,TIFF_BYTE,1,1,&value));
}

static int
TIFFWriteDirectoryTagCheckedByteArray(TIFF* tif, uint32* ndir, TIFFDirEntry* dir, uint16 tag, uint32 count, uint8* value)
{
	assert(sizeof(uint8)==1);
	return(TIFFWriteDirectoryTagData(tif,ndir,dir,tag,TIFF_BYTE,count,count,value));
}

static int
TIFFWriteDirectoryTagCheckedSbyte(TIFF* tif, uint32* ndir, TIFFDirEntry* dir, uint16 tag, int8 value)
{
	assert(sizeof(int8)==1);
	return(TIFFWriteDirectoryTagData(tif,ndir,dir,tag,TIFF_SBYTE,1,1,&value));
}

static int
TIFFWriteDirectoryTagCheckedSbyteArray(TIFF* tif, uint32* ndir, TIFFDirEntry* dir, uint16 tag, uint32 count, int8* value)
{
	assert(sizeof(int8)==1);
	return(TIFFWriteDirectoryTagData(tif,ndir,dir,tag,TIFF_SBYTE,count,count,value));
}

static int
TIFFWriteDirectoryTagCheckedShort(TIFF* tif, uint32* ndir, TIFFDirEntry* dir, uint16 tag, uint16 value)
{
	uint16 m;
	assert(sizeof(uint16)==2);
	m=value;
	if (tif->tif_flags&TIFF_SWAB)
		TIFFSwabShort(&m);
	return(TIFFWriteDirectoryTagData(tif,ndir,dir,tag,TIFF_SHORT,1,2,&m));
}

static int
TIFFWriteDirectoryTagCheckedShortArray(TIFF* tif, uint32* ndir, TIFFDirEntry* dir, uint16 tag, uint32 count, uint16* value)
{
	assert(count<0x80000000);
	assert(sizeof(uint16)==2);
	if (tif->tif_flags&TIFF_SWAB)
		TIFFSwabArrayOfShort(value,count);
	return(TIFFWriteDirectoryTagData(tif,ndir,dir,tag,TIFF_SHORT,count,count*2,value));
}

static int
TIFFWriteDirectoryTagCheckedSshort(TIFF* tif, uint32* ndir, TIFFDirEntry* dir, uint16 tag, int16 value)
{
	int16 m;
	assert(sizeof(int16)==2);
	m=value;
	if (tif->tif_flags&TIFF_SWAB)
		TIFFSwabShort((uint16*)(&m));
	return(TIFFWriteDirectoryTagData(tif,ndir,dir,tag,TIFF_SSHORT,1,2,&m));
}

static int
TIFFWriteDirectoryTagCheckedSshortArray(TIFF* tif, uint32* ndir, TIFFDirEntry* dir, uint16 tag, uint32 count, int16* value)
{
	assert(count<0x80000000);
	assert(sizeof(int16)==2);
	if (tif->tif_flags&TIFF_SWAB)
		TIFFSwabArrayOfShort((uint16*)value,count);
	return(TIFFWriteDirectoryTagData(tif,ndir,dir,tag,TIFF_SSHORT,count,count*2,value));
}

static int
TIFFWriteDirectoryTagCheckedLong(TIFF* tif, uint32* ndir, TIFFDirEntry* dir, uint16 tag, uint32 value)
{
	uint32 m;
	assert(sizeof(uint32)==4);
	m=value;
	if (tif->tif_flags&TIFF_SWAB)
		TIFFSwabLong(&m);
	return(TIFFWriteDirectoryTagData(tif,ndir,dir,tag,TIFF_LONG,1,4,&m));
}

static int
TIFFWriteDirectoryTagCheckedLongArray(TIFF* tif, uint32* ndir, TIFFDirEntry* dir, uint16 tag, uint32 count, uint32* value)
{
	assert(count<0x40000000);
	assert(sizeof(uint32)==4);
	if (tif->tif_flags&TIFF_SWAB)
		TIFFSwabArrayOfLong(value,count);
	return(TIFFWriteDirectoryTagData(tif,ndir,dir,tag,TIFF_LONG,count,count*4,value));
}

static int
TIFFWriteDirectoryTagCheckedSlong(TIFF* tif, uint32* ndir, TIFFDirEntry* dir, uint16 tag, int32 value)
{
	int32 m;
	assert(sizeof(int32)==4);
	m=value;
	if (tif->tif_flags&TIFF_SWAB)
		TIFFSwabLong((uint32*)(&m));
	return(TIFFWriteDirectoryTagData(tif,ndir,dir,tag,TIFF_SLONG,1,4,&m));
}

static int
TIFFWriteDirectoryTagCheckedSlongArray(TIFF* tif, uint32* ndir, TIFFDirEntry* dir, uint16 tag, uint32 count, int32* value)
{
	assert(count<0x40000000);
	assert(sizeof(int32)==4);
	if (tif->tif_flags&TIFF_SWAB)
		TIFFSwabArrayOfLong((uint32*)value,count);
	return(TIFFWriteDirectoryTagData(tif,ndir,dir,tag,TIFF_SLONG,count,count*4,value));
}

static int
TIFFWriteDirectoryTagCheckedLong8(TIFF* tif, uint32* ndir, TIFFDirEntry* dir, uint16 tag, uint64_new value)
{
	uint64_new m;
	assert(sizeof(uint64_new)==8);
	assert(tif->tif_flags&TIFF_BIGTIFF);
	m=value;
	if (tif->tif_flags&TIFF_SWAB)
		TIFFSwabLong8(&m);
	return(TIFFWriteDirectoryTagData(tif,ndir,dir,tag,TIFF_LONG8,1,8,&m));
}

static int
TIFFWriteDirectoryTagCheckedLong8Array(TIFF* tif, uint32* ndir, TIFFDirEntry* dir, uint16 tag, uint32 count, uint64_new* value)
{
	assert(count<0x20000000);
	assert(sizeof(uint64_new)==8);
	assert(tif->tif_flags&TIFF_BIGTIFF);
	if (tif->tif_flags&TIFF_SWAB)
		TIFFSwabArrayOfLong8(value,count);
	return(TIFFWriteDirectoryTagData(tif,ndir,dir,tag,TIFF_LONG8,count,count*8,value));
}

static int
TIFFWriteDirectoryTagCheckedSlong8(TIFF* tif, uint32* ndir, TIFFDirEntry* dir, uint16 tag, int64_new value)
{
	int64_new m;
	assert(sizeof(int64_new)==8);
	assert(tif->tif_flags&TIFF_BIGTIFF);
	m=value;
	if (tif->tif_flags&TIFF_SWAB)
		TIFFSwabLong8((uint64_new*)(&m));
	return(TIFFWriteDirectoryTagData(tif,ndir,dir,tag,TIFF_SLONG8,1,8,&m));
}

static int
TIFFWriteDirectoryTagCheckedSlong8Array(TIFF* tif, uint32* ndir, TIFFDirEntry* dir, uint16 tag, uint32 count, int64_new* value)
{
	assert(count<0x20000000);
	assert(sizeof(int64_new)==8);
	assert(tif->tif_flags&TIFF_BIGTIFF);
	if (tif->tif_flags&TIFF_SWAB)
		TIFFSwabArrayOfLong8((uint64_new*)value,count);
	return(TIFFWriteDirectoryTagData(tif,ndir,dir,tag,TIFF_SLONG8,count,count*8,value));
}

static int
TIFFWriteDirectoryTagCheckedRational(TIFF* tif, uint32* ndir, TIFFDirEntry* dir, uint16 tag, double value)
{
	uint32 m[2];
	assert(value>=0.0);
	assert(sizeof(uint32)==4);
	if (value==(uint32)value)
	{
		m[0]=value;
		m[1]=1;
	}
	else if (value<1.0)
	{
		m[0]=(uint32)(value*0xFFFFFFFF);
		m[1]=0xFFFFFFFF;
	}
	else
	{
		m[0]=0xFFFFFFFF;
		m[1]=(uint32)(0xFFFFFFFF/value);
	}
	if (tif->tif_flags&TIFF_SWAB)
	{
		TIFFSwabLong(&m[0]);
		TIFFSwabLong(&m[1]);
	}
	return(TIFFWriteDirectoryTagData(tif,ndir,dir,tag,TIFF_RATIONAL,1,8,&m[0]));
}

static int
TIFFWriteDirectoryTagCheckedRationalArray(TIFF* tif, uint32* ndir, TIFFDirEntry* dir, uint16 tag, uint32 count, float* value)
{
	assert(0);
}

static int
TIFFWriteDirectoryTagCheckedSrationalArray(TIFF* tif, uint32* ndir, TIFFDirEntry* dir, uint16 tag, uint32 count, float* value)
{
	assert(0);
}

static int
TIFFWriteDirectoryTagCheckedFloat(TIFF* tif, uint32* ndir, TIFFDirEntry* dir, uint16 tag, float value)
{
	float m;
	assert(sizeof(float)==4);
	m=value;
	TIFFCvtNativeToIEEEFloat(tif,1,&m);
	if (tif->tif_flags&TIFF_SWAB)
		TIFFSwabLong((uint32*)(&m));
	return(TIFFWriteDirectoryTagData(tif,ndir,dir,tag,TIFF_FLOAT,1,4,&m));
}

static int
TIFFWriteDirectoryTagCheckedFloatArray(TIFF* tif, uint32* ndir, TIFFDirEntry* dir, uint16 tag, uint32 count, float* value)
{
	assert(count<0x40000000);
	assert(sizeof(float)==4);
	TIFFCvtNativeToIEEEFloat(tif,count,&value);
	if (tif->tif_flags&TIFF_SWAB)
		TIFFSwabArrayOfLong((uint32*)value,count);
	return(TIFFWriteDirectoryTagData(tif,ndir,dir,tag,TIFF_FLOAT,count,count*4,value));
}

static int
TIFFWriteDirectoryTagCheckedDouble(TIFF* tif, uint32* ndir, TIFFDirEntry* dir, uint16 tag, double value)
{
	double m;
	assert(sizeof(double)==8);
	m=value;
	TIFFCvtNativeToIEEEDouble(tif,1,&m);
	if (tif->tif_flags&TIFF_SWAB)
		TIFFSwabLong8((uint64_new*)(&m));
	return(TIFFWriteDirectoryTagData(tif,ndir,dir,tag,TIFF_DOUBLE,1,8,&m));
}

static int
TIFFWriteDirectoryTagCheckedDoubleArray(TIFF* tif, uint32* ndir, TIFFDirEntry* dir, uint16 tag, uint32 count, double* value)
{
	assert(count<0x20000000);
	assert(sizeof(double)==8);
	TIFFCvtNativeToIEEEDouble(tif,count,&value);
	if (tif->tif_flags&TIFF_SWAB)
		TIFFSwabArrayOfLong8((uint64_new*)value,count);
	return(TIFFWriteDirectoryTagData(tif,ndir,dir,tag,TIFF_DOUBLE,count,count*8,value));
}

static int
TIFFWriteDirectoryTagCheckedIfdArray(TIFF* tif, uint32* ndir, TIFFDirEntry* dir, uint16 tag, uint32 count, uint32* value)
{
	assert(count<0x40000000);
	assert(sizeof(uint32)==4);
	if (tif->tif_flags&TIFF_SWAB)
		TIFFSwabArrayOfLong(value,count);
	return(TIFFWriteDirectoryTagData(tif,ndir,dir,tag,TIFF_IFD,count,count*4,value));
}

static int
TIFFWriteDirectoryTagCheckedIfd8Array(TIFF* tif, uint32* ndir, TIFFDirEntry* dir, uint16 tag, uint32 count, uint64_new* value)
{
	assert(count<0x20000000);
	assert(sizeof(uint64_new)==8);
	assert(tif->tif_flags&TIFF_BIGTIFF);
	if (tif->tif_flags&TIFF_SWAB)
		TIFFSwabArrayOfLong8(value,count);
	return(TIFFWriteDirectoryTagData(tif,ndir,dir,tag,TIFF_IFD8,count,count*8,value));
}

static int
TIFFWriteDirectoryTagData(TIFF* tif, uint32* ndir, TIFFDirEntry* dir, uint16 tag, uint16 datatype, uint32 count, uint32 datalength, void* data)
{
	uint32 m;
	m=0;
	while (m<(*ndir))
	{
		assert(dir[m].tdir_tag!=tag);
		if (dir[m].tdir_tag>tag)
			break;
		m++;
	}
	if (m<(*ndir))
	{
		uint32 n;
		for (n=*ndir; n>m; n--)
			dir[n]=dir[n-1];
	}
	dir[m].tdir_tag=tag;
	dir[m].tdir_type=datatype;
	dir[m].tdir_count=count;
	dir[m].tdir_offset=0;
	if (datalength<=((tif->tif_flags&TIFF_BIGTIFF)?8:4))
		_TIFFmemcpy(&dir[m].tdir_offset,data,datalength);
	else
	{
		uint64_new na,nb;
		na=tif->tif_dataoff;
		nb=na+datalength;
		if (!(tif->tif_flags&TIFF_BIGTIFF))
			nb=(uint32)nb;
		if ((nb<na)||(nb<datalength)||(nb-datalength!=na))
			assert(0);  /* maximum length reached */
		if (!SeekOK(tif,na))
			assert(0);
		if (!WriteOK(tif,data,datalength))
			assert(0);
		tif->tif_dataoff=nb;
		if (tif->tif_dataoff&1)
			tif->tif_dataoff++;
		if (!(tif->tif_flags&TIFF_BIGTIFF))
		{
			uint32 o;
			o=(uint32)na;
			if (tif->tif_flags&TIFF_SWAB)
				TIFFSwabLong(&o);
			_TIFFmemcpy(&dir[m].tdir_offset,&o,4);
		}
		else
		{
			dir[m].tdir_offset=na;
			if (tif->tif_flags&TIFF_SWAB)
				TIFFSwabLong8(&dir[m].tdir_offset);
		}
	}
	(*ndir)++;
	return(1);
}

/*
 * Link the current directory into the directory chain for the file.
 */
static int
TIFFLinkDirectory(TIFF* tif)
{
	static const char module[] = "TIFFLinkDirectory";

	tif->tif_diroff = (TIFFSeekFile(tif,0,SEEK_END)+1) &~ 1;

	/*
	 * Handle SubIFDs
	 */
	if (tif->tif_flags & TIFF_INSUBIFD)
	{
		if (!(tif->tif_flags&TIFF_BIGTIFF))
		{
			uint32 m;
			m = (uint32)tif->tif_diroff;
			if (tif->tif_flags & TIFF_SWAB)
				TIFFSwabLong(&m);
			(void) TIFFSeekFile(tif, tif->tif_subifdoff, SEEK_SET);
			if (!WriteOK(tif, &m, 4)) {
				TIFFErrorExt(tif->tif_clientdata, module,
				     "Error writing SubIFD directory link");
				return (0);
			}
			/*
			 * Advance to the next SubIFD or, if this is
			 * the last one configured, revert back to the
			 * normal directory linkage.
			 */
			if (--tif->tif_nsubifd)
				tif->tif_subifdoff += 4;
			else
				tif->tif_flags &= ~TIFF_INSUBIFD;
			return (1);
		}
		else
		{
			uint64_new m;
			m = tif->tif_diroff;
			if (tif->tif_flags & TIFF_SWAB)
				TIFFSwabLong8(&m);
			(void) TIFFSeekFile(tif, tif->tif_subifdoff, SEEK_SET);
			if (!WriteOK(tif, &m, 8)) {
				TIFFErrorExt(tif->tif_clientdata, module,
				     "Error writing SubIFD directory link");
				return (0);
			}
			/*
			 * Advance to the next SubIFD or, if this is
			 * the last one configured, revert back to the
			 * normal directory linkage.
			 */
			if (--tif->tif_nsubifd)
				tif->tif_subifdoff += 8;
			else
				tif->tif_flags &= ~TIFF_INSUBIFD;
			return (1);
		}
	}

	if (!(tif->tif_flags&TIFF_BIGTIFF))
	{
		uint32 m;
		uint32 nextdir;
		m = tif->tif_diroff;
		if (tif->tif_flags & TIFF_SWAB)
			TIFFSwabLong(&m);
		if (tif->tif_header.classic.tiff_diroff == 0) {
			/*
			 * First directory, overwrite offset in header.
			 */
			tif->tif_header.classic.tiff_diroff = (uint32) tif->tif_diroff;
			(void) TIFFSeekFile(tif,4, SEEK_SET);
			if (!WriteOK(tif, &m, 4)) {
				TIFFErrorExt(tif->tif_clientdata, tif->tif_name,
					     "Error writing TIFF header");
				return (0);
			}
			return (1);
		}
		/*
		 * Not the first directory, search to the last and append.
		 */
		nextdir = tif->tif_header.classic.tiff_diroff;
		while(1) {
			uint16 dircount;
			uint32 nextnextdir;

			if (!SeekOK(tif, nextdir) ||
			    !ReadOK(tif, &dircount, 2)) {
				TIFFErrorExt(tif->tif_clientdata, module,
					     "Error fetching directory count");
				return (0);
			}
			if (tif->tif_flags & TIFF_SWAB)
				TIFFSwabShort(&dircount);
			(void) TIFFSeekFile(tif,
			    nextdir+2+dircount*12, SEEK_SET);
			if (!ReadOK(tif, &nextnextdir, 4)) {
				TIFFErrorExt(tif->tif_clientdata, module,
					     "Error fetching directory link");
				return (0);
			}
			if (tif->tif_flags & TIFF_SWAB)
				TIFFSwabLong(&nextnextdir);
			if (nextnextdir==0)
			{
				(void) TIFFSeekFile(tif,
				    nextdir+2+dircount*12, SEEK_SET);
				if (!WriteOK(tif, &m, 4)) {
					TIFFErrorExt(tif->tif_clientdata, module,
					     "Error writing directory link");
					return (0);
				}
				break;
			}
			nextdir=nextnextdir;
		}
	}
	else
	{
		uint64_new m;
		uint64_new nextdir;
		m = tif->tif_diroff;
		if (tif->tif_flags & TIFF_SWAB)
			TIFFSwabLong8(&m);
		if (tif->tif_header.big.tiff_diroff == 0) {
			/*
			 * First directory, overwrite offset in header.
			 */
			tif->tif_header.big.tiff_diroff = tif->tif_diroff;
			(void) TIFFSeekFile(tif,8, SEEK_SET);
			if (!WriteOK(tif, &m, 8)) {
				TIFFErrorExt(tif->tif_clientdata, tif->tif_name,
					     "Error writing TIFF header");
				return (0);
			}
			return (1);
		}
		/*
		 * Not the first directory, search to the last and append.
		 */
		nextdir = tif->tif_header.big.tiff_diroff;
		while(1) {
			uint64_new dircount64;
			uint16 dircount;
			uint64_new nextnextdir;

			if (!SeekOK(tif, nextdir) ||
			    !ReadOK(tif, &dircount64, 8)) {
				TIFFErrorExt(tif->tif_clientdata, module,
					     "Error fetching directory count");
				return (0);
			}
			if (tif->tif_flags & TIFF_SWAB)
				TIFFSwabLong8(&dircount64);
			if (dircount64>0xFFFF)
				assert(0);
			dircount=(uint16)dircount64;
			(void) TIFFSeekFile(tif,
			    nextdir+8+dircount*20, SEEK_SET);
			if (!ReadOK(tif, &nextnextdir, 8)) {
				TIFFErrorExt(tif->tif_clientdata, module,
					     "Error fetching directory link");
				return (0);
			}
			if (tif->tif_flags & TIFF_SWAB)
				TIFFSwabLong8(&nextnextdir);
			if (nextnextdir==0)
			{
				(void) TIFFSeekFile(tif,
				    nextdir+8+dircount*20, SEEK_SET);
				if (!WriteOK(tif, &m, 8)) {
					TIFFErrorExt(tif->tif_clientdata, module,
					     "Error writing directory link");
					return (0);
				}
				break;
			}
			nextdir=nextnextdir;
		}
	}
	return (1);
}

	#ifdef NDEF
	uint16 dircount;
	toff_t diroff;
	ttag_t tag;
	uint32 nfields;
	tsize_t dirsize;
	char* data;
	TIFFDirEntry* dir;
	TIFFDirectory* td;
	unsigned long b, fields[FIELD_SETLONGS];
	int fi, nfi;

	if (tif->tif_mode == O_RDONLY)
		return (1);
	/*
	 * Clear write state so that subsequent images with
	 * different characteristics get the right buffers
	 * setup for them.
	 */
	if (done)
	{
		if (tif->tif_flags & TIFF_POSTENCODE) {
			tif->tif_flags &= ~TIFF_POSTENCODE;
			if (!(*tif->tif_postencode)(tif)) {
				TIFFErrorExt(tif->tif_clientdata,
					     tif->tif_name,
				"Error post-encoding before directory write");
				return (0);
			}
		}
		(*tif->tif_close)(tif);		/* shutdown encoder */
		/*
		 * Flush any data that might have been written
		 * by the compression close+cleanup routines.
		 */
		if (tif->tif_rawcc > 0
		    && (tif->tif_flags & TIFF_BEENWRITING) != 0
		    && !TIFFFlushData1(tif)) {
			TIFFErrorExt(tif->tif_clientdata, tif->tif_name,
			    "Error flushing data before directory write");
			return (0);
		}
		if ((tif->tif_flags & TIFF_MYBUFFER) && tif->tif_rawdata) {
			_TIFFfree(tif->tif_rawdata);
			tif->tif_rawdata = NULL;
			tif->tif_rawcc = 0;
			tif->tif_rawdatasize = 0;
		}
		tif->tif_flags &= ~(TIFF_BEENWRITING|TIFF_BUFFERSETUP);
	}

	td = &tif->tif_dir;
	/*
	 * Size the directory so that we can calculate
	 * offsets for the data items that aren't kept
	 * in-place in each field.
	 */
	nfields = 0;
	for (b = 0; b <= FIELD_LAST; b++)
		if (TIFFFieldSet(tif, b) && b != FIELD_CUSTOM)
			nfields += (b < FIELD_SUBFILETYPE ? 2 : 1);
	nfields += td->td_customValueCount;
	dirsize = nfields * sizeof (TIFFDirEntry);
	data = (char*) _TIFFmalloc(dirsize);
	if (data == NULL) {
		TIFFErrorExt(tif->tif_clientdata, tif->tif_name,
			     "Cannot write directory, out of space");
		return (0);
	}
	/*
	 * Directory hasn't been placed yet, put
	 * it at the end of the file and link it
	 * into the existing directory structure.
	 */
	if (tif->tif_diroff == 0 && !TIFFLinkDirectory(tif))
		goto bad;
	tif->tif_dataoff = (toff_t)(
	    tif->tif_diroff + sizeof (uint16) + dirsize + sizeof (toff_t));
	if (tif->tif_dataoff & 1)
		tif->tif_dataoff++;
	(void) TIFFSeekFile(tif, tif->tif_dataoff, SEEK_SET);
	tif->tif_curdir++;
	dir = (TIFFDirEntry*) data;
	/*
	 * Setup external form of directory
	 * entries and write data items.
	 */
	_TIFFmemcpy(fields, td->td_fieldsset, sizeof (fields));
	/*
	 * Write out ExtraSamples tag only if
	 * extra samples are present in the data.
	 */
	if (FieldSet(fields, FIELD_EXTRASAMPLES) && !td->td_extrasamples) {
		ResetFieldBit(fields, FIELD_EXTRASAMPLES);
		nfields--;
		dirsize -= sizeof (TIFFDirEntry);
	}								/*XXX*/
	for (fi = 0, nfi = tif->tif_nfields; nfi > 0; nfi--, fi++) {  ddd
		const TIFFFieldInfo* fip = tif->tif_fieldinfo[fi];

		/*
		 * For custom fields, we test to see if the custom field
		 * is set or not.  For normal fields, we just use the
		 * FieldSet test.
		*/
		if( fip->field_bit == FIELD_CUSTOM )
		{
			int ci, is_set = FALSE;

			for( ci = 0; ci < td->td_customValueCount; ci++ )
				is_set |= (td->td_customValues[ci].info == fip);

			if( !is_set )
				continue;
		}
		else if (!FieldSet(fields, fip->field_bit))
			continue;

		/*
		 * Handle other fields.
		 */
		switch (fip->field_bit)
		{
		case FIELD_STRIPOFFSETS:
			/*
			 * We use one field bit for both strip and tile

			 * offsets, and so must be careful in selecting
			 * the appropriate field descriptor (so that tags
			 * are written in sorted order).
			 */
			tag = isTiled(tif) ?
			    TIFFTAG_TILEOFFSETS : TIFFTAG_STRIPOFFSETS;
			if (tag != fip->field_tag)
				continue;
			
			dir->tdir_tag = (uint16) tag;
			dir->tdir_type = (uint16) TIFF_LONG;
			dir->tdir_count = (uint32) td->td_nstrips;  ddd
			if (!TIFFWriteLongArray(tif, dir, td->td_stripoffset))  ddd
				goto bad;
			break;
		case FIELD_STRIPBYTECOUNTS:
			/*
			 * We use one field bit for both strip and tile
			 * byte counts, and so must be careful in selecting
			 * the appropriate field descriptor (so that tags
			 * are written in sorted order).
			 */
			tag = isTiled(tif) ?
			    TIFFTAG_TILEBYTECOUNTS : TIFFTAG_STRIPBYTECOUNTS;
			if (tag != fip->field_tag)
				continue;

			dir->tdir_tag = (uint16) tag;
			dir->tdir_type = (uint16) TIFF_LONG;
			dir->tdir_count = (uint32) td->td_nstrips;  ddd
			if (!TIFFWriteLongArray(tif, dir, td->td_stripbytecount))
				goto bad;
			break;
		case FIELD_ROWSPERSTRIP:
			TIFFSetupShortLong(tif, TIFFTAG_ROWSPERSTRIP,
			    dir, td->td_rowsperstrip);
			break;
		case FIELD_COLORMAP:
			if (!TIFFWriteShortTable(tif, TIFFTAG_COLORMAP, dir,
			    3, td->td_colormap))
				goto bad;
			break;
		case FIELD_IMAGEDIMENSIONS:
			TIFFSetupShortLong(tif, TIFFTAG_IMAGEWIDTH,
			    dir++, td->td_imagewidth);
			TIFFSetupShortLong(tif, TIFFTAG_IMAGELENGTH,
			    dir, td->td_imagelength);
			break;
		case FIELD_TILEDIMENSIONS:
			TIFFSetupShortLong(tif, TIFFTAG_TILEWIDTH,
			    dir++, td->td_tilewidth);
			TIFFSetupShortLong(tif, TIFFTAG_TILELENGTH,
			    dir, td->td_tilelength);
			break;
		case FIELD_COMPRESSION:
			TIFFSetupShort(tif, TIFFTAG_COMPRESSION,
			    dir, td->td_compression);
			break;
		case FIELD_PHOTOMETRIC:
			TIFFSetupShort(tif, TIFFTAG_PHOTOMETRIC,
			    dir, td->td_photometric);
			break;
		case FIELD_POSITION:
			WriteRationalPair(TIFF_RATIONAL,
			    TIFFTAG_XPOSITION, td->td_xposition,
			    TIFFTAG_YPOSITION, td->td_yposition);
			break;
		case FIELD_RESOLUTION:
			WriteRationalPair(TIFF_RATIONAL,
			    TIFFTAG_XRESOLUTION, td->td_xresolution,
			    TIFFTAG_YRESOLUTION, td->td_yresolution);
			break;
		case FIELD_BITSPERSAMPLE:
		case FIELD_MINSAMPLEVALUE:
		case FIELD_MAXSAMPLEVALUE:
		case FIELD_SAMPLEFORMAT:
			if (!TIFFWritePerSampleShorts(tif, fip->field_tag, dir))
				goto bad;
			break;
		case FIELD_SMINSAMPLEVALUE:
		case FIELD_SMAXSAMPLEVALUE:
			if (!TIFFWritePerSampleAnys(tif,
			    _TIFFSampleToTagType(tif), fip->field_tag, dir))
				goto bad;
			break;
		case FIELD_PAGENUMBER:
		case FIELD_HALFTONEHINTS:
		case FIELD_YCBCRSUBSAMPLING:
			if (!TIFFSetupShortPair(tif, fip->field_tag, dir))
				goto bad;
			break;
		case FIELD_INKNAMES:
			if (!TIFFWriteInkNames(tif, dir))
				goto bad;
			break;
		case FIELD_TRANSFERFUNCTION:
			if (!TIFFWriteTransferFunction(tif, dir))
				goto bad;
			break;
		case FIELD_SUBIFD:
			/*
			 * XXX: Always write this field using LONG type
			 * for backward compatibility.
			 */
			dir->tdir_tag = (uint16) fip->field_tag;
			dir->tdir_type = (uint16) TIFF_LONG;
			dir->tdir_count = (uint32) td->td_nsubifd;
			if (!TIFFWriteLongArray(tif, dir, td->td_subifd))
				goto bad;
			/*
			 * Total hack: if this directory includes a SubIFD
			 * tag then force the next <n> directories to be
			 * written as ``sub directories'' of this one.  This
			 * is used to write things like thumbnails and
			 * image masks that one wants to keep out of the
			 * normal directory linkage access mechanism.
			 */
			if (dir->tdir_count > 0) {
				tif->tif_flags |= TIFF_INSUBIFD;
				tif->tif_nsubifd = (uint16) dir->tdir_count;
				if (dir->tdir_count > 1)
					tif->tif_subifdoff = dir->tdir_offset;
				else
					tif->tif_subifdoff = (uint32)(
					      tif->tif_diroff
					    + sizeof (uint16)
					    + ((char*)&dir->tdir_offset-data));
			}
			break;
		default:
			/* XXX: Should be fixed and removed. */
			if (fip->field_tag == TIFFTAG_DOTRANGE) {
				if (!TIFFSetupShortPair(tif, fip->field_tag, dir))
					goto bad;
			}
			else if (!TIFFWriteNormalTag(tif, dir, fip))
				goto bad;
			break;
		}
		dir++;

		if( fip->field_bit != FIELD_CUSTOM )
			ResetFieldBit(fields, fip->field_bit);
	}

	/*
	 * Write directory.
	 */
	dircount = (uint16) nfields;
	diroff = (uint32) tif->tif_nextdiroff;
	if (tif->tif_flags & TIFF_SWAB) {
		/*
		 * The file's byte order is opposite to the
		 * native machine architecture.  We overwrite
		 * the directory information with impunity
		 * because it'll be released below after we
		 * write it to the file.  Note that all the
		 * other tag construction routines assume that
		 * we do this byte-swapping; i.e. they only
		 * byte-swap indirect data.
		 */
		for (dir = (TIFFDirEntry*) data; dircount; dir++, dircount--) {
			TIFFSwabArrayOfShort(&dir->tdir_tag, 2);  ddd
			TIFFSwabArrayOfLong(&dir->tdir_count, 2);  ddd
		}
		dircount = (uint16) nfields;
		TIFFSwabShort(&dircount);
		TIFFSwabLong(&diroff);
	}
	(void) TIFFSeekFile(tif, tif->tif_diroff, SEEK_SET);
	if (!WriteOK(tif, &dircount, sizeof (dircount))) {
		TIFFErrorExt(tif->tif_clientdata, tif->tif_name,
			     "Error writing directory count");
		goto bad;
	}
	if (!WriteOK(tif, data, dirsize)) {
		TIFFErrorExt(tif->tif_clientdata, tif->tif_name,
			     "Error writing directory contents");
		goto bad;
	}
	if (!WriteOK(tif, &diroff, sizeof (uint32))) {
		TIFFErrorExt(tif->tif_clientdata, tif->tif_name,
			     "Error writing directory link");
		goto bad;
	}
	if (done) {
		TIFFFreeDirectory(tif);
		tif->tif_flags &= ~TIFF_DIRTYDIRECT;
		(*tif->tif_cleanup)(tif);

		/*
		* Reset directory-related state for subsequent
		* directories.
		*/
		TIFFCreateDirectory(tif);
	}
	_TIFFfree(data);
	return (1);
bad:
	_TIFFfree(data);
	return (0);
}
#endif
#undef WriteRationalPair

int
TIFFWriteDirectory(TIFF* tif)
{
	return _TIFFWriteDirectory(tif, TRUE);
}

#ifdef NDEF

/*
 * Similar to TIFFWriteDirectory(), writes the directory out
 * but leaves all data structures in memory so that it can be
 * written again.  This will make a partially written TIFF file
 * readable before it is successfully completed/closed.
 */
int
TIFFCheckpointDirectory(TIFF* tif)
{
	int rc;
	/* Setup the strips arrays, if they haven't already been. */
	if (tif->tif_dir.td_stripoffset == NULL)
	    (void) TIFFSetupStrips(tif);
	rc = _TIFFWriteDirectory(tif, FALSE);
	(void) TIFFSetWriteOffset(tif, TIFFSeekFile(tif, 0, SEEK_END));
	return rc;
}

static int
_TIFFWriteCustomDirectory(TIFF* tif, toff_t *pdiroff)
{
	uint16 dircount;
	uint32 nfields;
	tsize_t dirsize;
	char* data;
	TIFFDirEntry* dir;
	TIFFDirectory* td;
	unsigned long b, fields[FIELD_SETLONGS];
	int fi, nfi;

	if (tif->tif_mode == O_RDONLY)
		return (1);

	td = &tif->tif_dir;
	/*
	 * Size the directory so that we can calculate
	 * offsets for the data items that aren't kept
	 * in-place in each field.
	 */
	nfields = 0;
	for (b = 0; b <= FIELD_LAST; b++)
		if (TIFFFieldSet(tif, b) && b != FIELD_CUSTOM)
			nfields += (b < FIELD_SUBFILETYPE ? 2 : 1);
	nfields += td->td_customValueCount;
	dirsize = nfields * sizeof (TIFFDirEntry);
	data = (char*) _TIFFmalloc(dirsize);
	if (data == NULL) {
		TIFFErrorExt(tif->tif_clientdata, tif->tif_name,
			     "Cannot write directory, out of space");
		return (0);
	}
	/*
	 * Put the directory  at the end of the file.
	 */
	tif->tif_diroff = (TIFFSeekFile(tif, (toff_t) 0, SEEK_END)+1) &~ 1;
	tif->tif_dataoff = (toff_t)(
	    tif->tif_diroff + sizeof (uint16) + dirsize + sizeof (toff_t));
	if (tif->tif_dataoff & 1)
		tif->tif_dataoff++;
	(void) TIFFSeekFile(tif, tif->tif_dataoff, SEEK_SET);
	dir = (TIFFDirEntry*) data;
	/*
	 * Setup external form of directory
	 * entries and write data items.
	 */
	_TIFFmemcpy(fields, td->td_fieldsset, sizeof (fields));

	for (fi = 0, nfi = tif->tif_nfields; nfi > 0; nfi--, fi++) {  ddd
		const TIFFFieldInfo* fip = tif->tif_fieldinfo[fi];

		/*
		 * For custom fields, we test to see if the custom field
		 * is set or not.  For normal fields, we just use the
		 * FieldSet test.
		*/
		if( fip->field_bit == FIELD_CUSTOM )
		{
			int ci, is_set = FALSE;

			for( ci = 0; ci < td->td_customValueCount; ci++ )
				is_set |= (td->td_customValues[ci].info == fip);

			if( !is_set )
				continue;
		}
		else if (!FieldSet(fields, fip->field_bit))
			continue;
                
		if( fip->field_bit != FIELD_CUSTOM )
			ResetFieldBit(fields, fip->field_bit);
	}

	/*
	 * Write directory.
	 */
	dircount = (uint16) nfields;
	*pdiroff = (uint32) tif->tif_nextdiroff;
	if (tif->tif_flags & TIFF_SWAB) {
		/*
		 * The file's byte order is opposite to the
		 * native machine architecture.  We overwrite
		 * the directory information with impunity
		 * because it'll be released below after we
		 * write it to the file.  Note that all the
		 * other tag construction routines assume that
		 * we do this byte-swapping; i.e. they only
		 * byte-swap indirect data.
		 */
		for (dir = (TIFFDirEntry*) data; dircount; dir++, dircount--) {
			TIFFSwabArrayOfShort(&dir->tdir_tag, 2);  ddd
			TIFFSwabArrayOfLong(&dir->tdir_count, 2);  ddd
		}
		dircount = (uint16) nfields;
		TIFFSwabShort(&dircount);
		TIFFSwabLong(pdiroff);
	}
	(void) TIFFSeekFile(tif, tif->tif_diroff, SEEK_SET);
	if (!WriteOK(tif, &dircount, sizeof (dircount))) {
		TIFFErrorExt(tif->tif_clientdata, tif->tif_name,
			     "Error writing directory count");
		goto bad;
	}
	if (!WriteOK(tif, data, dirsize)) {
		TIFFErrorExt(tif->tif_clientdata, tif->tif_name,
			     "Error writing directory contents");
		goto bad;
	}
	if (!WriteOK(tif, pdiroff, sizeof (uint32))) {
		TIFFErrorExt(tif->tif_clientdata, tif->tif_name,
			     "Error writing directory link");
		goto bad;
	}
	_TIFFfree(data);
	return (1);
bad:
	_TIFFfree(data);
	return (0);
}

int
TIFFWriteCustomDirectory(TIFF* tif, toff_t *pdiroff)
{
	return _TIFFWriteCustomDirectory(tif, pdiroff);
}

/*
 * Process tags that are not special cased.
 */
static int
TIFFWriteNormalTag(TIFF* tif, TIFFDirEntry* dir, const TIFFFieldInfo* fip)
{
	uint16 wc = (uint16) fip->field_writecount;
	uint32 wc2;

	dir->tdir_tag = (uint16) fip->field_tag;
	dir->tdir_type = (uint16) fip->field_type;
	dir->tdir_count = wc;
	
	switch (fip->field_type) {
	case TIFF_SHORT:
	case TIFF_SSHORT:
		if (fip->field_passcount) {
			uint16* wp;
			if (wc == (uint16) TIFF_VARIABLE2) {
				TIFFGetField(tif, fip->field_tag, &wc2, &wp);
				dir->tdir_count = wc2;
			} else {	/* Assume TIFF_VARIABLE */
				TIFFGetField(tif, fip->field_tag, &wc, &wp);
				dir->tdir_count = wc;
			}
			if (!TIFFWriteShortArray(tif, dir, wp))
				return 0;
		} else {
			if (wc == 1) {
				uint16 sv;
				TIFFGetField(tif, fip->field_tag, &sv);
				dir->tdir_offset =
					TIFFInsertData(tif, dir->tdir_type, sv);
			} else {
				uint16* wp;
				TIFFGetField(tif, fip->field_tag, &wp);
				if (!TIFFWriteShortArray(tif, dir, wp))
					return 0;
			}
		}
		break;
	case TIFF_LONG:
	case TIFF_SLONG:
	case TIFF_IFD:
		if (fip->field_passcount) {
			uint32* lp;
			if (wc == (uint16) TIFF_VARIABLE2) {
				TIFFGetField(tif, fip->field_tag, &wc2, &lp);
				dir->tdir_count = wc2;
			} else {	/* Assume TIFF_VARIABLE */
				TIFFGetField(tif, fip->field_tag, &wc, &lp);
				dir->tdir_count = wc;
			}
			if (!TIFFWriteLongArray(tif, dir, lp))
				return 0;
		} else {
			if (wc == 1) {
				/* XXX handle LONG->SHORT conversion */
				TIFFGetField(tif, fip->field_tag,
					     &dir->tdir_offset);
			} else {
				uint32* lp;
				TIFFGetField(tif, fip->field_tag, &lp);
				if (!TIFFWriteLongArray(tif, dir, lp))
					return 0;
			}
		}
		break;
	case TIFF_RATIONAL:
	case TIFF_SRATIONAL:
		if (fip->field_passcount) {
			float* fp;
			if (wc == (uint16) TIFF_VARIABLE2) {
				TIFFGetField(tif, fip->field_tag, &wc2, &fp);
				dir->tdir_count = wc2;
			} else {	/* Assume TIFF_VARIABLE */
				TIFFGetField(tif, fip->field_tag, &wc, &fp);
				dir->tdir_count = wc;
			}
			if (!TIFFWriteRationalArray(tif, dir, fp))
				return 0;
		} else {
			if (wc == 1) {
				float fv;
				TIFFGetField(tif, fip->field_tag, &fv);
				if (!TIFFWriteRationalArray(tif, dir, &fv))
					return 0;
			} else {
				float* fp;
				TIFFGetField(tif, fip->field_tag, &fp);
				if (!TIFFWriteRationalArray(tif, dir, fp))
					return 0;
			}
		}
		break;
	case TIFF_FLOAT:
		if (fip->field_passcount) {
			float* fp;
			if (wc == (uint16) TIFF_VARIABLE2) {
				TIFFGetField(tif, fip->field_tag, &wc2, &fp);
				dir->tdir_count = wc2;
			} else {	/* Assume TIFF_VARIABLE */
				TIFFGetField(tif, fip->field_tag, &wc, &fp);
				dir->tdir_count = wc;
			}
			if (!TIFFWriteFloatArray(tif, dir, fp))
				return 0;
		} else {
			if (wc == 1) {
				float fv;
				TIFFGetField(tif, fip->field_tag, &fv);
				if (!TIFFWriteFloatArray(tif, dir, &fv))
					return 0;
			} else {
				float* fp;
				TIFFGetField(tif, fip->field_tag, &fp);
				if (!TIFFWriteFloatArray(tif, dir, fp))
					return 0;
			}
		}
		break;
	case TIFF_DOUBLE:
		if (fip->field_passcount) {
			double* dp;
			if (wc == (uint16) TIFF_VARIABLE2) {
				TIFFGetField(tif, fip->field_tag, &wc2, &dp);
				dir->tdir_count = wc2;
			} else {	/* Assume TIFF_VARIABLE */
				TIFFGetField(tif, fip->field_tag, &wc, &dp);
				dir->tdir_count = wc;
			}
			if (!TIFFWriteDoubleArray(tif, dir, dp))
				return 0;
		} else {
			if (wc == 1) {
				double dv;
				TIFFGetField(tif, fip->field_tag, &dv);
				if (!TIFFWriteDoubleArray(tif, dir, &dv))
					return 0;
			} else {
				double* dp;
				TIFFGetField(tif, fip->field_tag, &dp);
				if (!TIFFWriteDoubleArray(tif, dir, dp))
					return 0;
			}
		}
		break;
	case TIFF_ASCII:
		{ 
                    char* cp;
                    if (fip->field_passcount)
                    {
                        if( wc == (uint16) TIFF_VARIABLE2 )
                            TIFFGetField(tif, fip->field_tag, &wc2, &cp);
                        else
                            TIFFGetField(tif, fip->field_tag, &wc, &cp);
                    }
                    else
                        TIFFGetField(tif, fip->field_tag, &cp);

                    dir->tdir_count = (uint32) (strlen(cp) + 1);
                    if (!TIFFWriteByteArray(tif, dir, cp))
                        return (0);
		}
		break;

        case TIFF_BYTE:
        case TIFF_SBYTE:          
		if (fip->field_passcount) {
			char* cp;
			if (wc == (uint16) TIFF_VARIABLE2) {
				TIFFGetField(tif, fip->field_tag, &wc2, &cp);
				dir->tdir_count = wc2;
			} else {	/* Assume TIFF_VARIABLE */
				TIFFGetField(tif, fip->field_tag, &wc, &cp);
				dir->tdir_count = wc;
			}
			if (!TIFFWriteByteArray(tif, dir, cp))
				return 0;
		} else {
			if (wc == 1) {
				char cv;
				TIFFGetField(tif, fip->field_tag, &cv);
				if (!TIFFWriteByteArray(tif, dir, &cv))
					return 0;
			} else {
				char* cp;
				TIFFGetField(tif, fip->field_tag, &cp);
				if (!TIFFWriteByteArray(tif, dir, cp))
					return 0;
			}
		}
                break;

	case TIFF_UNDEFINED:
		{ char* cp;
		  if (wc == (unsigned short) TIFF_VARIABLE) {
			TIFFGetField(tif, fip->field_tag, &wc, &cp);
			dir->tdir_count = wc;
		  } else if (wc == (unsigned short) TIFF_VARIABLE2) {
			TIFFGetField(tif, fip->field_tag, &wc2, &cp);
			dir->tdir_count = wc2;
		  } else 
			TIFFGetField(tif, fip->field_tag, &cp);
		  if (!TIFFWriteByteArray(tif, dir, cp))
			return (0);
		}
		break;

        case TIFF_NOTYPE:
                break;
	}
	return (1);
}

/*
 * Setup a directory entry with either a SHORT
 * or LONG type according to the value.
 */
static void
TIFFSetupShortLong(TIFF* tif, ttag_t tag, TIFFDirEntry* dir, uint32 v)
{
	dir->tdir_tag = (uint16) tag;
	dir->tdir_count = 1;
	if (v > 0xffffL) {
		dir->tdir_type = (short) TIFF_LONG;
		dir->tdir_offset = v;
	} else {
		dir->tdir_type = (short) TIFF_SHORT;
		dir->tdir_offset = TIFFInsertData(tif, (int) TIFF_SHORT, v);
	}
}

/*
 * Setup a SHORT directory entry
 */
static void
TIFFSetupShort(TIFF* tif, ttag_t tag, TIFFDirEntry* dir, uint16 v)
{
	dir->tdir_tag = (uint16) tag;
	dir->tdir_count = 1;
	dir->tdir_type = (short) TIFF_SHORT;
	dir->tdir_offset = TIFFInsertData(tif, (int) TIFF_SHORT, v);
}
#undef MakeShortDirent

#define	NITEMS(x)	(sizeof (x) / sizeof (x[0]))
/*
 * Setup a directory entry that references a
 * samples/pixel array of SHORT values and
 * (potentially) write the associated indirect
 * values.
 */
static int
TIFFWritePerSampleShorts(TIFF* tif, ttag_t tag, TIFFDirEntry* dir)
{
	uint16 buf[10], v;
	uint16* w = buf;
	uint16 i, samples = tif->tif_dir.td_samplesperpixel;
	int status;

	if (samples > NITEMS(buf)) {
		w = (uint16*) _TIFFmalloc(samples * sizeof (uint16));
		if (w == NULL) {
			TIFFErrorExt(tif->tif_clientdata, tif->tif_name,
			    "No space to write per-sample shorts");
			return (0);
		}
	}
	TIFFGetField(tif, tag, &v);
	for (i = 0; i < samples; i++)
		w[i] = v;
	
	dir->tdir_tag = (uint16) tag;
	dir->tdir_type = (uint16) TIFF_SHORT;
	dir->tdir_count = samples;
	status = TIFFWriteShortArray(tif, dir, w);
	if (w != buf)
		_TIFFfree((char*) w);
	return (status);
}

/*
 * Setup a directory entry that references a samples/pixel array of ``type''
 * values and (potentially) write the associated indirect values.  The source
 * data from TIFFGetField() for the specified tag must be returned as double.
 */
static int
TIFFWritePerSampleAnys(TIFF* tif,
    TIFFDataType type, ttag_t tag, TIFFDirEntry* dir)
{
	double buf[10], v;
	double* w = buf;
	uint16 i, samples = tif->tif_dir.td_samplesperpixel;
	int status;

	if (samples > NITEMS(buf)) {
		w = (double*) _TIFFmalloc(samples * sizeof (double));
		if (w == NULL) {
			TIFFErrorExt(tif->tif_clientdata, tif->tif_name,
			    "No space to write per-sample values");
			return (0);
		}
	}
	TIFFGetField(tif, tag, &v);
	for (i = 0; i < samples; i++)
		w[i] = v;
	status = TIFFWriteAnyArray(tif, type, tag, dir, samples, w);
	if (w != buf)
		_TIFFfree(w);
	return (status);
}
#undef NITEMS

/*
 * Setup a pair of shorts that are returned by
 * value, rather than as a reference to an array.
 */
static int
TIFFSetupShortPair(TIFF* tif, ttag_t tag, TIFFDirEntry* dir)
{
	uint16 v[2];

	TIFFGetField(tif, tag, &v[0], &v[1]);

	dir->tdir_tag = (uint16) tag;
	dir->tdir_type = (uint16) TIFF_SHORT;
	dir->tdir_count = 2;
	return (TIFFWriteShortArray(tif, dir, v));
}

/*
 * Setup a directory entry for an NxM table of shorts,
 * where M is known to be 2**bitspersample, and write
 * the associated indirect data.
 */
static int
TIFFWriteShortTable(TIFF* tif,
    ttag_t tag, TIFFDirEntry* dir, uint32 n, uint16** table)
{
	uint32 i, off;

	dir->tdir_tag = (uint16) tag;
	dir->tdir_type = (short) TIFF_SHORT;
	/* XXX -- yech, fool TIFFWriteData */
	dir->tdir_count = (uint32) (1L<<tif->tif_dir.td_bitspersample);
	off = tif->tif_dataoff;
	for (i = 0; i < n; i++)
		if (!TIFFWriteData(tif, dir, (char *)table[i]))
			return (0);
	dir->tdir_count *= n;
	dir->tdir_offset = off;
	return (1);
}

/*
 * Write/copy data associated with an ASCII or opaque tag value.
 */
static int
TIFFWriteByteArray(TIFF* tif, TIFFDirEntry* dir, char* cp)
{
	if (dir->tdir_count > 4) {
		if (!TIFFWriteData(tif, dir, cp))
			return (0);
	} else
		_TIFFmemcpy(&dir->tdir_offset, cp, dir->tdir_count);
	return (1);
}

/*
 * Setup a directory entry of an array of SHORT
 * or SSHORT and write the associated indirect values.
 */
static int
TIFFWriteShortArray(TIFF* tif, TIFFDirEntry* dir, uint16* v)
{
	if (dir->tdir_count <= 2) {
		if (tif->tif_header.common.tiff_magic == TIFF_BIGENDIAN) {
			dir->tdir_offset = (uint32) ((long) v[0] << 16);
			if (dir->tdir_count == 2)
				dir->tdir_offset |= v[1] & 0xffff;
		} else {
			dir->tdir_offset = v[0] & 0xffff;
			if (dir->tdir_count == 2)
				dir->tdir_offset |= (long) v[1] << 16;
		}
		return (1);
	} else
		return (TIFFWriteData(tif, dir, (char*) v));
}

/*
 * Setup a directory entry of an array of LONG
 * or SLONG and write the associated indirect values.
 */
static int
TIFFWriteLongArray(TIFF* tif, TIFFDirEntry* dir, uint32* v)
{
	if (dir->tdir_count == 1) {
		dir->tdir_offset = v[0];
		return (1);
	} else
		return (TIFFWriteData(tif, dir, (char*) v));
}

/*
 * Setup a directory entry of an array of RATIONAL
 * or SRATIONAL and write the associated indirect values.
 */
static int
TIFFWriteRationalArray(TIFF* tif, TIFFDirEntry* dir, float* v)
{
	uint32 i;
	uint32* t;
	int status;

	t = (uint32*) _TIFFmalloc(2 * dir->tdir_count * sizeof (uint32));
	if (t == NULL) {
		TIFFErrorExt(tif->tif_clientdata, tif->tif_name,
		    "No space to write RATIONAL array");
		return (0);
	}
	for (i = 0; i < dir->tdir_count; i++) {
		float fv = v[i];
		int sign = 1;
		uint32 den;

		if (fv < 0) {
			if (dir->tdir_type == TIFF_RATIONAL) {
				TIFFWarningExt(tif->tif_clientdata,
					       tif->tif_name,
	"\"%s\": Information lost writing value (%g) as (unsigned) RATIONAL",
				_TIFFFieldWithTag(tif,dir->tdir_tag)->field_name,
						fv);
				fv = 0;
			} else
				fv = -fv, sign = -1;
		}
		den = 1L;
		if (fv > 0) {
			while (fv < 1L<<(31-3) && den < 1L<<(31-3))
				fv *= 1<<3, den *= 1L<<3;
		}
		t[2*i+0] = (uint32) (sign * (fv + 0.5));
		t[2*i+1] = den;
	}
	status = TIFFWriteData(tif, dir, (char *)t);
	_TIFFfree((char*) t);
	return (status);
}

static int
TIFFWriteFloatArray(TIFF* tif, TIFFDirEntry* dir, float* v)
{
	TIFFCvtNativeToIEEEFloat(tif, dir->tdir_count, v);
	if (dir->tdir_count == 1) {
		dir->tdir_offset = *(uint32*) &v[0];
		return (1);
	} else
		return (TIFFWriteData(tif, dir, (char*) v));
}

static int
TIFFWriteDoubleArray(TIFF* tif, TIFFDirEntry* dir, double* v)
{
	TIFFCvtNativeToIEEEDouble(tif, dir->tdir_count, v);
	return (TIFFWriteData(tif, dir, (char*) v));
}

/*
 * Write an array of ``type'' values for a specified tag (i.e. this is a tag
 * which is allowed to have different types, e.g. SMaxSampleType).
 * Internally the data values are represented as double since a double can
 * hold any of the TIFF tag types (yes, this should really be an abstract
 * type tany_t for portability).  The data is converted into the specified
 * type in a temporary buffer and then handed off to the appropriate array
 * writer.
 */
static int
TIFFWriteAnyArray(TIFF* tif,
    TIFFDataType type, ttag_t tag, TIFFDirEntry* dir, uint32 n, double* v)
{
	char buf[10 * sizeof(double)];
	char* w = buf;
	int i, status = 0;

	if (n * TIFFDataWidth(type) > sizeof buf) {
		w = (char*) _TIFFmalloc(n * TIFFDataWidth(type));
		if (w == NULL) {
			TIFFErrorExt(tif->tif_clientdata, tif->tif_name,
				     "No space to write array");
			return (0);
		}
	}

	dir->tdir_tag = (uint16) tag;
	dir->tdir_type = (uint16) type;
	dir->tdir_count = n;

	switch (type) {
	case TIFF_BYTE:
		{ 
			uint8* bp = (uint8*) w;
			for (i = 0; i < (int) n; i++)
				bp[i] = (uint8) v[i];
			if (!TIFFWriteByteArray(tif, dir, (char*) bp))
				goto out;
		}
		break;
	case TIFF_SBYTE:
		{ 
			int8* bp = (int8*) w;
			for (i = 0; i < (int) n; i++)
				bp[i] = (int8) v[i];
			if (!TIFFWriteByteArray(tif, dir, (char*) bp))
				goto out;
		}
		break;
	case TIFF_SHORT:
		{
			uint16* bp = (uint16*) w;
			for (i = 0; i < (int) n; i++)
				bp[i] = (uint16) v[i];
			if (!TIFFWriteShortArray(tif, dir, (uint16*)bp))
				goto out;
		}
		break;
	case TIFF_SSHORT:
		{ 
			int16* bp = (int16*) w;
			for (i = 0; i < (int) n; i++)
				bp[i] = (int16) v[i];
			if (!TIFFWriteShortArray(tif, dir, (uint16*)bp))
				goto out;
		}
		break;
	case TIFF_LONG:
		{
			uint32* bp = (uint32*) w;
			for (i = 0; i < (int) n; i++)
				bp[i] = (uint32) v[i];
			if (!TIFFWriteLongArray(tif, dir, bp))
				goto out;
		}
		break;
	case TIFF_SLONG:
		{
			int32* bp = (int32*) w;
			for (i = 0; i < (int) n; i++)
				bp[i] = (int32) v[i];
			if (!TIFFWriteLongArray(tif, dir, (uint32*) bp))
				goto out;
		}
		break;
	case TIFF_FLOAT:
		{ 
			float* bp = (float*) w;
			for (i = 0; i < (int) n; i++)
				bp[i] = (float) v[i];
			if (!TIFFWriteFloatArray(tif, dir, bp))
				goto out;
		}
		break;
	case TIFF_DOUBLE:
		return (TIFFWriteDoubleArray(tif, dir, v));
	default:
		/* TIFF_NOTYPE */
		/* TIFF_ASCII */
		/* TIFF_UNDEFINED */
		/* TIFF_RATIONAL */
		/* TIFF_SRATIONAL */
		goto out;
	}
	status = 1;
 out:
	if (w != buf)
		_TIFFfree(w);
	return (status);
}

static int
TIFFWriteTransferFunction(TIFF* tif, TIFFDirEntry* dir)
{
	TIFFDirectory* td = &tif->tif_dir;
	tsize_t n = (1L<<td->td_bitspersample) * sizeof (uint16);
	uint16** tf = td->td_transferfunction;
	int ncols;

	/*
	 * Check if the table can be written as a single column,
	 * or if it must be written as 3 columns.  Note that we
	 * write a 3-column tag if there are 2 samples/pixel and
	 * a single column of data won't suffice--hmm.
	 */
	switch (td->td_samplesperpixel - td->td_extrasamples) {
	default:	if (_TIFFmemcmp(tf[0], tf[2], n)) { ncols = 3; break; }
	case 2:		if (_TIFFmemcmp(tf[0], tf[1], n)) { ncols = 3; break; }
	case 1: case 0:	ncols = 1;
	}
	return (TIFFWriteShortTable(tif,
	    TIFFTAG_TRANSFERFUNCTION, dir, ncols, tf));
}

static int
TIFFWriteInkNames(TIFF* tif, TIFFDirEntry* dir)
{
	TIFFDirectory* td = &tif->tif_dir;

	dir->tdir_tag = TIFFTAG_INKNAMES;
	dir->tdir_type = (short) TIFF_ASCII;
	dir->tdir_count = td->td_inknameslen;
	return (TIFFWriteByteArray(tif, dir, td->td_inknames));
}

/*
 * Write a contiguous directory item.
 */
static int
TIFFWriteData(TIFF* tif, TIFFDirEntry* dir, char* cp)
{
	tsize_t cc;

	if (tif->tif_flags & TIFF_SWAB) {
		switch (dir->tdir_type) {
		case TIFF_SHORT:
		case TIFF_SSHORT:
			TIFFSwabArrayOfShort((uint16*) cp, dir->tdir_count);  ddd
			break;
		case TIFF_LONG:
		case TIFF_SLONG:
		case TIFF_FLOAT:
			TIFFSwabArrayOfLong((uint32*) cp, dir->tdir_count);  ddd
			break;
		case TIFF_RATIONAL:
		case TIFF_SRATIONAL:
			TIFFSwabArrayOfLong((uint32*) cp, 2*dir->tdir_count);  ddd
			break;
		case TIFF_DOUBLE:
			TIFFSwabArrayOfDouble((double*) cp, dir->tdir_count);  ddd
			break;
		}
	}
	dir->tdir_offset = tif->tif_dataoff;
	cc = dir->tdir_count * TIFFDataWidth((TIFFDataType) dir->tdir_type);
	if (SeekOK(tif, dir->tdir_offset) &&
	    WriteOK(tif, cp, cc)) {
		tif->tif_dataoff += (cc + 1) & ~1;
		return (1);
	}
	TIFFErrorExt(tif->tif_clientdata, tif->tif_name,
		     "Error writing data for field \"%s\"",
	_TIFFFieldWithTag(tif, dir->tdir_tag)->field_name);
	return (0);
}

/*
 * Similar to TIFFWriteDirectory(), but if the directory has already
 * been written once, it is relocated to the end of the file, in case it
 * has changed in size.  Note that this will result in the loss of the 
 * previously used directory space. 
 */ 

int
TIFFRewriteDirectory( TIFF *tif )
{
	static const char module[] = "TIFFRewriteDirectory";

	/* We don't need to do anything special if it hasn't been written. */
	if( tif->tif_diroff == 0 )
		return TIFFWriteDirectory( tif );

	/*
	 * Find and zero the pointer to this directory, so that TIFFLinkDirectory
	 * will cause it to be added after this directories current pre-link.
	 */

	if (!(tif->tif_flags&TIFF_BIGTIFF))
	{
		/* Is it the first directory in the file? */
		if (tif->tif_header.classic.tiff_diroff == tif->tif_diroff)
		{
			tif->tif_header.classic.tiff_diroff = 0;
			tif->tif_diroff = 0;

			TIFFSeekFile(tif, (toff_t)(sizeof(TIFFHeaderClassic)-sizeof(uint32)),
			    SEEK_SET);
			if (!WriteOK(tif, &(tif->tif_header.classic.tiff_diroff),
			    sizeof (tif->tif_diroff)))
			{
				TIFFErrorExt(tif->tif_clientdata, tif->tif_name,
				    "Error updating TIFF header");
				return (0);
			}
		}
		else
		{
			toff_t  nextdir, off;

			nextdir = tif->tif_header.classic.tiff_diroff;
			do {
				uint16 dircount;

				if (!SeekOK(tif, nextdir) ||
				    !ReadOK(tif, &dircount, sizeof (dircount))) {
					TIFFErrorExt(tif->tif_clientdata, module,
					    "Error fetching directory count");
					return (0);
				}
				if (tif->tif_flags & TIFF_SWAB)
					TIFFSwabShort(&dircount);
				(void) TIFFSeekFile(tif,
				    dircount * sizeof (TIFFDirEntry), SEEK_CUR);
				if (!ReadOK(tif, &nextdir, sizeof (nextdir))) {
					TIFFErrorExt(tif->tif_clientdata, module,
					    "Error fetching directory link");
					return (0);
				}
				if (tif->tif_flags & TIFF_SWAB)
					TIFFSwabLong(&nextdir);
			} while (nextdir != tif->tif_diroff && nextdir != 0);
			off = TIFFSeekFile(tif, 0, SEEK_CUR); /* get current offset */
			(void) TIFFSeekFile(tif, off - (toff_t)sizeof(nextdir), SEEK_SET);
			tif->tif_diroff = 0;
			if (!WriteOK(tif, &(tif->tif_diroff), sizeof (nextdir))) {
				TIFFErrorExt(tif->tif_clientdata, module,
				    "Error writing directory link");
				return (0);
			}
		}
	}
	else
	{
		/* TODO */
		TIFFErrorExt(tif->tif_clientdata,module,"TIFFRewriteDirectory not implemented yet for BigTIFF");
		return(0);
	}

	/*
	 * Now use TIFFWriteDirectory() normally.
	 */

	return TIFFWriteDirectory( tif );
}


#endif

/* vim: set ts=8 sts=8 sw=8 noet: */
