/* dhcpc.c
 *
 * udhcp DHCP client
 *
 * Russ Dill <Russ.Dill@asu.edu> July 2001
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <sys/time.h>
#include <sys/file.h>
#include <unistd.h>
#include <getopt.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <time.h>
#include <string.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <errno.h>
#include <signal.h>

#include <shvar.h>
#include <sutil.h>

#include "dhcpd.h"
#include "dhcpc.h"
#include "options.h"
#include "clientpacket.h"
#include "clientsocket.h"
#include "script.h"
#include "socket.h"
#include "common.h"
#include "signalpipe.h"
#include <stdarg.h>

static int state;
static int test_dhcp_state;
static unsigned long requested_ip; /* = 0 */
/* jimmy added 20080213 for remember last valid IP */
static unsigned long previous_ip = 0;
static unsigned long server_addr;
static unsigned long timeout;
static unsigned long test_dhcp_timeout;
static int packet_num; /* = 0 */
static int test_dhcp_packet_num; /* = 0 */
static int fd = -1;
static int test_dhcp_fd = -1;

/* jimmy added 20080718, for checking NetBios info changed */
#ifdef UDHCPD_NETBIOS
int netbios_changed = 0;
#endif
/* ------------------------------------- */

#define LISTEN_NONE 0
#define LISTEN_KERNEL 1
#define LISTEN_RAW 2
static int listen_mode;
static int test_dhcp_listen_mode;

#ifdef UDHCPC_UNICAST
int use_unicasting = 0;
#endif

struct client_config_t client_config = {
	/* Default options. */
	.abort_if_no_lease = 0,
	.foreground = 0,
	.quit_after_lease = 0,
	.background_if_no_lease = 0,
	.interface = "eth0",
	.pidfile = NULL,
	.script = DEFAULT_SCRIPT,
	.clientid = NULL,
	.vendorclass = NULL,
	.hostname = NULL,
	.fqdn = NULL,
	.ifindex = 0,
	.arp = "\0\0\0\0\0\0",		/* appease gcc-3.0 */
	.wan_proto = "dhcpc",
#ifdef UDHCPD_NETBIOS
	.netbios_enable = 0,
#endif
#ifdef RPPPOE
	.russia_enable = 0,
#endif
#ifdef CONFIG_BRIDGE_MODE
	.ap_mode = 0,
/*	Date: 2009-04-10
	*	Name: Ken Chiang
	*	Reason:	Add support for enable auto mode select(router mode/ap mode).
	*	Note: 
*/		
#ifdef AUTO_MODE_SELECT
	.auto_mode_select = 0,
#endif	
#endif
	/*	Date: 2009-01-07
	*	Name: jimmy huang
	*	Reason:	Add support for disable vendor class identifier
	*	Note: 
	*/
	.vendor_class_identifier_disable = 0,
	/*	Date: 2009-01-07
	*	Name: jimmy huang
	*	Reason:	Add support for enable classless static route (121/249)
	*	Note: 
	*/
#if defined(UDHCPC_MS_CLASSLESS_STATIC_ROUTE) || defined(UDHCPC_RFC_CLASSLESS_STATIC_ROUTE)
	.classless_static_route_enable = 0,
#endif
#ifdef IPV6_6RD
	.option_6rd_enable = 0, /*	support for enable 6rd DHCPv4 Option (212)	*/
#endif
};

#ifndef IN_BUSYBOX
static void __attribute__ ((noreturn)) show_usage(void)
{
	printf(
"Usage: udhcpc [OPTIONS]\n\n"
"  -c, --clientid=CLIENTID         Set client identifier - type is first char\n"
"  -C, --clientid-none             Suppress default client identifier\n"
"  -V, --vendorclass=CLASSID       Set vendor class identifier\n"
"  -H, --hostname=HOSTNAME         Client hostname\n"
"  -h                              Alias for -H\n"
"  -F, --fqdn=FQDN                 Client fully qualified domain name\n"
"  -f, --foreground                Do not fork after getting lease\n"
"  -b, --background                Fork to background if lease cannot be\n"
"                                  immediately negotiated.\n"
"  -i, --interface=INTERFACE       Interface to use (default: eth0)\n"
"  -w, --wan_proto=dhcpc		   WAN Protol of nvram setting (default: eth0)\n"
#ifdef UDHCPD_NETBIOS
"  -N, --netbios_enable            for dhcpd_netbios_enable\n"
#endif
#ifdef RPPPOE
"  -R, --russia_enable             for russia_enable\n"
#endif
#ifdef CONFIG_BRIDGE_MODE
"  -a, --ap_mode             	   for ap mode\n"
/*	Date: 2009-04-10
	*	Name: Ken Chiang
	*	Reason:	Add support for enable auto mode select(router mode/ap mode).
	*	Note: 
*/
#ifdef AUTO_MODE_SELECT
"  -A, --auto_mode_select          for auto_mode_select enable\n"
#endif
#endif
/*	Date: 2009-01-09
*	Name: jimmy huang
*	Reason:	Add support for disable vendor class identifier (60)
*			Add support for enable classless static route (121/249)
*	Note: 	Add the codes below
*/
"  --option60_off             	   for vendor_class_identifier_disable\n"
#if defined(UDHCPC_MS_CLASSLESS_STATIC_ROUTE) || defined(UDHCPC_RFC_CLASSLESS_STATIC_ROUTE)
"  --option121or249_on             	   for classless_static_route_enable\n"
#endif
#ifdef IPV6_6RD
"  --option212_on                  for option_6rd_enable\n"
#endif
"  -n, --now                       Exit with failure if lease cannot be\n"
"                                  immediately negotiated.\n"
"  -p, --pidfile=file              Store process ID of daemon in file\n"
"  -q, --quit                      Quit after obtaining lease\n"
"  -r, --request=IP                IP address to request (default: none)\n"
"  -s, --script=file               Run file at dhcp events (default:\n"
"                                  " DEFAULT_SCRIPT ")\n"
"  -v, --version                   Display version\n"
	);
	exit(0);
}
#else
#define show_usage bb_show_usage
extern void show_usage(void) __attribute__ ((noreturn));
#endif

/*	Date: 2009-01-07
*	Name: jimmy huang
*	Reason:	we don't want to use sutil library
*	Input:	pid file
*	Output:	pid number
*	Note: copy from sutil.c
*/
int read_pid(char *file)
{
	FILE *pidfile;
	char pid[20];
	pidfile = fopen(file, "r");
	if(pidfile) {
		fgets(pid,20, pidfile);
		fclose(pidfile);
	} else
		return -1;
	return atoi(pid);
}

/*	Date: 2009-01-07
*	Name: jimmy huang
*	Reason:	we don't want to use sutil library
*	Input:	parameters we want to execute
*	Output:
*	Note: copy from sutil.c
*/
int _system(const char *fmt, ...)
{
	va_list args;
	int i;
	char buf[512];

	va_start(args, fmt);
	i = vsprintf(buf, fmt,args);
	va_end(args);
	
	system(buf);
	return i;
}


/* just a little helper */
static void change_mode(int new_mode)
{
	DEBUG(LOG_DEBUG, "entering %s listen mode",
		new_mode ? (new_mode == 1 ? "kernel" : "raw") : "none");
	if (fd >= 0) close(fd);
	fd = -1;
	listen_mode = new_mode;
}


/* perform a renew */
static void perform_renew(void)
{
	LOG(LOG_DEBUG, "Performing a DHCPC renew");
	switch (state) {
	case BOUND:
		change_mode(LISTEN_KERNEL);
	case RENEWING:
	case REBINDING:
		state = RENEW_REQUESTED;
		break;
	case RENEW_REQUESTED: /* impatient are we? fine, square 1 */
		run_script(NULL, "deconfig");
	case REQUESTING:
	case RELEASED:
		change_mode(LISTEN_RAW);
		state = INIT_SELECTING;
		break;
	case INIT_SELECTING:
		break;
	}

	/* start things over */
	packet_num = 0;

	/* Kill any timeouts because the user wants this to hurry along */
	timeout = 0;
}


/* perform a release */
static void perform_release(void)
{
	char buffer[16];
	char cmd[50];
	memset(cmd,0,sizeof(cmd));

	struct in_addr temp_addr;

	LOG(LOG_DEBUG, "Performing a DHCPC release");

	/* send release packet */
	if (state == BOUND || state == RENEWING || state == REBINDING || state == RELEASED) 
	/*NickChou add state == RELEASED 07.08.17*/
	{
		temp_addr.s_addr = server_addr;
		sprintf(buffer, "%s", inet_ntoa(temp_addr));
		temp_addr.s_addr = requested_ip;
		LOG(LOG_INFO, "Unicasting a release of %s to %s",
				inet_ntoa(temp_addr), buffer);
		send_release(server_addr, requested_ip); /* unicast */
		/* run_script(NULL, "deconfig"); can't set wan ip = 0.0.0.0 */
		if(1/*strcmp(client_config.wan_proto, "dhcpc") == 0*/) 
		{	
			LOG(LOG_INFO, "DHCP Release WAN IP address = 0.0.0.0");
			sprintf(cmd,"ifconfig %s 0.0.0.0", client_config.interface);
			_system(cmd);
		} 
	}
	
	LOG(LOG_DEBUG, "Entering released state");

	change_mode(LISTEN_NONE);
	state = RELEASED;
	timeout = 0x7fffffff;
}


static void client_background(void)
{
	background(client_config.pidfile);
	client_config.foreground = 1; /* Do not fork again. */
	client_config.background_if_no_lease = 0;
}

static void save_dhcpc_lease_time(unsigned long lease_t){	
	FILE *fp = fopen("/var/tmp/dhcpc.tmp","w+");
	if(fp == NULL)
		return;
	/* save lease value for dhcp client */						
	fprintf(fp,"%lu%s",lease_t,"\n");			
	fprintf(fp,"%lu%s",uptime(),"\n");	
	fclose(fp);
}

static void save_dns_name(char *dns_name){
	char dname[34];	
	int len;
	FILE *fp = fopen("/var/tmp/dhcpc_dns.tmp","w+");
	if(fp == NULL)
		return;
	/* jimmy modified 20080505 , avoid option incorrect domain */
	//len = *(unsigned char *)dns_name;
	len = *(unsigned char *)(dns_name-1);
	/* ------------------------------------------------------- */
	
	memset(dname, 0 , 34);		
	/* jimmy modified 20080505 , avoid option incorrect domain */
	//memcpy(dname, dns_name+1, len);				
	memcpy(dname, dns_name, len);
	/* ------------------------------------------------------- */
	fprintf(fp,"%s",dname);			
	fclose(fp);
}

void dhcp_discover(void)
{

	uint8_t *temp, *message;
	unsigned long xid = 0;
	fd_set rfds;
	int retval;
	struct timeval tv;
	int len;
	struct dhcpMessage packet;
	long now;
	int max_fd;
	FILE *fp;

	fp=fopen("/var/tmp/test_dhcp_res.txt","w");
	if (fp == NULL)
	{
		printf("test_dhcp_res.txt file open failed. \n");
		return ;
	}
	
	if (read_interface(client_config.interface, &client_config.ifindex,
			   NULL, client_config.arp) < 0)
		return ;

	test_dhcp_state = INIT_SELECTING;
	if (test_dhcp_fd >= 0) 
		close(test_dhcp_fd);	
	test_dhcp_fd = -1;	
	test_dhcp_listen_mode = LISTEN_RAW;

	for (;;) {

		tv.tv_sec = test_dhcp_timeout - uptime();
		tv.tv_usec = 0;

		if (test_dhcp_listen_mode != LISTEN_NONE && test_dhcp_fd < 0) {
			test_dhcp_fd = raw_socket(client_config.ifindex);
			if (test_dhcp_fd < 0) {
				LOG(LOG_ERR, "FATAL: couldn't listen on socket, %m");
				return ;
			}
		}
		max_fd = udhcp_sp_fd_set(&rfds, test_dhcp_fd);

		if (tv.tv_sec > 0) {
			DEBUG(LOG_INFO, "Waiting on select...");
			retval = select(max_fd + 1, &rfds, NULL, NULL, &tv);
		} else retval = 0; /* If we already timed out, fall through */

		now = uptime();
		if (retval == 0) {
			/* test_dhcp_timeout dropped to zero */
			switch (test_dhcp_state) {
			case INIT_SELECTING:
				if (test_dhcp_packet_num < 3) {
					if (test_dhcp_packet_num == 0)
						xid = random_xid();

					/* send discover packet */
					send_discover(xid, requested_ip); /* broadcast */

					test_dhcp_timeout = now + ((test_dhcp_packet_num == 2) ? 4 : 2);
					test_dhcp_packet_num++;
				}
				else 
				{	
					printf("test dhcp failed\n");
					fwrite("0", 1, 1, fp);
					fclose(fp);
					LOG(LOG_ERR, "lease fail");
					return ;
				}
				break;
			}
		} else if (retval > 0 && test_dhcp_listen_mode != LISTEN_NONE && FD_ISSET(test_dhcp_fd, &rfds)) {
			/* a packet is ready, read it */

			len = get_raw_packet(&packet, test_dhcp_fd);

			if (len == -1 && errno != EINTR) {
				DEBUG(LOG_INFO, "error on read, %m, reopening socket");
			}
			if (len < 0)
				continue;	
			
			if (packet.xid != xid) {
				DEBUG(LOG_INFO, "Ignoring XID %lx (our xid is %lx)",
					(unsigned long) packet.xid, xid);
				continue;
			}
			/* Ignore packets that aren't for us */
			if (memcmp(client_config.arp,packet.chaddr,6))
				continue;

			if ((message = get_option(&packet, DHCP_MESSAGE_TYPE)) == NULL) {
				DEBUG(LOG_ERR, "couldnt get option from packet -- ignoring");
				continue;
			}

			if (test_dhcp_state == INIT_SELECTING && *message == DHCPOFFER) 
			{
				/* Must be a DHCPOFFER to one of our xid's */
				if ((temp = get_option(&packet, DHCP_SERVER_ID))) 
				{
					memcpy(&server_addr, temp, 4);
					xid = packet.xid;

					/* enter requesting state */
					test_dhcp_state = REQUESTING;
					test_dhcp_timeout = now;
					test_dhcp_packet_num = 0;
					printf("test dhcp success\n");
					fwrite("1", 1, 1, fp);	
					fclose(fp);
					LOG(LOG_ERR, "detect dhcp server");
					return ;
				} 
				else 
				{
					DEBUG(LOG_ERR, "No server ID in message");

				}
 			}
		} else if (retval == -1 && errno == EINTR) {
			/* a signal was caught */
		} else {
			/* An error occured */
			DEBUG(LOG_ERR, "Error on select");
		}

	}
	return ;
}

/* jimmy added 20080428 */

//int nvram_needed=0;

/* Only clean records in Nvram */
//void Clean_Nvram_Route(void){
//UDHCPC_STATIC_ROUTE
//UDHCPC_CLASSLESS_STATIC_ROUTE
//UDHCPC_MS_CLASSLESS_STATIC_ROUTE

//#ifndef CONFIG_USER_STATIC_ROUTE_NUMBER
//#define CONFIG_USER_STATIC_ROUTE_NUMBER 25
//#endif
//#ifndef NVRAM_STATIC_ROUTE_NAME
//#define NVRAM_STATIC_ROUTE_NAME "dhcp"
//#endif

//		char *nv_ptr = NULL;
//		char nvram_option[20];
// 		int s = 0;
// 		if((nv_ptr = (char *)nvram_get("static_routing_00"))!=NULL){
// 			/* There are static_routing options in nvram */
// 			nvram_needed = 1;
// 
// 			/* remove older static routing added by me */
// 			memset(nvram_option,'0',sizeof(nvram_option));
// 			for(s=0; s< CONFIG_USER_STATIC_ROUTE_NUMBER; s++){
// 				sprintf(nvram_option,"static_routing_%02d",s);
// 				nv_ptr = (char *)nvram_get(nvram_option);
// 				if((nv_ptr) && (strstr(nv_ptr,NVRAM_STATIC_ROUTE_NAME))){
// 					/* old static route added by me, clean it */
// 					nvram_set(nvram_option,"");
// 				}
// 			}
// 			nvram_commit();
// 			nvram_flag_reset();
// 		}
// }
/*	Date: 2009-01-07
*	Name: jimmy huang
*	Reason: we don't use that function now, and it uses nvram library
*	Note: Marked the function Clean_Nvram_Route() above
*/

/* option 33 */
int OptionStaticRoute(struct dhcpMessage *packet){
#ifdef UDHCPC_STATIC_ROUTE

#define STATIC_ROUTE_ADD_SHELL "/tmp/static_route_add.sh"
#define STATIC_ROUTE_DEL_SHELL "/tmp/static_route_del.sh"

#ifndef CONFIG_USER_STATIC_ROUTE_NUMBER
#define CONFIG_USER_STATIC_ROUTE_NUMBER 25
#endif

#ifndef NVRAM_STATIC_ROUTE_NAME
#define NVRAM_STATIC_ROUTE_NAME "dhcp"
#endif

	char *nv_ptr = NULL;
	int s = 0;
	char nvram_option[20];
	/* enalbe/  name  /dest_addr/dest_mask/gateway/interface/metric  */
	/*  1   +1+ name +1+ 15    +1+  15   +1+ 15  +1+ 4     +1+ 3    +1 */
	char nvram_value[1+1+strlen(NVRAM_STATIC_ROUTE_NAME)+1+15+1+15+1+15+1+4+1+3+1];
	uint32_t des_ip, gw;
	uint8_t option_len=0,flag_tmp=0,*temp;

	_system("[ -f /tmp/static_route_del.sh ] && /tmp/static_route_del.sh");
	unlink(STATIC_ROUTE_ADD_SHELL);
	unlink(STATIC_ROUTE_DEL_SHELL);
	if (!(temp = get_option(packet, DHCP_STATIC_ROUTE))) {
		LOG(LOG_DEBUG, "No DHCP ACK with option DHCP_STATIC_ROUTE");
		return 0;
	} else {
			LOG(LOG_DEBUG, "DHCP ACK with option DHCP_STATIC_ROUTE");
			option_len = (temp-1)[0];
			FILE *fp_add = NULL,*fp_del = NULL;
						
			if(((fp_add=fopen(STATIC_ROUTE_ADD_SHELL,"w")) != NULL)){
				if((fp_del=fopen(STATIC_ROUTE_DEL_SHELL,"w")) != NULL){
					fprintf(fp_add,"#!/bin/sh\n");
					fprintf(fp_del,"#!/bin/sh\n");
				}else{
					fclose(fp_add);
					LOG(LOG_ERR,"%s, can't open %s with write permission ",__FUNCTION__,STATIC_ROUTE_DEL_SHELL);
				}
			}else{
				LOG(LOG_ERR,"%s, can't open %s with write permission ",__FUNCTION__,STATIC_ROUTE_ADD_SHELL);
			}

			if((option_len > 0) && (option_len%8) == 0){
				while(flag_tmp < option_len){
					((unsigned char *)&des_ip)[0] = temp[flag_tmp];
					((unsigned char *)&des_ip)[1] = temp[flag_tmp+1];
					((unsigned char *)&des_ip)[2] = temp[flag_tmp+2];
					((unsigned char *)&des_ip)[3] = temp[flag_tmp+3];
					((unsigned char *)&gw)[0] = temp[flag_tmp+4];
					((unsigned char *)&gw)[1] = temp[flag_tmp+5];
					((unsigned char *)&gw)[2] = temp[flag_tmp+6];
					((unsigned char *)&gw)[3] = temp[flag_tmp+7];
					if((((unsigned char *)&des_ip)[0] == 0) && (((unsigned char *)&des_ip)[1] == 0) &&
							(((unsigned char *)&des_ip)[2] == 0) && (((unsigned char *)&des_ip)[3] == 0)){
						LOG(LOG_ERR,"%s,  illegal destion 0.0.0.0 for static routes (option 33)",__FUNCTION__);
						flag_tmp = flag_tmp + 8;
						continue;
					}
					/* save new route to nvram */
// 					if(nvram_needed){
// 						memset(nvram_option,'0',sizeof(nvram_option));
// 						memset(nvram_value,'0',sizeof(nvram_value));
// 						for(s=0; s< CONFIG_USER_STATIC_ROUTE_NUMBER; s++){
// 							sprintf(nvram_option,"static_routing_%02d",s);
// 							nv_ptr = (char *)nvram_get(nvram_option);
// 							if( (nv_ptr) && (strlen(nv_ptr) == 0) ){
// 								/* static route option with empty value */
// 								/* enalbe/  name  /dest_addr/dest_mask/gateway/interface/metric  */
// 								sprintf(nvram_value,"1/%s/%u.%u.%u.%u/255.255.255.255/%u.%u.%u.%u/%s/1",
// 									NVRAM_STATIC_ROUTE_NAME,
// 									((unsigned char *)&des_ip)[0],((unsigned char *)&des_ip)[1],
// 									((unsigned char *)&des_ip)[2],((unsigned char *)&des_ip)[3],
// 									((unsigned char *)&gw)[0],((unsigned char *)&gw)[1],
// 									((unsigned char *)&gw)[2],((unsigned char *)&gw)[3],
// 									nvram_safe_get("wan_eth")
// 									);
// 								nvram_set(nvram_option,nvram_value);
// 								break;
// 							}
// 						}
// 						nvram_commit();
// 						nvram_flag_reset();
// 					}
					/*	Date: 2009-01-07
					*	Name: jimmy huang
					*	Reason:	we don't use the feature - save option 121/249 to nvram now
					*			and it uses nvram library
					*	Note: Marked the codes above
					*/
					/* --------------------------------------- */
					if(fp_add && fp_del){
						fprintf(fp_add,"route add -host %u.%u.%u.%u gw %u.%u.%u.%u metric 1\n",
							((unsigned char *)&des_ip)[0],((unsigned char *)&des_ip)[1],
							((unsigned char *)&des_ip)[2],((unsigned char *)&des_ip)[3],
							((unsigned char *)&gw)[0],((unsigned char *)&gw)[1],
							((unsigned char *)&gw)[2],((unsigned char *)&gw)[3]);
						fprintf(fp_del,"route del -host %u.%u.%u.%u gw %u.%u.%u.%u metric 1\n",
							((unsigned char *)&des_ip)[0],((unsigned char *)&des_ip)[1],
							((unsigned char *)&des_ip)[2],((unsigned char *)&des_ip)[3],
							((unsigned char *)&gw)[0],((unsigned char *)&gw)[1],
							((unsigned char *)&gw)[2],((unsigned char *)&gw)[3]);
					}else{
						_system("route add -host %u.%u.%u.%u gw %u.%u.%u.%u metric 1",
							((unsigned char *)&des_ip)[0],((unsigned char *)&des_ip)[1],
							((unsigned char *)&des_ip)[2],((unsigned char *)&des_ip)[3],
							((unsigned char *)&gw)[0],((unsigned char *)&gw)[1],
							((unsigned char *)&gw)[2],((unsigned char *)&gw)[3]);
					}
					flag_tmp = flag_tmp + 8;
				}
				if(fp_add && fp_del){
					fclose(fp_add);
					chmod(STATIC_ROUTE_ADD_SHELL,S_IRWXU);
					fclose(fp_del);
					chmod(STATIC_ROUTE_DEL_SHELL,S_IRWXU);
					_system(STATIC_ROUTE_ADD_SHELL);
				}
			}else{
				LOG(LOG_ERR,"%s, Incorrect option length %u for dhcp option 33 (static route)",__FUNCTION__,option_len);
				return 0;
			}
		}
#else
	/*	Date: 2009-01-07
	*	Name: jimmy huang
	*	Reason:	to avoid compiler warning message
	*			warning: unused parameter 'packet'
	*	Note: Add the codes below
	*/
	if(packet){
		;
	}
#endif
	return 1;
}

