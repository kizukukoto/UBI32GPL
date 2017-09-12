#!/bin/sh

./configure --prefix=$PWD/xinst --host=arm-linux --disable-nls --disable-languages

make
make install
