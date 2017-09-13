########################################################################
#
# Makefile
#
#     Top level Makefile for Ubicom Distro
#
# Copyright © 2009 Ubicom Inc. <www.ubicom.com>
#
# The top Makefile to get ubicom distro building.
#
########################################################################

########################################################################
# TODO list: Please update this list
# * Clean-up LPJs, remove unused config defines like KERNEL_PATH.
#
# * Substitute silent echo commands with ECHO
#
# * When a IP7k board is selected, narrow ultra options to the 7k ones
#   In fact, LPJ selection must be removed and it must be selected
#   according to the board but it is still questionable.
#
# * There is board select duplication. We select board both in top
#   makefile and in distro kernel config. Find a smart solution.
#
# * There are many board files, linux kernel boards and the ultra ones
#   must be synchronized.
#
# * := can be used instead of = in some macros.
#
# * ?= can be used to use some defaults.
#
# * string Config.in defines include quotation marks. If they cause
#   problem remove them by using subst function. It is done so while
#   including config.mk.
#
# * User must type 'make menuconfig', then 'make'. Typing just make w/o
#   a previous 'make menuconfig' opens menuconfig but compilation cannot be
#   achieved because generated .config produced after menuconfig is not
#   included in the first 'make'. Here we can use recursive make call to
#   solve this problem.
#
# * When uboot is enabled/disabled, ultra configs must be regenerated.
#   Currently, we must do an explicit make realclean and make image_clean.
########################################################################

########################################################################
# Directories
# TODO: Currently, elf binaries are messy. Some of them are under bootexec.
#	The remaining ones under gdbloader/bin. Collect them into unique
#	location like a staging_dir.
########################################################################
SHELL:=/bin/bash
PWD:=$(shell pwd)
#ULTRA_DIR=$(subst ",,$(PWD)/$(CONFIG_ULTRA_DIR))
ULTRA_DIR?=$(PWD)/ultra

########################################################################
# Include menuconfig configuration
########################################################################
-include ./.config

# Include release information
# This file will only exist if we have a release
# it defines UBICOM_DISTRO_VERSION and UBICOM_DISTRO_DATE
-include ./version.in

ifeq ($(CONFIG_USE_PREBUILD_ULTRA),y)
  ULTRA_DIR:=$(PWD)/ultra_public
endif

export SDK_DIR=$(ULTRA_DIR)

# bootexec stuff
BOOTEXEC_DIR=$(ULTRA_DIR)/projects/bootexec
MAINEXEC_DIR=$(ULTRA_DIR)/projects/mainexec
BIN_DIR=bin

