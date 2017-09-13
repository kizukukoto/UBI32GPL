#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <net/if.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if_arp.h>
#include <errno.h>
#include <fcntl.h>
#include <strings.h>
#include <signal.h>
#include "cmds.h"
#include "shutils.h"
#include <nvramcmd.h>
#include "ipv6.h"

int ipv6_wan_proto;
char *wan_eth;
char *lan_bridge;

/*XXX : Config IPv6 By each mode (Joe Huang)    */

int ipv6_start_main(int argc, char *argv[])
{
	ipv6_wan_proto = check_ipv6_wan_proto();
	wan_eth = nvram_safe_get("wan_eth");
	lan_bridge = nvram_safe_get("lan_bridge");

        system("echo 1 > /proc/sys/net/ipv6/conf/all/forwarding");
        _system("echo 86400 > /proc/sys/net/ipv6/neigh/%s/gc_stale_time", lan_bridge);

	if(access(RESOLVFILE_IPV6, F_OK) != 0)
        	init_file(RESOLVFILE_IPV6);
	if(access(RESOLVFILE_DUAL, F_OK) != 0)
		init_file(RESOLVFILE_DUAL);

        if( NVRAM_MATCH("ipv6_wan_specify_dns", "1") && ipv6_wan_proto != LINKLOCAL )
		set_ipv6_specify_dns();
        
	_system("cat %s %s > %s", RESOLVFILE, RESOLVFILE_IPV6, RESOLVFILE_DUAL);

	switch(ipv6_wan_proto){
	case AUTODETECT:
		if(!NVRAM_MATCH("wan_proto","pppoe")) 
		_system("cli ipv6 rdnssd start %s", wan_eth);
		if(NVRAM_MATCH("ipv6_dhcp_pd_enable", "0") &&
			strlen(nvram_safe_get("ipv6_autodetect_lan_ip")) != 0){
			_system("ip -6 addr add %s/64 dev %s",
			nvram_safe_get("ipv6_autodetect_lan_ip"),
			nvram_safe_get("lan_bridge"));
			system("cli ipv6 dhcp6d start");
			system("cli ipv6 radvd start");
		}
		break;

	case STATIC:
		if(NVRAM_MATCH("ipv6_use_link_local", "0"))
	                _system("ip -6 addr add %s/%s dev %s", 
				nvram_safe_get("ipv6_static_wan_ip"),
				nvram_safe_get("ipv6_static_prefix_length"), wan_eth);
                _system("route -A inet6 add default gw %s dev %s", 
			nvram_safe_get("ipv6_static_default_gw"), wan_eth);
		if(strlen(nvram_safe_get("ipv6_static_lan_ip")) != 0){
			_system("ip -6 addr add %s/64 dev %s", 
				nvram_safe_get("ipv6_static_lan_ip"), 
				nvram_safe_get("lan_bridge"));
			system("cli ipv6 dhcp6d start");
			system("cli ipv6 radvd start");
		}
		break;
	
	case AUTOCONFIG:
		_system("cli ipv6 rdnssd start %s", wan_eth);
		if(NVRAM_MATCH("ipv6_dhcp_pd_enable", "0") && 
			strlen(nvram_safe_get("ipv6_autoconfig_lan_ip")) != 0){
			_system("ip -6 addr add %s/64 dev %s",
				nvram_safe_get("ipv6_autoconfig_lan_ip"),
				nvram_safe_get("lan_bridge"));
			system("cli ipv6 dhcp6d start");
			system("cli ipv6 radvd start");
		}
		break;
	
	case PPPOE:
	/*      If dhcp-pd is on                        */
	/*      PPPoE do dhcp6c in pppd/ipv6cp.c        */
		system("cli ipv6 pppoe start");
		if( strlen(nvram_safe_get("ipv6_pppoe_lan_ip")) != 0 && 
			NVRAM_MATCH("ipv6_dhcp_pd_enable","0")){
			_system("ip -6 addr add %s/64 dev %s", 
				nvram_safe_get("ipv6_pppoe_lan_ip"), 
				nvram_safe_get("lan_bridge"));
			system("cli ipv6 dhcp6d start");
			system("cli ipv6 radvd start");
		}else if (NVRAM_MATCH("ipv6_pppoe_connect_mode", "on_demand") && 
			NVRAM_MATCH("ipv6_pppoe_share","0"))
			/*On demand should start radvd , because user need gateway to dial pppoe*/
			system("cli ipv6 radvd start");
		break;

	/*Joe Huang : Do 6in4 6to 6rd start in rc/app.c start_app()*/
	case _6TO4:
		break;

	case _6IN4:
		if(strlen(nvram_safe_get("ipv6_6in4_lan_ip")) != 0 && 
			NVRAM_MATCH("ipv6_dhcp_pd_enable","0")){
			_system("ip -6 addr add %s/64 dev %s", 
				nvram_safe_get("ipv6_6in4_lan_ip"), nvram_safe_get("lan_bridge"));
			system("cli ipv6 dhcp6d start");
			system("cli ipv6 radvd start");
		}
		break;

	case _6RD:
		break;

	case LINKLOCAL:
		break;
	}

	system("cli ipv6 firewall start");
	system("cli ipv6 route start");

	return 0;
}

