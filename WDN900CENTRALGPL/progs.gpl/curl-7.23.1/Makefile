
##############################################################################
# Makefile for cURL
# Created by Yi-hung <yi_hung@alphanetworks.com>
##############################################################################
include ../arch.mk
include ../../path.mk
include ../../.config

VERBOSE := > /dev/null
APPLET := curl
VERSION := 7.23.1
TARBALL := $(APPLET)-$(VERSION).tar.bz2
PATCHFILE := $(TARBALL).diff
SRCROOT := $(APPLET)-$(VERSION)
SOURCEDIR := $(CURDIR)
BUILDDIR := $(CURDIR)/build
ROOTFS := $(TARGET)
MYJOBS := -j$(shell grep -c processor /proc/cpuinfo)

TARGET_CONFS := --build=$(shell uname -m)-unknown-linux-gnu
TARGET_CONFS += --host=$(HOST_TYPE)
TARGET_CONFS += --prefix=$(PREFIX)
TARGET_CONFS += --enable-shared
TARGET_CONFS += --enable-static
TARGET_CONFS += --disable-debug
TARGET_CONFS += --enable-optimize
TARGET_CONFS += --disable-warnings
TARGET_CONFS += --enable-werror
ifeq ($(ELBOX_PROGS_GPL_ZLIB_123),y)
ZLIB_ROOT = $(TOPDIR)/progs.gpl/zlib-1.2.3
TARGET_CONFS += --with-zlib=$(ZLIB_ROOT)
endif
TARGET_CONFS += --disable-ares
TARGET_CONFS += --enable-dependency-tracking
TARGET_CONFS += --enable-largefile
TARGET_CONFS += --disable-libtool-lock
TARGET_CONFS += --enable-http
TARGET_CONFS += --enable-ftp
TARGET_CONFS += --enable-file
TARGET_CONFS += --disable-ldap
TARGET_CONFS += --disable-ldaps
TARGET_CONFS += --enable-rtsp
TARGET_CONFS += --enable-proxy
TARGET_CONFS += --enable-dict
TARGET_CONFS += --enable-telnet
TARGET_CONFS += --enable-tftp
TARGET_CONFS += --enable-pop3
TARGET_CONFS += --enable-imap
TARGET_CONFS += --enable-smtp
TARGET_CONFS += --disable-gopher
TARGET_CONFS += --disable-manual
TARGET_CONFS += --enable-libgcc
ifeq ($(ELBOX_USE_IPV6),y)
TARGET_CONFS += --enable-ipv6
else
TARGET_CONFS += --disable-ipv6
endif
TARGET_CONFS += --enable-nonblocking
TARGET_CONFS += --enable-threaded-resolver
TARGET_CONFS += --disable-sspi
TARGET_CONFS += --enable-crypto-auth
TARGET_CONFS += --disable-tls-srp
TARGET_CONFS += --enable-cookies
TARGET_CONFS += --enable-hidden-symbols
TARGET_CONFS += --enable-soname-bump
TARGET_CONFS += --without-gnutls
TARGET_CONFS += --without-libssh2
TARGET_CONFS += --without-librtmp
TARGET_CONFS += --without-libidn
ifeq ($(ELBOX_PROGS_GPL_OPENSSL_098R),y)
TARGET_CONFS += --with-ssl=$(TOPDIR)/progs.gpl/openssl-0.9.8r/build/openssl-0.9.8r
TARGET_CONFS += --enable-crypto-auth
endif
TARGET_CONFS += CFLAGS="$(CFLAGS)" LDFLAGS="$(LDFLAGS)"

GARBAGE = cachefile confdefs.h config.nice config.log \
		  config.status \
		  curl-config \
		  libcurl.pc \
		  libtool \
		  lib/curl_config.h \
		  lib/Makefile \
		  include/Makefile \
		  include/curl/Makefile \
		  packages/Linux/Makefile \
		  packages/EPM/Makefile \
		  packages/EPM/curl.list \
		  packages/AIX/RPM/Makefile \
		  packages/AIX/RPM/curl.spec \
		  packages/AIX/Makefile \
		  packages/Linux/RPM/curl.spec \
		  packages/Linux/RPM/curl-ssl.spec \
		  packages/Linux/RPM/Makefile \
		  packages/Makefile \
		  packages/Solaris/Makefile \
		  packages/vms/Makefile \
		  packages/Win32/cygwin/Makefile \
		  packages/Win32/Makefile \
		  src/curl_config.h \
		  src/Makefile \
		  tests/configurehelp.pm \
		  tests/data/Makefile \
		  tests/libtest/Makefile \
		  tests/server/Makefile \
		  tests/unit/Makefile \
		  tests/Makefile \


