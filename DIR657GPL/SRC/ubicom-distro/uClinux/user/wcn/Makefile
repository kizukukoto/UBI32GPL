#
# Makefile for ARM-Linux (Marvell Platfrom)
#

TARGET      = $(ROOTDIR)/romfs
#CFLAGS += -DWCND_DEBUG

#CFLAGS  += -I. -I$(TOP)/sutil -I$(TOP)/nvram -I$(APPS_PATH)/libversion
CFLAGS += -I$(ROOTDIR)/user/sutil/ -I$(ROOTDIR)/user/nvram/
CFLAGS += -I$(ROOTDIR)/config

#LDFLAGS	+=	-L$(TOP)/nvram -lnvram -L$(TOP)/sutil -lsutil -L$(APPS_PATH)/libplatform -lversion
LDFLAGS += -L$(ROOTDIR)/user/sutil -L$(ROOTDIR)/user/nvram
LDFLAGS += -lsutil -lm -lnvram

OBJS := wcn.o main.o 

all: wcnd	

clean:
	rm -rf *.o *~ wcnd

install:
	install -d $(TARGET)/sbin
	install wcnd $(TARGET)/sbin
	$(STRIP)	$(TARGET)/sbin/wcnd

wcnd: $(OBJS)
	$(CC) -o  $@ $^ $(LDFLAGS)
	
romfs:
	$(ROMFSINST) wcnd /sbin

