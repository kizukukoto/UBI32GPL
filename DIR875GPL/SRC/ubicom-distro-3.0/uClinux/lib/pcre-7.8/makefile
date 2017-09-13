#
# build using configure

CONF_OPTS+=--host=$(CONFIGURE_HOST)
CONF_OPTS+=$(LIB_ENABLE_SHARED)
CONF_OPTS+=$(LIB_ENABLE_STATIC)
CONF_OPTS+=--with-gnu-ld
CONF_OPTS+=--with-pic
CONF_OPTS+=--disable-fast-install
CONF_OPTS+=--prefix=$(ROOTDIR)/lib/pcre-7.8/builddir/

LDFLAGS_FOR_SUBMAKE=-shared
 
all: builddir/config.status
	$(MAKE) -C builddir MAKE="$(MAKE) -f Makefile" LDFLAGS="$(LDFLAGS) $(LDFLAGS_FOR_SUBMAKE)"
	$(MAKE) -C builddir install MAKE="$(MAKE) -f Makefile"

romfs:
	$(ROMFSINST) -d -e CONFIG_BINFMT_ELF_FDPIC builddir/lib/libpcre.so.0.0.1 /usr/lib/libpcre.so.0


builddir/config.status: makefile	
	rm -rf builddir;					\
	chmod u+x configure;					\
	mkdir builddir; 					\
	cd builddir;						\
	../configure $(CONF_OPTS)

clean:
	rm -rf builddir

