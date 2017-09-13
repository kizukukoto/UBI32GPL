TARGET = pptp
OBJS = pptp.o  
MDIR = net

KDIR ?= $(shell sh find_kernel_headers)
#KDIR = /usr/src/linux-2.6.24-rc1

obj-m      := $(TARGET).o
#$(TARGET)-objs      := $(OBJS)

ifneq (,$(findstring 2.4.,$(CURRENT)))
CURRENT = $(shell uname -r)
EXTRA_CFLAGS = -DEXPORT_SYMTAB
PWD = $(shell pwd)
DEST = /lib/modules/$(CURRENT)/kernel/$(MDIR)
KERNEL_INCLUDE := /lib/modules/$(CURRENT)/build/include
## jimmy modified
#CC = gcc -c
#CROSS_COMPILE=/opt/ap71_tools/mips32k/bin/mips-linux-
CC := /opt/ap71_tools/mips32k/bin/mips-linux-gcc
## ------------

CFLAGS := -Wall -fno-strength-reduce -fomit-frame-pointer -O2 -fno-common 
CFLAGS := $(CFLAGS) -D__KERNEL__ -DMODULE -I$(KERNEL_INCLUDE) -DMOD_VERSIONS -include $(KERNEL_INCLUDE)/linux/modversions.h
CFLAGS := $(CFLAGS) -Wall -Wno-unused -Wno-multichar

ALL: $(SOURCES)
	echo "in driver !!!!!"
	ld  -r -o pptp_mod.o $(SOURCES)
else
default: kernel_headers
	@echo "kernel_headers !!!"
	@echo "findstring = $(findstring)"
	@echo "CURRENT = $(CURRENT)"
	make -C $(KDIR) SUBDIRS=$(PWD) modules

kernel_headers:
	@if test -z "$(KDIR)"; then \
	    echo "kernel headers not found"; \
	    exit 1; \
	else \
	    echo "using \"$(KDIR)\" kernel headers"; \
	fi
endif

$(TARGET).o: $(OBJS)
	$(LD) $(LD_RFLAG) -r -o $@ $(OBJS)

ifneq (,$(findstring 2.4.,$(CURRENT)))
install:
	su -c "cp -v $(TARGET)_mod.o $(DEST)/$(TARGET).o && /sbin/depmod -a"
else
install:
	make -C $(KDIR) SUBDIRS=$(PWD) modules_install
endif

clean:
	-rm -f *.o *.ko .*.cmd .*.flags *.mod.c

ifneq (,$(findstring 2.6.,$(CURRENT)))
-include $(KDIR)/Rules.make
endif
