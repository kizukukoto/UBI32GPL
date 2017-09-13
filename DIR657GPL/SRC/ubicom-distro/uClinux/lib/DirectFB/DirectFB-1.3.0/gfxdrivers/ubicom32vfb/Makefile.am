## Makefile.am for DirectFB/gfxdrivers/ubicom32vfb

INCLUDES = \
	-I$(top_srcdir)/include \
	-I$(top_builddir)/lib \
	-I$(top_srcdir)/lib \
	-I$(top_srcdir)/systems \
	-I$(top_srcdir)/src

ubicom32vfb_LTLIBRARIES = libdirectfb_ubicom32vfb.la

if BUILD_STATIC
ubicom32vfb_DATA = $(ubicom32vfb_LTLIBRARIES:.la=.o)
endif

ubicom32vfbdir = $(MODULEDIR)/gfxdrivers

libdirectfb_ubicom32vfb_la_SOURCES = \
	ubicom32vfb.c \
	ubicom32vfb.h \
	ubicom32vfb_primary.c \
	ubicom32vfb_blt.c \
	ubicom32vfb_lcd.c \
	ubicom32vfb_lcd.h \
	ubicom32vfb_lcd_panels.c \
	ubicom32vfb_lcd_panels.h

libdirectfb_ubicom32vfb_la_LDFLAGS = \
	-export-dynamic \
	-avoid-version \
	$(DFB_LDFLAGS)

libdirectfb_ubicom32vfb_la_LIBADD = \
	$(top_builddir)/lib/direct/libdirect.la \
	$(top_builddir)/src/libdirectfb.la

include $(top_srcdir)/rules/libobject.make
