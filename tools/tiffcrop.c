/* $Id$ */

/* tiffcrop.c -- a port of tiffcp.c extended to include cropping of selections
 *
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
 *
 * Richard Nolde  10/2006 Add support for the options below to extract 
 * sections of image(s) and to modify the whole image or selected portion
 * with rotations, mirroring, and colorscale/colormap inversion of selected
 * types of TIFF images when appropriate
 *
 * Options: 
 * -u units     [in, cm, px ] inches, centimeters or pixels
 * -x #         horizontal dimension of region to extract expressed in current
 *              units
 * -y #         vertical dimension of region to extract expressed in current
 *              units
 * -e t|l|r|b   edge to use as origin
 * -m #,#,#,#   margins from edges for selection: top, left, bottom, right
 *              (commas separated)
 * -z #:#,#:#   up to six zones of the image designated as zone X of Y,
 *              eg 1:3 would be first of three equal portions measured
 *              from reference edge
 * -n odd|even|#,#-#,#|last sequences and ranges of images within file to
 *              process the words odd or even may be used to specify all odd
 *              or even numbered images the word last may be used in place
 *              of a number in the sequence to indicate the final image in
 *              the file without knowing how many images there are
 * -R #         rotate image or crop selection by 90,180,or 270 degrees
 *              clockwise  
 * -F h|v       flip (mirror) image or crop selection horizontally
 *              or vertically
 * -I           invert the colormap, black to white, for bilevel
 *              and grayscale images
 */

#include "tif_config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ctype.h>
#include <limits.h>
#include <assert.h>

#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

#include "tiffio.h"

#define	streq(a,b)	(strcmp((a),(b)) == 0)
#define	strneq(a,b,n)	(strncmp((a),(b),(n)) == 0)

#define	TRUE	1
#define	FALSE	0

/*
 * Definitions and data structures required to support cropping and image
 * manipulations.
 */

#define EDGE_TOP      1
#define EDGE_LEFT     2
#define EDGE_BOTTOM   3
#define EDGE_RIGHT    4
#define MIRROR_HORIZ  1
#define MIRROR_VERT   2

#define CROP_NONE     0
#define CROP_MARGINS  1
#define CROP_WIDTH    2
#define CROP_LENGTH   4
#define CROP_ZONES    8
#define CROP_ROTATE  16
#define CROP_MIRROR  32
#define CROP_INVERT  64

#define STRIP    1
#define TILE     2

#define MAX_ZONES     6  /* number of sections on a singel page */
#define MAX_IMAGES  256  /* number of images in descrete list */

/* Offsets into buffer for margins and fixed width and length */
struct offset {
  uint32  tmargin;
  uint32  lmargin;
  uint32  bmargin;
  uint32  rmargin;
  uint32  crop_width;
  uint32  crop_length;
  uint32  startx;
  uint32  endx;
  uint32  starty;
  uint32  endy;
};

/* Description of a zone within the image. Position 1 of 3 zones would be 
 * the first third of the image. These are computed after margins and 
 * width/length requests are applied so that you can extract multiple 
 * zones from within a larger region for OCR or barcode recognition.
 */
struct  pageseg {
  uint32 x1;        /* index of left edge */
  uint32 x2;        /* index of right edge */
  uint32 y1;        /* index of top edge */
  uint32 y2;        /* index of bottom edge */
  int    total;     /* total equal sized divisions of crop area */
  int    position;  /* ordinal of segment to be extracted */
  uint32 buffsize;  /* size of buffer needed to hold the cropped region */
};

/* Cropping parameters from command line and image data */
struct crop_mask {
  uint16 crop_mode;       /* Crop options to be applied */
  uint16 res_unit;        /* Resolution unit for margins and selections */
  uint16 edge_ref;        /* Reference edge from which zones are calculated */
  uint16 rotation;        /* Clockwise rotation of the extracted region or image */
  uint16 mirror;          /* Mirror extracted region or image horizontally or vertically */
  uint16 invert;          /* Invert the color map of image or region */
  double width;           /* Selection width for master crop region in requested units */
  double length;          /* Selection length for master crop region in requesed units */
  double margins[4];      /* Top, left, bottom, right margins */
  uint32 combined_width;  /* Width of combined cropped zones */
  uint32 combined_length; /* Length of combined cropped zones */
  uint32 bufftotal;       /* size of buffer needed to hold all the cropped region */
  int    zones;           /* Number of zones requested */
  struct pageseg zonelist[MAX_ZONES]; /* Zones within page or master crop region */
};

/* 
 * Global variables saving state of the program.
 */
static  int outtiled = -1;
static  uint32 tilewidth;
static  uint32 tilelength;

static	uint16 config;
static	uint16 compression;
static	uint16 predictor;
static	uint16 fillorder;
static	uint16 orientation;
static	uint32 rowsperstrip;
static	uint32 g3opts;
static	int ignore = FALSE;		/* if true, ignore read errors */
static	uint32 defg3opts = (uint32) -1;
static	int quality = 75;		/* JPEG quality */
static	int jpegcolormode = JPEGCOLORMODE_RGB;
static	uint16 defcompression = (uint16) -1;
static	uint16 defpredictor = (uint16) -1;
static  TIFF* bias = NULL;
static  int pageNum = 0;

/* 
 * Helper functions declarations.
 */
static	int processCompressOptions(char*);
static	int tiffcp(TIFF*, TIFF*);
static	void usage(void);

static  int initCropMasks (struct crop_mask *cps);
static  int computeOffsets(uint16, float, float, uint32, uint32, 
			   struct crop_mask *, struct offset *);
static  int getCropOffsets(TIFF*, struct crop_mask *);
static  int loadImage(TIFF*, unsigned char **);
static  int extractCropRegions(TIFF *, struct crop_mask *, unsigned char *, unsigned char *);
static  int createCroppedImage(TIFF*, struct crop_mask *, unsigned char **, unsigned char **);
static  int rotateImage(uint16, uint16, uint16, uint32 *, uint32 *, unsigned char **);
static  int mirrorImage(uint16, uint16, uint16, uint32, uint32, unsigned char *);
static  int invertImage(uint16, uint16, uint16, uint32, uint32, unsigned char *);
static  int writeCroppedImage(TIFF *, TIFF *, struct crop_mask *, unsigned char *);

int
main(int argc, char* argv[])
  {
  uint16 defconfig = (uint16) -1;
  uint16 deffillorder = 0;
  uint32 deftilewidth = (uint32) -1;
  uint32 deftilelength = (uint32) -1;
  uint32 defrowsperstrip = (uint32) 0;
  uint32 dirnum = 0;
  uint32 bias_image = 0;
  TIFF* in = NULL;
  TIFF* out;
  char mode[10];
  char* mp = mode;
  struct  crop_mask    crop_data;       /* Cropping parameters for image */
  unsigned char *read_buff    = NULL;   /* input image data buffer */
  unsigned char *crop_buff    = NULL;   /* crop area buffer */
  char *opt_offset   = NULL;            /* Postion in string of value sought */
  char *opt_ptr      = NULL;        /* Pointer to next token in option set */
  char *sep          = NULL;            /* pointer to a token separator */
  uint32 start, end, image_count = 0, next_image = 0;
  uint32 imagelist[MAX_IMAGES + 1];     /* individually specified images */
  uint32 i, j;
  int c;

  extern int optind;
  extern char* optarg;

  initCropMasks(&crop_data);

  *mp++ = 'w';
  *mp = '\0';
  while ((c = getopt(argc, argv, "ab:c:d:f:il:p:r:st:w:BLMCE:F:IN:m:R:U:X:Y:")) != -1)
    switch (c) {
      case 'b': if (bias) {  /* this file is bias image subtracted from others */
                  fputs ("Only 1 bias image may be specified\n", stderr);
                  exit (-2);
                }
                {
                uint16    samples = (uint16) -1;
                char *biasFn = optarg;
	        sep = strpbrk(biasFn, ":");
	        if (sep)
                  {
                  *sep = '\0';
                  bias_image = atoi(sep +1);
		  }
                bias = TIFFOpen (biasFn, "r");
                if (!bias)
                  exit (-5);
                if (bias_image) {
                  if (!TIFFSetDirectory(bias, bias_image))
                    {
		    fputs ("Invalid IFD for bias image", stderr);
                    exit (-7);
                    }
		  } 
                if (TIFFIsTiled (bias)) {
                  fputs ("Bias image must be organized in strips\n", stderr);
                  exit (-7);
                }
	        TIFFGetField(bias, TIFFTAG_SAMPLESPERPIXEL, &samples);
                if (samples != 1) {
                  fputs ("Bias image must be monochrome\n", stderr);
                  exit (-7);
                  }
	        }
                break;
      case 'a': mode[0] = 'a'; 	/* append to output */
		break;
      case 'c':	if (!processCompressOptions(optarg)) 	/* compression scheme */
		   usage();
		break;
      case 'd':	dirnum = strtoul(optarg, NULL, 0); /* initial directory offset */
		break;
      case 'f':	if (streq(optarg, "lsb2msb"))	   /* fill order */
		  deffillorder = FILLORDER_LSB2MSB;
		else if (streq(optarg, "msb2lsb"))
		  deffillorder = FILLORDER_MSB2LSB;
		else
		  usage();
		break;
      case 'i':	ignore = TRUE; 		/* ignore errors */
		break;
      case 'l':	outtiled = TRUE;	 /* tile length */
		deftilelength = atoi(optarg);
		break;
      case 'p': /* planar configuration */
		if (streq(optarg, "separate"))
		  defconfig = PLANARCONFIG_SEPARATE;
		else if (streq(optarg, "contig"))
		  defconfig = PLANARCONFIG_CONTIG;
		else
		  usage();
		break;
      case 'r':	/* rows/strip */
		defrowsperstrip = atol(optarg);
		break;
      case 's':	/* generate stripped output */
		outtiled = FALSE;
		break;
      case 't':	/* generate tiled output */
		outtiled = TRUE;
		break;
      case 'w':	/* tile width */
		outtiled = TRUE;
		deftilewidth = atoi(optarg);
		break;
      /* options for file open modes */
      case 'B': *mp++ = 'b'; *mp = '\0';
		break;
      case 'L': *mp++ = 'l'; *mp = '\0';
		break;
      case 'M': *mp++ = 'm'; *mp = '\0';
		break;
      case 'C': *mp++ = 'c'; *mp = '\0';
		break;
      /* image manipulation routine options */
      case 'm': /* margins to exclude from selection*/
		/* order of values must be TOP, LEFT, BOTTOM, RIGHT */
		crop_data.crop_mode |= CROP_MARGINS;
                for (i = 0, opt_ptr = strtok (optarg, ",:");
                    ((opt_ptr != NULL) &&  (i < 4));
                     (opt_ptr = strtok (NULL, ",:")), i++)
                    {
		    crop_data.margins[i] = atof(opt_ptr);
                    }
		break;
      case 'E':	/* edge reference */
		switch (tolower(optarg[0]))
                  {
		  case 't': crop_data.edge_ref = EDGE_TOP;
                            break;
                  case 'b': crop_data.edge_ref = EDGE_BOTTOM;
                             break;
                  case 'l': crop_data.edge_ref = EDGE_LEFT;
                            break;
                  case 'r': crop_data.edge_ref = EDGE_RIGHT;
                            break;
		  default:  fprintf (stderr, "Edge reference must be top, bottom, left, or right.\n");
			    usage();
		  }
	        break;
      case 'F': /* flip eg mirror image or cropped segment, M was already used */
		crop_data.crop_mode |= CROP_MIRROR;
		switch (tolower(optarg[0]))
                  {
		  case  'h': crop_data.mirror = MIRROR_HORIZ;
                             break;
                  case  'v': crop_data.mirror = MIRROR_VERT;
                             break;
		  default:   fprintf (stderr, "Flip mode must be h or v.\n");
	                     usage();
		  }
		break;
      case 'I': /* invert the color space, eg black to white */
		crop_data.crop_mode |= CROP_INVERT;
		break;
      case 'N':	/* list of images to process */
                for (i = 0, opt_ptr = strtok (optarg, ",");
                    ((opt_ptr != NULL) &&  (i < MAX_IMAGES));
                     (opt_ptr = strtok (NULL, ",")))
                     { /* We do not know how many images are in file yet 
			* so we build a list to include the maximum allowed
                        * and follow it until we hit the end of the file
                        */
		     if (streq(opt_ptr, "odd"))
                       {
		       for (j = 1; j <= MAX_IMAGES; j += 2)
			 imagelist[i++] = j;
                         image_count = (MAX_IMAGES - 1) / 2;
                         break;
		       }
		     else
                       {
		       if (streq(opt_ptr, "even"))
                         {
			 for (j = 2; j <= MAX_IMAGES; j += 2)
			   imagelist[i++] = j;
                           image_count = MAX_IMAGES / 2;
                           break;
		         }
		       else
                         {
 			 if (streq(opt_ptr, "last"))
			   imagelist[i++] = MAX_IMAGES;
			 else  /* single value between commas */
			   {
			   sep = strpbrk(opt_ptr, ":-");
			   if (!sep)
			     imagelist[i++] = atoi(opt_ptr);
                           else
                             {
			     *sep = '\0';
                             start = atoi (opt_ptr);
                             if (!strcmp((sep + 1), "last"))
			       end = MAX_IMAGES;
                             else
                               end = atoi (sep + 1);
                             for (j = start; j <= end && j - start + i < MAX_IMAGES; j++)
			       imagelist[i++] = j;
			     }
			   }
		         }
		      }
		    }
                 image_count = i;
		 break;
      case 'R': /* rotate image or cropped segment */
		crop_data.crop_mode |= CROP_ROTATE;
		switch (strtoul(optarg, NULL, 0))
                  {
		  case  90:  crop_data.rotation = (uint16)90;
                             break;
                  case  180: crop_data.rotation = (uint16)180;
                             break;
                  case  270: crop_data.rotation = (uint16)270;
                             break;
		  default:  fprintf (stderr,
                    "Rotation must be 90, 180, or 270 degrees clockwise.\n");
			    usage();
		  }
		break;
      case 'U':	/* units for measurements and offsets */
		if (streq(optarg, "in"))
		  crop_data.res_unit = RESUNIT_INCH;
		else if (streq(optarg, "cm"))
		  crop_data.res_unit = RESUNIT_CENTIMETER;
		else if (streq(optarg, "px"))
		   crop_data.res_unit = RESUNIT_NONE;
		else
		  usage();
		break;
      case 'X':	/* selection width */
		crop_data.crop_mode |= CROP_WIDTH;
		crop_data.width = atof(optarg);
		break;
      case 'Y':	/* selection length */
		crop_data.crop_mode |= CROP_LENGTH;
		crop_data.length = atof(optarg);
		break;
      case 'Z': /* zones of an image X:Y read as zone X of Y */
		crop_data.crop_mode |= CROP_ZONES;
		for (i = 0, opt_ptr = strtok (optarg, ",");
                   ((opt_ptr != NULL) &&  (i < MAX_ZONES));
                    (opt_ptr = strtok (NULL, ",")), i++)
                    {
		    crop_data.zones++;
		    opt_offset = index(opt_ptr, ':');
                    *opt_offset = '\0';
                    crop_data.zonelist[i].position = atoi(opt_ptr);
                    crop_data.zonelist[i].total    = atoi(opt_offset + 1);
                    }
		break;
      case '?':	usage();
		/*NOTREACHED*/
      }

  if (argc - optind < 2)
    usage();

  out = TIFFOpen(argv[argc - 1], mode);
  if (out == NULL)
    return (-2);

  if ((argc - optind) == 2)
    pageNum = -1;

  for (; optind < argc-1 ; optind++)
    {
    in = TIFFOpen (argv[optind], "r");
    if (in == NULL)
      return (-3);

    if (image_count == 0)
      dirnum = 0;
    else
      {
      dirnum = (tdir_t)(imagelist[next_image] - 1);
      next_image++;
      }

    if (dirnum == MAX_IMAGES - 1)
      dirnum = TIFFNumberOfDirectories(in) - 1;

    if (dirnum != 0 && !TIFFSetDirectory(in, dirnum))
      {
      TIFFError(TIFFFileName(in),"Error, setting subdirectory at %#x", dirnum);
      (void) TIFFClose(out);
      return (1);
      }
	
    for (;;)
      {
      config = defconfig;
      compression = defcompression;
      predictor = defpredictor;
      fillorder = deffillorder;
      rowsperstrip = defrowsperstrip;
      tilewidth = deftilewidth;
      tilelength = deftilelength;
      g3opts = defg3opts;

      if (crop_data.crop_mode != CROP_NONE)
        {
	if (getCropOffsets(in, &crop_data))
          {
          TIFFError("main", "Unable to define crop regions");
          exit (-1);
	  }

        if (loadImage(in, &read_buff))
          {
          TIFFError("main", "Unable to load source image");
          exit (-1);
	  }
        if (createCroppedImage(in, &crop_data, &read_buff, &crop_buff))
          {
          TIFFError("main", "Unable to create output image");
          exit (-1);
	  }
        if (writeCroppedImage(in, out, &crop_data, crop_buff))
          {
          TIFFError("main", "Unable to write new image");
          exit (-1);
	  }
        }
      else
        { 
        if (!tiffcp(in, out) || !TIFFWriteDirectory(out))
          {
          TIFFClose(out);
          return (1);
	  }
        }
      /* No image list specified, just read the next image */
      if (image_count == 0)
        dirnum++;
      else
        {
	dirnum = (tdir_t)(imagelist[next_image] - 1);
        next_image++;
        }

      if (dirnum == MAX_IMAGES - 1)
        dirnum = TIFFNumberOfDirectories(in) - 1;

      if (!TIFFSetDirectory(in, dirnum))
         break;
      }
    }

  TIFFClose(in);

  /* If we did not use the read buffer as the crop buffer */
  if (read_buff)
        _TIFFfree(read_buff);

  if (crop_buff)
    _TIFFfree(crop_buff);

  TIFFClose(out);

  return (0);
  }


