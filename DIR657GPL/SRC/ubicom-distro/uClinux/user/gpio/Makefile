# GPIO Makefile
# 
#CFLAGS += -I./ -I../sutil/ -I../nvram/ -D$(CONFIG_MODEL_NAME)
CFLAGS += -I. -I$(ROOTDIR)/user/sutil/ -I$(ROOTDIR)/user/nvram/ -I$(ROOTDIR)/user/libplatform/
CFLAGS +=  -DDLINK_ROUTER_LED_DEFINE -DATHEROS_11N_DRIVER -DUBICOM -DAR7161	

OBJS = gpio.o 

all: gpio	

.c.o:
	$(CC) $(CFLAGS) -c $<

install:
	cp gpio $(TARGET)/sbin

clean:
	rm -f *.o gpio gpio.o

gpio: $(OBJS)
	$(CC) -o  $@ $^ $(CFLAGS)
	

romfs:
	$(ROMFSINST) gpio /sbin/gpio
