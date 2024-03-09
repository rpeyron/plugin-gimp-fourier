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

#--------------------------------------------------------------------------
AC_INIT([fourier],[fourier_version],[https://github.com/rpeyron/plugin-gimp-fourier/issues],
	[fourier_package_name],[https://www.lprp.fr/gimp_plugin_en/])
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
AC_PATH_PROG([GIMPTOOL],[gimptool-2.0],[:])
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
# therefore use AC_CHECK_HEADERS, AC_SEARCH_LIBS, AC_CHECK_FUNC for backup.
#
# NOTE: some older scripts have defective SEARCH, so we do CHECK if failed.

# Check for math.h include and math library (some OSes have -lm built-in).
have_libm=maybe
AC_CHECK_HEADER([math.h],
  AC_SEARCH_LIBS([cos],[m],[have_libm=yes],
    AC_CHECK_LIB([m],[cos],[have_libm=yes],
      AC_CHECK_FUNC([cos],[have_libm=yes]))))
if test x"${have_libm}" != xyes; then
    AC_MSG_FAILURE([ERROR: Please install the Math library and math.h],[1])
fi

# Check for package fftw, else fftw3.h include file and fftw3 library. GPL
have_libfftw=maybe
PKG_CHECK_MODULES([FFTW],[fftw3 >= 3.0],[have_libfftw=yes pkg_cflags="${FFTW_CFLAGS}" pkg_libs="${FFTW_LIBS}"],[have_libfftw=no])
if test x"${have_libfftw}" != xyes; then
  AC_CHECK_HEADER([fftw3.h],
    AC_SEARCH_LIBS([fftw_plan_dft_r2c_2d],[fftw3],[have_libfftw=yes],
      AC_CHECK_LIB([fftw3],[fftw_plan_dft_r2c_2d],[have_libfftw=yes],
        AC_CHECK_FUNC([fftw_plan_dft_r2c_2d],[have_libfftw=yes]))))
fi
if test x"${have_libfftw}" != xyes; then
   AC_MSG_FAILURE([ERROR: Please install the developer version of fftw3 library.],[1])
fi

# Check for libgimp/gimp.h include file and libgimp library. LGPL
have_libgimp=no
PKG_CHECK_MODULES([GIMP],[gimp-2.0],[have_libgimp=yes pkg_cflags="${pkg_cflags} ${GIMP_CFLAGS}" pkg_libs="${pkg_libs} ${GIMP_LIBS}"],[have_libgimp=no])
if test x"${have_libgimp}" != xyes; then
  AC_CHECK_HEADER([gimp.h],
    AC_SEARCH_LIBS([gimp_install_procedure],[gimp],[have_libgimp=yes],
      AC_CHECK_LIB([gimp],[gimp_install_procedure],[have_libgimp=yes],
        AC_CHECK_FUNC([gimp_install_procedure],[have_libgimp=yes]))))
fi
if test x"${have_libgimp}" != xyes; then
    AC_MSG_FAILURE([ERROR: Please install the developer version of libgimp.],[1])
fi

case "$build_os" in
  cygwin*|mingw32*|mingw64*)	BUILD_EXEEXT=.exe ;;
esac

AM_CONDITIONAL([HAVEGIMPTOOL],[test "${GIMPTOOL}"x != x])

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
AC_SUBST([PKG_CFLAGS],["$pkg_cflags"])
AC_SUBST([PKG_LIBS],["$pkg_libs"])
AC_SUBST([HOST],["$host"])

# Pass GIMP_LIBDIR to automake for default GIMP plug-ins directory
GIMP_GIMPLIBDIR=`$PKG_CONFIG --variable=gimplibdir gimp-2.0`
# Seems that gimp does not follow libdir of the distro, so just replacing prefix
# GIMP_LIBDIR=`$PKG_CONFIG --variable=libdir gimp-2.0`
# GIMP_RELATIVE=${GIMP_GIMPLIBDIR#$GIMP_LIBDIR}
# GIMP_BINDIR="$libdir$GIMP_RELATIVE"
GIMP_PREFIX=`$PKG_CONFIG --variable=exec_prefix gimp-2.0`
GIMP_RELATIVE=${GIMP_GIMPLIBDIR#$GIMP_PREFIX}
GIMP_BINDIR=\${exec_prefix}"$GIMP_RELATIVE"
AC_SUBST(GIMP_BINDIR)

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
  fourier bindir	${GIMP_BINDIR}
  docs root dir		${datarootdir}
  Compiler		${CC}
  cppflag		${CPPFLAGS}
  pkg_cflag		${PKG_CFLAGS}
  pkg_libs		${PKG_LIBS}
  CFLAGS		${CFLAGS}
  LIBS			${LIBS}
])
