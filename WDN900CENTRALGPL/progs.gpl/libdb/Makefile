-include ../arch.mk
-include ../../path.mk
-include $(TOPDIR)/.config


pwd = $(shell pwd)
BUILD=$(TOPDIR)/build
all: libdb mkend

libdb:
	if [ ! -f ./.configured ]; then \
	cd db-4.7.25/build_unix && /bin/sh ../dist/configure --prefix=$(BUILD) --host=$(HOST_TYPE) \
	CC=$(CC) CXX=$(CXX) AS=$(AS) AR=$(AR) LD=$(LD) RANLIB=$(RANLIB) STRIP=$(STRIP) OBJCOPY=$(OBJCOPY) \
	LDFLAGS="-L$(TPATH_UC)/ubicom32-linux-uclibc/runtime/lib/march-ubicom32v5 -L$(BUILD)/lib" \
	--enable-pthread_api --enable-rpc  --sysconfdir=$(BUILD)/etc --localstatedir=$(BUILD)/var;\
    fi
	make -C db-4.7.25/build_unix && cp db-4.7.25/build_unix/.libs/libdb-4.7.so $(BUILD)/lib/libdb-4.7.so
	make -C db-4.7.25/build_unix install

mkend:
	touch ./.configured;

install:
	if test ! -s "$(TARGET)/lib/libdb-4.7.so"; \
	then cp $(BUILD)/lib/libdb-4.7.so $(TARGET)/lib/. ;\
	fi


clean:
	make -C db-4.7.25/build_unix/ clean
	rm -rf db-4.7.25/build_unix/install
	rm .configured

.PHONY: all install clean
