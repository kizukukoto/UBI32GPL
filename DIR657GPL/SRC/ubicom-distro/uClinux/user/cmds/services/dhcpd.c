#include <stdio.h>
#include <stdlib.h>
#include "cmds.h"
#include "nvram.h"
#include "shutils.h"
#include "interface.h"
#include "proto.h"
#include "libcameo.h"

#define DHCPD_CONFIG	"/tmp/udhcpd.conf"
#define DHCPD_LEASES    "/tmp/udhcpd.leases"
#define DHCPD_PID	"/var/run/udhcpd.pid"

//jimmy added
#define DHCPRELAY_CONFIG	"/tmp/dhcp-fwd.conf"
#define DHCPRELAY_LOG	"/tmp/dhcp-fwd.log"
#define DHCPRELAY_PID	"/var/run/dhcp-fwd.pid"
//------------

//jimmy added for test
#if 0
#define DEBUG_MSG(fmt, arg...)       cprintf(fmt, ##arg)
#else
#define DEBUG_MSG(fmt, arg...) 
#endif

//static char dhcprelay_cfg[128]; // dhcp-fwd config file with alias name. example: /tmp/dhcp-fwd.conf.WAN
//static char dhcprelay_log[128]; // dhcp-fwd log file with alias name. example: /var/log/dhcp-fwd.log.WAN
//static char dhcprelay_pid[128]; // dhcp-fwd fpid ile with alias name. example: /var/run//dhcp-fwd.pid.WAN
//--------------------

static char dhcpd_cfg[128];	// udhcpd config file with alias name. example: /tmp/udhcpd.conf.WAN
static char dhcpd_pid[128];	// udhcpd pid file with alias name. example: /tmp/udhcpd.pid.WAN
static char dhcpd_dns[128];	// udhcpd dns ip when relay disable needed

static int dhcpd_get_info(const char *dev, char *p_proto, int *p_proto_idx)
{
	char tmp[512];
	char *addr = "0.0.0.0";
	char *if_proto = NULL;
	char *if_proto_alias = NULL;
	int if_idx = -1;
	int proto_idx = -1;
	int ret = -1;

	if (dev == NULL || strlen(dev) == 0)
		goto out;

	if_idx = nvram_find_index("if_dev", dev);

	DEBUG_MSG("find if_dev?=%s, ?=%d\n", dev, if_idx);

	if (if_idx == -1)
		goto out;

	bzero(tmp, sizeof(tmp));
	if_proto = nvram_safe_get(strcat_r("if_proto", itoa(if_idx), tmp));

	DEBUG_MSG("get if_proto%d=?, ?=%s\n", if_idx, if_proto);

	if (if_proto == NULL)
		goto out;

	bzero(tmp, sizeof(tmp));
	if_proto_alias = nvram_safe_get(strcat_r("if_proto_alias", itoa(if_idx), tmp));

	DEBUG_MSG("get if_proto_alias%d=?, ?=%s\n", if_idx, if_proto_alias);

	if (if_proto_alias == NULL)
		goto out;

	bzero(tmp, sizeof(tmp));
	proto_idx = nvram_find_index(strcat_r(if_proto, "_alias", tmp), if_proto_alias);

	DEBUG_MSG("find index %s_alias?=%s, ?=%d\n", if_proto, if_proto_alias, proto_idx);

	if (proto_idx == -1)
		goto out;

	strcpy(p_proto, if_proto);
	*p_proto_idx = proto_idx;

	ret = 0;
out:
	return ret;
}

static char *dhcpd_get_addr(const char *dev)
{
	int proto_idx = -1;
	char if_proto[512];
	char tmp[512];

	bzero(tmp, sizeof(tmp));
	bzero(if_proto, sizeof(if_proto));

	dhcpd_get_info(dev, if_proto, &proto_idx);
	return nvram_safe_get_i(strcat_r(if_proto, "_addr", tmp), proto_idx, g1);
}

static char *dhcpd_get_subnet(const char *dev)
{
	int proto_idx = -1;
	char if_proto[512];
	char tmp[512];

	bzero(tmp, sizeof(tmp));
	bzero(if_proto, sizeof(if_proto));

	dhcpd_get_info(dev, if_proto, &proto_idx);
	return nvram_safe_get_i(strcat_r(if_proto, "_netmask", tmp), proto_idx, g1);
}

/* Reserve DHCP Format: enable/host name/ip/Mac */
static void dhcpd_config_reserve(FILE *fd, const char *fmt)
{
	char *p, *h, *ip, *mac;

	if (fmt == NULL || strlen(fmt) <= 0 || *fmt != '1')
		return;
	if ((p = strdup(fmt)) == NULL)
		return;
	h = p;
	if ((strsep(&p, "/")) == NULL)	//skip enable|disable
		goto out;
	if ((strsep(&p, "/")) == NULL)	//skip hostname
		goto out;
	if ((ip = strsep(&p, "/")) == NULL)
		goto out;
	mac = p;
	DEBUG_MSG("static_lease %s %s\n", mac, ip);
	fprintf(fd, "static_lease %s %s\n", mac, ip);
out:
	free(h);
	return;
}

