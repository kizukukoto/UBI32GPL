#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <linux_vct.h>
#include <net/if.h>
#include <net/if_arp.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <stdarg.h>
#include <ctype.h>


#include "libdb.h"
#include "debug.h"
#include "hnap.h"
#include "hnapwan.h"
#include "sutil.h"
#include "shvar.h"

extern int do_xml_response(const char *);
extern int do_xml_mapping(const char *, hnap_entry_s *);
extern void do_get_element_value(const char *, char *, char *);

char wan_current_state[150] = {};
routing_table_t routing_table_list[];


/* for WanSettings*/
pppoe_pptp_l2tp_t pppoe_pptp_l2tp_table[] = {
	{ 	"wan_pppoe_connect_mode_00","wan_pppoe_username_00","wan_pppoe_password_00",
		"wan_pppoe_max_idle_time_00","wan_pppoe_service_00","wan_pppoe_ipaddr_00",
		"wan_pppoe_netmask_00","wan_pppoe_gateway_00","wan_pppoe_dynamic_00"},
		
	{	"wan_pptp_connect_mode","wan_pptp_username","wan_pptp_password",
		"wan_pptp_max_idle_time","wan_pptp_server_ip","wan_pptp_ipaddr",
		"wan_pptp_netmask","wan_pptp_gateway","wan_pptp_dynamic"},
		
	{	"wan_l2tp_connect_mode","wan_l2tp_username","wan_l2tp_password",
		"wan_l2tp_max_idle_time","wan_l2tp_server_ip","wan_l2tp_ipaddr",
		"wan_l2tp_netmask","wan_l2tp_gateway","wan_l2tp_dynamic"}
};

char *proc_gen_fmt_list(char *name, int more, FILE * fh,...)
{
	char buf[512], format[512] = "";
	char *title, *head, *hdr;
	va_list ap;

	if (!fgets(buf, (sizeof buf) - 1, fh))
		return NULL;
	strcat(buf, " ");

	va_start(ap, fh);
	title = va_arg(ap, char *);
	for (hdr = buf; hdr;) {
		while (isspace(*hdr) || *hdr == '|')
			hdr++;
		head = hdr;
		hdr = strpbrk(hdr, "| \t\n");
		if (hdr)
			*hdr++ = 0;

		if (!strcmp(title, head)) {
			strcat(format, va_arg(ap, char *));
			title = va_arg(ap, char *);
			if (!title || !head)
				break;
		} else {
			strcat(format, "%*s");  /* XXX */
		}
		strcat(format, " ");
	}
	va_end(ap);

	if (!more && title) {
		fprintf(stderr, "warning: %s does not contain required field %s\n",
				name, title);
		return NULL;
	}
	return strdup(format);
}

static char *mac_complement(char *mac,char *result)
{
	char tmp_1[3] = {};
	char tmp_2[3] = {};		
	char start[20] = {};
	int i=0;
	
	memcpy(start,mac,strlen(mac));
	strcat(start,":");	
	for(i=0; i<strlen(mac);){
		tmp_1[0] = start[i];
		tmp_2[0] = start[i+1];

		if(strcmp(tmp_1,":") == 0 )		
			return NULL;
		else if(strcmp(tmp_2,":") == 0) {
			sprintf(result,"%s%s%s%s",result,"0",tmp_1,":");
			i = i + 2 ;			
		}else {
			sprintf(result,"%s%s%s%s",result,tmp_1,tmp_2,":");
			i = i+3;			
		}
		memset(tmp_1,0,3);
		memset(tmp_2,0,3);
	}
	
	*(strrchr(result,':')) = '\0';
	return result;	
	
}

int mac_check(char *mac,int wan_flag)
{			
	char *start = NULL;
	char tmp[6] = {};	
	int i=0;
	char res[20] = {};
	
	if((strlen(mac) < MAC_LENGTH ) && (strlen(mac)>0) ){	// if < 17 is likely "1:2:3:4:5:6"
		start = mac_complement(mac,res);		
		if(start == NULL || strlen(start) != MAC_LENGTH)
			return 0;
	} else if(strlen(mac) == MAC_LENGTH)	// check len ( 17 is correct )	
		start = mac;
	else
		return 0;
	
        /*NickChou modify 20100430 for MAC Address = 1C:XX:XX:XX:XX:XX*/
	//'A'=65(Decimal), 'B'=66, 'a'=97, 'b'=98 
	memcpy(tmp,start,3);								
	if( tmp[1] > 64 && (tmp[1]%2 == 0) ) {
		/*Do not allow multicast address
		xb:xx:xx:xx:xx:xx, xd:xx:xx:xx:xx:xx, xf:xx:xx:xx:xx:xx
		xB:xx:xx:xx:xx:xx, xD:xx:xx:xx:xx:xx, xF:xx:xx:xx:xx:xx
		*/
		return 0;
	}
	
	//'0'=48(Decimal), '1'=49
	if( tmp[1] > 47 && (tmp[1]%2 == 1) ) {	
		/*Do not allow multicast address
		x1:xx:xx:xx:xx:xx, x3:xx:xx:xx:xx:xx, x5:xx:xx:xx:xx:xx, x7:xx:xx:xx:xx:xx, x9:xx:xx:xx:xx:xx
		*/
		return 0;
	}
	memset(tmp,0,6);	
	
	// if format correct
	for(i=0; i<MAC_LENGTH; i++) {
		memcpy(tmp,start,1);				
		if(i == 2 || i == 5 || i == 8 || i == 11 || i == 14) {
			// check ":" position
			if(strcmp(tmp,":") == 0)
				goto move;
			else				
				return 0;			
		}
				
		if( strcasecmp(tmp,"a") == 0 || strcasecmp(tmp,"b") == 0 || strcasecmp(tmp,"c") == 0 || 
				strcasecmp(tmp,"d") == 0 || strcasecmp(tmp,"e") == 0 || strcasecmp(tmp,"f") == 0)						
			goto move; // check "a to f"
							
		if( strcmp(tmp,"0") == 0) // check "0"
			goto move;

		// check "1~9"							
		if( 1 <= atoi(tmp) && atoi(tmp)<= 9)
			goto move;			
		else					 
			return 0;	
move:									
			start++;			
	}
	
	return 1;	
}



static int illegal_mask(char *mask_token)
{
	if( 0 >= strlen(mask_token) || strlen(mask_token) > 3 )  //	check length
		return 0;
	
	/* illegal netmask value */	
	if((strcmp(mask_token,"255") ==0) || (strcmp(mask_token,"254") ==0) ||
	   (strcmp(mask_token,"252") ==0) || (strcmp(mask_token,"248") ==0) ||
	   (strcmp(mask_token,"240") ==0) || (strcmp(mask_token,"224") ==0) ||
	   (strcmp(mask_token,"192") ==0) || (strcmp(mask_token,"128") ==0) ||
	   (strcmp(mask_token,"0") ==0) )
		return 1;
	else
		return 0;				
}

