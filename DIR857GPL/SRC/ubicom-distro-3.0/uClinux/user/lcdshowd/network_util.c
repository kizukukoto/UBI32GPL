#include "global.h"
#include "misc.h"
#include "network_util.h"
#include <stdint.h>
#include <stdarg.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>

#ifdef PC
#else

#include "project.h"

#endif

/*
Note:
	If we include both glibc header <net/if.h> and linux header <linux/netdevice.h>
		/usr/include/linux/if.h:119: error: redefinition of 'struct ifmap'
		/usr/include/linux/if.h:155: error: redefinition of 'struct ifreq'
		/usr/include/linux/if.h:205: error: redefinition of 'struct ifconf'
	Root cause: glibc header and linux header are ooxx...
solution 1: don't include <net/if.h> in you include <linux/netdevice.h>
solution 2:
	try add within linux header /include/linux/if.h 
	#ifndef _NET_IF_H 
		struct ifmap
		{
		...
	#endif
	...
	#ifndef _NET_IF_H 
		struct ifreq
	...
	#endif
	...
	#ifndef _NET_IF_H 
		struct ifconf
	...
	#endif
*/
#include <net/if.h>
#include <netdb.h>
#include <netinet/in.h>
//#include <linux/netdevice.h> // for struct net_device_stats


#if 0
//#define DEBUG_MSG(fmt, arg...)       printf("%s [%d]: "fmt, __FILE__, __LINE__, ##arg)
#define DEBUG_MSG(fmt, arg...)       printf(fmt, ##arg)
#else
#define DEBUG_MSG(fmt, arg...)
#endif

routing_table_t routing_table[];

char *proc_gen_fmt(char *name, int more, FILE * fh,...)
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


char *read_ipaddr(char *if_name)
{
	int sockfd;
	struct ifreq ifr;
	struct in_addr in_addr, netmask;
	int valid=1;
	static char ipaddr[64]={};

	if ((sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_RAW)) < 0)
		return 0;

	strcpy(ifr.ifr_name, if_name);
	if( ioctl(sockfd, SIOCGIFADDR, &ifr) < 0)
		valid = 0;

	in_addr.s_addr = sin_addr(&ifr.ifr_addr).s_addr;
	sprintf(ipaddr, "%s", inet_ntoa(in_addr));

	if( ioctl(sockfd, SIOCGIFNETMASK, &ifr) < 0)
		valid = 0;

	netmask.s_addr = sin_addr(&ifr.ifr_netmask).s_addr;
	sprintf(ipaddr, "%s/%s", ipaddr, inet_ntoa(netmask));

	close(sockfd);

	if(valid == 0)
		return "0.0.0.0";
	else
		return ipaddr;
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
		return "0.0.0.0";
	else 
		return ipaddr;
}

char *read_netmask_addr(char *if_name)
{
	int sockfd;
	struct ifreq ifr;
	struct in_addr netmask;
	int valid=1;
	static char netmask_ret[64]={'\0'};

	if ((sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_RAW)) < 0)
		return 0;

	strcpy(ifr.ifr_name, if_name);

	if( ioctl(sockfd, SIOCGIFNETMASK, &ifr) < 0)
		valid = 0;

	netmask.s_addr = sin_addr(&ifr.ifr_netmask).s_addr;
	sprintf(netmask_ret, "%s", inet_ntoa(netmask));

	close(sockfd);

	if(valid == 0)
		return "0.0.0.0";
	else
		return netmask_ret;
}

