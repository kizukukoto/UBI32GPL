# LiTE GUI makefile for Ubicom32 
VER = LiTE-0.8.10

#DIRECTFB_PREFIX:=$(shell pkg-config --variable=prefix directfb)
DIRECTFB_PREFIX:="$(ROOTDIR)/lib/DirectFB/build-ubicom/install"

#
# This flag changes some states of the textbuttons
# (heavily overrides BUTTON_HILITE state)
#
CFLAGS += -DUBICOM32_TOUCHSCREEN_INUSE

all: $(VER)/Makefile
	$(MAKE) -C $(VER)
	$(MAKE) -C $(VER) install > install.log

$(VER)/Makefile: $(VER)/configure Makefile
	find $(VER) -type f -print0 | xargs -0 touch -r $(VER)/configure
	set -e ; \
	cd $(VER) ; \
	DFB_CFLAGS="-D_REENTRANT -I$(DIRECTFB_PREFIX)/include/directfb" \
	DFB_LIBS="$(shell $(DIRECTFB_PREFIX)/bin/directfb-config --libs) -lc -lgcc" \
	./configure  \
		--host=$(CONFIGURE_HOST) \
		--disable-dependency-tracking \
		$(LIB_ENABLE_STATIC) \
		$(LIB_ENABLE_SHARED) \
		--disable-fast-install \
		--disable-libtool-lock \
		--disable-debug \
		--with-gnu-ld \
		--with-pic \
		--prefix=$(ROOTDIR)/lib/LiTE/build-ubicom \

clean:
	rm -rf build*
	-$(MAKE) -C $(VER) distclean

romfs:
	$(ROMFSINST) -d -e CONFIG_BINFMT_ELF_FDPIC $(ROOTDIR)/lib/LiTE/build-ubicom/lib/liblite.so.3.0.5 /usr/lib/liblite.so.3
	$(ROMFSINST) -d -e CONFIG_BINFMT_ELF_FDPIC $(ROOTDIR)/lib/LiTE/build-ubicom/lib/libleck.so.3.0.5 /usr/lib/libleck.so.3
	$(ROMFSINST) -d LiTE-0.8.10/fonts/whiterabbit.ttf	/share/fonts/truetype/whiterabbit.ttf
	$(ROMFSINST) -d LiTE-0.8.10/fonts/vera.ttf		/share/fonts/truetype/vera.ttf
	$(ROMFSINST) -d LiTE-0.8.10/fonts/verabd.ttf		/share/fonts/truetype/verabd.ttf
	$(ROMFSINST) -d LiTE-0.8.10/data/cursor.png		/share/LiTE/cursor.png
	$(ROMFSINST) -d LiTE-0.8.10/data/obenlinks.png		/share/LiTE/obenlinks.png
	$(ROMFSINST) -d LiTE-0.8.10/data/obenrechts.png		/share/LiTE/obenrechts.png
	$(ROMFSINST) -d LiTE-0.8.10/data/untenrechts.png	/share/LiTE/untenrechts.png
	$(ROMFSINST) -d LiTE-0.8.10/data/links.png		/share/LiTE/links.png
	$(ROMFSINST) -d LiTE-0.8.10/data/untenlinks.png		/share/LiTE/untenlinks.png
	$(ROMFSINST) -d LiTE-0.8.10/data/oben.png		/share/LiTE/oben.png
	$(ROMFSINST) -d LiTE-0.8.10/data/rechts.png		/share/LiTE/rechts.png
	$(ROMFSINST) -d LiTE-0.8.10/data/unten.png		/share/LiTE/unten.png
	$(ROMFSINST) -d LiTE-0.8.10/examples/textbuttonbgnd.png	/share/LiTE/textbuttonbgnd.png
	$(ROMFSINST) -d LiTE-0.8.10/examples/scrollbar.png	/share/LiTE/scrollbar.png

.PHONY: all clean romfs
