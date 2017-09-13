#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <net/if.h>
#include <net/ethernet.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/sysinfo.h>
#include <mtd/mtd-user.h>
#include <netinet/ip.h>
#include <netinet/ip6.h>
#include <linux/if_packet.h>
#include <linux/if_ether.h>
#include "ipv6.h"
#include "nvramcmd.h"
/*
 * Shared utility Debug Messgae Define
 */
#ifdef SUTIL_DEBUG
#define DEBUG_MSG(fmt, arg...)       printf("%s [%d]: "fmt, __FILE__, __LINE__, ##arg)
#else
#define DEBUG_MSG(fmt, arg...)
#endif

/* Init File and clear the content */
int init_file(char *file)
{
	FILE *fp;

	if ((fp = fopen(file ,"w")) == NULL) {
		printf("Can't open %s\n", file);
		exit(1);
	}

	fclose(fp);
	return 0;
}


/* Save data into file
 * Notice: You must call init_file() before save2file()
 */
void save2file(const char *file, const char *fmt, ...)
{
	char buf[10240];
	va_list args;
	FILE *fp;

	if ((fp = fopen(file ,"a")) == NULL) {
		printf("Can't open %s\n", file);
		exit(1);
	}

	va_start(args, fmt);
	vsnprintf(buf, sizeof(buf), fmt, args);
	va_end(args);
	va_start(args, fmt);
	fprintf(fp, "%s", buf);
	va_end(args);

	fclose(fp);
}

/*
 *	*st: dest string
 *	insert[]: insert string
 *	start: start location
 */
void string_insert(char *st, char insert[], int start) {
	int i, st_length, insert_length;

	st_length = strlen(st);
	insert_length = strlen(insert);
	if (st_length <= start) return;

	for (i = st_length ; i >= start ; i--) {
		st[i + insert_length] = st[i];
	}

	for (i = 0 ; i < insert_length ; i++) {
		st[start + i] = insert[i];
	}
}

/************************************************************************************************/
/*ipv6_addr:ipv6 address*************************************************************************/
/*length: prefix length (the function could get right prefix with length in 16 or 16's multiple)*/
/*return prefix of ipv6 address (Network ID******************************************************/
/************************************************************************************************/
char *get_ipv6_prefix (char *ipv6_addr, int length){

	int colon;      //colon account in the ipv6 address
	char *instr = "";	//insert pointer
	char *index = "";
	char *next = "";
	char ipv6_clone[64]={};
	int i;

	strcpy(ipv6_clone,ipv6_addr);
	index = ipv6_clone;
	colon=0;
	if((length%16))
		length = length/16 +1;
	else
		length = length/16;


	//Gathering colon's account
	while((index = strstr(index, ":")))
	{
		++colon;
		if(next != NULL)
		{
			if(index-next ==1) instr = next;
		}
		next = index;
		index++;
	}

	if(colon !=7){
		for(i=0; i< 8-colon; i++){
			if(i==0) string_insert(instr,"0", 1);
			else string_insert(instr,"0:", 1);
		}
		DEBUG_MSG("After regular:%s\n", ipv6_clone);
	}
	//Find out prefix
	index = ipv6_clone;
	i=0;
	while((index = strstr (index, ":"))){
		i++;
		if(i == length){
			*index ='\0';
			DEBUG_MSG("Prefix:%s\n", ipv6_clone);
			return ipv6_clone;
		}
		++index;
	}
}
/*
 * Reason: get dns from /var/etc/resolv_ipv6.conf
 * Notice: reference from httpd/htppd_utility.c
 */
char *read_dns(void)
{
	FILE *fp;
	char *buff;
	char dns[3][45]={};
	static char return_dns[3*45] ="";
	int num, i=0, len=0;

	fp = fopen (RESOLVFILE_IPV6, "r");
	if (!fp)
	{
		perror(RESOLVFILE_IPV6);
		return return_dns;
	}

	buff = (char *) malloc(1024);
	if (!buff)
	{	fclose(fp);
		return return_dns;
	}

	memset(buff, 0, 1024);

	while( fgets(buff, 1024, fp))
	{
		if (strstr(buff, "nameserver") )
		{
			num = strspn(buff+10, " ");
			len = strlen( (buff + 10 + num) );

			if(strchr(buff, '\n'))//Line Feed
				strncpy(dns[i], (buff+10+num), len-1);
			else
				strncpy(dns[i], (buff+10+num), len);

			DEBUG_MSG("strlen(buff)=%d, len=%d, dns[%d]=%s\n",
				strlen(buff), len, i, dns[i]);
			i++;
		}
		if(3<=i)
		{
			strcat(dns[2], "\0");
			break;
		}
	}
	free(buff);
	
	if (strlen(dns[0])) {
		sprintf(return_dns, "%s", dns[0]);
	}
	
	if (strlen(dns[1])) {
		if (strlen(return_dns))
			strcat(return_dns, ",");
		strcat(return_dns, dns[1]);
	}
	fclose(fp);
	return return_dns;
}

