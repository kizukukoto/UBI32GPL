VER = SDL_gfx-2.0.18
VER-SDL = SDL-1.2.13

BUILD_DIR:=build-ubicom
LIBSDL_PREFIX:=$(shell pkg-config --variable=prefix sdl)

CONF_OPTS = \
	--prefix=$(ROOTDIR)/lib/SDL_gfx/$(BUILD_DIR)/install \
	--host=$(CONFIGURE_HOST) \
	$(LIB_ENABLE_SHARED) \
	$(LIB_ENABLE_STATIC) \
	--disable-fast-install \
	--disable-dependency-tracking \
	--disable-libtool-lock \
	--enable-mmx=no \
	--with-gnu-ld \
	--without-x \
	--with-sdl-prefix=$(LIBSDL_PREFIX)

all: $(BUILD_DIR)/Makefile
	$(MAKE) -C $(BUILD_DIR)
	$(MAKE) -C $(BUILD_DIR) install > install.log

$(BUILD_DIR)/Makefile: Makefile
	find $(VER) -type f -print0 | xargs -0 touch -r $(VER)/configure
	set -e ; \
	rm -rf $(BUILD_DIR) ; \
	mkdir $(BUILD_DIR) ; \
	cd $(BUILD_DIR) ; \
	../$(VER)/configure $(CONF_OPTS)

clean:
	rm -rf build*

romfs:
	$(ROMFSINST) -d -e CONFIG_BINFMT_ELF_FDPIC $(ROOTDIR)/lib/SDL_gfx/$(BUILD_DIR)/install/lib/libSDL_gfx.so.0.0.18 /usr/lib/libSDL_gfx.so.0

.PHONY: all clean romfs
