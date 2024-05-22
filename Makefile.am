# Makefile.am - Top level automakefile for gimp3-fourier-plugin

SUBDIRS = . po

# The braces around ACLOCAL_FLAGS below instead of parentheses are intentional!
# Otherwise autoreconf misparses the line.
ACLOCAL_AMFLAGS=-I m4 ${ACLOCAL_FLAGS}
AM_CFLAGS = ${CFLAGS} ${CPPFLAGS} ${GTK_CFLAGS} ${PKG_CFLAGS}


bin_PROGRAMS = fourier
# 'make distcheck' can't write to ${GIMP_LIBDIR}/plug-ins/ because of
# directory permissions, therefore don't install fourier unless you are root
@SNIPPET1@

fourier_SOURCES = fourier.c
fourier.$(OBJEXT): fourier-config.h
fourier_LDADD = ${LIBS} ${GTK_LIBS} ${PKG_LIBS}

fourierdocsdir = ${datarootdir}/gimp3-plugin-fourier
fourierdocs_DATA = LICENSE README.md README.Moire

EXTRA_DIST = LICENSE Makefile.gimptool		\
	bootstrap.sh README.md README.Moire	\
	debian/changelog debian/compat		\
	debian/control debian/copyright		\
	debian/fourier-docs.docs debian/rules	\
	debian/source/format			\
	rpm/gimp3-plugin-fourier.spec.in
nodist_EXTRA_DATA = .git .github
DISTCHECK_CONFIGURE_FLAGS = --disable-silent-rules --disable-fourier_dialog

strip:
	$(STRIP) ${builddir}/fourier

# These use gimptool to install/uninstall fourier in user directory
install-user:
	$(GIMPTOOL) --install-bin ${builddir}/fourier

uninstall-user:
	$(GIMPTOOL) --uninstall-bin fourier

deb:
	dpkg-buildpackage -b -rfakeroot -us -uc
	dpkg-buildpackage -rfakeroot -Tclean

