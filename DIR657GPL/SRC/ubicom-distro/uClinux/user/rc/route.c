#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <shvar.h>
#include <nvram.h>
#include <sutil.h>
#include <rc.h>

#ifdef RC_DEBUG
#define DEBUG_MSG(fmt, arg...)       printf("%s [%d]: "fmt, __FILE__, __LINE__, ##arg)
#else
#define DEBUG_MSG(fmt, arg...) 
#endif

#ifdef MPPPOE
int multiple_routing_flag = 0;
#endif//#ifdef MPPPOE


#define RT_TABLES "/var/iproute2/rt_tables"


#ifdef CONFIG_USER_ZEBRA
int start_zebra(void)
{
	char *lan_eth, *wan_eth;
	int wan_rx_ver=0, wan_tx_ver=0, lan_rx_ver=0, lan_tx_ver=0;
#ifdef MPPPOE
	char pppoe_main_session[8] = {};
#endif

	lan_eth = nvram_get("lan_bridge");
#ifndef MPPPOE
	wan_eth = nvram_get("wan_eth");
#else
	sprintf(pppoe_main_session,"ppp%d",atoi(nvram_safe_get("wan_pppoe_main_session")));
	if(nvram_match("wan_proto","unnumbered") == 0 || nvram_match("wan_proto","pptp") == 0 || nvram_match("wan_proto","l2tp") == 0 )
        wan_eth = "ppp0";
    else if(nvram_match("wan_proto","pppoe") == 0)
		wan_eth = pppoe_main_session;
	else
		wan_eth = nvram_get("wan_eth");
#endif
	DEBUG_MSG("start_zebra\n");
	lan_rx_ver = atoi( nvram_safe_get("rip_rx_version") );
	lan_tx_ver = atoi( nvram_safe_get("rip_tx_version") );
	wan_rx_ver = lan_rx_ver;
/*  Date: 2009-8-11
*   Name: Ken Chiang
*   Reason: fixed the wan tx version is fail the version is same as the lan tx version 
            not same as lan rx version.
*   Notice :
*/	
/*
	wan_tx_ver = lan_rx_ver;
*/
	wan_tx_ver = lan_tx_ver;
	
	DEBUG_MSG("lan_rx_ver=%d\n",lan_rx_ver);
	DEBUG_MSG("lan_tx_ver=%d\n",lan_tx_ver);
	DEBUG_MSG("wan_rx_ver=%d\n",wan_rx_ver);
	DEBUG_MSG("wan_tx_ver=%d\n",wan_tx_ver);
	
		
	init_file(ZEBRA_CONF);	
	init_file(RIPD_CONF);
	save2file(RIPD_CONF, 
		"router rip\n"
		" network %s\n"
		" network %s\n"
		"redistribute connected\n"
		"redistribute kernel\n"
		, lan_eth \
		, wan_eth );
			
	/* LAN setting */
	DEBUG_MSG("lan_eth=%s setting\n",lan_eth);
	save2file(RIPD_CONF,	"interface %s\n ip split-horizon\n", lan_eth);
	if(lan_tx_ver > 0 )
		save2file(RIPD_CONF, " ip rip send version %d\n", lan_tx_ver);
	if(lan_rx_ver > 0 )
		save2file(RIPD_CONF, " ip rip receive version %d\n", lan_rx_ver);

	/* WAN setting */
	DEBUG_MSG("wan_eth=%s setting\n",wan_eth);
	save2file(RIPD_CONF, "interface %s\n ip split-horizon\n", wan_eth);
	if(wan_tx_ver > 0 )
		save2file(RIPD_CONF, " ip rip send version %d\n", wan_tx_ver);
	if(wan_rx_ver > 0 )
		save2file(RIPD_CONF, " ip rip receive version %d\n", wan_rx_ver);

	save2file(RIPD_CONF, "router rip\n");
	if(lan_tx_ver == 0 )
		save2file(RIPD_CONF, " distribute-list private out %s\n", lan_eth);
	if(lan_rx_ver == 0 )
		save2file(RIPD_CONF, " distribute-list private in %s\n", lan_eth);
	if(wan_tx_ver == 0 )
		save2file(RIPD_CONF, " distribute-list private out %s\n", wan_eth);
	if(wan_rx_ver == 0 )
		save2file(RIPD_CONF, " distribute-list private in %s\n", wan_eth);
	save2file(RIPD_CONF, "access-list private deny any\n");

	_system("zebra -f %s &", ZEBRA_CONF);
	system("sleep 1");
	_system("ripd -f %s &", RIPD_CONF);

	return 0;
}
#endif //#ifdef CONFIG_USER_ZEBRA