void write_dhcpd6_conf (int pd_len)
{
        char *tmp_nvram_value;
        char *v6_wan;
	printf("start write_dhcpd6_conf\n");
	init_file (DHCPD6_CONF_FILE);
	init_file (DHCPD6_GUEST_CONF_FILE);
	v6_wan = nvram_safe_get("ipv6_wan_proto");

	if(strlen(v6_wan) == 0)
		goto finish;

	char dummy = 0x0;
	char *dns1 = &dummy;
	char *dns2 = &dummy;
	char *return_dns = "";
	char primary_dns[50];
	char secondary_dns[50];
	char dns_enable[50];
	char *pos;

	FILE *fp_dhcppd;
	struct in6_addr pd_pf;
	char prefix[64];
	int i = 0;
	char buffer[128] = {};
	int guest_zone = 0;

	if(pd_len < 64 && ifconfirm("ath1"))
		guest_zone = 1;
		
	// set dns_enable = ipv6_XXX_dns_enable
	strcpy(dns_enable, v6_wan);
	strcat(dns_enable, "_dns_enable");
	
	// set primary_dns, secondary_dns = ipv6_XXX_primary_dns, ipv6_XXX_secondary_dns
	strcpy(primary_dns, v6_wan);
	strcpy(secondary_dns, v6_wan);
	strcat(primary_dns, "_primary_dns");
	strcat(secondary_dns, "_secondary_dns");
		
	// set dns_enable = nvram(ipv6_XXX_dns_enable)
	tmp_nvram_value = nvram_safe_get(dns_enable);
		
	// if nvarm(dns_enable)="" or nvram(dns_enable)=1
	if ( strcmp(tmp_nvram_value, "")==0 || atoi(tmp_nvram_value)){
		dns1 = nvram_safe_get(primary_dns);
		dns2 = nvram_safe_get(secondary_dns);
	} else if ( atoi(tmp_nvram_value)==0){
		return_dns = read_dns();
		if (strcmp(return_dns,"")!=0){
			dns1 = return_dns;
			pos = strchr(return_dns, ',');
		if(pos) {
			*pos = '\0';
			dns2 = pos+1;
			}
		}
	}

	if(strlen(dns1) || strlen(dns2))
		save2file(DHCPD6_CONF_FILE, "option domain-name-servers %s %s;\n", dns1, dns2);

	if(NVRAM_MATCH("ipv6_autoconfig_type", "stateful")){
		save2file(DHCPD6_CONF_FILE, "interface %s {\n", nvram_safe_get("lan_bridge"));
		save2file(DHCPD6_CONF_FILE, "	address-pool pool1 %d;\n",
			 60 * atoi(nvram_safe_get("ipv6_dhcpd_lifetime")));
		save2file(DHCPD6_CONF_FILE,"};\n");

		save2file(DHCPD6_CONF_FILE, "pool pool1 {\n");
		save2file(DHCPD6_CONF_FILE, "	range %s to %s;\n", 
			nvram_safe_get("ipv6_dhcpd_start"),nvram_safe_get("ipv6_dhcpd_end"));
		save2file(DHCPD6_CONF_FILE,"};\n");
		if(guest_zone){
			save2file(DHCPD6_GUEST_CONF_FILE, "option domain-name-servers %s %s;\n", dns1, dns2);
			save2file(DHCPD6_GUEST_CONF_FILE, "interface ath1 {\n");
			save2file(DHCPD6_GUEST_CONF_FILE, "	address-pool pool2 %d;\n",
				 60 * atoi(nvram_safe_get("ipv6_dhcpd_lifetime")));
			save2file(DHCPD6_GUEST_CONF_FILE,"};\n");

			save2file(DHCPD6_GUEST_CONF_FILE, "pool pool2 {\n");
			save2file(DHCPD6_GUEST_CONF_FILE, "	range %s to %s;\n", 
				nvram_safe_get("ipv6_guest_dhcpd_start"),nvram_safe_get("ipv6_guest_dhcpd_end"));
			save2file(DHCPD6_GUEST_CONF_FILE,"};\n");
		}
	}

	/*Joe Huang : DHCP-PD configuration for LAN side in spec 1.14R*/
	/*XXX host name should include in the "X"(you can chang it to a B c but not 1,2,3) to determine that is start and end word*/
	/*The DUID name MUST over 224 bits linklock+time type, I made it to 320 bits, I think it is enough.*/
	/*The host should include {DUID,PREFIX,ADDR-POOL(option)} */
	/*I made the PD lifetime to infinity. Change it when the new spec define it after 1.14R. */

	fp_dhcppd = fopen ( DHCP_PD , "r" );
	if ( fp_dhcppd && NVRAM_MATCH("ipv6_dhcp_pd_enable_l", "1") && pd_len < 63){
		if(fgets(prefix, sizeof(prefix), fp_dhcppd ))
			prefix[strlen(prefix)-1] = '\0';
		fclose(fp_dhcppd);
	}else 
		goto finish;

	inet_pton(AF_INET6, prefix, &pd_pf);
	if(pd_len <= 56){
		for( i = 1 ; i < 16 ; i++){
			if(pd_len % 8 == 0)
				pd_pf.s6_addr[pd_len/8] += 16;
			if(pd_len % 8 == 1)
				pd_pf.s6_addr[pd_len/8] += 8;
			if(pd_len % 8 == 2)
				pd_pf.s6_addr[pd_len/8] += 4;
			if(pd_len % 8 == 3)
				pd_pf.s6_addr[pd_len/8] += 2;
			if(pd_len % 8 == 4)
				pd_pf.s6_addr[pd_len/8] += 1;
			if(pd_len % 8 == 5){
				if(i % 2 == 0){
					pd_pf.s6_addr[pd_len/8] += 1;
					pd_pf.s6_addr[pd_len/8+1] = 0;
				}else
					pd_pf.s6_addr[pd_len/8+1] += 128;
			}
			if(pd_len % 8 == 6){
				if(i % 4 == 0){
					pd_pf.s6_addr[pd_len/8] += 1;
					pd_pf.s6_addr[pd_len/8+1] = 0;
				}else
					pd_pf.s6_addr[pd_len/8+1] += 64;
			}
			if(pd_len % 8 == 7){
				if(i % 8 == 0){
					pd_pf.s6_addr[pd_len/8] += 1;
					pd_pf.s6_addr[pd_len/8+1] = 0;
				}else
					pd_pf.s6_addr[pd_len/8+1] += 32;
			}
			inet_ntop(AF_INET6, &pd_pf, prefix, sizeof(prefix));
			save2file(DHCPD6_CONF_FILE, "host X%dX {\n", i);
			save2file(DHCPD6_CONF_FILE, 
				"	duid 00:00:00:00:00:00:00:00:00:00:00:00:00:00:00:00:00:00:00:00;\n");
			if(NVRAM_MATCH("ipv6_autoconfig_type", "stateful"))
				save2file(DHCPD6_CONF_FILE, "	address-pool pool1 %d;\n",
					 60 * atoi(nvram_safe_get("ipv6_dhcpd_lifetime")));
			save2file(DHCPD6_CONF_FILE, "	prefix %s/%d infinity;\n", prefix, pd_len + 4);
			save2file(DHCPD6_CONF_FILE,"};\n");
		}

	}else if(57 <= pd_len && pd_len <= 59){
		for(i = 1 ; i < 8 ; i++){
			if(pd_len == 57)
				pd_pf.s6_addr[pd_len/8] += 16;
			if(pd_len == 58)
				pd_pf.s6_addr[pd_len/8] += 8;
			if(pd_len == 59)
				pd_pf.s6_addr[pd_len/8] += 4;
			inet_ntop(AF_INET6, &pd_pf, prefix, sizeof(prefix));
			save2file(DHCPD6_CONF_FILE, "host X%dX {\n", i);
			save2file(DHCPD6_CONF_FILE, 
				"	duid 00:00:00:00:00:00:00:00:00:00:00:00:00:00:00:00:00:00:00:00;\n");
			if(NVRAM_MATCH("ipv6_autoconfig_type", "stateful"))
				save2file(DHCPD6_CONF_FILE, "	address-pool pool1 %d;\n",
					 60 * atoi(nvram_safe_get("ipv6_dhcpd_lifetime")));
			save2file(DHCPD6_CONF_FILE, "	prefix %s/%d infinity;\n", prefix, pd_len + 3);
			save2file(DHCPD6_CONF_FILE,"};\n");
		}
	}else if(60 <= pd_len && pd_len <= 61){
		for(i = 1 ; i < 4 ; i++){
			if(pd_len == 60)
				pd_pf.s6_addr[pd_len/8] += 4;
			if(pd_len == 61)
			pd_pf.s6_addr[pd_len/8] += 2;
			inet_ntop(AF_INET6, &pd_pf, prefix, sizeof(prefix));
			save2file(DHCPD6_CONF_FILE, "host X%dX {\n", i);
			save2file(DHCPD6_CONF_FILE, 
				"	duid 00:00:00:00:00:00:00:00:00:00:00:00:00:00:00:00:00:00:00:00;\n");
			if(NVRAM_MATCH("ipv6_autoconfig_type", "stateful"))
				save2file(DHCPD6_CONF_FILE, "	address-pool pool1 %d;\n",
					 60 * atoi(nvram_safe_get("ipv6_dhcpd_lifetime")));
			save2file(DHCPD6_CONF_FILE, "	prefix %s/%d infinity;\n", prefix, pd_len + 2);
			save2file(DHCPD6_CONF_FILE,"};\n");
		}

	}else if(pd_len == 62){
		pd_pf.s6_addr[pd_len/8] += 2;
		inet_ntop(AF_INET6, &pd_pf, prefix, sizeof(prefix));
		save2file(DHCPD6_CONF_FILE, "host X%dX {\n", i);
		save2file(DHCPD6_CONF_FILE, 
			"	duid 00:00:00:00:00:00:00:00:00:00:00:00:00:00:00:00:00:00:00:00;\n");
		if(NVRAM_MATCH("ipv6_autoconfig_type", "stateful"))
			save2file(DHCPD6_CONF_FILE, "	address-pool pool1 %d;\n",
				 60 * atoi(nvram_safe_get("ipv6_dhcpd_lifetime")));
		save2file(DHCPD6_CONF_FILE, "	prefix %s/%d infinity;\n", prefix, pd_len + 1);
		save2file(DHCPD6_CONF_FILE,"};\n");
	}

finish:
	printf("finish write_dhcpd6_conf\n");
}

char *get_link_local_ip_l(char *interface_name){
	FILE *fp;
	char buf[1024]={},link_local_ip[128]={};
	char *start,*end;
	init_file(LINK_LOCAL_INFO);
	if(strcmp(interface_name,"LAN") == 0)
		_system("ifconfig %s > %s",nvram_safe_get("lan_bridge"),LINK_LOCAL_INFO);
	else if(strcmp(interface_name,"WAN") == 0)
		_system("ifconfig %s > %s",nvram_safe_get("wan_eth"),LINK_LOCAL_INFO);
	fp = fopen(LINK_LOCAL_INFO,"r+");
	if(fp)
	{
		while(start = fgets(buf,1024,fp))
		{
			if(end = strstr(buf,"Scope:Link"))
			{
				end = end - 1;
				if(start = strstr(start,"addr:"))
				{
					start = start + 5;
					strncpy(link_local_ip,start,end - start);
					return link_local_ip;
				}
			}
		}
		fclose(fp);
	}
	return "ERROR:Can not get LINK local address";
}

#ifdef MRD_ENABLE
void write_mrd_conf(void)
{
	init_file(MRD_CONF_FILE);
	save2file(MRD_CONF_FILE,"load-module mld;\n");
	save2file(MRD_CONF_FILE,"groups {\n");
	save2file(MRD_CONF_FILE,"ff0e::/16 {\n");
	save2file(MRD_CONF_FILE,"}\n");
	save2file(MRD_CONF_FILE,"}\n");
}
#endif

