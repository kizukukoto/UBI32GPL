# To create your own version, just unpack ffmpeg tarball here
# and update the VER variable.  Or export your own from their
# svn via:
# svn export -r <REV> svn://svn.mplayerhq.hu/ffmpeg/trunk ffmpeg-svn-<REV>
REVISION = 0.5.2
VER = ffmpeg-$(REVISION)

LIBSDL_PREFIX:=$(shell pkg-config --variable=prefix sdl)

FFMPEG_PARAM=--disable-ffmpeg
FFPLAY_PARAM=--disable-ffplay

ifdef CONFIG_USER_FFMPEG_APP
	FFMPEG_PARAM=
endif
ifdef CONFIG_USER_FFPLAY_APP
	FFPLAY_PARAM=
endif

all:
	if [ ! -e install.log ]; then \
		cd $(VER); \
		echo "#define FFMPEG_VERSION \"$(REVISION)\"" > version.h ; \
		./configure \
		--prefix=$(ROOTDIR)/lib/ffmpeg/build-ubicom \
		--enable-gpl \
		--enable-cross-compile \
		--cross-prefix=$(CROSS_COMPILE) \
		--arch=$(ARCH) \
		--enable-shared \
		--enable-static \
		--disable-debug \
		--enable-gpl \
		--disable-libfaad \
		--disable-optimizations \
		--enable-small \
		--disable-stripping \
		--disable-vhook \
		--enable-zlib \
		--disable-ffserver \
		--disable-ipv6 \
		--enable-pthreads \
		--enable-postproc \
		--disable-mmx \
		--disable-mmx2 \
		--disable-ipv6 \
		$(FFMPEG_PARAM) \
		--disable-ffserver ; \
	fi

	$(MAKE) -C $(VER)
	$(MAKE) -C $(VER) install > install.log

clean:
	rm -rf *~ install.log .sgbuilt_lib
	rm -rf build*
	-$(MAKE) -C $(VER) distclean

romfs:
	$(ROMFSINST) -d -e CONFIG_BINFMT_ELF_FDPIC build-ubicom/lib/libavcodec.so.52.20.1 /lib/libavcodec.so.52.20.1
	$(ROMFSINST) -s libavcodec.so.52.20.1 /lib/libavcodec.so.52
	$(ROMFSINST) -s libavcodec.so.52.20.1 /lib/libavcodec.so
	$(ROMFSINST) -d -e CONFIG_BINFMT_ELF_FDPIC build-ubicom/lib/libavdevice.so.52.1.0 /lib/libavdevice.so.52.1.0
	$(ROMFSINST) -s libavdevice.so.52.1.0 /lib/libavdevice.so.52
	$(ROMFSINST) -s libavdevice.so.52.1.0 /lib/libavdevice.so
	$(ROMFSINST) -d -e CONFIG_BINFMT_ELF_FDPIC build-ubicom/lib/libavformat.so.52.31.0 /lib/libavformat.so.52.31.0
	$(ROMFSINST) -s libavformat.so.52.31.0 /lib/libavformat.so.52
	 $(ROMFSINST) -s libavformat.so.52.31.0 /lib/libavformat.so
	$(ROMFSINST) -d -e CONFIG_BINFMT_ELF_FDPIC build-ubicom/lib/libavutil.so.49.15.0 /lib/libavutil.so.49.15.0
	$(ROMFSINST) -s libavutil.so.49.15.0 /lib/libavutil.so.49
	$(ROMFSINST) -s libavutil.so.49.15.0 /lib/libavutil.so
#	$(ROMFSINST) -d -e CONFIG_BINFMT_ELF_FDPIC build/install/lib/libavfilter.so.0.4.0 /usr/lib/libavfilter.so.0
#	$(ROMFSINST) -d -e CONFIG_BINFMT_ELF_FDPIC build/install/lib/libswscale.so.0.7.1 /usr/lib/libswscale.so.0
#	$(ROMFSINST) -d -e CONFIG_BINFMT_ELF_FDPIC build/install/lib/libpostproc.so.51.2.0 /usr/lib/libpostproc.so.51
#	$(ROMFSINST) -d -e CONFIG_USER_FFMPEG_APP build/ffmpeg /usr/bin/ffmpeg
#	$(ROMFSINST) -d -e CONFIG_USER_FFPLAY_APP -e CONFIG_LIB_LIBSDL build/ffplay /usr/bin/ffplay
.PHONY: all clean romfs