#ifdef CONFIG_USER_ZEBRA
int stop_zebra(void)
{
	KILL_APP_ASYNCH("zebra");
	KILL_APP_ASYNCH("ripd");
	return 0;
}
#endif//#ifdef CONFIG_USER_ZEBRA

#ifdef MPPPOE
void check_table(char *input_string)
{
	char *start,*end,*table;
	char content_string[256] = {};
	char table_name[16] = {};
	FILE *fp;
	if(start = strstr(input_string,"from"))
	{
		start = start + 5;
		if(end = strstr(start,"lookup"))
		{
			table = end;
			table = table + 7;
			end = end - 1;
		}
		memcpy(content_string,start,end - start);
	}
	
	DEBUG_MSG("STRING  = %s\nTABLE name = %s\n",content_string,table);
	if(strcmp(table,"255") == 0 || strcmp(content_string,"all") == 0)
		return;

	if(inet_addr(content_string) != -1)//ip format , type = 0;          
                 _system("ip rule del from %s table %s",content_string,table);   
        else   
        {         
		if(start = strstr(content_string,"to"))   
                {   
                	start = start + 3;                          
                        if(inet_addr(start) != -1)//ip format , type = 1 or 2;                       
                        	_system("ip rule del to %s table %s",start,table);                                  
                }   
                else//port format , type =3   
                {   
                        start = content_string;   
                        start = start + 4;   
                        _system("ip rule del %s table %s",start,table);   
                }   
    
        }       
	//delete default route
	strncpy(table_name,table,3);
	_system("ip route show table %s > %s",table_name, DEL_MPPPOE_GW); 

	fp = fopen(DEL_MPPPOE_GW,"r");
	if(fp)
	{
		memset(content_string,0,sizeof(content_string));
		fgets(content_string,256,fp);
		content_string[strlen(content_string)-1] = '\0';
		_system("ip route del %s table %s",content_string,table_name);
		fclose(fp);
	}

}
#endif //#ifdef MPPPOE

#ifdef MPPPOE
void clean_up_multiple_routing_table(void)
{
	FILE *fp;
        char rule_content[256] = {};
	int count = 0;
	_system("ip rule ls > %s",MULTIPLE_ROUTING);
	fp = fopen(MULTIPLE_ROUTING,"r");
        if(fp)
        {
		while(fgets(rule_content,256,fp))
			count ++;
		fseek(fp,0,SEEK_SET);
		if(count == 3)//no rule inserted, just return
                {
                	fclose(fp);
                	return;
                }
		memset(rule_content,0,sizeof(rule_content));
		while(fgets(rule_content,256,fp))
                {
			
			if(strncmp(rule_content,"32766",5) == 0)// main table
				break;
			check_table(rule_content);
			memset(rule_content,0,sizeof(rule_content));
		}
		fclose(fp);
	}
}
#endif //#ifdef MPPPOE

