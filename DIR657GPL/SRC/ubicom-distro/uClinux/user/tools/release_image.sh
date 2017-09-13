#!/bin/sh

ENV_PATH=$2
PROJECTS_IMAGE_PATH=$3
HARDWARE_ID=$4

PLATFORM=`grep AP_TYPE= $ENV_PATH/.config | cut -f 2 -d '='`

echo $PLATFORM 
cd $PROJECTS_IMAGE_PATH
dd if=vmlinux.lzma.ub of=$1.bin bs=1k 
dd if=squash.bin of=$1.bin bs=1k seek=$5
echo -n $HARDWARE_ID >> $1.bin
#rm squash*
