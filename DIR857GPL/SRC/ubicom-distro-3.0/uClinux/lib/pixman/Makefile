VER = pixman-0.14.0

CONF_OPTS = \
	--prefix=$(ROOTDIR)/lib/pixman/build-ubicom/install \
	--host=$(CONFIGURE_HOST) \
	--disable-dependency-tracking \
	$(LIB_ENABLE_SHARED) \
	$(LIB_ENABLE_STATIC) \
	--disable-fast-install \
	--disable-libtool-lock \
	--disable-mmx \
	--disable-sse2 \
	--disable-vmx \
	--disable-arm-simd \
	--disable-gtk \
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
	$(ROMFSINST) -d -e CONFIG_BINFMT_ELF_FDPIC $(ROOTDIR)/lib/pixman/build-ubicom/install/lib/libpixman-1.so.0.14.0 /usr/lib/libpixman-1.so.0

.PHONY: all clean romfs