#ifdef MPPPOE
char *look_up_ip(void)
{
	FILE *fp;
	char content[128] = {};
	int total_line = 0, i = 0;
	char ipaddress[32] = {};
	fp = fopen(DNS_QUERY_RESULT,"r");
	if(fp)
	{
		while(fgets(content,128,fp))
		{
			if(strstr(content,"Unknown host"))
				return "UNKNOW HOST";
			total_line++;
			memset(content,0,sizeof(content));
		}
		if(total_line <= 3)
			return "UNKNOW HOST";
		fseek(fp,0,SEEK_SET);
		for(i=0;i<total_line-1;i++)
		{
			fgets(content,128,fp);
			memset(content,0,sizeof(content));
		}
		fgets(content,128,fp);
		strtok(content," ");
		strcpy(ipaddress,strtok(NULL," "));
		fclose(fp);
		ipaddress[strlen(ipaddress) - 1] = '\0';
		return ipaddress;
	}
	else
		return "UNKNOW HOST";

}
#endif //#ifdef MPPPOE

#ifdef MPPPOE
char *return_ppp_gateway_ipaddr(char *ifname)
{
        FILE *fp;
        char buf[128] = {};
	
	_system("ifconfig %s", ifname);
	fp = fopen(PPP_GATEWAY_IP,"r");
        if(fp)
        {
                if(fgets(buf,128,fp))
                {
			fclose(fp);
			return buf;			
                }
		else
                {	
			fclose(fp);
                	return "0.0.0.0";
		}
        }
        else
                return "0.0.0.0";
}
#endif//#ifdef MPPPOE

void clean_up_static_routing(void)
{
	FILE *fp;
	char buf[256] = {0};
	char *ifname=NULL, *dest_addr=NULL, *dest_mask=NULL, *gateway=NULL, *metirc=NULL;

	/* delet the contents of routing table */
	fp = fopen(ROUTING_INFO,"r");
	
	if(fp)
	{
		while(fgets(buf,256,fp))
		{
			ifname = strtok(buf,"/");
			dest_addr = strtok(NULL,"/");
			dest_mask = strtok(NULL,"/");
			gateway = strtok(NULL,"/");
			metirc = strtok(NULL,"/");
			route_del(ifname,dest_addr,dest_mask,gateway,atoi(metirc));
			memset(buf,0,sizeof(buf));
		}	
		fclose(fp);
	}
	sleep(1);

	/* clear the contents of file */
	init_file(ROUTING_INFO);
}

