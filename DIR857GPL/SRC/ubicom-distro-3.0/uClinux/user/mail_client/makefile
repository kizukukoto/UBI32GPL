# Configure options.
#CONFOPTS := --disable-rpath \


CFLAGS  += -I./ -I$(ROOTDIR)/user/nvram/ -I$(ROOTDIR)/user/sutil/
LDFLAGS += -L$(ROOTDIR)/user/nvram -L$(ROOTDIR)/user/sutil
LDLIBS += -lnvram -lsutil
CFLAGS += -I$(ROOTDIR)/config
 
ifeq ($(CONFIG_USER_MAIL_CLIENT_DEBUG),y)
CFLAGS += -DMAIL_CLIENT_DEBUG
endif

all: build/build
	$(MAKE) -C build

build/build: makefile
	@chmod u+x configure
	@rm -rf build
	@mkdir build
	(cd build; sh ../configure $(CONFIGURE_OPTS) $(CONFOPTS) CFLAGS="${CFLAGS}" LDFLAGS="${LDFLAGS}" LIBS="${LDLIBS}")
	@touch build/build

clean:
	rm -rf build

romfs:
	$(ROMFSINST) build/src/msmtp /sbin/

