# makefile for Ubicom32
VER = libdaemon-0.14

all:
	if [ ! -e install.log ]; then \
		cd $(VER) ; \
		echo "ac_cv_func_setpgrp_void=yes" > config.cache ; \
		./configure --host=$(CONFIGURE_HOST) \
		--target=$(CONFIGURE_HOST) \
		--prefix=$(ROOTDIR)/user/libdaemon/build --config-cache \
		INSTALL= --disable-lynx ;\
	fi

	$(MAKE) -C $(VER)
	$(MAKE) -C $(VER) install > install.log

clean:
	rm -rf *~ install.log .sgbuilt_lib
	rm -rf build*
	-$(MAKE) -C $(VER) distclean

romfs:
	$(ROMFSINST) build/lib/libdaemon.so.0 /lib/libdaemon.so.0

.PHONY: all clean romfs
