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

extern struct action_flag action_flags;

//#define RC_DEBUG 1
#ifdef RC_DEBUG
#define DEBUG_MSG(fmt, arg...)       printf("%s [%d]: "fmt, __FILE__, __LINE__, ##arg)
#else
#define DEBUG_MSG(fmt, arg...)
#endif

static char ori_wan_ipaddr[2][16]={{0}};
static char wan_ipaddr[2][16]={{0}};
static char wan_if[2][10]={{0}};
static char bri_eth[4];

#define FOR_EACH_WAN_IP for(k=0;k<2;k++)\
							if(strlen(wan_ipaddr[k]))

#define FOR_EACH_WAN_FACE for(k=0;k<2;k++)\
							if(strlen(wan_if[k]))

#define FW_FLUSH	"/var/tmp/fw_flush"

int lanip_4thoctet_plus1(void)
{
	char tmp_lan_ipaddr[16];
	static char lan_ipaddr[16];
	char *ipaddr[4];
	int tmp_fourth, i;

	memset(lan_ipaddr, 0, sizeof(lan_ipaddr));
	strcpy(tmp_lan_ipaddr, nvram_safe_get("lan_ipaddr"));

	for(i = 0; i < 4; i++){
		if(i == 0)
			ipaddr[i] = strtok(tmp_lan_ipaddr, ".");
		else
			ipaddr[i] = strtok(NULL, ".");
	}

	tmp_fourth = atoi(ipaddr[3]);

	if(tmp_fourth >= 254)
		return -1;

	tmp_fourth = tmp_fourth + 1;

	return tmp_fourth;
}

int bcastip_4thoctet_minus1(void)
{
	char *ipaddr[4];
	int tmp_fourth, i;
	FILE *fp;
	char buf[128];
	char bcast_ip[32];
	char *start,*end;

	_system("ifconfig %s > %s", nvram_safe_get("lan_bridge"), LAN_BRIDGE_INFO);

	memset(bcast_ip, 0, sizeof(bcast_ip));

	if((fp = fopen(LAN_BRIDGE_INFO, "r+")) == NULL){
		printf("fail to open LAN_BRIDGE_INFO\n");
		return -1;
	}
	else{
		while(fgets(buf, 128, fp))
		{
			if(strstr(buf, "Bcast") != NULL){
				start = strstr(buf, "cast:") + 5;
				end = strstr(start, "Mask");
				end = end - 2;
				strncpy(bcast_ip, start, end - start);
				break;
			}
		}
	}

	fclose(fp);

	for(i = 0; i < 4; i++){
		if(i == 0)
			ipaddr[i] = strtok(bcast_ip, ".");
		else
			ipaddr[i] = strtok(NULL, ".");
	}

	tmp_fourth = atoi(ipaddr[3]);

	tmp_fourth = tmp_fourth - 1;

	return tmp_fourth;
}

char * get_lanip_3_octets(void){

	char tmp_lan_ipaddr[16];
	static char lan_ipaddr[16];
	char *ipaddr[4];
	int i;

	memset(lan_ipaddr, 0, sizeof(lan_ipaddr));
	strcpy(tmp_lan_ipaddr, nvram_safe_get("lan_ipaddr"));

	for(i = 0; i < 4; i++){
		if(i == 0)
			ipaddr[i] = strtok(tmp_lan_ipaddr, ".");
		else
			ipaddr[i] = strtok(NULL, ".");
	}

	for(i = 0; i < 3; i++){
		sprintf(lan_ipaddr, "%s%s%s", lan_ipaddr, ipaddr[i], ".");
	}
	return lan_ipaddr;
}

void set_unnumber_snat(void){
	int i, unnumber_ip_start, unnumber_ip_end;
	char * tmp_ip, ip[32];

	unnumber_ip_start = lanip_4thoctet_plus1();
	unnumber_ip_end = bcastip_4thoctet_minus1();

	tmp_ip = get_lanip_3_octets();

	for(i = unnumber_ip_start; i <= unnumber_ip_end; i++){
		sprintf(ip, "%s%d", tmp_ip, i);
		save2file(FW_NAT, "-I POSTROUTING -s %s -j SNAT --to-source %s\n", ip, ip);
	}
}

#ifdef AR9100
#ifdef IP8000
#define NAT_LIB_PATH "/lib/modules/2.6.36+/kernel/net/ipv4/netfilter"
#define CONNTRACK_LIB_PATH "/lib/modules/2.6.36+/kernel/net/netfilter"
#else
/* Alper (Ubicom) changed module paths, Aug 18, 2009 */
#define NAT_LIB_PATH "/lib/modules/2.6.28.10/kernel/net/ipv4/netfilter"
#define CONNTRACK_LIB_PATH "/lib/modules/2.6.28.10/kernel/net/netfilter"
#endif
/* -------------------------------------*/
#endif

char *schedule2iptables(char *schedule_name)
{
	static char  schedule_value[100]={0};
	char value[128] = {0}, week[32]={0};
	char *nvram_ptr=NULL;
	char *name = NULL, *weekdays = NULL, *start_time = NULL, *end_time = NULL;
	char schedule_list[]="schedule_rule_00";
	char *weekday[] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
	int i=0, day, count=0;

	if ( (schedule_name == NULL) || (strcmp ("Always", schedule_name ) == 0) || (strcmp(schedule_name, "") == 0) )
		return "";

	/*	Schedule format
	 *	name/weekdays/start_time/end_time
	 *	Test/1101011/08:00/18:00
	 *  only valid for PRE_ROUTING, LOCAL_IN, FORWARD and OUTPUT
	 */
	for(i=0; i<MAX_SCHEDULE_NUMBER; i++)
	{
		snprintf(schedule_list, sizeof(schedule_list), "schedule_rule_%02d", i);
		nvram_ptr = nvram_safe_get(schedule_list);
		if ( nvram_ptr == NULL || strcmp(nvram_ptr, "") == 0)
			continue;
		strncpy(value,nvram_ptr,sizeof(value));
		if( getStrtok(value, "/", "%s %s %s %s", &name, &weekdays, &start_time, &end_time) !=4)
			continue;

		/* Add , symbol between date */
		if ( strcmp(name, schedule_name) == 0) {

			for(day=0; day<7; day++) {
				if ( *(weekdays + day) == '1') {
					count += 1;

					if( count >1 ) {
						strcat(week, ",");
						count -=1;
					}
					strcat(week, weekday[day] );
				}
			}

			/* If start_time = end_time or start_tiem=00:00 and end_time=24:00, it means all day */
			if((strcmp(start_time, end_time) == 0) || ( !strcmp(start_time,"00:00") && !strcmp(end_time, "24:00")))
				sprintf(schedule_value, "-m time --weekdays %s", week);
			else
				sprintf(schedule_value, "-m time --timestart %s --timestop %s --weekdays %s", start_time, end_time, week);
			return schedule_value;
		}
	}
	return "";
}

#ifdef INBOUND_FILTER
int inbound2iptables(char *target_name, char *inbound_action, struct IpRange *inbound_ip)
{
	char inbound_name[]="inbound_filter_name_XX";
	char inbound_ip_A[]="inbound_filter_ip_XX_A";
	char inbound_ip_B[]="inbound_filter_ip_XX_B";
	char *value=NULL, *name = NULL, *action = NULL, *enable=NULL, *ip_range=NULL;
	char *ip[8]={NULL};
	char name_value[128]={0};
	char ip_A_value[190]={0};
	char ip_B_value[190]={0};
	int ip_count=0, i=0, j=0;

	/*	inbound format
	 *  inbound_filter_name_xy = name/action
	 *  inbound_filter_ip_xy_A = enable/ip_range_1,enable/ip_range_2,enable/ip_range_3,enable/ip_range_4
	 *  inbound_filter_ip_xy_B = enable/ip_range_5,enable/ip_range_6,enable/ip_range_7,enable/ip_range_8
	 *
	 *	inbound_filter_name_00 = Test/allow
	 *  inbound_filter_ip_00_A = 1/1.1.1.0-1.1.1.1,0/2.2.2.0-2.2.2.2,0/3.3.3.0-3.3.3.3,1/4.4.4.0-4.4.4.4
	 *  inbound_filter_ip_00_B = 0/5.5.5.0-5.5.5.5,1/6.6.6.0-6.6.6.6,1/7.7.7.0-7.7.7.7,0/8.8.8.0-8.8.8.8
	 */
	for(i=0; i<MAX_INBOUND_FILTER_NUMBER; i++)
	{
		snprintf(inbound_name, sizeof(inbound_name), "inbound_filter_name_%02d", i);
		value = nvram_get(inbound_name);
		if ( value == NULL || strcmp(value, "") == 0)
			continue;
		strcpy(name_value, value);
		getStrtok(name_value, "/", "%s %s", &name, &action);

		/* Found the inbound filter */
		if ( !strcmp(name, target_name))
		{
			/* Save action */
			strcpy(inbound_action,action);

			/* Get the corresponding ip_range */
			snprintf(inbound_ip_A, sizeof(inbound_ip_A), "inbound_filter_ip_%02d_A", i);
			value = nvram_get(inbound_ip_A);
			if ( value == NULL || strcmp(value, "") == 0)
				continue;
			strcpy(ip_A_value, value);
			getStrtok(ip_A_value, ",", "%s %s %s %s", &ip[0], &ip[1], &ip[2], &ip[3]);

			snprintf(inbound_ip_B, sizeof(inbound_ip_B), "inbound_filter_ip_%02d_B", i);
			value = nvram_get(inbound_ip_B);
			if ( value == NULL || strcmp(value, "") == 0)
				continue;
			strcpy(ip_B_value, value);
			getStrtok(ip_B_value, ",", "%s %s %s %s", &ip[4], &ip[5], &ip[6], &ip[7]);

			/* Only save valid ip_range*/
			for(j=0; j<8; j++)
			{
				getStrtok(ip[j], "/", "%s %s", &enable, &ip_range);
				if(!strcmp(enable,"1"))
				{
					strcpy((inbound_ip + ip_count)->addr , ip_range);
					ip_count++;
				}
			}
			return ip_count;
		}
	}
	return 0;
}
#endif

/* Replace '-' with ':' . For example, replace "20-30,40" with "20:30,40" */
void removeMark(char* port_range)
{
	while(port_range=strchr(port_range,'-'))
	{
		*port_range=':';
	}
	return;
}

#ifdef ACCESS_CONTROL
/* LOG ONLY not only log web site, but also allow src_machine to access all web sites. */
int log_only(int src_count, struct IpRange *src_machine, char *schedule, char* bri_eth , char *wan_eth, int for_other)
{
	int i=0;
	char* url="ALL%1"; /* ALL => all url, % => log , 1 => allow(ACCEPT)*/

	/*NickChou add for_other flag
		If for_other=1 => This AccessControl log_only rule is about other machine setting
		If for_other=0 => This AccessControl log_only rule is about IP(MAC) setting
	*/

	if(!for_other)
	{
		for(i=0; i<src_count; i++)
			save2file(FW_FILTER,"-A FORWARD -i %s -o %s %s -m urlfilter --url %s %s -j LOG   --log-level info --log-prefix \"Log Web Access Only:\"\n",
						bri_eth, wan_eth, src_machine+i, url, schedule2iptables( schedule ));

		/*
		 * Reason: Fix D-Track HQ20110722000009
		 * Modified: Yufa Huang
		 * Date: 2011.07.27

		Restricted Website
			-www.facebook.com
			-www.friendster.com
			-www.jobstreet.com.sg

		Access Control
			Policy 1 - Block all PC from accessing those restricted website 
			Policy 2 - Allow an IP 192.168.0.101 to access to All Websites without any restriction. 
		*/
		for(i=0; i<src_count; i++)
			save2file(FW_FILTER,"-A FORWARD -i %s -o %s %s -j ACCEPT\n",
						bri_eth, wan_eth, src_machine+i);
	}
	else
	{
		int j=0;;
		char ac_rule_name[]="access_control_XX";
		char ac_value[128]={};
		char *src[MAX_SRC_MACHINE_NUMBER]={NULL};
		char *enable=NULL, *name=NULL, *src_machine=NULL;
		char *schedule_name=NULL, *log=NULL, *filter_action=NULL;

		/*NickChou Add
		Because other machine need to log,
		Whether url_filter rule need to log or not, url_filter rule jump to ACCEPT chain early.
		( If url_filter rule need log => add rule in url_filter() )
		( block_all rule do not have log requirement )
		So I add "-j ACCEPT" for url_filter rule before "-m urlfilter --url ALL%1".
		*/

		for(i=0; i<MAX_ACCESS_CONTROL_NUMBER; i++)
		{
			snprintf(ac_rule_name, sizeof(ac_rule_name), "access_control_%02d", i);
			strcpy(ac_value, nvram_safe_get(ac_rule_name));

			if( getStrtok(ac_value, "/", "%s %s %s %s %s %s", &enable, &name, &src_machine, &filter_action, &log, &schedule_name) != 6 )
				continue;

			if( strcmp(enable, "1") == 0 && strcmp(filter_action, "url_filter")==0 )
			{
				printf("log_only: set other machine: find other url_filter rule !!!!\n");

				/* for each src machine */
				getStrtok(src_machine, ",", "%s %s %s %s %s %s %s %s",
					&src[0], &src[1], &src[2], &src[3], &src[4], &src[5], &src[6], &src[7]);

				for(j=0; j<MAX_SRC_MACHINE_NUMBER; j++)
				{
					if(src[j])
					{
						if(strchr(src[j],'.'))/* src machine = ip*/
						{
							save2file(FW_FILTER,"-A FORWARD -i %s -o %s -s %s %s -j LOG   --log-level info --log-prefix \"Log Web Access Only:\"\n",
								bri_eth, wan_eth, src[j], schedule2iptables( schedule ));
						}
						else if(strchr(src[j],':'))/* src machine = mac*/
						{
							save2file(FW_FILTER,"-A FORWARD -i %s -o %s -m mac --mac-source %s %s -j LOG   --log-level info --log-prefix \"Log Web Access Only:\"\n",
								bri_eth, wan_eth, src[j], schedule2iptables( schedule ));
						}
					}
				}
			}
		}
		/*other machine => log only */
/*
		save2file(FW_FILTER,"-A FORWARD -i %s -o %s -m urlfilter --url %s %s -j ACCEPT\n",
			bri_eth, wan_eth, url, schedule2iptables( schedule ));
*/
	}
	return 1;
}
#endif //#ifdef ACCESS_CONTROL

#ifdef ACCESS_CONTROL
/*For Access Control block all access */
int block_all(int src_count, struct IpRange *src_machine, char *schedule, char* bri_eth , char *wan_eth, int for_other)
{
	int i=0;

	/*NickChou add for_other flag
		If for_other=1 => This AccessControl block_all rule is about other machine setting
		If for_other=0 => This AccessControl block_all rule is about IP(MAC) setting
	*/

	if(!for_other)
	{
		for(i=0; i<src_count; i++)
		{
			printf("src[%d]=ip:ip_range[%d].addr=%s\n",i, i, src_machine[i].addr);
			save2file(FW_FILTER,"-A FORWARD -i %s %s %s -j LOG_AND_DROP\n",
				bri_eth,src_machine+i, schedule2iptables( schedule ));
		}
	}
	else
	{
		int i=0, j=0;;
		char ac_rule_name[]="access_control_XX";
		char ac_value[128]={};
		char *src[MAX_SRC_MACHINE_NUMBER]={NULL};
		char *enable=NULL, *name=NULL, *src_machine=NULL;
		char *schedule_name=NULL, *log=NULL, *filter_action=NULL;

		/*NickChou Add
		Because other machine need to blocl all access,
		but url_filter rule only need to bolck some access, log_only rule do not nedd to block.
		( If url_filter rule need block some access => add rule in url_filter() )
		( log_only rule do not have block requirement )
		=> Packets pass url_filter rule jump to ACCEPT chain early.
		So I add "-j ACCEPT" for url_filter rule before "-j DROP".
		*/

		for(i=0; i<MAX_ACCESS_CONTROL_NUMBER; i++) // to find url_filter or log_only rule
		{
			snprintf(ac_rule_name, sizeof(ac_rule_name), "access_control_%02d", i) ;
			strcpy(ac_value, nvram_safe_get(ac_rule_name));

			if( getStrtok(ac_value, "/", "%s %s %s %s %s %s", &enable, &name, &src_machine, &filter_action, &log, &schedule_name) != 6 )
				continue;

			if( strcmp(enable, "1") == 0 && strcmp(filter_action, "url_filter")==0 )
			{
				printf("block_all: set other machine: find other url_filter rule !!!!\n");

				/* for each src machine */
				getStrtok(src_machine, ",", "%s %s %s %s %s %s %s %s",
					&src[0], &src[1], &src[2], &src[3], &src[4], &src[5], &src[6], &src[7]);

				for(j=0; j<MAX_SRC_MACHINE_NUMBER; j++)
				{
					if(src[j])
					{
						if(strchr(src[j],'.'))/* src machine = ip*/
						{
							save2file(FW_FILTER,"-A FORWARD -i %s -o %s -s %s %s -j ACCEPT\n",
								bri_eth, wan_eth, src[j], schedule2iptables( schedule ));
						}
						else if(strchr(src[j],':'))/* src machine = mac*/
						{
							save2file(FW_FILTER,"-A FORWARD -i %s -o %s -m mac --mac-source %s %s -j ACCEPT\n",
								bri_eth, wan_eth, src[j], schedule2iptables( schedule ));
						}
					}
				}
			}
		}
		/*other machine => block all */
		save2file(FW_FILTER,"-A FORWARD -i %s -o %s %s -j LOG_AND_DROP\n", bri_eth, wan_eth, schedule2iptables( schedule ));
	}

	return 1;
}
#endif //#ifdef ACCESS_CONTROL

#ifdef ACCESS_CONTROL
/*For Access Control block some access */
int url_filter(int src_count, struct IpRange *src_machine, char *schedule, char* bri_eth , char *wan_eth, char *log, int for_other)
{
	char action_1[32]={0};
	int i=0,j=0,k=0;
	char fw_rule[]="fw_XXXXXXXXXXXXXXXXXXXXXXXXXX";
	char fw_value[192]={0};
	char *getValue=NULL;
	char *needLogFlag="";
	char *acceptFlag_1="";
	char *url_domain_filter_type=nvram_safe_get("url_domain_filter_type");

	DEBUG_MSG("url_filter for Access Control block some access\n");
	if( (strcmp(url_domain_filter_type, "list_allow") ==0) || (strcmp(url_domain_filter_type, "list_deny") ==0) )
	{
		/* Decide action */
		if ( strcmp(url_domain_filter_type, "list_allow") ==0  )
		{
			DEBUG_MSG("url_domain_filter_type==list_allow\n");
			strcpy(action_1,"ACCEPT");
			if(!strcmp(log,"yes"))
			{
				/* If we need to log, the information must be appended into url, for example "google%1".*/
				needLogFlag="%"; //to log it.
				acceptFlag_1="1"; //allow log
			}
		}
		else
		{
			DEBUG_MSG("url_domain_filter_type==list_deny\n");
#ifdef IP8000
			strcpy(action_1,"REJECT --reject-with tcp-reset");
#else
			strcpy(action_1,"DROP");
#endif
			if(!strcmp(log,"yes"))
			{
				needLogFlag="%";
				acceptFlag_1="0";
			}
		}

		for(i=0; i<MAX_URL_FILTER_NUMBER; i++)
		{
			snprintf(fw_rule, sizeof(fw_rule), "url_domain_filter_%02d", i);
			getValue = nvram_get(fw_rule);
			if ( getValue == NULL ||  strlen(getValue) == 0 )
				continue;
			else
			{
				/* if url_rule = http://tw.yahoo.com
				   iptables command will ignore the key word "http" but keep the "://"
				   finally, it becomes url_rule://tw.yahoo.com, (will not work) */
				if(strstr(getValue,"http://") != NULL)
					strcpy(fw_value, getValue + strlen("http://"));
				else
					strcpy(fw_value, getValue);
			}
			for( k = 0 ; k < strlen(fw_value); k++ ){
				fw_value[k] = tolower( fw_value[k] );
			}
			
			DEBUG_MSG("fw_value=%s,src_count=%d\n", fw_value, src_count);

			/*NickChou add for_other flag
			If for_other=1 => This AccessControl url_filter rule is about other machine setting
			If for_other=0 => This AccessControl url_filter rule is about IP(MAC) setting
			*/
			if(!for_other)
			{
				for(j=0; j<src_count; j++)
				{
					save2file(FW_FILTER,"-A FORWARD -i %s -o %s %s -m urlfilter --url \"%s%s%s\" %s -j %s\n",
						bri_eth, wan_eth, src_machine+j, fw_value, needLogFlag, acceptFlag_1, schedule2iptables( schedule ), action_1);
				}
			}
			else //NickChou add for other machine
			{
#ifdef IP8000
				save2file(FW_FILTER,"-A FORWARD -i %s -p tcp -o %s -m urlfilter --url \"%s%s%s\" %s -j %s\n",
						bri_eth, wan_eth, fw_value, needLogFlag, acceptFlag_1, schedule2iptables( schedule ), action_1);
#else
				save2file(FW_FILTER,"-A FORWARD -i %s -o %s -m urlfilter --url \"%s%s%s\" %s -j %s\n",
						bri_eth, wan_eth, fw_value, needLogFlag, acceptFlag_1, schedule2iptables( schedule ), action_1);
#endif
			}
		}

		if (strcmp(url_domain_filter_type, "list_allow")==0)
		{
			/* If URL filter is list_allow, deny all http connections as default rule */
			printf("access control: url_filter: deny all http connections as default rule\n");
			for(j=0; j<src_count; j++) {
				save2file(FW_FILTER,"-A FORWARD -i %s -o %s %s -m urlfilter --url ALL -j LOG_AND_DROP\n", bri_eth, wan_eth, src_machine+j);
			}
		}

	}
	return 0;
}
#endif //#ifdef ACCESS_CONTROL

#ifdef ACCESS_CONTROL
void firewall_rule(int ac_index, int src_count, struct IpRange *src_machine, char *schedule)
{
	char *enable, *name, *iface_src, *protocol, *src_ip_start, *src_ip_end, *iface_dest, *dest_ip_start, *dest_ip_end, *dest_port_start, *dest_port_end, *schedule_name, *act;
	char ip_range_cmd[50],port_range_cmd[20];
	char fw_rule[]="fw_XXXXXXXXXXXXXXXXXXXXXXXXXX";
	char fw_value[192]={0};
	char *getValue;
	int i=0,j=0,k;
	int firewall_rule_index=0;

	for(i=0; i<8; i++)
	{
		firewall_rule_index = i +  ac_index*8;
		snprintf(fw_rule, sizeof(fw_rule), "firewall_rule_%02d",firewall_rule_index) ;
		getValue = nvram_get(fw_rule);

		if ( getValue == NULL ||  strlen(getValue) == 0 )
			continue;
		else
			strncpy(fw_value, getValue, sizeof(fw_value));

		if( getStrtok(fw_value, "/", "%s %s %s %s %s %s %s %s %s %s %s %s %s", &enable, &name, &iface_src, &protocol, &src_ip_start, &src_ip_end, \
                    &iface_dest, &dest_ip_start, &dest_ip_end, &dest_port_start, &dest_port_end, &schedule_name, &act) !=13 )
			continue;

		if(strcmp(enable, "1") == 0)
		{
			/* dest_ip range
			 * all = 0.0.0.0-255.255.255.255
			 * all = 0.0.0.0-0.0.0.0
			 */
			if (!strcmp(dest_ip_start, "0.0.0.0") && !strcmp(dest_ip_end, "0.0.0.0"))
				sprintf(ip_range_cmd,"-m iprange --dst-range 0.0.0.0-255.255.255.255");
			else
				sprintf(ip_range_cmd,"-m iprange --dst-range %s-%s",dest_ip_start ,dest_ip_end);

			/* dest port range
			 * all = 0-65535
			 * all = 0-0
			 */
			if(strcmp(protocol,"ICMP"))
			{
				if ((!strcmp(dest_port_start, "0")) || (!strcmp(dest_port_end, "0")) )
					sprintf(port_range_cmd,"--dport 0:65535");
				else
					sprintf(port_range_cmd,"--dport %s:%s",dest_port_start,dest_port_end);
			}

			/* Write down iptables command for each src machine */
			for(j=0; j<src_count; j++)
			{
				if( (!strcmp(protocol,"TCP")) || (!strcmp(protocol,"Any")) )
				{
					FOR_EACH_WAN_FACE					
					save2file(FW_FILTER, "-A FORWARD -o %s -p TCP %s %s %s %s -j LOG_AND_DROP\n", \
						wan_if[k], src_machine+j, ip_range_cmd, port_range_cmd, schedule2iptables(schedule) );
				}
				if( (!strcmp(protocol,"UDP")) || (!strcmp(protocol,"Any")) )
				{
					FOR_EACH_WAN_FACE
					save2file(FW_FILTER, "-A FORWARD -o %s -p UDP %s %s %s %s -j LOG_AND_DROP\n", \
						wan_if[k], src_machine+j, ip_range_cmd, port_range_cmd, schedule2iptables(schedule) );
				}
				if( (!strcmp(protocol,"ICMP")) || (!strcmp(protocol,"Any")) )
				{
					FOR_EACH_WAN_FACE
					save2file(FW_FILTER, "-A FORWARD -o %s -p ICMP %s %s %s -j LOG_AND_DROP\n", \
						wan_if[k], src_machine+j, ip_range_cmd, schedule2iptables(schedule) );
				}
			}
		}
	}
}
#endif //#ifdef ACCESS_CONTROL

