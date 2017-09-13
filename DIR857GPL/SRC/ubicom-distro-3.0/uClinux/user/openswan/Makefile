
CFLAGS := $(subst -Werror,,$(CFLAGS))

.EXPORT_ALL_VARIABLES:

FINALLIBEXECDIR="/bin"
ifdef CONFIG_USER_FLATFSD_FLATFSD
FINALCONFDIR        := "/etc/config"
FINALCONFDDIR       := "/etc/config"
USE_SINGLE_CONF_DIR := true
USE_WEAKSTUFF       := true
HAVE_DYNAMICDNS     := true
USE_DEFAULT_CONNS   := "no"
endif
FINALSBINDIR="/bin"
FINALLIBDIR="/bin"
OPENSWAN_DIR = $(ROOTDIR)/openswan-2.6.24rc4
CUR_DIR=$(shell pwd)
#ROMFSDIR=/share/rootfs/openswan
programs:  dummy_target
	if [ -d $(OPENSWAN_DIR) ]; then \
		ROOTDIR="$(ROOTDIR)" LDFLAGS="$(LDFLAGS) -L$(ROOTDIR)/lib/libgmp/build/install/lib/" LDLIBS="$(LDLIBS)" \
		$(MAKE) -C $(OPENSWAN_DIR) $@ \
		USERCOMPILE="-I$(ROOTDIR)/lib/libgmp/build/install/include $(LDFLAGS) -L$(ROOTDIR)/lib/libgmp/build/install/lib/";  \
	fi

clean:  dummy_target
	if [ -d $(OPENSWAN_DIR) ]; then \
		ROOTDIR="$(ROOTDIR)" CFLAGS="$(CFLAGS)" LDLIBS="$(LDLIBS)" $(MAKE) -C $(OPENSWAN_DIR) $@ ; \
	fi

romfs:
	$(ROMFSINST) -e CONFIG_USER_OPENSWAN_PLUTO_PLUTO $(OPENSWAN_DIR)/OBJ.linux.ubicom32/programs/pluto/pluto /bin/pluto
	$(ROMFSINST) -e CONFIG_USER_OPENSWAN_PLUTO_PLUTO $(OPENSWAN_DIR)/OBJ.linux.ubicom32/programs/pluto/_pluto_adns /bin/_pluto_adns
	$(ROMFSINST) -e CONFIG_USER_OPENSWAN_PLUTO_WHACK $(OPENSWAN_DIR)/OBJ.linux.ubicom32/programs/pluto/whack /bin/whack
	$(ROMFSINST) -e CONFIG_USER_OPENSWAN_UTILS_RANBITS $(OPENSWAN_DIR)/OBJ.linux.ubicom32/programs/ranbits/ranbits /bin/ranbits
	$(ROMFSINST) -e CONFIG_USER_OPENSWAN_UTILS_RSASIGKEY $(OPENSWAN_DIR)/OBJ.linux.ubicom32/programs/rsasigkey/rsasigkey /bin/rsasigkey
	$(ROMFSINST) -e CONFIG_USER_OPENSWAN_KLIPS_EROUTE $(OPENSWAN_DIR)/OBJ.linux.ubicom32/programs/eroute/eroute /bin/eroute
	$(ROMFSINST) -e CONFIG_USER_OPENSWAN_KLIPS_KLIPSDEBUG $(OPENSWAN_DIR)/OBJ.linux.ubicom32/programs/klipsdebug/klipsdebug /bin/klipsdebug
	$(ROMFSINST) -e CONFIG_USER_OPENSWAN_KLIPS_SPI $(OPENSWAN_DIR)/OBJ.linux.ubicom32/programs/spi/spi /bin/spi
	$(ROMFSINST) -e CONFIG_USER_OPENSWAN_KLIPS_SPIGRP $(OPENSWAN_DIR)/OBJ.linux.ubicom32/programs/spigrp/spigrp /bin/spigrp
	$(ROMFSINST) -e CONFIG_USER_OPENSWAN_KLIPS_TNCFG $(OPENSWAN_DIR)/OBJ.linux.ubicom32/programs/tncfg/tncfg /bin/tncfg
	#$(ROMFSINST) -e CONFIG_USER_OPENSWAN_PROGRAMS_LWDNSQ $(OBJDIRTOP)/OBJ.linux.ubicom32/programs/lwdnsq/lwdnsq /bin/lwdnsq
