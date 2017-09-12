include ../../arch.mk
include ../../../path.mk

MODULE_NAME := mod_xsendfile

MODULE_INCS := -I../build/httpd-2.2.9/include
MODULE_INCS += -I../build/httpd-2.2.9/os/unix
MODULE_INCS += -I../build/httpd-2.2.9/srclib/apr/include
MODULE_INCS += -I../build/httpd-2.2.9/srclib/apr-util/include

ifeq ($(wildcard $(DESTDIR)),)
DESTDIR=$(TARGET)
endif

all:
	$(Q)$(CC) $(CFLAGS) $(MODULE_INCS) -c *.c
	$(Q)$(CC) $(LDFLAGS) -shared -Wl,-soname,$(MODULE_NAME).so -o $(MODULE_NAME).so $(MODULE_NAME).o

install:
	$(Q)cp -f $(MODULE_NAME).so $(DESTDIR)/usr/modules/.

clean:
	$(Q)rm -rf *.o *.so
