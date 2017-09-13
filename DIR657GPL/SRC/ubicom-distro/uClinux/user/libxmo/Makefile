OBJS=libcmds.o cmds.o
CFLAGS=-Wall -fPIC
LDFLAGS=

all: libxmo.so

libxmo.so: $(OBJS)
	$(CC) -o $@ -shared $^

install:
	@cp  libxmo.so $(INSTALLDIR)/lib/
	-$(STRIP) $(INSTALLDIR)/lib/libxmo.so

romfs:
	$(ROMFSINST) libxmo.so  /lib/libxmo.so
clean:
	-@rm libxmo.so $(OBJS)
