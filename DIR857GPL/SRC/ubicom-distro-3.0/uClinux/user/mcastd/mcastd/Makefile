#CFLAGS = -fpic -I../../libxmo/
#LDFLAGS = -fpic -L../../libxmo -lxmo -ldl
# NO idea why dynamic link cmds.c will system fail at reg I/O
CFLAGS = 
LDFLAGS = -ldl
OBJS=mcastd.o hal.o cmds.o
all: mcastd 
clean:
	rm -fr $(OBJS) mcastd
install:
	install -d $(TARGET)/sbin
	cp romfs/* -arf $(TARGET)/
	install mcastd $(TARGET)/sbin/

mcastd: $(OBJS)
#make apps/mcastd CONFIG_MCASTD=y  -C ../../../

