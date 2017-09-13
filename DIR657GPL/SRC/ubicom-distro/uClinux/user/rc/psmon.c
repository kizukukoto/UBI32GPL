/*
 *	Process Monitor
 * $Id: psmon.c,v 1.4 2008/02/20 09:36:59 NickChou Exp $
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <syslog.h>															
#include <ctype.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sutil.h>
#include <nvram.h>
#include <shvar.h>
#include <app.h>
#include <rc.h>

#ifdef RC_DEBUG
#define DEBUG_MSG(fmt, arg...)       printf("%s [%d]: "fmt, __FILE__, __LINE__, ##arg)
#else
#define DEBUG_MSG(fmt, arg...) 
#endif

extern struct demand AppServices[];
char WANIP[16] = "0.0.0.0";
int wanDisconnected = 0;

/* Chun comment out: 
 * we don't need psmin_upnp anymore. When router obtains wan_ip, it will perform rc restart,
 * which will restart miniupnpd as well. There is no need to monitor upnp.
 */
//int psmon_upnp(void)
//{
//	char wan_eth[6], *wan_proto;
//	static char *wan_ip_flag;
//
//	wan_proto = nvram_safe_get("wan_proto");
//	if( !strcmp(wan_proto, "dhcpc") || !strcmp(wan_proto, "static") )
//		strcpy(wan_eth, nvram_safe_get("wan_eth") );
//	else {
//		/* not support multi pppoe */
//		strcpy(wan_eth, "ppp0");
//	}
//
//
//	/*	SIGUSR1: WAN disconnect/connect
//		SIGUSR2: WAN ip changed  */
//	if( !wan_connected_cofirm(wan_eth) && !wanDisconnected) {
//		KILL_APP_SYNCH("-SIGUSR1 upnpd");
//		wanDisconnected = 1;
//		strcpy(WANIP, "0.0.0.0");
//		return 1;
//	} 
//
//	wanDisconnected = !wan_connected_cofirm(wan_eth);	
//
//	/* if connected, check ip changed */
//	if(  !wanDisconnected ) {
//		wan_ip_flag = get_ipaddr(wan_eth);
//		if( strcmp(WANIP, wan_ip_flag) !=0) {
//			if( !strcmp(WANIP, "0.0.0.0") )
//				KILL_APP_SYNCH("-SIGUSR1 upnpd");
//			else
//				KILL_APP_SYNCH("-SIGUSR2 upnpd");
//			
//			strcpy(WANIP, wan_ip_flag);
//			return 1;
//		}
//	}
//
//	return 0;
//}

int usage(void)
{
	fprintf(stderr, "usage: psmon [sec]\n");
	return 0;
}


int psmon_main(int argc, char** argv)
{
	struct demand *d;

	if( !argv[1] ) {
		usage();
		return -1;
	}

	while(1) {

#ifdef __uClinux__
		if (vfork()) exit(0);
#else
		if (fork()) exit(0);
#endif
		
		sleep(atoi(argv[1]));
		for(d=AppServices; d->name ; d++) {
			if(d->psmon) {
				if( checkServiceAlivebyName(d->name) ) {
					d->psmon();
				}
			}
		}

	}


	return 0;
}
 
int lanmon_main(int argc, char** argv)
{
     char *curr_ipaddr;

	if( !argv[1] ) {
		usage();
		return -1;
	}

	while(1) {

#ifdef __uClinux__
		if (vfork()) exit(0);
#else
		if (fork()) exit(0);
#endif
		
		sleep(atoi(argv[1]));
		
		curr_ipaddr = get_ipaddr(argv[3]);
		if(curr_ipaddr != NULL && strcmp(argv[2], curr_ipaddr) != 0)
			{
				nvram_set("lan_ipaddr", curr_ipaddr);
				nvram_commit();
				/*NickChou 07.05.31 move to start_lan()*/
				//kill(read_pid(HTTPD_PID), SIGHUP);
				//_system("httpd &");
				printf("lanmon_main: rc restart\n");
				kill(read_pid(RC_PID), SIGHUP);
			}	
		} 
     
	
}
 
