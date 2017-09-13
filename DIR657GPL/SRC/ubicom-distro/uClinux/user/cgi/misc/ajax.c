#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/sysinfo.h>
#include <ctype.h>
#include <time.h>

#include "linux_vct.h"
#include "ssi.h"
#include "libdb.h"
#include "libwlan.h"
#include "shvar.h"
#include "sutil.h"
//#include "vct.h"

#ifndef MAX_RES_LEN_XML
#define MAX_RES_LEN_XML 4096
#endif

static char *header = "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\" ?>" \
		"<env:Envelope " \
		"xmlns:env=\"httpd://www.w3.org/org/2003/05/spap-envlop\" " \ 
		"env:encodingStyle=\"httpd://www.w3.org/org/2003/05/spap-envlop\">" \
		"<env:Body>";
static char *tail = "</env:Body></env:Envelope>";

static struct __ajax_struct {
	const char *param;
	const char *desc;
	int (*fn)(int, char *[]);
};

extern int get_ip(const char *, char *);
extern int get_netmask(const char *, char *);
extern int get_def_gw(char *);
extern int get_dns_srv(char *);
extern char *wan_statue(char *);
extern int get_wan_connect_time(int );
extern int get_wlan_channel(int, char *);

static int get_wireless_channel(char *wlan0_channel, char *wlan1_channel)
{
	get_wlan_channel(0, wlan0_channel);
	if (strcmp(nvram_safe_get("5g"), "1") == 0)
		get_wlan_channel(1, wlan1_channel);
	else
		strcpy(wlan1_channel, "NULL");
	return 0;
}

static int GetCPUInfo(char *usr_status, char *sys_status, char *idle_status, char *io_status)
{
	FILE *fp;
	char buf[160];
	unsigned long long u=0, u_sav=0, u_frme, s=0, s_sav=0, s_frme,n=0, n_sav=0, n_frme, 
			i=0, i_sav, i_frme,w=0, w_sav=0, w_frme, x=0, x_sav=0, x_frme,
			y=0, y_sav, y_frme, z=0, z_sav=0, z_frme,
			total_frme;
	float scale;

	memset(buf, '\0', sizeof(buf));

	if((fp=popen("cat /proc/stat","r")) == NULL)
		goto out;

	/* ex: "CPU:  15% usr  11% sys   0% nic  72% idle   0% io   0% irq   0% sirq "*/
	while(fgets(buf, sizeof(buf), fp)) {
		if(strstr(buf, "cpu")) {
			sscanf(buf, "cpu %Lu %Lu %Lu %Lu %Lu %Lu %Lu %Lu",
				&u, &n, &s, &i, &w, &x, &y, &z);
			break;
		}
	}
	
	memset(buf, '\0', sizeof(buf));
	pclose(fp);
	//usleep(500000);
	sleep(1);

	if((fp=popen("cat /proc/stat","r")) == NULL)
                goto out;

	while(fgets(buf, sizeof(buf), fp)) {
		if(strstr(buf, "cpu")) {
			sscanf(buf, "cpu %Lu %Lu %Lu %Lu %Lu %Lu %Lu %Lu",
				&u_sav, &n_sav, &s_sav, &i_sav, &w_sav, &x_sav, &y_sav, &z_sav) ;
			break;
		}
	}
	pclose(fp);

	u_frme = u_sav - u;
	n_frme = n_sav - n;
	s_frme = s_sav - s;
	i_frme = ( i_sav - i < 0 ? 0: i_sav - i);
	w_frme = w_sav - w;
	x_frme = x_sav - x;
	y_frme = y_sav - y;
	z_frme = z_sav - z;
	total_frme = u_frme + s_frme + n_frme + i_frme + w_frme +x_frme + y_frme + z_frme;
	
	if(total_frme < 1)
		total_frme = 1;
	scale = 100.0 / (float)total_frme;
	//cprintf("XX %5.1f%%us,%5.1f%%sy,%5.1f%%ni,%5.1f%%id,%5.1f%%wa,%5.1f%%hi,%5.1f%%si,%5.1f%%st\n XX",
	//	u_frme * scale, s_frme * scale, n_frme * scale,i_frme * scale, w_frme * scale, x_frme * scale, y_frme * scale, z_frme * scale);
	
	sprintf(usr_status,"%5.0f",u_frme * scale);
	sprintf(sys_status,"%5.0f",s_frme * scale);
	sprintf(idle_status,"%5.0f",i_frme * scale);
	sprintf(io_status,"%5.0f",w_frme * scale);
	
	return 0;
out:
	return -1;
}

