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
#include "nvram.h"

#if 0
#define DEBUG_MSG	cprintf
#else
#define DEBUG_MSG
#endif

#if 0
typedef struct _netconf_match_t {
	int ipproto;                    /* IP protocol (TCP/UDP) */
	struct {
		struct in_addr ipaddr;  /* Match by IP address */
		struct in_addr netmask;
		uint16 ports[2];        /* Match by TCP/UDP port range */
	} src, dst;
	struct ether_addr mac;          /* Match by source MAC address */
	struct {
		char name[IFNAMSIZ];    /* Match by interface name */
	} in, out;
	int state;                      /* Match by packet state */
	int flags;                      /* Match flags */
	uint days[2];                   /* Match by day of the week (local time) (0 == Sunday) */
	uint secs[2];                   /* Match by time of day (local time) (0 == 12:00 AM) */
	struct _netconf_match_t *next, *prev;
} netconf_match_t;
#endif

/* example:
 * 	@ports := 21-24
 * 	@ports := 80-80
 *
 * output:
 * 	@buf := 21:24
 * 	@buf := 80
 * */
static void fw_forward_inbound_portformat(const char *ports, char *buf)
{
	char *p, *pstart, t[128];

	if (ports == NULL || *ports == '\0')
		return;
	if (strstr(ports, "-") == NULL)
		return;

	p = t;
	strcpy(t, ports);

	pstart = strsep(&p, "-");

	if (strcmp(pstart, p) == 0)
		strcpy(buf, pstart);
	else
		sprintf(buf, "%s:%s", pstart, p);
}


static int
__fw_forward_port_start2_no_proto(
	const char *dev1,
	const char *dev2,
	const char *ip1,
	const char *ip2,
	const char *proto,
	const char *ports1,
	const char *ports2,
	const char *sched,
	int index)
{
	char buf[512];

	/*eval("iptables", "-t", "nat", "-A","PREROUTING", "-i", dev1,
		"-d", ip1, "-m", "mport", "--dports", ports1, sched,
		"-j", "DNAT", "--to-destination", ip2, ports2);*/
	sprintf(buf, "iptables -t nat -A PREROUTING -i %s -d %s -m mport --dports %s %s -j DNAT --to-destination %s:%s",
			dev1, ip1, ports1, sched, ip2, ports2);
	system(buf);

	/*eval("iptables", "-t", "filter", "-A","FORWARD",
		"-i", dev1, "-d", ip2, "-m", "mport",
		"--dports", ports2, sched, "-j", "ACCEPT");*/
	sprintf(buf, "iptables -t filter -A FORWARD -i %s -d %s -m mport --dports %s %s -j ACCEPT",
				 dev1, ip2, proto, ports2, sched);
	system(buf);

	sprintf(buf, "%s>%s;iptables -t nat -D PREROUTING -i %s -d %s "
		       "-m mport --dports %s %s -j DNAT --to-destination %s:%s;"
		       "iptables -t filter -D FORWARD -i %s -d %s "
		       "-m mport --dports %s %s -j ACCEPT",
		       dev1, dev2,
		       dev1, ip1, ports1, sched, ip2, ports2,
		       dev1, ip2, ports2, sched);
	
	nvram_set(strcat_i("forward_port_stop", index, g1), buf);
}

static int
__fw_forward_port_start2_icmp(
	const char *dev1,
	const char *dev2,
	const char *ip1,
	const char *ip2,
	const char *proto,
	const char *ports1,
	const char *ports2,
	const char *sched,
	int index)
{
	char buf[512];

	/*eval("iptables", "-t", "nat", "-A","PREROUTING", "-i", dev1,
		"-d", ip1, "-p", proto, "-m", "mport", "--dports", ports1,
		sched, "-j", "DNAT", "--to-destination", ip2, ports2);*/
	sprintf(buf, "iptables -t nat -A PREROUTING -i %s -d %s -p %s -m mport --dports %s %s -j DNAT --to-destination %s:%s",
			dev1, ip1, proto, ports1, sched, ip2, ports2);
	system(buf);

	/*eval("iptables", "-t", "filter", "-A","FORWARD",
		"-i", dev1, "-d", ip2, "-p", proto, "-m", "mport",
		"--dports", ports2, sched, "-j", "ACCEPT");*/
	sprintf(buf, "iptables -t filter -A FORWARD -i %s -d %s -p %s -m mport --dports %s %s -j ACCEPT",
				 dev1, ip2, proto, ports2, sched);
	system(buf);

	sprintf(buf, "%s>%s;iptables -t nat -D PREROUTING -i %s -d %s -p %s "
		       "-m mport --dports %s %s -j DNAT --to-destination %s:%s;"
		       "iptables -t filter -D FORWARD -i %s -d %s -p %s "
		       "-m mport --dports %s %s -j ACCEPT",
		       dev1, dev2,
		       dev1, ip1, proto, ports1, sched, ip2, ports2,
		       dev1, ip2, proto, ports2, sched);
	
	nvram_set(strcat_i("forward_port_stop", index, g1), buf);
}

