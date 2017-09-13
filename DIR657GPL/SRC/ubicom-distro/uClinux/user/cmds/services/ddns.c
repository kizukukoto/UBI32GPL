#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <net/if.h>
#include <net/route.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if_arp.h>
#include <net/ethernet.h>
#include <netinet/in.h>
#include "cmds.h"
#include <nvramcmd.h>
#include "shutils.h"

//#define USED_EAVL 1

#if 0
#define DEBUG_MSG(fmt, arg...)       cprintf(fmt, ##arg)
#else
#define DEBUG_MSG(fmt, arg...) 
#endif

/*
 * FIXME: Don't use global varibles here please!
 * I did not think it is necessary.
 * */
static int type = 0;
static char wildcards[64]={0};
static char ddns_dyndns_kinds[64] = {0};

void get_wan_iface(char *iface)
{
	if (NVRAM_MATCH("zone_if_proto2", "dhcpc") ||
		NVRAM_MATCH("zone_if_proto2", "static") ||
		NVRAM_MATCH("zone_if_proto2", "pppoe"))
	{
		strcpy( iface, nvram_safe_get("if_dev2") );

	} else
		strcpy( iface, nvram_safe_get("if_dev3") );
			
	return;
}

char *get_if_ipaddr(char *if_name)
{
	int sockfd;
	struct ifreq ifr;
	struct in_addr temp_ip;

	if ((sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_RAW)) < 0){
		cprintf("get_if_ipaddr: socket fail!");
		return 0;
	}	

	strcpy(ifr.ifr_name, if_name);
	if( ioctl(sockfd, SIOCGIFADDR, &ifr) < 0) {
		cprintf("get_if_ipaddr: ioctl fail!");
		close(sockfd);
		return 0;
	}
	close(sockfd);
	temp_ip.s_addr = sin_addr(&ifr.ifr_addr).s_addr;//KenWu:the line will error

	return inet_ntoa(temp_ip);
}

char *get_if_mask(char *if_name)
{
	int sockfd;
	struct ifreq ifr;
	struct in_addr temp_mask;

	if ((sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_RAW)) < 0){
		printf("get_if_mask: socket fail!");
		return 0;
	}	

	strcpy(ifr.ifr_name, if_name);
	if( ioctl(sockfd, SIOCGIFNETMASK, &ifr) < 0) {
		printf("get_if_mask: socket fail!");
		close(sockfd);
		return 0;
	}

	close(sockfd);
	temp_mask.s_addr = sin_addr(&ifr.ifr_netmask).s_addr;	
	return inet_ntoa(temp_mask);
}

int get_ddns_type(char *ddns_type)
{
	int type = 10;
	
	if (strstr( ddns_type, "DynDns.org" ) || strcmp( ddns_type, "0" ) == 0)
		type = 0;
	else if (strstr( ddns_type, "EasyDns.com" ) || strcmp( ddns_type, "1" ) == 0)
		type = 1;
	else if (strstr( ddns_type, "dynupdate.no-ip.com" ) || strcmp( ddns_type, "2" ) == 0)
		type = 2;
	else if (strstr( ddns_type, "changeip.com" ) || strcmp( ddns_type, "3" ) == 0)
		type = 3;
	else if (strstr( ddns_type, "eurodyndns.com" ) || strcmp( ddns_type, "4" ) == 0)
		type = 4;
	else if (strstr( ddns_type, "ods.com" ) || strcmp( ddns_type, "5" ) == 0)
		type = 5;
	else if (strstr( ddns_type, "ovh.com" ) || strcmp( ddns_type, "6" ) == 0)
		type = 6;
	else if (strstr( ddns_type, "regfish.com" ) || strcmp( ddns_type, "7" ) == 0)
		type = 7;
	else if (strstr( ddns_type, "tzo.com" ) || strcmp( ddns_type, "8" ) == 0)
		type = 8;
	else if (strstr( ddns_type, "zoneedit.com" ) || strcmp( ddns_type, "9" ) == 0)
		type = 9;
	else if (strstr( ddns_type, "dlinkddns.com" ) || strcmp( ddns_type, "10" ) == 0)
		type = 10;
	else if (strstr( ddns_type, "cybergate.com" ) || strcmp( ddns_type, "11" ) == 0)
		type = 11;
	else if (strstr( ddns_type, "oray.ddns.cn" ) || strcmp( ddns_type, "12") == 0)
		type = 12;

	return type;	
}	