void write_radvd_conf(int pd_len)
{
	printf("write_radvd_conf_cmd start\n");
	char *ra_interface_l;
	char *nvram_tmp_value;
	char *v6_wan_proto;
	int guest_zone = 0;

	ra_interface_l = nvram_safe_get("lan_bridge");
	init_file(RADVD_CONF_FILE);

ipv6_guest:
	if(nvram_match("ipv6_ra_interface","LAN") == 0)
	{
		if(guest_zone)
			save2file(RADVD_CONF_FILE,"interface ath1 {\n");
		else
			save2file(RADVD_CONF_FILE,"interface %s {\n",ra_interface_l);
		save2file(RADVD_CONF_FILE,"AdvSendAdvert on;\n");
		save2file(RADVD_CONF_FILE,"MaxRtrAdvInterval %s;\n",nvram_safe_get("ipv6_ra_maxrtr_advinterval_l"));
		save2file(RADVD_CONF_FILE,"MinRtrAdvInterval %s;\n",nvram_safe_get("ipv6_ra_minrtr_advinterval_l"));
		save2file(RADVD_CONF_FILE,"AdvCurHopLimit %s;\n",nvram_safe_get("ipv6_ra_adv_cur_hoplimit_l"));
		save2file(RADVD_CONF_FILE,"AdvManagedFlag %s;\n",nvram_safe_get("ipv6_ra_adv_managed_flag_l"));

/* Joe Huang : Support RDNSS+DHCPv6 DNS at spec. 1.14 */
		if(NVRAM_MATCH("ipv6_autoconfig_type", "stateless"))
			save2file(RADVD_CONF_FILE,"AdvOtherConfigFlag %s;\n",nvram_safe_get("ipv6_ra_adv_other_config_flag_l"));
		else
			save2file(RADVD_CONF_FILE,"AdvOtherConfigFlag on;\n");
		save2file(RADVD_CONF_FILE,"AdvDefaultLifetime %s;\n",nvram_safe_get("ipv6_ra_adv_default_lifetime_l"));
		save2file(RADVD_CONF_FILE,"AdvReachableTime %s;\n",nvram_safe_get("ipv6_ra_adv_reachable_time_l"));
		save2file(RADVD_CONF_FILE,"AdvRetransTimer %s;\n",nvram_safe_get("ipv6_ra_adv_retrans_timer_l"));
		save2file(RADVD_CONF_FILE,"AdvLinkMTU %s;\n",nvram_safe_get("ipv6_ra_adv_link_mtu_l"));
		if(strlen(nvram_safe_get("ipv6_ra_prefix64_l_one")) != 0)
		{
			if(guest_zone)
				save2file(RADVD_CONF_FILE,"prefix %s::/64{\n",nvram_safe_get("ipv6_guest_ra_prefix64_l_one"));
			else
				save2file(RADVD_CONF_FILE,"prefix %s::/64{\n",nvram_safe_get("ipv6_ra_prefix64_l_one"));
			save2file(RADVD_CONF_FILE,"AdvOnLink %s;\n",nvram_safe_get("ipv6_ra_adv_onlink_flag_l_one"));
			save2file(RADVD_CONF_FILE,"AdvAutonomous %s;\n",nvram_safe_get("ipv6_ra_adv_autonomous_flag_l_one"));
			save2file(RADVD_CONF_FILE,"AdvValidLifetime %s;\n",nvram_safe_get("ipv6_ra_adv_valid_lifetime_l_one"));
			save2file(RADVD_CONF_FILE,"AdvPreferredLifetime %s;\n",nvram_safe_get("ipv6_ra_adv_prefer_lifetime_l_one"));
			save2file(RADVD_CONF_FILE,"};\n");
		}
		if(strlen(nvram_safe_get("ipv6_ra_prefix64_l_two")) != 0)
		{
			if(guest_zone)
				save2file(RADVD_CONF_FILE,"prefix %s::/64{\n",nvram_safe_get("ipv6_guest_ra_prefix64_l_two"));
			else
				save2file(RADVD_CONF_FILE,"prefix %s::/64{\n",nvram_safe_get("ipv6_ra_prefix64_l_two"));
			save2file(RADVD_CONF_FILE,"AdvOnLink %s;\n",nvram_safe_get("ipv6_ra_adv_onlink_flag_l_two"));
			save2file(RADVD_CONF_FILE,"AdvAutonomous %s;\n",nvram_safe_get("ipv6_ra_adv_autonomous_flag_l_two"));
			save2file(RADVD_CONF_FILE,"AdvValidLifetime %s;\n",nvram_safe_get("ipv6_ra_adv_valid_lifetime_l_two"));
			save2file(RADVD_CONF_FILE,"AdvPreferredLifetime %s;\n",nvram_safe_get("ipv6_ra_adv_prefer_lifetime_l_two"));
			save2file(RADVD_CONF_FILE,"};\n");
		}
		/*
		 * Reason: add rdnss option.
		 */
		v6_wan_proto = nvram_safe_get("ipv6_wan_proto");
		if (strlen(v6_wan_proto) && !NVRAM_MATCH("ipv6_autoconfig_type", "stateful"))
		{
			char dummy = 0x0;
			char *dns1 = &dummy;
			char *dns2 = &dummy;
			char *return_dns = "";
			char primary_dns[50];
			char secondary_dns[50];
			char dns_enable[50];
			char *pos;
			
			// set dns_enable = ipv6_XXX_dns_enable
			strcpy(dns_enable, v6_wan_proto);
			strcat(dns_enable, "_dns_enable");
			
			// set primary_dns, secondary_dns = ipv6_XXX_primary_dns, ipv6_XXX_secondary_dns
			strcpy(primary_dns, v6_wan_proto);
			strcpy(secondary_dns, v6_wan_proto);
			strcat(primary_dns, "_primary_dns");
			strcat(secondary_dns, "_secondary_dns");
			
			// set dns_enable = nvram(ipv6_XXX_dns_enable)
			nvram_tmp_value = nvram_safe_get(dns_enable);
			
			// if nvarm(dns_enable)="" or nvram(dns_enable)=1
			if ( strcmp(nvram_tmp_value, "")==0 || atoi(nvram_tmp_value))
			{
				dns1 = nvram_safe_get(primary_dns);
				dns2 = nvram_safe_get(secondary_dns);
			} else if ( atoi(nvram_tmp_value)==0)
			{
				return_dns = read_dns();
				if (strcmp(return_dns,"")!=0)
				{
                                	dns1 = return_dns;
	                                pos = strchr(return_dns, ',');
        	                        if(pos) {
                	                        *pos = '\0';
                        	                dns2 = pos+1;
					}
				}
			}

			if(strlen(dns1) || strlen(dns2))
			{
				save2file(RADVD_CONF_FILE,"RDNSS %s %s{\n", dns1, dns2);
				save2file(RADVD_CONF_FILE,"AdvRDNSSPreference 8;\n");
				save2file(RADVD_CONF_FILE,"AdvRDNSSOpen off;\n");
				save2file(RADVD_CONF_FILE,"AdvRDNSSLifetime %d;\n",2*atoi(nvram_safe_get("ipv6_ra_maxrtr_advinterval_l")));//default: 2*MaxRtrAdvInterval
				save2file(RADVD_CONF_FILE,"};\n");
			}
		}//if(strlen(v6_wan))

		save2file(RADVD_CONF_FILE,"};\n");
	}

	if(pd_len < 64 && ifconfirm("ath1") && !guest_zone){
		guest_zone = 1;
		goto ipv6_guest;
	}

	printf("write radvd_conf_cmd finished\n");
}

