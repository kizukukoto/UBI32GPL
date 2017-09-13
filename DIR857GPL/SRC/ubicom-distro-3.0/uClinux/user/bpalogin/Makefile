###########################################################################
# Makefile for BPALogin v2.0
##

##
# Install locations as determined by configure
TARGET=$(ROOTDIR)/romfs
prefix=${TARGET}
exec_prefix=${prefix}
SBINPATH=${exec_prefix}/sbin
ETCPATH=${prefix}/etc

##
# Locations of system utilities as determined by configure
CHKCONFIG=/sbin/chkconfig
INSTALL=/usr/bin/install -c
SED=/bin/sed

##
#
EXEC = bpalogin
CFLAGS+= -I$(ROOTDIR)/user/nvram
LIBS=-lnsl 
LDFLAGS=
EXTRA_OBJS=$(ROOTDIR)/user/nvram/nvram.o
OBJS = bpalogin.o protocol.o transaction.o gen_md5.o unixmain.o

bpalogin: $(OBJS)
	$(CC) -o $@ $(OBJS) $(EXTRA_OBJS) $(LIBS) $(LDFLAGS)
	$(STRIP) -s $@

.c.o:
	$(CC) $(CFLAGS) -c $<

clean:
	rm -fr *.o bpalogin

distclean: clean
	rm -f bpalogin config.log config.cache config.status Makefile bpalogin.h

bpalogin.h: bpalogin.h.in
	$(SED) 's#@@BPALOGIN_CONF@@#$(ETCPATH)/bpalogin.conf#' < bpalogin.h.in > bpalogin.h

$(OBJS): bpalogin.h

install: bpalogin bpalogin.init bpalogin.bsd
	$(INSTALL) -m 755 bpalogin $(SBINPATH)

romfs:
	$(ROMFSINST) /bin/$(EXEC)

