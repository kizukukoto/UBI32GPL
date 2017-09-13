# PPP top-level Makefile for Linux.

DESTDIR = $(TARGET)
BINDIR = $(DESTDIR)/sbin
INCDIR = $(DESTDIR)/include
MANDIR = $(DESTDIR)/share/man
ETCDIR = /etc/ppp

# uid 0 = root
INSTALL= install

ifeq ($(CONFIG_USER_IPV6_PPPoE),y)
CFLAGS += -DIPV6_PPPoE
endif

all:
	@echo =================== all ppp used chat $(CONFIG_USER_PPP_USED_CHAT) ===================
	cd pppd/plugins; $(MAKE) $(MFLAGS) all
	cd pppd; $(MAKE) $(MFLAGS) all
#   cd pppstats; $(MAKE) $(MFLAGS) all	
# 	Date: 2009-1-19
# 	Name: Ken_Chiang
# 	Reason: modify for 3g usb client card to used.
# 	Notice :	
ifdef CONFIG_USER_PPP_USED_CHAT
	cd chat; $(MAKE) $(MFLAGS) all
endif	 
#	cd chat; $(MAKE) $(MFLAGS) all
#   cd pppdump; $(MAKE) $(MFLAGS) all		

#install: $(BINDIR) $(MANDIR)/man8 install-progs install-devel
install: $(BINDIR) install-progs

romfs:
	$(ROMFSINST) pppd/pppd /sbin/pppd
ifdef CONFIG_USER_PPP_USED_CHAT
	$(ROMFSINST) chat/chat /sbin/chat
endif	 
	$(ROMFSINST) -d pppd/plugins/rp-pppoe/rp-pppoe.so /lib/pppd/2.4.3/rp-pppoe.so

install-progs:
	@echo =================== install ppp used chat $(CONFIG_USER_PPP_USED_CHAT)===================
	cd pppd/plugins; $(MAKE) $(MFLAGS) install
	cd pppd; $(MAKE) $(MFLAGS) install
# 	Date: 2009-1-19
# 	Name: Ken_Chiang
# 	Reason: modify for 3g usb client card to used.
# 	Notice :	
ifdef CONFIG_USER_PPP_USED_CHAT
	cd chat; $(MAKE) $(MFLAGS) install
endif	
#	cd chat; $(MAKE) $(MFLAGS) install
#	cd pppstats; $(MAKE) $(MFLAGS) install
#	cd pppdump; $(MAKE) $(MFLAGS) install


install-etcppp: $(ETCDIR) $(ETCDIR)/options $(ETCDIR)/pap-secrets \
	$(ETCDIR)/chap-secrets

install-devel:
	cd pppd; $(MAKE) $(MFLAGS) install-devel

$(ETCDIR)/options:
	$(INSTALL) -c -m 644 etc.ppp/options $@
$(ETCDIR)/pap-secrets:
	$(INSTALL) -c -m 600 etc.ppp/pap-secrets $@
$(ETCDIR)/chap-secrets:
	$(INSTALL) -c -m 600 etc.ppp/chap-secrets $@

$(BINDIR):
	$(INSTALL) -d -m 755 $@
$(MANDIR)/man8:
	$(INSTALL) -d -m 755 $@
$(ETCDIR):
	$(INSTALL) -d -m 755 $@

clean:
	@echo =================== clean ppp used chat $(CONFIG_USER_PPP_USED_CHAT)===================
	rm -f `find . -name '*.[oas]' -print`
	rm -f `find . -name 'core' -print`
	rm -f `find . -name '*~' -print`	
# 	Date: 2009-1-19
# 	Name: Ken_Chiang
# 	Reason: modify for 3g usb client card to used.
# 	Notice :	
ifdef CONFIG_USER_PPP_USED_CHAT
	cd chat; $(MAKE) clean
endif	
#	cd chat; $(MAKE) clean
	cd pppd/plugins; $(MAKE) clean
	cd pppd; $(MAKE) clean
#	cd pppstats; $(MAKE) clean
#	cd pppdump; $(MAKE) clean

dist-clean:	clean
	rm -f Makefile `find . -name Makefile -print`

#kernel:
#	cd linux; ./kinstall.sh

# no tests yet, one day...
installcheck:
	true
