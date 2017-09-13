#
# Makefile
#	Ubicom Master Scripts Makefile
#
# Copyright � 2000-2010 Ubicom Inc. <www.ubicom.com>.  All rights reserved.
#
# This file contains confidential information of Ubicom, Inc. and your use of
# this file is subject to the Ubicom Software License Agreement distributed with
# this file. If you are uncertain whether you are an authorized user or to report
# any unauthorized use, please contact Ubicom, Inc. at +1-650-210-1500.
# Unauthorized reproduction or distribution of this file is subject to civil and
# criminal penalties.
#

#
# This version of the master makefile is based on using a technique called
# recursive include.  The main Makefile, which is in the project directory, includes
# other Makefiles, which include other Makefiles until all of the Makefiles
# have been included.
#
# For the most part, the targets are executed relative to the original project directory.
#
# The pattern rules assume that the paths will be full path names for both
# sources and targets.
#
# There are several global variables that are set by each package Makefile:
#
# ALL_SRCS = A list of all sources to compile using absolute path names.
# ALL_LIBS = A list of all libraries using absolute path names.
# GENERATED_INCLUDES = A list of generated include files using absolute path names.
# CUSTOM_BUILT = A list of all source files which should not be compiled using the
#   master makefile's default rules.
#
# They are defined empty here so that they can never accidentally be left undefined
#
ALL_SRCS :=
ALL_LIBS :=
GENERATED_INCLUDES :=
CUSTOM_BUILT :=

#
# Internal Master Makefile macros.
#
select_src_dir = $(filter $(1)%,$(2))
convert_to_objdir = $(subst $(1),$(OBJ_DIR), $(call select_src_dir,$(1),$(2)))
convert_to_objfile = $(subst .S,.o,$(subst .c,.o,$(1)))

#
# The following MACROS are used by the package and project Makefiles
# to convert from various directories to specific output paths.
#
basename_to_inc = $(addprefix $(INC_DIR)/,$(notdir $(1)))
basename_to_gsrc = $(addprefix $(GSRC_DIR)/,$(notdir $(1)))
#
# This converts a source file from a package or generated-sources directory into the appropriate
# object file in the object directory, so that you can generally say the following:
# $(call src_to_obj,$(PKG_DIR)/ipFoo/src/foo.c): $(PKG_DIR)/ipFoo/src/foo.c)
#
src_to_obj = $(call convert_to_objfile,$(call convert_to_objdir,$(PKG_DIR),$(1)) $(call convert_to_objdir,$(GSRC_DIR),$(1)))
#
# This does the same for source files in the app directory instead of the pkg or gsrc directories.
#
app_to_obj = $(call convert_to_objfile,$(subst $(APP_DIR),$(OBJ_DIR)/app,$(call select_src_dir,$(APP_DIR),$(1))) $(call convert_to_objdir,$(GSRC_DIR),$(1)))

#
# Setup the environment so that commands are invoked under a bash
# subshell and so that pathnames for prerequisites and targets
# resolve as unix paths rather than using DOS names.
#
include $(SDK_DIR)/scripts/Makefile.env

#
# Bash shell will vehemently complain if a /tmp dir does not
# exist, so make certain that we have one.  Force the mkdir
# to happen in a bash shell so we do not have DOS path trouble.
#
MKTMP := $(shell bash -t -c "mkdir -p /tmp")

MAKEFLAGS += --unix

#
# Tools.
#
ifdef IP8000
	UBICOM_PREFIX_ARCH = ubicom32v5
endif
ifdef IP7000
	UBICOM_PREFIX_ARCH = ubicom32v4
endif
ifdef IP7000_REV2
	UBICOM_PREFIX_ARCH = ubicom32v4
endif
ifdef IP5000
	UBICOM_PREFIX_ARCH = ubicom32v3
endif
ifdef IP5000_REV2
	UBICOM_PREFIX_ARCH = ubicom32v3
endif

