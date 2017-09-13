#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include "cmds.h"
#include <syslog.h>
#include <signal.h>
#include <errno.h>
#include "nvram.h"
#include "shutils.h"
#include "interface.h"
#include "proto.h"
#include "convert.h"
#include "libcameo.h"
#include "cmds.h"

struct zone_convert_struct zc_ppp[] = {
	{"Z_PROTO", NULL, "pppoe", ZF_COPY_PRIV, NULL},
	{"Z_DEVICE","IFNAME", NULL, 0, NULL},
	{"Z_PHY_DEVICE","DEVICE", NULL, 0, NULL},
	{"Z_LOCALIP", "IPLOCAL", NULL, 0, NULL},
	{"Z_LOCALMASK", NULL, "255.255.255.255", ZF_COPY_PRIV, NULL},
	{"Z_REMOTEIP", "IPREMOTE", NULL},
	{"Z_DNS1", "DNS1", NULL},
	{"Z_DNS2", "DNS2", NULL},
	{"Z_GATEWAY", "Z_REMOTEIP", NULL},
	{"Z_PID", "PPPD_PID", NULL},
	{NULL, NULL, NULL},
};

int __ppp_convert()
{
	convert_env(zc_ppp);
	return system("set >/tmp/cmds.cnv");
}

#define PPP_PID_DIR	"/var/run/"

pid_t __ppp_get_pid(const char *dev)
{
	return get_pid_from_pidfile("%s%s.pid", PPP_PID_DIR, dev);
}
int kill_pppd_from_pidfile(int sig, const char *dev)
{
	return kill_from_pidfile(sig, "%s%s.pid", PPP_PID_DIR, dev);
}

void route_add_host_if_need(
	const char *ifname,
	const char *local, const char *peer,
	const char *mask, const char *gw)
{
	if (ifname == NULL || local == NULL || peer == NULL ||
		mask == NULL|| gw == NULL)
	{
		return;
	}
	if (same_subnet(local, peer, mask) == 1)
		return;

	route_add(ifname, 1, peer, gw, "255.255.255.255");
	return;
}
int
pptp_l2tp_attach_dns(const char *pdev, const char *phy, const char *remote)
{
	int rev = -1;
	char pdev_gw[16], pdev_ip[16], pdev_mask[16];
	char phy_dev[16], phy_ip[16], phy_mask[16];
	char _pdev[16];

	if (!remote || strlen(remote) == 0)
		return rev;

	dns_append(remote);
	debug( "ppp: %s, phy: %s, remote: %s\n", pdev, phy, remote);

	/* if remote (DNS IP) in the same network with LAN, then... return */
	if (get_ip(phy, phy_ip) == -1)
		return rev;
	if (get_netmask(phy, phy_mask) == -1)
		return rev;
	if (same_subnet(phy_ip, remote, phy_mask))
		return rev;

	/* add remote (DNS IP) into routing table which using pppx interface go out */
	if (get_ip(pdev, pdev_ip) == -1)
		return rev;
	if (get_netmask(pdev, pdev_mask) == -1)
		return rev;
	if (same_subnet(pdev_ip, remote, pdev_mask))
		return rev;

	//getGatewayInterfaceByIP(remote, _pdev, pdev_gw);
	route_add_host_if_need(pdev, pdev_ip, remote, pdev_mask, getenv("IPREMOTE"));

	rev = 0;
	return rev;
}
/* Handle Rrouting/DNS issues for PPTP/L2TP including RU mode */
int pppxxx_route_update(const char *phy, const char *remote, const char *proto, int idx)
{
	int rev = -1;
	char dev[16], dev_gw[16], dev_ip[16], dev_mask[16], def_gw[INET_ADDRSTRLEN];
	char *p;
	/* 
	 * 1. delete default gw.
	 * 2. get local ip, remote ip,
	 * 3. add route if not in the same subnet.
	 * */
	printf("set gateways ...phy:%s, remote:%s, proto:%s, index:%d\n",
			phy, remote, proto, idx);
	
	/* save phy default gw to nvram or get from nvarm */
	if (get_def_gw(def_gw) == 0) {
		printf("delete default gw:%s\n", def_gw);
		route_del(phy, 0, "0.0.0.0", NULL, "0.0.0.0");
		nvram_set_i(strcat_r(proto, "_phy_gw", g1), idx, g2, def_gw);
	} else if ((p = nvram_get_i(strcat_r(proto, "_phy_gw", g1), idx, g2)) != NULL)
		strncpy(def_gw, p, sizeof(def_gw));
	else
		debug( "no default route\n");

	setenv1("DEFAULT_ROUTE", def_gw);

	if (get_ip(phy, dev_ip) == -1)
		return rev;
	if (get_netmask(phy, dev_mask) == -1)
		return rev;
	printf("PHYINFO :%s/%s\n", dev_ip, dev_mask);
	if (same_subnet(dev_ip, remote, dev_mask))
		goto out;
	fprintf(stderr, "set gw :%s/%s\n", dev_ip, dev_mask);
	/* This LIB have bugs */
	//getGatewayInterfaceByIP(remote, dev, dev_gw);
	fprintf(stderr, "get gw :%s/%s\n", dev, dev_gw);
	route_add_host_if_need(phy, dev_ip, remote, dev_mask, def_gw);
	fprintf(stderr, "XXXXXset gw :%s/%s\n", dev_ip, dev_mask);
	/* Routing DNS */
	{
		char b[512], *p, *s;
		int i;
		
		s = b;
		fprintf(stderr, "Start DNS parser\n");
		if ((i = dns_get(s)) <= 0)
			goto out;
		fprintf(stderr, "Start DNS parsear:%s return %d\n", s, i);

		p = strsep(&s, " ");
		
		fprintf(stderr, "Set:%s\n", p);
		route_add_host_if_need(phy, dev_ip, p, dev_mask, def_gw);
		if ((p = strsep(&s, " ")) != NULL) {
			route_add_host_if_need(phy, dev_ip, p, dev_mask, def_gw);
		}
	}
out:
	rev = 0;
	return rev;
}
/* @proto="pptp"|"l2tp" */
int pptp_l2tp_attach(const char *proto, int argc, char *argv[])
{
	int rev = -1, i;
	char *alias, *dev;

	__ppp_convert();
	if (get_std_args(argc, argv, &alias, &dev, NULL) == -1)
		goto out;
	
	if ((i = nvram_find_index(strcat_r(proto, "_alias", g1), alias)) == -1)
		goto out;
	
	// argv[3] is phy, argv[2] is pppx
	pptp_l2tp_attach_dns(argv[2], argv[3], getenv("Z_DNS1"));
	pptp_l2tp_attach_dns(argv[2], argv[3], getenv("Z_DNS2"));

	/* dial on demand will go through here */
	nvram_set(strcat_i(strcat_r(proto, "_pid", g1), i, g2), getenv("Z_PID"));
	
	nvram_set_i(strcat_r(proto, "_status", g2), i, g1, "connected");
	eval("iptables", "-I", "FORWARD", "-p", "tcp", "--tcp-flags", "SYN,RST", "SYN",
		"--jump", "TCPMSS", "--clamp-mss-to-pmtu");
	rev = 0;
out:
	rd(rev, "%s error", proto);
	return rev;
}

