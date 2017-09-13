# libogg makefile for Ubicom32
VER = libogg-1.1.4

all:
	if [ ! -e install.log ]; then \
		cd $(VER) ; \
		./configure  \
		--host=$(CONFIGURE_HOST) \
		--target=$(CONFIGURE_HOST) \
		--disable-nls \
		--prefix=$(ROOTDIR)/lib/libogg/build-ubicom ; \
	fi

	$(MAKE) -C $(VER)
	$(MAKE) -C $(VER) install > install.log

clean:
	rm -rf *~ install.log .sgbuilt_lib
	rm -rf build*
	-$(MAKE) -C $(VER) distclean

romfs:
	$(ROMFSINST) -d -e CONFIG_BINFMT_ELF_FDPIC $(ROOTDIR)/lib/libogg/build-ubicom/lib/libogg.so.0.6.0 /lib/libogg.so.0.6.0
	$(ROMFSINST) -s libogg.so.0.6.0 /lib/libogg.so.0
	$(ROMFSINST) -s libogg.so.0.6.0 /lib/libogg.so

.PHONY: all clean romfs
