include ../../Makefile.inc
GUI_CFLAGS += -I../../include

WINDOW_OBJS = main.o

all: window

window: $(WINDOW_OBJS)
	$(CC) $(GUI_CFLAGS) $(WINDOW_OBJS) $(GUI_LDFLAGS) -o $@

clean:
	@rm -rf *.o window

%.o : %.c $(DEPENDENCY_HEADER)
	$(CC) $(GUI_CFLAGS) -c $*.c -o $*.o