ifndef CONFIG_USER_IPSEC_CONFIGURATOR
ifndef CONFIG_PROP_IFMOND_IPSEC
	#$(ROMFSINST) -p 555 -e CONFIG_USER_OPENSWAN $(OPENSWAN_DIR)/OBJ.linux.ubicom32/programs/look/look /bin/look
	#$(ROMFSINST) -p 555 -e CONFIG_USER_OPENSWAN $(OPENSWAN_DIR)/OBJ.linux.ubicom32/programs/secrets/secrets /bin/secrets
	#$(ROMFSINST) -p 555 -e CONFIG_USER_OPENSWAN $(OPENSWAN_DIR)/OBJ.linux.ubicom32/programs/showhostkey/showhostkey /bin/showhostkey
	$(ROMFSINST) -p 555 -e CONFIG_USER_OPENSWAN $(OPENSWAN_DIR)/OBJ.linux.ubicom32/programs/_updown/_updown /bin/_updown
	$(ROMFSINST) -p 555 -e CONFIG_USER_OPENSWAN $(OPENSWAN_DIR)/OBJ.linux.ubicom32/programs/_updown.netkey/_updown.netkey /bin/_updown.netkey
	#$(ROMFSINST) -p 555 -e CONFIG_USER_OPENSWAN $(OPENSWAN_DIR)/OBJ.linux.ubicom32/programs/auto/auto /bin/auto
	#$(ROMFSINST) -p 555 -e CONFIG_USER_OPENSWAN $(OPENSWAN_DIR)/OBJ.linux.ubicom32/programs/_copyright/_copyright /bin/_copyright
	#$(ROMFSINST) -p 555 -e CONFIG_USER_OPENSWAN $(OPENSWAN_DIR)/OBJ.linux.ubicom32/programs/_keycensor/_keycensor /bin/_keycensor
	#$(ROMFSINST) -p 555 -e CONFIG_USER_OPENSWAN $(OPENSWAN_DIR)/OBJ.linux.ubicom32/programs/_plutoload/_plutoload /bin/_plutoload
	#$(ROMFSINST) -p 555 -e CONFIG_USER_OPENSWAN $(OPENSWAN_DIR)/OBJ.linux.ubicom32/programs/_realsetup/_realsetup /bin/_realsetup
	-$(ROMFSINST) -p 555 -e CONFIG_USER_OPENSWAN $(OPENSWAN_DIR)/OBJ.linux.ubicom32/programs/send-pr/send-pr /bin/send-pr
	#$(ROMFSINST) -p 555 -e CONFIG_USER_OPENSWAN $(OPENSWAN_DIR)/OBJ.linux.ubicom32/programs/barf/barf /bin/barf
	#$(ROMFSINST) -p 555 -e CONFIG_USER_OPENSWAN $(OPENSWAN_DIR)/OBJ.linux.ubicom32/programs/ikeping/ikeping /bin/ikeping
	#$(ROMFSINST) -p 555 -e CONFIG_USER_OPENSWAN $(OPENSWAN_DIR)/OBJ.linux.ubicom32/programs/newhostkey/newhostkey /bin/newhostkey
	$(ROMFSINST) -p 555 -e CONFIG_USER_OPENSWAN $(OPENSWAN_DIR)/OBJ.linux.ubicom32/programs/_plutorun/_plutorun /bin/_plutorun
	#$(ROMFSINST) -p 555 -e CONFIG_USER_OPENSWAN $(OPENSWAN_DIR)/OBJ.linux.ubicom32/programs/_startklips/_startklips /bin/_startklips
	#$(ROMFSINST) -p 555 -e CONFIG_USER_OPENSWAN $(OPENSWAN_DIR)/OBJ.linux.ubicom32/programs/verify/verify /bin/verify
	-$(ROMFSINST) -p 555 -e CONFIG_USER_OPENSWAN $(OPENSWAN_DIR)/OBJ.linux.ubicom32/programs/calcgoo/calcgoo /bin/calcgoo
	#$(ROMFSINST) -p 555 -e CONFIG_USER_OPENSWAN $(OPENSWAN_DIR)/OBJ.linux.ubicom32/programs/_include/_include /bin/_include
	#$(ROMFSINST) -p 555 -e CONFIG_USER_OPENSWAN $(OPENSWAN_DIR)/OBJ.linux.ubicom32/programs/livetest/livetest /bin/livetest
	#$(ROMFSINST) -p 555 -e CONFIG_USER_OPENSWAN $(OPENSWAN_DIR)/OBJ.linux.ubicom32/programs/pf_key/pf_key /bin/pf_key
	#$(ROMFSINST) -p 555 -e CONFIG_USER_OPENSWAN $(OPENSWAN_DIR)/OBJ.linux.ubicom32/programs/_secretcensor/_secretcensor /bin/_secretcensor
	#$(ROMFSINST) -p 555 -e CONFIG_USER_OPENSWAN $(OPENSWAN_DIR)/OBJ.linux.ubicom32/programs/showdefaults/showdefaults /bin/showdefaults
	$(ROMFSINST) -p 555 -e CONFIG_USER_OPENSWAN $(OPENSWAN_DIR)/OBJ.linux.ubicom32/programs/ipsec/ipsec /bin/ipsec
	#-$(ROMFSINST) -p 555 -e CONFIG_USER_OPENSWAN $(OPENSWAN_DIR)/OBJ.linux.ubicom32/programs/setup/setup /bin/setup
