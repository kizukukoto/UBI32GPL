#include $(APPS_PATH)/.config

#CFLAGS += -I. -D$(PLATFORM) -D$(CONFIG_MODEL_NAME_NO_DASH)
CFLAGS += -I.

VEROBJS += version.o linux_version.o apps_version.o wireless_version.o

VCTOBJS += linux_vct.o

ifeq ($(CONFIG_USER_LIBPLATFORM_DEBUG),y)
CFLAGS += -DLIBPLATFORM_DEBUG
endif

ifeq ($(CONFIG_USER_LIBPLATFORM_DEBUG_VCT),y)
CFLAGS += -DLIBPLATFORM_DEBUG_VCT
endif

ifeq ($(CONFIG_USER_VCT),y)
VCTOBJS += vct.o 
else
CFLAGS += -DNOVCT
endif

VEROUTPUT := libversion.a

#all: buildver
#	cp -f $(KERNEL_PATH)/../../drivers/vct/vct.* . ;\
#	$(MAKE) libvct.so ;\

all: buildver libvct.so

buildver:
#	cp -f $(WWW_PATH)/version.c .
#	cp -f $(APPS_PATH)/../doc/apps_version.c .
#	cp -f $(APPS_PATH)/doc/apps_version.c .
#	cp -f $(PRJ_PATH)/platform/$(PLATFORM)/doc/linux_version.c .
ifeq ($(WL_PLATFORM), RT2880)
#	cp -f $(WL_PATH)/doc/wireless_version.c .
else
ifeq ($(WL_PLATFORM), MV_AP95)
#	cp -f $(WL_PATH)/doc/wireless_version.c .
else
#	cp -f $(WL_PATH)/802_11/madwifi/doc/wireless_version.c .
endif #MV_AP95
endif #RT2880
#	cp -f $(WWW_PATH)/bsp.h .
	$(MAKE) verobj
	$(AR) -rc  $(VEROUTPUT) $(VEROBJS)

verobj: $(VEROBJS)


libvct.so: $(VCTOBJS)
#	$(LD) -shared -o $@ $^
	$(CC) -shared -o $@ $^

clean:
	rm -rf *.o *.so *.a *~ vct.*
#	rm -fr *version.c bsp.h 

install:
	install libvct.so $(FS_PATH)/lib ;\

romfs:
	$(ROMFSINST) libvct.so /lib