static int
__fw_forward_port_start2_tcp_udp(
	const char *dev1,
	const char *dev2,
	const char *ip1,
	const char *ip2,
	const char *proto,
	const char *ports1,
	const char *ports2,
	const char *sched,
	int index)
{
	char buf[512], portsb1[512], portsb2[512];

	/*eval("iptables", "-t", "nat", "-A","PREROUTING", "-i", dev1,
		"-d", ip1, "-p", proto, "-m", "mport", "--dports", ports1,
		sched, "-j", "DNAT", "--to-destination", ip2, ports2);*/

	/* @ports1 := 21-25
	 * @portsb1 := 21:25
	 * */
	fw_forward_inbound_portformat(ports1, portsb1);
	sprintf(buf, "iptables -t nat -A PREROUTING -i %s -d %s -p %s -m mport --dports %s %s -j DNAT --to-destination %s:%s",
				 dev1, ip1, proto, portsb1, sched, ip2, ports2);
	system(buf);

	/*eval("iptables", "-t", "filter", "-A","FORWARD",
		"-i", dev1, "-d", ip2, "-p", proto, "-m", "mport",
		"--dports", ports2, sched, "-j", "ACCEPT");*/

	fw_forward_inbound_portformat(ports2, portsb2);
	sprintf(buf, "iptables -t filter -A FORWARD -i %s -d %s -p %s -m mport --dports %s %s -j ACCEPT",
				 dev1, ip2, proto, portsb2, sched);
	system(buf);

	sprintf(buf, "%s>%s;iptables -t nat -D PREROUTING -i %s -d %s -p %s "
		       "-m mport --dports %s %s -j DNAT --to-destination %s:%s;"
		       "iptables -t filter -D FORWARD -i %s -d %s -p %s "
		       "-m mport --dports %s %s -j ACCEPT",
		       dev1, dev2,
		       dev1, ip1, proto, portsb1, sched, ip2, ports2,
		       dev1, ip2, proto, portsb2, sched);
	
	nvram_set(strcat_i("forward_port_stop", index, g1), buf);
}

static int __fw_forward_port_start2(
	const char *dev1,
	const char *dev2,
	const char *ip1,
	const char *ip2,
	const char *proto,
	const char *ports1,
	const char *ports2,
	const char *sched,
	int index)
{
#if 0
	if (strcasecmp(proto, "icmp") == 0) {
		__fw_forward_port_start2_icmp(dev1, dev2,
			ip1, ip2, proto, ports1, ports2, sched, index);
	} else if (strcasecmp(proto, "both") == 0) {
		__fw_forward_port_start2_tcp_udp(dev1, dev2,
			ip1, ip2, "tcp", ports1, ports2, sched, index);
		__fw_forward_port_start2_tcp_udp(dev1, dev2,
			ip1, ip2, "udp", ports1, ports1, sched, index);
	} else if (strcasecmp(proto, "all") == 0) {
		__fw_forward_port_start2_no_proto(dev1, dev2,
			ip1, ip2, proto, ports1, ports2, sched, index);
	} else {
		__fw_forward_port_start2_tcp_udp(dev1, dev2,
			ip1, ip2, proto, ports1, ports2, sched, index);
	}
#endif
	if (strcasecmp(proto, "any") == 0) {
		__fw_forward_port_start2_tcp_udp(dev1, dev2,
			ip1, ip2, "tcp", ports1, ports2, sched, index);
		__fw_forward_port_start2_tcp_udp(dev1, dev2,
			ip1, ip2, "udp", ports1, ports2, sched, index);
	} else {	/* @proto := {tcp | udp} */
		__fw_forward_port_start2_tcp_udp(dev1, dev2,
			ip1, ip2, proto, ports1, ports2, sched, index);
	}

	return 0;
}