static char *get_domain_name(const char *domain, char *_domain)
{
	if (strcmp(domain, "") == 0)
		return "";
	else{
		sprintf(_domain, "option domain	%s\n", domain);
		return _domain;
	}
		
}

static void clear_lease_before_start(const char *lease)
{
	char cmds[128];

	sprintf(cmds, "echo \"\" > %s", lease);
	DEBUG_MSG("cmds: %s\n" ,cmds);
	system(cmds);
}

static char *get_dns_if_relay_disable(const char *lanip, int alias_idx)
{
	char dhcpd_dnsrelay_t[128];

	bzero(dhcpd_dns, sizeof(dhcpd_dns));

	if (lanip == NULL || *lanip == '\0')
		goto out;

	strcpy(dhcpd_dnsrelay_t, nvram_safe_get(strcat_i("dhcpd_dns_relay", alias_idx, g1)));
	DEBUG_MSG("dhcpd_dnsrelay_t: %s\n", dhcpd_dnsrelay_t);

	if (strcmp(dhcpd_dnsrelay_t, "1") == 0)
		sprintf(dhcpd_dns, "option dns	%s\n", lanip);

out:
	return dhcpd_dns;
}

#define DHCPD_RESERVATION_SCALAR	49
static int dhcpd_config(const char *alias, int idx,
			const char *dev, const char *ip, const char *mask)
{
	FILE *fp = NULL;
	char cmdline[1024];
	int len, offset, i, ret = -1;
	char dhcpd_leases[128], *p, buf[1024], domain[256];

	if (alias == NULL || idx == -1)
		goto out;

	DEBUG_MSG("%s, alias = %s, idx = %d, dev = %s, ip = %s, mask = %s\n"
		,__FUNCTION__,alias,idx,dev,ip,mask);

	sprintf(dhcpd_cfg, "%s.%s", DHCPD_CONFIG, alias);
	sprintf(dhcpd_leases, "%s.%s", DHCPD_LEASES, alias);
	sprintf(dhcpd_pid, "%s.%s", DHCPD_PID, alias);

	clear_lease_before_start(dhcpd_leases);

	if ((fp = fopen(dhcpd_cfg, "w")) == NULL)
		goto out;
	
	len = snprintf(cmdline, sizeof(cmdline),
		"interface      %s\n"
		"start          %s\n"
		"end            %s\n"
		"option lease   %d\n"
		"option subnet  %s\n"
		"option router  %s\n"
		"%s"
		"%s"
		"lease_file     %s\n"
		"pidfile        %s\n"
		"auto_time      5\n"
		"max_lease      253\n",
		dev,
		nvram_safe_get_i("dhcpd_start", idx, g1),
		nvram_safe_get_i("dhcpd_end", idx, g1),
		atoi(nvram_safe_get_i("dhcpd_lease", idx, g1)) * 60,
		mask,
		ip,
		get_dns_if_relay_disable(ip, idx),
		get_domain_name(nvram_safe_get(strcat_i("static_hostname", idx, g1)), domain),
		dhcpd_leases,
		dhcpd_pid);

	if (len < 0) {
		perror(__func__);
		goto out;
	}

	fprintf(fp, "%s", cmdline);
	
	/* dhcpd reservation parser */
	offset = idx * DHCPD_RESERVATION_SCALAR;
	for (i = offset;i <= offset + DHCPD_RESERVATION_SCALAR; i++) {
		if ((p = nvram_get_i("dhcpd_reserve_", i, g1)) == NULL)
			continue;
		dhcpd_config_reserve(fp, p);
	}

	fclose(fp);
	ret = 0;
out:
	rd(ret, "error");
	return ret;
}

static inline int __dhcpd_start()
{
	return eval("udhcpd", dhcpd_cfg);
}

static int dhcpd_start(const char *alias, const char *dev,
			const char *ip, const char *mask)
{
	int idx, mode, ret = -1;

	debug("alias = %s, dev = %s, ip = %s, mask = %s",
			alias, dev, ip, mask);
	
	if (alias == NULL || alias[0] == '\0') {
		err_msg("alias is NULL !!, go out");
		goto out;
	}
	
	if ((idx = nvram_find_index("dhcpd_alias", alias)) == -1) {
		err_msg("dhcpd_alias '%s' not found", alias);
		goto out;
	}

	mode = atoi(nvram_safe_get(strcat_i("dhcpd_enable", idx, g1)));
	debug("idx = %d , mode = %s\n", idx,
		mode == 1 ? "dhcpd" : (mode == 2 ? "dhcp relay" : "dhcpd disable"));
	switch(mode) {
		/* "1" enable DHCP Server, "0" disable DHCP server , "2" dhcp relay */
		case 0: 
			break;
		case 1:
			if (dhcpd_config(alias, idx, dev, ip, mask) == -1){
				err_msg("dhcpd_config() fail");
				goto out;
			}
			eval("iptables", "-A", "INPUT", "-i", dev, "-p", "udp",
				"-m", "mport", "--dports", "53,67:68", "-j", "ACCEPT");
			ret = __dhcpd_start();
			break;
		case 2:
			if (dhcprelay_config(alias, idx, dev, ip, mask) == -1){
				err_msg("dhcpd_config(%s) fail",alias);
				goto out;
			}
			eval("iptables", "-A", "INPUT", "-i", dev, "-p", "udp",
				"-m", "mport", "--dports", "53,67:68", "-j", "ACCEPT");
			dhcprelay_start();
			break;
		default:
			break;
	}

	ret = 0;
out:
	DEBUG_MSG("alias: %s, idx:%d, ret:%d\n", alias, idx, ret);
	rd(ret, "error");
	return ret;
}


