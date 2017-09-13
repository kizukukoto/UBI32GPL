VER = glib-2.21.3

CONF_OPTS = \
	--prefix=$(ROOTDIR)/lib/glib/build-ubicom/install \
	--host=$(CONFIGURE_HOST) \
	--disable-maintainer-mode \
	--disable-dependency-tracking \
	--enable-debug=minimum \
	--disable-gc-friendly \
	--enable-threads \
	--disable-rebuilds \
	--disable-visibility \
	--disable-largefile \
	$(LIB_ENABLE_STATIC) \
	$(LIB_ENABLE_SHARED) \
	--disable-fast-install \
	--disable-libtool-lock \
	--disable-selinux \
	--disable-fam \
	--disable-xattr \
	--enable-regex \
	--disable-gtk-doc \
	--disable-man \
	--with-libiconv=gnu \
	--with-pic \
	--with-gnu-ld \
	--without-gio-module-dir \
	--with-threads=posix \
	--with-pcre \
	--without-html-dir \
	--without-xml-catalog \
	LIBS="-lintl -liconv -ldl"

all: build-ubicom/Makefile
	$(MAKE) -C build-ubicom
	$(MAKE) -C build-ubicom install > install.log

build-ubicom/Makefile: Makefile
	find $(VER) -type f -print0 | xargs -0 touch -r $(VER)/configure
	set -e ; \
	rm -rf build-ubicom ; \
	mkdir build-ubicom ; \
	cd build-ubicom ; \
	../$(VER)/configure $(CONF_OPTS)

clean:
	rm -rf build*

romfs:
	$(ROMFSINST) -d -e CONFIG_BINFMT_ELF_FDPIC $(ROOTDIR)/lib/glib/build-ubicom/install/lib/libgio-2.0.so.0.2103.0 /usr/lib/libgio-2.0.so.0
	$(ROMFSINST) -d -e CONFIG_BINFMT_ELF_FDPIC $(ROOTDIR)/lib/glib/build-ubicom/install/lib/libglib-2.0.so.0.2103.0 /usr/lib/libglib-2.0.so.0
	$(ROMFSINST) -d -e CONFIG_BINFMT_ELF_FDPIC $(ROOTDIR)/lib/glib/build-ubicom/install/lib/libgmodule-2.0.so.0.2103.0 /usr/lib/libgmodule-2.0.so.0
	$(ROMFSINST) -d -e CONFIG_BINFMT_ELF_FDPIC $(ROOTDIR)/lib/glib/build-ubicom/install/lib/libgobject-2.0.so.0.2103.0 /usr/lib/libgobject-2.0.so.0
	$(ROMFSINST) -d -e CONFIG_BINFMT_ELF_FDPIC $(ROOTDIR)/lib/glib/build-ubicom/install/lib/libgthread-2.0.so.0.2103.0 /usr/lib/libgthread-2.0.so.0

.PHONY: all clean romfs