/* return 0: the word is NOT a digit-number
 * return 1: the word is a digit-number
 */
int isNumber(char *word)
{
	int i=0;
	for(i=0;*(word+i)!='\0';i++)
	{
		if(!isdigit(*(word+i)))
		{
			return 0;
		}
	}
	return 1;
}

/* return 0: fail
 * return 1: success
 */
int transferProtoToNumber(char* protocol, char* number)
{
	if(!strcmp(protocol,"TCP"))
		strcpy(number, "6");
	else if(!strcmp(protocol,"UDP"))
		strcpy(number, "17");
	else if(!strcmp(protocol,"Both"))
		strcpy(number, "256");
	else if(!strcmp(protocol,"Any"))
		strcpy(number, "257");
	else
	{
		if(isNumber(protocol))
			strcpy(number, protocol);
		else
			return 0;
	}
	return 1;
}

/*
 	Date: 2009-1-05
 	Name: Ken_Chiang
 	Reason: modify for blocked url to redirect the reject page by crowdcontrol(http proxy).
 	Notice :
*/
#ifdef CONFIG_USER_CROWDCONTROL
#define CONFIG_USER_CROWDCONTROL_DEBUG 1
#ifdef CONFIG_USER_CROWDCONTROL_DEBUG
#define PROXY_DEBUG_MSG(fmt, arg...)       printf("%s [%d]: "fmt, __FILE__, __LINE__, ##arg)
#else
#define PROXY_DEBUG_MSG(fmt, arg...)
#endif
#define WEBFILTER_PROXY_PORT	3128
#define BLOCK_DOMAINS_FILE "/var/tmp/blocked-domains"
#define PERMIT_DOMAINS_FILE "/var/tmp/premit-domains"
#define FILTER_LOG_IP_FILE "/var/tmp/filter-log-ips"
#endif

void flush_iptables(void)
{
	/*
	 * Get the name of the bridge interface
	 */
	strcpy(bri_eth, nvram_safe_get("lan_bridge"));

	init_file(FW_FLUSH);
	save2file(FW_FLUSH,
		"# nat table\n"
		"*nat\n"
		":PREROUTING ACCEPT [0:0]\n"
		":POSTROUTING ACCEPT [0:0]\n"
		":OUTPUT ACCEPT [0:0]\n"
		"COMMIT\n"
		"# mangle table\n"
		"*mangle\n"
		":PREROUTING ACCEPT [0:0]\n"
		":INPUT ACCEPT [0:0]\n"
		":FORWARD ACCEPT [0:0]\n"
		":OUTPUT ACCEPT [0:0]\n"
		":POSTROUTING ACCEPT [0:0]\n");

	/*
	 * Pick up from where we left off
	 */
	save2file(FW_FLUSH,
		"COMMIT\n"
		"# filter table\n"
		"*filter\n"
		":INPUT ACCEPT [0:0]\n"
		":FORWARD DROP [0:0]\n"
		":OUTPUT ACCEPT [0:0]\n"
		"COMMIT\n",
		bri_eth, bri_eth);

	_system("iptables-restore %s", FW_FLUSH);

	/* between "wan becoming up" and "netfilter rules activation"
		drop all packets from wan.
	*/
	int k;
	FOR_EACH_WAN_FACE
		_system("iptables -A INPUT -i %s -j DROP\n", wan_if[k]);
}

void clear_firewall(void)
{
	DEBUG_MSG("Clear iptables and firewall setting\n");

	/*Disable linux ip forward function*/
	system("echo 0 > /proc/sys/net/ipv4/ip_forward");

	/* Normally, iptable are flushed when it goes through start_firewall() overall.
	 * However, in the case of wan_ip=NULL,
	 * it will break out in the meddle of start_firewall()
	 * and the old iptable still exists if it has been created before.
	 * So, I flush it at the beganning anyway.
	 */
	flush_iptables();

/* 	Date: 2009-1-05
 	Name: Ken_Chiang
 	Reason: modify for blocked url to redirect the reject page by crowdcontrol(http proxy).
 	Notice :
*/
#ifdef CONFIG_USER_CROWDCONTROL
	DEBUG_MSG("Clear firewall CROWDCONTROL\n");
	KILL_APP_SYNCH("crowdcontrol");
	init_file(PERMIT_DOMAINS_FILE);
	init_file(BLOCK_DOMAINS_FILE);
#endif

#ifdef WAKE_ON_LAN
	DEBUG_MSG("Clear firewall WAKE_ON_LAN\n");
	KILL_APP_SYNCH("wakeOnLanProxy");
#endif
}

/*	Date: 2009-03-30
*	Name: Ken Chiang
*	Output: mac_filter_type is list_allow and has setting rule.
*			return 1
*			else
*			return 0
*/
int mac_filter_type_allow(void)
{
	int i=0;
	char *getValue, *enable, *name, *mac, *schedule_name;
	char fw_value[256]={0};
	char fw_rule[]="fw_XXXXXXXXXXXXXXXXXXXXXXXXXX";

	if( nvram_match("mac_filter_type", "list_allow") == 0 )
	{
		for(i=0; i<MAX_MAC_FILTER_NUMBER; i++)
		{
			snprintf(fw_rule, sizeof(fw_rule), "mac_filter_%02d",i);

			getValue = nvram_safe_get(fw_rule);

			if (strlen(getValue) == 0 )
				continue;
			else
				strncpy(fw_value, getValue, sizeof(fw_value));

			if( getStrtok(fw_value, "/", "%s %s %s %s", &enable, &name, &mac, &schedule_name) !=4 )
				continue;

			if(strcmp(schedule_name, "Never") == 0)
				continue;

			if( strcmp("1", enable)==0 && strcmp("00:00:00:00:00:00", mac) != 0 ){
				DEBUG_MSG("mac filter is list_allow type\n");
				return 1;
			}
		}
		printf("mac filter is list_allow type, but no rule in it\n");
	}
	return 0;
}

//=========================================================================================================
/*	Mac filter (Network Filter)
*   Nvram name: mac_filter_%02d
*   Format: mac/schedule_name or 1/name/00:0c:6e:a5:8e:86/Always
*	If fw_mac_mode = deny
*		Iptables command:	 iptables -I FORWARD -m mac --mac-source 00:11:25:d4:3b:0e -j DROP
*	If fw_mac_mode = allow
*		Iptables -P INPUT  DROP
*		Iptables command:	 iptables -I FORWARD -m mac --mac-source 00:11:25:d4:3b:0e -j ACCEPT
*	if fw_mac_mode = disable
*		do not execute iptables -m mac
*/
void set_mac_filter_rule(void)
{
	char *mac_filter_type=nvram_safe_get("mac_filter_type");
	int i=0;
	char *getValue=NULL;
	char *enable=NULL, *name=NULL, *mac=NULL, *schedule_name=NULL;
	char fw_value[256]={0};
	char fw_rule[16]={0};

	DEBUG_MSG("set_mac_filter_rule\n");

	if ( mac_filter_type_allow() == 1 )
		save2file(FW_NAT, ":PREROUTING DROP [0:0]\n");

	if( (strcmp(mac_filter_type, "list_allow") == 0)  || (strcmp(mac_filter_type, "list_deny") == 0) )
	{
		for(i=0; i<MAX_MAC_FILTER_NUMBER; i++){
			snprintf(fw_rule, sizeof(fw_rule), "mac_filter_%02d", i);
			getValue = nvram_safe_get(fw_rule);
			if( strlen(getValue)==0 || strlen(getValue)>sizeof(fw_value) )
				continue;
			else{
				memset(fw_value, 0, sizeof(fw_value));
				strncpy(fw_value, getValue, strlen(getValue));
			}

			if( getStrtok(fw_value, "/", "%s %s %s %s", &enable, &name, &mac, &schedule_name) !=4 )
				continue;

			if(strcmp(schedule_name, "Never") == 0)
				continue;

			if( strcmp("1", enable)==0 && strcmp("00:00:00:00:00:00", mac)!=0 ){
				if ( strcmp(mac_filter_type, "list_allow") ==0 ){
					/* Fix mac filter didn't work immediately.(Denied client still can get IP) */
					save2file(FW_FILTER, "-A  INPUT %s -m mac --mac-source %s -j ACCEPT\n",
						schedule2iptables(schedule_name), mac );

					save2file(FW_FILTER, "-A  FORWARD %s -m mac --mac-source %s -j ACCEPT\n",
						schedule2iptables(schedule_name), mac );

					save2file(FW_NAT, "-A  PREROUTING %s -m mac --mac-source %s -j ACCEPT\n",
						schedule2iptables(schedule_name), mac );
				}
				else{
					/* Fix mac filter didn't work immediately.(Denied client still can get IP) */
					save2file(FW_FILTER, "-A  INPUT %s -m mac --mac-source %s -j LOG_AND_DROP\n",
						schedule2iptables(schedule_name), mac );

					save2file(FW_FILTER, "-A  FORWARD %s -m mac --mac-source %s -j LOG_AND_DROP\n",
						schedule2iptables(schedule_name), mac );

					save2file(FW_NAT, "-A  PREROUTING %s -m mac --mac-source %s -j LOG_AND_DROP\n",
						schedule2iptables(schedule_name), mac );
				}
			}
		}
	}
}
//=======================================================================================================

/*
* Date: 2009-01-07
* Name: Albert Chen
* Reason: some customer would not enable so much SPI rule through put issue.
* so we add a flag for different customer request
*/
#ifdef ADVANCE_SPI
void set_advance_spi_rule(void)
{
	int k=0;

	DEBUG_MSG("set_advance_spi_rule \n");
	if( !nvram_match("spi_enable", "1") )
	{
		save2file(FW_NAT,"-A PREROUTING -m state --state INVALID -j LOG_AND_DROP\n");

		FOR_EACH_WAN_FACE
		{
			/* UDP/TCP chargen/echo attack for exposed LAN host */
			/*
			save2file(FW_FILTER,"-I SPI_FORWARD -i %s -p udp --destination-port 7 -j LOG --log-prefix \"udp echo lan\"\n", wan_if[k]);
			save2file(FW_FILTER,"-I SPI_FORWARD -i %s -p udp --destination-port 19 -j LOG --log-prefix \"udp chargen lan\"\n", wan_if[k]);
			save2file(FW_FILTER,"-I SPI_FORWARD -i %s -p tcp --destination-port 7 -j LOG --log-prefix \"tcp echo lan\"\n", wan_if[k]);
			save2file(FW_FILTER,"-I SPI_FORWARD -i %s -p tcp --destination-port 19 -j LOG --log-prefix \"tcp chargen lan\"\n", wan_if[k]);
			*/

			/* UDP port scan for exposed LAN host */
			/*
			save2file(FW_FILTER,"-I SPI_FORWARD -o %s -p icmp --icmp-type port-unreachable -j DROP\n", wan_if[k]);
			save2file(FW_FILTER,"-I SPI_FORWARD -o %s -p icmp --icmp-type port-unreachable -j LOG --log-prefix \"udp port scan\"\n", wan_if[k]);
			*/

			/* Xmas Tree scan, FIN Scan, NULL Scan, ACK Scan, RST Scan, SYN/RST scan for exposed LAN host*/
			/*
			save2file(FW_FILTER,"-I SPI_FORWARD -i %s -p tcp -m tcp --tcp-flags ALL FIN -j DROP\n", wan_if[k]);
			save2file(FW_FILTER,"-I SPI_FORWARD -i %s -p tcp -m tcp --tcp-flags ALL FIN -j LOG --log-prefix \"FIN Scan\"\n", wan_if[k]);
			save2file(FW_FILTER,"-I SPI_FORWARD -i %s -p tcp -m tcp --tcp-flags ALL FIN,URG,PSH -j DROP\n", wan_if[k]);
			save2file(FW_FILTER,"-I SPI_FORWARD -i %s -p tcp -m tcp --tcp-flags ALL FIN,URG,PSH -j LOG --log-prefix \"Xmas Tress Scan\"\n", wan_if[k]);
			save2file(FW_FILTER,"-I SPI_FORWARD -i %s -p tcp -m tcp --tcp-flags SYN,FIN SYN,FIN -j DROP\n", wan_if[k]);
			save2file(FW_FILTER,"-I SPI_FORWARD -i %s -p tcp -m tcp --tcp-flags SYN,FIN SYN,FIN -j LOG --log-prefix \"IMAP FIN/SYN Scan\"\n", wan_if[k]);
			save2file(FW_FILTER,"-I SPI_FORWARD -i %s -p tcp -m tcp -m state --state NEW --tcp-flags ALL ACK -j DROP\n", wan_if[k]);
			save2file(FW_FILTER,"-I SPI_FORWARD -i %s -p tcp -m tcp -m state --state NEW --tcp-flags ALL ACK -j LOG --log-prefix \"ACK Scan\"\n", wan_if[k]);
			save2file(FW_FILTER,"-I SPI_FORWARD -i %s -p tcp -m tcp --tcp-flags ALL NONE -j DROP\n", wan_if[k]);
			save2file(FW_FILTER,"-I SPI_FORWARD -i %s -p tcp -m tcp --tcp-flags ALL NONE -j LOG --log-prefix \"NULL Scan\"\n", wan_if[k]);
			save2file(FW_FILTER,"-I SPI_FORWARD -i %s -p tcp -m tcp --tcp-flags ALL RST -j DROP\n", wan_if[k]);
			save2file(FW_FILTER,"-I SPI_FORWARD -i %s -p tcp -m tcp --tcp-flags ALL RST -j LOG --log-prefix \"RST only Scan\"\n", wan_if[k]);
			save2file(FW_FILTER,"-I SPI_FORWARD -i %s -p tcp -m tcp --tcp-flags ALL SYN,RST -j DROP\n", wan_if[k]);
			save2file(FW_FILTER,"-I SPI_FORWARD -i %s -p tcp -m tcp --tcp-flags ALL SYN,RST -j LOG --log-prefix \"SYNRST Scan\"\n", wan_if[k]);
			*/

			/* Winnuke attack for exposed LAN hosts*/
			/*
			save2file(FW_FILTER,"-I SPI_FORWARD -i %s -p tcp --destination-port 135 --tcp-flags URG URG -j DROP\n", wan_if[k]);
			save2file(FW_FILTER,"-I SPI_FORWARD -i %s -p tcp --destination-port 135 --tcp-flags URG URG -j LOG --log-prefix \"winnuke attack\"\n", wan_if[k]);
			save2file(FW_FILTER,"-I SPI_FORWARD -i %s -p tcp --destination-port 136 --tcp-flags URG URG -j DROP\n", wan_if[k]);
			save2file(FW_FILTER,"-I SPI_FORWARD -i %s -p tcp --destination-port 136 --tcp-flags URG URG -j LOG --log-prefix \"winnuke attack\"\n", wan_if[k]);
			save2file(FW_FILTER,"-I SPI_FORWARD -i %s -p tcp --destination-port 137 --tcp-flags URG URG -j DROP\n", wan_if[k]);
			save2file(FW_FILTER,"-I SPI_FORWARD -i %s -p tcp --destination-port 137 --tcp-flags URG URG -j LOG --log-prefix \"winnuke attack\"\n", wan_if[k]);
			save2file(FW_FILTER,"-I SPI_FORWARD -i %s -p tcp --destination-port 138 --tcp-flags URG URG -j DROP\n", wan_if[k]);
			save2file(FW_FILTER,"-I SPI_FORWARD -i %s -p tcp --destination-port 138 --tcp-flags URG URG -j LOG --log-prefix \"winnuke attack\"\n", wan_if[k]);
			save2file(FW_FILTER,"-I SPI_FORWARD -i %s -p tcp --destination-port 139 --tcp-flags URG URG -j DROP\n", wan_if[k]);
			save2file(FW_FILTER,"-I SPI_FORWARD -i %s -p tcp --destination-port 139 --tcp-flags URG URG -j LOG --log-prefix \"winnuke attack\"\n", wan_if[k]);
			*/
		}

		/* ICMP scan for router */
		save2file(FW_NAT,"-I PREROUTING -j SPI_DOS\n");

		//save2file(FW_NAT,"-A SPI_DOS -p tcp -m limit --limit 10/s --limit-burst 1000 --syn -j RETURN\n");
		//save2file(FW_NAT,"-A SPI_DOS -p tcp -m limit --limit 10/s --limit-burst 1000 --syn -j LOG --log-prefix '[SYN-FLOODING]:'\n");
		//save2file(FW_NAT,"-A SPI_DOS -p tcp -j DROP\n");

/*
 *	Date: 2009-10-08
 *	Name: Gareth Williams <gareth.williams@ubicom.com>
 *	Reason: Streamengine support - streamengine uses icmp and rate limiting this on a rate estimation algorithm can cause serious disruption
 */
#ifndef CONFIG_USER_STREAMENGINE
		save2file(FW_NAT,"-A SPI_DOS -p icmp --icmp-type echo-request -m limit --limit 10/s --limit-burst 50 -j RETURN\n");
		save2file(FW_NAT,"-A SPI_DOS -p icmp --icmp-type echo-reply -m limit --limit 10/s --limit-burst 50 -j RETURN\n");
		save2file(FW_NAT,"-A SPI_DOS -p icmp -m limit --limit 10/s --limit-burst 50 -j LOG --log-prefix '[PING-FLOODING]:'\n");
		save2file(FW_NAT,"-A SPI_DOS -p icmp -j DROP\n");
#endif

		/*
		save2file(FW_FILTER,"-A SPI_INPUT -p icmp --icmp-type router-solicitation -j LOG --log-prefix \"icmp scan 10\"\n");
		save2file(FW_FILTER,"-A SPI_INPUT -p icmp --icmp-type router-solicitation -j DROP\n");
		save2file(FW_FILTER,"-A SPI_INPUT -p icmp --icmp-type timestamp-request -j LOG --log-prefix \"icmp scan 13\"\n");
		save2file(FW_FILTER,"-A SPI_INPUT -p icmp --icmp-type timestamp-request -j DROP\n");
		save2file(FW_FILTER,"-A SPI_INPUT -p icmp --icmp-type address-mask-request -j LOG --log-prefix \"icmp scan 17\"\n");
		save2file(FW_FILTER,"-A SPI_INPUT -p icmp --icmp-type address-mask-request -j DROP\n");
		//save2file(FW_FILTER,"-A SPI_INPUT -p icmp --icmp-type information-request -j LOG --log-prefix \"icmp scan 15\"\n");
		//save2file(FW_FILTER,"-A SPI_INPUT -p icmp --icmp-type information-request -j DROP\n");
		//save2file(FW_FILTER,"-A SPI_INPUT -p icmp --icmp-type domain-name-request -j LOG --log-prefix \"icmp scan 37\"\n");
		//save2file(FW_FILTER,"-A SPI_INPUT -p icmp --icmp-type domain-name-request  -j DROP\n");
		save2file(FW_FILTER,"-A SPI_OUTPUT -p icmp --icmp-type router-advertisement -j LOG --log-prefix \"icmp scan 9\"\n");
		save2file(FW_FILTER,"-A SPI_OUTPUT -p icmp --icmp-type router-advertisement -j DROP\n");
		save2file(FW_FILTER,"-A SPI_OUTPUT -p icmp --icmp-type time-exceeded -j LOG --log-prefix \"icmp scan 11\"\n");
		save2file(FW_FILTER,"-A SPI_OUTPUT -p icmp --icmp-type time-exceeded -j DROP\n");
		save2file(FW_FILTER,"-A SPI_OUTPUT -p icmp --icmp-type parameter-problem -j LOG --log-prefix \"icmp scan 12\"\n");
		save2file(FW_FILTER,"-A SPI_OUTPUT -p icmp --icmp-type parameter-problem -j DROP\n");
		//save2file(FW_FILTER,"-A SPI_OUTPUT -p icmp --icmp-type traceroute -j LOG --log-prefix \"icmp scan 30\"\n");
		//save2file(FW_FILTER,"-A SPI_OUTPUT -p icmp --icmp-type traceroute -j DROP\n");
		*/

		/* smurf attack */
		/*
		FOR_EACH_WAN_IP
		{
			save2file(FW_FILTER,"-I SPI_FORWARD -s %s -p icmp -j DROP\n", wan_ipaddr[k]);
			save2file(FW_FILTER,"-I SPI_FORWARD -s %s -p icmp -j LOG --log-prefix \"smurf attack\"\n", wan_ipaddr[k]);
		}
		*/

		/* land attack */
		//FOR_EACH_WAN_IP
		//{
		//	save2file(FW_NAT,"-I SPI_PREROUT -p tcp -s %s -d %s -j DROP\n", wan_ipaddr[k], wan_ipaddr[k]);
		//	save2file(FW_NAT,"-I SPI_PREROUT -p tcp -s %s -d %s -j LOG --log-prefix '[LAND-ATTACK-SPI]:'\n", wan_ipaddr[k], wan_ipaddr[k]);
		//}

		/* Xmas Tree scan, FIN Scan, NULL Scan, ACK Scan, RST Scan, SYN/RST scan for router*/
		save2file(FW_NAT,"-I SPI_PREROUT -p tcp -m tcp --tcp-flags SYN,ACK SYN,ACK -j LOG --log-prefix '[BAD-PACKET]:'\n");
		save2file(FW_NAT,"-I SPI_PREROUT -p tcp -m tcp --tcp-flags SYN,ACK SYN,ACK -j DROP\n");

		save2file(FW_NAT,"-I SPI_PREROUT -p tcp -m tcp --tcp-flags ALL FIN -j LOG --log-prefix '[FIN-SCAN]:'\n");
		save2file(FW_NAT,"-I SPI_PREROUT -p tcp -m tcp --tcp-flags ALL FIN -j DROP\n");


		save2file(FW_NAT,"-I SPI_PREROUT -p tcp -m tcp --tcp-flags ALL ALL -j LOG --log-prefix '[XMASS-TREE-SCAN]:'\n");
		save2file(FW_NAT,"-I SPI_PREROUT -p tcp -m tcp --tcp-flags ALL ALL -j DROP\n");
		//save2file(FW_NAT,"-I SPI_PREROUT -p tcp -m tcp --tcp-flags ALL FIN,URG,PSH -j LOG --log-prefix '[NMAP-XMASS*TREE-SCAN]:'\n");
		//save2file(FW_NAT,"-I SPI_PREROUT -p tcp -m tcp --tcp-flags ALL FIN,URG,PSH -j DROP\n");
		save2file(FW_NAT,"-I SPI_PREROUT -p tcp -m tcp --tcp-flags ALL SYN,RST,ACK,FIN,URG -j LOG --log-prefix '[XMASS-TREE-SCAN]:'\n");
		save2file(FW_NAT,"-I SPI_PREROUT -p tcp -m tcp --tcp-flags ALL SYN,RST,ACK,FIN,URG -j DROP\n");

		save2file(FW_NAT,"-I SPI_PREROUT -p tcp -m tcp --tcp-flags SYN,FIN SYN,FIN -j LOG --log-prefix '[IMAP-FIN-SYN-SCAN]:'\n");
		save2file(FW_NAT,"-I SPI_PREROUT -p tcp -m tcp --tcp-flags SYN,FIN SYN,FIN -j DROP\n");
		//save2file(FW_NAT,"-I SPI_PREROUT -p tcp -m state --state NEW,RELATED -m tcp --tcp-flags ALL ACK -j LOG --log-prefix '[ACK-SCAN]:'\n");
		//save2file(FW_NAT,"-I SPI_PREROUT -p tcp -m state --state NEW,RELATED -m tcp --tcp-flags ALL ACK -j DROP\n");
		save2file(FW_NAT,"-I SPI_PREROUT -p tcp -m tcp --tcp-flags ALL NONE -j LOG --log-prefix '[NULL-SCAN]:'\n");
		save2file(FW_NAT,"-I SPI_PREROUT -p tcp -m tcp --tcp-flags ALL NONE -j DROP\n");
		//save2file(FW_NAT,"-I SPI_PREROUT -p tcp -m state --state NEW,RELATED -m tcp --tcp-flags ALL RST -j LOG --log-prefix '[RST-ONLY-SCAN]:'\n");
		//save2file(FW_NAT,"-I SPI_PREROUT -p tcp -m state --state NEW,RELATED -m tcp --tcp-flags ALL RST -j DROP\n");
		//save2file(FW_NAT,"-I SPI_PREROUT -p tcp -m tcp --tcp-flags ALL SYN,RST -j LOG --log-prefix '[SYNRST-SCAN]:'\n");
		//save2file(FW_NAT,"-I SPI_PREROUT -p tcp -m tcp --tcp-flags ALL SYN,RST -j DROP\n");

		/* Winnuke attack for router*/
		/*
		save2file(FW_FILTER,"-A SPI_INPUT -p tcp --destination-port 135 --tcp-flags URG URG -j LOG --log-prefix \"winnuke attack\"\n");
		save2file(FW_FILTER,"-A SPI_INPUT -p tcp --destination-port 135 --tcp-flags URG URG -j DROP\n");
		save2file(FW_FILTER,"-A SPI_INPUT -p tcp --destination-port 136 --tcp-flags URG URG -j LOG --log-prefix \"winnuke attack\"\n");
		save2file(FW_FILTER,"-A SPI_INPUT -p tcp --destination-port 136 --tcp-flags URG URG -j DROP\n");
		save2file(FW_FILTER,"-A SPI_INPUT -p tcp --destination-port 137 --tcp-flags URG URG -j LOG --log-prefix \"winnuke attack\"\n");
		save2file(FW_FILTER,"-A SPI_INPUT -p tcp --destination-port 137 --tcp-flags URG URG -j DROP\n");
		save2file(FW_FILTER,"-A SPI_INPUT -p tcp --destination-port 138 --tcp-flags URG URG -j LOG --log-prefix \"winnuke attack\"\n");
		save2file(FW_FILTER,"-A SPI_INPUT -p tcp --destination-port 138 --tcp-flags URG URG -j DROP\n");
		save2file(FW_FILTER,"-A SPI_INPUT -p tcp --destination-port 139 --tcp-flags URG URG -j LOG --log-prefix \"winnuke attack\"\n");
		save2file(FW_FILTER,"-A SPI_INPUT -p tcp --destination-port 139 --tcp-flags URG URG -j DROP\n");
		*/

		/* UDP/TCP chargen/echo attack for router */
		/*
		save2file(FW_FILTER,"-A SPI_INPUT -p udp --destination-port 7 -j LOG --log-prefix \"udp echo\"\n");
		save2file(FW_FILTER,"-A SPI_INPUT -p udp --destination-port 7 -j DROP\n");
		save2file(FW_FILTER,"-A SPI_INPUT -p udp --destination-port 19 -j LOG --log-prefix \"udp chargen\"\n");
		save2file(FW_FILTER,"-A SPI_INPUT -p udp --destination-port 19 -j DROP\n");
		save2file(FW_FILTER,"-A SPI_INPUT -p tcp --destination-port 7 -j LOG --log-prefix \"tcp echo\"\n");
		save2file(FW_FILTER,"-A SPI_INPUT -p tcp --destination-port 7 -j DROP\n");
		save2file(FW_FILTER,"-A SPI_INPUT -p tcp --destination-port 19 -j LOG --log-prefix \"tcp chargen\"\n");
		save2file(FW_FILTER,"-A SPI_INPUT -p tcp --destination-port 19 -j DROP\n");
		*/

		/* UDP port scan for router*/
		//save2file(FW_FILTER,"-A SPI_OUTPUT -p icmp --icmp-type port-unreachable -j LOG --log-prefix \"udp port scan\"\n");
		//save2file(FW_FILTER,"-A SPI_OUTPUT -p icmp --icmp-type port-unreachable -j DROP\n");
	}
	DEBUG_MSG("set_advance_spi_rule finish\n");
}
#endif //#ifdef ADVANCE_SPI