/* option 121 */
int OptionClasslessStaticRoute(struct dhcpMessage *packet){
/*
Note:
	- Vista dhcp client will send 121 and 249, and if dhcp server response 
		with 121, vista will only accept 121, ignore 249
	- We don't support router address is 0.0.0.0, refer to rfc 3442, page 5
	- if there are "static_routing" option in nvram, i will
		- remove old records
		- then find empty entry to record new records
		- maximun numbers of records are defined as CONFIG_USER_STATIC_ROUTE_NUMBER
		- the name in each entry list are definded as NVRAM_STATIC_ROUTE_NAME
			i will take it as the condition to recognize the records added by me 
			to remove/add to nvram
*/
#ifdef UDHCPC_RFC_CLASSLESS_STATIC_ROUTE
#define RFC_CLASSLESS_STATIC_STATIC_ROUTE_ADD_SHELL "/tmp/classes_static_route_add.sh"
#define RFC_CLASSLESS_STATIC_STATIC_ROUTE_DEL_SHELL "/tmp/classes_static_route_del.sh"

#ifndef CONFIG_USER_STATIC_ROUTE_NUMBER
#define CONFIG_USER_STATIC_ROUTE_NUMBER 25
#endif
#ifndef NVRAM_STATIC_ROUTE_NAME
#define NVRAM_STATIC_ROUTE_NAME "dhcp"
#endif

//	char *nv_ptr = NULL;
	int s = 0;
//	char nvram_option[20];
	/* enalbe/  name  /dest_addr/dest_mask/gateway/interface/metric  */
	/*  1   +1+ name +1+ 15    +1+  15   +1+ 15  +1+ 4     +1+ 3    +1 */
//	char nvram_value[1+1+strlen(NVRAM_STATIC_ROUTE_NAME)+1+15+1+15+1+15+1+4+1+3+1];
	uint32_t des_ip, netmask, gw;
	uint8_t option_len=0,mask_num=0,flag_tmp=0,*temp;
	FILE *fp_add = NULL,*fp_del = NULL;

/* jimmy added for dhcp option 121 enable/disable */
//	if((nv_ptr = (char *)nvram_get("classless_static_route"))!=NULL){
//		if(strcmp(nv_ptr,"1") != 0){
//			return 1;
//		}
//	}
//	nv_ptr = NULL;
	/*	Date: 2009-01-07
	*	Name: jimmy huang
	*	Reason:	we don't want to nvram library
	*	Note: Marked the codes above, and use the codes below
	*/
	if(client_config.classless_static_route_enable == 0){
		return 1;
	}

/* --------------------------------------------- */

	_system(RFC_CLASSLESS_STATIC_STATIC_ROUTE_DEL_SHELL);
	unlink(RFC_CLASSLESS_STATIC_STATIC_ROUTE_ADD_SHELL);
	unlink(RFC_CLASSLESS_STATIC_STATIC_ROUTE_DEL_SHELL);

	if (!(temp = get_option(packet, RFC_CLASSLESS_STATIC_ROUTE))) {
		LOG(LOG_DEBUG, "No DHCP ACK with option DHCPC_CLASSLESS_STATIC_ROUTE");
		return 0;
	} else {
		LOG(LOG_DEBUG, "DHCP ACK with option DHCPC_CLASSLESS_STATIC_ROUTE");
		
		option_len = (temp-1)[0];

		if(option_len >= 5){ //rfc specifies minimun length is 5
			if((fp_add=fopen(RFC_CLASSLESS_STATIC_STATIC_ROUTE_ADD_SHELL,"w")) != NULL){
				if((fp_del=fopen(RFC_CLASSLESS_STATIC_STATIC_ROUTE_DEL_SHELL,"w")) != NULL){
					fprintf(fp_add,"#!/bin/sh\n");
					fprintf(fp_del,"#!/bin/sh\n");
				}else{
					LOG(LOG_ERR,"%s, can't open %s with write permission ",__FUNCTION__,RFC_CLASSLESS_STATIC_STATIC_ROUTE_DEL_SHELL);
					fclose(fp_add);
				}
			}else{
				LOG(LOG_ERR,"%s, can't open %s with write permission ",__FUNCTION__,RFC_CLASSLESS_STATIC_STATIC_ROUTE_ADD_SHELL);
			}

			while(flag_tmp < option_len){
				mask_num = temp[flag_tmp];
				if(mask_num == 0){
					/* netmask is 0.0.0.0, default gw */
					((unsigned char *)&des_ip)[0] = ((unsigned char *)&des_ip)[1] = ((unsigned char *)&des_ip)[2] = ((unsigned char *)&des_ip)[3] = 0;
					((unsigned char *)&gw)[0] = temp[flag_tmp+1];
					((unsigned char *)&gw)[1] = temp[flag_tmp+2];
					((unsigned char *)&gw)[2] = temp[flag_tmp+3];
					((unsigned char *)&gw)[3] = temp[flag_tmp+4];
					flag_tmp = flag_tmp+5;
				}else if( 1 <= mask_num && mask_num <= 8 ){
					((unsigned char *)&des_ip)[0] = temp[flag_tmp+1];
					((unsigned char *)&des_ip)[1] = ((unsigned char *)&des_ip)[2] = ((unsigned char *)&des_ip)[3] = 0;
					((unsigned char *)&gw)[0] = temp[flag_tmp+2];
					((unsigned char *)&gw)[1] = temp[flag_tmp+3];
					((unsigned char *)&gw)[2] = temp[flag_tmp+4];
					((unsigned char *)&gw)[3] = temp[flag_tmp+5];
					flag_tmp = flag_tmp+6;
				}else if( 9 <= mask_num && mask_num <= 16 ){
					((unsigned char *)&des_ip)[0] = temp[flag_tmp+1];
					((unsigned char *)&des_ip)[1] = temp[flag_tmp+2];
					((unsigned char *)&des_ip)[2] = ((unsigned char *)&des_ip)[3] = 0;
					((unsigned char *)&gw)[0] = temp[flag_tmp+3];
					((unsigned char *)&gw)[1] = temp[flag_tmp+4];
					((unsigned char *)&gw)[2] = temp[flag_tmp+5];
					((unsigned char *)&gw)[3] = temp[flag_tmp+6];
					flag_tmp = flag_tmp+7;
				}else if( 17 <= mask_num && mask_num <= 24 ){
					((unsigned char *)&des_ip)[0] = temp[flag_tmp+1];
					((unsigned char *)&des_ip)[1] = temp[flag_tmp+2];
					((unsigned char *)&des_ip)[2] = temp[flag_tmp+3];
					((unsigned char *)&des_ip)[3] = 0;
					((unsigned char *)&gw)[0] = temp[flag_tmp+4];
					((unsigned char *)&gw)[1] = temp[flag_tmp+5];
					((unsigned char *)&gw)[2] = temp[flag_tmp+6];
					((unsigned char *)&gw)[3] = temp[flag_tmp+7];
					flag_tmp = flag_tmp+8;
				}else if( 25 <= mask_num && mask_num <= 32 ){
					((unsigned char *)&des_ip)[0] = temp[flag_tmp+1];
					((unsigned char *)&des_ip)[1] = temp[flag_tmp+2];
					((unsigned char *)&des_ip)[2] = temp[flag_tmp+3];
					((unsigned char *)&des_ip)[3] = temp[flag_tmp+4];
					((unsigned char *)&gw)[0] = temp[flag_tmp+5];
					((unsigned char *)&gw)[1] = temp[flag_tmp+6];
					((unsigned char *)&gw)[2] = temp[flag_tmp+7];
					((unsigned char *)&gw)[3] = temp[flag_tmp+8];
					flag_tmp = flag_tmp+9;
				}else{
					//error , should not go through here !!!
					LOG(LOG_ERR,"%s, Unknown mask num %u !",__FUNCTION__,mask_num);
					continue;
				}

		        memset(&netmask,0,sizeof(uint32_t));
		        for(s=0;s<mask_num;s++){
                	netmask = netmask | (1<<(31-s));
		        }
								
				/* save new route to nvram */
// 				if(nvram_needed){
// 					memset(nvram_option,'0',sizeof(nvram_option));
// 					memset(nvram_value,'0',sizeof(nvram_value));
// 					for(s=0; s< CONFIG_USER_STATIC_ROUTE_NUMBER; s++){
// 						sprintf(nvram_option,"static_routing_%02d",s);
// 						nv_ptr = (char *)nvram_get(nvram_option);
// 						if( (nv_ptr) && (strlen(nv_ptr) == 0) ){
// 							/* static route option with empty value */
// 							/* enalbe/  name  /dest_addr/dest_mask/gateway/interface/metric  */
// 							sprintf(nvram_value,"1/%s/%u.%u.%u.%u/%u.%u.%u.%u/%u.%u.%u.%u/%s/1",
// 								NVRAM_STATIC_ROUTE_NAME,
// 								((unsigned char *)&des_ip)[0],((unsigned char *)&des_ip)[1],
// 								((unsigned char *)&des_ip)[2],((unsigned char *)&des_ip)[3],
// 								((unsigned char *)&netmask)[0],((unsigned char *)&netmask)[1],
// 								((unsigned char *)&netmask)[2],((unsigned char *)&netmask)[3],
// 								((unsigned char *)&gw)[0],((unsigned char *)&gw)[1],
// 								((unsigned char *)&gw)[2],((unsigned char *)&gw)[3],
// 								nvram_safe_get("wan_eth")
// 								);
// 							nvram_set(nvram_option,nvram_value);
// 							break;
// 						}
// 					}
// 					nvram_commit();
// 					nvram_flag_reset();
// 				}
				/*	Date: 2009-01-07
					*	Name: jimmy huang
					*	Reason:	we don't use the feature - save option 121/249 to nvram now
					*			and it uses nvram library
					*	Note: Marked the codes above
					*/
					/* --------------------------------------- */
				/* --------------------------------------- */
								
				LOG(LOG_DEBUG, "Adding DHCP_STATIC_ROUTE -net %u.%u.%u.%u netmask[%u] %u.%u.%u.%u router %u.%u.%u.%u metric 1",
					((unsigned char *)&des_ip)[0],((unsigned char *)&des_ip)[1],
					((unsigned char *)&des_ip)[2],((unsigned char *)&des_ip)[3],
					mask_num,
					((unsigned char *)&netmask)[0],((unsigned char *)&netmask)[1],
					((unsigned char *)&netmask)[2],((unsigned char *)&netmask)[3],
					((unsigned char *)&gw)[0],((unsigned char *)&gw)[1],
					((unsigned char *)&gw)[2],((unsigned char *)&gw)[3]);
				if(fp_add && fp_del){
					fprintf(fp_add,"route add -net %u.%u.%u.%u netmask %u.%u.%u.%u gw %u.%u.%u.%u metric 1\n",
						((unsigned char *)&des_ip)[0],((unsigned char *)&des_ip)[1],
						((unsigned char *)&des_ip)[2],((unsigned char *)&des_ip)[3],
						((unsigned char *)&netmask)[0],((unsigned char *)&netmask)[1],
						((unsigned char *)&netmask)[2],((unsigned char *)&netmask)[3],
						((unsigned char *)&gw)[0],((unsigned char *)&gw)[1],
						((unsigned char *)&gw)[2],((unsigned char *)&gw)[3]);
					fprintf(fp_del,"route del -net %u.%u.%u.%u netmask %u.%u.%u.%u gw %u.%u.%u.%u metric 1\n",
						((unsigned char *)&des_ip)[0],((unsigned char *)&des_ip)[1],
						((unsigned char *)&des_ip)[2],((unsigned char *)&des_ip)[3],
						((unsigned char *)&netmask)[0],((unsigned char *)&netmask)[1],
						((unsigned char *)&netmask)[2],((unsigned char *)&netmask)[3],
						((unsigned char *)&gw)[0],((unsigned char *)&gw)[1],
						((unsigned char *)&gw)[2],((unsigned char *)&gw)[3]);
				}else{
					_system("route add -net %u.%u.%u.%u netmask %u.%u.%u.%u gw %u.%u.%u.%u metric 1",
						((unsigned char *)&des_ip)[0],((unsigned char *)&des_ip)[1],
						((unsigned char *)&des_ip)[2],((unsigned char *)&des_ip)[3],
						((unsigned char *)&netmask)[0],((unsigned char *)&netmask)[1],
						((unsigned char *)&netmask)[2],((unsigned char *)&netmask)[3],
						((unsigned char *)&gw)[0],((unsigned char *)&gw)[1],
						((unsigned char *)&gw)[2],((unsigned char *)&gw)[3]);
				}
			}
			if(fp_add){
				fclose(fp_add);
				chmod(RFC_CLASSLESS_STATIC_STATIC_ROUTE_ADD_SHELL,S_IRWXU);
			}
			if(fp_del){
				fclose(fp_del);
				chmod(RFC_CLASSLESS_STATIC_STATIC_ROUTE_DEL_SHELL,S_IRWXU);
			}
			_system(RFC_CLASSLESS_STATIC_STATIC_ROUTE_ADD_SHELL);
		}else{
			LOG(LOG_ERR,"%s, Invalid option length %u !",__FUNCTION__,option_len);
			return 0;
		}
	}
#else
	/*	Date: 2009-01-09
	*	Name: jimmy huang
	*	Reason:	to avoid compiler warning message
	*			warning: unused parameter 'packet'
	*	Note: Add the codes below
	*/
	if(packet){
		;
	}
#endif
	return 1;
}

/* option 249 */
int OptionMicroSoftClasslessStaticRoute(struct dhcpMessage *packet){
/*
Note:
	- Vista dhcp client will send 121 and 249, and if dhcp server response 
		with 121, vista will only accept 121, ignore 249
	- We don't support router address is 0.0.0.0, refer to rfc 3442, page 5
*/
#ifdef UDHCPC_MS_CLASSLESS_STATIC_ROUTE
#define MS_CLASSLESS_STATIC_ROUTE_ADD_SHELL "/tmp/ms_classes_static_route_add.sh"
#define MS_CLASSLESS_STATIC_ROUTE_DEL_SHELL "/tmp/ms_classes_static_route_del.sh"

#ifndef CONFIG_USER_STATIC_ROUTE_NUMBER
#define CONFIG_USER_STATIC_ROUTE_NUMBER 25
#endif
#ifndef NVRAM_STATIC_ROUTE_NAME
#define NVRAM_STATIC_ROUTE_NAME "dhcp"
#endif

	char *nv_ptr = NULL;
	int s = 0;
	char nvram_option[20];
	/* enalbe/  name  /dest_addr/dest_mask/gateway/interface/metric  */
	/*  1   +1+ name +1+ 15    +1+  15   +1+ 15  +1+ 4     +1+ 3    +1 */
	char nvram_value[1+1+strlen(NVRAM_STATIC_ROUTE_NAME)+1+15+1+15+1+15+1+4+1+3+1];
	uint32_t des_ip, netmask, gw;
	uint8_t option_len=0,mask_num=0,flag_tmp=0,*temp;
	FILE *fp_add = NULL,*fp_del = NULL;

/* jimmy added for dhcp option 249 enable/disable */
	//if((nv_ptr = (char *)nvram_get("classless_static_route"))!=NULL){
	//	if(strcmp(nv_ptr,"1") != 0){
	//		return 1;
	//	}
	//}
	//nv_ptr = NULL;
	/*	Date: 2009-01-07
	*	Name: jimmy huang
	*	Reason:	we don't want to nvram library
	*	Note: Marked the codes above, and use the codes below
	*/
	if(client_config.classless_static_route_enable == 0){
		return 1;
	}
/* --------------------------------------------- */
	
	_system(MS_CLASSLESS_STATIC_ROUTE_DEL_SHELL);
	unlink(MS_CLASSLESS_STATIC_ROUTE_ADD_SHELL);
	unlink(MS_CLASSLESS_STATIC_ROUTE_DEL_SHELL);
	if (!(temp = get_option(packet, MS_CLASSLESS_STATIC_ROUTE))) {
		LOG(LOG_DEBUG, "No DHCP ACK with MS_DHCP_STATIC_ROUTE (option 249)");
		return 0;
	} else {
		LOG(LOG_DEBUG, "DHCP ACK with MS_DHCP_STATIC_ROUTE (option 249)");
		option_len = (temp-1)[0];
		
		if(option_len >= 5){ //rfc specifies minimun length is 5
			if((fp_add=fopen(MS_CLASSLESS_STATIC_ROUTE_ADD_SHELL,"w")) != NULL){
				if((fp_del=fopen(MS_CLASSLESS_STATIC_ROUTE_DEL_SHELL,"w")) != NULL){
					fprintf(fp_add,"#!/bin/sh\n");
					fprintf(fp_del,"#!/bin/sh\n");
				}else{
					LOG(LOG_ERR,"%s, can't open %s with write permission ",__FUNCTION__,MS_CLASSLESS_STATIC_ROUTE_DEL_SHELL);
					fclose(fp_add);
				}
			}else{
				LOG(LOG_ERR,"%s, can't open %s with write permission ",__FUNCTION__,MS_CLASSLESS_STATIC_ROUTE_ADD_SHELL);
			}

			while(flag_tmp < option_len){
				mask_num = temp[flag_tmp];
				if(mask_num == 0){
					/* netmask is 0.0.0.0, default gw */
					((unsigned char *)&des_ip)[0] = ((unsigned char *)&des_ip)[1] = ((unsigned char *)&des_ip)[2] = ((unsigned char *)&des_ip)[3] = 0;
					((unsigned char *)&gw)[0] = temp[flag_tmp+1];
					((unsigned char *)&gw)[1] = temp[flag_tmp+2];
					((unsigned char *)&gw)[2] = temp[flag_tmp+3];
					((unsigned char *)&gw)[3] = temp[flag_tmp+4];
					flag_tmp = flag_tmp+5;
				}else if( 1 <= mask_num && mask_num <= 8 ){
					((unsigned char *)&des_ip)[0] = temp[flag_tmp+1];
					((unsigned char *)&des_ip)[1] = ((unsigned char *)&des_ip)[2] = ((unsigned char *)&des_ip)[3] = 0;
					((unsigned char *)&gw)[0] = temp[flag_tmp+2];
					((unsigned char *)&gw)[1] = temp[flag_tmp+3];
					((unsigned char *)&gw)[2] = temp[flag_tmp+4];
					((unsigned char *)&gw)[3] = temp[flag_tmp+5];
					flag_tmp = flag_tmp+6;
				}else if( 9 <= mask_num && mask_num <= 16 ){
					((unsigned char *)&des_ip)[0] = temp[flag_tmp+1];
					((unsigned char *)&des_ip)[1] = temp[flag_tmp+2];
					((unsigned char *)&des_ip)[2] = ((unsigned char *)&des_ip)[3] = 0;
					((unsigned char *)&gw)[0] = temp[flag_tmp+3];
					((unsigned char *)&gw)[1] = temp[flag_tmp+4];
					((unsigned char *)&gw)[2] = temp[flag_tmp+5];
					((unsigned char *)&gw)[3] = temp[flag_tmp+6];
					flag_tmp = flag_tmp+7;
				}else if( 17 <= mask_num && mask_num <= 24 ){
					((unsigned char *)&des_ip)[0] = temp[flag_tmp+1];
					((unsigned char *)&des_ip)[1] = temp[flag_tmp+2];
					((unsigned char *)&des_ip)[2] = temp[flag_tmp+3];
					((unsigned char *)&des_ip)[3] = 0;
					((unsigned char *)&gw)[0] = temp[flag_tmp+4];
					((unsigned char *)&gw)[1] = temp[flag_tmp+5];
					((unsigned char *)&gw)[2] = temp[flag_tmp+6];
					((unsigned char *)&gw)[3] = temp[flag_tmp+7];
					flag_tmp = flag_tmp+8;
				}else if( 25 <= mask_num && mask_num <= 32 ){
					((unsigned char *)&des_ip)[0] = temp[flag_tmp+1];
					((unsigned char *)&des_ip)[1] = temp[flag_tmp+2];
					((unsigned char *)&des_ip)[2] = temp[flag_tmp+3];
					((unsigned char *)&des_ip)[3] = temp[flag_tmp+4];
					((unsigned char *)&gw)[0] = temp[flag_tmp+5];
					((unsigned char *)&gw)[1] = temp[flag_tmp+6];
					((unsigned char *)&gw)[2] = temp[flag_tmp+7];
					((unsigned char *)&gw)[3] = temp[flag_tmp+8];
					flag_tmp = flag_tmp+9;
				}else{
					//error , should not go through here !!!
					LOG(LOG_ERR,"%s, Unknown mask num %u !",__FUNCTION__,mask_num);
					continue;
				}

		        memset(&netmask,0,sizeof(uint32_t));
		        for(s=0;s<mask_num;s++){
                	netmask = netmask | (1<<(31-s));
		        }

				/* save new route to nvram */
// 				if(nvram_needed){
// 					memset(nvram_option,'0',sizeof(nvram_option));
// 					memset(nvram_value,'0',sizeof(nvram_value));
// 					for(s=0; s< CONFIG_USER_STATIC_ROUTE_NUMBER; s++){
// 						sprintf(nvram_option,"static_routing_%02d",s);
// 						nv_ptr = (char *)nvram_get(nvram_option);
// 						if( (nv_ptr) && (strlen(nv_ptr) == 0) ){
// 							/* static route option with empty value */
// 							/* enalbe/  name  /dest_addr/dest_mask/gateway/interface/metric  */
// 							sprintf(nvram_value,"1/%s/%u.%u.%u.%u/%u.%u.%u.%u/%u.%u.%u.%u/%s/1",
// 								NVRAM_STATIC_ROUTE_NAME,
// 								((unsigned char *)&des_ip)[0],((unsigned char *)&des_ip)[1],
// 								((unsigned char *)&des_ip)[2],((unsigned char *)&des_ip)[3],
// 								((unsigned char *)&netmask)[0],((unsigned char *)&netmask)[1],
// 								((unsigned char *)&netmask)[2],((unsigned char *)&netmask)[3],
// 								((unsigned char *)&gw)[0],((unsigned char *)&gw)[1],
// 								((unsigned char *)&gw)[2],((unsigned char *)&gw)[3],
// 								nvram_safe_get("wan_eth")
// 								);
// 							nvram_set(nvram_option,nvram_value);
// 							break;
// 						}
// 					}
// 					nvram_commit();
// 					nvram_flag_reset();
// 				}
				/*	Date: 2009-01-07
					*	Name: jimmy huang
					*	Reason:	we don't use the feature - save option 121/249 to nvram now
					*			and it uses nvram library
					*	Note: Marked the codes above
					*/
					/* --------------------------------------- */
				/* --------------------------------------- */

				LOG(LOG_DEBUG, "Adding MS_DHCP_STATIC_ROUTE -net %u.%u.%u.%u netmask[%u] %u.%u.%u.%u router %u.%u.%u.%u metric 1",
					((unsigned char *)&des_ip)[0],((unsigned char *)&des_ip)[1],
					((unsigned char *)&des_ip)[2],((unsigned char *)&des_ip)[3],
					mask_num,
					((unsigned char *)&netmask)[0],((unsigned char *)&netmask)[1],
					((unsigned char *)&netmask)[2],((unsigned char *)&netmask)[3],
					((unsigned char *)&gw)[0],((unsigned char *)&gw)[1],
					((unsigned char *)&gw)[2],((unsigned char *)&gw)[3]);
				if(fp_add && fp_del){
					fprintf(fp_add,"route add -net %u.%u.%u.%u netmask %u.%u.%u.%u gw %u.%u.%u.%u metric 1\n",
						((unsigned char *)&des_ip)[0],((unsigned char *)&des_ip)[1],
						((unsigned char *)&des_ip)[2],((unsigned char *)&des_ip)[3],
						((unsigned char *)&netmask)[0],((unsigned char *)&netmask)[1],
						((unsigned char *)&netmask)[2],((unsigned char *)&netmask)[3],
						((unsigned char *)&gw)[0],((unsigned char *)&gw)[1],
						((unsigned char *)&gw)[2],((unsigned char *)&gw)[3]);
					fprintf(fp_del,"route del -net %u.%u.%u.%u netmask %u.%u.%u.%u gw %u.%u.%u.%u metric 1\n",
						((unsigned char *)&des_ip)[0],((unsigned char *)&des_ip)[1],
						((unsigned char *)&des_ip)[2],((unsigned char *)&des_ip)[3],
						((unsigned char *)&netmask)[0],((unsigned char *)&netmask)[1],
						((unsigned char *)&netmask)[2],((unsigned char *)&netmask)[3],
						((unsigned char *)&gw)[0],((unsigned char *)&gw)[1],
						((unsigned char *)&gw)[2],((unsigned char *)&gw)[3]);
				}else{
					_system("route add -net %u.%u.%u.%u netmask %u.%u.%u.%u gw %u.%u.%u.%u metric 1",
						((unsigned char *)&des_ip)[0],((unsigned char *)&des_ip)[1],
						((unsigned char *)&des_ip)[2],((unsigned char *)&des_ip)[3],
						((unsigned char *)&netmask)[0],((unsigned char *)&netmask)[1],
						((unsigned char *)&netmask)[2],((unsigned char *)&netmask)[3],
						((unsigned char *)&gw)[0],((unsigned char *)&gw)[1],
						((unsigned char *)&gw)[2],((unsigned char *)&gw)[3]);
				}
			}
			if(fp_add){
				fclose(fp_add);
				chmod(MS_CLASSLESS_STATIC_ROUTE_ADD_SHELL,S_IRWXU);
			}
			if(fp_del){
				fclose(fp_del);
				chmod(MS_CLASSLESS_STATIC_ROUTE_DEL_SHELL,S_IRWXU);
			}
			_system(MS_CLASSLESS_STATIC_ROUTE_ADD_SHELL);
		}else{
			LOG(LOG_ERR,"%s, Invalid option length %u !",__FUNCTION__,option_len);
			return 0;
		}
	}
#else
	/*	Date: 2009-01-07
	*	Name: jimmy huang
	*	Reason:	to avoid compiler warning message
	*			warning: unused parameter 'packet'
	*	Note: Add the codes below
	*/
	if(packet){
		;
	}
#endif
	return 1;
}


