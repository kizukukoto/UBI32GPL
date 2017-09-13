#include <stdio.h>
#include <stdlib.h>
#include "cmds.h"
#include "shutils.h"
#include "interface.h"
#include <nvramcmd.h>

#if 0
#define DEBUG_MSG(fmt, arg...)       cprintf(fmt, ##arg)
#else
#define DEBUG_MSG(fmt, arg...) 
#endif

#define ZEBRA_CONF	"/var/tmp/zebra.conf"
#define RIPD_CONF	"/var/tmp/ripd.conf"
#define OSPFD_CONF	"/var/tmp/ospfd.conf"


#define STATIC_ROUTE_MAX	100


char *get_ip_and_mask(char *if_addr, char *if_mask)
{
	char *ip[4];
	char *mask[4];
	char tmp[32];
	int ip_mask[4];
	int i=0;
	
	DEBUG_MSG("%s:if_addr=%s,if_mask=%s\n", __func__,if_addr,if_mask);	
	if ( (i = split(if_addr, ".", ip, 0)) != 4 ){
		DEBUG_MSG("%s:split fail %d\n", __func__,i);
		//return NULL;
	}
	DEBUG_MSG("%s:ip=%s.%s.%s.%s\n", __func__,ip[0],ip[1],ip[2],ip[3]);
	if ( (i = split(if_mask, ".", mask, 0)) != 4 ){
		DEBUG_MSG("%s:split fail %d\n", __func__,i);
		//return NULL;
	}		
	DEBUG_MSG("%s:mask=%s.%s.%s.%s\n", __func__,mask[0],mask[1],mask[2],mask[3]);	
	for(i=0;i<4;i++){
		ip_mask[i]= (atoi(ip[i]) & atoi(mask[i]));
		DEBUG_MSG("%s:ip_mask[%d]=%d\n", __func__,i,ip_mask[i]);
	}
	sprintf(tmp, "%d.%d.%d.%d",ip_mask[0],ip_mask[1],ip_mask[2],ip_mask[3]);
	DEBUG_MSG("%s:tmp%s\n", __func__,tmp);	
	return tmp;		
}	

char *get_ip_mask_map(char *if_addr, char *if_mask)
{
	char tmp[64]={0};
	int maskint=0;
	
	DEBUG_MSG("%s:if_addr=%s,if_mask=%s\n", __func__,if_addr,if_mask);	
	maskint=netmask2int(if_mask);
	DEBUG_MSG("%s:maskint=%d\n", __func__,maskint);
	sprintf(tmp , "%s/%d" ,get_ip_and_mask(if_addr,if_mask),maskint);	
	DEBUG_MSG("%s:tmp=%s", __func__,tmp);
	return tmp;
}


static int static_route_start_stop_main(int start, int argc, char *argv[])
{
	char *p, *h;
	int rev = -1, idx, i;
	char *sp[STATIC_ROUTE_MAX];
       	
	DEBUG_MSG("%s:\n",__func__);
	if (argc < 3){
		DEBUG_MSG("%s: argc < 3\n",__func__);
		goto out;
	}	
	if ((h = strdup(argv[2])) == NULL){
		DEBUG_MSG("%s: strdup(argv[2]) == NULL\n",__func__);
		goto out;
	}	
	p = h;

	/* parse : NAME1/NET/MASK/GW/METRIC,NAME2/NET/... */
	
	idx = split(p, ",", sp, 1);
	for (i = 0; i < idx; i++) {
		char *_s[5];
		int _i;
		
		if ((_i = split(sp[i], "/", _s, 1)) != 5)
			continue;	/* format error or unknow */
			
		DEBUG_MSG("Route %s: %s %d %s %s %s\n", start?"add":"del", 
			argv[1], atoi(_s[4])+1, _s[1], _s[3], _s[2]);
		/* route_add(ifname, metric, ipaddr, gateway, netmask) */
		start ? route_add(argv[1], atoi(_s[4])+1, _s[1], _s[3], _s[2]):
			route_del(argv[1], atoi(_s[4])+1, _s[1], _s[3], _s[2]);
	}
		
	free(h);
	rev = 0;
out:
	return rev;
}

