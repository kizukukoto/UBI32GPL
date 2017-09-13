# To create your own version, just unpack ffmpeg tarball here
# and update the VER variable.  Or export your own from their
# svn via:
# svn export -r <REV> svn://svn.mplayerhq.hu/ffmpeg/trunk ffmpeg-svn-<REV>
REVISION = 7.19.4
VER = curl-$(REVISION)

all: build/Makefile
	$(MAKE) -C build
	$(MAKE) -C build install > install.log

build/Makefile:
	find $(VER) -type f -print0 | xargs -0 touch -r $(VER)/configure
	set -e ; \
	rm -rf build ; \
	mkdir build ; \
	cd build ; \
	../$(VER)/configure \
	--prefix=$$PWD/install \
	--host=$(CONFIGURE_HOST) \
	--disable-optimize \
	--enable-warnings \
	--enable-dependency-tracking \
	$(LIB_ENABLE_STATIC) \
	$(LIB_ENABLE_SHARED) \
	--disable-fast-install \
	--disable-libtool-lock \
	--enable-http \
	--enable-ftp \
	--enable-file \
	--enable-ldap \
	--enable-ldaps \
	--enable-proxy \
	--enable-telnet \
	--enable-dict \
	--enable-tftp \
	--enable-manual \
	--enable-ipv6 \
	--enable-thread \
	--enable-nonblocking \
	--disable-ares \
	--enable-verbose \
	--enable-sspi \
	--disable-crypto-auth \
	--enable-cookies \
	--enable-hidden-symbols \
	--enable-soname-bump \
	--with-gnu-ld \
	--with-pic \
	--with-zlib \
	--with-ssl \
	--with-libssh2 \
	--with-gnupg \
	--with-gnutls=$(ROOTDIR)/lib/GnuTLS/build-ubicom/install \
	--with-random=/dev/urandom \
	--without-nss \
	--without-ca-bundle \
	--without-libidn

clean:
	rm -rf build

romfs:
	$(ROMFSINST) -d -e CONFIG_BINFMT_ELF_FDPIC build/install/lib/libcurl.so.5.1.1 /usr/lib/libcurl.so.5
	$(ROMFSINST) -d build/install/bin/curl /usr/bin/curl

.PHONY: all clean romfs
