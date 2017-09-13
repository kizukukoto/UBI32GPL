include ../Makefile.inc
GUI_CFLAGS += -I../include

SRCS = wizard.c wizard_step.c wizard_timezone.c dhcpc_list.c wizard_save.c 
OBJS = $(subst .c,.o,$(SRCS))

all: wizard.a

wizard.a: $(OBJS)
	$(AR) cr $@ $^

clean:
	@rm -rf $(OBJS) wizard.a *~

%.o : %.c $(DEPENDENCY_HEADER)
	$(CC) $(GUI_CFLAGS) -c $*.c -o $*.o