static int static_route_start_main(int argc, char *argv[])
{
	DEBUG_MSG("%s:\n",__func__);
	return static_route_start_stop_main(1, argc, argv);
}

static int static_route_stop_main(int argc, char *argv[])
{
	DEBUG_MSG("%s:\n",__func__);
	return static_route_start_stop_main(0, argc, argv);
}


/*
 * Enable;Name;Source_if;\
 * 	Source_IP/Source_mask/Source_sport-Source_eport;Protocol;
 * 	Dest_IP/Dest_mask/Dest_sport-Dest_eport;Forward;Gateway
 *
 **/
/*
int policy_route_start(void){
	cprintf("Start Policy Route\n");

	char *tmp[8], *src[3], *dest[3], *dest_port[2];
	char *p;
	char str[512]={0}, strKey[16]={0}, s[512]={0}, str_rule[512]={0};
	int n = 0, i = 0;

	for(i=0; i<25; i++){
		memset(strKey, sizeof(strKey), 0);
		memset(tmp, sizeof(tmp), 0);
		memset(str, sizeof(str), 0);
		memset(s, sizeof(s), 0);
	
		sprintf(strKey, "policy_rule%d", i);
		//cprintf("Key=[%s]\n", strKey);

		p = nvram_safe_get(strKey);
		strcpy(str, p);
		//cprintf(str);

		if (strlen(str) == 0)
			goto out;
		
		cprintf("\nStart Parsing:[%d]\n", i);
		if(split(str, ";", tmp, 1)!=8){
			cprintf("nvram:%s is invalid\n");
			return 1;
		}

		for (n = 0; n < 8; n++)
			cprintf("tmp[%d]=%s\n", n, tmp[n]);

		if (split(tmp[3], "/", src, 0)!=3){
			cprintf("nvram:%s source ip is invalid\n");
			return 2;
		}
		//cprintf("src:[%s], ip=[%s], mask=[%s], port=[%s]\n", s, src[0], src[1], src[2]);

		if (split(tmp[5], "/", dest, 0)!=3) {
			cprintf("nvram:%s source ip is invalid\n");
			return 2;
		}
		//cprintf("dest:[%s],[%s] ip=[%s], mask=[%s], port=[%s]\n", s, src[0], dest[0], dest[1], dest[2]);

		if ( split(dest[2], "-", dest_port, 0)!=2 ) {
			cprintf("nvram:%s dest port is invalid\n");
			return 3;
		}

		if (strcmp(tmp[0], "1")!= 0)
			goto out;

		memset(str_rule, sizeof(str_rule), 0);
		if( strcmp(tmp[4], "tcp") == 0 || strcmp(tmp[4], "udp") == 0 ){
			sprintf(str_rule, "iptables -t mangle -I PREROUTING -p %s -s %s/%s -d %s/%s -m mport --dport %s:%s -j MARK --set-mark 10",
				tmp[4], src[0], src[1], dest[0], dest[1], dest_port[0], dest_port[1] );
		} else{
			sprintf(str_rule, "iptables -t mangle -I PREROUTING -s %s/%s -d %s/%s -j MARK --set-mark 10",
				src[0], src[1], dest[0], dest[1] );
		}
		system(str_rule);
		cprintf("\n%s\n", str_rule);
	
		memset(str_rule, sizeof(str_rule), 0);
		sprintf(str_rule, "ip rule add fwmark 10 table 5");
		system(str_rule);
		cprintf("%s\n\n", str_rule);

		memset(str_rule, sizeof(str_rule), 0);
		sprintf(str_rule, "ip route add %s/%s via %s table 5 dev %s", dest[0], dest[1], tmp[7], tmp[6]);
		system(str_rule);
		cprintf("%s\n\n", str_rule);
	}
out:
	return 0;
}

int policy_route_stop(void){
	char str[512];

	cprintf("Stop Policy Route\n");
	
	sprintf(str, "ip route flush table 5");
	system(str);
	cprintf("%s\n", str);

	sprintf(str, "ip rule delete table 5");
	system(str);
	cprintf("%s\n", str);

	sprintf(str, "iptables -F -t mangle");
	system(str);
	cprintf("%s\n", str);
	return 0;
}
*/
static void _save2file(const char *dev, int rx, int tx)
{
	save2file(RIPD_CONF, "interface %s\n", dev);
	save2file(RIPD_CONF, "ip split-horizon\n");
	save2file(RIPD_CONF, "ip rip send version %d\n", tx);
	save2file(RIPD_CONF, "ip rip receive version %d\n", rx);
	
}

