
ROOT := ../../../../
SW_ROOT ?= $(ROOT)
export SW_ROOT
MDK_ROOT := $(SW_ROOT)/src/dk/mdk
CLIENT_ROOT := $(MDK_ROOT)/client
L_ROOT := $(TOPDIR)
TARGET_ARCH := mips

ifeq ($(AP71),1)
TARGET := owl
endif
ifeq ($(OB42),1)
TARGET := owl
endif
ifeq ($(PB42),1)
TARGET := owl
endif
ifeq ($(HYDRA),1)
TARGET := owl
endif
OS := soc_linux

ifeq ($(AP71),1)
CFLAGS += -DOWL_AP
endif

ifeq ($(OB42),1)
CFLAGS += -DOWL_OB42 -DOWL_AP 
endif

ifeq ($(PB42),1)
CFLAGS += -DOWL_PB42 -DOWL_OB42 -DOWL_AP 
endif

ifeq ($(OCTEON),1)
CFLAGS += -DOCTEON
endif

ifeq ($(HYDRA),1)
CFLAGS += -DOWL_PB42 -DOWL_OB42 -DOWL_AP 
endif

OUTPUT_DIR := $(CLIENT_ROOT)/$(TARGET)/$(OS)/obj

ifeq ($(OS), soc_linux)
TOOL_PREFIX := mips64-octeon-linux-gnu-
DEF_OS  := SOC_LINUX -DLinux -DLINUX
endif

CC := $(TOOL_PREFIX)gcc
LD := $(TOOL_PREFIX)gcc  
STRIP := $(TOOL_PREFIX)strip

CFLAGS += -I../common/include -Iinclude -I$(CLIENT_ROOT)/$(TARGET)/$(OS)/include  
CFLAGS += -Os

LDFLAGS += -lm -lc -lnss_files -lnss_dns -lresolv -lpthread 

#.LIBPATTERNS = "`lib%.a'"

.EXPORT_ALL_VARIABLES:

