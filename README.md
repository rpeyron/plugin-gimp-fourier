[![CI](https://github.com/rpeyron/plugin-gimp-fourier/actions/workflows/main.yml/badge.svg)](https://github.com/rpeyron/plugin-gimp-fourier/actions/workflows/main.yml)
[![Packaging status](https://repology.org/badge/tiny-repos/gimp:fourier.svg)](https://repology.org/project/gimp:fourier/versions)

# plugin-gimp-fourier

Fourier plugin for GIMP

[Use](#use) | [Install on Windows](#windows) | [Install on Linux](#linux) | [Install from source](#from-source) | [Maintainers instructions](#maintainers) | [History & Thanks](#history) 

## What it does

It does a direct and reverse Fourier Transform.
It allows you to work in the frequency domain.
For instance, it can be used to remove moiré patterns from images scanned from books. (See [README.Moire](README.Moire))

## Use

It adds 2 items in the filters menu:
*  Filters/Generic/FFT Forward
*  Filters/Generic/FFT Inverse

![image](https://user-images.githubusercontent.com/3126751/121738126-19e4ec80-cafa-11eb-9fec-ad923d853cde.png)


## Installation

### Windows

Binaries for windows are provided as separate packages. Please download the 32bits or 64bits according to you GIMP version
(this is not related to Windows version). Altough the GIMP API is quite stable, the binaries are not, and the plugin binaries
must be updated to new GIMP versions (some will work, some won't). The GIMP version is indicated in the package filename.
Download the binaries that fits the best to your GIMP version. Just copy the fourier folder (containing fourier.exe and libfftw3-3.dll) 
in the plugins directory of either:
- your personal gimp directory (ex: .gimp-2.2\plug-ins),
- or in the global directory (C:\Program Files\GIMP-2.2\lib\gimp\2.0\plug-ins)

You can also build with msys2 environment:
```
msys2 -c "pacman -Suy"
msys2 -c "pacman -S --noconfirm mingw-w64-x86_64-toolchain"
msys2 -c "pacman -S --noconfirm mingw-w64-x86_64-gimp=2.10.36"
msys2 -c "pacman -S --noconfirm mingw-w64-x86_64-fftw"
msys2 -mingw64 -c 'echo $(gimptool-2.0 -n --build fourier.c) -lfftw3 -O3 | sh'
msys2 -mingw64 -c 'cp `which libfftw3-3.dll` .'
msys2 -c "pacman -Scc"
```

This is for 32bits version ; replace x86_64 by i686 and -mingw64 by -mingw32 if you want 32bits.
Replace also 2.10.36 by your GIMP version (or leave empty for latest version)

Also, the windows binaries are built through GitHub Actions, so you may also fork this repository and build the plugin on your own.

### Linux

#### Repositories and pre-built packages

- Fedora repository: `sudo yum install gimp-fourier-plugin` (by the Fedora community)
- Debian/Ubuntu pre-built package: download the deb file and install with `sudo dpkg -i gimp-plugin-fourier_0.4.5-1_amd64.deb`
- and other distributions like openSUSE, slack, ArchLinux, Enterprise Linux, Guix and NixOS by experimental packages by their communities (see [repology list](https://repology.org/project/gimp:fourier/versions)).


#### From source

You will need the fftw3 package, and the development packages of gimp, fftw3, and glib
For instance, on debian/ubuntu : `sudo apt-get install libfftw3-dev libgimp2.0-dev`

Then if you cloned this repo, starts with the commands below.
If you downloaded the tar package, you may skip this step and go to the second one.
```sh
autoreconf -i  (or use 'autoreconf --install --force' for more modern setups)
automake --foreign -Wall
```

And then:
```sh
./configure
make
make strip
sudo make install
```

If you have non-standard GIMP plug-ins directory, you may have to add `--bindir=/usr/lib/gimp/2.0/plug-ins` to the configure command (replace by your plug-ins path)

## GIMP3

A simple port have been made. It does not currently use the new features of GIMP3.
I am waiting for the GIMP3 plugin developer documentation (not available yet), to see if a rewrite 
with new standards and features will be useful or not.

The plugin is unified can now be compiled for both GIMP2 or GIMP3 
(the gimptool maybe named differently depending on your distribution). 
There are draft versions with seperate plugins or with includes in the git history.
The GIMP3 part have been adapted from the `hot.c` bundled plugin

Below are some details of the changes I had to do for GIMP3:
* Missing headers in mingw packages to download from GIMP repo (note: )
  - `libgimpbase/gimpchoice.h` [gimp/#10900](https://gitlab.gnome.org/GNOME/gimp/-/issues/11454,https://gitlab.gnome.org/GNOME/gimp/-/issues/10900)
  - `libgimp/stdplugins-intl.h`
  - `libgimpwidgets/gimplabelstringwidget.h`
* To build hot.c, replaced missing `config.h`  with  `#define GETTEXT_PACKAGE "glib"` 
* Glib-2.0: there is major changes in glib2 [2.79.1](https://gitlab.gnome.org/GNOME/glib/blob/2.79.1/NEWS) (!3826 genums: use g_once_init_enter_pointer for GType initializers) that makes binaries incompatible ; Gimp 2.99.18 is shipped with 2.78 and mingw64 has 2.80 at the time I write this, so you will need to copy `libglib-2.0-0.dll` in same folder as the plugin fix the problem
* plugin must be in a folder, and plugin exe must have the same name as the folder

To install GIMP3 dev packages on mingw64:
- Use package `mingw-w64-x86_64-gimp3` instead of `mingw-w64-x86_64-gimp3` ; you will need to uninstall GIMP2 dev packages before as there is some file conflicts: `msys2 -c "pacman -R --noconfirm mingw-w64-x86_64-gimp && pacman -S --noconfirm mingw-w64-x86_64-gimp3"` 
- To switch back to GIMP2 dev packages: `msys2 -c "pacman -R --noconfirm mingw-w64-x86_64-gimp3 && pacman -S --noconfirm mingw-w64-x86_64-gimp"` 

### With configure

The configure script has been made compatible to build both gimp2 and gimp3 version. For now, as GIMP3 has not been released, the default is to build GIMP2 plugin, even on
the gimp2.99 branch. To switch tobuild the GIMP3 plugin with configure, use the option `--enable-gimp3-fourier`:
```
./configure --enable-gimp3-fourier
make
make strip
sudo make install
```


## Maintainers

To create a distributable gimp-plugin-fourier-{version}.tar.gz file, you  will need to do these steps:
First, update the MAJOR.MINOR version in configure.ac, and then:

```
$  wget -O config.guess 'https://git.savannah.gnu.org/gitweb/?p=config.git;a=blob_plain;f=config.guess;hb=HEAD'
$  wget -O config.sub 'https://git.savannah.gnu.org/gitweb/?p=config.git;a=blob_plain;f=config.sub;hb=HEAD'
$ autoreconf -i
$ automake --foreign -Wall
$ ./configure
$ make dist
$ ls -l
```
You should see a tar file named gimp-fourier-plugin-0.4.4.tar.gz in the same directory.
To verify that the dist package contains all files and nothing is missing, test build it....
```
$ tar -xzf gimp-fourier-plugin-0.4.4.tar.gz
$ cd gimp-fourier-plugin-0.4.4
$ ./configure --bindir=/usr/lib/gimp/2.0/plug-ins
$ make
$ sudo make install
```
If no errors, then copy gimp-fourier-plugin-0.4.4.tar.gz to your release webpage.
NOTE: rpm spec file Source0 URL links to this file.

## Debug

* Build & install `make clean && make && make install-user`
* Run Gimp `GIMP_PLUGIN_DEBUG=fourier,run gimp`
* Run plugin
* Attach fourier process to gdb (in vscode with debug gdb)

Note: optimization removes some variables and add some difficulties to debug, but I did not manage to get the plugin to compille with -O0 (getting link errors with local functions...)

## History

```
*  (May 2024): first version of GIMP3 compatibility (iso)
*  v0.4.5 (Mar 2024): fix selection overflow ([#6](https://github.com/rpeyron/plugin-gimp-fourier/issues/6))
*  v0.4.4 (Aug 2022):
    - Replaced deprecated functions
    - Autotools toolchain and initial_rpm.spec file by Joe Da Silva
    - Github action workflow to build gimp-fourier-plugin
*  v0.4.3 (Apr 2014); Makefile patch by bluedxca93 (-lm arg for ubuntu 13.04)
*  v0.4.2 (Feb 2012); Makefile patch by Bob Barry (gcc arg order)
*  v0.4.1 (Jan 2010): Patch by Martin Ramshaw
    - Select Gray after transform + doc
*  v0.4.0 (Oct 2009): Patch by Edgar Bonet
    -  No Fourier coefficient is lost
    -  Reordered the data in a more natural way
*  v0.3.2 (Feb 2009):
    - Officialized distribution under GPL
    - Fixed Makefile by using pkg-config instead of gimptool
*  v0.3.1 (Dec 2007):
   - Zero initialize padding by Rene Rebe
   - Windows compatibility, inverse remove parasite, cosmetics (Mar 2005)
*  v0.3.0 (Aug 2005): dynamic boosting from Alex Fernández
   - Dynamic boosted normalization : loss of quality is now un-noticeable
   - Removed the need of parasite information
*  v0.2.0 (Mar 2005): Many improvements from Mogens Kjaer
    - Moved to gimp-2.2
    - Handles RGB and grayscale images
    - Scale factors stored as parasite information
    - Columns are swapped
* v0.1.3 (Oct 2004): Moved to gimp-2.0 (Linux only)
* v0.1.2 (May 2002): Minor modifications by Mogens Kjaer
* v0.1.1 (Feb 2022): First release of this plugin

```

Many thanks to Mogens Kjaer, Alex Fernández, Rene Rebe, Edgar Bonet,
Martin Ramshaw, Bob Barry, bluedxca93 and Joe Da Silva for their contributions.

French readers may also interested by [this article](https://www.lprp.fr/2002/02/fourier/) that describes
the way the plugin works (even it is a little outdated as a GIMP parasite is used to store the scale
factor instead of the former 'magic pixel')
