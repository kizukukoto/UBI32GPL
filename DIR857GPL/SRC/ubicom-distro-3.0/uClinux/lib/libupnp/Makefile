VER = libupnp-1.6.6

CONF_OPTS = \
	--prefix=$(ROOTDIR)/lib/libupnp/build-ubicom/install \
	--host=$(CONFIGURE_HOST) \
	--disable-debug \
	--enable-client \
	--enable-device \
	--enable-webserver \
	--enable-tools \
	--enable-samples \
	--disable-dependency-tracking \
	$(LIB_ENABLE_STATIC) \
	$(LIB_ENABLE_SHARED) \
	--disable-fast-install \
	--disable-libtool-lock \
	--with-gnu-ld \
	--with-pic

all: build-ubicom/Makefile
	$(MAKE) -C build-ubicom
	$(MAKE) -C build-ubicom install > install.log

build-ubicom/Makefile: Makefile
	find $(VER) -type f -print0 | xargs -0 touch -r $(VER)/configure
	set -e ; \
	rm -rf build-ubicom ; \
	mkdir build-ubicom ; \
	cd build-ubicom ; \
	../$(VER)/configure $(CONF_OPTS)

clean:
	rm -rf build*

romfs:
	$(ROMFSINST) -d -e CONFIG_BINFMT_ELF_FDPIC $(ROOTDIR)/lib/libupnp/build-ubicom/install/lib/libixml.so.2.0.4 /usr/lib/libixml.so.2
	$(ROMFSINST) -d -e CONFIG_BINFMT_ELF_FDPIC $(ROOTDIR)/lib/libupnp/build-ubicom/install/lib/libthreadutil.so.2.2.3 /usr/lib/libthreadutil.so.2
	$(ROMFSINST) -d -e CONFIG_BINFMT_ELF_FDPIC $(ROOTDIR)/lib/libupnp/build-ubicom/install/lib/libupnp.so.3.0.5 /usr/lib/libupnp.so.3

.PHONY: all clean romfs