static int ip_check(char *token_1, char *token_2, char *token_3, char *token_4)
{
	/* check length */
	if( strlen(token_1) <= 0 || strlen(token_1) > 3 || 
		strlen(token_2) <= 0 || strlen(token_2) > 3 || 
		strlen(token_3) <= 0 || strlen(token_3) > 3 || 
		strlen(token_4) <= 0 || strlen(token_4) > 3)

		return 0;

	/* check range */	
	if( atoi(token_1) < 0 || atoi(token_1) > 255 || 
		atoi(token_2) < 0 || atoi(token_2) > 255 || 
		atoi(token_3) < 0 || atoi(token_3) > 255 || 
		atoi(token_4) < 0 || atoi(token_4) > 254 )

		return 0;
		
	return 1;	
}


static char *read_ipaddr(char *if_name)
{
	int sockfd;
	struct ifreq ifr;
	struct in_addr in_addr, netmask;
	static char ipaddr[64]={};

	if ((sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_RAW)) < 0)
		goto out;

	strcpy(ifr.ifr_name, if_name);
	if( ioctl(sockfd, SIOCGIFADDR, &ifr) < 0) 
		goto out;

	in_addr.s_addr = sin_addr(&ifr.ifr_addr).s_addr;
	sprintf(ipaddr, "%s", inet_ntoa(in_addr));

	if( ioctl(sockfd, SIOCGIFNETMASK, &ifr) < 0) 
		goto out;	

	netmask.s_addr = sin_addr(&ifr.ifr_netmask).s_addr;
	sprintf(ipaddr, "%s/%s", ipaddr, inet_ntoa(netmask));

	close(sockfd);

		return ipaddr;

out:
	return NULL;

}

char *read_ipaddr_no_mask(char *if_name)
{
	int sockfd;
	struct ifreq ifr;
	struct in_addr in_addr;
	int valid=1;
	static char ipaddr[64]={};

	if ((sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_RAW)) < 0)
		return 0;

	strcpy(ifr.ifr_name, if_name);
	if( ioctl(sockfd, SIOCGIFADDR, &ifr) < 0) 
		valid = 0;

	in_addr.s_addr = sin_addr(&ifr.ifr_addr).s_addr;
	sprintf(ipaddr, "%s", inet_ntoa(in_addr));

	close(sockfd);

	if(valid == 0)
		return NULL;
	else 
		return ipaddr;
}

static int netmask_check(char *address, char *token_1, char *token_2, char *token_3, char *token_4){
	/* check length & illegal value */
	if( (illegal_mask(token_1) == 0) || (illegal_mask(token_2) == 0) || 
		(illegal_mask(token_3) == 0) || (illegal_mask(token_4) == 0) )   
			return 0;

	if(strcmp(token_1,"255") == 0) {
		if(strcmp(token_2,"255") == 0) {                                         
  			if(strcmp(token_3,"255") == 0) { 
  			return 1;                                                                
 			                                                                
  			} else {

				strcat(token_1,".");                                                
	  			strcat(token_1,token_2);                                       
  				strcat(token_1,".");                                                
  				strcat(token_1,token_3);                                       
  				strcat(token_1,".0");				                                       
	  			if(strcmp(token_1,address) != 0)   // should be 255.255.255.0       
  					return 0;
			} 
	                                                                         
  		} else{                                                                        
 			strcat(token_1,".");                                                  
	  		strcat(token_1,token_2);                                         
		  	strcat(token_1,".0.0");	// should be 255.255.0.0         
  			if(strcmp(token_1,address) != 0)                                      
  				return 0;
		}
                                                                            
  	} else { 

		strcat(token_1,".0.0.0"); // should be 255.0.0.0
		
		if(strcmp(token_1,address) != 0)                                        
  		return 0;                                                                  
  	}  	     			
  
  	return 1;
}

int address_check(char *address,int type){
	char *start_1,*end_1;
	char *start_2,*end_2;
	char *start_3,*end_3;
	char *start_4;
	char mask_token_1[32] = "";
	char mask_token_2[32] = "";
	char mask_token_3[32] = "";
	char mask_token_4[32] = "";
	int i = 0;
	
	for(i=0; i<strlen(address); i++){						// check if it is "0 ~ 9", ignore "."
		if(*(address+i) == '.')
			continue;
		else
			if(isdigit(*(address+i)) == 0)	
				return 0;
	}

	if(strcmp(address,"0.0.0.0") == 0)					// is not allowed for nvram
		return 0;
		
	start_1 = address;
	if((end_1 = strstr(address,".")) == NULL)		
		return 0;
	start_2 = end_1 + 1;
	if((end_2 = strstr(start_2,".")) == NULL)   
		return 0;
	start_3 = end_2 + 1;
	if((end_3 = strstr(start_3,".")) == NULL)   
		return 0;
	start_4 = end_3 + 1;
	if((end_3 = strstr(start_3,".")) == NULL)   
		return 0;
	
	/* get value from xml value */ 
	strncpy(mask_token_1,start_1,end_1 - start_1);	
	strncpy(mask_token_2,start_2,end_2 - start_2);
	strncpy(mask_token_3,start_3,end_3 - start_3);
	strcpy(mask_token_4,start_4);    
	
	switch(type){
		case 1:   // ip , gateway , service
			if(!ip_check(mask_token_1,mask_token_2,mask_token_3,mask_token_4))
				return 0;
			break;
		case 2:		// netmask	
			if(!netmask_check(address,mask_token_1,mask_token_2,mask_token_3,mask_token_4))
			{
				return 0;
			}
			break;
	}
	return 1;
} 

char *read_dns(int ifnum)
{
	FILE *fp;
	char *buff;
	char dns[3][IPV4_IP_LEN]={};
	static char return_dns[3*IPV4_IP_LEN] ="";
	int num, i=0, len=0;

	fp = fopen (RESOLVFILE, "r");


	if(fp != NULL) {

		if(ifnum ==5)
			sprintf(return_dns, "%s/%s/%s", "0.0.0.0", "0.0.0.0","0.0.0.0");
		else
			sprintf(return_dns, "%s/%s", "0.0.0.0", "0.0.0.0");
	
		return return_dns;
	}

	buff = (char *) malloc(1024);
	if (!buff) {	
		fclose(fp);
		return 0;
	}

	memset(buff, 0, 1024);

	if(ifnum == 5){
		while( fgets(buff, 1024, fp)) {

			if (strstr(buff, "nameserver") ) {
				num = strspn(buff+10, " ");
				len = strlen( (buff + 10 + num) );
				strncpy(dns[i], (buff + 10 + num), len-1);  // for \n case
				i++;
        	        }
		
                	if(3<=i) {
				strcat(dns[2], "\0");
				break;
			}
		}
		free(buff);
		sprintf(return_dns, "%s/%s/%s", dns[0], dns[1], dns[2]);        

	} else {
		while( fgets(buff, 1024, fp)) {
			if (strstr(buff, "nameserver") ) {
				num = strspn(buff+10, " ");
				len = strlen( (buff + 10 + num) );
				strncpy(dns[i], (buff + 10 + num), len-1);  // for \n case
				i++;
			}
		
                	if(2<=i) {
				strcat(dns[1], "\0");
				break;
			}
		}
		free(buff);
		sprintf(return_dns, "%s/%s", dns[0], dns[1]);
	}
	fclose(fp);
	return return_dns;
}


