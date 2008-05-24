/* $Id$ */

/* tiffcrop.c -- a port of tiffcp.c extended to include manipulations of
 * the image data through additional options listed below
 *
 * Original code:
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
 * Additions (c) Richard Nolde  Last Updated 5/15/2008 
 * IN NO EVENT SHALL RICHARD NOLDE BE LIABLE FOR ANY SPECIAL, INCIDENTAL, 
 * INDIRECT OR CONSEQUENTIAL DAMAGES OF ANY KIND, OR ANY DAMAGES WHATSOEVER 
 * RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER OR NOT ADVISED OF 
 * THE POSSIBILITY OF DAMAGE, AND ON ANY THEORY OF  LIABILITY, ARISING OUT 
 * OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * Add support for the options below to extract sections of image(s) 
 * and to modify the whole image or selected portions of each image by
 * rotations, mirroring, and colorscale/colormap inversion of selected
 * types of TIFF images when appropriate. Some functions are restricted
 * to bilevel or 8 bit per sample images with integer data.
 *
 * Options: 
 * -h             Display the syntax guide.
 * -v             Report the version and last build date for tiffcrop
 * -z x1,y1,x2,y2:x3,y3,x4,y4:..xN,yN,xN + 1, yN + 1 
 *                Specify a series of coordinates to define regions
 * -e c|d|i|m|s   export mode for images and selections from input images
 *   combined     All images and selections are written to a single file (default)
 *                with multiple selections from one image combined into a single image
 *   divided      All images and selections are written to a single file
 *                with each selection from one image written to a new image
 *   image        Each input image is written to a new file (numeric filename sequence)
 *                with multiple selections from the image combined into one image
 *   multiple     Each input image is written to a new file (numeric filename sequence)
 *                with each selection from the image written to a new image
 *   separated    Individual selections from each image are written to separate files
 * -U units       [in, cm, px ] inches, centimeters or pixels
 * -H #           Set horizontal resolution of output images to #
 * -V #           Set vertical resolution of output images to #
 * -J #           Horizontal margin of output page to # expressed in current
 *                units
 * -K #           Vertical margin of output page to # expressed in current
 *                units
 * -X #           Horizontal dimension of region to extract expressed in current
 *                units
 * -Y #           Vertical dimension of region to extract expressed in current
 *                units
 * -O orient      Orientation for output image, portrait, landscape, auto
 * -P page        Page size for output image segments, eg letter, legal, tabloid,
 *                etc.
 * -S cols:rows   Divide the image into equal sized segments using cols across
 *                and rows down
 * -E t|l|r|b     Edge to use as origin
 * -m #,#,#,#     Margins from edges for selection: top, left, bottom, right
 *                (commas separated)
 * -Z #:#,#:#     Zones of the image designated as zone X of Y, 
 *                eg 1:3 would be first of three equal portions measured
 *                from reference edge
 * -N odd|even|#,#-#,#|last 
 *                Select sequences and/or ranges of images within file
 *                to process. The words odd or even may be used to specify
 *                all odd or even numbered images the word last may be used
 *                in place of a number in the sequence to indicate the final
 *                image in the file without knowing how many images there are.
 * -R #           Rotate image or crop selection by 90,180,or 270 degrees
 *                clockwise  
 * -F h|v         Flip (mirror) image or crop selection horizontally
 *                or vertically 
 * -I [black|white|data|both]
 *                Invert color space, eg dark to light for bilevel and grayscale images
 *                If argument is white or black, set the PHOTOMETRIC_INTERPRETATION 
 *                tag to MinIsBlack or MinIsWhite without altering the image data
 *                If the argument is data or both, the image data are modified:
 *                both inverts the data and the PHOTOMETRIC_INTERPRETATION tag,
 *                data inverts the data but not the PHOTOMETRIC_INTERPRETATION tag
 */

static   char tiffcrop_version_id[] = "1.0";
static   char tiffcrop_rev_date[] = "05/21/2008";

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

#ifndef HAVE_GETOPT
extern int getopt(int, char**, char*);
#endif

#include "tiffio.h"

#if defined(VMS)
# define unlink delete
#endif

#define	streq(a,b)	(strcmp((a),(b)) == 0)
#define	strneq(a,b,n)	(strncmp((a),(b),(n)) == 0)

/* NB: the uint32 casts are to silence certain ANSI-C compilers */
#define TIFFhowmany(x, y) ((((uint32)(x))+(((uint32)(y))-1))/((uint32)(y)))
#define TIFFhowmany8(x) (((x)&0x07)?((uint32)(x)>>3)+1:(uint32)(x)>>3)

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
#define EDGE_CENTER   5

#define MIRROR_HORIZ  1
#define MIRROR_VERT   2

#define CROP_NONE     0
#define CROP_MARGINS  1
#define CROP_WIDTH    2
#define CROP_LENGTH   4
#define CROP_ZONES    8
#define CROP_REGIONS 16
#define CROP_ROTATE  32
#define CROP_MIRROR  64
#define CROP_INVERT 128

/* Modes for writing out images and selections */
#define ONE_FILE_COMPOSITE       0 /* One file, sections combined sections */
#define ONE_FILE_SEPARATED       1 /* One file, sections to new IFDs */
#define FILE_PER_IMAGE_COMPOSITE 2 /* One file per image, combined sections */
#define FILE_PER_IMAGE_SEPARATED 3 /* One file per input image */
#define FILE_PER_SELECTION       4 /* One file per selection */

#define COMPOSITE_IMAGES         0 /* Selections combined into one image */  
#define SEPARATED_IMAGES         1 /* Selections saved to separate images */

#define STRIP    1
#define TILE     2

#define MAX_REGIONS   8  /* number of regions to extract from a single page */
#define MAX_OUTBUFFS  8  /* must match larger of zones or regions */
#define MAX_SECTIONS 32  /* number of sections per page to write to output */
#define MAX_IMAGES  512  /* number of images in descrete list, not in the file */

/* Offsets into buffer for margins and fixed width and length segments */
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

struct  buffinfo {
  uint32 size;           /* size of this buffer */
  unsigned char *buffer; /* address of the allocated buffer */
};

struct  zone {
  int   position;  /* ordinal of segment to be extracted */
  int   total;     /* total equal sized divisions of crop area */
  };

struct  pageseg {
  uint32 x1;        /* index of left edge */
  uint32 x2;        /* index of right edge */
  uint32 y1;        /* index of top edge */
  uint32 y2;        /* index of bottom edge */
  int    position;  /* ordinal of segment to be extracted */
  int    total;     /* total equal sized divisions of crop area */
  uint32 buffsize;  /* size of buffer needed to hold the cropped zone */
};

struct  coordpairs {
  double X1;        /* index of left edge in current units */
  double X2;        /* index of right edge in current units */
  double Y1;        /* index of top edge in current units */
  double Y2;        /* index of bottom edge in current units */
};

struct  region {
  uint32 x1;        /* pixel offset of left edge */
  uint32 x2;        /* pixel offset of right edge */
  uint32 y1;        /* pixel offset of top edge */
  uint32 y2;        /* picel offset of bottom edge */
  uint32 width;     /* width in pixels */
  uint32 length;    /* length in pixels */
  uint32 buffsize;  /* size of buffer needed to hold the cropped region */
  unsigned char *buffptr; /* address of start of the region */
};

/* Cropping parameters from command line and image data */
struct crop_mask {
  double width;           /* Selection width for master crop region in requested units */
  double length;          /* Selection length for master crop region in requesed units */
  double margins[4];      /* Top, left, bottom, right margins */
  float  xres;            /* Horizontal resolution read from image*/
  float  yres;            /* Vertical resolution read from image */
  uint32 combined_width;  /* Width of combined cropped zones */
  uint32 combined_length; /* Length of combined cropped zones */
  uint32 bufftotal;       /* Size of buffer needed to hold all the cropped region */
  uint16 img_mode;        /* Composite or separate images created from zones or regions */
  uint16 exp_mode;        /* Export input images or selections to one or more files */
  uint16 crop_mode;       /* Crop options to be applied */
  uint16 res_unit;        /* Resolution unit for margins and selections */
  uint16 edge_ref;        /* Reference edge for sections extraction and combination */
  uint16 rotation;        /* Clockwise rotation of the extracted region or image */
  uint16 mirror;          /* Mirror extracted region or image horizontally or vertically */
  uint16 invert;          /* Invert the color map of image or region */
  uint16 photometric;     /* Status of photometric interpretation for inverted image */
  uint16 selections;      /* Number of regions or zones selected */
  uint16 regions;         /* Number of regions delimited by corner coordinates */
  struct region regionlist[MAX_REGIONS]; /* Regions within page or master crop region */
  uint16 zones;           /* Number of zones delimited by Ordinal:Total requested */
  struct zone zonelist[MAX_REGIONS]; /* Zones indices to define a region */
  struct coordpairs corners[MAX_REGIONS]; /* Coordinates of upper left and lower right corner */
};

#define MAX_PAPERNAMES 49
#define MAX_PAPERNAME_LENGTH 15
#define DEFAULT_RESUNIT      RESUNIT_INCH
#define DEFAULT_PAGE_HEIGHT   14.0
#define DEFAULT_PAGE_WIDTH     8.5
#define DEFAULT_RESOLUTION   300
#define DEFAULT_PAPER_SIZE  "legal"

#define ORIENTATION_NONE       0
#define ORIENTATION_PORTRAIT   1
#define ORIENTATION_LANDSCAPE  2
#define ORIENTATION_SEASCAPE   4
#define ORIENTATION_AUTO      16

#define PAGE_MODE_NONE         0
#define PAGE_MODE_RESOLUTION   1
#define PAGE_MODE_PAPERSIZE    2
#define PAGE_MODE_MARGINS      4
#define PAGE_MODE_ROWSCOLS     8

#define INVERT_DATA_ONLY      10
#define INVERT_DATA_AND_TAG   11

struct paperdef {
  char   name[MAX_PAPERNAME_LENGTH];
  double width;
  double length;
  double asratio;
  };

/* Paper Size       Width   Length  Aspect Ratio */
struct paperdef PaperTable[MAX_PAPERNAMES] = {
  {"default",         8.500,  14.000,  0.607},
  {"pa4",             8.264,  11.000,  0.751},
  {"letter",          8.500,  11.000,  0.773},
  {"legal",           8.500,  14.000,  0.607},
  {"half-letter",     8.500,   5.514,  1.542},
  {"executive",       7.264,  10.528,  0.690},
  {"tabloid",        11.000,  17.000,  0.647},
  {"11x17",          11.000,  17.000,  0.647},
  {"ledger",         17.000,  11.000,  1.545},
  {"archa",           9.000,  12.000,  0.750},
  {"archb",          12.000,  18.000,  0.667},
  {"archc",          18.000,  24.000,  0.750},
  {"archd",          24.000,  36.000,  0.667},
  {"arche",          36.000,  48.000,  0.750},
  {"csheet",         17.000,  22.000,  0.773},
  {"dsheet",         22.000,  34.000,  0.647},
  {"esheet",         34.000,  44.000,  0.773},
  {"superb",         11.708,  17.042,  0.687},
  {"commercial",      4.139,   9.528,  0.434},
  {"monarch",         3.889,   7.528,  0.517},
  {"envelope-dl",     4.333,   8.681,  0.499},
  {"envelope-c5",     6.389,   9.028,  0.708},
  {"europostcard",    4.139,   5.833,  0.710},
  {"a0",             33.111,  46.806,  0.707},
  {"a1",             23.389,  33.111,  0.706},
  {"a2",             16.542,  23.389,  0.707},
  {"a3",             11.694,  16.542,  0.707},
  {"a4",              8.264,  11.694,  0.707},
  {"a5",              5.833,   8.264,  0.706},
  {"a6",              4.125,   5.833,  0.707},
  {"a7",              2.917,   4.125,  0.707},
  {"a8",              2.056,   2.917,  0.705},
  {"a9",              1.458,   2.056,  0.709},
  {"a10",             1.014,   1.458,  0.695},
  {"b0",             39.375,  55.667,  0.707},
  {"b1",             27.833,  39.375,  0.707},
  {"b2",             19.681,  27.833,  0.707},
  {"b3",             13.903,  19.681,  0.706},
  {"b4",              9.847,  13.903,  0.708},
  {"b5",              6.931,   9.847,  0.704},
  {"b6",              4.917,   6.931,  0.709},
  {"c0",             36.097,  51.069,  0.707},
  {"c1",             25.514,  36.097,  0.707},
  {"c2",             18.028,  25.514,  0.707},
  {"c3",             12.750,  18.028,  0.707},
  {"c4",              9.014,  12.750,  0.707},
  {"c5",              6.375,   9.014,  0.707},
  {"c6",              4.486,   6.375,  0.704},
  {"",                0.000,   0.000,  1.000},
};

/* Structure to define in input image parameters */
struct image_data {
  float  xres;
  float  yres;
  uint32 width;
  uint32 length;
  uint16 res_unit;
  uint16 bps;
  uint16 spp;
  uint16 planar;
  uint16 photometric;
};

/* Structure to define the output image modifiers */
struct pagedef {
  char          name[16];
  double        width;    /* width in pixels */
  double        length;   /* length in pixels */
  double        hmargin;  /* margins to subtract from width of sections */
  double        vmargin;  /* margins to subtract from height of sections */
  double        hres;     /* horizontal resolution for output */
  double        vres;     /* vertical resolution for output */
  uint32        mode;     /* bitmask of modifiers to page format */
  uint16        res_unit; /* resolution unit for output image */
  unsigned int  rows;     /* number of section rows */
  unsigned int  cols;     /* number of section cols */
  unsigned int  orient;   /* portrait, landscape, seascape, auto */
};

/* globals */
static int    outtiled = -1;
static uint32 tilewidth;
static uint32 tilelength;

static uint16 config;
static uint16 compression;
static uint16 predictor;
static uint16 fillorder;
static uint16 orientation;
static uint32 rowsperstrip;
static uint32 g3opts;
static int    ignore = FALSE;		/* if true, ignore read errors */
static uint32 defg3opts = (uint32) -1;
static int    quality = 75;		/* JPEG quality */
static int    jpegcolormode = JPEGCOLORMODE_RGB;
static uint16 defcompression = (uint16) -1;
static uint16 defpredictor = (uint16) -1;
static int   pageNum = 0;

static	int processCompressOptions(char*);
static	void usage(void);

/* New functions by Richard Nolde  not found in tiffcp */
static void initImageData (struct image_data *);
static void initCropMasks (struct crop_mask *);
static void initPageSetup (struct pagedef *, struct pageseg *, struct buffinfo []);
static int  get_page_geometry (char *, struct pagedef*);
static int  computeInputPixelOffsets(struct crop_mask *, struct image_data *, 
                                     struct offset *);
static int  computeOutputPixelOffsets (struct crop_mask *, struct image_data *,
				       struct pagedef *, struct pageseg *);
static int  loadImage(TIFF *, struct image_data *, unsigned char **);
static int  getCropOffsets(struct image_data *, struct crop_mask *);
static int  processCropSelections(struct image_data *, struct crop_mask *, 
                                  unsigned char **, struct buffinfo []);
