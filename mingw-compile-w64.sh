#  To be used with build packages from aljacom  http://www.aljacom.com/~gimp/divers.html

/mingw/bin/x86_64-w64-mingw32-g++.exe -o fourier.exe fourier.c `gimptool-2.0.exe --cflags` -I/mingw/include -I/mingw/x86_64-w64-mingw32/include -pipe -O3 -mms-bitfields -s -m64 -W -mtune=generic `gimptool-2.0.exe --libs` -L/mingw/lib -L/mingw/x86_64-w64-mingw32/lib -mwindows -lpthread -lm -lfftw3 -lpng16 -lz -ltiff -ljpeg
