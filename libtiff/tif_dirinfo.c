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
 * Core Directory Tag Support.
 */
#include "tiffiop.h"
#include <stdlib.h>

void
_TIFFSetupFieldInfo(TIFF* tif, const TIFFFieldInfo info[], int n)
{
	if (tif->tif_fieldinfo) {
		size_t  i;

		for (i = 0; i < tif->tif_nfields; i++) 
		{
			TIFFFieldInfo *fld = tif->tif_fieldinfo[i];
			if (fld->field_bit == FIELD_CUSTOM && 
				strncmp("Tag ", fld->field_name, 4) == 0) {
					_TIFFfree(fld->field_name);
					_TIFFfree(fld);
				}
		}   
      
		_TIFFfree(tif->tif_fieldinfo);
		tif->tif_nfields = 0;
	}
	_TIFFMergeFieldInfo(tif, info, n);
}

static int
tagCompare(const void* a, const void* b)
{
	const TIFFFieldInfo* ta = *(const TIFFFieldInfo**) a;
	const TIFFFieldInfo* tb = *(const TIFFFieldInfo**) b;
	/* NB: be careful of return values for 16-bit platforms */
	if (ta->field_tag != tb->field_tag)
		return (ta->field_tag < tb->field_tag ? -1 : 1);
	else
		return ((int)tb->field_type - (int)ta->field_type);
}

static int
tagNameCompare(const void* a, const void* b)
{
	const TIFFFieldInfo* ta = *(const TIFFFieldInfo**) a;
	const TIFFFieldInfo* tb = *(const TIFFFieldInfo**) b;

        return strcmp(ta->field_name, tb->field_name);
}

void
_TIFFMergeFieldInfo(TIFF* tif, const TIFFFieldInfo info[], int n)
{
	TIFFFieldInfo** tp;
	int i;

        tif->tif_foundfield = NULL;

	if (tif->tif_nfields > 0) {
		tif->tif_fieldinfo = (TIFFFieldInfo**)
		    _TIFFrealloc(tif->tif_fieldinfo,
			(tif->tif_nfields+n) * sizeof (TIFFFieldInfo*));
	} else {
		tif->tif_fieldinfo = (TIFFFieldInfo**)
		    _TIFFmalloc(n * sizeof (TIFFFieldInfo*));
	}
	assert(tif->tif_fieldinfo != NULL);
	tp = tif->tif_fieldinfo + tif->tif_nfields;
	for (i = 0; i < n; i++)
		*tp++ = (TIFFFieldInfo*) (info + i);	/* XXX */

        /* Sort the field info by tag number */
        qsort(tif->tif_fieldinfo, tif->tif_nfields += n,
	      sizeof (TIFFFieldInfo*), tagCompare);
}

void
_TIFFPrintFieldInfo(TIFF* tif, FILE* fd)
{
	size_t i;

	fprintf(fd, "%s: \n", tif->tif_name);
	for (i = 0; i < tif->tif_nfields; i++) {
		const TIFFFieldInfo* fip = tif->tif_fieldinfo[i];
		fprintf(fd, "field[%2d] %5lu, %2d, %2d, %d, %2d, %5s, %5s, %s\n"
			, (int)i
			, (unsigned long) fip->field_tag
			, fip->field_readcount, fip->field_writecount
			, fip->field_type
			, fip->field_bit
			, fip->field_oktochange ? "TRUE" : "FALSE"
			, fip->field_passcount ? "TRUE" : "FALSE"
			, fip->field_name
		);
	}
}

/*
 * Return size of TIFFDataType in bytes
 */