void write_ipv6_pppoe_options(void)
{
	printf("write_ipv6_pppoe_options start\n");
	init_file(IPv6PPPOE_OPTIONS_FILE);
	
	char username[] = "ipv6_pppoe_username";
	char default_gateway[16] = {};
	char *use_peerdns="";

	if(!nvram_match("ipv6_wan_specify_dns", "0"))
		use_peerdns="usepeerdns";

	if( nvram_match("ipv6_wan_proto", "ipv6_pppoe") ==0 ) 
	{

		memset(default_gateway,0,sizeof(default_gateway));
		
		strcpy(default_gateway,"defaultroute");
			
		// debug option is need, i don't know why the pppd write to file need this option
		init_file(IPv6PPPOE_OPTIONS_FILE);
		save2file(IPv6PPPOE_OPTIONS_FILE,
				"plugin /lib/pppd/2.4.3/rp-pppoe.so\n"
				"login\n"
				"noauth\n"
				"debug\n"
				"refuse-eap\n"
				"noipdefault\n"
				"defaultroute\n"
				"passive\n"
				"lcp-echo-interval 30\n"
				"lcp-echo-failure 4\n"
				"%s\n"
				"maxfail 3\n"
				"user \"%s\"\n"
				"mtu  %s\n"
				"mru  %s\n"
				"remotename %s\n"
				"novj \n"
				"nopcomp \n"
				"noaccomp \n"
				"-am\n"
				"noccp \n"
				"noip \n"
				"ipv6 , \n",
			use_peerdns,\
				nvram_safe_get(username),\
				nvram_safe_get("ipv6_pppoe_mtu"),\
				nvram_safe_get("ipv6_pppoe_mtu"),\
				CHAP_REMOTE_NAME);
	}
	
	
	printf("write_ipv6_pppoe_options finished\n");
}

