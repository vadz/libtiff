$! $Id$
$!
$! Configure procedure for libtiff
$! (c) Alexey Chupahin  18-APR-2006
$!
$! Permission to use, copy, modify, distribute, and sell this software and 
$! its documentation for any purpose is hereby granted without fee, provided
$! that (i) the above copyright notices and this permission notice appear in
$! all copies of the software and related documentation, and (ii) the names of
$! Sam Leffler and Silicon Graphics may not be used in any advertising or
$! publicity relating to the software without the specific, prior written
$! permission of Sam Leffler and Silicon Graphics.
$! 
$! THE SOFTWARE IS PROVIDED "AS-IS" AND WITHOUT WARRANTY OF ANY KIND, 
$! EXPRESS, IMPLIED OR OTHERWISE, INCLUDING WITHOUT LIMITATION, ANY 
$! WARRANTY OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE.  
$! 
$! IN NO EVENT SHALL SAM LEFFLER OR SILICON GRAPHICS BE LIABLE FOR
$! ANY SPECIAL, INCIDENTAL, INDIRECT OR CONSEQUENTIAL DAMAGES OF ANY KIND,
$! OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
$! WHETHER OR NOT ADVISED OF THE POSSIBILITY OF DAMAGE, AND ON ANY THEORY OF 
$! LIABILITY, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE 
$! OF THIS SOFTWARE.
$!
$!
$ SET NOON
$WRITE SYS$OUTPUT " "
$WRITE SYS$OUTPUT "Configuring libTIFF library"
$WRITE SYS$OUTPUT " "
$! Checking architecture
$DECC = F$SEARCH("SYS$SYSTEM:DECC$COMPILER.EXE") .NES. ""
$ALPHA = F$GETSYI("HW_MODEL") .GE. 1024
$IF (ALPHA) THEN $WRITE SYS$OUTPUT "Checking architecture 	...  Alpha"
$IF (.NOT. ALPHA) THEN $WRITE SYS$OUTPUT "Checking architecture	...  VAX"
$IF (DECC) THEN $WRITE SYS$OUTPUT  "Compiler		...  DEC C"
$IF (.NOT. DECC) THEN $WRITE SYS$OUTPUT  "BAD compiler" GOTO EXIT
$MMS = F$SEARCH("SYS$SYSTEM:MMS.EXE") .NES. ""
$MMK = F$TYPE(MMK) 
$IF (MMS .OR. MMK) THEN GOTO TEST_LIBRARIES
$! I cant find any make tool
$GOTO EXIT
$!
$!
$TEST_LIBRARIES:
$!   Setting as MAKE utility one of MMS or MMK. I prefer MMS.
$IF (MMK) THEN MAKE="MMK"
$IF (MMS) THEN MAKE="MMS"
$WRITE SYS$OUTPUT "Checking build utility	...  ''MAKE'"
$WRITE SYS$OUTPUT " "
$!
$!
$!"Checking for strcasecmp "
$ DEFINE SYS$ERROR _NLA0:
$ DEFINE SYS$OUTPUT _NLA0:
$ CC/OBJECT=TEST.OBJ/INCLUDE=(ZLIB) SYS$INPUT
	#include  <strings.h>
	#include  <stdlib.h>

    int main()
	{
        if (strcasecmp("bla", "Bla")==0) exit(0);
	   else exit(2);
	}
