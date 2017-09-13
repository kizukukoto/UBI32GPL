
CONFOPTS := --enable-static --enable-shlib

all: build/build
	$(MAKE) -C build
	$(MAKE) -C build prefix=$(ROOTDIR)/ DESTDIR=$(shell pwd)/build/install install

build/build: makefile
	@chmod u+x configure
	@rm -rf build
	@mkdir build
	(cd build; sh ../configure $(CONFIGURE_OPTS) $(CONFOPTS))
	@touch build/build

clean:
	rm -rf build

romfs:
	$(ROMFSINST) build/dmalloc /bin/dmalloc
	$(ROMFSINST) build/libdmalloc.so /lib/libdmalloc.so