int ipv6_stop_main(int argc, char *argv[])
{
	char ipv6_wan_proto_lan_ip[64] ={}; 
	ipv6_wan_proto = check_ipv6_wan_proto();
        wan_eth = nvram_safe_get("wan_eth");
	lan_bridge = nvram_safe_get("lan_bridge");
	set_default_accept_ra(0);
	if(NVRAM_MATCH("ipv6_dhcp_pd_enable", "1") &&
		(ipv6_wan_proto == AUTOCONFIG ||
		 ipv6_wan_proto == PPPOE ||
		 ipv6_wan_proto == _6IN4)){
		sprintf(ipv6_wan_proto_lan_ip, "%s_lan_ip", nvram_safe_get("ipv6_wan_proto"));
		nvram_set(ipv6_wan_proto_lan_ip, "");
	}
	system("cli ipv6 route stop");
	_system("ip -6 route flush ::/0 dev %s", wan_eth);
	_system("ip -6 addr flush scope global dev %s",lan_bridge);
        _system("ip -6 addr flush scope global dev %s",wan_eth);
        system("ip -6 addr flush scope global dev ath1");
	_system("ip -6 route flush root 2000::/3 dev %s", wan_eth);
	_system("ip -6 route flush root 2000::/3 dev %s", lan_bridge);
	system("ip -6 route flush root 2000::/3 dev lo");
	_system("ip -6 route flush proto kernel dev %s",wan_eth);
	_system("ip -6 neigh flush dev %s", lan_bridge);
	system("cli ipv6 firewall stop");

        init_file(RESOLVFILE_IPV6);
	_system("cat %s %s > %s", RESOLVFILE, RESOLVFILE_IPV6, RESOLVFILE_DUAL);

	if (access(SIT1_PID, F_OK) == 0)
		system("cli ipv6 6in4 stop");
	if (access(TUN6TO4_PID, F_OK) == 0)
		system("cli ipv6 6to4 stop");
	if (access(SIT2_PID, F_OK) == 0)
		system("cli ipv6 6rd stop");
	if (access(PPP6_PID, F_OK) == 0)
		system("cli ipv6 pppoe stop");
	if (access(RADVD_PID, F_OK) == 0)
		system("cli ipv6 radvd stop");
	if (access(RDNSSD_PID, F_OK) == 0)
		system("cli ipv6 rdnssd stop");
	if (access(DHCPD6_PID, F_OK) == 0)
		system("cli ipv6 dhcp6d stop");
	if (access(DHCP6C_PID, F_OK) == 0)
		system("cli ipv6 dhcp6c stop");
	if (access(MON6_PID, F_OK) == 0){
		system("killall mon6");
		unlink(MON6_PID);
	}
	if (access(RED6_PID, F_OK) == 0){
		system("killall red6");
		unlink(RED6_PID);
	}

	return 0;
}