static void save_RIPD_CONF(const char *lan, const char *dmz, const char *wan)
{
	char _lan[128], _dmz[128], _wan[128];
	
	if(lan != NULL)
		sprintf(_lan, "network %s\n", lan);
	
	if(dmz != NULL)
		sprintf(_dmz, "network %s\n", dmz);

	if(wan[0] != '\0')
		sprintf(_wan, "network %s\n", wan);

	printf("LAN %s\nDMZ %s\nWAN %s\n", _lan, _dmz, _wan);

	save2file(RIPD_CONF, 
	"router rip\n"
	"%s"
	"%s"
	"%s"
	"redistribute connected\n"
	"redistribute kernel\n"
	"redistribute ospf\n",
	_lan, _wan, _dmz
	);
}

int start_zebra_rip(void)
{
	char *lan_eth, *dmz_eth;
	char wan_eth[16]={0}, buf[64]={0};

	int rip_rx_ver0 = 0, rip_tx_ver0 = 0; /* WAN */
	int rip_rx_ver1 = 0, rip_tx_ver1 = 0; /* LAN */
	int rip_rx_ver2 = 0, rip_tx_ver2 = 0; /* DMZ */

	/* WAN */
	if (NVRAM_MATCH("rip_enable0", "1")){
		get_wan_iface(wan_eth);
		rip_rx_ver0 = atoi( nvram_safe_get("rip_rx_version0") );
		rip_tx_ver0 = atoi( nvram_safe_get("rip_tx_version0") );
	}
	/* LAN */
	if (NVRAM_MATCH("rip_enable1", "1")){
		lan_eth = nvram_safe_get("if_dev0");
		rip_rx_ver1 = atoi( nvram_safe_get("rip_rx_version1") );
		rip_tx_ver1 = atoi( nvram_safe_get("rip_tx_version1") );
	}
	/* DMZ */
	if (NVRAM_MATCH("rip_enable2", "1")){
		dmz_eth = nvram_safe_get("if_dev1");
		rip_rx_ver2 = atoi( nvram_safe_get("rip_rx_version2") );
		rip_tx_ver2 = atoi( nvram_safe_get("rip_tx_version2") );
	}
	//config
	init_file(ZEBRA_CONF);	
	init_file(RIPD_CONF);
	save_RIPD_CONF(lan_eth, dmz_eth, wan_eth);

	/* LAN setting */
//	DEBUG_MSG("lan_eth=%s setting\n",lan_eth);
	if(lan_eth!=NULL)
		_save2file(lan_eth, rip_rx_ver1, rip_tx_ver1);

	if(dmz_eth!=NULL)
		_save2file(dmz_eth, rip_rx_ver2, rip_tx_ver2);

	if(wan_eth[0] != '\0')
		_save2file(wan_eth, rip_rx_ver0, rip_tx_ver0);



	//rip
	save2file(RIPD_CONF, "router rip\n");

	/* LAN tx */
	if(rip_tx_ver1 == 0){
		save2file(RIPD_CONF, 
		" distribute-list private out %s\n", lan_eth);
	}
	
	/* WAN tx */
	if(rip_tx_ver0 == 0){
		save2file(RIPD_CONF, 
		" distribute-list private out %s\n", wan_eth);
	}
	
	/* DMZ tx */
	if(rip_tx_ver2 == 0){
		save2file(RIPD_CONF, 
		" distribute-list private out %s\n", dmz_eth);
	}

	/* LAN rx */
	if(rip_rx_ver1== 0){
		save2file(RIPD_CONF, 
		" distribute-list private in %s\n", lan_eth);
	}
	
	/* WAN rx */
	if(rip_rx_ver0== 0){
		save2file(RIPD_CONF, 
		" distribute-list private in %s\n", wan_eth);
	}

	/* DMZ rx */
	if(rip_rx_ver2== 0){
		save2file(RIPD_CONF, 
		" distribute-list private in %s\n", dmz_eth);
	}
	
	save2file(RIPD_CONF, "access-list private deny any\n");
		
	//zebra
	sprintf(buf,"zebra -f %s -d",ZEBRA_CONF);
	DEBUG_MSG("ntp_start_main: buf=%s\n",buf);
	system(buf);	
	
	//ripd
	sprintf(buf,"ripd -f %s -d",RIPD_CONF);
	DEBUG_MSG("ntp_start_main: buf=%s\n",buf);
	system(buf);
	
	return 0;
}