static int  writeSelections(TIFF *, TIFF **, struct crop_mask *, 
                            struct buffinfo [], char *, char *, 
                            unsigned int*, unsigned int);
static int  extractSeparateRegion(struct image_data *, struct crop_mask *,
				unsigned char *, unsigned char *, int);
static int  extractCompositeRegions(struct image_data *,  struct crop_mask *,
				  unsigned char *, unsigned char *);
/* Section functions */
static int  createImageSection(uint32, unsigned char **);
static int  extractImageSection(struct image_data *, struct pageseg *, 
                                unsigned char *, unsigned char *);
static int  writeSingleSection(TIFF *, TIFF *, uint32, uint32,
			       double, double, unsigned char *);
static int  writeImageSections(TIFF *, TIFF *, struct image_data *,
			       struct pagedef *, struct pageseg *,
			       unsigned char *, unsigned char **);
/* Whole image functions */
static int  createCroppedImage(struct image_data *, struct crop_mask *, 
                               unsigned char **, unsigned char **);
static int  writeCroppedImage(TIFF *, TIFF *, uint32, uint32,
			      unsigned char *, int, int);

/* Image manipulation functions */
static int  rotateImage(uint16, struct image_data *, uint32 *, uint32 *,
			unsigned char **);
static int  mirrorImage(uint16, uint16, uint16, uint32, uint32,
			unsigned char *);
static int  invertImage(uint16, uint16, uint16, uint32, uint32,
			unsigned char *);

void  process_command_opts (int, char *[], char *, char *, uint32 *,
	                    uint16 *, uint16 *, uint32 *, uint32 *, uint32 *,
		            struct crop_mask *, struct pagedef *, 
                            unsigned int *, unsigned int *);
static  int update_output_file (TIFF **, char *, int, char *, unsigned int *);

/* End function declarations */