int pppxxx_attach_main(const char *proto, int argc, char *argv[])
{
	int rev = -1, i;
	char *alias, *dev;

	__ppp_convert();
	if (get_std_args(argc, argv, &alias, &dev, NULL) == -1)
		goto out;
	
	if ((i = nvram_find_index(strcat_r(proto, "_alias", g1), alias)) == -1)
		goto out;
		
	if (strcmp(proto, PROTO_PPTP_STR) == 0 || 
	    strcmp(proto, PROTO_L2TP_STR) == 0)
	{
		// argv[3] is phy, argv[2] is pppx
		pptp_l2tp_attach_dns(argv[2], argv[3], getenv("Z_DNS1"));
		pptp_l2tp_attach_dns(argv[2], argv[3], getenv("Z_DNS2"));

		/* dial on demand will go through here */
		nvram_set(strcat_i(strcat_r(proto, "_pid", g1), i, g2), getenv("Z_PID"));
		/* FIXME*/
		eval("iptables", "-I", "FORWARD", "-p", "tcp", "--tcp-flags",
					"SYN,RST", "SYN", "--jump", "TCPMSS",
							"--clamp-mss-to-pmtu");
	} else if (strcmp(proto, PROTO_PPPOE_STR) == 0 || 
	    	   strcmp(proto, PROTO_USB3G_STR) == 0)
	{
		/* 7 = strlen("1.1.1.1") */
		if (strlen(nvram_safe_get_i(strcat_r(proto, "_primary_dns", g2), i, g1)) >= 7 ||
		    strlen(nvram_safe_get_i(strcat_r(proto,"_secondary_dns", g2), i, g1)) >= 7)
		{
			dns_delete(nvram_safe_get_i(strcat_r(proto, "_primary_dns", g2), i, g1));
			dns_append(nvram_safe_get_i(strcat_r(proto, "_primary_dns", g2), i, g1));
			dns_delete(nvram_safe_get_i(strcat_r(proto, "_secondary_dns", g2), i, g1));
			dns_append(nvram_safe_get_i(strcat_r(proto, "_secondary_dns", g2), i, g1));
		} else {
			//trace("pppoe attach_dns: %s, %s", getenv("Z_DNS1"), getenv("Z_DNS2"));
			char *dp;
			/* 7 = strlen("1.1.1.1") */
			if ((dp = getenv("Z_DNS1")) != NULL && strlen(dp) >= 7) {
				dns_delete(dp);
				dns_append(dp);
			}
			if ((dp = getenv("Z_DNS2")) != NULL && strlen(dp) >= 7) {
				dns_delete(dp);
				dns_append(dp);
			}
		}
		/*FIXME:*/
		eval("iptables", "-I", "INPUT", "-i", dev, "-j", "ACCEPT");
	} else {
		err_msg("unknow proto:%s attached!", proto);
		goto out;
	}
	nvram_set_i(strcat_r(proto, "_status", g2), i, g1, "connected");
	rev = 0;
out:
	rd(rev, "error");
	return rev;
}

