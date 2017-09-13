ifndef SRCBASE	# DIR-130/330 don't need
SUBDIR = hnap
endif
BUILTIN_OBJS=$(addsuffix /built-in.o, $(SUBDIR))

SOURCES= clone.o stat_info.o firmware_info.o dynamicdhcp.o staticdhcp.o wl_sta_list.o get_trx.o cgi_tool.o log_page.o active.o delet_vpn.o firmw_check.o socket.o xp.o file_exist.o user_behavior.o os_version.o checksum.o get_os_date.o get_wl_channel.o list_url.o get_wl_domain.o graphlib.o graph.o get_wl_country.o get_usb_status.o auth_mech.o auth_local.o ipsec_dns.o lang.o cgi_tool_main.o

ifeq ($(CONFIG_MODEL_NAME), DIR-665_A1)
SOURCES += artblock_info.o do_config.o system_time.o ajax.o  get_ddns_status.o get_log_page.o get_wireless_list.o get_route_table.o statistics.o libsession.o internet_session.o wan_conn.o cmo_get_list.o tc.o check_wps_status.o ipv6.o get_wps_pin.o online_fw_check.o ca_read_file.o
include Makefile.DIR665
endif
ifeq ($(CONFIG_VENDOR), Ubicom)
SOURCES += artblock_info.o do_config.o system_time.o ajax.o  get_ddns_status.o get_log_page.o get_wireless_list.o get_route_table.o statistics.o libsession.o internet_session.o wan_conn.o cmo_get_list.o tc.o check_wps_status.o ipv6.o get_wps_pin.o online_fw_check.o ca_read_file.o
include Makefile.DIR665
endif


all: indep $(SUBDIR) $(SOURCES)
	$(CC) $(CFLAGS) $(LDFLAGS) -o cgi $(SOURCES) $(BUILTIN_OBJS) -lnvram $(CGILIB)

$(SUBDIR):
	$(MAKE) -C $@

clean: uindep
	@for i in $(SUBDIR); do make -C $$i $@; done
	$(RM) cgi *\.o
	$(RM) firmw_check

distclean: clean
	$(RM) *~

install:
	[ -d $(INSTALLDIR)/bin ] || mkdir -p $(INSTALLDIR)/bin
	cp cgi $(INSTALLDIR)/bin
ifeq ($(CONFIG_VENDOR), Ubicom)
	cp cert_verify $(INSTALLDIR)/sbin
endif
	-$(STRIP) $(INSTALLDIR)/bin/*
	cp log_awk $(INSTALLDIR)/bin
	cp vpn_st  $(INSTALLDIR)/bin
	cd $(INSTALLDIR)/bin && ln -sf ../tmp/vpn_st vpn_stat
	cd $(INSTALLDIR)/bin && ln -sf cgi vpn
	cd $(INSTALLDIR)/bin && ln -sf cgi schedule
	cd $(INSTALLDIR)/bin && ln -sf cgi strip_cmd
	cd $(INSTALLDIR)/bin && ln -sf cgi active
	cd $(INSTALLDIR)/bin && ln -sf cgi clone
	cd $(INSTALLDIR)/bin && ln -sf cgi delete_vpn
	cd $(INSTALLDIR)/bin && ln -sf cgi dynamicdhcp
	cd $(INSTALLDIR)/bin && ln -sf cgi firmware_info
	cd $(INSTALLDIR)/bin && ln -sf cgi firmw_check
	cd $(INSTALLDIR)/bin && ln -sf cgi get_trx
	cd $(INSTALLDIR)/bin && ln -sf cgi log_page
	cd $(INSTALLDIR)/bin && ln -sf cgi staticdhcp
	cd $(INSTALLDIR)/bin && ln -sf cgi stat_info
	cd $(INSTALLDIR)/bin && ln -sf cgi wl_sta_list
	cd $(INSTALLDIR)/bin && ln -sf cgi get_time
	cd $(INSTALLDIR)/bin && ln -sf cgi ca_inused
	cd $(INSTALLDIR)/bin && ln -sf cgi group_inused
	cd $(INSTALLDIR)/bin && ln -sf cgi http_login
	cd $(INSTALLDIR)/bin && ln -sf cgi vpn_ip
	cd $(INSTALLDIR)/bin && ln -sf cgi firmupdate
	cd $(INSTALLDIR)/bin && ln -sf cgi file_exist
	cd $(INSTALLDIR)/bin && ln -sf cgi user_behavior
	cd $(INSTALLDIR)/bin && ln -sf cgi get_os_version
	cd $(INSTALLDIR)/bin && ln -sf cgi checksum
	cd $(INSTALLDIR)/bin && ln -sf cgi get_os_date
	cd $(INSTALLDIR)/bin && ln -sf cgi get_wl_channel
	cd $(INSTALLDIR)/bin && ln -sf cgi list_url
	cd $(INSTALLDIR)/bin && ln -sf cgi get_wl_domain
	cd $(INSTALLDIR)/bin && ln -sf cgi graph_auth
	cd $(INSTALLDIR)/bin && ln -sf cgi get_wl_country
	cd $(INSTALLDIR)/bin && ln -sf cgi get_usb_status
	cd $(INSTALLDIR)/bin && ln -sf cgi auth_mech
	cd $(INSTALLDIR)/bin && ln -sf cgi auth_local
	cd $(INSTALLDIR)/bin && ln -sf cgi socket_sslvpn
	cd $(INSTALLDIR)/bin && ln -sf cgi ipsec_dns
	cd $(INSTALLDIR)/bin && ln -sf cgi lang
	cd $(INSTALLDIR)/bin && ln -sf cgi get_wireless_list
	cd $(INSTALLDIR)/bin && ln -sf cgi get_route_table
	cd $(INSTALLDIR)/bin && ln -sf cgi get_log_page
	cd $(INSTALLDIR)/bin && ln -sf cgi internet_session
	cd $(INSTALLDIR)/bin && ln -sf cgi statistics
	cd $(INSTALLDIR)/bin && ln -sf cgi system_time
ifndef SRCBASE
	cd $(INSTALLDIR)/bin && ln -sf cgi wan_conn
	cd $(INSTALLDIR)/bin && ln -sf cgi ajax
	cd $(INSTALLDIR)/bin && ln -sf cgi hnap_main
	cd $(INSTALLDIR)/bin && ln -sf cgi cmo_get_list
	cd $(INSTALLDIR)/bin && ln -sf cgi get_ddns_status
	cd $(INSTALLDIR)/bin && ln -sf cgi get_artblock
	cd $(INSTALLDIR)/bin && ln -sf cgi tc_measure
	cd $(INSTALLDIR)/bin && ln -sf cgi check_wps
	cd $(INSTALLDIR)/bin && ln -sf cgi get_wps_pin
	cd $(INSTALLDIR)/bin && ln -sf cgi online_firmware_check
	cd $(INSTALLDIR)/bin && ln -sf cgi download_fw_lp
	cd $(INSTALLDIR)/bin && ln -sf cgi ipv6
	cd $(INSTALLDIR)/bin && ln -sf cgi ca_read_file
endif
#       cd $(INSTALLDIR)/bin && ln -sf cgi vpn_exist
#       #for link in $(wildcard $(patsubst %.c,%,$(SOURCES))) ; do \
                (cd $(INSTALLDIR)/bin && ln -sf cgi ${link}) \
        done

.PHONY: all indep uindep clean distclean install $(SUBDIR)
