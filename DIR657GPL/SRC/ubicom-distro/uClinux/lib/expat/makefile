#
# build using configure

CONF_OPTS+=--host=$(CONFIGURE_HOST)
CONF_OPTS+=$(LIB_ENABLE_SHARED)
CONF_OPTS+=--enable-static
CONF_OPTS+=--with-gnu-ld
CONF_OPTS+=--with-pic
CONF_OPTS+=--disable-fast-install
CONF_OPTS+=--prefix=$(ROOTDIR)/lib/expat/build

LDFLAGS_FOR_SUBMAKE=-shared

all: build/config.status
	$(MAKE) -C build MAKE="$(MAKE) -f Makefile" LDFLAGS="$(LDFLAGS) $(LDFLAGS_FOR_SUBMAKE)"
	#$(MAKE) -C build install MAKE="$(MAKE) -f Makefile"

romfs:
	$(ROMFSINST) -d -e CONFIG_BINFMT_ELF_FDPIC build/.libs/libexpat.so.0.5.0 /usr/lib/libexpat.so.0


build/config.status: makefile
	rm -rf build;           \
	chmod u+x configure;    \
	mkdir build;            \
	cd build;               \
	../configure $(CONF_OPTS)

clean:
	rm -rf build


