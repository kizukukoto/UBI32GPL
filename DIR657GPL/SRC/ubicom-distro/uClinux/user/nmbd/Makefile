# Router rc Makefile
#
TARGET	    = $(ROOTDIR)/romfs

CFLAGS += -I. -I$(ROOTDIR)/user/sutil/ -I$(ROOTDIR)/user/libplatform/ -I$(ROOTDIR)/user/nvram/
LDFLAGS += -L$(ROOTDIR)/user/sutil -L$(ROOTDIR)/user/libplatform -L$(ROOTDIR)/user/nvram
LDFLAGS += -lsutil -lm -lnvram

OBJS :=  sserver.o 

all: nmbd

.c.o:
	$(CC) $(CFLAGS) -c $<
		
clean:
	rm -f *.o *~ nmbd .*.depend

install: nmbd
	install -d $(TARGET)/sbin
	install nmbd $(TARGET)/sbin
	$(STRIP) $(TARGET)/sbin/nmbd
nmbd: $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)
#	$(CC) -o $@ $(OBJS) $(CFLAGS)

romfs:
	$(ROMFSINST) nmbd /sbin/nmbd
