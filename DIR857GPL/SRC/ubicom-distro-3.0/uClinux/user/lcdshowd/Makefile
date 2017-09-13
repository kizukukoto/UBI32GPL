include Makefile.inc

PRJNAME = lcdshowd
# IMAGE_INSTALL_DIR should be the same as the definition "IMAGE_DIR" in ./include/global.h
IMAGE_INSTALL_DIR = /share/LiTE/lcdshowd/images
SRCS = main.c settings.c info.c status.c lan.c \
	wan.c wlan.c router_info.c statistics.c wps.c  \
	network_util.c lan_settings.c power_saving.c \
	luminance.c

OBJS = $(subst .c,.o,$(SRCS))

DEPENDENCY_HEADER = include/*.h Makefile
SUBDIRS = libs component wizard wizard/wt sample

all: $(PRJNAME) test

$(PRJNAME): $(OBJS) $(foreach i,$(SUBDIRS),$(wildcard $(i)/*.c))
	@for d in $(SUBDIRS); do $(MAKE) -C $$d all || exit 1 ; done
	$(CC) -o $@ $(GUI_CFLAGS) $(OBJS) $(GUI_LDFLAGS) libs/lcdlib.a wizard/wizard.a component/lcdcomponent.a

test: test.o
	$(CC) $(GUI_CFLAGS) test.o $(GUI_LDFLAGS) -o $@ libs/lcdlib.a

clean:
	@rm -rf *.o *~ .sgbuilt_user
	@rm -rf $(PRJNAME)
	@rm -rf include/*~
	@for d in $(SUBDIRS); do $(MAKE) -C $$d clean; done

romfs:
	$(ROMFSINST) -d $(ROOTDIR)/user/lcdshowd/$(PRJNAME) /bin
	$(ROMFSINST) -d $(ROOTDIR)/user/lcdshowd/images $(IMAGE_INSTALL_DIR)
	$(ROMFSINST) -d $(ROOTDIR)/user/lcdshowd/wizard/wt/wt /bin
#	$(ROMFSINST) -d $(ROOTDIR)/user/lcdshowd/wizard/wizard /bin


ifeq ($(CC),cc)
install:
	@if [ ! -d $(IMAGE_INSTALL_DIR) ]; then \
		mkdir -p $(IMAGE_INSTALL_DIR); \
	fi;
	cp -f images/* $(IMAGE_INSTALL_DIR)
else
install: romfs
endif



uninstall:
ifeq ($(CC),cc)
	@if [ -d $(IMAGE_INSTALL_DIR) ]; then \
		rm -rf $(IMAGE_INSTALL_DIR)/*; \
	fi
endif

#.SUFFIXES:  .c

#.c.o: 
#	$(CC) $(GUI_CFLAGS) -c $*.c -o $*.o

# Make sure if any header file is modified, recompile the object
%.o : %.c $(DEPENDENCY_HEADER)
	$(CC) $(GUI_CFLAGS) -c $*.c -o $*.o


PHONY: all clean romfs install uninstall



