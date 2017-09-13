/*
 * PPTP Server configuration
 *
 * $Copyright (C) 2003 Broadcom Corporation$
 *
 * $Id: pptpd.c,v 1.3 2009/03/31 14:32:03 peter_pan Exp $
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <shutils.h>
/*
#include <nvpptp.h>
#include <bcmnvram.h>
*/
#include <nvramcmd.h>
#include "nvpptp.h"
#define uint unsigned int
#define uint8 unsigned char
#define uint32 unsigned long
#define uint64 unsigned long long

#define PPTPD_OPTION_FILE   "/tmp/ppp/options.pptpd"
#define PPTPD_CONF_FILE     "/tmp/ppp/pptpd.conf"
#define PPTPD_AUTHMETHOD    nvram_safe_get("pptpd_authmethod")
#define PPTPD_PLUGIN        (!strcmp(PPTPD_AUTHMETHOD, "DB"))?"plugin ppp-login.so\n":""

int write_pptpd_conf(void);
int write_pptpd_options_pptpd(void);
//int write_pptpd_secrets( void );
/*
 * Get name server ip from /tmp/resolv.conf
 */
char *vpn_dns(char *out)
{
	FILE *fd;
	char buf[256];
	int i = 0;

	if ((fd = fopen("/etc/resolv.conf", "r")) == NULL)
		return;

	while (fgets(buf, sizeof(buf), fd) && i < 2) {
		char *p;
		p = buf;
		printf("buf:%s\n", buf);
		if (strncmp(buf, "nameserver", strlen("nameserver")) != 0)
			continue;

		p += strlen("nameserver");
		while (*p == ' ' || *p == '\t')
			p++;
		//chop(p);
		if (strcmp(p, "0.0.0.0") == 0)
			continue;
		sprintf(out + strlen(out), "ms-dns %s", p);
		i++;
	}

	fclose(fd);
	return out;
}

/*
* create/recreate the options file in /etc/ppp
*/
int write_pptpd_conf(void)
{
	FILE *fd;
	pptpd_Conn_t pptpd_Conn;

	mkdir("/tmp/ppp", 0777);

	memset(&pptpd_Conn, 0x00, sizeof(pptpd_Conn_t));
	if (get_nvpptpd_Conn(&pptpd_Conn) == -1)
		return -1;	/* no parameters */

	if (!pptpd_Conn.enable)
		return -1;	// pptpd disabled

	/* Write to options  file based on current information */
	if (!(fd = fopen(PPTPD_CONF_FILE, "w"))) {
		perror(PPTPD_CONF_FILE);
		return -1;
	}

	fprintf(fd, "option %s\n" 
//             "logwtmp\n"
		"localip %s\n" "remoteip %s\n", PPTPD_OPTION_FILE, pptpd_Conn.localip, pptpd_Conn.remoteip);

	fclose(fd);

	return 0;
}


int write_pptpd_options_pptpd(void)
{
	FILE *fd;
	char tmp[128], buf[128], p[128];
	char *auth[] = { "pap", "chap", "mschap", "mschap-v2", NULL };
	int i = 0;

	/* Write to options  file based on current information */
	if (!(fd = fopen(PPTPD_OPTION_FILE, "w"))) {
		perror(PPTPD_OPTION_FILE);
		return -1;
	}

	tmp[0] = '\0';
	while (auth[i]) {
		if (nvram_invmatch("pptpd_auth_proto", auth[i])) {
			sprintf(p, "refuse-%s\n", auth[i]);
			strcat(tmp, p);
		}
		i++;
	}
	sprintf(p, "require-%s\n", nvram_safe_get("pptpd_auth_proto"));
	strcat(tmp, p);
	if (nvram_invmatch("pptpd_auth_proto", "pap") && nvram_invmatch("pptpd_auth_proto", "chap")) {
		if (NVRAM_MATCH("pptpd_encryption", "none"))
			sprintf(p, "");
		else if (NVRAM_MATCH("pptpd_encryption", "mppe-40"))
			sprintf(p, "require-mppe\nnomppe-128\nrequire-mppe-40\n");
		else
			sprintf(p, "require-mppe\nnomppe-40\nrequire-mppe-128\n");
		strcat(tmp, p);
	}

	buf[0] = '\0';
	fprintf(fd,
		"name %s\n"
		"%s" 
		"proxyarp\n" 
//		"debug\n" 
//		"dump\n" 
		"lock\n" 
		"nobsdcomp\n" 
		"novj\n" 
		"novjccomp\n" 
		"default-mru\n" 
		"unit 110\n" 
		"ipparam pptpd\n" 
		"%s" 
		"%s", 
		nvram_safe_get("pptpd_servername"), tmp, PPTPD_PLUGIN, vpn_dns(buf)
	    );
	fclose(fd);

	return 0;
}

static void pptpd_netfilter_updown(int up)
{
	char *act = up ? "-I" : "-D";

	//eval(up ? "rmmod" : "insmod", "/lib/modules/2.6.15/kernel/net/ipv4/netfilter/ip_nat_pptp.ko");
	eval("iptables", act, "INPUT", "--proto", "tcp", "--dport", "1723", "-j", "ACCEPT");
	eval("iptables", act, "INPUT", "-p", "47", "-j", "ACCEPT");
	eval("iptables", act, "FORWARD", "-i", "ppp+", "-j", "ACCEPT");

}

#include "cmds.h"
#include "shutils.h"
/*
 * Start PPTPD
 */
static int start_pptpd(void)
{
	int rc = 0;
	// create pptpd conf file
	if ((rc = write_pptpd_conf()) < 0) {
		trace("No pptpd wrote");
		goto out;
	}
	write_secrets();

	// create options_pptpd file
	if ((rc = write_pptpd_options_pptpd()) < 0) {
		err_msg("no pptpd_options wrote");
		goto out;
	}
	pptpd_netfilter_updown(1);
	eval("pptpd", "-i", "-c", "/tmp/ppp/pptpd.conf", "updetach");
	rc = 0;
out:
	return rc;
}

/*
 * Stop PPTPD
 */
static int stop_pptpd(void)
{
	pptpd_netfilter_updown(0);
	eval("killall", "pptpd");
	return 0;
}

/* start and stop main */
#include "cmds.h"
#include "shutils.h"
static int pptpd_start_main(int argc, char *argv[])
{
	start_pptpd();
}
static int pptpd_stop_main(int argc, char *argv[])
{
	stop_pptpd();
}
int pptpd_main(int argc, char *argv[])
{
	struct subcmd cmds[] = {
		{"start", pptpd_start_main,"start pptpd: rev1.3.4"},
		{"stop", pptpd_stop_main,"stop pptpd: rev1.3.4"},
		{NULL, NULL}
	};
	return lookup_subcmd_then_callout(argc, argv, cmds);
}
