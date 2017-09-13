# Alsa-lib makefile for Ubicom32 
VER = alsa-lib-1.0.20

all: $(VER)/Makefile
	$(MAKE) -C $(VER)
	$(MAKE) -C $(VER) install > install.log

$(VER)/Makefile: $(VER)/configure Makefile
	find $(VER) -type f -print0 | xargs -0 touch -r $(VER)/configure
	set -e ; \
	cd $(VER) ; \
	LIBS="-lc -lgcc" \
	./configure \
		--host=$(CONFIGURE_HOST) \
		--with-gnu-ld=yes \
		--enable-static \
		--disable-python \
		--prefix=$(ROOTDIR)/lib/libalsa/build-ubicom \

clean:
	-$(MAKE) -C $(VER) distclean
	rm -rf build*

romfs:
	$(ROMFSINST) -d build-ubicom/share /usr/share
	$(ROMFSINST) -d build-ubicom/lib/libasound.so.2.0.0 /lib
	$(ROMFSINST) -s libasound.so.2.0.0 /lib/libasound.so
	$(ROMFSINST) -s libasound.so.2.0.0 /lib/libasound.so.2

.PHONY: all clean romfs
