#
# Makefile for ubicom32
#
TARGET	    = $(ROOTDIR)/romfs
#all: modules nvram
all: replace_link  nvram

libnvram.so: libnvram.o nvram_compatible.o
	$(CC)  $(LDFLAGS) $^ -shared -o $@

replace_link:
	[ -h ../nvram ] || (cd ../;rm -fr nvram;ln -s nvram_linux nvram)
nvram: main.o libnvram.so
	$(CC) $(LDFLAGS) $^ -o $@

clean:
	rm -rf *.o *~ core .depend .*.cmd *.ko *.mod.c .tmp_versions
	rm -rf nvram *.so

install:
	install -d $(TARGET)/bin
	install nvram $(TARGET)/bin
	$(STRIP)	$(TARGET)/bin/nvram
	install -D libnvram.so $(TARGET)/lib/libnvram.so	
	$(STRIP) $(TARGET)/lib/libnvram.so	
	#install $(WWW_PATH)/nvram.default $(TARGET)/etc/

uninstall:
	rm -rf $(TARGET)/bin/nvram
	rm -rf $(TARGET)/lib/libnvram.so
	

romfs:
	$(ROMFSINST) nvram /bin
	$(ROMFSINST) libnvram.so /lib


