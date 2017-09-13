#
# GCC Library
#
CC := ubicom32-uclinux-gcc -march=ubicom32v4
LD := ubicom32-uclinux-ld
GCCLIBDIR := $(dir $(shell $(CC) $(CFLAGS) -print-libgcc-file-name))
UCLIBCDIR := ../../uClibc/lib
UCLIBC_INCLUDE_DIR := $(UCLIBCDIR)/../include
LINUX_INCLUDE_DIR := $(UCLIBCDIR)/../linux-2.6.x/include
LDFLAGS += -elf2flt -g -L$(UCLIBCDIR) -L$(GCCLIBDIR)
CFLAGS +=  -g \
	-Wall \
	-I$(UCLIBC_INCLUDE_DIR) \
	-I$(LINUX_INCLUDE_DIR)

%.o : %.c
	$(CC) -c $(CFLAGS) $< -o $@

%.o : %.cpp
	$(CC) -c $(CFLAGS) $< -o $@

all: hello hello++

clean:
	rm -f hello++
	rm -f hello++.o
	rm -f hello
	rm -f hello.o
	rm -f hack.o
	rm -f *.gdb *~

hello: hello.o hack.o
	$(LD) $(LDFLAGS) -o $@ $^ $(UCLIBCDIR)/crt1.o -lc -lgcc

hello++: hello++.o
	$(CC) $(LDFLAGS) -o $@ $^ -lc -lgcc -lstdc++ -lpthread

