MODS=mod_access.so mod_cgi.so mod_indexfile.so mod_accesslog.so mod_dirlisting.so mod_staticfile.so mod_graph_auth.so mod_auth.so mod_expire.so mod_evasive.so mod_flv_streaming.so mod_alias.so mod_setenv.so

all: lighttpd md5 cgi

lighttpd: lighttpd-1.4.28
	
cgi:
	$(MAKE) -C cgi-ws

md5:
	$(MAKE) -C md5

romfs:
	$(ROMFSINST) lighttpd-1.4.28/src/lighttpd /bin/lighttpd
	$(ROMFSINST) lighttpd-1.4.28/src/lighttpd-angel /bin/lighttpd-angel
	for i in $(MODS) ; do \
		$(ROMFSINST) lighttpd-1.4.28/src/.libs/$$i /lib ; \
	done
	$(ROMFSINST) webfile_access/ /www/webfile_access/
	$(ROMFSINST) cgi-ws/webfile_cgi.cgi /www/webfile_access/webfile_cgi.cgi
	$(ROMFSINST) rootfs-ws/etc/ /etc/	
	ln -sf /var/tmp/lighttpd-wa/lighttpd.conf $(ROMFSDIR)/etc/lighttpd.conf 
	ln -sf /var/tmp/lighttpd-wa/lighttpd.user $(ROMFSDIR)/etc/lighttpd-ws.user
	ln -sf /var/tmp/lighttpd-wa/auth.conf $(ROMFSDIR)/etc/conf.d/auth.conf
	$(MAKE) -C md5 $@
  
clean:
	$(MAKE) -C lighttpd-1.4.28 $@
	rm -f $(ROMFSDIR)/bin/lighttpd ; \
	rm -f $(ROMFSDIR)/bin/lighttpd-angel ; \
	$(MAKE) -C cgi-ws $@	
	for i in $(MODS) ; do \
		rm -f $(ROMFSDIR)/lib/$$i ; \
	done
	rm -f $(ROMFSDIR)/www/webfile_access/webfile_cgi.cgi ; \
	$(MAKE) -C md5 $@	

lighttpd-1.4.28/Makefile:
	cd lighttpd-1.4.28 ; \
	./configure --host=mips-linux --target=mips-linux --without-zlib --without-bzip2 --with-libpcre-includes=$(ROOTDIR)/lib/pcre-7.8/builddir/include --with-libpcre-libraries=$(ROOTDIR)/lib/pcre-7.8/builddir/lib --with-openssl=$(ROOTDIR)/lib/libssl/ --with-openssl-libs=$(ROOTDIR)/lib/libssl/

lighttpd-1.4.28: lighttpd-1.4.28/Makefile
	$(MAKE) -C lighttpd-1.4.28

.PHONY: lighttpd-1.4.28 md5
	
