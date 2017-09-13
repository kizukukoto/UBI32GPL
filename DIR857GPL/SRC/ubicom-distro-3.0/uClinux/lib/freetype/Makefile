VER = freetype-2.3.8

BUILD_DIR=build-ubicom
FREETYPE_PREFIX:=$(ROOTDIR)/lib/freetype/$(BUILD_DIR)/install

CONF_OPTS = \
	--prefix=$(FREETYPE_PREFIX) \
	--host=$(CONFIGURE_HOST) \
	$(LIB_ENABLE_SHARED) \
	$(LIB_ENABLE_STATIC) \
	--disable-fast-install \
	--with-zlib=yes \
	--with-old-mac-fonts=no \
	--with-fsspec=no \
	--with-fsref=no \
	--with-quickdraw-toolbox=no \
	--with-quickdraw-carbon=no \
	--with-ats=no \
	--with-gnu-ld \
	--with-pic \
	LIBS="-lc -lgcc"

all: build-ubicom/Makefile
	$(MAKE) -C build-ubicom
	$(MAKE) -C build-ubicom install > install.log

build-ubicom/Makefile: Makefile
	set -e ; \
	rm -rf $(FREETYPE_PREFIX) ; \
	rm -rf $(BUILD_DIR) ; \
	mkdir $(BUILD_DIR) ; \
	cd $(BUILD_DIR) ; \
	../$(VER)/configure $(CONF_OPTS)
clean:
	rm -rf $(FREETYPE_PREFIX)
	rm -rf $(BUILD_DIR)

romfs:
	$(ROMFSINST) -d -e CONFIG_BINFMT_ELF_FDPIC $(FREETYPE_PREFIX)/lib/libfreetype.so.6.3.19 /usr/lib/libfreetype.so.6

.PHONY: all clean romfs
