VER = libiconv-1.13.1

CONF_OPTS = \
	--prefix=$(ROOTDIR)/lib/libiconv/build-ubicom/install \
	--host=$(CONFIGURE_HOST) \
	--disable-dependency-tracking \
	--disable-relocatable \
	--disable-extra-encodings \
	$(LIB_ENABLE_STATIC) \
	$(LIB_ENABLE_SHARED) \
	--disable-fast-install \
	--disable-libtool-lock \
	--disable-rpath \
	--disable-nls \
	--with-gnu-ld \
	--with-pic \
	--without-libiconv-prefix

all:
	if [ ! -e install.log ]; then \
		rm -rf build-ubicom ; \
		mkdir build-ubicom ; \
		cd build-ubicom ; \
		../$(VER)/configure $(CONF_OPTS) ; \
	fi

	$(MAKE) -C build-ubicom
	$(MAKE) -C build-ubicom install > install.log

clean:
	rm -rf build*
	rm -f install.log

romfs:
	$(ROMFSINST) -d -e CONFIG_BINFMT_ELF_FDPIC $(ROOTDIR)/lib/libiconv/build-ubicom/install/lib/libiconv.so.2.5.0 /lib/libiconv.so.2.5.0
	$(ROMFSINST) -s libiconv.so.2.5.0 /lib/libiconv.so.2
	$(ROMFSINST) -s libiconv.so.2.5.0 /lib/libiconv.so
#$(ROMFSINST) -d -e CONFIG_BINFMT_ELF_FDPIC $(ROOTDIR)/lib/libiconv/build-ubicom/install/lib/libcharset.so.1.0.0 /usr/lib/libcharset.so.1

.PHONY: all clean romfs
