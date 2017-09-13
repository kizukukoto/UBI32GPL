#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include "nvram.h"
#include "shutils.h"
#include "cmds.h"

#if 0
extern int dhcpd_main(int argc, char *argv[]);
extern int ntp_main(int argc, char *argv[]);
extern int upnp_main(int argc, char *argv[]);
extern int igmp_main(int argc, char *argv[]);
extern int ddns_main(int argc, char *argv[]);
extern int syslogd_main(int argc, char *argv[]);
extern int klogd_main(int argc, char *argv[]);
extern int httpd_main(int argc, char *argv[]);
#endif
extern int https_main(int argc, char *argv[]);
extern int dhcprelay_main(int argc, char *argv[]);
#if 0
extern int ipsec_main(int argc, char *argv[]);
extern int xl2tpd_main(int argc, char *argv[]);
extern int https_main(int argc, char *argv[]);
extern int tftpd_main(int argc, char *argv[]);
//extern int static_route_main(int argc, char *argv[]);
extern int route_main(int argc, char *argv[]);
/* jimmy added for dns relay */
extern int dnsmasq_main(int argc, char *argv[]);
/* ------------------------------- */
extern int pptpd_main(int argc, char *argv[]);
extern int usbd_main(int argc, char *argv[]);
extern int qosd_main(int argc, char *argv[]);
extern int webfilter_main(int argc, char *argv[]);
/* Albert Chen add for wps daemon*/
extern int wps_main(int argc, char *argv[]);
/*KenWu add for policy route*/
extern int policy_route_main(int argc, char *argv[]);
extern int ips_main(int argc, char *argv[]);
/* 2009-1-13 fred add */
extern int ii_main(int argc, char *argv[]);
extern int inbound_filter_main(int argc, char *argv[]);
#endif
static struct subcmd services_cmds[] = {
#if 0
	{ "ips", ips_main},     /* 2009-1-13 fred add */
	{ "ddns", ddns_main},
	{ "dnsmasq", dnsmasq_main},
	{ "dhcpd", dhcpd_main},
	{ "httpd", httpd_main},
#endif
	{ "https", https_main },
	{ "dhcprelay", dhcprelay_main },
#if 0
	{ "igmp", igmp_main},
	{ "ipsec", ipsec_main},
	{ "klogd", klogd_main},
	{ "ntp", ntp_main},
	{ "pptpd", pptpd_main},
	{ "route", route_main},
	{ "syslogd", syslogd_main},
	{ "tftpd", tftpd_main},
	{ "upnp", upnp_main},
	{ "xl2tpd", xl2tpd_main},
	{ "usb", usbd_main},
	{ "qos", qosd_main},
	{ "webfilter", webfilter_main},
/*2008-12-19 albert add for wps*/
	{ "wps", wps_main},	
/*KenWu add for policy route*/
	{ "policy_route", policy_route_main},
	/* jimmy added for dns relay */
	/* ------------------------------- */
	{ "netdevice", ii_main, "call cli ii <ACTION> <ALIAS>"}, /* net/ii.c */
	{ "inbound_filter", inbound_filter_main},
#endif
	{ NULL, NULL}
};
struct subcmd *get_services_cmds()
{
	return services_cmds;
}
int services_subcmd(int argc, char *argv[])
{
	return lookup_subcmd_then_callout(argc, argv, services_cmds);
}
