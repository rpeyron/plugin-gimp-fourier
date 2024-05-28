# -*- Autoconf -*-
dnl Process this file with "autoreconf -i;automake" to produce a configure script.

# Copyright (C) 2022 by Joe Da Silva

AC_PREREQ([2.68])
#--------------------------------------------------------------------------
# Setup variables before running AC_INIT()
#
# Making point releases:
#   fourier_major_version += 0;
#   fourier_minor_version += 1; (patches or added function(s))
#
# If any new functions have been added:
#   fourier_major_version += 0;
#   fourier_minor_version += 1; (added function(s))
#
# If backwards compatibility has been broken:
#   fourier_major_version += 1;
#   fourier_minor_version = 0;
#
m4_define([fourier_major_version], [0.4])
m4_define([fourier_minor_version], [5])
m4_define([fourier_version],[fourier_major_version.fourier_minor_version])
m4_define([fourier_package_name], [gimp-plugin-fourier])
m4_define([fourier_package_home], [https://www.lprp.fr/gimp_plugin_en/])
m4_define([fourier_package_email], [https://github.com/rpeyron/plugin-gimp-fourier/issues])

#--------------------------------------------------------------------------
AC_INIT([fourier],[fourier_version],[fourier_package_email],
	[fourier_package_name],[fourier_package_home])
# old registry location was: http://registry.gimp.org/node/19596
#--------------------------------------------------------------------------
AC_CONFIG_SRCDIR([fourier.c])
AC_CONFIG_MACRO_DIR([m4])
AC_CANONICAL_HOST
AC_CANONICAL_BUILD
AC_USE_SYSTEM_EXTENSIONS
AM_INIT_AUTOMAKE([foreign -Wall])

#--------------------------------------------------------------------------
# automake 1.12 needs AM_PROG_AR but automake < 1.11.2 doesn't recognize it
m4_ifdef([AM_PROG_AR], [AM_PROG_AR])

LT_INIT
AC_SUBST([LIBTOOL_DEPS])

#--------------------------------------------------------------------------
# Checks for programs.
AC_PROG_CC
AC_PROG_GREP
AC_PROG_SED
AC_PROG_LN_S
AC_PROG_MKDIR_P
AC_PATH_PROG([STRIP],[strip],[:])
AC_PATH_PROG([GIMPTOOL2],[gimptool-2.0],[:])
AM_CONFIG_HEADER(fourier-config.h)
AC_PROG_INSTALL
AC_PROG_MAKE_SET

#--------------------------------------------------------------------------
# Enable silent build rules by default, this requires atleast Automake-1.11
# Disable by passing --disable-silent-rules to configure or using make V=1
m4_ifdef([AM_SILENT_RULES],[AM_SILENT_RULES([yes])],[AC_SUBST([AM_DEFAULT_VERBOSITY],[1])])

#--------------------------------------------------------------------------
# The following is for benefit of links using paths relative to top_srcdir.
CPPFLAGS="${CPPFLAGS} AS_ESCAPE([-I${top_builddir}]) AS_ESCAPE([-I${top_srcdir}])"

#--------------------------------------------------------------------------
# Check for libraries.
# NOTE: Some distros don't have /usr/local included in the /etc/ld.so PATH,
# so, PKG_CHECK_MODULES may not find libraries you compile into /usr/local,
# therefore use AC_SEARCH_LIBS, AC_CHECK_FUNC for backup.
#
# NOTE: some older scripts have defective SEARCH, so we do CHECK if failed.

# Check for math.h include and math library (some OSes have -lm built-in).
have_libm=maybe
AC_CHECK_HEADER([math.h],
  AC_SEARCH_LIBS([cos],[m],[have_libm=yes],
    AC_CHECK_FUNC([cos],[have_libm=yes])))
if test x"${have_libm}" != xyes; then
    AC_MSG_FAILURE([ERROR: Please install the Math library and math.h],[1])
fi

# Check for package fftw, else fftw3.h include file and fftw3 library. GPL
have_libfftw=maybe
PKG_CHECK_MODULES([FFTW],[fftw3 >= 3.0],[have_libfftw=yes],[have_libfftw=no])
if test x"${have_libfftw}" != xyes; then
  AC_CHECK_HEADER([fftw3.h],
    AC_SEARCH_LIBS([fftw_plan_dft_r2c_2d],[fftw3],[have_libfftw=yes],
      AC_CHECK_FUNC([fftw_plan_dft_r2c_2d],[have_libfftw=yes])))
fi
if test x"${have_libfftw}" != xyes; then
   AC_MSG_FAILURE([ERROR: Please install the developer version of fftw3 library.],[1])
fi
AC_SUBST(FFTW_CFLAGS)
AC_SUBST(FFTW_LIBS)

# Check for libgimp/gimp.h include file and libgimp library. LGPL
GIMP2_CFLAGS=
GIMP2_LIBS=
gimp2_bindir=
have_libgimp=no
PKG_CHECK_MODULES([GIMP2],[gimp-2.0 >= 2.10.0 gimpui-2.0 >= 2.10.0],[have_libgimp=yes])
if test x"${have_libgimp}" != xyes; then
    AC_MSG_FAILURE([ERROR: Please install the developer version of libgimp2.],[1])
fi
AC_SUBST(GIMP2_CFLAGS)
AC_SUBST(GIMP2_LIBS)

# Pass GIMP_LIBDIR to automake for default GIMP plug-ins directory
gimp2_gimplibdir=`${PKG_CONFIG} --variable=gimplibdir gimp-2.0`
gimp2_prefix=`${PKG_CONFIG} --variable=exec_prefix gimp-2.0`
gimp2_relative=${gimp2_gimplibdir#$gimp2_prefix}
GIMP2_BINDIR=\${exec_prefix}"$gimp2_relative"
AC_SUBST(GIMP2_BINDIR)

AM_CONDITIONAL([HAVEGIMPTOOL2],[test "${GIMPTOOL2}"x != x])

# Fetch necessary flags for building gimp2 version of gimp-plugin-fourier
GTK2_CFLAGS=
GTK2_LIBS=
have_libgtk=no
PKG_CHECK_MODULES([GTK2],[gtk+-2.0],[have_libgtk=yes])
if test x"${have_libgtk}" != xyes; then
    AC_MSG_FAILURE([ERROR: Please install the developer version of libgtk+2.],[1])
fi
AC_SUBST(GTK2_CFLAGS)
AC_SUBST(GTK2_LIBS)

case "$build_os" in
  cygwin*|mingw32*|mingw64*)	BUILD_EXEEXT=.exe ;;
esac

#--------------------------------------------------------------------------
# Pass variables to fourier-config.h
AC_DEFINE([FOURIER_MAJOR_VERSION],["fourier_major_version"],[gimp-fourier-plugin major version])
AC_DEFINE([FOURIER_MINOR_VERSION],["fourier_minor_version"],[gimp-fourier-plugin minor version])

#--------------------------------------------------------------------------
# Pass variables to several MAKEFILE.AM
AC_SUBST([FOURIER_MAJOR_VERSION],[fourier_major_version])
AC_SUBST([FOURIER_MINOR_VERSION],[fourier_minor_version])
AC_SUBST([FOURIER_VERSION],[fourier_version])
AC_SUBST([CPPFLAGS],["$CPPFLAGS"])
AC_SUBST([HOST],["$host"])

#--------------------------------------------------------------------------
# Put ifndef wrapper on fourier-config.h so we don't call it repeatedly.
AH_TOP([#ifndef FOURIER_CONFIG_H
#define FOURIER_CONFIG_H 1])
AH_BOTTOM([

#endif])

#--------------------------------------------------------------------------
AC_CONFIG_FILES([
Makefile
rpm/gimp-fourier-plugin.spec
])
AC_OUTPUT
AC_MSG_NOTICE([

Configuration:

  Source code location	${srcdir}
  Build code location	${builddir}
  fourier bindir	${GIMP2_BINDIR}
  Compiler		${CC}
  cppflag		${CPPFLAGS}
  GIMP2_CFLAGS		${GIMP2_CFLAGS}
  GIMP2_LIBS		${GIMP2_LIBS}
  GTK2_CFLAGS		${GTK2_CFLAGS}
  GTK2_LIBS		${GTK2_LIBS}
  FFTW_CFLAGS		${FFTW_CFLAGS}
  FFTW_LIBS		${FFTW_LIBS}
  CFLAGS		${CFLAGS}
  LIBS			${LIBS}
])
