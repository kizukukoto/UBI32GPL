#!/bin/bash
########################################################################
# Add by Yufa, 2009/08/11
#    1. The HARDWARE_COUNTRYID and HARDWARE_ID must equal to bsp.h
#    2. Append HARDWARE_COUNTRYID, HARDWARE_ID to binary image.
########################################################################

HARDWARE_ID='MB97-AR9380-RT-110103-00'
HARDWARE_COUNTRYID='00'

cp ./bin/bootexec_bd.bin+u-boot.ub ./bin/DIR-827_u-boot.bin
echo -n $HARDWARE_COUNTRYID$HARDWARE_ID >> $1./bin/DIR-827_u-boot.bin
cp ./bin/upgrade.ub ./bin/DIR-827_upgrade.bin
echo -n $HARDWARE_COUNTRYID$HARDWARE_ID >> $1./bin/DIR-827_upgrade.bin
