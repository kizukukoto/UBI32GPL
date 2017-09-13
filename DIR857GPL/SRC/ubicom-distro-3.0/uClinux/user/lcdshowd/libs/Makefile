include ../Makefile.inc
GUI_CFLAGS += -I../include -I$(ROOTDIR)/lib/tslib/build-ubicom/install/include

SRCS = misc.c mouse.c mouse1.c menu.c label.c image.c input.c dialog.c lite_destroy.c check.c timer.c calibrate.c calibration.c
OBJS = $(subst .c,.o,$(SRCS))

all: lcdlib.a

lcdlib.a: $(OBJS)
	$(AR) cr $@ $^

clean:
	@rm -rf $(OBJS) *.a *~

%.o : %.c $(DEPENDENCY_HEADER)
	$(CC) $(GUI_CFLAGS) -c $*.c -o $*.o
