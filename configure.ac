dnl                                               -*- Autoconf -*-
dnl Tag Image File Format (TIFF) Software
dnl
dnl Copyright (C) 2004, Andrey Kiselev <dron@remotesensing.org>
dnl
dnl Permission to use, copy, modify, distribute, and sell this software and 
dnl its documentation for any purpose is hereby granted without fee, provided
dnl that (i) the above copyright notices and this permission notice appear in
dnl all copies of the software and related documentation, and (ii) the names of
dnl Sam Leffler and Silicon Graphics may not be used in any advertising or
dnl publicity relating to the software without the specific, prior written
dnl permission of Sam Leffler and Silicon Graphics.
dnl 
dnl THE SOFTWARE IS PROVIDED "AS-IS" AND WITHOUT WARRANTY OF ANY KIND, 
dnl EXPRESS, IMPLIED OR OTHERWISE, INCLUDING WITHOUT LIMITATION, ANY 
dnl WARRANTY OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE.  
dnl 
dnl IN NO EVENT SHALL SAM LEFFLER OR SILICON GRAPHICS BE LIABLE FOR
dnl ANY SPECIAL, INCIDENTAL, INDIRECT OR CONSEQUENTIAL DAMAGES OF ANY KIND,
dnl OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
dnl WHETHER OR NOT ADVISED OF THE POSSIBILITY OF DAMAGE, AND ON ANY THEORY OF 
dnl LIABILITY, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE 
dnl OF THIS SOFTWARE.

dnl Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)
AC_INIT([LibTIFF Software], 3.7.0alpha, [tiff@remotesensing.org], tiff)

dnl Compute the canonical target-system type variable
AC_CANONICAL_TARGET

AM_INIT_AUTOMAKE
dnl Do not rebuild generated files every time
AM_MAINTAINER_MODE
AC_CONFIG_SRCDIR(libtiff/config.h.in)
AM_CONFIG_HEADER(libtiff/config.h)

dnl Versioning.
dnl Don't fill the ALPHA_VERSION field, if not applicable.
LIBTIFF_MAJOR_VERSION=3
LIBTIFF_MINOR_VERSION=7
LIBTIFF_MICRO_VERSION=0
LIBTIFF_ALPHA_VERSION=alpha
LIBTIFF_VERSION=$LIBTIFF_MAJOR_VERSION.$LIBTIFF_MINOR_VERSION.$LIBTIFF_MICRO_VERSION$LIBTIFF_ALPHA_VERSION
dnl This will be used with the 'make release' target
LIBTIFF_RELEASE_DATE=`date +"%Y%m%d"`

AC_SUBST(LIBTIFF_MAJOR_VERSION)
AC_SUBST(LIBTIFF_MINOR_VERSION)
AC_SUBST(LIBTIFF_MICRO_VERSION)
AC_SUBST(LIBTIFF_ALPHA_VERSION)
AC_SUBST(LIBTIFF_VERSION)
AC_SUBST(LIBTIFF_RELEASE_DATE)

dnl Checks for programs.
AC_PROG_LIBTOOL
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_RANLIB

dnl Checks for libraries.
AC_CHECK_LIB([c], [main])

dnl We don't need to add math library at all targets
case "$target_os" in
    cygwin* | mingw32* | beos* | darwin*)
        ;;
    *)
	AC_CHECK_LIB(m,main,,,)
        ;;
esac

dnl Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([fcntl.h limits.h malloc.h memory.h stdlib.h string.h sys/time.h unistd.h])

dnl Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_C_INLINE
AC_C_BIGENDIAN
AC_TYPE_OFF_T
AC_TYPE_SIZE_T
AC_HEADER_TIME
AC_STRUCT_TM
dnl Check for BSDTYPES
AC_CHECK_TYPES([u_char, u_short, u_int, u_long])
dnl Some compilers (IBM VisualAge) has these types defined, so check it here:
AC_CHECK_TYPES([int8, int16, int32])

dnl Checks for library functions.
AC_CHECK_FUNCS([floor isascii memmove memset mmap pow sqrt strcasecmp strchr strncasecmp strrchr strstr strtol])

dnl Will use local replacements for unavailable functions
AC_REPLACE_FUNCS(getopt)
AC_REPLACE_FUNCS(strcasecmp)
AC_REPLACE_FUNCS(strtoul)

AC_SYS_LARGEFILE

