VER = giflib-4.1.6

CONF_OPTS += $(LIB_ENABLE_SHARED) $(LIB_ENABLE_STATIC)
CONF_OPTS += --host=ubicom32
CONF_OPTS += --disable-cpp
LDFLAGS_FOR_SUBMAKE=$(patsubst %crt1.o,,$(LDFLAGS))

all: build/Makefile
	$(MAKE) -C build/lib LDFLAGS="$(LDFLAGS_FOR_SUBMAKE)"
	cp build/lib/.libs/libgif.a .
	cp $(VER)/lib/gif_lib.h $(ROOTDIR)/include

build/Makefile:
	find $(VER) -type f -print0 | xargs -0 touch -r $(VER)/configure
	set -e
	rm -rf build
	mkdir build
	cd build; \
	export LDFLAGS="$(LDFLAGS)" LIBS="$(LDLIBS)"; \
	CC="$(CC) $(CFLAGS)" ../$(VER)/configure $(CONF_OPTS)

clean:
	rm -rf build-$(VER)
	rm libgif.a
	rm $(ROOTDIR)/include/gif_lib.h

romfs: 
