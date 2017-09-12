#!/bin/sh

if [ ! -e ${TOOLCHAIN_FS}/lib/libfl.a ]; then
	echo ""
	echo "*ERROR*: You need to build flex 2.5.4 first."
	echo ""
	echo "\$ cd ../flex-2.5.4"
	echo "\$ sh build.sh"
	echo ""
	exit 1
fi

rm -rvf config.cache

./configure --host=${TARGET_HOST} --prefix=${TOOLCHAIN_FS} --includedir=${TOOLCHAIN_FS}/include/security/ host_alias=${TARGET_HOST} --disable-selinux --disable-regenerate-docu CC=${CC} CFLAGS="-g -O1"

make clean
make
make install
