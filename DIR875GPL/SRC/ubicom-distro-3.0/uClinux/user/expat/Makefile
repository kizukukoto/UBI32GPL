# makefile for Ubicom32
VER = expat-2.0.1

all:
	if [ ! -e install.log ]; then \
		cd $(VER) ; \
		./configure \
		--host=$(CONFIGURE_HOST) \
		--prefix=$(ROOTDIR)/user/expat/build \
		INSTALL= ; \
	fi

	$(MAKE) -C $(VER)
	$(MAKE) -C $(VER) install > install.log

clean:
	rm -rf *~ install.log .sgbuilt_lib
	rm -rf build*
	-$(MAKE) -C $(VER) distclean

romfs:
	$(ROMFSINST)  build/lib/libexpat.so.1 /lib/libexpat.so.1
.PHONY: all clean romfs