//======================================================================================================
/* Get WAN IP and WAN interface
* Chun: If russia is enabled, we need to add another rule for WAN_PHY interface and start_firewall()
* need to be performed when one of both interfaces has obtained wan IP.
*---------------wan_if[0]---wan_if[1]-----------------
* 1)dhcpc	->	eth0
* 2)static  ->	eth0
* 3)pppoe	->	ppp0 		(+ eth0) (if russia enable)
* 4)pptp	->	ppp0 		(+ eth0) (if russia enable)
* 5)l2tp	->	ppp0
*----------wan_ipaddr[0]---wan_ipaddr[1]-----------------
*
* 1)PPPoE/PPTP/L2TP (Russia enable)
*		a)Only eth0 obtains IP: 	wan_ipaddr[1] will be set into iptables
*		b)Only ppp0 obtains IP: 	wan_ipaddr[0] will be set into iptables
*		c)eth0 and ppp0 obtain IP: 	wan_ipaddr[0] and wan_ipaddr[1] will be set into iptables
*
* 2)PPPoE/PPTP/L2TP (Russia disable)
*		a)ppp0 obtains IP: 			wan_ipaddr[0] will be set into iptables

* 3)DHCP/Static
*		a)eth0 obtains IP: 			wan_ipaddr[0] will be set into iptables
*
*/
void set_wan_ip_if( int russia_enable, char *wan_protocal, char *wan_eth)
{
	char *tmp_wan_ip1=NULL;
#ifdef RPPPOE
	char *tmp_wan_ip2=NULL;
#endif

	memset(wan_ipaddr[0], 0, sizeof(wan_ipaddr[0]));
	memset(wan_ipaddr[1], 0, sizeof(wan_ipaddr[1]));

	DEBUG_MSG("set_wan_ip_if: russia_enable=%d, wan_protocal=%s, wan_eth=%s\n",
		russia_enable, wan_protocal, wan_eth);

	if (( strcmp(wan_protocal, "dhcpc") ==0 ) || ( strcmp(wan_protocal, "static") ==0 )
#if defined(CONFIG_USER_3G_USB_CLIENT_WM5) || defined(CONFIG_USER_3G_USB_CLIENT_IPHONE)
                || ( strcmp(wan_protocal, "usb3g_phone") ==0 )
#endif
	)
	{
		tmp_wan_ip1 = get_ipaddr(wan_eth);
		if(tmp_wan_ip1)
		{
			strncpy(wan_ipaddr[0], tmp_wan_ip1, strlen(tmp_wan_ip1));
			strcpy(wan_if[0], wan_eth);
		}
	}
	else
	{
		tmp_wan_ip1 = get_ipaddr( "ppp0" );
		if(tmp_wan_ip1)
		{
			strncpy(wan_ipaddr[0], tmp_wan_ip1, strlen(tmp_wan_ip1));
			strcpy(wan_if[0], "ppp0");
		}
#ifdef RPPPOE
		if(russia_enable)
		{
			tmp_wan_ip2 = get_ipaddr(wan_eth);
			if(tmp_wan_ip2)
			{
				strncpy(wan_ipaddr[1], tmp_wan_ip2, strlen(tmp_wan_ip2));
				strcpy(wan_if[1], wan_eth);
			}
		}
#endif
	}
}


//=======================================================================================================
void set_tables_policy(int have_wan_ip, int enable_mac_filter)
{

	DEBUG_MSG("set_tables_policy: have_wan_ip=%d, enable_mac_filter=%d\n",
		have_wan_ip, enable_mac_filter);

	init_file(FW_FILTER);
	save2file(FW_FILTER,
		"*filter\n"
		":INPUT ACCEPT [0:0]\n"
		":%s\n"
		":OUTPUT ACCEPT [0:0]\n"
#ifdef ADVANCE_SPI
		":SPI_FORWARD - [0:0]\n"
		":SPI_OUTPUT - [0:0]\n"
		":SPI_INPUT - [0:0]\n"
#endif
		":MINIUPNPD - [0:0]\n"
		":LOG_AND_DROP - [0:0]\n"
		,(have_wan_ip==0 && enable_mac_filter==0) ? "FORWARD ACCEPT [0:0]" : "FORWARD DROP [0:0]"
	);

	init_file(FW_NAT);
	save2file(FW_NAT,
		"*nat\n"
		":%s\n"
		":POSTROUTING ACCEPT [0:0]\n"
		":OUTPUT ACCEPT [0:0]\n"
#ifdef ADVANCE_SPI
		":SPI_DOS - [0:0]\n"
		":SPI_PREROUT - [0:0]\n"
#endif
		":MINIUPNPD - [0:0]\n"
		":LOG_AND_DROP - [0:0]\n"
		,(enable_mac_filter == 1) ? "PREROUTING DROP [0:0]" : "PREROUTING ACCEPT [0:0]"
	);

	if(have_wan_ip==1)
	{
	init_file(FW_MANGLE);
	save2file(FW_MANGLE,
		"*mangle\n"
		":PREROUTING ACCEPT [0:0]\n"
		":INPUT ACCEPT [0:0]\n"
		":FORWARD ACCEPT [0:0]\n"
		":OUTPUT ACCEPT [0:0]\n"
		":POSTROUTING ACCEPT [0:0]\n");
	}

	DEBUG_MSG("set_tables_policy: finish\n");
}

//=======================================================================================================
void set_pass_through(char *wan_eth, char *wan_protocal, int russia_enable)
{
	char *pptp_pass_through=nvram_safe_get("pptp_pass_through");
	char *l2tp_pass_through=nvram_safe_get("l2tp_pass_through");
	char *wan_eth_ip=NULL;
	int k=0;//FOR_EACH_WAN_FACE

	DEBUG_MSG("set_pass_through: wan_eth=%s, wan_protocal=%s, russia_enable=%d\n",
		wan_eth, wan_protocal, russia_enable);

	/* PPTP Pass Through Disable*/
	if( strcmp(pptp_pass_through, "0") == 0 )
		FOR_EACH_WAN_FACE
			save2file(FW_FILTER,"-A FORWARD -o %s -p tcp --dport %d -j LOG_AND_DROP\n", wan_if[k], PPTP_PORT);

	/* L2TP Pass Through Disable*/
	if( strcmp(l2tp_pass_through, "0") == 0 )
		FOR_EACH_WAN_FACE
			save2file(FW_FILTER,"-A FORWARD -o %s -p udp --dport %d -j LOG_AND_DROP\n", wan_if[k], L2TP_PORT);

	/* PPTP Pass Through  or L2TP Pass Through Enable*/
	/* support Israel VPN
	=> All VPN pass-throughput can over on all VPN clients (WAN Type)
	=> PPTP/L2TP/IPSEC client from PC side with VPN WAN
	*/

	if((( !strcmp(wan_protocal, "pptp")  || !strcmp(wan_protocal, "l2tp")) && (strcmp(pptp_pass_through, "1") == 0 || strcmp(l2tp_pass_through, "1") == 0) )
#ifdef RPPPOE
	||( (!strcmp(wan_protocal, "pppoe")) && (russia_enable ==1) )
	||( (!strcmp(wan_protocal, "pptp")) && (russia_enable ==1) )
	||( (!strcmp(wan_protocal, "l2tp")) && (russia_enable ==1) )
#endif
	)
	{

		wan_eth_ip = get_ipaddr(wan_eth);
		if(wan_eth_ip)
			save2file(FW_NAT,"-A POSTROUTING -o %s -j SNAT --to-source %s\n", wan_eth, wan_eth_ip);
	}

	/* IPSec Pass Through */
	if(nvram_match("ipsec_pass_through", "0") == 0  )
		FOR_EACH_WAN_FACE
			save2file(FW_FILTER,"-A FORWARD -o %s -p udp --dport %d -j LOG_AND_DROP\n", wan_if[k], ISAKMP_PORT);

	/* Multicast Pass Through */
	if(nvram_match("multicast_stream_enable", "1") == 0 )
		FOR_EACH_WAN_FACE
			save2file(FW_FILTER,"-A FORWARD -i %s -p udp --destination %s -j ACCEPT\n", wan_if[k], IP_MULTICAST);

	/* Ident Stealth disable */
	if(0) //John 2010.04.23 remove unused (nvram_match("ident_enable", "0") == 0  )
		save2file(FW_FILTER,"-A FORWARD -o %s -p tcp --dport %d -j LOG_AND_DROP\n", wan_eth, IDENT_PORT);
}
//==================================================================================================================
#ifndef ACCESS_CONTROL
/*   firewall rule
*   Nvram name: firewall_rule_%02d
*   Format:
*	enable/name/iface_src/protocol/src_ip_start/src_ip_end/iface_dest/dest_ip_start/dest_ip_end/dest_port_start/dest_port_end/schedule/action
*/
void set_firewall_rule(void)
{
	char fw_rule[20]={0};
	char fw_value[256]={0};
	char *getValue=NULL;
	char *enable=NULL, *name=NULL, *iface_src=NULL, *iface_dest=NULL, *protocol=NULL;
	char *src_ip_start=NULL, *src_ip_end=NULL;
	char *dest_ip_start=NULL, *dest_ip_end, *dest_port_start=NULL, *dest_port_end=NULL;
	char *schedule_name=NULL, *act=NULL;
	char interface[2][10]={{0}}, src_ip_range[44]={0};
	char ip_range_cmd[128]={0}, dst_ip_range[44]={0}, port_range_cmd[20]={0};
	char action[10]={0};
	int k=0, i=0;

	printf("set_firewall_rule: No Define Access Control !!\n");

	for(i=0; i<MAX_FIREWALL_RULE_NUMBER; i++)
	{
		memset(fw_value, 0, sizeof(fw_value));
		snprintf(fw_rule, sizeof(fw_rule), "firewall_rule_%02d", i);

		getValue = nvram_safe_get(fw_rule);
		if ( strlen(getValue) == 0 )
			continue;
		else
			strncpy(fw_value, getValue, sizeof(fw_value));

		if( getStrtok(fw_value, "/", "%s %s %s %s %s %s %s %s %s %s %s %s %s",
			&enable, &name, &iface_src, &protocol, &src_ip_start, &src_ip_end, \
			&iface_dest, &dest_ip_start, &dest_ip_end, &dest_port_start, &dest_port_end, \
			&schedule_name, &act) !=13 )

			continue;

		DEBUG_MSG("Firewall Rule: %s %s\n%s %s %s %s\n%s %s %s %s %s\n%s %s\n",
			enable, name, iface_src, protocol, src_ip_start, src_ip_end,
			iface_dest, dest_ip_start, dest_ip_end, dest_port_start,
			dest_port_end, schedule_name, act);

		if(strcmp(schedule_name, "Never") == 0)
			continue;

		if(strcmp(enable, "1") == 0)
		{
			/* Check if it specifies interface or not */
			if (!strcmp(iface_src, "source"))
				strcpy(interface[0]," ");
			else if (!strcmp(iface_src, "LAN"))
				sprintf(interface[0],"-i %s", bri_eth);
			else
			{
				FOR_EACH_WAN_FACE
					sprintf(interface[k],"-i %s",wan_if[k]);
			}

			/* Oragnize the iprange command */
			if (!strcmp(src_ip_start, "*"))
				sprintf(src_ip_range," ");/* There is no limit in source ip */
			else
			{
				/* no assign src_ip_end */
				if (src_ip_end == NULL)
					src_ip_end = src_ip_start;
				sprintf(src_ip_range,"--src-range %s-%s", src_ip_start, src_ip_end);
			}

			if (!strcmp(dest_ip_start, "*"))
				sprintf(dst_ip_range," ");/* There is no limit in dest ip */
			else
			{
				/* no assign dest_ip_end */
				if (dest_ip_end==NULL)
					dest_ip_end = dest_ip_start;
				sprintf(dst_ip_range,"--dst-range %s-%s", dest_ip_start, dest_ip_end);
			}

			if( (!strcmp(src_ip_range, " ")) && (!strcmp(dst_ip_range, " ")) )
				sprintf(ip_range_cmd," ");
			else
				sprintf(ip_range_cmd,"-m iprange %s %s", src_ip_range, dst_ip_range);

			/* Oragnize the port_range command */
			if ((!strcmp(dest_port_start, "*")) || (!strcmp(protocol,"ICMP")) )
				sprintf(port_range_cmd," ");/* There is no limit in dest port */
			else
			{
				/* no assign dest_port_end */
				if (dest_port_end==NULL)
					dest_port_end = dest_port_start;
				sprintf(port_range_cmd,"--dport %s:%s", dest_port_start, dest_port_end);
			}

			/* Decide action */
			if (!strcmp(act, "Allow"))
				strcpy(action,"ACCEPT");
			else
				strcpy(action,"DROP");

			if( (!strcmp(protocol,"TCP")) || (!strcmp(protocol,"UDP")) || (!strcmp(protocol,"ICMP")))
			{
				DEBUG_MSG("Firewall rule: protocol = TCP or UDP or ICMP\n");
				if (strcmp(iface_src, "WAN") == 0 && strcmp(iface_dest, "LAN") == 0 && strcmp(act, "Allow") == 0)
				{
					FOR_EACH_WAN_IP
						save2file(FW_NAT, "-A PREROUTING -p %s -d %s --dport %s:%s %s -j DNAT --to %s:%s-%s\n",\
						protocol, wan_ipaddr[k], dest_port_start, dest_port_end, schedule2iptables(schedule_name), dest_ip_start, dest_port_start, dest_port_end);
				}
				else
				{
					for(k=0;k<2;k++)
						if(strlen(interface[k]))
							save2file(FW_NAT, "-A  PREROUTING %s -p %s %s %s %s -j %s\n",\
							protocol, interface[k], ip_range_cmd, port_range_cmd, schedule2iptables(schedule_name), action );
				}

				/* for WAN to LAN and Deny*/
				save2file(FW_FILTER,"-A FORWARD -o %s -p %s %s %s %s -m state --state NEW -j %s\n", \
					bri_eth, protocol, ip_range_cmd, port_range_cmd, schedule2iptables(schedule_name), action);

			}
			else if( (!strcmp(protocol,"Any")) )
			{
				if (strcmp(iface_src, "WAN") == 0 && strcmp(iface_dest, "LAN") == 0 && strcmp(act, "Allow") == 0)
				{
					FOR_EACH_WAN_IP
					{
						save2file(FW_NAT, "-A PREROUTING -p TCP -d %s --dport %s:%s %s -j DNAT --to %s:%s-%s\n",\
							wan_ipaddr[k], dest_port_start, dest_port_end, schedule2iptables(schedule_name), dest_ip_start, dest_port_start, dest_port_end);

						save2file(FW_NAT, "-A PREROUTING -p UDP -d %s --dport %s:%s %s -j DNAT --to %s:%s-%s\n",\
							wan_ipaddr[k], dest_port_start, dest_port_end, schedule2iptables(schedule_name), dest_ip_start, dest_port_start, dest_port_end);

						save2file(FW_NAT, "-A PREROUTING -p ICMP -d %s --dport %s:%s %s -j DNAT --to %s:%s-%s\n",\
							wan_ipaddr[k], dest_port_start, dest_port_end, schedule2iptables(schedule_name), dest_ip_start, dest_port_start, dest_port_end);
					}
				}
				else
				{
					for(k=0;k<2;k++)
						if(strlen(interface[k]))
						{
							save2file(FW_NAT, "-A  PREROUTING %s -p TCP %s %s %s -j %s\n",
								interface[k], ip_range_cmd, port_range_cmd, schedule2iptables(schedule_name), action );

							save2file(FW_NAT, "-A  PREROUTING %s -p UDP %s %s %s -j %s\n",
								interface[k], ip_range_cmd, port_range_cmd, schedule2iptables(schedule_name), action );

							save2file(FW_NAT, "-A  PREROUTING %s -p ICMP %s %s %s -j %s\n",
								interface[k], ip_range_cmd, port_range_cmd, schedule2iptables(schedule_name), action );
						}
				}

				/* for WAN to LAN and Deny*/
				save2file(FW_FILTER,"-A FORWARD -o %s -p TCP %s %s %s -m state --state NEW -j %s\n", \
					bri_eth, ip_range_cmd, port_range_cmd, schedule2iptables(schedule_name), action);

				save2file(FW_FILTER,"-A FORWARD -o %s -p UDP %s %s %s -m state --state NEW -j %s\n", \
					bri_eth, ip_range_cmd, port_range_cmd, schedule2iptables(schedule_name), action);

				save2file(FW_FILTER,"-A FORWARD -o %s -p ICMP %s %s %s -m state --state NEW -j %s\n", \
					bri_eth, ip_range_cmd, port_range_cmd, schedule2iptables(schedule_name), action);
			}
		}
	}
}
#endif
//==================================================================================================================
void set_block_service(void)
{
	int i=0;
	char fw_rule[20]={0};
	char fw_value[256]={0};
	char *getValue=NULL;
	char *enable=NULL, *name=NULL, *wan_proto=NULL;
	char *start_port=NULL, *end_port=NULL;
	char *start_ip=NULL, *end_ip=NULL, *schedule_name=NULL;

	DEBUG_MSG("set_block_service\n");

	for(i=0; i<MAX_BLOCK_SERVICE_NUMBER; i++)
	{
		snprintf(fw_rule, sizeof(fw_rule), "block_service_%02d", i);

		getValue = nvram_safe_get(fw_rule);
		if ( strlen(getValue) == 0 )
			continue;
		else
			strncpy(fw_value, getValue, sizeof(fw_value));

		if( getStrtok(fw_value, "/", "%s %s %s %s %s %s %s %s",
			&enable, &name, &wan_proto, &start_port, &end_port, &start_ip, &end_ip, &schedule_name) != 8)
			continue;

		if(strcmp(schedule_name, "Never") == 0)
			continue;

		if ( strcmp(enable, "1") == 0 )
		{
			if( (strcmp("TCP", wan_proto) == 0) || (strcmp("UDP", wan_proto) == 0) )
			{
				/* For dns filter work issue */
				if(atoi(start_port)<=53 && atoi(end_port)>=53){
					printf("TCP port 53 input filter\n");
					save2file(FW_FILTER,"-A INPUT -p %s -i %s %s -m iprange --src-range %s-%s --dport 53:53 -j LOG --log-prefix \"Blocked Service=%d \"\n"
						,wan_proto ,bri_eth, schedule2iptables(schedule_name), start_ip, end_ip,i);
					save2file(FW_FILTER,"-A INPUT -p %s -i %s %s -m iprange --src-range %s-%s --dport 53:53 -j DROP\n"
						,wan_proto ,bri_eth, schedule2iptables(schedule_name), start_ip, end_ip);
				}
				save2file(FW_FILTER,"-A FORWARD -p %s -i %s %s -m iprange --src-range %s-%s --dport %s:%s -j LOG --log-prefix \"Blocked Service=%d \"\n"
					,wan_proto ,bri_eth, schedule2iptables(schedule_name), start_ip, end_ip, start_port, end_port,i);
				save2file(FW_FILTER,"-A FORWARD -p %s -i %s %s -m iprange --src-range %s-%s --dport %s:%s -j DROP\n"
					,wan_proto ,bri_eth, schedule2iptables(schedule_name), start_ip, end_ip, start_port, end_port);
			}
			else if( (strcmp("Any", wan_proto) == 0) || (strcmp("Both", wan_proto) == 0) )
			{
				/* For dns filter work issue */
				if(atoi(start_port)<=53 && atoi(end_port)>=53){
					printf("UDP port 53 input filter\n");
					save2file(FW_FILTER,"-A INPUT -p TCP -i %s %s -m iprange --src-range %s-%s --dport 53:53 -j LOG --log-prefix \"Blocked Service=%d \"\n",bri_eth, schedule2iptables(schedule_name), start_ip, end_ip,i);
					save2file(FW_FILTER,"-A INPUT -p TCP -i %s %s -m iprange --src-range %s-%s --dport 53:53 -j DROP\n", bri_eth, schedule2iptables(schedule_name), start_ip, end_ip);
					save2file(FW_FILTER,"-A INPUT -p UDP -i %s %s -m iprange --src-range %s-%s --dport 53:53 -j LOG --log-prefix \"Blocked Service=%d \"\n",bri_eth, schedule2iptables(schedule_name), start_ip, end_ip,i);
					save2file(FW_FILTER,"-A INPUT -p UDP -i %s %s -m iprange --src-range %s-%s --dport 53:53 -j DROP\n", bri_eth, schedule2iptables(schedule_name), start_ip, end_ip);
				}
				save2file(FW_FILTER,"-A FORWARD -p TCP -i %s %s -m iprange --src-range %s-%s --dport %s:%s -j LOG --log-prefix \"Blocked Service=%d \"\n",bri_eth, schedule2iptables(schedule_name), start_ip, end_ip, start_port, end_port,i);
				save2file(FW_FILTER,"-A FORWARD -p TCP -i %s %s -m iprange --src-range %s-%s --dport %s:%s -j DROP\n", bri_eth, schedule2iptables(schedule_name), start_ip, end_ip, start_port, end_port);
				save2file(FW_FILTER,"-A FORWARD -p UDP -i %s %s -m iprange --src-range %s-%s --dport %s:%s -j LOG --log-prefix \"Blocked Service=%d \"\n",bri_eth, schedule2iptables(schedule_name), start_ip, end_ip, start_port, end_port,i);
				save2file(FW_FILTER,"-A FORWARD -p UDP -i %s %s -m iprange --src-range %s-%s --dport %s:%s -j DROP\n", bri_eth, schedule2iptables(schedule_name), start_ip, end_ip, start_port, end_port);
			}
		}
	}
	DEBUG_MSG("set_block_service finish\n");
}

