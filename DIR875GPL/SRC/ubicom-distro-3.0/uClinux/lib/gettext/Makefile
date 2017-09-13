VER = gettext-0.18.1.1

CONF_OPTS = \
	--prefix=$(ROOTDIR)/lib/gettext/build-ubicom/install \
	--host=$(CONFIGURE_HOST) \
	--disable-dependency-tracking \
	$(LIB_ENABLE_STATIC) \
	$(LIB_ENABLE_SHARED) \
	--disable-fast-install \
	--disable-libtool-lock \
	--disable-csharp \
	--disable-relocatable \
	--enable-nls \
	--disable-rpath \
	--disable-libasprintf \
	--with-included-gettext \
	--without-libintl-prefix \
	--without-included-regex \
	--without-libexpat-prefix \
	--without-lispdir \
	--without-emacs \
	--disable-openmp \
	--disable-libasprintf \
	--disable-java \
	--disable-native-java 


all:
	if [ ! -e install.log ]; then \
		rm -rf build-ubicom ; \
		mkdir build-ubicom ; \
		cd $(VER); \
		./configure $(CONF_OPTS) ; \
	fi

	$(MAKE) -C $(VER)
	$(MAKE) -C $(VER) install > install.log

clean:
	rm -rf build*
	rm -f install.log

romfs:
	$(ROMFSINST) -d -e CONFIG_BINFMT_ELF_FDPIC $(ROOTDIR)/lib/gettext/build-ubicom/install/lib/libintl.so.8.1.1 /lib/libintl.so.8.1.1
	$(ROMFSINST) -s libintl.so.8.1.1 /lib/libintl.so.8
	$(ROMFSINST) -s libintl.so.8.1.1 /lib/libintl.so
#$(ROMFSINST) -d -e CONFIG_BINFMT_ELF_FDPIC $(ROOTDIR)/lib/gettext/build-ubicom/install/lib/libgettextlib-0.14.6.so /usr/lib/libgettextlib.so
#$(ROMFSINST) -d -e CONFIG_BINFMT_ELF_FDPIC $(ROOTDIR)/lib/gettext/build-ubicom/install/lib/libgettextpo.so.0.1.0 /usr/lib/libgettextpo.so.0
#$(ROMFSINST) -d -e CONFIG_BINFMT_ELF_FDPIC $(ROOTDIR)/lib/gettext/build-ubicom/install/lib/libgettextsrc-0.14.6.so /usr/lib/libgettextsrc.so

.PHONY: all clean romfs
