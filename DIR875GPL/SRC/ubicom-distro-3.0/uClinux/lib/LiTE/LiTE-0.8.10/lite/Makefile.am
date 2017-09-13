liteincludedir = $(includedir)/lite

INCLUDES = @DFB_CFLAGS@

AM_CPPFLAGS = \
	-DDATADIR=\"@DATADIR@/@PACKAGE@/\"	\
	-DLITEFONTDIR=\"@LITEFONTDIR@\"

lib_LTLIBRARIES = liblite.la

liblite_la_SOURCES =	\
	box.c		\
	cursor.c	\
	event.c		\
	font.c		\
	lite.c		\
	lite_internal.h	\
	lite_config.h	\
	theme.c		\
	util.c		\
	window.c

liteinclude_HEADERS =	\
	box.h		\
	cursor.h	\
	event.h		\
	font.h		\
	lite.h		\
	theme.h		\
	util.h		\
	window.h


liblite_la_LDFLAGS = -version-info @LITE_VERSION_INFO@ -Wl,-O1 -no-undefined
liblite_la_LIBADD = @DFB_LIBS@

