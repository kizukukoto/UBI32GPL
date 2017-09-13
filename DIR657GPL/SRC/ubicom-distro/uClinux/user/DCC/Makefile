
TARGET      = $(ROOTDIR)/romfs

CFLAGS += -I$(ROOTDIR)/user/sutil/ -I$(ROOTDIR)/user/nvram/ -I$(ROOTDIR)/user/libplatform/
CFLAGS += -I$(ROOTDIR)/config
LDFLAGS += -L$(ROOTDIR)/user/sutil -L$(ROOTDIR)/user/nvram -L$(ROOTDIR)/user/libplatform/
LDFLAGS += -lsutil -lm -lnvram -lvct
ifeq ($(CONFIG_USER_LINUX_NVRAM),y)
CFLAGS += -I../artblock
LDFLAGS += -L../artblock -lartblock
endif

OBJS := dcc.o

all: dcc


clean:
	rm -f *.o *~ dcc

install: all
	install -d $(TARGET)/sbin
	install dcc $(TARGET)/sbin
	$(STRIP) $(TARGET)/sbin/dcc

dcc: $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

romfs:
	$(ROMFSINST) dcc /sbin

