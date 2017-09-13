#
# Makefile for libcrashlog
# (C) Copyright 2009, Ubicom Inc.
#
# This file is subject to the terms and conditions of the GNU Lesser
# General Public License.  See the file COPYING.LIB in the main
# directory of this archive for more details.
#

default:

LIBNAME=libcrashlog

SRCS:= crash_log.c
ifndef BINDIR
	BINDIR=.
endif

OBJS:= $(addprefix $(BINDIR)/,$(SRCS:.c=.o))
CFLAGS+= -D_GNU_SOURCE


default:$(BINDIR)/$(LIBNAME).a $(BINDIR)/$(LIBNAME).so.0
	echo $(LIBNAME) Complete
clean:
	rm -f $(BINDIR)/$(LIBNAME).so* $(BINDIR)/$(LIBNAME).a $(OBJS)

$(BINDIR)/$(LIBNAME).so.0: $(OBJS) $(SRCS) Makefile
	@echo LD $@
	$(CC) $(LDFLAGS) -shared -ldl -Wl,-soname,$(LIBNAME).so.0 -o $@ $(OBJS)
	ln -sf $@ $(BINDIR)/$(LIBNAME).so

$(BINDIR)/$(LIBNAME).a: $(OBJS) Makefile
	@echo AR $@
	$(AR) rc $@ $(OBJS)

$(BINDIR)/%.o: %.c Makefile
	@echo CC $< .. $@
	$(CC) $(CFLAGS) -c $< -o $@

romfs:
	$(ROMFSINST) -d $(BINDIR)/$(LIBNAME).so.0 /lib/$(LIBNAME).so.0