dnl ---------------------------------------------------------------------------
dnl Check the native cpu bit order.
dnl ---------------------------------------------------------------------------
AC_MSG_CHECKING([native cpu bit order])
case "$target_cpu" in
    i*86*)
        HOST_FILLORDER=FILLORDER_LSB2MSB
	AC_MSG_RESULT([lsb2msb])
	;;
    *)
	HOST_FILLORDER=FILLORDER_MSB2LSB
	AC_MSG_RESULT([msb2lsb])
        ;;
esac
AC_DEFINE_UNQUOTED(HOST_FILLORDER, $HOST_FILLORDER, [Set the native cpu bit order])

dnl ---------------------------------------------------------------------------
dnl Set the floating point format.
dnl FIXME: write appropriate test.
dnl ---------------------------------------------------------------------------
HAVE_IEEEFP=1
AC_DEFINE_UNQUOTED(HAVE_IEEEFP, $HAVE_IEEEFP, [Define as 0 or 1 according to the floating point format suported by the machine])

dnl ---------------------------------------------------------------------------
dnl Switch on/off internal codecs.
dnl ---------------------------------------------------------------------------

HAVE_CCITT=yes

AC_ARG_ENABLE(ccitt, AS_HELP_STRING([--disable-ccitt], [disable support for CCITT Group 3 & 4 algorithms]), [HAVE_CCITT=no], [HAVE_CCITT=yes])

if test "$HAVE_CCITT" = "yes" ; then
  CPPFLAGS="-DCCITT_SUPPORT $CPPFLAGS"
fi

HAVE_PACKBITS=yes

AC_ARG_ENABLE(packbits, AS_HELP_STRING([--disable-packbits], [disable support for Macintosh PackBits algorithm]), [HAVE_PACKBITS=no], [HAVE_PACKBITS=yes])

if test "$HAVE_PACKBITS" = "yes" ; then
  CPPFLAGS="-DPACKBITS_SUPPORT $CPPFLAGS"
fi

HAVE_LZW=yes

AC_ARG_ENABLE(lzw, AS_HELP_STRING([--disable-lzw], [disable support for LZW algorithm]), [HAVE_LZW=no], [HAVE_LZW=yes])

if test "$HAVE_LZW" = "yes" ; then
  CPPFLAGS="-DLZW_SUPPORT $CPPFLAGS"
fi

HAVE_THUNDER=yes

AC_ARG_ENABLE(thunder, AS_HELP_STRING([--disable-thunder], [disable support for ThunderScan 4-bit RLE algorithm]), [HAVE_THUNDER=no], [HAVE_THUNDER=yes])

if test "$HAVE_THUNDER" = "yes" ; then
  CPPFLAGS="-DTHUNDER_SUPPORT $CPPFLAGS"
fi

HAVE_NEXT=yes

AC_ARG_ENABLE(next, AS_HELP_STRING([--disable-next], [disable support for NeXT 2-bit RLE algorithm]), [HAVE_NEXT=no], [HAVE_NEXT=yes])

if test "$HAVE_NEXT" = "yes" ; then
  CPPFLAGS="-DNEXT_SUPPORT $CPPFLAGS"
fi

HAVE_LOGLUV=yes

AC_ARG_ENABLE(logluv, AS_HELP_STRING([--disable-logluv], [disable support for LogLuv high dynamic range encoding]), [HAVE_LOGLUV=no], [HAVE_LOGLUV=yes])

if test "$HAVE_LOGLUV" = "yes" ; then
  CPPFLAGS="-DLOGLUV_SUPPORT $CPPFLAGS"
fi

dnl ---------------------------------------------------------------------------
dnl Check for ZLIB.
dnl ---------------------------------------------------------------------------

HAVE_ZLIB=no

AC_ARG_ENABLE(zlib, AS_HELP_STRING([--disable-zlib], [disable Zlib usage (required for Deflate compression, enabled by default)]),,)
AC_ARG_WITH(zlib-include-dir, AS_HELP_STRING([--with-zlib-include-dir=DIR], [location of Zlib headers]),,)
AC_ARG_WITH(zlib-lib-dir, AS_HELP_STRING([--with-zlib-lib-dir=DIR], [location of Zlib library binary]),,)