#define INBOUND_STOP_PREFIX	"fw_inbound_stop"

static void fw_forward_inbound_updown(
	int up, const char *inbound, const char *dev,
	const char *ip2, const char *proto,
	const char *ports1, const char *ports2, int index)
{
	int i = 0;
	char cmds[512], policy[128], portbuf[512];

	sprintf(cmds, "/bin/cli security inbound_filter %s \"%s\" %s",
		up ? "start": "stop",
		inbound,
		dev);

	sprintf(policy, "DNAT --to-destination %s:%s", ip2, ports2);

	DEBUG_MSG("cmds: %s\n", cmds);
	DEBUG_MSG("dev [%s]\nip2 [%s]\n", dev, ip2);
	DEBUG_MSG("proto [%s]\nport1 [%s]\nport2 [%s]\n", proto, ports1, ports2);
	DEBUG_MSG("policy [%s]\n", policy);

	if (strcmp(inbound, "0") == 0 || strcmp(inbound, "Allow ALL") == 0)
		return;

	fw_forward_inbound_portformat(ports1, portbuf);

	setenv("INBOUND_TABLE", "nat", 1);
	setenv("INBOUND_CHAIN", "PREROUTING", 1);
	setenv("INBOUND_PROTO", proto, 1);
	setenv("INBOUND_PORT", portbuf, 1);
	setenv("INBOUND_POLICY", policy, 1);
	setenv("INBOUND_STORE", strcat_i(INBOUND_STOP_PREFIX, index, g1), 1);

	system(cmds);
}

static int match_schedule_index(const char *schedule)
{
	int i;
	char *sch_rule, *ptr;
	char srule[256];
	for(i = 0; i < 25 ; i++) {
		sprintf(srule, "%s", nvram_safe_get_i("schedule_rule", i, g1));
		if(strcmp(srule, "") != 0){
			sch_rule = srule;
			ptr = strsep(&sch_rule ,"/");
			if(strcmp(schedule, ptr) == 0){
				return i;
			}
		}
	}
}

static int __fw_forward_port_start(const char *dev, char *raw, int index)
{
	char *p1, *dev1, ip1[16], *realdev1, *ports1, *pip1;
	char *p2, *dev2, *ip2   , *realdev2, *ports2;
	char *proto, *ports, *sched, *inbound, *time_str = "", *tmp, str[1024] = "";
	char *argv[48] = {""};
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
	int rev = -1, i, idx;
	
	
	/* v0.2 "1;WAN_primary>LAN;Ex_virtual;always;172.21.33.134>192.168.0.22;tcp;33,44:99;44:99" */
	if ((raw = fw_paser_common(raw, NULL, &dev1, &dev2, NULL, &sched)) == NULL)
		goto fmt_err;
	idx = match_schedule_index(sched);
	if( !(idx < 0 || idx > 9) ){
		alg_append_sched(argv, idx, time_cmd);
		for( i =0; time_cmd[i][0] != '\0'; i++){
			strcat(str, time_cmd[i]);
			strcat(str, " ");
		}
	}
	time_str = str;
	//cprintf("\n>>>sched=[%s], time_str=[%s], str=[%s]<<<\n",sched, time_str, str);
	
	DEBUG_MSG("[%s>%s]\n", dev1, dev2);
	if ((realdev1 = __nvram_map("if_alias", dev1, "if_dev")) == NULL)
		goto no_dev;
	if ((realdev2 = __nvram_map("if_alias", dev2, "if_dev")) == NULL)
		goto no_dev;
	
	DEBUG_MSG("[%s=%s>%s=%s]\n", dev1, realdev1, dev2, realdev2);
	/* fail if no any @dev in @dev1 nor @dev2 */
	if (strcmp(realdev1, dev) != 0 && strcmp(realdev2, dev) != 0)
		goto out;
	
	DEBUG_MSG("ip %s>%s\n", realdev1, realdev2);
	/* make sure two devices are up */
	if (get_ip(realdev2, ip1) == -1) goto out;
	if (get_ip(realdev1, ip1) == -1) goto out;
	
	if ((pip1 = strsep(&raw, ">")) == NULL) goto fmt_err;
	if ((ip2 = strsep(&raw, ";")) == NULL) goto fmt_err;
	if ((proto = strsep(&raw, ";")) == NULL) goto fmt_err;
	if ((ports1 = strsep(&raw, ";")) == NULL) goto fmt_err;
	if ((ports2 = strsep(&raw, ";")) == NULL) goto fmt_err;
	if ((inbound = strsep(&raw, ";")) == NULL) goto fmt_err;

	DEBUG_MSG("%s.%s>%s.%s:%s, proto:%s, ports:%s>%s, time_cmd:%s\n",
			dev1, realdev1, dev2, realdev2,ip2,
			proto, ports1, ports2, time_str);

	__fw_forward_port_start2(realdev1, realdev2, ip1, ip2,
			proto, ports1, ports2, time_str, index);

	fw_forward_inbound_updown(1, inbound, realdev1,
			ip2, proto, ports1, ports2, index);

	rev = 0;
no_dev:
out:
	rd(rev, "error");
	return rev;
fmt_err:
	rd(rev, "Format error");
	return rev;
}