int start_zebra_ospf(void)
{
	char *lan_eth, *dmz_eth;
	char wan_eth[16]={0}, buf[64]={0};	
	char ip_addr[32]={0} ,mask[32]={0};
	char lan_ipmask[32]={0} ,dmz_ipmask[32]={0}, wan_ipmask[32]={0};
	
	DEBUG_MSG("%s:\n",__func__);
	get_wan_iface(wan_eth);	
	lan_eth = nvram_safe_get("if_dev0");
	dmz_eth = nvram_safe_get("if_dev1");
	DEBUG_MSG("%s: wan_eth=%s\n",__func__,wan_eth);
	DEBUG_MSG("%s: lan_eth=%s\n",__func__,lan_eth);
	DEBUG_MSG("%s: dmz_eth=%s\n",__func__,dmz_eth);
	//lan
	strcpy(ip_addr,get_if_ipaddr(lan_eth));
	strcpy(mask,get_if_mask(lan_eth));
	DEBUG_MSG("ospf lan ipaddr=%s,mask=%s\n",ip_addr,mask);
	sprintf(lan_ipmask,"%s",get_ip_mask_map(ip_addr,mask));
	DEBUG_MSG("%s: lan_ipmask=%s\n",__func__,lan_ipmask);
	//dmz
	strcpy(ip_addr,get_if_ipaddr(dmz_eth));
	strcpy(mask,get_if_mask(dmz_eth));
	DEBUG_MSG("ospf dmz ipaddr=%s,mask=%s\n",ip_addr,mask);
	sprintf(dmz_ipmask,"%s",get_ip_mask_map(ip_addr,mask));
	DEBUG_MSG("%s: dmz_ipmask=%s\n",__func__,dmz_ipmask);
	//wan
	strcpy(ip_addr,get_if_ipaddr(wan_eth));
	strcpy(mask,get_if_mask(wan_eth));
	DEBUG_MSG("ospf wan ipaddr=%s,mask=%s\n",ip_addr,mask);
	sprintf(wan_ipmask,"%s",get_ip_mask_map(ip_addr,mask));
	DEBUG_MSG("%s: wan_ipmask=%s\n",__func__,wan_ipmask);
	//config
	init_file(ZEBRA_CONF);
	init_file(OSPFD_CONF);			
	/* LAN setting */
	DEBUG_MSG("lan_eth=%s setting\n",lan_eth);
	save2file(OSPFD_CONF, "interface %s\n", lan_eth);
	save2file(OSPFD_CONF, "ip ospf cost %s\n", nvram_safe_get("ospf_if_cost0"));	
	save2file(OSPFD_CONF, "ip ospf priority %s\n", nvram_safe_get("ospf_if_priority0"));	
	/* WAN setting */
	DEBUG_MSG("wan_eth=%s setting\n",wan_eth);
	save2file(OSPFD_CONF, "interface %s\n", wan_eth);	
	save2file(OSPFD_CONF, "ip ospf cost %s\n", nvram_safe_get("ospf_if_cost1"));	
	save2file(OSPFD_CONF, "ip ospf priority %s\n", nvram_safe_get("ospf_if_priority1"));			
	/* DMZ setting */
	DEBUG_MSG("dmz_eth=%s setting\n",dmz_eth);
	save2file(OSPFD_CONF, "interface %s\n", dmz_eth);
	save2file(OSPFD_CONF, "ip ospf cost %s\n", nvram_safe_get("ospf_if_cost2"));	
	save2file(OSPFD_CONF, "ip ospf priority %s\n", nvram_safe_get("ospf_if_priority2"));	
	//ospf
	save2file(OSPFD_CONF, 
	"router ospf\n"
	"network %s area %s\n"
	"network %s area %s\n"
	"network %s area %s\n"
	"redistribute connected\n"
	"redistribute kernel\n"
	"redistribute rip\n"
	"access-list private deny any\n"
	, lan_ipmask , nvram_safe_get("ospf_if_area0")
	, wan_ipmask , nvram_safe_get("ospf_if_area1")
	, dmz_ipmask , nvram_safe_get("ospf_if_area2"));
		
	//zebra
	sprintf(buf,"zebra -f %s -d",ZEBRA_CONF);
	DEBUG_MSG("ntp_start_main: buf=%s\n",buf);
	system(buf);		
	//ospfd
	sprintf(buf,"ospfd -f %s -d",OSPFD_CONF);
	DEBUG_MSG("ntp_start_main: buf=%s\n",buf);
	system(buf);
		
	return 0;
}

