REVISION = 1.2.1
VER = flac-$(REVISION)

FLAC_APP_PARAM=""

ifdef CONFIG_USER_FLAC_APP
	FLAC_APP_PARAM=""
endif

# mkdir build-ubicom/doc/html/api since we don't use doxygen.

all:
	if [ ! -e install.log ]; then \
		cd $(VER) ; \
		./configure \
		--host=$(CONFIGURE_HOST) \
		--enable-cross-compile \
		--disable-debug \
		--prefix=$(ROOTDIR)/lib/FLAC/build-ubicom \
		--disable-nls \
		--disable-cpplibs \
		--disable-sse \
		--disable-3dnow \
		--disable-altivec \
		--disable-doxgen-docs \
		--disable-local-xmms-plugin \
		--disable-xmms-plugin \
		--disable-ogg \
		--disable-oggtest \
		--disable-debug ; \
	fi

	$(MAKE) -C $(VER)
	mkdir -p build-ubicom/doc/html/api
	touch build-ubicom/doc/html/api/dummy
	$(MAKE) -C $(VER) install > install.log

romfs:
	$(ROMFSINST) -d -e CONFIG_BINFMT_ELF_FDPIC build-ubicom/lib/libFLAC.so.8.2.0 /lib/libFLAC.so.8.2.0
	$(ROMFSINST) -s libFLAC.so.8.2.0 /lib/libFLAC.so.8
	$(ROMFSINST) -s libFLAC.so.8.2.0 /lib/libFLAC.so

clean:
	rm -rf *~ install.log .sgbuilt_lib
	rm -rf build*
	-$(MAKE) -C $(VER) distclean

.PHONY: all clean romfs
