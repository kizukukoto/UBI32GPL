#!/bin/bash

SUM=0;
for i in `ubicom32-elf-objdump -h $1/*ko | grep ocm_tex | awk '{print $3}' `; do SUM=$(($SUM+0x$i)); done; 
printf "%x - %d\n" $SUM $SUM
