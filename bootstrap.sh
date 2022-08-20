# From https://github.com/rpeyron/plugin-gimp-fourier/pull/2 - DO NOT AUTOMATE

wget -O config.guess 'https://git.savannah.gnu.org/gitweb/?p=config.git;a=blob_plain;f=config.guess;hb=HEAD'
wget -O config.sub 'https://git.savannah.gnu.org/gitweb/?p=config.git;a=blob_plain;f=config.sub;hb=HEAD'
autoreconf` -i
automake