static void
processG3Options(char* cp)
{
	if( (cp = strchr(cp, ':')) ) {
		if (defg3opts == (uint32) -1)
			defg3opts = 0;
		do {
			cp++;
			if (strneq(cp, "1d", 2))
				defg3opts &= ~GROUP3OPT_2DENCODING;
			else if (strneq(cp, "2d", 2))
				defg3opts |= GROUP3OPT_2DENCODING;
			else if (strneq(cp, "fill", 4))
				defg3opts |= GROUP3OPT_FILLBITS;
			else
				usage();
		} while( (cp = strchr(cp, ':')) );
	}
}

static int
processCompressOptions(char* opt)
{
	if (streq(opt, "none")) {
		defcompression = COMPRESSION_NONE;
	} else if (streq(opt, "packbits")) {
		defcompression = COMPRESSION_PACKBITS;
	} else if (strneq(opt, "jpeg", 4)) {
		char* cp = strchr(opt, ':');

                defcompression = COMPRESSION_JPEG;
                while( cp )
                {
                    if (isdigit((int)cp[1]))
			quality = atoi(cp+1);
                    else if (cp[1] == 'r' )
			jpegcolormode = JPEGCOLORMODE_RAW;
                    else
                        usage();

                    cp = strchr(cp+1,':');
                }
	} else if (strneq(opt, "g3", 2)) {
		processG3Options(opt);
		defcompression = COMPRESSION_CCITTFAX3;
	} else if (streq(opt, "g4")) {
		defcompression = COMPRESSION_CCITTFAX4;
	} else if (strneq(opt, "lzw", 3)) {
		char* cp = strchr(opt, ':');
		if (cp)
			defpredictor = atoi(cp+1);
		defcompression = COMPRESSION_LZW;
	} else if (strneq(opt, "zip", 3)) {
		char* cp = strchr(opt, ':');
		if (cp)
			defpredictor = atoi(cp+1);
		defcompression = COMPRESSION_ADOBE_DEFLATE;
	} else
		return (0);
	return (1);
}

char* stuff[] = {
"usage: tiffcrop [options] input output",
"where options are:",
" -a		append to output instead of overwriting",
" -d offset	set initial directory offset",
" -p contig	pack samples contiguously (e.g. RGBRGB...)",
" -p separate	store samples separately (e.g. RRR...GGG...BBB...)",
" -s		write output in strips",
" -t		write output in tiles",
" -i		ignore read errors",
" -b file:#	bias (dark) monochrome image to be subtracted from all others",
"               Note that bias filename may be of the form filename:#",
"               where # specifies image file directory for bias filename.",
" For example:  tiffcrop -b bias.tif:1  esp.tif test.tif",
"  subtract 2nd image in bias.tif from all images in esp.tiff producing test.tif",
"",
" -r #		make each strip have no more than # rows",
" -w #		set output tile width (pixels)",
" -l #		set output tile length (pixels)",
"",
" -f lsb2msb	force lsb-to-msb FillOrder for output",
" -f msb2lsb	force msb-to-lsb FillOrder for output",
"",
" -c lzw[:opts]	compress output with Lempel-Ziv & Welch encoding",
" -c zip[:opts]	compress output with deflate encoding",
" -c jpeg[:opts]	compress output with JPEG encoding",
" -c packbits	compress output with packbits encoding",
" -c g3[:opts]	compress output with CCITT Group 3 encoding",
" -c g4		compress output with CCITT Group 4 encoding",
" -c none	use no compression algorithm on output",
"",
"Group 3 options:",
" 1d		use default CCITT Group 3 1D-encoding",
" 2d		use optional CCITT Group 3 2D-encoding",
" fill		byte-align EOL codes",
"For example, -c g3:2d:fill to get G3-2D-encoded data with byte-aligned EOLs",
"",
"JPEG options:",
" #		set compression quality level (0-100, default 75)",
" r		output color image as RGB rather than YCbCr",
"For example, -c jpeg:r:50 to get JPEG-encoded RGB data with 50% comp. quality",
"",
"LZW and deflate options:",
" #		set predictor value",
"For example, -c lzw:2 to get LZW-encoded data with horizontal differencing",
"",
" -N odd|even|#,#-#,#|last         sequences and ranges of images within file to process",
"             the words odd or even may be used to specify all odd or even numbered images",
"             the word last may be used in place of a number in the sequence to indicate",
"             the final image in the file without knowing how many images there are",
"",
" -E t|l|r|b  edge to use as origin for width and length of crop region",
" -U units    [in, cm, px ] inches, centimeters or pixels",
"",
" -M #,#,#,#  margins from edges for selection: top, left, bottom, right separated by commas",
" -X #        horizontal dimension of region to extract expressed in current units",
" -Y #        vertical dimension of region to extract expressed in current units",
" -Z #:#,#:#  zones of the image designated as position X of Y,",
"             eg 1:3 would be first of three equal portions measured from reference edge",
"",
" -F h|v      flip ie mirror image or extracted region horizontally or vertically",
" -R #        [90,180,or 270] degrees clockwise rotation of image or extracted region",
" -I          invert the color space, eg dark to light for bilevel and grayscale images",
"",
"      Note that images to process may be specified with -d # to process all",
"beginning at image # (numbering from zero) or by the -N option with a comma",
"separated list of images (numbered from one) which may include the word last or",
"the words odd or even to process all the odd or even numbered images",
"",
"For example, -n 1,5-7,last to process the 1st, 5th through 7th, and final image",
NULL
};


static void
usage(void)
{
	char buf[BUFSIZ];
	int i;

	setbuf(stderr, buf);
        fprintf(stderr, "%s\n\n", TIFFGetVersion());
	for (i = 0; stuff[i] != NULL; i++)
		fprintf(stderr, "%s\n", stuff[i]);
	exit(-1);
}

#define	CopyField(tag, v) \
    if (TIFFGetField(in, tag, &v)) TIFFSetField(out, tag, v)
#define	CopyField2(tag, v1, v2) \
    if (TIFFGetField(in, tag, &v1, &v2)) TIFFSetField(out, tag, v1, v2)
#define	CopyField3(tag, v1, v2, v3) \
    if (TIFFGetField(in, tag, &v1, &v2, &v3)) TIFFSetField(out, tag, v1, v2, v3)
#define	CopyField4(tag, v1, v2, v3, v4) \
    if (TIFFGetField(in, tag, &v1, &v2, &v3, &v4)) TIFFSetField(out, tag, v1, v2, v3, v4)

static void
cpTag(TIFF* in, TIFF* out, uint16 tag, uint16 count, TIFFDataType type)
{
	switch (type) {
	case TIFF_SHORT:
		if (count == 1) {
			uint16 shortv;
			CopyField(tag, shortv);
		} else if (count == 2) {
			uint16 shortv1, shortv2;
			CopyField2(tag, shortv1, shortv2);
		} else if (count == 4) {
			uint16 *tr, *tg, *tb, *ta;
			CopyField4(tag, tr, tg, tb, ta);
		} else if (count == (uint16) -1) {
			uint16 shortv1;
			uint16* shortav;
			CopyField2(tag, shortv1, shortav);
		}
		break;
	case TIFF_LONG:
		{ uint32 longv;
		  CopyField(tag, longv);
		}
		break;
	case TIFF_RATIONAL:
		if (count == 1) {
			float floatv;
			CopyField(tag, floatv);
		} else if (count == (uint16) -1) {
			float* floatav;
			CopyField(tag, floatav);
		}
		break;
	case TIFF_ASCII:
		{ char* stringv;
		  CopyField(tag, stringv);
		}
		break;
	case TIFF_DOUBLE:
		if (count == 1) {
			double doublev;
			CopyField(tag, doublev);
		} else if (count == (uint16) -1) {
			double* doubleav;
			CopyField(tag, doubleav);
		}
		break;
          default:
                TIFFError(TIFFFileName(in),
                          "Data type %d is not supported, tag %d skipped.",
                          tag, type);
	}
}

static struct cpTag {
	uint16	tag;
	uint16	count;
	TIFFDataType type;
} tags[] = {
	{ TIFFTAG_SUBFILETYPE,		1, TIFF_LONG },
	{ TIFFTAG_THRESHHOLDING,	1, TIFF_SHORT },
	{ TIFFTAG_DOCUMENTNAME,		1, TIFF_ASCII },
	{ TIFFTAG_IMAGEDESCRIPTION,	1, TIFF_ASCII },
	{ TIFFTAG_MAKE,			1, TIFF_ASCII },
	{ TIFFTAG_MODEL,		1, TIFF_ASCII },
	{ TIFFTAG_MINSAMPLEVALUE,	1, TIFF_SHORT },
	{ TIFFTAG_MAXSAMPLEVALUE,	1, TIFF_SHORT },
	{ TIFFTAG_XRESOLUTION,		1, TIFF_RATIONAL },
	{ TIFFTAG_YRESOLUTION,		1, TIFF_RATIONAL },
	{ TIFFTAG_PAGENAME,		1, TIFF_ASCII },
	{ TIFFTAG_XPOSITION,		1, TIFF_RATIONAL },
	{ TIFFTAG_YPOSITION,		1, TIFF_RATIONAL },
	{ TIFFTAG_RESOLUTIONUNIT,	1, TIFF_SHORT },
	{ TIFFTAG_SOFTWARE,		1, TIFF_ASCII },
	{ TIFFTAG_DATETIME,		1, TIFF_ASCII },
	{ TIFFTAG_ARTIST,		1, TIFF_ASCII },
	{ TIFFTAG_HOSTCOMPUTER,		1, TIFF_ASCII },
	{ TIFFTAG_WHITEPOINT,		(uint16) -1, TIFF_RATIONAL },
	{ TIFFTAG_PRIMARYCHROMATICITIES,(uint16) -1,TIFF_RATIONAL },
	{ TIFFTAG_HALFTONEHINTS,	2, TIFF_SHORT },
	{ TIFFTAG_INKSET,		1, TIFF_SHORT },
	{ TIFFTAG_DOTRANGE,		2, TIFF_SHORT },
	{ TIFFTAG_TARGETPRINTER,	1, TIFF_ASCII },
	{ TIFFTAG_SAMPLEFORMAT,		1, TIFF_SHORT },
	{ TIFFTAG_YCBCRCOEFFICIENTS,	(uint16) -1,TIFF_RATIONAL },
	{ TIFFTAG_YCBCRSUBSAMPLING,	2, TIFF_SHORT },
	{ TIFFTAG_YCBCRPOSITIONING,	1, TIFF_SHORT },
	{ TIFFTAG_REFERENCEBLACKWHITE,	(uint16) -1,TIFF_RATIONAL },
	{ TIFFTAG_EXTRASAMPLES,		(uint16) -1, TIFF_SHORT },
	{ TIFFTAG_SMINSAMPLEVALUE,	1, TIFF_DOUBLE },
	{ TIFFTAG_SMAXSAMPLEVALUE,	1, TIFF_DOUBLE },
	{ TIFFTAG_STONITS,		1, TIFF_DOUBLE },
};
#define	NTAGS	(sizeof (tags) / sizeof (tags[0]))

#define	CopyTag(tag, count, type)	cpTag(in, out, tag, count, type)

typedef int (*copyFunc)
    (TIFF* in, TIFF* out, uint32 l, uint32 w, uint16 samplesperpixel);
static	copyFunc pickCopyFunc(TIFF*, TIFF*, uint16, uint16);

