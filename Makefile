#  Use gimptool-2.0 to sett these variables
GIMPTOOL=gimptool-2.0
PLUGIN_BUILD=$(GIMPTOOL) --build
PLUGIN_INSTALL=$(GIMPTOOL) --install-bin
GCC=gcc
LIBS=$(shell pkg-config fftw3 gimp-2.0 --libs) -lm
CFLAGS=-O2 $(shell pkg-config fftw3 gimp-2.0 --cflags)
VERSION=0.4.2
DIR=fourier-$(VERSION)

export

FILES= \
    fourier.c \
    Makefile \
    Makefile.win \
    README \
    README.Moire \
    fourier.dev

all: fourier

# Use of pkg-config is the recommended way
fourier: fourier.c
	$(GCC) $(CFLAGS) -o fourier fourier.c $(LIBS)

# To avoid gimptool use, just copy the fourier in the directory you want
install: fourier
	$(PLUGIN_INSTALL) fourier
	 
dist:
	mkdir $(DIR)
	cp $(FILES) $(DIR)
	tar czf "$(DIR).tar.gz" $(DIR)
	rm -Rf $(DIR)
   
clean:
	rm -f fourier