char *read_gate_way_addr(char *if_name,char *wanipaddr)
{
	FILE *fp;
	char buff[1024]={}, iface[16]={};
	char gate_addr[128]={}, net_addr[128]={};
	char mask_addr[128]={}, *fmt;
	int num, iflags, metric, refcnt, use, mss, window, irtt;
	struct in_addr gateway, dest, route, mask;
	int success=0, count =0;
	
	int i = 0;
	int already_found_in_nvramORdhcp = 0;
	char lan_bridge[5]={};

	typedef struct static_routing_table {
		char dest_addr[16];
		char dest_mask[16];
		char gateway[16];
		char interface[8];
		char metric[3];
	} static_routing_table;

	static_routing_table *static_routing_lists_nvram[MAX_STATIC_ROUTING_NUMBER];//shvra.h


	//read nvram routing info
	memset(lan_bridge,'\0',sizeof(lan_bridge));

	for(i = 0;i < MAX_STATIC_ROUTING_NUMBER; i++)
		static_routing_lists_nvram[i]=NULL;
	
	fp = fopen (PATH_PROCNET_ROUTE, "r");

	if(!fp) {
		perror(PATH_PROCNET_ROUTE);
		return 0;
	}

	fmt = proc_gen_fmt_list(PATH_PROCNET_ROUTE, 0, fp,
			"Iface", "%16s",
			"Destination", "%128s",
			"Gateway", "%128s",
			"Flags", "%X",
			"RefCnt", "%d",
			"Use", "%d",
			"Metric", "%d",
			"Mask", "%128s",
			"MTU", "%d",
			"Window", "%d",
			"IRTT", "%d",
			NULL);

	if (!fmt)
		return 0;

	
	for(count =0;  strlen(routing_table_list[count].dest_addr) >0; count++ ) {
		strcpy(routing_table_list[count].dest_addr, "") ;
	}
	count =0;
	
	while( fgets(buff, 1024, fp)) {
		num = sscanf(buff, fmt, iface, net_addr, gate_addr, &iflags, &refcnt, &use, &metric, mask_addr, &mss, &window, &irtt);
		if(iflags & RTF_UP) {
			dest.s_addr = strtoul(net_addr, NULL, 16);
			route.s_addr = strtoul(gate_addr, NULL, 16); 
			mask.s_addr = strtoul(mask_addr, NULL, 16); 

			/* for route_table */
			strcpy(routing_table_list[count].dest_addr, inet_ntoa(dest));
			strcpy(routing_table_list[count].dest_mask, inet_ntoa(mask));
			strcpy(routing_table_list[count].gateway, inet_ntoa(route));
			strcpy(routing_table_list[count].interface, iface);
			routing_table_list[count].metric = metric;

			/*	Date: 2008-12-22
			*	Name: jimmy huang
			*	Reason: for the feature routing information can show "Type" and "Creator"
			*			so we add 2 more fileds in routing_table_s structure
			*	Note:
			*	  Type:
			*		0:INTERNET: 	The table is learned from Internet
			*		1:DHCP Option:	The table is learned from DHCP server in Internet
			*		2:STATIC:		The table is learned from "Static Route" for Internet
			*						Port of DIR-Series
			*		3:INTRANET	:	The table is used for Intranet
			*		4:LOCAL		:	Local loop back
			*
			*	Creator:
			*		0:System:		Learned automatically
			*		1:ADMIN:		Learned via user's operation in Web GUI			
			*/
			if(strncmp(routing_table_list[count].interface,lan_bridge,strlen(lan_bridge)) == 0)//br0
				routing_table_list[count].type = 3;
			else if(strcmp(routing_table_list[count].interface,"lo") == 0)
				routing_table_list[count].type = 4;
			else
				routing_table_list[count].type = 0;
			routing_table_list[count].creator = 0;
			already_found_in_nvramORdhcp = 0;

			// check if this route is added via rfc 3442 (option 121/249)

			// check if this route is added by user via Web function "static route"
			if((already_found_in_nvramORdhcp != 1) && (static_routing_lists_nvram[0] != NULL)){
				for(i=0; i < MAX_STATIC_ROUTING_NUMBER && static_routing_lists_nvram[i] != NULL && 
					strlen(static_routing_lists_nvram[i]->dest_addr) > 0; i++ ){

					if((strcmp(routing_table_list[count].dest_addr,static_routing_lists_nvram[i]->dest_addr) == 0)
						&& (strcmp(routing_table_list[count].dest_mask,static_routing_lists_nvram[i]->dest_mask) == 0)
						&& (strcmp(routing_table_list[count].gateway,static_routing_lists_nvram[i]->gateway) == 0)
						)
					{
						routing_table_list[count].type = 2;//STATIC
						routing_table_list[count].creator = 1;//Admin
						already_found_in_nvramORdhcp = 1;
						break;
					}
				}
			}
			
			count +=1;
			/* end */

			if ((iflags & RTF_GATEWAY) && !strcmp("0.0.0.0", inet_ntoa(dest)) && !strcmp(iface, if_name) ) {
				gateway.s_addr = strtoul(gate_addr,NULL,16);
				success =1;
			}
		}
	}

	free(fmt);
	fclose(fp);

	for(i = 0;i < MAX_STATIC_ROUTING_NUMBER; i++){
		if(static_routing_lists_nvram[i] != NULL)
			free(static_routing_lists_nvram[i]);
	}

	if(success)
		return inet_ntoa(gateway); 
	else 
		return "0.0.0.0";
}

