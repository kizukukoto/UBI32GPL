
all: mcastd
install:
	$(MAKE) -C mcastd install
clean:
	rm -fr build
	make clean -C mcastd

mcastd/.unpacked:
	svn co $(SVNPACKAGE)/mcastd mcastd
	touch $@

mcastd/.configured:mcastd/.unpacked
	touch $@

build/lib/libcrypto.a:
	$(MAKE) -C mcastd

mcastd: mcastd/.configured
	$(MAKE) -C mcastd

.PHONY: mcastd


romfs:
#	$(MAKE) -C build romfs
	$(ROMFSINST) mcastd/mcastd /sbin/mcastd
	$(ROMFSINST) mac_filter.sh /sbin/mac_filter.sh

