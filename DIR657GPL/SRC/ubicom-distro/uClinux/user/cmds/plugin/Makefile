TARGET:= radclient.so slapd-bind.so

RAD_SRC=../../../../apps/freeradius-client-1.1.6
RAD_CFLAGS:= -I $(RAD_SRC)/ -I $(RAD_SRC)/include/
RAD_LDFLAGS:= $(RAD_SRC)/lib/.libs/libfreeradius-client.a

LDAP_SRC=../../../../apps/openldap-2.4.11
LDAP_CFLAGS:=-I $(LDAP_SRC)/include
LDAP_LDFLAGS:= \
$(LDAP_SRC)/libraries/libldap/.libs/libldap.a \
$(LDAP_SRC)/libraries/liblber/.libs/liblber.a \
$(LDAP_SRC)/libraries/liblber/.libs/liblber.a \
$(LDAP_SRC)/libraries/liblutil/liblutil.a -lresolv


all: $(TARGET)

radclient.o: radclient.c
	$(CC) $(RAD_CFLAGS) -c -o $@ $^
radclient.so: radclient.o
	$(CC) -shared -o $@ $^ $(RAD_LDFLAGS)


slapd-bind.o: slapd-bind.c
	$(CC) $(LDAP_CFLAGS) -c -o $@ $^
slapd-common.o: slapd-common.c
	$(CC) $(LDAP_CFLAGS) -c -o $@ $^
slapd-bind.so: slapd-bind.o slapd-common.o 
	$(CC) -shared -o $@ $^ $(LDAP_LDFLAGS)

install:
	install -d $(INSTALLDIR)/lib/cmds/
	cp $(TARGET) $(INSTALLDIR)/lib/cmds/
	-$(STRIP) $(INSTALLDIR)/lib/cmds/*.so

clean:
	@rm  -f *.o *.so
