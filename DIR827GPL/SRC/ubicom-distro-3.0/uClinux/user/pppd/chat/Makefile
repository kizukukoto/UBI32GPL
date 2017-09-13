#	$Id: Makefile,v 1.1 2009/01/19 15:53:05 ken_chiang Exp $

DESTDIR = $(TARGET)
BINDIR = $(DESTDIR)/sbin
MANDIR = $(DESTDIR)/share/man

CDEF1=	-DTERMIOS			# Use the termios structure
CDEF2=	-DSIGTYPE=void			# Standard definition
CDEF3=	-UNO_SLEEP			# Use the usleep function
CDEF4=	-DFNDELAY=O_NDELAY		# Old name value
CDEFS=	$(CDEF1) $(CDEF2) $(CDEF3) $(CDEF4)

COPTS=	-O2 -g -pipe
CFLAGS=	$(COPTS) $(CDEFS)

INSTALL= install

all:	chat

chat:	chat.o
	$(CC) -o chat chat.o

chat.o:	chat.c
	$(CC) -c $(CFLAGS) -o chat.o chat.c

install: chat
#	mkdir -p $(BINDIR) $(MANDIR)
	mkdir -p $(BINDIR)
	$(INSTALL) -c chat $(BINDIR)
#	$(INSTALL) -c -m 644 chat.8 $(MANDIR)

clean:
	rm -f chat.o chat *~
