#!/bin/sh -x
#
# $3 == image path
# $2 == kernel tree path
# $1 == projects path
MKIMAGE=$1/tools/mkimage
VMLINUX=$2/vmlinux
VMLINUXBIN=$2/arch/mips/boot/vmlinux.bin
LDADDR=0x80002000
IMAGEDIR=$3/vmlinux.lzma

ENTRY=`readelf -a ${VMLINUX}|grep "Entry"|cut -d":" -f 2`

${MKIMAGE} -A mips -O linux -T kernel -C lzma \
        -a ${LDADDR} -e ${ENTRY} -n "Linux Kernel Image"    \
                -d ${IMAGEDIR} $3/vmlinux.lzma.ub
