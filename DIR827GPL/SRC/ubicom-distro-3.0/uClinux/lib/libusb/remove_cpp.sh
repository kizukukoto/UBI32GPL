#!/bin/sh
###################################################################################
#	We don't need the cpp library. So we remove the the building target in Makefile
#	And in IP7k environment
#	if we build cpp library, the library pthread link is missing when compile 
#
#	Removed Targets in Makefile:
#		- usbpp.cpp
#		- usbpp.h
#		- tests/
#		- docs/
###################################################################################
SED="/bin/sed"
TARGET_PATH=$1
TARGET_TEST_PATH="$1/tests"
SOURCE_MAKE=Makefile
NEW_MAKE=Makefile.nocpp

if [ -z $TARGET_PATH ] ;then
	echo "Target directory is not specified !!"
	exit 0;
fi

echo "Removing the cpp building target in $TARGET_PATH/Makefile";
cd $TARGET_PATH
$SED 's/usbpp.cpp//' $SOURCE_MAKE | $SED 's/usbpp.h//' \
	| $SED 's/^lib_LTLIBRARIES \= libusb.la.*$/lib_LTLIBRARIES \= libusb.la/' \
	| $SED 's/^SUBDIRS \= .*$/SUBDIRS \= /' > $NEW_MAKE ;
mv -f $NEW_MAKE $SOURCE_MAKE ;

