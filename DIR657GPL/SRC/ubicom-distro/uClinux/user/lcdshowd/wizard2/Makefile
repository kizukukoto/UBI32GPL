include ../Makefile.inc
GUI_CFLAGS += -I../include

SRCS = wizard.c	button_option.c welcome.c install.c network.c wireless.c timezone.c final.c dhcpc_list.c nvram.c page.c info.c
OBJS = $(subst .c,.o,$(SRCS))

all: wizard.a

wizard.a: $(OBJS)
	$(AR) cr $@ $^

clean:
	@rm -rf $(OBJS) wizard.a *~

%.o : %.c $(DEPENDENCY_HEADER)
	$(CC) $(GUI_CFLAGS) -c $*.c -o $*.o
