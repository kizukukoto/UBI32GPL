
EXEC = mathopd
OBJS = base64.o cgi.o config.o core.o dump.o imap.o log.o main.o \
	redirect.o request.o util.o stub.o

all: $(EXEC)

romfs:
	$(ROMFSINST) /bin/$(EXEC)

$(EXEC): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LDLIBS) -lresolv -lcrypt -lnsl

clean:
	-rm -f $(EXEC) *.elf *.gdb *.o
	

$(OBJS): mathopd.h