//==================================================================================================================
/*	Virtaul Server
*  	Nvram name:  vs_rule_%02d
*  	Format:  enable/name/prot/public_port/private_port/ip_addr/schedule_rule_00_name
*   Iptables command:	iptables -A PREROUTING -t nat -p tcp -d 172.21.34.17 --dport 21 -j DNAT --to 192.168.0.100:21
*					   	iptables -A FORWARD -i eth0 -o br0 -p tcp --sport 1024:65535 -d 192.168.0.100 --dport 21 -m state --state NEW -j ACCEPT
*/
void set_virtual_server(int index, int type, char *lan_ipaddr, char *lan_netmask, char *numProto)
{
	int  j=0, k=0;
	char fw_rule[16]={0};
	char fw_value[256]={0};
	char *enable=NULL, *name=NULL, *wan_proto=NULL;
	char *pub_ports=NULL, *pri_ports=NULL, *lan_ip=NULL;
	char *schedule_name=NULL, *inbound_name=NULL;
	char action[10]={0};
	struct IpRange ip_range[8];
	int inbound_ip_count=0;
	char public_port_range[25]={0}, private_port_range_1[25]={0};
	char port_1[25]={0},  port_2[25]={0},  port_3[25]={0};
#ifdef WAKE_ON_LAN
	int wake_on_lan=0;
#endif

	DEBUG_MSG("set_virtual_server: index=%d, type=%d, lan_ipaddr=%s, lan_netmask=%s, numProto=%s\n",
		index, type, lan_ipaddr, lan_netmask, numProto);

	/*
	type=1 => !strcmp(numProto,"6") || !strcmp(numProto,"256") || !strcmp("NetMeeting", name)
	type=2 => (!strcmp(numProto,"17") || !strcmp(numProto,"256")) && (strcmp("NetMeeting", name))
	type=3 => (strcmp(numProto,"6")) && (strcmp(numProto,"17")) && (strcmp(numProto,"256"))
	*/

	snprintf(fw_rule, sizeof(fw_rule), "vs_rule_%02d", index);
	strncpy(fw_value, nvram_safe_get(fw_rule), sizeof(fw_value));

	getStrtok(fw_value, "/", "%s %s %s %s %s %s %s %s", \
			&enable, &name, &wan_proto, &pub_ports, &pri_ports, &lan_ip, &schedule_name, &inbound_name);

#ifdef INBOUND_FILTER
	if(inbound_name && strlen(inbound_name))
	{
		/* Deny All implies that the rule is invalid */
		if(!strcmp(inbound_name, "Deny_All"))
			return;
		memset(action, 0, sizeof(action));
		memset(ip_range, 0, 8*sizeof(struct IpRange));
		inbound_ip_count = inbound2iptables(inbound_name, action, ip_range);
	}
#endif

	memset(port_1, 0, sizeof(port_1));
	memset(port_2, 0, sizeof(port_2));
	memset(port_3, 0, sizeof(port_3));

	if( type == 1 )
	{
		memset(public_port_range, 0, sizeof(public_port_range));
		memset(private_port_range_1, 0, sizeof(private_port_range_1));
		strcpy(public_port_range, pub_ports);
		strcpy(private_port_range_1, pri_ports);

		sprintf(port_1, "--dport %s", public_port_range);
		sprintf(port_2, "--dport %s", private_port_range_1);
		sprintf(port_3, ":%s", private_port_range_1);
	}

	if( type == 2 )
	{
		sprintf(port_1, "--dport %s", pub_ports);
		sprintf(port_2, "--dport %s", pri_ports);
		sprintf(port_3, ":%s", pri_ports);
	}

	if( type == 3 )
	{
		/* Any and Other
		* Any: all protocol without port number
		* Other: specific protocol without port number
		*/
		if(!strcmp(numProto,"257"))
			strcpy(numProto,"ALL");
	}

	DEBUG_MSG("set_virtual_server: port_1=%s, port_2=%s, port_3=%s\n",
		port_1, port_2, port_3);

#ifdef WAKE_ON_LAN
	DEBUG_MSG("set_virtual_server WAKE_ON_LAN\n");
	if( type == 2 ) //UDP
	{
		if( strcmp("Wake-On-LAN", name)==0 )
		{
			if( strcmp(lan_ip, get_broadcast_addr(bri_eth)) ==0 )
			{
				DEBUG_MSG("set_virtual_server: Wake_On_LAN Enable\n");
				wake_on_lan = 1;
			}
		}
		else
			wake_on_lan = 0;
	}
#endif //WAKE_ON_LAN

#ifdef INBOUND_FILTER
	if(inbound_ip_count && strcmp(inbound_name, "Allow_All"))
	{
		printf("Virtual Server apply to Inbound Filter\n");
		if( !strcmp(action, "allow") )
		{
			for(j=0;j<inbound_ip_count;j++)
			{
#ifdef WAKE_ON_LAN
				if(type == 2 && wake_on_lan==1) //using lan_ipaddr as wake-on-lan proxy => change lan_ip to lan_ipaddr
				{
					FOR_EACH_WAN_IP
						save2file(FW_NAT, "-A PREROUTING -m iprange --src-range %s -p UDP -d %s --dport %s %s -j DNAT --to %s:%s\n", \
							ip_range[j].addr, wan_ipaddr[k], pub_ports, schedule2iptables(schedule_name), lan_ipaddr, pri_ports);
				}
				else
#endif //#ifdef WAKE_ON_LAN
				{
					FOR_EACH_WAN_IP
						save2file(FW_NAT, "-A PREROUTING -m iprange --src-range %s -p %s -d %s %s %s -j DNAT --to %s%s\n", \
							ip_range[j].addr, numProto, wan_ipaddr[k], port_1, schedule2iptables(schedule_name), lan_ip, port_3);
				}
			}
		}
		else
		{
			for(j=0;j<inbound_ip_count;j++)
			{
				FOR_EACH_WAN_IP
					save2file(FW_NAT, "-A PREROUTING -m iprange --src-range %s -p %s -d %s %s %s -j DROP\n",\
						ip_range[j].addr, numProto, wan_ipaddr[k], port_1, schedule2iptables(schedule_name));
			}

#ifdef WAKE_ON_LAN
			if( type==2 && wake_on_lan==1 ) //using lan_ipaddr as wake-on-lan proxy => change lan_ip to lan_ipaddr
			{
				FOR_EACH_WAN_IP
					save2file(FW_NAT, "-A PREROUTING -p UDP -d %s --dport %s %s -j DNAT --to %s:%s\n", \
						wan_ipaddr[k], pub_ports, schedule2iptables(schedule_name), lan_ipaddr, pri_ports);
			}
			else
#endif // #ifdef WAKE_ON_LAN
			{
				FOR_EACH_WAN_IP
					save2file(FW_NAT, "-A PREROUTING -p %s -d %s %s %s -j DNAT --to %s%s\n",\
						numProto, wan_ipaddr[k], port_1, schedule2iptables(schedule_name), lan_ip, port_3);
				/*	Date:	2009-04-09
				*	Name:	jimmy huang
				*	Reason:	LAN_B is virtual server, LAN_A use WAN IP to connect LAN_B
				*/
				FOR_EACH_WAN_IP
					save2file(FW_NAT, "-I POSTROUTING -p %s -s %s/%s -d %s %s -j SNAT --to %s\n",\
						numProto, lan_ipaddr,lan_netmask, lan_ip ,port_2, wan_ipaddr[k]);
			}
		}
	}
	else/* Allow All implies not to apply inbound filter */
#endif//#ifdef INBOUND_FILTER
	{
		printf("Virtual Server NOT apply to Inbound Filter\n");
#ifdef WAKE_ON_LAN
		if(type==2 && wake_on_lan==1) //using lan_ipaddr as wake-on-lan proxy => change lan_ip to lan_ipaddr
		{
			FOR_EACH_WAN_IP
				save2file(FW_NAT, "-A PREROUTING -p UDP -d %s --dport %s %s -j DNAT --to %s:%s\n", \
					wan_ipaddr[k], pub_ports, schedule2iptables(schedule_name), lan_ipaddr, pri_ports);
		}
		else
#endif //#ifdef WAKE_ON_LAN
		{
			FOR_EACH_WAN_IP
				save2file(FW_NAT, "-A PREROUTING -p %s -d %s %s %s -j DNAT --to %s%s\n",
					numProto, wan_ipaddr[k], port_1, schedule2iptables(schedule_name), lan_ip, port_3);
			/*	Date:	2009-04-09
			*	Name:	jimmy huang
			*	Reason:	LAN_B is virtual server, LAN_A use WAN IP to connect LAN_B
			*/
			FOR_EACH_WAN_IP
				save2file(FW_NAT, "-I POSTROUTING -p %s -s %s/%s -d %s %s -j SNAT --to %s\n",\
					 numProto, lan_ipaddr, lan_netmask, lan_ip, port_2, wan_ipaddr[k]);
		}

		/*Albert:If we setting pptp in virtual feature, we need to add gre rule in iptable*/
		if(type ==1)
		{
			if(!strcmp(private_port_range_1,"1723"))
				FOR_EACH_WAN_IP
					save2file(FW_NAT, "-A PREROUTING -p 47 -d %s %s -j DNAT --to %s\n", \
						wan_ipaddr[k], schedule2iptables(schedule_name), lan_ip);
		}

		/*
		* Date:2009-01-06
		* Name:Albert Chen
		* Reason: when user turn on 1720 port we need also turn on port 1503 for netmeeting application.
		*/
		if(type ==1)
		{
			if(!strcmp(private_port_range_1,"1720"))
				FOR_EACH_WAN_IP
					save2file(FW_NAT, "-A PREROUTING -p TCP -d %s --dport 1503 %s -j DNAT --to %s:1503\n", \
						wan_ipaddr[k], schedule2iptables(schedule_name), lan_ip);
		}
	}

#ifdef WAKE_ON_LAN
	if(type==2 && wake_on_lan==1)//using lan_ipaddr as wake-on-lan proxy => change lan_ip to lan_ipaddr
	{
		save2file(FW_FILTER,"-A FORWARD -o %s -p UDP -d %s --dport %s %s -m state --state NEW -j ACCEPT\n", \
			bri_eth, lan_ipaddr, pri_ports, schedule2iptables(schedule_name));
	}
	else
#endif //#ifdef WAKE_ON_LAN
	{
		save2file(FW_FILTER,"-A FORWARD -o %s -p %s -d %s %s %s -m state --state NEW -j ACCEPT\n",\
		bri_eth, numProto, lan_ip, port_2, schedule2iptables(schedule_name));
	}

	if(type ==1)
	{
		/*Albert:If we setting pptp in virtual feature, we need to add gre rule in iptable*/
		if(!strcmp(private_port_range_1,"1723"))
			save2file(FW_FILTER,"-A FORWARD -p 47 -d %s %s -j ACCEPT\n", lan_ip, schedule2iptables(schedule_name));
		/*
		* Date:2009-01-06
		* Name:Albert Chen
		* Reason: when user turn on 1720 port we need also turn on port 1503 for netmeeting application.
		*/
		if(!strcmp(private_port_range_1,"1720"))
			save2file(FW_FILTER,"-A FORWARD -p tcp -d %s --dport 1503 %s -m state --state NEW -j ACCEPT\n", \
			lan_ip, schedule2iptables(schedule_name));
	}
#ifdef WAKE_ON_LAN
	if(type ==2 && wake_on_lan==1)
	{
		printf("wakeOnLanProxy: pri_ports=%s\n", pri_ports);
		FOR_EACH_WAN_IP
			_system("wakeOnLanProxy %s %s %s %s &", lan_ipaddr, lan_netmask, lan_ipaddr, pri_ports);
	}
#endif //#ifdef WAKE_ON_LAN
	DEBUG_MSG("set_virtual_server:finish\n");
}
//==================================================================================================================

void set_ProcSysNet_parameter(void)
{
	DEBUG_MSG("set_ProcSysNet_parameter\n");
	/* Network Performance */
#ifndef CONFIG_USER_FASTNAT_APPS
	_system("echo \"%d\" > /proc/sys/net/ipv4/netfilter/ip_conntrack_max", MAX_IP_CONNTRACK_NUMBER);
#else
	system("echo \"512\" > /proc/sys/net/ipv4/netfilter/ip_conntrack_max"); /*from 4096 to 512*/
#endif
	system("echo \"512\" > /proc/sys/net/ipv4/netfilter/ip_conntrack_buckets");

	system("echo \"1024 65000\" > /proc/sys/net/ipv4/ip_local_port_range");

    /*65 for XBOX session policy and 24hr test */
    system("echo \"65\" > /proc/sys/net/ipv4/netfilter/ip_conntrack_udp_timeout");

    /* from 180 to 65 for [ASSURE]_UDP_session.
    After disable trigger port, the rule may still work for 65 seconds if it's in ASSURE state */
    /* from 65 to 80 for "BT certification"(udp pinhole) , NickChou 20090521 */
    system("echo \"80\" > /proc/sys/net/ipv4/netfilter/ip_conntrack_udp_timeout_stream");

	system("echo 50 > /proc/sys/net/ipv4/netfilter/ip_conntrack_generic_timeout");
#ifndef CONFIG_USER_FASTNAT_APPS
	/*from 432000 to 2400, prevent conntrack table full*/
   	system("echo \"2400\" > /proc/sys/net/ipv4/netfilter/ip_conntrack_tcp_timeout_established");
#else
   	system("echo \"120\" > /proc/sys/net/ipv4/netfilter/ip_conntrack_tcp_timeout_established");
#endif
	system("echo 5 > /proc/sys/net/ipv4/netfilter/ip_conntrack_tcp_timeout_close");
	system("echo 120 > /proc/sys/net/ipv4/netfilter/ip_conntrack_tcp_timeout_close_wait");
	system("echo 60 > /proc/sys/net/ipv4/netfilter/ip_conntrack_tcp_timeout_time_wait");

/*
 *	Date: 2009-10-08
 *	Name: Gareth Williams <gareth.williams@ubicom.com>
 *	Reason: Streamengine support - a streamengine iteration can take far longer than 3 seconds
 */
#ifndef CONFIG_USER_STREAMENGINE
	/*from 30 to 3, prevent when change WAN proto, LAN IP passthrough to WAN.*/
	system("echo 3 > /proc/sys/net/ipv4/netfilter/ip_conntrack_icmp_timeout");
#endif

	/* refer to: platform/AR9100/kernels/mips-linux-2.6.15/Documentation/networking/ip-sysctl.txt) */
	system("echo 1 > /proc/sys/net/ipv4/conf/all/arp_ignore");
	DEBUG_MSG("set_ProcSysNet_parameter finish\n");
}
//===================================================================================================

#ifdef DIR652V
static int is_sslvpn_enable()
{
	int i = 1;
	char *p, nvk[24];

	for (; i <= 6; i++) {
		sprintf(nvk, "sslvpn%d", i);
		p = nvram_safe_get(nvk);
		if (*p == '1')
			goto enable;
	}

	return 0;
enable:
	return 1;
}

static int do_iprange_firewall(const char *ipr, const char *sslport)
{
	char *s, *p, iprange_fmt[1024], fw_rule[256];

	p = iprange_fmt;
	strcpy(iprange_fmt, ipr);

	while ((s = strsep(&p, ",;")) != NULL) {
#ifdef IP8000	
		sprintf(fw_rule, "-A INPUT ! -i %s -p tcp --dport %s ",
			nvram_safe_get("lan_bridge"), sslport);
#else
		sprintf(fw_rule, "-A INPUT -i ! %s -p tcp --dport %s ",
			nvram_safe_get("lan_bridge"), sslport);
#endif

		if (strcmp(iprange_fmt, "*") == 0)
			strcat(fw_rule, "-j ACCEPT\n");
		else if (strchr(s, '-') == NULL)
			sprintf(fw_rule, "%s-s %s -j ACCEPT\n", fw_rule, s);
		else
			sprintf(fw_rule, "%s-m iprange --src-range %s -j ACCEPT\n",
				fw_rule, s);
		save2file(FW_FILTER, fw_rule);
	}

	return 0;
}
#endif

