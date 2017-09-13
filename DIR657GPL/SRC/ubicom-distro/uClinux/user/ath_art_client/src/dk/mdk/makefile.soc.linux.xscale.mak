
ROOT := ../../../../
SW_ROOT ?= $(ROOT)
export SW_ROOT
MDK_ROOT := $(SW_ROOT)/src/dk/mdk
CLIENT_ROOT := $(MDK_ROOT)/client
L_ROOT := $(TOPDIR)
TARGET_ARCH := armeb

ifeq ($(AP71),1)
TARGET := owl
endif
ifeq ($(OB42),1)
TARGET := owl
endif
OS := soc_linux

ifeq ($(AP71),1)
CFLAGS += -DOWL_AP
endif

ifeq ($(OB42),1)
CFLAGS += -DOWL_OB42 -DOWL_AP 
endif

OUTPUT_DIR := $(CLIENT_ROOT)/$(TARGET)/$(OS)/obj

ifeq ($(OS), soc_linux)
TOOL_PREFIX := $(TARGET_ARCH)-linux-
DEF_OS  := SOC_LINUX -DLinux -DLINUX
endif

CC := $(TOOL_PREFIX)gcc
LD := $(TOOL_PREFIX)gcc  
STRIP := $(TOOL_PREFIX)strip

CFLAGS +=  -isystem $(L_ROOT)/tools/buildroot/build_armeb/
CFLAGS +=  -I$(L_ROOT)/linux/include
CFLAGS +=  -I$(L_ROOT)/src/kernels/linux-2.6.13.2/include/
CFLAGS += -I../common/include -Iinclude -I$(CLIENT_ROOT)/$(TARGET)/$(OS)/include  
CFLAGS += -Os

LDFLAGS = -B$(L_ROOT)/tools/buildroot/toolchain_build_armeb/uClibc-0.9.28/lib/ 
LDFLAGS += $(L_ROOT)/tools/buildroot/toolchain_build_armeb/uClibc-0.9.28/lib/libm.a
LDFLAGS += $(L_ROOT)/tools/buildroot/toolchain_build_armeb/uClibc-0.9.28/lib/libpthread.a

#.LIBPATTERNS = "`lib%.a'"

.EXPORT_ALL_VARIABLES:

