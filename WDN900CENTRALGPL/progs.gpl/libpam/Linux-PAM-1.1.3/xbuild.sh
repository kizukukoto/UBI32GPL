#!/bin/sh

./configure --build=i386-linux --host=arm-none-linux-gnueabi --prefix=$PWD/_install --includedir=$PWD/_install/include/security/ build_alias=i386-linux host_alias=arm-none-linux-gnueabi CC=arm-none-linux-gnueabi-gcc CFLAGS="-g -O1"

make
make install
