FLTFLAGS = -s 32768

all:
	$(MAKE) -C build
	$(MAKE) -C test
clean:
	$(MAKE) -C build clean
romfs:
#	$(MAKE) -C build romfs
	$(ROMFSINST) build/bin/tr /sbin/tr069


# copy base file
	$(ROMFSINST) -d conf/tr.conf /home/tr/conf/tr.conf
	$(ROMFSINST) conf/ca.pem  /home/tr/conf/ca.pem
	$(ROMFSINST) conf/client.crt  /home/tr/conf/client.crt
	$(ROMFSINST) conf/client.key  /home/tr/conf/client.key
	$(ROMFSINST) conf/tr.xml  /home/tr/conf/tr.xml
	$(ROMFSINST) conf/tr.xml  /home/tr/conf/tr.xml.bak
	$(ROMFSINST) conf/update  /home/tr/conf/update



# 652EU tr.xml
ifeq ($(CONFIG_PRODUCT),DIR652EU)
	echo DIR652EU
	$(ROMFSINST) conf/tr_652EU.xml /home/tr/conf/tr.xml
	$(ROMFSINST) conf/tr_652EU.xml /home/tr/conf/tr.xml.bak

endif

ifeq ($(CONFIG_PRODUCT),DIR655)
	echo DIR655
	$(ROMFSINST) conf/tr_655.xml /home/tr/conf/tr.xml
	$(ROMFSINST) conf/tr_655.xml /home/tr/conf/tr.xml.bak

endif

ifeq ($(CONFIG_PRODUCT),DIR655GL)
	echo DIR655GL
	$(ROMFSINST) conf/tr_655GL.xml /home/tr/conf/tr.xml
	$(ROMFSINST) conf/tr_655GL.xml /home/tr/conf/tr.xml.bak

endif
