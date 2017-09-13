TOPDIR:=../..
include $(TOPDIR)/../.config
ULTRA_DIR=$(TOPDIR)/../ultra_public
ULTRA_CONFIG_DIR=$(ULTRA_DIR)/projects/bootexec/config
OUT_DIR=$(TOPDIR)/../bin
BOARD_NAME=$(patsubst %.brd,%,$(subst ",,$(CONFIG_BOARD_NAME)))
LPJ_NAME=$(patsubst %.lpj,%,$(subst ",,$(CONFIG_LPJ_NAME)))

CONFIG_FILES:=config.h config.mk

ifeq ($(CONFIG_USE_PREBUILD_ULTRA),y)

all: copy_ultra

copy_ultra:
	for file in $(CONFIG_FILES); do \
		cp -rf ./files/$(LPJ_NAME)/$(BOARD_NAME)/$$file $(ULTRA_CONFIG_DIR)/ ; \
	done
	for file in bootexec mainexec; do \
		mkdir -p $(ULTRA_DIR)/projects/$${file}; \
		cp -rf ./files/$(LPJ_NAME)/$(BOARD_NAME)/$${file}_ultra.elf $(ULTRA_DIR)/projects/$${file}/ultra.elf ; \
	done
	mkdir -p $(ULTRA_DIR)/projects/bootexec/build/bin
	cp -rf ./files/$(LPJ_NAME)/$(BOARD_NAME)/bd.bin $(ULTRA_DIR)/projects/bootexec/build/bin/bd.bin
	cp -rf ./files/$(LPJ_NAME)/$(BOARD_NAME)/ocm_size.h $(TOPDIR)/$(LINUXDIR)/include/asm-ubicom32/ocm_size.h

romfs: copy_ultra

clean:
	for file in $(CONFIG_FILES); do \
		rm -rf $(ULTRA_CONFIG_DIR)/$$file; \
	done
	for file in bootexec mainexec; do \
		rm -rf $(ULTRA_DIR)/projects/$$file/ultra.elf ; \
	done
	rm -rf $(ULTRA_DIR)/projects/build/bin/bd.bin
	rm -rf $(LINUX_DIR)/include/asm-ubicom32/ocm_size.h

else

all:

romfs:

clean:

endif

.PHONY: all clean romfs

