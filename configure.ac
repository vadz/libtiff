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
AC_INIT([LibTIFF Software], 3.7.0, [tiff@remotesensing.org], libtiff)
AM_INIT_AUTOMAKE
AC_CONFIG_SRCDIR(libtiff/config.h.in)
AM_CONFIG_HEADER(libtiff/config.h)

dnl Versioning
LIBTIFF_MAJOR_VERSION=3
LIBTIFF_MINOR_VERSION=7
LIBTIFF_MICRO_VERSION=0
LIBTIFF_VERSION=$LIBTIFF_MAJOR_VERSION.$LIBTIFF_MINOR_VERSION.$LIBTIFF_MICRO_VERSION

AC_SUBST(LIBTIFF_MAJOR_VERSION)
AC_SUBST(LIBTIFF_MINOR_VERSION)
AC_SUBST(LIBTIFF_MICRO_VERSION)
AC_SUBST(LIBTIFF_VERSION)

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
AC_HEADER_STDBOOL
AC_C_CONST
AC_C_BIGENDIAN
AC_TYPE_OFF_T
AC_TYPE_SIZE_T
AC_HEADER_TIME
AC_STRUCT_TM

dnl Checks for library functions.
AC_FUNC_ERROR_AT_LINE
AC_FUNC_MALLOC
AC_FUNC_MEMCMP
AC_FUNC_MMAP
AC_FUNC_REALLOC
AC_FUNC_VPRINTF
AC_CHECK_FUNCS([floor gettimeofday isascii memmove memset munmap pow sqrt strcasecmp strchr strncasecmp strrchr strstr strtol strtoul])

AC_SYS_LARGEFILE

dnl ---------------------------------------------------------------------------
dnl Check for ZLIB.
dnl ---------------------------------------------------------------------------

HAVE_ZLIB=no

AC_ARG_ENABLE(zlib, AS_HELP_STRING([--disable-zlib], [disable Zlib usage (required for Deflate compression, enabled by default)]),,)
AC_ARG_WITH(zlib-include-dir, AS_HELP_STRING([--with-zlib-include-dir=DIR], [location of Zlib headers (specify, if not in standard place)]),,)
AC_ARG_WITH(zlib-lib-dir, AS_HELP_STRING([--with-zlib-lib-dir=DIR], [location of Zlib library binary (specify, if not in standard place)]),,)

if test "x$enable_zlib" != "xno" ; then

  if test "x$with_zlib_lib" = "xyes" ; then
    AC_CHECK_LIB(z, inflateEnd, [zlib_lib=yes], [zlib_lib=no],)
  elif test "x$with_zlib_lib" != "xno" ; then
    LIBS="$with_zlib_lib $LIBS"
    AC_CHECK_LIB(z, inflateEnd, [zlib_lib=yes], [zlib_lib=no],)
    if test "$zlib_lib" = "no" ; then
      AC_MSG_ERROR([Zlib library not found at $with_zlib_lib])
    fi
  fi
    
  if test "x$with_zlib_h" = "xyes" ; then
    AC_CHECK_HEADER(zlib.h, [zlib_h=yes], [zlib_h=no])
  elif test "x$with_zlib_h" != "xno" ; then
    INCLUDE="$with_zlib_h $INCLUDE"
    AC_CHECK_HEADER(zlib.h, [zlib_h=yes], [zlib_h=no])
    if test "$zlib_h" = "no" ; then
      AC_MSG_ERROR([Zlib headers not found at $with_zlib_h])
    fi
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
dnl Check for JPEG.
dnl ---------------------------------------------------------------------------

HAVE_JPEG=no

AC_ARG_ENABLE(jpeg, AS_HELP_STRING([--disable-jpeg], [disable IJG JPEG library usage (required for JPEG compression, enabled by default)]),,)
AC_ARG_WITH(jpeg-include-dir, AS_HELP_STRING([--with-jpeg-include-dir=DIR], [location of IJG JPEG library headers (specify, if not in standard place)]),,)
AC_ARG_WITH(jpeg-lib-dir, AS_HELP_STRING([--with-jpeg-lib-dir=DIR], [location of IJG JPEG library binary (specify, if not in standard place)]),,)

if test "x$enable_jpeg" != "xno" ; then

  if test "x$with_jpeg_lib" = "xyes" ; then
    AC_CHECK_LIB(jpeg, jpeg_read_scanlines, [jpeg_lib=yes], [jpeg_lib=no],)
  elif test "x$with_jpeg_lib" != "xno" ; then
    LIBS="$with_jpeg_lib $LIBS"
    AC_CHECK_LIB(jpeg, jpeg_read_scanlines, [jpeg_lib=yes], [jpeg_lib=no],)
    if test "$jpeg_lib" = "no" ; then
      AC_MSG_ERROR([IJG JPEG library not found at $with_jpeg_lib])
    fi
  fi
    
  if test "x$with_jpeg_h" = "xyes" ; then
    AC_CHECK_HEADER(jpeglib.h, [jpeg_h=yes], [jpeg_h=no])
  elif test "x$with_jpeg_h" != "xno" ; then
    INCLUDE="$with_jpeg_h $INCLUDE"
    AC_CHECK_HEADER(jpeglib.h, [jpeg_h=yes], [jpeg_h=no])
    if test "$jpeg_h" = "no" ; then
      AC_MSG_ERROR([IJG JPEG library headers not found at $with_jpeg_h])
    fi
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