int pppxxx_detach_main(const char *proto, int argc, char *argv[])
{
	int rev = -1, i;
	char *alias, *dev;
	
	system("set >/tmp/pppd.detach");
	__ppp_convert();
	if (get_std_args(argc, argv, &alias, &dev, NULL) == -1)
		goto out;
	
	if ((i = nvram_find_index(strcat_r(proto, "_alias", g1), alias)) == -1)
		goto out;

	if (strcmp(proto, PROTO_PPTP_STR) == 0 || 
	    strcmp(proto, PROTO_L2TP_STR) == 0)
	{
		/* FIXME:*/
		eval("iptables", "-D", "FORWARD", "-p", "tcp", "--tcp-flags",
					"SYN,RST", "SYN", "--jump", "TCPMSS",
							"--clamp-mss-to-pmtu");
		
	} else if (strcmp(proto, PROTO_PPPOE_STR) == 0 || 
	    	   strcmp(proto, PROTO_USB3G_STR) == 0)
	{
		/* FIXME:*/
		eval("iptables", "-D", "INPUT", "-i", dev, "-j", "ACCEPT");
	}
	nvram_set_i(strcat_r(proto, "_status", g2), i, g1, "disconnected");
	rev = 0;
out:
	rd(rev, "error");
	return rev;
}
/* @proto="pppoe"|"usb3g" */
int pppoe_usb3g_attach_main(const char *proto, int argc, char *argv[])
{
	int rev = -1, i;
	char *alias, *dev;
	char buf[128];

	__ppp_convert();
	if (get_std_args(argc, argv, &alias, &dev, NULL) == -1)
		goto out;
	
	if ((i = nvram_find_index(strcat_r(proto, "_alias", g1), alias)) == -1)
		goto out;
	/* 7 = strlen("1.1.1.1") */
	if (strlen(nvram_safe_get_i(strcat_r(proto, "_primary_dns", g2), i, g1)) >= 7 ||
	    strlen(nvram_safe_get_i(strcat_r(proto,"_secondary_dns", g2), i, g1)) >= 7)
	{
		dns_delete(nvram_safe_get_i(strcat_r(proto, "_primary_dns", g2), i, g1));
		dns_append(nvram_safe_get_i(strcat_r(proto, "_primary_dns", g2), i, g1));
		dns_delete(nvram_safe_get_i(strcat_r(proto, "_secondary_dns", g2), i, g1));
		dns_append(nvram_safe_get_i(strcat_r(proto, "_secondary_dns", g2), i, g1));
	} else {
		//trace("pppoe attach_dns: %s, %s", getenv("Z_DNS1"), getenv("Z_DNS2"));
		char *dp;
		/* 7 = strlen("1.1.1.1") */
		//trace("XXX Z_DNS1: %s, Z_DNS2: %s XXXX", getenv("Z_DNS1"), getenv("Z_DNS2"));
		if ((dp = getenv("Z_DNS1")) != NULL && strlen(dp) >= 7) {
			dns_delete(dp);
			dns_append(dp);
		}
		if ((dp = getenv("Z_DNS2")) != NULL && strlen(dp) >= 7) {
			dns_delete(dp);
			dns_append(dp);
		}
	}
	nvram_set_i(strcat_r(proto, "_status", g2), i, g1, "connected");
	/* XXX: ? */
	eval("iptables", "-I", "INPUT", "-i", dev, "-j", "ACCEPT");
	rev = 0;
out:
	return rev;
}

