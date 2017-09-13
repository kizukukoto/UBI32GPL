# Router rc Makefile
#

#CFLAGS += -I./ -I$(TOP)/sutil/ -I$(TOP)/nvram/ -I$(APPS_PATH)/libplatform -D$(PLATFORM)
CFLAGS += -I./ -I$(ROOTDIR)/user/sutil/ -I$(ROOTDIR)/user/nvram/ -I$(ROOTDIR)/user/libplatform/ -I$(ROOTDIR)/user/rc/
CFLAGS += -I$(ROOTDIR)/config
#LDFLAGS += -L$(TOP)/nvram -lnvram -L$(TOP)/sutil -lsutil -L$(APPS_PATH)/libplatform -lversion
LDFLAGS += -L$(ROOTDIR)/user/nvram -L$(ROOTDIR)/user/sutil -L$(ROOTDIR)/user/libplatform
#LDFLAGS += -L$(APPS_PATH)/libplatform
LDLIBS += -lnvram -lsutil -lversion -lvct


ifeq ($(CONFIG_USER_LINUX_NVRAM),y)
CFLAGS += -I../artblock
LDFLAGS += -L../artblock -lartblock
endif

OBJS :=  sserver.o tftp.o 

ifeq ($(CONFIG_USER_TFTPD_DEBUG),y)
CFLAGS += -DTFTPD_DEBUG
endif

all: tftpd

.c.o:
	$(CC) $(CFLAGS) -c $<
		

clean:
	rm -f *.o *~ tftpd .*.depend
	rm -f $(WWW_PATH)/*.o


install: tftpd
	install -d $(TARGET)/sbin
	install tftpd $(TARGET)/sbin
	$(STRIP) $(TARGET)/sbin/tftpd

tftpd: $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS) $(LDLIBS)
#	$(CC) -o $@ $(OBJS) $(CFLAGS)


romfs:
	$(ROMFSINST) tftpd /sbin/tftpd