if test "x$enable_zlib" != "xno" ; then

  if test "x$with_zlib_lib_dir" != "x" ; then
    LIBS="-L$with_zlib_lib_dir $LIBS"
  fi
  AC_CHECK_LIB(z, inflateEnd, [zlib_lib=yes], [zlib_lib=no],)
  if test "$zlib_lib" = "no" -a "x$with_zlib_lib_dir" != "x"; then
    AC_MSG_ERROR([Zlib library not found at $with_zlib_lib_dir])
  fi
    
  if test "x$with_zlib_include_dir" != "x" ; then
    INCLUDE="-I$with_zlib_include_dir $INCLUDE"
  fi
  AC_CHECK_HEADER(zlib.h, [zlib_h=yes], [zlib_h=no])
  if test "$zlib_h" = "no" -a "x$with_zlib_include_dir" != "x" ; then
    AC_MSG_ERROR([Zlib headers not found at $with_zlib_include_dir])
  fi

  if test "$zlib_lib" = "yes" -a "$zlib_h" = "yes" ; then
    HAVE_ZLIB=yes
  fi

fi

if test "$HAVE_ZLIB" = "yes" ; then
  CPPFLAGS="-DZIP_SUPPORT $CPPFLAGS"
  LIBS="-lz $LIBS"
fi

dnl ---------------------------------------------------------------------------
dnl Check for Pixar log-format algorithm.
dnl ---------------------------------------------------------------------------

AC_ARG_ENABLE(pixarlog, AS_HELP_STRING([--disable-pixarlog], [disable support for Pixar log-format algorithm (requires Zlib)]), [HAVE_PIXARLOG=no], [HAVE_PIXARLOG=yes])

if test "$HAVE_ZLIB" = "yes" -a "$HAVE_PIXARLOG" = "yes" ; then
  CPPFLAGS="-DPIXARLOG_SUPPORT $CPPFLAGS"
else
  HAVE_PIXARLOG=no
fi

dnl ---------------------------------------------------------------------------
dnl Check for JPEG.
dnl ---------------------------------------------------------------------------

HAVE_JPEG=no

AC_ARG_ENABLE(jpeg, AS_HELP_STRING([--disable-jpeg], [disable IJG JPEG library usage (required for JPEG compression, enabled by default)]),,)
AC_ARG_WITH(jpeg-include-dir, AS_HELP_STRING([--with-jpeg-include-dir=DIR], [location of IJG JPEG library headers]),,)
AC_ARG_WITH(jpeg-lib-dir, AS_HELP_STRING([--with-jpeg-lib-dir=DIR], [location of IJG JPEG library binary]),,)

if test "x$enable_jpeg" != "xno" ; then

  if test "x$with_jpeg_lib_dir" != "x" ; then
    LIBS="-L$with_jpeg_lib_dir $LIBS"
  fi
  AC_CHECK_LIB(jpeg, jpeg_read_scanlines, [jpeg_lib=yes], [jpeg_lib=no],)
  if test "$jpeg_lib" = "no" -a "x$with_jpeg_lib_dir" != "x" ; then
    AC_MSG_ERROR([IJG JPEG library not found at $with_jpeg_lib_dir])
  fi
    
  if test "x$with_jpeg_include_dir" != "x" ; then
    INCLUDE="-I$with_jpeg_include_dir $INCLUDE"
  fi
  AC_CHECK_HEADER(jpeglib.h, [jpeg_h=yes], [jpeg_h=no])
  if test "$jpeg_h" = "no" -a "x$with_jpeg_include_dir" != "x" ; then
    AC_MSG_ERROR([IJG JPEG library headers not found at $with_jpeg_include_dir])
  fi

  if test "$jpeg_lib" = "yes" -a "$jpeg_h" = "yes" ; then
    HAVE_JPEG=yes
  fi

fi

if test "$HAVE_JPEG" = "yes" ; then
  CPPFLAGS="-DJPEG_SUPPORT $CPPFLAGS"
  LIBS="-ljpeg $LIBS"
fi

dnl ---------------------------------------------------------------------------
dnl Check for Old JPEG.
dnl ---------------------------------------------------------------------------

AC_ARG_ENABLE(old-jpeg, AS_HELP_STRING([--enable-old-jpeg], [enable support for Old JPEG compresson (read contrib/ojpeg/README first! Compilation fails with unpatched IJG JPEG library)]), [HAVE_OJPEG=yes], [HAVE_OJPEG=no])