static int
tiffcp(TIFF* in, TIFF* out)
{
	uint16 bitspersample, samplesperpixel;
	copyFunc cf;
	uint32 width, length;
	struct cpTag* p;

	CopyField(TIFFTAG_IMAGEWIDTH, width);
	CopyField(TIFFTAG_IMAGELENGTH, length);
	CopyField(TIFFTAG_BITSPERSAMPLE, bitspersample);
	CopyField(TIFFTAG_SAMPLESPERPIXEL, samplesperpixel);
	if (compression != (uint16)-1)
		TIFFSetField(out, TIFFTAG_COMPRESSION, compression);
	else
		CopyField(TIFFTAG_COMPRESSION, compression);
	if (compression == COMPRESSION_JPEG) {
	    uint16 input_compression, input_photometric;

            if (TIFFGetField(in, TIFFTAG_COMPRESSION, &input_compression)
                 && input_compression == COMPRESSION_JPEG) {
                TIFFSetField(in, TIFFTAG_JPEGCOLORMODE, JPEGCOLORMODE_RGB);
            }
	    if (TIFFGetField(in, TIFFTAG_PHOTOMETRIC, &input_photometric)) {
		if(input_photometric == PHOTOMETRIC_RGB) {
			if (jpegcolormode == JPEGCOLORMODE_RGB)
		    		TIFFSetField(out, TIFFTAG_PHOTOMETRIC,
					     PHOTOMETRIC_YCBCR);
			else
		    		TIFFSetField(out, TIFFTAG_PHOTOMETRIC,
					     PHOTOMETRIC_RGB);
		} else
			TIFFSetField(out, TIFFTAG_PHOTOMETRIC,
				     input_photometric);
	    }
        }
	else if (compression == COMPRESSION_SGILOG
		 || compression == COMPRESSION_SGILOG24)
		TIFFSetField(out, TIFFTAG_PHOTOMETRIC,
		    samplesperpixel == 1 ?
			PHOTOMETRIC_LOGL : PHOTOMETRIC_LOGLUV);
	else
		CopyTag(TIFFTAG_PHOTOMETRIC, 1, TIFF_SHORT);
	if (fillorder != 0)
		TIFFSetField(out, TIFFTAG_FILLORDER, fillorder);
	else
		CopyTag(TIFFTAG_FILLORDER, 1, TIFF_SHORT);
	/*
	 * Will copy `Orientation' tag from input image
	 */
	TIFFGetFieldDefaulted(in, TIFFTAG_ORIENTATION, &orientation);
	switch (orientation) {
		case ORIENTATION_BOTRIGHT:
		case ORIENTATION_RIGHTBOT:	/* XXX */
			TIFFWarning(TIFFFileName(in), "using bottom-left orientation");
			orientation = ORIENTATION_BOTLEFT;
		/* fall thru... */
		case ORIENTATION_LEFTBOT:	/* XXX */
		case ORIENTATION_BOTLEFT:
			break;
		case ORIENTATION_TOPRIGHT:
		case ORIENTATION_RIGHTTOP:	/* XXX */
		default:
			TIFFWarning(TIFFFileName(in), "using top-left orientation");
			orientation = ORIENTATION_TOPLEFT;
		/* fall thru... */
		case ORIENTATION_LEFTTOP:	/* XXX */
		case ORIENTATION_TOPLEFT:
			break;
	}
	TIFFSetField(out, TIFFTAG_ORIENTATION, orientation);
	/*
	 * Choose tiles/strip for the output image according to
	 * the command line arguments (-tiles, -strips) and the
	 * structure of the input image.
	 */
	if (outtiled == -1)
		outtiled = TIFFIsTiled(in);
	if (outtiled) {
		/*
		 * Setup output file's tile width&height.  If either
		 * is not specified, use either the value from the
		 * input image or, if nothing is defined, use the
		 * library default.
		 */
		if (tilewidth == (uint32) -1)
			TIFFGetField(in, TIFFTAG_TILEWIDTH, &tilewidth);
		if (tilelength == (uint32) -1)
			TIFFGetField(in, TIFFTAG_TILELENGTH, &tilelength);
		TIFFDefaultTileSize(out, &tilewidth, &tilelength);
		TIFFSetField(out, TIFFTAG_TILEWIDTH, tilewidth);
		TIFFSetField(out, TIFFTAG_TILELENGTH, tilelength);
	} else {
		/*
		 * RowsPerStrip is left unspecified: use either the
		 * value from the input image or, if nothing is defined,
		 * use the library default.
		 */
		if (rowsperstrip == (uint32) 0) {
			if (!TIFFGetField(in, TIFFTAG_ROWSPERSTRIP,
					  &rowsperstrip)) {
				rowsperstrip =
					TIFFDefaultStripSize(out, rowsperstrip);
			}
			if (rowsperstrip > length)
				rowsperstrip = length;
		}
		else if (rowsperstrip == (uint32) -1)
			rowsperstrip = length;
		TIFFSetField(out, TIFFTAG_ROWSPERSTRIP, rowsperstrip);
	}
	if (config != (uint16) -1)
		TIFFSetField(out, TIFFTAG_PLANARCONFIG, config);
	else
		CopyField(TIFFTAG_PLANARCONFIG, config);
	if (samplesperpixel <= 4)
		CopyTag(TIFFTAG_TRANSFERFUNCTION, 4, TIFF_SHORT);
	CopyTag(TIFFTAG_COLORMAP, 4, TIFF_SHORT);
/* SMinSampleValue & SMaxSampleValue */
	switch (compression) {
	case COMPRESSION_JPEG:
		TIFFSetField(out, TIFFTAG_JPEGQUALITY, quality);
		TIFFSetField(out, TIFFTAG_JPEGCOLORMODE, jpegcolormode);
		break;
	case COMPRESSION_LZW:
	case COMPRESSION_ADOBE_DEFLATE:
	case COMPRESSION_DEFLATE:
		if (predictor != (uint16)-1)
			TIFFSetField(out, TIFFTAG_PREDICTOR, predictor);
		else
			CopyField(TIFFTAG_PREDICTOR, predictor);
		break;
	case COMPRESSION_CCITTFAX3:
	case COMPRESSION_CCITTFAX4:
		if (compression == COMPRESSION_CCITTFAX3) {
			if (g3opts != (uint32) -1)
				TIFFSetField(out, TIFFTAG_GROUP3OPTIONS,
				    g3opts);
			else
				CopyField(TIFFTAG_GROUP3OPTIONS, g3opts);
		} else
			CopyTag(TIFFTAG_GROUP4OPTIONS, 1, TIFF_LONG);
		CopyTag(TIFFTAG_BADFAXLINES, 1, TIFF_LONG);
		CopyTag(TIFFTAG_CLEANFAXDATA, 1, TIFF_LONG);
		CopyTag(TIFFTAG_CONSECUTIVEBADFAXLINES, 1, TIFF_LONG);
		CopyTag(TIFFTAG_FAXRECVPARAMS, 1, TIFF_LONG);
		CopyTag(TIFFTAG_FAXRECVTIME, 1, TIFF_LONG);
		CopyTag(TIFFTAG_FAXSUBADDRESS, 1, TIFF_ASCII);
		break;
	}
	{ uint32 len32;
	  void** data;
	  if (TIFFGetField(in, TIFFTAG_ICCPROFILE, &len32, &data))
		TIFFSetField(out, TIFFTAG_ICCPROFILE, len32, data);
	}
	{ uint16 ninks;
	  const char* inknames;
	  if (TIFFGetField(in, TIFFTAG_NUMBEROFINKS, &ninks)) {
		TIFFSetField(out, TIFFTAG_NUMBEROFINKS, ninks);
		if (TIFFGetField(in, TIFFTAG_INKNAMES, &inknames)) {
		    int inknameslen = strlen(inknames) + 1;
		    const char* cp = inknames;
		    while (ninks > 1) {
			    cp = strchr(cp, '\0');
			    if (cp) {
				    cp++;
				    inknameslen += (strlen(cp) + 1);
			    }
			    ninks--;
		    }
		    TIFFSetField(out, TIFFTAG_INKNAMES, inknameslen, inknames);
		}
	  }
	}
	{
	  unsigned short pg0, pg1;
	  if (TIFFGetField(in, TIFFTAG_PAGENUMBER, &pg0, &pg1)) {
		if (pageNum < 0) /* only one input file */
			TIFFSetField(out, TIFFTAG_PAGENUMBER, pg0, pg1);
		else 
			TIFFSetField(out, TIFFTAG_PAGENUMBER, pageNum++, 0);
	  }
	}

	for (p = tags; p < &tags[NTAGS]; p++)
		CopyTag(p->tag, p->count, p->type);

	cf = pickCopyFunc(in, out, bitspersample, samplesperpixel);
	return (cf ? (*cf)(in, out, length, width, samplesperpixel) : FALSE);
}

/*
 * Copy Functions.
 */
#define	DECLAREcpFunc(x) \
static int x(TIFF* in, TIFF* out, \
    uint32 imagelength, uint32 imagewidth, tsample_t spp)

#define	DECLAREreadFunc(x) \
static int x(TIFF* in, \
    uint8* buf, uint32 imagelength, uint32 imagewidth, tsample_t spp)
typedef int (*readFunc)(TIFF*, uint8*, uint32, uint32, tsample_t);

#define	DECLAREwriteFunc(x) \
static int x(TIFF* out, \
    uint8* buf, uint32 imagelength, uint32 imagewidth, tsample_t spp)
typedef int (*writeFunc)(TIFF*, uint8*, uint32, uint32, tsample_t);

/*
 * Contig -> contig by scanline for rows/strip change.
 */
DECLAREcpFunc(cpContig2ContigByRow)
{
	tdata_t buf = _TIFFmalloc(TIFFScanlineSize(in));
	uint32 row;

	(void) imagewidth; (void) spp;
	for (row = 0; row < imagelength; row++) {
		if (TIFFReadScanline(in, buf, row, 0) < 0 && !ignore) {
			TIFFError(TIFFFileName(in),
				  "Error, can't read scanline %lu",
				  (unsigned long) row);
			goto bad;
		}
		if (TIFFWriteScanline(out, buf, row, 0) < 0) {
			TIFFError(TIFFFileName(out),
				  "Error, can't write scanline %lu",
				  (unsigned long) row);
			goto bad;
		}
	}
	_TIFFfree(buf);
	return 1;
bad:
	_TIFFfree(buf);
	return 0;
}


typedef void biasFn (void *image, void *bias, uint32 pixels);

#define subtract(bits) \
static void subtract##bits (void *i, void *b, uint32 pixels)\
{\
   uint##bits *image = i;\
   uint##bits *bias = b;\
   while (pixels--) {\
     *image = *image > *bias ? *image-*bias : 0;\
     image++, bias++; \
   } \
}

subtract(8)
subtract(16)
subtract(32)

static biasFn *lineSubtractFn (unsigned bits)
{
    switch (bits) {
      case  8:  return subtract8;
      case 16:  return subtract16;
      case 32:  return subtract32;
    }
    return NULL;
}

/*
 * Contig -> contig by scanline while subtracting a bias image.
 */
DECLAREcpFunc(cpBiasedContig2Contig)
{
	if (spp == 1) {
	  tsize_t biasSize = TIFFScanlineSize(bias);
	  tsize_t bufSize = TIFFScanlineSize(in);
	  tdata_t buf, biasBuf;
	  uint32 biasWidth = 0, biasLength = 0;
	  TIFFGetField(bias, TIFFTAG_IMAGEWIDTH, &biasWidth);
	  TIFFGetField(bias, TIFFTAG_IMAGELENGTH, &biasLength);
	  if (biasSize == bufSize && 
	      imagelength == biasLength && imagewidth == biasWidth) {
		uint16 sampleBits = 0;
		biasFn *subtractLine;
		TIFFGetField(in, TIFFTAG_BITSPERSAMPLE, &sampleBits);
		subtractLine = lineSubtractFn (sampleBits);
		if (subtractLine) {
			uint32 row;
			buf = _TIFFmalloc(bufSize);
			biasBuf = _TIFFmalloc(bufSize);
			for (row = 0; row < imagelength; row++) {
				if (TIFFReadScanline(in, buf, row, 0) < 0
				    && !ignore) {
					TIFFError(TIFFFileName(in),
					"Error, can't read scanline %lu",
					(unsigned long) row);
					goto bad;
				}
				if (TIFFReadScanline(bias, biasBuf, row, 0) < 0
				    && !ignore) {
					TIFFError(TIFFFileName(in),
					"Error, can't read biased scanline %lu",
					(unsigned long) row);
					goto bad;
				}
				subtractLine (buf, biasBuf, imagewidth);
				if (TIFFWriteScanline(out, buf, row, 0) < 0) {
					TIFFError(TIFFFileName(out),
					"Error, can't write scanline %lu",
					(unsigned long) row);
					goto bad;
				}
			}
		
			_TIFFfree(buf);
			_TIFFfree(biasBuf);
			TIFFSetDirectory(bias,
				TIFFCurrentDirectory(bias)); /* rewind */
			return 1;
bad:
			_TIFFfree(buf);
			_TIFFfree(biasBuf);
			return 0;
	    } else {
	      TIFFError(TIFFFileName(in),
			"No support for biasing %d bit pixels\n",
			sampleBits);
	      return 0;
	    }
	  }
	  TIFFError(TIFFFileName(in),
		    "Bias image %s,%d\nis not the same size as %s,%d\n",
		    TIFFFileName(bias), TIFFCurrentDirectory(bias),
		    TIFFFileName(in), TIFFCurrentDirectory(in));
	  return 0;
	} else {
	  TIFFError(TIFFFileName(in),
		    "Can't bias %s,%d as it has >1 Sample/Pixel\n",
		    TIFFFileName(in), TIFFCurrentDirectory(in));
	  return 0;
	}

}


/*
 * Strip -> strip for change in encoding.
 */
DECLAREcpFunc(cpDecodedStrips)
{
	tsize_t stripsize  = TIFFStripSize(in);
	tdata_t buf = _TIFFmalloc(stripsize);

	(void) imagewidth; (void) spp;
	if (buf) {
		tstrip_t s, ns = TIFFNumberOfStrips(in);
		uint32 row = 0;
		for (s = 0; s < ns; s++) {
			tsize_t cc = (row + rowsperstrip > imagelength) ?
			    TIFFVStripSize(in, imagelength - row) : stripsize;
			if (TIFFReadEncodedStrip(in, s, buf, cc) < 0
			    && !ignore) {
				TIFFError(TIFFFileName(in),
					  "Error, can't read strip %lu",
					  (unsigned long) s);
				goto bad;
			}
			if (TIFFWriteEncodedStrip(out, s, buf, cc) < 0) {
				TIFFError(TIFFFileName(out),
					  "Error, can't write strip %lu",
					  (unsigned long) s);
				goto bad;
			}
			row += rowsperstrip;
		}
		_TIFFfree(buf);
		return 1;
	} else {
		TIFFError(TIFFFileName(in),
			  "Error, can't allocate memory buffer of size %lu "
			  "to read strips", (unsigned long) stripsize);
		return 0;
	}

bad:
	_TIFFfree(buf);
	return 0;
}

/*
 * Separate -> separate by row for rows/strip change.
 */
DECLAREcpFunc(cpSeparate2SeparateByRow)
{
	tdata_t buf = _TIFFmalloc(TIFFScanlineSize(in));
	uint32 row;
	tsample_t s;

	(void) imagewidth;
	for (s = 0; s < spp; s++) {
		for (row = 0; row < imagelength; row++) {
			if (TIFFReadScanline(in, buf, row, s) < 0 && !ignore) {
				TIFFError(TIFFFileName(in),
					  "Error, can't read scanline %lu",
					  (unsigned long) row);
				goto bad;
			}
			if (TIFFWriteScanline(out, buf, row, s) < 0) {
				TIFFError(TIFFFileName(out),
					  "Error, can't write scanline %lu",
					  (unsigned long) row);
				goto bad;
			}
		}
	}
	_TIFFfree(buf);
	return 1;
bad:
	_TIFFfree(buf);
	return 0;
}

/*
 * Contig -> separate by row.
 */
DECLAREcpFunc(cpContig2SeparateByRow)
{
	tdata_t inbuf = _TIFFmalloc(TIFFScanlineSize(in));
	tdata_t outbuf = _TIFFmalloc(TIFFScanlineSize(out));
	register uint8 *inp, *outp;
	register uint32 n;
	uint32 row;
	tsample_t s;

	/* unpack channels */
	for (s = 0; s < spp; s++) {
		for (row = 0; row < imagelength; row++) {
			if (TIFFReadScanline(in, inbuf, row, 0) < 0
			    && !ignore) {
				TIFFError(TIFFFileName(in),
					  "Error, can't read scanline %lu",
					  (unsigned long) row);
				goto bad;
			}
			inp = ((uint8*)inbuf) + s;
			outp = (uint8*)outbuf;
			for (n = imagewidth; n-- > 0;) {
				*outp++ = *inp;
				inp += spp;
			}
			if (TIFFWriteScanline(out, outbuf, row, s) < 0) {
				TIFFError(TIFFFileName(out),
					  "Error, can't write scanline %lu",
					  (unsigned long) row);
				goto bad;
			}
		}
	}
	if (inbuf) _TIFFfree(inbuf);
	if (outbuf) _TIFFfree(outbuf);
	return 1;
bad:
	if (inbuf) _TIFFfree(inbuf);
	if (outbuf) _TIFFfree(outbuf);
	return 0;
}

