VER = dbus-1.2.16

BUILD_DIR:=build-ubicom
DBUS_PREFIX:=$(ROOTDIR)/lib/dbus/$(BUILD_DIR)/install

all: $(BUILD_DIR)/Makefile
	$(MAKE) -C $(BUILD_DIR)
	$(MAKE) -C $(BUILD_DIR) install > install.log

$(BUILD_DIR)/Makefile: $(VER)/configure Makefile
	set -e ; \
	rm -rf $(BUILD_DIR) ; \
	mkdir $(BUILD_DIR) ; \
	cd $(BUILD_DIR) ; \
	CFLAGS="$(CFLAGS) -I$(ROOTDIR)/linux-2.6.x/include -I$$PWD/include" \
	LDFLAGS="$(LDFLAGS) -L$$PWD/install/lib" \
	LIBS="-lc -lgcc" \
	../$(VER)/configure \
	--localedir=/var \
	--sysconfdir=/etc \
	--host=$(CONFIGURE_HOST) \
	$(LIB_ENABLE_SHARED) \
	$(LIB_ENABLE_STATIC) \
	--with-gnu-ld \
	--with-pic \
	--disable-abstract-sockets \
	--disable-xml-docs \
	--disable-selinux \
	--disable-doxygen-docs \
	--disable-fast-install \
	--enable-dependency-tracking \
	--enable-tests \
	--disable-checks \
	--disable-ansi \
	--disable-verbose-mode \
	--disable-gcov \
	--disable-libaudit \
	--disable-dnotify \
	--disable-inotify \
	--disable-kqueue \
	--disable-asserts \
	--disable-console-owner-file \
	--disable-userdb-cache \
	--without-init-scripts \
	--without-console-owner-file \
	--with-dbus-user=root \
	--with-dbus-daemondir=/usr/sbin \
	--with-system-socket=/var/run/dbus/system_bus_socket \
	--with-system-pid-file=/var/run/dbus.pid \
	--disable-libtool-lock \
	--with-xml=expat \
	--without-x \
	--prefix=$(DBUS_PREFIX) \
	EXPAT_CFLAGS="$(pkg-config --cflags expat)" \
	EXPAT_LIBS="$(pkg-config --libs expat)"

clean:
	rm -rf build*

romfs:
	$(ROMFSINST) -d -e CONFIG_BINFMT_ELF_FDPIC $(DBUS_PREFIX)/lib/libdbus-1.so.3.4.0 /usr/lib/libdbus-1.so.3
	$(ROMFSINST) -d $(DBUS_PREFIX)/bin/dbus-uuidgen /usr/sbin/dbus-uuidgen
	$(ROMFSINST) -d $(DBUS_PREFIX)/bin/dbus-send /usr/sbin/dbus-send
	$(ROMFSINST) -d $(BUILD_DIR)/bus/dbus-daemon /usr/sbin/dbus-daemon
	$(ROMFSINST) -d $(BUILD_DIR)/bus/system.conf /etc/dbus-1/system.conf
	$(ROMFSINST) -d $(BUILD_DIR)/bus/session.conf /etc/dbus-1/session.conf
	test -d $(ROMFSDIR)/dbus-1/session.d || mkdir -p $(ROMFSDIR)/etc/dbus-1/session.d
	test -d $(ROMFSDIR)/dbus-1/system.d  || mkdir -p $(ROMFSDIR)/etc/dbus-1/system.d

.PHONY: all clean romfs
