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

#ifndef _TIFFDIR_
#define	_TIFFDIR_
/*
 * ``Library-private'' Directory-related Definitions.
 */

/*
 * Internal format of a TIFF directory entry.
 */
typedef	struct {
#define	FIELD_SETLONGS	4
	/* bit vector of fields that are set */
	unsigned long	td_fieldsset[FIELD_SETLONGS];

	uint32	td_imagewidth, td_imagelength, td_imagedepth;
	uint32	td_tilewidth, td_tilelength, td_tiledepth;
	uint32	td_subfiletype;
	uint16	td_bitspersample;
	uint16	td_sampleformat;
	uint16	td_compression;
	uint16	td_photometric;
	uint16	td_threshholding;
	uint16	td_fillorder;
	uint16	td_orientation;
	uint16	td_samplesperpixel;
	uint32	td_rowsperstrip;
	uint16	td_minsamplevalue, td_maxsamplevalue;
	double	td_sminsamplevalue, td_smaxsamplevalue;
	float	td_xresolution, td_yresolution;
	uint16	td_resolutionunit;
	uint16	td_planarconfig;
	float	td_xposition, td_yposition;
	uint16	td_pagenumber[2];
	uint16*	td_colormap[3];
	uint16	td_halftonehints[2];
	uint16	td_extrasamples;
	uint16*	td_sampleinfo;
	double	td_stonits;
	tstrip_t td_stripsperimage;
	tstrip_t td_nstrips;		/* size of offset & bytecount arrays */
	uint32*	td_stripoffset;
	uint32*	td_stripbytecount;
	int	td_stripbytecountsorted; /* is the bytecount array sorted ascending? */
	uint16	td_nsubifd;
	uint32*	td_subifd;
	/* YCbCr parameters */
	uint16	td_ycbcrsubsampling[2];
	uint16	td_ycbcrpositioning;
	/* Colorimetry parameters */
	float*	td_whitepoint;
	uint16*	td_transferfunction[3];
	/* CMYK parameters */
	uint16	td_inkset;
	uint16	td_ninks;
	uint16	td_dotrange[2];
	int	td_inknameslen;
	char*	td_inknames;
	/* ICC parameters */
	uint32	td_profileLength;
	void	*td_profileData;
	/* Adobe Photoshop tag handling */
	uint32	td_photoshopLength;
	void	*td_photoshopData;
	/* IPTC parameters */
	uint32	td_richtiffiptcLength;
	void	*td_richtiffiptcData;
	uint32	td_xmlpacketLength;
	void	*td_xmlpacketData;
	int     td_customValueCount;
        TIFFTagValue *td_customValues;
} TIFFDirectory;

/*
 * Field flags used to indicate fields that have
 * been set in a directory, and to reference fields
 * when manipulating a directory.
 */

/*
 * FIELD_IGNORE is used to signify tags that are to
 * be processed but otherwise ignored.  This permits
 * antiquated tags to be quietly read and discarded.
 * Note that a bit *is* allocated for ignored tags;
 * this is understood by the directory reading logic
 * which uses this fact to avoid special-case handling
 */ 
#define	FIELD_IGNORE			0

/* multi-item fields */
#define	FIELD_IMAGEDIMENSIONS		1
#define FIELD_TILEDIMENSIONS		2
#define	FIELD_RESOLUTION		3
#define	FIELD_POSITION			4

/* single-item fields */
#define	FIELD_SUBFILETYPE		5
#define	FIELD_BITSPERSAMPLE		6
#define	FIELD_COMPRESSION		7
#define	FIELD_PHOTOMETRIC		8
#define	FIELD_THRESHHOLDING		9
#define	FIELD_FILLORDER			10
/* unused - was FIELD_DOCUMENTNAME	11 */
/* unused - was FIELD_IMAGEDESCRIPTION	12 */
/* unused - was FIELD_MAKE		13 */
/* unused - was FIELD_MODEL		14 */
#define	FIELD_ORIENTATION		15
#define	FIELD_SAMPLESPERPIXEL		16
#define	FIELD_ROWSPERSTRIP		17
#define	FIELD_MINSAMPLEVALUE		18
#define	FIELD_MAXSAMPLEVALUE		19
#define	FIELD_PLANARCONFIG		20
/* unused - was FIELD_PAGENAME		21 */
#define	FIELD_RESOLUTIONUNIT		22
#define	FIELD_PAGENUMBER		23
#define	FIELD_STRIPBYTECOUNTS		24
#define	FIELD_STRIPOFFSETS		25
#define	FIELD_COLORMAP			26
/* unused - was FIELD_ARTIST		27 */
/* unused - was FIELD_DATETIME		28 */
/* unused - was FIELD_HOSTCOMPUTER	29 */
/* unused - was FIELD_SOFTWARE          30 */
#define	FIELD_EXTRASAMPLES		31
#define FIELD_SAMPLEFORMAT		32
#define	FIELD_SMINSAMPLEVALUE		33
#define	FIELD_SMAXSAMPLEVALUE		34
#define FIELD_IMAGEDEPTH		35
#define FIELD_TILEDEPTH			36
#define	FIELD_HALFTONEHINTS		37
/* unused - was FIELD_YCBCRCOEFFICIENTS	38 */
#define FIELD_YCBCRSUBSAMPLING		39
#define FIELD_YCBCRPOSITIONING		40
/* unused - was FIELD_REFBLACKWHITE	41 */
#define	FIELD_WHITEPOINT		42
/* unused - was FIELD_PRIMARYCHROMAS	43 */
#define	FIELD_TRANSFERFUNCTION		44
#define	FIELD_INKSET			45
#define	FIELD_INKNAMES			46
#define	FIELD_DOTRANGE			47
/* unused - was FIELD_TARGETPRINTER	48 */
#define	FIELD_SUBIFD			49
#define	FIELD_NUMBEROFINKS		50
#define FIELD_ICCPROFILE		51
#define FIELD_PHOTOSHOP			52
#define FIELD_RICHTIFFIPTC		53
#define FIELD_STONITS			54
/* unused - was FIELD_IMAGEFULLWIDTH	55 */
/* unused - was FIELD_IMAGEFULLLENGTH	56 */
/* unused - was FIELD_TEXTUREFORMAT	57 */
/* unused - was FIELD_WRAPMODES		58 */
/* unused - was FIELD_FOVCOT		59 */
/* unused - was FIELD_MATRIX_WORLDTOSCREEN	60 */
/* unused - was FIELD_MATRIX_WORLDTOCAMERA	61 */
/* unused - was FIELD_COPYRIGHT		62 */
#define FIELD_XMLPACKET			63
/*      FIELD_CUSTOM (see tiffio.h)     65 */
/* end of support for well-known tags; codec-private tags follow */
#define	FIELD_CODEC			66	/* base of codec-private tags */