/*
 * Separate -> contig by row.
 */
DECLAREcpFunc(cpSeparate2ContigByRow)
{
	tdata_t inbuf = _TIFFmalloc(TIFFScanlineSize(in));
	tdata_t outbuf = _TIFFmalloc(TIFFScanlineSize(out));
	register uint8 *inp, *outp;
	register uint32 n;
	uint32 row;
	tsample_t s;

	for (row = 0; row < imagelength; row++) {
		/* merge channels */
		for (s = 0; s < spp; s++) {
			if (TIFFReadScanline(in, inbuf, row, s) < 0
			    && !ignore) {
				TIFFError(TIFFFileName(in),
					  "Error, can't read scanline %lu",
					  (unsigned long) row);
				goto bad;
			}
			inp = (uint8*)inbuf;
			outp = ((uint8*)outbuf) + s;
			for (n = imagewidth; n-- > 0;) {
				*outp = *inp++;
				outp += spp;
			}
		}
		if (TIFFWriteScanline(out, outbuf, row, 0) < 0) {
			TIFFError(TIFFFileName(out),
				  "Error, can't write scanline %lu",
				  (unsigned long) row);
			goto bad;
		}
	}
	if (inbuf) _TIFFfree(inbuf);
	if (outbuf) _TIFFfree(outbuf);
	return 1;
bad:
	if (inbuf) _TIFFfree(inbuf);
	if (outbuf) _TIFFfree(outbuf);
	return 0;
}

static void
cpStripToTile(uint8* out, uint8* in,
	uint32 rows, uint32 cols, int outskew, int inskew)
{
	while (rows-- > 0) {
		uint32 j = cols;
		while (j-- > 0)
			*out++ = *in++;
		out += outskew;
		in += inskew;
	}
}

static void
cpContigBufToSeparateBuf(uint8* out, uint8* in,
           uint32 rows, uint32 cols, int outskew, int inskew, tsample_t spp,
           int bytes_per_sample )
{
	while (rows-- > 0) {
		uint32 j = cols;
		while (j-- > 0)
                {
                        int n = bytes_per_sample;

                        while( n-- ) {
                            *out++ = *in++;
                        }
                        in += (spp-1) * bytes_per_sample;
                }
		out += outskew;
		in += inskew;
	}
}

static void
cpSeparateBufToContigBuf(uint8* out, uint8* in,
	uint32 rows, uint32 cols, int outskew, int inskew, tsample_t spp,
                         int bytes_per_sample)
{
	while (rows-- > 0) {
		uint32 j = cols;
		while (j-- > 0) {
                        int n = bytes_per_sample;

                        while( n-- ) {
                                *out++ = *in++;
                        }
                        out += (spp-1)*bytes_per_sample;
                }
		out += outskew;
		in += inskew;
	}
}

static int
cpImage(TIFF* in, TIFF* out, readFunc fin, writeFunc fout,
	uint32 imagelength, uint32 imagewidth, tsample_t spp)
{
	int status = 0;
	tdata_t buf = NULL;
	tsize_t scanlinesize = TIFFRasterScanlineSize(in);
        tsize_t bytes = scanlinesize * (tsize_t)imagelength;                                      
        /*
         * XXX: Check for integer overflow.
         */
        if (scanlinesize
	    && imagelength
	    && bytes / (tsize_t)imagelength == scanlinesize) {
                buf = _TIFFmalloc(bytes);
		if (buf) {
			if ((*fin)(in, (uint8*)buf, imagelength, 
				   imagewidth, spp)) {
				status = (*fout)(out, (uint8*)buf,
						 imagelength, imagewidth, spp);
			}
			_TIFFfree(buf);
		} else {
			TIFFError(TIFFFileName(in),
				"Error, can't allocate space for image buffer");
		}
	} else {
		TIFFError(TIFFFileName(in), "Error, no space for image buffer");
	}

	return status;
}

DECLAREreadFunc(readContigStripsIntoBuffer)
{
	tsize_t scanlinesize = TIFFScanlineSize(in);
	uint8* bufp = buf;
	uint32 row;

	(void) imagewidth; (void) spp;
	for (row = 0; row < imagelength; row++) {
		if (TIFFReadScanline(in, (tdata_t) bufp, row, 0) < 0
		    && !ignore) {
			TIFFError(TIFFFileName(in),
				  "Error, can't read scanline %lu",
				  (unsigned long) row);
			return 0;
		}
		bufp += scanlinesize;
	}

	return 1;
}

DECLAREreadFunc(readSeparateStripsIntoBuffer)
{
	int status = 1;
	tsize_t scanlinesize = TIFFScanlineSize(in);
	tdata_t scanline = _TIFFmalloc(scanlinesize);
	if (!scanlinesize)
		return 0;

	(void) imagewidth;
	if (scanline) {
		uint8* bufp = (uint8*) buf;
		uint32 row;
		tsample_t s;
		for (row = 0; row < imagelength; row++) {
			/* merge channels */
			for (s = 0; s < spp; s++) {
				uint8* bp = bufp + s;
				tsize_t n = scanlinesize;
                                uint8* sbuf = scanline;

				if (TIFFReadScanline(in, scanline, row, s) < 0
				    && !ignore) {
					TIFFError(TIFFFileName(in),
					"Error, can't read scanline %lu",
					(unsigned long) row);
					status = 0;
					goto done;
				}
				while (n-- > 0)
					*bp = *sbuf++, bp += spp;
			}
			bufp += scanlinesize * spp;
		}
	}

done:
	_TIFFfree(scanline);
	return status;
}

DECLAREreadFunc(readContigTilesIntoBuffer)
{
	int status = 1;
	tdata_t tilebuf = _TIFFmalloc(TIFFTileSize(in));
	uint32 imagew = TIFFScanlineSize(in);
	uint32 tilew  = TIFFTileRowSize(in);
	int iskew = imagew - tilew;
	uint8* bufp = (uint8*) buf;
	uint32 tw, tl;
	uint32 row;

	(void) spp;
	if (tilebuf == 0)
		return 0;
	(void) TIFFGetField(in, TIFFTAG_TILEWIDTH, &tw);
	(void) TIFFGetField(in, TIFFTAG_TILELENGTH, &tl);
        
	for (row = 0; row < imagelength; row += tl) {
		uint32 nrow = (row+tl > imagelength) ? imagelength-row : tl;
		uint32 colb = 0;
		uint32 col;

		for (col = 0; col < imagewidth; col += tw) {
			if (TIFFReadTile(in, tilebuf, col, row, 0, 0) < 0
			    && !ignore) {
				TIFFError(TIFFFileName(in),
					  "Error, can't read tile at %lu %lu",
					  (unsigned long) col,
					  (unsigned long) row);
				status = 0;
				goto done;
			}
			if (colb + tilew > imagew) {
				uint32 width = imagew - colb;
				uint32 oskew = tilew - width;
				cpStripToTile(bufp + colb,
                                              tilebuf, nrow, width,
                                              oskew + iskew, oskew );
			} else
				cpStripToTile(bufp + colb,
                                              tilebuf, nrow, tilew,
                                              iskew, 0);
			colb += tilew;
		}
		bufp += imagew * nrow;
	}
done:
	_TIFFfree(tilebuf);
	return status;
}

DECLAREreadFunc(readSeparateTilesIntoBuffer)
{
	int status = 1;
	uint32 imagew = TIFFRasterScanlineSize(in);
	uint32 tilew = TIFFTileRowSize(in);
	int iskew  = imagew - tilew*spp;
	tdata_t tilebuf = _TIFFmalloc(TIFFTileSize(in));
	uint8* bufp = (uint8*) buf;
	uint32 tw, tl;
	uint32 row;
        uint16 bps, bytes_per_sample;

	if (tilebuf == 0)
		return 0;
	(void) TIFFGetField(in, TIFFTAG_TILEWIDTH, &tw);
	(void) TIFFGetField(in, TIFFTAG_TILELENGTH, &tl);
	(void) TIFFGetField(in, TIFFTAG_BITSPERSAMPLE, &bps);
        assert( bps % 8 == 0 );
        bytes_per_sample = bps/8;

	for (row = 0; row < imagelength; row += tl) {
		uint32 nrow = (row+tl > imagelength) ? imagelength-row : tl;
		uint32 colb = 0;
		uint32 col;

		for (col = 0; col < imagewidth; col += tw) {
			tsample_t s;

			for (s = 0; s < spp; s++) {
				if (TIFFReadTile(in, tilebuf, col, row, 0, s) < 0
				    && !ignore) {
					TIFFError(TIFFFileName(in),
					  "Error, can't read tile at %lu %lu, "
					  "sample %lu",
					  (unsigned long) col,
					  (unsigned long) row,
					  (unsigned long) s);
					status = 0;
					goto done;
				}
				/*
				 * Tile is clipped horizontally.  Calculate
				 * visible portion and skewing factors.
				 */
				if (colb + tilew*spp > imagew) {
					uint32 width = imagew - colb;
					int oskew = tilew*spp - width;
					cpSeparateBufToContigBuf(
                                            bufp+colb+s*bytes_per_sample,
					    tilebuf, nrow,
                                            width/(spp*bytes_per_sample),
					    oskew + iskew,
                                            oskew/spp, spp,
                                            bytes_per_sample);
				} else
					cpSeparateBufToContigBuf(
                                            bufp+colb+s*bytes_per_sample,
					    tilebuf, nrow, tw,
					    iskew, 0, spp,
                                            bytes_per_sample);
			}
			colb += tilew*spp;
		}
		bufp += imagew * nrow;
	}
done:
	_TIFFfree(tilebuf);
	return status;
}

DECLAREwriteFunc(writeBufferToContigStrips)
{
	uint32 row, rowsperstrip;
	tstrip_t strip = 0;

	(void) imagewidth; (void) spp;
	(void) TIFFGetFieldDefaulted(out, TIFFTAG_ROWSPERSTRIP, &rowsperstrip);
	for (row = 0; row < imagelength; row += rowsperstrip) {
		uint32 nrows = (row+rowsperstrip > imagelength) ?
		    imagelength-row : rowsperstrip;
		tsize_t stripsize = TIFFVStripSize(out, nrows);
		if (TIFFWriteEncodedStrip(out, strip++, buf, stripsize) < 0) {
			TIFFError(TIFFFileName(out),
				  "Error, can't write strip %lu", strip - 1);
			return 0;
		}
		buf += stripsize;
	}
	return 1;
}

DECLAREwriteFunc(writeBufferToSeparateStrips)
{
	uint32 rowsize = imagewidth * spp;
	uint32 rowsperstrip;
	tdata_t obuf = _TIFFmalloc(TIFFStripSize(out));
	tstrip_t strip = 0;
	tsample_t s;

	if (obuf == NULL)
		return (0);
	(void) TIFFGetFieldDefaulted(out, TIFFTAG_ROWSPERSTRIP, &rowsperstrip);
	for (s = 0; s < spp; s++) {
		uint32 row;
		for (row = 0; row < imagelength; row += rowsperstrip) {
			uint32 nrows = (row+rowsperstrip > imagelength) ?
			    imagelength-row : rowsperstrip;
			tsize_t stripsize = TIFFVStripSize(out, nrows);

			cpContigBufToSeparateBuf(
			    obuf, (uint8*) buf + row*rowsize + s, 
			    nrows, imagewidth, 0, 0, spp, 1);
			if (TIFFWriteEncodedStrip(out, strip++, obuf, stripsize) < 0) {
				TIFFError(TIFFFileName(out),
					  "Error, can't write strip %lu",
					  strip - 1);
				_TIFFfree(obuf);
				return 0;
			}
		}
	}
	_TIFFfree(obuf);
	return 1;

}

DECLAREwriteFunc(writeBufferToContigTiles)
{
	uint32 imagew = TIFFScanlineSize(out);
	uint32 tilew  = TIFFTileRowSize(out);
	int iskew = imagew - tilew;
	tdata_t obuf = _TIFFmalloc(TIFFTileSize(out));
	uint8* bufp = (uint8*) buf;
	uint32 tl, tw;
	uint32 row;

	(void) spp;
	if (obuf == NULL)
		return 0;
	(void) TIFFGetField(out, TIFFTAG_TILELENGTH, &tl);
	(void) TIFFGetField(out, TIFFTAG_TILEWIDTH, &tw);
	for (row = 0; row < imagelength; row += tilelength) {
		uint32 nrow = (row+tl > imagelength) ? imagelength-row : tl;
		uint32 colb = 0;
		uint32 col;

		for (col = 0; col < imagewidth; col += tw) {
			/*
			 * Tile is clipped horizontally.  Calculate
			 * visible portion and skewing factors.
			 */
			if (colb + tilew > imagew) {
				uint32 width = imagew - colb;
				int oskew = tilew - width;
				cpStripToTile(obuf, bufp + colb, nrow, width,
				    oskew, oskew + iskew);
			} else
				cpStripToTile(obuf, bufp + colb, nrow, tilew,
				    0, iskew);
			if (TIFFWriteTile(out, obuf, col, row, 0, 0) < 0) {
				TIFFError(TIFFFileName(out),
					  "Error, can't write tile at %lu %lu",
					  (unsigned long) col,
					  (unsigned long) row);
				_TIFFfree(obuf);
				return 0;
			}
			colb += tilew;
		}
		bufp += nrow * imagew;
	}
	_TIFFfree(obuf);
	return 1;
}

DECLAREwriteFunc(writeBufferToSeparateTiles)
{
	uint32 imagew = TIFFScanlineSize(out);
	tsize_t tilew  = TIFFTileRowSize(out);
	uint32 iimagew = TIFFRasterScanlineSize(out);
	int iskew = iimagew - tilew*spp;
	tdata_t obuf = _TIFFmalloc(TIFFTileSize(out));
	uint8* bufp = (uint8*) buf;
	uint32 tl, tw;
	uint32 row;
        uint16 bps, bytes_per_sample;

	if (obuf == NULL)
		return 0;
	(void) TIFFGetField(out, TIFFTAG_TILELENGTH, &tl);
	(void) TIFFGetField(out, TIFFTAG_TILEWIDTH, &tw);
	(void) TIFFGetField(out, TIFFTAG_BITSPERSAMPLE, &bps);
        assert( bps % 8 == 0 );
        bytes_per_sample = bps/8;
        
	for (row = 0; row < imagelength; row += tl) {
		uint32 nrow = (row+tl > imagelength) ? imagelength-row : tl;
		uint32 colb = 0;
		uint32 col;

		for (col = 0; col < imagewidth; col += tw) {
			tsample_t s;
			for (s = 0; s < spp; s++) {
				/*
				 * Tile is clipped horizontally.  Calculate
				 * visible portion and skewing factors.
				 */
				if (colb + tilew > imagew) {
					uint32 width = (imagew - colb);
					int oskew = tilew - width;

					cpContigBufToSeparateBuf(obuf,
					    bufp + (colb*spp) + s,
					    nrow, width/bytes_per_sample,
					    oskew, (oskew*spp)+iskew, spp,
                                            bytes_per_sample);
				} else
					cpContigBufToSeparateBuf(obuf,
					    bufp + (colb*spp) + s,
					    nrow, tilewidth,
					    0, iskew, spp,
                                            bytes_per_sample);
				if (TIFFWriteTile(out, obuf, col, row, 0, s) < 0) {
					TIFFError(TIFFFileName(out),
					"Error, can't write tile at %lu %lu "
					"sample %lu",
					(unsigned long) col,
					(unsigned long) row,
					(unsigned long) s);
					_TIFFfree(obuf);
					return 0;
				}
			}
			colb += tilew;
		}
		bufp += nrow * iimagew;
	}
	_TIFFfree(obuf);
	return 1;
}

