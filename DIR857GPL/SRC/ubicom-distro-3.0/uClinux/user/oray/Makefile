# Router rc Makefile
#
CFLAGS =
#CFLAGS += -I./
LDFLAGS = 
#CC = $(TOOLCHAIN)gcc
#CC = $(CROSS_COMPILE)gcc
#STRIP = $(CROSS_COMPILE)strip

OBJS := orayddns.o oray_md5.o base64.o blowfish.o option.o

all: orayddns

.c.o:
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f *.o orayddns .*.depend *~

orayddns: $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

romfs:
	$(ROMFSINST) orayddns /sbin/orayddns

