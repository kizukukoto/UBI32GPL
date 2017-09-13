
#
# Makefile for http stuff
#

include $(TOPDIR)/config.mk
#include $(TOPDIR)/board/mv88fxx81/db88f5181/mvRules.mk
include $(TOPDIR)/include/config.mk
include $(TOPDIR)/../uClinux/.config

BYTE_ORDER = BIG_ENDIAN
_BYTE_ORDER_ = $(BYTE_ORDER)
ifeq ($(_BYTE_ORDER_),)
$(error "Must specify BYTE_ORDER to LITTLE_ENDIAN or BIG_ENDIAN")
else
CFLAGS += -DBYTE_ORDER=$(_BYTE_ORDER_)
endif

#_DDR2_SIZE_ = $(DDR2_SIZE)
#ifeq ($(_DDR2_SIZE_),)
#$(error "Must specify DDR2_SIZE to 64MB or 128MB or ...")
#else
#CFLAGS += -DDDR2_SIZE_$(_DDR2_SIZE_)
#endif

#CFLAGS += -DDEBUG_HTTP -DUIP_LOGGING=1
#CFLAGS += -DDEBUG_HTTP
CFLAGS += -DATHEROS_11N_DRIVER -I../include -I../httpd


LIB   = libhttpd.a

OBJS += uip.o uip_arch.o uip_arp.o httpd.o fs.o cgi.o

all: cp_bsp $(LIB)
#all: $(LIB)

cp_bsp:
	cp -f $(TOPDIR)/../uClinux/vendors/$(CONFIG_VENDOR)/$(CONFIG_PRODUCT)/bsp.h ./bsp.h

$(LIB):  $(START) $(OBJS)
	   $(AR) crv $@ $(OBJS)


#%.o:
#	   $(CC) $(CFLAGS) -c $(<:.o=.c)

#########################################################################

.depend: Makefile $(OBJS:.o=.c)
	      $(CC) -M $(CFLAGS) $(OBJS:.o=.c) > $@
include $(SRCTREE)/rules.mk
sinclude .depend

#########################################################################

