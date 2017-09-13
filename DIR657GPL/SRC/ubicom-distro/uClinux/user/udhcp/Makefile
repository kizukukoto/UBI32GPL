# udhcp makefile

FS_PATH	    = $(ROOTDIR)/romfs
DESTDIR     = $(FS_PATH)
prefix      = 
SBINDIR     = /sbin
USRSBINDIR  = $(DESTDIR)${prefix}/sbin
USRBINDIR   = $(DESTDIR)${prefix}/bin
USRSHAREDIR = $(ROMFSDIR)/usr/share

# Uncomment this to get a shared binary. Call as udhcpd for the server,
# and udhcpc for the client
#COMBINED_BINARY=0

ifeq ($(CONFIG_USER_UDHCP_SYSLOG),y)
CFLAGS += -DUDHCP_SYSLOG
endif

ifeq ($(CONFIG_MODULE_UBICOM_STREAMENGINE2_IP8K),y)
CFLAGS += -DCONFIG_MODULE_UBICOM_STREAMENGINE2_IP8K
endif

ifeq ($(CONFIG_USER_UDHCP_DEBUG),y)
CFLAGS += -DUDHCP_DEBUG -g
STRIP=true
else
CFLAGS += -Os -fomit-frame-pointer
STRIP=$(CROSS_COMPILE)strip
endif

# ChaseCheng modified 2009.4.17
# Reason: 1. unlink the behavior with static route

ifeq ($(CONFIG_IGNORE_IP_CONTRAST),y)
CFLAGS += -DIGNORE_IP_CONTRAST
endif

# # ChaseCheng modified 2009.4.16
# # Reason: 1. Write static route after re-connect.
#ifeq ($(CONFIG_STATIC_ROUTE),y)
#CFLAGS += -DCONFIG_STATIC_ROUTE
#endif

#CROSS_COMPILE=arm-linux-gnu-
LD = $(CC)
INSTALL = install

OBJS_SHARED = common.o options.o packet.o pidfile.o signalpipe.o socket.o
DHCPD_OBJS = dhcpd.o arpping.o files.o leases.o serverpacket.o static_leases.o 
DHCPC_OBJS = dhcpc.o clientpacket.o clientsocket.o script.o

ifdef COMBINED_BINARY
EXEC1 = udhcpd
OBJS1 = $(DHCPD_OBJS) $(DHCPC_OBJS) $(OBJS_SHARED) frontend.o
CFLAGS += -DCOMBINED_BINARY
else
EXEC1 = udhcpd
OBJS1 = $(DHCPD_OBJS) $(OBJS_SHARED)

EXEC2 = udhcpc
OBJS2 = $(DHCPC_OBJS) $(OBJS_SHARED)
endif

EXEC3 = dumpleases
OBJS3 = dumpleases.o

BOOT_PROGRAM = udhcpc
DAEMON = udhcpd
COMMAND = dumpleases

CFLAGS += -I$(ROOTDIR)/config
CFLAGS += -W -Wall -Wstrict-prototypes -D_GNU_SOURCE
## jimmy modified 20080428
CFLAGS += -I./ -I$(ROOTDIR)/user/sutil/ -I$(ROOTDIR)/user/nvram/
#CFLAGS += -I./ -I$(TOP)/sutil/ -I$(TOP)/nvram/
#	Date: 2009-01-07
#	Name: jimmy huang
#	Reason:	we don't want to use nvram library and header file
#	Note: Modified the codes above
#

ifeq ($(CONFIG_TC),y)
CFLAGS +=-DCONFIG_TC
endif

LDFLAGS += -L$(ROOTDIR)/user/sutil -L$(ROOTDIR)/user/nvram
LDLIBS += -lsutil -lnvram
#LDFLAGS += -L$(TOP)/sutil -lsutil -L$(TOP)/nvram -lnvram
#	Date: 2009-01-07
#	Name: jimmy huang
#	Reason:	we don't want to use sutil and nvram library and header file
#	Note: Modified the codes above


