#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <ctype.h>

#include <sutil.h>
#include <nvram.h>
#include <shvar.h>
#include <rc.h>

#define unlink(f)	do { } while(0)

#ifdef RC_DEBUG
#define DEBUG_MSG(fmt, arg...)       printf("%s [%d]: "fmt, __FILE__, __LINE__, ##arg)
#else
#define DEBUG_MSG(fmt, arg...) 
#endif

#define FOR_EACH_WAN_IP for(k=0;k<2;k++)\
							if(strlen(wan_ipaddr[k]))

#define FOR_EACH_WAN_FACE for(k=0;k<2;k++)\
							if(strlen(wan_if[k]))

/*	Port Forwarding format with both TCP and UDP
*  	Nvram name:  port_forward_both_%02d
*  	Format:  enable/name/lan_ip/tcp_ports/udp_ports/schedule/inbound_filter
*
*/

/*	Port Forwarding 
 *  	Nvram name:  port_forward_%02d
 *  	Format:  enable/name/proto/public_port_start/public_port_end/private_port_start/private_port_end/lan_ip/schedule_rule_00_name
 *   	Iptables command:
 *   		iptables -A PREROUTING -t nat -p tcp -d 172.21.34.17 
 *   			--dport 21:30 -j DNAT --to 192.168.0.100:21-30
 *   		iptables -A FORWARD -i eth0 -o br0 -p tcp --sport 1024:65535 
 *   			-d 192.168.0.100 --dport 21:30 -m state --state NEW -j ACCEPT
 */
struct rc_forward {
	char *enable, *name, *wan_proto, *public_port_start;
	char *public_port_end, *private_port_start, *private_port_end;
	char *lan_ip, *schedule_name;
#ifdef PORT_FORWARD_BOTH
	char *tcp_ports, *udp_ports;
#ifdef INBOUND_FILTER
	char *inbound_name;
#endif /* INBOUND_FILTER */
#endif /* PORT_FORWARD_BOTH */
};

static int k;
static int __setup_port_forward(
	struct rc_forward *f,
	const char wan_if[2][10],
	const char wan_ipaddr[2][16],
	const char *bri_eth
)
{
	char public_port_range[12];
	char private_port_range_1[12]={0}, private_port_range_2[12]={0};
	char tcp_port_range[64]={0};

	if ((strcmp("TCP", f->wan_proto) == 0) || 
		(strcmp("Any", f->wan_proto) == 0) ||
		(strcmp("NetMeeting", f->name) == 0))  
	{
		memset(public_port_range, 0, sizeof(public_port_range));
		memset(private_port_range_1, 0, sizeof(private_port_range_1));
		memset(private_port_range_2, 0, sizeof(private_port_range_2));
		
		if((strcmp("NetMeeting", f->name) == 0)) {
			strcpy(public_port_range, "1503");
			strcpy(private_port_range_1, "1503");
			strcpy(private_port_range_2, "1503");
		} else {
			sprintf(public_port_range,"%s:%s",
						f->public_port_start,
						f->public_port_end);
			sprintf(private_port_range_1,"%s-%s",
						f->private_port_start,
						f->private_port_end);
			sprintf(private_port_range_2,"%s:%s",
						f->private_port_start,
						f->private_port_end);
		}
	
		FOR_EACH_WAN_IP
			save2file(FW_NAT, "-A PREROUTING -p TCP -d %s "
			" --dport %s %s -j DNAT --to %s:%s\n",
			wan_ipaddr[k], 
			public_port_range,
			schedule2iptables(f->schedule_name),
			f->lan_ip, 
			private_port_range_1);
		
		save2file(FW_FILTER,"-A FORWARD -o %s -p TCP -d %s "
		"--dport %s %s -m state --state NEW -j ACCEPT\n",
		bri_eth, 
		f->lan_ip, private_port_range_2,
		schedule2iptables(f->schedule_name));
	}	

	/* XXX: lost if name is "NetMeeting" and "UDP" ? */
	if (((strcmp("UDP", f->wan_proto) == 0) ||
		(strcmp("Any", f->wan_proto) == 0)) &&
		(strcmp("NetMeeting", f->name)))
	{
		FOR_EACH_WAN_IP
			save2file(FW_NAT,"-A PREROUTING -p UDP -d %s "
			"--dport %s:%s %s -j DNAT --to %s:%s-%s\n",
			wan_ipaddr[k], 
			f->public_port_start,
			f->public_port_end, 
			schedule2iptables(f->schedule_name),
			f->lan_ip, 
			f->private_port_start, 
			f->private_port_end);
		
		save2file(FW_FILTER,"-A FORWARD -o %s -p UDP -d %s "
		"--dport %s:%s %s -m state --state NEW -j ACCEPT\n",
		bri_eth, 
		f->lan_ip, 
		f->private_port_start, 
		f->private_port_end,
		schedule2iptables(f->schedule_name));
	}	
}