/*
 * Pseudo-tags don't normally need field bits since they
 * are not written to an output file (by definition).
 * The library also has express logic to always query a
 * codec for a pseudo-tag so allocating a field bit for
 * one is a waste.   If codec wants to promote the notion
 * of a pseudo-tag being ``set'' or ``unset'' then it can
 * do using internal state flags without polluting the
 * field bit space defined for real tags.
 */
#define	FIELD_PSEUDO			0

#define	FIELD_LAST			(32*FIELD_SETLONGS-1)

/*
 * NB: NB: THIS ARRAY IS ASSUMED TO BE SORTED BY TAG.
 *       If a tag can have both LONG and SHORT types then the LONG must be
 *       placed before the SHORT for writing to work properly.
 *
 * NOTE: The second field (field_readcount) and third field (field_writecount)
 *       sometimes use the values TIFF_VARIABLE (-1), TIFF_VARIABLE2 (-3)
 *       and TIFFTAG_SPP (-2). The macros should be used but would throw off 
 *       the formatting of the code, so please interprete the -1, -2 and -3 
 *       values accordingly.
 */
static const TIFFFieldInfo tiffFieldInfo[] = {
    { TIFFTAG_SUBFILETYPE,	 1, 1,	TIFF_LONG,	FIELD_SUBFILETYPE,
      1,	0,	"SubfileType" },
/* XXX SHORT for compatibility w/ old versions of the library */
    { TIFFTAG_SUBFILETYPE,	 1, 1,	TIFF_SHORT,	FIELD_SUBFILETYPE,
      1,	0,	"SubfileType" },
    { TIFFTAG_OSUBFILETYPE,	 1, 1,	TIFF_SHORT,	FIELD_SUBFILETYPE,
      1,	0,	"OldSubfileType" },
    { TIFFTAG_IMAGEWIDTH,	 1, 1,	TIFF_LONG,	FIELD_IMAGEDIMENSIONS,
      0,	0,	"ImageWidth" },
    { TIFFTAG_IMAGEWIDTH,	 1, 1,	TIFF_SHORT,	FIELD_IMAGEDIMENSIONS,
      0,	0,	"ImageWidth" },
    { TIFFTAG_IMAGELENGTH,	 1, 1,	TIFF_LONG,	FIELD_IMAGEDIMENSIONS,
      1,	0,	"ImageLength" },
    { TIFFTAG_IMAGELENGTH,	 1, 1,	TIFF_SHORT,	FIELD_IMAGEDIMENSIONS,
      1,	0,	"ImageLength" },
    { TIFFTAG_BITSPERSAMPLE,	-1,-1,	TIFF_SHORT,	FIELD_BITSPERSAMPLE,
      0,	0,	"BitsPerSample" },
/* XXX LONG for compatibility with some broken TIFF writers */
    { TIFFTAG_BITSPERSAMPLE,	-1,-1,	TIFF_LONG,	FIELD_BITSPERSAMPLE,
      0,	0,	"BitsPerSample" },
    { TIFFTAG_COMPRESSION,	-1, 1,	TIFF_SHORT,	FIELD_COMPRESSION,
      0,	0,	"Compression" },
/* XXX LONG for compatibility with some broken TIFF writers */
    { TIFFTAG_COMPRESSION,	-1, 1,	TIFF_LONG,	FIELD_COMPRESSION,
      0,	0,	"Compression" },
    { TIFFTAG_PHOTOMETRIC,	 1, 1,	TIFF_SHORT,	FIELD_PHOTOMETRIC,
      0,	0,	"PhotometricInterpretation" },
/* XXX LONG for compatibility with some broken TIFF writers */
    { TIFFTAG_PHOTOMETRIC,	 1, 1,	TIFF_LONG,	FIELD_PHOTOMETRIC,
      0,	0,	"PhotometricInterpretation" },
    { TIFFTAG_THRESHHOLDING,	 1, 1,	TIFF_SHORT,	FIELD_THRESHHOLDING,
      1,	0,	"Threshholding" },
    { TIFFTAG_CELLWIDTH,	 1, 1,	TIFF_SHORT,	FIELD_IGNORE,
      1,	0,	"CellWidth" },
    { TIFFTAG_CELLLENGTH,	 1, 1,	TIFF_SHORT,	FIELD_IGNORE,
      1,	0,	"CellLength" },
    { TIFFTAG_FILLORDER,	 1, 1,	TIFF_SHORT,	FIELD_FILLORDER,
      0,	0,	"FillOrder" },
    { TIFFTAG_DOCUMENTNAME,	-1,-1,	TIFF_ASCII,	FIELD_CUSTOM,
      1,	0,	"DocumentName" },
    { TIFFTAG_IMAGEDESCRIPTION,	-1,-1,	TIFF_ASCII,	FIELD_CUSTOM,
      1,	0,	"ImageDescription" },
    { TIFFTAG_MAKE,		-1,-1,	TIFF_ASCII,	FIELD_CUSTOM,
      1,	0,	"Make" },
    { TIFFTAG_MODEL,		-1,-1,	TIFF_ASCII,	FIELD_CUSTOM,
      1,	0,	"Model" },
    { TIFFTAG_STRIPOFFSETS,	-1,-1,	TIFF_LONG,	FIELD_STRIPOFFSETS,
      0,	0,	"StripOffsets" },
    { TIFFTAG_STRIPOFFSETS,	-1,-1,	TIFF_SHORT,	FIELD_STRIPOFFSETS,
      0,	0,	"StripOffsets" },
    { TIFFTAG_ORIENTATION,	 1, 1,	TIFF_SHORT,	FIELD_ORIENTATION,
      0,	0,	"Orientation" },
    { TIFFTAG_SAMPLESPERPIXEL,	 1, 1,	TIFF_SHORT,	FIELD_SAMPLESPERPIXEL,
      0,	0,	"SamplesPerPixel" },
    { TIFFTAG_ROWSPERSTRIP,	 1, 1,	TIFF_LONG,	FIELD_ROWSPERSTRIP,
      0,	0,	"RowsPerStrip" },
    { TIFFTAG_ROWSPERSTRIP,	 1, 1,	TIFF_SHORT,	FIELD_ROWSPERSTRIP,
      0,	0,	"RowsPerStrip" },
    { TIFFTAG_STRIPBYTECOUNTS,	-1,-1,	TIFF_LONG,	FIELD_STRIPBYTECOUNTS,
      0,	0,	"StripByteCounts" },
    { TIFFTAG_STRIPBYTECOUNTS,	-1,-1,	TIFF_SHORT,	FIELD_STRIPBYTECOUNTS,
      0,	0,	"StripByteCounts" },
    { TIFFTAG_MINSAMPLEVALUE,	-2,-1,	TIFF_SHORT,	FIELD_MINSAMPLEVALUE,
      1,	0,	"MinSampleValue" },
    { TIFFTAG_MAXSAMPLEVALUE,	-2,-1,	TIFF_SHORT,	FIELD_MAXSAMPLEVALUE,
      1,	0,	"MaxSampleValue" },
    { TIFFTAG_XRESOLUTION,	 1, 1,	TIFF_RATIONAL,	FIELD_RESOLUTION,
      1,	0,	"XResolution" },
    { TIFFTAG_YRESOLUTION,	 1, 1,	TIFF_RATIONAL,	FIELD_RESOLUTION,
      1,	0,	"YResolution" },
    { TIFFTAG_PLANARCONFIG,	 1, 1,	TIFF_SHORT,	FIELD_PLANARCONFIG,
      0,	0,	"PlanarConfiguration" },
    { TIFFTAG_PAGENAME,		-1,-1,	TIFF_ASCII,	FIELD_CUSTOM,
      1,	0,	"PageName" },
    { TIFFTAG_XPOSITION,	 1, 1,	TIFF_RATIONAL,	FIELD_POSITION,
      1,	0,	"XPosition" },
    { TIFFTAG_YPOSITION,	 1, 1,	TIFF_RATIONAL,	FIELD_POSITION,
      1,	0,	"YPosition" },
    { TIFFTAG_FREEOFFSETS,	-1,-1,	TIFF_LONG,	FIELD_IGNORE,
      0,	0,	"FreeOffsets" },
    { TIFFTAG_FREEBYTECOUNTS,	-1,-1,	TIFF_LONG,	FIELD_IGNORE,
      0,	0,	"FreeByteCounts" },
    { TIFFTAG_GRAYRESPONSEUNIT,	 1, 1,	TIFF_SHORT,	FIELD_IGNORE,
      1,	0,	"GrayResponseUnit" },
    { TIFFTAG_GRAYRESPONSECURVE,-1,-1,	TIFF_SHORT,	FIELD_IGNORE,
      1,	0,	"GrayResponseCurve" },
    { TIFFTAG_RESOLUTIONUNIT,	 1, 1,	TIFF_SHORT,	FIELD_RESOLUTIONUNIT,
      1,	0,	"ResolutionUnit" },
    { TIFFTAG_PAGENUMBER,	 2, 2,	TIFF_SHORT,	FIELD_PAGENUMBER,
      1,	0,	"PageNumber" },
    { TIFFTAG_COLORRESPONSEUNIT, 1, 1,	TIFF_SHORT,	FIELD_IGNORE,
      1,	0,	"ColorResponseUnit" },
    { TIFFTAG_TRANSFERFUNCTION,	-1,-1,	TIFF_SHORT,	FIELD_TRANSFERFUNCTION,
      1,	0,	"TransferFunction" },
    { TIFFTAG_SOFTWARE,		-1,-1,	TIFF_ASCII,	FIELD_CUSTOM,
      1,	0,	"Software" },
    { TIFFTAG_DATETIME,		20,20,	TIFF_ASCII,	FIELD_CUSTOM,
      1,	0,	"DateTime" },
    { TIFFTAG_ARTIST,		-1,-1,	TIFF_ASCII,	FIELD_CUSTOM,
      1,	0,	"Artist" },
    { TIFFTAG_HOSTCOMPUTER,	-1,-1,	TIFF_ASCII,	FIELD_CUSTOM,
      1,	0,	"HostComputer" },
    { TIFFTAG_WHITEPOINT,	 2, 2,	TIFF_RATIONAL,	FIELD_WHITEPOINT,
      1,	0,	"WhitePoint" },
    { TIFFTAG_PRIMARYCHROMATICITIES,6,6,TIFF_RATIONAL,	FIELD_CUSTOM,
      1,	0,	"PrimaryChromaticities" },
    { TIFFTAG_COLORMAP,		-1,-1,	TIFF_SHORT,	FIELD_COLORMAP,
      1,	0,	"ColorMap" },
    { TIFFTAG_HALFTONEHINTS,	 2, 2,	TIFF_SHORT,	FIELD_HALFTONEHINTS,
      1,	0,	"HalftoneHints" },
    { TIFFTAG_TILEWIDTH,	 1, 1,	TIFF_LONG,	FIELD_TILEDIMENSIONS,
      0,	0,	"TileWidth" },
    { TIFFTAG_TILEWIDTH,	 1, 1,	TIFF_SHORT,	FIELD_TILEDIMENSIONS,
      0,	0,	"TileWidth" },
    { TIFFTAG_TILELENGTH,	 1, 1,	TIFF_LONG,	FIELD_TILEDIMENSIONS,
      0,	0,	"TileLength" },
    { TIFFTAG_TILELENGTH,	 1, 1,	TIFF_SHORT,	FIELD_TILEDIMENSIONS,
      0,	0,	"TileLength" },
    { TIFFTAG_TILEOFFSETS,	-1, 1,	TIFF_LONG,	FIELD_STRIPOFFSETS,
      0,	0,	"TileOffsets" },
    { TIFFTAG_TILEBYTECOUNTS,	-1, 1,	TIFF_LONG,	FIELD_STRIPBYTECOUNTS,
      0,	0,	"TileByteCounts" },
    { TIFFTAG_TILEBYTECOUNTS,	-1, 1,	TIFF_SHORT,	FIELD_STRIPBYTECOUNTS,
      0,	0,	"TileByteCounts" },
    { TIFFTAG_SUBIFD,		-1,-1,	TIFF_IFD,	FIELD_SUBIFD,
      1,	1,	"SubIFD" },
    { TIFFTAG_SUBIFD,		-1,-1,	TIFF_LONG,	FIELD_SUBIFD,
      1,	1,	"SubIFD" },
    { TIFFTAG_INKSET,		 1, 1,	TIFF_SHORT,	FIELD_INKSET,
      0,	0,	"InkSet" },
    { TIFFTAG_INKNAMES,		-1,-1,	TIFF_ASCII,	FIELD_INKNAMES,
      1,	1,	"InkNames" },
    { TIFFTAG_NUMBEROFINKS,	 1, 1,	TIFF_SHORT,	FIELD_NUMBEROFINKS,
      1,	0,	"NumberOfInks" },
    { TIFFTAG_DOTRANGE,		 2, 2,	TIFF_SHORT,	FIELD_DOTRANGE,
      0,	0,	"DotRange" },
    { TIFFTAG_DOTRANGE,		 2, 2,	TIFF_BYTE,	FIELD_DOTRANGE,
      0,	0,	"DotRange" },
    { TIFFTAG_TARGETPRINTER,	-1,-1,	TIFF_ASCII,	FIELD_CUSTOM,
      1,	0,	"TargetPrinter" },
    { TIFFTAG_EXTRASAMPLES,	-1,-1,	TIFF_SHORT,	FIELD_EXTRASAMPLES,
      0,	1,	"ExtraSamples" },
/* XXX for bogus Adobe Photoshop v2.5 files */
    { TIFFTAG_EXTRASAMPLES,	-1,-1,	TIFF_BYTE,	FIELD_EXTRASAMPLES,
      0,	1,	"ExtraSamples" },
    { TIFFTAG_SAMPLEFORMAT,	-1,-1,	TIFF_SHORT,	FIELD_SAMPLEFORMAT,
      0,	0,	"SampleFormat" },
    { TIFFTAG_SMINSAMPLEVALUE,	-2,-1,	TIFF_ANY,	FIELD_SMINSAMPLEVALUE,
      1,	0,	"SMinSampleValue" },
    { TIFFTAG_SMAXSAMPLEVALUE,	-2,-1,	TIFF_ANY,	FIELD_SMAXSAMPLEVALUE,
      1,	0,	"SMaxSampleValue" },
    { TIFFTAG_CLIPPATH,		-1, -3, TIFF_BYTE,	FIELD_CUSTOM,
      0,	1,	"ClipPath" },
    { TIFFTAG_XCLIPPATHUNITS,	 1, 1,	TIFF_SLONG,	FIELD_CUSTOM,
      0,	0,	"XClipPathUnits" },
    { TIFFTAG_XCLIPPATHUNITS,	 1, 1,	TIFF_SSHORT,	FIELD_CUSTOM,
      0,	0,	"XClipPathUnits" },
    { TIFFTAG_XCLIPPATHUNITS,	 1, 1,	TIFF_SBYTE,	FIELD_CUSTOM,
      0,	0,	"XClipPathUnits" },
    { TIFFTAG_YCLIPPATHUNITS,	 1, 1,	TIFF_SLONG,	FIELD_CUSTOM,
      0,	0,	"YClipPathUnits" },
    { TIFFTAG_YCLIPPATHUNITS,	 1, 1,	TIFF_SSHORT,	FIELD_CUSTOM,
      0,	0,	"YClipPathUnits" },
    { TIFFTAG_YCLIPPATHUNITS,	 1, 1,	TIFF_SBYTE,	FIELD_CUSTOM,
      0,	0,	"YClipPathUnits" },
    { TIFFTAG_YCBCRCOEFFICIENTS, 3, 3,	TIFF_RATIONAL,	FIELD_CUSTOM,
      0,	0,	"YCbCrCoefficients" },
    { TIFFTAG_YCBCRSUBSAMPLING,	 2, 2,	TIFF_SHORT,	FIELD_YCBCRSUBSAMPLING,
      0,	0,	"YCbCrSubsampling" },
    { TIFFTAG_YCBCRPOSITIONING,	 1, 1,	TIFF_SHORT,	FIELD_YCBCRPOSITIONING,
      0,	0,	"YCbCrPositioning" },
    { TIFFTAG_REFERENCEBLACKWHITE, 6, 6, TIFF_RATIONAL,	FIELD_CUSTOM,
      1,	0,	"ReferenceBlackWhite" },
/* XXX temporarily accept LONG for backwards compatibility */
    { TIFFTAG_REFERENCEBLACKWHITE, 6, 6, TIFF_LONG,	FIELD_CUSTOM,
      1,	0,	"ReferenceBlackWhite" },
    { TIFFTAG_XMLPACKET,	-1,-3,	TIFF_BYTE,	FIELD_XMLPACKET,
      0,	1,	"XMLPacket" },
/* begin SGI tags */
    { TIFFTAG_MATTEING,		 1, 1,	TIFF_SHORT,	FIELD_EXTRASAMPLES,
      0,	0,	"Matteing" },
    { TIFFTAG_DATATYPE,		-2,-1,	TIFF_SHORT,	FIELD_SAMPLEFORMAT,
      0,	0,	"DataType" },
    { TIFFTAG_IMAGEDEPTH,	 1, 1,	TIFF_LONG,	FIELD_IMAGEDEPTH,
      0,	0,	"ImageDepth" },
    { TIFFTAG_IMAGEDEPTH,	 1, 1,	TIFF_SHORT,	FIELD_IMAGEDEPTH,
      0,	0,	"ImageDepth" },
    { TIFFTAG_TILEDEPTH,	 1, 1,	TIFF_LONG,	FIELD_TILEDEPTH,
      0,	0,	"TileDepth" },
    { TIFFTAG_TILEDEPTH,	 1, 1,	TIFF_SHORT,	FIELD_TILEDEPTH,
      0,	0,	"TileDepth" },
/* end SGI tags */
/* begin Pixar tags */
    { TIFFTAG_PIXAR_IMAGEFULLWIDTH,  1, 1, TIFF_LONG,	FIELD_CUSTOM,
      1,	0,	"ImageFullWidth" },
    { TIFFTAG_PIXAR_IMAGEFULLLENGTH, 1, 1, TIFF_LONG,	FIELD_CUSTOM,
      1,	0,	"ImageFullLength" },
    { TIFFTAG_PIXAR_TEXTUREFORMAT,  -1, -1, TIFF_ASCII,	FIELD_CUSTOM,
      1,	0,	"TextureFormat" },
    { TIFFTAG_PIXAR_WRAPMODES,	    -1, -1, TIFF_ASCII,	FIELD_CUSTOM,
      1,	0,	"TextureWrapModes" },
    { TIFFTAG_PIXAR_FOVCOT,	     1, 1, TIFF_FLOAT,	FIELD_CUSTOM,
      1,	0,	"FieldOfViewCotangent" },
    { TIFFTAG_PIXAR_MATRIX_WORLDTOSCREEN,	16,16,	TIFF_FLOAT,
      FIELD_CUSTOM,	1,	0,	"MatrixWorldToScreen" },
    { TIFFTAG_PIXAR_MATRIX_WORLDTOCAMERA,	16,16,	TIFF_FLOAT,
       FIELD_CUSTOM,	1,	0,	"MatrixWorldToCamera" },
    { TIFFTAG_COPYRIGHT,	-1, -1,	TIFF_ASCII,	FIELD_CUSTOM,
      1,	0,	"Copyright" },
/* end Pixar tags */
    { TIFFTAG_RICHTIFFIPTC, -1, -3,	TIFF_LONG,   FIELD_RICHTIFFIPTC, 
      0,    1,   "RichTIFFIPTC" },
    { TIFFTAG_PHOTOSHOP,    -1, -3,	TIFF_BYTE,   FIELD_PHOTOSHOP, 
      0,    1,   "Photoshop" },
    { TIFFTAG_EXIFIFD,		1, 1,	TIFF_LONG,	FIELD_CUSTOM,
      0,	0,	"EXIFIFDOffset" },
    { TIFFTAG_ICCPROFILE,	-1, -3,	TIFF_UNDEFINED,	FIELD_ICCPROFILE,
      0,	1,	"ICC Profile" },
    { TIFFTAG_GPSIFD,		1, 1,	TIFF_LONG,	FIELD_CUSTOM,
      0,	0,	"GPSIFDOffset" },
    { TIFFTAG_STONITS,		 1, 1,	TIFF_DOUBLE,	FIELD_STONITS,
      0,	0,	"StoNits" },
    { TIFFTAG_INTEROPERABILITYIFD,	1, 1,	TIFF_LONG,	FIELD_CUSTOM,
      0,	0,	"InteroperabilityIFDOffset" },
/* begin DNG tags */
    { TIFFTAG_DNGVERSION,	4, 4,	TIFF_BYTE,	FIELD_CUSTOM, 
      0,	0,	"DNGVersion" },
    { TIFFTAG_DNGBACKWARDVERSION,	4, 4,	TIFF_BYTE,	FIELD_CUSTOM, 
      0,	0,	"DNGBackwardVersion" },
    { TIFFTAG_UNIQUECAMERAMODEL,    -1, -1, TIFF_ASCII,	FIELD_CUSTOM,
      1,	0,	"UniqueCameraModel" },
    { TIFFTAG_LOCALIZEDCAMERAMODEL,    -1, -1, TIFF_ASCII,	FIELD_CUSTOM,
      1,	0,	"LocalizedCameraModel" },
    { TIFFTAG_LOCALIZEDCAMERAMODEL,    -1, -1, TIFF_BYTE,	FIELD_CUSTOM,
      1,	1,	"LocalizedCameraModel" },
    { TIFFTAG_CFAPLANECOLOR,	-1, -1,	TIFF_BYTE,	FIELD_CUSTOM, 
      0,	1,	"CFAPlaneColor" },
    { TIFFTAG_CFALAYOUT,	1, 1,	TIFF_SHORT,	FIELD_CUSTOM, 
      0,	0,	"CFALayout" },
    { TIFFTAG_LINEARIZATIONTABLE,	-1, -1,	TIFF_SHORT,	FIELD_CUSTOM, 
      0,	1,	"LinearizationTable" },
    { TIFFTAG_BLACKLEVELREPEATDIM,	2, 2,	TIFF_SHORT,	FIELD_CUSTOM, 
      0,	0,	"BlackLevelRepeatDim" },
    { TIFFTAG_BLACKLEVEL,	-1, -1,	TIFF_LONG,	FIELD_CUSTOM, 
      0,	1,	"BlackLevel" },
    { TIFFTAG_BLACKLEVEL,	-1, -1,	TIFF_SHORT,	FIELD_CUSTOM, 
      0,	1,	"BlackLevel" },
    { TIFFTAG_BLACKLEVEL,	-1, -1,	TIFF_RATIONAL,	FIELD_CUSTOM, 
      0,	1,	"BlackLevel" },
    { TIFFTAG_BLACKLEVELDELTAH,	-1, -1,	TIFF_SRATIONAL,	FIELD_CUSTOM, 
      0,	1,	"BlackLevelDeltaH" },
    { TIFFTAG_BLACKLEVELDELTAV,	-1, -1,	TIFF_SRATIONAL,	FIELD_CUSTOM, 
      0,	1,	"BlackLevelDeltaV" },
    { TIFFTAG_WHITELEVEL,	-2, -2,	TIFF_LONG,	FIELD_CUSTOM, 
      0,	0,	"WhiteLevel" },
    { TIFFTAG_WHITELEVEL,	-2, -2,	TIFF_SHORT,	FIELD_CUSTOM, 
      0,	0,	"WhiteLevel" },
    { TIFFTAG_DEFAULTSCALE,	2, 2,	TIFF_RATIONAL,	FIELD_CUSTOM, 
      0,	0,	"DefaultScale" },
    { TIFFTAG_BESTQUALITYSCALE,	1, 1,	TIFF_RATIONAL,	FIELD_CUSTOM, 
      0,	0,	"BestQualityScale" },
    { TIFFTAG_DEFAULTCROPORIGIN,	2, 2,	TIFF_LONG,	FIELD_CUSTOM, 
      0,	0,	"DefaultCropOrigin" },
    { TIFFTAG_DEFAULTCROPORIGIN,	2, 2,	TIFF_SHORT,	FIELD_CUSTOM, 
      0,	0,	"DefaultCropOrigin" },
    { TIFFTAG_DEFAULTCROPORIGIN,	2, 2,	TIFF_RATIONAL,	FIELD_CUSTOM, 
      0,	0,	"DefaultCropOrigin" },
    { TIFFTAG_DEFAULTCROPSIZE,	2, 2,	TIFF_LONG,	FIELD_CUSTOM, 
      0,	0,	"DefaultCropSize" },
    { TIFFTAG_DEFAULTCROPSIZE,	2, 2,	TIFF_SHORT,	FIELD_CUSTOM, 
      0,	0,	"DefaultCropSize" },
    { TIFFTAG_DEFAULTCROPSIZE,	2, 2,	TIFF_RATIONAL,	FIELD_CUSTOM, 
      0,	0,	"DefaultCropSize" },
    { TIFFTAG_COLORMATRIX1,	-1, -1,	TIFF_SRATIONAL,	FIELD_CUSTOM, 
      0,	1,	"ColorMatrix1" },
    { TIFFTAG_COLORMATRIX2,	-1, -1,	TIFF_SRATIONAL,	FIELD_CUSTOM, 
      0,	1,	"ColorMatrix2" },
    { TIFFTAG_CAMERACALIBRATION1,	-1, -1,	TIFF_SRATIONAL,	FIELD_CUSTOM, 
      0,	1,	"CameraCalibration1" },
    { TIFFTAG_CAMERACALIBRATION2,	-1, -1,	TIFF_SRATIONAL,	FIELD_CUSTOM, 
      0,	1,	"CameraCalibration2" },
    { TIFFTAG_REDUCTIONMATRIX1,	-1, -1,	TIFF_SRATIONAL,	FIELD_CUSTOM, 
      0,	1,	"ReductionMatrix1" },
    { TIFFTAG_REDUCTIONMATRIX2,	-1, -1,	TIFF_SRATIONAL,	FIELD_CUSTOM, 
      0,	1,	"ReductionMatrix2" },
    { TIFFTAG_ANALOGBALANCE,	-1, -1,	TIFF_RATIONAL,	FIELD_CUSTOM, 
      0,	1,	"AnalogBalance" },
    { TIFFTAG_ASSHOTNEUTRAL,	-1, -1,	TIFF_SHORT,	FIELD_CUSTOM, 
      0,	1,	"AsShotNeutral" },
    { TIFFTAG_ASSHOTNEUTRAL,	-1, -1,	TIFF_RATIONAL,	FIELD_CUSTOM, 
      0,	1,	"AsShotNeutral" },
    { TIFFTAG_ASSHOTWHITEXY,	2, 2,	TIFF_RATIONAL,	FIELD_CUSTOM, 
      0,	0,	"AsShotWhiteXY" },
    { TIFFTAG_BASELINEEXPOSURE,	1, 1,	TIFF_SRATIONAL,	FIELD_CUSTOM, 
      0,	0,	"BaselineExposure" },
    { TIFFTAG_BASELINENOISE,	1, 1,	TIFF_RATIONAL,	FIELD_CUSTOM, 
      0,	0,	"BaselineNoise" },
    { TIFFTAG_BASELINESHARPNESS,	1, 1,	TIFF_RATIONAL,	FIELD_CUSTOM, 
      0,	0,	"BaselineSharpness" },
    { TIFFTAG_BAYERGREENSPLIT,	1, 1,	TIFF_LONG,	FIELD_CUSTOM, 
      0,	0,	"BayerGreenSplit" },
    { TIFFTAG_LINEARRESPONSELIMIT,	1, 1,	TIFF_RATIONAL,	FIELD_CUSTOM, 
      0,	0,	"LinearResponseLimit" },
    { TIFFTAG_CAMERASERIALNUMBER,    -1, -1, TIFF_ASCII,	FIELD_CUSTOM,
      1,	0,	"CameraSerialNumber" },
    { TIFFTAG_LENSINFO,	4, 4,	TIFF_RATIONAL,	FIELD_CUSTOM, 
      0,	0,	"LensInfo" },
    { TIFFTAG_CHROMABLURRADIUS,	1, 1,	TIFF_RATIONAL,	FIELD_CUSTOM, 
      0,	0,	"ChromaBlurRadius" },
    { TIFFTAG_ANTIALIASSTRENGTH,	1, 1,	TIFF_RATIONAL,	FIELD_CUSTOM, 
      0,	0,	"AntiAliasStrength" },
    { TIFFTAG_SHADOWSCALE,	1, 1,	TIFF_RATIONAL,	FIELD_CUSTOM, 
      0,	0,	"ShadowScale" },
    { TIFFTAG_DNGPRIVATEDATA,    -1, -1, TIFF_BYTE,	FIELD_CUSTOM,
      0,	1,	"DNGPrivateData" },
    { TIFFTAG_MAKERNOTESAFETY,	1, 1,	TIFF_SHORT,	FIELD_CUSTOM, 
      0,	0,	"MakerNoteSafety" },
    { TIFFTAG_CALIBRATIONILLUMINANT1,	1, 1,	TIFF_SHORT,	FIELD_CUSTOM, 
      0,	0,	"CalibrationIlluminant1" },
    { TIFFTAG_CALIBRATIONILLUMINANT2,	1, 1,	TIFF_SHORT,	FIELD_CUSTOM, 
      0,	0,	"CalibrationIlluminant2" },
    { TIFFTAG_RAWDATAUNIQUEID,	16, 16,	TIFF_BYTE,	FIELD_CUSTOM, 
      0,	0,	"RawDataUniqueID" },
    { TIFFTAG_ORIGINALRAWFILENAME,    -1, -1, TIFF_ASCII,	FIELD_CUSTOM,
      1,	0,	"OriginalRawFileName" },
    { TIFFTAG_ORIGINALRAWFILENAME,    -1, -1, TIFF_BYTE,	FIELD_CUSTOM,
      1,	1,	"OriginalRawFileName" },
    { TIFFTAG_ORIGINALRAWFILEDATA,    -1, -1, TIFF_UNDEFINED,	FIELD_CUSTOM,
      0,	1,	"OriginalRawFileData" },
    { TIFFTAG_ACTIVEAREA,	4, 4,	TIFF_LONG,	FIELD_CUSTOM, 
      0,	0,	"ActiveArea" },
    { TIFFTAG_ACTIVEAREA,	4, 4,	TIFF_SHORT,	FIELD_CUSTOM, 
      0,	0,	"ActiveArea" },
    { TIFFTAG_MASKEDAREAS,	-1, -1,	TIFF_LONG,	FIELD_CUSTOM, 
      0,	1,	"MaskedAreas" },
    { TIFFTAG_ASSHOTICCPROFILE,    -1, -1, TIFF_UNDEFINED,	FIELD_CUSTOM,
      0,	1,	"AsShotICCProfile" },
    { TIFFTAG_ASSHOTPREPROFILEMATRIX,	-1, -1,	TIFF_SRATIONAL,	FIELD_CUSTOM, 
      0,	1,	"AsShotPreProfileMatrix" },
    { TIFFTAG_CURRENTICCPROFILE,    -1, -1, TIFF_UNDEFINED,	FIELD_CUSTOM,
      0,	1,	"CurrentICCProfile" },
    { TIFFTAG_CURRENTPREPROFILEMATRIX,	-1, -1,	TIFF_SRATIONAL,	FIELD_CUSTOM, 
      0,	1,	"CurrentPreProfileMatrix" },
/* end DNG tags */
};

