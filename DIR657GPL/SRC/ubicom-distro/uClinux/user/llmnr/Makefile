# LLMNR Makefile

TARGET      = $(ROOTDIR)/romfs

CFLAGS += -I. -I$(APPS_PATH)/libplatform/
LDFLAGS += -L$(APPS_PATH)/libplatform
OBJS1 = llmnr.o
ALL = llmnr

all: $(ALL)

llmnr:$(OBJS1)
	$(CC) -o $@ $^ $(LDFLAGS)

install:
	$(CROSS_COMPILE)strip $(ALL)
	cp $(ALL) $(TARGET)/sbin
	cp $(ALL) /home/AthSDK

clean:
	rm -f *.o *~ $(ALL)

romfs:
	$(ROMFSINST) llmnr /sbin/llmnr