if test "$HAVE_JPEG" = "yes" -a "$HAVE_OJPEG" = "yes" ; then
  CPPFLAGS="-DOJPEG_SUPPORT $CPPFLAGS"
else
  HAVE_OJPEG=no
fi

dnl ---------------------------------------------------------------------------
dnl Check for OpenGL and GLUT.
dnl ---------------------------------------------------------------------------

HAVE_OPENGL=no
OPENGL_LIBS=""

AC_ARG_ENABLE(opengl, AS_HELP_STRING([--disable-opengl], [disable building OpenGL viewer (enabled by default)]),,)
AC_ARG_WITH(opengl-include-dir, AS_HELP_STRING([--with-opengl-include-dir=DIR], [location of OpenGL library headers]),,)
AC_ARG_WITH(opengl-lib-dir,AS_HELP_STRING([--with-opengl-lib-dir=DIR], [location of OpenGL library binary]),,)
AC_ARG_WITH(glut-include-dir, AS_HELP_STRING([--with-glut-include-dir=DIR], [location of GLUT library headers]),,)
AC_ARG_WITH(glut-lib-dir, AS_HELP_STRING([--with-glut-lib-dir=DIR], [location of GLUT library binary]),,)

if test "x$enable_opengl" != "xno" ; then

  if test "x$with_opengl_lib_dir" != "x" ; then
    LIBS="-L$with_opengl_lib_dir $LIBS"
  fi
  AC_CHECK_LIB(GL, glDrawPixels, [opengl_lib=yes], [opengl_lib=no],)
  if test "$opengl_lib" = "no" -a "x$with_opengl_lib_dir" != "x" ; then
    AC_MSG_ERROR([OpenGL library not found at $with_opengl_lib_dir])
  fi
    
  if test "x$with_opengl_include_dir" != "x" ; then
    INCLUDE="-I$with_opengl_include_dir $INCLUDE"
  fi
  AC_CHECK_HEADER(GL/gl.h, [opengl_h=yes], [opengl_h=no])
  if test "$opengl_h" = "no" -a "x$with_opengl_include_dir" != "x" ; then
    AC_MSG_ERROR([OpenGL library headers not found at $with_opengl_include_dir])
  fi

  if test "x$with_glut_lib_dir" != "x" ; then
    LIBS="-L$with_glut_lib_dir $LIBS"
  fi
  AC_CHECK_LIB(glut, glutInit, [glut_lib=yes], [glut_lib=no],)
  if test "$glut_lib" = "no" -a "x$with_glut_lib_dir" != "x" ; then
    AC_MSG_ERROR([GLUT library not found at $with_glut_lib_dir])
  fi
    
  if test "x$with_glut_include_dir" != "x" ; then
    INCLUDE="-I$with_glut_include_dir $INCLUDE"
  fi
  AC_CHECK_HEADER(GL/glut.h, [glut_h=yes], [glut_h=no])
  if test "$glut_h" = "no" -a "x$with_glut_include_dir" != "x" ; then
    AC_MSG_ERROR([GLUT library headers not found at $with_glut_include_dir])
  fi

  if test "$opengl_lib" = "yes" -a "$opengl_h" = "yes" \
          -a "$glut_lib" = "yes" -a "$glut_h" = "yes" ; then
    HAVE_OPENGL=yes
    OPENGL_LIBS="-lGL -lglut"
  fi

fi

AM_CONDITIONAL(HAVE_OPENGL, test "$HAVE_OPENGL" = "yes")
AC_SUBST(OPENGL_LIBS)

dnl ===========================================================================
dnl ``Orthogonal Features''
dnl ===========================================================================

dnl ---------------------------------------------------------------------------
dnl Default handling of strip chopping support.
dnl ---------------------------------------------------------------------------

HAVE_STRIPCHOP=yes

AC_ARG_ENABLE(strip-chopping, AS_HELP_STRING([--disable-strip-chopping], [disable support for strip chopping (whether or not to convert single-strip uncompressed images to mutiple strips of ~8Kb to reduce memory usage)]), [HAVE_STRIPCHOP=no], [HAVE_STRIPCHOP=yes])

if test "$HAVE_STRIPCHOP" = "yes" ; then
  CPPFLAGS="-DSTRIPCHOP_DEFAULT=TIFF_STRIPCHOP $CPPFLAGS"
