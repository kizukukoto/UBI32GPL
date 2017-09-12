##############################################################################
# Makefile for PHP
# Created by Yi-hung <yi_hung@alphanetworks.com>
##############################################################################
include ../arch.mk
include ../../path.mk
include ../../.config

VERBOSE := > /dev/null
APPLET := php
VERSION := 5.3.0
TARBALL := $(APPLET)-$(VERSION).tar.bz2
PATCHFILE := $(TARBALL).diff
SRCROOT := $(APPLET)-$(VERSION)
SOURCEDIR := $(CURDIR)
BUILDDIR := $(CURDIR)/build
ROOTFS := /usr
MYJOBS := -j$(shell grep -c processor /proc/cpuinfo)

TARGET_CACHE :=
ifeq ($(strip $(HOST_CPU)),ubicom32)
TARGET_CACHE += ac_cv_c_bigendian_php=yes
endif

TARGET_CONFS := --build=$(shell uname -m)-unknown-linux-gnu
TARGET_CONFS += --host=$(HOST_TYPE)
TARGET_CONFS += --target=$(HOST_TYPE)
TARGET_CONFS += --prefix=$(ROOTFS)
TARGET_CONFS += --sysconfdir=/var/etc
TARGET_CONFS += --with-config-file-path=/var/etc/php
#TARGET_CONFS += --enable-debug
TARGET_CONFS += --disable-phar
TARGET_CONFS += --disable-rpath
TARGET_CONFS += --enable-apc
#TARGET_CONFS += --enable-apc-debug
TARGET_CONFS += --enable-apc-filehits
TARGET_CONFS += --disable-apc-mmap
TARGET_CONFS += --enable-apc-sem
TARGET_CONFS += --enable-apc-memprotect
TARGET_CONFS += --enable-cli
TARGET_CONFS += --enable-cgi
TARGET_CONFS += --enable-pdo
#TARGET_CONFS += --enable-safe-mode
TARGET_CONFS += --enable-session
TARGET_CONFS += --enable-shared
TARGET_CONFS += --enable-sigchild
TARGET_CONFS += --enable-xml
TARGET_CONFS += --enable-xmlreader
TARGET_CONFS += --enable-xmlwriter
TARGET_CONFS += --with-pcre-dir=shared
TARGET_CONFS += --with-pic
TARGET_CONFS += --with-tsrm-pthreads
TARGET_CONFS += --without-pear
ifeq ($(strip $(ELBOX_USE_IPV6)),y)
TARGET_CONFS += --enable-ipv6
else
TARGET_CONFS += --disable-ipv6
endif
ifeq ($(strip $(ELBOX_PROGS_GPL_APACHE_229)),y)
APACHE_ROOT = $(TOPDIR)/progs.gpl/apache-2.2.9/build/httpd-2.2.9
APACHE_ENV = $(APACHE_ROOT)/build/config_vars.mk
APACHE_APR_CONFIG = $(APACHE_ROOT)/srclib/apr/apr-1-config
APACHE_APU_CONFIG = $(APACHE_ROOT)/srclib/apr-util/apu-1-config
TARGET_CONFS += --with-apxs2=$(SOURCEDIR)/apxs
ENVVARS = $(SOURCEDIR)/envvars
endif
ifeq ($(strip $(ELBOX_PROGS_GPL_LIBXML2_278)),y)
LIBXML2_ROOT = $(TOPDIR)/progs.gpl/libxml2-2.7.8/build/libxml2-2.7.8
XML2_CONFIG = $(LIBXML2_ROOT)/xml2-config
TARGET_CONFS += --with-libxml=shared,$(LIBXML2_ROOT)
TARGET_CONFS += --with-libxml-dir=shared,$(LIBXML2_ROOT)
TARGET_CACHE += ac_cv_php_xml2_config_path="$(XML2_CONFIG)"
TARGET_CACHE += php_cv_libxml_build_works="yes"
CFLAGS += -I$(LIBXML2_ROOT)/include
CFLAGS += -L$(LIBXML2_ROOT)/.libs
endif
ifeq ($(strip $(ELBOX_PROGS_GPL_OPENSSL_098R)),y)
OPENSSL_ROOT = $(TOPDIR)/progs.gpl/openssl-0.9.8r/build/openssl-0.9.8r
TARGET_CONFS += --with-openssl=$(OPENSSL_ROOT)
TARGET_CONFS += --with-openssl-dir=$(OPENSSL_ROOT)
CFLAGS += -I$(OPENSSL_ROOT)/include
CFLAGS += -L$(OPENSSL_ROOT)
endif
ifeq ($(strip $(ELBOX_PROGS_GPL_SQLITE_374)),y)
SQLITE_ROOT = $(TOPDIR)/progs.gpl/sqlite-3.7.4/build/sqlite-autoconf-3070400
TARGET_CONFS += --with-sqlite3=$(SQLITE_ROOT)
TARGET_CONFS += --with-sqlite3-dir=$(SQLITE_ROOT)
CFLAGS += -I$(SQLITE_ROOT)
CFLAGS += -L$(SQLITE_ROOT)/.libs
TARGET_CONFS += --with-pdo-sqlite=$(SQLITE_ROOT)
TARGET_CACHE += ac_cv_lib_sqlite3_sqlite3_key=yes
endif
ifeq ($(strip $(ELBOX_PROGS_GPL_ZLIB_123)),y)
ZLIB_ROOT = $(TOPDIR)/progs.gpl/zlib-1.2.3
TARGET_CONFS += --with-zlib=$(ZLIB_ROOT)
TARGET_CONFS += --with-zlib-dir=$(ZLIB_ROOT)
CFLAGS += -I$(ZLIB_ROOT)
CFLAGS += -L$(ZLIB_ROOT)
endif
ifeq ($(strip $(ELBOX_PROGS_GPL_CURL_7231)),y)
CURL_ROOT = $(TOPDIR)/progs.gpl/curl-7.23.1/build/curl-7.23.1
CURL_CONFIG = $(CURL_ROOT)/curl-config
TARGET_CONFS += --with-curl=$(CURL_ROOT)
TARGET_CONFS += --with-curl-dir=$(CURL_ROOT)
CFLAGS += -I$(CURL_ROOT)
CFLAGS += -L$(CURL_ROOT)/lib/.libs
endif
ifeq ($(strip $(ELBOX_PROGS_GPL_LIBICONV_1_13_1)),y)
ICONV_ROOT = $(TOPDIR)/progs.gpl/libiconv-1.13.1/libiconv-1.13.1
TARGET_CONFS += --with-iconv=$(ICONV_ROOT)
CFLAGS += -I$(ICONV_ROOT)/include
LDFLAGS += -L$(ICONV_ROOT)/lib/.libs -liconv
else
TARGET_CONFS += --without-iconv
endif
ifeq ($(strip $(ELBOX_PROGS_GPL_GETTEXT_01811)),y)
GETTEXT_ROOT = $(TOPDIR)/progs.gpl/gettext-0.18.1.1/build/gettext-0.18.1.1
TARGET_CONFS += --with-gettext=$(GETTEXT_ROOT)
CFLAGS += -I$(GETTEXT_ROOT)/gettext-runtime/intl -L$(GETTEXT_ROOT)/gettext-runtime/intl/.libs
LDFLAGS += -L$(GETTEXT_ROOT)/gettext-runtime/intl/.libs -lintl
endif