static int Getbtime(char *uptime)
{
	FILE *fp;
	char buf[64];
	char start_time[32],idle_time[32];
	unsigned long  btime;
	int day=0,hour=0, min=0;
	
	if((fp = popen("cat /proc/uptime","r")) == NULL)
		goto out;
	
	fgets(buf, sizeof(buf), fp);
	sscanf(buf, "%s %s", start_time,idle_time);

	btime = atoi(start_time);
	day = btime / 86400;
	btime %=86400;
	hour = btime / 3600;
	btime %= 3600;
	min = btime / 60;
	btime %= 60;
	
	if(day == 0)
		sprintf(uptime,"%02d:%02d:%02d",hour,min,btime);
	else if (day == 1)
		sprintf(uptime," %d day, %02d:%02d:%02d",hour,min,btime);
	else
		sprintf(uptime," %d days, %02d:%02d:%02d",hour,min,btime);
	
	pclose(fp);
	return 0;
out:
	return -1;
}

int GetMemInfo(char *total, char *free, char *used)
{
	char mfree[32], mused[32];
	int tmp_m = 0, memfree = 0, ret = 0;

	memset(mfree, '\0', sizeof(mfree));
	memset(mused, '\0', sizeof(mused));

	if (!_GetMemInfo("MemFree:", mfree))
		goto out;
	if ((memfree = strtol(mfree, NULL, 10)) < 3)
                goto out;
	/* It lose 5384 spaces in /proc/meminfo if the memory of DUT is 64MB(65536) */
	tmp_m = 64 - memfree;
	sprintf(mused, "%ld", tmp_m);

	strcpy(used, mused);
	strcpy(free, mfree);
	strcpy(total, "64");
	ret = 1;
out:
	return ret;
}

static int get_network_info(const char *dev, const char *proto, 
				char *ip, char *netmask)
{
	if (strcmp(proto, "dhcpc") == 0 || strcmp(proto, "static") == 0) {
		get_ip(dev, ip);
		get_netmask(dev, netmask);
	} else {
		get_ip("ppp0", ip);
		get_netmask("ppp0", netmask);
	}

	if (strcmp(ip, "") == 0)
		strcpy(ip, "NULL");

	if (strcmp(netmask, "") == 0)
		strcpy(netmask, "NULL");

	return 0;
}

static int get_default_gw(char *default_gw)
{
	char gw[32];
	
	memset(gw, '\0', sizeof(gw));

	get_def_gw(gw);
	if (strcmp(gw, "") == 0 || strlen(gw) == 0)
		strcpy(default_gw, "NULL");
	else
		strcpy(default_gw, gw);

	return 0;
}

static int get_dns(char *p_dns, char *s_dns)
{
	char tmp[256], pdns[64], sdns[64];

	memset(tmp, '\0', sizeof(tmp));
	memset(pdns, '\0', sizeof(pdns));
	memset(sdns, '\0', sizeof(sdns));

	get_dns_srv(tmp);
	sscanf(tmp, "%s %s", &pdns, &sdns);
	if (strcmp(pdns, "") == 0)
		strcpy(p_dns, "NULL");
	else
		strcpy(p_dns, pdns);
	
	if (strcmp(sdns, "") == 0)
		strcpy(s_dns, "NULL");
	else
		strcpy(s_dns, sdns);

	return 0;
}

