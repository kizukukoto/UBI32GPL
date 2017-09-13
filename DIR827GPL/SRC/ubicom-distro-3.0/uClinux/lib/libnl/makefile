CONFOPTS = --prefix=

LDLIBS += -lm
LIBNLCFLAGS = -I$(ROOTDIR)/lib/libnl/include

all: build
	$(MAKE) -f Makefile

build: makefile
	CC="$(CC) $(LIBNLCFLAGS) $(CFLAGS)" LDFLAGS="$(LDFLAGS)" LIBS="$(LDLIBS)" \
	./configure $(CONFIGURE_OPTS) $(CONFOPTS)
	touch build

clean:
	$(MAKE) -f Makefile clean
	rm -rf build

romfs:
#ifdef CONFIG_UCLINUX
#	$(ROMFSINST) lib/libnl.a /lib/libnl.a
#else
#	$(ROMFSINST) lib/libnl.so.1 /lib/libnl.so.1
#endif

