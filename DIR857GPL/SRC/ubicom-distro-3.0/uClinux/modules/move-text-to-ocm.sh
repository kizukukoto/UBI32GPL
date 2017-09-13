#!/bin/bash
#
# This is a simple script that moves top X functions to OCM in module (ELF).
# This will be run by make system during wlan module compilation from source.
#

if [ $# -ne 1 ]; then
	echo "Usage: move-text-to-ocm.sh ocm_text_list.txt"
	exit
fi

for MODULE in `ls *ko`; do

MODULENAME=`echo $MODULE | cut -d. -f1`
FUNCTIONS=`grep "$MODULENAME:" $1 | cut -d: -f2 | cut -d" " -f1`

for FUNC in $FUNCTIONS; do
$ROOTDIR/vendors/config/ubicom32nommu/move-text-to-ocm $MODULE $FUNC
done

# group function sections to .text, .init, .exit and .ocm_text
ubicom32-elf-ld -r -T $ROOTDIR/modules/group_func_sections.lds -o tmp.ko $MODULE
rm $MODULE
mv tmp.ko $MODULE

done
