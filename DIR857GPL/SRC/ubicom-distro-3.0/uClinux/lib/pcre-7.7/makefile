#
# build using configure

CONF_OPTS += $(LIB_ENABLE_SHARED)
CONF_OPTS += $(LIB_ENABLE_STATIC)
CONF_OPTS += --host=ubicom32
CONF_OPTS += --disable-cpp

LDFLAGS_FOR_SUBMAKE=$(patsubst %crt1.o,,$(LDFLAGS))
 
all: builddir/config.status
	$(MAKE) -C builddir MAKE="$(MAKE) -f Makefile" LDFLAGS="$(LDFLAGS_FOR_SUBMAKE)"
	
romfs: builddir/config.status

builddir/config.status: makefile	
	rm -rf builddir;					\
	chmod u+x configure;					\
	mkdir builddir; 					\
	cd builddir;						\
	../configure $(CONF_OPTS)

clean:
	rm -rf builddir