$!
$TMP = $STATUS
$DEASS SYS$ERROR
$DEAS  SYS$OUTPUT
$IF (TMP .NE. %X10B90001)
$  THEN
$       HAVE_STRCASECMP=0
$       GOTO NEXT1
$ENDIF
$DEFINE SYS$ERROR _NLA0:
$DEFINE SYS$OUTPUT _NLA0:
$LINK/EXE=TEST TEST
$TMP = $STATUS
$DEAS  SYS$ERROR
$DEAS  SYS$OUTPUT
$!WRITE SYS$OUTPUT TMP
$IF (TMP .NE. %X10000001)
$  THEN
$       HAVE_STRCASECMP=0
$       GOTO NEXT1
$ENDIF
$!
$DEFINE SYS$ERROR _NLA0:
$DEFINE SYS$OUTPUT _NLA0:
$RUN TEST
$IF ($STATUS .NE. %X00000001)
$  THEN
$	HAVE_STRCASECMP=0
$  ELSE
$	 HAVE_STRCASECMP=1
$ENDIF
$DEAS  SYS$ERROR
$DEAS  SYS$OUTPUT
$NEXT1:
$IF (HAVE_STRCASECMP.EQ.1)
$  THEN
$ 	WRITE SYS$OUTPUT "Checking for strcasecmp ...   Yes"	
$  ELSE
$	WRITE SYS$OUTPUT "Checking for strcasecmp ...   No"
$ENDIF
$!
$!

$!"Checking for lfind "
$ DEFINE SYS$ERROR _NLA0:
$ DEFINE SYS$OUTPUT _NLA0:
$ CC/OBJECT=TEST.OBJ/INCLUDE=(ZLIB) SYS$INPUT
        #include  <search.h>

    int main()
        {
        lfind((const void *)key, (const void *)NULL, (size_t *)NULL,
           (size_t) 0, NULL);
        }
$!
$TMP = $STATUS
$DEASS SYS$ERROR
$DEAS  SYS$OUTPUT
$IF (TMP .NE. %X10B90001)
$  THEN
$       HAVE_LFIND=0
$       GOTO NEXT2
$ENDIF
$DEFINE SYS$ERROR _NLA0:
$DEFINE SYS$OUTPUT _NLA0:
$LINK/EXE=TEST TEST
$TMP = $STATUS
$DEAS  SYS$ERROR
$DEAS  SYS$OUTPUT
$!WRITE SYS$OUTPUT TMP
$IF (TMP .NE. %X10000001)
$  THEN
$       HAVE_LFIND=0
$       GOTO NEXT2
$  ELSE
$        HAVE_LFIND=1
$ENDIF
$!
$NEXT2:
$IF (HAVE_LFIND.EQ.1)
$  THEN
$       WRITE SYS$OUTPUT "Checking for lfind ...   Yes"
$  ELSE
$       WRITE SYS$OUTPUT "Checking for lfind ...   No"
$ENDIF
$!
$!
$!
$!"Checking for correct zlib library    "
$ DEFINE SYS$ERROR _NLA0:
$ DEFINE SYS$OUTPUT _NLA0:
$ CC/OBJECT=TEST.OBJ/INCLUDE=(ZLIB) SYS$INPUT
      #include <stdlib.h>
      #include <stdio.h>
      #include <zlib.h>
   int main()
     {
	printf("checking version zlib:  %s\n",zlibVersion());
     }
$TMP = $STATUS
$DEASS SYS$ERROR
$DEAS  SYS$OUTPUT
$!WRITE SYS$OUTPUT TMP
$IF (TMP .NE. %X10B90001) 
$  THEN 
$	HAVE_ZLIB=0
$	GOTO EXIT
$ENDIF
$DEFINE SYS$ERROR _NLA0:
$DEFINE SYS$OUTPUT _NLA0:
$LINK/EXE=TEST TEST,ZLIB:LIBZ/LIB 
$TMP = $STATUS
$DEAS  SYS$ERROR
$DEAS  SYS$OUTPUT
$!WRITE SYS$OUTPUT TMP
$IF (TMP .NE. %X10000001) 
$  THEN 
$	HAVE_ZLIB=0
$       GOTO EXIT
$  ELSE
$	HAVE_ZLIB=1
$ENDIF
$IF (HAVE_ZLIB.EQ.1)
$  THEN
$       WRITE SYS$OUTPUT "Checking for correct zlib library ...   Yes"
$  ELSE
$	WRITE SYS$OUTPUT "Checking for correct zlib library ...   No"
$       WRITE SYS$OUTPUT "This is fatal. Please download and install good library from fafner.dyndns.org/~alexey/libsdl/public.html
$ENDIF
$RUN TEST
$!
$! Checking for JPEG ...
$!"Checking for correct zlib library    "
$ DEFINE SYS$ERROR _NLA0:
$ DEFINE SYS$OUTPUT _NLA0:
$ CC/OBJECT=TEST.OBJ/INCLUDE=(JPEG) SYS$INPUT
      #include <stdlib.h>
      #include <stdio.h>
      #include <jpeglib.h>
      #include <jversion.h>	
   int main()
     {
	printf("checking version jpeg:  %s\n",JVERSION);
	jpeg_quality_scaling(0);
        return 0;
     }