int stop_zebra_rip(void)
{	
	DEBUG_MSG("%s:\n",__func__);
	system("killall ripd &");
	
	return 0;
}

int stop_zebra_ospf(void)
{
	DEBUG_MSG("%s:\n",__func__);	
	system("killall ospfd &");
	
	return 0;
}

static int dynamic_route_start_main(int argc, char *argv[])
{
	int ret = -1;
	DEBUG_MSG("%s:\n",__func__);
	/* WAN */
#if 0
	if (nvram_match("rip_enable", "1") && !nvram_match("zebra_enable", "1")){					
		ret = start_zebra_rip();
		nvram_set("zebra_enable","1");	
	}
#endif
	if ((NVRAM_MATCH("rip_enable0", "1") ||
	    NVRAM_MATCH("rip_enable1", "1") ||
	    NVRAM_MATCH("rip_enable2", "1"))
    	    && !NVRAM_MATCH("zebra_enable", "1")){					
		ret = start_zebra_rip();
		nvram_set("zebra_enable","1");	
	}

#if 0
	if (nvram_match("ospf_enable", "1")&& !nvram_match("zebra_enable", "1")){			
		ret = start_zebra_ospf();
		nvram_set("zebra_enable","1");		
	}				
#endif
	return ret;
}

static int dynamic_route_stop_main(int argc, char *argv[])
{
	int ret = -1;
	DEBUG_MSG("%s:\n",__func__);
	system("killall zebra &");
	ret = stop_zebra_rip();
	ret |= stop_zebra_ospf();	
	nvram_unset("zebra_enable");
	return ret;
}

static int route_start_main(int argc, char *argv[])
{
	DEBUG_MSG("%s:\n",__func__);
	if (NVRAM_MATCH("rip_enable0", "1") || 
	    NVRAM_MATCH("rip_enable1", "1") || 
	    NVRAM_MATCH("rip_enable2", "1") || 
            NVRAM_MATCH("ospf_enable", "1")) {
		DEBUG_MSG("%s: dyn_route_enable\n",__func__);
		dynamic_route_start_main(argc, argv);
	}
	//policy_route_start();
	return static_route_start_main(argc, argv);
}

static int route_stop_main(int argc, char *argv[])
{
	DEBUG_MSG("%s:\n",__func__);
	dynamic_route_stop_main(argc, argv);
	//policy_route_stop();
	return static_route_stop_main(argc, argv);
}

int route_main(int argc, char *argv[])
{
	struct subcmd cmds[] = {
		{ "start", route_start_main}, /* @ifname, @route_list */
		{ "stop", route_stop_main},
		{ NULL, NULL}
	};

	return lookup_subcmd_then_callout(argc, argv, cmds);
}