static int create_device_status(int argc, char *argv[])
{
	char ajax_xml[MAX_RES_LEN_XML];
	char *wan_eth, *wan_proto;
	char tmp[256], ip[64], netmask[64], gw[64], pdns[64], sdns[64], \
	cable_status[64], connection_status[64], wan_timer[64], \
	wlan_channel[64], wlan1_channel[64];
	int network_status = 0;
	char cpu_usr[32],cpu_sys[32],cpu_idle[32],cpu_io[32];
	char mem_total[32]="", mem_free[32]="", mem_used[32]="";
	char btime[32];
	char strKey[10]="";

	memset(ip, '\0', sizeof(ip));
	memset(netmask, '\0', sizeof(netmask));
	memset(gw, '\0', sizeof(gw));
	memset(tmp, '\0', sizeof(tmp));
	memset(pdns, '\0', sizeof(pdns));
	memset(sdns, '\0', sizeof(sdns));
	memset(cable_status, '\0', sizeof(cable_status));
	memset(connection_status, '\0', sizeof(connection_status));
	memset(wan_timer, '\0', sizeof(wan_timer));
	memset(wlan_channel, '\0', sizeof(wlan_channel));
	memset(wlan1_channel, '\0', sizeof(wlan1_channel));
	memset(ajax_xml, '\0', sizeof(ajax_xml));
	memset( btime,'\0', sizeof(btime) );
	memset( mem_total,'\0', sizeof(mem_total) );
	memset( mem_free, '\0', sizeof(mem_free) );
	memset( mem_used, '\0', sizeof(mem_used) );
	memset( strKey, '\0', sizeof(strKey) );
	
	wan_eth = nvram_safe_get("wan_eth");
	wan_proto = nvram_safe_get("wan_proto");

	if (!GetMemInfo(mem_total, mem_free, mem_used))
		goto out;
	
	get_network_info(wan_eth, wan_proto, ip, netmask);
	get_default_gw(gw);
	get_dns(pdns, sdns);
	GetCPUInfo(cpu_usr,cpu_sys,cpu_idle,cpu_io);
	Getbtime(btime);
	get_wireless_channel(wlan_channel, wlan1_channel);

        VCTGetPortConnectState(wan_eth, VCTWANPORT0, cable_status);
	if (strncmp(cable_status, "disconnect", 10) != 0 && strcmp(ip, "NULL") != 0)
		network_status = 1;
	sprintf(connection_status, "%s", wan_statue(wan_proto)); 
	sprintf(wan_timer , "%lu", get_wan_connect_time(1));

	sprintf(ajax_xml, "%s", header);
	sprintf(ajax_xml+strlen(ajax_xml), "<wan_ip>%s</wan_ip>", ip);
	sprintf(ajax_xml+strlen(ajax_xml), "<wan_netmask>%s</wan_netmask>", netmask);
	sprintf(ajax_xml+strlen(ajax_xml), "<wan_default_gateway>%s</wan_default_gateway>", gw);
	sprintf(ajax_xml+strlen(ajax_xml), "<wan_primary_dns>%s</wan_primary_dns>", pdns);
	sprintf(ajax_xml+strlen(ajax_xml), "<wan_secondary_dns>%s</wan_secondary_dns>", sdns);
	sprintf(ajax_xml+strlen(ajax_xml), "<wan_cable_status>%s</wan_cable_status>", cable_status);
        sprintf(ajax_xml+strlen(ajax_xml), "<wan_network_status>%d</wan_network_status>", network_status);
        sprintf(ajax_xml+strlen(ajax_xml), "<wan_connection_status>%s</wan_connection_status>", connection_status);
        sprintf(ajax_xml+strlen(ajax_xml), "<wan_uptime>%s</wan_uptime>", wan_timer);
	sprintf(ajax_xml+strlen(ajax_xml), "<wlan_channel>%s</wlan_channel>", wlan_channel);
	sprintf(ajax_xml+strlen(ajax_xml), "<wlan1_channel>%s</wlan1_channel>", wlan1_channel);
	sprintf(ajax_xml+strlen(ajax_xml), "<uptime>%s</uptime>", btime);
	sprintf(ajax_xml+strlen(ajax_xml), "<cpu_usr>%s</cpu_usr>", cpu_usr);
	sprintf(ajax_xml+strlen(ajax_xml), "<cpu_sys>%s</cpu_sys>", cpu_sys);
	sprintf(ajax_xml+strlen(ajax_xml), "<cpu_idle>%s</cpu_idle>", cpu_idle);
	sprintf(ajax_xml+strlen(ajax_xml), "<cpu_io>%s</cpu_io>", cpu_io);
	sprintf(ajax_xml+strlen(ajax_xml), "<mem_total>%s</mem_total>", mem_total);
	sprintf(ajax_xml+strlen(ajax_xml), "<mem_used>%s</mem_used>", mem_used);
	sprintf(ajax_xml+strlen(ajax_xml), "<mem_free>%s</mem_free>", mem_free);
	sprintf(ajax_xml+strlen(ajax_xml), "%s", tail);

	printf("%s", ajax_xml);
out:
	return 0;
}