#ifdef IPV6_6RD
/*XXX Joe Huang : Parse option 212 for IPv6 Spec 1.13		*/
/*		  Send the result to cli, that can enable 6rd	*/
int Option6RD(struct dhcpMessage *packet)
{
	uint8_t option_len = 0,*temp;
	uint8_t IPv4MaskLen = 0, _6rdPrefixLen = 0;
	uint8_t _6rdPrefix[16] = {0};
	uint8_t _6rdBRIPv4Address[4] = {0};
	char result[1024] = {0};
	char cmd[1048] = {0};
	int i = 0;
	FILE *fp;

	if(client_config.option_6rd_enable == 0){
		return 1;
	}

	if (!(temp = get_option(packet, OPTION_6RD))){
		LOG(LOG_DEBUG, "No DHCP ACK with option OPTION_6RD");
		return 0;
	}else{
		LOG(LOG_DEBUG, "DHCP ACK with option OPTION_6RD");
		option_len = (temp-1)[0];
					
		if((option_len >= 22)){
			/*Joe Huang : IPv4MaskLen + 6rdPrefixLen + 6rdPrefix + 6rdBRIPv4Address*/
			/*XXX The order of assign MUST NOT change, it's follow RFC 5969*/
			for(i = 0; i < 1 + 1 + 16 + 4; i++){
				if(i == 0)
					IPv4MaskLen = temp[i];
				if(i == 1)
					_6rdPrefixLen = temp[i];
				if(i >= 2 && i <= 17)
					_6rdPrefix[i - 2] = temp[i];
				if(i >= 18)
					_6rdBRIPv4Address[i - 18] = temp[i];
			}
		
			sprintf(result, "%d %d %02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x %d.%d.%d.%d",
				IPv4MaskLen, _6rdPrefixLen,
				_6rdPrefix[0], _6rdPrefix[1], _6rdPrefix[2], _6rdPrefix[3],
				_6rdPrefix[4], _6rdPrefix[5], _6rdPrefix[6], _6rdPrefix[7],
				_6rdPrefix[8], _6rdPrefix[9], _6rdPrefix[10], _6rdPrefix[11],
				_6rdPrefix[12], _6rdPrefix[13], _6rdPrefix[14], _6rdPrefix[15],
				_6rdBRIPv4Address[0], _6rdBRIPv4Address[1], _6rdBRIPv4Address[2], _6rdBRIPv4Address[3]);
				sprintf(cmd, "cli ipv6 6rd dhcp %s", result);
	      system(cmd);
	      sprintf(cmd, "echo \"%s\" > /var/tmp/option_6rd", result);
			system(cmd);
		}else{
			LOG(LOG_ERR, "%s, Incorrect option length %u for dhcp option 212 (OPTION-6RD)", __FUNCTION__, option_len);
			return 0;
		}
	}
	return 0;
}
#endif //IPV6_6RD

/* ----------------------------------- */

// 2011.03.15 ACS URL option 43 issue
char *acs_url_process(FILE *fp,char *s,int len) 
{
	int length;
	int done = 0;
	int i=0;
	uint8_t *optionptr;
	

	optionptr = s;

	//length = 255; // max size 255
	length = len;
	if( length > 255)
		length = 255;
	while (!done) {
		if (i >= length) {
			LOG(LOG_WARNING, "bogus packet, option fields too long.");
			return NULL;
		}
		if (optionptr[i + OPT_CODE] == 0) {
			break;
		}
		if (optionptr[i + OPT_CODE] == 255) {
			break;
		}

		if (optionptr[i + OPT_CODE] == 1) {
			// got acs url
			int l;
			char url[128];
			l = optionptr[OPT_LEN + i];
			if( l < 128) {
				memcpy(url,&optionptr[OPT_DATA+i],l);
				url[l] = 0; // null end string
               	fprintf(fp,"%s\n",url);
				printf("Got ACS URL %s\n",url);
				LOG(LOG_INFO, "Got ACS URL %s",url);
			}
			
			printf("Got ACS URL %d\n",l);
			LOG(LOG_INFO, "Got ACS URL %d",l);
			return NULL;
		}

		i += optionptr[OPT_LEN + i] + 2;
	}


	return NULL;
}


/* ----------------------------------- */

