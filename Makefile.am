# Makefile.am - Top level automakefile for fourier

# The braces around ACLOCAL_FLAGS below instead of parentheses are intentional!
# Otherwise autoreconf misparses the line.
ACLOCAL_AMFLAGS=-I m4 ${ACLOCAL_FLAGS}
AM_CFLAGS = ${CFLAGS} ${CPPFLAGS} ${GIMP2_CFLAGS} ${GTK2_CFLAGS} ${FFTW_CFLAGS}


bin2_PROGRAMS = fourier
bin2dir = $(GIMP2_BINDIR)/plug-ins

fourier_SOURCES = fourier.c
fourier.$(OBJEXT): fourier-config.h
fourier_LDADD = ${LIBS} ${GIMP2_LIBS} ${GTK2_LIBS} ${FFTW_LIBS}

#avoid line below due to dirs gimp-plugin-fourier vs gimp-fourier-plugin
#doc_DATA = README.md README.Moire

EXTRA_DIST = LICENSE Makefile.gimptool		\
	bootstrap.sh README.md README.Moire	\
	debian/changelog debian/compat		\
	debian/control debian/copyright		\
	debian/fourier-docs.docs debian/rules	\
	debian/source/format			\
	rpm/gimp-fourier-plugin.spec.in
nodist_EXTRA_DATA = .git .github
DISTCHECK_CONFIGURE_FLAGS = --disable-silent-rules

strip:
	$(STRIP) ${builddir}/fourier

# These use gimptool to install/uninstall fourier in user directory
install-user:
	$(GIMPTOOL2) --install-bin ${builddir}/fourier

uninstall-user:
	$(GIMPTOOL2) --uninstall-bin fourier

deb:
	dpkg-buildpackage -b -rfakeroot -us -uc
	dpkg-buildpackage -rfakeroot -Tclean


# NOTE: To install/uninstall fourier in the correct directory we need to use:
# ./configure --bindir=/usr/lib/gimp/2.0/plug-ins/fourier
# make
# make strip
# sudo make install