void read_current_ipaddr(int ifnum,char *wan_current_state)
{
	char wan_current_ipaddr[128]={};
	char eth[10] = {};
	char status[15] = {};
	char *wan_proto=NULL;
	char *eth_ipaddr=NULL;

	wan_proto = nvram_safe_get("wan_proto"); 

	if( strcmp(wan_proto, "dhcpc") ==0 || strcmp(wan_proto, "static") ==0) {
        	ifnum= 5;		// for read_dns() to differentiate wan_proto. (if wan_proto==dhcpc | static, it has 3 dns servers)
		strcpy(eth, nvram_safe_get("wan_eth"));
	} else
		sprintf(eth, "ppp%d", ifnum);

	/* Check phy connect statue */
	VCTGetPortConnectState( nvram_safe_get("wan_eth"), VCTWANPORT0, status);
	if( !strncmp("disconnect", status, 10) ) {
		sprintf(wan_current_state,"0.0.0.0/0.0.0.0/0.0.0.0/0.0.0.0/0.0.0.0");
		return 0;
	}

	eth_ipaddr = read_ipaddr(eth);
	if(eth_ipaddr == NULL)
		sprintf(wan_current_ipaddr, "0.0.0.0/0.0.0.0/0.0.0.0/0.0.0.0/0.0.0.0");
	else if(!strncmp(eth_ipaddr, "10.64.64.64", 11) || strlen(eth_ipaddr)==0)
		sprintf(wan_current_ipaddr, "0.0.0.0/0.0.0.0/0.0.0.0/0.0.0.0/0.0.0.0");

	else {
		sprintf(wan_current_ipaddr, "%s", eth_ipaddr);
		sprintf(wan_current_ipaddr + strlen(wan_current_ipaddr), "/%s", read_gate_way_addr(eth,wan_current_ipaddr));
	    	sprintf(wan_current_ipaddr + strlen(wan_current_ipaddr), "/%s", read_dns(ifnum));
	}
	//return wan_current_ipaddr;
	//strcpy(wan_current_state,wan_current_ipaddr);
	//cprintf("XXX %s[%d] wan_current_ipaddr:%s XXX\n", __FUNCTION__, __LINE__,wan_current_ipaddr);
	sprintf(wan_current_state,"%s",wan_current_ipaddr);

}

static int get_pppoe_pptp_l2tp_settings(int w_index){
	char *mode_tmp = NULL;
	
	mode_tmp = nvram_safe_get(pppoe_pptp_l2tp_table[w_index].mode);

	//if(strcmp(mode_tmp,"always_on") == 0 )
	if(strcmp(mode_tmp,"on_demand") == 0 )
		wan_settings.mode = "true";					
	else if(strcmp(mode_tmp,"manual") == 0 )
		wan_settings.mode = "false";
	else if	(strcmp(mode_tmp,"on_demand") == 0)		// not support in pure
		return 0;
	else 
		return 0;
		
	wan_settings.name = nvram_safe_get(pppoe_pptp_l2tp_table[w_index].name);
	wan_settings.password = nvram_safe_get(pppoe_pptp_l2tp_table[w_index].password);
	wan_settings.idle_time = nvram_safe_get(pppoe_pptp_l2tp_table[w_index].idle_time);
	wan_settings.service = nvram_safe_get(pppoe_pptp_l2tp_table[w_index].service);			
	wan_settings.ip = nvram_safe_get(pppoe_pptp_l2tp_table[w_index].ip);
	wan_settings.netmask = nvram_safe_get(pppoe_pptp_l2tp_table[w_index].netmask);
	wan_settings.gateway = nvram_safe_get(pppoe_pptp_l2tp_table[w_index].gateway);
	
	return 1;
}


static int set_pppoe_pptp_l2tp_settings(int w_index){
	if(strcasecmp(wan_settings.mode,"true") ==  0)
		//wan_settings.mode = "always_on";								
		wan_settings.mode = "on_demand";
	else if(strcasecmp(wan_settings.mode,"false") == 0 )
		wan_settings.mode = "manual";
	else
		return 0;
	
	nvram_set(pppoe_pptp_l2tp_table[w_index].name,wan_settings.name);
	nvram_set(pppoe_pptp_l2tp_table[w_index].password,wan_settings.password);
	nvram_set(pppoe_pptp_l2tp_table[w_index].idle_time,wan_settings.idle_time);
	nvram_set(pppoe_pptp_l2tp_table[w_index].service,wan_settings.service);
	nvram_set(pppoe_pptp_l2tp_table[w_index].mode,wan_settings.mode);
	nvram_set(pppoe_pptp_l2tp_table[w_index].ip,wan_settings.ip);
	nvram_set(pppoe_pptp_l2tp_table[w_index].netmask,wan_settings.netmask);
	nvram_set(pppoe_pptp_l2tp_table[w_index].gateway,wan_settings.gateway);
	nvram_set(pppoe_pptp_l2tp_table[w_index].dynamic,wan_settings.dynamic);
	
	return 1;
}

