# Makefile.am - Top level automakefile for fourier

SUBDIRS = .

# The braces around ACLOCAL_FLAGS below instead of parentheses are intentional!
# Otherwise autoreconf misparses the line.
ACLOCAL_AMFLAGS=-I m4 ${ACLOCAL_FLAGS}
AM_CFLAGS = ${CFLAGS} ${CPPFLAGS} ${FFTW_CFLAGS}

bin2_PROGRAMS =
bin2dir =
fourier_SOURCES =
fourier_LDADD =
fourier_CFLAGS =
if MAKEGIMP2
bin2_PROGRAMS += fourier
bin2dir += $(GIMP2_BINDIR)/plug-ins
fourier_SOURCES += fourier.c
fourier.$(OBJEXT): fourier-config.h
fourier_LDADD += ${LIBS} ${GIMP2_LIBS} ${GTK2_LIBS} ${FFTW_LIBS}
fourier_CFLAGS += ${GIMP2_CFLAGS} ${GTK2_CFLAGS} -DMAKE_FOR_GIMP3=0
endif

bin3_PROGRAMS =
bin3dir =
fourier3_SOURCES =
fourier3_LDADD =
fourier3_CFLAGS =
if MAKEGIMP3
SUBDIRS += po
bin3_PROGRAMS += fourier3
# 'make distcheck' can't write to ${GIMP3_LIBDIR}/plug-ins/fourier because of
# directory permissions, therefore don't install fourier3 unless you are root
# bin3dir += $(GIMP3_BINDIR)/plug-ins/fourier
@SNIPPET1@
fourier3_SOURCES += fourier.c
fourier3.$(OBJEXT): fourier-config.h
fourier3_LDADD += ${LIBS} ${GIMP3_LIBS} ${GTK3_LIBS} ${FFTW_LIBS}
fourier3_CFLAGS += ${GIMP3_CFLAGS} ${GTK3_CFLAGS} -DMAKE_FOR_GIMP3=1
endif

# Avoid using this line below (Dirs gimp-plugin-fourier vs gimp-fourier-plugin).
#doc_DATA = README.md README.Moire

EXTRA_DIST = LICENSE Makefile.gimptool		\
	bootstrap.sh README.md README.Moire	\
	debian/changelog debian/compat		\
	debian/control debian/copyright		\
	debian/fourier-docs.docs debian/rules	\
	debian/source/format			\
	rpm/gimp-fourier-plugin.spec.in		\
	rpm/gimp3-fourier-plugin.spec.in
nodist_EXTRA_DATA = .git .github
DISTCHECK_CONFIGURE_FLAGS = --disable-silent-rules --enable-silent-rules --enable-fourier_dialog

strip:
	if MAKEGIMP2; \
	then $(STRIP) ${builddir}/fourier; \
	fi; \
	if MAKEGIMP3; \
	then $(STRIP) ${builddir}/fourier3; \
	fi

# These use gimptool to install/uninstall fourier in user directory
install-user:
	if MAKEGIMP2; \
	then $(GIMPTOOL2) --install-bin ${builddir}/fourier; \
	fi; \
	if MAKEGIMP3; \
	then $(GIMPTOOL3) --install-bin ${builddir}/fourier3; \
	fi

uninstall-user:
	if MAKEGIMP2; \
	then $(GIMPTOOL2) --uninstall-bin fourier; \
	fi; \
	if MAKEGIMP3; \
	then $(GIMPTOOL3) --uninstall-bin fourier3; \
	fi

deb:
	dpkg-buildpackage -b -rfakeroot -us -uc
	dpkg-buildpackage -rfakeroot -Tclean

