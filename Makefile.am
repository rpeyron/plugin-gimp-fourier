# Makefile.am - Top level automakefile for fourier

SUBDIRS = .

# The braces around ACLOCAL_FLAGS below instead of parentheses are intentional!
# Otherwise autoreconf misparses the line.
ACLOCAL_AMFLAGS=-I m4 ${ACLOCAL_FLAGS}
AM_CFLAGS = ${CFLAGS} ${CPPFLAGS} ${FFTW_CFLAGS}

fourier_SOURCES = fourier.c
fourier.$(OBJEXT): fourier-config.h
fourier_LDADD = ${LIBS} ${FFTW_LIBS}
fourier_CFLAGS =
GIMPTOOL =

bin2_PROGRAMS =
bin2dir =
if MAKEGIMP2
bin2_PROGRAMS += fourier
bin2dir += $(GIMP2_BINDIR)/plug-ins
fourier_LDADD += ${GIMP2_LIBS} ${GTK2_LIBS}
fourier_CFLAGS += ${GIMP2_CFLAGS} ${GTK2_CFLAGS} -DMAKE_FOR_GIMP3=0
GIMPTOOL += ${GIMPTOOL2}
endif

bin3_PROGRAMS =
bin3dir =
if MAKEGIMP3
SUBDIRS += po
bin3_PROGRAMS += fourier
# 'make distcheck' can't write to ${GIMP3_LIBDIR}/plug-ins/fourier because of
# directory permissions, therefore don't install fourier unless you are root.
# bin3dir += $(GIMP3_BINDIR)/plug-ins/fourier
@SNIPPET1@
fourier_LDADD += ${GIMP3_LIBS} ${GTK3_LIBS}
fourier_CFLAGS += ${GIMP3_CFLAGS} ${GTK3_CFLAGS} -DMAKE_FOR_GIMP3=1
GIMPTOOL += ${GIMPTOOL3}
endif

# Avoid using this line below (Dirs gimp-plugin-fourier vs gimp-fourier-plugin).
#doc_DATA = README.md README.Moire

EXTRA_DIST = LICENSE Makefile.gimptool		\
	bootstrap.sh README.md README.Moire	\
	debian/gimp2/changelog debian/gimp2/control		\
	debian/gimp3/changelog debian/gimp3/control		\
	debian/compat debian/copyright		\
	debian/fourier-docs.docs debian/rules	\
	debian/source/format			\
	rpm/gimp-fourier-plugin.spec.in		\
	rpm/gimp3-fourier-plugin.spec.in
nodist_EXTRA_DATA = .git .github
DISTCHECK_CONFIGURE_FLAGS = --disable-silent-rules --enable-silent-rules --enable-fourier_dialog

strip:
	$(STRIP) ${builddir}/fourier

# These use gimptool to install/uninstall fourier in user directory
install-user:
	$(GIMPTOOL) --install-bin ${builddir}/fourier

uninstall-user:
	$(GIMPTOOL) --uninstall-bin fourier

deb_gimp2:
	cp -f debian/gimp2/* debian
	export DEB_CONFIGURE_EXTRA_FLAGS="" 
	dpkg-buildpackage -b -rfakeroot -us -uc
	dpkg-buildpackage -rfakeroot -Tclean

deb_gimp3:
	cp -f debian/gimp3/* debian
	export DEB_CONFIGURE_EXTRA_FLAGS="--enable-gimp3-fourier" 
	dpkg-buildpackage -b -rfakeroot -us -uc
	dpkg-buildpackage -rfakeroot -Tclean
