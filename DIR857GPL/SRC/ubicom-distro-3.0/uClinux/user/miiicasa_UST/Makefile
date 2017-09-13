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

all: miiicasa_UST

clean:
	rm -rf *.o *~ miiicasa_UST

install:
	install -d $(TARGET)/sbin
	install miiicasa_UST $(TARGET)/sbin
	$(STRIP) $(TARGET)/sbin/miiicasa_UST

miiicasa_UST: $(OBJS)
	$(CC) -o  $@ $^ $(LDFLAGS)

romfs:
	$(ROMFSINST) miiicasa_UST /sbin
