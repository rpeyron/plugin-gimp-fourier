# Makefile.am - Top level automakefile for fourier

# The braces around ACLOCAL_FLAGS below instead of parentheses are intentional!
# Otherwise autoreconf misparses the line.
ACLOCAL_AMFLAGS=-I m4 ${ACLOCAL_FLAGS}
AM_CFLAGS = ${CFLAGS} ${CPPFLAGS} ${PKG_CFLAGS}


bin_PROGRAMS = fourier
bindir = $(GIMP_BINDIR)/plug-ins

fourier_SOURCES = fourier.c
fourier.$(OBJEXT): fourier-config.h
fourier_LDADD = ${LIBS} ${PKG_LIBS}

fourierdocsdir = ${datarootdir}/gimp-fourier-plugin
fourierdocs_DATA = LICENSE README README.Moire

EXTRA_DIST = LICENSE Makefile.gimptool		\
	README README.md README.Moire	     	\
	packaging/gimp-fourier-plugin.spec.in	\
	packaging/gimp-fourier-plugin.spec
nodist_EXTRA_DATA = .git .github
DISTCHECK_CONFIGURE_FLAGS = --disable-silent-rules

# Simple copy for now to keep the same name as the fedora package
README: README.md
	cp -f README.md README

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


# NOTE: To install/uninstall fourier in the correct directory we need to use:
# ./configure --bindir=/usr/lib/gimp/2.0/plug-ins/fourier
# make
# make strip
# sudo make install