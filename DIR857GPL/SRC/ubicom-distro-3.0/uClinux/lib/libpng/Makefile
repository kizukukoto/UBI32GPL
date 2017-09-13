VER = libpng-1.2.35

CONF_OPTS = \
	--prefix=$(ROOTDIR)/lib/libpng/build-ubicom/install \
	--host=$(CONFIGURE_HOST) \
	--enable-dependency-tracking \
	$(LIB_ENABLE_SHARED) \
	$(LIB_ENABLE_STATIC) \
	--without-gnu-ld

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
	$(ROMFSINST) -d -e CONFIG_BINFMT_ELF_FDPIC $(ROOTDIR)/lib/libpng/build-ubicom/install/lib/libpng12.so.0.35.0 /usr/lib/libpng12.so.0
	$(ROMFSINST) -d -e CONFIG_BINFMT_ELF_FDPIC $(ROOTDIR)/lib/libpng/build-ubicom/install/lib/libpng.so.3.35.0 /usr/lib/libpng.so.3


.PHONY: all clean romfs
