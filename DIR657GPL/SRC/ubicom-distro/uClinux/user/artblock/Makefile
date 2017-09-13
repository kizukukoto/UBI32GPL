CFLAGS += -I. -I../sutil/

all: artblock.o libartblock.so artblock_cmd

artblock_cmd.o: artblock_cmd.c
	$(CC) $(CFLAGS) -c $^

artblock_cmd: artblock_cmd.o artblock.o
	$(CC) $(CFLAGS) -o $@ $^

artblock.o: artblock.c
	$(CC) $(CFLAGS) -c $^

libartblock.so: artblock.o
	$(CC) -shared -o $@ $^

install:
	install -D artblock_cmd $(TARGET)/bin/artblock_cmd
	install -D libartblock.so $(TARGET)/lib/libartblock.so
	$(STRIP) $(TARGET)/bin/artblock_cmd
	$(STRIP) $(TARGET)/lib/libartblock.so
romfs:
	$(ROMFSINST) artblock_cmd /bin/artblock_cmd
	$(ROMFSINST) libartblock.so /lib/libartblock.so

.PHONY: clean install

clean:
	rm  -rf *.o libartblock.so artblock_cmd