#define FORWARD_MAX	99
#define FORWARD_PORT_PREFIX "forward_port"
int fw_forward_port_start(const char *dev)
{
	char *p, buf[512], *if_alias, *t;
	int i, rev = -1;
	
	for (i = 0; i < FORWARD_MAX; i++) {
		if ((p = nvram_get_i(FORWARD_PORT_PREFIX, i, g1)) == NULL)
			continue;
		if (*p != '1')	// @p = '\0' or '1' or '0'
			continue;
		
		p = strlen(p) > sizeof(buf) ? strdup(p) : strcpy(buf, p);
		__fw_forward_port_start(dev, p, i);
		if (p != buf)
			free(p);
	}
out:
	return rev;
}

static void __fw_forward_inbound_stop(int index)
{
	char *tk, *p, *rule, t[2048];

	if (index < 0 || index > FORWARD_MAX)
		return;

	tk = nvram_get(strcat_i(INBOUND_STOP_PREFIX, index, g1));
	strcpy(t, tk?tk:"");

	if (*t == '\0')
		return;
	p = t;
	while (rule = strsep(&p, ";")) {
		char *act;

		if (*rule == '\0') continue;

		if ((act = strstr(rule, "-A")) || (act = strstr(rule, "-I")))
			*(act + 1) = 'D';

		DEBUG_MSG("fw_inbound_stop: %s\n", rule);
		system(rule);
		system(rule);
	}

	nvram_unset_i(INBOUND_STOP_PREFIX, index, g1);
}

int __fw_forward_port_stop(const char *dev, char *s, int index)
{
	char *nat, *filter, *p;
	int rev;
	
	if ((p = strsep(&s, ";")) == NULL) goto out;
	
	if (strstr(p, dev) == NULL)
		goto out;
	
	if ((nat = strsep(&s, ";")) == NULL) goto out;
	if ((filter = strsep(&s, ";")) == NULL) goto out;

	system(nat);
	system(filter);
	nvram_unset_i("forward_port_stop", index, g1);

	__fw_forward_inbound_stop(index);

	rev = 0;
out:
	rd(rev, "error");
	return rev;
}

int fw_forward_port_stop(const char *dev)
{
	char *p, buf[512], *if_alias, *t;
	int i, rev = -1;
	
	for (i = 0; i < FORWARD_MAX; i++) {
		if ((p = nvram_get_i("forward_port_stop", i, g1)) == NULL)
			continue;
		if (strstr(p, dev) == NULL)
			continue;
		p = strlen(p) > sizeof(buf) ? strdup(p) : strcpy(buf, p);
		__fw_forward_port_stop(dev, p, i);
		if (p != buf)
			free(p);
	}
out:
	return rev;
}

int forward_port_start_main(int argc, char *argv[])
{
	if (argc < 2)
		return -1;
	return fw_forward_port_start(argv[1]);
}

int forward_port_stop_main(int argc, char *argv[])
{
	if (argc < 2)
		return -1;
	return fw_forward_port_stop(argv[1]);
}

int forward_port_main(int argc, char *argv[])
{
	struct subcmd cmds[] = {
		{ "start", forward_port_start_main}, //@dev
		{ "stop", forward_port_stop_main}, //@dev
		{ NULL, NULL}
	};
	return lookup_subcmd_then_callout(argc, argv, cmds);
}