char *parse_special_char(char *p_string)
{
	char hex[400], hex_tmp[200];
	int len = 0, c1 = 0, c2 = 0;

	memset(hex, 0, sizeof(hex));
	memset(hex_tmp, 0, sizeof(hex_tmp));
	len = strlen(p_string);
	strcpy(hex_tmp, p_string);

	for(c1 = 0; c1 < len; c1++)
	{
		/*   ` = 0x60, " = 0x22, \ = 0x5c, ' = 0x27, $ = 0x24       */		

		if((hex_tmp[c1] == 0x22) || (hex_tmp[c1] == 0x5c) || (hex_tmp[c1] == 0x60) || (hex_tmp[c1] == 0x24))
		{
			hex[c2] = 0x5c;
			c2++;
			hex[c2] = hex_tmp[c1];
		}	
		else
			hex[c2] = hex_tmp[c1];
		c2++;		
	}

	strcpy(p_string, hex);
	return p_string;
}

void write_ipv6_pppoe_sercets(char *file)
{
	char name[64];
	char psw[64];
	init_file(file);
	strncpy(name, nvram_safe_get("ipv6_pppoe_username"), sizeof(psw));
	strncpy(psw, nvram_safe_get("ipv6_pppoe_password"), sizeof(psw));
	save2file(file, "\"%s\" \"%s\" \"%s\" *\n", 
		parse_special_char(name), CHAP_REMOTE_NAME, parse_special_char(psw) );
	save2file(file, "\"%s\" \"%s\" \"%s\" *\n", 
		CHAP_REMOTE_NAME, parse_special_char(name), parse_special_char(psw) );
}