int
TIFFDataWidth(TIFFDataType type)
{
	switch(type)
	{
	case 0:  /* nothing */
	case 1:  /* TIFF_BYTE */
	case 2:  /* TIFF_ASCII */
	case 6:  /* TIFF_SBYTE */
	case 7:  /* TIFF_UNDEFINED */
		return 1;
	case 3:  /* TIFF_SHORT */
	case 8:  /* TIFF_SSHORT */
		return 2;
	case 4:  /* TIFF_LONG */
	case 9:  /* TIFF_SLONG */
	case 11: /* TIFF_FLOAT */
        case 13: /* TIFF_IFD */
		return 4;
	case 5:  /* TIFF_RATIONAL */
	case 10: /* TIFF_SRATIONAL */
	case 12: /* TIFF_DOUBLE */
		return 8;
	default:
		return 0; /* will return 0 for unknown types */
	}
}

/*
 * Return size of TIFFDataType in bytes.
 *
 * XXX: We need a separate function to determine the space needed
 * to store the value. For TIFF_RATIONAL values TIFFDataWidth() returns 8,
 * but we use 4-byte float to represent rationals.
 */
int
_TIFFDataSize(TIFFDataType type)
{
	switch (type) {
		case TIFF_BYTE:
		case TIFF_SBYTE:
		case TIFF_ASCII:
		case TIFF_UNDEFINED:
		    return 1;
		case TIFF_SHORT:
		case TIFF_SSHORT:
		    return 2;
		case TIFF_LONG:
		case TIFF_SLONG:
		case TIFF_FLOAT:
		case TIFF_IFD:
		case TIFF_RATIONAL:
		case TIFF_SRATIONAL:
		    return 4;
		case TIFF_DOUBLE:
		    return 8;
		default:
		    return 0;
	}
}

/*
 * Return nearest TIFFDataType to the sample type of an image.
 */
TIFFDataType
_TIFFSampleToTagType(TIFF* tif)
{
	uint32 bps = TIFFhowmany8(tif->tif_dir.td_bitspersample);

	switch (tif->tif_dir.td_sampleformat) {
	case SAMPLEFORMAT_IEEEFP:
		return (bps == 4 ? TIFF_FLOAT : TIFF_DOUBLE);
	case SAMPLEFORMAT_INT:
		return (bps <= 1 ? TIFF_SBYTE :
		    bps <= 2 ? TIFF_SSHORT : TIFF_SLONG);
	case SAMPLEFORMAT_UINT:
		return (bps <= 1 ? TIFF_BYTE :
		    bps <= 2 ? TIFF_SHORT : TIFF_LONG);
	case SAMPLEFORMAT_VOID:
		return (TIFF_UNDEFINED);
	}
	/*NOTREACHED*/
	return (TIFF_UNDEFINED);
}

const TIFFFieldInfo*
_TIFFFindFieldInfo(TIFF* tif, ttag_t tag, TIFFDataType dt)
{
	int i, n;

	if (tif->tif_foundfield && tif->tif_foundfield->field_tag == tag &&
	    (dt == TIFF_ANY || dt == tif->tif_foundfield->field_type))
		return (tif->tif_foundfield);
	/* NB: use sorted search (e.g. binary search) */
	if(dt != TIFF_ANY) {
            TIFFFieldInfo key = {0, 0, 0, 0, 0, 0, 0, 0};
	    TIFFFieldInfo* pkey = &key;
	    const TIFFFieldInfo **ret;

	    key.field_tag = tag;
            key.field_type = dt;

	    ret = (const TIFFFieldInfo **) bsearch(&pkey,
						   tif->tif_fieldinfo, 
						   tif->tif_nfields,
						   sizeof(TIFFFieldInfo *), 
						   tagCompare);
	    return (ret) ? (*ret) : NULL;
        } else for (i = 0, n = tif->tif_nfields; i < n; i++) {
		const TIFFFieldInfo* fip = tif->tif_fieldinfo[i];
		if (fip->field_tag == tag &&
		    (dt == TIFF_ANY || fip->field_type == dt))
			return (tif->tif_foundfield = fip);
	}
	return ((const TIFFFieldInfo *)0);
}

