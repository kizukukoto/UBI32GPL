TARGET=ssi
SUBDIR=lib core ext
BUILTIN_OBJS=$(addsuffix /built-in.o, $(SUBDIR))
TOPDIR=$(shell pwd)

#SOURCES:= firmware_check.o iprange.o libdb.o libssc.o public.o virtual_file.o
SOURCES:= iprange.o public.o virtual_file.o
#export LDFLAGS += -Wall -lnvram
export CFLAGS += -I$(TOPDIR)/../include -Wall

#CFLAGS +=-I../include -Wall
#LDFLAGS += -L. -lssi

all: $(TARGET)

$(TARGET):$(SUBDIR) $(SOURCES)
	$(CC) $(LDFLAGS) -lnvram -lm $(BUILTIN_OBJS) $(SOURCES) -o $@ $(CGILIB)

$(SUBDIR):
	@$(MAKE) -C $@

clean:
	@for i in $(SUBDIR); do make -C $$i $@; done
	@rm -f $(TARGET) *.o
install:
	#cp ssi $(INSTALLDIR)/bin
	#-$(STRIP) $(INSTALLDIR)/bin/ssi
	install -d $(INSTALLDIR)/www/cgi
	install -d $(INSTALLDIR)/https/cgi
	install -d $(INSTALLDIR)/www/tmp/cgi
	install $(TARGET) $(INSTALLDIR)/www/cgi
	ln -sf ../../cgi/$(TARGET) $(INSTALLDIR)/www/tmp/cgi/$(TARGET)
	ln -sf ../../www/cgi/$(TARGET) $(INSTALLDIR)/https/cgi/$(TARGET)
	$(STRIP) $(INSTALLDIR)/www/cgi/$(TARGET)
.PHONY: $(SUBDIR)