char *read_ipv6addr(const char *if_name,const int scope, char *ipv6addr,const int length)
{
        FILE *fp;
        char buf[1024]={};
        char *start=NULL,*end=NULL;
        int num_flag=0;

        if(!ipv6addr)
                return NULL;

	ipv6addr[0] = '\0';
        init_file(LINK_LOCAL_INFO);
        _system("ifconfig %s 2>&- > %s", if_name,LINK_LOCAL_INFO);
        fp = fopen(LINK_LOCAL_INFO,"r+");

        if(!fp)
                return NULL;

        while(start = fgets(buf,1024,fp)){
                if(scope== SCOPE_GLOBAL)
                        end = strstr(buf,"Scope:Global");
                else if(scope== SCOPE_LOCAL)
                        end = strstr(buf,"Scope:Link");
                if(end){
                        end = end - 1;
                        if(start = strstr(start,"addr:")){
                                start = start + 5 + 1; //skip space
                                if(strlen(ipv6addr)+(end - start)>=length){
                                        break;
                                }
                                if(num_flag==0){
                                        strncpy(ipv6addr,start,end - start);
                                        num_flag=1;
                                }else{
                                        strcat(ipv6addr,",");
                                        strncat(ipv6addr,start,end - start);
                                }
                        }
                }
        }
        fclose(fp);

	if (num_flag)
		return ipv6addr;
	else
		return NULL;
}

void copy_mac_to_eui64(char *mac, char *ifid)
{
        ifid[8] = mac[0];
        ifid[8] ^= 0x02; /* reverse the u/l bit*/
        ifid[9] = mac[1];
        ifid[10] = mac[2];
        ifid[11] = 0xff;
        ifid[12] = 0xfe;
        ifid[13] = mac[3];
        ifid[14] = mac[4];
        ifid[15] = mac[5];
}

void trigger_send_rs(char *interface)
{
        char ipv6_lla[50] = {};

        if ( read_ipv6addr(interface, SCOPE_LOCAL, ipv6_lla, sizeof(ipv6_lla))){
                _system("ip -6 addr flush dev %s", interface);
                _system("ip -6 addr add %s dev %s", ipv6_lla, interface);
	}
}

void set_default_accept_ra(int flag)
{
        if (flag) {
                system("echo 1 > /proc/sys/net/ipv6/conf/all/accept_ra");
                system("echo 2 > /proc/sys/net/ipv6/conf/default/accept_ra");
        } else {
                system("echo 0 > /proc/sys/net/ipv6/conf/default/accept_ra");
                system("echo 0 > /proc/sys/net/ipv6/conf/all/accept_ra");
        }
}

int set_ipv6_specify_dns(void)
{
        char ipv6_dns[50] = {};
        char *ipv6_wan_proto = nvram_safe_get("ipv6_wan_proto");

        sprintf(ipv6_dns, "%s_primary_dns", ipv6_wan_proto);
        save2file(RESOLVFILE_IPV6,"nameserver %s\n", nvram_safe_get(ipv6_dns));

        sprintf(ipv6_dns, "%s_secondary_dns", ipv6_wan_proto);
        save2file(RESOLVFILE_IPV6,"nameserver %s\n", nvram_safe_get(ipv6_dns));

        return 0;
}

