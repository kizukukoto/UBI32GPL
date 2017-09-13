#
# Makefile for MIPS-Linux (Atheros Platfrom)
#

TARGET      = $(ROOTDIR)/romfs

#CFLAGS  += -I. -I$(APPS_PATH)/libplatform/
CFLAGS += -I$(ROOTDIR)/user/libplatform/
#LDFLAGS	+= -L$(APPS_PATH)/libplatform -lversion
LDFLAGS += -L$(ROOTDIR)/user/libplatform
#LDFLAGS += -L$(APPS_PATH)/libplatform -lvct
LDFLAGS += -lvct -lversion

OBJS := main.o

all: fwqd

clean:
	rm -rf *.o *~ fwqd

install:
	install -d $(TARGET)/sbin
	install fwqd $(TARGET)/sbin
	$(STRIP) $(TARGET)/sbin/fwqd

fwqd: $(OBJS)
	$(CC) -o  $@ $^ $(LDFLAGS)

romfs:
	$(ROMFSINST) fwqd /sbin