$TMP = $STATUS
$DEASS SYS$ERROR
$DEAS  SYS$OUTPUT
$!WRITE SYS$OUTPUT TMP
$IF (TMP .NE. %X10B90001)
$  THEN
$	HAVE_JPEG=0
$       GOTO EXIT
$ENDIF
$DEFINE SYS$ERROR _NLA0:
$DEFINE SYS$OUTPUT _NLA0:
$LINK/EXE=TEST TEST,JPEG:LIBJPEG/LIB
$TMP = $STATUS
$DEAS  SYS$ERROR
$DEAS  SYS$OUTPUT
$!WRITE SYS$OUTPUT TMP
$IF (TMP .NE. %X10000001)
$  THEN
$	HAVE_JPEG=0
$       GOTO EXIT
$  ELSE
$	HAVE_JPEG=1
$ENDIF
$IF (HAVE_JPEG.EQ.1)
$  THEN
$       WRITE SYS$OUTPUT "Checking for correct jpeg library ...   Yes"
$  ELSE
$       WRITE SYS$OUTPUT "Checking for correct jpeg library ...   No"
$       WRITE SYS$OUTPUT "This is fatal. Please download and install good library from fafner.dyndns.org/~alexey/libsdl/public.html
$ENDIF
$RUN TEST
$!
$! Checking for X11 ...
$IF F$TRNLNM("DECW$INCLUDE") .NES. ""
$  THEN
$	WRITE SYS$OUTPUT "Checking for X11 ...   Yes"
$  ELSE
$	WRITE SYS$OUTPUT "Checking for X11 ...   No"
$	WRITE SYS$OUTPUT "This is fatal. Please install X11 software"
$	GOTO EXIT
$ENDIF
$!
$!WRITING BUILD FILES
$OPEN/WRITE OUT BUILD.COM
$ WRITE OUT "$set def [.libtiff]"
$ WRITE OUT "$",MAKE
$ WRITE OUT "$set def [-.PORT]"
$ WRITE OUT "$",MAKE
$ WRITE OUT "$set def [-.tools]"
$ WRITE OUT "$",MAKE
$ WRITE OUT "$set def [-]"
$ WRITE OUT "$cop [.PORT]LIBPORT.OLB [.LIBTIFF]LIBPORT.OLB"
$ WRITE OUT "$ CURRENT = F$ENVIRONMENT (""DEFAULT"") "
$ WRITE OUT "$TIFF=CURRENT"
$ WRITE OUT "$OPEN/WRITE OUTT LIBTIFF$STARTUP.COM"
$ WRITE OUT "$TIFF[F$LOCATE(""]"",TIFF),9]:="".LIBTIFF]"""
$ WRITE OUT "$WRITE OUTT ""DEFINE TIFF ","'","'","TIFF'""
$ WRITE OUT "$TIFF=CURRENT"
$ WRITE OUT "$TIFF[F$LOCATE(""]"",TIFF),7]:="".TOOLS]"""
$ WRITE OUT "$WRITE OUTT ""BMP2TIFF:==$", "'","'","TIFF'BMP2TIFF"""
$ WRITE OUT "$WRITE OUTT ""FAX2PS:==$", "'","'","TIFF'FAX2PS"""
$ WRITE OUT "$WRITE OUTT ""FAX2TIFF:==$", "'","'","TIFF'FAX2TIFF"""
$ WRITE OUT "$WRITE OUTT ""GIF2TIFF:==$", "'","'","TIFF'GIF2TIFF"""
$ WRITE OUT "$WRITE OUTT ""PAL2RGB:==$", "'","'","TIFF'PAL2RGB"""
$ WRITE OUT "$WRITE OUTT ""PPM2TIFF:==$", "'","'","TIFF'PPM2TIFF"""
$ WRITE OUT "$WRITE OUTT ""RAS2TIFF:==$", "'","'","TIFF'RAS2TIFF"""
$ WRITE OUT "$WRITE OUTT ""RAW2TIFF:==$", "'","'","TIFF'RAW2TIFF"""
$ WRITE OUT "$WRITE OUTT ""RGB2YCBCR:==$", "'","'","TIFF'RGB2YCBCR"""
$ WRITE OUT "$WRITE OUTT ""THUMBNAIL:==$", "'","'","TIFF'THUMBNAIL"""
$ WRITE OUT "$WRITE OUTT ""TIFF2BW:==$", "'","'","TIFF'TIFF2BW"""
$ WRITE OUT "$WRITE OUTT ""TIFF2PDF:==$", "'","'","TIFF'TIFF2PDF"""
$ WRITE OUT "$WRITE OUTT ""TIFF2PS:==$", "'","'","TIFF'TIFF2PS"""
$ WRITE OUT "$WRITE OUTT ""TIFF2RGBA:==$", "'","'","TIFF'TIFF2RGBA"""
$ WRITE OUT "$WRITE OUTT ""TIFFCMP:==$", "'","'","TIFF'TIFFCMP"""
$ WRITE OUT "$WRITE OUTT ""TIFFCP:==$", "'","'","TIFF'TIFFCP"""
$ WRITE OUT "$WRITE OUTT ""TIFFDITHER:==$", "'","'","TIFF'TIFFDITHER"""
$ WRITE OUT "$WRITE OUTT ""TIFFDUMP:==$", "'","'","TIFF'TIFFDUMP"""
$ WRITE OUT "$WRITE OUTT ""TIFFINFO:==$", "'","'","TIFF'TIFFINFO"""
$ WRITE OUT "$WRITE OUTT ""TIFFMEDIAN:==$", "'","'","TIFF'TIFFMEDIAN"""
$ WRITE OUT "$CLOSE OUTT"
$ WRITE OUT "$OPEN/WRITE OUTT [.LIBTIFF]LIBTIFF.OPT"
$ WRITE OUT "$WRITE OUTT ""TIFF:TIFF/LIB""
$ WRITE OUT "$WRITE OUTT ""TIFF:LIBPORT/LIB""
$ WRITE OUT "$WRITE OUTT ""JPEG:LIBJPEG/LIB""
$ WRITE OUT "$WRITE OUTT ""ZLIB:LIBZ/LIB""
$ WRITE OUT "$CLOSE OUTT"
$!
$ WRITE OUT "$WRITE SYS$OUTPUT "" ""
$ WRITE OUT "$WRITE SYS$OUTPUT ""***************************************************************************** ""
$ WRITE OUT "$WRITE SYS$OUTPUT ""LIBTIFF$STARTUP.COM has been created. ""
$ WRITE OUT "$WRITE SYS$OUTPUT ""This file setups all logicals needed. It should be execute before using LibTIFF ""
$ WRITE OUT "$WRITE SYS$OUTPUT ""Nice place to call it - LOGIN.COM ""
$ WRITE OUT "$WRITE SYS$OUTPUT ""***************************************************************************** ""
$CLOSE OUT
$!
$! DESCRIP.MMS in [.PORT]
$OBJ="DUMMY.OBJ"
$IF HAVE_STRCASECMP.NE.1 
$  THEN 
$     OBJ=OBJ+",STRCASECMP.OBJ"
$ENDIF
$IF HAVE_LFIND.NE.1   
$   THEN 
$       OBJ=OBJ+",LFIND.OBJ"
$ENDIF
$OPEN/WRITE OUT [.PORT]DESCRIP.MMS
$WRITE OUT "OBJ=",OBJ
$WRITE OUT ""
$WRITE OUT "LIBPORT.OLB : $(OBJ)"
$WRITE OUT "	LIB/CREA LIBPORT $(OBJ)"
$WRITE OUT ""
$CLOSE OUT
$!
$!
$COPY SYS$INPUT [.LIBTIFF]DESCRIP.MMS
# Makefile for DEC C compilers.
#

