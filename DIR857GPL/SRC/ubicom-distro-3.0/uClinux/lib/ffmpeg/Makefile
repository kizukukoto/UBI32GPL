# To create your own version, just unpack ffmpeg tarball here
# and update the VER variable.  Or export your own from their
# svn via:
# svn export -r <REV> svn://svn.mplayerhq.hu/ffmpeg/trunk ffmpeg-svn-<REV>
REVISION = 0.6.1
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
		--target-os="linux" \
		--enable-shared \
		--enable-static \
		--disable-debug \
		--enable-gpl \
		--disable-libfaad \
		--disable-optimizations \
		--enable-small \
		--disable-stripping \
		--enable-zlib \
		--enable-pthreads \
		--enable-postproc \
		--disable-mmx \
		--disable-mmx2 \
		--disable-ffmpeg \
		--disable-ffserver \
		--disable-bsfs \
		--disable-devices \
		--disable-encoders \
		--disable-decoders \
		--enable-decoder="aac" \
		--enable-decoder="ac3" \
		--enable-decoder="atrac3" \
		--enable-decoder="h261" \
		--enable-decoder="h263" \
		--enable-decoder="h263i" \
		--enable-decoder="h264" \
		--enable-decoder="h264" \
		--enable-decoder="jpegls" \
		--enable-decoder="mp3" \
		--enable-decoder="mpeg1video" \
		--enable-decoder="mpeg2video" \
		--enable-decoder="mpeg4" \
		--enable-decoder="mpeg4aac" \
		--enable-decoder="mpegvideo" \
		--enable-decoder="png" \
		--enable-decoder="vc1" \
		--enable-decoder="wmav1" \
		--enable-decoder="wmav2" \
		--enable-decoder="wmv1" \
		--enable-decoder="wmv2" \
		--enable-decoder="wmv3" \
		--enable-decoder="zlib" \
		--disable-muxers \
		--disable-demuxers \
		--enable-demuxer="aac" \
		--enable-demuxer="ac3" \
		--enable-demuxer="asf" \
		--enable-demuxer="avi" \
		--enable-demuxer="flac" \
		--enable-demuxer="h261" \
		--enable-demuxer="h263" \
		--enable-demuxer="h264" \
		--enable-demuxer="matroska" \
		--enable-demuxer="mov" \
		--enable-demuxer="mp3" \
		--enable-demuxer="mpegps" \
		--enable-demuxer="mpegts" \
		--enable-demuxer="mpegvideo" \
		--enable-demuxer="ogg" \
		--enable-demuxer="rm" \
		--enable-demuxer="rtsp" \
		--enable-demuxer="sdp" \
		--enable-demuxer="vc1" \
		--disable-parsers \
		--enable-parser="aac" \
		--enable-parser="ac3" \
		--enable-parser="h261" \
		--enable-parser="h263" \
		--enable-parser="h264" \
		--enable-parser="h264" \
		--enable-parser="mpegaudio" \
		--enable-parser="mpegvideo" \
		--enable-parser="mpeg4video" \
		--enable-parser="vc1" \
		--enable-parser="vc1t" \
		--disable-protocols \
		--enable-protocol="file" \
		$(FFMPEG_PARAM) \
		--disable-ffserver \
		--enable-memalign-hack; \
	fi

	$(MAKE) -C $(VER)
	$(MAKE) -C $(VER) install > install.log

clean:
	rm -rf *~ install.log .sgbuilt_lib
	rm -rf build*
	-$(MAKE) -C $(VER) distclean

romfs:
	$(ROMFSINST) -d -e CONFIG_BINFMT_ELF_FDPIC build-ubicom/lib/libavcodec.so.52.72.2 /lib/libavcodec.so.52.72.2
	$(ROMFSINST) -s libavcodec.so.52.72.2 /lib/libavcodec.so.52
	$(ROMFSINST) -s libavcodec.so.52.72.2 /lib/libavcodec.so
	$(ROMFSINST) -d -e CONFIG_BINFMT_ELF_FDPIC build-ubicom/lib/libavdevice.so.52.2.0 /lib/libavdevice.so.52.2.0
	$(ROMFSINST) -s libavdevice.so.52.2.0 /lib/libavdevice.so.52
	$(ROMFSINST) -s libavdevice.so.52.2.0 /lib/libavdevice.so
	$(ROMFSINST) -d -e CONFIG_BINFMT_ELF_FDPIC build-ubicom/lib/libavformat.so.52.64.2 /lib/libavformat.so.52.64.2
	$(ROMFSINST) -s libavformat.so.52.64.2 /lib/libavformat.so.52
	 $(ROMFSINST) -s libavformat.so.52.64.2 /lib/libavformat.so
	$(ROMFSINST) -d -e CONFIG_BINFMT_ELF_FDPIC build-ubicom/lib/libavutil.so.50.15.1 /lib/libavutil.so.50.15.1
	$(ROMFSINST) -s libavutil.so.50.15.1 /lib/libavutil.so.50
	$(ROMFSINST) -s libavutil.so.50.15.1 /lib/libavutil.so
#	$(ROMFSINST) -d -e CONFIG_BINFMT_ELF_FDPIC build/install/lib/libavfilter.so.0.4.0 /usr/lib/libavfilter.so.0
#	$(ROMFSINST) -d -e CONFIG_BINFMT_ELF_FDPIC build/install/lib/libswscale.so.0.7.1 /usr/lib/libswscale.so.0
#	$(ROMFSINST) -d -e CONFIG_BINFMT_ELF_FDPIC build/install/lib/libpostproc.so.51.2.0 /usr/lib/libpostproc.so.51
#	$(ROMFSINST) -d -e CONFIG_USER_FFMPEG_APP build/ffmpeg /usr/bin/ffmpeg
#	$(ROMFSINST) -d -e CONFIG_USER_FFPLAY_APP -e CONFIG_LIB_LIBSDL build/ffplay /usr/bin/ffplay
.PHONY: all clean romfs