CC	= ubicom32-elf-gcc -march=$(UBICOM_PREFIX_ARCH)
AS	= ubicom32-elf-as --gstabs -m$(UBICOM_PREFIX_ARCH)
LD	= ubicom32-elf-ld
AR	= ubicom32-elf-ar
OBJCOPY = ubicom32-elf-objcopy
OBJDUMP = ubicom32-elf-objdump
DB 	= ubicom32-elf-gdb
NM 	= ubicom32-elf-nm

#
# Make sure that the build directory hierarchy has been specified
#
BUILD_DIR ?= $(CURDIR)/build
BIN_DIR = $(BUILD_DIR)/bin
GSRC_DIR = $(BUILD_DIR)/src
OBJ_DIR = $(BUILD_DIR)/obj
LIB_DIR = $(BUILD_DIR)/lib
INC_DIR = $(BUILD_DIR)/include
DOC_DIR = $(BUILD_DIR)/doc
APP_DIR = $(CURDIR)/app

#
# Simplify to relative paths, if possible.
#
BUILD_REL = $(subst $(CURDIR)/,,$(BUILD_DIR))
BIN_REL = $(subst $(CURDIR)/,,$(BIN_DIR))
CSRC_REL = $(subst $(CURDIR)/,,$(GSRC_DIR))
OBJ_REL = $(subst $(CURDIR)/,,$(OBJ_DIR))
LIB_REL = $(subst $(CURDIR)/,,$(LIB_DIR))
INC_REL = $(subst $(CURDIR)/,,$(INC_DIR))
DOC_REL = $(subst $(CURDIR)/,,$(DOC_DIR))
APP_REL = $(subst $(CURDIR)/,,$(APP_DIR))

#
# General Package Variables and defines
#
PROJECT_DIR := $(CURDIR)
PKG_DIR := $(SDK_DIR)/pkg
CONFIG_DIR := $(PROJECT_DIR)/config
OCM_SIZE_H := $(INC_DIR)/ocm_size.h
PROJECT_NAME_INTERMED_ELF := $(BIN_DIR)/$(PROJECT_NAME)_i.elf

ifeq ($(OS_ENV), win32)
	SDK_WIN_DIR := $(shell cygpath -m $(SDK_DIR))
	PATH_SEP :=\\
else
	SDK_WIN_DIR := $(SDK_DIR)
	PATH_SEP :=/
endif

#
# Miscellaneous Tools used by the Ubicom Make structure
#
TOOLS_SUBDIR := $(OS_ENV)
SDK_TOOLS_DIR := $(SDK_DIR)/tools/bin/$(TOOLS_SUBDIR)

CTOOL := $(SDK_TOOLS_DIR)/ctool$(BINSUF)
ELF2BIN := $(SDK_TOOLS_DIR)/elf2bin$(BINSUF)
FTRANS := $(SDK_TOOLS_DIR)/ftrans$(BINSUF)
BIN2H := $(SDK_TOOLS_DIR)/bin2h$(BINSUF)
PKGINTEGRATE := $(SDK_TOOLS_DIR)/pkgintegrate$(BINSUF)
PREINCLUDE := $(SDK_TOOLS_DIR)/preinclude$(BINSUF)
SAXON := $(SDK_TOOLS_DIR)/saxon.exe
ARJ := $(SDK_TOOLS_DIR)/arj$(BINSUF)
GEN_OCM_SIZE := $(SDK_DIR)/scripts/gen_ocm_size.sh
GET_SECTOR_BOUNDARY := $(SDK_DIR)/scripts/get_sector_boundary.sh

#
# Miscellaneous standard filenames
#
UBICOM_PKGID_LDS = $(INC_DIR)/UbicomPkgID.lds

#
# C preprocessor flags.
# Note: Add include directories to CPPFLAGS.
#
CPPFLAGS += \
	-D$(ARCH) \
	-I$(GSRC_DIR) \
	-Wa,-I$(GSRC_DIR) \
	-I$(INC_REL) \
	-Wa,-I$(INC_REL) \
	-Iconfig \
	-Wa,-Iconfig \
	-I. \
	-Wa,-I. \

