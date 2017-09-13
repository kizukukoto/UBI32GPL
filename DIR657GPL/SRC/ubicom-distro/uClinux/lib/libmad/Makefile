# Libmad makefile for Ubicom32 
VER = libmad-0.15.1b

#ifdef CONFIG_UBICOM32_V4
CONFIG_PARAMS := --enable-fpm=ubicom32-ip7k
#endif

all: $(VER)/Makefile
	$(MAKE) -C $(VER)
	$(MAKE) -C $(VER) install > install.log

$(VER)/Makefile: $(VER)/configure Makefile
	find $(VER) -type f -print0 | xargs -0 touch -r $(VER)/configure
	set -e ; \
	cd $(VER) ; \
	./configure \
		--disable-dependency-tracking \
		--disable-libtool-lock \
		$(LIB_ENABLE_STATIC) \
		$(LIB_ENABLE_SHARED) \
		--host=$(CONFIGURE_HOST) \
		--disable-aso \
		--enable-sso \
		--enable-speed \
		--enable-strict-iso \
		--disable-debugging \
		--disable-profiling \
		--with-gnu-ld \
		--with-pic  \
		$(CONFIG_PARAMS) \
		--prefix=$(ROOTDIR)/lib/libmad/build-ubicom \

clean:
	-$(MAKE) -C $(VER) distclean
	rm -rf build*
romfs:
	$(ROMFSINST) -d -e CONFIG_BINFMT_ELF_FDPIC $(ROOTDIR)/lib/libmad/build-ubicom/lib/libmad.so.0.2.1 /usr/lib/libmad.so.0

.PHONY: all clean romfs
