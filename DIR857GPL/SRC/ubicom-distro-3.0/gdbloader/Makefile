UBICOM32_ARCH = ubicom32v5
ifeq ($(ARCH_EXTENSION),)
$(error Must define ARCH_EXTENSION)
endif

ULTRA_DIR?=../ultra

all:	lk.elf loader.bin

loader_kernel_ip5000_entry.o: loader_kernel_ip5000_entry.S config.h ip5000.h $(ULTRA_DIR)/projects/bootexec/config/config.h
	ubicom32-elf-gcc -std=gnu99 -c -march=$(UBICOM32_ARCH) -Wa,-m$(UBICOM32_ARCH) -D$(ARCH_EXTENSION) -D__ASSEMBLY__ loader_kernel_ip5000_entry.S

loader_kernel_ip5000.o: loader_kernel_ip5000.c $(ULTRA_DIR)/projects/bootexec/config/config.h
	ubicom32-elf-gcc -std=gnu99 -g -Os -mfastcall -c -march=$(UBICOM32_ARCH) -D$(ARCH_EXTENSION) -fno-common -fno-builtin -nostdinc loader_kernel_ip5000.c

loader_kernel_ip5000_thread.o: loader_kernel_ip5000_thread.c $(ULTRA_DIR)/projects/bootexec/config/config.h
	ubicom32-elf-gcc -std=gnu99 -g -Os -mfastcall -c -march=$(UBICOM32_ARCH) -D$(ARCH_EXTENSION) -fno-common -fno-builtin -nostdinc loader_kernel_ip5000_thread.c

lk.elf:	loader_kernel_ip5000_entry.o loader_kernel_ip5000.o loader_kernel_ip5000_thread.o $(ULTRA_DIR)/projects/bootexec/config/config.h
	ubicom32-elf-ld --gc-sections -e start_vector -o lk.elf -T loader_kernel_ip5000.ld loader_kernel_ip5000_entry.o loader_kernel_ip5000.o loader_kernel_ip5000_thread.o

loader.bin: lk.elf
	ubicom32-elf-objcopy -O binary -j .text lk.elf loader.bin

clean:
	rm -f *.o *.bin *.elf *~