#
# Global CFLAGS for gcc.
#
# Note: DO NOT add includes here, add them to CPPFLAGS.
#
CFLAGS += \
	-W \
	-Wall \
	-Wbad-function-cast \
	-Wcast-align \
	-Wno-unused-parameter \
	-Wsign-compare \
	-Wshadow \
	-Wstrict-prototypes \
	-Wundef \
	-fno-builtin \
	-nostdinc \
	-std=gnu99 \
	-ffunction-sections \
	-fdata-sections \
	-fno-common \
	-funsigned-char \
	-L$(OBJ_DIR) \
	-mipos-abi

ifneq (,$(findstring s, $(MAKEFLAGS)))
	QUIET_MODE := 1
else
	ARFLAGS ?= r
endif

ARFLAGS := cu$(ARFLAGS)o

#
# GLOBAL_CFLAGS comes from the LPJ.  It is appended to the end of the CFLAGS
# so that it can "override" previous flags assigned to CFLAGS.
#
# Do not move this above the CFLAGS declaration.
#
CPPFLAGS += $(GLOBAL_CFLAGS)
CFLAGS += $(CPPFLAGS) $(CPPFLAGS_EXTRA)

#
# Set the global flags to pass to gcc for compiling an assembler file.
#
ASFLAGS += -D__ASM__ -Wa,--gdwarf2 -I$(INC_REL)

START_MAGIC := 0x55AA
ASFLAGS += -DSTART_MAGIC=$(START_MAGIC)
ifdef IPBOOTDECOMPRESSOR
     BD_MAGIC := 0x55AA
     ASFLAGS += -DBD_MAGIC=$(BD_MAGIC)
endif

#
# Linker flags
#
ifdef IPBOOTDECOMPRESSOR
	LDSCRIPT := $(GSRC_DIR)/ip3kelf_compress.ld
	BOOTDECOMPRESSOR_LDSCRIPT:= $(GSRC_DIR)/bd.ld
	BOOTDECOMPRESSOR_ELF:= $(BIN_DIR)/bd.elf
else
	LDSCRIPT := $(GSRC_DIR)/ip3kelf.ld
endif

LINK_COMMAND_FILE := $(patsubst %.ld,%.linkcmd,$(LDSCRIPT))
LINK_SCRIPT_GENERATE_FILE := $(patsubst %.ld,%.genscript,$(LDSCRIPT))

LDFLAGS += \
	--gc-sections \
	--relax \
	-L$(LIB_REL) \
	-L$(GCCLIBDIR) \
	--whole-archive \
	$(addprefix -l,$(subst .a,,$(patsubst lib%,%,$(notdir $(ALL_LIBS))))) \
	$(addprefix -l,$(subst .a,,$(patsubst lib%,%,$(notdir $(GCC_LIBS))))) \
	--no-whole-archive \
	$(LDFLAGS_AT_END)

ifdef IPBOOTDECOMPRESSOR
BOOTDECOMPRESSOR_LDFLAGS += \
	--gc-sections \
	--relax \
	-L$(LIB_REL) \
	-L$(GCCLIBDIR) \
	--whole-archive \
	$(LIB_DIR)/libipBootDecompressor.a \
	$(addprefix -l,$(subst .a,,$(patsubst lib%,%,$(notdir $(ALL_LIBS))))) \
	$(addprefix -l,$(subst .a,,$(patsubst lib%,%,$(notdir $(GCC_LIBS))))) \
	--no-whole-archive \
	$(LDFLAGS_AT_END)
endif

#
# Dongle support for programming a board.
#
GDB_TARGET := ubicom32
PROG := expect $(SDK_DIR)/scripts/prog_board.exp $(GDB_TARGET)

#
# Dongle support for getting a backtrace from a board.
#
BACKTRACE := expect $(SDK_DIR)/scripts/backtrace.exp $(GDB_TARGET)

#
# Dongle support for getting a coredump from a board.
#
COREDUMP := expect $(SDK_DIR)/scripts/coredump.exp $(GDB_TARGET)

#
# GCC Library
#
GCCLIBDIR := $(dir $(shell $(CC) -fleading-underscore -mipos-abi -print-libgcc-file-name))
GCC_LIBS := $(GCCLIBDIR)/libgcc.a

