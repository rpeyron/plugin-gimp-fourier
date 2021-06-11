[![CI](https://github.com/rpeyron/plugin-gimp-fourier/actions/workflows/main.yml/badge.svg)](https://github.com/rpeyron/plugin-gimp-fourier/actions/workflows/main.yml)

# plugin-gimp-fourier

Plugin GIMP : Fourier Transform

## What it does

It does a direct and reverse Fourier Transform.
It allows you to work in the frequency domain.
For instance, it can be used to remove moiré patterns from images scanned from books. (See README.Moire)

## Use 

It adds 2 items in the filters menu:
*  Filters/Generic/Foward FFT
*  Filters/Generic/Inverse FFT

![image](https://user-images.githubusercontent.com/3126751/121738126-19e4ec80-cafa-11eb-9fec-ad923d853cde.png)


## Installation

### Linux

You will need the fftw3 package, and the development packages of gimp, fftw3, and glib  
For instance, on debian/ubuntu : `sudo apt-get install libfftw3-dev libgimp2.0-dev`

Then:
```
make
make install
```

### Windows
 
Binaries for windows are provided as separate packages. Please download the 32bits or 64bits according to you GIMP version (this is not related to Windows version). Altough the GIMP API is quite stable, the binaries are not, and the plugin binaries must be updated to new GIMP versions (some will work, some won't). The GIMP version is indicated in the package filename. Download the binaries that fits the best to your GIMP version. Just copy the files (fourier.exe and libfftw3-3.dll) in the plugins directory of eiher:
- your personal gimp directory (ex: .gimp-2.2\plug-ins), 
- or in the global directory (C:\Program Files\GIMP-2.2\lib\gimp\2.0\plug-ins)

You can also build with msys2 environment:
```
msys2 -c "pacman -S --noconfirm mingw-w64-i686-toolchain"
msys2 -c "pacman -S --noconfirm mingw-w64-i686-gimp=2.10.24"
msys2 -c "pacman -S --noconfirm mingw-w64-i686-fftw"
msys2 -mingw32 -c 'echo $(gimptool-2.0 -n --build fourier.c) -lfftw3 -O3 | sh'
```
This is for 32bits version ; replace i686 by x84_64 and -mingw32 byt -mingw64 if you want 64bits. Replace also 2.10.24 by your GIMP version (or leave empty for latest version)

Also, the windows binaries are build through GitHub Actions. So you may also fork this repository and build the plugin on your own.

## History
```
 v0.1.1 : First release of this plugin
 v0.1.2 : BugFixes by Mogens Kjaer <mk@crc.dk>, May 5, 2002 
 v0.1.3 : Converted to Gimp 2.0 (dirty conversion)
 v0.2.0 : Many improvements from Mogens Kjaer <mk@crc.dk>, Mar 16, 2005
              * Moved to gimp-2.2
              * Handles RGB and grayscale images
              * Scale factors stored as parasite information
              * Columns are swapped
 v0.3.0 : Great Improvement from Alex Fernández with dynamic boosting :
              * Dynamic boosted normalization : 
                    fft/inverse loss of quality is now un-noticeable 
              * Removed the need of parasite information
 v0.3.1 : Zero initialize padding (patch provided by Rene Rebe)
 v0.3.2 : GPL, Fixed Makefile with pkg-config
 v0.4.0 : Patch by Edgar Bonet :
             * Reordered the data in a more natural way
             * No Fourier coefficient is lost
 v0.4.1 : Select Gray after transform + doc (patch by Martin Ramshaw)
 v0.4.2 : Makefile patch by Bob Barry (gcc argument order)
 v0.4.3 : Makefile patch by bluedxca93 (-lm argument for ubuntu 13.04)
```

Many thanks to Mogens Kjaer, Alex Fernández, Rene Rebe, Edgar Bonet,
Martin Ramshaw, Bob Barry and bluedxca93 for their contributions.