static int get_wan_settings(int type)
{
	/* for dhcp mode */

	switch(type){
		case 1:	// static
			wan_settings.name = "";
			wan_settings.password = "";
			wan_settings.idle_time = "0";
			wan_settings.service = "";
			wan_settings.mode = "";
			wan_settings.ip = nvram_safe_get("wan_static_ipaddr");
			wan_settings.netmask = nvram_safe_get("wan_static_netmask");
			wan_settings.gateway = nvram_safe_get("wan_static_gateway");					
			wan_settings.primary_dns = nvram_safe_get("wan_primary_dns");
			wan_settings.secondary_dns = nvram_safe_get("wan_secondary_dns");
/*
*   Date: 2010-05-25
*   Name: Marine Chiu
*   Reason: to pass HNAP TestDevice 10.2.8130
*   Notice : get all of wan settings
*/
                        wan_settings.mtu= nvram_safe_get("wan_mtu");				
			break;
	
		case 2:  // static pppoe			
			if(!get_pppoe_pptp_l2tp_settings(0))
				return 0;            
			wan_settings.primary_dns = nvram_safe_get("wan_primary_dns");
			wan_settings.secondary_dns = nvram_safe_get("wan_secondary_dns");
                        wan_settings.mtu= nvram_safe_get("wan_pppoe_mtu");					 
			break;

		case 3:  // dynamic pppoe						
			if(!get_pppoe_pptp_l2tp_settings(0))
				return 0;
			wan_settings.primary_dns = nvram_safe_get("wan_primary_dns");
			wan_settings.secondary_dns = nvram_safe_get("wan_secondary_dns");
                        wan_settings.mtu= nvram_safe_get("wan_pppoe_mtu");				
			break;	

		case 4: // dhcp
			/* initial wan current status */
			memset(wan_current_state, 0 ,sizeof(wan_current_state));
			read_current_ipaddr(0,wan_current_state);
			/* save value to struct */
			wan_settings.name = "";
			wan_settings.password = "";
			wan_settings.idle_time = "0";
			wan_settings.service = "";
			wan_settings.mode = "";
			getStrtok(wan_current_state, "/", "%s %s %s %s %s", 
					&wan_settings.ip, &wan_settings.netmask, &wan_settings.gateway, 
					&wan_settings.primary_dns, &wan_settings.secondary_dns);	
                        wan_settings.mtu= nvram_safe_get("wan_mtu");		
			break;

		case 5:	// static pptp
			if(!get_pppoe_pptp_l2tp_settings(1))
				return 0;
			wan_settings.primary_dns = nvram_safe_get("wan_primary_dns");
			wan_settings.secondary_dns = nvram_safe_get("wan_secondary_dns");
                        wan_settings.mtu= nvram_safe_get("wan_pptp_mtu");				
			break;

		case 6:	// dynamic pptp
			if(!get_pppoe_pptp_l2tp_settings(1))
				return 0;
			wan_settings.primary_dns = nvram_safe_get("wan_primary_dns");
			wan_settings.secondary_dns = nvram_safe_get("wan_secondary_dns");
                        wan_settings.mtu= nvram_safe_get("wan_pptp_mtu");						
			break;

		case 7:	// static l2tp
			if(!get_pppoe_pptp_l2tp_settings(2))
				return 0;
			wan_settings.primary_dns = nvram_safe_get("wan_primary_dns");
			wan_settings.secondary_dns = nvram_safe_get("wan_secondary_dns");
                        wan_settings.mtu= nvram_safe_get("wan_l2tp_mtu");					
			break;

		case 8:	// dynamic l2tp
			if(!get_pppoe_pptp_l2tp_settings(2))
				return 0;
			wan_settings.primary_dns = nvram_safe_get("wan_primary_dns");
			wan_settings.secondary_dns = nvram_safe_get("wan_secondary_dns");
                        wan_settings.mtu= nvram_safe_get("wan_l2tp_mtu");					
			break;	

		case 9:	// bigpond
			wan_settings.name = nvram_safe_get("wan_bigpond_username");
			wan_settings.password = nvram_safe_get("wan_bigpond_password");
			wan_settings.idle_time = nvram_safe_get("wan_bigpond_max_idle_time");
			wan_settings.service = nvram_safe_get("wan_bigpond_server");
			wan_settings.mode =nvram_safe_get("wan_bigpond_auto_connect");
			wan_settings.ip = "";
			wan_settings.netmask = "";
			wan_settings.gateway = "";
			wan_settings.primary_dns = nvram_safe_get("wan_primary_dns");
			wan_settings.secondary_dns = nvram_safe_get("wan_secondary_dns");
                        wan_settings.mtu= nvram_safe_get("wan_mtu");			
			break;	
/*
*   Date: 2010-05-25
*   Name: Marine Chiu
*   Reason: to pass HNAP TestDevice 10.2.8130
*   Notice : for pass some wan type DUT don't support, such as case[169] Dymanic1483Bridged.
*/
		case 10:// others type
			wan_settings.name = nvram_safe_get("wan_others_username");
			wan_settings.password = nvram_safe_get("wan_others_password");
			wan_settings.idle_time = nvram_safe_get("wan_others_max_idle_time");
			wan_settings.service = nvram_safe_get("wan_others_service");
			wan_settings.mode =nvram_safe_get("wan_others_connect_mode");
			wan_settings.ip = nvram_safe_get("wan_others_ipaddr");
			wan_settings.netmask = nvram_safe_get("wan_others_netmask");
			wan_settings.gateway = nvram_safe_get("wan_others_gateway");					
			wan_settings.primary_dns = nvram_safe_get("wan_primary_dns");
			wan_settings.secondary_dns = nvram_safe_get("wan_secondary_dns");
                        wan_settings.mtu= nvram_safe_get("wan_others_mtu");		
			break;				
	}
	return 1;
}