int pptp_l2tp_detach_main(const char *proto, int argc, char *argv[])
{
	int rev = -1, i;
	char *alias, *dev;

	system("set >/tmp/pppd.detach");
	__ppp_convert();
	if (get_std_args(argc, argv, &alias, &dev, NULL) == -1)
		goto out;

	//debug( "%s, alias = %s, dev = %s\n", __FUNCTION__,alias,dev);
	if ((i = nvram_find_index(strcat_r(proto, "_alias", g1), alias)) == -1)
		goto out;
	
	nvram_set_i(strcat_r(proto,"_status", g2), i, g1, "disconnected");
	eval("iptables", "-D", "FORWARD", "-p", "tcp", "--tcp-flags", "SYN,RST", "SYN",
		"--jump", "TCPMSS", "--clamp-mss-to-pmtu");
	rev = 0;
out:
	rd(rev, "error");
	return rev;
}


int init_pppd_options(
	FILE *fd,
	struct proto_ppp *ppp,
	const char *ipparam,
	int unit)
{
	fprintf(fd,
		"noipdefault\n"
		"defaultroute\n"
		"passive\n"
		"debug\n"
		"refuse-eap\n"  /*Don't agree to authenticate to peer with EAP*/
		"lcp-echo-interval 30\n"
		"lcp-echo-failure 4\n"                               
		"%s\n"
		"maxfail 3\n"
		"user %s\n"
		"mtu  %s\n"
		"mru  %s\n"
		"noauth\n"      /*"Don't require peer to authenticate"*/
		"noaccomp\n"    /*Disable address/control compression*/
		"-am\n",         /*Disable asyncmap negotiation*/
		ppp->dns1 ? "" : "usepeerdns",
		ppp->user ,
		ppp->mru,
		ppp->mru
	);
	if (ipparam != NULL)
		fprintf(fd, "ipparam %s\n", ipparam);
	if (unit != -1)
		fprintf(fd, "unit %d\n", unit);
	
	switch (atoi(ppp->dial)) {
		case 0:				/* manual */
		case 1:				/* always */
			if (strncmp(ipparam, "l2tp", 4) == 0) {
				fprintf(fd, "nopersist\n");
				fprintf(fd, "connect /bin/ppp.connect\n");
				fprintf(fd, "disconnect /bin/ppp.disconnect\n");
			} else
				fprintf(fd, "persist\n");
			break;
		case 2:				/* on demand */
			fprintf(fd, "idle %s\n", ppp->idle);
			fprintf(fd, "demand\n");
			if (strncmp(ipparam, "l2tp", 4) == 0) {
				fprintf(fd, "nopersist\n");
				fprintf(fd, "connect /bin/ppp.connect\n");
				fprintf(fd, "disconnect /bin/ppp.disconnect\n");
			} else
				fprintf(fd,"persist\n"); /* pptp or PPPoE */
			fprintf(fd,"active-filter 'outbound and tcp or udp or icmp'\n");
			break;
		default:
			break;
	}
	if (strcmp(ppp->mppe, "require-mppe") == 0 ||*ppp->mppe == '1')
		fprintf(fd, "require-mppe\nnomppe-stateful\n");	/* force stateless mode */
	else
		fprintf(fd, "nopcomp\nnoccp\n");

	return 0;
}
#define CHAP_SECRETS			"/var/ppp/chap-secrets"
#define PAP_SECRETS				"/var/ppp/pap-secrets"
#define CHAP_REMOTE_NAME        "*"
int init_pap_secret(struct proto_ppp *ppp)
{
	init_file(PAP_SECRETS);			
	save2file(PAP_SECRETS, "\"%s\" * \"%s\" *\n", ppp->user, ppp->pass);
	chmod(PAP_SECRETS, 0777);
	return 0;
}

int init_chap_secret(struct proto_ppp *ppp)
{
	init_file(CHAP_SECRETS);       
	save2file(CHAP_SECRETS, "%s %s \"%s\" *\n", ppp->user, CHAP_REMOTE_NAME, ppp->pass);
	save2file(CHAP_SECRETS, "%s %s \"%s\" *\n", CHAP_REMOTE_NAME, ppp->user, ppp->pass);
	chmod(CHAP_SECRETS, 0777);
	return 0;
}
void set_ppp_env(struct proto_ppp *ppp, const char *proto, int index)
{
	setenv1("PPPD_PROTO", proto);
	setenv1("PPPD_INDEX", itoa(index));
	setenv1("PPPD_UNIT", ppp->unit);
	setenv1("PPPD_USER", ppp->user);
	setenv1("PPPD_PASS", ppp->pass);
	setenv1("PPPD_IDLE", ppp->idle);
	//setenv1("PPPD_IFNAME", phy);
	setenv1("PPPD_SERVICENAME", ppp->servicename);
	setenv1("PPPD_DNS1", ppp->dns1);
	setenv1("PPPD_DNS2", ppp->dns2);
	setenv1("PPPD_DAIL", ppp->dial);
	//setenv1("PPPD_MODE", ppp->mode);
}
