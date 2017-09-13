#
# wrapper of Atheros driver makefiles
#
.EXPORT_ALL_VARIABLES:
.PHONY: all wlan art prebuilt strip app clean romfs romfs_mod romfs_app

export KERNELARCH=ubicom32
export BUILD_UMAC=1
export BUS=PCI
export TARGETARCH=ubicom32-elf
export ATH_SUPPORT_HTC=0

#
## Environmental variables used to configure the HAL/ATH layers at compile time
#
#
export ATH_RXBUF=1024
export ATH_BEACON_DEFERRED_PROC=1
export ATH_CAP_AMSDU=0
export REMOVE_PKT_LOG=1
export ATH_SUPPORT_IQUE=1
export IEEE_MCAST_ENHANCEMENT=0
export ATH_WPS_IE=1
export ATH_SUPPORT_PAPRD=1
export ATH_SUPPORT_DYN_TX_CHAINMASK=1
export ATH_EXT_AP=1
export ATH_SUPPORT_QUICK_KICKOUT=1
#export ATH_SUPPORT_TxBF=1
export ATH_SUPPORT_IBSS=0
export UMAC_SUPPORT_IBSS=0
export UMAC_SUPPORT_ACS=1
export UMAC_SUPPORT_WDS=0
export UMAC_SUPPORT_NAWDS=0
export UMAC_SUPPORT_APONLY=1
export UMAC_SUPPORT_SCAN=0
export UMAC_SUPPORT_APLIST=0
export ATH_SUPPORT_WIRESHARK=0
export ATH_DEBUG=0
export DBG=0
export ATHEROS_LINUX_PERIODIC_SCAN=0
export ATH_THREAD=0

ifndef CONFIG_ATH_WLAN_SIMPLE_CONFIG
NO_SIMPLE_CONFIG=1
else
NO_SIMPLE_CONFIG=0
endif

ifdef CONFIG_ATH_WLAN_DFS
export ATH_SUPPORT_DFS=1
else
export ATH_SUPPORT_DFS=0
endif

ifndef ATH_LSDK_DIR
ATH_LSDK_DIR := $(shell pwd)
endif

ATH_WLAN_DIR := $(ATH_LSDK_DIR)
ATH_HAL_DIR := $(ATH_WLAN_DIR)/hal

ATH_APP_DIR := $(ATH_LSDK_DIR)/install/apps
ATH_MOD_DIR := $(ATH_LSDK_DIR)/install/mods
ATH_STRIP_DIR := $(ATH_LSDK_DIR)/install/strip

HAL_TARGET=$(KERNEL_CONFIGURE_HOST)
KERNELPATH=$(ROOTDIR)/$(LINUXDIR)

TARGET=$(HAL_TARGET)

ifndef CONFIG_ATH_WLAN_COMPILE_SOURCE
ATH_STRIP :=
else

ifdef CONFIG_ATH_WLAN
ATH_MODULES += wlan
ATH_STRIP = strip
endif

ifdef CONFIG_ATH_ART
ATH_MODULES += art
ATH_STRIP = strip
endif

ifdef CONFIG_ATH_APPS
ATH_MODULES += app
endif
endif

#
# The set of loadable modules to be copied
#
ATH_MODS := $(ATH_WLAN_DIR)/os/linux/ath_hal/*.ko
ATH_MODS += $(ATH_WLAN_DIR)/adf/*.ko
ATH_MODS += $(ATH_WLAN_DIR)/asf/*.ko
ATH_MODS += $(ATH_WLAN_DIR)/umac/*.ko
ATH_MODS += $(ATH_WLAN_DIR)/lmac/ath_dev/*.ko
ATH_MODS += $(ATH_WLAN_DIR)/lmac/ath_pktlog/*.ko
ATH_MODS += $(ATH_WLAN_DIR)/lmac/ratectrl/*.ko
ifeq ($(strip ${ATH_SUPPORT_HTC}),1)
ATH_MODS += $(ATH_WLAN_DIR)/htc/*.ko
ATH_MODS += $(ATH_WLAN_DIR)/htc/hif/*.ko
endif
ifdef CONFIG_ATH_WLAN_DFS
ATH_MODS += $(ATH_WLAN_DIR)/lmac/dfs/*.ko
endif

ifdef CONFIG_ATH_WLAN_COMPILE_SOURCE
ifdef CONFIG_ATH_WLAN
ROMFS_MOD=romfs_mod
endif

ifdef CONFIG_ATH_ART
ROMFS_MOD=romfs_mod
endif

ifdef CONFIG_ATH_APPS
ROMFS_APP=romfs_app
endif
endif

all: $(ATH_MODULES) $(ATH_STRIP)

wlan:
	# build HAL
	$(MAKE) -C $(ATH_HAL_DIR)/linux CROSS_COMPILE=$(KERNEL_CROSS_COMPILE) $(HAL_TARGET) 
	$(MAKE) -C $(ATH_HAL_DIR)/linux CROSS_COMPILE=$(KERNEL_CROSS_COMPILE) $(HAL_TARGET)-release
	# build WLAN drivers
	$(MAKE) -C $(ATH_WLAN_DIR)/os/linux ARCH=$(ARCH) CROSS_COMPILE=$(KERNEL_CROSS_COMPILE) TARGET=$(HAL_TARGET)
	# keep a local copy of unstripped .kos
	mkdir -p $(ATH_MOD_DIR)
	cp $(ATH_MODS) $(ATH_MOD_DIR)

art:
	#build ART modules, pick PB42 as the closest target
	cd $(ATH_LSDK_DIR)/art && $(MAKE) ARCH=$(ARCH) PB42=1 UBICOM_IP8K=1 DEBUG=$(ART_DEBUG) CROSS_COMPILE=$(KERNEL_CROSS_COMPILE) -C $(KERNELPATH) M=$(ATH_LSDK_DIR)/art/modules modules

	# keep a local copy of unstripped .kos
	mkdir -p $(ATH_MOD_DIR)
	cp $(ATH_LSDK_DIR)/art/modules/art.ko $(ATH_MOD_DIR)

strip:
	rm -rf $(ATH_STRIP_DIR)
	mkdir -p $(ATH_STRIP_DIR)
	cd $(ATH_MOD_DIR); sh $(ROOTDIR)/modules/move-text-to-ocm.sh $(ATH_LSDK_DIR)/ocm_text_null.txt
	cp $(ATH_MOD_DIR)/*.ko $(ATH_STRIP_DIR)
	cd $(ATH_STRIP_DIR); $(STRIP) -S *.ko

app:
	# build applications
	$(MAKE) -f Makefile.apps 

clean:
	$(MAKE) -C $(ATH_HAL_DIR)/linux clean
	$(MAKE) -C $(ATH_WLAN_DIR)/os/linux clean
	cd $(ATH_LSDK_DIR)/art/modules && make clean
	$(MAKE) -f Makefile.apps clean
	rm -rf $(ATH_MOD_DIR)
	rm -rf $(ATH_STRIP_DIR)
	rm -rf $(ATH_APP_DIR)
	rm -rf $(ATH_LSDK_DIR)/install

romfs: $(ROMFS_MOD) $(ROMFS_APP)

romfs_mod:
#	mkdir -p $(ROMFSDIR)/lib/modules/wireless
#	rm -f $(ROMFSDIR)/lib/modules/wireless/*.ko
	cp $(ATH_STRIP_DIR)/*.ko $(ROMFSDIR)/lib/modules/

romfs_app:
	mkdir -p $(ROMFSDIR)/bin
	$(MAKE) -f Makefile.apps romfs

