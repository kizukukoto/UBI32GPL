#
# Makefile for ARM-Linux (Star Platfrom)
#
TARGET=$(ROOTDIR)/romfs
APPS_PATH=$(ROOTDIR)/user

ifeq ($(CONFIG_USER_NVRAM_DEBUG),y)
CFLAGS += -DNVRAM_DEBUG
endif

ifeq ($(CONFIG_USER_NVRAM_DEBUG_ARTBLOCK),y)
CFLAGS += -DNVRAM_DEBUG_ARTBLOCK
endif

CFLAGS += -I$(ROOTDIR)/config
CFLAGS  += -I. -I$(APPS_PATH)/sutil/
LDFLAGS += -L.
OBJS := nvram.o main.o

all: libnvram.so nvram

clean:
	rm -rf *.o *.so *~ nvram

install:
	install -d $(TARGET)/bin
	install nvram $(TARGET)/bin
	$(STRIP)	$(TARGET)/bin/nvram
	install -D libnvram.so $(TARGET)/lib/libnvram.so
	$(STRIP) $(TARGET)/lib/libnvram.so
	install $(WWW_PATH)/nvram.default $(TARGET)/etc/

uninstall:
	rm -rf $(TARGET)/bin/nvram
	rm -rf $(TARGET)/lib/libnvram.so

libnvram.so: nvram.o
ifeq ($(PLATFORM), sl3516)
	$(CC) -shared -o $@ $^
else
	$(CC) -shared -o $@ $^
#	$(LD) -shared -o $@ $^
endif

nvram: main.o
	$(CC) -o  $@ $^ $(CFLAGS) $(LDFLAGS) -lnvram

romfs:
	$(ROMFSINST) nvram /bin
	$(ROMFSINST) libnvram.so /lib

