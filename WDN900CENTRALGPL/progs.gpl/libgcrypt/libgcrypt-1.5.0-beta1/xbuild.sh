#!/bin/sh

if [ ! -e $PWD/../libgpg-error-1.10/xinst/lib/libgpg-error.so ]; then
	echo ""
	echo "*ERROR*: You need to build libgpg-error first."
	echo ""
	echo "\$ cd ../libgpg-error-1.10/"
	echo "\$ sh xbuild.sh"
	echo ""
	exit 1
fi

./configure --prefix=$PWD/xinst --host=arm-linux --disable-padlock-support --disable-dependency-tracking --with-pic --enable-noexecstack --disable-O-flag-munging --with-gpg-error-prefix=$PWD/../libgpg-error-1.10/xinst

make
make install
