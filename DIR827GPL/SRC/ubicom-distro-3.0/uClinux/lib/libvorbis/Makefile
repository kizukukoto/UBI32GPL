# libvorbis makefile for Ubicom32
VER = libvorbis-1.2.3

all:
	if [ ! -e install.log ]; then \
		cd $(VER) ; \
		./configure  \
		--host=$(CONFIGURE_HOST) \
		--target=$(CONFIGURE_HOST) \
		--disable-nls \
		--disable-oggtest \
		--with-ogg=$(ROOTDIR)/lib/libogg/libogg-1.1.4 \
		--with-ogg-libraries=$(ROOTDIR)/lib/libogg/build-ubicom/lib \
		--with-ogg-includes=$(ROOTDIR)/lib/libogg/build-ubicom/include \
		--prefix=$(ROOTDIR)/lib/libvorbis/build-ubicom ; \
	fi

	$(MAKE) -C $(VER)
	$(MAKE) -C $(VER) install > install.log

clean:
	rm -rf *~ install.log .sgbuilt_lib
	rm -rf build*
	-$(MAKE) -C $(VER) distclean

romfs:
	$(ROMFSINST) -d -e CONFIG_BINFMT_ELF_FDPIC $(ROOTDIR)/lib/libvorbis/build-ubicom/lib/libvorbis.so.0.4.3 /lib/libvorbis.so.0.4.3
	$(ROMFSINST) -s libvorbis.so.0.4.3 /lib/libvorbis.so.0
	$(ROMFSINST) -s libvorbis.so.0.4.3 /lib/libvorbis.so
	$(ROMFSINST) -d -e CONFIG_BINFMT_ELF_FDPIC $(ROOTDIR)/lib/libvorbis/build-ubicom/lib/libvorbisenc.so.2.0.6 /lib/libvorbisenc.so.2.0.6
	$(ROMFSINST) -s libvorbisenc.so.2.0.6 /lib/libvorbisenc.so.2
	$(ROMFSINST) -s libvorbisenc.so.2.0.6 /lib/libvorbisenc.so
	$(ROMFSINST) -d -e CONFIG_BINFMT_ELF_FDPIC $(ROOTDIR)/lib/libvorbis/build-ubicom/lib/libvorbisfile.so.3.3.2 /lib/libvorbisfile.so.3.3.2
	$(ROMFSINST) -s libvorbisfile.so.3.3.2 /lib/libvorbisfile.so.3
	$(ROMFSINST) -s libvorbisfile.so.3.3.2 /lib/libvorbisfile.so

.PHONY: all clean romfs
