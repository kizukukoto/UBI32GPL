#
#       Makefile -- Build instructions for loadable modules
#
#
.EXPORT_ALL_VARIABLES:
.PHONY: all romfs clean prune

#
# Include architecture specific build rules.
#
#
ifndef ROOTDIR
ROOTDIR=..
endif

UCLINUX_BUILD_MODULE=1
-include $(LINUX_CONFIG)
-include $(CONFIG_CONFIG)
-include $(ARCH_CONFIG)
-include $(MODULES_CONFIG)


dir_y :=
dir_n :=

dir_$(CONFIG_MODULE_ATH_SAG_RC11)                       += ath_sag_rc11
dir_$(CONFIG_MODULE_ATH_SAG_RC9)			+= ath_sag_rc9
dir_$(CONFIG_MODULE_ATH_SAG)				+= ath_sag
dir_$(CONFIG_MODULE_ATH_WLAN)				+= ath_wlan
dir_$(CONFIG_MODULE_ATH_WLAN_9B21)			+= ath_wlan_9b21
dir_$(CONFIG_MODULE_ATH_WLAN_9B55)			+= ath_wlan_9b55
dir_$(CONFIG_MODULE_ATH_AR17)				+= AR17
dir_$(CONFIG_RALINK_WLAN_RT3052_iNIC)		+= ralink_wlan_rt3052_iNIC
dir_$(CONFIG_RALINK_WLAN_RT3883_iNIC)		+= ralink_wlan_rt3883_iNIC
dir_$(CONFIG_MODULE_SILEX_USB)				+= sxuptp
dir_$(CONFIG_MODULE_SYNOPSYS_OTG)         		+= synopsys-otg
dir_$(CONFIG_RALINK_WLAN_RT3070_USB)		+= ralink_wlan_rt3070_USB
dir_$(CONFIG_MODULE_UBICOM_STREAMENGINE)	+= Ubicom_StreamEngine
dir_$(CONFIG_MODULE_UBICOM_STREAMENGINE2)	+= Ubicom_StreamEngine2
dir_$(CONFIG_MODULE_UBICOM_STREAMENGINE2_IP8K)	+= Ubicom_StreamEngine2_ip8K
dir_$(CONFIG_MODULE_UBICOM_WISH)			+= Ubicom_WISH
dir_$(CONFIG_MODULE_UBICOM_WISH_IP8K)			+= Ubicom_WISH_ip8K
dir_$(CONFIG_MODULE_IPHONE)					+= iphone
dir_$(CONFIG_MODULE_KCODES)					+= Kcodes

all:
	[ -z "$(dir_y)"] || $(MAKE) -j$(HOST_NCPU) $(sort $(dir_y)) || exit $$?

include $(ROOTDIR)/config/Makefile.conf

.PHONY: oldconfig config menuconfig gconfig qconfig xconfig

oldconfig: conf
	$(SCRIPTSDIR)/conf -o Kconfig

config: conf
	$(SCRIPTSDIR)/conf Kconfig

menuconfig: mconf
	$(SCRIPTSDIR)/mconf Kconfig

gconfig: gconf
	$(SCRIPTSDIR)/gconf Kconfig

qconfig: qconf
	$(SCRIPTSDIR)/qconf Kconfig

xconfig: gconfig

#
# add directory dependancies here
#
.PHONY: $(sort $(dir_y))
$(sort $(dir_y)):
	[ ! -d "$@" ] || ( touch $@/.sgbuilt_module && $(MAKE) -j1 -C $@ ) || exit $$?

%_only:
	touch $(@:_only=)/.sgbuilt_module && $(MAKE) -j1 -C $(@:_only=)

%_clean:
	$(MAKE) -j1 -C $(@:_clean=) clean; rm -f $(@:_clean=)/.sgbuilt_module; true

%_romfs:
	$(MAKE) -j1 -C $(@:_romfs=) romfs

romfs:
	for i in $(sort $(dir_y)) ; do \
		[ ! -d $$i ] || $(MAKE) -C $$i romfs || exit $$? ; \
	done

clean:
	-for i in $(sort $(dir_y) $(dir_n)) ; do \
		if [ -f $$i/.sgbuilt_module ]; then \
			$(MAKE) -C $$i clean ; \
			rm -f $$i/.sgbuilt_module; \
		fi; \
	done

prune:
	-for i in $(sort $(dir_n) $(dir_)) ; do \
		found=0; \
		for j in $(sort $(dir_y)) ; do \
			if [ $$i = $$j ]; then \
				found=1; \
			fi; \
		done; \
		if [ $$found = 0 ]; then \
			rm -fr $$i; \
		fi; \
	done