//jimmy added
static int dhcprelay_stop(void){
	eval("killall dhcp-fwd");
	return 1;
}

static int dhcprelay_start(void){
	eval("dhcp-fwd", "-c", DHCPRELAY_CONFIG);
	return 1;
}

static int dhcprelay_config(
	const char *alias, int idx, const char *dev,
	const char *ip, const char *mask)
{
	FILE *fp;
	int ret = -1;
	char *p, dhcpd_leases[128], buf[1024];
	char *lan_if, *wan_if;

	if (alias == NULL || idx == -1)
		goto out;
	
	lan_if = nvram_safe_get("if_dev0");
	wan_if = nvram_safe_get("if_dev2");

	DEBUG_MSG("%s, alias = %s, idx = %d, dev = %s, ip = %s, mask = %s\n"
		,__FUNCTION__, alias, idx, dev, ip, mask);

	/*
	memset(dhcprelay_cfg,'\0',sizeof(dhcprelay_cfg));
	memset(dhcprelay_log,'\0',sizeof(dhcprelay_log));
	memset(dhcprelay_pid,'\0',sizeof(dhcprelay_pid));
	*/

	if ((fp = fopen(DHCPRELAY_CONFIG, "w")) == NULL)
		goto out;

	fprintf(fp, "logfile	%s\n", DHCPRELAY_LOG);
	fprintf(fp, "loglevel	1\n");
	fprintf(fp, "pidfile	%s\n", DHCPRELAY_PID);
	/* IFNAME  clients servers bcast */
	fprintf(fp,"if	%s	true	false	true\n", lan_if);//br0
	fprintf(fp,"if	%s	false	false	true\n", wan_if);//eth1
	//fprintf(fp,"if    eth1    false    false    ture",nvram_safe_get(strcat_i("if_dev", idx, g1)));

	/* IFNAME  agent-idname */
	fprintf(fp,"name	%s	DIR-730\n", lan_if);//br0
		
	/*   TYPE    address */
	//fprintf(fp,"server  ip  192.168.8.66\n");
	fprintf(fp,"server	bcast	%s\n", wan_if);//eth1
	fclose(fp);
	ret = 0;
out:
	rd(ret, "error");
	return ret;
}
//-----------

static int dhcpd_start_main(int argc, char *argv[])
{
	int ret = -1;
	char *alias, *dev, *ip, *netmask;	/* from argv */
	//printf("What Happen\n");
	args(argc -1, argv + 1, &alias, &dev, &ip, &netmask, NULL);
	//printf("What Happen\n");

	ret = dhcpd_start(alias, dev, ip, netmask);
	//printf("What Happen\n");
out:
	rd(ret, "dhcpcd");
	//printf("What Happen\n");
	return ret;
}

static int dhcpd_stop(const char *alias)
{
	FILE *fp = NULL;
	int pid, ret = -1;

	DEBUG_MSG("##### DHCPD(%s) STOP #####\n", alias);

	snprintf(dhcpd_pid, sizeof(dhcpd_pid) - 1, "%s.%s", DHCPD_PID, alias);

	if ((fp = fopen(dhcpd_pid, "r")) == NULL)
		goto out;

	fscanf(fp, "%d", &pid);
	fclose(fp);

	DEBUG_MSG("##### DHCPD PID %d FROM FILE %s #####\n", pid, dhcpd_pid);

	ret = eval("kill", "-9", itoa(pid));
out:
	rd(ret, "error");
	return ret;
}

static int dhcpd_stop_main(int argc, char *argv[])
{
	int ret = dhcpd_stop(argv[1]);

	if (argc >= 3) {
		/*BUG: argv[2] will be random code? memory lack? */
		char *dev = argv[2];

		//printf("dhcp stop on argv[2]=[%s]:%x\n", argv[2], argv[2]);
		eval("iptables", "-D", "INPUT", "-i", dev, "-p", "udp",
			"-m", "mport", "--dports", "53,67:68", "-j", "ACCEPT");

	}
	
	dhcprelay_stop();

	return ret;
}

/*
 * operation policy:
 * start: evel dhcpd with interfces
 * stop:  kill dhpcpd
 * */
int dhcpd_main(int argc, char *argv[])

{
	struct subcmd cmds[] = {
		{ "start", dhcpd_start_main}, //args: alias, dev, ip, mask
		{ "stop", dhcpd_stop_main},
		{ NULL, NULL}
	};

	return lookup_subcmd_then_callout(argc, argv, cmds);
}
