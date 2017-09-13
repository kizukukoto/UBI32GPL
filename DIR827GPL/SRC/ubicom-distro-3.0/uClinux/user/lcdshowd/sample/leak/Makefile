include ../../Makefile.inc
GUI_CFLAGS += -I../../include

all: leak loop

leak: leak.o
	$(CC) $(GUI_CFLAGS) leak.o $(GUI_LDFLAGS) -o $@

loop: loop.o
	$(CC) $(GUI_CFLAGS) loop.o $(GUI_LDFLAGS) -o $@

clean:
	@rm -rf *.o leak loop

%.o : %.c $(DEPENDENCY_HEADER)
	$(CC) $(GUI_CFLAGS) -c $*.c -o $*.o