/* 
 * inbound_ip_count >0 implies to apply inbound filter. 
 * If allowing multiple ranges of IP to port forwarding, we add rules to DNAT for each ip range.
 * ex. 
 * -A PREROUTING -m iprange --src-range 1.1.1.1-2.2.2.2 -p TCP -d 1.1.1.1 -m multiport --destination-ports 20:23,80  -j DNAT --to 192.0.0.1
 * -A PREROUTING -m iprange --src-range 3.3.3.3-4.4.4.4 -p TCP -d 1.1.1.1 -m multiport --destination-ports 20:23,80  -j DNAT --to 192.0.0.1
 *
 * Else Deny multiple ranges of IP to access virtual server, we add rules to ACCEPT it for each ip range first.
 * Then add the last rule to DNAT. Once packets match a rule, it will leave the iptable chain. 
 * So, Denied-ip won't reach the last rule to DNAT. 
 * ex.
 * -A PREROUTING -m iprange --src-range 1.1.1.1-2.2.2.2 -p TCP -d 1.1.1.1 -m multiport --destination-ports 20:23,80 -j ACCEPT
 * -A PREROUTING -m iprange --src-range 3.3.3.3-4.4.4.4 -p TCP -d 1.1.1.1 -m multiport --destination-ports 20:23,80 -j ACCEPT
 * -A PREROUTING -p TCP -d 1.1.1.1 -m multiport --destination-ports 20:23,80 -j DNAT --to 192.0.0.1
 *
 */
static void __setup_port_forward_both_proto(
	struct rc_forward *f,
        const char wan_if[2][10], const char wan_ipaddr[2][16],
        const char *bri_eth, int inbound_ip_count,
	char *action, struct IpRange ip_range[8], 
	const char *proto, const char *port_range
){
	if(inbound_ip_count && strcmp(f->inbound_name, "Allow_All")) {
#ifdef INBOUND_FILTER		
		int j;
		if(!strcmp(action, "allow")) {
			for(j = 0; j < inbound_ip_count; j++)	{
				FOR_EACH_WAN_IP
					save2file(FW_NAT, 
					"-A PREROUTING -m iprange"
					" --src-range %s -p %s"
					" -d %s -m multiport"
					" --destination-ports"
					" %s %s -j DNAT --to %s\n",
					ip_range[j].addr, proto,
					wan_ipaddr[k], 
					port_range, 
					schedule2iptables(f->schedule_name), 
					f->lan_ip);
			}
		} else {
		  	for(j = 0; j < inbound_ip_count; j++) {
				FOR_EACH_WAN_IP
					save2file(FW_NAT, 
					"-A PREROUTING -m iprange"
					" --src-range %s -p %s"
					" -d %s -m multiport"
					" --destination-ports"
					" %s %s -j ACCEPT\n",
					ip_range[j].addr, proto,
					wan_ipaddr[k], 
					port_range, 
					schedule2iptables(f->schedule_name));	
		  	}

			FOR_EACH_WAN_IP
				 save2file(FW_NAT,
				 "-A PREROUTING -p %s -d %s"
				 " -m multiport"
				 " --destination-ports"
				 " %s %s -j DNAT --to %s\n",
				 proto, wan_ipaddr[k], 
				 port_range, 
				 schedule2iptables(f->schedule_name), 
				 f->lan_ip);
		}
#endif	
	}
	/* Allow All implies not to apply inbound filter */
	else {
		FOR_EACH_WAN_IP
			 save2file(FW_NAT,
			 "-A PREROUTING -p %s -d %s -m multiport"
			 " --destination-ports %s %s"
			 " -j DNAT --to %s\n",
			 proto, wan_ipaddr[k], 
			 port_range, 
			 schedule2iptables(f->schedule_name),
			 f->lan_ip);
	}

	save2file(FW_FILTER,
	"-A FORWARD -o %s -p %s -d %s -m multiport"
	" --destination-ports %s %s -m state --state NEW -j ACCEPT\n",
	bri_eth, proto, f->lan_ip, port_range, 
	schedule2iptables(f->schedule_name));
}