#ifdef MPPPOE
void add_dns_to_rt_by_ifname(int ifname)
{
	DEBUG_MSG("Start ADD DNS INTO RT BY IFNAME\n");
	
	char dns_ifname[] = "pppXYXX";
	char specify_dns[] = "wan_pppoe_specify_dns_XYXX";
	char primary_dns[] = "wan_pppoe_primary_dns_XYXX";
	char secondary_dns[] = "wan_pppoe_secondary_dns_XYXX";
	char dns_table[] = "dns_table_XYXX";
	int prio = 0;
	char *resolve_file;
	FILE *fp;
	char *buff;
	char dns[2][32] = {};
	int num, i=0, len=0;

	sprintf(dns_ifname,"ppp%d",ifname);
	sprintf(specify_dns,"wan_pppoe_specify_dns_%02d",ifname);
	sprintf(primary_dns,"wan_pppoe_primary_dns_%02d",ifname);
	sprintf(secondary_dns,"wan_pppoe_secondary_dns_%02d",ifname);
	
	switch(ifname)
	{
		case 0:
			prio = 10;
			resolve_file = DNS_FILE_00;
			break;
		case 1:
                        prio = 20;
			resolve_file = DNS_FILE_01;
                        break;
		case 2:
                        prio = 30;
			resolve_file = DNS_FILE_02;
                        break;
		case 3:
                        prio = 40;
			resolve_file = DNS_FILE_03;
                        break;
		case 4:
                        prio = 50;
			resolve_file = DNS_FILE_04;
                        break;
		default:
			DEBUG_MSG("wrong type\n");
			break;
	}

	if(get_ipaddr(dns_ifname))
	{
		if(nvram_match(specify_dns,"1") == 0)// specify dns given by user
		{
			DEBUG_MSG("Setup dns by user\n");
			if(nvram_match(primary_dns,"0.0.0.0") != 0)
			{
				DEBUG_MSG("STATIC: Add primary dns\n");
				sprintf(dns_table,"dns_table_%d",prio);
				_system("echo %d %s >> /var/iproute2/rt_tables",prio,dns_table);
				_system("ip rule add to %s table %s priority %d",nvram_safe_get(primary_dns),dns_table, prio);
				_system("ip route add default via %s dev %s table %s",return_ppp_gateway_ipaddr(dns_ifname),dns_ifname,dns_table);
			}
			prio++;
			if(nvram_match(secondary_dns,"0.0.0.0") != 0)
                        {
                                DEBUG_MSG("STATIC: Add secondary dns\n");
                                sprintf(dns_table,"dns_table_%d",prio);
                                _system("echo %d %s >> /var/iproute2/rt_tables",prio,dns_table);
                                _system("ip rule add to %s table %s priority %d",nvram_safe_get(secondary_dns),dns_table, prio);
                                _system("ip route add default via %s dev %s table %s",return_ppp_gateway_ipaddr(dns_ifname),dns_ifname,dns_table);
                        }
		}
		else if(nvram_match(specify_dns,"0") == 0)// auto dns given by server
		{
			DEBUG_MSG("Setup dns by server\n");
			buff = (char *) malloc(1024);
		        memset(buff, 0, 1024);

			fp = fopen(resolve_file,"r");
			if(fp)
			{
				while( fgets(buff, 1024, fp))
		                {
                		        if (strstr(buff, "nameserver") )
                        		{
                                		num = strspn(buff+10, " ");
                                		len = strlen( (buff + 10 + num) );
                                		strncpy(dns[i], (buff + 10 + num), len-1);  // for \n case
                                		i++;
                        		}
                        		if(2<=i)
                        		{
                                		strcat(dns[1], "\0");
                                		break;
                        		}
					memset(buff, 0, 1024);
				}
				fclose(fp);
                	}
			DEBUG_MSG("DNS1 = %s DNS2 = %s\n",dns[0],dns[1]);
                	if(strcmp(dns[0],"") != 0 && strcmp(dns[0]," ") != 0 && inet_addr(dns[0]) != -1)
			{
				DEBUG_MSG("DYNAMIC: Add primary dns\n");
				sprintf(dns_table,"dns_table_%d",prio);
                                _system("echo %d %s >> /var/iproute2/rt_tables",prio,dns_table);
                                _system("ip rule add to %s table %s priority %d",dns[0],dns_table, prio);
                                _system("ip route add default via %s dev %s table %s",return_ppp_gateway_ipaddr(dns_ifname),dns_ifname,dns_table);
			}
			prio++;
			if(strcmp(dns[1],"") != 0 && strcmp(dns[1]," ") != 0 && inet_addr(dns[1]) != -1)
                        {
                                DEBUG_MSG("DYNAMIC: Add secondary dns\n");
                                sprintf(dns_table,"dns_table_%d",prio);
                                _system("echo %d %s >> /var/iproute2/rt_tables",prio,dns_table);
                                _system("ip rule add to %s table %s priority %d",dns[1],dns_table, prio);
                                _system("ip route add default via %s dev %s table %s",return_ppp_gateway_ipaddr(dns_ifname),dns_ifname,dns_table);
                        }
			free(buff);
		}
	}
}
#endif //#ifdef MPPPOE

#ifdef MPPPOE
void add_dns_to_rt(void)
{
	DEBUG_MSG("Start ADD DNS INTO RT\n");

	int i = 0;
	for(i=0;i<MAX_PPPOE_CONNECTION;i++)
		add_dns_to_rt_by_ifname(i);
}
#endif //#ifdef MPPPOE

