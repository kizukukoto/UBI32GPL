# To create your own version, just unpack ffmpeg tarball here
# and update the VER variable.  Or export your own from their
# svn via:
# svn export -r <REV> svn://svn.mplayerhq.hu/ffmpeg/trunk ffmpeg-svn-<REV>
REVISION = 3.7.2
VER = sqlite-$(REVISION)

all:
	if [ ! -e install.log ]; then \
		rm -rf build ; \
		mkdir build ; \
		cd build ; \
		../$(VER)/configure \
		--prefix=$$PWD/install \
		--host=$(CONFIGURE_HOST) \
		--disable-nls \
		$(LIB_ENABLE_STATIC) \
		$(LIB_ENABLE_SHARED) ; \
	fi

	$(MAKE) -C build
	$(MAKE) -C build install > install.log

clean:
	rm -f install.log
	rm -rf build

romfs:
	$(ROMFSINST) -d -e CONFIG_BINFMT_ELF_FDPIC build/install/lib/libsqlite3.so.0.8.6 /lib/libsqlite3.so.0.8.6
	$(ROMFSINST) -s libsqlite3.so.0.8.6 /lib/libsqlite3.so.0
	$(ROMFSINST) -s libsqlite3.so.0.8.6 /lib/libsqlite3.so
#	$(ROMFSINST) -d build/install/bin/sqlite3 /usr/bin/sqlite3

.PHONY: all clean romfs