fi

dnl ---------------------------------------------------------------------------
dnl Default handling of ASSOCALPHA support.
dnl ---------------------------------------------------------------------------

HAVE_EXTRASAMPLE_AS_ALPHA=yes

AC_ARG_ENABLE(extrasample-as-alpha, AS_HELP_STRING([--disable-extrasample-as-alpha], [the RGBA interface will treat a fourth sample with no EXTRASAMPLE_ value as being ASSOCALPHA. Many packages produce RGBA files but don't mark the alpha properly]), [HAVE_EXTRASAMPLE_AS_ALPHA=no], [HAVE_EXTRASAMPLE_AS_ALPHA=yes])

if test "$HAVE_EXTRASAMPLE_AS_ALPHA" = "yes" ; then
  CPPFLAGS="-DDEFAULT_EXTRASAMPLE_AS_ALPHA $CPPFLAGS"
fi

dnl ---------------------------------------------------------------------------
dnl Default handling of YCbCr subsampling support.
dnl See Bug 168 in Bugzilla, and JPEGFixupTestSubsampling() for details.
dnl ---------------------------------------------------------------------------

CHECK_JPEG_YCBCR_SUBSAMPLING=yes

AC_ARG_ENABLE(check-ycbcr-subsampling, AS_HELP_STRING([--disable-check-ycbcr-subsampling], [disable picking up YCbCr subsampling info from the JPEG data stream to support files lacking the tag]), [CHECK_JPEG_YCBCR_SUBSAMPLING=no], [CHECK_JPEG_YCBCR_SUBSAMPLING=yes])

if test "$CHECK_JPEG_YCBCR_SUBSAMPLING" = "yes" ; then
  CPPFLAGS="-DCHECK_JPEG_YCBCR_SUBSAMPLING $CPPFLAGS"
fi

dnl ---------------------------------------------------------------------------

AC_CONFIG_FILES([Makefile \
		 contrib/Makefile \
		 contrib/acorn/Makefile \
		 contrib/addtiffo/Makefile \
		 contrib/dbs/Makefile \
		 contrib/dbs/xtiff/Makefile \
		 contrib/iptcutil/Makefile \
		 contrib/mac-cw/Makefile \
		 contrib/mac-mpw/Makefile \
		 contrib/mfs/Makefile \
		 contrib/ojpeg/Makefile \
		 contrib/pds/Makefile \
		 contrib/ras/Makefile \
		 contrib/stream/Makefile \
		 contrib/tags/Makefile \
		 contrib/win_dib/Makefile \
                 html/Makefile \
		 html/images/Makefile \
		 html/man/Makefile \
                 libtiff/Makefile \
                 man/Makefile \
		 port/Makefile \
                 tools/Makefile])
AC_OUTPUT

dnl ---------------------------------------------------------------------------
dnl Display configuration status
dnl ---------------------------------------------------------------------------

LOC_MSG()
LOC_MSG([Libtiff is now configured for ${host}])
LOC_MSG()
LOC_MSG([  Installation directory:            ${prefix}])
LOC_MSG([  C compiler:                        ${CC} ${CFLAGS}])
LOC_MSG()
LOC_MSG([ Support for internal codecs:])
LOC_MSG([  CCITT Group 3 & 4 algorithms:      ${HAVE_CCITT}])
LOC_MSG([  Macintosh PackBits algorithm:      ${HAVE_PACKBITS}])
LOC_MSG([  LZW algorithm:                     ${HAVE_LZW}])
LOC_MSG([  ThunderScan 4-bit RLE algorithm:   ${HAVE_THUNDER}])
LOC_MSG([  NeXT 2-bit RLE algorithm:          ${HAVE_NEXT}])
LOC_MSG([  LogLuv high dynamic range encoding:${HAVE_LOGLUV}])
LOC_MSG()
LOC_MSG([ Support for external codecs:])
LOC_MSG([  ZLIB support:                      ${HAVE_ZLIB}])
LOC_MSG([  Pixar log-format algorithm:        ${HAVE_PIXARLOG}])
LOC_MSG([  JPEG support:                      ${HAVE_JPEG}])
LOC_MSG([  Old JPEG support:                  ${HAVE_OJPEG}])
LOC_MSG()
LOC_MSG([  OpenGL support:                    ${HAVE_OPENGL}])
LOC_MSG()

