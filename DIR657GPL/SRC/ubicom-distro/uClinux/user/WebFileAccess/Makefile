all: lighttpd cgi

lighttpd: lighttpd-1.4.28
	
cgi:
	$(MAKE) -C cgi-ws
romfs:
	$(ROMFSINST) lighttpd-1.4.28/src/lighttpd /bin/lighttpd
	$(ROMFSINST) lighttpd-1.4.28/src/lighttpd-angel /bin/lighttpd-angel
	$(ROMFSINST) lighttpd-1.4.28/src/.libs/ /lib/
	$(ROMFSINST) webfile_access/ /www/webfile_access/
	$(ROMFSINST) cgi-ws/webfile_cgi.cgi /www/webfile_access/webfile_cgi.cgi
	$(ROMFSINST) rootfs-ws/etc/ /etc/	
	ln -sf /var/tmp/lighttpd-wa/lighttpd.conf $(ROMFSDIR)/etc/lighttpd.conf 
	ln -sf /var/tmp/lighttpd-wa/lighttpd.user $(ROMFSDIR)/etc/lighttpd-ws.user
	ln -sf /var/tmp/lighttpd-wa/auth.conf $(ROMFSDIR)/etc/conf.d/auth.conf             
clean:
	$(MAKE) -C lighttpd-1.4.28 $@; \
	$(MAKE) -C cgi-ws $@	
lighttpd-1.4.28/Makefile:
	cd lighttpd-1.4.28 ; \
	./configure --host=mips-linux --target=mips-linux --without-zlib --without-bzip2 --with-libpcre-includes=$(ROOTDIR)/lib/pcre-7.8/builddir --with-libpcre-libraries=$(ROOTDIR)/lib/pcre-7.8/.libs --with-openssl=$(ROOTDIR)/lib/libssl/ --with-openssl-libs=$(ROOTDIR)/lib/libssl/

lighttpd-1.4.28: lighttpd-1.4.28/Makefile
	$(MAKE) -C lighttpd-1.4.28

.PHONY: lighttpd-1.4.28
	
