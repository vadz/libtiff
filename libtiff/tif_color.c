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

/*
 * CIE L*a*b* to CIE XYZ and CIE XYZ to RGB conversion routines are taken
 * from the VIPS library (http://www.vips.ecs.soton.ac.uk) with
 * the permission of John Cupitt, the VIPS author.
 */

/*
 * TIFF Library.
 *
 * Color space conversion routines.
 */

#include "tiffiop.h"
#include <math.h>

/*
 * Convert color value from the CIE L*a*b* 1976 space to CIE XYZ. Different
 * reference white tristimuli can be specified.
 */
void
TIFFCIELabToXYZ(TIFFCIELabToRGB *cielab, uint32 l, int32 a, int32 b,
		float *X, float *Y, float *Z)
{
	float L = (float)l * 100.0 / 255.0;
	float cby, tmp;

	if( L < 8.856 ) {
		*Y = (L * cielab->Y0) / 903.292;
		cby = 7.787 * (*Y / cielab->Y0) + 16.0 / 116.0;
	} else {
		cby = (L + 16.0) / 116.0;
		*Y = cielab->Y0 * cby * cby * cby;
	}

	tmp = (double)a / 500.0 + cby;
	if( tmp < 0.2069 )
		*X = cielab->X0 * (tmp - 0.13793) / 7.787;
	else    
		*X = cielab->X0 * tmp * tmp * tmp;

	tmp = cby - (double)b / 200.0;
	if( tmp < 0.2069 )
		*Z = cielab->Z0 * (tmp - 0.13793) / 7.787;
	else    
		*Z = cielab->Z0 * tmp * tmp * tmp;
}

/*
 * Convert color value from the XYZ space to RGB.
 */
void
TIFFXYZToRGB(TIFFCIELabToRGB *cielab, float X, float Y, float Z,
	     uint32 *r, uint32 *g, uint32 *b)
{
	int i;
	float Yr, Yg, Yb;
	float *matrix = &cielab->display->d_mat[0][0];

	/* Multiply through the matrix to get luminosity values. */
	Yr =  matrix[0] * X + matrix[1] * Y + matrix[2] * Z;
	Yg =  matrix[3] * X + matrix[4] * Y + matrix[5] * Z;
	Yb =  matrix[6] * X + matrix[7] * Y + matrix[8] * Z;

	/* Clip input */
	Yr = TIFFmax( Yr, cielab->display->d_Y0R );
	Yg = TIFFmax( Yg, cielab->display->d_Y0G );
	Yb = TIFFmax( Yb, cielab->display->d_Y0B );

	/* Turn luminosity to colour value. */
	i = TIFFmin(cielab->range,
		    (Yr - cielab->display->d_Y0R) / cielab->rstep);
	*r = TIFFrint(cielab->Yr2r[i]);

	i = TIFFmin(cielab->range,
		    (Yg - cielab->display->d_Y0G) / cielab->gstep);
	*g = TIFFrint(cielab->Yg2g[i]);

	i = TIFFmin(cielab->range,
		    (Yb - cielab->display->d_Y0B) / cielab->bstep);
	*b = TIFFrint(cielab->Yb2b[i]);

	/* Clip output. */
	*r = TIFFmin( *r, cielab->display->d_Vrwr );
	*g = TIFFmin( *g, cielab->display->d_Vrwg );
	*b = TIFFmin( *b, cielab->display->d_Vrwb );
}

/* 
 * Allocate conversion state structures and make look_up tables for
 * the Yr,Yb,Yg <=> r,g,b conversions.
 */
int
TIFFCIELabToRGBInit(TIFFCIELabToRGB* cielab, TIFFDisplay *display,
		    float X0, float Y0, float Z0)
{
	static char module[] = "TIFFCIELabToRGBInit";

	int i;
	float gamma;

	cielab->range = 1500;

	cielab->display = (TIFFDisplay *)_TIFFmalloc(sizeof(TIFFDisplay));
	if (cielab == NULL) {
		TIFFError(module, "No space for display structure");
		return -1;
	}

	cielab->Yr2r = (float *)
		_TIFFmalloc((cielab->range + 1) * sizeof(float));
	if (cielab->Yr2r == NULL) {
		TIFFError(module, "No space for Red conversion array");
		_TIFFfree(cielab->display);
		return -1;
	}

	cielab->Yg2g = (float *)
		_TIFFmalloc((cielab->range + 1) * sizeof(float));
	if (cielab->Yg2g == NULL) {
		TIFFError(module,
			  "No space for Green conversion array");
		_TIFFfree(cielab->Yr2r);
		_TIFFfree(cielab->display);
		return -1;
	}

	cielab->Yb2b = (float *)
		_TIFFmalloc((cielab->range + 1) * sizeof(float));
	if (cielab->Yb2b == NULL) {
		TIFFError(module, "No space for Blue conversion array");
		_TIFFfree(cielab->Yb2b);
		_TIFFfree(cielab->Yr2r);
		_TIFFfree(cielab->display);
		return -1;
	}

	_TIFFmemcpy(cielab->display, display, sizeof(TIFFDisplay));

	/* Red */
	gamma = 1.0 / cielab->display->d_gammaR ;
	cielab->rstep =
		(cielab->display->d_YCR - cielab->display->d_Y0R)
		/ cielab->range;
	for(i = 0; i <= cielab->range; i++) {
		cielab->Yr2r[i] = cielab->display->d_Vrwr
		    * (pow((double)i / cielab->range, gamma));
	}

	/* Green */
	gamma = 1.0 / cielab->display->d_gammaG ;
	cielab->gstep =
	    (cielab->display->d_YCR - cielab->display->d_Y0R)
	    / cielab->range;
	for(i = 0; i <= cielab->range; i++) {
		cielab->Yg2g[i] = cielab->display->d_Vrwg
		    * (pow((double)i / cielab->range, gamma));
	}

	/* Blue */
	gamma = 1.0 / cielab->display->d_gammaB ;
	cielab->bstep =
	    (cielab->display->d_YCR - cielab->display->d_Y0R)
	    / cielab->range;
	for(i = 0; i <= cielab->range; i++) {
		cielab->Yb2b[i] = cielab->display->d_Vrwb
		    * (pow((double)i / cielab->range, gamma));
	}

	/* Init reference white point */
	cielab->X0 = X0;
	cielab->Y0 = Y0;
	cielab->Z0 = Z0;

	return 0;
}