#ifdef MPPPOE
int check_domain_result(char *domain_name)
{
	DEBUG_MSG("Start check_domain_result\n");
	FILE *fp;
	char buf[256] = {};
	fp = fopen(DOMAIN_RECORDS,"r");
	if(fp)
	{
		while(fgets(buf,256,fp))
		{
			if(strstr(buf,"UNKNOW HOST"))
				return 0;
			if(strstr(buf,domain_name))
				return 1;
			memset(buf,0,sizeof(buf));
		}
		fclose(fp);
		return 0;
	}
	else
		return 0;
}
#endif //#ifdef MPPPOE

#ifdef MPPPOE
char *return_domain_record(char *domain_name)
{
	DEBUG_MSG("Start return_domain_record\n");
        FILE *fp;
        char buf[256] = {},ip[32] = {};
	char *name;
        fp = fopen(DOMAIN_RECORDS,"r");
        if(fp)
        {
                while(fgets(buf,256,fp))
                {
                        if(strstr(buf,domain_name))
			{
				name = strtok(buf,"/");
				strcpy(ip,strtok(NULL,"/"));
				ip[strlen(ip) - 1] = '\0';
				fclose(fp);
				return ip;		
			}
			memset(buf,0,sizeof(buf));
                }
                fclose(fp);
        }

	return "UNKNOW HOST";//never go this line
}
#endif //#ifdef MPPPOE


