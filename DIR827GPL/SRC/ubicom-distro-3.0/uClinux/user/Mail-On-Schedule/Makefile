#
# Makefile for ARM-Linux (Marvell Platfrom)
#

TARGET      = $(ROOTDIR)/romfs

CFLAGS += -I$(ROOTDIR)/user/sutil/ -I$(ROOTDIR)/user/nvram/ -I$(ROOTDIR)/user/httpd/ 
CFLAGS += -I$(ROOTDIR)/config
LDFLAGS += -L$(ROOTDIR)/user/sutil -L$(ROOTDIR)/user/nvram
LDFLAGS += -lsutil -lm -lnvram

OBJS := main.o

all: mailosd	

clean:
	rm -rf *.o *~ mailosd

install:
	install -d $(TARGET)/sbin
	$(STRIP) mailosd
	install mailosd $(TARGET)/sbin
	
mailosd: $(OBJS)
	$(CC) -o  $@ $^ $(LDFLAGS)

romfs:
	$(ROMFSINST) mailosd /sbin