static int create_ipv6_status(int argc, char *argv[])
{
	char network_status[15]={0};
	char global_wanip[5*MAX_IPV6_IP_LEN+4]={0};
	char local_wanip[5*MAX_IPV6_IP_LEN+4]={0};
	char global_lanip[MAX_IPV6_IP_LEN]={0};
	char local_lanip[MAX_IPV6_IP_LEN]={0};
	char default_gateway[MAX_IPV6_IP_LEN]={0};
	char *primary_dns=NULL, *secondary_dns=NULL;
	char dns_stream[2*MAX_IPV6_IP_LEN+1]={0};
	char ajax_xml[MAX_RES_LEN_XML + sizeof(global_wanip) + sizeof(local_wanip) + sizeof(global_lanip) + 
		sizeof(local_lanip) + sizeof(default_gateway) + sizeof(dns_stream)] = {0};

	char wan_interface[8] = {};
	char *wan_eth = NULL;
	char *wan_proto = NULL;

	char dhcp_pd_enable[10]={0};
	char dhcp_pd[100]={0};
	char ppp6_ip[64] = {};
	char autoconfig_ip[64] = {};
	wan_proto = nvram_safe_get("ipv6_wan_proto");
	wan_eth = nvram_safe_get("wan_eth");

	if (strcmp(wan_proto, "ipv6_6to4") == 0)
		strcpy(wan_interface, "tun6to4");

	else if (strcmp(wan_proto, "ipv6_6in4") == 0)
		strcpy(wan_interface, "sit1");

	else if (strcmp(wan_proto, "ipv6_pppoe") == 0)
		if(nvram_match("ipv6_pppoe_share", "1") == 0)
			strcpy(wan_interface, "ppp0");
		else
			strcpy(wan_interface, "ppp6");

	else if(strcmp(wan_proto, "ipv6_6rd") == 0)
	        strcpy(wan_interface, "sit2");

	else if(strcmp(wan_proto, "ipv6_autodetect") == 0)
		if(read_ipv6addr("ppp0", SCOPE_GLOBAL, ppp6_ip, sizeof(ppp6_ip))){
			strcpy(wan_interface, "ppp0");
			strcpy(wan_proto, "ipv6_pppoe");
		}else if(read_ipv6addr(wan_eth, SCOPE_GLOBAL, autoconfig_ip, sizeof(autoconfig_ip))){
			strcpy(wan_interface, wan_eth);
			strcpy(wan_proto, "ipv6_autoconfig");
		}else{
			strcpy(wan_interface, wan_eth);
			strcpy(wan_proto, "ipv6_autodetect");
		}
	else
		strcpy(wan_interface, wan_eth);

	/*Phy interface connect state*/
	VCTGetPortConnectState( wan_eth, VCTWANPORT0, network_status);
	if (strncmp("connect", network_status, 7) == 0) {
		if (read_ipv6addr(wan_interface, SCOPE_GLOBAL, global_wanip, sizeof(global_wanip)) == NULL)
			strcpy(network_status, "disconnect");
		/*Special Case : It do not have global address in 6RD #Joe Huang*/
		if (strcmp(wan_proto, "ipv6_6rd") == 0 && access("/var/run/sit2.pid", F_OK) == 0)
			strcpy(network_status,"connect");

		if(strcmp(wan_proto, "ipv6_static") == 0 && nvram_match("ipv6_use_link_local", "1") == 0)
			strcpy(network_status,"connect");

		read_ipv6addr(wan_interface, SCOPE_LOCAL, local_wanip, sizeof(local_wanip));

		/*PPPoE connect or not is depand on link local address*/		
		if(strcmp(wan_proto, "ipv6_pppoe") == 0 && strlen(local_wanip) > 0)
			strcpy(network_status,"connect");

		read_ipv6defaultgatewy(wan_interface, default_gateway);
		if(read_ipv6_dns() != NULL){
			memcpy(dns_stream, read_ipv6_dns(), sizeof(dns_stream));
			getStrtok(dns_stream, "/", "%s %s", &primary_dns, &secondary_dns);
		}
	}

	if (read_ipv6addr(nvram_safe_get("lan_bridge"), SCOPE_GLOBAL, global_lanip, sizeof(global_lanip)) == NULL)
		strcpy(global_lanip,"NULL"); /*avoiding ui get empty string cause show error*/

	if (read_ipv6addr(nvram_safe_get("lan_bridge"), SCOPE_LOCAL, local_lanip, sizeof(local_lanip)) == NULL)
		strcpy(local_lanip,"NULL");

	if (strlen(global_wanip) < 1)
		strcpy(global_wanip,"NULL");

	if (strlen(local_wanip) < 1)
		strcpy(local_wanip,"NULL");

	if (strlen(default_gateway) < 1)
		strcpy(default_gateway,"NULL");

	if ( strcmp(wan_proto, "ipv6_static") == 0 || strcmp(wan_proto, "ipv6_6to4") == 0 || 
		strcmp(wan_proto, "link_local") == 0 || strcmp(wan_proto, "ipv6_6rd") == 0)
		strcpy(dhcp_pd_enable, "Disabled");
	else {
		if (atoi(nvram_safe_get("ipv6_dhcp_pd_enable")))
			strcpy(dhcp_pd_enable, "Enabled");
		else
			strcpy(dhcp_pd_enable, "Disabled");
	}

	if (strcmp(dhcp_pd_enable, "Enabled") == 0) {
		FILE *fp_dhcppd;
	/* Joe Huang : get the address of dhcp-pd form /var/tmp/dhcp_pd	*/
	/* /var/tmp/dhcp_pd generated by dhcp6c  			*/
		fp_dhcppd = fopen ( "/var/tmp/dhcp_pd" , "r" );
		if ( fp_dhcppd )
		{
			char buffer[128] = {};
			if ( fgets(buffer, sizeof(buffer), fp_dhcppd ) ) {
				buffer[strlen(buffer)-1] = '\0'; /*replace '\n'*/
				strcpy(dhcp_pd, buffer);
			}
			if ( fgets(buffer, sizeof(buffer), fp_dhcppd ) ) {
				buffer[strlen(buffer)-1] = '\0'; /*replace '\n'*/
				strcat(dhcp_pd, "/");
				strcat(dhcp_pd, buffer);
			}
			fclose(fp_dhcppd);
		}

		if (strlen(dhcp_pd)==0)
			strcpy(dhcp_pd,"(null)");
	} else
		strcpy(dhcp_pd,"(null)");

	sprintf(ajax_xml, "%s", header);
	sprintf(ajax_xml+strlen(ajax_xml),"<ipv6_wan_proto>%s</ipv6_wan_proto>", wan_proto);
	sprintf(ajax_xml+strlen(ajax_xml),"<ipv6_wan_network_status>%s</ipv6_wan_network_status>", network_status);
	sprintf(ajax_xml+strlen(ajax_xml),"<ipv6_wan_ip>%s</ipv6_wan_ip>", global_wanip);
	sprintf(ajax_xml+strlen(ajax_xml),"<ipv6_wan_ip_local>%s</ipv6_wan_ip_local>", local_wanip);
	sprintf(ajax_xml+strlen(ajax_xml),"<ipv6_lan_ip_global>%s</ipv6_lan_ip_global>", global_lanip);
	sprintf(ajax_xml+strlen(ajax_xml),"<ipv6_lan_ip_local>%s</ipv6_lan_ip_local>", local_lanip);
	sprintf(ajax_xml+strlen(ajax_xml),"<ipv6_wan_default_gateway>%s</ipv6_wan_default_gateway>", default_gateway);
	sprintf(ajax_xml+strlen(ajax_xml),"<ipv6_wan_primary_dns>%s</ipv6_wan_primary_dns>", primary_dns);
	sprintf(ajax_xml+strlen(ajax_xml),"<ipv6_wan_secondary_dns>%s</ipv6_wan_secondary_dns>", secondary_dns);
	sprintf(ajax_xml+strlen(ajax_xml),"<ipv6_dhcp_pd_enable>%s</ipv6_dhcp_pd_enable>", dhcp_pd_enable);
	sprintf(ajax_xml+strlen(ajax_xml),"<ipv6_dhcp_pd>%s</ipv6_dhcp_pd>", dhcp_pd);
	sprintf(ajax_xml+strlen(ajax_xml), "%s", tail);

	printf("%s", ajax_xml);

    return 0;
}

