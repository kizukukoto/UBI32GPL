/*
 * $Id: lan.c,v 1.23 2008/11/28 09:17:31 jerry_shyu Exp $
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <sutil.h>
#include <nvram.h>
#include <shvar.h>
#include <rc.h>

#ifdef RC_DEBUG
#define DEBUG_MSG(fmt, arg...)       printf("%s [%d]: "fmt, __FILE__, __LINE__, ##arg)
#else
#define DEBUG_MSG(fmt, arg...) 
#endif

extern struct action_flag action_flags;

extern char http_bind_ipaddr[16];

int start_lan(void)
{
#ifdef	CONFIG_BRIDGE_MODE
	char *wlan0_mode="rt";
#endif	
	char *lan_bridge;

	if(!( action_flags.all  || action_flags.lan ) ) {
		//return 1;	
		goto startdhcpd;
	}
		
	DEBUG_MSG("Start LAN\n");
	lan_bridge = nvram_safe_get("lan_bridge");

	/* LAN eth */
	ifconfig_up( nvram_safe_get("lan_eth"), "0.0.0.0", "0.0.0.0" );
		
#ifdef	CONFIG_BRIDGE_MODE
	printf("start_lan():HAVE_BRIDGE_MODE\n");
	wlan0_mode = nvram_safe_get("wlan0_mode");

	if(strcmp(wlan0_mode, "ap") == 0)
	{
#ifndef AP_NOWAN_HASBRIDGE
		/*when DUT switch from rt to ap, bridge need to add wan_eth*/
		_system("brctl addif %s %s", lan_bridge, nvram_safe_get("wan_eth") );
        	if(nvram_match("wan_proto", "static") == 0)
			ifconfig_up( lan_bridge, nvram_safe_get("wan_static_ipaddr"), nvram_safe_get("wan_static_netmask") );
#endif

	}		
	else if(strcmp(wlan0_mode, "rt") == 0)/* Bridge setting */	
	{
		/*when DUT switch from ap to rt, bridge need to delete wan_eth*/
		_system("brctl delif %s %s", lan_bridge, nvram_safe_get("wan_eth") );
		ifconfig_up( lan_bridge, nvram_safe_get("lan_ipaddr"), nvram_safe_get("lan_netmask") );
	}
#else
	ifconfig_up( lan_bridge, nvram_safe_get("lan_ipaddr"), nvram_safe_get("lan_netmask") );	
#endif		
				
/* Chun comment out: httpd doesn't need to restart when ip is changed */	
#if 0	
	/*************** If LAN ip changed, restart httpd ********************************/
	
#ifdef	CONFIG_BRIDGE_MODE
	DEBUG_MSG("%s mode, http_bind_ipaddr=%s, lan_bridge=%s\n", 
		wlan0_mode, http_bind_ipaddr, get_ipaddr(lan_bridge));
#else
	DEBUG_MSG("http_bind_ipaddr=%s, lan_bridge=%s\n", 
		 http_bind_ipaddr, get_ipaddr(lan_bridge));		
#endif
		
	if(strcmp(http_bind_ipaddr, get_ipaddr(lan_bridge)) != 0 )
    {	
        DEBUG_MSG("http_bind_ipaddr changed, restart httpd\n");		
		strcpy(http_bind_ipaddr, get_ipaddr(lan_bridge));		
		kill(read_pid(HTTPD_PID), SIGHUP);
		_system("httpd &");
		sleep(1); //prevent httpd dead
	}
#endif	//#if 0

	DEBUG_MSG("Start LAN Finished\n");

	/* jimmy added 20080528 */
startdhcpd:
	if(action_flags.all || action_flags.wan || action_flags.lan || action_flags.app){
		start_dhcpd();
	}
	/* -------------------- */

#ifdef CONFIG_WEB_404_REDIRECT
	system("killall -SIGUSR2 httpd");
#endif
        char ipv6_lla[50] = {};

		 if ( read_ipv6addr(lan_bridge, SCOPE_LOCAL, ipv6_lla, sizeof(ipv6_lla))){
                _system("ip -6 addr flush dev %s", lan_bridge);
                _system("ip -6 addr add %s dev %s", ipv6_lla, lan_bridge);
						}
	return 0;
}



int stop_lan(void)
{
	if(!( action_flags.all  || action_flags.lan ) ) 
		return 1;	

	DEBUG_MSG("Stop LAN\n");

	/* change MAC */
#ifdef SET_GET_FROM_ARTBLOCK	
	if(artblock_get("lan_mac"))
		ifconfig_mac( nvram_get("lan_eth"), artblock_get("lan_mac") );
	else
		ifconfig_mac( nvram_get("lan_eth"), nvram_get("lan_mac") );	
#else
	ifconfig_mac( nvram_get("lan_eth"), nvram_get("lan_mac") );

#endif
	/* If LAN ip changed, restart httpd */
	
	DEBUG_MSG("Stop LAN Finished\n");
	/*	Marked by NickChou 070517, Move to star_lan()
	if(strcmp(http_bind_ipaddr, nvram_safe_get("lan_ipaddr")) != 0 ) 
		KILL_APP_SYNCH("httpd");
	*/
	return 0;	
}
