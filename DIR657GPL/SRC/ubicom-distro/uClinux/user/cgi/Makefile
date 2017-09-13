TOPDIR=$(shell pwd)
SUBDIR= lib main misc

CGILIB_OBJ = $(TOPDIR)/lib/cgilib.a
INCPATH += -I $(TOPDIR)/include -I $(DBINC_PATH) -I $(TOPDIR)/../../include
CFLAGS  += -Wall -O2 $(INCPATH)
LDFLAGS += -L$(DBLIB_PATH) -L$(TOPDIR)/main

all: env indep $(SUBDIR)

host:$(SUBDIR)
	
$(SUBDIR):
	$(MAKE) CFLAGS="$(CFLAGS)" LDFLAGS="$(LDFLAGS)" -C $@ all CGILIB="$(CGILIB_OBJ)"

clean: uindep
	@for i in $(SUBDIR);do make -C $$i clean;done

distclean:clean

install: all
	#install -D main/ssi $(INSTALLDIR)/www/cgi/ssi
	#$(STRIP) $(INSTALLDIR)/www/cgi/ssi
	#$(MAKE) -C misc install INSTALLDIR=$(INSTALLDIR)
	@for i in $(SUBDIR);do make -C $$i install INSTALLDIR=$(INSTALLDIR);done


ifeq ($(CONFIG_MODEL_NAME), DIR-665_A1)
include Makefile.DIR665
endif
ifeq ($(CONFIG_VENDOR), Ubicom)
CFLAGS  += -DCONFIG_USER_STREAMENGINE
include Makefile.Ubicom
else
include Makefile.DIRX30
endif

env:
ifndef DBINC_PATH
	$(error Nvram include path lost, you must define DBINC_PATH variable)
endif
ifndef DBLIB_PATH
	$(error Nvram library path lost, you must define DBLIB_PATH variable)
endif
.PHONY: all indep uindep clean distclean install $(SUBDIR)
