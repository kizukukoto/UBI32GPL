#
# Board dependent Makefile for WRG-ND14
#

MYNAME	:= WRG-ND14
MKSQFS	:= ./tools/squashfs-tools-4.0/mksquashfs

SEAMA	:= ./tools/seama/seama
LZMA	:= ./tools/lzma/lzma
MYMAKE	:= $(Q)make V=$(V) DEBUG=$(DEBUG)
FWDEV	:= /dev/mtdblock/1
UBOOTDEV:= /dev/mtdblock/6
SVNREV	:= $(shell svn info $(TOPDIR) | grep Revision: | cut -f2 -d' ')
ROUTER_VER := $(shell cat $(TARGET)/etc/config/buildver)

ifeq ($(strip $(ELBOX_USE_IPV6)),y)
KERNELCONFIG := kernel.aries.ipv6.config
else
KERNELCONFIG := kernel.aries.config
endif

ifdef PREDEFINE_RELIMAGE
RELIMAGE:=$(PREDEFINE_RELIMAGE)
UBOOTIMAGE:=$(PREDEFINE_UBOOTIMAGE)
else
BUILDNO :=$(shell cat buildno)
RELIMAGE:=$(shell echo My_Net_N900_Central_$(ELBOX_FIRMWARE_VERSION)_$(BUILDNO))
UBOOTIMAGE:=$(shell echo $(ELBOX_MODEL_NAME)_uboot_v$(SVNREV)_$(BUILDNO))
endif

#############################################################################
# This one will be make in fakeroot.
fakeroot_rootfs_image:
	@rm -f fakeroot.rootfs.img
	@./progs.board/makedevnodes rootfs
	$(Q)$(MKSQFS) rootfs fakeroot.rootfs.img $(MKSQFS_BLOCK)

.PHONY: rootfs_image

#############################################################################
# The real image files

$(ROOTFS_IMG): $(MKSQFS) $(LZMA)
	@echo -e "\033[32m$(MYNAME): building squashfs (LZMA)!\033[0m"
	$(Q)make clean_CVS
	$(Q)fakeroot make -f progs.board/config.mk fakeroot_rootfs_image
	$(Q)mv fakeroot.rootfs.img $(ROOTFS_IMG)
	$(Q)chmod 664 $(ROOTFS_IMG)
	$(Q)cp $(ROOTFS_IMG) progs.board/ubicom/bin/.

$(MKSQFS) $(SEAMA) $(LZMA):
	$(Q)make -C $(dir $@)

##########################################################################

rootfs_image:
	@echo -e "\033[32m$(MYNAME): creating rootfs image ...\033[0m"
	$(Q)rm -f $(ROOTFS_IMG)
	$(MYMAKE) $(ROOTFS_IMG)

.PHONY: rootfs_image

ifeq ($(strip $(CONFIG_CGIBIN_ARIES_FIRMWARE_IN_HD)),y)
ROOTFS_HD:=rootfs_hd
ROOTFS_HD_IMG:=rootfs_hd.tar
HD_INSDIR:=/internalhd/root/
HD_TMPDIR:=/internalhd/tmp/
ORION_VER := 179
rootfs_hd:
	@echo -e "\033[32m$(MYNAME): making rootfs_hd ...\033[0m"
	$(Q)mkdir -p $(TOPDIR)$(HD_INSDIR)etc/config
	$(Q)echo $(ELBOX_FIRMWARE_VERSION) > $(TOPDIR)$(HD_INSDIR)etc/config/buildver
	$(Q)echo $(ELBOX_FIRMWARE_REVISION) > $(TOPDIR)$(HD_INSDIR)etc/config/buildrev
	$(Q)SVNREV=`svn info | head -n 5 | tail -n 1 | cut -f 2 -d' '`
	$(Q)echo "$$SVNREV" > $(TOPDIR)$(HD_INSDIR)etc/config/svnrev
	$(Q)tar -czf $(ROOTFS_HD_IMG) $(CONFIG_CGIBIN_ARIES_IMAGE_INTERIM_DIR)
	$(Q)$(SEAMA) -i $(ROOTFS_HD_IMG) \
		-m dev=$(HD_TMPDIR)rootfs_hd.tar \
		-m noheader=1 \
		-m Orion_ver=$(ORION_VER)
#		-m prior="/var/bin/busybox mount -O sync /dev/sda1 $(HD_INSDIR); /var/bin/busybox sleep 3" \
#		-m prior="/var/bin/busybox mount -O sync /dev/sda3 $(HD_TMPDIR); /var/bin/busybox sleep 3" \
#		-m post="/var/bin/busybox echo \"Preparing to install firmware in the internal HDD\"" \
#		-m post="/var/bin/busybox fuser -km $(HD_INSDIR); rm -rf $(HD_INSDIR)/*" \
#		-m post="/var/bin/busybox echo start > $(HD_INSDIR)HD_upgrade_start" \
#		-m post="/var/bin/busybox tar -xzf $(HD_TMPDIR)rootfs_hd.tar" \
#		-m post="/var/bin/busybox echo end > $(HD_INSDIR)HD_upgrade_end"\
#		-m post="/var/bin/busybox cd $(HD_INSDIR); sync; cd /" \
#		-m post="/var/bin/sdparm --command=sync /dev/sda1" \
#		-m post="/var/bin/busybox umount /dev/sda1; sleep 5"

.PHONY: rootfs_hd
endif

##########################################################################
#
#   Major targets: kernel, kernel_clean, release & tftpimage
#
##########################################################################

