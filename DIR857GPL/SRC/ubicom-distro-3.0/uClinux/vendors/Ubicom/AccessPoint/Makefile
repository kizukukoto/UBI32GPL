#
#	Makefile -- Build instructions for Ubicom32 / IP5K
#
# include the common rules for creating/cleaning a root filesystem
include $(ROOTDIR)/vendors/config/ubicom32nommu/Makefile.common

romfs:
ifndef CONFIG_ATH_WLAN_COMPILE_SOURCE
	$(ROMFSINST) -S wlan/lib /lib
	$(ROMFSINST) wlan/apps/bin /bin
	$(ROMFSINST) wlan/apps/sbin /sbin
endif

