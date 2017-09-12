-include ../arch.mk
-include ../../path.mk
-include $(TOPDIR)/.config

pwd = $(shell pwd)
BUILD=$(TOPDIR)/build
all: libgcrypt mkend

libgcrypt:
	if [ ! -f ./.configured ]; then \
	cd libgcrypt-1.5.0-beta1 && /bin/sh configure --prefix=$(BUILD) --host=$(HOST_TYPE) CC=$(CC) AS=$(AS) AR=$(AR) \
	LD=$(LD) RANLIB=$(RANLIB) STRIP=$(STRIP) --disable-padlock-support --disable-dependency-tracking --with-pic \
	--enable-noexecstack --disable-O-flag-munging --with-gpg-error-prefix=$(BUILD) \
	CPPFLAGS=-I$(pwd)/../libgpg-error/libgpg-error-1.10/src \
	LDFLAGS="-L$(TPATH_UC)/ubicom32-linux-uclibc/runtime/lib/march-ubicom32v5 -L$(BUILD)/lib -L$(pwd)/../libgpg-error/libgpg-error-1.10/src/.libs" \
	--sysconfdir=$(BUILD)/etc --localstatedir=$(BUILD)/var;\
	fi
	make -C libgcrypt-1.5.0-beta1 && cp -rvf libgcrypt-1.5.0-beta1/src/libgcrypt.la $(BUILD)/lib/.
	make -C libgcrypt-1.5.0-beta1 install

mkend:
	touch ./.configured;

install:
	if test ! -s "$(TARGET)/lib/libgcrypt.so.11"; then \
	cp -rvf libgcrypt-1.5.0-beta1/src/libgcrypt.la $(TARGET)/lib/. && \
	cp -rvf libgcrypt-1.5.0-beta1/src/.libs/libgcrypt.so.11.7.0 $(TARGET)/lib/libgcrypt.so.11 ;\
	fi

clean:
	make -C libgcrypt-1.5.0-beta1 clean
	rm .configured

.PHONY: all install clean