CFLAGS += -lpthread
LDFLAGS += -lpthread

GARBAGE = cachefile confdefs.h config.nice config.log \
		  ext/date/lib/timelib_config.h \
		  ext/iconv/php_have_bsd_iconv.h \
		  ext/iconv/php_have_glibc_iconv.h \
		  ext/iconv/php_have_ibm_iconv.h \
		  ext/iconv/php_have_iconv.h \
		  ext/iconv/php_have_libiconv.h \
		  ext/iconv/php_iconv_supports_errno.h \
		  ext/iconv/php_php_iconv_h_path.h \
		  ext/iconv/php_php_iconv_impl.h \
		  ext/sqlite/libsqlite/src/config.h \
		  ext/sqlite/libsqlite/src/sqlite.h \
		  Makefile.fragments \
		  Makefile.objects \
		  main/build-defs.h \
		  main/internal_functions.c \
		  main/internal_functions_cli.c \
		  sapi/cli/php.1 \
		  scripts/man1/php-config.1 \
		  scripts/man1/phpize.1 \
		  scripts/php-config \
		  scripts/phpize \
		  TSRM/tsrm_config.h \
		  Zend/zend_config.h

CLEANFILES = $(BUILDDIR) orig apxs $(ENVVARS) apr-1-config apu-1-config

