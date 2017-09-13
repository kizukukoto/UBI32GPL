
EXEC = ubicom-perf
OBJS = ubicom-perf.o

all: $(EXEC)

$(EXEC): $(OBJS)
	$(CC) $(LDFLAGS) -lpthread -o $@ $(OBJS) $(LDLIBS)

romfs:
	$(ROMFSINST) /bin/$(EXEC)
clean:
	-rm -f $(EXEC) *.gdb *.elf *.o