int check_ipv6_wan_proto(void)
{
	char *ipv6_wan_proto = nvram_safe_get("ipv6_wan_proto");
	
	if(strcmp(ipv6_wan_proto, "ipv6_autodetect") == 0)
		return AUTODETECT;
	else if(strcmp(ipv6_wan_proto, "ipv6_static") == 0)
		return STATIC;
	else if(strcmp(ipv6_wan_proto, "ipv6_autoconfig") == 0)
		return AUTOCONFIG;
	else if(strcmp(ipv6_wan_proto, "ipv6_pppoe") == 0)
		return PPPOE;
	else if(strcmp(ipv6_wan_proto, "ipv6_6in4") == 0)
		return _6IN4;
	else if(strcmp(ipv6_wan_proto, "ipv6_6to4") == 0)
		return _6TO4;
	else if(strcmp(ipv6_wan_proto, "ipv6_6rd") == 0)
		return _6RD;
	else if(strcmp(ipv6_wan_proto, "link_local") == 0)
		return LINKLOCAL;
}

int ifconfirm(char *if_name)
{
	FILE *fp;
	char cmd[32];
	char buf[128];
	sprintf(cmd, "ifconfig %s", if_name);
	if(fp = popen(cmd, "r"))
		if(fgets(buf, sizeof(buf), fp))
			if(!strstr(buf, "Device not found"))
				return 1;
	return 0;

}

int write_static_pd()
{
	char static_routing[]="static_ipv6_routing_XYXXXXX";
	char *enable = NULL, *name = NULL, *value = NULL, *dest_addr = NULL;
	char *dest_prefix = NULL, *gateway = NULL, *ifname = NULL, *metric = NULL;
	char route_value[256] = {0};
	int i;
	int j = 16;
	int have_static_pd = 0;

	if(!(NVRAM_MATCH("ipv6_autoconfig","1") && 
		(!NVRAM_MATCH("ipv6_autoconfig_type","stateless") || NVRAM_MATCH("ipv6_dhcp_pd_enable_l", "1"))))
		init_file (DHCPD6_CONF_FILE);

	for(i = 0; i < MAX_STATIC_ROUTINGV6_NUMBER; i++,j++) {
                memset(route_value,0,sizeof(route_value));
                sprintf(static_routing, "static_routing_ipv6_%02d", i);
                value = nvram_safe_get(static_routing);

                if ( strlen(value) == 0 )
                        continue;
                else
                        strcpy(route_value, value);

                enable = strtok(route_value, "/");
                name = strtok(NULL, "/");
                dest_addr = strtok(NULL, "/");
                dest_prefix = strtok(NULL, "/");
                gateway = strtok(NULL, "/");
                ifname = strtok(NULL, "/");
                metric = strtok(NULL, "/");

                if( enable == NULL || name == NULL || dest_addr == NULL ||
                        dest_prefix == NULL || gateway == NULL || ifname == NULL )
                        continue;
                /*XXX Joe Huang : Add this condition to avoid the gateway is empty would made token error*/
                else if ( (strcmp(gateway, "NULL") == 0 || strcmp(gateway, "DHCPPD") == 0) &&
                        metric == NULL ){
                        metric = ifname;
                        ifname = gateway;
                }

		if(strcmp(enable ,"1") == 0 && strcmp(ifname, "DHCPPD") == 0){
			save2file(DHCPD6_CONF_FILE, "host X%dX {\n", j);
			save2file(DHCPD6_CONF_FILE, 
				"	duid 00:00:00:00:00:00:00:00:00:00:00:00:00:00:00:00:00:00:00:00;\n");
			if(NVRAM_MATCH("ipv6_autoconfig_type", "stateful") && NVRAM_MATCH("ipv6_autoconfig", "1"))
				save2file(DHCPD6_CONF_FILE, "	address-pool pool1 %d;\n",
					60 * atoi(nvram_safe_get("ipv6_dhcpd_lifetime")));
			save2file(DHCPD6_CONF_FILE, "	prefix %s/%s infinity;\n", dest_addr, dest_prefix);
			save2file(DHCPD6_CONF_FILE,"};\n");
			have_static_pd ++;
		}
	}
	if(have_static_pd){
		printf("Write static DHCP-PD in dhcpd6.conf\n");
		return 1;
	}else
		return 0;
}