char *read_gatewayaddr(char *if_name)
{
	FILE *fp;
	char buff[1024], iface[16];
	char gate_addr[128], net_addr[128];
	char mask_addr[128], *fmt;
	int num, iflags, metric, refcnt, use, mss, window, irtt;
	struct in_addr gateway, dest, route, mask;
	int success=0, count =0;
	
	int i = 0, under_static_routing_area = 0;
	char *ptr = NULL,*ptr_tmp = NULL;

	int already_found_in_nvramORdhcp = 0;
	char lan_bridge[5];

	typedef struct static_routing_table{
	char dest_addr[16];
	char dest_mask[16];
	char gateway[16];
	char interface[8];
	char metric[3];
	}static_routing_table;
	
	memset(&gateway,'\0',sizeof(struct in_addr));

	static_routing_table *static_routing_lists_nvram[MAX_STATIC_ROUTING_NUMBER];//shvra.h
	
#if defined(CLASSLESS_STATIC_ROUTE_SHOW_UI) && (defined(UDHCPC_MS_CLASSLESS_STATIC_ROUTE) || defined(UDHCPC_RFC_CLASSLESS_STATIC_ROUTE))
	// definition need to be identical with the file
	// Eagle/apps/udhcp/dhcpc.c OptionMicroSoftClasslessStaticRoute()
#define MAX_STATIC_ROUTE_NUM_DHCP 25
#ifdef UDHCPC_MS_CLASSLESS_STATIC_ROUTE
#define MS_CLASSLESS_STATIC_ROUTE_ADD_SHELL "/tmp/ms_classes_static_route_add.sh"
#define MS_CLASSLESS_STATIC_ROUTE_DEL_SHELL "/tmp/ms_classes_static_route_del.sh"
#endif

#ifdef UDHCPC_RFC_CLASSLESS_STATIC_ROUTE
#define RFC_CLASSLESS_STATIC_STATIC_ROUTE_ADD_SHELL "/tmp/classes_static_route_add.sh"
#define RFC_CLASSLESS_STATIC_STATIC_ROUTE_DEL_SHELL "/tmp/classes_static_route_del.sh"
#endif

static_routing_table *static_routing_lists_dhcp[MAX_STATIC_ROUTE_NUM_DHCP];

#endif

#ifdef MPPPOE
	if(!nvram_match("wan_proto","pppoe"))
	{
		if(read_ipaddr_no_mask(if_name))
		{
			_system("ifconfig %s",if_name);
			return 	return_ppp_gateway();		
		}
		else
			return "0.0.0.0";
	}
#endif
	
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
	*		3:INTRANET:	    The table is used for Intranet
	*		4:LOCAL		:	Local loop back
	*
	*	Creator:
	*		0:System:		Learned automatically
	*		1:ADMIN:		Learned via user's operation in Web GUI			
	*/

	//read nvram routing info
	DEBUG_MSG("%s, reading nvram info from %s\n",__FUNCTION__,NVRAM_FILE);
	memset(lan_bridge,'\0',sizeof(lan_bridge));
	for(i = 0;i < MAX_STATIC_ROUTING_NUMBER; i++)
		static_routing_lists_nvram[i]=NULL;

	if((fp = fopen(NVRAM_FILE,"r"))!=NULL){
		i = 0;
		while(!feof(fp) && i < MAX_STATIC_ROUTING_NUMBER){
			memset(buff,'\0',sizeof(buff));
			fgets(buff,sizeof(buff),fp);
			if(buff[0] != '\n' && buff[0] != '\0' && buff[0] != '#'){	//ignore #!/bin.sh
				if( strncmp(buff,"static_routing_",strlen("static_routing_")) == 0){
					DEBUG_MSG("%s, %s",__FUNCTION__,buff);
					under_static_routing_area = 1;
					//static_routing_xx     enable/name/dest_addr/dest_mask/gw/interface/metric
					//static_routing_05=1/test_2/192.168.220.0/255.255.0.0/192.168.100.220/WAN/1
					ptr = strchr(buff,'/');
					if(ptr && (*(ptr-1) == '0')){//disable
						goto next_round;
					}
					DEBUG_MSG("%s, parsing %s",__FUNCTION__,buff);
					static_routing_lists_nvram[i] = malloc(sizeof(static_routing_table));
					/*	Date: 2009-02-05
					*	Name: jimmy huang
					*	Reason: in case memory alloc failed
					*	Note:	Add new codes below
					*/
					if(static_routing_lists_nvram[i] == NULL){
						printf("%s , line %d, memory alloc failed !\n",__FUNCTION__,__LINE__);
						break;
					}
					if(NULL == (ptr_tmp = strtok(ptr,"/")))
						goto next_round;
					if(NULL ==  (ptr_tmp = strtok(NULL,"/")))
						goto next_round;
					strcpy(static_routing_lists_nvram[i]->dest_addr,ptr_tmp);
					if(NULL ==  (ptr_tmp = strtok(NULL,"/")))
						goto next_round;
					strcpy(static_routing_lists_nvram[i]->dest_mask,ptr_tmp);
					if(NULL ==  (ptr_tmp = strtok(NULL,"/")))
						goto next_round;
					strcpy(static_routing_lists_nvram[i]->gateway,ptr_tmp);
					if(NULL ==  (ptr_tmp = strtok(NULL,"/")))
						goto next_round;
					strcpy(static_routing_lists_nvram[i]->interface,ptr_tmp);
					if(NULL ==  (ptr_tmp = strtok(NULL,"/")))
						goto next_round;
					strcpy(static_routing_lists_nvram[i]->metric,ptr_tmp);
					DEBUG_MSG("%s, read from nvram : dest_addr = %s, dest_mask = %s, gw = %s, interface = %s, metric = %s\n" \
						,__FUNCTION__ \
						,static_routing_lists_nvram[i]->dest_addr \
						,static_routing_lists_nvram[i]->dest_mask \
						,static_routing_lists_nvram[i]->gateway \
						,static_routing_lists_nvram[i]->interface \
						,static_routing_lists_nvram[i]->metric \
					);
next_round:
					if(static_routing_lists_nvram[i] != NULL)
						i++;
				}
				else
				{
					if(strncmp(buff,"lan_bridge",strlen("lan_bridge")) == 0){
						ptr=strchr(buff,'=');
						if(ptr){
							ptr++;
							strcpy(lan_bridge,ptr);
							if(lan_bridge[strlen(lan_bridge)-1] == '\n'){
								lan_bridge[strlen(lan_bridge)-1]='\0';
							}
						}
					}
					if(under_static_routing_area)
						break;
				}
			}
		}
		fclose(fp);
	}
	//read dhcp routing info
#if defined(CLASSLESS_STATIC_ROUTE_SHOW_UI) && (defined(UDHCPC_MS_CLASSLESS_STATIC_ROUTE) || defined(UDHCPC_RFC_CLASSLESS_STATIC_ROUTE))
	for(i = 0;i < MAX_STATIC_ROUTE_NUM_DHCP; i++)
		static_routing_lists_dhcp[i]=NULL;
	i = 0;
#ifdef UDHCPC_MS_CLASSLESS_STATIC_ROUTE
	if((fp = fopen(MS_CLASSLESS_STATIC_ROUTE_ADD_SHELL,"r")) != NULL){
#else
	if((fp = fopen(RFC_CLASSLESS_STATIC_STATIC_ROUTE_ADD_SHELL,"r")) != NULL){
#endif
		while(!feof(fp)){
			memset(buff,'\0',sizeof(buff));
			fgets(buff,sizeof(buff),fp);
			if(buff[0] != '\n' && buff[0] != '\0' && buff[0] != '#'){	//ignore #!/bin.sh
				static_routing_lists_dhcp[i] = malloc(sizeof(static_routing_table));
				if(static_routing_lists_dhcp[i] == NULL){
					printf("%s memory alloc failed !\n",__FUNCTION__);
					break;
				}
					//route add -net 192.168.100.100 netmask 255.255.255.255 gw 192.168.0.10 metric 1
				sscanf(buff,"%s %s %s %s %s %s %s %s %s %s"
						,useless,useless,useless,static_routing_lists_dhcp[i]->dest_addr
						,useless,static_routing_lists_dhcp[i]->dest_mask,useless
						,static_routing_lists_dhcp[i]->gateway,useless
						,static_routing_lists_dhcp[i]->metric);
				DEBUG_MSG("%s, read from dhcp : dest_addr = %s, dest_mask = %s, gw = %s, metric = %s\n" \
						,__FUNCTION__,static_routing_lists_dhcp[i]->dest_addr \
						,static_routing_lists_dhcp[i]->dest_mask \
						,static_routing_lists_dhcp[i]->gateway \
						,static_routing_lists_dhcp[i]->metric);
				i++;
			}
		}
		fclose(fp);
	}else{
		DEBUG_MSG("%s, dhcp option 121/249 shell file not exists\n",__FUNCTION__);
	}
#endif	

	fp = fopen (PATH_PROCNET_ROUTE, "r");
	if(!fp) {
		perror(PATH_PROCNET_ROUTE);
		return 0;
	}

	fmt = proc_gen_fmt(PATH_PROCNET_ROUTE, 0, fp,
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
	/* "%16s %128s %128s %X %d %d %d %128s %d %d %d\n" */

	if (!fmt)
		return 0;

	/* jimmy marked 20080505 */
	//printf("sizeof routing_table: %d\n", sizeof(&routing_table));
	/* --------------------- */
	for(count =0;  strlen(routing_table[count].dest_addr) >0; count++ ) {
		strcpy(routing_table[count].dest_addr, "") ;
	}
	count =0;
	
	while( fgets(buff, 1024, fp)) {
		num = sscanf(buff, fmt, iface, net_addr, gate_addr, &iflags, &refcnt, &use, &metric, mask_addr, &mss, &window, &irtt);

		if(iflags & RTF_UP) {
			dest.s_addr = strtoul(net_addr, NULL, 16);
			route.s_addr = strtoul(gate_addr, NULL, 16); 
			mask.s_addr = strtoul(mask_addr, NULL, 16); 

			/* for route_table */
			strcpy(routing_table[count].dest_addr, inet_ntoa(dest));
			strcpy(routing_table[count].dest_mask, inet_ntoa(mask));
			strcpy(routing_table[count].gateway, inet_ntoa(route));
			strcpy(routing_table[count].interface, iface);
			routing_table[count].metric = metric;
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
			if(strncmp(routing_table[count].interface,lan_bridge,strlen(lan_bridge)) == 0)//br0
				routing_table[count].type = 3;
			else if(strcmp(routing_table[count].interface,"lo") == 0)
				routing_table[count].type = 4;
			else
				routing_table[count].type = 0;
			routing_table[count].creator = 0;
			already_found_in_nvramORdhcp = 0;
			//DEBUG_MSG("%s, reading in dest_addr = %s, dest_mask = %s, gw = %s, interface = %s, type = %d(%s), creator = %d(%s), metric = %d\n"
			//	,__FUNCTION__
			//	,routing_table[count].dest_addr
			//	,routing_table[count].dest_mask
			//	,routing_table[count].gateway
			//	,routing_table[count].interface
			//	,routing_table[count].type
			//	,(routing_table[count].type == 0) ? "Internet" : ((routing_table[count].type == 1) ? "DHCP Option" : "STATIC")
			//	,routing_table[count].creator
			//	,routing_table[count].creator ? "ADMIN" : "System"
			//	,routing_table[count].metric
			//	);

			// check if this route is added via rfc 3442 (option 121/249)
#if defined(CLASSLESS_STATIC_ROUTE_SHOW_UI) && (defined(UDHCPC_MS_CLASSLESS_STATIC_ROUTE) || defined(UDHCPC_RFC_CLASSLESS_STATIC_ROUTE))
			if((already_found_in_nvramORdhcp != 1) && (static_routing_lists_dhcp[0] != NULL)){
				/*
				for(i=0; static_routing_lists_dhcp[i] != NULL && strlen(static_routing_lists_dhcp[i]->dest_addr) > 0; i++ ){
				*/
				/*	Date: 2009-02-04
				*	Name: jimmy huang
				*	Reason: original condition rule maybe cause unstable memory access
				*			and will let httpd crash
				*	Note:	Modified the codes above to new belows
				*/
				for(i=0; i < MAX_STATIC_ROUTING_NUMBER && static_routing_lists_dhcp[i] != NULL && strlen(static_routing_lists_dhcp[i]->dest_addr) > 0; i++ ){
					if((strcmp(routing_table[count].dest_addr,static_routing_lists_dhcp[i]->dest_addr) == 0)
						&& (strcmp(routing_table[count].dest_mask,static_routing_lists_dhcp[i]->dest_mask) == 0)
						&& (strcmp(routing_table[count].gateway,static_routing_lists_dhcp[i]->gateway) == 0))
					{
						routing_table[count].type = 1;//DHCP Option
						routing_table[count].creator = 0; //System
						already_found_in_nvramORdhcp = 1;
						break;
					}
				}
			}
#endif

			// check if this route is added by user via Web function "static route"
			if((already_found_in_nvramORdhcp != 1) && (static_routing_lists_nvram[0] != NULL)){
				/*
				for(i=0; static_routing_lists_nvram[i] != NULL && strlen(static_routing_lists_nvram[i]->dest_addr) > 0; i++ ){
				*/
				/*	Date: 2009-02-04
				*	Name: jimmy huang
				*	Reason: original condition rule maybe cause unstable memory access
				*			and will let httpd crash
				*	Note:	Modified the codes above to new belows
				*/
				for(i=0; i < MAX_STATIC_ROUTING_NUMBER && static_routing_lists_nvram[i] != NULL && strlen(static_routing_lists_nvram[i]->dest_addr) > 0; i++ ){
					if((strcmp(routing_table[count].dest_addr,static_routing_lists_nvram[i]->dest_addr) == 0)
						&& (strcmp(routing_table[count].dest_mask,static_routing_lists_nvram[i]->dest_mask) == 0)
						&& (strcmp(routing_table[count].gateway,static_routing_lists_nvram[i]->gateway) == 0)
						)
					{
						routing_table[count].type = 2;//STATIC
						routing_table[count].creator = 1;//Admin
						already_found_in_nvramORdhcp = 1;
						break;
					}
				}
			}

			DEBUG_MSG("%s, table[%d] dest_addr = %s, dest_mask = %s, gw = %s, interface = %s, type = %d (%s), creator = %d (%s), metric = %d\n"
				,__FUNCTION__,count
				,routing_table[count].dest_addr
				,routing_table[count].dest_mask
				,routing_table[count].gateway
				,routing_table[count].interface
				,routing_table[count].type
				,(routing_table[count].type == 0) ? "Internet" : ((routing_table[count].type == 1) ? "DHCP Option" : ((routing_table[count].type == 2) ? "STATIC" : "INTRANET"))
				,routing_table[count].creator
				,routing_table[count].creator ? "ADMIN" : "System"
				,routing_table[count].metric
				);

			
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
#if defined(CLASSLESS_STATIC_ROUTE_SHOW_UI) && (defined(UDHCPC_MS_CLASSLESS_STATIC_ROUTE) || defined(UDHCPC_RFC_CLASSLESS_STATIC_ROUTE))
	for(i = 0;i < MAX_STATIC_ROUTE_NUM_DHCP; i++){
		if(static_routing_lists_dhcp[i] != NULL)
			free(static_routing_lists_dhcp[i]);
	}
#endif

	if(success)
		return inet_ntoa(gateway); 
	else 
		return "0.0.0.0";
}



#ifdef PC
char *wan_statue(const char *proto){
	return "Disconnect";
}
#else
char *wan_statue(const char *proto)
{
	char pid_file[40]="";
	char mpppoe_mode[40]="";
	char eth[5], ifnum='0';
	char status[15];
	int demand = 0, alwayson = 0;
	struct stat filest;
	char *ConnStatue[] = {"Connected", "Connecting", "Disconnected"};
	enum {Connected, Connecting, Disconnected};
	char *wan_eth = NULL;
	char *ip_addr = NULL;

#ifdef CONFIG_USER_3G_USB_CLIENT
	if(strcmp(proto, "usb3g")==0)
	{
		struct stat usbVendorID;
		/* Check USB Port connection */
		if(stat(PPP_3G_VID_PATH, &usbVendorID) != 0)
			return ConnStatue[Disconnected];
	}
	else
	{
#endif //CONFIG_USER_3G_USB_CLIENT

	wan_eth = nvram_safe_get("wan_eth");

#ifdef CONFIG_BRIDGE_MODE
	if(!nvram_match("wlan0_mode","rt"))
#endif
	{
		/* Check WAN eth phy connect */
		VCTGetPortConnectState( wan_eth, VCTWANPORT0, status);
		if( !strncmp("disconnect", status, 10) )
			return ConnStatue[Disconnected];
	}
#ifdef CONFIG_USER_3G_USB_CLIENT
	}
#endif //CONFIG_USER_3G_USB_CLIENT

	if( strcmp(proto, "dhcpc") ==0 || strcmp(proto, "static") ==0)
	{
#ifdef CONFIG_BRIDGE_MODE
		if(!nvram_match("wlan0_mode","ap"))
			strcpy(eth, nvram_safe_get("lan_bridge"));
		else
#endif
			strcpy(eth, wan_eth);
	}
	else
    {
#ifdef MPPPOE
	    ifnum = *(proto + 7);
        sprintf(eth, "ppp%c", ifnum);
#else
        strcpy(eth, "ppp0");
#endif
    }

    ip_addr = read_ipaddr(eth);

    if(ip_addr)
    {
		if( strncmp(proto, "pppoe", 5)==0
#ifdef CONFIG_USER_3G_USB_CLIENT
		 || strncmp(proto, "usb3g", 5)==0
#endif
		)
        {
			if(!strncmp(ip_addr, "10.64.64.64", 11))
				return ConnStatue[Disconnected];
        }
	    if( strcmp(read_ipaddr_no_mask(eth),"0.0.0.0") !=0)
		    return ConnStatue[Connected];
    }

	if(strncmp(proto, "pppoe", 5) ==0) {
		sprintf( pid_file, "/var/run/ppp-0%c.pid", ifnum );
		sprintf( mpppoe_mode, "wan_pppoe_connect_mode_0%c", ifnum);
		if(nvram_match(mpppoe_mode,"on_demand")==0)
			demand = 1;
		else if(nvram_match(mpppoe_mode,"always_on")==0)
			alwayson = 1;
	} else if(strcmp(proto, "pptp") ==0 ) {
		strcpy(pid_file, "/var/run/ppp-pptp.pid");
		if( nvram_match("wan_pptp_connect_mode", "on_demand")==0 )
			demand = 1;
		else if(nvram_match("wan_pptp_connect_mode", "always_on")==0)
			alwayson = 1;
	} else if(strcmp(proto, "l2tp") ==0){
		strcpy(pid_file, "/var/run/l2tp.pid");
		if( nvram_match("wan_l2tp_connect_mode", "on_demand")==0 )
			demand = 1;
		else if( nvram_match("wan_l2tp_connect_mode", "always_on")==0 )
			alwayson = 1;
	}
#ifdef CONFIG_USER_3G_USB_CLIENT
	else if(strncmp(proto, "usb3g", 5) ==0) {
		sprintf(pid_file, "/var/run/ppp0.pid");
		if(nvram_match("usb3g_reconnect_mode","on_demand")==0)
			demand = 1;
		else if(nvram_match("usb3g_reconnect_mode","always_on")==0)
			alwayson = 1;
	}
#endif

	if( stat(pid_file, &filest) == 0 || alwayson == 1)
		return ConnStatue[Connecting];

	return ConnStatue[Disconnected];
}
#endif // end ifdef PC


/* get the wan port connection time */
unsigned long get_wan_connect_time(char *wan_proto, int check_wan_status){
	FILE *fp = NULL;
	char tmp[10] = {0};
	unsigned long connect_time  = 0;
	unsigned long sys_up_time = 0;
	char wan_status[16]={'\0'};
	sys_up_time = uptime();
	
	DEBUG_MSG("lcdshowd: %s (%d), sys_up_time = %ld \n",__func__,__LINE__,sys_up_time);
	
	if(check_wan_status)
	{
		memset(wan_status, 0 , sizeof(wan_status));
		if(strcmp(wan_proto, "pppoe") == 0)
			strcpy(wan_status, wan_statue("pppoe-00"));
		else
			strcpy(wan_status, wan_statue(wan_proto));

		if(strncmp(wan_status, "Connected", 9) == 0)
		{
			DEBUG_MSG("lcdshowd: %s (%d), wan_status Connected\n",__func__,__LINE__);
			/* open read-only file to get wan connect time stamp */
			fp = fopen(WAN_CONNECT_FILE,"r+");
			if(fp == NULL)
				return 0;

	        if( fgets(tmp, sizeof(tmp), fp) )
				connect_time = atol(tmp);

			fclose(fp);

			DEBUG_MSG("lcdshowd: %s (%d), connect_time = %ld \n",__func__,__LINE__,connect_time);
			if( sys_up_time == connect_time )
				return 1;
			else{
				DEBUG_MSG("lcdshowd: %s (%d), return connect_time = %ld \n",__func__,__LINE__,connect_time ? sys_up_time - connect_time : 0);
				return connect_time ? sys_up_time - connect_time : 0;
			}
        }
		else{
			DEBUG_MSG("lcdshowd: wan_status disconnected, wan up connection time 0\n");
			return 0;
		}
	}
	else
	{
		/* open read-only file to get wan connect time stamp */
		fp = fopen(WAN_CONNECT_FILE,"r+");
		if(fp == NULL)
			return 0;

		if( fgets(tmp, sizeof(tmp), fp) )
			connect_time = atol(tmp);

		fclose(fp);

		if( sys_up_time == connect_time )
			return 1;
		else
			return connect_time ? sys_up_time - connect_time : 0;
	}
}

int get_wlan_stats(char *if_name, performance_info *info)
{
	FILE *fp;
	char buf[256];
	char cmd[128];
	char *p_rx, *p_tx, *p_tmp;
	time_t cur_time;
	long int time_drift = 0;

	unlink(PACKET_LOG_PATH);

	sprintf(cmd, "cmds wlan utility statistics %s > %s", if_name, PACKET_LOG_PATH);
	system(cmd);

	if ((fp = fopen(PACKET_LOG_PATH, "r")) == NULL)
		return -1;
	else {
			memset(buf, '\0', sizeof(buf));
			fgets(buf, sizeof(buf), fp);
//			p_rx = strstr(buf, "RxCount:");

			memset(buf, '\0', sizeof(buf));
			fgets(buf, sizeof(buf), fp);
//			p_tx = strstr(buf, "TxCount:");

			memset(buf, '\0', sizeof(buf));
			fgets(buf, sizeof(buf), fp);
			p_rx = strstr(buf, "ReceivedByteCount:");
			p_rx += strlen("ReceivedByteCount: ");
			//p_rxbytes = strstr(buf, "ReceivedByteCount:");

			memset(buf, '\0', sizeof(buf));
			fgets(buf, sizeof(buf), fp);
			p_tx = strstr(buf, "TransmittedByteCount: ");
			p_tx += strlen("TransmittedByteCount: ");
			//p_txbytes = strstr(buf, "TransmittedByteCount:");

	}

	cur_time = time(NULL);

	if(info->last_update_time == 0){
		info->last_update_time = cur_time;
		info->rx = atol(p_rx);
		info->tx = atol(p_tx);
		info->throughput_rx = 0;
		info->throughput_tx = 0;
	}else{
		time_drift = cur_time - info->last_update_time;
		if(time_drift <= 0){
			time_drift = STATISTICS_TIMEOUT;
		}
		info->throughput_rx = ( atol(p_rx) - info->rx ) / time_drift;
		info->throughput_tx = ( atol(p_tx) - info->tx ) / time_drift;
		//printf("tx: diff %ld \n",atol(ptr_rx) - info->rx);
		info->rx = atol(p_rx);
		info->tx = atol(p_tx);
		info->last_update_time = cur_time;
	}

	return 0;
}


int get_net_stats(char *if_name, performance_info *info){
	FILE *fp;
	char buf[256] = {'\0'};
	char cmd[128]={'\0'};
	char *ptr_rx = NULL;
	char *ptr_tx = NULL;
	char *ptr_tmp = NULL;
	time_t cur_time;
	long int time_drift = 0;

	unlink(PACKET_LOG_PATH);
	sprintf(cmd,"ifconfig %s |grep \"RX bytes:\" > %s", if_name, PACKET_LOG_PATH);
	system(cmd);

	if((fp=fopen(PACKET_LOG_PATH,"r"))!=NULL){
		while(!feof(fp)){
			fgets(buf,sizeof(buf),fp);
			if(!feof(fp)){
/*
eth0.1    Link encap:Ethernet  HWaddr 00:01:23:45:67:8A  
          inet addr:172.21.33.143  Bcast:172.21.47.255  Mask:255.255.240.0
          UP BROADCAST RUNNING MULTICAST  MTU:1500  Metric:1
          RX packets:190735 errors:0 dropped:0 overruns:0 frame:0
          TX packets:47 errors:0 dropped:0 overruns:0 carrier:0
          collisions:0 txqueuelen:750 
          RX bytes:15473611 (14.7 MiB)  TX bytes:5998 (5.8 KiB)
*/
				ptr_rx=strstr(buf,"RX bytes:");
				ptr_tx=strstr(buf,"TX bytes:");
				*(ptr_tx - 1) = '\0';

				ptr_rx += strlen("RX bytes:");
				ptr_tmp = ptr_rx;
				while(ptr_tmp[0] != ' ' && ptr_tmp[0] != '('){
					ptr_tmp++;
				}
				*ptr_tmp = '\0';
				//printf("%s \n",ptr_rx);
				ptr_tx += strlen("TX bytes:");
				ptr_tmp = ptr_tx;
				while(ptr_tmp[0] != ' ' && ptr_tmp[0] != '('){
					ptr_tmp++;
				}
				*ptr_tmp = '\0';
				//printf("%s \n",ptr_tx);
				break;
			}
		}
		fclose(fp);
	}else{
		return -1;
	}

	cur_time = time(NULL);
	if(info->last_update_time == 0){
		info->last_update_time = cur_time;
		info->rx = atol(ptr_rx);
		info->tx = atol(ptr_tx);
		info->throughput_rx = 0;
		info->throughput_tx = 0;
	}else{
		time_drift = cur_time - info->last_update_time;
		if(time_drift <= 0){
			time_drift = STATISTICS_TIMEOUT;
		}
		info->throughput_rx = ( atol(ptr_rx) - info->rx ) / time_drift;
		info->throughput_tx = ( atol(ptr_tx) - info->tx ) / time_drift;
		//printf("tx: diff %ld \n",atol(ptr_rx) - info->rx);
		info->rx = atol(ptr_rx);
		info->tx = atol(ptr_tx);
		info->last_update_time = cur_time;
	}
	return 1;
}

void reset_performance(performance_info *info){
	info->rx = 0;
	info->tx = 0;
	info->throughput_rx = 0;
	info->throughput_tx = 0;
	info->last_update_time = 0;
}

/* copy from httpd/core.c */
void get_ath_macaddr(char *strMac)
{
#ifdef PC
	sprintf(strMac,"00:11:22:99:88:77");
	return ;
#else
	FILE *fp;
	unsigned char mac[6];
	fp = fopen ( SYS_CAL_MTD, "r" );
	fseek ( fp, ATH_MAC_OFFSET, SEEK_SET );
	fread ( mac, 1, 6, fp );
	sprintf ( strMac,"%02x:%02x:%02x:%02x:%02x:%02x\n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5] );
	fclose ( fp );
#endif
}



#ifdef PC

#else

#endif