HAVE_OJPEG=no

AC_ARG_ENABLE(old-jpeg, AS_HELP_STRING([--enable-old-jpeg], [enable support for Old JPEG compresson (read contrib/ojpeg/README first! Compilation fails with unpatched IJG JPEG library)]), [HAVE_OJPEG=yes], [HAVE_OJPEG=no])

if test "$HAVE_JPEG" = "yes" -a "$HAVE_OJPEG" = "yes" ; then
  CPPFLAGS="-DOJPEG_SUPPORT $CPPFLAGS"
fi

dnl ---------------------------------------------------------------------------
dnl Check for OpenGL and GLUT.
dnl ---------------------------------------------------------------------------

HAVE_OPENGL=no
OPENGL_LIBS=""

AC_ARG_ENABLE(opengl, AS_HELP_STRING([--disable-opengl], [disable building OpenGL viewer (enabled by default)]),,)
AC_ARG_WITH(opengl-include-dir, AS_HELP_STRING([--with-opengl-include-dir=DIR], [location of OpenGL library headers (specify, if not in standard place)]),,)
AC_ARG_WITH(opengl-lib-dir,AS_HELP_STRING([--with-opengl-lib-dir=DIR], [location of OpenGL library binary (specify, if not in standard place)]),,)
AC_ARG_WITH(glut-include-dir, AS_HELP_STRING([--with-glut-include-dir=DIR], [location of GLUT library headers (specify, if not in standard place)]),,)
AC_ARG_WITH(glut-lib-dir, AS_HELP_STRING([--with-glut-lib-dir=DIR], [location of GLUT library binary (specify, if not in standard place)]),,)

if test "x$enable_opengl" != "xno" ; then

  if test "x$with_opengl_lib" = "xyes" ; then
    AC_CHECK_LIB(GL, glDrawPixels, [opengl_lib=yes], [opengl_lib=no],)
  elif test "x$with_opengl_lib" != "xno" ; then
    LIBS="$with_opengl_lib $LIBS"
    AC_CHECK_LIB(GL, glDrawPixels, [opengl_lib=yes], [opengl_lib=no],)
    if test "$opengl_lib" = "no" ; then
      AC_MSG_ERROR([OpenGL library not found at $with_opengl_lib])
    fi
  fi
    
  if test "x$with_opengl_h" = "xyes" ; then
    AC_CHECK_HEADER(GL/gl.h, [opengl_h=yes], [opengl_h=no])
  elif test "x$with_opengl_h" != "xno" ; then
    INCLUDE="$with_opengl_h $INCLUDE"
    AC_CHECK_HEADER(GL/gl.h, [opengl_h=yes], [opengl_h=no])
    if test "$opengl_h" = "no" ; then
      AC_MSG_ERROR([OpenGL library headers not found at $with_opengl_h])
    fi
  fi

  if test "x$with_glut_lib" = "xyes" ; then
    AC_CHECK_LIB(glut, glutInit, [glut_lib=yes], [glut_lib=no],)
  elif test "x$with_glut_lib" != "xno" ; then
    LIBS="$with_glut_lib $LIBS"
    AC_CHECK_LIB(glut, glutInit, [glut_lib=yes], [glut_lib=no],)
    if test "$glut_lib" = "no" ; then
      AC_MSG_ERROR([GLUT library not found at $with_glut_lib])
    fi
  fi
    
  if test "x$with_glut_h" = "xyes" ; then
    AC_CHECK_HEADER(GL/glut.h, [glut_h=yes], [glut_h=no])
  elif test "x$with_glut_h" != "xno" ; then
    INCLUDE="$with_glut_h $INCLUDE"
    AC_CHECK_HEADER(GL/glut.h, [glut_h=yes], [glut_h=no])
    if test "$glut_h" = "no" ; then
      AC_MSG_ERROR([GLUT library headers not found at $with_glut_h])
    fi
  fi

  if test "$opengl_lib" = "yes" -a "$opengl_h" = "yes" \
          -a "$glut_lib" = "yes" -a "$glut_h" = "yes" ; then
    HAVE_OPENGL=yes
    OPENGL_LIBS="-lGL -lglut"
  fi

fi

AM_CONDITIONAL(HAVE_OPENGL, test "$HAVE_OPENGL" = "yes")
AC_SUBST(OPENGL_LIBS)

dnl ---------------------------------------------------------------------------

AC_CONFIG_FILES([Makefile \
		 contrib/Makefile \
		 contrib/addtiffo/Makefile \
                 html/Makefile \
		 html/images/Makefile \
		 html/man/Makefile \
                 libtiff/Makefile \
                 man/Makefile \
		 port/Makefile \
                 tools/Makefile])
AC_OUTPUT