static int auto_ipv6_status(int argc, char *argv[])
{
	FILE *fd;
	int test;
	char ajax_xml[MAX_RES_LEN_XML],buf[128];
	char cmd[32];

	sprintf(cmd, "cat %s", WIZARD_IPV6);
	if ((fd =  popen(cmd, "r")) == NULL)
                goto out;

        while (fgets(buf, sizeof(buf), fd)) {
                buf[strlen(buf) - 1] = '\0';
        }
        pclose(fd);	
	

	sprintf(ajax_xml, "%s", header);
	sprintf(ajax_xml+strlen(ajax_xml),"<ipv6_wan_connect_percentage>%s</ipv6_wan_connect_percentage>", buf);
	sprintf(ajax_xml+strlen(ajax_xml), "%s", tail);

	printf("%s", ajax_xml);
out:
	return 0;
}

static int misc_ajax_help(struct __ajax_struct *p)
{
	cprintf("Usage:\n");

	for (; p && p->param; p++)
		cprintf("\t%s: %s\n", p->param, p->desc);

	cprintf("\n");
	return 0;
}

int ajax_main(int argc, char *argv[])
{
	int ret = -1;
	struct __ajax_struct *p, list[] = {
		{ "device_status", "Create Device Status Information", create_device_status},
		{ "ipv6_status", "Create IPv6 Status Information", create_ipv6_status},
		{ "ipv6_auto_connect", "Auto connect IPv6 status information", auto_ipv6_status},
		{ NULL, NULL, NULL }
	};

	if (argc == 1 || strcmp(argv[1], "help") == 0)
		goto help;

	for (p = list; p && p->param; p++) {
		if (strcmp(argv[1], p->param) == 0) {
			ret = p->fn(argc - 1, argv + 1);
			goto out;
		}
	}
help:
	ret = misc_ajax_help(list);
out:
	return ret;
}