//==================================================================================================================
int start_firewall(void)
{
#if defined(HAVE_HTTPS)
#define HTTPS_PORT	443
#endif

#ifdef AP_NOWAN_HASBRIDGE
	return 1;
#endif
	char fw_rule[]="fw_XXXXXXXXXXXXXXXXXXXXXXXXXX";
	static char fw_value[256]={}, *getValue;
	char *lan_ipaddr, *lan_netmask;
	char *enable, *name, *wan_proto, *pub_ports, *pri_ports, *lan_ip;
	char *lan_proto;
	char *trigger_ports;
	char *schedule_name;
	char *tcp_ports, *udp_ports;
	char tcp_port_range[64]={0};
	char action[10];
	int i, j, k, u, p;
	struct stat filebuf;
	struct IpRange ip_range[8];
	char *inbound_name = NULL;
	int inbound_ip_count=0;
	char *src[MAX_SRC_MACHINE_NUMBER]={NULL};
	int src_count=0;
	int russia_enable = 0;
	int other_count =0 ;
	int enable_url_filter =0 ;
	struct OtherMachineInfo other_machine[MAX_ACCESS_CONTROL_NUMBER];
	char numProto[6]={0};
	char *wan_protocal=NULL, *wan_eth=NULL;
#ifdef ACCESS_CONTROL
	char *src_machine, *log, *filter_action;
#endif

	/* Chun modify:
	 * a)
	 * When wan protocol are PPPoE/PPTP/L2TP, the interface of static route
	 * as "ppp0" isn't available until the "ppp0" is connected (dialed).
	 * Since PPPoE/PPTP/L2TP will only restart firewall/apps after dialed up,
	 * move start_route() from start_wan() to start_firewall() is reasonable.
	 * b)
	 * When wan protocol is russia_pppoe, user could configure routing table
	 * for WAN_PHY(eth0) and WAN(ppp0). In case of manual or dial-on-demand mode,
	 * since ppp0 is not avaiable until user want to dial up and start_firewall()
	 * don't run since ppp0 wan_ip is NULL, putting start_route() in the end of
	 * start_firewall() will fail to add routing rule for eth0. Therefore , I decide
	 * to run start_route() in the beginning of start_firewall().
	 * c)
	 * After PPPOE dialing up, it will add an "address" routing rule.
	 * For example, "111.111.222.111   0.0.0.0   255.255.255.255   ppp0"
	 * It effects that other rules in regards to dev=ppp0 and gateway=111.111.222.XXX
	 * will fail to be added. However, it's not a big issue. Adding routing rule with ppp0
	 * is meaningless. Noce the router dial up tp PPPoE Server, there will be a default
	 * route to PPPoE server, and it can access to internet regardless of the routing
	 * rules are added or not!
	 */

	/* Start route */
	/* static route & dynamic */
#ifndef UNSUPPORT_ROUTE
	start_route();
#endif

#ifdef	CONFIG_BRIDGE_MODE
	if(nvram_match("wlan0_mode", "ap") == 0)
	{
		printf("Start firewall ap mode\n");
		flush_iptables();
		return 1;
	}
	else
#endif
	{
		if(!( action_flags.all || action_flags.lan || action_flags.wan || action_flags.app || action_flags.firewall ))
			return 1;
	}

	DEBUG_MSG("********** Start firewall initiation **********\n");

	wan_protocal = nvram_safe_get("wan_proto");
    wan_eth = nvram_safe_get("wan_eth");
    lan_ipaddr = nvram_safe_get("lan_ipaddr");
	lan_netmask = nvram_safe_get("lan_netmask");
	strcpy(bri_eth, nvram_safe_get("lan_bridge"));

#ifdef RPPPOE
	/* Check if Russia is enabled */
	if ((( strcmp(wan_protocal, "pppoe") ==0 ) && ( nvram_match("wan_pppoe_russia_enable", "1") ==0 )) ||
		(( strcmp(wan_protocal, "pptp") ==0 ) && ( nvram_match("wan_pptp_russia_enable", "1") ==0 )) ||
		(( strcmp(wan_protocal, "l2tp") ==0 ) && ( nvram_match("wan_l2tp_russia_enable", "1") ==0 )) )
		russia_enable = 1;
#endif //#ifdef RPPPOE

	set_wan_ip_if(russia_enable, wan_protocal, wan_eth);

	if(!(strlen(wan_ipaddr[0])) && !(strlen(wan_ipaddr[1])))
	{
		printf("wan_ipaddr == NULL, firewall don't start\n");
		memset(ori_wan_ipaddr[0], 0, sizeof(ori_wan_ipaddr[0]));
		memset(ori_wan_ipaddr[1], 0, sizeof(ori_wan_ipaddr[1]));

		clear_firewall();

		/* when wan is disconnected, mac_filter should works */
		if( (nvram_match("mac_filter_type", "list_allow") == 0) || (nvram_match("mac_filter_type", "list_deny") ==0))
		{
			_system("iptables -t nat -A PREROUTING -i %s -p udp --sport 68 --dport 67 -j DROP\n", bri_eth);

			if(mac_filter_type_allow() == 1)
				set_tables_policy(0, 1);
			else
				set_tables_policy(0, 0);

#if !(ONLY_FILTER_WLAN_MAC)
			set_mac_filter_rule();
#endif
		}
		else{
			set_tables_policy(0, 0);
		}
#if defined(HAVE_HTTPS)
{
		char *https_enable, *remote_https_enable, *remote_https_port, *remote_http_port;
		char fw_value[256]={}, *getValue;
		DEBUG_MSG("Start firewall HAVE_HTTPS\n");
		getValue = nvram_safe_get("https_config");
		strncpy(fw_value, getValue, sizeof(fw_value));

		getStrtok(fw_value, "/", "%s %s %s %s", &https_enable, &remote_https_enable, &remote_https_port, &remote_http_port);

		if (strcmp(https_enable, "0") == 0) {
			save2file(FW_NAT, "-A PREROUTING -i %s -d %s -p tcp --dport %d -j DROP\n", bri_eth, lan_ipaddr, HTTPS_PORT);
		}
}
#endif	/* HAVE_HTTPS */
        /* Disable RPC port (111) */
        save2file(FW_NAT, "-A PREROUTING -i %s -d %s -p tcp --dport %d -j DROP\n", bri_eth, lan_ipaddr, 111);

		save2file(FW_NAT, "COMMIT\n");
		_system("iptables-restore %s", FW_NAT);
		_system("cp %s %s_LAST", FW_NAT, FW_NAT);
		unlink(FW_NAT);
		save2file(FW_FILTER, "COMMIT\n");
		_system("iptables-restore %s", FW_FILTER);
		_system("cp %s %s_LAST", FW_FILTER, FW_FILTER);
		unlink(FW_FILTER);
		save2file(FW_MANGLE, "COMMIT\n");
		_system("iptables-restore %s", FW_MANGLE);
		_system("cp %s %s_LAST", FW_MANGLE, FW_MANGLE);
		unlink(FW_MANGLE);

		return 0;
	}//!(strlen(wan_ipaddr[0])) && !(strlen(wan_ipaddr[1]))

	if( !(strlen(ori_wan_ipaddr[0])) && !(strlen(ori_wan_ipaddr[1])) )
	{
		strncpy(ori_wan_ipaddr[0], wan_ipaddr[0], strlen(wan_ipaddr[0]));
		strncpy(ori_wan_ipaddr[1], wan_ipaddr[1], strlen(wan_ipaddr[1]));
		DEBUG_MSG("start_firewall: init ori_wan_ipaddr\n");
	}
	else if( strcmp(ori_wan_ipaddr[0], wan_ipaddr[0]) || strcmp(ori_wan_ipaddr[1], wan_ipaddr[1]) )
	{
		DEBUG_MSG("start_firewall: ori_wan_ipaddr != now_wan_ipaddr\n");
		memset(ori_wan_ipaddr[0], 0, sizeof(ori_wan_ipaddr[0]));
		memset(ori_wan_ipaddr[1], 0, sizeof(ori_wan_ipaddr[1]));
		strncpy(ori_wan_ipaddr[0], wan_ipaddr[0], strlen(wan_ipaddr[0]));
		strncpy(ori_wan_ipaddr[1], wan_ipaddr[1], strlen(wan_ipaddr[1]));
		DEBUG_MSG("start_firewall: update ori_wan_ipaddr\n");
	}
	else
	{
		DEBUG_MSG("start_firewall: WAN IP don't change\n");
		DEBUG_MSG("action_flags.wan:%d, action_flags.app:%d, action_flags.firewall:%d\n", action_flags.wan, action_flags.app, action_flags.firewall);
		if( action_flags.wan || action_flags.app )
		{
			printf("start_firewall: firewall rule don't change from GUI\n");
			return 0;
		}
	}

	printf("Start firewall Setting\n");

	clear_firewall();

	/*Deny WAN hosts ping router's LAN IP. */
	FOR_EACH_WAN_FACE
		_system("iptables -A INPUT -p ICMP -i %s -d %s -j DROP\n", wan_if[k], lan_ipaddr);

#ifdef AR9100
/* 	for passive ftp client pass through our router when no virtual server is set
    By defult, these 2 modules will monitor port 21
*/
	system("[ -n \"`lsmod | grep nf_nat_ftp`\" ] && rmmod nf_nat_ftp.ko");
	system("[ -n \"`lsmod | grep nf_conntrack_ftp`\" ] && rmmod nf_conntrack_ftp.ko");
	_system("insmod %s/nf_conntrack_ftp.ko", CONNTRACK_LIB_PATH);
	_system("insmod %s/nf_nat_ftp.ko", NAT_LIB_PATH);
#endif

	//Load other connection tracking modules
	_system("[ \"0\" == \"`lsmod | grep 'nf_conntrack_tftp' | wc -l`\" ] && insmod %s/nf_conntrack_tftp.ko", CONNTRACK_LIB_PATH);
	_system("[ \"0\" == \"`lsmod | grep 'nf_nat_tftp' | wc -l`\" ] && insmod %s/nf_nat_tftp.ko", NAT_LIB_PATH);
	_system("[ \"0\" == \"`lsmod | grep 'nf_conntrack_h323' | wc -l`\" ] && insmod %s/nf_conntrack_h323.ko", CONNTRACK_LIB_PATH);
	_system("[ \"0\" == \"`lsmod | grep 'nf_nat_h323' | wc -l`\" ] && insmod %s/nf_nat_h323.ko", NAT_LIB_PATH);
	_system("[ \"0\" == \"`lsmod | grep 'nf_conntrack_irc' | wc -l`\" ] && insmod %s/nf_conntrack_irc.ko", CONNTRACK_LIB_PATH);
	_system("[ \"0\" == \"`lsmod | grep 'nf_nat_irc' | wc -l`\" ] && insmod %s/nf_nat_irc.ko", NAT_LIB_PATH);
	_system("[ \"0\" == \"`lsmod | grep 'nf_conntrack_sip' | wc -l`\" ] && insmod %s/nf_conntrack_sip.ko", CONNTRACK_LIB_PATH);
	_system("[ \"0\" == \"`lsmod | grep 'nf_nat_sip' | wc -l`\" ] && insmod %s/nf_nat_sip.ko", NAT_LIB_PATH);

 	if( nvram_match("alg_rtsp", "1") == 0 )
        {
                _system("[ \"0\" == \"`lsmod | grep 'nf_conntrack_rtsp' | wc -l`\" ] && insmod %s/nf_conntrack_rtsp.ko", CONNTRACK_LIB_PATH);
                _system("[ \"0\" == \"`lsmod | grep 'nf_nat_rtsp' | wc -l`\" ] && insmod %s/nf_nat_rtsp.ko", NAT_LIB_PATH);
        }
        else
        {
                system("[ -n \"`lsmod | grep nf_nat_rtsp`\" ] && rmmod nf_nat_rtsp.ko");
                system("[ -n \"`lsmod | grep nf_conntrack_rtsp`\" ] && rmmod nf_conntrack_rtsp.ko");

        }

	set_ProcSysNet_parameter();

	/* ==0 mean firewall is running, do not double insert firewall rules */
	if( stat(FW_FILTER, &filebuf) == 0 )
	{
		printf("firewall is running, firewall don't start\n");
		return 0;
	}

	set_tables_policy(1, 0);

	if(nvram_match("log_dropped_packets", "1") == 0) {
		save2file(FW_NAT,"-A LOG_AND_DROP -j LOG --log-level info --log-prefix '[DROPPED-PACKET]:'\n");
		save2file(FW_FILTER,"-A LOG_AND_DROP -j LOG --log-level info --log-prefix '[DROPPED-PACKET]:'\n");
	}
	save2file(FW_NAT,"-A LOG_AND_DROP -j DROP\n");
	save2file(FW_FILTER,"-A LOG_AND_DROP -j DROP\n");

	/*Ident Stealth enable*/
	if(0) //John 2010.04.23 remove unused ( nvram_match("ident_enable", "1" ) == 0 )
		save2file(FW_FILTER,"-A INPUT -p tcp --dport %d -j REJECT --reject-with tcp-reset\n",IDENT_PORT);

/*  Date: 2009-3-9
*   Name: Ken Chiang
*   Reason: Move to below to fix Anti-Spoof don't show message in syslog.
*   Notice :
*/
/*
	save2file(FW_FILTER,"-A INPUT -p tcp -m state --state NEW -m tcp --tcp-flags ALL ACK -j LOG_AND_DROP\n");
*/

	/*prevent igmp packet to wan, 239.255.255.250 */
	save2file(FW_FILTER,"-A OUTPUT -d 239.255.255.250 -o %s -j DROP\n", wan_eth);

	DEBUG_MSG("\tUPNP AddPortMapping\n");
	/* miniupnpd AddPortMapping function */
	// GGG Dynamically done now 14/dec/2009 Gareth Williams gwilliams@ubicom.com save2file(FW_FILTER,"-A FORWARD -j MINIUPNPD\n");

#if defined(UPNP_ATH_MINIUPNPD_1_3) && defined(UPNP_ATH_MINIUPNPD_1_3_HAIRPIN)
	// because we add rules to MINIUPNPD chain for hairpin
	// so we need not only wan packet but also lan packet to go into MINIUPNPD chain
	// GGG Dynamically done now 14/dec/2009 Gareth Williams gwilliams@ubicom.com 	save2file(FW_NAT,"-A PREROUTING -j MINIUPNPD\n");
#else
	// GGG Dynamically done now 14/dec/2009 Gareth Williams gwilliams@ubicom.com     FOR_EACH_WAN_FACE
	// GGG Dynamically done now 14/dec/2009 Gareth Williams gwilliams@ubicom.com 	    save2file(FW_NAT,"-A PREROUTING -i %s -j MINIUPNPD\n", wan_if[k]);
#endif
	DEBUG_MSG("UPNP AddPortMapping finish\n");

	DEBUG_MSG("\tSPI function\n");
	/* SPI function */
	if( nvram_match("spi_enable", "1" ) == 0 )  {
#ifdef ADVANCE_SPI		
		FOR_EACH_WAN_FACE
		{
		    save2file(FW_NAT,"-A PREROUTING -i %s -j SPI_PREROUT\n", wan_if[k]);
		    save2file(FW_FILTER,"-A OUTPUT -o %s -j SPI_OUTPUT\n", wan_if[k]);
		    save2file(FW_FILTER,"-A INPUT -i %s -j SPI_INPUT\n", wan_if[k]);
		}
		save2file(FW_FILTER,"-A INPUT -m state --state INVALID -j LOG_AND_DROP\n");
		save2file(FW_FILTER,"-A FORWARD -j SPI_FORWARD\n");
#endif		
        system("echo 1 > /proc/sys/net/ipv4/spi_enable");
	}else
		system("echo 0 > /proc/sys/net/ipv4/spi_enable");

    DEBUG_MSG("SPI function finish\n");


	if(!strcmp(wan_protocal, "unnumbered")){
		set_unnumber_snat();
	}

	FOR_EACH_WAN_FACE
		save2file(FW_NAT, "-A POSTROUTING -o %s -s %s/%s -j MASQUERADE\n", wan_if[k], lan_ipaddr, lan_netmask);
		//save2file(FW_NAT, "-A POSTROUTING -o %s -s %s/%s -j SNATP2P --to-source %s\n", wan_if[k], lan_ipaddr, lan_netmask, wan_ipaddr[k]);
	/* MTU, MSS */
	FOR_EACH_WAN_FACE
		save2file(FW_FILTER, "-A FORWARD -o %s -p tcp -m tcp --tcp-flags SYN,RST SYN -j TCPMSS --clamp-mss-to-pmtu\n",wan_if[k]);

	 /*	Enable NAT settings if nat_enable!=0 */
#ifdef NAT_ENABLE
	DEBUG_MSG("\tNAT function\n");
	set_pass_through(wan_eth, wan_protocal, russia_enable);

	//ALG RTSP
	if( nvram_match("alg_rtsp", "0") == 0 ){
		FOR_EACH_WAN_FACE
			save2file(FW_FILTER,"-A FORWARD -o %s -p tcp --dport %d -j LOG_AND_DROP\n", wan_if[k], RTSP_PORT);
#ifdef RPPPOE
		FOR_EACH_WAN_FACE
			save2file(FW_FILTER,"-A FORWARD -o %s -p tcp --dport 557 -j LOG_AND_DROP\n", wan_if[k]);
#endif
		}

	if( nvram_match("alg_sip", "0") == 0 ){
		FOR_EACH_WAN_FACE
			save2file(FW_FILTER,"-A FORWARD -o %s -p udp --dport %d -j LOG_AND_DROP\n", wan_if[k], SIP_PORT);
	}

#ifndef ACCESS_CONTROL
	set_firewall_rule();
#endif

	/*Block service*/
	set_block_service();

#ifdef MIII_BAR
	if( nvram_match("miiicasa_enabled", "1") == 0 ){
#ifdef IP8000		
		save2file(FW_NAT,"-I PREROUTING -i %s ! -s %s ! -d %s -p tcp --dport 80 %s -j DNAT --to-destination %s:%d\n", 
		bri_eth,lan_ipaddr,lan_ipaddr, schedule2iptables("Always"), lan_ipaddr,WEBFILTER_PROXY_PORT);
#else
		save2file(FW_NAT,"-I PREROUTING -i %s -s ! %s -d ! %s -p tcp --dport 80 %s -j DNAT --to-destination %s:%d\n", 
		bri_eth,lan_ipaddr,lan_ipaddr, schedule2iptables("Always"), lan_ipaddr,WEBFILTER_PROXY_PORT);		
#endif
	}
#endif
	/*Virtaul Server*/
	DEBUG_MSG("\tSet Virtual Server\n");
	for(i=0; i<MAX_VIRTUAL_SERVER_NUMBER; i++)
	{
		memset(fw_rule, 0, sizeof(fw_rule));
		snprintf(fw_rule, sizeof(fw_rule), "vs_rule_%02d", i);
		getValue = nvram_safe_get(fw_rule);
		if ( strlen(getValue) == 0 )
			continue;
		else
			strncpy(fw_value, getValue, sizeof(fw_value));

		if( getStrtok(fw_value, "/", "%s %s %s %s %s %s %s %s",
			&enable, &name, &wan_proto, &pub_ports, &pri_ports, &lan_ip, &schedule_name, &inbound_name) != 8 )
		{
			printf("vs_rule_%02d: parser error\n", i);
			continue;
		}

		if(strcmp(schedule_name, "Never") == 0)
			continue;

		if( strcmp("1", enable) == 0 )
		{
			/*If we fail to transfer protocl to number, skip the rule.*/
			if(!transferProtoToNumber(wan_proto, numProto))
				continue;

#ifdef AR9100
/* for passive ftp client pass through our router
	- Once we change UI to suopport more than one "FTP" (application name),
		we need to record all FTP ports,then insmod with all ports
	- Currently, kernel module ip_conntrack_ftp.c only support 8 ports (#define MAX_PORTS 8)
	usage:
		ex: insmod nf_conntrack_ftp.ko ports=21,1000,3000
*/
			if(strcmp(name, "FTP" ) == 0){
				system("rmmod nf_nat_ftp.ko");
				system("rmmod nf_conntrack_ftp.ko");
				_system("insmod %s/nf_conntrack_ftp.ko ports=21,%s", CONNTRACK_LIB_PATH, pri_ports);
				_system("insmod %s/nf_nat_ftp.ko ports=21,%s", NAT_LIB_PATH, pri_ports);
			}
#endif
			/* TCP and Both */
			if( !strcmp(numProto,"6") || !strcmp(numProto,"256") || !strcmp("NetMeeting", name) )
				set_virtual_server(i, 1, lan_ipaddr, lan_netmask, "6");

			/* UDP and Both */
			if( ( !strcmp(numProto,"17") || !strcmp(numProto,"256") ) && (strcmp("NetMeeting", name)) )
				set_virtual_server(i, 2, lan_ipaddr, lan_netmask, "17");

			/* Any and Other
			* Any: all protocol without port number
			* Other: specific protocol without port number
			*/
			if((strcmp(numProto,"6")) && (strcmp(numProto,"17")) && (strcmp(numProto,"256")))
				set_virtual_server(i, 3, lan_ipaddr, lan_netmask, numProto);
		}
	}
	DEBUG_MSG("Set Virtual Server finished\n");

#ifdef CONFIG_USER_FIREWALL2	/* replaces portfoward rule */
	setup_port_forwarding(wan_if, wan_ipaddr, bri_eth);
#else
#ifdef PORT_FORWARD_BOTH
		 /*	Port Forwarding format with both TCP and UDP
		 *  	Nvram name:  port_forward_both_%02d
		 *  	Format:  enable/name/lan_ip/tcp_ports/udp_ports/schedule/inbound_filter
		 *
		 */
		DEBUG_MSG("\tPort Forwarding format with both TCP and UDP\n");
		for(i=0; i<MAX_PORT_FORWARDING_NUMBER; i++)
		{
			snprintf(fw_rule, sizeof(fw_rule), "port_forward_both_%02d", i);
			getValue = nvram_safe_get(fw_rule);
			if ( getValue == NULL ||  strlen(getValue) == 0 )
				continue;
			else
				strncpy(fw_value, getValue, sizeof(fw_value));

			getStrtok(fw_value, "/", "%s %s %s %s %s %s %s ", \
				&enable, &name, &lan_ip, &tcp_ports, &udp_ports, &schedule_name, &inbound_name);

			if(strcmp(schedule_name, "Never") == 0)
			   	continue;

			if(strcmp("1", enable) == 0)
			{
#ifdef INBOUND_FILTER
				if(inbound_name && strlen(inbound_name))
				{
					/* Deny All implies that the rule is invalid */
					if(!strcmp(inbound_name, "Deny_All"))
						continue;
					memset(action,0,sizeof(action));
					memset(ip_range,0,8*sizeof(struct IpRange));
					inbound_ip_count = inbound2iptables(inbound_name, action, ip_range);
				}
#endif
				/* TCP */
				if(strcmp(tcp_ports,"0") || (strcmp("NetMeeting", name) == 0))
				{
					memset(tcp_port_range,0,sizeof(tcp_port_range));
					if((strcmp("NetMeeting", name) == 0))
						strcpy(tcp_port_range,"1503");
					else
					{
						removeMark(tcp_ports);
						strcpy(tcp_port_range,tcp_ports);
					}
#ifdef INBOUND_FILTER
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
					if(inbound_ip_count && strcmp(inbound_name, "Allow_All"))
					{
						if( !strcmp(action, "allow") )
						{
							for(j=0;j<inbound_ip_count;j++)
							{
								FOR_EACH_WAN_IP
									save2file(FW_NAT, "-A PREROUTING -m iprange --src-range %s -p TCP -d %s -m multiport --destination-ports %s %s -j DNAT --to %s\n",\
										ip_range[j].addr, wan_ipaddr[k], tcp_port_range, schedule2iptables(schedule_name), lan_ip);
							}
						}
						else
						{
							for(j=0;j<inbound_ip_count;j++)
							{
								FOR_EACH_WAN_IP
									save2file(FW_NAT, "-A PREROUTING -m iprange --src-range %s -p TCP -d %s -m multiport --destination-ports %s %s -j ACCEPT\n", \
										ip_range[j].addr, wan_ipaddr[k], tcp_port_range, schedule2iptables(schedule_name));
							}
							FOR_EACH_WAN_IP
								save2file(FW_NAT, "-A PREROUTING -p TCP -d %s -m multiport --destination-ports %s %s -j DNAT --to %s\n",\
									wan_ipaddr[k], tcp_port_range, schedule2iptables(schedule_name), lan_ip);
						}
					}
					/* Allow All implies not to apply inbound filter */
					else
#endif
					{
						FOR_EACH_WAN_IP
							save2file(FW_NAT, "-A PREROUTING -p TCP -d %s -m multiport --destination-ports %s %s -j DNAT --to %s\n",\
								wan_ipaddr[k], tcp_port_range, schedule2iptables(schedule_name), lan_ip);
					}
					save2file(FW_FILTER,"-A FORWARD -o %s -p TCP -d %s -m multiport --destination-ports %s %s -m state --state NEW -j ACCEPT\n", \
						bri_eth, lan_ip, tcp_port_range, schedule2iptables(schedule_name));
				}

				/* UDP */
				if(strcmp(udp_ports,"0"))
				{
					removeMark(udp_ports);
					{
#ifdef INBOUND_FILTER
						/* inbound_ip_count >0 implies to apply inbound filter. */
						if(inbound_ip_count && strcmp(inbound_name, "Allow_All"))
						{
							if( !strcmp(action, "allow") )
							{
								for(j=0;j<inbound_ip_count;j++)
								{
									FOR_EACH_WAN_IP
										save2file(FW_NAT, "-A PREROUTING -m iprange --src-range %s -p UDP -d %s -m multiport --destination-ports %s %s -j DNAT --to %s\n",\
											ip_range[j].addr, wan_ipaddr[k], udp_ports, schedule2iptables(schedule_name), lan_ip);
								}
							}
							else
							{
								for(j=0;j<inbound_ip_count;j++)
								{
									FOR_EACH_WAN_IP
										save2file(FW_NAT, "-A PREROUTING -m iprange --src-range %s -p UDP -d %s -m multiport --destination-ports %s %s -j ACCEPT\n", \
											ip_range[j].addr, wan_ipaddr[k], udp_ports, schedule2iptables(schedule_name));
								}
								FOR_EACH_WAN_IP
									save2file(FW_NAT,"-A PREROUTING -p UDP -d %s -m multiport --destination-ports %s %s -j DNAT --to %s\n", \
										wan_ipaddr[k], udp_ports, schedule2iptables(schedule_name), lan_ip);
							}
						}
						/* Allow All implies not to apply inbound filter */
						else
#endif
						{
							FOR_EACH_WAN_IP
								save2file(FW_NAT,"-A PREROUTING -p UDP -d %s -m multiport --destination-ports %s %s -j DNAT --to %s\n", \
									wan_ipaddr[k], udp_ports, schedule2iptables(schedule_name), lan_ip );
						}
						save2file(FW_FILTER,"-A FORWARD -o %s -p UDP -d %s -m multiport --destination-ports %s %s -m state --state NEW -j ACCEPT\n", \
							bri_eth, lan_ip, udp_ports, schedule2iptables(schedule_name));
					}
				}
			}
		}

#else //#ifdef PORT_FORWARD_BOTH
		/*	Port Forwarding
		 *  	Nvram name:  port_forward_%02d
		 *  	Format:  enable/name/proto/public_port_start/public_port_end/private_port_start/private_port_end/lan_ip/schedule_rule_00_name
		 *   	Iptables command:	iptables -A PREROUTING -t nat -p tcp -d 172.21.34.17 --dport 21:30 -j DNAT --to 192.168.0.100:21-30
		 *					   	iptables -A FORWARD -i eth0 -o br0 -p tcp --sport 1024:65535 -d 192.168.0.100 --dport 21:30 -m state --state NEW -j ACCEPT
		 */
		DEBUG_MSG("\tPort Forwarding\n");
		for(i=0; i<MAX_PORT_FORWARDING_NUMBER; i++) {
			snprintf(fw_rule, sizeof(fw_rule), "port_forward_%02d", i);
			getValue = nvram_safe_get(fw_rule);
			if ( getValue == NULL ||  strlen(getValue) == 0 )
				continue;
			else
				strncpy(fw_value, getValue, sizeof(fw_value));

			if( getStrtok(fw_value, "/", "%s %s %s %s %s %s %s %s %s", &enable, &name, &wan_proto, &public_port_start, &public_port_end, &private_port_start, &private_port_end, &lan_ip, &schedule_name) !=9 )
				continue;

			if(strcmp(schedule_name, "Never") == 0)
				continue;

			if( strcmp("1", enable) == 0)
			{
				if( (strcmp("TCP", wan_proto) == 0) || (strcmp("Any", wan_proto) == 0) || (strcmp("NetMeeting", name) == 0))
				{
					{
						memset(public_port_range,0,sizeof(public_port_range));
						memset(private_port_range_1,0,sizeof(private_port_range_1));
						memset(private_port_range_2,0,sizeof(private_port_range_2));

						if((strcmp("NetMeeting", name) == 0))
						{
							strcpy(public_port_range,"1503");
							strcpy(private_port_range_1,"1503");
							strcpy(private_port_range_2,"1503");
						}
						else
						{
							sprintf(public_port_range,"%s:%s",public_port_start,public_port_end);
							sprintf(private_port_range_1,"%s-%s",private_port_start,private_port_end);
							sprintf(private_port_range_2,"%s:%s",private_port_start,private_port_end);
						}

						FOR_EACH_WAN_IP
							save2file(FW_NAT, "-A PREROUTING -p TCP -d %s --dport %s %s -j DNAT --to %s:%s\n",\
								wan_ipaddr[k], public_port_range, schedule2iptables(schedule_name), lan_ip, private_port_range_1);
					}
					save2file(FW_FILTER,"-A FORWARD -o %s -p TCP -d %s --dport %s %s -m state --state NEW -j ACCEPT\n", \
						bri_eth, lan_ip, private_port_range_2, schedule2iptables(schedule_name));
				}

				if( ((strcmp("UDP", wan_proto) == 0) || (strcmp("Any", wan_proto) == 0)) && (strcmp("NetMeeting", name) ) )
				{
	                {
						FOR_EACH_WAN_IP
							save2file(FW_NAT,"-A PREROUTING -p UDP -d %s --dport %s:%s %s -j DNAT --to %s:%s-%s\n", \
								wan_ipaddr[k], public_port_start, public_port_end, schedule2iptables(schedule_name), lan_ip, private_port_start, private_port_end);
	                }
					save2file(FW_FILTER,"-A FORWARD -o %s -p UDP -d %s --dport %s:%s %s -m state --state NEW -j ACCEPT\n", \
						bri_eth, lan_ip, private_port_start, private_port_end, schedule2iptables(schedule_name));
				}
			}
		}
#endif //#ifdef PORT_FORWARD_BOTH
#endif /* CONFIG_USER_FIREWALL2: replaces portfoward rule */

#ifdef OPENDNS
		if( nvram_match("opendns_enable","1")==0 ) {
      // only family and parental control dns
			if( nvram_match("opendns_mode","3")==0 || nvram_match("opendns_mode","2")==0 )
#ifdef IP8000
			save2file(FW_NAT,"-I PREROUTING -i %s ! -d %s -p udp --dport 53 %s -j DNAT --to-destination %s:%d\n"
							   , bri_eth, lan_ipaddr, "", lan_ipaddr,53);
#else
			save2file(FW_NAT,"-I PREROUTING -i %s -d ! %s -p udp --dport 53 %s -j DNAT --to-destination %s:%d\n"
							   , bri_eth, lan_ipaddr, "", lan_ipaddr,53);
#endif
		}
#endif


#ifndef ACCESS_CONTROL
		/*	URL Domain Filter
		 *	Nvram name:	url_domain_filter_%02d
		 *	Nvram name: url_domain_filter_schedule_%02d
		 *	format: string
		 *	format: schedule_name
		 *	Iptables command:	 iptables -A FORWARD -p tcp --dport 80 -m string --string 'kimo' -j ACCEPT
		 */

        DEBUG_MSG("\tURL /domain filter:!using_access_control\n");
		/* Allow trust ip visit block website */
        for(i=0; i<MAX_TRUST_IP_NUMBER; i++)
		{
			snprintf(fw_rule, sizeof(fw_rule), "url_domain_filter_trust_ip_%02d", i);
			getValue = nvram_get(fw_rule);
			if ( getValue == NULL ||  strlen(getValue) == 0 )
				continue;
			else
				strncpy(fw_value, getValue, sizeof(fw_value));

			if( getStrtok(fw_value, "/", "%s %s", &enable, &trust_ip) !=2 )
				continue;

			if((enable != NULL) && (trust_ip != NULL) && (strcmp(enable, "1") == 0))
				save2file(FW_FILTER,"-A FORWARD -s %s -m urlfilter --url ALL -j ACCEPT\n", trust_ip);
		}

		if( (nvram_match("url_domain_filter_type", "list_allow") ==0) || (nvram_match("url_domain_filter_type", "list_deny") ==0) )
        {
			DEBUG_MSG("url_domain_filter_type==list_allow||list_deny\n");
			for(i=0; i<MAX_URL_FILTER_NUMBER; i++)
            {
				snprintf(fw_rule, sizeof(fw_rule), "url_domain_filter_%02d", i);
				snprintf(url_schedule, sizeof(url_schedule), "url_domain_filter_schedule_%02d", i);
				snprintf(url_enable, sizeof(url_enable), "url_domain_filter_enable_%02d", i);

				/* Check if the rule is enable */
				getValue = nvram_get(url_enable);
				//DEBUG_MSG("url_enable=%s,getValue=%s\n",url_enable,getValue);
				if ( getValue == NULL ||  strlen(getValue) == 0 || strcmp(getValue,"1") )
					continue;
				getValue = NULL;
				getValue = nvram_get(fw_rule);
				//DEBUG_MSG("fw_rule=%s,getValue=%s\n",fw_rule,getValue);
				if ( getValue == NULL ||  strlen(getValue) == 0 )
					continue;
				else{
					/* if url_rule = http://tw.yahoo.com
					   iptables command will ignore the key word "http" but keep the "://"
					   finally, it becomes url_rule://tw.yahoo.com, (will not work) */
					if(strstr(getValue,"http://") != NULL)
						strcpy(fw_value, getValue + strlen("http://"));
					else
						strcpy(fw_value, getValue);
				}

				if ( nvram_match("url_domain_filter_type", "list_allow") ==0 )
				{
					{
		                FOR_EACH_WAN_FACE
						{
			                save2file(FW_FILTER,"-A FORWARD -i %s -o %s %s -m urlfilter --url \'%s\' -j LOG --log-prefix \"Allowed Site=%d \"\n", bri_eth, wan_if[k], schedule2iptables( nvram_get(url_schedule) ), fw_value,i);
							save2file(FW_FILTER,"-A FORWARD -i %s -o %s %s -m urlfilter --url \"%s\" -j ACCEPT\n", bri_eth, wan_if[k], schedule2iptables( nvram_get(url_schedule) ), fw_value);
						}
					}
	            }
				else
				{
					{
					FOR_EACH_WAN_FACE
						{
			                save2file(FW_FILTER,"-A FORWARD -i %s -o %s %s -m urlfilter --url \"%s\" -j LOG --log-prefix \"Blocked Site=%d \"\n", bri_eth, wan_if[k], schedule2iptables( nvram_get(url_schedule) ), fw_value,i);
							save2file(FW_FILTER,"-A FORWARD -i %s -o %s %s -m urlfilter --url \"%s\" -j DROP\n", bri_eth, wan_if[k], schedule2iptables( nvram_get(url_schedule) ), fw_value);
						}
					}
				}
			}
		}
		/* If URL filter is list_allow, deny all http connections for default rule */
		if ( nvram_match("url_domain_filter_type", "list_allow") ==0 )
        {
            DEBUG_MSG("url_domain_filter_type==list_allow DROP\n");
	        FOR_EACH_WAN_FACE
				save2file(FW_FILTER,"-A FORWARD -i %s -o %s -m urlfilter --url ALL  -j LOG_AND_DROP\n", bri_eth, wan_if[k]);
		}
/*
 	Date: 2009-1-05
 	Name: Ken_Chiang
 	Reason: modify for blocked url to redirect the reject page by crowdcontrol(http proxy).
 	Notice :
*/
#ifdef CONFIG_USER_CROWDCONTROL //when ACCESS_CONTROL is not defined
	{  //To creat permit domains file or block domains file for crowdcontrol
	   // , added PREROUTING rule and start crowdcontrol demanes.
		int file_flag=0;
		PROXY_DEBUG_MSG("USED PROXY\n");
		if( (nvram_match("url_domain_filter_type", "list_allow") ==0) || (nvram_match("url_domain_filter_type", "list_deny") ==0) )
        {
			PROXY_DEBUG_MSG("PROXY url_domain_filter_type==list_allow||list_deny\n");
			for(i=0; i<MAX_URL_FILTER_NUMBER; i++)
            {
				snprintf(fw_rule, sizeof(fw_rule), "url_domain_filter_%02d", i);
				snprintf(url_schedule, sizeof(url_schedule), "url_domain_filter_schedule_%02d", i);
				snprintf(url_enable, sizeof(url_enable), "url_domain_filter_enable_%02d", i);

				/* Check if the rule is enable */
				getValue = nvram_get(url_enable);
				//PROXY_DEBUG_MSG("PROXY url_enable=%s,getValue=%s\n",url_enable,getValue);
				if ( getValue == NULL ||  strlen(getValue) == 0 || strcmp(getValue,"1") )
					continue;
				getValue = NULL;
				getValue = nvram_get(fw_rule);
				//PROXY_DEBUG_MSG("PROXY fw_rule=%s,getValue=%s\n",fw_rule,getValue);
				if ( getValue == NULL ||  strlen(getValue) == 0 )
					continue;
				else{
					/* if url_rule = http://tw.yahoo.com
					   iptables command will ignore the key word "http" but keep the "://"
					   finally, it becomes url_rule://tw.yahoo.com, (will not work)
					*/
					if(strstr(getValue,"http://") != NULL)
						strcpy(fw_value, getValue + strlen("http://"));
					else
						strcpy(fw_value, getValue);
				}

				if ( nvram_match("url_domain_filter_type", "list_allow") ==0 ) {

					PROXY_DEBUG_MSG("PROXY %s added rule=%s\n",PERMIT_DOMAINS_FILE,fw_value);
					if(!file_flag){
						init_file(PERMIT_DOMAINS_FILE);
						file_flag++;
					}
					save2file(PERMIT_DOMAINS_FILE, "%s\n", fw_value);
				}
				else{
					PROXY_DEBUG_MSG("PROXY %s added rule=%s\n",BLOCK_DOMAINS_FILE,fw_value);
					if(!file_flag){
						init_file(BLOCK_DOMAINS_FILE);
						file_flag++;
					}
					save2file(BLOCK_DOMAINS_FILE, "%s\n", fw_value);
				}
			}//end for

			if ( nvram_match("url_domain_filter_type", "list_allow") ==0 ) {
				PROXY_DEBUG_MSG("No ACCESS_CONTROL PROXY list_allow\n");
				_system("crowdcontrol -a %s -s %s -p %d --%s %s &",
						lan_ipaddr, lan_netmask, WEBFILTER_PROXY_PORT, "permit-domains", PERMIT_DOMAINS_FILE);
						//lan_ipaddr, lan_netmask, WEBFILTER_PROXY_PORT, "permit-urls", PERMIT_DOMAINS_FILE);
	        }
		    else {
				PROXY_DEBUG_MSG("No ACCESS_CONTROL PROXY list_deny\n");
				_system("crowdcontrol -a %s -s %s -p %d --%s %s &",
						lan_ipaddr, lan_netmask, WEBFILTER_PROXY_PORT, "deny-domains", BLOCK_DOMAINS_FILE);
						//lan_ipaddr, lan_netmask, WEBFILTER_PROXY_PORT, "deny-urls", BLOCK_DOMAINS_FILE);
		    }
		    
#ifdef IP8000
			save2file(FW_NAT,"-I PREROUTING -i %s ! -d %s -p tcp --dport 80 %s -j DNAT --to-destination %s:%d\n"
							   , bri_eth, lan_ipaddr, schedule2iptables( nvram_get(url_schedule) ), lan_ipaddr,WEBFILTER_PROXY_PORT);
#else
			save2file(FW_NAT,"-I PREROUTING -i %s -d ! %s -p tcp --dport 80 %s -j DNAT --to-destination %s:%d\n"
							   , bri_eth, lan_ipaddr, schedule2iptables( nvram_get(url_schedule) ), lan_ipaddr,WEBFILTER_PROXY_PORT);
#endif
		}

		/* Allow trust ip visit block website */
        for(i=0; i<MAX_TRUST_IP_NUMBER; i++)
		{
			snprintf(fw_rule, sizeof(fw_rule), "url_domain_filter_trust_ip_%02d", i);
			getValue = nvram_get(fw_rule);
			if ( getValue == NULL ||  strlen(getValue) == 0 )
				continue;
			else
				strncpy(fw_value, getValue, sizeof(fw_value));

			if( getStrtok(fw_value, "/", "%s %s", &enable, &trust_ip) !=2 )
				continue;

			if((enable != NULL) && (trust_ip != NULL) && (strcmp(enable, "1") == 0)){
				save2file(FW_NAT,"-I PREROUTING -i br0 -s %s -p tcp --dport 80 -j ACCEPT", trust_ip);

			}
		}
	}
#endif//CONFIG_USER_CROWDCONTROL
	DEBUG_MSG("\tURL /domain filter:!using_access_control finish\n");
#endif //#ifndef ACCESS_CONTROL

#ifdef ACCESS_CONTROL
		/*	Access Control
		 *	Nvram name: access_control_%02d
		 *	Iptables command:	iptables -A PREROUTING -s 192.168.0.55 -m urlfilter --url www.google.com.tw -j ACCEPT
		 *						iptables -t nat -A PREROUTING -p UDP -m mac --mac-source 00:03:11:12:34:56 -m iprange --dst-range  1.1.1.1-2.2.2.2 --dport 10:20 -j DROP
		 */
		DEBUG_MSG("\t********** Access Control **********\n");
#ifdef CONFIG_USER_CROWDCONTROL
		/*
	 	Date: 2009-1-05
	 	Name: Ken_Chiang
	 	Reason: modify for blocked url to redirect the reject page by crowdcontrol(http proxy).
		*/
		int log_flag=0,file_flag=0;
		DEBUG_MSG("USED PROXY\n");
#ifdef MIII_BAR
		if( nvram_match("miiicasa_enabled", "1") == 0 ){
			_system("crowdcontrol -a %s -s 255.255.255.0 -p 3128 &",lan_ipaddr);
		}
#endif
		PROXY_DEBUG_MSG("USED CROWDCONTROL PROXY\n");
#endif
		if(!nvram_match("access_control_filter_type","enable"))
		{
			DEBUG_MSG("access_control_filter_type enable\n");
			/* process individual src machine */
			for(i=0; i<MAX_ACCESS_CONTROL_NUMBER; i++)
			{
				snprintf(fw_rule, sizeof(fw_rule), "access_control_%02d",i) ;
				getValue = nvram_get(fw_rule);
				DEBUG_MSG("fw_rule=%s,getValue=%s\n",fw_rule,getValue);
				if ( getValue == NULL ||  strlen(getValue) == 0 )
					continue;
				else
					strncpy(fw_value, getValue, sizeof(fw_value));

				if( getStrtok(fw_value, "/", "%s %s %s %s %s %s", &enable, &name, &src_machine, &filter_action, &log, &schedule_name) != 6 )
					continue;

				DEBUG_MSG("fw_value=%s,enable=%s,name=%s,src_machine=%s,filter_action=%s,log=%s,schedule_name=%s\n",fw_value,enable,name,src_machine,filter_action,log,schedule_name);
				if(strcmp(schedule_name, "Never") == 0)
			   	        continue;

				if(strcmp(enable, "1") == 0)
				{
					memset(ip_range,0,8*sizeof(struct IpRange));

					/* for each src machine */
					getStrtok(src_machine, ",", "%s %s %s %s %s %s %s %s", &src[0], &src[1], &src[2], &src[3], &src[4], &src[5], &src[6], &src[7]);
					DEBUG_MSG("enable : src_machine=%s,src[0]=%s,src[1]=%s,src[2]=%s,src[3]=%s,src[4]=%s,src[5]=%s,src[6]=%s,src[7]=%s\n"
								,src_machine,src[0],src[1],src[2],src[3],src[4],src[5],src[6],src[7]);

					src_count=0;	//NickChou reset src_count

					for(j=0; j<MAX_SRC_MACHINE_NUMBER; j++)
					{
						if(src[j])
						{
							if(strchr(src[j],'.'))				/* src machine = ip*/
							{
								sprintf(ip_range[src_count].addr,"-s %s", src[j]);
								src_count++;
							}
							else if(strchr(src[j],':'))			/* src machine = mac*/
							{
								sprintf(ip_range[src_count].addr,"-m mac --mac-source %s", src[j]);
								src_count++;
							}
							else if(!strcmp(src[j],"other"))	/* src machine = other */
							{
								/* Other machines rule need to be placed in the iptables last,
								 * so now I just record it and will implement it later.
								 */
								 strcpy(other_machine[other_count].filter, filter_action);
								 strcpy(other_machine[other_count].log, log);
								 strcpy(other_machine[other_count].schedule, schedule_name);
								 other_machine[other_count].index = i;
								 DEBUG_MSG("src[%d]=other:other_machine[%d].filter=%s,log=%s,schedule=%s\n"
								 			,j,other_count,other_machine[other_count].filter,other_machine[other_count].log,other_machine[other_count].schedule);
								 other_count++;
								 continue;
							}
						}
					}

					DEBUG_MSG("access control: ACCESS_CONTROL_RULE %d: src_count=%d,other_count=%d\n"
						,i, src_count, other_count);

					/* filter_action */
					if(!strcmp(filter_action,"log_only"))
					{
						DEBUG_MSG("access control: log_only\n");
						FOR_EACH_WAN_FACE
							log_only(src_count, ip_range, schedule_name, bri_eth, wan_if[k], 0);
					}
					else if(!strcmp(filter_action,"block_all"))
					{
						DEBUG_MSG("access control: block_all\n");
						FOR_EACH_WAN_FACE
							block_all(src_count, ip_range, schedule_name, bri_eth, wan_if[k], 0);
					}
					else if(!strcmp(filter_action,"url_filter"))
					{
#ifndef CONFIG_USER_CROWDCONTROL //when ACCESS_CONTROL is defined
						DEBUG_MSG("access control: url_filter: UnDefine CONFIG_USER_CROWDCONTROL\n");
						if(strcmp(log,"yes") == 0){
							FOR_EACH_WAN_FACE
								log_only(src_count, ip_range, schedule_name, bri_eth, wan_if[k], 0);
						}
						FOR_EACH_WAN_FACE
							url_filter(src_count, ip_range, schedule_name, bri_eth, wan_if[k], log, 0);
#endif	//#ifndef CONFIG_USER_CROWDCONTROL

						enable_url_filter = 1;

#ifdef CONFIG_USER_CROWDCONTROL //when ACCESS_CONTROL is defined
/*
 	Date: 2009-1-05
 	Name: Ken_Chiang
 	Reason: modify for blocked url to redirect the reject page by crowdcontrol(http proxy).
 	Notice :
*/
	{  //To creat filter log ip file and PREROUTING rule for crowdcontrol.
						int j;
						if( strcmp(log,"yes") == 0 && !log_flag){
							init_file(FILTER_LOG_IP_FILE);
							PROXY_DEBUG_MSG("log file %s init\n",FILTER_LOG_IP_FILE);
							log_flag++;
						}
						if( (nvram_match("url_domain_filter_type", "list_allow") ==0) || (nvram_match("url_domain_filter_type", "list_deny") ==0) )
        				{
							PROXY_DEBUG_MSG("PROXY url_domain_filter_type==list_allow||list_deny\n");
							for(j=0; j<src_count; j++)
							{
								if((ip_range+j) && (strlen(ip_range+j) >= 4)){//"-m mac --mac..." or "-s xxx.xxx.xxx.xxx"
									PROXY_DEBUG_MSG("PROXY ip_range+%d=%s\n",j,ip_range+j);

#ifdef IP8000
									save2file(FW_NAT,"-I PREROUTING -i %s %s ! -d %s -p tcp --dport 80 %s -j DNAT --to-destination %s:%d\n"
											   , bri_eth, ip_range+j,lan_ipaddr, schedule2iptables( schedule_name ), lan_ipaddr,WEBFILTER_PROXY_PORT);
#else
									save2file(FW_NAT,"-I PREROUTING -i %s %s -d ! %s -p tcp --dport 80 %s -j DNAT --to-destination %s:%d\n"
											   , bri_eth, ip_range+j,lan_ipaddr, schedule2iptables( schedule_name ), lan_ipaddr,WEBFILTER_PROXY_PORT);
#endif

									if(strcmp(log,"yes") == 0){
										char *iprange;
										PROXY_DEBUG_MSG("log ip_range+%d=%s\n",j,ip_range+j);
										if(strstr(ip_range+j,"-mac")){//mac addr -m mac --mac-source
											iprange=ip_range+j;
											iprange=iprange+20;
											PROXY_DEBUG_MSG("mac iprange=%s\n",iprange);
											save2file(FILTER_LOG_IP_FILE,"%s\n",iprange);
										}
										else{//ip addr
											iprange=ip_range+j;
											PROXY_DEBUG_MSG("ip addr=%s\n",iprange+3);
											save2file(FILTER_LOG_IP_FILE,"%s\n",iprange+3);
										}
									}
								}
							}
						}
	}
#endif//CONFIG_USER_CROWDCONTROL
					}
					else if(!strcmp(filter_action,"firewall_rule"))
					{
						DEBUG_MSG("access control: firewall_rule\n");
						firewall_rule(i, src_count, ip_range, schedule_name);
					}
					else if(!strcmp(filter_action,"url_firewall"))
					{
						DEBUG_MSG("access control: url_firewall\n");
						if(strcmp(log,"yes") == 0){
							FOR_EACH_WAN_FACE
								log_only(src_count, ip_range, schedule_name, bri_eth, wan_if[k], 0);
						}

						FOR_EACH_WAN_FACE
 							url_filter(src_count, ip_range, schedule_name, bri_eth, wan_if[k], log, 0);

#ifdef CONFIG_USER_CROWDCONTROL
	{ //To creat filter log ip file and PREROUTING rule for crowdcontrol.
						int j;
						if( strcmp(log,"yes") == 0 && !log_flag){
							PROXY_DEBUG_MSG("log file %s init\n",FILTER_LOG_IP_FILE);
							init_file(FILTER_LOG_IP_FILE);
							log_flag++;
						}
						if( (nvram_match("url_domain_filter_type", "list_allow") ==0) || (nvram_match("url_domain_filter_type", "list_deny") ==0) )
        				{
							PROXY_DEBUG_MSG("PROXY url_domain_filter_type==list_allow||list_deny\n");
							for(j=0; j<src_count; j++)
							{
								if((ip_range+j) && (strlen(ip_range+j) >= 4)){//"-m mac --mac..." or "-s xxx.xxx.xxx.xxx"
									PROXY_DEBUG_MSG("PROXY ip_range+%d=%s\n",j,ip_range+j);

#ifdef IP8000
									save2file(FW_NAT,"-I PREROUTING -i %s %s ! -d %s -p tcp --dport 80 %s -j DNAT --to-destination %s:%d\n"
											   , bri_eth, ip_range+j,lan_ipaddr, schedule2iptables( schedule_name ), lan_ipaddr,WEBFILTER_PROXY_PORT);
#else
									save2file(FW_NAT,"-I PREROUTING -i %s %s -d ! %s -p tcp --dport 80 %s -j DNAT --to-destination %s:%d\n"
											   , bri_eth, ip_range+j,lan_ipaddr, schedule2iptables( schedule_name ), lan_ipaddr,WEBFILTER_PROXY_PORT);
#endif

									if(strcmp(log,"yes") == 0){
										char *iprange;
										PROXY_DEBUG_MSG("log ip_range+%d=%s\n",j,ip_range+j);
										if(strstr(ip_range+j,"-mac")){//mac addr -m mac --mac-source
											iprange=ip_range+j;
											iprange=iprange+20;
											PROXY_DEBUG_MSG("mac iprange=%s\n",iprange);
											save2file(FILTER_LOG_IP_FILE,"%s\n",iprange);
										}
										else{//ip addr
											iprange=ip_range+j;
											PROXY_DEBUG_MSG("ip addr=%s\n",iprange+3);
											save2file(FILTER_LOG_IP_FILE,"%s\n",iprange+3);
										}
									}
								}
							}
						}
	}
#endif//CONFIG_USER_CROWDCONTROL
						firewall_rule(i, src_count, ip_range, schedule_name);
						enable_url_filter = 1;
					}
					else
						continue;
				}
			}//for individual src machine

			/* other src machine */
			memset(ip_range, 0, 8*sizeof(struct IpRange));

			for(i=0; i<other_count; i++)
			{
				/* filter_action */
				if(!strcmp(other_machine[i].filter,"log_only"))
				{
					DEBUG_MSG("access control: other machine: log_only : src_count=%d,schedule_name=%s\n"
						,src_count, schedule_name);

					FOR_EACH_WAN_FACE
						log_only(src_count, ip_range, schedule_name, bri_eth, wan_if[k], 1);
				}
				else if(!strcmp(other_machine[i].filter,"block_all"))
				{
					DEBUG_MSG("access control: other machine: block_all : other_machine[i].schedule=%s\n",
						other_machine[i].schedule);

					FOR_EACH_WAN_FACE
						block_all(1, ip_range, other_machine[i].schedule, bri_eth, wan_if[k], 1);
				}
				else if(!strcmp(other_machine[i].filter,"url_filter"))
				{
					DEBUG_MSG("access control: other machine: url_filter : other_machine[%d].log ? : %s\n", i, other_machine[i].log);

					FOR_EACH_WAN_FACE
						url_filter(1, ip_range, other_machine[i].schedule, bri_eth, wan_if[k], other_machine[i].log, 1);

#ifdef CONFIG_USER_CROWDCONTROL
	{ //To creat filter log ip file and PREROUTING rule for crowdcontrol.
						int j;
						if( strcmp(log,"yes") == 0 && !log_flag){
							PROXY_DEBUG_MSG("log file %s init\n",FILTER_LOG_IP_FILE);
							init_file(FILTER_LOG_IP_FILE);
							log_flag++;
						}
						if( (nvram_match("url_domain_filter_type", "list_allow") ==0) || (nvram_match("url_domain_filter_type", "list_deny") ==0) )
        				{
							PROXY_DEBUG_MSG("PROXY url_domain_filter_type==list_allow||list_deny\n");
							for(j=0; j<1; j++)
							{
								if((ip_range+j) && (strlen(ip_range+j) >= 4)){//"-m mac --mac..." or "-s xxx.xxx.xxx.xxx"
									PROXY_DEBUG_MSG("PROXY ip_range+%d=%s\n",j,ip_range+j);

#ifdef IP8000
									save2file(FW_NAT,"-I PREROUTING -i %s %s ! -d %s -p tcp --dport 80 %s -j DNAT --to-destination %s:%d\n"
											   , bri_eth, ip_range+j,lan_ipaddr, schedule2iptables( other_machine[i].schedule ), lan_ipaddr,WEBFILTER_PROXY_PORT);
#else
									save2file(FW_NAT,"-I PREROUTING -i %s %s -d ! %s -p tcp --dport 80 %s -j DNAT --to-destination %s:%d\n"
											   , bri_eth, ip_range+j,lan_ipaddr, schedule2iptables( other_machine[i].schedule ), lan_ipaddr,WEBFILTER_PROXY_PORT);
#endif

									if(strcmp(log,"yes") == 0){
										char *iprange;
										PROXY_DEBUG_MSG("log ip_range+%d=%s\n",j,ip_range+j);
										if(strstr(ip_range+j,"-mac")){//mac addr -m mac --mac-source
											iprange=ip_range+j;
											iprange=iprange+20;
											PROXY_DEBUG_MSG("mac iprange=%s\n",iprange);
											save2file(FILTER_LOG_IP_FILE,"%s\n",iprange);
										}
										else{//ip addr
											iprange=ip_range+j;
											PROXY_DEBUG_MSG("ip addr=%s\n",iprange+3);
											save2file(FILTER_LOG_IP_FILE,"%s\n",iprange+3);
										}
									}
								}
							}
						}
	}
#endif//CONFIG_USER_CROWDCONTROL
					enable_url_filter = 1;
				}
				else if(!strcmp(other_machine[i].filter,"firewall_rule"))
				{
					DEBUG_MSG("other firewall_rule : other_machine[i].schedule=%s\n",other_machine[i].schedule);
					firewall_rule(other_machine[i].index, 1, ip_range, other_machine[i].schedule);
				}
				else if(!strcmp(other_machine[i].filter,"url_firewall"))
				{
					DEBUG_MSG("other url_firewall : other_machine[i].schedule=%s\n",other_machine[i].schedule);
					FOR_EACH_WAN_FACE
						url_filter(1, ip_range, schedule_name, bri_eth, wan_if[k], log, 1);
/*
 	Date: 2009-1-05
 	Name: Ken_Chiang
 	Reason: modify for blocked url to redirect the reject page by crowdcontrol(http proxy).
 	Notice :
*/
#ifdef CONFIG_USER_CROWDCONTROL
	{ //To creat filter log ip file and PREROUTING rule for crowdcontrol.
						int j;
						if( strcmp(log,"yes") == 0 && !log_flag){
							PROXY_DEBUG_MSG("log file %s init\n",FILTER_LOG_IP_FILE);
							init_file(FILTER_LOG_IP_FILE);
							log_flag++;
						}
						if( (nvram_match("url_domain_filter_type", "list_allow") ==0) || (nvram_match("url_domain_filter_type", "list_deny") ==0) )
        				{
							PROXY_DEBUG_MSG("PROXY url_domain_filter_type==list_allow||list_deny\n");
							for(j=0; j<1; j++)
							{
								if((ip_range+j) && (strlen(ip_range+j) >= 4)){//"-m mac --mac..." or "-s xxx.xxx.xxx.xxx"
									PROXY_DEBUG_MSG("PROXY ip_range+%d=%s\n",j,ip_range+j);

#ifdef IP8000
									save2file(FW_NAT,"-I PREROUTING -i %s %s ! -d %s -p tcp --dport 80 %s -j DNAT --to-destination %s:%d\n"
											   , bri_eth, ip_range+j,lan_ipaddr, schedule2iptables( schedule_name ), lan_ipaddr,WEBFILTER_PROXY_PORT);
#else
									save2file(FW_NAT,"-I PREROUTING -i %s %s -d ! %s -p tcp --dport 80 %s -j DNAT --to-destination %s:%d\n"
											   , bri_eth, ip_range+j,lan_ipaddr, schedule2iptables( schedule_name ), lan_ipaddr,WEBFILTER_PROXY_PORT);
#endif

									if(strcmp(log,"yes") == 0){
										char *iprange;
										PROXY_DEBUG_MSG("log ip_range+%d=%s\n",j,ip_range+j);
										if(strstr(ip_range+j,"-mac")){//mac addr -m mac --mac-source
											iprange=ip_range+j;
											iprange=iprange+20;
											PROXY_DEBUG_MSG("mac iprange=%s\n",iprange);
											save2file(FILTER_LOG_IP_FILE,"%s\n",iprange);
										}
										else{//ip addr
											iprange=ip_range+j;
											PROXY_DEBUG_MSG("ip addr=%s\n",iprange+3);
											save2file(FILTER_LOG_IP_FILE,"%s\n",iprange+3);
										}
									}
								}
							}
						}
	}
#endif//CONFIG_USER_CROWDCONTROL
					firewall_rule(other_machine[i].index, 1, ip_range, other_machine[i].schedule);
					enable_url_filter = 1;
				}
				else
					continue;
			}//for other src machine

#ifdef CONFIG_USER_CROWDCONTROL
	{  //To creat permit domains file or block domains file for crowdcontrol
	   // and start crowdcontrol demanes
			if( enable_url_filter == 1 && ((nvram_match("url_domain_filter_type", "list_allow") ==0) || (nvram_match("url_domain_filter_type", "list_deny") ==0)))
    		{
				PROXY_DEBUG_MSG("PROXY url_domain_filter_type==list_allow||list_deny\n");
				for(i=0; i<MAX_URL_FILTER_NUMBER; i++)
    		    {
					snprintf(fw_rule, sizeof(fw_rule), "url_domain_filter_%02d", i);
					getValue = nvram_get(fw_rule);
					PROXY_DEBUG_MSG("PROXY fw_rule=%s,getValue=%s\n",fw_rule,getValue);
					if ( getValue == NULL ||  strlen(getValue) == 0 )
						continue;
					else{
						/* if url_rule = http://tw.yahoo.com
						   iptables command will ignore the key word "http" but keep the "://"
						   finally, it becomes url_rule://tw.yahoo.com, (will not work)
						*/
						if(strstr(getValue,"http://") != NULL)
							strcpy(fw_value, getValue + strlen("http://"));
						else
							strcpy(fw_value, getValue);
					}
					for( p = 0 ; p < strlen(fw_value); p++ ){
						fw_value[p] = tolower( fw_value[p] );
					}
					if ( nvram_match("url_domain_filter_type", "list_allow") ==0 ) {

						PROXY_DEBUG_MSG("PROXY %s added rule=%s\n",PERMIT_DOMAINS_FILE,fw_value);
						if(!file_flag){
							init_file(PERMIT_DOMAINS_FILE);
							file_flag++;
						}
						save2file(PERMIT_DOMAINS_FILE, "%s\n", fw_value);
					}
					else{
						PROXY_DEBUG_MSG("PROXY %s added rule=%s\n",BLOCK_DOMAINS_FILE,fw_value);
						if(!file_flag){
							init_file(BLOCK_DOMAINS_FILE);
							file_flag++;
						}
						save2file(BLOCK_DOMAINS_FILE, "%s\n", fw_value);
					}
				}//end for

				if ( nvram_match("url_domain_filter_type", "list_allow") ==0 ) {
					if(log_flag){
						PROXY_DEBUG_MSG("ACCESS_CONTROL PROXY list_allow log\n");
						_system("crowdcontrol -a %s -s %s -p %d --%s %s -l %s &",
							lan_ipaddr, lan_netmask, WEBFILTER_PROXY_PORT, "permit-domains", PERMIT_DOMAINS_FILE,FILTER_LOG_IP_FILE);
					}
					else{
						PROXY_DEBUG_MSG("ACCESS_CONTROL PROXY list_allow\n");
						_system("crowdcontrol -a %s -s %s -p %d --%s %s &",
							lan_ipaddr, lan_netmask, WEBFILTER_PROXY_PORT, "permit-domains", PERMIT_DOMAINS_FILE);
					}
			    }
			    else {
					if(log_flag){
						PROXY_DEBUG_MSG("ACCESS_CONTROL PROXY list_deny log\n");
						_system("crowdcontrol -a %s -s %s -p %d --%s %s -l %s &",
							lan_ipaddr, lan_netmask, WEBFILTER_PROXY_PORT, "deny-domains", BLOCK_DOMAINS_FILE,FILTER_LOG_IP_FILE);
					}
					else{
						PROXY_DEBUG_MSG("ACCESS_CONTROL PROXY list_deny\n");
						_system("crowdcontrol -a %s -s %s -p %d --%s %s &",
							lan_ipaddr, lan_netmask, WEBFILTER_PROXY_PORT, "deny-domains", BLOCK_DOMAINS_FILE);
					}
			    }
			}
	}
#endif//CONFIG_USER_CROWDCONTROL
	}
	DEBUG_MSG("\t********** Access Control finish**********\n");
#endif//#ifdef ACCESS_CONTROL

#if !(ONLY_FILTER_WLAN_MAC)
	set_mac_filter_rule();
#endif

		/* Deny Ping Request from WAN */
		if( nvram_match("wan_port_ping_response_enable", "0") == 0 ) {
			DEBUG_MSG("\tDeny Ping Request from WAN\n");
			FOR_EACH_WAN_FACE
				save2file(FW_FILTER,"-A INPUT -i %s -p icmp --icmp-type echo-request -j LOG_AND_DROP\n", wan_if[k] );
		}else{
			DEBUG_MSG("\tNo Deny Ping Request from WAN\n");
#ifdef INBOUND_FILTER
			/*
			 * inbound_ip_count >0 implies to apply inbound filter.
			 * If allowing multiple ranges of IP to ping wan port, we add rules to ACCEPT packets from these ip for each ip range.
			 * And add the last rule to DROP other icmp packets.
			 *
			 * ex.
			 * -A INPUT -i eth0 -m iprange --src-range 1.1.1.1-2.2.2.2 -p icmp --icmp-type echo-request -j ACCEPT
			 * -A INPUT -i eth0 -m iprange --src-range 3.3.3.3-4.4.4.4 -p icmp --icmp-type echo-request -j ACCEPT
			 * -A INPUT -i eth0 -p icmp --icmp-type echo-request -j DROP
			 *
			 * Else Deny multiple ranges of IP to ping wan port, we add rules to DROP packets from these ip for each ip range.
			 * ex.
			 * -A INPUT -i eth0 -m iprange --src-range 1.1.1.1-2.2.2.2 -p icmp --icmp-type echo-request -j DROP
			 * -A INPUT -i eth0 -m iprange --src-range 3.3.3.3-4.4.4.4 -p icmp --icmp-type echo-request -j DROP
			 *
			 */
			DEBUG_MSG("\t********** INBOUND_FILTER**********\n");
			inbound_name = nvram_safe_get("wan_port_ping_response_inbound_filter");
			/* Allow All implies that we don't need to add rulues to DROP icmp packets */
			if(inbound_name && strlen(inbound_name) && strcmp(inbound_name, "Allow_All")){
				/* Deny All implies not to apply inbound filter and DROP all icmp packets */
				if(!strcmp(inbound_name, "Deny_All")){
				FOR_EACH_WAN_FACE
					save2file(FW_FILTER,"-A INPUT -i %s -p icmp --icmp-type echo-request -j LOG_AND_DROP\n", wan_if[k] );
				}else{
					memset(action,0,sizeof(action));
					memset(ip_range,0,8*sizeof(struct IpRange));
					inbound_ip_count = inbound2iptables(inbound_name, action, ip_range);
					if(inbound_ip_count){
						if( !strcmp(action, "allow") ){
							for(j=0;j<inbound_ip_count;j++){
								FOR_EACH_WAN_FACE
									save2file(FW_FILTER,"-A INPUT -i %s -m iprange --src-range %s -p icmp --icmp-type echo-request -j ACCEPT\n", wan_if[k], ip_range[j].addr );
							}
							FOR_EACH_WAN_FACE
								save2file(FW_FILTER,"-A INPUT -i %s -p icmp --icmp-type echo-request -j LOG_AND_DROP\n", wan_if[k] );
						}else{
							for(j=0;j<inbound_ip_count;j++){
								FOR_EACH_WAN_FACE
									save2file(FW_FILTER,"-A INPUT -i %s -m iprange --src-range %s -p icmp --icmp-type echo-request -j LOG_AND_DROP\n", wan_if[k], ip_range[j].addr );
							}
						}
					}
				}
			}
#endif

#if 0
			/* If wan_port_ping_response_start_ip = *, it means allow all */
			if( strcmp(nvram_safe_get("wan_port_ping_response_start_ip"), "*") == 0 ) {
				{
					save2file(FW_FILTER,"-A INPUT -i %s -p icmp --icmp-type echo-request -j ACCEPT\n", wan_eth );
				}
			}else{
	   			char* wan_port_ping_response_end_ip = nvram_safe_get("wan_port_ping_response_end_ip");
	   			if(strcmp(wan_port_ping_response_end_ip,"")==0)
	   				wan_port_ping_response_end_ip=nvram_safe_get("wan_port_ping_response_start_ip");

				{
	                save2file(FW_FILTER,"-A INPUT -m iprange --src-range %s-%s -p icmp --icmp-type echo-request -j ACCEPT\n",nvram_safe_get("wan_port_ping_response_start_ip"),wan_port_ping_response_end_ip);
	                save2file(FW_FILTER,"-A INPUT -i %s -p icmp --icmp-type echo-request -j DROP\n", wan_eth );
				}
	    	}
#endif
		}

		/*	Anti-Spoof checking
		*	Nvram name:  anti_spoof_check	(0:disable / 1:enable)
		*	Iptables command:	iptables -t nat -A PREROUTING -i eth0 -s 192.168.0.0/16 -j DROP
		*/
		if( nvram_match("anti_spoof_check", "1") == 0 )
		{
#if 0
			char *p_spoof_ip;
			char spoof_ip[16]={};
			DEBUG_MSG("\tAnti-Spoof checking\n");

			p_spoof_ip = rindex(lan_ipaddr, '.');
			strncpy(spoof_ip, lan_ipaddr, p_spoof_ip - lan_ipaddr);

			/*private*/
			//save2file(FW_NAT, "-A PREROUTING -i %s -s 10.0.0.0/8 -j DROP\n", wan_eth);
			//save2file(FW_NAT, "-A PREROUTING -i %s -s 172.16.0.0/12 -j DROP\n", wan_eth);
			//save2file(FW_NAT, "-A PREROUTING -i %s -s 192.168.0.0/16 -j DROP\n", wan_eth);
			save2file(FW_NAT, "-A PREROUTING -i %s -s %s.0/24 -j DROP\n", wan_eth, spoof_ip);

			/*Loopback*/
			save2file(FW_NAT, "-A PREROUTING -i %s -s 127.0.0.0/8 -j DROP\n", wan_eth);

			/*Class D => Multicasting*/
			//save2file(FW_NAT,"-A PREROUTING -i %s -s 224.0.0.0/4 -j DROP\n", wan_eth);

			/*Class E*/
			save2file(FW_NAT, "-A PREROUTING -i %s -s 240.0.0.0/5 -j DROP\n", wan_eth);
#else
			DEBUG_MSG("\tAnti-Spoof checking\n");
			FOR_EACH_WAN_FACE
			{
				/* IP Spoofing attack for lan*/
                		save2file(FW_FILTER,"-I FORWARD -i %s -s %s/%s -j DROP\n", wan_if[k], lan_ipaddr, lan_netmask);
				save2file(FW_FILTER,"-I FORWARD -i %s -s %s/%s -j LOG  --log-prefix \"[IP_spoof attack]:\"\n", wan_if[k], lan_ipaddr, lan_netmask);

				/* IP Spoofing attack for router*/
				save2file(FW_FILTER,"-A INPUT -i %s -s %s/%s -j LOG  --log-prefix \"[IP_spoof attack]:\"\n", wan_if[k], lan_ipaddr, lan_netmask);
				save2file(FW_FILTER,"-A INPUT -i %s -s %s/%s -j DROP\n", wan_if[k], lan_ipaddr, lan_netmask);
            }
#endif
		}


// try 2010.01.21
#ifdef CONFIG_USER_OPENAGENT

// need to check enable cpe option ^^ wait next

// here need to check port number
	int port_number;
	char port_str[32];
	
	port_number = 7547;

	char *cpe_url;	
	
    cpe_url = nvram_safe_get("tr069_cpe_url");
    
    if (strlen(cpe_url)) //parse port and URI of CPE.
    {
        char *p = NULL;

        p = strstr(cpe_url, ":");
        if( p) {
       		port_number = atol(p+1);
       		if( port_number == 0) 
       			port_number = 7547;
       	}
         
         
//                        if (p)
//                        {
//                                if (cpe_url[0] == 0x3A) // char : as having specify port
//                                        strncpy(port, cpe_url + 1, p - (cpe_url + 1));
//                                else
//                                        strncpy(port, cpe_url, p - cpe_url); // TODO: only having uri(no specify port)
//                                strncpy(uri, p+1, strlen(p));
//                                _system("tr069 mngsrv reqport %s", port);
//                                _system("tr069 mngsrv requri %s", uri);
//                        }
   }
	
	
	sprintf(port_str,"%d",port_number);

		FOR_EACH_WAN_IP
			save2file(FW_NAT, "-A PREROUTING -p TCP -d %s --dport %s %s -j DNAT --to %s:%d\n",\
							wan_ipaddr[k], port_str, "", wan_ipaddr[0], port_number );
					save2file(FW_FILTER,"-A INPUT  %s -p TCP --dport %d  -j ACCEPT\n",\
						"", port_number);

		FOR_EACH_WAN_IP
			save2file(FW_NAT, "-A PREROUTING -p UDP -d %s --dport %s %s -j DNAT --to %s:%d\n",\
							wan_ipaddr[k], port_str, "", wan_ipaddr[0], port_number );
					save2file(FW_FILTER,"-A INPUT  %s -p UDP --dport %d  -j ACCEPT\n",\
						"", port_number);
/*
			save2file(FW_NAT, "-A PREROUTING -p TCP -d %s --dport %s %s -j DNAT --to %s:%d\n",\
							wan_ipaddr[k], "7547", "", wan_ipaddr[0], 7547 );
					save2file(FW_FILTER,"-A INPUT  %s -p TCP --dport %d  -j ACCEPT\n",\
						"", 7547);
*/
#endif


/*  Date: 2009-3-9
*   Name: Ken Chiang
*   Reason: Move from SPI part to Deny invalid TCP ACK packet
*   		To fix Anti-Spoof don't show message in syslog.
*   Notice :
*/
		save2file(FW_FILTER,"-A INPUT -p tcp -m state --state NEW -m tcp --tcp-flags ALL ACK -j LOG_AND_DROP\n");
#ifdef DIR652V
		
		/* DIR652V use 'remote_ssl' key to make decision
		 * which remote https login enable or disable
		 *
		 * 2010/8/17 Fredhung */
		if (strcmp(nvram_safe_get("remote_ssl"), "1") == 0 || is_sslvpn_enable()) {
			char *sslport = nvram_safe_get("sslport");

			save2file(FW_NAT, "-A PREROUTING -p tcp --dport %s -j ACCEPT\n", sslport);

			if (strcmp(nvram_safe_get("remote_ssl"), "1") == 0)
				do_iprange_firewall(nvram_safe_get("ssl_remote_ipaddr"), sslport);
			if (is_sslvpn_enable()) {
				do_iprange_firewall(nvram_safe_get("sslvpn_remote_ipaddr"), sslport);
				save2file(FW_FILTER, "-A FORWARD -i tap+ -p all -m state --state NEW -j ACCEPT\n");
			}
		}

		/* DIR652V support dhcp relay */
		if (nvram_match("dhcp_relay", "1") == 0) {
			char cmd[256], *wandev = "eth0.1";

			if (nvram_match("wan_proto", "pppoe") == 0 ||
			    nvram_match("wan_proto", "pptp") == 0||
			    nvram_match("wan_proto", "l2tp") == 0)
				wandev = "ppp0";

			save2file(FW_NAT, "-I PREROUTING -i %s -s %s -d %s -p udp --dport 67:68 -j DNAT --to-destination %s\n",
				  wandev,
				  nvram_safe_get("dhcp_relay_ip"),
				  nvram_safe_get("lan_ipaddr"),
				  get_ipaddr(wandev));
		}
#if 0
		if (strcmp(nvram_safe_get("remote_ssl"), "1") == 0 || is_sslvpn_enable()) {
			char *sslport = nvram_safe_get("sslport");
			char *s, *p, fw_rule[256], iprange_fmt[1024];

			save2file(FW_NAT, "-A PREROUTING -i %s -d %s -p tcp --dport %s -j ACCEPT\n",
				nvram_safe_get("lan_bridge"), nvram_safe_get("lan_ipaddr"), sslport);
			save2file(FW_NAT, "-A PREROUTING -i %s -p tcp --dport %s -j ACCEPT\n",
				nvram_safe_get("wan_eth"), sslport);

			p = iprange_fmt;
			strcpy(iprange_fmt, nvram_safe_get("ssl_remote_ipaddr"));

			while ((s = strsep(&p, ",;")) != NULL) {
				sprintf(fw_rule, "-A INPUT -i %s "
					"-p tcp --dport %s ", nvram_safe_get("wan_eth"), sslport);

				if (strcmp(iprange_fmt, "*") == 0)
					strcat(fw_rule, "-j ACCEPT\n");
				else if (strchr(s, '-') == NULL)
					sprintf(fw_rule, "%s-s %s -j ACCEPT\n", fw_rule, s);
				else
					sprintf(fw_rule, "%s-m iprange --src-range %s -j ACCEPT\n",
						fw_rule, s);
				save2file(FW_FILTER, fw_rule);
			}
		}
#endif
#endif	/* DIR652V */
		/* Remote HTTP management */
		if ( nvram_match("remote_http_management_enable", "1") == 0 )
		{
/*	Date:	2009-04-09
 *	Name:	Yufa Huang
 *	Reason: add HTTPS function.
 */
			int http_port = HTTP_PORT;
			DEBUG_MSG("\tRemote HTTP Management\n");
#if defined(HAVE_HTTPS)
			char *https_enable, *remote_https_enable, *remote_https_port, *remote_http_port;
			DEBUG_MSG("HAVE_HTTPS\n");
			getValue = nvram_get("https_config");
			strncpy(fw_value, getValue, sizeof(fw_value));

			if (getStrtok(fw_value, "/", "%s %s %s %s", &https_enable, &remote_https_enable, &remote_https_port, &remote_http_port) == 4)
			{
				if (strcmp(remote_https_enable, "1") == 0) {
					http_port = HTTPS_PORT;
				}
			}
#endif

#ifdef INBOUND_FILTER
			inbound_name = nvram_safe_get("remote_http_management_inbound_filter");
			/* Deny All implies that the rule is invalid */
			if(inbound_name && strlen(inbound_name) && strcmp(inbound_name, "Deny_All"))
			{
				/* Allow All implies not to apply inbound filter */
				if(!strcmp(inbound_name, "Allow_All")){
/*	Date:	2009-04-09
 *	Name:	Yufa Huang
 *	Reason: add HTTPS function.
 */
					FOR_EACH_WAN_IP
						save2file(FW_NAT, "-A PREROUTING -p TCP -d %s --dport %s %s -j DNAT --to %s:%d\n",\
							wan_ipaddr[k], nvram_get("remote_http_management_port"), schedule2iptables( nvram_get("remote_http_management_schedule_name") ), wan_ipaddr[0], http_port );
					save2file(FW_FILTER,"-A INPUT  %s -p TCP --dport %d  -j ACCEPT\n",\
						schedule2iptables( nvram_get("remote_http_management_schedule_name") ), http_port);
				}else{
					memset(action,0,sizeof(action));
					memset(ip_range,0,8*sizeof(struct IpRange));
					inbound_ip_count = inbound2iptables(inbound_name, action, ip_range);
					/*
					 * inbound_ip_count >0 implies to apply inbound filter.
					 * If allowing multiple ranges of IP to remote manage, we add rules to DNAT/ACCEPT for each ip range.
					 * ex.
					 * -A PREROUTING -m iprange --src-range 1.1.1.1-2.2.2.2 -p TCP -d 172.1.1.1 --dport 8080  -j DNAT --to 1.2.3.4:80
					 * -A PREROUTING -m iprange --src-range 3.3.3.3-4.4.4.4 -p TCP -d 172.1.1.1 --dport 8080  -j DNAT --to 1.2.3.4:80
					 * -A INPUT 	 -m iprange --src-range 1.1.1.1-2.2.2.2 -p TCP 				--dport 80    -j ACCEPT
					 * -A INPUT 	 -m iprange --src-range 3.3.3.3-4.4.4.4 -p TCP 				--dport 80    -j ACCEPT
					 *
					 * Else Deny multiple ranges of IP to remote manage, we add rules to ACCEPT it in NAT for each ip range first.
					 * Then add the last rule to DNAT. Once packets match a rule, it will leave the iptable chain.
					 * So, Denied-ip won't reach the last rule to DNAT.
					 * Also, we add rules to DROP it in FILTER for each ip range. It prevents hosts in denied-ip that know
					 * http management port to access router's web site successfully.
					 *
					 * ex.
					 * -A PREROUTING -m iprange --src-range 1.1.1.1-2.2.2.2 -p TCP -d 1.1.1.1 --dport 8080 	-j ACCEPT
					 * -A PREROUTING -m iprange --src-range 3.3.3.3-4.4.4.4 -p TCP -d 1.1.1.1 --dport 8080 	-j ACCEPT
					 * -A PREROUTING -p TCP -d 1.1.1.1 --dport 8080 -j DNAT --to 1.2.3.4:80
					 * -A INPUT 	 -m iprange --src-range 1.1.1.1-2.2.2.2 -p TCP 			  --dport 80    -j DROP
					 * -A INPUT 	 -m iprange --src-range 3.3.3.3-4.4.4.4 -p TCP 			  --dport 80    -j DROP
					 */
					if(inbound_ip_count){
						if( !strcmp(action, "allow") ){
							for(j=0;j<inbound_ip_count;j++){
/*	Date:	2009-04-09
 *	Name:	Yufa Huang
 *	Reason: add HTTPS function.
 */
								FOR_EACH_WAN_IP
									save2file(FW_NAT, "-A PREROUTING -m iprange --src-range %s -p TCP -d %s --dport %s %s -j DNAT --to %s:%d\n", \
										ip_range[j].addr, wan_ipaddr[k], nvram_get("remote_http_management_port"), schedule2iptables( nvram_get("remote_http_management_schedule_name") ), wan_ipaddr[0], http_port );
								save2file(FW_FILTER,"-A INPUT -m iprange --src-range %s -p TCP --dport %d %s -j ACCEPT\n",\
									ip_range[j].addr, http_port, schedule2iptables( nvram_get("remote_http_management_schedule_name") ));
							}
						}else{
							for(j=0;j<inbound_ip_count;j++){
/*	Date:	2009-04-09
 *	Name:	Yufa Huang
 *	Reason: add HTTPS function.
 */
								FOR_EACH_WAN_IP
									save2file(FW_NAT, "-A PREROUTING -m iprange --src-range %s -p TCP -d %s --dport %s %s -j ACCEPT\n", \
										ip_range[j].addr, wan_ipaddr[k], nvram_get("remote_http_management_port"), schedule2iptables( nvram_get("remote_http_management_schedule_name") ) );
								save2file(FW_FILTER,"-A INPUT -m iprange --src-range %s -p TCP --dport %d %s -j LOG_AND_DROP\n",\
									ip_range[j].addr, http_port, schedule2iptables( nvram_get("remote_http_management_schedule_name") ));
									/*Albert add 080306: avoid remote mangment conflict with tcp port scan*/
									save2file(FW_FILTER,"-A INPUT  %s -p TCP --dport %d  -j ACCEPT\n",\
										schedule2iptables( nvram_get("remote_http_management_schedule_name") ), http_port);
							}
							FOR_EACH_WAN_IP
								save2file(FW_NAT, "-A PREROUTING -p TCP -d %s --dport %s %s -j DNAT --to %s:%d\n", \
									wan_ipaddr[k], nvram_get("remote_http_management_port"), schedule2iptables( nvram_get("remote_http_management_schedule_name") ), wan_ipaddr[0], http_port );
						}
					}
				}
			}

#else
			remote_ips = nvram_get("remote_http_management_ipaddr_range");
#ifdef REMOTE_IPS_NOT_IN_NVRAM
			if(remote_ips)
			{
				/* If remote_ips = * or empty, it means allow all */
				if( strcmp(remote_ips, "*") == 0 || strlen(remote_ips) == 0)
				{
#endif //#ifdef REMOTE_IPS_NOT_IN_NVRAM
/*	Date:	2009-04-09
 *	Name:	Yufa Huang
 *	Reason: add HTTPS function.
 */
					FOR_EACH_WAN_IP
						save2file(FW_NAT, "-A PREROUTING -p TCP -d %s --dport %s %s -j DNAT --to %s:%d\n",\
							wan_ipaddr[k], nvram_get("remote_http_management_port"), schedule2iptables( nvram_get("remote_http_management_schedule_name") ), wan_ipaddr[k], http_port );
/*	Date:	2009-04-09
 *	Name:	Yufa Huang
 *	Reason: add HTTPS function.
 */
					save2file(FW_FILTER,"-A INPUT  %s -p TCP --dport %d  -j ACCEPT\n",\
						schedule2iptables( nvram_get("remote_http_management_schedule_name") ), http_port);
#ifdef REMOTE_IPS_NOT_IN_NVRAM
				}
				/* else find out ip range */
				else
				{
					foreach(word, remote_ips, next, ",")
					{
						{
/*	Date:	2009-04-09
 *	Name:	Yufa Huang
 *	Reason: add HTTPS function.
 */
							FOR_EACH_WAN_IP
								save2file(FW_NAT, "-A PREROUTING -m iprange --src-range %s -p TCP -d %s --dport %s %s -j DNAT --to %s:%d\n", \
									word, wan_ipaddr[k], nvram_get("remote_http_management_port"), schedule2iptables( nvram_get("remote_http_management_schedule_name") ), wan_ipaddr[k], http_port );
						}
/*	Date:	2009-04-09
 *	Name:	Yufa Huang
 *	Reason: add HTTPS function.
 */
						save2file(FW_FILTER,"-A INPUT -m iprange --src-range %s -p TCP --dport %d %s -j ACCEPT\n",\
							word, http_port, schedule2iptables( nvram_get("remote_http_management_schedule_name") ));
					}
				}
			}
#endif //#ifdef REMOTE_IPS_NOT_IN_NVRAM
#endif //#ifdef INBOUND_FILTER
	                FOR_EACH_WAN_FACE
		                save2file(FW_NAT, "-A PREROUTING -p TCP -i %s -d %s --dport %d  -j LOG_AND_DROP\n", wan_if[k], lan_ipaddr, HTTP_PORT);

		}
#ifdef CONFIG_USER_WEB_FILE_ACCESS 
/* Web File Acess Remote management */ 
		if ( nvram_match("file_access_enable", "1") == 0 ) 
		{ 
				/* Web File Acess Remote HTTP management */ 
				if ( nvram_match("file_access_remote", "1") == 0 ) 
				{ 
						char *http_port = nvram_safe_get("file_access_remote_port"); 
						save2file(FW_FILTER,"-A INPUT -p TCP --dport %s -j ACCEPT\n", http_port); 
				} 
				/* Web File Acess Remote HTTPS management */ 
				if ( nvram_match("file_access_ssl", "1") == 0 ) 
				{ 
						char *sslport = nvram_safe_get("file_access_ssl_port"); 
						save2file(FW_NAT, "-A PREROUTING -p tcp --dport %s -j ACCEPT\n", sslport); 
						save2file(FW_FILTER,"-A INPUT ! -i %s -p tcp --dport %s -j ACCEPT\n",nvram_safe_get("lan_bridge"), sslport); 
				} 
		} 
#endif 


#ifdef MIII_BAR
		if (nvram_match("miiicasa_enabled", "1") == 0) {
			FOR_EACH_WAN_IP
				save2file(FW_NAT, "-A PREROUTING -p TCP -d %s --dport %s %s -j DNAT --to %s:%d\n",
				wan_ipaddr[k], "5555", schedule2iptables("Always"), wan_ipaddr[0], 80);
		}
#endif

		/*	Special application (port trigger)
		 *	Nvram name:	application_%02d
		 *	format: enable/name/lan_ip/trigger_prot/trigger_ports/public_prot/public_ports/schedule_rule_name
		 *	Iptables command:	 iptables -A FORWARD -p TCP --dport 1863 -j ACCEPT
		 *					 iptables -A FORWARD -p TCP -m multiport --ports 20:23,80 -j ACCEPT
		 */
		DEBUG_MSG("\tSpecial application\n");
		//John 2010.04.23 remove unused application_enable = nvram_get("application_enable");
		/*NickChou 070521 for old NVRAM setting don't has application_enable and lan_ip*/
		if(1) //John 2010.04.23 remove unused ( (application_enable == NULL) || !strcmp(application_enable, "1") )
        {
		    int tmp=0;
			for(i=0; i<MAX_APPLICATION_RULE_NUMBER; i++)
            {
				snprintf(fw_rule, sizeof(fw_rule), "application_%02d", i);
				getValue = nvram_get(fw_rule);

				if ( getValue == NULL ||  strlen(getValue) == 0 )
					continue;
				else
					strncpy(fw_value, getValue, sizeof(fw_value));

			    if( getStrtok(fw_value, "/", "%s %s %s %s %s %s %s %s", &enable, &name, &lan_ip, &lan_proto, &trigger_ports, &wan_proto, &pub_ports, &schedule_name) !=8 )
	            {
						strncpy(fw_value, getValue, sizeof(fw_value));
						/* for old NVRAM setting */
						if( getStrtok(fw_value, "/", "%s %s %s %s %s %s %s", &enable, &name, &lan_proto, &trigger_ports, &wan_proto, &pub_ports, &schedule_name) == 7 )
							lan_ip = "Any";
						else
							continue;
				}

				if( !strcmp("1", enable) )
	            {
                     if(strcmp(schedule_name, "Never") == 0)
		   		continue;
					{

                        FOR_EACH_WAN_FACE
						{
							/* trigger rule */
                            save2file(FW_FILTER,"-A FORWARD -i %s -o %s %s%s -p %s %s "
								"-j PORTTRIGGER --mode forward_out --trigger-proto %s --forward-proto %s --trigger-ports %s --forward-ports %s\n",
								bri_eth, wan_if[k], \
		                        strcasecmp(lan_ip, "any") ? "-s " : "", \
	                            strcasecmp(lan_ip, "any") ? lan_ip : "", \
								strcasecmp(lan_proto, "any") ? lan_proto : "ALL", \
								schedule2iptables(schedule_name),\
								strcasecmp(lan_proto, "any") ? lan_proto : "ALL", \
								strcasecmp(wan_proto, "any") ? wan_proto : "ALL", \
								trigger_ports, pub_ports);

							/* forward accept for forward_ports */
							save2file(FW_FILTER,"-A FORWARD -i %s -o %s -p %s %s -j PORTTRIGGER --mode forward_in\n",
								wan_if[k], bri_eth, \
								strcasecmp(wan_proto, "any") ? wan_proto : "ALL", \
								schedule2iptables(schedule_name));
						}
						tmp=1;
					}
				}
			}
			if(tmp==1)
				for(u=0;u<2;u++)
					if(strlen(wan_ipaddr[u]))
						FOR_EACH_WAN_FACE
							save2file(FW_NAT, "-A PREROUTING -i %s -d %s -j PORTTRIGGER --mode dnat\n",wan_if[k],wan_ipaddr[u]);
		}
		DEBUG_MSG("Special application finish\n");
		/*
			For XBOX NAT test (Gaming Mode)
			NAT Type => 0=PORT_ADDRESS_RESTRICT(dependent), Gaming Mode Disable
			            1=ENDPOINT_INDEPEND (Full Cone NAT) , Gaming Mode Enable
			            2=ADDRESS_RESTRICT(dependent)
		*/
#ifdef CONFIG_USER_FIREWALL2
		/* dhcp relay agent */
		if((nvram_match("udp_nat_type", "1") == 0) ||
			(nvram_match("udp_nat_type", "2") == 0))
		{
			if(nvram_match("dhcpd_enable","2")==0)
				save2file(FW_NAT,"-A PREROUTING -p "
					"UDP --dport 67:68 -j ACCEPT\n");

		}
		setup_nattype(wan_if, bri_eth);
#else
		if(nvram_match("tcp_nat_type", "1") == 0)
		{
			FOR_EACH_WAN_FACE
			{
				save2file(FW_FILTER, "-A FORWARD -i %s -o %s -p TCP -j NATTYPE --mode forward_out --type 1\n",\
					bri_eth, wan_if[k]);
				save2file(FW_FILTER, "-A FORWARD -i %s -o %s -p TCP -j NATTYPE --mode forward_in --type 1\n",\
					wan_if[k], bri_eth);
				save2file(FW_NAT, "-A PREROUTING -i %s -p TCP -j NATTYPE --mode dnat --type 1\n",\
					wan_if[k]);
			}
		}
		else if(nvram_match("tcp_nat_type", "2") == 0)
		{
			FOR_EACH_WAN_FACE
			{
				save2file(FW_FILTER, "-A FORWARD -i %s -o %s -p TCP -j NATTYPE --mode forward_out --type 2\n",\
					bri_eth, wan_if[k]);
				save2file(FW_FILTER, "-A FORWARD -i %s -o %s -p TCP -j NATTYPE --mode forward_in --type 2\n",\
					wan_if[k], bri_eth);
				save2file(FW_NAT, "-A PREROUTING -i %s -p TCP -j NATTYPE --mode dnat --type 2\n",\
					wan_if[k]);
			}
		}
		//Bing added 20090511 for dhcp relay agent
		if((nvram_match("udp_nat_type", "1") == 0) || (nvram_match("udp_nat_type", "2") == 0))
		{
			if(nvram_match("dhcpd_enable","2")==0)
				save2file(FW_NAT,"-A PREROUTING -p UDP --dport 67:68 -j ACCEPT\n");

		}
		//
		if(nvram_match("udp_nat_type", "1") == 0)
		{
			FOR_EACH_WAN_FACE
	        {
				save2file(FW_FILTER, "-A FORWARD -i %s -o %s -p UDP -j NATTYPE --mode forward_out --type 1\n",
					bri_eth, wan_if[k]);
				save2file(FW_FILTER, "-A FORWARD -i %s -o %s -p UDP -j NATTYPE --mode forward_in --type 1\n",
					wan_if[k], bri_eth);
				save2file(FW_NAT, "-A PREROUTING -i %s -p UDP -j NATTYPE --mode dnat --type 1\n",
					wan_if[k]);
			}
		}
		else if(nvram_match("udp_nat_type", "2") == 0)
		{
			FOR_EACH_WAN_FACE
			{
				/*
					To pass Internet Connectivity Evaluation Tool (http://www.microsoft.com/windows/using/tools/igd/default.mspx)
				*/
				/*
				save2file(FW_FILTER, "-A FORWARD -i %s -o %s -p UDP -j NATTYPE --mode forward_out --type 2\n",
					bri_eth, wan_if[k]);
				save2file(FW_FILTER, "-A FORWARD -i %s -o %s -p UDP -j NATTYPE --mode forward_in --type 2\n",
					wan_if[k], bri_eth);
				save2file(FW_NAT, "-A PREROUTING -i %s -p UDP -j NATTYPE --mode dnat --type 2\n",
					wan_if[k]);
				*/
				save2file(FW_FILTER, "-A FORWARD -i %s -o %s -p UDP -j NATTYPE --mode forward_out --type 2\n",
					bri_eth, wan_if[k]);
				save2file(FW_FILTER, "-A FORWARD -i %s -o %s -p UDP -j NATTYPE --mode forward_in --type 2\n",
					wan_if[k], bri_eth);
				save2file(FW_NAT, "-A PREROUTING -i %s -p UDP -j NATTYPE --mode dnat --type 1\n",
					wan_if[k]);
	        }
		}

#endif //CONFIG_USER_FIREWALL2

		/*
		 * Gareth Williams (gwilliams@ubicom.com) 2/Dec/2009
		 * FAST PATH: allow packets that are related to or which are part of an established connection to be accepted.
		 */
		save2file(FW_FILTER,"-A FORWARD -m state --state ESTABLISHED,RELATED -j ACCEPT\n");

		/* DMZ */
		DEBUG_MSG("\tDMZ\n");
		if( nvram_match("dmz_enable", "1" ) == 0 ){
			DEBUG_MSG("\tDMZ setting\n");
			{
			/* jimmy added 20081118 , to let
				1. DMZ pc can ping router's wan instead of icmp NATing to itself
				2. !DMZ local pc can correctly get router's wan icmp reply, not DMZ pc's icmp reply
			*/
				FOR_EACH_WAN_IP
				{
					save2file(FW_NAT, "-A PREROUTING -i %s -p ICMP -j ACCEPT\n", bri_eth);
					break;
				}
			/* ---------------------------------------------------------------------- */

				/* ICMP would not redirect to lan pc(dmz pc) */
				FOR_EACH_WAN_IP
				{
					save2file(FW_NAT, "-A PREROUTING -i %s -p ICMP -d %s -j ACCEPT\n", wan_if[k], wan_ipaddr[k]);
					save2file(FW_NAT, "-A PREROUTING -p ALL -d %s %s -j DNAT --to %s\n", wan_ipaddr[k], schedule2iptables(nvram_safe_get("dmz_schedule")), nvram_get("dmz_ipaddr") );
				}
	        	}

			/* jimmy modified 20081118 , prevent wan pc can directly ping lan DMZ pc */
	        //FOR_EACH_WAN_FACE
			//	save2file(FW_FILTER,"-A FORWARD -i %s -o %s -p ALL -d %s %s -m state --state NEW -j ACCEPT\n", wan_if[k], bri_eth, nvram_get("dmz_ipaddr"), schedule2iptables(nvram_safe_get("dmz_schedule")) );
			FOR_EACH_WAN_FACE
			{
				save2file(FW_FILTER,"-A FORWARD -i %s -o %s -p icmp --icmp-type echo-request -d %s %s -j LOG_AND_DROP\n", wan_if[k], bri_eth, nvram_get("dmz_ipaddr"), schedule2iptables(nvram_safe_get("dmz_schedule")) );
				save2file(FW_FILTER,"-A FORWARD -i %s -o %s -p ALL -d %s %s -m state --state NEW -j ACCEPT\n", wan_if[k], bri_eth, nvram_get("dmz_ipaddr"), schedule2iptables(nvram_safe_get("dmz_schedule")) );
			}
			/* ----------------------------------------------------------------------- */
		}
		DEBUG_MSG("DMZ finish\n");
		DEBUG_MSG("NAT finish\n");
#endif //#ifdef NAT_ENABLE

	/* Default rule */
//GGG moved above NEW fast path test 2/Dec/2009	save2file(FW_FILTER,"-A FORWARD -m state --state ESTABLISHED,RELATED -j ACCEPT\n");
	save2file(FW_FILTER,"-A FORWARD -i %s -m state --state NEW -j ACCEPT\n", bri_eth );
	save2file(FW_FILTER,"-A FORWARD -i lo -m state --state NEW -j ACCEPT\n");
	/* Deny WAN acces router's HTTP , default rule*/
	FOR_EACH_WAN_FACE
		save2file(FW_NAT, "-A PREROUTING -p TCP -d %s --dport %d  -j LOG_AND_DROP\n", wan_ipaddr[k], HTTP_PORT);
#if defined(HAVE_HTTPS)
	FOR_EACH_WAN_FACE
		save2file(FW_NAT, "-A PREROUTING -p TCP -d %s --dport %d  -j DROP\n", wan_ipaddr[k], HTTPS_PORT);

{
	char *https_enable, *remote_https_enable, *remote_https_port, *remote_http_port;
	char fw_value[256]={}, *getValue;

	getValue = nvram_get("https_config");
	strncpy(fw_value, getValue, sizeof(fw_value));

	getStrtok(fw_value, "/", "%s %s %s %s", &https_enable, &remote_https_enable, &remote_https_port, &remote_http_port);
	if (strcmp(https_enable, "0") == 0) {
		save2file(FW_NAT, "-A PREROUTING -i %s -d %s -p tcp --dport %d -j LOG_AND_DROP\n", nvram_safe_get("lan_bridge"),nvram_safe_get("lan_ipaddr"), HTTPS_PORT);
	}
}
#endif
    /* Disable RPC port (111) */
    save2file(FW_NAT, "-A PREROUTING -i %s -d %s -p tcp --dport %d -j DROP\n", nvram_safe_get("lan_bridge"), nvram_safe_get("lan_ipaddr"), 111);

	/* Chun: I set it as default rule, which needs to be added in the end of firewall or it may deny other operations*/

	/* 1) Deny TCP port scan for router */
	FOR_EACH_WAN_FACE
		save2file(FW_FILTER, "-A INPUT -p TCP -i %s --syn -j LOG_AND_DROP\n", wan_if[k]);

	/* 2) Deny UDP port scan for router */
	save2file(FW_FILTER,"-A OUTPUT -p icmp --icmp-type port-unreachable -j LOG_AND_DROP\n");

	/* 3) Deny WAN hosts ping router's LAN IP. */
	FOR_EACH_WAN_FACE
		save2file(FW_FILTER, "-A INPUT -p ICMP -i %s -d %s -j LOG_AND_DROP\n", wan_if[k], lan_ipaddr);

#ifdef ADVANCE_SPI
	set_advance_spi_rule();
#endif

	save2file(FW_FILTER, "COMMIT\n");
	_system("iptables-restore %s", FW_FILTER);
	save2file(FW_NAT, "COMMIT\n");
	_system("iptables-restore %s", FW_NAT);
	save2file(FW_MANGLE, "COMMIT\n");
	_system("iptables-restore %s", FW_MANGLE);
	_system("cp %s %s_LAST", FW_FILTER, FW_FILTER);
	unlink(FW_FILTER);
	_system("cp %s %s_LAST", FW_NAT, FW_NAT);
	unlink(FW_NAT);
#ifndef CONFIG_USER_TC
	_system("cp %s %s_LAST", FW_MANGLE, FW_MANGLE);
	unlink(FW_MANGLE);
#endif

	/* Enable linux ip forward function */
	init_file(IP_FORWARD);
	save2file(IP_FORWARD, "1");

/*	Date:	2009-04-20
*	Name:	jimmy huang
*	Reason:	to fixed the bug, When rc restart, firewall restart, miniupnpd not
*			re-add previous rules, cause firewall will clean all rules first
*			in rc, start_app() => start_firewall()
*			Now, we let rc to send signal to ask miniupnpd to reload
*			port forward rules added via UPnP previously
*/
#if defined(UPNP_ATH_MINIUPNPD_1_3) && defined(UPNP_ATH_MINIUPNPD_1_3_LEASE_FILES)
	KILL_APP_SYNCH("-SIGUSR1 miniupnpd");
#endif

	DEBUG_MSG("********** start_firewall end **********\n");
	return 0;
}

int stop_firewall(void)
{
#ifdef AP_NOWAN_HASBRIDGE
	return 1;
#endif
	if(!( action_flags.all || action_flags.lan || action_flags.wan || action_flags.app || action_flags.firewall ) )
		return 1;

	printf("Stop Firewall\n");

	//flush_iptables();

#ifdef CONFIG_USER_TC
	if( !nvram_match("qos_enable", "0") )
       	stop_qos();
#endif

#ifdef HW_NAT
	DEBUG_MSG("\nStop the hw nat now!\n");
	system("/sbin/nat_cfg set disable");
#endif

	stop_route(); //move from stop_wan()
	return 0;
}
