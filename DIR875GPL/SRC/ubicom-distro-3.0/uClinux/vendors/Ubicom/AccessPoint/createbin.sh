#!/bin/bash
########################################################################
# Add by Yufa, 2009/08/11
#    1. The HARDWARE_COUNTRYID and HARDWARE_ID must equal to uClinux/user/libplatform/bsp.h
#    2. Append HARDWARE_COUNTRYID, HARDWARE_ID to binary image.
########################################################################

HARDWARE_ID='AR94-AR9223-RT-090811-00'
HARDWARE_COUNTRYID='00'

cp ./bin/upgrade.ub ./bin/upgrade.bin
echo -n $HARDWARE_COUNTRYID$HARDWARE_ID >> $1./bin/upgrade.bin
