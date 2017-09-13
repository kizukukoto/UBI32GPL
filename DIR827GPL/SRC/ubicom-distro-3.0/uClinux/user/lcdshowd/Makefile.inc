#ROOTDIR: generate and export in ?, it would be /home/ubicom-distro/uClinux
#
APPS_PATH=$(ROOTDIR)/user
NVRAM_PATH=$(APPS_PATH)/nvram
#KERNEL_PATH=$(ROOTDIR)/linux-2.6.x

ifeq ($(CC),cc)
GUI_CFLAGS := -Wall -DLITEFONTDIR=\"/usr/local/share/fonts/truetype\" -D_REENTRANT -DPC
GUI_CFLAGS += -I/usr/local/include/directfb -I./include
GUI_LDFLAGS = -L/usr/local/lib -llite -lleck -ldirectfb -lfusion -ldirect -lpthread -ldl -lm
ROMFSINST = install
else
GUI_CFLAGS = $(CFLAGS) -I./include -I$(NVRAM_PATH) -I$(APPS_PATH)/libplatform/ -I$(APPS_PATH)/sutil -DLITEFONTDIR=\"/share/fonts/truetype\" -D_REENTRANT
GUI_LDFLAGS = $(LDFLAGS) -L$(NVRAM_PATH) -lnvram -L$(APPS_PATH)/libplatform -lversion -lvct -llite -lleck -ldirectfb -lfusion -ldirect -lpthread -ldl -lm -lts
endif