char *get_ddns_kinds(char *ddns_type)
{
	if (strstr(ddns_type, "Custom" ))
		return "Custom";
	else if (strstr( ddns_type, "Static" ))
		return "Static";
	else if (strstr( ddns_type, "Dynamic" ))
		return "Dynamic";
	
	return "Dynamic";	
}
/* FIXME: BAD! use gethostbyname() instead 
 *	It seems only want to resolve FQDN!
 * */
int wan_ip_differ_ddns_ip(char *wan_eth)
{
	FILE *fp = NULL;
	char buf[80]={0};
	char wan_ip[16]={0}, host_ip[16]={0};
	char *ip_s=NULL, *ip_e=NULL;
	int rev = -1;
	
	sprintf(buf,"ping -c 1 \"%s\" > /var/tmp/ddns_check.txt 2>&1", nvram_safe_get("ddns_hostname"));
	DEBUG_MSG("wan_ip_differ_ddns_ip: buf=%s\n",buf);
	system(buf);	
	
	strcpy(wan_ip,get_if_ipaddr(wan_eth));
	DEBUG_MSG("wan_ip=%s\n",wan_ip);
	rev = 0;
#if 0
	if ((fp = fopen("/var/tmp/ddns_check.txt", "r")) == NULL) {
		cprintf("open /var/tmp/ddns_check.txt fail");
		goto out;
	} else {		
		memset(buf, 0, 80);
		fgets(buf, sizeof(buf), fp);
		ip_s = strstr(buf, "(");
		ip_e = strstr(buf, ")");
		if (ip_s && ip_e) {
			strncpy(host_ip, ip_s+1, ip_e-(ip_s+1));
			DEBUG_MSG("host_ip=%s\n",host_ip);
			if (strcmp(wan_ip, host_ip)) {
				DEBUG_MSG("wan ip:%s != %s:%s\n", wan_ip, nvram_safe_get("ddns_hostname"), host_ip);
				fclose(fp);
				goto out;
			}	
		}	
		else
			return 1;	
			
		fclose(fp);
	}
#endif
out:
	return	rev;	
}

