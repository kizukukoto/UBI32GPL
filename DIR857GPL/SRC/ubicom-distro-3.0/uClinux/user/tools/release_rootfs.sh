#!/bin/sh

KERNELPATH=$2
ENV_PATH=$3
FS_PATH=$4
PROJECTS_PATH=$5
PROJECTS_IMAGE_PATH=$PROJECTS_PATH/image
#reduce 64K to avoid write error in rootfs flash block
BS=$(($6-64)) 
ROOTFS_SIZE=$(($BS*1024))
if [ -z $KERNELPATH ]; then
echo "KERNELPATH is not defined! "
exit 1
fi

PLATFORM=`grep AP_TYPE= $ENV_PATH/.config | cut -f 2 -d '='`
echo " Using mksquashfs :"
echo " $PLATFORM"
	$FS_PATH/../setup_rootdir
	rm -r -f /$PROJECTS_IMAGE_PATH/squash.bin
	rm -r -f /$PROJECTS_IMAGE_PATH/squash
	$PROJECTS_PATH/tools/mksquashfs $FS_PATH $PROJECTS_IMAGE_PATH/squash -be
	chmod 644 $PROJECTS_IMAGE_PATH/squash
	dd if=$PROJECTS_IMAGE_PATH/squash of=$PROJECTS_IMAGE_PATH/squash.bin bs=$ROOTFS_SIZE count=1 conv=sync
