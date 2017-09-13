CONFTARGET := --host=ubicom32-linux-uclibc --build=i686-pc-linux-gnu --target=ubicom32-linux-uclibc
CONFOPTS += --with-readline --disable-nls

TARGETS += build/build

all: $(TARGETS)
	$(MAKE) -C build

build/build: makefile
	rm -rf build
	mkdir build
	(cd build; sh ../gdb/gdbserver/configure $(CONFTARGET) $(CONFOPTS))
	touch $@

clean:
	rm -rf build

romfs:
	$(ROMFSINST) -e CONFIG_USER_GDB_GDBSERVER build/gdbserver /bin/gdbserver