STRIP_TARGETS = $(TARGET)/usr/bin/php-config \
				$(TARGET)/usr/bin/phpize \
				$(TARGET)/usr/include \
				$(TARGET)/usr/lib/php/build \
				$(TARGET)/usr/man

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

# $0 <cachefile> <cacheoptions>
define GenCachefile
$(Q)if [ "$(2)" != "" ]; then \
		$(call Msg,Create cache file $(1)) \
		for i in "$(2)"; do \
			echo "$$i" >> "$(1)"; \
		done \
	else \
		touch "$(1)/cachefile"; \
	fi
endef

# $0 <apache env file> <target env file> <apxs.in> <apr config> <apu config>
define GenApxs
$(call Msg,Generate Apache Environment Variables,@)
$(Q)cat $(1) | sed -e 's, = ,=",g' -e 's, =,=",g' -e 's,$$,",g' -e 's,(,{,g' -e 's,),},g' > $(2)
$(Q)sed -e 's,top_builddir,abs_srcdir,g' -e 's,top_srcdir,abs_srcdir,g' -i $(2) $(2)
$(Q)sed -e '/^APR_CONFIG/d' -e '/^APU_CONFIG/d' -i $(2) $(2)
$(Q)echo "APR_CONFIG=$(SOURCEDIR)/apr-1-config" >> $(2)
$(Q)echo "APU_CONFIG=$(SOURCEDIR)/apu-1-config" >> $(2)
$(Q)cp -f $(4) $(5) $(SOURCEDIR)/.
$(call Msg,Generate apxs,@)
$(Q)cat $(3) | sed -e 's,@@APACHEENVVARS@@,$(2),g' > $(SOURCEDIR)/apxs
$(Q)chmod 775 $(SOURCEDIR)/apxs
endef

# $0 <file> <prefix>
define FixedCurl-config
$(Q)sed -e 's,^prefix=.*$$,prefix=$(2),' -i $(1) $(1)
$(Q)chmod 777 $(1)
endef

# $0 <file> <prefix>
define FixedXml2-config
$(Q)sed -e 's,^prefix=.*$$,prefix=$(2),' -i $(1) $(1)
endef

# $0 <directory> <options>
define RunConfigure
$(Q)if [ ! -f "$(1)/config.log" ]; then \
		echo "Enter $(1) and run configure $(2)"; \
		cd "$(1)";export CC=$(CC);export CFLAGS="$(CFLAGS)";export LDFLAGS="$(LDFLAGS)";./configure $(2) --cache-file=./cachefile $(VERBOSE); \
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
	$(Q)echo "" > $(BUILDDIR)/$(SRCROOT)/cachefile
ifeq ($(ELBOX_PROGS_GPL_APACHE_229),y)
	$(call GenApxs,$(APACHE_ENV),$(ENVVARS),$(SOURCEDIR)/apxs.in,$(APACHE_APR_CONFIG),$(APACHE_APU_CONFIG))
endif
ifeq ($(ELBOX_PROGS_GPL_CURL_7231),y)
	$(call FixedCurl-config,$(CURL_CONFIG),$(CURL_ROOT))
endif
ifeq ($(ELBOX_PROGS_GPL_LIBXML2_278),y)
	$(call FixedXml2-config,$(XML2_CONFIG),$(LIBXML2_ROOT))
