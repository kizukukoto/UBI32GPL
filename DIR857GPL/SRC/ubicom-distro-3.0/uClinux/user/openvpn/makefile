CONFOPTS=--enable-password-save

all: build/build
	$(MAKE) -C build

build/build: makefile
	rm -rf build
	mkdir build
	(cd build; sh ../configure $(CONFIGURE_OPTS) $(CONFOPTS) \
		--disable-lzo \
		--with-ssl-headers=$(ROOTDIR)/lib/libssl/build/install/include/ \
		--disable-ntlm --enable-small -disable-nls --disable-pthread --disable-debug --disable-socks \
	)

		#--disable-ntlm --enable-small -disable-nls --disable-pthread --disable-debug --disable-socks

	touch build/build

clean:
	rm -rf build

romfs:
	$(ROMFSINST) build/openvpn /bin/openvpn