/* 
 * Free TIFFYCbCrToRGB structure.
 */
int
TIFFCIELabToRGBEnd(TIFFCIELabToRGB* cielab)
{
	static char module[] = "TIFFCIELabToRGBEnd";

	_TIFFfree(cielab->Yr2r);
	_TIFFfree(cielab->Yg2g);
	_TIFFfree(cielab->Yb2b);
	_TIFFfree(cielab->display);
}

#define	SHIFT			16
#define	FIX(x)			((int32)((x) * (1L<<SHIFT) + 0.5))
#define	ONE_HALF		((int32)(1<<(SHIFT-1)))

/*
 * Initialize the YCbCr->RGB conversion tables.  The conversion
 * is done according to the 6.0 spec:
 *
 *    R = Y + Cr*(2 - 2*LumaRed)
 *    B = Y + Cb*(2 - 2*LumaBlue)
 *    G =   Y
 *        - LumaBlue*Cb*(2-2*LumaBlue)/LumaGreen
 *        - LumaRed*Cr*(2-2*LumaRed)/LumaGreen
 *
 * To avoid floating point arithmetic the fractional constants that
 * come out of the equations are represented as fixed point values
 * in the range 0...2^16.  We also eliminate multiplications by
 * pre-calculating possible values indexed by Cb and Cr (this code
 * assumes conversion is being done for 8-bit samples).
 */
int
TIFFYCbCrToRGBInit(TIFFYCbCrToRGB* ycbcr,
		   float LumaRed, float LumaGreen, float LumaBlue)
{
    TIFFRGBValue* clamptab;
    int i;

    clamptab = (TIFFRGBValue*)(
	(tidata_t) ycbcr+TIFFroundup(sizeof (TIFFYCbCrToRGB), sizeof (long)));
    _TIFFmemset(clamptab, 0, 256);		/* v < 0 => 0 */
    ycbcr->clamptab = (clamptab += 256);
    for (i = 0; i < 256; i++)
	clamptab[i] = (TIFFRGBValue) i;
    _TIFFmemset(clamptab+256, 255, 2*256);	/* v > 255 => 255 */
    { float f1 = 2-2*LumaRed;		int32 D1 = FIX(f1);
      float f2 = LumaRed*f1/LumaGreen;	int32 D2 = -FIX(f2);
      float f3 = 2-2*LumaBlue;		int32 D3 = FIX(f3);
      float f4 = LumaBlue*f3/LumaGreen;	int32 D4 = -FIX(f4);
      int x;

      ycbcr->Cr_r_tab = (int*) (clamptab + 3*256);
      ycbcr->Cb_b_tab = ycbcr->Cr_r_tab + 256;
      ycbcr->Cr_g_tab = (int32*) (ycbcr->Cb_b_tab + 256);
      ycbcr->Cb_g_tab = ycbcr->Cr_g_tab + 256;
      /*
       * i is the actual input pixel value in the range 0..255
       * Cb and Cr values are in the range -128..127 (actually
       * they are in a range defined by the ReferenceBlackWhite
       * tag) so there is some range shifting to do here when
       * constructing tables indexed by the raw pixel data.
       *
       * XXX handle ReferenceBlackWhite correctly to calculate
       *     Cb/Cr values to use in constructing the tables.
       */
      for (i = 0, x = -128; i < 256; i++, x++) {
	  ycbcr->Cr_r_tab[i] = (int)((D1*x + ONE_HALF)>>SHIFT);
	  ycbcr->Cb_b_tab[i] = (int)((D3*x + ONE_HALF)>>SHIFT);
	  ycbcr->Cr_g_tab[i] = D2*x;
	  ycbcr->Cb_g_tab[i] = D4*x + ONE_HALF;
      }
    }

    return 0;
}
#undef	SHIFT
#undef	ONE_HALF
#undef	FIX