static int ddns_start_main(int argc, char *argv[])
{
	int ret = -1;
	char wan_eth[32] = {0}, type_tmp[10];
	char buf[512];
	char *ddns_timeout;
	int int_ddns_timeout = 0;
	FILE *fp;

	if (!(NVRAM_MATCH("ddns_status", "Disconnect") && /* FIXME: why ddns_status MUST be Disconnect? */
		NVRAM_MATCH("ddns_enable", "1")))
	{
		goto out;
	}
	DEBUG_MSG("ddns_enable\n");
	/* set ddns_status Connecting */
	nvram_set("ddns_status", "Connecting");			
	system("iptables -I INPUT --proto icmp -j ACCEPT");	

	if (NVRAM_MATCH("ddns_type", "oray.ddns.cn")){
		cprintf("START DDNS: oray \n");
		if((fp = fopen("/tmp/oray.conf", "w+")) != NULL){
			fprintf(fp, "username=%s\n", nvram_safe_get("ddns_username"));
			fprintf(fp, "password=%s\n", nvram_safe_get("ddns_password"));
			fprintf(fp, "register_name=%s\n", nvram_safe_get("ddns_hostname"));
			fclose(fp);
			if (NVRAM_MATCH("ddns_enable", "1")  ){
			#if USED_EVAL
				eval("/sbin/orayddns", "&");
			#else
				_system("/sbin/orayddns &");
			#endif
			}
		}
		ret = 0;
		goto out;
	} else {
		/* FIXME: the @wan_eth should init by argv[XXX] */
		get_wan_iface(wan_eth);
		DEBUG_MSG("wan_eth=%s\n",wan_eth);	
		if (wan_ip_differ_ddns_ip(wan_eth) == -1)
			goto out;

		DEBUG_MSG("differ_ddns_ip\n");
		type = get_ddns_type(nvram_safe_get("ddns_type"));
		DEBUG_MSG("type=%d\n",type);
	
		if (NVRAM_MATCH("ddns_wildcards_enable", "1") && (type == 0))
			strcpy(wildcards,"-w");
		else
			strcpy(wildcards," ");	
		DEBUG_MSG("wildcards=%s\n",wildcards);
	
		strcpy(ddns_dyndns_kinds,get_ddns_kinds(nvram_safe_get("ddns_type")));
		DEBUG_MSG("ddns_dyndns_kinds=%s\n",ddns_dyndns_kinds);			
	
		ddns_timeout = nvram_safe_get("ddns_timeout"); /* XXX: ddns_timeout=""?*/
		int_ddns_timeout = atoi(ddns_timeout);
		if (int_ddns_timeout <= 0)
			int_ddns_timeout = 86400;//1 day 86400 secs

		#if USED_EAVL
		sprintf( type_tmp,"%d",type);
		ret = eval("noip2", "-T", type_tmp, "-R", nvram_safe_get("ddns_hostname"),
			"-u", nvram_safe_get("ddns_username"), "-p", nvram_safe_get("ddns_password"),
			wildcards, "-I" ,wan_eth , "-k", ddns_dyndns_kinds,
			"-a" , nvram_safe_get("model_number") ,"-n" , nvram_safe_get("model_name")
			, "-U" , int_ddns_timeout
			, "&"
			);
		#else					
		sprintf( buf,"/sbin/noip2 -T %d -R %s -u %s -p %s %s -I %s -k %s -a %s -n %s -U %d &",
			type, nvram_safe_get("ddns_hostname"),
			nvram_safe_get("ddns_username"),
			nvram_safe_get("ddns_password"),
			wildcards, wan_eth, ddns_dyndns_kinds,
			nvram_safe_get("model_number"),
			nvram_safe_get("model_name") , int_ddns_timeout
			);
		DEBUG_MSG("ddns_enable: buf=%s\n",buf);
		system(buf);
		ret = 0;
		#endif
	}

out:
	DEBUG_MSG("ddns_start_main:done:%d\n", ret);
	return ret;
}

static int ddns_stop_main(int argc, char *argv[])
{
	int ret = -1;	

	/* set ddns_status Connecting */
	nvram_set("ddns_status", "Disconnect");
	system("iptables -D INPUT --proto icmp -j ACCEPT");


#if USED_EAVL
	if (NVRAM_MATCH("ddns_type", "oray.ddns.cn"))
		ret = eval("killall", "orayddns");
	else
		ret = eval("killall", "noip2");
#else
	if (NVRAM_MATCH("ddns_type", "oray.ddns.cn"))
		_system("killall orayddns &");
	else
		system("killall noip2 &");	
	ret = 0;
#endif	
	DEBUG_MSG("ddns_stop_main: done\n");
	return ret;
}

/*
 * 
 */
int ddns_restore_main(int argc, char *argv[])
{
	nvram_set("ddns_status", "Disconnect");
}

int ddns_main(int argc, char *argv[])
{
	struct subcmd cmds[] = {
		{ "start", ddns_start_main},
		{ "stop", ddns_stop_main},
		{ "restore", ddns_restore_main},
		{ NULL, NULL}
	};

	return lookup_subcmd_then_callout(argc, argv, cmds);
}