INCL    = /INCLUDE=(JPEG,ZLIB,[])

CFLAGS =  $(INCL)

OBJ_SYSDEP_MODULE = tif_vms.obj

OBJ     = tif_aux.obj, \
        tif_close.obj, \
        tif_codec.obj, \
        tif_color.obj, \
        tif_compress.obj, \
        tif_dir.obj, \
        tif_dirinfo.obj, \
        tif_dirread.obj, \
        tif_dirwrite.obj, \
        tif_dumpmode.obj, \
        tif_error.obj, \
        tif_extension.obj, \
        tif_fax3.obj, \
        tif_fax3sm.obj, \
        tif_getimage.obj, \
        tif_jpeg.obj, \
        tif_ojpeg.obj, \
        tif_flush.obj, \
        tif_luv.obj, \
        tif_lzw.obj, \
        tif_next.obj, \
        tif_open.obj, \
        tif_packbits.obj, \
        tif_pixarlog.obj, \
        tif_predict.obj, \
        tif_print.obj, \
        tif_read.obj, \
        tif_stream.obj, \
        tif_swab.obj, \
        tif_strip.obj, \
        tif_thunder.obj, \
        tif_tile.obj, \
        tif_version.obj, \
        tif_warning.obj, \
        tif_write.obj, \
        tif_zip.obj, \
        $(OBJ_SYSDEP_MODULE)


