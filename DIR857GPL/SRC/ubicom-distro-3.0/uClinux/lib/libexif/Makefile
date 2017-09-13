# To create your own version, just unpack ffmpeg tarball here
# and update the VER variable.  Or export your own from their
# svn via:
# svn export -r <REV> svn://svn.mplayerhq.hu/ffmpeg/trunk ffmpeg-svn-<REV>
REVISION = 0.6.19
VER = libexif-$(REVISION)

all:
	if [ ! -e install.log ]; then \
		cd $(VER); \
		./configure \
		--host=$(CONFIGURE_HOST) \
		--target=$(CONFIGURE_HOST) \
		--disable-dependency-tracking \
		--disable-fast-install \
		--disable-libtool-lock \
		--enable-shared \
		--enable-static \
		--disable-rpath \
		--without-libiconv-prefix \
		--without-libintl-prefix \
		--prefix=$(ROOTDIR)/lib/libexif/build-ubicom ; \
	fi

	$(MAKE) -C $(VER)
	$(MAKE) -C $(VER) install > install.log

clean:
	rm -rf *~ install.log .sgbuilt_lib
	rm -rf build*
	-$(MAKE) -C $(VER) distclean

romfs:
	$(ROMFSINST) -d -e CONFIG_BINFMT_ELF_FDPIC $(ROOTDIR)/lib/libexif/build-ubicom/lib/libexif.so.12.3.1 /lib/libexif.so.12.3.1
	$(ROMFSINST) -s libexif.so.12.3.1 /lib/libexif.so.12
	$(ROMFSINST) -s libexif.so.12.3.1 /lib/libexif.so

.PHONY: all clean romfs
