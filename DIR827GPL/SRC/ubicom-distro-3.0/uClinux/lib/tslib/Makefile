VER = tslib-1.0

BUILD_DIR:=build-ubicom
TSLIB_PREFIX:=$(ROOTDIR)/lib/tslib/$(BUILD_DIR)/install

all: $(BUILD_DIR)/Makefile
	$(MAKE) -C $(BUILD_DIR)
	$(MAKE) -C $(BUILD_DIR) install > install.log

$(BUILD_DIR)/Makefile: $(VER)/configure Makefile
	set -e ; \
	rm -rf $(BUILD_DIR) ; \
	mkdir $(BUILD_DIR) ; \
	cd $(BUILD_DIR) ; \
	LIBS="-lpthread" \
	../$(VER)/configure  \
	--host=$(CONFIGURE_HOST) \
	--disable-linear-h2200 \
	--disable-ucb1x00 \
	--disable-corgi \
	--disable-collie \
	--disable-h3600 \
	--disable-mk712 \
	--disable-arctic2 \
	--prefix=$(TSLIB_PREFIX)

clean:
	rm -rf build*

romfs:
	$(ROMFSINST) -d build-ubicom/install/bin /bin
	$(ROMFSINST) -d build-ubicom/install/lib /lib
	$(ROMFSINST) -d build-ubicom/install/etc /etc

.PHONY: all clean romfs