tiff.olb :  tif_config.h, tiffconf.h $(OBJ)
        lib/crea tiff.olb $(OBJ)

tif_config.h : tif_config.h-vms
        copy tif_config.h-vms tif_config.h

tiffconf.h : tiffconf.h-vms
        copy tiffconf.h-vms tiffconf.h

clean :
        del *.obj;*
        del *.olb;*
$!
$!
$!
$COPY SYS$INPUT [.TOOLS]DESCRIP.MMS
INCL            = /INCL=([],[-.LIBTIFF])
CFLAGS = $(INCL)

TARGETS =       bmp2tiff.exe tiffcp.exe tiffinfo.exe tiffdump.exe \
                fax2tiff.exe fax2ps.exe gif2tiff.exe pal2rgb.exe ppm2tiff.exe \
                rgb2ycbcr.exe thumbnail.exe ras2tiff.exe raw2tiff.exe \
                tiff2bw.exe tiff2rgba.exe tiff2pdf.exe tiff2ps.exe \
                tiffcmp.exe tiffdither.exe tiffmedian.exe 
#tiffsplit.exe

tiffsplit.exe : $(TARGETS)

bmp2tiff.exe : bmp2tiff.obj
        LINK bmp2tiff, [-.LIBTIFF]TIFF/LIB,JPEG:LIBJPEG/LIB, [-.PORT]LIBPORT/LIB

tiffcp.exe : tiffcp.obj
        LINK tiffcp, [-.LIBTIFF]TIFF/LIB,JPEG:LIBJPEG/LIB, [-.PORT]LIBPORT/LIB

tiffinfo.exe : tiffinfo.obj
        LINK tiffinfo, [-.LIBTIFF]TIFF/LIB,JPEG:LIBJPEG/LIB, [-.PORT]LIBPORT/LIB

tiffdump.exe : tiffdump.obj
        LINK tiffdump, [-.LIBTIFF]TIFF/LIB,JPEG:LIBJPEG/LIB, [-.PORT]LIBPORT/LIB

fax2tiff.exe : fax2tiff.obj
        LINK fax2tiff, [-.LIBTIFF]TIFF/LIB,JPEG:LIBJPEG/LIB, [-.PORT]LIBPORT/LIB

fax2ps.exe : fax2ps.obj
        LINK fax2ps, [-.LIBTIFF]TIFF/LIB,JPEG:LIBJPEG/LIB, [-.PORT]LIBPORT/LIB

