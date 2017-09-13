CFLAGS += -Wall -W

APPS_PATH=$(ROOTDIR)/user


LDFLAGS += -L$(APPS_PATH)/libplatform -L$(APPS_PATH)/nvram -lvct
CFLAGS += -I$(APPS_PATH)/libplatform -I$(APPS_PATH)/nvram

# add 2011.11.17
ifeq ($(CONFIG_USER_OPENDNS),y)
CFLAGS += -DOPENDNS
endif

LIBS += -lpthread -lnvram

OBJS = cache.o rfc1035.o util.o option.o forward.o isc.o network.o \
       dnsmasq.o dhcp.o lease.o rfc2131.o netlink.o dbus.o bpf.o \
       helper.o tftp.o log.o $(EXTRA_OBJS)

.c.o:
	$(CC) $(CFLAGS) $(COPTS) $(DBUS_MINOR) $(I18N) $(DBUS_CFLAGS) $(SUNOS_VER) $(RPM_OPT_FLAGS) -c $<

dnsmasq : $(OBJS)
	$(CC) $(LDFLAGS) -o $@  $(OBJS) $(DBUS_LIBS) $(SUNOS_LIBS) $(LIBS) 
 
dnsmasq.pot : $(OBJS:.o=.c) dnsmasq.h config.h
	xgettext -d dnsmasq --foreign-user --keyword=_ -o dnsmasq.pot -i $(OBJS:.o=.c)

%.mo : ../po/%.po dnsmasq.pot
	msgmerge -o - ../po/$*.po dnsmasq.pot | msgfmt -o $*.mo -