/*
 * Contig strips -> contig tiles.
 */
DECLAREcpFunc(cpContigStrips2ContigTiles)
{
	return cpImage(in, out,
	    readContigStripsIntoBuffer,
	    writeBufferToContigTiles,
	    imagelength, imagewidth, spp);
}

/*
 * Contig strips -> separate tiles.
 */
DECLAREcpFunc(cpContigStrips2SeparateTiles)
{
	return cpImage(in, out,
	    readContigStripsIntoBuffer,
	    writeBufferToSeparateTiles,
	    imagelength, imagewidth, spp);
}

/*
 * Separate strips -> contig tiles.
 */
DECLAREcpFunc(cpSeparateStrips2ContigTiles)
{
	return cpImage(in, out,
	    readSeparateStripsIntoBuffer,
	    writeBufferToContigTiles,
	    imagelength, imagewidth, spp);
}

/*
 * Separate strips -> separate tiles.
 */
DECLAREcpFunc(cpSeparateStrips2SeparateTiles)
{
	return cpImage(in, out,
	    readSeparateStripsIntoBuffer,
	    writeBufferToSeparateTiles,
	    imagelength, imagewidth, spp);
}

/*
 * Contig strips -> contig tiles.
 */
DECLAREcpFunc(cpContigTiles2ContigTiles)
{
	return cpImage(in, out,
	    readContigTilesIntoBuffer,
	    writeBufferToContigTiles,
	    imagelength, imagewidth, spp);
}

/*
 * Contig tiles -> separate tiles.
 */
DECLAREcpFunc(cpContigTiles2SeparateTiles)
{
	return cpImage(in, out,
	    readContigTilesIntoBuffer,
	    writeBufferToSeparateTiles,
	    imagelength, imagewidth, spp);
}

/*
 * Separate tiles -> contig tiles.
 */
DECLAREcpFunc(cpSeparateTiles2ContigTiles)
{
	return cpImage(in, out,
	    readSeparateTilesIntoBuffer,
	    writeBufferToContigTiles,
	    imagelength, imagewidth, spp);
}

/*
 * Separate tiles -> separate tiles (tile dimension change).
 */
DECLAREcpFunc(cpSeparateTiles2SeparateTiles)
{
	return cpImage(in, out,
	    readSeparateTilesIntoBuffer,
	    writeBufferToSeparateTiles,
	    imagelength, imagewidth, spp);
}

/*
 * Contig tiles -> contig tiles (tile dimension change).
 */
DECLAREcpFunc(cpContigTiles2ContigStrips)
{
	return cpImage(in, out,
	    readContigTilesIntoBuffer,
	    writeBufferToContigStrips,
	    imagelength, imagewidth, spp);
}

/*
 * Contig tiles -> separate strips.
 */
DECLAREcpFunc(cpContigTiles2SeparateStrips)
{
	return cpImage(in, out,
	    readContigTilesIntoBuffer,
	    writeBufferToSeparateStrips,
	    imagelength, imagewidth, spp);
}

/*
 * Separate tiles -> contig strips.
 */
DECLAREcpFunc(cpSeparateTiles2ContigStrips)
{
	return cpImage(in, out,
	    readSeparateTilesIntoBuffer,
	    writeBufferToContigStrips,
	    imagelength, imagewidth, spp);
}

/*
 * Separate tiles -> separate strips.
 */
DECLAREcpFunc(cpSeparateTiles2SeparateStrips)
{
	return cpImage(in, out,
	    readSeparateTilesIntoBuffer,
	    writeBufferToSeparateStrips,
	    imagelength, imagewidth, spp);
}

/*
 * Select the appropriate copy function to use.
 */
static copyFunc
pickCopyFunc(TIFF* in, TIFF* out, uint16 bitspersample, uint16 samplesperpixel)
{
	uint16 shortv;
	uint32 w, l, tw, tl;
	int bychunk;

	(void) TIFFGetField(in, TIFFTAG_PLANARCONFIG, &shortv);
	if (shortv != config && bitspersample != 8 && samplesperpixel > 1) {
		fprintf(stderr,
"%s: Cannot handle different planar configuration w/ bits/sample != 8\n",
		    TIFFFileName(in));
		return (NULL);
	}
	TIFFGetField(in, TIFFTAG_IMAGEWIDTH, &w);
	TIFFGetField(in, TIFFTAG_IMAGELENGTH, &l);
        if (!(TIFFIsTiled(out) || TIFFIsTiled(in))) {
	    uint32 irps = (uint32) -1L;
	    TIFFGetField(in, TIFFTAG_ROWSPERSTRIP, &irps);
            /* if biased, force decoded copying to allow image subtraction */
	    bychunk = !bias && (rowsperstrip == irps);
	}else{  /* either in or out is tiled */
            if (bias) {
                  fprintf(stderr,
"%s: Cannot handle tiled configuration w/bias image\n",
                  TIFFFileName(in));
                  return (NULL);
            }
	    if (TIFFIsTiled(out)) {
		if (!TIFFGetField(in, TIFFTAG_TILEWIDTH, &tw))
			tw = w;
		if (!TIFFGetField(in, TIFFTAG_TILELENGTH, &tl))
			tl = l;
		bychunk = (tw == tilewidth && tl == tilelength);
	    } else {  /* out's not, so in must be tiled */
		TIFFGetField(in, TIFFTAG_TILEWIDTH, &tw);
		TIFFGetField(in, TIFFTAG_TILELENGTH, &tl);
		bychunk = (tw == w && tl == rowsperstrip);
            }
	}
#define	T 1
#define	F 0
#define pack(a,b,c,d,e)	((long)(((a)<<11)|((b)<<3)|((c)<<2)|((d)<<1)|(e)))
	switch(pack(shortv,config,TIFFIsTiled(in),TIFFIsTiled(out),bychunk)) {
/* Strips -> Tiles */
	case pack(PLANARCONFIG_CONTIG,   PLANARCONFIG_CONTIG,   F,T,F):
	case pack(PLANARCONFIG_CONTIG,   PLANARCONFIG_CONTIG,   F,T,T):
		return cpContigStrips2ContigTiles;
	case pack(PLANARCONFIG_CONTIG,   PLANARCONFIG_SEPARATE, F,T,F):
	case pack(PLANARCONFIG_CONTIG,   PLANARCONFIG_SEPARATE, F,T,T):
		return cpContigStrips2SeparateTiles;
        case pack(PLANARCONFIG_SEPARATE, PLANARCONFIG_CONTIG,   F,T,F):
        case pack(PLANARCONFIG_SEPARATE, PLANARCONFIG_CONTIG,   F,T,T):
		return cpSeparateStrips2ContigTiles;
	case pack(PLANARCONFIG_SEPARATE, PLANARCONFIG_SEPARATE, F,T,F):
	case pack(PLANARCONFIG_SEPARATE, PLANARCONFIG_SEPARATE, F,T,T):
		return cpSeparateStrips2SeparateTiles;
/* Tiles -> Tiles */
	case pack(PLANARCONFIG_CONTIG,   PLANARCONFIG_CONTIG,   T,T,F):
	case pack(PLANARCONFIG_CONTIG,   PLANARCONFIG_CONTIG,   T,T,T):
		return cpContigTiles2ContigTiles;
	case pack(PLANARCONFIG_CONTIG,   PLANARCONFIG_SEPARATE, T,T,F):
	case pack(PLANARCONFIG_CONTIG,   PLANARCONFIG_SEPARATE, T,T,T):
		return cpContigTiles2SeparateTiles;
        case pack(PLANARCONFIG_SEPARATE, PLANARCONFIG_CONTIG,   T,T,F):
        case pack(PLANARCONFIG_SEPARATE, PLANARCONFIG_CONTIG,   T,T,T):
		return cpSeparateTiles2ContigTiles;
	case pack(PLANARCONFIG_SEPARATE, PLANARCONFIG_SEPARATE, T,T,F):
	case pack(PLANARCONFIG_SEPARATE, PLANARCONFIG_SEPARATE, T,T,T):
		return cpSeparateTiles2SeparateTiles;
/* Tiles -> Strips */
	case pack(PLANARCONFIG_CONTIG,   PLANARCONFIG_CONTIG,   T,F,F):
	case pack(PLANARCONFIG_CONTIG,   PLANARCONFIG_CONTIG,   T,F,T):
		return cpContigTiles2ContigStrips;
	case pack(PLANARCONFIG_CONTIG,   PLANARCONFIG_SEPARATE, T,F,F):
	case pack(PLANARCONFIG_CONTIG,   PLANARCONFIG_SEPARATE, T,F,T):
		return cpContigTiles2SeparateStrips;
        case pack(PLANARCONFIG_SEPARATE, PLANARCONFIG_CONTIG,   T,F,F):
        case pack(PLANARCONFIG_SEPARATE, PLANARCONFIG_CONTIG,   T,F,T):
		return cpSeparateTiles2ContigStrips;
	case pack(PLANARCONFIG_SEPARATE, PLANARCONFIG_SEPARATE, T,F,F):
	case pack(PLANARCONFIG_SEPARATE, PLANARCONFIG_SEPARATE, T,F,T):
		return cpSeparateTiles2SeparateStrips;
/* Strips -> Strips */
	case pack(PLANARCONFIG_CONTIG,   PLANARCONFIG_CONTIG,   F,F,F):
		return bias ? cpBiasedContig2Contig : cpContig2ContigByRow;
	case pack(PLANARCONFIG_CONTIG,   PLANARCONFIG_CONTIG,   F,F,T):
		return cpDecodedStrips;
	case pack(PLANARCONFIG_CONTIG, PLANARCONFIG_SEPARATE,   F,F,F):
	case pack(PLANARCONFIG_CONTIG, PLANARCONFIG_SEPARATE,   F,F,T):
		return cpContig2SeparateByRow;
	case pack(PLANARCONFIG_SEPARATE, PLANARCONFIG_CONTIG,   F,F,F):
	case pack(PLANARCONFIG_SEPARATE, PLANARCONFIG_CONTIG,   F,F,T):
		return cpSeparate2ContigByRow;
	case pack(PLANARCONFIG_SEPARATE, PLANARCONFIG_SEPARATE, F,F,F):
	case pack(PLANARCONFIG_SEPARATE, PLANARCONFIG_SEPARATE, F,F,T):
		return cpSeparate2SeparateByRow;
	}
#undef pack
#undef F
#undef T
	fprintf(stderr, "tiffcrop: %s: Don't know how to copy/convert image.\n",
	    TIFFFileName(in));
	return (NULL);
}

static int 
initCropMasks (struct crop_mask *cps)
   {
   int i;

   cps->crop_mode = CROP_NONE;
   cps->res_unit  = RESUNIT_NONE;
   cps->edge_ref  = EDGE_TOP;
   cps->width = 0;
   cps->length = 0;
   for (i = 0; i < 4; i++)
     cps->margins[i] = 0.0;
   cps->bufftotal = (uint32)0;
   cps->combined_width = (uint32)0;
   cps->combined_length = (uint32)0;
   cps->rotation = (uint16)0;
   cps->mirror   = (uint16)0;
   cps->invert   = (uint16)0;
   cps->zones = 0;
   for (i = 0; i < MAX_ZONES; i++)
     {
     cps->zonelist[i].position = 0;
     cps->zonelist[i].total = 0;
     cps->zonelist[i].x1 = (uint32)0;
     cps->zonelist[i].x2 = (uint32)0;
     cps->zonelist[i].y1 = (uint32)0;
     cps->zonelist[i].y2 = (uint32)0;
     }

   return (0);
   }

/* Compute offsets into the image for margins */
static int
computeOffsets(uint16 res_unit, float xres, float yres, uint32 img_width, uint32 img_length, 
	       struct crop_mask *crop, struct offset *off)
  {
  uint32 tmargin, bmargin, lmargin, rmargin;
  uint32 startx, endx;   /* offsets of first and last columns to extract */
  uint32 starty, endy;   /* offsets of first and last row to extract */
  uint32 width, length, crop_width, crop_length;
  double scale;

  if (res_unit != RESUNIT_INCH && res_unit != RESUNIT_CENTIMETER)
    {
    xres = 1.0;
    yres = 1.0;
    if ((crop->res_unit != RESUNIT_NONE) && ((crop->crop_mode & CROP_MARGINS) ||
	(crop->crop_mode & CROP_LENGTH) || (crop->crop_mode & CROP_WIDTH)))
      {
      TIFFError("computeOffsets", "Cannot compute margins or fixed size sections without image resolution");
      TIFFError("computeOffsets", "Specify units in pixels and try again");
      return (-1);
      }
    }
  /* Translate user units to image units */
  scale = 1.0;
  switch (crop->res_unit) {
    case RESUNIT_CENTIMETER:
         if (res_unit == RESUNIT_INCH)
	   scale = 1.0/2.54;
	 break;
    case RESUNIT_INCH:
	 if (res_unit == RESUNIT_CENTIMETER)
	     scale = 2.54;
	 break;
    case RESUNIT_NONE: /* Dimensions in pixels */
    default:
    break;
    }

#ifdef DEBUG
  fprintf (stderr, "Scale: %f  Requested resunit %s, Image resunit %s\n\n",
           scale, (crop->res_unit == RESUNIT_INCH) ? "inch" :
	   ((crop->res_unit == RESUNIT_CENTIMETER) ? "centimeter" : "pixel"),
           (res_unit == RESUNIT_INCH) ? "inch" :
	   ((res_unit == RESUNIT_CENTIMETER) ? "centimeter" : "pixel"));
#endif
  
  /* Convert crop margins into offsets into image */
  if (crop->crop_mode & CROP_MARGINS)
    {
    if (crop->res_unit != RESUNIT_INCH && crop->res_unit != RESUNIT_CENTIMETER)
      { /* User specified pixels as reference unit */
      tmargin = (uint32)(crop->margins[0]);
      lmargin = (uint32)(crop->margins[1]);
      bmargin = (uint32)(crop->margins[2]);
      rmargin = (uint32)(crop->margins[3]);
      }
    else
      { /* inches or centimeters specified */
      tmargin = (uint32)(crop->margins[0] * scale * yres);
      lmargin = (uint32)(crop->margins[1] * scale * xres);
      bmargin = (uint32)(crop->margins[2] * scale * yres);
      rmargin = (uint32)(crop->margins[3] * scale * xres);
      }

    if ((lmargin + rmargin) > img_width)
      {
      TIFFError("computeOffsets", "Combined left and right margins exceed image width");
      lmargin = (uint32) 0;
      rmargin = (uint32) 0;
      return (-1);
      }
    if ((tmargin + bmargin) > img_length)
      {
      TIFFError("computeOffsets", "Combined top and bottom margins exceed image length"); 
      tmargin = (uint32) 0; 
      bmargin = (uint32) 0;
      return (-1);
      }
    }
  else
    { /* no margins requested */
    tmargin = (uint32) 0;
    lmargin = (uint32) 0;
    bmargin = (uint32) 0;
    rmargin = (uint32) 0;
    }

  if (crop->crop_mode & CROP_WIDTH)
    width = (uint32)(crop->width * scale * xres);
  else
    width = img_width - lmargin - rmargin;

  if (crop->crop_mode & CROP_LENGTH)
    length  = (uint32)(crop->length * scale * yres);
  else
    length = img_length - tmargin - bmargin;

  off->tmargin = tmargin;
  off->bmargin = bmargin;
  off->lmargin = lmargin;
  off->rmargin = rmargin;

  /* Calculate regions defined by margins, width, and length. 
   * Coordinates expressed as 1 to imagewidth, imagelength */
  switch (crop->edge_ref) {
    case EDGE_BOTTOM:          startx = lmargin;
         if ((startx + width) >= (img_width - rmargin))
           endx = img_width - rmargin - 1;
         else
           endx = startx + width;

         endy = img_length - bmargin - 1;
         if ((endy - length) <= tmargin)
           starty = tmargin;
         else
           starty = endy - length - 1;
         break;
    case EDGE_RIGHT:         endx = img_width - rmargin - 1;
         if ((endx - width) <= lmargin)
           startx = lmargin;
         else
           startx = endx - width;

         starty = tmargin;
         if ((starty + length) >= (img_length - bmargin))
           endy = img_length - bmargin - 1;
         else
           endy = starty + length;
         break;
    case EDGE_TOP:  /* width from left, length from top */
    case EDGE_LEFT:
    default:
         startx = lmargin;
         if ((startx + width) >= (img_width - rmargin))
           endx = img_width - rmargin - 1;
         else
           endx = startx + width;

         starty = tmargin;
         if ((starty + length) >= (img_length - bmargin))
           endy = img_length - bmargin - 1;
         else
           endy = starty + length;
         break;
    }

  off->startx = startx;
  off->starty = starty;
  off->endx   = endx;
  off->endy   = endy;

  crop_width  = endx - startx + 1;
  crop_length = endy - starty + 1;

  if (crop_width <= 0)
    {
    TIFFError("computeOffsets", "Invalid left/right margins and /or image crop width requested");
    return (-1);
    }
  if (crop_width > img_width)
    crop_width = img_width;

  if (crop_length <= 0)
    {
    TIFFError("computeOffsets", "Invalid top/bottom margins and /or image crop length requested");
    return (-1);
    }
  if (crop_length > img_length)
    crop_length = img_length;

  off->crop_width = crop_width;
  off->crop_length = crop_length;

  return (0);
  }