#ifdef COMBINED_BINARY
int udhcpc_main(int argc, char *argv[])
#else
int main(int argc, char *argv[])
#endif
{
	uint8_t *temp, *message;
	unsigned long t1 = 0, t2 = 0, xid = 0;
	unsigned long start = 0, lease;
	fd_set rfds;
	int retval;
	struct timeval tv;
	int c, len;
	struct dhcpMessage packet;
	struct in_addr temp_addr;
	long now;
	int max_fd;
	int sig;
	int no_clientid = 0;
	int rc_restart_flag = 0;
	FILE *fp = NULL;
	FILE *fp1 = NULL;
	/*
	FILE *fp_dr = NULL;
	*/
	/*  Date: 2009-01-09
	*   Name: Cosmo Chang
	*   Reason: dhcpc will rewrite dns in the l2tp-mode while release-time timeout
	*   Note:
	*/
	FILE *fp_dr_pptp = NULL;
	FILE *fp_dr_l2tp = NULL;
	char data[2] = {0};
	int send_signal_done = 0, sleep_count=0;
#ifdef AUTO_MODE_SELECT
	/*  Date: 2010-04-28
	*   Name: Jimmy Huang
	*   Reason: Used for temperary store new ip then check it's public or private IP
	*   Note:
	*/
	int ip_a = 0,ip_b = 0;
#endif
#ifdef CONFIG_USER_TC	
	FILE *fp_bandwidth = NULL;
	FILE *fp_tmp = NULL;
	char value[32] = {0}; 
#endif

	static const struct option arg_options[] = {
		{"clientid",	required_argument,	0, 'c'},//option_index = 0
		{"clientid-none", no_argument,		0, 'C'},
		{"vendorclass",	required_argument,	0, 'V'},
		{"foreground",	no_argument,		0, 'f'},
		{"rc-restart",  no_argument,   		0, 'j'},
		{"background",	no_argument,		0, 'b'},
		{"hostname",	required_argument,	0, 'H'},
		{"hostname",	required_argument,	0, 'h'},
		{"fqdn",	required_argument,	0, 'F'},  //option_index = 7
		{"interface",	required_argument,	0, 'i'},
		{"now", 	no_argument,		0, 'n'},
		{"pidfile",	required_argument,	0, 'p'},  //option_index = 10
		{"quit",	no_argument,		0, 'q'},
		{"request",	required_argument,	0, 'r'},
		{"script",	required_argument,	0, 's'},
		{"version",	no_argument,		0, 'v'},
 		{"wan_proto",		required_argument,	0, 'w'},//option_Index = 15
#ifdef UDHCPD_NETBIOS
		{"netbios_enable",	no_argument,	0, 'N'},
#endif		
#ifdef RPPPOE	
        {"russia_enable",	no_argument,	0, 'R'},
#endif	
#ifdef CONFIG_BRIDGE_MODE	
        {"ap_mode",	no_argument,	0, 'a'},
/*	Date: 2009-04-10
	*	Name: Ken Chiang
	*	Reason:	Add support for enable auto mode select(router mode/ap mode).
	*	Note: 
*/		
#ifdef AUTO_MODE_SELECT	
        {"auto_mode_select",	no_argument,	0, 'A'},
#endif        
#endif

// 2011.03.18
        {"tr069_select",	no_argument,	0, 'z'},
		
		
		/*	Date: 2009-01-09
		*	Name: jimmy huang
		*	Reason:	Add support for disable vendor class identifier (60)
		*			Add support for enable classless static route (121/249)
		*	Note: 
				struct option {
					const char *name;
					int has_arg;
					int *flag;
					int val;
				};
				has_tag:
					"no_argument" (or 0) if the option does not take an argument,
					"required_argument" (or 1) if the option requires an argument,  or
					"optional_argument"  (or  2) if the option takes an optional argument.
		*/
		{"option60_off",	no_argument,	0, 0},
#if defined(UDHCPC_MS_CLASSLESS_STATIC_ROUTE) || defined(UDHCPC_RFC_CLASSLESS_STATIC_ROUTE)
		{"option121or249_on",	no_argument,	0, 0},
#endif
#ifdef IPV6_6RD
		{"option212_on",	no_argument,	0, 0},
#endif
		{0, 0, 0, 0}
	};

	/* get options */
	while (1) {
		int option_index = 0;
		c = getopt_long(argc, argv, "c:CV:fjbH:h:F:i:w:np:qr:s:v:uNRaAz", arg_options, &option_index);
		if (c == -1) break;

		switch (c) {
			/*	Date: 2009-01-09
			*	Name: jimmy huang
			*	Reason:	Add support for vendor class identifier disable (60)
			*			Add support for enable classless static route (121/249)
			*	Note: 	This coding method for the option 60, 121or249
			*			make udhcpc only accept parameters with --
			*			Not accept parameters with -
			*			And if needed, we could me it accept "udhcpc --optipn60 on"
			*			or									"udhcpc --optipn60 off"
			*/
			case 0:
				if(strcmp("option60_off",arg_options[option_index].name)==0){
					client_config.vendor_class_identifier_disable = 1;
					break;
				}
#if defined(UDHCPC_MS_CLASSLESS_STATIC_ROUTE) || defined(UDHCPC_RFC_CLASSLESS_STATIC_ROUTE)
				if(strcmp("option121or249_on",arg_options[option_index].name)==0){
					client_config.classless_static_route_enable = 1;
					break;
				}
#endif
#ifdef IPV6_6RD
				if(strcmp("option212_on",arg_options[option_index].name)==0){
					client_config.option_6rd_enable = 1;
					break;
				}
#endif
				break;
			case 'c':
				if (no_clientid) show_usage();
				len = strlen(optarg) > 255 ? 255 : strlen(optarg);
				if (client_config.clientid) free(client_config.clientid);
				client_config.clientid = xmalloc(len + 2);
				client_config.clientid[OPT_CODE] = DHCP_CLIENT_ID;
				client_config.clientid[OPT_LEN] = len;
				client_config.clientid[OPT_DATA] = '\0';
				strncpy(client_config.clientid + OPT_DATA, optarg, len);
				break;
			case 'C':
				if (client_config.clientid) show_usage();
				no_clientid = 1;
				break;
			case 'j':
				rc_restart_flag = 1;
				break;
			case 'V':
				len = strlen(optarg) > 255 ? 255 : strlen(optarg);
				if (client_config.vendorclass) free(client_config.vendorclass);
				client_config.vendorclass = xmalloc(len + 2);
				client_config.vendorclass[OPT_CODE] = DHCP_VENDOR;
				client_config.vendorclass[OPT_LEN] = len;
				strncpy(client_config.vendorclass + OPT_DATA, optarg, len);
				break;
			case 'f':
				client_config.foreground = 1;
				break;
			case 'b':
				client_config.background_if_no_lease = 1;
				break;
			case 'h':
			case 'H':
				len = strlen(optarg) > 255 ? 255 : strlen(optarg);
				if (client_config.hostname) free(client_config.hostname);
				client_config.hostname = xmalloc(len + 2);
				client_config.hostname[OPT_CODE] = DHCP_HOST_NAME;
				client_config.hostname[OPT_LEN] = len;
				strncpy(client_config.hostname + 2, optarg, len);
				break;
			case 'F':
				len = strlen(optarg) > 255 ? 255 : strlen(optarg);
				if (client_config.fqdn) free(client_config.fqdn);
				client_config.fqdn = xmalloc(len + 5);
				client_config.fqdn[OPT_CODE] = DHCP_FQDN;
				client_config.fqdn[OPT_LEN] = len + 3;
				/* Flags: 0000NEOS
				S: 1 => Client requests Server to update A RR in DNS as well as PTR
				O: 1 => Server indicates to client that DNS has been updated regardless
				E: 1 => Name data is DNS format, i.e. <4>host<6>domain<4>com<0> not "host.domain.com"
				N: 1 => Client requests Server to not update DNS
				*/
				client_config.fqdn[OPT_LEN + 1] = 0x1;
				client_config.fqdn[OPT_LEN + 2] = 0;
				client_config.fqdn[OPT_LEN + 3] = 0;
				strncpy(client_config.fqdn + 5, optarg, len);
				break;
			case 'i':
				client_config.interface =  optarg;
				break;
			case 'w':
				client_config.wan_proto =  optarg;	
				break;
	#ifdef UDHCPD_NETBIOS
			case 'N':
				client_config.netbios_enable = 1;
				break;
	#endif
	#ifdef RPPPOE
			case 'R':
				client_config.russia_enable = 1;
				break;
	#endif
	#ifdef CONFIG_BRIDGE_MODE
			case 'a':
				client_config.ap_mode = 1;
				break;
/*	Date: 2009-04-10
	*	Name: Ken Chiang
	*	Reason:	Add support for enable auto mode select(router mode/ap mode).
	*	Note: 
*/		
	#ifdef AUTO_MODE_SELECT	
			case 'A':				
				client_config.auto_mode_select = 1;
				//LOG(LOG_INFO, "Enable mode select =%x", client_config.auto_mode_select);
				break;
	#endif
	#endif	
			case 'n':
				client_config.abort_if_no_lease = 1;
				break;
			case 'p':
				client_config.pidfile = optarg;
				break;
			case 'q':
				client_config.quit_after_lease = 1;
				break;
// 2011.03.18
			case 'z':
				client_config.tr069_mode = 1;
				break;

			case 'r':
				requested_ip = inet_addr(optarg);
				break;
			case 's':
				client_config.script = optarg;
				break;
	#ifdef UDHCPC_UNICAST
			case 'u':
				use_unicasting = 1;
				break;
	#endif			
			case 'v':
				printf("udhcpcd, version %s\n\n", VERSION);
				return 0;
				break;
			default:
				show_usage();
		}
	}

	/* Start the log, sanitize fd's, and write a pid file */
	start_log_and_pid("udhcpc", client_config.pidfile);
	LOG(LOG_INFO,"DHCP client start.");
	if (read_interface(client_config.interface, &client_config.ifindex,
			   NULL, client_config.arp) < 0)
		return 1;

	/* if not set, and not suppressed, setup the default client ID */
	if (!client_config.clientid && !no_clientid) {
		client_config.clientid = xmalloc(6 + 3);
		client_config.clientid[OPT_CODE] = DHCP_CLIENT_ID;
		client_config.clientid[OPT_LEN] = 7;
		client_config.clientid[OPT_DATA] = 1;
		memcpy(client_config.clientid + 3, client_config.arp, 6);
	}

// 2011.03.18
	if (client_config.tr069_mode) {
		client_config.tr069id = xmalloc(6 + 12);
		client_config.tr069id[OPT_CODE] = 60;
		client_config.tr069id[OPT_LEN] = 12;
		memcpy(client_config.tr069id + 2, "dslforum.org", 12);
	}


	if (!client_config.vendorclass) {
		client_config.vendorclass = xmalloc(sizeof("udhcp "VERSION) + 2);
		client_config.vendorclass[OPT_CODE] = DHCP_VENDOR;
		client_config.vendorclass[OPT_LEN] = sizeof("udhcp "VERSION) - 1;
		client_config.vendorclass[OPT_DATA] = 1;
		memcpy(&client_config.vendorclass[OPT_DATA], 
			"udhcp "VERSION, sizeof("udhcp "VERSION) - 1);
	}


	/* setup the signal pipe */
	udhcp_sp_setup();

	state = INIT_SELECTING;
	run_script(NULL, "deconfig");
	change_mode(LISTEN_RAW);

	for (;;) {

		tv.tv_sec = timeout - uptime();
		tv.tv_usec = 0;

		if (listen_mode != LISTEN_NONE && fd < 0) {
			if (listen_mode == LISTEN_KERNEL)
				fd = listen_socket(INADDR_ANY, CLIENT_PORT, client_config.interface);
			else
				fd = raw_socket(client_config.ifindex);
			if (fd < 0) {
				LOG(LOG_ERR, "FATAL: couldn't listen on socket, %m");
				return 0;
			}
		}
		max_fd = udhcp_sp_fd_set(&rfds, fd);

		if (tv.tv_sec > 0) {
			DEBUG(LOG_INFO, "Waiting on select...");
			retval = select(max_fd + 1, &rfds, NULL, NULL, &tv);
		} else retval = 0; /* If we already timed out, fall through */

		now = uptime();
		if (retval == 0) {
			/* timeout dropped to zero */
			switch (state) {
			case INIT_SELECTING:
				if (packet_num < 3) {
					if (packet_num == 0)
						xid = random_xid();

					/*NickChou,071126, before sending discover packet, 
						set WAN IP = 0.0.0.0 for wantimer detecting*/
					if((strcmp(client_config.wan_proto, "dhcpc") == 0) 
#ifdef CONFIG_BRIDGE_MODE
					&& (client_config.ap_mode == 0) 
#endif
					)
					{	
						char cmd[24];
						memset(cmd,0,sizeof(cmd));
						//LOG(LOG_INFO, "Set DHCP WAN IP address = 0.0.0.0");
						sprintf(cmd,"ifconfig %s 0.0.0.0", client_config.interface);
						_system(cmd);
					} 
					
					/* send discover packet */
					send_discover(xid, requested_ip); /* broadcast */

					timeout = now + ((packet_num == 2) ? 4 : 2);
					packet_num++;
				} else {
					run_script(NULL, "leasefail");
					if (client_config.background_if_no_lease) {
						LOG(LOG_DEBUG, "No lease, forking to background.");
						client_background();
					} else if (client_config.abort_if_no_lease) {
						LOG(LOG_DEBUG, "No lease, failing.");
						return 1;
				  	}
					/* wait to try again */
					packet_num = 0;
					timeout = now + 60;
				}
				break;
			case RENEW_REQUESTED:
			case REQUESTING:
				if (packet_num < 3) {
					/* send request packet */
					if (state == RENEW_REQUESTED)
						send_renew(xid, server_addr, requested_ip); /* unicast */
					else 
						send_selecting(xid, server_addr, requested_ip); /* broadcast */

					timeout = now + ((packet_num == 2) ? 10 : 2);
					packet_num++;
				} else {
					/* timed out, go back to init state */
					if (state == RENEW_REQUESTED) 
						run_script(NULL, "deconfig");
					state = INIT_SELECTING;
					timeout = now;
					packet_num = 0;
					//To pass cdrouter test item
					//test item:cdrouter_dhcp_20
					change_mode(LISTEN_RAW);
				}
				break;
			case BOUND:
				/* Lease is starting to run out, time to enter renewing state */
				state = RENEWING;
				change_mode(LISTEN_KERNEL);
				DEBUG(LOG_INFO, "Entering renew state");
				/* fall right through */
			case RENEWING:
				/* Either set a new T1, or enter REBINDING state */
				if ((t2 - t1) <= (lease / 14400 + 1)) {
					/* timed out, enter rebinding state */
					state = REBINDING;
					timeout = now + (t2 - t1);
					DEBUG(LOG_INFO, "Entering rebinding state");
				} else {
					/* send a request packet */
					send_renew(xid, server_addr, requested_ip); /* unicast */
					t1 = (t2 - t1) / 2 + t1;
					timeout = t1 + start;
					//change_mode(LISTEN_RAW);
				}
				break;
			case REBINDING:
				/* Either set a new T2, or enter INIT state */
				if ((lease - t2) <= (lease / 14400 + 1)) {
					/* timed out, enter init state */
					state = INIT_SELECTING;
					LOG(LOG_DEBUG, "Lease lost, entering init state");
					run_script(NULL, "deconfig");
					timeout = now;
					packet_num = 0;
					change_mode(LISTEN_RAW);
				} else {
					/* send a request packet */
					send_renew(xid, 0, requested_ip); /* broadcast */

					t2 = (lease - t2) / 2 + t2;
					timeout = t2 + start;
				}
				break;
			case RELEASED:
				/* yah, I know, *you* say it would never happen */
				timeout = 0x7fffffff;
				break;
			}
		} else if (retval > 0 && listen_mode != LISTEN_NONE && FD_ISSET(fd, &rfds)) {
			/* a packet is ready, read it */

			if (listen_mode == LISTEN_KERNEL)
				len = get_packet(&packet, fd);
			else len = get_raw_packet(&packet, fd);

			if (len == -1 && errno != EINTR) {
				DEBUG(LOG_INFO, "error on read, %m, reopening socket");
				change_mode(listen_mode); /* just close and reopen */
			}
			if (len < 0)
				continue;	
			
			if (packet.xid != xid) {
				DEBUG(LOG_INFO, "Ignoring XID %lx (our xid is %lx)",
					(unsigned long) packet.xid, xid);
				continue;
			}
			/* Ignore packets that aren't for us */
			if (memcmp(client_config.arp,packet.chaddr,6))
				continue;

			if ((message = get_option(&packet, DHCP_MESSAGE_TYPE)) == NULL) {
				DEBUG(LOG_ERR, "couldnt get option from packet -- ignoring");
				continue;
			}

			switch (state) {
			case INIT_SELECTING:
				/* Must be a DHCPOFFER to one of our xid's */
				if (*message == DHCPOFFER) 
				{
					
// add 2011.11.17
					if ((temp = get_option(&packet, DHCP_DNS_SERVER))) 
					{
						static unsigned long dns_addr;
						//memcpy(&dns_addr, temp, 4);
						int len;
						
						fp1=fopen("/var/tmp/resolv.txt","w");	
						if (fp1 == NULL)
							LOG(LOG_ERR, "open resolv.txt fail \n");
                        else
                        {				
							len = *(unsigned char *)(temp-1);
	
                        	//fprintf(fp1,"%d\n",len);
                        
                        	while(len > 0) {
								memcpy(&dns_addr, temp, 4);
							    temp_addr.s_addr = dns_addr;

                        		fprintf(fp1,"nameserver %s\n",inet_ntoa(temp_addr));
                        		temp += 4; // usnigned char 
                        		len -= 4;
                        	}	
                        	
                        	//fprintf(fp1,"%02x %02x %02x %02x %02x\n",temp[0],temp[1],temp[2],temp[3],temp[4]);	


                        	fclose(fp1);
                        }	
	
					
					}

// 2011.03.15 ACS URL option 43 issue
					if ((temp = get_option(&packet, 43))) 
					{
						static unsigned long dns_addr;
						//memcpy(&dns_addr, temp, 4);
						int len;
						
						fp1=fopen("/var/tmp/acs.txt","w+");	
						if (fp1 == NULL)
							LOG(LOG_ERR, "open acs.txt fail \n");
                        else
                        {				
							len = *(unsigned char *)(temp-1);

							// temp is id/len/value format
	
                        	//fprintf(fp1,"%d\n",len);
                        	acs_url_process(fp1,temp,len);


                        	fclose(fp1);
                        }	
	
					
					}
					
					
					if ((temp = get_option(&packet, DHCP_SERVER_ID))) 
					{
						memcpy(&server_addr, temp, 4);
						xid = packet.xid;
						requested_ip = packet.yiaddr;

						/* enter requesting state */
						state = REQUESTING;
						timeout = now;
						packet_num = 0;
					}
					else
						DEBUG(LOG_ERR, "No server ID in message");
						
					/*NickChou 2008.1.22 
					  for RUSSIA PPTP/L2TP or PPTP/L2TP in multi-router environment
					*/	
					if ((temp = get_option(&packet, DHCP_ROUTER))) 
					{
						static unsigned long gateway_addr;
						memcpy(&gateway_addr, temp, 4);
						fp1=fopen("/var/tmp/dhcp_gateway.txt","w");	
						if (fp1 == NULL)
							LOG(LOG_ERR, "open dhcp_gateway.txt fail \n");
                        else
                        {						
						    temp_addr.s_addr = gateway_addr;
                            LOG(LOG_DEBUG, "DHCPC get gateway = %s", inet_ntoa(temp_addr));
						    fwrite(inet_ntoa(temp_addr), 1, 15, fp1);
						    fclose(fp1);
					}
				}
					else
						DEBUG(LOG_ERR, "No Gateway in message");
				}
				break;
			case RENEW_REQUESTED:
			case REQUESTING:
			case RENEWING:
			case REBINDING:
				if (*message == DHCPACK) {
					if (!(temp = get_option(&packet, DHCP_LEASE_TIME))) {
						LOG(LOG_ERR, "No lease time with ACK, using 1 hour lease");
						lease = 60 * 60;
					} else {
						memcpy(&lease, temp, 4);
						lease = ntohl(lease);
					}


// 2011.03.24 ACS URL option 43 issue & consider ACK issue
					if ((temp = get_option(&packet, 43))) 
					{
						static unsigned long dns_addr;
						//memcpy(&dns_addr, temp, 4);
						int len;
						
						fp1=fopen("/var/tmp/acs.txt","w+");	
						if (fp1 == NULL)
							LOG(LOG_ERR, "open acs.txt fail \n");
                        else
                        {				
							len = *(unsigned char *)(temp-1);

							// temp is id/len/value format
	
                        	//fprintf(fp1,"%d\n",len);
                        	acs_url_process(fp1,temp,len);


                        	fclose(fp1);
                        }	
	
					
					}


					/* enter bound state */
					t1 = lease / 2;

					/* little fixed point for n * .875 */
					t2 = (lease * 0x7) >> 3;
					temp_addr.s_addr = packet.yiaddr;
					LOG(LOG_INFO, "Lease of %s obtained, lease time %ld",
						inet_ntoa(temp_addr), lease);
												
					/* save lease value for dhcp client */				
					save_dhcpc_lease_time(lease);					
					
					start = now;
					timeout = t1 + start;
					requested_ip = packet.yiaddr;
#ifdef CONFIG_BRIDGE_MODE					
/*	Date: 2009-04-10
	*	Name: Ken Chiang
	*	Reason:	Add support for enable auto mode select(router mode/ap mode).
	*	Note: 
*/		
#ifdef AUTO_MODE_SELECT	
					if( (strcmp(client_config.wan_proto, "dhcpc") == 0) && (client_config.auto_mode_select) ){
						/*	Date: 2010-04-28
						*	Name: Jimmy Huang
						*	Reason:	Fixed the bug that mis-recognize private/public ip for 172.x.x.x
						*	Note: Follow by Ken Chiang's modification
						*/
						ip_a = ((unsigned char *)(&packet.yiaddr))[0];
						ip_b = ((unsigned char *)(&packet.yiaddr))[1];
						if( (ip_a == 10) || (ip_a == 192 && ip_b == 168) || (ip_a == 172 && ip_b >= 16 && ip_b <= 31) ){// 10.X.X.X,192.168.X.X,172.16-31.X.x
						/*
						previous_ip = (packet.yiaddr & inet_addr("255.255.0.0"));
						LOG(LOG_INFO, "Lease of %x obtained previous_ip", previous_ip);							
						if( ((previous_ip & 0xff000000) == 0x0a000000 ) || ((previous_ip & 0xffff0000) == 0xc0a80000) || ((previous_ip & 0xffff0000) == (0xac100000 & 0xfff00000)) ){// 10.X.X.X,192.168.X.X,172.16-31.X.x
						*/
							LOG(LOG_INFO, "dhcpc get private ip %u.%u.%u.%u"
										,((unsigned char *)(&packet.yiaddr))[0],((unsigned char *)(&packet.yiaddr))[1]
										,((unsigned char *)(&packet.yiaddr))[2],((unsigned char *)(&packet.yiaddr))[3]
										);
							if(!client_config.ap_mode){
								LOG(LOG_INFO, "To change router to AP mode");
								system("nvram set wlan0_mode=ap");
								system("nvram set wan_proto=static");																								
								system("nvram set wan_static_ipaddr=192.168.0.50");
								system("nvram set wan_static_netmask=255.255.255.0");
								system("nvram set wan_static_gateway=0.0.0.0");
								system("nvram set wan_primary_dns=0.0.0.0");
								system("nvram set wan_secondary_dns=0.0.0.0");											
								//_system("nvram set lan_ipaddr=192.168.0.50");
								//_system("nvram set lan_netmask=255.255.255.0");																
								system("nvram commit");
								//sleep(1);								
								//kill(read_pid(RC_PID), SIGHUP);
								system("reboot -d2 &");
							}	
						}
						else{
							LOG(LOG_INFO, "dhcpc get public ip %u.%u.%u.%u"
										,((unsigned char *)(&packet.yiaddr))[0],((unsigned char *)(&packet.yiaddr))[1]
										,((unsigned char *)(&packet.yiaddr))[2],((unsigned char *)(&packet.yiaddr))[3]
										);
							if(client_config.ap_mode){
								LOG(LOG_INFO, "To change router to RT mode");
								_system("nvram set wlan0_mode=rt");
								_system("nvram set wan_proto=dhcpc");
								_system("nvram set wan_static_ipaddr=0.0.0.0");	
								_system("nvram set wan_static_netmask=0.0.0.0");	
								_system("nvram set wan_static_gateway=0.0.0.0");
								_system("nvram set wan_primary_dns=0.0.0.0");
								_system("nvram set wan_secondary_dns=0.0.0.0");	
/*	Date: 2010-01-06
*	Name: Ken Chiang
*	Reason:	fixed swap to AP mode will clean dns setting but not disable advance dns service and wan_specify_dns
*           then swap to Router mode will lose dns setting issue.
*	Note: 
*/									
								_system("nvram set wan_specify_dns=0");	
								_system("nvram set opendns_enable=0");					
								//_system("nvram set lan_ipaddr=192.168.0.1");
								//_system("nvram set lan_netmask=255.255.255.0");
								_system("nvram commit");
								//sleep(1);								
								//kill(read_pid(RC_PID), SIGHUP);
								_system("reboot -d2 &");
							}	
						}							
					}
#endif	
#endif						
					/*NickChou 2008.02.27, prevent udhcpc modify default gateway more than once*/
	/* Jackey Chen 2010.03.16
	   Russia PPPoE dns fail, DNS will rewrite to DHCP after PPPoE got IP
	 */
					if(	strcmp(client_config.wan_proto, "pptp") == 0 ||
						strcmp(client_config.wan_proto, "l2tp") == 0 ||
						strcmp(client_config.wan_proto, "pppoe") == 0 )
					{
						/*
						fp_dr = fopen("/var/run/ppp-pptp.pid", "r");
						if(fp_dr)
						{
							fclose(fp_dr);
							LOG(LOG_INFO, "WAN proto=%s => udhcpc don't need to change default gateway again\n", client_config.wan_proto);	
						}
						*/
						/*  Date: 2009-01-09
						*   Name: Cosmo Chang
						*   Reason: dhcpc will rewrite dns in the l2tp-mode while release-time timeout
						*   Note:
						*/
						fp_dr_pptp = fopen("/var/run/ppp-pptp.pid", "r");
						fp_dr_l2tp = fopen("/var/run/ppp0.pid", "r");
						
						if(fp_dr_pptp || fp_dr_l2tp)
						{
							if(fp_dr_pptp)
								fclose(fp_dr_pptp);
							if(fp_dr_l2tp)
								fclose(fp_dr_l2tp);
							LOG(LOG_INFO, "WAN proto=%s => udhcpc don't need to change default gateway again\n", client_config.wan_proto);	
						}
						else
							run_script(&packet,
								((state == RENEWING || state == REBINDING) ? "renew" : "bound"));	
					}
					else						
					run_script(&packet,
						   ((state == RENEWING || state == REBINDING) ? "renew" : "bound"));

					/* jimmy added, 20080429 */
					/* clean Nvram Records if needed */
					//Clean_Nvram_Route();//jimmy marked 20080520

					/* handle option 33 if needed */
					OptionStaticRoute(&packet);

					/* handle option 121 if needed */
					OptionClasslessStaticRoute(&packet);

					/* 20080429, handle option 249 if needed */
					OptionMicroSoftClasslessStaticRoute(&packet);

#ifdef IPV6_6RD
					/*Joe Huang : Handle option 212 for IPv6 Spec 1.13 */
					Option6RD(&packet);
#endif
					state = BOUND;
					change_mode(LISTEN_NONE);
					if (client_config.quit_after_lease)
						return 0;
					if (!client_config.foreground)
						client_background();

					if ((temp = get_option(&packet, DHCP_DOMAIN_NAME))) {
						 save_dns_name(temp);
					}

					/*NickChou add client_config.wan_proto 07.05.28*/
#ifdef CONFIG_VLAN_ROUTER					

/*	ChaseCheng modified 2009.4.16 
 *	Reason: 1. Write static route after re-connect.									
 */
//					//if IP don't change, not restart rc			
//#ifdef UDHCPD_NETBIOS
//					if( netbios_changed || /* jimmy added 20080718, for checking NetBios info changed */
//							((previous_ip != packet.yiaddr) && 
//#else
//					if( (previous_ip != packet.yiaddr) &&
//#endif
					if(	
#ifdef UDHCPD_NETBIOS
					   netbios_changed || /* jimmy added 20080718, for checking NetBios info changed */
					   ( 
#endif
#ifndef IGNORE_IP_CONTRAST
						//if IP don't change, not restart rc
					  		(previous_ip != packet.yiaddr) &&
#endif
							(    (strcmp(client_config.wan_proto, "dhcpc") == 0) 
						    || (strcmp(client_config.wan_proto, "static") == 0)
#ifdef RPPPOE
							|| ( strcmp(client_config.wan_proto, "pppoe") == 0 && client_config.russia_enable )
							|| ( strcmp(client_config.wan_proto, "pptp") == 0 && client_config.russia_enable ) 
							|| ( strcmp(client_config.wan_proto, "l2tp") == 0 && client_config.russia_enable )
#endif	
							)
						)
#ifdef UDHCPD_NETBIOS
					  ) 	
#endif
					{
					/* jimmy added for checking netbios info */
#ifdef UDHCPD_NETBIOS
						netbios_changed = 0;
#endif
					/* ------------------------------------- */
						/* Chun:
						 * a) In russia PPPoE/russia PPTP/L2TP with WAN_PHY dynamic IP mode, we nned to restart rc 
						 * once WAN_PHY interface obtains IP.
						 * b) Before dhcpd sends signal to rc, we have to make sure rc is NOT restarting NOW.
	 					 * If rc flag is set to busy, dhcpd will wait for 2 seconds and query rc again.
	 					 * Waiting interval prevents dhcpd from consuming CPU resource.  
	 					 * c) If sleep_count > 10 times, dhcpd will break the loop.
						 */
						while(!send_signal_done)
						{
							fp = fopen(RC_FLAG_FILE,"r");
							if(fp)
							{
								LOG(LOG_DEBUG, "dhcpc is waiting for rc to be idle.");
								fread(data, 1, sizeof(data), fp);
								if(!strncmp(data, "i", 1))
								{
									LOG(LOG_DEBUG, "dhcpc sent restart signal  to rc (RESTART APP)");
									kill(read_pid(RC_PID), SIGPIPE);
									send_signal_done = 1;
									break;
								}
								fclose(fp);	
							}
							else
							{
								LOG(LOG_DEBUG, "dhcpc open %s failed",RC_FLAG_FILE);
								send_signal_done = 1;
							}
							
							sleep(2); 
								
							if(sleep_count>15)
								send_signal_done = 1;
							else
								sleep_count++;
						}	
						send_signal_done = 0;		
#ifdef CONFIG_USER_TC								
					}else{
						/* Chun :If IP are the same, we copy the old bandwidth file.*/
						
						fp_tmp = fopen("/var/tmp/bandwidth_tmp.txt","r");
						if (fp_tmp)
						{
							fread(value, sizeof(value), 1, fp_tmp);
							fclose(fp_tmp);
						}
						else
						{
							strcpy(value,"0");
							//LOG(LOG_DEBUG,"open bandwidth_tmp.txt fail");	
						}
						/*	Date:	2010-03-29
						*	Name:	Jimmy Huang
						*	Reason:	Only when we have a valueable value, we keep the qos value,
						*			otherwise, we let it re-calulate
						*	Note:	If we do not do this, after booting, Qos sometimes will not start to measure
						*			
						*/
						if(atoi(value) > 0){
							fp_bandwidth = fopen("/var/tmp/bandwidth_result.txt","w");
							if(fp_bandwidth)
							{
								if(fputs(value,fp_bandwidth)!= EOF)//kbps
									LOG(LOG_DEBUG,"write the bandwidth successfully!");
								else
									LOG(LOG_DEBUG,"write the bandwidth fail!");
											
								fclose(fp_bandwidth);	
							}
							else
							{
								LOG(LOG_DEBUG,"open bandwidth_result.txt fail");
							}
						}	
#endif
					}
					/* jimmy added 20080213 for remember last valid IP */
					previous_ip = packet.yiaddr;
#endif //#ifdef CONFIG_VLAN_ROUTER							
					if(rc_restart_flag)
						system("killall rc");
				} else if (*message == DHCPNAK) {
					/* return to init state */
					LOG(LOG_DEBUG, "Received DHCP NAK");
					run_script(&packet, "nak");
					if (state != REQUESTING)
						run_script(NULL, "deconfig");
					state = INIT_SELECTING;
					timeout = now;
					requested_ip = 0;
					packet_num = 0;
					change_mode(LISTEN_RAW);
					sleep(3); /* avoid excessive network traffic */
				}
				break;
			/* case BOUND, RELEASED: - ignore all packets */
			}
		} else if (retval > 0 && (sig = udhcp_sp_read(&rfds))) {
			if(sig == SIGUSR1)
			{
				LOG(LOG_DEBUG, "DHCPC Received SIGUSR1=>DHCPC Renew");
				perform_renew();
			}
			else if(sig == SIGUSR2)
			{
				LOG(LOG_DEBUG, "DHCPC Received SIGUSR2=>DHCPC Release");
				perform_release();
			}
			else if(sig == SIGALRM)
			{
				LOG(LOG_DEBUG, "Received SIGALRM");
				printf("recv SIGALRM\n");
				dhcp_discover();
			}
			else if(sig == SIGTERM)
			{
				LOG(LOG_DEBUG, "Received SIGTERM");
				return 0;				
			}				
						
		} else if (retval == -1 && errno == EINTR) {
			/* a signal was caught */
		} else {
			/* An error occured */
			DEBUG(LOG_ERR, "Error on select");
		}

	}
	return 0;
}