static const TIFFFieldInfo exifFieldInfo[] = {
    { EXIFTAG_EXIFVERSION,	4, 4,		TIFF_UNDEFINED,	FIELD_CUSTOM,
      1,	0,	"ExifVersion" },
    { EXIFTAG_COMPONENTSCONFIGURATION,	 4, 4,	TIFF_UNDEFINED,	FIELD_CUSTOM,
      1,	0,	"ComponentsConfiguration" },
    { EXIFTAG_COMPRESSEDBITSPERPIXEL,	 1, 1,	TIFF_RATIONAL,	FIELD_CUSTOM,
      1,	0,	"CompressedBitsPerPixel" },
    { EXIFTAG_MAKERNOTE,	-1, -1,		TIFF_UNDEFINED,	FIELD_CUSTOM,
      1,	1,	"MakerNote" },
    { EXIFTAG_USERCOMMENT,	-1, -1,		TIFF_UNDEFINED,	FIELD_CUSTOM,
      1,	1,	"UserComment" },
    { EXIFTAG_DATETIMEORIGINAL,	20,20,		TIFF_ASCII,	FIELD_CUSTOM,
      1,	0,	"DateTimeOriginal" },
    { EXIFTAG_FLASHPIXVERSION,	4, 4,		TIFF_UNDEFINED,	FIELD_CUSTOM,
      1,	0,	"FlashpixVersion" },
    { EXIFTAG_PIXELXDIMENSION,	1, 1,		TIFF_LONG,	FIELD_CUSTOM,
      1,	0,	"PixelXDimension" },
    { EXIFTAG_PIXELXDIMENSION,	1, 1,		TIFF_SHORT,	FIELD_CUSTOM,
      1,	0,	"PixelXDimension" },
    { EXIFTAG_PIXELYDIMENSION,	1, 1,		TIFF_LONG,	FIELD_CUSTOM,
      1,	0,	"PixelYDimension" },
    { EXIFTAG_PIXELYDIMENSION,	1, 1,		TIFF_SHORT,	FIELD_CUSTOM,
      1,	0,	"PixelYDimension" },
    { EXIFTAG_RELATEDSOUNDFILE,	13, 13,		TIFF_ASCII,	FIELD_CUSTOM,
      1,	0,	"RelatedSoundFile" },
};

