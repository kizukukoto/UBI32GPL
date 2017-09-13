CONFOPTS = --disable-nls

all: build/build
	$(MAKE) -C build

build/build: makefile
	rm -rf build
	mkdir build
	rm -f libiberty
	ln -s ../libiberty/builld libiberty
	(cd build; sh ../../../user/gdb/bfd/configure $(CONFIGURE_OPTS) $(CONFOPTS))
	touch build/build

clean:
	rm -rf build

romfs:
