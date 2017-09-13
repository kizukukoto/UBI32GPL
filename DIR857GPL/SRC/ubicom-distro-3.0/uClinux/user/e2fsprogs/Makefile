# To create your own version, just unpack ffmpeg tarball here
# and update the VER variable.  Or export your own from their
# svn via:
# svn export -r <REV> svn://svn.mplayerhq.hu/ffmpeg/trunk ffmpeg-svn-<REV>
REVISION = 1.41.12
VER = e2fsprogs-$(REVISION)

all: $(VER)/Makefile
	$(MAKE) -C $(VER)
	$(MAKE) -C $(VER) install > install.log

$(VER)/Makefile: $(VER)/configure Makefile
	find $(VER) -type f -print0 | xargs -0 touch -r $(VER)/configure
	set -e ; \
	cd $(VER); \
	./configure \
	--host=$(CONFIGURE_HOST) \
	--target=$(CONFIGURE_HOST) \
	--disable-nls \
	--disable-rpath \
	--enable-elf-shlibs \
	--enable-fsck \
	--disable-tls \
	--prefix=$(ROOTDIR)/user/e2fsprogs/build-ubicom ; 

clean:
	rm -rf *~ install.log .sgbuilt_lib
	rm -rf build*
	-$(MAKE) -C $(VER) distclean

romfs:
	$(ROMFSINST) -d -e CONFIG_BINFMT_ELF_FDPIC $(ROOTDIR)/user/e2fsprogs/build-ubicom/lib/libuuid.so.1.2 /lib/libuuid.so.1.2
	$(ROMFSINST) -s libuuid.so.1.2 /lib/libuuid.so.1
	$(ROMFSINST) -s libuuid.so.1.2 /lib/libuuid.so
	$(ROMFSINST) -d -e CONFIG_BINFMT_ELF_FDPIC $(ROOTDIR)/user/e2fsprogs/build-ubicom/lib/libblkid.so.1.0 /lib/libblkid.so.1.0
	$(ROMFSINST) -s libblkid.so.1.0 /lib/libblkid.so.1
	$(ROMFSINST) -s libblkid.so.1.0 /lib/libblkid.so
#	$(ROMFSINST) -e CONFIG_USER_E2FSPROGS_E2FSCK_E2FSCK build/e2fsck/e2fsck /bin/e2fsck
#	rm -f $(ROMFSDIR)/bin/fsck.ext2
#	$(ROMFSINST) -e CONFIG_USER_E2FSPROGS_E2FSCK_E2FSCK -s e2fsck /bin/fsck.ext2
#	$(ROMFSINST) -e CONFIG_USER_E2FSPROGS_DEBUGFS_DEBUGFS build/debugfs/debugfs /bin/debugfs
#	$(ROMFSINST) -e CONFIG_USER_E2FSPROGS_MISC_BADBLOCKS build/misc/badblocks /bin/badblocks
#	$(ROMFSINST) -e CONFIG_USER_E2FSPROGS_MISC_CHATTR build/misc/chattr /bin/chattr
#	$(ROMFSINST) -e CONFIG_USER_E2FSPROGS_MISC_DUMPE2FS build/misc/dumpe2fs /bin/dumpe2fs
#	$(ROMFSINST) -e CONFIG_USER_E2FSPROGS_MISC_FSCK build/misc/fsck /bin/fsck
#	$(ROMFSINST) -e CONFIG_USER_E2FSPROGS_MISC_LSATTR build/misc/lsattr /bin/lsattr
#	$(ROMFSINST) -e CONFIG_USER_E2FSPROGS_MISC_MKE2FS build/misc/mke2fs /bin/mke2fs
	$(ROMFSINST) -e CONFIG_USER_E2FSPROGS_MISC_BLKID $(ROOTDIR)/user/e2fsprogs/build-ubicom/sbin/blkid /bin/blkid
#	$(ROMFSINST) -e CONFIG_USER_E2FSPROGS_MISC_MKE2FS -s /bin/mke2fs /bin/mkfs.ext2
#	$(ROMFSINST) -e CONFIG_USER_E2FSPROGS_MISC_MKLOST_FOUND build/misc/mklost+found /bin/mklost+found
#	$(ROMFSINST) -e CONFIG_USER_E2FSPROGS_MISC_TUNE2FS build/misc/tune2fs /bin/tune2fs
#	$(ROMFSINST) -e CONFIG_USER_E2FSPROGS_MISC_UUIDGEN build/misc/uuidgen /bin/uuidgen
#	$(ROMFSINST) -e CONFIG_USER_E2FSPROGS_MISC_E2LABEL -s /bin/tune2fs /bin/e2label
	
.PHONY: all clean romfs