all: $(EXEC1) $(EXEC2) $(EXEC3)
	#$(STRIP) --remove-section=.note --remove-section=.comment $(EXEC1) $(EXEC2) $(EXEC3)

$(OBJS1) $(OBJS2) $(OBJS3): *.h Makefile
$(EXEC1) $(EXEC2) $(EXEC3): Makefile

.c.o:
	$(CC) -c $(CFLAGS) $<
	
$(EXEC1): $(OBJS1)
	$(LD) $(LDFLAGS) $(OBJS1) -o $(EXEC1)

$(EXEC2): $(OBJS2)
	$(LD) $(LDFLAGS) $(OBJS2) -o $(EXEC2)

$(EXEC3): $(OBJS3)
	$(LD) $(LDFLAGS) $(OBJS3) -o $(EXEC3)


install: all
	mkdir -p $(USRSBINDIR) $(USRBINDIR) 
	$(INSTALL) -m 755 $(DAEMON) $(USRSBINDIR)
	$(STRIP)	$(USRSBINDIR)/$(DAEMON)
	$(INSTALL) -m 755 $(COMMAND) $(USRBINDIR)
	$(STRIP) $(USRBINDIR)/$(COMMAND)
ifdef COMBINED_BINARY
	ln -sf $(DAEMON) $(USRSBINDIR)/$(BOOT_PROGRAM)
else
	$(INSTALL) -m 755 $(BOOT_PROGRAM) $(USRBINDIR)
	$(STRIP) $(USRBINDIR)/$(BOOT_PROGRAM)
endif
	mkdir -p $(USRSHAREDIR)/udhcpc
	for name in russia-bound-nodns russia-bound-dns bound-dns bound-nodns deconfig nak renew script ; do \
		$(INSTALL) -m 755 samples/sample.$$name \
			$(USRSHAREDIR)/udhcpc/default.$$name ; \
	done

#	touch $(TARGET)/var/misc/udhcpd.leases
#	touch $(TARGET)/etc/udhcpd.conf
#	mkdir -p $(USRSHAREDIR)/man/man1
#	$(INSTALL) -m 644 dumpleases.1 $(USRSHAREDIR)/man/man1
#	mkdir -p $(USRSHAREDIR)/man/man5
#	$(INSTALL) -m 644 udhcpd.conf.5 $(USRSHAREDIR)/man/man5
#	mkdir -p $(USRSHAREDIR)/man/man8
#	$(INSTALL) -m 644 udhcpc.8 udhcpd.8 $(USRSHAREDIR)/man/man8

clean:
	-rm -f udhcpd udhcpc dumpleases *.o *~ core

romfs:
#	while IPV6_SUPPORT, create symbolic link at rc/Makefile
ifneq ($(CONFIG_USER_IPV6_SUPPORT),y)
	cd $(ROMFSDIR)/etc ; ln -sf ../var/etc/resolv.conf resolv.conf
endif

	mkdir -p $(ROMFSDIR)/var/misc ; touch $(ROMFSDIR)/var/misc/udhcpd.leases
	mkdir -p $(ROMFSDIR)/usr/share/udhcpc

ifeq ($(CONFIG_USER_IPV6_SUPPORT),y)
#replace resolv_conf from ipv4 only to ipv4/ipv6
	for name in russia-bound-dns bound-dns renew ; do \
		$(ROMFSINST) samples/dual.sample.$$name /usr/share/udhcpc/default.$$name ; \
	done
	for name in russia-bound-nodns bound-nodns deconfig nak script ; do \
		$(ROMFSINST) samples/sample.$$name /usr/share/udhcpc/default.$$name ; \
	done
endif

	$(ROMFSINST) /bin/$(EXEC1)
	$(ROMFSINST) /bin/$(EXEC2)
	$(ROMFSINST) /bin/$(EXEC3)
