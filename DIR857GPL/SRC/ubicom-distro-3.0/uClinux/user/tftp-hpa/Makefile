VER=-hpa-0.49

all: .build
	$(MAKE) -C tftp$(VER) 

.build:Makefile 
	(cd tftp$(VER); ./configure $(CONFIGURE_OPTS))
	touch Makefile	

clean:
	$(MAKE) -C tftp$(VER) clean

distclean:
	$(MAKE) -C tftp$(VER) distclean

romfs:
	$(ROMFSINST) tftp$(VER)/tftp/tftp /bin/tftp
	$(ROMFSINST) tftp$(VER)/tftpd/tftpd /bin/tftpd 
	$(ROMFSINST) -e CONFIG_USER_TFTP_HPA \
	   	-a "tftp    dgram  udp wait   root /bin/tftpd -c" /etc/inetd.conf
