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
 * the permission of John Cupitt, the author.
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
TIFFCIELabToXYZ(uint32 l, int32 a, int32 b, float *X, float *Y, float *Z,
		float X0, float Y0, float Z0)
{
	float L = (float)l * 100.0 / 255.0;
	float cby, tmp;

	if( L < 8.856 ) {
		*Y = (L * Y0) / 903.292;
		cby = 7.787 * (*Y / Y0) + 16.0 / 116.0;
	} else {
		cby = (L + 16.0) / 116.0;
		*Y = Y0 * cby * cby * cby;
	}

	tmp = (double)a / 500.0 + cby;
	if( tmp < 0.2069 )
		*X = X0 * (tmp - 0.13793) / 7.787;
	else    
		*X = X0 * tmp * tmp * tmp;

	tmp = cby - (double)b / 200.0;
	if( tmp < 0.2069 )
		*Z = Z0 * (tmp - 0.13793) / 7.787;
	else    
		*Z = Z0 * tmp * tmp * tmp;
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
TIFFCIELabToRGBInit(TIFFCIELabToRGB** cielab)
{
	static char module[] = "TIFFCIELabToRGBInit";

	int i;
	float gamma;

        TIFFDisplay sRGB_display = {
		{			/* XYZ -> luminance matrix */
			{  3.2410, -1.5374, -0.4986 },
			{  -0.9692, 1.8760, 0.0416 },
			{  0.0556, -0.2040, 1.0570 }
		},	
		100, 100, 100,		/* Light o/p for reference white */
		255, 255, 255,		/* Pixel values for ref. white */
		1, 1, 1,		/* Residual light o/p for black pixel */
		2.4, 2.4, 2.4,		/* Gamma values for the three guns */
        };


	if (!(*cielab)) {
		*cielab = (TIFFCIELabToRGB *)
			_TIFFmalloc(sizeof(TIFFCIELabToRGB));
		if (*cielab == NULL) {
			TIFFError(module,
				  "No space for CIE L*a*b* control structure");
			return -1;
		}

		(*cielab)->range = 1500;

		(*cielab)->display =
			(TIFFDisplay *)_TIFFmalloc(sizeof(TIFFDisplay));
		if ((*cielab) == NULL) {
			TIFFError(module, "No space for display structure");
			_TIFFfree(*cielab);
			*cielab = 0;
			return -1;
		}

		(*cielab)->Yr2r = (float *)
			_TIFFmalloc(((*cielab)->range + 1) * sizeof(float));
		if ((*cielab)->Yr2r == NULL) {
			TIFFError(module, "No space for Red conversion array");
			_TIFFfree((*cielab)->display);
			_TIFFfree(*cielab);
			*cielab = 0;
			return -1;
		}

		(*cielab)->Yg2g = (float *)
			_TIFFmalloc(((*cielab)->range + 1) * sizeof(float));
		if ((*cielab)->Yg2g == NULL) {
			TIFFError(module,
				  "No space for Green conversion array");
			_TIFFfree((*cielab)->Yr2r);
			_TIFFfree((*cielab)->display);
			_TIFFfree(*cielab);
			*cielab = 0;
			return -1;
		}

		(*cielab)->Yb2b = (float *)
			_TIFFmalloc(((*cielab)->range + 1) * sizeof(float));
		if ((*cielab)->Yb2b == NULL) {
			TIFFError(module, "No space for Blue conversion array");
			_TIFFfree((*cielab)->Yb2b);
			_TIFFfree((*cielab)->Yr2r);
			_TIFFfree((*cielab)->display);
			_TIFFfree(*cielab);
			*cielab = 0;
			return -1;
		}

		_TIFFmemcpy((*cielab)->display, &sRGB_display,
			    sizeof(TIFFDisplay));
	}

	/* Red */
	gamma = 1.0 / (*cielab)->display->d_gammaR ;
	(*cielab)->rstep =
		((*cielab)->display->d_YCR - (*cielab)->display->d_Y0R)
		/ (*cielab)->range;
	for(i = 0; i <= (*cielab)->range; i++) {
		(*cielab)->Yr2r[i] =
		    (*cielab)->display->d_Vrwr
		    * (pow((double)i / (*cielab)->range, gamma));
	}

	/* Green */
	gamma = 1.0 / (*cielab)->display->d_gammaG ;
	(*cielab)->gstep =
	    ((*cielab)->display->d_YCR - (*cielab)->display->d_Y0R)
	    / (*cielab)->range;
	for(i = 0; i <= (*cielab)->range; i++) {
		(*cielab)->Yg2g[i] =
		    (*cielab)->display->d_Vrwg
		    * (pow((double)i / (*cielab)->range, gamma));
	}

	/* Blue */
	gamma = 1.0 / (*cielab)->display->d_gammaB ;
	(*cielab)->bstep =
	    ((*cielab)->display->d_YCR - (*cielab)->display->d_Y0R)
	    / (*cielab)->range;
	for(i = 0; i <= (*cielab)->range; i++) {
		(*cielab)->Yb2b[i] =
		    (*cielab)->display->d_Vrwb
		    * (pow((double)i / (*cielab)->range, gamma));
	}

	return 0;
}


