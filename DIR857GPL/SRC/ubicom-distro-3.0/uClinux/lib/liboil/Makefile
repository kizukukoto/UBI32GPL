# Liboil makefile for Ubicom32 
VER = liboil-0.3.10

all: $(VER)/config.mak
	$(MAKE) -C $(VER)
	$(MAKE) -C $(VER) install > install.log

$(VER)/config.mak: $(VER)/configure
	find $(VER) -type f -print0 | xargs -0 touch -r $(VER)/configure
	set -e ; \
	cd $(VER) ; \
	./configure \
		--prefix=$(ROOTDIR)/lib/liboil/build-ubicom \
		--host=$(CONFIGURE_HOST) \
		$(LIB_ENABLE_STATIC) \
		$(LIB_ENABLE_SHARED) \
		--with-gnu-ld \
		--with-pic \
		--disable-gtk-doc \
		--enable-broken-implementations \
		--enable-alternate-optimization \
		--enable-prototype-checking \

clean:
	-$(MAKE) -C $(VER) distclean
	rm -rf build*
romfs:
	$(ROMFSINST) -d -e CONFIG_BINFMT_ELF_FDPIC $(ROOTDIR)/lib/liboil/build-ubicom/lib/liboil-0.3.so.0.1.0 /usr/lib/liboil-0.3.so.0

.PHONY: all clean romfs