# $0 <message> <echo>
define Msg
$(2)echo -e "\033[32m$(1)\033[0m";
endef

# $0 <tarball> <directory> <patchfile>
define PrepareSource
$(call Msg,Prepare source $(2) $(3),@)
$(Q)if [ ! -d "$(2)/$(SRCROOT)" ]; then \
		mkdir -p $(2); \
		tar xf $(1) -C $(2); \
		if [ -f "$(3)" ]; then cd "$(2)" && cat "$(3)" | patch -p1; fi \
		fi
endef

# $0 <directory> <options>
define RunConfigure
$(Q)if [ ! -f "$(1)/config.log" ]; then \
		$(call Msg,Enter $(1) and run configure $(2)) \
		cd "$(1)";/bin/sh "$(1)/configure" $(2) $(VERBOSE); \
		if [ "$$?" = "0" ]; then touch "$(1)/config.log"; fi \
	fi
endef

# $0 <dirctory>
define BuildApplet
$(call Msg,Enter $(1) and make,@)
$(Q)make -C "$(1)" $(VERBOSE)
endef

# $0 <file/dir>
define RemoveFileDir
#
$(Q)if [ -e $(1) ]; then rm -rf $(1) && echo "Remove $(1)"; fi
endef

# $0 <orig> <new> <patch file>
define Mkdiff
$(Q)if test -f $(3); then cp -f $(3) $(3).bak; fi
$(CURDIR)/mkdiff "$(1)" "$(2)" "$(3).tmp"
$(CURDIR)/stripdate.pl < "$(3).tmp" > "$(3)"
$(Q)if test -f $(3).tmp; then rm -f $(3).tmp; fi
endef

.PHONY: all
all:
	$(Q)make build

prepare:
	$(call PrepareSource,$(SOURCEDIR)/$(TARBALL),$(BUILDDIR),$(SOURCEDIR)/$(PATCHFILE))

config: prepare
	$(call RunConfigure,$(BUILDDIR)/$(SRCROOT),$(TARGET_CONFS))

build: config
	$(call Msg,Building target,@)
	$(call BuildApplet,$(BUILDDIR)/$(SRCROOT))

.PHONY: prepare config build

.PHONY: cleantarget
cleantarget:
	$(Q)if [ -f "$(BUILDDIR)/$(SRCROOT)/Makefile" ]; then make -C "$(BUILDDIR)/$(SRCROOT)" distclean; fi

.PHONY: mkdiff
mkdiff:
	$(call Msg,Make the patch file,@)
	$(Q)mkdir -p $(CURDIR)/orig
	$(Q)tar xf $(SOURCEDIR)/$(TARBALL) -C $(CURDIR)/orig
	$(Q)if test -f "$(BUILDDIR)/$(SRCROOT)/Makefile"; then make -C "$(BUILDDIR)/$(SRCROOT)" clean; fi
	$(foreach fd,$(GARBAGE),$(call RemoveFileDir,$(BUILDDIR)/$(SRCROOT)/$(fd)))
	-$(Q)find $(BUILDDIR)/$(SRCROOT) -name ".deps" -exec rm -rf {} \;
	$(Q)if test -f "$(BUILDDIR)/$(SRCROOT)/config.log"; then rm -f "$(BUILDDIR)/$(SRCROOT)/config.log"; fi
	$(call Mkdiff,orig,build,$(SOURCEDIR)/$(PATCHFILE))

.PHONY: install
install:
	$(call Msg,Installing $(APPLET) in $(TARGET),@)
	$(Q)cd "$(BUILDDIR)/$(SRCROOT)/lib/.libs"; cp -Rp libcurl.so* $(TARGET)/lib
	$(Q)$(STRIP) $(TARGET)/lib/libcurl.so*
	$(Q)cd "$(BUILDDIR)/$(SRCROOT)"; cp -f src/.libs/curl $(TARGET)/usr/sbin/.
	$(Q)$(STRIP) $(TARGET)/usr/sbin/curl

.PHONY: clean
clean:
	$(call Msg,Clean $(APPLET),@)
	$(Q)if [ -d "$(BUILDDIR)" ]; then rm -rf "$(BUILDDIR)"; fi
	$(Q)if [ -d "orig" ]; then rm -rf "orig"; fi

.PHONY: clear
clear:
	$(Q)if test -f "$(BUILDDIR)/$(SRCROOT)/config.log"; then rm -f "$(BUILDDIR)/$(SRCROOT)/config.log"; fi
	$(Q)make -C $(BUILDDIR)/$(SRCROOT) clean