/* 
 * Translate cropping options into offsets for one or more regions in the image.
 * Options are applied in this order: margins, specific width and length, zones,
 * but all are optional. Margins are relative to each edge. Width, length and
 * zones are relative to the specified reference edge. Zones are expressed as
 * X:Y where X is the ordinal value in a set of Y equal sized portions. eg.
 * 2:3 would indicate the middle third of the region qualified by margins and
 * any explicit width and length specified.
 */

static int
getCropOffsets(TIFF* in, struct crop_mask *crop)
{
  int    i, seg, total, cropbuff = 0;
  uint32 test, img_width, img_length, buffsize;
  uint16 spp, bps, res_unit;
  float  xres, yres;
  double zwidth, zlength;
  tdir_t cur_dir;
  struct offset offsets;

  TIFFGetField(in, TIFFTAG_IMAGEWIDTH, &img_width);
  TIFFGetField(in, TIFFTAG_IMAGELENGTH, &img_length);
  TIFFGetFieldDefaulted(in, TIFFTAG_BITSPERSAMPLE, &bps);
  TIFFGetFieldDefaulted(in, TIFFTAG_SAMPLESPERPIXEL, &spp);
  TIFFGetFieldDefaulted(in, TIFFTAG_RESOLUTIONUNIT, &res_unit);
  TIFFGetField(in, TIFFTAG_XRESOLUTION, &xres);
  TIFFGetField(in, TIFFTAG_YRESOLUTION, &yres);
  cur_dir = TIFFCurrentDirectory(in);

  /* Compute offsets if margins or fixed width or length specified */
  if ((crop->crop_mode & CROP_MARGINS) ||
      (crop->crop_mode & CROP_LENGTH) || 
      (crop->crop_mode & CROP_WIDTH))
    {
    if (computeOffsets(res_unit, xres, yres, img_width, img_length, crop, &offsets))
      {
      TIFFError ("getCropOffsets", "Unable to compute crop margins");
      return (-1);
      }
    cropbuff = TRUE;
    }
  else
    { /* cropped area is the full image */
    offsets.tmargin = 0;
    offsets.lmargin = 0;
    offsets.bmargin = 0;
    offsets.rmargin = 0;
    offsets.crop_width = img_width;
    offsets.crop_length = img_length;
    offsets.startx = 0;
    offsets.endx = img_width - 1;
    offsets.starty = 0;
    offsets.endy = img_length - 1;
    cropbuff = FALSE;
    }

#ifdef DEBUG
  fprintf (stderr, "Margins: Top: %d  Left: %d  Bottom: %d  Right: %d\n", 
                    offsets.tmargin, offsets.lmargin, offsets.bmargin, offsets.rmargin); 
  fprintf (stderr, "Crop region within margins: Adjusted Width:  %6d  Length: %6d\n\n", 
                    offsets.crop_width, offsets.crop_length);
#endif

  if (!(crop->crop_mode & CROP_ZONES)) /* no crop zones requested */
    {
    if (cropbuff == FALSE)  /* No margins or fixed width or length areas */
      {
      crop->zones = 0;
      crop->combined_width  = img_width;
      crop->combined_length = img_length;
      /* crop->bufftotal = (ceil)(((img_width * bps + 7 ) / 8) * spp) * (ceil(img_length)); */
      return (0);
      }
    else  /* we need one crop zone for margins and fixed width or length areas */
      {
      crop->zones = 1;
      crop->zonelist[0].total = 1;
      crop->zonelist[0].position = 1;
      }
    }     

  /* Compute the start and end values for each zone */
  crop->bufftotal = 0;
  crop->combined_width  = (uint32)0;
  crop->combined_length = (uint32)0;

  switch (crop->edge_ref) {
    case EDGE_LEFT: /* zones from left to right, length from top */
         for (i = 0; i < crop->zones; i++)
           {
	   seg = crop->zonelist[i].position;
	   total = crop->zonelist[i].total;
           zlength = offsets.crop_length;
	   crop->zonelist[i].y1 = offsets.starty;
           crop->zonelist[i].y2 = offsets.endy;

           crop->zonelist[i].x1 = offsets.startx + (uint32)(offsets.crop_width * 1.0 * (seg - 1) / total);
           test = offsets.startx + (uint32)(offsets.crop_width * 1.0 * seg / total);
           if (test > img_width)
             crop->zonelist[i].x2 = img_width;
           else
             crop->zonelist[i].x2 = test;

           zwidth = crop->zonelist[i].x2 - crop->zonelist[i].x1  + 1;

           /* Storing size of individual buffers in case we want to create a separte IFD for each zone */
           buffsize = (ceil)(((zwidth * bps + 7 ) / 8) * spp) * (ceil(zlength));
           /* crop->zonelist[i].buffsize = buffsize; */
           crop->bufftotal += buffsize;
           crop->combined_length += zlength;
           crop->combined_width = zwidth;
#ifdef DEBUG
	   fprintf (stderr, "Zone %d, width: %4d, length: %4d, x1: %4d  x2: %4d  y1: %4d  y2: %4d\n",
                    i + 1, (uint32)zwidth, (uint32)zlength,
  	            crop->zonelist[i].x1, crop->zonelist[i].x2, 
                    crop->zonelist[i].y1, crop->zonelist[i].y2);
#endif
	   }
         break;
    case EDGE_BOTTOM: /* width from left, zones from bottom to top */
         for (i = 0; i < crop->zones; i++)
           {
	   seg = crop->zonelist[i].position;
	   total = crop->zonelist[i].total;
           zwidth = offsets.crop_width;
	   crop->zonelist[i].x1 = offsets.startx;
           crop->zonelist[i].x2 = offsets.endx;

           test = offsets.endy - (uint32)(offsets.crop_length * 1.0 * seg / total);
           if (test < 1 )
	     crop->zonelist[i].y1 = 1;
           else
	     crop->zonelist[i].y1 = test;

           test = offsets.endy - (uint32)(offsets.crop_length * 1.0 * (seg - 1) / total);
           if (test > img_length)
             crop->zonelist[i].y2 = img_length;
           else 
             crop->zonelist[i].y2 = test;

           zlength = crop->zonelist[i].y2 - crop->zonelist[i].y1 + 1;

           /* Storing size of individual buffers in case we want to create a separte IFD for each zone */
           buffsize = (ceil)(((zwidth * bps + 7 ) / 8) * spp) * (ceil(zlength));
           /* crop->zonelist[i].buffsize = buffsize; */
           crop->bufftotal += buffsize;
           crop->combined_length += zlength;
           crop->combined_width = zwidth;

#ifdef DEBUG
	   fprintf (stderr, "Zone %d, width: %4d, length: %4d, x1: %4d  x2: %4d  y1: %4d  y2: %4d\n",
                    i + 1, (uint32)zwidth, (uint32)zlength,
	            crop->zonelist[i].x1, crop->zonelist[i].x2,
                    crop->zonelist[i].y1, crop->zonelist[i].y2);
#endif
	   }
         break;
    case EDGE_RIGHT: /* zones from right to left, length from top */
         for (i = 0; i < crop->zones; i++)
           {
	   seg = crop->zonelist[i].position;
	   total = crop->zonelist[i].total;
           zwidth =  (offsets.crop_width * 1.0) / total;
           zlength = offsets.crop_length;

           /* Storing size of individual buffers in case we want to create a separte IFD for each zone */
           buffsize = (ceil)(((zwidth * bps + 7 ) / 8) * spp) * (ceil(zlength));

           crop->zonelist[i].x1 = offsets.rmargin - (uint32)(offsets.endx - (zwidth * seg));
           crop->zonelist[i].x2 = offsets.rmargin - (uint32)(offsets.endx - (zwidth * (seg - 1)));
	   crop->zonelist[i].y1 = offsets.starty;
           crop->zonelist[i].y2 = offsets.endy;
           /* crop->zonelist[i].buffsize = buffsize; */
           crop->bufftotal += buffsize;
           crop->combined_length += zlength;
           crop->combined_width = zwidth;

#ifdef DEBUG
	   fprintf (stderr, "Zone %d, width: %4d, length: %4d, x1: %4d  x2: %4d  y1: %4d  y2: %4d\n",
                    i + 1, (uint32)zwidth, (uint32)zlength,
	            crop->zonelist[i].x1, crop->zonelist[i].x2, 
                    crop->zonelist[i].y1, crop->zonelist[i].y2);
#endif
	   }
         break;
    case EDGE_TOP: /* width from left, zones from top to bottom */
    default:
         for (i = 0; i < crop->zones; i++)
           {
	   seg = crop->zonelist[i].position;
	   total = crop->zonelist[i].total;
           zwidth = offsets.crop_width;
	   crop->zonelist[i].x1 = offsets.startx;
           crop->zonelist[i].x2 = offsets.endx;

           crop->zonelist[i].y1 = offsets.starty + (uint32)(offsets.crop_length * 1.0 * (seg - 1) / total);
           test = offsets.starty + (uint32)(offsets.crop_length * 1.0 * seg / total);
 	   if (test > img_length)
	     crop->zonelist[i].y2 = img_length;
           else
	     crop->zonelist[i].y2 = test - 1;

           zlength = crop->zonelist[i].y2 - crop->zonelist[i].y1 + 1;

           /* Storing size of individual buffers in case we want to create a separte IFD for each zone */
           buffsize = (ceil)(((zwidth * bps + 7 ) / 8) * spp) * (ceil(zlength));

           /* crop->zonelist[i].buffsize = buffsize; */
           crop->bufftotal += buffsize;
           crop->combined_length += zlength;
           crop->combined_width = zwidth;

#ifdef DEBUG
	   fprintf (stderr, "Zone %d, width: %4d, length: %4d, x1: %4d  x2: %4d  y1: %4d  y2: %4d\n", 
		    i + 1, (uint32)zwidth, (uint32)zlength,
	            crop->zonelist[i].x1, crop->zonelist[i].x2, 
                    crop->zonelist[i].y1, crop->zonelist[i].y2);
#endif
           }
         break;
    }
  return (0);
}

static int
loadImage(TIFF* in, unsigned char **read_ptr)
  {
  int      readunit = 0;
  uint16   nstrips, ntiles, planar, bps, spp;
  uint32   width, length;
  unsigned char *read_buff = NULL;
  tsize_t  stsize, tlsize, buffsize;
  static   tsize_t  prev_readsize = 0;

  TIFFGetFieldDefaulted(in, TIFFTAG_BITSPERSAMPLE, &bps);
  TIFFGetFieldDefaulted(in, TIFFTAG_SAMPLESPERPIXEL, &spp);
  TIFFGetFieldDefaulted(in, TIFFTAG_PLANARCONFIG, &planar);
  TIFFGetField(in, TIFFTAG_IMAGEWIDTH,  &width);
  TIFFGetField(in, TIFFTAG_IMAGELENGTH, &length);

  if ((bps == 0) || (spp == 0)) {
        TIFFError("loadImage",
                  "Invalid samples per pixel (%d) or bits per sample (%d)",
                  spp, bps);
        return (-1);
  }

  if (TIFFIsTiled(in))
    {
    readunit = TILE;
    tlsize = TIFFTileSize(in);
    ntiles = TIFFNumberOfTiles(in);
    buffsize = tlsize * ntiles;
    }
  else
    {
    readunit = STRIP;
    stsize = TIFFStripSize(in);
    nstrips = TIFFNumberOfStrips(in);
    buffsize = stsize * nstrips;
    }

  read_buff = *read_ptr;
  if (!read_buff)
     read_buff = (unsigned char *)_TIFFmalloc(buffsize);
  else
    {
    if (prev_readsize < buffsize)
      _TIFFrealloc(read_buff, buffsize);
    }

  if (!read_buff)
    {
    TIFFError("loadImageImage", "Unable to allocate/reallocate read buffer");
    return (-1);
    }
  prev_readsize = buffsize;
  *read_ptr = read_buff;

  /* read current image into memory */
  switch (readunit) {
    case STRIP:
         if (planar == PLANARCONFIG_CONTIG)
           {
	   if (!(readContigStripsIntoBuffer(in, read_buff, length, width, spp)))
	     {
	     TIFFError("loadImageImage", "Unable to read contiguous strips into buffer");
	     return (-1);
             }
           }
         else
           {
           if (!(readSeparateStripsIntoBuffer(in, read_buff, length, width, spp)))
	     {
	     TIFFError("loadImageImage", "Unable to read separate strips into buffer");
	     return (-1);
             }
           }
         break;

    case TILE:
         if (planar == PLANARCONFIG_CONTIG)
           {
	   if (!(readContigTilesIntoBuffer(in, read_buff, length, width, spp)))
	     {
	     TIFFError("loadImageImage", "Unable to read contiguous tiles into buffer");
	     return (-1);
             }
           }
         else
           {
	     if (!(readSeparateTilesIntoBuffer(in, read_buff, length, width, spp)))
	     {
	     TIFFError("loadImageImage", "Unable to read separate tiles into buffer");
	     return (-1);
             }
           }
         break;
    default: TIFFError("loadImageImage", "Unsupported image file format");
          return (-1);
          break;
    }

return (0);
}