ifndef CONFIG_SHELL_SUPPORT_FUNCTION
	#
	# Overwrite the original version with spin off shell functions into seperate shell scripts
	#
	#$(ROMFSINST) -p 555 -e CONFIG_USER_OPENSWAN $(CUR_DIR)/ipsec /bin/ipsec
	#$(ROMFSINST) -p 555 -e CONFIG_USER_OPENSWAN $(CUR_DIR)/_updown /bin/_updown
	$(ROMFSINST) -p 555 -e CONFIG_USER_OPENSWAN $(CUR_DIR)/addsource /bin/addsource
	$(ROMFSINST) -p 555 -e CONFIG_USER_OPENSWAN $(CUR_DIR)/changesource /bin/changesource
	$(ROMFSINST) -p 555 -e CONFIG_USER_OPENSWAN $(CUR_DIR)/doroute /bin/doroute
	$(ROMFSINST) -p 555 -e CONFIG_USER_OPENSWAN $(CUR_DIR)/dorule /bin/dorule
	$(ROMFSINST) -p 555 -e CONFIG_USER_OPENSWAN $(CUR_DIR)/downrule /bin/downrule
	$(ROMFSINST) -p 555 -e CONFIG_USER_OPENSWAN $(CUR_DIR)/uprule /bin/uprule
	$(ROMFSINST) -p 555 -e CONFIG_USER_OPENSWAN $(CUR_DIR)/pluto_updown /etc/sysconfig/pluto_updown
	#
	# Copy over reference setup scripts before we port all ipsec shell scripts
	#
	$(ROMFSINST) -p 555 -e CONFIG_USER_OPENSWAN $(CUR_DIR)/ipsec.secrets /etc/ipsec.secrets
	#$(ROMFSINST) -p 555 -e CONFIG_USER_OPENSWAN $(CUR_DIR)/ipsec_uc_start /bin/ipsec_uc_start
	#$(ROMFSINST) -p 555 -e CONFIG_USER_OPENSWAN $(CUR_DIR)/ipsec_uc_stop /bin/ipsec_uc_stop
endif
endif
endif

dummy_target:
