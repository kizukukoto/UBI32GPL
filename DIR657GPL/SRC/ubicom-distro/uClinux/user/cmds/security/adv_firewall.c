#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <net/if_arp.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "cmds.h"
#include "shutils.h"
#include "nvram_ext.h"

int
init_iprane_argv(char **argv_cmd, char *ip1, char *ip2, char iprange_cmds[8][64])
{
	//char buf[1024];
	char *n1, *n2;
	int nCount = 0, i=0;

	//memset(buf, 0, sizeof(buf));
        n1 = index(ip1, '-');
        n2 = index(ip2, '-');

        if(n1==0){
		strcpy(iprange_cmds[nCount], "-s");
		nCount ++;
        } else {
       		strcpy(iprange_cmds[nCount], "-m");
		nCount ++;
		strcpy(iprange_cmds[nCount], "iprange");
		nCount ++;
		strcpy(iprange_cmds[nCount], "--src-range");
		nCount ++;
	}
	strcpy(iprange_cmds[nCount], ip1);
	nCount ++;

        if(n2==0){
                strcpy(iprange_cmds[nCount], "-d");
		nCount ++;
        } else {
                if(n1==0) {
                       	strcpy(iprange_cmds[nCount], "-m");
			nCount ++;
			strcpy(iprange_cmds[nCount], "iprange");
			nCount ++;
                }
		strcpy(iprange_cmds[nCount], "--dst-range");
		nCount ++;
        }
	strcpy(iprange_cmds[nCount], ip2);
	nCount ++;
	
	for(i=0; i<nCount; i++){
		//strcat(buf, iprange_cmds[i]);
		*argv_cmd = iprange_cmds[i];
		*argv_cmd++;
	}
        //cprintf("buf=[%s]\n", buf);
	return nCount;
}

char *
fw_paser_common(
	char *s, char **en,
	char **dev1, char **dev2,
	char **name, char **sched)
{
	char *p, *r = NULL;

	if ((p = strsep(&s, ";")) == NULL)
		goto out;
	if (en != NULL)
		*en = p;
	
	if ((p = strsep(&s, ">")) == NULL)
		goto out;
	if (dev1 != NULL)
		*dev1 = p;
	
	if ((p = strsep(&s, ";")) == NULL)
		goto out;
	if (dev2 != NULL)
		*dev2 = p;
	
	if ((p = strsep(&s, ";")) == NULL)
		goto out;
	if (name != NULL)
		*name = p;
	
	if ((p = strsep(&s, ";")) == NULL)
		goto out;
	if (sched != NULL)
		*sched = p;

	r = s;
out:
	return r;
}
/* strip "33:33" to "33" */
static char *strip_same_ports(const char *mports, char *out)
{
	char *_p;
	if ((_p = strchr(mports, ':')) == NULL) {
		return strcpy(out, mports);
	}
	/* rugly check the same port range ex: "22:22" */
	if (strncmp(mports, _p + 1, strlen(_p + 1)) == 0) {
		strncpy(out, mports, _p - mports);
		out[_p - mports] = '\0';
		return out;
	}
	return strcpy(out, mports);
}

static void __fw_firewall_updown2(
	int up,
	const char *dev1,
	const char *dev2,
	const char *ip1,
	const char *ip2,
	const char *proto,
	const char *ports,
	const char *sched,
	const char *act)
{
	char buf[1024], _ports[128];
	char *argv[48] = {
		"iptables",
		up ? "-I" : "-D",
		"FORWARD",
		"-m",
		"state",
		"--state",
		"NEW",
	};
	char time_cmd[][64] = {
		"-m",
		"time",
		"--timestart",
		"HH:MM:SS",
		"--timestop",
		"HH:MM:SS",
		"--days",
		"",
		""
	};
	char iprange_cmds[8][64];
	int argc = 7, i, offset = 0;
	
	if (strcasecmp(proto, "both") == 0) {
		__fw_firewall_updown2(up, dev1, dev2,
			ip1, ip2, "tcp", ports, sched, act);
		__fw_firewall_updown2(up, dev1, dev2,
			ip1, ip2, "udp", ports, sched, act);
		return;
	}
	
	strip_same_ports(ports, _ports);

	if (*dev1 != '*') {
		argv[argc++] = "-i";
		argv[argc++] = dev1;
	}
	if (*dev2 != '*') {
		argv[argc++] = "-o";
		argv[argc++] = dev2;
	}
	if (strcasecmp(proto, "all") != 0) {
		argv[argc++] = "-p";
		argv[argc++] = proto;
		
		if (strcasecmp(proto, "tcp") == 0 ||
		    strcasecmp(proto, "udp") == 0)
		{
			argv[argc++] = "-m";
			argv[argc++] = "mport";
			argv[argc++] = "--dports";
			argv[argc++] = _ports;
		}
	}
	
	i = init_iprane_argv(argv + argc, ip1, ip2, iprange_cmds);
	argc = argc + i;

	argv[argc] = NULL;
	if (!(*sched < '0' || '9' < *sched)) {
		/*
		 * @alg_append will format schedule_rule to @time_cmd
		 * then APPEND to @prefix_cmd
		 * */
		fprintf(stderr, "yes to schedule :%s\n", sched);
		alg_append_sched(argv, atoi(sched), time_cmd);
	}
	while (argv[argc] != NULL) argc++;
	
	argv[argc++] = "-j";
	argv[argc++] = *act == '1' ? "ACCEPT" : "DROP";
	argv[argc] = NULL;
	
	_eval(argv, ">/dev/console", 0, NULL);
	for (i = 0; i < argc;i++) {
		offset += sprintf(buf + offset, "%s ", argv[i]);
	}
	fprintf(stderr, "***[%s]***\n", buf);
	//cprintf("\nfirewall cmds=[%s]\n", buf);
}

int __fw_firewall_updown(int up, const char *dev, char *raw, int index)
{
	char *dev1, *realdev1, *ip1;
	char *dev2, *realdev2, *ip2;
	char *proto, *ports, *act, *sched;
	int rev = -1;
	
	/* "1;LAN>WAN_primary;my_fw;always;192.168.0.3>168.95.1.1;tcp;80;0" */
	if ((raw = fw_paser_common(raw, NULL, &dev1, &dev2, NULL, &sched)) == NULL)
		goto fmt_err;

/*	
	if ((realdev1 = __nvram_map("if_alias", dev1, "if_dev")) == NULL)
		realdev1 = "*";
		//goto no_dev;
	
	if ((realdev2 = __nvram_map("if_alias", dev2, "if_dev")) == NULL)
		realdev2 = "*";
		//goto no_dev;
*/
#if 0
	/* make sure two devices are up */
	if (get_ip(realdev2, g1) == -1) goto out;
	if (get_ip(realdev1, g2) == -1) goto out;
#endif	
	if ((ip1 = strsep(&raw, ">")) == NULL) goto fmt_err;
	if ((ip2 = strsep(&raw, ";")) == NULL) goto fmt_err;
	if ((proto = strsep(&raw, ";")) == NULL) goto fmt_err;
	if ((ports = strsep(&raw, ";")) == NULL) goto fmt_err;
	if ((act = strsep(&raw, ";")) == NULL) goto fmt_err;
#if 0
	__fw_firewall_updown2(up, dev1, dev2, ip1, ip2, proto, ports, sched, act);
	return 0;
#endif
	{
	char *vdev1[6], *vdev2[6], b1[128], b2[128];
	int v1, v2, i1, i2;

	v1 = vdev_get_group(dev1, vdev1, b1);
	v2 = vdev_get_group(dev2, vdev2, b2);
	if (v1 == -1) {
		vdev1[0] = dev1;
		v1 = 1;
	}
	if (v2 == -1) {
		vdev2[0] = dev2;
		v2 = 1;
	}
	for (i1 = 0; i1 < v1; i1++) {
		if ((realdev1 = __nvram_map("if_alias", vdev1[i1], "if_dev")) == NULL)
			realdev1 = "*";
			//goto no_dev;
		for (i2 = 0;i2 < v2; i2++) {
			if ((realdev2 = __nvram_map("if_alias", vdev2[i2], "if_dev")) == NULL)
				realdev2 = "*";
				//goto no_dev;
			printf("####### %s devices1:[%s], %s devices2:[%s]####\n",
					vdev1[i1],realdev1, vdev2[i2], realdev2);
			__fw_firewall_updown2(up, realdev1, realdev2,
				ip1, ip2, proto, ports, sched, act);

		}
	}
	}
	return 0;

no_dev:
out:
	return rev;
fmt_err:
	rd(rev, "Format error");
	return rev;
}

/*
 * common head:
 *  @ENABLE;@DEV1>@DEV2;@NAME;@SCHEDULE;
 *
 * firewall head:
 *  IP1>IP2;PROTO;PORTS;ACTION
 * 
 * MAC:
 *  MAC
 *
 * */
#define FIREWALL_MAX (300 - 1)
#define FIREWALL_RULES_PREFIX	"firewall_rule"

int fw_firewall_updown(int up, const char *dev)
{
	char *p, buf[512], *if_alias, *t;
	int i, rev = -1;

	for (i = FIREWALL_MAX;  i >= 0; i--) {
		if ((p = nvram_get_i(FIREWALL_RULES_PREFIX, i, g1)) == NULL)
			continue;
		if (*p != '1')	// @p = '\0' or '1' or '0'
			continue;
		
		p = strlen(p) > sizeof(buf) ? strdup(p) : strcpy(buf, p);
		fprintf(stderr, "#### rules:%d: %s######\n", i, p);
		__fw_firewall_updown(up, dev, p, i);
		if (p != buf)
			free(p);
	}
	rev = 0;
out:
	return rev;
}
#if 0	//depricate
static int __fw_firewall_stop(const char *dev, char *s, int index)
{
	char *filter, *p;
	int rev = -1;
	
	if ((p = strsep(&s, ";")) == NULL) goto out;
	
	if (strstr(p, dev) == NULL)
		goto out;
	
	if ((filter = strsep(&s, ";")) == NULL) goto out;

	system(filter);
	nvram_unset_i("adv_firewall_stop", index, g1);
	rev = 0;
out:
	rd(rev, "error");
	return rev;
}

int fw_firewall_stop(const char *dev)
{
	char *p, buf[512], *if_alias, *t;
	int i, rev = -1;
	
	//for (i = 0; i <= FIREWALL_MAX; i++) {
	for (i = FIREWALL_MAX; i <= 0; i--) {
		if ((p = nvram_get_i("adv_firewall_stop", i, g1)) == NULL)
			continue;
		if (strstr(p, dev) == NULL)
			continue;
		p = strlen(p) > sizeof(buf) ? strdup(p) : strcpy(buf, p);
		//__fw_firewall_stop(dev, p, i);
		//__fw_firewall_stop(dev, p, i);
		if (p != buf)
			free(p);
	}
out:
	return rev;
}
#endif 
int firewall_stop_main(int argc, char *argv[])
{
	return fw_firewall_updown(0, argv[1]);
}

int firewall_start_main(int argc, char *argv[])
{
	return fw_firewall_updown(1, argv[1]);
}

int adv_firewall_main(int argc, char *argv[])
{
	struct subcmd cmds[] = {
		{ "start", firewall_start_main},	//@dev
		{ "stop", firewall_stop_main},		//@dev
		{ NULL, NULL}
	};
	return lookup_subcmd_then_callout(argc, argv, cmds);
}