/* Copy the zones out of input buffer into output buffer.
 * N.B. The read functions used copy separate plane data into a buffer as interleaved
 * samples rather than separate planes so the same logic works to extract zones
 * regardless of the way the data are organized in the input file.
 */

static int
extractCropRegions(TIFF *in,  struct crop_mask *crop, unsigned char *read_buff, unsigned char *crop_buff)
  {
  int       i, shift1, shift2, trailing_bits;
  uint16    bps, spp;
  uint32    row, first_row, last_row, col, first_col, last_col;
  uint32    src_offset, dst_offset, row_offset, col_offset;
  uint32    offset1, offset2, full_bytes, j;
  uint32    crop_width, crop_length, img_width, img_length;
  unsigned  char  bytebuff1, bytebuff2;
  unsigned  char *src_ptr, *dst_ptr;

  TIFFGetFieldDefaulted(in, TIFFTAG_BITSPERSAMPLE, &bps);
  TIFFGetFieldDefaulted(in, TIFFTAG_SAMPLESPERPIXEL, &spp);
  TIFFGetField(in, TIFFTAG_IMAGEWIDTH,  &img_width);
  TIFFGetField(in, TIFFTAG_IMAGELENGTH, &img_length);

  src_ptr = read_buff;
  dst_ptr = crop_buff;
  src_offset = 0;
  dst_offset = 0;
  for (i = 0; i < crop->zones; i++)
    {
    first_row = crop->zonelist[i].y1;
    last_row  = crop->zonelist[i].y2;
    first_col = crop->zonelist[i].x1;
    last_col  = crop->zonelist[i].x2;

    crop_width = last_col - first_col + 1;
    crop_length = last_row - first_row + 1;

    full_bytes = spp * (crop_width * bps) / 8;   /* number of COMPLETE bytes per row in crop area */
    trailing_bits = (crop_width * bps) % 8;

    if ((bps % 8) == 0)
      {
      for (row = first_row; row <= last_row; row++)
        {
	row_offset = row * img_width * spp;
	for (col = first_col; col <= last_col; col++)
	  {
	  col_offset = col * spp;
          src_ptr = read_buff + row_offset + col_offset;
          for (j = 0; j < spp; j++)
            *dst_ptr++ = *src_ptr++;
	  }
	}        
      }
    else
      { /* bps != 8 */
      for (row = first_row; row <= last_row; row++)
        {
        /* pull out the first byte */
	row_offset = spp * row * ((img_width * bps + 7) / 8);
	offset1 = row_offset + (first_col * bps / 8);
        shift1  = spp * ((first_col * bps) % 8);

        offset2 = row_offset + (last_col * bps / 8);
        shift2  = spp * ((last_col * bps) % 8);

        bytebuff1 = bytebuff2 = 0;
        if (shift1 == 0) /* the region is byte and sample alligned */
          {
	  _TIFFmemcpy (crop_buff + dst_offset, read_buff + offset1, full_bytes);
          dst_offset += full_bytes;

          if (trailing_bits != 0)
            {
	    bytebuff2 = read_buff[offset2] & ((unsigned char)255 << (8 - shift2 - (bps * spp)));
            crop_buff[dst_offset] = bytebuff2;
            dst_offset++;
            }
          }
        else   /* each destination byte will have to be built from two source bytes*/
          {
          for (j = 0; j <= full_bytes; j++) 
            {
	    bytebuff1 = read_buff[offset1 + j] & ((unsigned char)255 >> shift1);
	    bytebuff2 = read_buff[offset1 + j + 1] & ((unsigned char)255 << (8 - shift1 - (bps * spp)));
            crop_buff[dst_offset + j] = (bytebuff1 << shift1) | (bytebuff2 >> (8 - shift1 - (bps * spp)));
            }
          dst_offset += full_bytes;

          if (trailing_bits != 0)
            {
	    if (shift2 > shift1)
              {
	      bytebuff1 = read_buff[row_offset + full_bytes] & ((unsigned char)255 << (8 - shift2 - (bps * spp)));
              bytebuff2 = bytebuff1 & ((unsigned char)255 << shift1);
              crop_buff[dst_offset + j] = bytebuff2;
              }
            else
              {
 	      if (shift2 < shift1)
                {
                bytebuff2 = ((unsigned char)255 << (8 - shift2 - (bps * spp)));
                crop_buff[dst_offset + j] &= bytebuff2;
                }
              }
	    }
          dst_offset++;
	  }
        }
      }
    }
  return (0);
  }

/* Copy the crop section of the data from the current image into a buffer
 * and adjust the IFD values to reflect the new size. If no cropping is
 * required, use the origial read buffer as the crop buffer.
 */
static int
createCroppedImage(TIFF* in, struct crop_mask *crop, unsigned char **read_buff_ptr, unsigned char **crop_buff_ptr)
  {
  uint32    img_width, img_length;
  uint16    bps, spp, photometric;
  tsize_t   cropsize;
  unsigned  char *read_buff = NULL;
  unsigned  char *crop_buff = NULL;
  static    tsize_t  prev_cropsize = 0;
  
  TIFFGetField(in, TIFFTAG_IMAGEWIDTH,  &img_width);
  TIFFGetField(in, TIFFTAG_IMAGELENGTH, &img_length);
  TIFFGetField(in, TIFFTAG_PHOTOMETRIC, &photometric);
  TIFFGetFieldDefaulted(in, TIFFTAG_BITSPERSAMPLE, &bps);
  TIFFGetFieldDefaulted(in, TIFFTAG_SAMPLESPERPIXEL, &spp);

  read_buff = *read_buff_ptr;
  if (crop->zones == 0)
    { /* process full image, no crop buffer needed */
    crop_buff = read_buff;
    *crop_buff_ptr = read_buff;
    crop->combined_width = img_width;
    crop->combined_length = img_length;
    }   
  else
    { /* one of more crop zones */
    cropsize = crop->bufftotal;
    crop_buff = *crop_buff_ptr;
    if (!crop_buff)
      {
      crop_buff = (unsigned char *)_TIFFmalloc(cropsize);
      *crop_buff_ptr = crop_buff;
      }
    else
      {
      if (prev_cropsize < cropsize)
        {
        crop_buff = _TIFFrealloc(crop_buff, cropsize);
        *crop_buff_ptr = crop_buff;
        }
      }

    if (!crop_buff)
      {
      TIFFError("createCroppedImage", "Unable to allocate/reallocate crop buffer");
      return (-1);
      }

    if (extractCropRegions(in, crop, read_buff, crop_buff))
      {
      TIFFError("createCroppedImage", "Unable to extract cropped regions from image");
      return (-1);
      }
    } /* end if crop->zones != 0) */

  if (crop->crop_mode & CROP_INVERT)
    {
      if (invertImage(photometric, spp, bps, crop->combined_width, crop->combined_length, crop_buff))
      {
      TIFFError("createCroppedImage", "Failed to invert colorspace for image or cropped selection");
      return (-1);
      }
    }

  if (crop->crop_mode & CROP_MIRROR)
    {
    if (mirrorImage(spp, bps, crop->mirror, crop->combined_width, crop->combined_length, crop_buff))
      {
      TIFFError("createCroppedImage", "Failed to mirror image or cropped selection %s", 
	       (crop->rotation == MIRROR_HORIZ) ? "horizontally" : "vertically");
      return (-1);
      }
    }

  if (crop->crop_mode & CROP_ROTATE) /* rotate should be last as it can reallocate the buffer */
    {
    if (rotateImage(crop->rotation, spp, bps, &crop->combined_width, &crop->combined_length, crop_buff_ptr))
      {
      TIFFError("createCroppedImage", "Failed to rotate image or cropped selection by %d degrees", crop->rotation);
      return (-1);
      }
    }

  if (crop_buff == read_buff) /* we used the read buffer for the crop buffer */
    *read_buff_ptr = NULL;    /* so we don't try to free it later */

  return (0);
  }

static int  
writeCroppedImage(TIFF *in, TIFF *out, struct crop_mask *crop, unsigned char *crop_buff)
  {
  uint16 bps, spp;
  uint32 width, length;
  struct cpTag* p;

  width = crop->combined_width;
  length = crop->combined_length;
  TIFFSetField(out, TIFFTAG_IMAGEWIDTH, width);
  TIFFSetField(out, TIFFTAG_IMAGELENGTH, length);

  CopyField(TIFFTAG_BITSPERSAMPLE, bps);
  CopyField(TIFFTAG_SAMPLESPERPIXEL, spp);
  if (compression != (uint16)-1)
    TIFFSetField(out, TIFFTAG_COMPRESSION, compression);
  else
    CopyField(TIFFTAG_COMPRESSION, compression);

  if (compression == COMPRESSION_JPEG) {
    uint16 input_compression, input_photometric;

    if (TIFFGetField(in, TIFFTAG_COMPRESSION, &input_compression)
        && input_compression == COMPRESSION_JPEG) {
          TIFFSetField(in, TIFFTAG_JPEGCOLORMODE, JPEGCOLORMODE_RGB);
        }
    if (TIFFGetField(in, TIFFTAG_PHOTOMETRIC, &input_photometric)) {
	if(input_photometric == PHOTOMETRIC_RGB) {
	   if (jpegcolormode == JPEGCOLORMODE_RGB)
	     TIFFSetField(out, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_YCBCR);
	   else
	     TIFFSetField(out, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_RGB);
	   } else
	      TIFFSetField(out, TIFFTAG_PHOTOMETRIC, input_photometric);
	}
      }
  else if (compression == COMPRESSION_SGILOG || compression == COMPRESSION_SGILOG24)
       TIFFSetField(out, TIFFTAG_PHOTOMETRIC, spp == 1 ?
			PHOTOMETRIC_LOGL : PHOTOMETRIC_LOGLUV);
       else
	  CopyTag(TIFFTAG_PHOTOMETRIC, 1, TIFF_SHORT);
       if (fillorder != 0)
	  TIFFSetField(out, TIFFTAG_FILLORDER, fillorder);
       else
	  CopyTag(TIFFTAG_FILLORDER, 1, TIFF_SHORT);
       /*
        * Will copy `Orientation' tag from input image
        */
  TIFFGetFieldDefaulted(in, TIFFTAG_ORIENTATION, &orientation);
  switch (orientation) {
    case ORIENTATION_BOTRIGHT:
    case ORIENTATION_RIGHTBOT:	/* XXX */
         TIFFWarning(TIFFFileName(in), "using bottom-left orientation");
         orientation = ORIENTATION_BOTLEFT;
         /* fall thru... */
    case ORIENTATION_LEFTBOT:	/* XXX */
    case ORIENTATION_BOTLEFT:
         break;
    case ORIENTATION_TOPRIGHT:
    case ORIENTATION_RIGHTTOP:	/* XXX */
    default:
         TIFFWarning(TIFFFileName(in), "using top-left orientation");
         orientation = ORIENTATION_TOPLEFT;
         /* fall thru... */
    case ORIENTATION_LEFTTOP:	/* XXX */
    case ORIENTATION_TOPLEFT:
         break;
   }
  TIFFSetField(out, TIFFTAG_ORIENTATION, orientation);
	
  /*
   * Choose tiles/strip for the output image according to
   * the command line arguments (-tiles, -strips) and the
   * structure of the input image.
   */
  if (outtiled == -1)
    outtiled = TIFFIsTiled(in);
  if (outtiled) {
    /*
     * Setup output file's tile width&height.  If either
     * is not specified, use either the value from the
     * input image or, if nothing is defined, use the
     * library default.
     */
    if (tilewidth == (uint32) -1)
      TIFFGetField(in, TIFFTAG_TILEWIDTH, &tilewidth);
    if (tilelength == (uint32) -1)
      TIFFGetField(in, TIFFTAG_TILELENGTH, &tilelength);

    if (tilewidth > width)
      tilewidth = width;
    if (tilelength > length)
      tilelength = length;

    TIFFDefaultTileSize(out, &tilewidth, &tilelength);
    TIFFSetField(out, TIFFTAG_TILEWIDTH, tilewidth);
    TIFFSetField(out, TIFFTAG_TILELENGTH, tilelength);
    } else {
       /*
	* RowsPerStrip is left unspecified: use either the
	* value from the input image or, if nothing is defined,
	* use the library default.
	*/
	if (rowsperstrip == (uint32) 0) {
	  if (!TIFFGetField(in, TIFFTAG_ROWSPERSTRIP, &rowsperstrip)) {
	      rowsperstrip = TIFFDefaultStripSize(out, rowsperstrip);
	     }
	  if (rowsperstrip > length)
	       rowsperstrip = length;
	  }
	else if (rowsperstrip == (uint32) -1)
		rowsperstrip = length;
		TIFFSetField(out, TIFFTAG_ROWSPERSTRIP, rowsperstrip);
	}
  if (config != (uint16) -1)
    TIFFSetField(out, TIFFTAG_PLANARCONFIG, config);
  else
    CopyField(TIFFTAG_PLANARCONFIG, config);
  if (spp <= 4)
    CopyTag(TIFFTAG_TRANSFERFUNCTION, 4, TIFF_SHORT);
  CopyTag(TIFFTAG_COLORMAP, 4, TIFF_SHORT);

/* SMinSampleValue & SMaxSampleValue */
  switch (compression) {
    case COMPRESSION_JPEG:
         TIFFSetField(out, TIFFTAG_JPEGQUALITY, quality);
	 TIFFSetField(out, TIFFTAG_JPEGCOLORMODE, jpegcolormode);
	 break;
   case COMPRESSION_LZW:
   case COMPRESSION_ADOBE_DEFLATE:
   case COMPRESSION_DEFLATE:
       	if (predictor != (uint16)-1)
          TIFFSetField(out, TIFFTAG_PREDICTOR, predictor);
	else
	  CopyField(TIFFTAG_PREDICTOR, predictor);
	break;
   case COMPRESSION_CCITTFAX3:
   case COMPRESSION_CCITTFAX4:
       	if (compression == COMPRESSION_CCITTFAX3) {
          if (g3opts != (uint32) -1)
	    TIFFSetField(out, TIFFTAG_GROUP3OPTIONS, g3opts);
	  else
	    CopyField(TIFFTAG_GROUP3OPTIONS, g3opts);
	} else
	    CopyTag(TIFFTAG_GROUP4OPTIONS, 1, TIFF_LONG);
	    CopyTag(TIFFTAG_BADFAXLINES, 1, TIFF_LONG);
	    CopyTag(TIFFTAG_CLEANFAXDATA, 1, TIFF_LONG);
	    CopyTag(TIFFTAG_CONSECUTIVEBADFAXLINES, 1, TIFF_LONG);
	    CopyTag(TIFFTAG_FAXRECVPARAMS, 1, TIFF_LONG);
	    CopyTag(TIFFTAG_FAXRECVTIME, 1, TIFF_LONG);
	    CopyTag(TIFFTAG_FAXSUBADDRESS, 1, TIFF_ASCII);
	break;
   }
   { uint32 len32;
     void** data;
     if (TIFFGetField(in, TIFFTAG_ICCPROFILE, &len32, &data))
       TIFFSetField(out, TIFFTAG_ICCPROFILE, len32, data);
   }
   { uint16 ninks;
     const char* inknames;
     if (TIFFGetField(in, TIFFTAG_NUMBEROFINKS, &ninks)) {
       TIFFSetField(out, TIFFTAG_NUMBEROFINKS, ninks);
       if (TIFFGetField(in, TIFFTAG_INKNAMES, &inknames)) {
	 int inknameslen = strlen(inknames) + 1;
	 const char* cp = inknames;
	 while (ninks > 1) {
	   cp = strchr(cp, '\0');
	   if (cp) {
	     cp++;
	     inknameslen += (strlen(cp) + 1);
	   }
	   ninks--;
         }
	 TIFFSetField(out, TIFFTAG_INKNAMES, inknameslen, inknames);
       }
     }
   }
   {
   unsigned short pg0, pg1;
   if (TIFFGetField(in, TIFFTAG_PAGENUMBER, &pg0, &pg1)) {
     if (pageNum < 0) /* only one input file */
	TIFFSetField(out, TIFFTAG_PAGENUMBER, pg0, pg1);
     else 
	TIFFSetField(out, TIFFTAG_PAGENUMBER, pageNum++, 0);
     }
   }

  for (p = tags; p < &tags[NTAGS]; p++)
		CopyTag(p->tag, p->count, p->type);

  /* Compute the tile or strip dimensions and write to disk */
  if (outtiled)
    {
    if (config == PLANARCONFIG_CONTIG)
      {
      writeBufferToContigTiles (out, crop_buff, length, width, spp);
      }
    else
      writeBufferToSeparateTiles (out, crop_buff, length, width, spp);
    }
  else
    {
    if (config == PLANARCONFIG_CONTIG)
      {
      writeBufferToContigStrips (out, crop_buff, length, width, spp);
      }
    else
      {
      writeBufferToSeparateStrips(out, crop_buff, length, width, spp);
      }
    }

  if (!TIFFWriteDirectory(out))
    {
    TIFFClose(out);
    return (-1);
    }

  return (0);
  }

