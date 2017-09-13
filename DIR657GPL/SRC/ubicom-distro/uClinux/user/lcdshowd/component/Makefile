include ../Makefile.inc
GUI_CFLAGS += -I../include

SRCS = clist.c select.c
OBJS = $(subst .c,.o,$(SRCS))

all: lcdcomponent.a

lcdcomponent.a: $(OBJS)
	$(AR) cr $@ $^

clean:
	@rm -rf $(OBJS) *.a *.o

%.o : %.c $(DEPENDENCY_HEADER)
	$(CC) $(GUI_CFLAGS) -c $*.c -o $*.o
