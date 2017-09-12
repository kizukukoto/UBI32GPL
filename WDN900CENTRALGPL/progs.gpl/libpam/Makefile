-include ../arch.mk
-include ../../path.mk
-include $(TOPDIR)/.config

pwd = $(shell pwd)
BUILD=$(TOPDIR)/build

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

all: libpam mkend

libpam:
	cp $(TOPDIR)/progs.gpl/flex-2.5.33/build/*.* $(BUILD)/.
	cp $(LIBICONV)/* $(BUILD)/lib/. || cp $(LIBICONV)/.libs/* $(BUILD)/lib/.
	if [ ! -f ./.configured ]; then \
	cd Linux-PAM-1.1.3 && /bin/sh configure --prefix=$(BUILD) --host=$(HOST_TYPE) host_alias=$(HOST_TYPE) \
	--disable-nls --includedir=$(BUILD)/include/security --with-libiconv-prefix=$(BUILD)/lib \
	--with-db-prefix=$(BUILD)/lib --disable-selinux --disable-regenerate-docu \
	CC=$(CC) AS=$(AS) AR=$(AR) LD=$(LD) RANLIB=$(RANLIB) STRIP=$(STRIP) \
	CPPFLAGS=-I$(BUILD)/include LDFLAGS="${LDFLAGS} -L$(BUILD) -L$(BUILD)/lib" CFLAGS="${CFLAGS}";\
	fi
	make -C Linux-PAM-1.1.3
	make -C Linux-PAM-1.1.3 install
	cp Linux-PAM-1.1.3/modules/pam_unix/.libs/pam_unix.so $(BUILD)/lib/.
	cp Linux-PAM-1.1.3/libpam/.libs/libpam.so.0 $(BUILD)/lib/.

mkend:
	touch ./.configured;

install:
	mkdir -p $(TARGET)/etc/pam.d
	mkdir -p $(TARGET)/lib/security
	if test ! -s "$(TARGET)/lib/security/pam_unix.so"; \
	then cp Linux-PAM-1.1.3/modules/pam_unix/.libs/pam_unix.so $(TARGET)/lib/security/. ;\
	$(STRIP) -d $(TARGET)/lib/security/pam_unix.so;\
	cp Linux-PAM-1.1.3/libpam/.libs/libpam.so.0 $(TARGET)/lib/. ;\
	$(STRIP) -d $(TARGET)/lib/libpam.so.0;\
	fi

clean:
	make -C Linux-PAM-1.1.3 clean
	rm .configured

.PHONY: all install clean


