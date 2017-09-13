TARGET	    = $(ROOTDIR)/romfs
CFLAGS += -I./ -I$(ROOTDIR)/user/sutil/
CFLAGS += -I$(ROOTDIR)/config
LDFLAGS += -L$(ROOTDIR)/user/sutil -lsutil

ifeq ($(CONFIG_USER_WL_ATH_5GHZ),y)
CFLAGS += -DUSER_WL_ATH_5GHZ
endif

OBJS := arpping.o

all: arpping

.c.o:
	$(CC) $(CFLAGS) -c $< $(LDFLAGS)

clean:
	rm -f *.o arpping .*.depend *~

install: all
	install -d $(TARGET)/sbin
	install arpping $(TARGET)/sbin
	$(STRIP) $(TARGET)/sbin/arpping

arpping: $(OBJS)
	$(CC) -lpthread -o $@ $^ $(LDFLAGS)

romfs:
	$(ROMFSINST) arpping /sbin