static int check_wan_settings(int type)
{
	/* condition protection , may add more conditions */	
	switch(type){  
	 
		case 1:  // static debug 
			if( strcmp(wan_settings.name,"") || strcmp(wan_settings.password,"") || 
				strcmp(wan_settings.idle_time, "0") || strcasecmp(wan_settings.mode,"false") || 
				atoi(wan_settings.mtu) < 68 || atoi(wan_settings.mtu) > 1500 || 
	  			strcmp(wan_settings.ip,"") == 0 || address_check(wan_settings.ip,1) == 0 || 
	  			address_check(wan_settings.netmask,2) == 0 || address_check(wan_settings.gateway,1) == 0 || 
	  			mac_check(wan_settings.mac,1) == 0 ) {	

				if(atoi(wan_settings.mtu) < 68 || atoi(wan_settings.mtu) > 1500) {
		  		        strcpy(wan_settings.mtu,"1500");
        	                        return 1;
				}  		
	
	  			return 0;
			}
			
			break;

		case 2: // static pppoe debug
			if( strcmp(wan_settings.name,"") == 0 || strcmp(wan_settings.password,"") == 0 || 
				atoi(wan_settings.mtu) < 68 || atoi(wan_settings.mtu) > 1492 || atoi(wan_settings.idle_time) < 0 || 
				strcmp(wan_settings.ip,"") == 0 || strcmp(wan_settings.netmask,"") != 0 || strcmp(wan_settings.gateway,"") != 0 || 
				(atoi(wan_settings.idle_time) == 180 && strcasecmp(wan_settings.mode,"false") ) || 
				address_check(wan_settings.ip,1) == 0 ) {

				if(atoi(wan_settings.mtu) < 68 || atoi(wan_settings.mtu) > 1492) {
					strcpy(wan_settings.mtu,"1492");
					return 1;
				}
				return 0;
			}
			
			break;

		case 3: // dynamic pppoe debug
			if( strcmp(wan_settings.name,"") == 0 || strcmp(wan_settings.password,"") == 0 || 
				atoi(wan_settings.mtu) < 68 || atoi(wan_settings.mtu) > 1492 || 
				(strcmp(wan_settings.primary_dns,"") == 0 && strlen(wan_settings.secondary_dns) != 0 ) || 
				atoi(wan_settings.idle_time) < 0 || strcmp(wan_settings.ip,"") || 
				strcmp(wan_settings.netmask,"") ||strcmp(wan_settings.gateway,"") || 
				(atoi(wan_settings.idle_time) == 180 && strcasecmp(wan_settings.mode,"false")) ) {
			
				if(atoi(wan_settings.mtu) < 68 || atoi(wan_settings.mtu) > 1492) {
					strcpy(wan_settings.mtu,"1492");
					return 1;
                         	}
				return 0;
			}  		  	
	  		
			break;

		case 4:	// dhcp debug 
			if( strcmp(wan_settings.name,"") || strcmp(wan_settings.password,"") || 
				strcmp(wan_settings.idle_time, "0") || strcasecmp(wan_settings.mode, "false") || 
				atoi(wan_settings.mtu) < 50 || atoi(wan_settings.mtu) > 1500 || 
				(strcmp(wan_settings.primary_dns,"") == 0 && strlen(wan_settings.secondary_dns) != 0 ) || 
				strcmp(wan_settings.ip,"") || strcmp(wan_settings.netmask,"") || strcmp(wan_settings.gateway,"") || 
				mac_check(wan_settings.mac,1) == 0 ) {
			
				if(atoi(wan_settings.mtu) < 50 || atoi(wan_settings.mtu) > 1500) {
		  		        strcpy(wan_settings.mtu,"1500");
					return 1;
				}
				return 0;
	  		}
	  	
		  	break;

		case 5: // static pptp debug 
			if( strcmp(wan_settings.ip,"") == 0 || atoi(wan_settings.mtu) < 68 || atoi(wan_settings.mtu) > 1464 || 
				atoi(wan_settings.idle_time) < 0 || (atoi(wan_settings.idle_time) == 180 && 
				strcasecmp(wan_settings.mode,"false") ) || strcmp(wan_settings.name,"") == 0 || 
				strcmp(wan_settings.password,"") == 0 || address_check(wan_settings.ip,1) == 0 || 
				address_check(wan_settings.gateway,1) == 0 || address_check(wan_settings.netmask,2) == 0 || 
				address_check(wan_settings.service,1) == 0 || strcmp(wan_settings.service,"") == 0 ) {

				if(atoi(wan_settings.mtu) < 68 || atoi(wan_settings.mtu) > 1464) {
						
					strcpy(wan_settings.mtu,"1464");
					return 1;
                         	} 
	  			return 0;
	  		}	
  				 				
	 		break;

		case 6:	// dynamic pptp debug 
			if((strcmp(wan_settings.primary_dns,"") == 0 && strlen(wan_settings.secondary_dns) != 0 ) || 
				strcmp(wan_settings.ip,"") != 0 || strcmp(wan_settings.netmask,"") != 0 || strcmp(wan_settings.gateway,"") != 0 || 
				atoi(wan_settings.mtu) < 68 || atoi(wan_settings.mtu) > 1464 || atoi(wan_settings.idle_time) < 0 || 
				(atoi(wan_settings.idle_time) == 180 && strcasecmp(wan_settings.mode,"false") != 0 ) || 
				strcmp(wan_settings.name,"") == 0 || strcmp(wan_settings.password,"") == 0 || 
				address_check(wan_settings.service,1) == 0 || strcmp(wan_settings.service,"") == 0 ) {

					if(atoi(wan_settings.mtu) < 68 || atoi(wan_settings.mtu) > 1464) {
		  		        	strcpy(wan_settings.mtu,"1464");
	        	                        return 1;
        	        	         } 
					return 0;
	  			}	
  			
	  			break;

		case 7: // static l2tp debug
			if( strcmp(wan_settings.ip,"") == 0 || atoi(wan_settings.mtu) < 68 || atoi(wan_settings.mtu) > 1464 || 
				atoi(wan_settings.idle_time) < 0 || (atoi(wan_settings.idle_time) == 180 && 
				strcasecmp(wan_settings.mode,"false") != 0 ) || strcmp(wan_settings.name,"") == 0 || 
				strcmp(wan_settings.password,"") == 0 || address_check(wan_settings.ip,1) == 0 || 
				address_check(wan_settings.gateway,1) == 0 || address_check(wan_settings.netmask,2) == 0 || 
				address_check(wan_settings.service,1) == 0 || strcmp(wan_settings.service,"") == 0 ) {

				if(atoi(wan_settings.mtu) < 68 || atoi(wan_settings.mtu) > 1464) {
					strcpy(wan_settings.mtu,"1464");
					return 1;
				} 
				return 0;
			}

			break;

		case 8:	// dynamic l2tp debug 
			if( (strcmp(wan_settings.primary_dns,"") == 0 && strlen(wan_settings.secondary_dns) != 0 ) || 
				strcmp(wan_settings.ip,"") != 0 || strcmp(wan_settings.netmask,"") != 0 || 
				strcmp(wan_settings.gateway,"") != 0 || atoi(wan_settings.mtu) < 68 || 
				atoi(wan_settings.mtu) > 1464 || atoi(wan_settings.idle_time) < 0 || 
				(atoi(wan_settings.idle_time) == 180 && strcasecmp(wan_settings.mode,"false") != 0 ) || 
				strcmp(wan_settings.name,"") == 0 || strcmp(wan_settings.password,"") == 0 || 
				address_check(wan_settings.service,1) == 0 || strcmp(wan_settings.service,"") == 0 ) {

				if(atoi(wan_settings.mtu) < 68 || atoi(wan_settings.mtu) > 1464) {
					strcpy(wan_settings.mtu,"1464");
					return 1;
				}
				return 0;
			}

			break;
		
		case 9:	// bigpond debug
			if(atoi(wan_settings.mtu) < 68 || atoi(wan_settings.mtu) > 1500) {
				strcpy(wan_settings.mtu,"1500");
				return 1;
			} 

			break;

		case 10:
			if(atoi(wan_settings.mtu) < 68 || atoi(wan_settings.mtu) > 9180) {
				strcpy(wan_settings.mtu,"9180");
				return 1;
			}
 
			break;
		
		default:
			break;
	}			
	return 1;
}

