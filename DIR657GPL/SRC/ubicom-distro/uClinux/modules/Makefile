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

dir_$(CONFIG_MODULE_ATH_WLAN_9B21)			+= modules_romfs
dir_$(CONFIG_MODULE_ATH_WLAN_9B21)			+= ath_wlan_9b21
dir_$(CONFIG_MODULE_SILEX_USB)				+= sxuptp
dir_$(CONFIG_MODULE_UBICOM_STREAMENGINE2)	+= Ubicom_StreamEngine2
dir_$(CONFIG_MODULE_UBICOM_WISH)			+= Ubicom_WISH


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