#
# Include all of the package makefiles and the project Makefile
#
PACKAGE_MAKEFILES := $(addsuffix /Makefile.inc,$(addprefix $(SDK_DIR)/pkg/,$(PKG_SUBDIRS)))
ifneq ($(PACKAGE_MAKEFILES),)
include $(PACKAGE_MAKEFILES)
endif
include app/Makefile.inc

#
# Force the creation of libApp.a for the developer's project.
#
APP_OBJS := $(call app_to_obj, $(APP_SRCS))
APP_LIB := $(LIB_DIR)/libApp.a
ALL_SRCS += $(APP_SRCS)
ALL_LIBS += $(APP_LIB)

#
# Common application/user level rules from here down.
#
$(APP_LIB): $(APP_OBJS)
	$(AR) $(ARFLAGS) $@ $^

#
# Obtain a list of the relevant package files for this project.
#
PACKAGE_PKG_DIRS := $(addsuffix /pkginfo, $(addprefix $(PKG_DIR)/, $(PKG_SUBDIRS)))
PACKAGE_PKG_FILES := $(wildcard $(addsuffix /*.pkg, $(PACKAGE_PKG_DIRS)))
PACKAGE_LDI_FILES := $(wildcard $(addsuffix /*.ldi, $(PACKAGE_PKG_DIRS)))

#
# Compute the output directory list.
# Note: since $(dir leaves the trailing slash, we add a trailing slash of our known to make sure
# we generate a unique list.
#
DIRS_TO_MAKE := $(call convert_to_objdir,$(PKG_DIR),$(ALL_SRCS))
DIRS_TO_MAKE += $(call convert_to_objdir,$(PROJECT_DIR),$(filter $(APP_DIR)%,$(ALL_SRCS)))
DIRS_TO_MAKE += $(call convert_to_objdir,$(GSRC_DIR),$(ALL_SRCS))
DIRS_TO_MAKE += $(BUILD_DIR)/ $(OBJ_DIR)/ $(GSRC_DIR)/ $(LIB_DIR)/ $(BIN_DIR)/ $(INC_DIR)/ $(DOC_DIR)/
DIRS := $(sort $(dir $(DIRS_TO_MAKE)))

#
# This is the primary (default) build rule.
#
all: $(PROJECT_NAME).elf

$(PROJECT_NAME).bin: $(PROJECT_NAME).elf
	$(OBJCOPY) -O binary $< $@

ifdef IPBOOTDECOMPRESSOR
$(BOOTDECOMPRESSOR_ELF): $(ALL_LIBS) $(GCC_LIBS) $(BOOTDECOMPRESSOR_LDSCRIPT) $(LIB_DIR)/libipBootDecompressor.a
	$(LD) -e __bd_start_vector -T $(BOOTDECOMPRESSOR_LDSCRIPT) $(UBICOM_PKGID_LDS) $(BOOTDECOMPRESSOR_LDFLAGS)  -o $@
endif

#
# Create final elf file
#
ifdef IPBOOTDECOMPRESSOR
$(PROJECT_NAME).elf: dirs $(PROJECT_NAME_INTERMED_ELF) $(BOOTDECOMPRESSOR_ELF)
	$(LD) -o $@ -T$(LDSCRIPT) $(UBICOM_PKGID_LDS) $(LDFLAGS)
	$(ELF2BIN) -b __main_load_begin -e __main_load_end -f 0xff -pad -o $(BIN_DIR)/bd.bin $(BOOTDECOMPRESSOR_ELF)
	$(ELF2BIN) -b __filesystem_section_begin -e __filesystem_section_end -f 0xff -pad -o $(BIN_DIR)/img.bin $@
	$(RM) $(BIN_DIR)/img.arj
	$(ARJ) a $(BIN_DIR)/img.arj $(BIN_DIR)/img.bin
	$(CAT) $(BIN_DIR)/img.arj >> $(BIN_DIR)/bd.bin
	$(CP) $(SDK_DIR)/scripts/image.S $(BIN_DIR)
	(cd $(BIN_DIR); $(CC) $(CFLAGS) image.S -c -o $(BIN_DIR)/image.o)
	$(LD) -o $@ -T$(LDSCRIPT) $(UBICOM_PKGID_LDS) $(LDFLAGS) $(BIN_REL)/image.o
else
$(PROJECT_NAME).elf: dirs $(PROJECT_NAME_INTERMED_ELF)
	$(LD) -o $@ -T$(LDSCRIPT) $(UBICOM_PKGID_LDS) $(LDFLAGS)
	$(ELF2BIN) -b __main_load_begin -e __main_load_end -f 0xff -pad -o $(BIN_DIR)/bios.bin $@
endif

#
# Rule to ensure that if the config file or a package pkg file has
# changed, the entire structure is rebuilt.
#
.config.mk.t: config/config.mk
	$(MAKE) clean --no-print-directory
	@$(TOUCH) .config.mk.t

%.i : %.c
	$(CC) -E $< $(CFLAGS) -S -o $(OBJ_DIR)/$@

%.i : %.S
	$(CC) -E $< $(CPPFLAGS) $(ASFLAGS) -S -M -o $(OBJ_DIR)/$@

%.s : %.c
	$(CC) -c $< $(CFLAGS) -S -o $(OBJ_DIR)/$@

%.s : %.S
	$(CC) -c $< $(CPPFLAGS) $(ASFLAGS) -S -M -o $(OBJ_DIR)/$@

%.debug : %.c
	$(CC) -c $< $(CFLAGS) -save-temps -dA -dp -o $(OBJ_DIR)/$@ -Wp,-MD,$(OBJ_DIR)/$(patsubst %.o,%.d,$@)
	$(MV) $(patsubst %.debug,%.i,$@) $(OBJ_DIR)
	$(MV) $(patsubst %.debug,%.s,$@) $(OBJ_DIR)

#
# Link project into a final elf file.
#
$(PROJECT_NAME_INTERMED_ELF): $(ALL_LIBS) $(GCC_LIBS) $(LDSCRIPT)
	echo "***** Generating "$(OCM_SIZE_H)
	. $(GEN_OCM_SIZE) > $(OCM_SIZE_H)
	. $(LINK_SCRIPT_GENERATE_FILE)
	$(LD) -o $(PROJECT_NAME_INTERMED_ELF) -T$(LDSCRIPT) $(UBICOM_PKGID_LDS) $(LDFLAGS)
	echo "***** Second link to move OCM data up"
	. $(GEN_OCM_SIZE) $(PROJECT_NAME_INTERMED_ELF) > $(OCM_SIZE_H)
	. $(LINK_SCRIPT_GENERATE_FILE)
	$(LD) -o $(PROJECT_NAME_INTERMED_ELF) -T$(LDSCRIPT) $(UBICOM_PKGID_LDS) $(LDFLAGS)

#
# This is to allow the command "make ubicom_sdk_h" to be issued at the command
# line, instead of having to figure out its full path.
#
.PHONY: ubicom_sdk_h
ubicom_sdk_h: $(INC_DIR)/UbicomSDK.h

#
# Pre-built package headers.
#
$(INC_DIR)/UbicomSDK.h: $(INC_DIR)/UbicomSDK.pre $(GENERATED_INCLUDES)
	$(PREINCLUDE) $(INC_DIR)/UbicomSDK.pre -o $(INC_DIR)/UbicomSDK.h -d $(INC_DIR)/UbicomSDK.d -P $(PKG_DIR)

#
# UbicomPkgID.lds, UbicomSDK.pre, and UbicomSDK.ldi are made at the same time by $(PKGINTEGRATE) so
# we cascade the dependency for parallel makes.
#
$(UBICOM_PKGID_LDS): $(INC_DIR)/UbicomSDK.pre
$(INC_DIR)/UbicomSDK.pre: $(INC_DIR)/UbicomSDK.ldi
$(INC_DIR)/UbicomSDK.ldi:  $(PACKAGE_LDI_FILES) $(CONFIG_DIR)/config.order
	$(PKGINTEGRATE) -o $(INC_DIR) $(CONFIG_DIR)/config.order

$(INC_DIR)/%.h:
	$(PREINCLUDE) $(PKG_DIR)/$*/include/$*.h -o $@ -I $(PKG_DIR)/$*/include -d $(INC_DIR)/$*.d

$(CONFIG_DIR)/config.order: $(PACKAGE_PKG_FILES)
	$(warning Run ConfigTool to generate $(CONFIG_DIR)/config.order)

#
# Build the linker script
#
# Special variables have been added below to allow for combining multiple projects into one elf file.
# This is achieved by linking each project separately.  The user first links the individual project
# at the normal address to compute it's size and then relinks setting location of the Dynamic Non-Volatile
# section $(DNV_OFFSET) and the absolute link address for the image $(MULTILINK_ABS_FLASH_ADDR)
#
CONFIG_APP_LDI := $(if $(wildcard config/app.ldi), config/app.ldi,)
$(LDSCRIPT): APP_LDI_FILE := $(if $(wildcard config/app.ldi),-DAPP_LDI_FILE,)
$(LDSCRIPT): LD_INC_DIRS := $(addsuffix /pkginfo,$(addprefix -I$(SDK_DIR)/pkg/,$(PKG_SUBDIRS)))
$(LDSCRIPT): $(SDK_DIR)/scripts/$(notdir $(LDSCRIPT:.ld=.lds)) $(INC_DIR)/UbicomSDK.ldi $(CONFIG_APP_LDI)
	echo $(OS_ENV)
	. $(GEN_OCM_SIZE) > $(OCM_SIZE_H)
	$(CC) -E -C -P $(CPPFLAGS) -I $(INC_DIR) -D__LINKER__ $(APP_LDI_FILE) $(LD_INC_DIRS) -x c $< -o $@ -Wp,-MD,$(patsubst %.ld,%.d,$@)

	$(SED) -i -e s'/.o:/.ld:/' $(@:.ld=.d)
	echo $(CC) -E -C -P $(CPPFLAGS) -I $(INC_DIR) -D__LINKER__ $(APP_LDI_FILE) $(LD_INC_DIRS) -x c $< -o $@ -Wp,-MD,$(patsubst %.ld,%.d,$@) > $(LINK_SCRIPT_GENERATE_FILE)

ifdef IPBOOTDECOMPRESSOR
$(BOOTDECOMPRESSOR_LDSCRIPT): $(PKG_DIR)/ipBootDecompressor/src/bd.lds $(INC_DIR)/UbicomSDK.ldi
	$(CC) -E -C -P $(CPPFLAGS) -I $(INC_DIR) -D__LINKER__  $(LD_INC_DIRS) -x c $< -o $@ -Wp,-MD,$(patsubst %.ld,%.d,$@)
	$(SED) -i -e s'/.o:/.ld:/' $(@:.ld=.d)
endif

#
# This rule causes the dependency list for the documentation
# files to be included.  Since we don't want to do this when
# are not creating documentation for now base this on a
# define.  Later we an figure out a better method for doing
# this.  All of the documentation build rules are located
# in Makedoc.inc
#
ifdef DOINGDOCS
ifneq ($(MAKECMDGOALS),clean)
-include $(SDK_DIR)/scripts/Makedoc.inc
-include $(ALL_TOCS_DEPENDS)
endif
endif

PKG_INCLUDE_DIRS = $(addsuffix /include/,$(addprefix $(PKG_DIR)/,$(PKG_SUBDIRS)))

#
# Provide a rule for creating a file list and then generating
# cscope and tags files.  For now we include everything in your lpj
# file and exclude all packages that are not in your lpj file.
#
# You use this by saying make ctags, make etags, and make cscope
# You then use vi -t <tag>, or emacs tags, or cscope: whichever is
# appropriate.
#
# Note: these absolutely depend on either being on Unix or running under
# cygwin.  Don't expect these directives to work under DOS Command Prompt
#
ctags: $(BUILD_DIR)/files.txt
	ctags --sort=yes --extra=+f --if0=no -I ACCELERATE --links=yes --c-types=+px -L $<
	@for hf in "$(wildcard $(addsuffix *.h,$(sort $(PKG_INCLUDE_DIRS))))" ; do \
		ctags -a $$hf;	\
	done

etags: $(BUILD_DIR)/files.txt
	ctags -e -L $<

cscope: $(BUILD_DIR)/files.txt
	@$(CP) $< cscope.files

.INTERMEDIATE: $(BUILD_DIR)/find.txt $(BUILD_DIR)/files.txt

$(BUILD_DIR)/files.txt: $(BUILD_DIR)/find.txt
	@$(PATH_CONV_FILE) $< > $@

$(BUILD_DIR)/find.txt:
	@echo "$(wildcard $(addsuffix *.h,$(sort $(dir $(ALL_SRCS)))))" > $@
	@echo "$(wildcard $(addsuffix *.S,$(sort $(dir $(ALL_SRCS)))))" >> $@
	@echo "$(wildcard $(addsuffix *.c,$(sort $(dir $(ALL_SRCS)))))" >> $@
	@echo "$(wildcard $(addsuffix *.inc,$(sort $(dir $(ALL_SRCS)))))" >> $@
	@echo "$(wildcard $(addsuffix $(ARCH_DIR)/*.h,$(sort $(PKG_INCLUDE_DIRS))))" >> $@
#	@echo "$(wildcard $(addsuffix *.h,$(sort $(PKG_INCLUDE_DIRS))))" >> $@
	@echo $(GENERATED_INCLUDES) >> $@
	@$(FIND) $(APP_DIR) -name '*.[hSc]' -or -name '*.inc' >> $@
	@sed -i 's/ /\n/g' $@

#
# Install
#
install: $(PROJECT_NAME).elf
	$(PROG) $(PROJECT_NAME).elf

#
# Backtrace
#
backtrace:
	$(if $(wildcard $(PROJECT_NAME).elf),$(BACKTRACE) $(PROJECT_NAME).elf, @echo "$(PROJECT_NAME).elf: file not found")

#
# Coredump
#
coredump:
	$(if $(wildcard $(PROJECT_NAME).elf),$(COREDUMP) $(PROJECT_NAME).elf, @echo "$(PROJECT_NAME).elf: file not found")

#
#  simple cleanup and directory creation rules
#
distclean realclean: clean
	-$(RMDIR) $(BUILD_REL)
	-$(RM) config/config.*
	-$(RM) config/*_config.h
	-$(RM) config/debug_map.c
	-$(RM) *~
	-$(RM) files.txt
	-$(RM) cscope cscope.*
	-$(RM) tags TAGS
	-$(RM) .config.mk.t

clean:
	-$(RMDIR) $(BUILD_REL)/?[!n]*
	-$(RM) $(BUILD_REL)/include/Ubicom*.*
	-$(RM) *.s
	-$(RM) *.i
	-$(RM) $(PROJECT_NAME).elf
	-$(RM) auto*

#
# Rule for building the various target directories needed as prerequisites
#
.PHONY: dirs
dirs: .config.mk.t
	$(MKDIR) -p $(DIRS)

#
# We don't want to build the files that we've been specifically asked not to
#
SOURCES_TO_BUILD := $(filter-out $(CUSTOM_BUILT),$(ALL_SRCS))

#
# This separates out the ALL_SRCS variable into pkg, project, and gen-src sections
# and separates these sections into .S and .c components
#
PKG_C_SRCS := $(filter %.c,$(call convert_to_objdir,$(PKG_DIR),$(SOURCES_TO_BUILD)))
PKG_ASM_SRCS := $(filter %.S,$(call convert_to_objdir,$(PKG_DIR),$(SOURCES_TO_BUILD)))
GEN_C_SRCS := $(filter %.c,$(call convert_to_objdir,$(GSRC_DIR),$(SOURCES_TO_BUILD)))
GEN_ASM_SRCS := $(filter %.S,$(call convert_to_objdir,$(GSRC_DIR),$(SOURCES_TO_BUILD)))

#
# Next we create a list of object files dependent on each source file
#
PKG_C_OBJS := $(subst .c,.o,$(PKG_C_SRCS))
PKG_ASM_OBJS := $(subst .S,.o,$(PKG_ASM_SRCS))
GEN_C_OBJS := $(subst .c,.o,$(GEN_C_SRCS))
GEN_ASM_OBJS := $(subst .S,.o,$(GEN_ASM_SRCS))

#
# App sources handled slightly differently
#
APP_C_SRCS := $(filter %.c,$(call select_src_dir,$(APP_DIR),$(SOURCES_TO_BUILD)))
APP_ASM_SRCS := $(filter %.S,$(call select_src_dir,$(APP_DIR),$(SOURCES_TO_BUILD)))
APP_C_OBJS := $(call app_to_obj,$(APP_C_SRCS))
APP_ASM_OBJS := $(call app_to_obj,$(APP_ASM_SRCS))

#
# General pattern rules PKG_DIR to OBJ_DIR
#
$(PKG_C_OBJS): $(OBJ_DIR)/%.o: $(PKG_DIR)/%.c $(INC_DIR)/UbicomSDK.h
	$(CC) -c $< $(CFLAGS) -o $@ -MD -MF $(@:.o=.d) -MT $@

$(PKG_ASM_OBJS): $(OBJ_DIR)/%.o: $(PKG_DIR)/%.S $(INC_DIR)/UbicomSDK.h
	$(CC) -c $< $(CPPFLAGS) $(ASFLAGS) -o $@ -MD -MF $(@:.o=.d) -MT $@

#
# General pattern rules PROJECT_DIR to OBJ_DIR
#
$(APP_C_OBJS): $(OBJ_DIR)/%.o: $(PROJECT_DIR)/%.c $(INC_DIR)/UbicomSDK.h
	$(CC) -c $< $(CFLAGS) -o $@ -MD -MF $(@:.o=.d) -MT $@

$(APP_ASM_OBJS): $(OBJ_DIR)/%.o: $(PROJECT_DIR)/%.S $(INC_DIR)/UbicomSDK.h
	$(CC) -c $< $(CPPFLAGS) $(ASFLAGS) -o $@ -MD -MF $(@:.o=.d) -MT $@

#
# General pattern rule GSRC_DIR to OBJ_DIR
#
$(GEN_C_OBJS): $(OBJ_DIR)/%.o: $(GSRC_DIR)/%.c $(INC_DIR)/UbicomSDK.h
	$(CC) -c $< $(CFLAGS) -o $@ -MD -MF $(@:.o=.d) -MT $@

$(GEN_ASM_OBJS): $(OBJ_DIR)/%.o: $(GSRC_DIR)/%.S $(INC_DIR)/UbicomSDK.h
	$(CC) -c $< $(CPPFLAGS) $(ASFLAGS) -o $@ -MD -MF $(@:.o=.d) -MT $@

#
# Below is the rule to make temporary files for debugging. Make foo.debug
# will search for a source file named foo.c or foo.S, compile it into
# foo.debug, save the intermediate files as foo.s and foo.i, and place them
# all into $(OBJ_DIR).
#

#
# This is how we find the target the user is looking for if they ask for a debug build
#
DEBUG_TARGET = $(filter %/$*.c %/$*.S %\\$*.c %\\$*.S,$(ALL_SRCS))

#
# This is not really a phony rule: it does create a real file named foo.debug.
# However, we cannot easily express its dependencies until we have begun to
# execute the rule; therefore, we declare it as phony to ensure that it is
# remade.
#
.PHONY: %.debug %.s %.i
%.debug:
	$(CC) -c $(DEBUG_TARGET) $(CFLAGS) -save-temps -dA -dp -o $@ -Wp,-MD,$(@:.o=.d)
	$(MV) -f $*.debug $*.i $*.s $(OBJ_DIR)
	@echo 'Debug files have been created in the object directory'

#
# Disable implicit build rules for .c and .S files
#
%.o: %.c

%.o: %.S

#
# Include all of the dependency rules (auto generated by the compiler)
# (We put a directory in just so include does not generate an empty string line)
#
-include $(INC_DIR)/*.d $(wildcard $(addsuffix *.d,$(DIRS)))
