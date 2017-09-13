#
# GCC Library
#
CC := ubicom32-elf-gcc -march=ubicom32v3
LD := ubicom32-elf-ld
GCCLIBDIR := $(dir $(shell $(CC) $(CFLAGS) -print-libgcc-file-name))
UCLIBCDIR := /scratch/natg/uClinux/uClibc/lib
UCLIBC_INCLUDE_DIR := $(UCLIBCDIR)/../include
LINUX_INCLUDE_DIR := /scratch/natg/uClinux/linux-2.6.x/include
LDFLAGS += -elf2flt -g -L$(UCLIBCDIR) -L$(GCCLIBDIR)
CFLAGS +=  -g \
	-Wall \
	-I$(UCLIBC_INCLUDE_DIR) \
	-I$(LINUX_INCLUDE_DIR)

%.o : %.c
	$(CC) -c $(CFLAGS) $< -o $@

all: readwrite
	cp readwrite ../../linux-2.6.x/arch/ubicom32/initramfs/init

clean:
	rm -f readwrite
	rm -f readwrite.o
	rm -f hack.o
	rm -f *.gdb *~

readwrite: readwrite.o hack.o
	$(LD) $(LDFLAGS) -o $@ $^ $(UCLIBCDIR)/crt1.o  -lc -lgcc

