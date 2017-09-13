# libusb makefile for Ubicom32
VER = libusb-0.1.12

all: $(VER)/Makefile
	$(MAKE) -C $(VER)
	$(MAKE) -C $(VER) install > install.log

$(VER)/Makefile: $(VER)/configure Makefile
	find $(VER) -type f -print0 | xargs -0 touch -r $(VER)/configure
	set -e ; \
	cd $(VER) ; \
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
		--prefix=$(ROOTDIR)/lib/libusb/build-ubicom ;
	./remove_cpp.sh $(VER)
	

clean:
	rm -rf *~ install.log .sgbuilt_lib
	rm -rf build*
	-$(MAKE) -C $(VER) distclean

romfs:
	$(ROMFSINST) -d -e CONFIG_BINFMT_ELF_FDPIC $(ROOTDIR)/lib/libusb/build-ubicom/lib/libusb-0.1.so.4.4.4 /lib/libusb-0.1.so.4


.PHONY: all clean romfs
