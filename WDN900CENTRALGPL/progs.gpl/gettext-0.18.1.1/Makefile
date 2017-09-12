##############################################################################
# Makefile for gettext
# Created by Yi-hung <yi_hung@alphanetworks.com>
##############################################################################
include ../arch.mk
include ../../path.mk
include ../../.config

VERBOSE := > /dev/null
APPLET := gettext
VERSION := 0.18.1.1
TARBALL := $(APPLET)-$(VERSION).tar.gz
PATCHFILE := $(TARBALL).diff
SRCROOT := $(APPLET)-$(VERSION)
SOURCEDIR := $(CURDIR)
BUILDDIR := $(CURDIR)/build
ROOTFS := $(TARGET)
MYJOBS := -j$(shell grep -c processor /proc/cpuinfo)

ifeq ($(strip $(CONFIG_CGIBIN_ARIES_FIRMWARE_IN_HD)),y)
DESTDIR = $(TOPDIR)/$(CONFIG_CGIBIN_ARIES_IMAGE_INTERIM_DIR)/usr
else
DESTDIR = $(ROOTFS)
endif

TARGET_CONFS := --build=$(shell uname -m)-unknown-linux-gnu
TARGET_CONFS += --host=$(HOST_TYPE)
TARGET_CONFS += --prefix=$(PREFIX)
TARGET_CONFS += --enable-shared
TARGET_CONFS += --disable-static
TARGET_CONFS += --with-included-gettext
TARGET_CONFS += --disable-curses
TARGET_CONFS += --enable-nls
TARGET_CONFS += --disable-acl
TARGET_CONFS += --enable-threads=pth
ifeq ($(ELBOX_PROGS_GPL_LIBICONV_1_13_1),y)
LIBICONV_ROOT := $(TOPDIR)/progs.gpl/libiconv-1.13.1/libiconv-1.13.1
TARGET_CONFS += --with-libiconv-prefix=$(LIBICONV_ROOT)
CFLAGS += -I$(LIBICONV_ROOT)/include
LDFLAGS += -L$(LIBICONV_ROOT)/lib/.libs
endif
TARGET_CONFS += CFLAGS="$(CFLAGS)" LDFLAGS="$(LDFLAGS)"

STRIP_TARGETS = bin include share

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
	$(Q)if test -f "$(BUILDDIR)/$(SRCROOT)/Makefile"; then make -C "$(BUILDDIR)/$(SRCROOT)" distclean; fi
	$(Q)if test -f "$(BUILDDIR)/$(SRCROOT)/config.log"; then rm -f "$(BUILDDIR)/$(SRCROOT)/config.log"; fi
	$(call Mkdiff,orig,build,$(SOURCEDIR)/$(PATCHFILE))

.PHONY: install
install:
	$(call Msg,Installing $(APPLET) in $(TARGET),@)
	$(Q)make -C "$(BUILDDIR)/$(SRCROOT)" install DESTDIR=$(DESTDIR)
	$(foreach df,$(STRIP_TARGETS),$(call RemoveDirFile,$(DESTDIR)/$(df)))

.PHONY: clean
clean:
	$(call Msg,Clean $(APPLET),@)
	$(Q)if [ -d "$(BUILDDIR)" ]; then rm -rf "$(BUILDDIR)"; fi
	$(Q)if [ -d "orig" ]; then rm -rf "orig"; fi
