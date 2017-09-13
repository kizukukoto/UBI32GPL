include ../../Makefile.inc
GUI_CFLAGS += -I../../include

SLIDING_OBJS = sliding.o menu.o label.o image.o misc.o mouse.o

all: sliding

sliding: $(SLIDING_OBJS)
	$(CC) $(GUI_CFLAGS) $(SLIDING_OBJS) $(GUI_LDFLAGS) -o $@

clean:
	@rm -rf *.o sliding

%.o : %.c $(DEPENDENCY_HEADER)
	$(CC) $(GUI_CFLAGS) -c $*.c -o $*.o
