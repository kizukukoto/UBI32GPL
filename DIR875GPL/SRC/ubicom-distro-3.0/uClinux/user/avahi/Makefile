# makefile for Ubicom32
VER = avahi-0.6.30

all:
	if [ ! -e install.log ]; then \
		cd $(VER) ; \
		export CFLAGS="$(CFLAGS) -L$(ROOTDIR)/user/expat/expat-2.0.1/.libs -lexpat -I$(ROOTDIR)/user/expat/expat-2.0.1/lib -I$(ROOTDIR)/user/libdaemon/libdaemon-0.14" && \
		./configure \
		--host=$(CONFIGURE_HOST) \
		--prefix=$(ROOTDIR)/user/avahi/build \
		LIBDAEMON_LIBS=-L$(ROOTDIR)/user/libdaemon/libdaemon-0.14/libdaemon/.libs LIBDAEMON_CFLAGS=-ldaemon \
		CXX=gcc INSTALL= --with-gnu-ld --disable-glib --disable-gobject --disable-dbus \
		--disable-stack-protector --with-distro=none --disable-gdbm \
		--disable-qt3 --disable-qt4 --disable-gtk --disable-gtk3 --disable-pygtk \
		--disable-python --disable-mono --disable-monodoc --disable-autoipd \
		--disable-manpages --with-avahi-user=root --with-avahi-group=root --localstatedir=/var/tmp \
		--sysconfdir=/var/etc || exit 1; \
	fi

	$(MAKE) -C $(VER)
	$(MAKE) -C $(VER) install > install.log

clean:
	rm -rf *~ install.log .sgbuilt_lib
	rm -rf build*
	-$(MAKE) -C $(VER) distclean

romfs:
	$(ROMFSINST)  build/sbin/avahi-daemon /bin/avahi-daemon
	$(ROMFSINST)  build/lib/libavahi-common.so.3 /lib/libavahi-common.so.3
	$(ROMFSINST)  build/lib/libavahi-core.so.7 /lib/libavahi-core.so.7
.PHONY: all clean romfs
