# Makefile to build the SDL tests

srcdir  = .

MY_DIR = $(shell pwd)
ROOTDIR = $(MY_DIR)/../../..
MY_GCCLIBDIR = $(dir $(shell ubicom32-elf-gcc -print-libgcc-file-name))
 
CC      = ubicom32-elf-gcc
EXE	= 
CFLAGS  = --spec $(ROOTDIR)/vendors/config/ubicom32nommu/spec -march=ubicom32v4 -O2 -g -fomit-frame-pointer -pipe -fno-common -fno-builtin -Wall -DEMBED -Dlinux -D__linux__ -Dunix -D__uClinux__ -I$(ROOTDIR)/include -I$(ROOTDIR)/uClibc/include -I$(ROOTDIR)/linux-2.6.x/include -I$(ROOTDIR)/lib/libsdl/build-SDL-1.2.13/install/include/SDL -D_GNU_SOURCE=1 -D_REENTRANT
LIBS	= -Wl,-elf2flt -g -static -L$(ROOTDIR)/uClibc/lib -L$(MY_GCCLIBDIR) -Wl,-elf2flt $(ROOTDIR)/uClibc/lib/crt1.o -L$(ROOTDIR)/lib -L$(ROOTDIR)/lib/SDL_mixer/build-SDL_mixer-1.2.8/build/.libs -Wl,-rpath,$(ROOTDIR)/lib/SDL_mixer/build-SDL_mixer-1.2.8/build/.libs/ $(ROOTDIR)/lib/SDL_mixer/build-SDL_mixer-1.2.8/build/.libs/libSDL_mixer.a -L$(ROOTDIR)/lib/libsdl/build-SDL-1.2.13/install/lib -Wl,-rpath,$(ROOTDIR)/lib/libsdl/build-SDL-1.2.13/install/lib $(ROOTDIR)/lib/libsdl/build-SDL-1.2.13/install/lib/libSDLmain.a $(ROOTDIR)/lib/libsdl/build-SDL-1.2.13/install/lib/libSDL.a -lpthread -lm

TARGETS = playmus$(EXE) playwave$(EXE)

all: $(TARGETS)

playmus$(EXE): $(srcdir)/playmus.c
	$(CC) -o $@ $? $(CFLAGS) $(LIBS)

playwave$(EXE): $(srcdir)/playwave.c
	$(CC) -o $@ $? $(CFLAGS) $(LIBS)

clean:
	rm -f $(TARGETS)