int start_route(void)
{
	char static_routing[]="static_routing_XYXXXXX";
	char *enable=NULL, *name=NULL, *value=NULL, *dest_addr=NULL;
	char *dest_mask=NULL, *gateway=NULL, *ifname=NULL, *metric=NULL;
	char route_value[256]={0};
    char interface[8]={0};
	char rule_in_file[256]={0};
	char routing_buf[1024]={0};
	FILE *fp ;
    int i;
	char *wan_proto=NULL;
#ifdef MPPPOE
	char multiple_routing[] = "multiple_routing_table_XYXXXXX";
	char table_name[] = "ppp_table_XYXXXXX";
	char *interface_name,*type,*content;
	int priority = 100;
	char obtain_ip[32] = {},transfer_ip[32] = {};
	char rule[32] = {};
	int multi_route_src_mark = 66000, flag_mroute_added = 0;
	char mpppoe_main_gw_ip[8] = {};
	int main_if_name = atoi(nvram_safe_get("wan_pppoe_main_session"));
	sprintf(mpppoe_main_gw_ip,"ppp%d",main_if_name);
#endif
	
	DEBUG_MSG("start_route\n");
	
	wan_proto = nvram_safe_get("wan_proto"); 

#ifdef MPPPOE
	/* multiple routing */
	if(nvram_match("wan_proto","pppoe") == 0)
	{
		init_file(DOMAIN_INFO);
		init_file(DNS_MPPPOE);
		save2file(DNS_MPPPOE,"%s\n",nvram_safe_get("wan_pppoe_main_session"));
		/* clean up multiple routing table*/
		if(multiple_routing_flag)
		{
			_system("rm -f %s", RT_TABLES);
        		clean_up_multiple_routing_table();
			//_system("ip route flush cache");
			multiple_routing_flag = 0;
		}
		
		add_dns_to_rt();
	
		for(i=0; i<MAX_MULTIPLE_ROUTING_NUMBER; i++) 
		{
			memset(route_value,0,sizeof(route_value));
			memset(obtain_ip,0,sizeof(obtain_ip));
			memset(table_name,0,sizeof(table_name));
			sprintf(multiple_routing, "multiple_routing_table_%02d", i);
			sprintf(table_name, "ppp_table_%02d", i);
			value = nvram_safe_get(multiple_routing);
			if ( strlen(value) == 0 )
				break;
			else
				strcpy(route_value, value);
			/* for dns query*/
			save2file(DNS_MPPPOE,"%s\n",route_value);			
			interface_name = strtok(route_value, "/");
			type = strtok(NULL, "/");
			content = strtok(NULL, "/");
			DEBUG_MSG("interface_name = %s\ntype = %s\ncontent = %s\ntable_name = %s\n",interface_name,type,content,table_name);
				
			//if(get_ipaddr(interface_name) && check_previous_setting(type,content,table_name) == 0)
			if(get_ipaddr(interface_name))
			{	
				flag_mroute_added = 1;
				_system("echo %d %s >> /var/iproute2/rt_tables",priority,table_name);	
				if(strcmp(type,"0") == 0)
					_system("ip rule add from %s table %s priority %d", content, table_name, priority);
				else if(strcmp(type,"1") == 0)
					_system("ip rule add to %s table %s priority %d",content,table_name, priority);
				else if((strcmp(type,"2") == 0) && (check_domain_result(content) == 0))
				{//first look up or look up fail
					DEBUG_MSG("First look up or lookup fail\n");
					_system("nslookup %s > %s",content,DNS_QUERY_RESULT);
					strcpy(transfer_ip,look_up_ip());
					DEBUG_MSG("transfer_ip = %s\n",transfer_ip);
					save2file(DOMAIN_INFO,"%s/%s\n",content,transfer_ip);	
					if(strcmp(transfer_ip,"UNKNOW HOST") != 0 && inet_addr(transfer_ip) != -1)
						_system("ip rule add to %s table %s priority %d",transfer_ip,table_name, priority);
					init_file(DNS_QUERY_RESULT);
				}
				else if((strcmp(type,"2") == 0) && (check_domain_result(content) == 1))
                                {//already have a record 
					DEBUG_MSG("Already have a record\n");
					strcpy(transfer_ip,return_domain_record(content));
					save2file(DOMAIN_INFO,"%s/%s\n",content,transfer_ip);
					if(strcmp(transfer_ip,"UNKNOW HOST") != 0 && inet_addr(transfer_ip) != -1)
                                                _system("ip rule add to %s table %s priority %d",transfer_ip,table_name, priority);	
				}
				else if(strcmp(type, "3") == 0)
					_system("ip rule add fwmark %s table %s priority %d", content, table_name, priority);
					
				_system("ip route add default via %s dev %s table %s",return_ppp_gateway_ipaddr(interface_name),interface_name,table_name);				
				priority++;
			}
		}
		multiple_routing_flag = 1;
		_system("cp -f %s %s",DOMAIN_INFO,DOMAIN_RECORDS);
		//if(flag_mroute_added == 1)
			//_system("ip route replace default equalize nexthop via %s dev %s", return_ppp_gateway_ipaddr(mpppoe_main_gw_ip),mpppoe_main_gw_ip);
	}
	else
	{
		DEBUG_MSG("multiple_routing_flag = %d\n",multiple_routing_flag);
		if(multiple_routing_flag)
                {
                	_system("rm -f %s", RT_TABLES);
                        clean_up_multiple_routing_table();
                        _system("ip route flush cache");
                        multiple_routing_flag = 0;
                }
	}
#endif //#ifdef MPPPOE

	//delete route rule
    /* delet the contents of routing table */
	clean_up_static_routing();
	
	/* Static Routing */
	/* nvram: enable/name/dest_addr/dest_mask/gateway/interface/metric */
	for(i=0; i<MAX_STATIC_ROUTING_NUMBER; i++) 
	{
		memset(route_value,0,sizeof(route_value));
		sprintf(static_routing, "static_routing_%02d", i);
		value = nvram_safe_get(static_routing);		
		//DEBUG_MSG("static_routing=%s,value=%s\n",static_routing,value);	
		
        if ( strlen(value) == 0 ) 
			continue;
		else 
			strcpy(route_value, value);

		enable = strtok(route_value, "/");
		name = strtok(NULL, "/");
		dest_addr = strtok(NULL, "/");
		dest_mask = strtok(NULL, "/");
		gateway = strtok(NULL, "/");
		ifname = strtok(NULL, "/");
		metric = strtok(NULL, "/");
		
		if( enable==NULL || name==NULL || dest_addr==NULL || 
			dest_mask==NULL || gateway==NULL || ifname==NULL || metric==NULL)
			continue;	
		
		if( !strcmp(enable, "1" ) )
		{
			DEBUG_MSG("enable=%s,name=%s,dest_addr=%s,dest_mask=%s,gateway=%s,ifname=%s,metric=%s\n",enable,name,dest_addr,dest_mask,gateway,ifname,metric);
			memset(interface,0,sizeof(interface));
			memset(rule_in_file,0,sizeof(rule_in_file));
			
			/* Decide interface 
			 * 1. LAN: br0
			 * 2. WAN:
			 * 			Static/DHCP: eth0
			 * 			PPTP/L2TP/PPPoE: ppp0
			 * 			PPTP/L2TP/PPPoE(Russia): ppp0
			 * 3. WAN_PHY:
			 * 			PPTP/L2TP/PPPoE(Russia): eth0
			 */

			if(!strncmp(ifname, "LAN", 3))
				strcpy(interface,nvram_get("lan_bridge"));					
			else
			{	
				if( !strcmp(wan_proto,"static") || !strcmp(wan_proto,"dhcpc") )
				{
					if(!strncmp(ifname, "WAN_PHY", 7))
						continue;
					else if(!strncmp(ifname, "WAN", 3))
						strcpy(interface,nvram_safe_get("wan_eth"));
					else 
						continue;
				}
				else
				{
					if(!strncmp(ifname, "WAN_PHY", 7))
					{
#ifdef RPPPOE	
						if(!nvram_match("wan_pptp_russia_enable","1") || 
						   !nvram_match("wan_pppoe_russia_enable","1") ||
						   !nvram_match("wan_l2tp_russia_enable","1")
						)
						    strcpy(interface,nvram_safe_get("wan_eth"));
						else 
#endif
						    continue;
					}
					else if(!strncmp(ifname, "WAN", 3))
					{
#ifndef MPPPOE
						strcpy(interface,"ppp0");
#else
						if(strcmp(wan_proto,"pppoe")==0)
							strcpy(interface,mpppoe_main_gw_ip);
						else if(strcmp(wan_proto,"unnumbered")==0 || 
								strcmp(wan_proto,"pptp")==0 || strcmp(wan_proto,"l2tp")==0)
							strcpy(interface,"ppp0");
#endif
		}
					else 
						continue;
		}
	}

			/* Insert rule into routing table */
			DEBUG_MSG("route_add interface=%s\n",interface);
			route_add( interface, dest_addr, dest_mask, gateway, atoi(metric));

			/* Save the rule into file*/
				sprintf(rule_in_file,"%s/%s/%s/%s/%s\n",interface, dest_addr, dest_mask, gateway, metric);
				save2file(ROUTING_INFO, rule_in_file);				
		}
	}

#ifdef CONFIG_USER_ZEBRA
	if( !nvram_match("rip_enable", "1") )
	{
		DEBUG_MSG("rip_enable\n");
        if(checkServiceAlivebyName("zebra") || checkServiceAlivebyName("ripd"))
		{
			stop_zebra();
			sleep(1);
		}
		start_zebra();
	}
#endif	
	
#ifdef MPPPOE	
	DEBUG_MSG("Finish ROUTE \n");
	if(multiple_routing_flag)
	_system("ip route flush cache");
#endif//#ifdef MPPPOE		
	return 0;
}


int stop_route(void)
{
	DEBUG_MSG("Stop ROUTE \n");
#ifdef CONFIG_USER_ZEBRA	
	stop_zebra();
#endif	
	return 0;
}
