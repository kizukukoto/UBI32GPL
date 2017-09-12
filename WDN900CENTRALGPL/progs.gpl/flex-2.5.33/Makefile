-include ../arch.mk
-include ../../path.mk
-include ../../.config
-include /tmp/$(USER)/$(shell echo $(ELBOX_MODEL_NAME))_path.mk

CUR_DIR := $(shell pwd)
BUILD_DIR := build
PREFIX :=
APPLET := flex
TARBALL := flex_2.5.33.orig.tar.gz
PATCH := flex_2.5.33-11.diff
SRC_DIR := flex-2.5.33

.PHONY: all
all: prepare_source
	@echo -e "\033[32mBuilding $(APPLET) ...\033[0m";
	@if test ! -d $(BUILD_DIR); then \
		mkdir -p $(BUILD_DIR); \
		make _config_; \
		make -C $(BUILD_DIR); \
	else \
		cd $(BUILD_DIR); \
		make; \
	fi
	@if test ! -d /tmp/$(USER); then mkdir -p /tmp/$(USER); fi
	@if test ! -f /tmp/$(USER)/$(ELBOX_MODEL_NAME)_path.mk; then touch /tmp/$(USER)/$(ELBOX_MODEL_NAME)_path.mk; fi
	@echo "FLEX_DIR:=$(CUR_DIR)/$(BUILD_DIR)" >> /tmp/$(USER)/$(ELBOX_MODEL_NAME)_path.mk

.PHONY: prepare_source
ifeq ($(SRC_DIR), $(wildcard $(SRC_DIR)))
prepare_source:
else
prepare_source:
	@echo -e "\033[32mExtracting $(APPLET) source codes ...\033[0m";
	@tar zxf $(TARBALL)
	@if test ! -z $(PATCH); then \
		cd $(CUR_DIR)/$(SRC_DIR) && patch -p1 < $(CUR_DIR)/$(PATCH); \
	fi
endif

.PHONY: _config_
_config_:
	@cd $(BUILD_DIR); \
		$(CUR_DIR)/$(SRC_DIR)/configure \
			--host=$(HOST_TYPE) \
			--prefix=$(PREFIX) \
			CFLAGS="${CFLAGS}" LDFLAGS="${LDFLAGS}"

.PHONY: clean
clean:
	@echo -e "\033[32mCleaning $(APPLET) ...\033[0m"
	@if test -f $(BUILD_DIR)/Makefile; then \
		make -C $(BUILD_DIR) clean; \
	fi
	@if test -f /tmp/$(USER)/$(ELBOX_MODEL_NAME)_path.mk; then rm -f /tmp/$(USER)/$(ELBOX_MODEL_NAME)_path.mk; fi
	@make distclean;

.PHONY: distclean
distclean:
	@if test -d $(BUILD_DIR); then rm -rf $(BUILD_DIR); fi;
	@if test -d $(SRC_DIR); then rm -rf $(SRC_DIR); fi;

.PHONY: install
ifeq ($(strip $(ELBOX_FLEX_DONT_INSTALL)), y)
install:
	@echo -e "\033[32m$(APPLET): No Install!!!\033[0m"
else
install:
	@echo -e "\033[32mInstalling $(APPLET) ...\033[0m"
	@cp $(BUILD_DIR)/$(APPLET) $(TARGET)/usr/sbin/.
	@$(STRIP) $(TARGET)/usr/sbin/$(APPLET)
endif

.PHONY: showconfig
showconfig:
	@echo CC=$(CC)
	@echo CFLAGS=$(CFLAGS)
	@echo LDFLAGS=$(LDFLAGS)
	@echo TOPDIR=$(TOPDIR)
	@echo TARGET=$(TARGET)