#define	TIFFExtractData(tif, type, v) \
    ((uint32) ((tif)->tif_header.tiff_magic == TIFF_BIGENDIAN ? \
        ((v) >> (tif)->tif_typeshift[type]) & (tif)->tif_typemask[type] : \
	(v) & (tif)->tif_typemask[type]))
#define	TIFFInsertData(tif, type, v) \
    ((uint32) ((tif)->tif_header.tiff_magic == TIFF_BIGENDIAN ? \
        ((v) & (tif)->tif_typemask[type]) << (tif)->tif_typeshift[type] : \
	(v) & (tif)->tif_typemask[type]))


#define BITn(n)				(((unsigned long)1L)<<((n)&0x1f)) 
#define BITFIELDn(tif, n)		((tif)->tif_dir.td_fieldsset[(n)/32]) 
#define TIFFFieldSet(tif, field)	(BITFIELDn(tif, field) & BITn(field)) 
#define TIFFSetFieldBit(tif, field)	(BITFIELDn(tif, field) |= BITn(field))
#define TIFFClrFieldBit(tif, field)	(BITFIELDn(tif, field) &= ~BITn(field))

#define	FieldSet(fields, f)		(fields[(f)/32] & BITn(f))
#define	ResetFieldBit(fields, f)	(fields[(f)/32] &= ~BITn(f))

#if defined(__cplusplus)
extern "C" {
#endif
extern	void _TIFFSetupFieldInfo(TIFF*, const TIFFFieldInfo[], int);
extern	void _TIFFPrintFieldInfo(TIFF*, FILE*);
extern	TIFFDataType _TIFFSampleToTagType(TIFF*);
extern  const TIFFFieldInfo* _TIFFFindOrRegisterFieldInfo( TIFF *tif,
							   ttag_t tag,
							   TIFFDataType dt );
extern  TIFFFieldInfo* _TIFFCreateAnonFieldInfo( TIFF *tif, ttag_t tag,
                                                 TIFFDataType dt );

#define _TIFFMergeFieldInfo	    TIFFMergeFieldInfo
#define _TIFFFindFieldInfo	    TIFFFindFieldInfo
#define _TIFFFindFieldInfoByName    TIFFFindFieldInfoByName
#define _TIFFFieldWithTag	    TIFFFieldWithTag
#define _TIFFFieldWithName	    TIFFFieldWithName

#if defined(__cplusplus)
}
#endif
#endif /* _TIFFDIR_ */

/* vim: set ts=8 sts=8 sw=8 noet: */
