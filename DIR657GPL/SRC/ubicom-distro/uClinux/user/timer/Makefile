#CFLAGS += -I./ -I$(APPS_PATH)/sutil/ -I$(APPS_PATH)/rc/ -I$(APPS_PATH)/nvram
#LDFLAGS += -L$(APPS_PATH)/nvram -lnvram -L$(APPS_PATH)/sutil -lsutil -lipv6

#CFLAGS  += -I. -I$(APPS_PATH)/libplatform/
#LDFLAGS	+= -L$(APPS_PATH)/libplatform -lversion
#LDFLAGS += -L$(APPS_PATH)/libplatform -lvct

TARGET	    = $(ROOTDIR)/romfs

CFLAGS += -I$(ROOTDIR)/config
CFLAGS += -I$(ROOTDIR)/user/sutil/ -I$(ROOTDIR)/user/nvram/ -I$(ROOTDIR)/user/rc/ -I$(ROOTDIR)/user/libplatform/
LDFLAGS += -L$(ROOTDIR)/user/sutil -L$(ROOTDIR)/user/nvram -L$(ROOTDIR)/user/libplatform
LDFLAGS += -lnvram -lsutil -lversion -lvct

CFLAGS += -DCONFIG_IP_LOOKUP

#ifeq ($(CONFIG_USER_IPV6_SUPPORT),y)
CFLAGS += -DIPv6_SUPPORT
LDFLAGS += -lipv6
#endif

# Support radvd or not
ifeq ($(CONFIG_USER_RADVD_RADVD),y)
CFLAGS += -DRADVD
endif

# Support monitor ipv6 clients in LAN side
ifeq ($(CONFIG_USER_IPV6_CLIENT_LIST),y)
CFLAGS += -DIPV6_CLIENT_LIST
endif


OBJS := timer.o

all: timer

thread: $(OBJS)
	$(CC) -o $@ $^

install:
	cp timer $(TARGET)/sbin
clean:
	rm -f *.o *~ timer

romfs:
	$(ROMFSINST) timer /sbin/timer