static void __setup_port_forward_both_protocol(
	struct rc_forward *f,
        const char wan_if[2][10], const char wan_ipaddr[2][16],
        const char *bri_eth, int inbound_ip_count,
	char *action, struct IpRange ip_range[8], const char *proto
)
{
	char port_range[64]={0};

	if(strcmp(proto, "tcp") == 0){
		/* TCP */
		if(strcmp(f->tcp_ports,"0") || (strcmp("NetMeeting", f->name) == 0)) {
			memset(port_range, 0, sizeof(port_range));
			if((strcmp("NetMeeting", f->name) == 0))
				strcpy(port_range,"1503");
			else {
				removeMark(f->tcp_ports);
				strcpy(port_range, f->tcp_ports);
			}
			__setup_port_forward_both_proto( f, wan_if, 
							wan_ipaddr, bri_eth, 
							inbound_ip_count, action, ip_range,
							proto, port_range);
		}
	}else{
		/* UDP */
		if(strcmp(f->udp_ports, "0")){
			removeMark(f->udp_ports);
			strcpy(port_range, f->udp_ports);
			__setup_port_forward_both_proto( f, wan_if, 
							wan_ipaddr, bri_eth, 
							inbound_ip_count, action, ip_range,
							proto, port_range);
		}
	}
}

void __setup_port_forward_both(
	struct rc_forward *f,
	const char wan_if[2][10],
	const char wan_ipaddr[2][16],
	const char *bri_eth
)
{
	int inbound_ip_count=0;

#ifdef PORT_FORWARD_BOTH
	char tcp_port_range[64]={0};
	struct IpRange ip_range[8];
	char action[10];
#endif //PORT_FORWARD_BOTH

#ifdef INBOUND_FILTER
	if(f->inbound_name && strlen(f->inbound_name))
	{
		/* Deny All implies that the rule is invalid */
		if(!strcmp(f->inbound_name, "Deny_All"))
			return; //continue;
		memset(action,0,sizeof(action));
		memset(ip_range,0,8*sizeof(struct IpRange));
		inbound_ip_count = inbound2iptables(f->inbound_name, 
						    action, 
						    ip_range);
	}
#endif
	__setup_port_forward_both_protocol(f, wan_if, wan_ipaddr, 
			     	      bri_eth, inbound_ip_count,
				      action, ip_range, "tcp");

	__setup_port_forward_both_protocol(f, wan_if, wan_ipaddr, 
			     	      bri_eth, inbound_ip_count,
				      action, ip_range, "udp");
}