const TIFFFieldInfo*
_TIFFFindFieldInfoByName(TIFF* tif, const char *field_name, TIFFDataType dt)
{
	int i, n;

	if (tif->tif_foundfield
	    && streq(tif->tif_foundfield->field_name, field_name)
	    && (dt == TIFF_ANY || dt == tif->tif_foundfield->field_type))
		return (tif->tif_foundfield);
	/* NB: use sorted search (e.g. binary search) */
	if(dt != TIFF_ANY) {
            TIFFFieldInfo key = {0, 0, 0, 0, 0, 0, 0, 0};
	    TIFFFieldInfo* pkey = &key;
	    const TIFFFieldInfo **ret;

            key.field_name = (char *)field_name;
            key.field_type = dt;

            ret = (const TIFFFieldInfo **) lfind(&pkey,
						 tif->tif_fieldinfo, 
						 &tif->tif_nfields,
						 sizeof(TIFFFieldInfo *),
						 tagNameCompare);
	    return (ret) ? (*ret) : NULL;
        } else
		for (i = 0, n = tif->tif_nfields; i < n; i++) {
			const TIFFFieldInfo* fip = tif->tif_fieldinfo[i];
			if (streq(fip->field_name, field_name) &&
			    (dt == TIFF_ANY || fip->field_type == dt))
				return (tif->tif_foundfield = fip);
		}
	return ((const TIFFFieldInfo *)0);
}

const TIFFFieldInfo*
_TIFFFieldWithTag(TIFF* tif, ttag_t tag)
{
	const TIFFFieldInfo* fip = _TIFFFindFieldInfo(tif, tag, TIFF_ANY);
	if (!fip) {
		TIFFError("TIFFFieldWithTag",
			  "Internal error, unknown tag 0x%x",
                          (unsigned int) tag);
		assert(fip != NULL);
		/*NOTREACHED*/
	}
	return (fip);
}

const TIFFFieldInfo*
_TIFFFieldWithName(TIFF* tif, const char *field_name)
{
	const TIFFFieldInfo* fip =
		_TIFFFindFieldInfoByName(tif, field_name, TIFF_ANY);
	if (!fip) {
		TIFFError("TIFFFieldWithName",
			  "Internal error, unknown tag %s", field_name);
		assert(fip != NULL);
		/*NOTREACHED*/
	}
	return (fip);
}

const TIFFFieldInfo*
_TIFFFindOrRegisterFieldInfo( TIFF *tif, ttag_t tag, TIFFDataType dt )

{
    const TIFFFieldInfo *fld;

    fld = _TIFFFindFieldInfo( tif, tag, dt );
    if( fld == NULL )
    {
        fld = _TIFFCreateAnonFieldInfo( tif, tag, dt );
        _TIFFMergeFieldInfo( tif, fld, 1 );
    }

    return fld;
}

TIFFFieldInfo*
_TIFFCreateAnonFieldInfo(TIFF *tif, ttag_t tag, TIFFDataType field_type)
{
	TIFFFieldInfo *fld;
	(void) tif;

	fld = (TIFFFieldInfo *) _TIFFmalloc(sizeof (TIFFFieldInfo));
	if (fld == NULL)
	    return NULL;
	_TIFFmemset( fld, 0, sizeof(TIFFFieldInfo) );

	fld->field_tag = tag;
	fld->field_readcount = TIFF_VARIABLE;
	fld->field_writecount = TIFF_VARIABLE;
	fld->field_type = field_type;
	fld->field_bit = FIELD_CUSTOM;
	fld->field_oktochange = TRUE;
	fld->field_passcount = TRUE;
	fld->field_name = (char *) _TIFFmalloc(32);
	if (fld->field_name == NULL) {
	    _TIFFfree(fld);
	    return NULL;
	}

	/* note that this name is a special sign to TIFFClose() and
	 * _TIFFSetupFieldInfo() to free the field
	 */
	sprintf(fld->field_name, "Tag %d", (int) tag);

	return fld;    
}

/* vim: set ts=8 sts=8 sw=8 noet: */
