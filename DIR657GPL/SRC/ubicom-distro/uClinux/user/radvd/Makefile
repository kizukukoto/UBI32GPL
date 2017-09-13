# makefile for Ubicom32
VER = radvd-1.7

all:
	if [ ! -e install.log ]; then \
		cd $(VER) ; \
		./configure  \
		--host=$(CONFIGURE_HOST) \
		--target=$(CONFIGURE_HOST) \
		--program-prefix="" \
		--prefix=$(ROOTDIR)/user/radvd/build ; \
	fi

	$(MAKE) -C $(VER)
	$(MAKE) -C $(VER) install > install.log

clean:
	rm -rf *~ install.log .sgbuilt_lib
	rm -rf build*
	-$(MAKE) -C $(VER) distclean

romfs:
	$(ROMFSINST) $(VER)/radvd.conf /etc/radvd.conf
	$(ROMFSINST) -e CONFIG_USER_RADVD_RADVD build/sbin/radvd /bin/radvd
#	$(ROMFSINST) -e CONFIG_USER_RADVD_RADVD build/sbin/radvdump /bin/radvdump

.PHONY: all clean romfs
