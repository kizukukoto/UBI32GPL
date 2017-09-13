#
# build squashfs4.0 for the host - rkeller@ubicom.com
#

CFLAGS += -Isquashfs-tools

PROGS =

all: $(PROGS)
	make -C squashfs-tools

.PHONY:	host_build
host_build:
	make -C squashfs-tools

romfs:

clean:
	make -C squashfs-tools clean

