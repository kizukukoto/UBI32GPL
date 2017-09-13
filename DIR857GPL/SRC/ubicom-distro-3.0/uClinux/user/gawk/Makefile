VER = gawk-3.1.6

#for debugging purposes
CFLAGS += -O0

all: build-$(VER)/Makefile
	$(MAKE) -C build-$(VER) install DESTDIR=.

build-$(VER)/Makefile:
	find $(VER) -type f -print0 | xargs -0 touch -r $(VER)/configure
	set -e ; \
	rm -rf build-$(VER) ; \
	mkdir build-$(VER) ; \
	cd build-$(VER) ; \
	CC="$(CC) $(CFLAGS)" LDFLAGS="$(LDFLAGS)" LIBS="$(LDLIBS)" ac_cv_func_malloc_0_nonnull=yes ac_cv_func_realloc_0_nonnull=yes ../$(VER)/configure $(CONFIGURE_OPTS)

clean:
	rm -rf build*

romfs:
	$(ROMFSINST) build-$(VER)/usr /usr

.PHONY: all clean romfs