void setup_port_forwarding(const char wan_if[2][10], const char wan_ipaddr[2][16], const char *bri_eth)
{
	int i;
	char fw_rule[64],  fw_value[192], *getValue;
	struct rc_forward fwd, *f;
	/*
	char *name, *wan_proto, *public_port_start;
	char *public_port_end, *private_port_start, *private_port_end;
	char *lan_ip, *schedule_name;
	*/
	f = &fwd;
	
	DEBUG_MSG("\t NEW Port Forwarding format with both TCP and UDP\n");
	for (i = 0; i < MAX_PORT_FORWARDING_NUMBER; i++) {
#ifdef PORT_FORWARD_BOTH
		snprintf(fw_rule, sizeof(fw_rule), "port_forward_both_%02d", i);
#else
		snprintf(fw_rule, sizeof(fw_rule), "port_forward_%02d", i);
#endif //PORT_FORWARD_BOTH
		getValue = nvram_safe_get(fw_rule);
		
		DEBUG_MSG("setup fw_rule:%s\n", getValue);
		if (getValue == NULL ||  strlen(getValue) == 0 ) 
			continue;
		else 
			strncpy(fw_value, getValue, sizeof(fw_value));

#ifdef PORT_FORWARD_BOTH
		getStrtok(fw_value, "/", "%s %s %s %s %s %s %s ",
			&f->enable, &f->name, &f->lan_ip, &f->tcp_ports,
			&f->udp_ports, &f->schedule_name, &f->inbound_name);
		DEBUG_MSG("%s->%d\n",__FUNCTION__, __LINE__);

#else
		DEBUG_MSG("%s->%d\n",__FUNCTION__, __LINE__);
		if ( getStrtok(fw_value, "/", "%s %s %s %s %s %s %s %s %s",
			&f->enable, &f->name, &f->wan_proto, &f->public_port_start,
			&f->public_port_end, &f->private_port_start,
			&f->private_port_end, &f->lan_ip, &f->schedule_name) != 9)
			continue;
#endif //PORT_FORWARD_BOTH
		DEBUG_MSG("%s->%d, enable:%s\n",__FUNCTION__, __LINE__, f->enable);
		if(strcmp(f->schedule_name, "Never") == 0)
			continue;
		if (strcmp("1", f->enable) != 0)
			continue;


		DEBUG_MSG("setup fw_rule\n");
#ifdef PORT_FORWARD_BOTH
		DEBUG_MSG("%s->%d\n",__FUNCTION__, __LINE__);
		__setup_port_forward_both(f, wan_if, wan_ipaddr, bri_eth);
#else
		DEBUG_MSG("%s->%d\n",__FUNCTION__, __LINE__);
		__setup_port_forward(f, wan_if, wan_ipaddr, bri_eth);
#endif //PORT_FORWARD_BOTH
	}
	DEBUG_MSG("%s->%d: finished\n",__FUNCTION__, __LINE__);
}
void setup_nattype(const char wan_if[2][10], const char *bri_eth)
{
	struct {
		char *nvram;
		char *value;
		char *ipt_proto;
	} nt[] = {
		{ "tcp_nat_type", "1", "TCP"},
		{ "tcp_nat_type", "2", "TCP"},
		{ "udp_nat_type", "1", "UDP"},
		{ "udp_nat_type", "2", "UDP"},
		{ NULL, NULL, NULL},
	}, *np = nt;
	int k;
	
	/*
	 * for skip setup nattype , just set nvram key to null or out of range
	 * eg: nvram unset tcp_nat_type
	 * */

	/* NATTYPE --mode has questions in current */
	return ;
	for (np = nt; np->nvram != NULL; np++) {
		if (nvram_match(np->nvram, np->value) != 0)
			continue;
		FOR_EACH_WAN_FACE {
			save2file(FW_FILTER, "-A FORWARD -i %s -o %s "
				"-p %s -j NATTYPE --mode forward_out "
				"--type %s\n",
				bri_eth, wan_if[k], np->ipt_proto, np->value);
		
			save2file(FW_FILTER, "-A FORWARD -i %s -o %s "
				"-p %s -j NATTYPE --mode forward_in "
				"--type %s\n",
				wan_if[k], bri_eth, np->ipt_proto, np->value);
			
			save2file(FW_NAT, "-A PREROUTING -i %s -p %s "
				"-j NATTYPE --mode dnat --type %s\n",
				wan_if[k], np->ipt_proto, np->value);
		}
	}
}

/* WEB FILTER ROUTINE, without used now */
void setup_website_filter(
	const char *lan_dev,
	const char *lan_addr,
	const char *lan_mask)
{
	int i = 0;
	const char *wf_conf = "/tmp/web_filter";
	char *wf_type = nvram_safe_get("url_domain_filter_type"), wf_rule[32];
	FILE *fp = fopen(wf_conf, "w");

	if (fp == NULL)
		goto out;
	if (lan_addr == NULL || *lan_addr == '\0'
		|| lan_mask == NULL || *lan_mask == '\0')
		goto perr;
	for (; i < MAX_URL_FILTER_NUMBER; i++) {
		char *wf_txt;

		sprintf(wf_rule, "url_domain_filter_%02d", i);
		if (*(wf_txt = nvram_safe_get(wf_rule)) == '\0')
			continue;
		fprintf(fp, "%s\n", wf_txt);
	}

	fclose(fp);

	_system("crowdcontrol -a %s -s %s -p %d --%s %s",
		lan_addr, lan_mask, 3128,
		(strcmp(wf_type, "list_allow") == 0)? "permit-domains": "deny-domains",
		wf_conf);

#ifdef IP8000
	_system("iptables -t nat -I PREROUTING -i %s ! -d %s "
		"-p tcp --dport 80 -j DNAT --to-destination %s:3128",
		lan_dev,
		lan_addr,
		lan_addr);
#else
	_system("iptables -t nat -I PREROUTING -i %s -d ! %s "
		"-p tcp --dport 80 -j DNAT --to-destination %s:3128",
		lan_dev,
		lan_addr,
		lan_addr);
#endif

	goto out;
perr:
	fclose(fp);
out:
	return;
}