#UBOOT_DIR=$(subst ",,$(PWD)/$(CONFIG_UBOOT_DIR))
UBOOT_DIR?=$(PWD)/u-boot
DISTRO_DIR=$(subst ",,$(PWD)/$(CONFIG_DISTRO_TYPE))
#DISTRO_DIR?=$(PWD)/$(if $(CONFIG_DISTRO_UCLINUX),uClinux,openwrt)
#TOOLCHAIN_DIR=$(subst ",,$(PWD)/$(CONFIG_TOOLCHAIN_DIR))
TOOLCHAIN_DIR=$(PWD)/toolchain
LOADER_DIR=$(PWD)/gdbloader
FDPICHDR_DIR=$(PWD)/fdpichdr
FDPICHDR_BIN=$(FDPICHDR_DIR)/fdpichdr
KERNEL_DIR=$(subst ",,$(DISTRO_DIR)/$(CONFIG_DISTRO_KERNEL))
SQUASHFS_IMAGE_FILE=$(subst ",,$(DISTRO_DIR)/$(CONFIG_SQUASHFS_IMAGE_FILE))
DISTRO_OCM_SIZE_H=$(subst ",,$(PWD)/$(CONFIG_DISTRO_OCM_SIZE_H))

OPENWRT_DL_DIR?=$(PWD)/$(firstword $(subst -, ,$(subst ",,$(CONFIG_DISTRO_TYPE))))-dl

ULTRA_TOOLS=$(ULTRA_DIR)/tools/bin/linux
LUAGREP = $(ULTRA_TOOLS)/luagrep

export FDPICHDR_BIN

########################################################################
# Command macros
########################################################################
DO_X=@true
MAKE_SILENT_OPT=-s
# enable this for debug of overly complicated make system
ifdef V
DO_X=$(ECHO) "... do $@"
MAKE_SILENT_OPT=
endif

ECHO=@echo
MAKE_MAINEXEC=$(MAKE) $(MAKE_SILENT_OPT) \
	 DISTRO_OCM_SIZE_H=$(DISTRO_OCM_SIZE_H) \
	-C $(MAINEXEC_DIR)

ifdef CONFIG_DISABLE_MAINEXEC
MAKE_BOOTEXEC=$(MAKE) $(MAKE_SILENT_OPT) \
	 DISTRO_OCM_SIZE_H=$(DISTRO_OCM_SIZE_H) \
	-C $(BOOTEXEC_DIR)
else
MAKE_BOOTEXEC=$(MAKE) $(MAKE_SILENT_OPT) \
	-C $(BOOTEXEC_DIR)
endif

ifeq ($(CONFIG_USE_PREBUILD_ULTRA),y)
MAKE_MAINEXEC=true
MAKE_BOOTEXEC=true
endif

MAKE_DISTRO=unset ARCH; $(MAKE) -C $(DISTRO_DIR) ROOTFS_TYPE=$(ROOTFS_TYPE) PROFILER_ENABLED=$(PROFILER_ENABLED)
MAKE_UBOOT=$(MAKE) -C $(UBOOT_DIR)

########################################################################
# Version number shown on menuconfig
########################################################################
export RELEASE=Ubicom Distro
export UBICOMDISTROVERSION=3.0

########################################################################
# Get Board file and architecture
########################################################################
ULTRA_BOARD_FILE=$(ULTRA_DIR)/boards/$(CONFIG_BOARD_NAME)
ARCH_EXTENSION := $(if $(CONFIG_BOARD_NAME),$(shell $(LUAGREP) $(ULTRA_BOARD_FILE) TListNode Name Architecture\ Extension Value),UNDEFINED)

########################################################################
# Add toolchain to the PATH and LD_LIBRARY
########################################################################
NATIVE_TOOLCHAIN_DIR:=$(TOOLCHAIN_DIR)
PATH := $(shell echo $(TOOLCHAIN_DIR)/bin$${PATH:+:$(PATH)})
LD_LIBRARY_PATH := $(shell echo $(TOOLCHAIN_DIR)/lib$${LD_LIBRARY_PATH:+:$(LD_LIBRARY_PATH)})
export PATH LD_LIBRARY_PATH NATIVE_TOOLCHAIN_DIR

########################################################################
# If ipProfile is enabled, pass related data to ctool.
########################################################################
ifeq ($(CONFIG_IPPROFILE),y)
IPPROFILER_PARAMS=-p ipProfile
endif

########################################################################
# If u-boot is enabled, pass also selected u-boot directory to ctool.
########################################################################
ifeq ($(CONFIG_LOADER_UBOOT),y)
UBOOT_PARAMS=-c APP_UBOOT_ENABLE 1
else
UBOOT_PARAMS=-c APP_UBOOT_ENABLE 0
endif
ifndef CONFIG_DISABLE_MAINEXEC
BOOT_PARAMS	= -c BOOTEXEC_ULTRA 1 -c APP_UBOOT_DIR $(UBOOT_DIR) \
		  $(UBOOT_PARAMS) -p ipBootDecompressor
else
BOOT_PARAMS	= -c BOOTEXEC_ULTRA 1 -c APP_UBOOT_DIR $(UBOOT_DIR) \
		  $(UBOOT_PARAMS) -p ipBootDecompressor $(IPPROFILER_PARAMS)
endif
MAIN_PARAMS	= -c MAINEXEC_ULTRA 1 $(IPPROFILER_PARAMS)

########################################################################
# Pass RootFS type to distro
########################################################################
ifeq ($(CONFIG_DISTRO_INITRAMFS),y)
ROOTFS_TYPE=INITRAMFS
endif
ifeq ($(CONFIG_DISTRO_SQUASHFS),y)
ROOTFS_TYPE=SQUASHFS
endif

########################################################################
# Pass Profiler type to distro
########################################################################
ifeq ($(CONFIG_IPPROFILE),y)
PROFILER_ENABLED=Y
else
PROFILER_ENABLED=N
endif

########################################################################
# If we don't have Ubicom distro .config, popup menuconfig
########################################################################
define UbicomDistroConfig
	@echo "Checking $(PWD)/.config"
	@if [ ! -f $(PWD)/.config ]; then \
		echo "You need to type make menuconfig to continue"; \
		exit 1; \
	fi
endef

########################################################################
# To generate ultra configs, run the following command
# TODO: There are some static stuff in LPJ files. Also, there are some
#	stuff that can be configured. These configurable stuff must also be
#	configured from this ctool command. ipProfiler and uboot are examples.
#	This conludes that we also need editing Config.in for them.
########################################################################
ifeq ($(CONFIG_DISTRO_INITRAMFS),y)
define RunCtool
	echo "Generating ultra configs for '$(1)'";\
	echo "Identity String: '$(CONFIG_IDENTITY_STRING)'"; \
	echo "extra params = $($(1)_PARAMS)"; \
	$(ULTRA_TOOLS)/ctool \
		-o $($(1)EXEC_DIR)/config/$(CONFIG_LPJ_NAME) \
		-b $(ULTRA_BOARD_FILE) \
		-c SDK_DIR $(ULTRA_DIR) \
		-c APP_IDENTITY_STRING $(CONFIG_IDENTITY_STRING) \
		$($(1)_PARAMS) \
		-y -g
endef
else
define RunCtool
	echo "Generating ultra configs for '$(1)'";\
	EXTRA_BOOTARGS=`$(ULTRA_TOOLS)/ctool -o $($(1)EXEC_DIR)/config/$(CONFIG_LPJ_NAME) -w APP_BOOTARGS_EXTRA \
		| grep APP_BOOTARGS_EXTRA \
		| sed 's/.*APP_BOOTARGS_EXTRA:\t\(.*\)/\1/'` ; \
	APPBOOTARGS=`$(ULTRA_TOOLS)/ctool -o $($(1)EXEC_DIR)/config/$(CONFIG_LPJ_NAME) -w APP_BOOTARGS \
		| grep APP_BOOTARGS \
		| sed 's/.*APP_BOOTARGS:\t\(.*\)/\1/'` ; \
	BRDBOOTARGS=`$(ULTRA_TOOLS)/ctool -b $(ULTRA_DIR)/boards/$(CONFIG_BOARD_NAME) -o $($(1)EXEC_DIR)/config/$(CONFIG_LPJ_NAME) -w BRD_BOOTARGS \
		| grep BRD_BOOTARGS \
		| sed 's/.*BRD_BOOTARGS:\t\(.*\)/\1/'` ; \
	[ "$$BRDBOOTARGS" = "(null)" ] && { BOOTARGS=APP_BOOTARGS; } || { BOOTARGS=BRD_BOOTARGS && APPBOOTARGS=$$BRDBOOTARGS; }; \
	echo "extra params = $($(1)_PARAMS) `[ "$$EXTRA_BOOTARGS" = "(null)" ] || echo -c $$BOOTARGS $$APPBOOTARGS $$EXTRA_BOOTARGS `"; \
	if [ "$$EXTRA_BOOTARGS" = "(null)" ]; then \
	$(ULTRA_TOOLS)/ctool \
		-o $($(1)EXEC_DIR)/config/$(CONFIG_LPJ_NAME) \
		-b $(ULTRA_BOARD_FILE) \
		-c SDK_DIR $(ULTRA_DIR) \
		-c APP_IDENTITY_STRING $(CONFIG_IDENTITY_STRING) \
		$($(1)_PARAMS) \
		-y -g ; \
	else \
	$(ULTRA_TOOLS)/ctool \
		-o $($(1)EXEC_DIR)/config/$(CONFIG_LPJ_NAME) \
		-b $(ULTRA_BOARD_FILE) \
		-c SDK_DIR $(ULTRA_DIR) \
		-c APP_IDENTITY_STRING $(CONFIG_IDENTITY_STRING) \
		-c $$BOOTARGS "$$APPBOOTARGS $$EXTRA_BOOTARGS" \
		$($(1)_PARAMS) \
		-y -g ; \
	fi
endef
endif

ifeq ($(CONFIG_USE_PREBUILD_ULTRA),y)
  define RunCtool
  endef
endif

########################################################################
# Prepare distro to be compiled properly
########################################################################
define DistroPreSettings
	@echo "Checking $(DISTRO_DIR)/<settings>"; \
	if [ $(CONFIG_DISTRO_TYPE) = "openwrt" ]; then \
		if [ -L "$(DISTRO_DIR)/dl" ]; then \
			if ! ls -al $(DISTRO_DIR)/dl/ >/dev/null 2>/dev/null; then \
				rm $(DISTRO_DIR)/dl; \
			fi; \
		fi; \
		if [ ! -e "$(DISTRO_DIR)/dl" ]; then \
			if [ ! -d "$(OPENWRT_DL_DIR)" ]; then \
				mkdir $(OPENWRT_DL_DIR); \
			fi; \
			ln -s $(OPENWRT_DL_DIR) $(DISTRO_DIR)/dl; \
		fi; \
	fi; \

        if [ "$(CONFIG_DISTRO_TYPE)" = "openwrt" ]; then \
		if [ ! -d "$(DISTRO_DIR)/feeds" ]; then \
			$(DISTRO_DIR)/scripts/feeds update -a; \
			$(DISTRO_DIR)/scripts/feeds install -a; \
		fi; \
	fi
endef

########################################################################
# Some configuration, needs kernel configuration to be regenerated.
########################################################################
define KernelPreSettings
	-@if [ -f .config.old -a \
		"$(CONFIG_DISTRO_INITRAMFS)" != "`grep '^CONFIG_DISTRO_INITRAMFS=y$$' .config.old 2>/dev/null| cut -d '=' -f 2`" -o \
		"$(CONFIG_DISTRO_SQUASHFS)" != "`grep '^CONFIG_DISTRO_SQUASHFS=y$$' .config.old 2>/dev/null | cut -d '=' -f 2`" ];then \
		echo "System configuration is changed, removing $(KERNEL_DIR)/.config"; \
		rm $(KERNEL_DIR)/.config; \
	fi
endef

########################################################################
# If we don't have distro .config, popup menuconfig
########################################################################
define DistroConfig
	@echo "Checking $(DISTRO_DIR)/.config"; \
	if [ ! -f $(DISTRO_DIR)/.config ]; then \
		echo "**********************************************************"; \
		echo "** $(CONFIG_DISTRO_TYPE) config is missing: $(DISTRO_DIR)/.config"; \
		echo "** Running menuconfig..."; \
		echo "**********************************************************"; \
		$(MAKE_DISTRO) menuconfig; \
	fi
endef

########################################################################
# If we don't have internally generated directory configs, create them
########################################################################
define DirConfig
	@echo "Checking directory configs"
	@if [ ! -f config/Config.in.ultra ]; then \
		./config/create_dir_configs.sh; \
	fi
endef

########################################################################
# If we don't have ultra configs, generate them
########################################################################
ifeq ($(CONFIG_USE_PREBUILD_ULTRA),y)
BOARD_NAME=$(patsubst %.brd,%,$(subst ",,$(CONFIG_BOARD_NAME)))
LPJ_NAME=$(patsubst %.lpj,%,$(subst ",,$(CONFIG_LPJ_NAME)))
  define GenerateConfig
	if [ $(CONFIG_DISTRO_TYPE) = "uClinux" ]; then \
		cp -rf $(DISTRO_DIR)/user/ultra/files/$(LPJ_NAME)/$(BOARD_NAME)/ocm_size.h $(KERNEL_DIR)/arch/ubicom32/include/asm/ocm_size.h; \
	fi;
  endef
else
  define GenerateConfig
	@echo "Checking $(1) bootexec configs"; \
	if [ ! -e $($(1)EXEC_DIR)/build/include/Application_config.h ]; then \
		$(call RunCtool,$(1)); \
	fi
  endef
endif

########################################################################
# If ocm_size.h is changed, recompile openwrt kernel with the new one.
# TODO: We remove openwrt kernel before recompilation. Find a smart way
#	not to compile all kernel stuff again.
########################################################################
define DistroIntegrity
	@if [ "$(CONFIG_DISTRO_TYPE)" = "openwrt" ]; then \
		if [ -d $(KERNEL_DIR) ]; then \
			cmp -s $(DISTRO_OCM_SIZE_H) $(KERNEL_DIR)/arch/ubicom32/include/asm/ocm_size.h || { \
				echo "****************************************************************************"; \
				echo "** Recompile $(CONFIG_DISTRO_TYPE) kernel"; \
				echo "** because $(KERNEL_DIR)/arch/ubicom32/include/asm/ocm_size.h has changed"; \
				echo "****************************************************************************"; \
				rm -rf $(KERNEL_DIR); \
			} \
		fi \
	fi
endef

########################################################################
# Check parametered bootloader elf file and if it is absent, compile bootloader
########################################################################
define CheckBootloaderFile
	$(DO_X) $(1)
	@if [ ! -e $(1) ]; then \
		$(MAKE) -f Makefile bootloader; \
	fi
endef

########################################################################
# Check to see if board file exists
########################################################################
define CheckBoardFile
	$(DO_X)
	@if [ "$(CONFIG_BOARD_NAME)" = "" ]; then \
		echo "CONFIG_BOARD_NAME not set"; \
		exit 1; \
	fi
	@if [ ! -f $(ULTRA_BOARD_FILE) ]; then \
		echo "Could not open $(ULTRA_BOARD_FILE)"; \
		exit 1; \
	fi
endef

########################################################################
# Check bootloader elf files
########################################################################
ifndef CONFIG_DISABLE_MAINEXEC
define CheckBootloader
	@echo "Checking bootloader + mainexec";
	$(call CheckBootloaderFile,$(BOOTEXEC_DIR)/ultra.elf);
	$(if $(CONFIG_LOADER_UBOOT),$(call CheckBootloaderFile,$(UBOOT_FINAL)))
	$(call CheckBootloaderFile,$(MAINEXEC_DIR)/ultra.elf);
endef
else
define CheckBootloader
	@echo "Checking bootloader";
	$(call CheckBootloaderFile,$(BOOTEXEC_DIR)/ultra.elf);
	$(if $(CONFIG_LOADER_UBOOT),$(call CheckBootloaderFile,$(UBOOT_FINAL)))
endef
endif

########################################################################
# Default and general targets
# TODO: Find a more generic way for distro_[all | default | world] to provide
#	a solution when we have a third distro because the first default
#	target changes from distro to distro.
########################################################################
default: check_list check_regenconfig check_bootloader check_distro_config
default: $(FDPICHDR_BIN) $(if $(CONFIG_DISTRO_UCLINUX),distro_all,distro_world) recursive_distro
ifdef CONFIG_DISABLE_MAINEXEC
bootloader: check_list bootexec_default $(if $(CONFIG_LOADER_UBOOT),$(if $(CONFIG_USE_PREBUILD_ULTRA),,recursive_uboot))
else
bootloader: check_list bootexec_default mainexec_default $(if $(CONFIG_LOADER_UBOOT),$(if $(CONFIG_USE_PREBUILD_ULTRA),,recursive_uboot))
endif
bootexec: check_list bootexec_default
	$(DO_X)
	$(MAKE) -f Makefile image_bootloader

mainexec: check_list mainexec_default
	$(DO_X)
	$(MAKE) -f Makefile image_mainexec

$(FDPICHDR_BIN):
	make -C $(FDPICHDR_DIR)

########################################################################
# Check rules
########################################################################
check_list_genconfig: check_dir_config check_ubicom_distro_config
check_list: check_identity_string check_list_genconfig check_generate_config $(BIN_DIR)

########################################################################
# Check to see that the identity string is 31 chars or less
########################################################################
check_identity_string:
	$(DO_X)
	@if [ `awk -v ident=$(CONFIG_IDENTITY_STRING) 'BEGIN {if (length(ident) > 31) {print "1";} else {print "0";}}'` = "1" ]; then \
		echo "ERROR: CONFIG_IDENTITY_STRING should be 31 characters or less"; \
		exit 1; \
	fi
	$(ECHO) Identity String: '$(CONFIG_IDENTITY_STRING)'

check_dir_config:
	$(DO_X)
	$(call DirConfig)

check_ubicom_distro_config:
	$(DO_X)
	$(call UbicomDistroConfig)

check_generate_config:
	$(DO_X)
	$(call GenerateConfig,BOOT)
ifndef CONFIG_DISABLE_MAINEXEC
	$(call GenerateConfig,MAIN)
endif

check_kernel_presettings:
	$(DO_X)
	$(call KernelPreSettings)

check_distro_presettings:
	$(DO_X)
	$(call DistroPreSettings)

check_distro_config: check_distro_presettings
	$(DO_X)
	$(call DistroConfig)

check_distro_integrity:
	$(DO_X)
	$(call DistroIntegrity)

check_bootloader:
	$(DO_X)
	$(call CheckBootloader)

########################################################################
# Explicit request to scan directories, board files, and lpjs; then to
# create them as Config.in.xx under ./config directory
########################################################################
init:
	$(DO_X)
	@./config/create_dir_configs.sh

########################################################################
# Explicit generate config request for ultra
########################################################################
genconfig: check_list_genconfig
	$(DO_X)
	@sed -i '/^CONFIG_RUN_CTOOL=/ c\# CONFIG_RUN_CTOOL is not set' .config
	$(call RunCtool,BOOT)
ifndef CONFIG_DISABLE_MAINEXEC
	$(call RunCtool,MAIN)
endif

########################################################################
# Remove generated Config.in.xxx, .config, and do distro distclean
########################################################################
distclean: distro_distclean bootloader_distclean image_clean gdbloader_clean
	$(DO_X)
	
	rm -f .config .config.old
	$(MAKE) -C scripts/config clean
	-$(MAKE) -C uClibc -k $@

# obsolete
distclean_all: distclean ;

clean: distro_clean image_distro_clean gdbloader_clean
	$(DO_X)
clean_all: distro_clean bootloader_clean image_clean gdbloader_clean
	$(DO_X)

########################################################################
# GDB Loader Rules
########################################################################
gdbloader: gdbloader_all ;
gdbloader_%: ;
	$(DO_X)
	make -C $(LOADER_DIR) $(patsubst gdbloader_%,%,$@) ARCH_EXTENSION=$(ARCH_EXTENSION) ULTRA_DIR=$(ULTRA_DIR)

########################################################################
# Bootloader targets will be passed to bootexec and uboot if enabled
#
# realclean is special since u-boot doesn't have realclean, to prevent
# an error, we will perform a distclean instead.
########################################################################
bootloader_realclean: check_toolchain
	$(DO_X) bootexec
	$(MAKE_BOOTEXEC) realclean
	$(DO_X) mainexec
	$(MAKE_MAINEXEC) realclean
ifeq ($(CONFIG_LOADER_UBOOT),y)
	$(DO_X) uboot
	$(MAKE_UBOOT) distclean
endif

bootloader_%: check_toolchain
	$(DO_X) bootexec
	$(MAKE_BOOTEXEC) $(patsubst bootloader_%,%,$@)
ifndef CONFIG_DISABLE_MAINEXEC
	$(DO_X) mainexec
	$(MAKE_MAINEXEC) $(patsubst bootloader_%,%,$@)
endif
ifeq ($(CONFIG_LOADER_UBOOT),y)
	$(DO_X) uboot
	$(MAKE_UBOOT) $(patsubst bootloader_%,%,$@)
endif

########################################################################
# bootexec clean doesn't remove uboot if it is enabled, remove uboot.elf
########################################################################
define RemoveUbootElf
	@rm -f $(UBOOT_FINAL)
endef

bootloader_distclean: remove_uboot_elf
bootloader_clean: remove_uboot_elf
remove_uboot_elf:
	$(call RemoveUbootElf)

########################################################################
# All make targets starting with bootexec_ will be passed to bootexec
# project. If generate ultra config is selected from menuconfig, realclean
# ultra, and generate configs. Clear the selected config also.
########################################################################

bootexec_%: check_toolchain check_regenconfig
	$(DO_X)
	$(MAKE_BOOTEXEC) $(patsubst bootexec_%,%,$@)

mainexec_%: check_toolchain check_regenconfig
	$(DO_X)
	$(MAKE_MAINEXEC) $(patsubst mainexec_%,%,$@)

check_regenconfig:
ifeq ($(CONFIG_RUN_CTOOL),y)
	@sed -i '/^CONFIG_RUN_CTOOL=/ c\# CONFIG_RUN_CTOOL is not set' .config
	@$(MAKE) -f Makefile mainexec_realclean
	@$(MAKE) -f Makefile bootexec_realclean
	@$(MAKE) -f Makefile image_clean
	$(call RunCtool,BOOT)
ifndef CONFIG_DISABLE_MAINEXEC
	$(call RunCtool,MAIN)
endif
endif

########################################################################
# All make targets starting with distro_ will be passed to distro project
# Linux kernel is a symlink while using openwrt. Therefore, while doing
# make distclean on openwrt, we must distclean linux kernel at first before
# removing kernel symlink.
########################################################################
distro_%: check_distro_integrity check_toolchain pre_distro_%
	$(DO_X)
ifneq ($(CONFIG_DISTRO_TYPE),)
	$(MAKE_DISTRO) $(patsubst distro_%,%,$@)
endif

pre_distro_distclean:
	$(DO_X)
ifeq ($(CONFIG_DISTRO_OPENWRT),y)
	@if test -d $(KERNEL_DIR); then unset ARCH; $(MAKE) -C $(KERNEL_DIR) distclean; fi
endif

pre_distro_%: ;

########################################################################
# Distro targets requiring an image_distro
########################################################################
$(if $(CONFIG_DISTRO_UCLINUX),all image single,world):
	$(DO_X)
	$(MAKE_DISTRO) $@
	$(MAKE) -f Makefile image_distro

recursive_distro: $(if $(CONFIG_USE_PREBUILD_ULTRA),recursive_uboot)
	$(DO_X)
	$(MAKE) -f Makefile image_distro

########################################################################
# Remaining targets are bridged to distro
########################################################################
Makefile Makefile.loader .config version.in $(MAINEXEC_DIR)/config/config.mk $(BOOTEXEC_DIR)/config/config.mk: ;

.DEFAULT: check_distro_integrity check_toolchain
	@echo "... do $@"
	$(MAKE_DISTRO) $@

########################################################################
# Include bootexec config.mk to get flash address settings
########################################################################
-include $(BOOTEXEC_DIR)/config/config.mk

########################################################################
# Include loader Makefile to install bootexec, u-boot, and distro
########################################################################
include Makefile.loader

########################################################################
# Include ultra Makefile for ultra/* targets
########################################################################
.PHONY: $(ULTRA_DIR)/Makefile.ultra
ifeq ($(CONFIG_USE_PREBUILD_ULTRA),)
  -include $(ULTRA_DIR)/Makefile.ultra
endif

########################################################################
# Compile u-boot
# TODO: Why do we need the following long lines? Doesn't u-boot provide
#	a way to move them into u-boot?
########################################################################
ifeq ($(CONFIG_LOADER_UBOOT),y)

uboot: check_list bootexec_default recursive_uboot

########################################################################
# After bootexec/config/config.mk is ready, we make a recursive call to
# get values in config.mk
# TODO: don't always make this recursive call, instead, only call it if
#	config.mk does not exist during first make invocation.
########################################################################
recursive_uboot:
	$(DO_X)
	@$(MAKE) $(MAKE_SILENT_OPT) -f Makefile recursive_uboot_do

recursive_uboot_do: check_ultra_uboot_config_h check_uboot_config uboot_all image_uboot
	$(DO_X)

uboot_%:
	$(DO_X)
	$(MAKE_UBOOT) $(patsubst uboot_%,%,$@)

check_ultra_uboot_config_h: $(ULTRA_UBOOT_CONFIG_H)
	$(DO_X)
	uClinux_image_start_addr_dec=$(uClinux_image_start_addr_dec); \
	uClinux_image_start_addr_hex=0x`echo "obase=16; ibase=10; " $$uClinux_image_start_addr_dec | bc`; \
	uClinux_image_run_addr_dec=$(uClinux_image_run_addr_dec); \
	uClinux_image_run_addr_hex=0x`echo "obase=16; ibase=10; " $$uClinux_image_run_addr_dec | bc`; \
	awk -v new_config_env_addr=$(uboot_env_addr) \
	    -v new_config_uboot_image_last_sector=$(uboot_image_last_sector) \
	    -v new_config_uboot_mem_size=$$(($(APP_UBOOT_MEM_SIZE))) \
	    -v new_config_uClinux_image_start_addr=$$uClinux_image_start_addr_hex \
	    -v new_config_uClinux_mem_start_addr=$$uClinux_image_run_addr_hex \
	    -v new_config_env_size=$(uboot_env_size) ' \
		BEGIN { \
			old_config_env_addr = old_config_env_size = old_config_uboot_mem_size = old_config_uboot_image_last_sector =-1; \
			old_config_uClinux_image_start_addr = old_config_uClinux_mem_start_addr = -1; \
		} \
		/CONFIG_ENV_ADDR/	{ old_config_env_addr = $$3 } \
		/CONFIG_ENV_SIZE/	{ old_config_env_size = $$3 } \
		/CONFIG_UBOOT_MEM_SIZE/ { old_config_uboot_mem_size = $$3 } \
		/CONFIG_UBOOT_IMAGE_LAST_SECTOR/ { old_config_uboot_image_last_sector = $$3 } \
		/CONFIG_UCLINUX_IMAGE_START_ADDR/ { old_config_uClinux_image_start_addr = $$3 } \
		/CONFIG_UCLINUX_MEM_START_ADDR/ { old_config_uClinux_mem_start_addr = $$3 } \
		END { \
			if ((new_config_env_addr != old_config_env_addr) || \
			    (new_config_env_size != old_config_env_size) || \
			    (new_config_uboot_mem_size != old_config_uboot_mem_size) || \
			    (new_config_uClinux_image_start_addr != old_config_uClinux_image_start_addr) || \
			    (new_config_uClinux_mem_start_addr != old_config_uClinux_mem_start_addr) || \
			    (new_config_uboot_image_last_sector != old_config_uboot_image_last_sector)) { \
				print "#define CONFIG_ENV_ADDR ", new_config_env_addr >  "$<"; \
				print "#define CONFIG_ENV_SIZE ", new_config_env_size >> "$<"; \
				print "#define CONFIG_UBOOT_MEM_SIZE ", new_config_uboot_mem_size >> "$<"; \
				print "#define CONFIG_UBOOT_IMAGE_LAST_SECTOR ", new_config_uboot_image_last_sector >> "$<"; \
				print "#define CONFIG_UCLINUX_IMAGE_START_ADDR ", new_config_uClinux_image_start_addr >> "$<"; \
				print "#define CONFIG_UCLINUX_MEM_START_ADDR ", new_config_uClinux_mem_start_addr >> "$<"; \
			} \
		}' $<

ifdef CONFIG_DISABLE_MAINEXEC
$(ULTRA_UBOOT_CONFIG_H): check_bootexec_elf $(BOOTEXEC_FINAL)
else
$(ULTRA_UBOOT_CONFIG_H): check_bootexec_elf check_mainexec_elf $(BOOTEXEC_FINAL) $(MAINEXEC_FINAL)
endif
	$(DO_X)
	@if [ ! -f $@ ]; then \
		uClinux_image_start_addr_dec=$(uClinux_image_start_addr_dec); \
		uClinux_image_start_addr_hex=0x`echo "obase=16; ibase=10; " $$uClinux_image_start_addr_dec | bc`; \
		uClinux_image_run_addr_dec=$(uClinux_image_run_addr_dec); \
		uClinux_image_run_addr_hex=0x`echo "obase=16; ibase=10; " $$uClinux_image_run_addr_dec | bc`; \
		echo "#define CONFIG_ENV_ADDR			$(uboot_env_addr)" > $@; \
		echo "#define CONFIG_ENV_SIZE			$(uboot_env_size)" >> $@; \
		echo "#define CONFIG_UBOOT_MEM_SIZE		$(APP_UBOOT_MEM_SIZE)" >> $@; \
		echo "#define CONFIG_UBOOT_IMAGE_LAST_SECTOR	$(uboot_image_last_sector)" >> $@; \
		echo "#define CONFIG_UCLINUX_IMAGE_START_ADDR	" $$uClinux_image_start_addr_hex >> $@; \
		echo "#define CONFIG_UCLINUX_MEM_START_ADDR	" $$uClinux_image_run_addr_hex >> $@; \
	fi

check_uboot_config: $(ULTRA_UBOOT_CONFIG_H) $(UBOOT_DIR)/include/config.mk
	@echo "Board is $(AP_BOARD_NAME)"
	@awk '/BOARD/ { \
			if ($$3 != "$(AP_BOARD_NAME)") { \
				system("make -C $(UBOOT_DIR) distclean"); \
				system("make -C $(UBOOT_DIR) $(AP_BOARD_NAME)_config"); \
			} \
		}' $(UBOOT_DIR)/include/config.mk

$(UBOOT_DIR)/include/config.mk:
	make -C $(UBOOT_DIR) $(AP_BOARD_NAME)_config

endif

########################################################################
# Output current settings
########################################################################
info:
	@echo "DISTRO_DIR=$(DISTRO_DIR)"
	@echo "DISTRO_OCM_SIZE_H=$(CONFIG_DISTRO_OCM_SIZE_H)"
	@echo "ULTRA_DIR=$(ULTRA_DIR)"
	@echo "UBOOT_DIR=$(if $(CONFIG_LOADER_UBOOT),$(UBOOT_DIR),disabled)"
	@echo "TOOLCHAIN_DIR=$(TOOLCHAIN_DIR)"
	@echo "PATH=$(PATH)"
	@echo "LD_LIBRARY_PATH=$(LD_LIBRARY_PATH)"
ifeq ($(CONFIG_DISTRO_OPENWRT),y)
	@echo "OPENWRT_DL_DIR=$(OPENWRT_DL_DIR)"
endif

########################################################################
# Sanity check on the state of TOOLCHAIN_DIR
########################################################################
check_toolchain: check_bin_dir
	$(DO_X)
	$(ECHO) Checking toolchain in $(TOOLCHAIN_DIR)
	@if [ ! -f "$(TOOLCHAIN_DIR)/bin/ubicom32-elf-gcc" ]; then \
		echo "ERROR: Could not locate compiler in $(TOOLCHAIN_DIR)"; \
		$(if $(UBICOM_DISTRO_VERSION),, echo "You could try to run setup_toolchain to fix this.";) \
		exit 1; \
	fi

ifndef UBICOM_DISTRO_VERSION
setup_toolchain:
	./setup_toolchain
endif

########################################################################
# Makefile help
# TODO: Update the list if you add a new target
########################################################################
help:
	$(ECHO) "Basic Targets"
	$(ECHO) " <default>          - build currently selected Linux distribution and bootloader"
	$(ECHO) "                          NOTE: When you use this, the bootloader is only built once and "
	$(ECHO) "                        will not be rebuilt again unless you use 'make bootloader_clean'. "
	$(ECHO) "                        Alternatively, you can type 'make bootloader' to force bootloader"
	$(ECHO) "                        recompilation. Use 'make ready' to see if *.elf files are ready."
#	$(ECHO) " init               - update menuconfig entries after adding \
#a new <ultra | distro | toolchain > directory"
	$(ECHO) " menuconfig         - top level Configure"
	$(ECHO) "                          NOTE: This allows you to Change/Update the selected Linux distribution "
	$(ECHO) "                        (uClinux, OpenWrt) and Enable/Disable U-Boot, etc."
	$(ECHO) " info               - see your current settings"
	$(ECHO) " ready              - check if binaries are ready to start loading to board"
	$(ECHO) " bootloader         - build <ultra>/projects/bootexec and <uboot> if enabled"
	$(ECHO) "                          NOTE: If U-Boot is disabled, only Ultra will be built. Use"
	$(ECHO) "                        'make ready' to see if *.elf files are ready."
	$(ECHO) " bootloader_X       - call 'make X' under <ultra>/projects/bootexec and <uboot> if enabled"
	$(ECHO) " install_bootloader - load bootloader to board"
	$(ECHO) " install            - load currently selected Linux distribution to board"
	$(ECHO) " install_all        - load bootloader and currently selected Linux distribution to board"
	$(ECHO) " clean              - clean currently selected Linux distribution build files"
	$(ECHO) " clean_all          - clean bootloader and currently selected Linux distribution build files"
	$(ECHO) " distclean          - distclean currently selected Linux distribution and bootloader and global ubicom-distro build files"
ifndef UBICOM_DISTRO_VERSION
	$(ECHO) " setup_toolchain    - try to symlink/download the toolchain"
endif

	$(ECHO) ""
	$(ECHO) "Advanced Targets"
	$(ECHO) " genconfig          - force to regenerate ultra configs"
	$(ECHO) "                          NOTE: If configs are never generated, they will already be generated internally"
	$(ECHO) " bootexec           - build <ultra>/projects/bootexec"
	$(ECHO) " bootexec_X         - build <ultra>/projects/bootexec target X"
	$(ECHO) " mainexec           - build <ultra>/projects/mainexec"
	$(ECHO) " mainexec_X         - build <ultra>/projects/mainexec target X"
ifeq ($(CONFIG_LOADER_UBOOT),y)
	$(ECHO) " uboot              - build <uboot>"
	$(ECHO) " uboot_X            - build in <uboot> target X"
	$(ECHO) " install_uboot      - load <uboot> binary (and bootexec)"
else
	$(ECHO) " install_bootexec   - load <ultra>/projects/bootexec binary"
endif
	$(ECHO) ""
	$(ECHO) "Please read README.build for additional information."
	$(ECHO) ""

########################################################################
# TODO: The below ones will be revised.
########################################################################

scripts/config/mconf:
	@$(MAKE) -s -C scripts/config all

$(eval $(call rdep,scripts/config,scripts/config/mconf))

scripts/config/conf:
	@$(MAKE) -s -C scripts/config conf

########################################################################
# Produce directory configs also
########################################################################
config: scripts/config/conf check_dir_config FORCE
	$(DO_X)
	$< Config.in
	$(MAKE) -f Makefile check_distro_run_config

check_distro_run_config:
ifeq ($(CONFIG_DISTRO_MENUCONFIG),y)
	@sed -i '/^CONFIG_DISTRO_MENUCONFIG=/ c\# CONFIG_DISTRO_MENUCONFIG is not set' .config
	$(MAKE) distro_config
endif

config-clean: FORCE
	$(MAKE) -C scripts/config clean

defconfig: scripts/config/conf FORCE
	touch .config
	$< -D .config Config.in

oldconfig: scripts/config/conf FORCE
	$< -$(if $(CONFDEFAULT),$(CONFDEFAULT),o) Config.in

########################################################################
# If user selects run distro menuconfig, popup it also.
# If user enable/disable u-boot or profiler tool, enable regenerate option
# if user didn't select it explicitly. Produce directory configs also
########################################################################
menuconfig: scripts/config/mconf check_dir_config FORCE
	$(DO_X)
	if [ \! -e .config -a -e $(HOME)/defconfig ]; then \
		cp $(HOME)/defconfig .config; \
	fi
	$< Config.in
	$(MAKE) -f Makefile check_distro_menuconfig
	$(MAKE) -f Makefile check_regenerate_conds \
		OLD_CONFIG_LOADER_UBOOT=$(CONFIG_LOADER_UBOOT) \
		OLD_CONFIG_IPPROFILE=$(CONFIG_IPPROFILE) \
		OLD_CONFIG_IPPROFILE_IPADDRESS=$(CONFIG_IPPROFILE_IPADDRESS) \
		OLD_CONFIG_IPPROFILE_RATE=$(CONFIG_IPPROFILE_RATE)

check_distro_menuconfig: check_distro_presettings check_kernel_presettings
ifeq ($(CONFIG_DISTRO_MENUCONFIG),y)
	@sed -i '/^CONFIG_DISTRO_MENUCONFIG=/ c\# CONFIG_DISTRO_MENUCONFIG is not set' .config
	$(MAKE) distro_menuconfig
endif

########################################################################
# Compare old values and new values of the config parameters in which
# if there is an update, we must run ctool to regenerate configs.
########################################################################
define CheckRegenerateRequirement
	@if [ "$(CONFIG_LOADER_UBOOT)" != "$(OLD_CONFIG_LOADER_UBOOT)" -o \
		"$(OLD_CONFIG_IPPROFILE)" != "$(CONFIG_IPPROFILE)" -o \
		"$(OLD_CONFIG_IPPROFILE_IPADDRESS)" != "$(CONFIG_IPPROFILE_IPADDRESS)" -o \
		"$(OLD_CONFIG_IPPROFILE_RATE)" != "$(CONFIG_IPPROFILE_RATE)" ]; then \
		sed -i '/^# CONFIG_RUN_CTOOL/ c\CONFIG_RUN_CTOOL=y' .config; \
	fi
endef

check_regenerate_conds:
	$(DO_X)
	$(call CheckRegenerateRequirement)

GPL :
	@echo "Creating GPL..."

ifeq ($(PRODUCT), DIR857)
	@echo "Download DIR857 source tree form SVN server"
	svn co svn://172.21.49.143/UBICOM/SDK3.0/trunk/ubicom-distro-3.0
	svn co svn://172.21.49.143/UBICOM/IP8K/trunk/ubicom-distro_SDK_2.0/uClinux ubicom-distro-3.0/uClinux
	@echo "Deleting Ultra source code... "
	rm -rf ubicom-distro-3.0/ultra
	rsync -a --exclude='*/.svn/*' config/* ubicom-distro-3.0/config
	sed -i 's/rm -rf config\/Config.in.*//g' ubicom-distro-3.0/Makefile	
	@echo "Deleting driver source code... "
	rm -rf ubicom-distro-3.0/vendors/AR17 
	rm -rf ubicom-distro-3.0/vendors/ath_sag 
	rm -rf ubicom-distro-3.0/vendors/ath_sag_rc9
	rm -rf ubicom-distro-3.0/vendors/ath_wlan
	rm -rf ubicom-distro-3.0/vendors/ath_wlan_v730407
	rm -rf ubicom-distro-3.0/vendors/iphone
	rm -rf ubicom-distro-3.0/vendors/Kcodes
	rm -rf ubicom-distro-3.0/vendors/ralink_wlan*
	rm -rf ubicom-distro-3.0/vendors/rt3070
	rm -rf ubicom-distro-3.0/vendors/sxuptp
	rm -rf ubicom-distro-3.0/vendors/synopsys-otg
	rm -rf ubicom-distro-3.0/vendors/Ubicom_StreamEngine2
	rm -rf ubicom-distro-3.0/vendors/Ubicom_StreamEngine2_ip8K
	rm -rf ubicom-distro-3.0/vendors/Ubicom_WISH
	rm -rf ubicom-distro-3.0/vendors/Ubicom_WISH_ip8K
	@echo "Deleting application source code... "
	rm -rf ubicom-distro-3.0/uClinux/user/artblock
	rm -rf ubicom-distro-3.0/uClinux/user/cgi
	rm -rf ubicom-distro-3.0/uClinux/user/cmds
	rm -rf ubicom-distro-3.0/uClinux/user/crowdcontrol
	rm -rf ubicom-distro-3.0/uClinux/user/DCC
	rm -rf ubicom-distro-3.0/uClinux/user/fw_query
	rm -rf ubicom-distro-3.0/uClinux/user/gpio
	rm -rf ubicom-distro-3.0/uClinux/user/iplookup
	rsync -a --exclude='*/.svn/*' uClinux/user/httpd/* ubicom-distro-3.0/uClinux/user/httpd
	rm -rf ubicom-distro-3.0/uClinux/user/httpd/*.c
	sed -i 's/clean:/do_not_clean:/g' ubicom-distro-3.0/uClinux/user/httpd/Makefile
	rsync -a --exclude='*/.svn/*' uClinux/user/libplatform/* ubicom-distro-3.0/uClinux/user/libplatform
	rm -rf ubicom-distro-3.0/uClinux/user/libplatform/*.c
	sed -i 's/clean:/do_not_clean:/g' ubicom-distro-3.0/uClinux/user/libplatform/Makefile
	rm -rf ubicom-distro-3.0/uClinux/user/libxmo	
	rm -rf ubicom-distro-3.0/uClinux/user/mldc
	rsync -a --exclude='*/.svn/*' uClinux/user/nvram/* ubicom-distro-3.0/uClinux/user/nvram
	rm -rf ubicom-distro-3.0/uClinux/user/nvram/*.c
	sed -i 's/clean:/do_not_clean:/g' ubicom-distro-3.0/uClinux/user/nvram/Makefile
	rm -rf ubicom-distro-3.0/uClinux/user/nvram_linux
	rm -rf ubicom-distro-3.0/uClinux/user/rc
	rsync -a --exclude='*/.svn/*' uClinux/user/sutil/* ubicom-distro-3.0/uClinux/user/sutil
	rm -rf ubicom-distro-3.0/uClinux/user/sutil/*.c
	sed -i 's/clean:/do_not_clean:/g' ubicom-distro-3.0/uClinux/user/sutil/Makefile
	rm -rf ubicom-distro-3.0/uClinux/user/tftpd
	rm -rf ubicom-distro-3.0/uClinux/user/timer
	rm -rf ubicom-distro-3.0/uClinux/user/wcn
	rsync -a --exclude='*/.svn/*' uClinux/user/WebFileAccess/cgi-ws/* ubicom-distro-3.0/uClinux/user/WebFileAccess/cgi-ws
	rm -rf ubicom-distro-3.0/uClinux/user/WebFileAccess/cgi-ws/*.c
	sed -i 's/$(MAKE) -C cgi-ws//g' ubicom-distro-3.0/uClinux/user/WebFileAccess/Makefile
	sed -i 's/$(MAKE) -C cgi-ws $\@//g' ubicom-distro-3.0/uClinux/user/WebFileAccess/Makefile
	sed -i 's/clean :/do_not_clean:/g' ubicom-distro-3.0/uClinux/user/WebFileAccess/cgi-ws/Makefile
	@echo "Deleting Others Project folder... "
	rm -rf ubicom-distro-3.0/uClinux/vendors/Ubicom/DIR657*
	rm -rf ubicom-distro-3.0/uClinux/vendors/Ubicom/DIR827*
	rm -rf ubicom-distro-3.0/uClinux/vendors/Ubicom/DIR857AX*
	rm -rf ubicom-distro-3.0/uClinux/vendors/Ubicom/DIR857NA*
	@echo "Copy application binary to rootfs folder... "
	mkdir -p ubicom-distro-3.0/uClinux/vendors/Ubicom/DIR857G/rootfs/bin
	cp uClinux/romfs/bin/cli ubicom-distro-3.0/uClinux/vendors/Ubicom/DIR857G/rootfs/bin
	cp uClinux/romfs/sbin/crowdcontrol ubicom-distro-3.0/uClinux/vendors/Ubicom/DIR857G/rootfs/sbin
	cp uClinux/romfs/sbin/dcc ubicom-distro-3.0/uClinux/vendors/Ubicom/DIR857G/rootfs/sbin
	cp uClinux/romfs/sbin/fwqd ubicom-distro-3.0/uClinux/vendors/Ubicom/DIR857G/rootfs/sbin
	cp uClinux/romfs/sbin/gpio ubicom-distro-3.0/uClinux/vendors/Ubicom/DIR857G/rootfs/sbin
	cp uClinux/romfs/sbin/arpping ubicom-distro-3.0/uClinux/vendors/Ubicom/DIR857G/rootfs/sbin
	cp uClinux/romfs/sbin/httpd ubicom-distro-3.0/uClinux/vendors/Ubicom/DIR857G/rootfs/sbin
	cp uClinux/romfs/sbin/smac ubicom-distro-3.0/uClinux/vendors/Ubicom/DIR857G/rootfs/sbin
	cp uClinux/romfs/lib/libvct.so ubicom-distro-3.0/uClinux/vendors/Ubicom/DIR857G/rootfs/lib
	cp uClinux/romfs/lib/libxmo.so ubicom-distro-3.0/uClinux/vendors/Ubicom/DIR857G/rootfs/lib
	cp uClinux/romfs/sbin/mldc ubicom-distro-3.0/uClinux/vendors/Ubicom/DIR857G/rootfs/sbin
	cp uClinux/romfs/bin/nvram ubicom-distro-3.0/uClinux/vendors/Ubicom/DIR857G/rootfs/bin
	cp uClinux/romfs/lib/libnvram.so ubicom-distro-3.0/uClinux/vendors/Ubicom/DIR857G/rootfs/lib
	cp uClinux/romfs/sbin/rc ubicom-distro-3.0/uClinux/vendors/Ubicom/DIR857G/rootfs/sbin
	cp uClinux/romfs/lib/libsutil.so ubicom-distro-3.0/uClinux/vendors/Ubicom/DIR857G/rootfs/lib
	cp uClinux/romfs/lib/libproject.so ubicom-distro-3.0/uClinux/vendors/Ubicom/DIR857G/rootfs/lib
	cp uClinux/romfs/lib/libipv6.so ubicom-distro-3.0/uClinux/vendors/Ubicom/DIR857G/rootfs/lib
	cp uClinux/romfs/sbin/tftpd ubicom-distro-3.0/uClinux/vendors/Ubicom/DIR857G/rootfs/sbin
	cp uClinux/romfs/sbin/timer ubicom-distro-3.0/uClinux/vendors/Ubicom/DIR857G/rootfs/sbin
	cp uClinux/romfs/sbin/wcnd ubicom-distro-3.0/uClinux/vendors/Ubicom/DIR857G/rootfs/sbin
	@echo "Copy drivers binray to specify folder..."
	rm -rf uClinux/romfs/*
	mkdir -p uClinux/romfs/sbin
	make modules_romfs
	rsync -a uClinux/romfs/* ubicom-distro-3.0/uClinux/vendors/Ubicom/DIR857G/drivers
	@echo "Modify uClinux/Makefile to copy drivers in romfs (modules_install: line 169)..."
	sed -i '168a\\tcp -a $$\(ROOTDIR\)/vendors/Ubicom/DIR857G/drivers/* $$\(ROMFSDIR\)' ubicom-distro-3.0/uClinux/Makefile
	@echo "Modify config.modules file..."
	sed -i 's/CONFIG_ATH_WLAN_COMPILE_SOURCE=y/# CONFIG_ATH_WLAN_COMPILE_SOURCE is not set/g' ubicom-distro-3.0/uClinux/vendors/Ubicom/DIR857G/config.modules
	sed -i 's/CONFIG_MODULE_ATH_SAG_RC9=y/# CONFIG_MODULE_ATH_SAG_RC9 is not set/g' ubicom-distro-3.0/uClinux/vendors/Ubicom/DIR857G/config.modules
	sed -i 's/CONFIG_MODULE_SILEX_USB=y/# CONFIG_MODULE_SILEX_USB is not set/g' ubicom-distro-3.0/uClinux/vendors/Ubicom/DIR857G/config.modules
	sed -i 's/CONFIG_MODULE_UBICOM_STREAMENGINE2_IP8K=y/# CONFIG_MODULE_UBICOM_STREAMENGINE2_IP8K is not set/g' ubicom-distro-3.0/uClinux/vendors/Ubicom/DIR857G/config.modules
	sed -i 's/CONFIG_MODULE_SYNOPSYS_OTG=y/# CONFIG_MODULE_SYNOPSYS_OTG is not set/g' ubicom-distro-3.0/uClinux/vendors/Ubicom/DIR857G/config.modules
	sed -i 's/CONFIG_DWC_MODE_OTG=y/# CONFIG_DWC_MODE_OTG is not set/g' ubicom-distro-3.0/uClinux/vendors/Ubicom/DIR857G/config.modules
	sed -i 's/CONFIG_MODULE_UBICOM_WISH_IP8K=y/# CONFIG_MODULE_UBICOM_WISH_IP8K is not set/g' ubicom-distro-3.0/uClinux/vendors/Ubicom/DIR857G/config.modules
	sed -i 's/CONFIG_UBICOM_WISH_IP8K_COMPILE_SOURCE=y/# CONFIG_UBICOM_WISH_IP8K_COMPILE_SOURCE is not set/g' ubicom-distro-3.0/uClinux/vendors/Ubicom/DIR857G/config.modules
	@echo "Modify config.vendor"
	sed -i 's/CONFIG_USER_CLI=y/# CONFIG_USER_CLI is not set/g' ubicom-distro-3.0/uClinux/vendors/Ubicom/DIR857G/config.vendor
	sed -i 's/CONFIG_USER_CROWDCONTROL=y/# CONFIG_USER_CROWDCONTROL is not set/g' ubicom-distro-3.0/uClinux/vendors/Ubicom/DIR857G/config.vendor
	sed -i 's/CONFIG_USER_DCC_DCC=y/# CONFIG_USER_DCC_DCC is not set/g' ubicom-distro-3.0/uClinux/vendors/Ubicom/DIR857G/config.vendor
	sed -i 's/CONFIG_USER_FWQD=y/# CONFIG_USER_FWQD is not set/g' ubicom-distro-3.0/uClinux/vendors/Ubicom/DIR857G/config.vendor
	sed -i 's/CONFIG_USER_GPIO_GPIO=y/# CONFIG_USER_GPIO_GPIO is not set/g' ubicom-distro-3.0/uClinux/vendors/Ubicom/DIR857G/config.vendor
	sed -i 's/CONFIG_USER_IPLOOKUP_IPLOOKUP=y/# CONFIG_USER_IPLOOKUP_IPLOOKUP is not set/g' ubicom-distro-3.0/uClinux/vendors/Ubicom/DIR857G/config.vendor
	sed -i 's/CONFIG_USER_HTTPD_HTTPD=y/# CONFIG_USER_HTTPD_HTTPD is not set/g' ubicom-distro-3.0/uClinux/vendors/Ubicom/DIR857G/config.vendor
	sed -i 's/CONFIG_USER_LIBPLATFORM=y/# CONFIG_USER_LIBPLATFORM is not set/g' ubicom-distro-3.0/uClinux/vendors/Ubicom/DIR857G/config.vendor
	sed -i 's/dir_y += mldc//g' ubicom-distro-3.0/uClinux/vendors/Ubicom/DIR857G/config.vendor
	sed -i 's/CONFIG_USER_NVRAM=y/# CONFIG_USER_NVRAM is not set/g' ubicom-distro-3.0/uClinux/vendors/Ubicom/DIR857G/config.vendor
	sed -i 's/CONFIG_USER_RC=y/# CONFIG_USER_RC is not set/g' ubicom-distro-3.0/uClinux/vendors/Ubicom/DIR857G/config.vendor
	sed -i 's/CONFIG_USER_SUTIL=y/# CONFIG_USER_SUTIL is not set/g' ubicom-distro-3.0/uClinux/vendors/Ubicom/DIR857G/config.vendor
	sed -i 's/CONFIG_USER_TFTPD=y/# CONFIG_USER_TFTPD is not set/g' ubicom-distro-3.0/uClinux/vendors/Ubicom/DIR857G/config.vendor
	sed -i 's/CONFIG_USER_TIMER_TIMER=y/# CONFIG_USER_TIMER_TIMER is not set/g' ubicom-distro-3.0/uClinux/vendors/Ubicom/DIR857G/config.vendor
	sed -i 's/CONFIG_USER_WCN=y/# CONFIG_USER_WCN is not set/g' ubicom-distro-3.0/uClinux/vendors/Ubicom/DIR857G/config.vendor
	@echo "Modify uClinux/vendors/Ubicom/DIR857G/Makefile..."
	sed -i 's/romfs/no_romfs/g' ubicom-distro-3.0/uClinux/vendors/Ubicom/DIR857G/Makefile
	@echo "Deleting .svn folders..."
	find ubicom-distro-3.0 -type d -name ".svn" | xargs rm -rf
	tar czvf ubicom-distro-3.0.tgz ubicom-distro-3.0
endif

########################################################################
# SILENT, PHONY, and NOTPARALLEL
########################################################################
ifeq ($(findstring v,$(DEBUG)),)
  .SILENT: clean distclean help .config scripts/config/mconf scripts/config/conf menuconfig
endif
.PHONY: HELP FORCE check_toolchain setup_toolchain
.NOTPARALLEL:

