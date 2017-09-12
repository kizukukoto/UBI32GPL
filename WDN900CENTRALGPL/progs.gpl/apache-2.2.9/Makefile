##############################################################################
# Makefile for Apache
# Created by Yi-hung <yi_hung@alphanetworks.com>
##############################################################################
include ../arch.mk
include ../../path.mk
include ../../.config

VERBOSE := > /dev/null
APPLET := httpd
VERSION := 2.2.9
TARBALL := $(APPLET)-$(VERSION).tar.bz2
PATCHFILE := $(TARBALL).diff
SRCROOT := $(APPLET)-$(VERSION)
SOURCEDIR := $(CURDIR)
BUILDDIR := $(CURDIR)/build
ROOTFS := /usr
MYJOBS := -j$(shell grep -c processor /proc/cpuinfo)

TARGET_CONFS := --build=$(shell uname -m)-unknown-linux-gnu
TARGET_CONFS += --host=$(HOST_TYPE)
TARGET_CONFS += --prefix=$(ROOTFS)
TARGET_CONFS += --sysconfdir=/var/etc/apache2
TARGET_CONFS += --with-program-name=apache2
TARGET_CONFS += --enable-auth-digest
TARGET_CONFS += --enable-dav
TARGET_CONFS += --enable-dav-fs
TARGET_CONFS += --enable-headers
TARGET_CONFS += --enable-pcre
TARGET_CONFS += --enable-rewrite
TARGET_CONFS += --enable-so
TARGET_CONFS += --enable-static-rotatelogs
TARGET_CONFS += --enable-vhost-alias

ifneq ($(strip $(ELBOX_PROGS_GPL_APACHE_RUNTIME_LIBRARY_PATH)),"")
LDFLAGS += -Wl,$(ELBOX_PROGS_GPL_APACHE_RUNTIME_LIBRARY_PATH)
endif

TARGET_CONFS += CFLAGS="$(CFLAGS) -DBIG_SECURITY_HOLE " LDFLAGS="$(LDFLAGS) -lpthread"

ifeq ($(ELBOX_PROGS_GPL_ZLIB_123),y)
TARGET_CONFS += --enable-deflate
TARGET_CONFS += --enable-z
TARGET_CONFS += --with-z=$(TOPDIR)/progs.gpl/zlib-1.2.3
endif
ifeq ($(ELBOX_PROGS_GPL_OPENSSL_098G),y)
TARGET_CONFS += --enable-ssl
TARGET_CONFS += --with-ssl=$(TOPDIR)/progs.gpl/openssl-0.9.8g/build/openssl-0.9.8g
endif
ifeq ($(ELBOX_PROGS_GPL_OPENSSL_098R),y)
TARGET_CONFS += --enable-ssl
TARGET_CONFS += --with-ssl=$(TOPDIR)/progs.gpl/openssl-0.9.8r/build/openssl-0.9.8r
endif
ifeq ($(ELBOX_PROGS_GPL_OPENSSL_100E),y)
TARGET_CONFS += --enable-ssl
TARGET_CONFS += --with-ssl=$(TOPDIR)/progs.gpl/openssl-1.0.0e/build/openssl-1.0.0e
endif

TARGET_CACHE := ac_cv_file__dev_zero="yes"
TARGET_CACHE += ac_cv_func_setpgrp_void="yes"
TARGET_CACHE += ac_cv_tcp_nodelay_inherited="yes"
TARGET_CACHE += apr_cv_tcp_nodelay_with_cork="yes"
TARGET_CACHE += ac_cv_o_nonblock_inherited="yes"
TARGET_CACHE += ap_void_ptr_lt_long="no"
TARGET_CACHE += apr_cv_process_shared_works="yes"
TARGET_CACHE += apr_cv_mutex_robust_shared="yes"

STRIP_TARGETS = man manual include build logs error cgi-bin \
				bin/apr-1-config \
				bin/apu-1-config \
				bin/httxt2dbm \
				bin/htdbm \
				bin/dbmmanage \
				bin/logresolve \
				/bin/ab \
				/bin/apxs

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

# $0 <directory> <options> <cachefile> <cacheoptions> <env>
define RunConfigure
$(Q)if [ "$(3)" != "" ]; then \
		$(call Msg,Create cache file $(3)) \
		for i in "$(4)"; do \
			echo "$$i" >> "$(3)"; \
		done \
	else \
		touch "$(1)/cachefile"; \
	fi
$(Q)if [ ! -f "$(1)/Makefile" ]; then \
		$(call Msg,Enter $(1) and run configure $(2)) \
		cd "$(1)"; $(5) /bin/sh "$(1)/configure" $(2) --cache-file=./cachefile $(VERBOSE); \
	fi
endef

# $0 <host dir> <target dir>
define SetupCrossEnv
$(call Msg,Setup cross environment $(1) $(2),@)
$(Q)if [ ! -f "$(2)/srclib/pcre/dftables" ]; then \
		sed -e "s/c: dftables/c:/" "$(2)/srclib/pcre/Makefile" -i "$(2)/srclib/pcre/Makefile"; \
		ln -s "$(1)/srclib/pcre/dftables"    "$(2)/srclib/pcre/."; \
	fi
$(Q)if [ ! -f "$(2)/server/gen_test_char" ]; then \
		sed -e "s/h: gen_test_char/h:/" "$(2)/server/Makefile" -i "$(2)/server/Makefile"; \
		ln -s "$(1)/server/gen_test_char"    "$(2)/server/."; \
	fi
endef

# $0 <dirctory>
define BuildApplet
$(call Msg,Enter $(1) and make $(MYJOBS),@)
$(Q)make -C "$(1)" $(MYJOBS) $(VERBOSE)
endef

# $0 <orig> <new> <patch file>
define Mkdiff
$(Q)if test -f $(3); then cp -f $(3) $(3).bak; fi
$(CURDIR)/mkdiff "$(1)" "$(2)" "$(3).tmp"
$(CURDIR)/stripdate.pl < "$(3).tmp" > "$(3)"
$(Q)if test -f $(3).tmp; then rm -f $(3).tmp; fi
endef

# $0 <directory/file>
define RemoveDirFile
test -e $(1) && rm -rf $(1) && echo "Removed $(1)";
endef

.PHONY: all
all:
	$(Q)make build

preparehost:
	$(call PrepareSource,$(SOURCEDIR)/$(TARBALL),$(BUILDDIR)/host,$(SOURCEDIR)/$(PATCHFILE))

confighost: preparehost
	$(call RunConfigure,$(BUILDDIR)/host/$(SRCROOT),$(HOST_CONFS),,,$(HOST_ENV))

buildhost: confighost
	$(call Msg,Building host utilities,@)
	$(call BuildApplet,$(BUILDDIR)/host/$(SRCROOT))

.PHONY: preparehost confighost buildhost

prepare:
	$(call PrepareSource,$(SOURCEDIR)/$(TARBALL),$(BUILDDIR),$(SOURCEDIR)/$(PATCHFILE))

config: prepare
	$(call RunConfigure,$(BUILDDIR)/$(SRCROOT),$(TARGET_CONFS),$(BUILDDIR)/$(SRCROOT)/cachefile,$(TARGET_CACHE))

build_xsend:
	$(Q)make -C mod_xsendfile-0.12

build: config
	$(call Msg,Building target,@)
	$(call BuildApplet,$(BUILDDIR)/$(SRCROOT))
	$(Q)make build_xsend

.PHONY: prepare config build_xsend build

.PHONY: cleantarget
cleantarget:
	$(Q)if [ -f "$(BUILDDIR)/$(SRCROOT)/Makefile" ]; then make -C "$(BUILDDIR)/$(SRCROOT)" distclean; fi
	$(Q)if [ -f "$(BUILDDIR)/$(SRCROOT)/cachefile" ]; then rm -f "$(BUILDDIR)/$(SRCROOT)/cachefile"; fi

.PHONY: mkdiff
mkdiff:
	$(call Msg,Make the patch file,@)
	$(Q)mkdir -p $(CURDIR)/orig
	$(Q)tar xf $(SOURCEDIR)/$(TARBALL) -C $(CURDIR)/orig
	$(Q)if test -d "$(BUILDDIR)/host"; then rm -rf "$(BUILDDIR)/host"; fi
	$(Q)if test -f "$(BUILDDIR)/$(SRCROOT)/Makefile"; then make -C "$(BUILDDIR)/$(SRCROOT)" distclean; fi
	$(Q)if test -f "$(BUILDDIR)/$(SRCROOT)/cachefile"; then rm -f "$(BUILDDIR)/$(SRCROOT)/cachefile"; fi
	$(Q)find "$(BUILDDIR)/$(SRCROOT)" -name config.nice -exec rm -f {} \;
	$(call Mkdiff,orig,build,$(SOURCEDIR)/$(PATCHFILE))

ifeq ($(strip $(ELBOX_APACHE_IN_HD)),y)
MNTP=$(CONFIG_CGIBIN_ARIES_HD_MOUNT_POINT)
TARGETHD=$(TOPDIR)/$(CONFIG_CGIBIN_ARIES_IMAGE_INTERIM_DIR)
APACHECTL=$(TARGETHD)/usr/bin/apachectl

.PHONY: install
install:
	$(call Msg,Installing $(APPLET) in $(TARGETHD),@)
	$(Q)make -C "$(BUILDDIR)/$(SRCROOT)" install DESTDIR=$(TARGETHD) $(VERBOSE)
	$(foreach df,$(STRIP_TARGETS),$(call RemoveDirFile,$(TARGETHD)$(ROOTFS)/$(df)))
	$(Q)test -d $(TARGETHD)/var/etc && rm -rf $(TARGETHD)/var/etc && $(call Msg,Removed $(TARGETHD)/var/etc)
	$(Q)make -C mod_xsendfile-0.12 install DESTDIR=$(TARGETHD)
	$(Q)sed -e 's,^HTTPD=.*,HTTPD="/$(shell echo $(CONFIG_CGIBIN_ARIES_IMAGE_INTERIM_DIR))/usr/bin/apache2",' -i $(APACHECTL) $(APACHECTL)
else
.PHONY: install
install:
	$(call Msg,Installing $(APPLET) in $(TARGET),@)
	$(Q)make -C "$(BUILDDIR)/$(SRCROOT)" install DESTDIR=$(TARGET) $(VERBOSE)
	$(foreach df,$(STRIP_TARGETS),$(call RemoveDirFile,$(TARGET)$(ROOTFS)/$(df)))
	$(Q)test -d $(TARGET)/var/etc && rm -rf $(TARGET)/var/etc && $(call Msg,Removed $(TARGET)/var/etc)
	$(Q)make -C mod_xsendfile-0.12 install
endif

.PHONY: clean_xsend
clean_xsend:
	$(Q)make -C mod_xsendfile-0.12 clean

.PHONY: clean
clean:
	$(Q)if [ -d "$(BUILDDIR)" ]; then rm -rf "$(BUILDDIR)"; fi
	$(Q)if [ -d "orig" ]; then rm -rf "orig"; fi
	$(Q)make clean_xsend