void  process_command_opts (int argc, char *argv[], char *mp, char *mode, uint32 *dirnum,
	                    uint16 *defconfig, uint16 *deffillorder, uint32 *deftilewidth,
                            uint32 *deftilelength, uint32 *defrowsperstrip,
		            struct crop_mask *crop_data, struct pagedef *page, 
                            unsigned int     *imagelist, unsigned int   *image_count )
    {
    int   c;
    char *opt_offset   = NULL;    /* Position in string of value sought */
    char *opt_ptr      = NULL;    /* Pointer to next token in option set */
    char *sep          = NULL;    /* Pointer to a token separator */
    unsigned int  i, j, start, end;

    extern int optind;
    extern char* optarg;

    *mp++ = 'w';
    *mp = '\0';
    while ((c = getopt(argc, argv,
       "ac:d:e:f:hil:m:p:r:st:vw:z:BCE:F:H:I:J:K:LMN:O:P:R:S:U:V:X:Y:Z:")) != -1)
    switch (c) {
      case 'a': mode[0] = 'a';	/* append to output */
		break;
      case 'c':	if (!processCompressOptions(optarg)) /* compression scheme */
		   usage();
		break;
      case 'd':	*dirnum = strtoul(optarg, NULL, 0); /* initial IFD offset */
		break;
      case 'e': switch (tolower(optarg[0])) /* image export modes*/
                  {
		  case 'c': crop_data->exp_mode = ONE_FILE_COMPOSITE;
 		            crop_data->img_mode = COMPOSITE_IMAGES;
		            break; /* Composite */
		  case 'd': crop_data->exp_mode = ONE_FILE_SEPARATED;
 		            crop_data->img_mode = SEPARATED_IMAGES;
		            break; /* Divided */
		  case 'i': crop_data->exp_mode = FILE_PER_IMAGE_COMPOSITE;
 		            crop_data->img_mode = COMPOSITE_IMAGES;
		            break; /* Image */
		  case 'm': crop_data->exp_mode = FILE_PER_IMAGE_SEPARATED;
 		            crop_data->img_mode = SEPARATED_IMAGES;
		            break; /* Multiple */
		  case 's': crop_data->exp_mode = FILE_PER_SELECTION;
 		            crop_data->img_mode = SEPARATED_IMAGES;
		            break; /* Sections */
		  default:  usage();
                  }
	        break;
      case 'f':	if (streq(optarg, "lsb2msb"))	   /* fill order */
		  *deffillorder = FILLORDER_LSB2MSB;
		else if (streq(optarg, "msb2lsb"))
		  *deffillorder = FILLORDER_MSB2LSB;
		else
		  usage();
		break;
      case 'h':	usage();
		break;
      case 'i':	ignore = TRUE;		/* ignore errors */
		break;
      case 'l':	outtiled = TRUE;	 /* tile length */
		*deftilelength = atoi(optarg);
		break;
      case 'p': /* planar configuration */
		if (streq(optarg, "separate"))
		  *defconfig = PLANARCONFIG_SEPARATE;
		else if (streq(optarg, "contig"))
		  *defconfig = PLANARCONFIG_CONTIG;
		else
		  usage();
		break;
      case 'r':	/* rows/strip */
		*defrowsperstrip = atol(optarg);
		break;
      case 's':	/* generate stripped output */
		outtiled = FALSE;
		break;
      case 't':	/* generate tiled output */
		outtiled = TRUE;
		break;
      case 'v': printf ("Tiffcrop version %s, last updated: %s\n", 
			tiffcrop_version_id, tiffcrop_rev_date);
	        exit (0);
		break;
      case 'w':	/* tile width */
		outtiled = TRUE;
		*deftilewidth = atoi(optarg);
		break;
      case 'z': /* regions of an image specified as x1,y1,x2,y2:x3,y3,x4,y4 etc */
	        crop_data->crop_mode |= CROP_REGIONS;
		for (i = 0, opt_ptr = strtok (optarg, ":");
                   ((opt_ptr != NULL) &&  (i < MAX_REGIONS));
                    (opt_ptr = strtok (NULL, ":")), i++)
                    {
		    crop_data->regions++;
                    if (sscanf(opt_ptr, "%lf,%lf,%lf,%lf",
			       &crop_data->corners[i].X1, &crop_data->corners[i].Y1,
			       &crop_data->corners[i].X2, &crop_data->corners[i].Y2) != 4)
                      {
                      TIFFError ("process_command_opts", "Unable to parse coordinates for region %d", i);
                      exit (-1);
		      }
                    }
                /*  check for remaining elements over MAX_REGIONS */
                if ((opt_ptr != NULL) && (i >= MAX_REGIONS))
                  {
		  TIFFError ("process_command_opts", "Region list exceeds limit of %d regions\n", MAX_REGIONS);
		  exit (-1);
                  }
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
      case 'm': /* margins to exclude from selection, uppercase M was already used */
		/* order of values must be TOP, LEFT, BOTTOM, RIGHT */
		crop_data->crop_mode |= CROP_MARGINS;
                for (i = 0, opt_ptr = strtok (optarg, ",:");
                    ((opt_ptr != NULL) &&  (i < 4));
                     (opt_ptr = strtok (NULL, ",:")), i++)
                    {
		    crop_data->margins[i] = atof(opt_ptr);
                    }
		break;
      case 'E':	/* edge reference */
		switch (tolower(optarg[0]))
                  {
		  case 't': crop_data->edge_ref = EDGE_TOP;
                            break;
                  case 'b': crop_data->edge_ref = EDGE_BOTTOM;
                             break;
                  case 'l': crop_data->edge_ref = EDGE_LEFT;
                            break;
                  case 'r': crop_data->edge_ref = EDGE_RIGHT;
                            break;
		  default:  TIFFError ("process_command_opts", "Edge reference must be top, bottom, left, or right.\n");
			    usage();
		  }
		break;
      case 'F': /* flip eg mirror image or cropped segment, M was already used */
		crop_data->crop_mode |= CROP_MIRROR;
		switch (tolower(optarg[0]))
                  {
		  case  'h': crop_data->mirror = MIRROR_HORIZ;
                             break;
                  case  'v': crop_data->mirror = MIRROR_VERT;
                             break;
		  default:   TIFFError ("process_command_opts", "Flip mode must be h or v.\n");
			     usage();
		  }
		break;
      case 'H': /* set horizontal resolution to new value */
		page->hres = atof (optarg);
                page->mode |= PAGE_MODE_RESOLUTION;
		break;
      case 'I': /* invert the color space, eg black to white */
		crop_data->crop_mode |= CROP_INVERT;
                /* The PHOTOMETIC_INTERPRETATION tag may be updated */
                if (streq(optarg, "black"))
                  {
		  crop_data->photometric = PHOTOMETRIC_MINISBLACK;
		  continue;
                  }
                if (streq(optarg, "white"))
                  {
		  crop_data->photometric = PHOTOMETRIC_MINISWHITE;
                  continue;
                  }
                if (streq(optarg, "data")) 
                  {
		  crop_data->photometric = INVERT_DATA_ONLY;
                  continue;
                  }
                if (streq(optarg, "both"))
                  {
		  crop_data->photometric = INVERT_DATA_AND_TAG;
                  continue;
                  }

		TIFFError("process_command_opts", 
                "Missing or unknown option for inverting PHOTOMETRIC_INTERPRETATION");
		usage();

		break;
      case 'J': /* horizontal margin for sectioned ouput pages */ 
		page->hmargin = atof(optarg);
                page->mode |= PAGE_MODE_MARGINS;
		break;
      case 'K': /* vertical margin for sectioned ouput pages*/ 
                page->vmargin = atof(optarg);
                page->mode |= PAGE_MODE_MARGINS;
		break;
      case 'N':	/* list of images to process */
                for (i = 0, opt_ptr = strtok (optarg, ",");
                    ((opt_ptr != NULL) &&  (i < MAX_IMAGES));
                     (opt_ptr = strtok (NULL, ",")))
                     { /* We do not know how many images are in file yet 
			* so we build a list to include the maximum allowed
                        * and follow it until we hit the end of the file.
                        * Image count is not accurate for odd, even, last
                        * so page numbers won't be valid either.
                        */
		     if (streq(opt_ptr, "odd"))
                       {
		       for (j = 1; j <= MAX_IMAGES; j += 2)
			 imagelist[i++] = j;
                       *image_count = (MAX_IMAGES - 1) / 2;
                       break;
		       }
		     else
                       {
		       if (streq(opt_ptr, "even"))
                         {
			 for (j = 2; j <= MAX_IMAGES; j += 2)
			   imagelist[i++] = j;
                         *image_count = MAX_IMAGES / 2;
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
                *image_count = i;
		break;
      case 'O': /* page orientation */ 
		switch (tolower(optarg[0]))
                  {
		  case  'a': page->orient = ORIENTATION_AUTO;
                             break;
		  case  'p': page->orient = ORIENTATION_PORTRAIT;
                             break;
		  case  'l': page->orient = ORIENTATION_LANDSCAPE;
                             break;
		  default:  TIFFError ("process_command_opts", 
                            "Orientation must be portrait, landscape, or auto.\n\n");
			    usage();
		  }
		break;
      case 'P': /* page size selection */ 
                if (get_page_geometry (optarg, page))
                  {
		  if (!strcmp(optarg, "list"))
                    {
		    fprintf(stderr, "Name            Width   Length (in inches)\n");
                    for (i = 0; i < MAX_PAPERNAMES - 1; i++)
                      fprintf (stderr, "%-15.15s %5.2f   %5.2f%s", 
			       PaperTable[i].name, PaperTable[i].width, 
                               PaperTable[i].length, i % 2 ? "       " : "\n");
                    fprintf (stderr, "\n\n");
		    exit (-1);                   
                    }
     
		  fprintf (stderr, "Invalid paper size %s\n\n", optarg);
                  fprintf (stderr, "Select one of:\n");
                  for (i = 0; i < MAX_PAPERNAMES; i++)
                    fprintf (stderr, "%-15.15s%s", PaperTable[i].name, i % 5 ? "  " : "\n");
                  fprintf (stderr, "\n\n");
		  exit (-1);
		  }
		else
                  {
                  page->mode |= PAGE_MODE_PAPERSIZE;
		  }
		break;
      case 'R': /* rotate image or cropped segment */
		crop_data->crop_mode |= CROP_ROTATE;
		switch (strtoul(optarg, NULL, 0))
                  {
		  case  90:  crop_data->rotation = (uint16)90;
                             break;
                  case  180: crop_data->rotation = (uint16)180;
                             break;
                  case  270: crop_data->rotation = (uint16)270;
                             break;
		  default:   TIFFError ("process_command_opts", 
                            "Rotation must be 90, 180, or 270 degrees clockwise.\n\n");
			     usage();
		  }
		break;
      case 'S':	/* subdivide into Cols:Rows sections, eg 3:2 would be 3 across and 2 down */
		sep = strpbrk(optarg, ",:");
		if (sep)
                  {
                  *sep = '\0';
                  page->cols = atoi(optarg);
                  page->rows = atoi(sep +1);
		  }
                else
                  {
                  page->cols = atoi(optarg);
                  page->rows = atoi(optarg);
		  }
                if ((page->cols * page->rows) > MAX_SECTIONS)
                  {
		  TIFFError ("process_command_opts", 
                  "Limit of %d subdivisions, ie rows x columns, exceeded\n", MAX_SECTIONS);
		  exit (-1);
                  }
                page->mode |= PAGE_MODE_ROWSCOLS;
		break;
      case 'U':	/* units for measurements and offsets */
		if (streq(optarg, "in"))
                  {
		  crop_data->res_unit = RESUNIT_INCH;
		  page->res_unit = RESUNIT_INCH;
		  }
		else if (streq(optarg, "cm"))
		  {
		  crop_data->res_unit = RESUNIT_CENTIMETER;
		  page->res_unit = RESUNIT_CENTIMETER;
		  }
		else if (streq(optarg, "px"))
		  {
		  crop_data->res_unit = RESUNIT_NONE;
		  page->res_unit = RESUNIT_NONE;
		  }
		else
                  {
		  TIFFError ("process_command_opts", 
                             "Illegal unit of measure: %s\n\n", optarg);
		  usage();
		  }
		break;
      case 'V': /* set vertical resolution to new value */
		page->vres = atof (optarg);
                page->mode |= PAGE_MODE_RESOLUTION;
		break;
      case 'X':	/* selection width */
		crop_data->crop_mode |= CROP_WIDTH;
		crop_data->width = atof(optarg);
		break;
      case 'Y':	/* selection length */
		crop_data->crop_mode |= CROP_LENGTH;
		crop_data->length = atof(optarg);
		break;
      case 'Z': /* zones of an image X:Y read as zone X of Y */
		crop_data->crop_mode |= CROP_ZONES;
		for (i = 0, opt_ptr = strtok (optarg, ",");
                   ((opt_ptr != NULL) &&  (i < MAX_REGIONS));
                    (opt_ptr = strtok (NULL, ",")), i++)
                    {
		    crop_data->zones++;
		    opt_offset = strchr(opt_ptr, ':');
                    *opt_offset = '\0';
                    crop_data->zonelist[i].position = atoi(opt_ptr);
                    crop_data->zonelist[i].total    = atoi(opt_offset + 1);
                    }
                /*  check for remaining elements over MAX_REGIONS */
                if ((opt_ptr != NULL) && (i >= MAX_REGIONS))
                  {
		  TIFFError("process_command_opts",
                            "Zone list exceed limit of %d regions\n", MAX_REGIONS);
		  exit (-1);
                  }
		break;
      case '?':	usage();
		/*NOTREACHED*/
      }
    }  /* end process_command_opts */

/* Start a new output file if one has not been previously opened or
 * autoindex is set to non-zero. Update page and file counters
 * so TIFFTAG PAGENUM will be correct in image.
 */
static int 
update_output_file (TIFF **tiffout, char *mode, int autoindex,
                    char *outname, unsigned int *page)
  {
  static int findex = 0;    /* file sequence indicator */
  char  *sep;
  char   filenum[16];
  char   export_ext[6];
  char   exportname[PATH_MAX];

  if (autoindex && (*tiffout != NULL))
    {   
    /* Close any export file that was previously opened */
    TIFFClose (*tiffout);
    *tiffout = NULL;
    }

  strncpy (exportname, outname, PATH_MAX - 15);
  if (*tiffout == NULL)   /* This is a new export file */
    {
    if (autoindex)
      { /* create a new filename for each export */
      findex++;
      if ((sep = strstr(exportname, ".tif")) || (sep = strstr(exportname, ".TIF")))
        {
        strncpy (export_ext, sep, 5);
        *sep = '\0';
        }
      else
        {  /* find file name in path */
        if ((sep = strrchr(exportname, '/'))|| (sep = strstr(exportname, "\\")))
          {
          if ((sep = strrchr(sep, '.'))) /* possible file type extension */
            {
            strncpy (export_ext, sep, 5);
            *sep = '\0';
            }
	  }
        else
          strncpy (export_ext, ".tiff", 5);
        }
      export_ext[5] = '\0';

      sprintf (filenum, "-%03d%s", findex, export_ext);
      filenum[15] = '\0';
      strncat (exportname, filenum, 14);
      }

    *tiffout = TIFFOpen(exportname, mode);
    if (*tiffout == NULL)
      {
      TIFFError("update_output_file", "Unable to open output file %s\n", exportname);
      return (1);
      }
    *page = 0; 

    return (0);
    }
  else 
    (*page)++;

  return (0);
  } /* end update_output_file */


int
main(int argc, char* argv[])
  {
  uint16 defconfig = (uint16) -1;
  uint16 deffillorder = 0;
  uint32 deftilewidth = (uint32) -1;
  uint32 deftilelength = (uint32) -1;
  uint32 defrowsperstrip = (uint32) 0;
  uint32 dirnum = 0;

  TIFF *in = NULL;
  TIFF *out = NULL;
  char  mode[10];
  char *mp = mode;

  /** RJN additions **/
  int    seg;
  struct image_data image;     /* Image parameters for one image */
  struct crop_mask  crop;      /* Cropping parameters for all images */
  struct pagedef    page;      /* Page definition for output pages */
  struct pageseg   sections[MAX_SECTIONS];   /* Sections of one output page */
  struct buffinfo  seg_buffs[MAX_SECTIONS];  /* Segment buffer sizes and pointers */
  unsigned char *read_buff    = NULL;      /* Input image data buffer */
  unsigned char *crop_buff    = NULL;      /* Crop area buffer */
  unsigned char *sect_buff    = NULL;      /* Image section buffer */
  unsigned char *sect_src     = NULL;      /* Image section buffer pointer */
  unsigned int  imagelist[MAX_IMAGES + 1]; /* individually specified images */
  unsigned int  image_count  = 0;
  unsigned int  next_image   = 0;
  unsigned int  next_page    = 0;
  unsigned int  total_pages  = 0;
  unsigned int  total_images = 0;
  unsigned int  end_of_input = FALSE;

  initImageData(&image);
  initCropMasks(&crop);
  initPageSetup(&page, sections, seg_buffs);

  process_command_opts (argc, argv, mp, mode, &dirnum, &defconfig, 
                        &deffillorder, &deftilewidth, &deftilelength, &defrowsperstrip,
	                &crop, &page, imagelist, &image_count);

  if (argc - optind < 2)
    usage();

  if ((argc - optind) == 2)
    pageNum = -1;
  else
    total_images = 0;
  /* read multiple input files and write to output file(s) */
  while (optind < argc - 1)
    {
    in = TIFFOpen (argv[optind], "r");
    if (in == NULL)
      return (-3);

    /* If only one input file is specified, we can use directory count */
    total_images = TIFFNumberOfDirectories(in); 
    if (image_count == 0)
      {
      dirnum = 0;
      total_pages = total_images; /* Only valid with single input file */
      }
    else
      {
      dirnum = (tdir_t)(imagelist[next_image] - 1);
      next_image++;

      /* Total pages only valid for enumerated list of pages not derived
       * using odd, even, or last keywords.
       */
      if (image_count >  total_images)
	image_count = total_images;
      
      total_pages = image_count;
      }
    if (dirnum == (MAX_IMAGES - 1))
      dirnum = TIFFNumberOfDirectories(in) - 1;

    if (dirnum != 0 && !TIFFSetDirectory(in, (tdir_t)dirnum))
      {
      TIFFError(TIFFFileName(in),"Error, setting subdirectory at %#x", dirnum);
      (void) TIFFClose(out);
      return (1);
      }

    end_of_input = FALSE;
    while (end_of_input == FALSE)
      {
      config = defconfig;
      compression = defcompression;
      predictor = defpredictor;
      fillorder = deffillorder;
      rowsperstrip = defrowsperstrip;
      tilewidth = deftilewidth;
      tilelength = deftilelength;
      g3opts = defg3opts;

      if (loadImage(in, &image, &read_buff))
        {
        TIFFError("main", "Unable to load source image");
        exit (-1);
        }

      if (getCropOffsets(&image, &crop))
        {
        TIFFError("main", "Unable to define crop regions");
        exit (-1);
	}

      if (crop.selections > 0)
        {
        if (processCropSelections(&image, &crop, &read_buff, seg_buffs))
          {
          TIFFError("main", "Unable to process image selections");
          exit (-1);
	  }
	}
      else  /* Single image segment without zones or regions */
        {
        if (createCroppedImage(&image, &crop, &read_buff, &crop_buff))
          {
          TIFFError("main", "Unable to create output image");
          exit (-1);
	  }
	}
      if (page.mode == PAGE_MODE_NONE)
        {  /* Whole image or sections not based on output page size */
        if (crop.selections > 0)
          {
	  writeSelections(in, &out, &crop, seg_buffs, mp, argv[argc - 1], &next_page, total_pages);
          }
	else  /* One file all images and sections */
          {
	  if (update_output_file (&out, mp, crop.exp_mode, argv[argc - 1],
                                  &next_page))
             exit (1);
          if (writeCroppedImage(in, out, crop.combined_width, 
                                crop.combined_length, 
                                crop_buff, next_page, total_pages))
            {
             TIFFError("main", "Unable to write new image");
             exit (-1);
	    }
          }
	}
      else
        {  /* Break input image into pages or rows and columns */
	if (crop.crop_mode == CROP_NONE)
	  sect_src = read_buff;
        else
	  sect_src = crop_buff;

        if (computeOutputPixelOffsets(&crop, &image, &page, sections))
          {
          TIFFError("main", "Unable to compute output section data");
          exit (-1);
	  }

	if (update_output_file (&out, mp, crop.exp_mode, argv[argc - 1], &next_page))
          exit (1);

	if (writeImageSections(in, out, &image, &page, sections, sect_src, &sect_buff))
          {
          TIFFError("main", "Unable to write image sections");
          exit (-1);
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

      if (!TIFFSetDirectory(in, (tdir_t)dirnum))
        end_of_input = TRUE;;
      }
    TIFFClose(in);
    optind++;
    }

  /* If we did not use the read buffer as the crop buffer */
  if (read_buff)
    _TIFFfree(read_buff);

  if (crop_buff)
    _TIFFfree(crop_buff);

  if (sect_buff)
    _TIFFfree(sect_buff);

   /* Clean up any segment buffers used for zones or regions */
  for (seg = 0; seg < crop.selections; seg++)
    _TIFFfree (seg_buffs[seg].buffer);

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
" -h		print this syntax listing",
" -v		print tiffcrop version identifier and last revision date",
" ",
" -a		append to output instead of overwriting",
" -d offset	set initial directory offset",
" -p contig	pack samples contiguously (e.g. RGBRGB...)",
" -p separate	store samples separately (e.g. RRR...GGG...BBB...)",
" -s		write output in strips",
" -t		write output in tiles",
" -i		ignore read errors",
" ",
" -r #		make each strip have no more than # rows",
" -w #		set output tile width (pixels)",
" -l #		set output tile length (pixels)",
" ",
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
" ",
"Group 3 options:",
" 1d		use default CCITT Group 3 1D-encoding",
" 2d		use optional CCITT Group 3 2D-encoding",
" fill		byte-align EOL codes",
"For example, -c g3:2d:fill to get G3-2D-encoded data with byte-aligned EOLs",
" ",
"JPEG options:",
" #		set compression quality level (0-100, default 75)",
" r		output color image as RGB rather than YCbCr",
"For example, -c jpeg:r:50 to get JPEG-encoded RGB data with 50% comp. quality",
" ",
"LZW and deflate options:",
" #		set predictor value",
"For example, -c lzw:2 to get LZW-encoded data with horizontal differencing",
" ",
"Page and selection options:",
" -N odd|even|#,#-#,#|last         sequences and ranges of images within file to process",
"             the words odd or even may be used to specify all odd or even numbered images",
"             the word last may be used in place of a number in the sequence to indicate",
"             the final image in the file without knowing how many images there are",
" ",
" -E t|l|r|b  edge to use as origin for width and length of crop region",
" -U units    [in, cm, px ] inches, centimeters or pixels",
" ",
" -m #,#,#,#  margins from edges for selection: top, left, bottom, right separated by commas",
" -X #        horizontal dimension of region to extract expressed in current units",
" -Y #        vertical dimension of region to extract expressed in current units",
" -Z #:#,#:#  zones of the image designated as position X of Y,",
"             eg 1:3 would be first of three equal portions measured from reference edge",
" -z x1,y1,x2,y2:...:xN,yN,xN+1,yN+1",
"             regions of the image designated by upper left and lower right coordinates",
"",
"Export grouping options:",
" -e c|d|i|m|s    export mode for images and selections from input images.",
"                 When exporting a composite image from multiple zones or regions",
"                 (combined and image modes), the selections must have equal sizes",
"                 for the axis perpendicular to the edge specified with -E ",
"    c|combined   All images and selections are written to a single file (default)",
"                 with multiple selections from one image combined into a single image",
"    d|divided    All images and selections are written to a single file",
"                 with each selection from one image written to a new image",
"    i|image      Each input image is written to a new file (numeric filename sequence)",
"                 with multiple selections from the image combined into one image",
"    m|multiple   Each input image is written to a new file (numeric filename sequence)",
"                 with each selection from the image written to a new image",
"    s|separated  Individual selections from each image are written to separate files",
"",
"Output options:",
" -H #        set horizontal resolution of output images to #",
" -V #        set vertical resolution of output images to #",
" -J #        set horizontal margin of output page to # expressed in current units",
" -K #        set verticalal margin of output page to # expressed in current units",
" ",
" -O orient    orientation for output image, portrait, landscape, auto",
" -P page      page size for output image segments, eg letter, legal, tabloid, etc",
" -S cols:rows divide the image into equal sized segments using cols across and rows down",
" ",
" -F h|v      flip ie mirror image or extracted region horizontally or vertically",
" -R #        [90,180,or 270] degrees clockwise rotation of image or extracted region",
" -I [black|white|data|both]",
"             invert color space, eg dark to light for bilevel and grayscale images",
"             If argument is white or black, set the PHOTOMETRIC_INTERPRETATION ",
"             tag to MinIsBlack or MinIsWhite without altering the image data",
"             If the argument is data or both, the image data are modified:",
"             both inverts the data and the PHOTOMETRIC_INTERPRETATION tag,",
"             data inverts the data but not the PHOTOMETRIC_INTERPRETATION tag",
" ",
"      Note that images to process may be specified with -d # to process all",
"beginning at image # (numbering from zero) or by the -N option with a comma",
"separated list of images (numbered from one) which may include the word last or",
"the words odd or even to process all the odd or even numbered images",
" ",
"For example, -N 1,5-7,last to process the 1st, 5th through 7th, and final image",
NULL
};


static void
usage(void)
{
	char buf[BUFSIZ];
	int i;

	setbuf(stderr, buf);
        fprintf(stderr, "\n%s\n\n", TIFFGetVersion());
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


static int
get_page_geometry (char *name, struct pagedef *page)
    {
    char *ptr;
    int n; 

    for (ptr = name; *ptr; ptr++)
      *ptr = (char)tolower((int)*ptr);

    for (n = 0; n < MAX_PAPERNAMES; n++)
      {
      if (strcmp(name, PaperTable[n].name) == 0)
        {
	page->width = PaperTable[n].width;
	page->length = PaperTable[n].length;
        strncpy (page->name, PaperTable[n].name, 15);
        page->name[15] = '\0';
        return (0);
        }
      }

  return (1);
  }


static void
initPageSetup (struct pagedef *page, struct pageseg *pagelist, 
               struct buffinfo seg_buffs[])
   {
   int i; 

   strcpy (page->name, "");
   page->mode = PAGE_MODE_NONE;
   page->res_unit = RESUNIT_NONE;
   page->hres = 0.0;
   page->vres = 0.0;
   page->width = 0.0;
   page->length = 0.0;
   page->hmargin = 0.0;
   page->vmargin = 0.0;
   page->rows = 0;
   page->cols = 0;
   page->orient = ORIENTATION_NONE;

   for (i = 0; i < MAX_SECTIONS; i++)
     {
     pagelist[i].x1 = (uint32)0;
     pagelist[i].x2 = (uint32)0;
     pagelist[i].y1 = (uint32)0;
     pagelist[i].y2 = (uint32)0;
     pagelist[i].buffsize = (uint32)0;
     pagelist[i].position = 0;
     pagelist[i].total = 0;
     }

   for (i = 0; i < MAX_OUTBUFFS; i++)
     {
     seg_buffs[i].size = 0;
     seg_buffs[i].buffer = NULL;
     }
   }

static void
initImageData (struct image_data *image)
  {
  image->xres = 0.0;
  image->yres = 0.0;
  image->width = 0;
  image->length = 0;
  image->res_unit = RESUNIT_NONE;
  image->bps = 0;
  image->spp = 0;
  image->planar = 0;
  image->photometric = 0;
  }

static void
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
   cps->photometric = INVERT_DATA_AND_TAG;
   cps->mirror   = (uint16)0;
   cps->invert   = (uint16)0;
   cps->zones    = (uint32)0;
   cps->regions  = (uint32)0;
   for (i = 0; i < MAX_REGIONS; i++)
     {
     cps->corners[i].X1 = 0.0;
     cps->corners[i].X2 = 0.0;
     cps->corners[i].Y1 = 0.0;
     cps->corners[i].Y2 = 0.0;
     cps->regionlist[i].x1 = 0;
     cps->regionlist[i].x2 = 0;
     cps->regionlist[i].y1 = 0;
     cps->regionlist[i].y2 = 0;
     cps->regionlist[i].width = 0;
     cps->regionlist[i].length = 0;
     cps->regionlist[i].buffsize = 0;
     cps->regionlist[i].buffptr = NULL;
     cps->zonelist[i].position = 0;
     cps->zonelist[i].total = 0;
     }
   cps->exp_mode = ONE_FILE_COMPOSITE;
   cps->img_mode = COMPOSITE_IMAGES;
   }

/* Compute pixel offsets into the image for margins and fixed regions */
static int
computeInputPixelOffsets(struct crop_mask *crop, struct image_data *image,
                         struct offset *off)
  {
  double scale;
  float xres, yres;
  /* Values for these offsets are in pixels from start of image, not bytes
   * since some images may have more than 8 bits per pixel */
  uint32 tmargin, bmargin, lmargin, rmargin;
  uint32 startx, endx;   /* offsets of first and last columns to extract */
  uint32 starty, endy;   /* offsets of first and last row to extract */
  uint32 width, length, crop_width, crop_length; 
  uint32 i, max_width, max_length, zwidth, zlength, buffsize;
  double x1, x2, y1, y2;

  if (image->res_unit != RESUNIT_INCH && image->res_unit != RESUNIT_CENTIMETER)
    {
    xres = 1.0;
    yres = 1.0;
    }
  else
    {
    if (((image->xres == 0) || (image->yres == 0)) &&
	((crop->crop_mode & CROP_REGIONS) || (crop->crop_mode & CROP_MARGINS) ||
 	 (crop->crop_mode & CROP_LENGTH)  || (crop->crop_mode & CROP_WIDTH)))
      {
      TIFFError("computeInputPixelOffsets", "Cannot compute margins or fixed size sections without image resolution");
      TIFFError("computeInputPixelOffsets", "Specify units in pixels and try again");
      return (-1);
      }
    xres = image->xres;
    yres = image->yres;
    }

  /* Translate user units to image units */
  scale = 1.0;
  switch (crop->res_unit) {
    case RESUNIT_CENTIMETER:
         if (image->res_unit == RESUNIT_INCH)
	   scale = 1.0/2.54;
	 break;
    case RESUNIT_INCH:
	 if (image->res_unit == RESUNIT_CENTIMETER)
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
           (image->res_unit == RESUNIT_INCH) ? "inch" :
	   ((image->res_unit == RESUNIT_CENTIMETER) ? "centimeter" : "pixel"));
#endif

  if (crop->crop_mode & CROP_REGIONS)
    {
    max_width = max_length = 0;
    for (i = 0; i < crop->regions; i++)
      {
      if ((crop->res_unit == RESUNIT_INCH) || (crop->res_unit == RESUNIT_CENTIMETER))
        {
        x1 = crop->corners[i].X1 * scale * xres;
        x2 = crop->corners[i].X2 * scale * xres;
        y1 = crop->corners[i].Y1 * scale * yres;
        y2 = crop->corners[i].Y2 * scale * yres;
        }
      else
        {
        x1 = crop->corners[i].X1;
        x2 = crop->corners[i].X2;
        y1 = crop->corners[i].Y1;
        y2 = crop->corners[i].Y2;       
	}
      if (x1 < 1)
        crop->regionlist[i].x1 = 0;
      else
        crop->regionlist[i].x1 = x1 - 1;

      if (x2 > image->width - 1)
        crop->regionlist[i].x2 = image->width - 1;
      else
        crop->regionlist[i].x2 = x2 - 1;
      zwidth  = crop->regionlist[i].x2 - crop->regionlist[i].x1 + 1; 

      if (y1 < 1)
        crop->regionlist[i].y1 = 0;
      else
        crop->regionlist[i].y1 = y1 - 1;

      if (y2 > image->length - 1)
        crop->regionlist[i].y2 = image->length - 1;
      else
        crop->regionlist[i].y2 = y2 - 1;

      zlength = crop->regionlist[i].y2 - crop->regionlist[i].y1 + 1; 

      if (zwidth > max_width)
        max_width = zwidth;
      if (zlength > max_length)
        max_length = zlength;

      buffsize = (uint32)
	((ceil)(((zwidth * image->bps + 7 ) / 8) * image->spp) * (ceil(zlength)));
      crop->regionlist[i].buffsize = buffsize;
      crop->bufftotal += buffsize;
      if (crop->img_mode == COMPOSITE_IMAGES)
        {
        switch (crop->edge_ref)
          {
          case EDGE_LEFT:
          case EDGE_RIGHT:
               crop->combined_length = zlength;
               crop->combined_width += zwidth;
               break;
          case EDGE_BOTTOM:
          case EDGE_TOP:  /* width from left, length from top */
          default:
               crop->combined_width = zwidth;
               crop->combined_length += zlength;
	       break;
          }
	}
      }
    return (0);
    }
  
  /* Convert crop margins into offsets into image
   * Margins are expressed as pixel rows and columns, not bytes
   */
  if (crop->crop_mode & CROP_MARGINS)
    {
    if (crop->res_unit != RESUNIT_INCH && crop->res_unit != RESUNIT_CENTIMETER)
      { /* User has specified pixels as reference unit */
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

    if ((lmargin + rmargin) > image->width)
      {
      TIFFError("computeInputPixelOffsets", "Combined left and right margins exceed image width");
      lmargin = (uint32) 0;
      rmargin = (uint32) 0;
      return (-1);
      }
    if ((tmargin + bmargin) > image->length)
      {
      TIFFError("computeInputPixelOffsets", "Combined top and bottom margins exceed image length"); 
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

  /* Width, height, and margins are expressed as pixel offsets into image */
  if (crop->res_unit != RESUNIT_INCH && crop->res_unit != RESUNIT_CENTIMETER)
    {
    if (crop->crop_mode & CROP_WIDTH)
      width = (uint32)crop->width;
    else
      width = image->width - lmargin - rmargin;

    if (crop->crop_mode & CROP_LENGTH)
      length  = (uint32)crop->length;
    else
      length = image->length - tmargin - bmargin;
    }
  else
    {
    if (crop->crop_mode & CROP_WIDTH)
      width = (uint32)(crop->width * scale * image->xres);
    else
      width = image->width - lmargin - rmargin;

    if (crop->crop_mode & CROP_LENGTH)
      length  = (uint32)(crop->length * scale * image->yres);
    else
      length = image->length - tmargin - bmargin;
    }

  off->tmargin = tmargin;
  off->bmargin = bmargin;
  off->lmargin = lmargin;
  off->rmargin = rmargin;

  /* Calculate regions defined by margins, width, and length. 
   * Coordinates expressed as 0 to imagewidth - 1, imagelength - 1,
   * since they are used to compute offsets into buffers */
  switch (crop->edge_ref) {
    case EDGE_BOTTOM:
         startx = lmargin;
         if ((startx + width) >= (image->width - rmargin))
           endx = image->width - rmargin - 1;
         else
           endx = startx + width - 1;

         endy = image->length - bmargin - 1;
         if ((endy - length) <= tmargin)
           starty = tmargin;
         else
           starty = endy - length + 1;
         break;
    case EDGE_RIGHT:
         endx = image->width - rmargin - 1;
         if ((endx - width) <= lmargin)
           startx = lmargin;
         else
           startx = endx - width + 1;

         starty = tmargin;
         if ((starty + length) >= (image->length - bmargin))
           endy = image->length - bmargin - 1;
         else
           endy = starty + length - 1;
         break;
    case EDGE_TOP:  /* width from left, length from top */
    case EDGE_LEFT:
    default:
         startx = lmargin;
         if ((startx + width) >= (image->width - rmargin))
           endx = image->width - rmargin - 1;
         else
           endx = startx + width - 1;

         starty = tmargin;
         if ((starty + length) >= (image->length - bmargin))
           endy = image->length - bmargin - 1;
         else
           endy = starty + length - 1;
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
    TIFFError("computeInputPixelOffsets", 
               "Invalid left/right margins and /or image crop width requested");
    return (-1);
    }
  if (crop_width > image->width)
    crop_width = image->width;

  if (crop_length <= 0)
    {
    TIFFError("computeInputPixelOffsets", 
              "Invalid top/bottom margins and /or image crop length requested");
    return (-1);
    }
  if (crop_length > image->length)
    crop_length = image->length;

  off->crop_width = crop_width;
  off->crop_length = crop_length;

#ifdef DEBUG
  fprintf (stderr, "Startx: %d endx: %d Starty: %d endy: %d  Crop width: %d length: %d\n",
           startx, endx, starty, endy, crop_width, crop_length);
#endif

  return (0);
  } /* end computeInputPixelOffsets */

/* 
 * Translate crop options into pixel offsets for one or more regions of the image.
 * Options are applied in this order: margins, specific width and length, zones,
 * but all are optional. Margins are relative to each edge. Width, length and
 * zones are relative to the specified reference edge. Zones are expressed as
 * X:Y where X is the ordinal value in a set of Y equal sized portions. eg.
 * 2:3 would indicate the middle third of the region qualified by margins and
 * any explicit width and length specified. Regions are specified by coordinates
 * of the top left and lower right corners with range 1 to width or height.
 */

static int
getCropOffsets(struct image_data *image, struct crop_mask *crop)
  {
  struct offset offsets;
  int    i;
  uint32 seg, total, need_buff = 0;
  uint32 test, buffsize;
  double zwidth, zlength;

  memset(&offsets, '\0', sizeof(struct offset));
  crop->bufftotal = 0;
  crop->combined_width  = (uint32)0;
  crop->combined_length = (uint32)0;
  crop->selections = 0;

  /* Compute pixel offsets if margins or fixed width or length specified */
  if ((crop->crop_mode & CROP_MARGINS) ||
      (crop->crop_mode & CROP_REGIONS) ||
      (crop->crop_mode & CROP_LENGTH)  || 
      (crop->crop_mode & CROP_WIDTH))
    {
    if (computeInputPixelOffsets(crop, image, &offsets))
      {
      TIFFError ("getCropOffsets", "Unable to compute crop margins");
      return (-1);
      }
    need_buff = TRUE;
    crop->selections = crop->regions;
    /* Regions are only calculated from top and left edges with no margins */
    if (crop->crop_mode & CROP_REGIONS)
      return (0);
    }
  else
    { /* cropped area is the full image */
    offsets.tmargin = 0;
    offsets.lmargin = 0;
    offsets.bmargin = 0;
    offsets.rmargin = 0;
    offsets.crop_width = image->width;
    offsets.crop_length = image->length;
    offsets.startx = 0;
    offsets.endx = image->width - 1;
    offsets.starty = 0;
    offsets.endy = image->length - 1;
    need_buff = FALSE;
    }

#ifdef DEBUG
  fprintf (stderr, "Margins: Top: %d  Left: %d  Bottom: %d  Right: %d\n", 
           offsets.tmargin, offsets.lmargin, offsets.bmargin, offsets.rmargin); 
  fprintf (stderr, "Crop region within margins: Adjusted Width:  %6d  Length: %6d\n\n", 
           offsets.crop_width, offsets.crop_length);
#endif

  if (!(crop->crop_mode & CROP_ZONES)) /* no crop zones requested */
    {
    if (need_buff == FALSE)  /* No margins or fixed width or length areas */
      {
      crop->selections = 0;
      /*
      crop->zones = 0;
      */
      crop->combined_width  = image->width;
      crop->combined_length = image->length;
      return (0);
      }
    else 
      {
      /* Use one region for margins and fixed width or length areas
       * even though it was not formally declared as a region.
       */
      crop->selections = 1;
      crop->zones = 1;
      crop->zonelist[0].total = 1;
      crop->zonelist[0].position = 1;
      }
    }     
  else
    crop->selections = crop->zones;

  for (i = 0; i < crop->zones; i++)
    {
    seg = crop->zonelist[i].position;
    total = crop->zonelist[i].total;

    switch (crop->edge_ref) 
      {
      case EDGE_LEFT: /* zones from left to right, length from top */
           zlength = offsets.crop_length;
	   crop->regionlist[i].y1 = offsets.starty;
           crop->regionlist[i].y2 = offsets.endy;

           crop->regionlist[i].x1 = offsets.startx + 
                                  (uint32)(offsets.crop_width * 1.0 * (seg - 1) / total);
           test = offsets.startx + 
                  (uint32)(offsets.crop_width * 1.0 * seg / total);
           if (test > image->width - 1)
             crop->regionlist[i].x2 = image->width - 1;
           else
             crop->regionlist[i].x2 = test - 1;
           zwidth = crop->regionlist[i].x2 - crop->regionlist[i].x1  + 1;

	   /* This is passed to extractCropZone or extractCompositeZones */
           crop->combined_length = (uint32)zlength;
           if (crop->exp_mode == COMPOSITE_IMAGES)
             crop->combined_width += (uint32)zwidth;
           else
             crop->combined_width = (uint32)zwidth;
           break;
      case EDGE_BOTTOM: /* width from left, zones from bottom to top */
           zwidth = offsets.crop_width;
	   crop->regionlist[i].x1 = offsets.startx;
           crop->regionlist[i].x2 = offsets.endx;

           test = offsets.endy - (uint32)(offsets.crop_length * 1.0 * seg / total);
           if (test < 1 )
	     crop->regionlist[i].y1 = 0;
           else
	     crop->regionlist[i].y1 = test + 1;

           test = offsets.endy - (uint32)(offsets.crop_length * 1.0 * (seg - 1) / total);
           if (test > image->length - 1)
             crop->regionlist[i].y2 = image->length - 1;
           else 
             crop->regionlist[i].y2 = test;
           zlength = crop->regionlist[i].y2 - crop->regionlist[i].y1 + 1;

	   /* This is passed to extractCropZone or extractCompositeZones */
           if (crop->exp_mode == COMPOSITE_IMAGES)
             crop->combined_length += (uint32)zlength;
           else
             crop->combined_length = (uint32)zlength;
           crop->combined_width = (uint32)zwidth;
           break;
      case EDGE_RIGHT: /* zones from right to left, length from top */
           zlength = offsets.crop_length;
	   crop->regionlist[i].y1 = offsets.starty;
           crop->regionlist[i].y2 = offsets.endy;

           crop->regionlist[i].x1 = offsets.startx +
                                  (uint32)(offsets.crop_width  * (total - seg) * 1.0 / total);
           test = offsets.startx + 
	          (uint32)(offsets.crop_width * (total - seg + 1) * 1.0 / total);

           if (test > image->width - 1)
             crop->regionlist[i].x2 = image->width - 1;
           else
             crop->regionlist[i].x2 = test - 1;
           zwidth = crop->regionlist[i].x2 - crop->regionlist[i].x1  + 1;

	   /* This is passed to extractCropZone or extractCompositeZones */
           crop->combined_length = (uint32)zlength;
           if (crop->exp_mode == COMPOSITE_IMAGES)
             crop->combined_width += (uint32)zwidth;
           else
             crop->combined_width = (uint32)zwidth;
           break;
      case EDGE_TOP: /* width from left, zones from top to bottom */
      default:
           zwidth = offsets.crop_width;
	   crop->regionlist[i].x1 = offsets.startx;
           crop->regionlist[i].x2 = offsets.endx;

           crop->regionlist[i].y1 = offsets.starty + (uint32)(offsets.crop_length * 1.0 * (seg - 1) / total);
           test = offsets.starty + (uint32)(offsets.crop_length * 1.0 * seg / total);
	   if (test > image->length - 1)
	     crop->regionlist[i].y2 = image->length - 1;
           else
	     crop->regionlist[i].y2 = test - 1;
           zlength = crop->regionlist[i].y2 - crop->regionlist[i].y1 + 1;

	   /* This is passed to extractCropZone or extractCompositeZones */
           if (crop->exp_mode == COMPOSITE_IMAGES)
             crop->combined_length += (uint32)zlength;
           else
             crop->combined_length = (uint32)zlength;
           crop->combined_width = (uint32)zwidth;
           break;
      } /* end switch statement */
    buffsize = (uint32)((ceil)(((zwidth * image->bps + 7 ) / 8) * image->spp)
		    * (ceil(zlength)));
    crop->regionlist[i].width = zwidth;
    crop->regionlist[i].length = zlength;
    crop->regionlist[i].buffsize = buffsize;
    crop->bufftotal += buffsize;

#ifdef DEBUG
  fprintf (stderr, "Zone %d, width: %4d, length: %4d, x1: %4d  x2: %4d  y1: %4d  y2: %4d\n",
                    i + 1, (uint32)zwidth, (uint32)zlength,
		    crop->regionlist[i].x1, crop->regionlist[i].x2, 
                    crop->regionlist[i].y1, crop->regionlist[i].y2);
#endif
    }

  return (0);
  } /* end getCropOffsets */


static int
computeOutputPixelOffsets (struct crop_mask *crop, struct image_data *image,
                           struct pagedef *page, struct pageseg *sections)
  {
  double scale;
  uint32 iwidth, ilength;
  uint32 owidth, olength;
  uint32 orows, ocols;                   /* rows and cols for output */
  uint32 hmargin, vmargin;
  uint32 x1, x2, y1, y2, line_bytes;
  unsigned int orientation;
  uint32 i, j, k;
 
  scale = 1.0;
  if (page->res_unit == RESUNIT_NONE)
    page->res_unit = image->res_unit;

  switch (image->res_unit) {
    case RESUNIT_CENTIMETER:
         if (page->res_unit == RESUNIT_INCH)
	   scale = 1.0/2.54;
	 break;
    case RESUNIT_INCH:
	 if (page->res_unit == RESUNIT_CENTIMETER)
	     scale = 2.54;
	 break;
    case RESUNIT_NONE: /* Dimensions in pixels */
    default:
    break;
    }

  /* get width, height, resolutions of input image selection */
  if (crop->combined_width > 0)
    iwidth = crop->combined_width;
  else
    iwidth = image->width;
  if (crop->combined_length > 0)
    ilength = crop->combined_length;
  else
    ilength = image->length;

  if (page->hres <= 1.0)
    page->hres = image->xres;
  if (page->vres <= 1.0)
    page->vres = image->yres;

  if ((page->hres < 1.0) || (page->vres < 1.0))
    {
    TIFFError("computeOutputPixelOffsets",
    "Invalid horizontal or vertical resolution specified or read from input image");
    return (1);
    }

#ifdef DEBUG
  fprintf (stderr, "Page size: %s, Vres: %3.2f, Hres: %3.2f, "
                   "Hmargin: %3.2f, Vmargin: %3.2f\n",
	   page->name, page->vres, page->hres,
           page->hmargin, page->vmargin);
  fprintf (stderr, "Res_unit: %d, Scale: %3.2f, Page width: %3.2f, length: %3.2f\n", 
           page->res_unit, scale, page->width, page->length);
#endif

  /* compute margins at specified unit and resolution */
  if (page->mode & PAGE_MODE_MARGINS)
    {
    if (page->res_unit == RESUNIT_INCH || page->res_unit == RESUNIT_CENTIMETER)
      { /* inches or centimeters specified */
      hmargin = (uint32)(page->hmargin * scale * page->hres * ((image->bps + 7)/ 8));
      vmargin = (uint32)(page->vmargin * scale * page->vres * ((image->bps + 7)/ 8));
      }
    else
      { /* Otherwise user has specified pixels as reference unit */
      hmargin = (uint32)(page->hmargin * scale * ((image->bps + 7)/ 8));
      vmargin = (uint32)(page->vmargin * scale * ((image->bps + 7)/ 8));
      }

    if ((hmargin * 2.0) > (page->width * page->hres))
      {
      TIFFError("computeOutputPixelOffsets", 
                "Combined left and right margins exceed page width");
      hmargin = (uint32) 0;
      return (-1);
      }
    if ((vmargin * 2.0) > (page->length * page->vres))
      {
      TIFFError("computeOutputPixelOffsets", 
                "Combined top and bottom margins exceed page length"); 
      vmargin = (uint32) 0; 
      return (-1);
      }
    }
  else
    {
    hmargin = 0;
    vmargin = 0;
    }

  if (page->mode & PAGE_MODE_ROWSCOLS )
    {
    /* Maybe someday but not for now */
    if (page->mode & PAGE_MODE_MARGINS)
      TIFFError("computeOutputPixelOffsets", 
      "Output margins cannot be specified with rows and columns"); 

    owidth  = TIFFhowmany(iwidth, page->cols);
    olength = TIFFhowmany(ilength, page->rows);
    }
  else
    {
    if (page->mode & PAGE_MODE_PAPERSIZE )
      {
      owidth  = (uint32)((page->width * page->hres) - (hmargin * 2));
      olength = (uint32)((page->length * page->vres) - (vmargin * 2));
      }
    else
      {
      owidth = (uint32)(iwidth - (hmargin * 2 * page->hres));
      olength = (uint32)(ilength - (vmargin * 2 * page->vres));
      }
    }

  if (owidth > iwidth)
    owidth = iwidth;
  if (olength > ilength)
    olength = ilength;

  /* Compute the number of pages required for Portrait or Landscape */
  switch (page->orient)
    {
    case ORIENTATION_NONE:
    case ORIENTATION_PORTRAIT:
         ocols = TIFFhowmany(iwidth, owidth);
         orows = TIFFhowmany(ilength, olength);
         orientation = ORIENTATION_PORTRAIT;
         break;

    case ORIENTATION_LANDSCAPE:
         ocols = TIFFhowmany(iwidth, olength);
         orows = TIFFhowmany(ilength, owidth);
         x1 = olength;
         olength = owidth;
         owidth = x1;
         orientation = ORIENTATION_LANDSCAPE;
         break;

    case ORIENTATION_AUTO:
    default:
         x1 = TIFFhowmany(iwidth, owidth);
         x2 = TIFFhowmany(ilength, olength); 
         y1 = TIFFhowmany(iwidth, olength);
         y2 = TIFFhowmany(ilength, owidth); 

         if ( (x1 * x2) < (y1 * y2))
           { /* Portrait */
           ocols = x1;
           orows = x2;
           orientation = ORIENTATION_PORTRAIT;
	   }
         else
           { /* Landscape */
           ocols = y1;
           orows = y2;
           x1 = olength;
           olength = owidth;
           owidth = x1;
           orientation = ORIENTATION_LANDSCAPE;
           }
    }

  if (ocols < 1)
    ocols = 1;
  if (orows < 1)
    orows = 1;

  /* If user did not specify rows and cols, set them from calcuation */
  if (page->rows < 1)
    page->rows = orows;
  if (page->cols < 1)
    page->cols = ocols;

  line_bytes = TIFFhowmany8(owidth * image->bps) * image->spp;

#ifdef DEBUG
fprintf (stderr, "\nBest fit:  rows: %d,  cols:  %d,  orientation: %s\n", 
         orows, ocols,
         orientation == ORIENTATION_PORTRAIT ? "portrait" : "landscape");
fprintf (stderr, "Width: %d, Length: %d, Hmargin: %d, Vmargin: %d\n\n",
	 owidth, olength, hmargin, vmargin);
#endif

 if ((page->rows * page->cols) > MAX_SECTIONS)
   {
   TIFFError("computeOutputPixelOffsets",
	     "Rows and Columns exceed maximum sections\nIncrease resolution or reduce sections");
   return (-1);
   }

  /* build the list of offsets for each output section */
  for (k = 0, i = 0 && k <= MAX_SECTIONS; i < orows; i++)
    {
    y1 = (uint32)(olength * i);
    y2 = (uint32)(olength * (i +  1) - 1);
    if (y2 >= ilength)
      y2 = ilength - 1;
    for (j = 0; j < ocols; j++, k++)
      {
      x1 = (uint32)(owidth * j); 
      x2 = (uint32)(owidth * (j + 1) - 1);
      if (x2 >= iwidth)
        x2 = iwidth - 1;
      sections[k].x1 = x1;
      sections[k].x2 = x2;
      sections[k].y1 = y1;
      sections[k].y2 = y2;
      sections[k].buffsize = line_bytes * olength;
      sections[k].position = k + 1;
      sections[k].total = orows * ocols;
#ifdef DEBUG
     fprintf (stderr, "Sect %d, width: %4d, length: %4d, x1: %4d  x2: %4d  y1: %4d  y2: %4d\n",
                 k + 1, sections[k].x2 - sections[k].x1 + 1, sections[k].y2 - sections[k].y1 + 1,
		 sections[k].x1, sections[k].x2, sections[k].y1, sections[k].y2);
#endif
      } 
    } 
  return (0);
  } /* end computeOutputPixelOffsets */

static int
loadImage(TIFF* in, struct image_data *image, unsigned char **read_ptr)
  {
  float    xres, yres;
  uint16   nstrips, ntiles, planar, bps, spp, res_unit, photometric;
  uint32   width, length;
  uint32   stsize, tlsize, buffsize;
  unsigned char *read_buff = NULL;
  unsigned char *new_buff  = NULL;
  int      readunit = 0;
  static   uint32  prev_readsize = 0;

  TIFFGetFieldDefaulted(in, TIFFTAG_BITSPERSAMPLE, &bps);
  TIFFGetFieldDefaulted(in, TIFFTAG_SAMPLESPERPIXEL, &spp);
  TIFFGetFieldDefaulted(in, TIFFTAG_PLANARCONFIG, &planar);
  TIFFGetField(in, TIFFTAG_PHOTOMETRIC, &photometric);
  TIFFGetField(in, TIFFTAG_IMAGEWIDTH,  &width);
  TIFFGetField(in, TIFFTAG_IMAGELENGTH, &length);
  TIFFGetField(in, TIFFTAG_XRESOLUTION, &xres);
  TIFFGetField(in, TIFFTAG_YRESOLUTION, &yres);
  TIFFGetFieldDefaulted(in, TIFFTAG_RESOLUTIONUNIT, &res_unit);

  image->bps = bps;
  image->spp = spp;
  image->planar = planar;
  image->width = width;
  image->length = length;
  image->xres = xres;
  image->yres = yres;
  image->res_unit = res_unit;
  image->photometric = photometric;

  if ((bps == 0) || (spp == 0))
    {
    TIFFError("loadImage", "Invalid samples per pixel (%d) or bits per sample (%d)",
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
      {
      new_buff = _TIFFrealloc(read_buff, buffsize);
      if (!new_buff)
        {
	free (read_buff);
        read_buff = (unsigned char *)_TIFFmalloc(buffsize);
        }
      else
        read_buff = new_buff;
      }
    }

  if (!read_buff)
    {
    TIFFError("loadImageImage", "Unable to allocate/reallocate read buffer");
    return (-1);
    }
  _TIFFmemset(read_buff, '\0', buffsize);
  prev_readsize = buffsize;
  *read_ptr = read_buff;

  /* read current image into memory */
  switch (readunit) {
    case STRIP:
         if (planar == PLANARCONFIG_CONTIG)
           {
	   if (!(readContigStripsIntoBuffer(in, read_buff, length, width, spp)))
	     {
	     TIFFError("loadImage", "Unable to read contiguous strips into buffer");
	     return (-1);
             }
           }
         else
           {
           if (!(readSeparateStripsIntoBuffer(in, read_buff, length, width, spp)))
	     {
	     TIFFError("loadImage", "Unable to read separate strips into buffer");
	     return (-1);
             }
           }
         break;

    case TILE:
         if (planar == PLANARCONFIG_CONTIG)
           {
	   if (!(readContigTilesIntoBuffer(in, read_buff, length, width, spp)))
	     {
	     TIFFError("loadImage", "Unable to read contiguous tiles into buffer");
	     return (-1);
             }
           }
         else
           {
	     if (!(readSeparateTilesIntoBuffer(in, read_buff, length, width, spp)))
	     {
	     TIFFError("loadImage", "Unable to read separate tiles into buffer");
	     return (-1);
             }
           }
         break;
    default: TIFFError("loadImage", "Unsupported image file format");
          return (-1);
          break;
    }

  return (0);
  }   /* end loadImage */

/* Extract multiple zones from an image and combine into a single composite image */
static int
extractCompositeRegions(struct image_data *image,  struct crop_mask *crop, 
                        unsigned char *read_buff, unsigned char *crop_buff)
  {
  uint32    i, j, shift1, shift2, shift3, trailing_bits, prev_trailing_bits;
  uint32    row, first_row, last_row, first_col, last_col;
  uint32    src_offset, dst_offset, row_offset, col_offset;
  uint32    offset1, offset2, full_bytes, rowsize;
  uint32    crop_width, crop_length, img_width, img_length;
  uint32    prev_width, composite_width;
  uint16    bps, spp;
  unsigned  char  bytebuff1, bytebuff2, src_byte;
  unsigned  char *src_ptr, *dst_ptr;

  img_width = image->width;
  img_length = image->length;
  bps = image->bps;
  spp = image->spp;

  src_ptr = read_buff;
  src_offset = 0;
  dst_ptr = crop_buff;
  dst_offset = 0;

  rowsize = spp * ((img_width * bps + 7) / 8);
  prev_width = 0;
  prev_trailing_bits = 0;
  composite_width = crop->combined_width;
  crop->combined_width = 0;
  crop->combined_length = 0;

  for (i = 0; i < crop->selections; i++)
    {
    /* rows, columns, width, length are expressed in pixels */
    first_row = crop->regionlist[i].y1;
    last_row  = crop->regionlist[i].y2;
    first_col = crop->regionlist[i].x1;
    last_col  = crop->regionlist[i].x2;

    crop_width = last_col - first_col + 1;
    crop_length = last_row - first_row + 1;

    /* number of COMPLETE bytes per row in crop area */
    full_bytes = (crop_width * spp * bps) / 8;
    trailing_bits = (crop_width * bps) % 8;

    /* These should not be needed for composite images */
    crop->regionlist[i].width = crop_width;
    crop->regionlist[i].length = crop_length;
    crop->regionlist[i].buffptr = crop_buff;

    switch (crop->edge_ref)
      {
      case EDGE_TOP:
      case EDGE_BOTTOM:
	   if ((i > 0) && (crop_width != crop->regionlist[i - 1].width))
             {
	     TIFFError ("extractCompositeRegions", 
                          "Only equal width regions can be combined for -E top or bottom");
	     return (1);
             }
     
           crop->combined_width = crop_width;
           crop->combined_length += crop_length;

           if ((bps % 8) == 0)
             {
             col_offset = first_col * spp * bps / 8;
             for (row = first_row; row <= last_row; row++)
               {
	       row_offset = row * rowsize;
               src_ptr = read_buff + row_offset + col_offset;
               _TIFFmemcpy (crop_buff + dst_offset, src_ptr, full_bytes);
               dst_offset += full_bytes;
	       }        
             }
           else
             { /* bps != 8 */
             shift1  = spp * ((first_col * bps) % 8);
             shift2  = spp * ((last_col * bps) % 8);
             for (row = first_row; row <= last_row; row++)
               {
               /* pull out the first byte */
	       row_offset = row * rowsize;
	       offset1 = row_offset + (first_col * bps / 8);
               offset2 = offset1 + full_bytes;

               bytebuff1 = bytebuff2 = 0;
               if (shift1 == 0) /* the region is byte and sample alligned */
	         _TIFFmemcpy (crop_buff + dst_offset, read_buff + offset1, full_bytes);
               else   /* each destination byte will have to be built from two source bytes*/
                 {
	         for (j = 0; j < full_bytes; j++)
                   {
	           bytebuff1 = read_buff[offset1 + j] & ((unsigned char)255 >> shift1);
	           bytebuff2 = read_buff[offset1 + j + 1] & ((unsigned char)255 << (7 - shift1));
                   crop_buff[dst_offset + j] = (bytebuff1 << shift1) | (bytebuff2 >> (8 - shift1));
                   }
                 }
               dst_offset += full_bytes;
               if (trailing_bits != 0)
                 {
	         if (shift2 > shift1)
                   {
	           bytebuff1 = read_buff[offset2] & ((unsigned char)255 << (7 - shift2));
                   bytebuff2 = (bytebuff1 << shift1) & ((unsigned char)255 << shift1);
                   crop_buff[dst_offset] |= bytebuff2;
	           }
                 else
                   {
	           if (shift2 <= shift1)
                     {
	             bytebuff1 = read_buff[offset2] & ((unsigned char)255 >> shift1);
	             bytebuff2 = read_buff[offset2 + 1] & ((unsigned char)255 << (7 - shift1));
                     crop_buff[dst_offset] = (bytebuff1 << shift1) | (bytebuff2 >> (8 - shift1));
                     }
                   }
	         dst_offset++;
	         }
               }
             }
	   break;
      case EDGE_LEFT:  /* splice the pieces of each row together, side by side */
      case EDGE_RIGHT:
	   if ((i > 0) && (crop_length != crop->regionlist[i - 1].length))
             {
	     TIFFError ("extractCompositeRegions", 
                          "Only equal length regions can be combined for -E left or right");
	     return (1);
             }
     
           dst_offset = prev_width;
           crop->combined_width += crop_width;
           crop->combined_length = crop_length;

           if ((bps % 8) == 0)
             {
             col_offset = first_col * spp * bps / 8;
             for (row = first_row; row <= last_row; row++)
               {
	       row_offset = row * rowsize;
               src_ptr = read_buff + row_offset + col_offset;
               _TIFFmemcpy (crop_buff + dst_offset, src_ptr, full_bytes);
               dst_offset += (composite_width * spp * bps / 8);
	       }
             prev_width += ((spp * bps * crop_width) / 8); 
             }
           else
             /* bps % 8 != 0 
              * This will not work for spp != 1 because of casts to unsigned char
              * and shifts limited to seven bits rather than sizeof(data) - 1.
              */
	     {
             shift1  = spp * ((first_col * bps) % 8);
             shift2  = spp * ((last_col * bps) % 8);
             if (prev_trailing_bits > 7)
	       {
	       prev_trailing_bits-= 8;
               dst_offset++;
               }
	     shift3  = spp * (8 - prev_trailing_bits);

             for (row = first_row; row <= last_row; row++)
               {
	       row_offset = row * rowsize;
	       offset1 = row_offset + (first_col * bps / 8);
               offset2 = offset1 + full_bytes;

               src_byte = bytebuff1 = bytebuff2 = 0;
               if ((shift1 == 0) && ((prev_trailing_bits % 8) == 0)) /* Regions are byte and sample alligned */
	         _TIFFmemcpy (crop_buff + dst_offset, read_buff + offset1, full_bytes);
               else   /* each destination byte will have to be built from two source bytes*/
                 {
 	         for (j = 0; j < full_bytes; j++)
                   {  /* Extract the source byte */
                   if (shift1 == 0) 
                     src_byte = read_buff[offset1 + j];
	           else 
                     {  /* The source region is not byte aligned */
	             bytebuff1 = read_buff[offset1 + j] & ((unsigned char)255 >> shift1);
	             bytebuff2 = read_buff[offset1 + j + 1] & ((unsigned char)255 << (7 - shift1));
                     src_byte  = (bytebuff1 << shift1) | (bytebuff2 >> (8 - shift1)); 
		   		 
                     /* Compute the destination byte(s) */
		     if ((prev_trailing_bits % 8) == 0) 
                       crop_buff[dst_offset + j] |= src_byte;
                     else
                       { /* The destination is not byte aligned */
		       bytebuff1 = src_byte >> prev_trailing_bits;
		       bytebuff2 = src_byte << shift3;
                       crop_buff[dst_offset + j]     |= bytebuff1;
                       crop_buff[dst_offset + j + 1] |= bytebuff2;
		       }
		     }
		   } 
	         }
	       /* Handle any trailing bits in the src line */
               if (trailing_bits != 0)
                 {
	         if (shift2 > shift1)
                   {
	           bytebuff1 = read_buff[offset2] & ((unsigned char)255 << (7 - shift2));
                   src_byte = (bytebuff1 << shift1) & ((unsigned char)255 << shift1);
	           }
                 else
                   {
	           if (shift2 <= shift1)
                     {
	             bytebuff1 = read_buff[offset2] & ((unsigned char)255 >> shift1);
	             bytebuff2 = read_buff[offset2 + 1] & ((unsigned char)255 << (7 - shift1));
                     src_byte = ((bytebuff1 << shift1) | (bytebuff2 >> (8 - shift1)));
                     }
                   }

	         if ((prev_trailing_bits % 8) == 0) /* The destination is byte aligned */
                   crop_buff[dst_offset + full_bytes] = src_byte;
                 else
                   {
		   bytebuff1 = src_byte >> prev_trailing_bits;
                   crop_buff[dst_offset + full_bytes] |= bytebuff1;

                   if (trailing_bits > shift3)
                     {
  		     bytebuff2 = src_byte << shift3;
                     crop_buff[dst_offset + full_bytes + 1] = bytebuff2;
		     }
		   } 
                 }
               dst_offset += (composite_width + 7 )/ 8;
               /* We are building combined width above so we can't use it here.
                *  dst_offset += (crop->combined_width + 7 )/ 8;
	       */
	       }
             prev_width += ((spp * crop_width) / 8); 

             prev_trailing_bits += trailing_bits;
	     }
	   break;
      }
    }
      
  return (0);
  }  /* end extractCompositeRegions */

/* Copy a single region of input buffer to an output buffer.
 * N.B. The read functions used copy separate plane data into a buffer as interleaved
 * samples rather than separate planes so the same logic works to extract regions
 * regardless of the way the data are organized in the input file.
 */

static int
extractSeparateRegion(struct image_data *image,  struct crop_mask *crop,
                      unsigned char *read_buff, unsigned char *crop_buff, int region)
  {
  uint32    j, shift1, shift2, trailing_bits;
  uint32    row, first_row, last_row, first_col, last_col;
  uint32    src_offset, dst_offset, row_offset, col_offset;
  uint32    offset1, offset2, full_bytes, rowsize;
  uint32    crop_width, crop_length, img_width, img_length;
  uint16    bps, spp;
  unsigned  char  bytebuff1, bytebuff2;
  unsigned  char *src_ptr, *dst_ptr;

  img_width = image->width;
  img_length = image->length;
  bps = image->bps;
  spp = image->spp;

  src_ptr = read_buff;
  src_offset = 0;

  dst_ptr = crop_buff;
  dst_offset = 0;

  /* rows, columns, width, length are expressed in pixels */
  first_row = crop->regionlist[region].y1;
  last_row  = crop->regionlist[region].y2;
  first_col = crop->regionlist[region].x1;
  last_col  = crop->regionlist[region].x2;

  crop_width = last_col - first_col + 1;
  crop_length = last_row - first_row + 1;

  crop->regionlist[region].width = crop_width;
  crop->regionlist[region].length = crop_length;
  crop->regionlist[region].buffptr = crop_buff;

  /* number of COMPLETE bytes per row in crop area */
  full_bytes = (crop_width * spp * bps) / 8;
  trailing_bits = (crop_width * bps) % 8;
  rowsize = spp * ((img_width * bps + 7) / 8);

  if ((bps % 8) == 0)
    {
    col_offset = first_col * spp * bps / 8;
    for (row = first_row; row <= last_row; row++)
      {
      row_offset = row * rowsize;
      src_ptr = read_buff + row_offset + col_offset;
      _TIFFmemcpy (crop_buff + dst_offset, src_ptr, full_bytes);
      dst_offset += full_bytes;
      }        
    }
  else
    { /* bps != 8 */
    shift1  = spp * ((first_col * bps) % 8);
    shift2  = spp * ((last_col * bps) % 8);
    for (row = first_row; row <= last_row; row++)
      {
      /* pull out the first byte */
      row_offset = row * rowsize;
      offset1 = row_offset + (first_col * bps / 8);
      offset2 = offset1 + full_bytes;

      bytebuff1 = bytebuff2 = 0;
      if (shift1 == 0) /* the region is byte and sample alligned */
	_TIFFmemcpy (crop_buff + dst_offset, read_buff + offset1, full_bytes);
      else   /* each destination byte will have to be built from two source bytes*/
        {
	for (j = 0; j < full_bytes; j++)
          {
	  bytebuff1 = read_buff[offset1 + j] & ((unsigned char)255 >> shift1);
	  bytebuff2 = read_buff[offset1 + j + 1] & ((unsigned char)255 << (7 - shift1));
          crop_buff[dst_offset + j] = (bytebuff1 << shift1) | (bytebuff2 >> (8 - shift1));
          }
        }
      dst_offset += full_bytes;
      if (trailing_bits != 0)
        {
	if (shift2 > shift1)
          {
	  bytebuff1 = read_buff[offset2] & ((unsigned char)255 << (7 - shift2));
          bytebuff2 = (bytebuff1 << shift1) & ((unsigned char)255 << shift1);
          crop_buff[dst_offset] |= bytebuff2;
	  }
        else
          {
	  if (shift2 <= shift1)
            {
	    bytebuff1 = read_buff[offset2] & ((unsigned char)255 >> shift1);
	    bytebuff2 = read_buff[offset2 + 1] & ((unsigned char)255 << (7 - shift1));
            crop_buff[dst_offset] = (bytebuff1 << shift1) | (bytebuff2 >> (8 - shift1));
            }
          }
	dst_offset++;
	}
      }
    }
          
  return (0);
  }  /* end extractSeparateRegion */

static int
extractImageSection(struct image_data *image, struct pageseg *section, 
                    unsigned char *src_buff, unsigned char *sect_buff)
  {
  unsigned  char  bytebuff1, bytebuff2;
  unsigned  char *src_ptr, *dst_ptr;

  uint32    img_width, img_length, img_rowsize;
  uint32    j, shift1, shift2, trailing_bits;
  uint32    row, first_row, last_row, first_col, last_col;
  uint32    src_offset, dst_offset, row_offset, col_offset;
  uint32    offset1, offset2, full_bytes;
  uint32    sect_width, sect_length;
  uint16    bps, spp;

#ifdef DEBUG2
  int      k;
  unsigned char bitset;
  static char *bitarray = NULL;
#endif

  img_width = image->width;
  img_length = image->length;
  bps = image->bps;
  spp = image->spp;

  src_ptr = src_buff;
  dst_ptr = sect_buff;
  src_offset = 0;
  dst_offset = 0;

#ifdef DEBUG2
  if (bitarray == NULL)
    {
    if ((bitarray = (char *)malloc(img_width)) == NULL)
      {
      fprintf (stderr, "DEBUG: Unable to allocate debugging bitarray\n");
      return (-1);
      }
    }
#endif

  /* rows, columns, width, length are expressed in pixels */
  first_row = section->y1;
  last_row  = section->y2;
  first_col = section->x1;
  last_col  = section->x2;

  sect_width = last_col - first_col + 1;
  sect_length = last_row - first_row + 1;
  img_rowsize = ((img_width * bps + 7) / 8) * spp;
  full_bytes = (sect_width * spp * bps) / 8;   /* number of COMPLETE bytes per row in section */
  trailing_bits = (sect_width * bps) % 8;

#ifdef DEBUG2
  fprintf (stderr, "First row: %d, last row: %d, First col: %d, last col: %d\n",
           first_row, last_row, first_col, last_col);
  fprintf (stderr, "Image width: %d, Image length: %d, bps: %d, spp: %d\n",
	   img_width, img_length, bps, spp);
  fprintf (stderr, "Sect  width: %d,  Sect length: %d, full bytes: %d trailing bits %d\n", 
           sect_width, sect_length, full_bytes, trailing_bits);
#endif

  if ((bps % 8) == 0)
    {
    col_offset = first_col * spp * bps / 8;
    for (row = first_row; row <= last_row; row++)
      {
      /* row_offset = row * img_width * spp * bps / 8; */
      row_offset = row * img_rowsize;
      src_offset = row_offset + col_offset;
#ifdef DEBUG2
      fprintf (stderr, "Src offset: %8d, Dst offset: %8d\n", src_offset, dst_offset); 
#endif
      _TIFFmemcpy (sect_buff + dst_offset, src_buff + src_offset, full_bytes);
      dst_offset += full_bytes;
      }        
    }
  else
    { /* bps != 8 */
    shift1  = spp * ((first_col * bps) % 8);
    shift2  = spp * ((last_col * bps) % 8);
    for (row = first_row; row <= last_row; row++)
      {
      /* pull out the first byte */
      row_offset = row * img_rowsize;
      offset1 = row_offset + (first_col * bps / 8);
      offset2 = row_offset + (last_col * bps / 8);

#ifdef DEBUG2
      for (j = 0, k = 7; j < 8; j++, k--)
        {
        bitset = *(src_buff + offset1) & (((unsigned char)1 << k)) ? 1 : 0;
        sprintf(&bitarray[j], (bitset) ? "1" : "0");
        }
      sprintf(&bitarray[8], " ");
      sprintf(&bitarray[9], " ");
      for (j = 10, k = 7; j < 18; j++, k--)
        {
        bitset = *(src_buff + offset2) & (((unsigned char)1 << k)) ? 1 : 0;
        sprintf(&bitarray[j], (bitset) ? "1" : "0");
        }
      bitarray[18] = '\0';
      fprintf (stderr, "Row: %3d Offset1: %d,  Shift1: %d,    Offset2: %d,  Shift2:  %d\n", 
               row, offset1, shift1, offset2, shift2); 
#endif
      bytebuff1 = bytebuff2 = 0;
      if (shift1 == 0) /* the region is byte and sample alligned */
        {
	_TIFFmemcpy (sect_buff + dst_offset, src_buff + offset1, full_bytes);
#ifdef DEBUG2
	fprintf (stderr, "        Alligned data src offset1: %8d, Dst offset: %8d\n", offset1, dst_offset); 
	sprintf(&bitarray[18], "\n");
	sprintf(&bitarray[19], "\t");
        for (j = 20, k = 7; j < 28; j++, k--)
          {
          bitset = *(sect_buff + dst_offset) & (((unsigned char)1 << k)) ? 1 : 0;
          sprintf(&bitarray[j], (bitset) ? "1" : "0");
          }
        bitarray[28] = ' ';
        bitarray[29] = ' ';
#endif
        dst_offset += full_bytes;

        if (trailing_bits != 0)
          {
	  bytebuff2 = src_buff[offset2] & ((unsigned char)255 << (7 - shift2));
          sect_buff[dst_offset] = bytebuff2;
#ifdef DEBUG2
	  fprintf (stderr, "        Trailing bits src offset:  %8d, Dst offset: %8d\n", 
                              offset2, dst_offset); 
          for (j = 30, k = 7; j < 38; j++, k--)
            {
            bitset = *(sect_buff + dst_offset) & (((unsigned char)1 << k)) ? 1 : 0;
            sprintf(&bitarray[j], (bitset) ? "1" : "0");
            }
          bitarray[38] = '\0';
          fprintf (stderr, "\tFirst and last bytes before and after masking:\n\t%s\n\n", bitarray);
#endif
          dst_offset++;
          }
        }
      else   /* each destination byte will have to be built from two source bytes*/
        {
#ifdef DEBUG2
	  fprintf (stderr, "        Unalligned data src offset: %8d, Dst offset: %8d\n", offset1 , dst_offset); 
#endif
        for (j = 0; j <= full_bytes; j++) 
          {
	  bytebuff1 = src_buff[offset1 + j] & ((unsigned char)255 >> shift1);
	  bytebuff2 = src_buff[offset1 + j + 1] & ((unsigned char)255 << (7 - shift1));
          sect_buff[dst_offset + j] = (bytebuff1 << shift1) | (bytebuff2 >> (8 - shift1));
          }
#ifdef DEBUG2
	sprintf(&bitarray[18], "\n");
	sprintf(&bitarray[19], "\t");
        for (j = 20, k = 7; j < 28; j++, k--)
          {
          bitset = *(sect_buff + dst_offset) & (((unsigned char)1 << k)) ? 1 : 0;
          sprintf(&bitarray[j], (bitset) ? "1" : "0");
          }
        bitarray[28] = ' ';
        bitarray[29] = ' ';
#endif
        dst_offset += full_bytes;

        if (trailing_bits != 0)
          {
#ifdef DEBUG2
	    fprintf (stderr, "        Trailing bits   src offset: %8d, Dst offset: %8d\n", offset1 + full_bytes, dst_offset); 
#endif
	  if (shift2 > shift1)
            {
	    bytebuff1 = src_buff[offset1 + full_bytes] & ((unsigned char)255 << (7 - shift2));
            bytebuff2 = bytebuff1 & ((unsigned char)255 << shift1);
            sect_buff[dst_offset] = bytebuff2;
#ifdef DEBUG2
	    fprintf (stderr, "        Shift2 > Shift1\n"); 
#endif
            }
          else
            {
	    if (shift2 < shift1)
              {
              bytebuff2 = ((unsigned char)255 << (shift1 - shift2 - 1));
	      sect_buff[dst_offset] &= bytebuff2;
#ifdef DEBUG2
	      fprintf (stderr, "        Shift2 < Shift1\n"); 
#endif
              }
#ifdef DEBUG2
            else
	      fprintf (stderr, "        Shift2 == Shift1\n"); 
#endif
            }
	  }
#ifdef DEBUG2
	  sprintf(&bitarray[28], " ");
	  sprintf(&bitarray[29], " ");
          for (j = 30, k = 7; j < 38; j++, k--)
            {
            bitset = *(sect_buff + dst_offset) & (((unsigned char)1 << k)) ? 1 : 0;
            sprintf(&bitarray[j], (bitset) ? "1" : "0");
            }
          bitarray[38] = '\0';
          fprintf (stderr, "\tFirst and last bytes before and after masking:\n\t%s\n\n", bitarray);
#endif
        dst_offset++;
        }
      }
    }

  return (0);
  } /* end extractImageSection */

static int 
writeSelections(TIFF *in, TIFF **out, struct crop_mask *crop, 
                struct buffinfo seg_buffs[], char *mp, char *filename, 
                unsigned int *page, unsigned int total_pages)
  {
  int i, page_count;
  int autoindex = 0;
  unsigned char *crop_buff = NULL;

  /* Where we open a new file depends on the export mode */  
  switch (crop->exp_mode)
    {
    case ONE_FILE_COMPOSITE: /* Regions combined into single image */
         autoindex = 0;
         crop_buff = seg_buffs[0].buffer;
         if (update_output_file (out, mp, autoindex, filename, page))
           return (1);
         page_count = total_pages;
         if (writeCroppedImage(in, *out, 
                               crop->combined_width, 
                               crop->combined_length, 
                               crop_buff, *page, total_pages))
            {
             TIFFError("writeRegions", "Unable to write new image");
             return (-1);
             }
	 break;
    case ONE_FILE_SEPARATED: /* Regions as separated images */
         autoindex = 0;
         if (update_output_file (out, mp, autoindex, filename, page))
           return (1);
         page_count = crop->selections * total_pages;
         for (i = 0; i < crop->selections; i++)
           {
           crop_buff = seg_buffs[i].buffer;
           if (writeCroppedImage(in, *out, 
                                 crop->regionlist[i].width, 
                                 crop->regionlist[i].length, 
                                 crop_buff, *page, page_count))
             {
             TIFFError("writeRegions", "Unable to write new image");
             return (-1);
             }
	   }
         break;
    case FILE_PER_IMAGE_COMPOSITE: /* Regions as composite image */
         autoindex = 1;
         if (update_output_file (out, mp, autoindex, filename, page))
           return (1);

         crop_buff = seg_buffs[0].buffer;
         if (writeCroppedImage(in, *out, 
                               crop->combined_width, 
                               crop->combined_length, 
                               crop_buff, *page, total_pages))
           {
           TIFFError("writeRegions", "Unable to write new image");
           return (-1);
           }
         break;
    case FILE_PER_IMAGE_SEPARATED: /* Regions as separated images */
         autoindex = 1;
         page_count = crop->selections;
         if (update_output_file (out, mp, autoindex, filename, page))
           return (1);
                
         for (i = 0; i < crop->selections; i++)
           {
           crop_buff = seg_buffs[i].buffer;
           /* Write the current region to the current file */
           if (writeCroppedImage(in, *out,
                                 crop->regionlist[i].width, 
                                 crop->regionlist[i].length, 
                                 crop_buff, *page, page_count))
             {
             TIFFError("writeRegions", "Unable to write new image");
             return (-1);
             }
           }
         break;
    case FILE_PER_SELECTION:
         autoindex = 1;
	 page_count = 1;
         for (i = 0; i < crop->selections; i++)
           {
           if (update_output_file (out, mp, autoindex, filename, page))
             return (1);

           crop_buff = seg_buffs[i].buffer;
           /* Write the current region to the current file */
           if (writeCroppedImage(in, *out,
                                 crop->regionlist[i].width, 
                                 crop->regionlist[i].length, 
                                 crop_buff, *page, page_count))
             {
             TIFFError("writeRegions", "Unable to write new image");
             return (-1);
             }
           }
	 break;
    default: return (1);
    }

  return (0);
  } /* end writeRegions */

static int
writeImageSections(TIFF *in, TIFF *out, struct image_data *image,
		   struct pagedef *page, struct pageseg *sections,
		   unsigned char *src_buff, unsigned char **sect_buff_ptr)
  {
  double  hres, vres;
  uint32  i, k, width, length, sectsize;
  unsigned char *sect_buff = *sect_buff_ptr;

  hres = page->hres;
  vres = page->vres;

#ifdef DEBUG
  fprintf(stderr,
    "Writing %d sections for each original page. Hres: %3.2f Vres: %3.2f\n", 
          page->rows * page->cols, hres, vres);
#endif
  k = page->cols * page->rows;
  if ((k < 1) || (k > MAX_SECTIONS))
   {
   TIFFError("writeImageSections",
	     "%d Rows and Columns exceed maximum sections\nIncrease resolution or reduce sections", k);
   return (-1);
   }

  for (i = 0; i < k; i++)
    {
    width  = sections[i].x2 - sections[i].x1 + 1;
    length = sections[i].y2 - sections[i].y1 + 1;
    sectsize = (uint32)
	    ceil((width * image->bps + 7) / (double)8) * image->spp * length;
    /* allocate a buffer if we don't have one already */
    if (createImageSection(sectsize, sect_buff_ptr))
      {
      TIFFError("writeImageSections", "Unable to allocate section buffer");
      exit (-1);
      }
    sect_buff = *sect_buff_ptr;

#ifdef DEBUG
    fprintf (stderr, "\nSection: %d, Width: %4d, Length: %4d, x1: %4d  x2: %4d  y1: %4d  y2: %4d\n",
             i + 1, width, length, sections[i].x1, sections[i].x2, sections[i].y1, sections[i].y2);
#endif

    if (extractImageSection (image, &sections[i], src_buff, sect_buff))
      {
      TIFFError("writeImageSections", "Unable to extract image sections");
      exit (-1);
      }

  /* call the write routine here instead of outside the loop */
    if (writeSingleSection(in, out, width, length, hres, vres, sect_buff))
      {
      TIFFError("writeImageSections", "Unable to write image section");
      exit (-1);
      }
    }

  return (0);
  } /* end writeImageSections */

static int  
writeSingleSection(TIFF *in, TIFF *out, uint32 width, uint32 length,
		   double hres, double vres, unsigned char *sect_buff)
  {
  uint16 bps, spp;
  struct cpTag* p;

#ifdef DEBUG
  fprintf (stderr,
"\nWriting single section: Width %d Length: %d Hres: %4.1f, Vres: %4.1f\n\n",
	   width, length, hres, vres);
#endif
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
	  if (rowsperstrip > length && rowsperstrip != (uint32)-1)
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

  /* Update these since they are overwritten from input res by loop above */
  TIFFSetField(out, TIFFTAG_XRESOLUTION, (float)hres);
  TIFFSetField(out, TIFFTAG_YRESOLUTION, (float)vres);

  /* Compute the tile or strip dimensions and write to disk */
  if (outtiled)
    {
    if (config == PLANARCONFIG_CONTIG)
      {
      writeBufferToContigTiles (out, sect_buff, length, width, spp);
      }
    else
      writeBufferToSeparateTiles (out, sect_buff, length, width, spp);
    }
  else
    {
    if (config == PLANARCONFIG_CONTIG)
      {
      writeBufferToContigStrips (out, sect_buff, length, width, spp);
      }
    else
      {
      writeBufferToSeparateStrips(out, sect_buff, length, width, spp);
      }
    }

  if (!TIFFWriteDirectory(out))
    {
    TIFFClose(out);
    return (-1);
    }

  return (0);
  } /* end writeSingleSection */


/* Create a buffer to write one section at a time */
static int
createImageSection(uint32 sectsize, unsigned char **sect_buff_ptr)
  {
  unsigned  char *sect_buff = NULL;
  unsigned  char *new_buff  = NULL;
  static    uint32  prev_sectsize = 0;
  
  sect_buff = *sect_buff_ptr;

  if (!sect_buff)
    {
    sect_buff = (unsigned char *)_TIFFmalloc(sectsize);
    *sect_buff_ptr = sect_buff;
    _TIFFmemset(sect_buff, 0, sectsize);
    }
  else
    {
    if (prev_sectsize < sectsize)
      {
      new_buff = _TIFFrealloc(sect_buff, sectsize);
      if (!new_buff)
        {
	free (sect_buff);
        sect_buff = (unsigned char *)_TIFFmalloc(sectsize);
        }
      else
        sect_buff = new_buff;

      _TIFFmemset(sect_buff, 0, sectsize);
      }
    }

  if (!sect_buff)
    {
    TIFFError("createImageSection", "Unable to allocate/reallocate section buffer");
    return (-1);
    }
  prev_sectsize = sectsize;
  *sect_buff_ptr = sect_buff;

  return (0);
  }  /* end createImageSection */


/* Process selections defined by regions, zones, margins, or fixed sized areas */
static int
processCropSelections(struct image_data *image, struct crop_mask *crop, 
                      unsigned char **read_buff_ptr, struct buffinfo seg_buffs[])
  {
  int       i;
  uint32    width, length, total_width, total_length;
  tsize_t   cropsize;
  unsigned  char *crop_buff = NULL;
  unsigned  char *read_buff = NULL;
  unsigned  char *next_buff = NULL;
  tsize_t   prev_cropsize = 0;

  read_buff = *read_buff_ptr;

  if (crop->img_mode == COMPOSITE_IMAGES)
    {
    cropsize = crop->bufftotal;
    crop_buff = seg_buffs[0].buffer; 
    if (!crop_buff)
      crop_buff = (unsigned char *)_TIFFmalloc(cropsize);
    else
      {
      prev_cropsize = seg_buffs[0].size;
      if (prev_cropsize < cropsize)
        {
        next_buff = _TIFFrealloc(crop_buff, cropsize);
        if (! next_buff)
          {
          _TIFFfree (crop_buff);
          crop_buff = (unsigned char *)_TIFFmalloc(cropsize);
          }
        else
          crop_buff = next_buff;
        }
      }

    if (!crop_buff)
      {
      TIFFError("processCropSelections", "Unable to allocate/reallocate crop buffer");
      return (-1);
      }
 
    _TIFFmemset(crop_buff, 0, cropsize);
    seg_buffs[0].buffer = crop_buff;
    seg_buffs[0].size = cropsize;

    /* Checks for matching width or length as required */
    if (extractCompositeRegions(image, crop, read_buff, crop_buff) != 0)
      return (1);

    if (crop->crop_mode & CROP_INVERT)
      {
      switch (crop->photometric)
        {
        /* Just change the interpretation */
        case PHOTOMETRIC_MINISWHITE:
        case PHOTOMETRIC_MINISBLACK:
	     image->photometric = crop->photometric;
	     break;
        case INVERT_DATA_ONLY:
        case INVERT_DATA_AND_TAG:
             if (invertImage(image->photometric, image->spp, image->bps, 
                             crop->combined_width, crop->combined_length, crop_buff))
               {
               TIFFError("processCropRegions", 
                         "Failed to invert colorspace for composite regions");
               return (-1);
               }
             if (crop->photometric == INVERT_DATA_AND_TAG)
               {
               switch (image->photometric)
                 {
                 case PHOTOMETRIC_MINISWHITE:
 	              image->photometric = PHOTOMETRIC_MINISBLACK;
	              break;
                 case PHOTOMETRIC_MINISBLACK:
 	              image->photometric = PHOTOMETRIC_MINISWHITE;
	              break;
                 default:
	              break;
	         }
	       }
             break;
        default: break;
        }
      }

    /* Mirror and Rotate will not work with multiple regions unless they are the same width */
    if (crop->crop_mode & CROP_MIRROR)
      {
      if (mirrorImage(image->spp, image->bps, crop->mirror, 
                      crop->combined_width, crop->combined_length, crop_buff))
        {
        TIFFError("processCropRegions", "Failed to mirror composite regions %s", 
	         (crop->rotation == MIRROR_HORIZ) ? "horizontally" : "vertically");
        return (-1);
        }
      }

    if (crop->crop_mode & CROP_ROTATE) /* rotate should be last as it can reallocate the buffer */
      {
      if (rotateImage(crop->rotation, image, &crop->combined_width, 
                      &crop->combined_length, &crop_buff))
        {
        TIFFError("processCropRegions", 
                  "Failed to rotate composite regions by %d degrees", crop->rotation);
        return (-1);
        }
      seg_buffs[0].buffer = crop_buff;
      seg_buffs[0].size = (((crop->combined_width * image->bps + 7 ) / 8)
                            * image->spp) * crop->combined_length; 
      }
    }
  else  /* Separated Images */
    {
    total_width = total_length = 0;
    for (i = 0; i < crop->selections; i++)
      {
      cropsize = crop->bufftotal;
      crop_buff = seg_buffs[i].buffer; 
      if (!crop_buff)
        crop_buff = (unsigned char *)_TIFFmalloc(cropsize);
      else
        {
        prev_cropsize = seg_buffs[0].size;
        if (prev_cropsize < cropsize)
          {
          next_buff = _TIFFrealloc(crop_buff, cropsize);
          if (! next_buff)
            {
            _TIFFfree (crop_buff);
            crop_buff = (unsigned char *)_TIFFmalloc(cropsize);
            }
          else
            crop_buff = next_buff;
          }
        }

      if (!crop_buff)
        {
        TIFFError("processCropSelections", "Unable to allocate/reallocate crop buffer");
        return (-1);
        }
 
      _TIFFmemset(crop_buff, 0, cropsize);
      seg_buffs[i].buffer = crop_buff;
      seg_buffs[i].size = cropsize;

      if (extractSeparateRegion(image, crop, read_buff, crop_buff, i))
        {
	TIFFError("processCropRegions", "Unable to extract cropped region %d from image", i);
        return (-1);
        }
    
      width  = crop->regionlist[i].width;
      length = crop->regionlist[i].length;

      if (crop->crop_mode & CROP_INVERT)
        {
        switch (crop->photometric)
          {
          /* Just change the interpretation */
          case PHOTOMETRIC_MINISWHITE:
          case PHOTOMETRIC_MINISBLACK:
	       image->photometric = crop->photometric;
	       break;
          case INVERT_DATA_ONLY:
          case INVERT_DATA_AND_TAG:
               if (invertImage(image->photometric, image->spp, image->bps, 
                               width, length, crop_buff))
                 {
                 TIFFError("processCropRegions", 
                           "Failed to invert colorspace for region");
                 return (-1);
                 }
               if (crop->photometric == INVERT_DATA_AND_TAG)
                 {
                 switch (image->photometric)
                   {
                   case PHOTOMETRIC_MINISWHITE:
 	                image->photometric = PHOTOMETRIC_MINISBLACK;
	                break;
                   case PHOTOMETRIC_MINISBLACK:
 	                image->photometric = PHOTOMETRIC_MINISWHITE;
	                break;
                   default:
	                break;
	           }
	         }
               break;
          default: break;
          }
        }

      if (crop->crop_mode & CROP_MIRROR)
        {
        if (mirrorImage(image->spp, image->bps, crop->mirror, 
                        width, length, crop_buff))
          {
          TIFFError("processCropRegions", "Failed to mirror crop region %s", 
	           (crop->rotation == MIRROR_HORIZ) ? "horizontally" : "vertically");
          return (-1);
          }
        }

      if (crop->crop_mode & CROP_ROTATE) /* rotate should be last as it can reallocate the buffer */
        {
	if (rotateImage(crop->rotation, image, &crop->regionlist[i].width, 
			&crop->regionlist[i].length, &crop_buff))
          {
          TIFFError("processCropRegions", 
                    "Failed to rotate crop region by %d degrees", crop->rotation);
          return (-1);
          }
        total_width  += crop->regionlist[i].width;
        total_length += crop->regionlist[i].length;
        crop->combined_width = total_width;
        crop->combined_length = total_length;
        seg_buffs[i].buffer = crop_buff;
        seg_buffs[i].size = (((crop->regionlist[i].width * image->bps + 7 ) / 8)
                               * image->spp) * crop->regionlist[i].length; 
        }
      }
    }
  return (0);
  } /* end processCropSelections */


/* Copy the crop section of the data from the current image into a buffer
 * and adjust the IFD values to reflect the new size. If no cropping is
 * required, use the origial read buffer as the crop buffer.
 *
 * There is quite a bit of redundancy between this routine and the more
 * specialized processCropZones and processCropRegions, but this provides
 * the most optimized path when no Zones or Regions are required.
 */
static int
createCroppedImage(struct image_data *image, struct crop_mask *crop, 
                   unsigned char **read_buff_ptr, unsigned char **crop_buff_ptr)
  {
  tsize_t   cropsize;
  unsigned  char *read_buff = NULL;
  unsigned  char *crop_buff = NULL;
  unsigned  char *new_buff  = NULL;
  static    tsize_t  prev_cropsize = 0;

  read_buff = *read_buff_ptr;

  /* process full image, no crop buffer needed */
  crop_buff = read_buff;
  *crop_buff_ptr = read_buff;
  crop->combined_width = image->width;
  crop->combined_length = image->length;

  cropsize = crop->bufftotal;
  crop_buff = *crop_buff_ptr;
  if (!crop_buff)
    {
    crop_buff = (unsigned char *)_TIFFmalloc(cropsize);
    *crop_buff_ptr = crop_buff;
    _TIFFmemset(crop_buff, 0, cropsize);
    prev_cropsize = cropsize;
    }
  else
    {
    if (prev_cropsize < cropsize)
      {
      new_buff = _TIFFrealloc(crop_buff, cropsize);
      if (!new_buff)
        {
	free (crop_buff);
        crop_buff = (unsigned char *)_TIFFmalloc(cropsize);
        }
      else
        crop_buff = new_buff;
      _TIFFmemset(crop_buff, 0, cropsize);
      }
    }

  if (!crop_buff)
    {
    TIFFError("createCroppedImage", "Unable to allocate/reallocate crop buffer");
    return (-1);
    }
  *crop_buff_ptr = crop_buff;

  if (crop->crop_mode & CROP_INVERT)
    {
    switch (crop->photometric)
      {
      /* Just change the interpretation */
      case PHOTOMETRIC_MINISWHITE:
      case PHOTOMETRIC_MINISBLACK:
	   image->photometric = crop->photometric;
	   break;
      case INVERT_DATA_ONLY:
      case INVERT_DATA_AND_TAG:
           if (invertImage(image->photometric, image->spp, image->bps, 
                           crop->combined_width, crop->combined_length, crop_buff))
             {
             TIFFError("createCroppedImage", 
                       "Failed to invert colorspace for image or cropped selection");
             return (-1);
             }
           if (crop->photometric == INVERT_DATA_AND_TAG)
             {
             switch (image->photometric)
               {
               case PHOTOMETRIC_MINISWHITE:
 	            image->photometric = PHOTOMETRIC_MINISBLACK;
	            break;
               case PHOTOMETRIC_MINISBLACK:
 	            image->photometric = PHOTOMETRIC_MINISWHITE;
	            break;
               default:
	            break;
	       }
	     }
           break;
      default: break;
      }
    }

  if (crop->crop_mode & CROP_MIRROR)
    {
    if (mirrorImage(image->spp, image->bps, crop->mirror, 
                    crop->combined_width, crop->combined_length, crop_buff))
      {
      TIFFError("createCroppedImage", "Failed to mirror image or cropped selection %s", 
	       (crop->rotation == MIRROR_HORIZ) ? "horizontally" : "vertically");
      return (-1);
      }
    }

  if (crop->crop_mode & CROP_ROTATE) /* rotate should be last as it can reallocate the buffer */
    {
    if (rotateImage(crop->rotation, image, &crop->combined_width, 
                    &crop->combined_length, crop_buff_ptr))
      {
      TIFFError("createCroppedImage", 
                "Failed to rotate image or cropped selection by %d degrees", crop->rotation);
      return (-1);
      }
    }

  if (crop_buff == read_buff) /* we used the read buffer for the crop buffer */
    *read_buff_ptr = NULL;    /* so we don't try to free it later */

  return (0);
  } /* end createCroppedImage */

static int  
writeCroppedImage(TIFF *in, TIFF *out, uint32 width, uint32 length, 
                  unsigned char *crop_buff, int pagenum, int total_pages)
  {
  uint16 bps, spp;
  struct cpTag* p;

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
     TIFFSetField(out, TIFFTAG_PAGENUMBER, pagenum, total_pages);
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
rotateImage(uint16 rotation, struct image_data *image, uint32 *img_width, 
            uint32 *img_length, unsigned char **crop_buff_ptr)
  {
  uint32   i, row, col, width, length, full_bytes, trailing_bits;
  uint32   rowsize, colsize, buffsize, row_offset, col_offset, pix_offset;
  unsigned char bitset, bytebuff1, bytebuff2, bytes_per_pixel;
  unsigned char *crop_buff = *crop_buff_ptr;
  unsigned char *src_ptr;
  unsigned char *dst_ptr;
  uint16   spp, bps;
  float    res_temp;
  int      j, k;
  static unsigned char *rotate_buff = NULL;

  width  = *img_width;
  length = *img_length;
  spp = image->spp;
  bps = image->bps;

  rowsize = (width * bps + 7) / 8;
  colsize = (length * bps + 7) / 8;
  bytes_per_pixel = (spp * bps + 7) / 8;
  full_bytes = width * spp * bps / 8;
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
  _TIFFmemset(rotate_buff, '\0', buffsize);

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
                trailing_bits = (width * spp * bps) % 8;
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
                for (row = 0; row < length; row++)
                  {
		  dst_ptr = rotate_buff + (spp * colsize) - ((row + 1) * bytes_per_pixel);
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
                trailing_bits = (length * spp * bps) % 8;
                for (row = 0; row < length; row++)
                  {
		  if ((length % 8) == 0)
		    dst_ptr = rotate_buff + colsize - (row / 8);
                  else
		    dst_ptr = rotate_buff + colsize - (row / 8) - 1;

                  for (col = 0; col < width; col+=  8 /(bps * spp))
                    {
                    for (i = 0, j = 7; i < 8; i++, j--)
                      {
		      if (col + i < width)
			{
			bitset = ((*src_ptr) & (((unsigned char)1 << j)) ? 1 : 0);
			k = (row % 8) + (8 - trailing_bits);
                       
                        if (k > 7)
                          {
    			  *(dst_ptr - 1) |= (bitset << ((row % 8) - trailing_bits));
                          }
                        else
                          {
			  *(dst_ptr) |= (bitset << ((row % 8) + (8 - trailing_bits)));
			  }
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
              image->width = length;
              image->length = width;
              res_temp = image->xres;
              image->xres = image->yres;
              image->yres = res_temp;
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
              image->width = length;
              image->length = width;
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
  uint32   rowsize, colsize, row_offset, col_offset, pix_offset;
  unsigned char  bytebuff1, bytebuff2, bytes_per_pixel, bitset;
  unsigned char *line_buff = NULL;
  unsigned char *src_ptr;
  unsigned char *dst_ptr;
  unsigned char workbuff[12];

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
		switch (bps / 8)
                  {
		  case 2:
                    for (row = 0; row < length; row++)
                      {
		      row_offset = row * rowsize * spp;
                      src_ptr = crop_buff + row_offset;
                      dst_ptr = crop_buff + row_offset + (spp * rowsize);
                      for (col = 0; col < (width / 2); col++)
                        {
			col_offset = col * bytes_per_pixel;                     
			_TIFFmemcpy (workbuff, src_ptr + col_offset, bytes_per_pixel);
			_TIFFmemcpy (src_ptr + col_offset, dst_ptr - col_offset - bytes_per_pixel, bytes_per_pixel);
			_TIFFmemcpy (dst_ptr - col_offset - bytes_per_pixel, workbuff, bytes_per_pixel);
                        }
                      }
		    break;

		  case 1:
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
		    break;
		  default:  
                     TIFFError("mirrorImage", "Unsupported bits per pixel");
                     return (-1);
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
                  _TIFFmemset (line_buff, '\0', rowsize);

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
		   _TIFFmemcpy (src_ptr, line_buff, spp * rowsize);
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
	      _TIFFmemcpy(line_buff, src_ptr, spp * rowsize);
	      _TIFFmemcpy(src_ptr, dst_ptr, spp * rowsize);
	      _TIFFmemcpy(dst_ptr, line_buff, spp * rowsize);
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
  uint16        *src_ptr_uint16;

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
    case 16: src_ptr_uint16 = (unsigned short *)src_ptr;
             for (row = 0; row < length; row++)
               for (col = 0; col < width; col++)
                 {
		 *src_ptr_uint16 = (unsigned short)0xFFFF - *src_ptr_uint16;
                  src_ptr_uint16++;
                 }
            break;
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
		bytebuff1 = 16 - (unsigned char)(*src_ptr & 240 >> 4);
		bytebuff2 = 16 - (*src_ptr & 15);
		*src_ptr = bytebuff1 << 4 & bytebuff2;
                src_ptr++;
                }
            break;
    case 2: for (row = 0; row < length; row++)
              for (col = 0; col < width; col++)
                {
		bytebuff1 = 4 - (unsigned char)(*src_ptr & 192 >> 6);
		bytebuff2 = 4 - (unsigned char)(*src_ptr & 48  >> 4);
		bytebuff3 = 4 - (unsigned char)(*src_ptr & 12  >> 2);
		bytebuff4 = 4 - (unsigned char)(*src_ptr & 3);
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

