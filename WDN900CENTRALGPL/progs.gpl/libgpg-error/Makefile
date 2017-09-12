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

all: libgpg mkend

libgpg:
	mkdir -p $(BUILD) && chmod 777 $(BUILD) && mkdir -p $(BUILD)/bin && chmod 777 $(BUILD)/bin \
	&& mkdir -p $(BUILD)/lib && chmod 777 $(BUILD)/lib && mkdir -p $(BUILD)/include && chmod 777 $(BUILD)/include \
	&& mkdir -p $(BUILD)/etc && chmod 777 $(BUILD)/etc
	cp $(LIBICONV)/* $(BUILD)/lib/. || cp $(LIBICONV)/.libs/* $(BUILD)/lib/.
	if [ ! -f ./.configured ]; then \
	cd libgpg-error-1.10 && /bin/sh configure --prefix=$(BUILD) --host=$(HOST_TYPE) CC=$(CC) AS=$(AS) AR=$(AR) \
	LDFLAGS="${LDFLAGS} -L$(BUILD)/lib" CFLAGS="${CFLAGS}" LD=$(LD) RANLIB=$(RANLIB) STRIP=$(STRIP) \
	--with-libiconv-prefix=$(BUILD) --sysconfdir=$(BUILD) --localstatedir=$(BUILD);\
	fi
	rm -rf libgpg-error-1.10/src/gpg-error-config
	make -C libgpg-error-1.10 && make -C libgpg-error-1.10 install \
	&& cp -rvf libgpg-error-1.10/src/gpg-error-config $(BUILD)/bin/gpg-error-config

mkend:
	touch ./.configured;

install:
	if test ! -s "$(TARGET)/lib/libgpg-error.so.0"; then \
	cp -rvf libgpg-error-1.10/src/.libs/libgpg-error.so.0.8.0 $(TARGET)/lib/libgpg-error.so.0 ;\
	fi

clean:
	make -C libgpg-error-1.10 clean
	rm .configured

.PHONY: all install clean
