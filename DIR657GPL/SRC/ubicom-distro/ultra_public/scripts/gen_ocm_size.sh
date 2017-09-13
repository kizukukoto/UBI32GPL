#!/bin/sh

# this might be called with no args, in which case the constants will be 0

if [ $1 ]
then
	echo -n \#define APP_OCM_CODE_SIZE \(
	ubicom32-elf-objdump -t $1 | grep -e ' __ocm_text_run_end$' | { read value rest; echo -n 0x$value; }
	echo -n \-
	ubicom32-elf-objdump -t $1 | grep -e ' __ocm_begin$' | { read value rest; echo -n 0x$value; }
	echo \)
	echo -n \#define APP_OCM_DATA_SIZE \(
	ubicom32-elf-objdump -t $1 | grep -e ' __ocm_end$' | { read value rest; echo -n 0x$value; }
	echo -n \-
	ubicom32-elf-objdump -t $1 | grep -e ' __data_begin$' | { read value rest; echo -n 0x$value; }
	echo \)
else
	echo \#define APP_OCM_CODE_SIZE 0
	echo \#define APP_OCM_DATA_SIZE 0
fi