static int set_wan_settings(int type,char *mtu)
{	
	switch(type){
		case 0:
			return 0;

		case 1:		// static
			cprintf("set_wan_settings: static\n");																		
			nvram_set("wan_static_ipaddr",wan_settings.ip);
			nvram_set("wan_static_netmask",wan_settings.netmask);
			nvram_set("wan_static_gateway",wan_settings.gateway);
			nvram_set("wan_mtu",mtu);	
			break;

		case 2:		// static pppoe	
			cprintf("set_wan_settings: static pppoe\n");							 									
			if(!set_pppoe_pptp_l2tp_settings(0))
				return 0;
			nvram_set("wan_pppoe_mtu",mtu);					
			nvram_set("wan_pppoe_username_00",wan_settings.name);
			nvram_set("wan_pppoe_password_00",wan_settings.password);
			nvram_set("wan_pppoe_max_idle_time_00",wan_settings.idle_time);
			nvram_set("wan_pppoe_service_00",wan_settings.service);
			nvram_set("wan_pppoe_connect_mode_00",wan_settings.mode); 
			nvram_set("wan_pppoe_dynamic_00","0");
			nvram_set("wan_pppoe_ipaddr_00",wan_settings.ip);
			nvram_set("wan_pppoe_netmask_00",wan_settings.netmask);
			nvram_set("wan_pppoe_gateway_00",wan_settings.gateway);
			break;

		case 3:		// dynamic pppoe
			cprintf("set_wan_settings: dynamic pppoe\n");															
			if(!set_pppoe_pptp_l2tp_settings(0))
				return 0;

			nvram_set("wan_pppoe_mtu",mtu);								
			break;
	
		case 4:		// dhcpc	
			cprintf("set_wan_settings: dhcpc\n");
			nvram_set("wan_mtu",mtu);
			break;

		case 5:		// static pptp	
			cprintf("static pptp\n");			
			if(!set_pppoe_pptp_l2tp_settings(1))
				return 0;
			nvram_set("wan_pptp_mtu",mtu);
			break;

		case 6:		// dynamic pptp
			cprintf("dynamic pptp\n");	
			if(!set_pppoe_pptp_l2tp_settings(1))
				return 0;
			nvram_set("wan_pptp_mtu",mtu);				
			break;

		case 7:		// static l2tp
			cprintf("static l2tp\n");
			if(!set_pppoe_pptp_l2tp_settings(2))
				return 0;
			nvram_set("wan_l2tp_mtu",mtu);				
			break;	

		case 8:		// dynamic l2tp
			cprintf("dynamic l2tp\n");
			if(!set_pppoe_pptp_l2tp_settings(2))
				return 0;

			nvram_set("wan_l2tp_mtu",mtu);						
			break;								

		case 9:
			cprintf("bigpond\n");
			nvram_set("wan_bigpond_username",wan_settings.name);
			nvram_set("wan_bigpond_password",wan_settings.password);			
			nvram_set("wan_bigpond_server",wan_settings.service);	
			nvram_set("wan_bigpond_max_idle_time", wan_settings.idle_time);	
			nvram_set("wan_bigpond_auto_connect",wan_settings.mode );
                        nvram_set("wan_mtu",mtu);
			break;

		case 10:
			cprintf("Others type\n");
			nvram_set("wan_others_username",wan_settings.name);
			nvram_set("wan_others_password",wan_settings.password);			
			nvram_set("wan_others_max_idle_time", wan_settings.idle_time);	
			nvram_set("wan_others_auto_connect",wan_settings.mode );
			nvram_set("wan_others_service",wan_settings.service);
			nvram_set("wan_others_connect_mode",wan_settings.mode); 
                        nvram_set("wan_others_mtu",mtu);
			nvram_set("wan_others_ipaddr",wan_settings.ip);
			nvram_set("wan_others_netmask",wan_settings.netmask);
			nvram_set("wan_others_gateway",wan_settings.gateway);
			break;

                default:
                        break;
	}
	return 1;
}

static int do_get_wan_settings_to_xml(char *dest_xml, char *gereral_xml)
{	
	char *wan_protocol = NULL;
	char *parse_result = "ERROR";
	int type = 0;
	

#ifdef OPENDNS
	if(strlen(nvram_safe_get("opendns_enable")) != 0) {
		if(nvram_match("opendns_enable","1") == 0)
			wan_settings.opendns = "true";
		else
			wan_settings.opendns = "false";
	}
	else
		wan_settings.opendns = "false";
#endif

	/* get wan protocol type */
	wan_protocol = nvram_safe_get("wan_proto");
	
	if(strcasecmp(wan_protocol,"static") == 0) {
		wan_settings.protocol = "Static";
		type = 1;
	
	} else if(strcasecmp(wan_protocol,"pppoe") == 0) {
		if(strcasecmp(nvram_safe_get("wan_pppoe_dynamic_00"),"0") == 0) {
			wan_settings.protocol = "StaticPPPoE";
			type = 2;
		
		} else if(strcasecmp(nvram_safe_get("wan_pppoe_dynamic_00"),"1") == 0) {
			wan_settings.protocol = "DHCPPPPoE";
			type = 3;

		}

	} else if(strcasecmp(wan_protocol,"dhcpc") == 0) {
		wan_settings.protocol = "DHCP";
		type = 4;

	} else if(strcasecmp(wan_protocol,"pptp") == 0) {			
		if(strcasecmp(nvram_safe_get("wan_pptp_dynamic"),"0") == 0) {
			wan_settings.protocol = "StaticPPTP";
			type = 5;

		} else if(strcasecmp(nvram_safe_get("wan_pptp_dynamic"),"1") == 0) {
			wan_settings.protocol = "DynamicPPTP";
			type = 6;
		}

	} else if(strcasecmp(wan_protocol,"l2tp") == 0){			
		if(strcasecmp(nvram_safe_get("wan_l2tp_dynamic"),"0") == 0){
			wan_settings.protocol = "StaticL2TP";
			type = 7;
	
		}else if(strcasecmp(nvram_safe_get("wan_l2tp_dynamic"),"1") == 0){
			wan_settings.protocol = "DynamicL2TP";
			type = 8;
		}

	} else if(strcasecmp(wan_protocol,"bigpond") == 0){
		wan_settings.protocol = "BigPond";
		type = 9;

	} else{
		/*Notice : for pass some wan type DUT don't support, such as case[169] Dymanic1483Bridged.*/
		wan_settings.protocol=wan_protocol;
		type=10;
   	}
		//return 0;

	/* get wan settings from nvram to by wan_settings struction */
	if(get_wan_settings(type)){		
		parse_result = "OK";
	}
	
	/* set value to xml */
	sprintf(dest_xml,gereral_xml,parse_result,
		wan_settings.protocol,wan_settings.name,wan_settings.password,
		wan_settings.idle_time,wan_settings.service,wan_settings.mode,
		wan_settings.ip,wan_settings.netmask,wan_settings.gateway,
		wan_settings.primary_dns, wan_settings.secondary_dns
#ifdef OPENDNS
                ,wan_settings.opendns
#endif
		,nvram_safe_get("wan_mac")
		,wan_settings.mtu);
	
	
	return 1;
}

