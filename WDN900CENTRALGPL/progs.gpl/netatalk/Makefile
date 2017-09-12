-include ../arch.mk
-include ../../path.mk
-include $(TOPDIR)/.config

pwd = $(shell pwd)
BUILD = $(TOPDIR)/build

ifeq ($(strip $(ELBOX_PROGS_GPL_LIBICONV_1_9_1)),y)
	CFLAGS  += -I$(TOPDIR)/progs.gpl/libiconv/libiconv-1.9.1/include
	LDFLAGS += -L$(TOPDIR)/progs.gpl/libiconv/libiconv-1.9.1/lib
	LDFLAGS += -L$(TOPDIR)/progs.gpl/libiconv/libiconv-1.9.1/lib/.libs
	LIBICONV=$(TOPDIR)/progs.gpl/libiconv/libiconv-1.9.1/lib
endif
ifeq ($(strip $(ELBOX_PROGS_GPL_LIBICONV_1_13_1)),y)
	CFLAGS  += -I$(TOPDIR)/progs.gpl/libiconv-1.13.1/libiconv-1.13.1/include
	LDFLAGS += -L$(TOPDIR)/progs.gpl/libiconv-1.13.1/libiconv-1.13.1/lib
	LDFLAGS += -L$(TOPDIR)/progs.gpl/libiconv-1.13.1/libiconv-1.13.1/lib/.libs
	LIBICONV=$(TOPDIR)/progs.gpl/libiconv-1.13.1/libiconv-1.13.1/lib
endif

all: pre_check netatalk mkend

netatalk:
	cp $(LIBICONV)/* $(BUILD)/lib/. || cp $(LIBICONV)/.libs/* $(BUILD)/lib/.
	if [ ! -f ./.configured ]; then \
	cd netatalk-2.2.0 && /bin/sh ./configure --prefix=$(BUILD) --host=$(HOST_TYPE) \
	CC=$(CC) CXX=$(CXX) AS=$(AS) AR=$(AR) LD=$(LD) RANLIB=$(RANLIB) STRIP=$(STRIP) \
	OBJCOPY=$(OBJCOPY) CPPFLAGS=-I$(BUILD)/include \
	LDFLAGS="-L$(TPATH_UC)/ubicom32-linux-uclibc/runtime/usr/lib/march-ubicom32v5 \
	-L$(TPATH_UC)/ubicom32-linux-uclibc/runtime/lib/march-ubicom32v5 -L$(BUILD)/lib" \
	--with-pam=$(BUILD)/ --with-bdb=$(BUILD) --with-ssl-dir=$(TOPDIR)/progs.gpl/openssl-0.9.8r/build/openssl-0.9.8r \
	--with-libgcrypt-dir=$(BUILD) --includedir=$(BUILD)/include --with-libiconv-prefix=$(BUILD)/lib \
	--with-db-prefix=$(BUILD)/lib --enable-cups=no --enable-force-uidgid --enable-quota=yes \
	--with-ldap=no --with-shadow --with-cnid-cdb-backend --with-cnid-dbd-backend ;\
	fi
	make -C netatalk-2.2.0

mkend:
	touch ./.configured;

install:
	mkdir -p $(TARGET)/lib/uams && mkdir -p $(TARGET)/etc/netatalk
	cp netatalk-2.2.0/netatalk/netatalk $(TARGET)/etc/pam.d/.

	cp netatalk-2.2.0/netatalk/* $(TARGET)/etc/netatalk/.

	cp netatalk-2.2.0/etc/afpd/.libs/afpd $(TARGET)/usr/sbin/.

	cp netatalk-2.2.0/etc/uams/.libs/uams_passwd.so $(TARGET)/lib/uams/.
	$(STRIP) -d $(TARGET)/lib/uams/uams_passwd.so

	cp netatalk-2.2.0/etc/uams/.libs/uams_guest.so $(TARGET)/lib/uams/.
	$(STRIP) -d $(TARGET)/lib/uams/uams_guest.so

	cp netatalk-2.2.0/etc/uams/.libs/uams_pam.so $(TARGET)/lib/uams/.
	$(STRIP) -d $(TARGET)/lib/uams/uams_pam.so

	cp netatalk-2.2.0/etc/uams/.libs/uams_dhx2_pam.so $(TARGET)/lib/uams/.
	$(STRIP) -d $(TARGET)/lib/uams/uams_dhx2_pam.so

	cd $(TARGET)/lib/uams && ln -s -f uams_pam.so uams_clrtxt.so && ln -s -f uams_dhx2_pam.so uams_dhx2.so

clean:
	make -C netatalk-2.2.0/ clean
	rm -rf .configured
	rm -rf $(TOPDIR)/build

#######################################################################################################
#                                        Detect Library                                               #
#                   (To indicate what library should be needed for Netatalk    )                      #
#######################################################################################################

pre_check:
	@if [ ! -f $(TOPDIR)/progs.gpl/libgpg-error/.configured ]; then \
	echo -e "Please add Libgpg-error library for Netatalk"; \
	exit 1; \
	fi

	@if [ ! -f $(TOPDIR)/progs.gpl/libgcrypt/.configured ]; then \
	echo -e "Please add Libgcrypt library for Netatalk"; \
	exit 1; \
	fi

	@if [ ! -f $(TOPDIR)/progs.gpl/libdb/.configured ]; then \
	echo -e "Please add Libdb library for Netatalk"; \
	exit 1; \
	fi

	@if [ ! -f $(TOPDIR)/progs.gpl/libpam/.configured ]; then \
	echo -e "Please add Libpam library for Netatalk"; \
	exit 1; \
	fi

	@if [ ! -f $(LIBICONV)/../../.configured ]; then \
	echo -e "Please add Libiconv library for Netatalk"; \
	exit 1; \
	fi

	@if [ ! -d $(TOPDIR)/progs.gpl/flex-2.5.33/build ]; then \
	echo -e "Please add Flex-2.5.33 library for Netatalk"; \
	exit 1; \
	fi

	@if [ ! -d $(TOPDIR)/progs.gpl/openssl-0.9.8r/build ]; then \
	echo -e "Please add openssl-0.9.8r library for Netatalk"; \
	exit 1; \
	fi

.PHONY: all install clean