endif
	$(call GenCachefile,$(BUILDDIR)/$(SRCROOT)/cachefile,$(TARGET_CACHE))
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
	$(Q)if test -f "$(BUILDDIR)/$(SRCROOT)/Makefile"; then make -C "$(BUILDDIR)/$(SRCROOT)" distclean; fi
	$(foreach fd,$(GARBAGE),$(call RemoveFileDir,$(BUILDDIR)/$(SRCROOT)/$(fd)))
	$(call Mkdiff,orig,build,$(SOURCEDIR)/$(PATCHFILE))

ifeq ($(ELBOX_PHP_IN_HDD),y)
MNTP=$(CONFIG_CGIBIN_ARIES_HD_MOUNT_POINT)
TARGETHD=$(TOPDIR)/$(CONFIG_CGIBIN_ARIES_IMAGE_INTERIM_DIR)
STRIP_HDTARGETS = $(TARGETHD)/usr/bin/php-config \
		  $(TARGETHD)/usr/bin/phpize \
		  $(TARGETHD)/usr/include \
		  $(TARGETHD)/usr/lib/php/build \
		  $(TARGETHD)/usr/man

.PHONY: install
install:
	$(call Msg,Installing $(APPLET) in $(TARGETHD),@)
	$(Q)make -C "$(BUILDDIR)/$(SRCROOT)" install INSTALL_ROOT=$(TARGETHD)
	$(Q)if [ -d $(TARGETHD)/usr/lib/php/extensions ]; then find $(TARGETHD)/usr/lib/php/extensions -type -f -exec $(STRIP) {} \; ; fi
	$(Q)cp -f $(BUILDDIR)/$(SRCROOT)/.libs/libphp5.so $(TARGETHD)/usr/modules/libphp5.so
	$(Q)$(STRIP) $(TARGETHD)/usr/modules/libphp5.so
	$(Q)cp -f $(BUILDDIR)/$(SRCROOT)/sapi/cli/.libs/php $(TARGETHD)/usr/bin/php
	$(Q)$(STRIP) $(TARGETHD)/usr/bin/php
	$(foreach fd,$(STRIP_HDTARGETS),$(call RemoveFileDir,$(fd)))
else
.PHONY: install
install:
	$(call Msg,Installing $(APPLET) in $(TARGET),@)
	$(Q)make -C "$(BUILDDIR)/$(SRCROOT)" install INSTALL_ROOT=$(TARGET)
	$(Q)if [ -d $(TARGET)/usr/lib/php/extensions ]; then find $(TARGET)/usr/lib/php/extensions -type f -exec $(STRIP) {} \; ; fi
	$(Q)cp -f $(BUILDDIR)/$(SRCROOT)/.libs/libphp5.so $(TARGET)/usr/modules/libphp5.so
	$(Q)$(STRIP) $(TARGET)/usr/modules/libphp5.so
	$(Q)cp -f $(BUILDDIR)/$(SRCROOT)/sapi/cli/.libs/php $(TARGET)/usr/bin/php
	$(Q)$(STRIP) $(TARGET)/usr/bin/php
	$(foreach fd,$(STRIP_TARGETS),$(call RemoveFileDir,$(fd)))
endif

.PHONY: clean
clean:
	$(call Msg,Clean $(APPLET),@)
	$(foreach fd,$(CLEANFILES),$(call RemoveFileDir,$(fd)))

.PHONY: clear
clear:
	$(Q)if test -f "$(BUILDDIR)/$(SRCROOT)/config.log"; then rm -f "$(BUILDDIR)/$(SRCROOT)/config.log"; fi
	$(Q)make -C $(BUILDDIR)/$(SRCROOT) clean

.PHONY: apxs
apxs:
	$(call GenApxs,$(APACHE_ENV),$(ENVVARS),$(SOURCEDIR)/apxs.in,$(APACHE_APR_CONFIG),$(APACHE_APU_CONFIG))

