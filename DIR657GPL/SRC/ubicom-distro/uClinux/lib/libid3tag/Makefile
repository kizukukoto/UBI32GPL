# libid3tag makefile for Ubicom32
VER=libid3tag-0.15.1b

all:
	if [ ! -e install.log ]; then \
		cd $(VER) ; \
		./configure \
		--host=$(CONFIGURE_HOST) \
		--target=$(CONFIGURE_HOST) \
		--disable-nls \
		--disable-dependency-tracking \
		--disable-libtool-lock \
		--prefix=$(ROOTDIR)/lib/libid3tag/build-ubicom \
		--enable-shared \
		--enable-static \
		--disable-profiling \
		--disable-debugging ; \
	fi
	
	$(MAKE) -C $(VER)
	$(MAKE) -C $(VER) install > install.log

clean:
	-$(MAKE) -C $(VER) distclean
	rm -rf build* install.log

romfs:
	$(ROMFSINST)  -d -e CONFIG_BINFMT_ELF_FDPIC build-ubicom/lib/libid3tag.0.3.0 /lib/libid3tag.so.0.3.0
	$(ROMFSINST) -s libid3tag.so.0.3.0 /lib/libid3tag.so.0
	$(ROMFSINST) -s libid3tag.so.0.3.0 /lib/libid3tag.so

.PHONY: all clean romfs