kernel_clean:
	@echo -e "\033[32m$(MYNAME): cleaning kernel ...\033[0m"
	$(Q)make -C progs.board/ubicom kernel_clean V=$(V) DEBUG=$(DEBUG)

kernel:
	@echo -e "\033[32m$(MYNAME) Building kernel ...\033[0m"
	$(Q)cp progs.board/$(KERNELCONFIG) kernel/.config
	#$(Q)make -C progs.board/ubicom -f sources.mk
	#$(Q)make -C progs.board/ubicom ultra V=$(V) DEBUG=$(DEBUG)
	$(Q)make -C progs.board/ubicom kernel_image V=$(V) DEBUG=$(DEBUG)

ifeq (buildno, $(wildcard buildno))
BUILDNO := $(shell cat buildno)

uboot_release: $(SEAMA)
	@echo -e "\033[32m"; \
	echo "=====================================";   \
	echo "You are going to build release uboot.";   \
	echo "=====================================";   \
	echo -e "\033[32m$(MYNAME) make release uboot... \033[0m"
	$(Q)[ -d images ] || mkdir -p images
	@echo -e "\033[32m$(MYNAME) prepare uboot...\033[0m"
	$(Q)make -C progs.board/ubicom uboot
	$(Q)cp progs.board/ubicom/bin/bootexec_bd.bin+u-boot.ub raw.img
	$(Q)cp raw.img $(UBOOTIMAGE).bin
#	$(Q)$(SEAMA) -i raw.img\
		-m dev=$(UBOOTDEV) -m type=firmware -m signature=$(ELBOX_SIGNATURE) -m noheader=1
#	$(Q)mv raw.img.seama web.img; rm -f raw.img
#	$(Q)$(SEAMA) -d web.img
#	$(Q)./tools/release.sh web.img $(UBOOTIMAGE).bin
#	$(Q)make sealpac_template
#	$(Q)if [ -f sealpac.slt ]; then ./tools/release.sh sealpac.slt $(UBOOTIMAGE).slt; fi

uboot_release_with_seama: $(SEAMA)
	@echo -e "\033[32m"; \
	echo "=====================================";   \
	echo "You are going to build release uboot.";   \
	echo "=====================================";   \
	echo -e "\033[32m$(MYNAME) make release uboot... \033[0m"
	$(Q)[ -d images ] || mkdir -p images
	@echo -e "\033[32m$(MYNAME) prepare uboot...\033[0m"
	$(Q)make -C progs.board/ubicom uboot
	$(Q)cp progs.board/ubicom/bin/bootexec_bd.bin+u-boot.ub raw.img
	$(Q)$(SEAMA) -i raw.img\
		-m dev=$(UBOOTDEV) -m type=firmware -m signature=$(ELBOX_SIGNATURE) -m noheader=1
	$(Q)mv raw.img.seama web.img; rm -f raw.img
	$(Q)$(SEAMA) -d web.img
	$(Q)./tools/release.sh web.img $(UBOOTIMAGE).bin
	$(Q)make sealpac_template
	$(Q)if [ -f sealpac.slt ]; then ./tools/release.sh sealpac.slt $(UBOOTIMAGE).slt; fi

release: rootfs_image $(SEAMA) $(ROOTFS_HD)
	@echo -e "\033[32m"; \
	echo "=====================================";   \
	echo "You are going to build release image.";   \
	echo "=====================================";   \
	echo -e "\033[32m$(MYNAME) make release image... \033[0m"
	$(Q)[ -d images ] || mkdir -p images
	@echo -e "\033[32m$(MYNAME) prepare image...\033[0m"
	$(Q)make -C progs.board/ubicom image_distro
	$(Q)cp progs.board/ubicom/bin/upgrade.ub raw.img
	$(Q)$(SEAMA) -i raw.img\
		-m dev=$(FWDEV) -m type=firmware -m signature=$(ELBOX_SIGNATURE) -m noheader=0 -m Router_ver=$(ROUTER_VER)
ifeq ($(strip $(CONFIG_CGIBIN_ARIES_FIRMWARE_IN_HD)),y)
	$(Q)$(SEAMA) -s web.img -i raw.img.seama -i $(ROOTFS_HD_IMG).seama
	$(Q)rm -f raw.img; rm -f raw.img.seama; rm -f $(ROOTFS_HD_IMG); rm -f $(ROOTFS_HD_IMG).seama
else
	$(Q)mv raw.img.seama web.img; rm -f raw.img
endif
	$(Q)$(SEAMA) -d web.img
	$(Q)./tools/release.sh web.img $(RELIMAGE).bin
	$(Q)make sealpac_template
	$(Q)if [ -f sealpac.slt ]; then ./tools/release.sh sealpac.slt $(RELIMAGE).slt; fi

tftpimage: rootfs_image $(SEAMA)
	@echo -e "\033[32mThe tftpimage of $(MYNAME) can be load by uboot via tftp!\033[0m"
	$(Q)make -C progs.board/ubicom image_distro
	$(Q)rm -f raw.img; cp progs.board/ubicom/bin/upgrade.ub raw.img
	$(Q)$(SEAMA) -i raw.img -m dev=$(FWDEV) -m type=firmware -m noheader=0
	$(Q)rm -rf raw.img; mv raw.img.seama raw.img
	$(Q)$(SEAMA) -d raw.img
	$(Q)./tools/tftpimage.sh $(TFTPIMG)

else
release tftpimage:
	@echo -e "\033[32m$(MYNAME): Can not build image, ROOTFS is not created yet !\033[0m"
endif
