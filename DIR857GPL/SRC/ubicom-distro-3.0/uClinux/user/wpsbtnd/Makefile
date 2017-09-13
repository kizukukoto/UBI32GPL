
EXEC = wpsbtnd
OBJS = wpsbtnd.o

all: $(EXEC)

$(EXEC): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LDLIBS)

romfs:
	$(ROMFSINST) -d $(EXEC) /sbin/$(EXEC)

clean:
	-rm -f $(EXEC) *.elf *.gdb *.o