gif2tiff.exe : gif2tiff.obj
        LINK gif2tiff, [-.LIBTIFF]TIFF/LIB,JPEG:LIBJPEG/LIB, [-.PORT]LIBPORT/LIB

pal2rgb.exe : pal2rgb.obj
        LINK pal2rgb, [-.LIBTIFF]TIFF/LIB,JPEG:LIBJPEG/LIB, [-.PORT]LIBPORT/LIB

ppm2tiff.exe : ppm2tiff.obj
        LINK ppm2tiff, [-.LIBTIFF]TIFF/LIB,JPEG:LIBJPEG/LIB, [-.PORT]LIBPORT/LIB

rgb2ycbcr.exe : rgb2ycbcr.obj
        LINK rgb2ycbcr, [-.LIBTIFF]TIFF/LIB,JPEG:LIBJPEG/LIB, [-.PORT]LIBPORT/LIB

thumbnail.exe : thumbnail.obj
        LINK thumbnail, [-.LIBTIFF]TIFF/LIB,JPEG:LIBJPEG/LIB, [-.PORT]LIBPORT/LIB

ras2tiff.exe : ras2tiff.obj
        LINK ras2tiff, [-.LIBTIFF]TIFF/LIB,JPEG:LIBJPEG/LIB, [-.PORT]LIBPORT/LIB

raw2tiff.exe : raw2tiff.obj
        LINK raw2tiff, [-.LIBTIFF]TIFF/LIB,JPEG:LIBJPEG/LIB, [-.PORT]LIBPORT/LIB

tiff2bw.exe : tiff2bw.obj
        LINK tiff2bw, [-.LIBTIFF]TIFF/LIB,JPEG:LIBJPEG/LIB, [-.PORT]LIBPORT/LIB

tiff2rgba.exe : tiff2rgba.obj
        LINK tiff2rgba, [-.LIBTIFF]TIFF/LIB,JPEG:LIBJPEG/LIB, [-.PORT]LIBPORT/LIB

tiff2pdf.obj : tiff2pdf.c
        CC/NOWARN $(CFLAGS) tiff2pdf

tiff2pdf.exe : tiff2pdf.obj
        LINK tiff2pdf, [-.LIBTIFF]TIFF/LIB,JPEG:LIBJPEG/LIB, [-.PORT]LIBPORT/LIB

tiff2ps.exe : tiff2ps.obj
        LINK tiff2ps, [-.LIBTIFF]TIFF/LIB,JPEG:LIBJPEG/LIB, [-.PORT]LIBPORT/LIB

tiffcmp.exe : tiffcmp.obj
        LINK tiffcmp, [-.LIBTIFF]TIFF/LIB,JPEG:LIBJPEG/LIB, [-.PORT]LIBPORT/LIB

tiffdither.exe : tiffdither.obj
        LINK tiffdither, [-.LIBTIFF]TIFF/LIB,JPEG:LIBJPEG/LIB, [-.PORT]LIBPORT/LIB

tiffmedian.exe : tiffmedian.obj
        LINK tiffmedian, [-.LIBTIFF]TIFF/LIB,JPEG:LIBJPEG/LIB, [-.PORT]LIBPORT/LIB

tiffsplit.exe : tiffsplit.obj
        LINK tiffsplit, [-.LIBTIFF]TIFF/LIB,JPEG:LIBJPEG/LIB, [-.PORT]LIBPORT/LIB

CLEAN :
	DEL ALL.;*
	DEL *.OBJ;*
	DEL *.EXE;*

$!
$!
$WRITE SYS$OUTPUT " "
$WRITE SYS$OUTPUT " "
$WRITE SYS$OUTPUT "Now you can type @BUILD "
$!
$EXIT:
$DEFINE SYS$ERROR _NLA0:
$DEFINE SYS$OUTPUT _NLA0:
$DEL TEST.OBJ;*
$DEL TEST.C;*
$DEL TEST.EXE;*
$DEAS SYS$ERROR
$DEAS SYS$OUTPUT
