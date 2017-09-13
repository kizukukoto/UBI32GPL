#
# 
#

UCLINUX_ROOT ?= $(shell cd ../..; pwd)

ifndef ARCH
all: TAGS
	echo	root = $(UCLINUX_ROOT)
	echo Make ubicom32v3 
	$(MAKE) ARCH=ubicom32v3 arch
	echo Make ubicom32v4
	$(MAKE) ARCH=ubicom32v4 arch
	echo Make ubicom32v5
	$(MAKE) ARCH=ubicom32v5 arch

clean:
	@echo Clean
	-rm TAGS -f
	$(MAKE) ARCH=ubicom32v3 clean-arch
	$(MAKE) ARCH=ubicom32v4 clean-arch
	$(MAKE) ARCH=ubicom32v5 clean-arch
else
all: arch
clean: clean-arch
endif

ifeq ($(ARCH),x86)
CC := gcc
CFLAGS :=
OBJDUMP:=
else
#OBJDUMP:=ubicom32-elf-objdump
CC := ubicom32-uclinux-gcc -march=$(ARCH)
GCCLIBDIR := $(dir $(shell $(CC) $(CFLAGS) -print-libgcc-file-name))
UCLIBCDIR := $(UCLINUX_ROOT)/uClibc/lib
UCLIBC_INCLUDE_DIR := $(UCLIBCDIR)/../include
LINUX_INCLUDE_DIR := $(UCLINUX_ROOT)/linux-2.6.x/include
export LANG=C

CFLAGS := -g 								\
	-elf2flt -Wall -L$(UCLIBCDIR)					\
	-I$(UCLINUX_ROOT)/include					\
	-I$(UCLINUX_ROOT)/uClibc/include				\
	-D__ubicom32__ -fexec-charset=ascii				\
	-Os -g -fomit-frame-pointer					\
	-fno-common -fno-builtin -Wall					\
	-Dlinux -D__linux__ -Dunix -D__uClinux__			\
	-D__linux -DEMBED -DNO_FORK

endif

CFLAGS += -D_GNU_SOURCE
BIN:=$(ARCH)
BASENAME:= profiler_helper

PROGRAMS:= $(addprefix $(BIN)/, $(BASENAME))

OBJS= $(addsuffix .o, $(PROGRAMS))

TAGS: Makefile
	echo Make tags
	etags $(UCLINUX_ROOT)/uClibc/libpthread/linuxthreads.old/* *.c

$(OBJS) : Makefile

arch: $(BIN) $(PROGRAMS)
	echo ... Completed $(ARCH)

clean-arch:
	@echo Clean $(BIN)
	-rm TAGS -f
	-rm -rf $(PROGRAMS)

$(BIN):
	if [ ! -e $@ ]; then			\
		echo mkdir $@;			\
		mkdir -p $@;			\
	fi;

$(PROGRAMS): $(BIN)/% : %.c Makefile
#$(BIN) Makefile $(wildcard $(UCLINUX_ROOT)/uClibc/lib/*)
	@echo Build Ubicom $@ $<
	$(CC) $(CFLAGS) -o $@  $(notdir $@).c  -lc -lgcc -std=c99
ifdef OBJDUMP
	$(OBJDUMP) -DSr $@.gdb > $@.dump
endif


