#include ../../.config

#CFLAGS  += -I. -I$(KERNEL_PATH)/include/
CFLAGS += -I. -I$(ROOTDIR)/linux-2.6.x/include/
CFLAGS += -I$(ROOTDIR)/user/mtd-utils/mtd/
CFLAGS += -I$(ROOTDIR)/config

ifeq ($(CONFIG_USER_WL_ATH_5GHZ),y)
CFLAGS += -DUSER_WL_ATH_5GHZ
endif

ifeq ($(CONFIG_USER_SUTIL_DEBUG),y)
CFLAGS += -DSUTIL_DEBUG
endif

ifeq ($(CONFIG_USER_SUTIL_DEBUG_FLASH),y)
CFLAGS += -DSUTIL_DEBUG_FLASH
endif

#ifeq ($(CONFIG_USER_IPV6_SUPPORT),y)
CFLAGS += -DIPV6_SUPPORT
#endif

ifeq ($(CONFIG_USER_IPV6_PPPoE),y)
CFLAGS += -DIPV6_PPPoE
endif

ifeq ($(CONFIG_USER_IPV6_DHCP),y)
CFLAGS += -DDHCPv6
endif

# Support 6rd or not
ifeq ($(CONFIG_USER_IPV6_6RD),y)
CFLAGS += -DIPV6_6RD 
endif

# Support disable lan rdnss function or not
ifeq ($(CONFIG_USER_IPV6_DISABLE_LAN_RDNSS ),y)
CFLAGS += -DIPV6_DISABLE_LAN_RDNSS 
endif

ifeq ($(CONFIG_USER_LINUX_NVRAM),y)
CFLAGS += -DLINUX_NVRAM
endif
all: libsutil.so libproject.so libipv6.so

do_not_clean:
	rm -rf *.o *.so *~

install:
	install -D libsutil.so $(TARGET)/lib/libsutil.so
	install -D libproject.so $(TARGET)/lib/libproject.so
	install -D libipv6.so $(TARGET)/lib/libipv6.so
	$(STRIP) $(TARGET)/lib/libsutil.so
	$(STRIP) $(TARGET)/lib/libproject.so
	$(STRIP) $(TARGET)/lib/libipv6.so

libsutil.so: sutil.o scmd_nvram.o
#	$(LD) -shared -o $@ $^
	$(CC) -shared -o $@ $^

libproject.so: project.o scmd_nvram.o
#	$(LD) -shared -o $@ $^
	$(CC) -shared -o $@ $^

libipv6.so: ipv6.o scmd_nvram.o
#	$(LD) -shared -o $@ $^
	$(CC) -shared -o $@ $^

romfs:
	$(ROMFSINST) libsutil.so /lib
	$(ROMFSINST) libproject.so /lib
	$(ROMFSINST) libipv6.so /lib

