#Router ntpclient Makefile
#
#CFLAGS += -DNTP_DEBUG
#CFLAGS += -I./ -I$(APPS_PATH)/sutil/ -I$(APPS_PATH)/nvram/
CFLAGS += -I./ -I$(ROOTDIR)/user/sutil/ -I$(ROOTDIR)/user/nvram/
CFLAGS += -I$(ROOTDIR)/config
LDFLAGS =


OBJS1 := phaselock.o ntpclient.o 
#OBJS2 := rc_ntp.o 


#all: ntpclient ntptime
all: ntpclient

.c.o:
	$(CC) $(CFLAGS) -c $<
		

clean:
	rm -f *.o *~ ntpclient  .*.depend
#	rm -f *.o *~ ntpclient ntptime .*.depend

install: all
	install -d $(TARGET)/sbin
#	install ntpclient ntptime $(TARGET)/sbin
	install ntpclient $(TARGET)/sbin
	$(STRIP)	$(TARGET)/sbin/ntpclient
#	$(STRIP) $(TARGET)/sbin/ntptime

ntpclient: $(OBJS1)
	$(CC) -o $@ $^ $(LDFLAGS)
#	$(CC) -o $@ $(OBJS1) $(CFLAGS)

#ntptime: $(OBJS2)
#	$(CC) -o $@ $^ $(LDFLAGS)
#	$(CC) -o $@ $(OBJS2) $(CFLAGS)

romfs:
	$(ROMFSINST) ntpclient /sbin/ntpclient