static int do_set_wan_setting_to_xml(char *source_xml)
{
	int type = 0;
	char wan_protocol[25] = {}; 
	char username[POE_NAME_LEN+1] = {}; 
	char password[POE_PASS_LEN+1] = {};
	char max_idle_time[12] = {};   
	char service_name[POE_NAME_LEN+1] = {};   
	char auto_reconnect[6] = {};   
	char ip[16] = {};   
	char mask[16] = {};   
	char gateway[16] = {};   
	char dns1[16] = {};   
	char dns2[16] = {};   
	char mac[18] = {};   
	char mtu[5] = {};           
#ifdef OPENDNS
        char open_dns[8] = {};
#endif	
	
	/* get from xml */
	do_get_element_value(source_xml, wan_protocol, "Type");
	do_get_element_value(source_xml, username, "Username");
	do_get_element_value(source_xml, password, "Password");
	do_get_element_value(source_xml, max_idle_time, "MaxIdleTime");
	do_get_element_value(source_xml, service_name, "ServiceName");
	do_get_element_value(source_xml, auto_reconnect, "AutoReconnect");
	do_get_element_value(source_xml, ip, "IPAddress");
	do_get_element_value(source_xml, mask, "SubnetMask");
	do_get_element_value(source_xml, gateway, "Gateway");
	do_get_element_value(source_xml, dns1, "Primary");
	do_get_element_value(source_xml, dns2, "Secondary");
	do_get_element_value(source_xml, mac, "MacAddress");
	do_get_element_value(source_xml, mtu, "MTU");
#ifdef OPENDNS
/*      ex. "<OpenDNS>"
 *           "<enable>%s</enable>"
 *           "</OpenDNS>"
 */
        do_get_element_value(source_xml, open_dns, "enable");
#endif

	
	/* set to wan settings struct */		
	wan_settings.name = username;
	wan_settings.password = password;
	wan_settings.idle_time = max_idle_time;
	wan_settings.service = service_name;
	wan_settings.mode = auto_reconnect;
	wan_settings.ip = ip;
	wan_settings.netmask = mask;
	wan_settings.gateway = gateway;
	wan_settings.primary_dns = dns1;
	wan_settings.secondary_dns = dns2;
	wan_settings.mac = mac;
	wan_settings.mtu = mtu;
	
#ifdef OPENDNS
        wan_settings.opendns = open_dns;
#endif

	
	/* get wan protocol type */
	if(strcasecmp(wan_protocol,"Static") == 0){
		wan_settings.protocol = "static";
		type = 1;

	} else if(strcasecmp(wan_protocol,"StaticPPPoE") == 0){
		wan_settings.protocol = "pppoe";
		wan_settings.dynamic = "0";
		type = 2;

	} else if(strcasecmp(wan_protocol,"DHCPPPPoE") == 0){		
		wan_settings.protocol = "pppoe";
		wan_settings.dynamic = "1";
		type = 3;

	} else if(strcasecmp(wan_protocol,"DHCP") == 0){
		wan_settings.protocol = "dhcpc";
		type = 4;

	} else if(strcasecmp(wan_protocol,"StaticPPTP") == 0){
		wan_settings.protocol = "pptp";
		wan_settings.dynamic = "0";
		type = 5;

	} else if(strcasecmp(wan_protocol,"DynamicPPTP") == 0){
		wan_settings.protocol = "pptp";
		wan_settings.dynamic = "1";
		type = 6;

	} else if(strcasecmp(wan_protocol,"StaticL2TP") == 0){
		wan_settings.protocol = "l2tp";
		wan_settings.dynamic = "0";
		type = 7;

	} else if(strcasecmp(wan_protocol,"DynamicL2TP") == 0){
		wan_settings.protocol = "l2tp";
		wan_settings.dynamic = "1";
		type = 8;

	} else if(strcasecmp(wan_protocol,"BigPond") == 0){
		wan_settings.protocol = "bigpond";
		type = 9;

	} else {
		wan_settings.protocol=wan_protocol;
		type = 10;
	}		
			
	/* check the value of xml */		
	if(!check_wan_settings(type)) {
		cprintf("parse_pure_wan_settings: check_wan_settings() error\n");
		return 0;
	}	
	
	if(!set_wan_settings(type,wan_settings.mtu)) {
		cprintf("parse_pure_wan_settings: set_wan_settings() error\n");
		return 0;			
	}				
	
	/* set common value to nvram */			
	nvram_set("wan_proto",wan_settings.protocol);	
	nvram_set("wan_mac",mac);


	/* Maybe can't delete below code */ 	
	/* if dhcp , must have default dns1 */
	if(type != 4) {
		/* ken modfiy for dns is null web UI show fail */
		if(!strcmp(dns1,""))
			nvram_set("wan_primary_dns", "0.0.0.0");
		else	
			nvram_set("wan_primary_dns", dns1);	
	}			
	else if(!strcmp(dns1,"") && !strcmp(dns2,""))
			nvram_set("wan_primary_dns",nvram_safe_get("lan_ipaddr"));

	/* ken modfiy for dns is null web UI show fail */			
	if(!strcmp(dns2,""))
		nvram_set("wan_secondary_dns", "0.0.0.0");
	else	
		nvram_set("wan_secondary_dns", dns2);	
			
	/* ken modfiy for dns can't show in status page */
	if( strcmp(dns1,"") || strcmp(dns2,"")){		
		cprintf("pure wan_specify_dns =1\n\n");
		nvram_set("wan_specify_dns","1");
	}
			
	return 1;
}


static int do_renew_wan_connection_to_xml(char *source_xml)
{
	char renew_time[6] = {};
	/* get element value from response of pure */
        do_get_element_value(source_xml, renew_time, "RenewTimeout");

	/*This MUST stay distinct from the Reboot method. 
	 *RenewWanConnection keeps all LAN DHCP information intact 
  	 *and has less impact on the device than the Reboot method would.
        */

	if(atoi(renew_time) <= 15)
		return 0;
	/*joelin Remark*/
	/*Fixed pure test Unauthenticated calls case 15 not reboot*/
	/*Fixed pure test Unauthenticated calls case 16 disallow set*/
        //_system("killall rc -SIGPIPE");
        //sleep(10);/*joelin Remark*/
        
        /* insert settings */
	return 1;

}

int do_get_wan_settings(const char *key, char *raw)
{
	int ret;
	char xml_resraw[2048];
	char xml_resraw2[1024];

	
	bzero(xml_resraw, sizeof(xml_resraw));
	do_xml_create_mime(xml_resraw);

	ret=do_get_wan_settings_to_xml(xml_resraw2, get_wan_settings_result);
        
	strcat(xml_resraw,xml_resraw2);
	return do_xml_response(xml_resraw);	
}

int do_set_wan_settings(const char *key,char *raw)
{
	char *xml_resraw[2048];
	char *xml_resraw2[1024];
	
	bzero(xml_resraw, sizeof(xml_resraw));

	do_xml_create_mime(xml_resraw);
	sprintf(xml_resraw2,set_wan_settings_result,
		do_set_wan_setting_to_xml(raw)? "OK":"ERROR");

	strcat(xml_resraw, xml_resraw2);

	return do_xml_response(xml_resraw);
}


int do_renew_wan_connection(const char *key, char *raw)
{
	char *xml_resraw[2048];
	char *xml_resraw2[1024];
	
	bzero(xml_resraw, sizeof(xml_resraw));
	do_xml_create_mime(xml_resraw);
	sprintf(xml_resraw2,renew_wan_connection_result
		,do_renew_wan_connection_to_xml(raw) ? "OK":"ERROR");

	strcat(xml_resraw,xml_resraw2);
	
	return do_xml_response(xml_resraw);
}