/* Rotate an image by a multiple of 90 degrees clockwise */
static int
rotateImage(uint16 rotation, uint16 spp, uint16 bps, uint32 *img_width, uint32 *img_length, unsigned char **crop_buff_ptr)
  {
  uint32   i, j, row, col, width, length, full_bytes, trailing_bits;
  uint32   rowsize, colsize, buffsize, row_offset, col_offset, pix_offset;
  unsigned char bitset, bytebuff1, bytebuff2, bytes_per_pixel;
  unsigned char *crop_buff = *crop_buff_ptr;
  unsigned char *src_ptr;
  unsigned char *dst_ptr;
  static unsigned char *rotate_buff = NULL;

  width = *img_width;
  length = *img_length;
  rowsize = (width * bps + 7) / 8;
  colsize = (length * bps + 7) / 8;
  bytes_per_pixel = (spp * bps + 7) / 8;
  full_bytes = width * spp * bps / 8;
  trailing_bits = (width * spp * bps) % 8;
  pix_offset = (spp * bps) / 8;
  
  /* rotating image may change the end of line padding and increase buffer size */
  switch (rotation) 
    {
    case  90:
    case 180: 
    case 270: buffsize = spp * (colsize + 1) * (rowsize + 1) * 8;
              break;
    default:  TIFFError("rotateImage", "Invalid rotation angle %d", rotation);
              return (-1);
              break;
    }

  if (!(rotate_buff = (unsigned char *)_TIFFmalloc(buffsize)))
    {
    TIFFError("rotateImage", "Unable to allocate rotation buffer of %1u bytes", buffsize);
    return (-1);
    }

  src_ptr = crop_buff;
  switch (rotation)
    {
    case 180: if ((bps % 8) == 0) /* byte alligned data */
                { 
                for (row = 0; row < length; row++)
                    {
		    row_offset = (length - row - 1) * rowsize * spp;
                    for (col = 0; col < width; col++)
                      { 
		      col_offset = (width - col - 1) * pix_offset;
                      dst_ptr = rotate_buff + row_offset + col_offset;

		      for (i = 0; i  < bytes_per_pixel; i++)
			*dst_ptr++ = *src_ptr++;
                      }
                    }
                }
	      else
                { /* non 8 bit per pixel data */
                for (row = 0; row < length; row++)
                  {
		  src_ptr = crop_buff + row * rowsize * spp;

		  row_offset = (length - row - 1) * rowsize * spp;
		  col_offset = (rowsize * spp) - 1;
                  dst_ptr = rotate_buff + row_offset + col_offset;

                  if ((width % 8) == 0)
		    {
                    for (col = 0; col < rowsize; col++)
                      {
                      for (i = 0, j = 7; i < 8; i++, j--)
                        {
			  bitset = ((*src_ptr) & (((unsigned char)1 << j)) ? 1 : 0);
		      *dst_ptr |= (bitset << i);
                        }
                      src_ptr++;
                      dst_ptr--;
		      }
		    }

                  else
                    {
                    bytebuff2 = 0;
                    for (i = 0, j = 7; i < trailing_bits; i++, j--)
                      {
		      bitset = ((*src_ptr) & (((unsigned char)1 << j)) ? 1 : 0);
                      bytebuff2 |= bitset << (8 - trailing_bits + i);
                      }
                    *(dst_ptr--) = bytebuff2;

                    for (col = 0; col < full_bytes; col++)
                      {
		      bytebuff1 = *(src_ptr) & ((unsigned char)255 >> trailing_bits);
		      bytebuff2 = *(src_ptr + 1) & ((unsigned char)255 << (8 - trailing_bits));
		      *dst_ptr = (bytebuff1 << trailing_bits) | (bytebuff2 >> (8 - trailing_bits));

		      TIFFReverseBits(dst_ptr, 1);
                      src_ptr++;
                      dst_ptr--;
                      }
		    }
                  }
	        }
              _TIFFfree(crop_buff);
              *(crop_buff_ptr) = rotate_buff;
              break;

    case 90:  if ((bps % 8) == 0) /* byte aligned data */
                {
                for (row = 1; row <= length; row++)
                  {
		  dst_ptr = rotate_buff + (spp * colsize) - (row * bytes_per_pixel);
                  for (col = 0; col < width; col++)
                    {
		    for (i = 0; i  < bytes_per_pixel; i++)
                      *(dst_ptr + i) = *src_ptr++;
                    dst_ptr += (spp * colsize);
                    }
                  }
	        }
              else
                {
                for (row = 0; row < length; row++)
                  {
		  dst_ptr = rotate_buff + colsize - (row / 8);
                  for (col = 0; col < width; col+=  8 /(bps * spp))
                    {
                    for (i = 0, j = 7; i < 8; i++, j--)
                      {
                      if (col + i < width)
			{
		        bitset = ((*src_ptr) & (((unsigned char)1 << j)) ? 1 : 0);
		        *(dst_ptr) |= (bitset << ((row + trailing_bits) % 8));
 		        dst_ptr += colsize;
			}
                      }
                    src_ptr++;
                    }
		  }
                }
              _TIFFfree(crop_buff);
              *(crop_buff_ptr) = rotate_buff;

              *img_width = length;
              *img_length = width;
	      break;

    case 270: if ((bps % 8) == 0) /* byte aligned data */
                {
                for (row = 0; row < length; row++)
                  {
	          dst_ptr = rotate_buff + (spp * rowsize * length) + (row * bytes_per_pixel);
                  for (col = 0; col < width; col++)
                    {
		    for (i = 0; i  < bytes_per_pixel; i++)
                      *(dst_ptr + i) = *src_ptr++;
                    dst_ptr -= (spp * colsize);
                    }
                  }
	        }
              else
                {
                for (row = 0; row < length; row++)
                  {
		  dst_ptr = rotate_buff + (colsize * width) + (row / 8);
                  for (col = 0; col < width; col+=  8 /(bps * spp))
                    {
                    for (i = 0, j = 7; i < 8; i++, j--)
                      {
                      if (col + i < width)
			{
		        bitset = ((*src_ptr) & (((unsigned char)1 << j)) ? 1 : 0);
		        dst_ptr -= colsize;
		        *(dst_ptr) |= (bitset << ( 7 - (row % 8)));
			}
                      }
                    src_ptr++;
                    }
		  }
                }

              _TIFFfree(crop_buff);
              *(crop_buff_ptr) = rotate_buff;

              *img_width = length;
              *img_length = width;
              break;
    default:
              break;
    }

  return (0);
  }


/* Mirror an image horizontally or vertically */
static int
mirrorImage(uint16 spp, uint16 bps, uint16 mirror, uint32 width, uint32 length, unsigned char *crop_buff)
  {
  uint32   i, j, row, col, full_bytes, trailing_bits;
  uint32   rowsize, colsize, row_offset, pix_offset;
  unsigned char  bytebuff1, bytebuff2, bytes_per_pixel, bitset;
  unsigned char *line_buff = NULL;
  unsigned char *src_ptr;
  unsigned char *dst_ptr;

  rowsize = (width * bps + 7) / 8;
  colsize = (length * bps + 7) / 8;
  bytes_per_pixel = (spp * bps + 7) / 8;
  full_bytes = width * spp * bps / 8;
  trailing_bits = (width * bps) % 8;
  pix_offset = (spp * bps) / 8;

  src_ptr = crop_buff;
  switch (mirror)
    {
    case MIRROR_HORIZ :
              if ((bps % 8) == 0) /* byte alligned data */
                { 
                for (row = 0; row < length; row++)
                    {
		    row_offset = row * rowsize * spp;
                    src_ptr = crop_buff + row_offset;
                    dst_ptr = src_ptr + (spp * rowsize);
                    for (col = 0; col < (width / 2); col++)
                      { 
		      for (i = 0; i  < spp; i++)
                        {
			bytebuff1 = *src_ptr;
			*src_ptr++ = *(dst_ptr - spp + i);
                        *(dst_ptr - spp + i) = bytebuff1;
			}
		      dst_ptr -= spp;
                      }
                    }
                }
	      else
                { /* non 8 bit per sample  data */
                if (!(line_buff = (unsigned char *)_TIFFmalloc(spp * rowsize + 1)))
                  {
                  TIFFError("mirrorImage", "Unable to allocate mirror line buffer");
                  return (-1);
                  }
                for (row = 0; row < length; row++)
                  {
		  row_offset = row * rowsize * spp;
		  src_ptr = crop_buff + row_offset;
                  dst_ptr = line_buff + (spp * rowsize) - 1;
                  memset (line_buff, '\0', rowsize);

                  if ((width % 8) == 0)
		    {
                    for (col = 0; col < rowsize; col++)
                      {
                      for (i = 0, j = 7; i < 8; i++, j--)
                        {
			bitset = (*(src_ptr + col) & (((unsigned char)1 << j)) ? 1 : 0);
                        line_buff[rowsize - col] |= (bitset << i);
                        }
		      }
		    memcpy (src_ptr, line_buff, spp * rowsize);
		    }
                  else
                    {
                    bytebuff2 = 0;
                    for (i = 0, j = 7; i < trailing_bits; i++, j--)
                      {
		      bitset = ((*src_ptr) & (((unsigned char)1 << j)) ? 1 : 0);
                      bytebuff2 |= bitset << (8 - trailing_bits + i);
                      }
                    *(dst_ptr--) = bytebuff2;

                    for (col = 0; col < full_bytes; col++)
                      {
		      bytebuff1 = *(src_ptr) & ((unsigned char)255 >> trailing_bits);
		      bytebuff2 = *(src_ptr + 1) & ((unsigned char)255 << (8 - trailing_bits));
		      *dst_ptr = (bytebuff1 << trailing_bits) | (bytebuff2 >> (8 - trailing_bits));

		      TIFFReverseBits(dst_ptr, 1);
                      src_ptr++;
                      dst_ptr--;
                      }
		    _TIFFmemcpy (crop_buff + row_offset, line_buff, spp * rowsize);
                    }
	          }
                if (line_buff)
                  _TIFFfree(line_buff);
		}
             break;

    case MIRROR_VERT: 
             if (!(line_buff = (unsigned char *)_TIFFmalloc(spp * rowsize)))
               {
	       TIFFError ("mirrorImage", "Unable to allocate mirror line buffer of %1u bytes", rowsize);
               return (-1);
               }
  
             dst_ptr = crop_buff + (spp * rowsize * (length - 1));
             for (row = 0; row < length / 2; row++)
               {
	       memcpy(line_buff, src_ptr, spp * rowsize);
	       memcpy(src_ptr, dst_ptr, spp * rowsize);
	       memcpy(dst_ptr, line_buff, spp * rowsize);
               src_ptr += (spp * rowsize);
               dst_ptr -= (spp * rowsize);                                 
               }

             if (line_buff)
               _TIFFfree(line_buff);
             break;

    default: TIFFError ("mirrorImage", "Invalid mirror axis %d", mirror);
             return (-1);
             break;
    }

  return (0);
  }

/* Invert the light and dark values for a bilevel or grayscale image */
static int
invertImage(uint16 photometric, uint16 spp, uint16 bps, uint32 width, uint32 length, unsigned char *crop_buff)
  {
  uint32   row, col;
  unsigned char  bytebuff1, bytebuff2, bytebuff3, bytebuff4;
  unsigned char *src_ptr;

  if (spp != 1)
    {
    TIFFError("invertImage", "Image inversion not supported for more than one sample per pixel");
    return (-1);
    }

  if (photometric !=  PHOTOMETRIC_MINISWHITE && photometric !=  PHOTOMETRIC_MINISBLACK)
    {
    TIFFError("invertImage", "Only black and white and grayscale images can be inverted");
    return (-1);
    }

  src_ptr = crop_buff;
  if (src_ptr == NULL)
    {
    TIFFError ("invertImage", "Invalid crop buffer passed to invertImage");
    return (-1);
    }

  switch (bps)
    {
    case 8: for (row = 0; row < length; row++)
              for (col = 0; col < width; col++)
                {
	        *src_ptr = (unsigned char)255 - *src_ptr;
                 src_ptr++;
                }
            break;
    case 4: for (row = 0; row < length; row++)
              for (col = 0; col < width; col++)
                {
	        bytebuff1 = 16 - (*src_ptr & (unsigned char)240 >> 4);
		bytebuff2 = 16 - (*src_ptr & (unsigned char)15);
	        *src_ptr = bytebuff1 << 4 & bytebuff2;
                src_ptr++;
                }
            break;
    case 2: for (row = 0; row < length; row++)
              for (col = 0; col < width; col++)
                {
	        bytebuff1 = 4 - (*src_ptr & (unsigned char)192 >> 6);
	        bytebuff2 = 4 - (*src_ptr & (unsigned char)48  >> 4);
	        bytebuff3 = 4 - (*src_ptr & (unsigned char)12  >> 2);
		bytebuff4 = 4 - (*src_ptr & (unsigned char)3);
	        *src_ptr = (bytebuff1 << 6) || (bytebuff2 << 4) || (bytebuff3 << 2) || bytebuff4;
                src_ptr++;
                }
            break;
    case 1: for (row = 0; row < length; row++)
              for (col = 0; col < width; col += 8 /(spp * bps))
                {
                *src_ptr = ~(*src_ptr);
                src_ptr++;
                }
            break;
    default: TIFFError("invertImage", "Unsupported bit depth %d", bps);
      return (-1);
    }

  return (0);
  }

/* vim: set ts=8 sts=8 sw=8 noet: */
