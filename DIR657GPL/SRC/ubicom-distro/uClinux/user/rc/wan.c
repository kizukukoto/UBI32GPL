/*
 *
 * $Id: wan.c,v 1.106 2009/08/06 03:00:46 NickChou Exp $
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <linux_vct.h>
#include <sutil.h>
#include <nvram.h>
#include <shvar.h>
#include <rc.h>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

extern struct action_flag action_flags;

#ifdef RC_DEBUG
#define DEBUG_MSG(fmt, arg...)       printf("%s [%d]: "fmt, __FILE__, __LINE__, ##arg)
#else
#define DEBUG_MSG(fmt, arg...)
#endif

#define dhcpc_renew()   KILL_APP_ASYNCH("-SIGUSR1 udhcpc")
#define dhcpc_release() KILL_APP_ASYNCH("-SIGUSR2 udhcpc")
#define CHAP_REMOTE_NAME        "CAMEO"
#ifdef CONFIG_USER_TC
#define HEADER_LENGTH   54
#define TEST_COUNT      5
#endif

/*      Date: 2009-2-1
*       Name: Nick Chou
*       Reason: check 3G USB Device Type
*       Note:
*/
#ifdef CONFIG_USER_3G_USB_CLIENT
usb_3g_device_table_t usb_3g_device_table[]={
        {"1186","3e04","Dlink","DWM-652"},
        {"12d1","1003","HUAWEI", "E220"},
	{"07d1","3e01","QUANTA","DWM-152"},
	{"07d1","3e02","QUANTA","DWM-156"},	
	{"07d1","7e11","QUANTA","DWM-152_156_A3"},	
        {"NULL","NULL","NULL","NULL"}
};

#if defined(CONFIG_USER_3G_USB_CLIENT_WM5) || defined(CONFIG_USER_3G_USB_CLIENT_IPHONE)
int start_usb_mobile_phone(int phone_type);
#endif

#endif

char * modify_unnumbered_ip(void)
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

        if((tmp_fourth == 1) || (tmp_fourth == 255))
                return "ip error";

        tmp_fourth = tmp_fourth - 1;

        for(i = 0; i < 4; i++){
                if(i < 3){
                        sprintf(lan_ipaddr, "%s%s%s", lan_ipaddr, ipaddr[i], ".");
                }else
                        sprintf(lan_ipaddr, "%s%d", lan_ipaddr, tmp_fourth);
        }

        return lan_ipaddr;
}

int modify_l2tp_conf(char *l2tp_ip)
{
        init_file(L2TP_CONF);
        save2file(L2TP_CONF,
                        "global\n"
                        "load-handler \"sync-pppd.so\"\n"
                        "load-handler \"cmd.so\"\n"
                        "listen-port 1701\n"
                        "section sync-pppd\n"
                        "pppd-path /sbin/pppd\n"
                        "section peer\n"
                        "peer %s\n"
                        "port 1701\n"
                        "lac-handler sync-pppd\n"
                        "hide-avps no\n"
                        "section cmd\n",
                        l2tp_ip/*nvram_get("wan_l2tp_server_ip")*/ );

        return 0;
}

int modify_sercets(char *file)
{
        int i=0;
        char username[] = "wan_pppoe_username_XX";
        char password[] = "wan_pppoe_password_XX";
        char *wan_proto = nvram_safe_get("wan_proto");

        init_file(file);

        if(strcmp(wan_proto, "pppoe") == 0 ) {
                for(i=0; i<MAX_PPPOE_CONNECTION; i++) {
                        sprintf(username, "wan_pppoe_username_%02d", i);
                        sprintf(password, "wan_pppoe_password_%02d", i);
                        save2file(file, "\"%s\" * \"%s\" *\n", nvram_safe_get(username), nvram_safe_get(password) );
                }
        } else if (strcmp(wan_proto, "pptp") == 0 ) {
                save2file(file, "\"%s\" * \"%s\" *\n", nvram_safe_get("wan_pptp_username"), nvram_safe_get("wan_pptp_password") );
        } else if (strcmp(wan_proto, "l2tp") == 0 ) {
                save2file(file, "\"%s\" * \"%s\" *\n", nvram_safe_get("wan_l2tp_username"), nvram_safe_get("wan_l2tp_password") );
        }
/*
*       Date: 2009-1-19
*       Name: Ken_Chiang
*       Reason: modify for 3g usb client card to used.
*       Notice :
*/
#ifdef CONFIG_USER_3G_USB_CLIENT
        else if (strcmp(wan_proto, "usb3g") == 0 ) {
                save2file(file, "\"%s\" * \"%s\" *\n",
                        strlen(nvram_safe_get("usb3g_username")) ? nvram_safe_get("usb3g_username") : "*",
                        strlen(nvram_safe_get("usb3g_password")) ? nvram_safe_get("usb3g_password") : "*");
        }
#endif
        chmod(file, 0600);
        return 0;
}

int modify_chap_sercets(char *file)
{
        int i=0;
        char username[] = "wan_pppoe_username_XX";
        char password[] = "wan_pppoe_password_XX";
                char *wan_proto = nvram_safe_get("wan_proto");
                char name[64]={0};
                char psw[64]={0};

        init_file(file);

        if(strcmp(wan_proto, "pppoe") == 0 )
        {
                        for(i=0; i<MAX_PPPOE_CONNECTION; i++)
                        {
                        sprintf(username, "wan_pppoe_username_%02d", i);
                        sprintf(password, "wan_pppoe_password_%02d", i);

                                strcpy(name,  nvram_safe_get(username));
                                strcpy(psw,  nvram_safe_get(password));

                                save2file(file, "\"%s\" \"%s\" \"%s\" *\n",
                                        parse_special_char(name), CHAP_REMOTE_NAME, parse_special_char(psw) );
                                save2file(file, "\"%s\" \"%s\" \"%s\" *\n",
                                        CHAP_REMOTE_NAME, parse_special_char(name), parse_special_char(psw) );
                }
        }
        else if (strcmp(wan_proto, "pptp") == 0 )
        {

                        strcpy(name,  nvram_safe_get("wan_pptp_username"));
                        strcpy(psw,  nvram_safe_get("wan_pptp_password"));

                        save2file(file, "\"%s\" \"%s\" \"%s\" *\n",
                                parse_special_char(name),CHAP_REMOTE_NAME,parse_special_char(psw) );
                        save2file(file, "\"%s\" \"%s\" \"%s\" *\n",
                                CHAP_REMOTE_NAME,parse_special_char(name),parse_special_char(psw) );
        }
        else if (strcmp(wan_proto, "l2tp") == 0 )
        {

                strcpy(name,  nvram_safe_get("wan_l2tp_username"));
                        strcpy(psw,  nvram_safe_get("wan_l2tp_password"));

                save2file(file, "\"%s\" \"%s\" \"%s\" *\n",
                        parse_special_char(name),CHAP_REMOTE_NAME,parse_special_char(psw) );
                        save2file(file, "\"%s\" \"%s\" \"%s\" *\n",
                                CHAP_REMOTE_NAME,parse_special_char(name),parse_special_char(psw) );
        }
/*
*       Date: 2009-1-19
*       Name: Ken_Chiang
*       Reason: modify for 3g usb client card to used.
*       Notice :
*/
#ifdef CONFIG_USER_3G_USB_CLIENT
                else if (strcmp(wan_proto, "usb3g") == 0 ) {
                        save2file(file, "\"%s\" * \"%s\" *\n",
                                strlen(nvram_safe_get("usb3g_username")) ? nvram_safe_get("usb3g_username") : "*",
                                strlen(nvram_safe_get("usb3g_password")) ? nvram_safe_get("usb3g_password") : "*");
                }
#endif
        chmod(file, 0600);
        return 0;
}

int start_bigpond(void)
{
        _system("bpalogin &");
        return 0;
}


int stop_bigpond(void)
{
        KILL_APP_ASYNCH("bpalogin");
        return 0;
}

/*      Date: 2009-01-07
*       Name: jimmy huang
*       Reason: to avoid compiler warning messages
*       Note: Add these 2 function prototype
*/
int start_unnumbered(int ppp_ifunit);
extern char *get_netmask(char *if_name);//in network.c

#ifdef MPPPOE
void remove_mpppoe_files(void)
{
        unlink(PPP_GATEWAY_IP);
        unlink(PPP_WAN_IP);
        unlink(MULTIPLE_ROUTING);
        unlink(DNS_FILE_00);
        unlink(DNS_FILE_01);
        unlink(DNS_FILE_02);
        unlink(DNS_FILE_03);
        unlink(DNS_FILE_04);
        unlink(DNS_FINAL_FILE);
        unlink(DNS_MPPPOE);
        unlink(PPP_IDLE_00);
        unlink(PPP_IDLE_01);
        unlink(PPP_IDLE_02);
        unlink(PPP_IDLE_03);
        unlink(PPP_IDLE_04);
        unlink(PPP_START_IDLE_00);
        unlink(PPP_START_IDLE_01);
        unlink(PPP_START_IDLE_02);
        unlink(PPP_START_IDLE_03);
        unlink(PPP_START_IDLE_04);
        unlink(PPP_RESET_00);
        unlink(PPP_RESET_01);
        unlink(PPP_RESET_02);
        unlink(PPP_RESET_03);
        unlink(PPP_RESET_04);
}
#endif

int start_wan(void)
{
        char *wan_proto;
        char wlan0_mode[] = "rt";
        char tmp_hostname[68] = {'\0'}; // maximun legth would be 63, refer to rfc 1035, chap 2.3.1
        char pppoe_service_name[20] = "";
        char pppoe_ac_name[20] = "";
        char sn[128] = "";
        char ac[128] = "";
        char pppoe_type[] = "wan_pppoe_connect_mode_XX";
        char pppoe_enable[] = "wan_pppoe_enable_XX";
        int ppp_ifunit = 0;
        char *mtu;
        char *hostname;
        char *wan_eth, *lan_bridge;
        char *service_name = NULL;
        char *ac_name = NULL;
        int  wan_specify_dns = 0;
        char *wan_primary_dns = nvram_safe_get("wan_primary_dns");
        char *wan_secondary_dns = nvram_safe_get("wan_secondary_dns");
        char tmp_netbios[5]={0};
        char tmp_ucast[3]={0};
        char tmp_russia[5]={0};
        char *lan_ipaddr, *lan_netmask;
        char *ipv6_wan_proto=NULL;
#ifdef SET_GET_FROM_ARTBLOCK
//        char *wmac_art = NULL;
#endif
#ifdef CONFIG_USER_WAN_8021X
        int wan_8021x_enable;
#endif

        /*      Date: 2009-01-07
        *       Name: jimmy huang
        *       Reason: to avoid udhcp use nvram library, so we use parameter
        *                       to tell udhcpc if he needs to enable/disable option 60/121/249
        *                       variables cmd, cmd_tmp, nv_ptr are used for these purpose
        *       Note:   Add variables cmd, cmd_tmp, nv_ptr
        */
        char *cmd = NULL, *cmd_tmp = NULL;
        char *nv_ptr = NULL;

                if(!( action_flags.all  || action_flags.wan ) )
                        return 1;

        DEBUG_MSG("Start WAN\n");
        /* jimmy added 20080912 */
        /* send signal to wantimer, to reset auth, discovery related variables
                SIGPIPE: ppp/pppd/main.c
                SIGTTOU: ppp/pppd/chap_ms.c, ppp/pppd/plugins/rp-pppoe/plugin.c
                SIGTTIN: ppp/pppd/plugins/rp-pppoe/plugin.c
                used by rc/wantimer.c
        */
#ifdef DLINK_ROUTER_LED_DEFINE
        KILL_APP_SYNCH("-SIGTTIN wantimer");
#endif
        /* ---------------------------------------------- */

#ifdef  UDHCPD_NETBIOS
                if((nvram_match("dhcpd_netbios_enable", "1") == 0 ) && (nvram_match("dhcpd_netbios_learn", "1") == 0 ))
                        strcpy(tmp_netbios,"-N");
#endif

#ifdef  UDHCPC_UNICAST
                if(nvram_match("dhcpc_use_ucast", "1") == 0 )
                        strcpy(tmp_ucast,"-u");
#endif

#ifdef RPPPOE
                if(nvram_match("wan_pptp_russia_enable", "1") == 0 || nvram_match("wan_l2tp_russia_enable", "1") == 0)
                        strcpy(tmp_russia,"-R");
#endif

#ifdef  CONFIG_BRIDGE_MODE
        strcpy(wlan0_mode, nvram_safe_get("wlan0_mode"));
#endif

#ifdef MPPPOE
        if(nvram_match("wan_proto","pppoe") != 0)
                remove_mpppoe_files();
#endif

    wan_eth = nvram_safe_get("wan_eth");
        lan_bridge = nvram_safe_get("lan_bridge");
        wan_proto = nvram_safe_get("wan_proto");
        lan_ipaddr = nvram_safe_get("lan_ipaddr");
        lan_netmask = nvram_safe_get("lan_netmask");
        ipv6_wan_proto = nvram_safe_get("ipv6_wan_proto");


    //ifconfig_up( wan_eth, "0.0.0.0", "0.0.0.0");
        /* change MAC */


/* Date: 2010-7-22
 * Name : Michael Jhong
 * Reason: To avoid used default wan mac (artblock wan mac) access Internet.
 * 	   Artblock_get("wan_mac") has been performed in the rc service_init() function.
*/

/*
#ifdef SET_GET_FROM_ARTBLOCK
        if((wmac_art = artblock_get("wan_mac")))
                ifconfig_mac( wan_eth, wmac_art );
        else
                ifconfig_mac( wan_eth, nvram_safe_get("wan_mac") );
#else
        ifconfig_mac( wan_eth, nvram_safe_get("wan_mac") );
#endif
*/
        ifconfig_mac( wan_eth, nvram_safe_get("wan_mac") );

#ifdef CONFIG_UBICOM_SWITCH_AR8316
	char wanmac_tmp[100],*wanmac_ptr=NULL,*wanmac_clone_ptr;
	FILE *f;
	int switch_value,imt_l;

	wanmac_ptr = nvram_safe_get("wan_mac");
	wanmac_clone_ptr = nvram_safe_get("mac_clone_addr");
	if(!memcmp(wanmac_ptr, wanmac_clone_ptr, 17))
	{
	    while(wanmac_ptr)
	    {
		_system("echo 0x50 > /proc/switch/ar8316-smi/reg/addr");
		if ((f = fopen("/proc/switch/ar8316-smi/reg/addr", "r")) != NULL)
		{
			fscanf(f, "%x", &switch_value);
			printf("/proc/switch/ar8316-smi/reg/addr=0x50(%x)\n",switch_value);
			fclose(f);
			if(switch_value != 0x50) break;
		}
		
		imt_l = 0;
		while ((f = fopen("/proc/switch/ar8316-smi/reg/val", "r")) != NULL)
		{
			fscanf(f, "%x", &switch_value);
			fclose(f);
			printf("/proc/switch/ar8316-smi/reg/value=0x8(%x)\n",switch_value);
			if (!(switch_value & (1 << 3))) break;
			if(++imt_l > 20) break; 
		}
		if(imt_l > 20) break;

		_system("echo 0x58 > /proc/switch/ar8316-smi/reg/addr");
		if ((f = fopen("/proc/switch/ar8316-smi/reg/addr", "r")) != NULL)
		{
			fscanf(f, "%x", &switch_value);
			fclose(f);
			if(switch_value != 0x58) break;
		}
		_system("echo 0x%x > /proc/switch/ar8316-smi/reg/val",((1<< 16) | (1 << 15) | 1));

		_system("echo 0x54 > /proc/switch/ar8316-smi/reg/addr");
		if ((f = fopen("/proc/switch/ar8316-smi/reg/addr", "r")) != NULL)
		{
			fscanf(f, "%x", &switch_value);
			fclose(f);
			if(switch_value != 0x54) break;
		}
		memset(wanmac_tmp, 0, 100);
		sprintf(wanmac_tmp, "echo 0x%c%c%c%c%c%c%c%c > /proc/switch/ar8316-smi/reg/val",*wanmac_ptr,*(wanmac_ptr+1),*(wanmac_ptr+3),*(wanmac_ptr+4),*(wanmac_ptr+6),*(wanmac_ptr+7),*(wanmac_ptr+9),*(wanmac_ptr+10));
		printf("Mac clone cmd : %s\n", wanmac_tmp);
		_system(wanmac_tmp);

		_system("echo 0x50 > /proc/switch/ar8316-smi/reg/addr");
		if ((f = fopen("/proc/switch/ar8316-smi/reg/addr", "r")) != NULL)
		{
			fscanf(f, "%x", &switch_value);
			fclose(f);
			if(switch_value != 0x50) break;
		}
		memset(wanmac_tmp, 0, 100);
		sprintf(wanmac_tmp, "echo 0x%c%c%c%c000A > /proc/switch/ar8316-smi/reg/val",*(wanmac_ptr+12),*(wanmac_ptr+13),*(wanmac_ptr+15),*(wanmac_ptr+16));
		printf("Mac clone cmd : %s\n", wanmac_tmp);
		_system(wanmac_tmp);

		break;
	    }
	}
	else
	{
		DEBUG_MSG("not mac clone\n");
	}
#endif	
	
        init_file(RESOLVFILE);
#ifdef RPPPOE
        /*NickChou add 10080718
        When RUSSIA PPT/PL2TP/PPPOE disconnect, we can recovery WAN Physical DNS (get from DHCP server) from RUSSIA_PHY_RESOLVFILE.
        So WEB GUI would not show PPP DNS if RUSSIA PPTP(PPPOE) disconnect.
        */
        init_file(RUSSIA_PHY_RESOLVFILE);
#endif
/*	Date: 2010-01-11
*	Name: Ken Chiang
*	Reason: modify for AP mode no dns issue.
*	Note:
*/
#ifdef	CONFIG_BRIDGE_MODE
	if(strcmp(wlan0_mode, "ap") == 0){//ap
		if( !nvram_match("wan_specify_dns", "1") )
		{
			if(strlen(wan_secondary_dns) > 6) //0.0.0.0
			{
				DEBUG_MSG("ap dns1=%s ,dns2=%s\n",wan_primary_dns,wan_secondary_dns);
				save2file(RESOLVFILE, "nameserver %s\nnameserver %s\n", wan_primary_dns, wan_secondary_dns);
#ifdef RPPPOE
				if( !nvram_match("wan_pptp_russia_enable", "1") || !nvram_match("wan_l2tp_russia_enable", "1"))
					save2file(RUSSIA_PHY_RESOLVFILE, "nameserver %s\nnameserver %s\n", wan_primary_dns, wan_secondary_dns);
#endif
			}
			else
			{
				DEBUG_MSG("ap dns1=%s\n",wan_primary_dns);
				save2file(RESOLVFILE, "nameserver %s\n", wan_primary_dns);
#ifdef RPPPOE
				if( !nvram_match("wan_pptp_russia_enable", "1") || !nvram_match("wan_l2tp_russia_enable", "1"))
					save2file(RUSSIA_PHY_RESOLVFILE, "nameserver %s", wan_primary_dns);
#endif
			}
        }
	}
	else{//rt
		if( !nvram_match("wan_specify_dns", "1") )
		{
			wan_specify_dns = 1;
#ifdef RPPPOE
		    if( nvram_match("wan_pppoe_russia_enable", "1") == 0)
		    {
				if(strlen(nvram_safe_get("wan_pppoe_russia_secondary_dns")) > 6)
				{
					save2file(RESOLVFILE, "nameserver %s\nnameserver %s\n",
					nvram_safe_get("wan_pppoe_russia_primary_dns"),
					nvram_safe_get("wan_pppoe_russia_secondary_dns") );

					save2file(RUSSIA_PHY_RESOLVFILE, "nameserver %s\nnameserver %s\n",
						nvram_safe_get("wan_pppoe_russia_primary_dns"),
						nvram_safe_get("wan_pppoe_russia_secondary_dns") );

				}
				else
				{
					save2file(RESOLVFILE, "nameserver %s\n",
						nvram_safe_get("wan_pppoe_russia_primary_dns"));
					save2file(RUSSIA_PHY_RESOLVFILE, "nameserver %s",
						nvram_safe_get("wan_pppoe_russia_primary_dns"));
				}
		    }
		    else
#endif
		    {
				if(strlen(wan_secondary_dns) > 6) //0.0.0.0
				{
					DEBUG_MSG("rt dns1=%s ,dns2=%s\n",wan_primary_dns,wan_secondary_dns);
					save2file(RESOLVFILE, "nameserver %s\nnameserver %s\n", wan_primary_dns, wan_secondary_dns);
#ifdef RPPPOE
					if( !nvram_match("wan_pptp_russia_enable", "1") || !nvram_match("wan_l2tp_russia_enable", "1"))
						save2file(RUSSIA_PHY_RESOLVFILE, "nameserver %s\nnameserver %s\n", wan_primary_dns, wan_secondary_dns);
#endif
				}
				else
				{
					DEBUG_MSG("rt dns1=%s ,dns2=%s\n",wan_primary_dns,wan_secondary_dns);
					save2file(RESOLVFILE, "nameserver %s\n", wan_primary_dns);
#ifdef RPPPOE
					if( !nvram_match("wan_pptp_russia_enable", "1") || !nvram_match("wan_l2tp_russia_enable", "1"))
						save2file(RUSSIA_PHY_RESOLVFILE, "nameserver %s", wan_primary_dns);
#endif
				}
		    }
		}
	}
#else
        if( !nvram_match("wan_specify_dns", "1") )
        {
                wan_specify_dns = 1;
#ifdef RPPPOE
            if( nvram_match("wan_pppoe_russia_enable", "1") == 0)
            {
                        if(strlen(nvram_safe_get("wan_pppoe_russia_secondary_dns")) > 6)
                        {
                        save2file(RESOLVFILE, "nameserver %s\nnameserver %s\n",
                                nvram_safe_get("wan_pppoe_russia_primary_dns"),
                                nvram_safe_get("wan_pppoe_russia_secondary_dns") );

                                save2file(RUSSIA_PHY_RESOLVFILE, "nameserver %s\nnameserver %s\n",
                                        nvram_safe_get("wan_pppoe_russia_primary_dns"),
                                        nvram_safe_get("wan_pppoe_russia_secondary_dns") );

                        }
                        else
                        {
                                save2file(RESOLVFILE, "nameserver %s\n",
                                        nvram_safe_get("wan_pppoe_russia_primary_dns"));
                                save2file(RUSSIA_PHY_RESOLVFILE, "nameserver %s",
                                        nvram_safe_get("wan_pppoe_russia_primary_dns"));
                        }
            }
            else
#endif
            {
                        if(strlen(wan_secondary_dns) > 6) //0.0.0.0
                        {
                                save2file(RESOLVFILE, "nameserver %s\nnameserver %s\n", wan_primary_dns, wan_secondary_dns);
#ifdef RPPPOE
                                if( !nvram_match("wan_pptp_russia_enable", "1") || !nvram_match("wan_l2tp_russia_enable", "1"))
                                        save2file(RUSSIA_PHY_RESOLVFILE, "nameserver %s\nnameserver %s\n", wan_primary_dns, wan_secondary_dns);
#endif
                        }
                        else
                        {
                                save2file(RESOLVFILE, "nameserver %s\n", wan_primary_dns);
#ifdef RPPPOE
                                if( !nvram_match("wan_pptp_russia_enable", "1") || !nvram_match("wan_l2tp_russia_enable", "1"))
                                        save2file(RUSSIA_PHY_RESOLVFILE, "nameserver %s", wan_primary_dns);
#endif
                        }
            }
        }
#endif//CONFIG_BRIDGE_MODE
        hostname = nvram_safe_get("hostname");
                strncpy(tmp_hostname, hostname,63);// maximun legth would be 63, refer to rfc 1035, chap 2.3.1

#ifdef  CONFIG_BRIDGE_MODE
/*	Date: 2010-01-05
*	Name: Ken Chiang
*	Reason: modify for AP mode the wan port LED can't bright issue.
*	Note:
*/
	if (strcmp(wlan0_mode, "rt") == 0) {
		if (!checkServiceAlivebyName("wantimer"))
			system("/var/sbin/wantimer &");
	}
	else
		system("/sbin/gpio SWITCH_CONTROL on &");
#else
	/* charles: because of not kill wantimer in stop_wan function 
  		    add check to avoid multiple running wantimer 
  	*/
	if (!checkServiceAlivebyName("wantimer")) {
		system("/var/sbin/wantimer &");
	}
	else {
		/*restart wantimer, let wantimer update some variable */
		system("killall wantimer &");
		sleep(1);
		system("/var/sbin/wantimer &");
	}
#endif//CONFIG_BRIDGE_MODE

#ifdef AP_NOWAN_HASBRIDGE
        char *dhcpc_enable = nvram_safe_get("dhcpc_enable");
        if(strncmp(dhcpc_enable, "0",1) == 0 )
#else
/*      Date: 2009-6-5
*       Name: Bing Chou
*       Reason: Add the definition of LAN_SUPPORT_DHCPC to support dhcpc for lan
*       Note:Although AP_NOWAN_HASBRIDGE has been define to support dhcpc for lan , but some behavior can't be allow for other model
*/
#ifdef LAN_SUPPORT_DHCPC
        char *dhcpc_enable = nvram_safe_get("dhcpc_enable");
        if(strcmp(dhcpc_enable,"0")==0)
#else
        if(strcmp(wan_proto, "static")==0) /* Static IP */
#endif//LAN_SUPPORT_DHCPC
#endif//AP_NOWAN_HASBRIDGE
        {
		if(strcmp(wlan0_mode, "ap") == 0){
#ifdef AP_NOWAN_HASBRIDGE
                        ifconfig_up( lan_bridge, nvram_safe_get("lan_ipaddr"), nvram_safe_get("lan_netmask") );
#else
#ifdef LAN_SUPPORT_DHCPC
                ifconfig_up(lan_bridge,nvram_safe_get("lan_ipaddr"),nvram_safe_get("lan_netmask"));
#else
			DEBUG_MSG("ap iterface=%s up ip=%s\n",lan_bridge,nvram_safe_get("wan_static_ipaddr"));
                        ifconfig_up( lan_bridge, nvram_safe_get("wan_static_ipaddr"), nvram_safe_get("wan_static_netmask") );
#endif//LAN_SUPPORT_DHCPC
#endif//AP_NOWAN_HASBRIDGE
		}else//rt
                {
                        ifconfig_up( wan_eth, nvram_safe_get("wan_static_ipaddr"), nvram_safe_get("wan_static_netmask") );

                        /*
                                NickChou 20080710, improve issue => [WAN PC] PING [LAN Device IP] and Device will send PING_REPLY with WAN MAC
                                Maybe Firewall INPUT Chain policy is ACCEPT, so i add one rule here.
                                After Firewall Setting finished, this situation will not happen.
                        */
                        _system("iptables -A INPUT -i %s -d %s/%s -p icmp --icmp-type echo-request -j DROP",
                                wan_eth, lan_ipaddr, lan_netmask);
                        sleep(1);
                }

#ifdef AP_NOWAN_HASBRIDGE
                route_add( lan_bridge, "0.0.0.0", "0.0.0.0", nvram_safe_get("lan_gateway"), 0);
#else
#ifdef	CONFIG_BRIDGE_MODE
/*	Date: 2010-01-11
*	Name: Ken Chiang
*	Reason: modify for AP mode no gateway route issue.
*	Note:
*/
		if(strcmp(wlan0_mode, "ap") == 0){//lan_bridge
			DEBUG_MSG("ap iterface=%s route ip=%s\n",lan_bridge,nvram_safe_get("wan_static_gateway"));
			if(nvram_match("wan_static_gateway", "0.0.0.0"))
				route_add( lan_bridge, "0.0.0.0", "0.0.0.0", nvram_safe_get("wan_static_gateway"), 0);
	    }
	    else{//rt
	    	DEBUG_MSG("rt iterface=%s up ip=%s\n",wan_eth,nvram_safe_get("wan_static_gateway"));
	    	route_add( wan_eth, "0.0.0.0", "0.0.0.0", nvram_safe_get("wan_static_gateway"), 0);
	    }
#else
                route_add( wan_eth, "0.0.0.0", "0.0.0.0", nvram_safe_get("wan_static_gateway"), 0);
#endif//CONFIG_BRIDGE_MODE
#endif//AP_NOWAN_HASBRIDGE
        }
#ifdef AP_NOWAN_HASBRIDGE
        else if( strncmp( dhcpc_enable, "1", 1) == 0 )
#else
#ifdef LAN_SUPPORT_DHCPC
        else if(strcmp(dhcpc_enable,"1")==0)
#else
        else if ( strcmp(wan_proto, "dhcpc") ==0 ) /* DHCP */
#endif//LAN_SUPPORT_DHCPC
#endif//AP_NOWAN_HASBRIDGE
        {
//              if(strcmp(wlan0_mode, "ap") == 0)
//              {
//                      _system("udhcpc -i %s -H \"%s\" %s -s %s %s -a &", lan_bridge, parse_special_char(tmp_hostname),
//                              tmp_ucast,
//                              wan_specify_dns ? DHCPC_NODNS_SCRIPT:DHCPC_DNS_SCRIPT,
//                              tmp_netbios);
//              }
//              else
//              {
//                      _system("udhcpc -w %s -i %s -H \"%s\" %s -s %s %s &", wan_proto, wan_eth, parse_special_char(tmp_hostname),
//                                      tmp_ucast,
//                                      wan_specify_dns ? DHCPC_NODNS_SCRIPT:DHCPC_DNS_SCRIPT,
//                                      tmp_netbios);
//              }

                /*      Date: 2009-01-07
                *       Name: jimmy huang
                *       Reason: to avoid udhcp use nvram library, so we use parameter
                *                       to tell udhcpc if he needs to enable/disable option 60/121/249
                *       Note: Modified the codes above to the new codes below
                */
                DEBUG_MSG("wan_proto dhcpc\n");
                if(!cmd){
                        free(cmd);
                }
                cmd = NULL;
                cmd_tmp = NULL;
                nv_ptr = NULL;
                /*	Date: 2010-07-26
                *	Name: Jimmy Huang
                *	Reason:	cmd line length must be larger enough, DHCPC_CMD_MAX_LENGTH is defined in shvar.h
                *	Note: udhcpc -w dhcpc -i eth1 -H "HOST_NAME_CAN_UP_TO_63_CHARACTERS_LONG" -s /usr/share/udhcpc/default.bound-dns --option121or249_on
                */
                cmd = malloc(sizeof(char) * DHCPC_CMD_MAX_LENGTH);
                memset(cmd,'\0',DHCPC_CMD_MAX_LENGTH);
                cmd_tmp = cmd;
                cmd_tmp += sprintf(cmd_tmp,"udhcpc ");
/*      Date: 2009-04-10
*       Name: Ken Chiang
*       Reason: Add support for enable auto mode select(router mode/ap mode).
*       Note:
*/
/*

*/

#ifdef CONFIG_USER_OPENAGENT
        if ( !nvram_match("tr069_enable", "1") && !nvram_match("tr069_ui", "1"))
        {
                        cmd_tmp += sprintf(cmd_tmp,"-z ");

		}
#endif


#ifdef CONFIG_USER_WAN_8021X
/*      Date: 2009-05-23
*       Name: Macpaul Lin
*       Reason: Add support for enable wan 8021X Auth
*       Note: this should be executed after interface up and before/at the same
*               time of  dhcp
*/
        start_wan_supplicant();
#endif

#ifdef  CONFIG_BRIDGE_MODE
#ifdef AUTO_MODE_SELECT
                DEBUG_MSG("AUTO_MODE_SELECT\n");
                if(nvram_match("auto_mode_select", "1") == 0 ){
                        DEBUG_MSG("automodeselect enable\n");
                        cmd_tmp += sprintf(cmd_tmp,"-A ");
                }
#endif
                if(strcmp(wlan0_mode, "ap") == 0){
                        cmd_tmp += sprintf(cmd_tmp,"-i %s -a ",lan_bridge);
                }else{
                        cmd_tmp += sprintf(cmd_tmp,"-w %s -i %s ",wan_proto,wan_eth);
                }
#else
                cmd_tmp += sprintf(cmd_tmp,"-w %s -i %s ",wan_proto,wan_eth);
#endif
                cmd_tmp += sprintf(cmd_tmp,"-H \"%s\" %s ",parse_special_char(tmp_hostname),tmp_ucast);
                cmd_tmp += sprintf(cmd_tmp,"-s %s %s ",wan_specify_dns ? DHCPC_NODNS_SCRIPT:DHCPC_DNS_SCRIPT,tmp_netbios);
#if defined(UDHCPC_MS_CLASSLESS_STATIC_ROUTE) || defined(UDHCPC_RFC_CLASSLESS_STATIC_ROUTE)
                if((nv_ptr = (char *)nvram_get("classless_static_route"))!=NULL){
                        //our nvram has this key
                        if(strcmp(nv_ptr,"1") == 0){
                                //default in udhcpc is disable, but the value is set to enable
                                cmd_tmp += sprintf(cmd_tmp,"--option121or249_on ");//enable option 121 or 249
                        }
                }
#endif
                nv_ptr = NULL;
                if((nv_ptr = (char *)nvram_get("dhcpc_enable_vendor_class"))!=NULL){
                        //our nvram has this key
                        if(strcmp(nv_ptr,"0") == 0){
                                //default in udhcpc is enable, but the value is set to disable
                                cmd_tmp += sprintf(cmd_tmp,"--option60_off ");//disable option 60
                        }
                }
                cmd_tmp += sprintf(cmd_tmp,"&");
                DEBUG_MSG("%s\n",cmd);
                _system(cmd);
                free(cmd);
                cmd = NULL;
        }
        else if(strcmp(wan_proto, "pppoe") == 0 && strcmp(wlan0_mode, "rt") == 0) /* PPPoE */
        {
#ifndef MPPPOE
                modify_sercets(PAP_SECRETS);
                modify_chap_sercets(CHAP_SECRETS);
#endif
                modify_ppp_options();

#ifdef RPPPOE

                /* Russia PPPoE needs WAN_ETH interface prior dial up */
                if( nvram_match("wan_pppoe_russia_enable", "1") == 0)
                {
                        if(nvram_match("wan_pppoe_russia_dynamic", "1") == 0 )
                        {
//                              _system("udhcpc -w %s -i %s -H \"%s\" -s %s %s -R &", wan_proto, wan_eth, parse_special_char(tmp_hostname),
//                                      wan_specify_dns ? DHCPC_NODNS_SCRIPT:DHCPC_DNS_SCRIPT, tmp_netbios);
                                /*      Date: 2009-01-07
                                *       Name: jimmy huang
                                *       Reason: to avoid udhcp use nvram library, so we use parameter
                                *                       to tell udhcpc if he needs to enable/disable option 60/121/249
                                *       Note: Modified the codes above to the new codes below
                                */
                                if(!cmd){
                                        free(cmd);
                                }
                                cmd = NULL;
                                cmd_tmp = NULL;
                                nv_ptr = NULL;
                                /*	Date: 2010-07-26
                                *	Name: Jimmy Huang
                                *	Reason:	cmd line length must be larger enough, DHCPC_CMD_MAX_LENGTH is defined in shvar.h
                                *	Note: udhcpc -w dhcpc -i eth1 -H "HOST_NAME_CAN_UP_TO_63_CHARACTERS_LONG" -s /usr/share/udhcpc/default.bound-dns --option121or249_on
                                */
                                cmd = malloc(sizeof(char) * DHCPC_CMD_MAX_LENGTH);
                                memset(cmd,'\0',DHCPC_CMD_MAX_LENGTH);
                                cmd_tmp = cmd;
                                cmd_tmp += sprintf(cmd_tmp,"udhcpc ");
                                cmd_tmp += sprintf(cmd_tmp,"-w %s -i %s ",wan_proto,wan_eth);
                                cmd_tmp += sprintf(cmd_tmp,"-H \"%s\" ",parse_special_char(tmp_hostname));
                                cmd_tmp += sprintf(cmd_tmp,"-s %s %s ",wan_specify_dns ? DHCPC_NODNS_SCRIPT:DHCPC_DNS_SCRIPT,tmp_netbios);
#if defined(UDHCPC_MS_CLASSLESS_STATIC_ROUTE) || defined(UDHCPC_RFC_CLASSLESS_STATIC_ROUTE)
                                if((nv_ptr = (char *)nvram_get("classless_static_route"))!=NULL){
                                        //our nvram has this key
                                        if(strcmp(nv_ptr,"1") == 0){
                                                //default in udhcpc is disable, but the value is set to enable
                                                cmd_tmp += sprintf(cmd_tmp,"--option121or249_on ");//enable option 121 or 249
                                        }
                                }
#endif
                                nv_ptr = NULL;
                                if((nv_ptr = (char *)nvram_get("dhcpc_enable_vendor_class"))!=NULL){
                                        //our nvram has this key
                                        if(strcmp(nv_ptr,"0") == 0){
                                                //default in udhcpc is enable, but the value is set to disable
                                                cmd_tmp += sprintf(cmd_tmp,"--option60_off ");//disable option 60
                                        }
                                }
                                cmd_tmp += sprintf(cmd_tmp,"-R ");
                                cmd_tmp += sprintf(cmd_tmp,"&");
                                DEBUG_MSG("%s\n",cmd);
                                _system(cmd);
                                free(cmd);
                                cmd = NULL;
                        }
                        else
                        {
                                ifconfig_up( wan_eth, nvram_safe_get("wan_pppoe_russia_ipaddr"), nvram_safe_get("wan_pppoe_russia_netmask") );
                                route_add( wan_eth, "0.0.0.0", "0.0.0.0", nvram_safe_get("wan_pppoe_russia_gateway"), 0);
                        }
                }
#endif

                for (ppp_ifunit=0; ppp_ifunit < MAX_PPPOE_CONNECTION; ppp_ifunit ++)
                {
                        sprintf(pppoe_enable, "wan_pppoe_enable_%02d", ppp_ifunit);
                        sprintf(pppoe_type, "wan_pppoe_connect_mode_%02d", ppp_ifunit);

                        if( nvram_match(pppoe_enable, "1") == 0 )
                        {
                                /* Only always on can start_pppoe function */
                                if(nvram_match(pppoe_type,"always_on") == 0)
                                {
                                        sprintf(pppoe_service_name, "wan_pppoe_service_%02d", ppp_ifunit);
                                        service_name = nvram_safe_get(pppoe_service_name);
//Albert add
//---
                                        sprintf(pppoe_ac_name, "wan_pppoe_ac_%02d", ppp_ifunit);
                                        ac_name = nvram_safe_get(pppoe_ac_name);
                                        if(strcmp(ac_name, "") == 0)
                                                strncpy(ac, "null_ac", strlen("null_ac"));
                                        else
                                                strncpy(ac, pppoe_ac_name, strlen(pppoe_ac_name));

//---
                                        if(strcmp(service_name, "") == 0)
                                                strncpy(sn, "null_sn", strlen("null_sn"));
                                        else
                                                strncpy(sn, service_name, strlen(service_name));

#ifdef MPPPOE
                                        start_pppoe(ppp_ifunit);
#endif
                                        start_redial(wan_proto, wan_eth, sn, ac);
                                }
                                else if(nvram_match(pppoe_type,"on_demand") == 0)
                {
#ifdef MPPPOE
                                        start_ppptimer();
#endif
                    start_monitor(ppp_ifunit, wan_proto, wan_eth);
#ifdef DLINK_ROUTER_LED_DEFINE
/* jimmy modified , 20080912, fixed the bug that sometimes wantimer is not up after booting */
                                        {
                                                sleep(1);// in case wantimer is not ready to receive signal
                                                int wantimer_pid = read_pid(WANTIMER_PID);
                                                if(wantimer_pid == -1){
                                                        printf("Can't get wantimer_pid, so send SIGTSTP to all process !!");
                                                }
                                                kill(wantimer_pid, SIGTSTP);
                                        }
                                        //kill(read_pid(WANTIMER_PID), SIGTSTP);
/* ---------------------------------------------------------------------------------------- */
                                }
                                else /*manual mode*/
/* jimmy modified , 20080912, fixed the bug that sometimes wantimer is not up after booting */
                                {
                                        sleep(1);// in case wantimer is not ready to receive signal
                                        int wantimer_pid = read_pid(WANTIMER_PID);
                                        if(wantimer_pid == -1){
                                                printf("Can't get wantimer_pid, so send SIGALRM to all process !!");
                                        }
                                        kill(wantimer_pid, SIGALRM);
                                }
//                              kill(read_pid(WANTIMER_PID), SIGALRM);
/* ---------------------------------------------------------------------------------------- */
#else
                                }
#endif
                        }
                }
        /* jimmy modified , 20080912, fixed the bug that sometimes wantimer is not up after booting */
                sleep(1);
                //sleep(2);
        /* ---------------------------------------------------------------------------------------- */
        }
        else if(strcmp(wan_proto, "unnumbered") == 0) /* IP Unnumbered */
        {
                modify_ppp_options();
                start_unnumbered(ppp_ifunit);
        }
        else if( strcmp(wan_proto, "pptp") == 0 && strcmp(wlan0_mode, "rt") == 0 ) /* PPTP */
        {
                modify_sercets(PAP_SECRETS);
                modify_chap_sercets(CHAP_SECRETS);

		/* In order to connect to the pptp server (103/330) using the MPPE*/
		if(nvram_match("wan_pptp_proto_client", "mschap-v2") == 0){
			if(nvram_match("wan_pptp_encryption_client", "none") == 0)
				nvram_set("support_pptp_mppe","0");
			
			else 
				nvram_set("support_pptp_mppe","1");
		}

                modify_ppp_options();
                if(nvram_match("wan_pptp_dynamic", "1") == 0 )
                {
//                      _system("udhcpc -w %s -i %s -H \"%s\" -s %s %s %s &", wan_proto, wan_eth, parse_special_char(tmp_hostname),
//                              wan_specify_dns ? DHCPC_NODNS_SCRIPT:DHCPC_DNS_SCRIPT, tmp_netbios,     tmp_russia);
                        /*      Date: 2009-01-07
                        *       Name: jimmy huang
                        *       Reason: to avoid udhcp use nvram library, so we use parameter
                        *                       to tell udhcpc if he needs to enable/disable option 60/121/249
                        *       Note: Modified the codes above to the new codes below
                        */
                        if(!cmd){
                                free(cmd);
                        }
                        cmd = NULL;
                        cmd_tmp = NULL;
                        nv_ptr = NULL;
                        /*	Date: 2010-07-26
                        *	Name: Jimmy Huang
                        *	Reason:	cmd line length must be larger enough, DHCPC_CMD_MAX_LENGTH is defined in shvar.h
                        *	Note: udhcpc -w dhcpc -i eth1 -H "HOST_NAME_CAN_UP_TO_63_CHARACTERS_LONG" -s /usr/share/udhcpc/default.bound-dns --option121or249_on
                        */
                        cmd = malloc(sizeof(char) * DHCPC_CMD_MAX_LENGTH);
                        memset(cmd,'\0',DHCPC_CMD_MAX_LENGTH);
                        cmd_tmp = cmd;
                        cmd_tmp += sprintf(cmd_tmp,"udhcpc ");
                        cmd_tmp += sprintf(cmd_tmp,"-w %s -i %s ",wan_proto,wan_eth);
                        cmd_tmp += sprintf(cmd_tmp,"-H \"%s\" ",parse_special_char(tmp_hostname));
                        cmd_tmp += sprintf(cmd_tmp,"-s %s %s ",wan_specify_dns ? DHCPC_NODNS_SCRIPT:DHCPC_DNS_SCRIPT,tmp_netbios);
#if defined(UDHCPC_MS_CLASSLESS_STATIC_ROUTE) || defined(UDHCPC_RFC_CLASSLESS_STATIC_ROUTE)
                        if((nv_ptr = (char *)nvram_get("classless_static_route"))!=NULL){
                                //our nvram has this key
                                if(strcmp(nv_ptr,"1") == 0){
                                        //default in udhcpc is disable, but the value is set to enable
                                        cmd_tmp += sprintf(cmd_tmp,"--option121or249_on ");//enable option 121 or 249
                                }
                        }
#endif
                        nv_ptr = NULL;
                        if((nv_ptr = (char *)nvram_get("dhcpc_enable_vendor_class"))!=NULL){
                                //our nvram has this key
                                if(strcmp(nv_ptr,"0") == 0){
                                        //default in udhcpc is enable, but the value is set to disable
                                        cmd_tmp += sprintf(cmd_tmp,"--option60_off ");//disable option 60
                                }
                        }
                        cmd_tmp += sprintf(cmd_tmp,"%s ",tmp_russia);//-R ?
                        cmd_tmp += sprintf(cmd_tmp,"&");
                        DEBUG_MSG("%s\n",cmd);
                        _system(cmd);
                        free(cmd);
                        cmd = NULL;
                }
                else
                {
                        ifconfig_up( wan_eth, nvram_safe_get("wan_pptp_ipaddr"), nvram_safe_get("wan_pptp_netmask") );
                        route_add( wan_eth, "0.0.0.0", "0.0.0.0", nvram_safe_get("wan_pptp_gateway"), 0);
                }

                        /* Only always on can start pptp function */
                if(nvram_match("wan_pptp_connect_mode", "always_on") == 0)
                {
                        start_pptp(); //2011.08.11 add from Ubicom Barbaros for redial issue
                        start_redial(wan_proto, wan_eth, "0", "0");
                }
                else if(nvram_match("wan_pptp_connect_mode", "on_demand") == 0)
                {
                        start_monitor(0, wan_proto, wan_eth);
#ifdef DLINK_ROUTER_LED_DEFINE
                        kill(read_pid(WANTIMER_PID), SIGTSTP);
                }
                else /*manual mode*/
                        kill(read_pid(WANTIMER_PID), SIGALRM);
#else
                }
#endif
        }
        else if( strcmp(wan_proto, "l2tp") == 0 && strcmp(wlan0_mode, "rt") == 0 ) /* L2TP */
        {
                modify_sercets(PAP_SECRETS);
                modify_chap_sercets(CHAP_SECRETS);
                modify_ppp_options();
//Albert modify //2008/10/21
//--
//              modify_l2tp_conf();
//--
                if(nvram_match("wan_l2tp_dynamic", "1") == 0 )
                {
//                      _system("udhcpc -w %s -i %s -H \"%s\" -s %s %s &", wan_proto, wan_eth, parse_special_char(tmp_hostname),
//                              wan_specify_dns ? DHCPC_NODNS_SCRIPT:DHCPC_DNS_SCRIPT, tmp_netbios, tmp_russia);
                        /*      Date: 2009-01-07
                        *       Name: jimmy huang
                        *       Reason: to avoid udhcp use nvram library, so we use parameter
                        *                       to tell udhcpc if he needs to enable/disable option 60/121/249
                        *       Note: Modified the codes above to the new codes below
                        */
                        if(!cmd){
                                free(cmd);
                        }
                        cmd = NULL;
                        cmd_tmp = NULL;
                        nv_ptr = NULL;
                        /*	Date: 2010-07-26
                        *	Name: Jimmy Huang
                        *	Reason:	cmd line length must be larger enough, DHCPC_CMD_MAX_LENGTH is defined in shvar.h
                        *	Note: udhcpc -w dhcpc -i eth1 -H "HOST_NAME_CAN_UP_TO_63_CHARACTERS_LONG" -s /usr/share/udhcpc/default.bound-dns --option121or249_on
                        */
                        cmd = malloc(sizeof(char) * DHCPC_CMD_MAX_LENGTH);
                        memset(cmd,'\0',DHCPC_CMD_MAX_LENGTH);
                        cmd_tmp = cmd;
                        cmd_tmp += sprintf(cmd_tmp,"udhcpc ");
                        cmd_tmp += sprintf(cmd_tmp,"-w %s -i %s ",wan_proto,wan_eth);
                        cmd_tmp += sprintf(cmd_tmp,"-H \"%s\" ",parse_special_char(tmp_hostname));
                        cmd_tmp += sprintf(cmd_tmp,"-s %s %s ",wan_specify_dns ? DHCPC_NODNS_SCRIPT:DHCPC_DNS_SCRIPT,tmp_netbios);
#if defined(UDHCPC_MS_CLASSLESS_STATIC_ROUTE) || defined(UDHCPC_RFC_CLASSLESS_STATIC_ROUTE)
                        if((nv_ptr = (char *)nvram_get("classless_static_route"))!=NULL){
                                //our nvram has this key
                                if(strcmp(nv_ptr,"1") == 0){
                                        //default in udhcpc is disable, but the value is set to enable
                                        cmd_tmp += sprintf(cmd_tmp,"--option121or249_on ");//enable option 121 or 249
                                }
                        }
#endif
                        nv_ptr = NULL;
                        if((nv_ptr = (char *)nvram_get("dhcpc_enable_vendor_class"))!=NULL){
                                //our nvram has this key
                                if(strcmp(nv_ptr,"0") == 0){
                                        //default in udhcpc is enable, but the value is set to disable
                                        cmd_tmp += sprintf(cmd_tmp,"--option60_off ");//disable option 60
                                }
                        }
                        cmd_tmp += sprintf(cmd_tmp,"%s ",tmp_russia);//-R ?
                        cmd_tmp += sprintf(cmd_tmp,"&");
                        DEBUG_MSG("%s\n",cmd);
                        _system(cmd);
                        free(cmd);
                        cmd = NULL;
                }
                else
                {
                        ifconfig_up( wan_eth, nvram_safe_get("wan_l2tp_ipaddr"), nvram_safe_get("wan_l2tp_netmask") );
                        route_add( wan_eth, "0.0.0.0", "0.0.0.0", nvram_safe_get("wan_l2tp_gateway"), 0);
                }
                        /* Only always on can start l2tp function */
                if(nvram_match("wan_l2tp_connect_mode", "always_on") == 0)
                {
                        start_l2tp(); //2011.08.11 add from Ubicom Barbaros for redial issue
                        start_redial(wan_proto, wan_eth, "0", "0");
                }
                else if(nvram_match("wan_l2tp_connect_mode", "on_demand") == 0)
                {
                        start_monitor(0, wan_proto, wan_eth);
#ifdef DLINK_ROUTER_LED_DEFINE
                        kill(read_pid(WANTIMER_PID), SIGTSTP);
                }
                else /*manual mode*/
                        kill(read_pid(WANTIMER_PID), SIGALRM);
#else
                }
#endif
        }
        else if( strcmp(wan_proto, "bigpond") == 0 && strcmp(wlan0_mode, "rt") == 0 ) /* Big Pond */
        {
                start_bigpond();
                sleep(2);
        }
/*
*       Date: 2009-1-19
*       Name: Ken_Chiang
*       Reason: modify for 3g usb client card to used.
*       Notice :
*/
#ifdef CONFIG_USER_3G_USB_CLIENT
        else if( strcmp(wan_proto, "usb3g") == 0 && strcmp(wlan0_mode, "rt") == 0 ) /* usb3g */
        {
		char dial_auth[16]={0}, usb3g_mtu[10]={0};		
                char *use_peerdns="";
                int usb3g_auth=atoi(nvram_safe_get("usb3g_auth"));

                if(wan_specify_dns==0)
                        use_peerdns="usepeerdns";

                modify_sercets(PAP_SECRETS);
                modify_chap_sercets(CHAP_SECRETS);

                /* creat PPP_3G_IPUP file*/
                init_file(PPP_3G_IP_UP);
                save2file(PPP_3G_IP_UP,
                        "echo \"Hello ip-up\"\n"
                        "PATH=/sbin:/usr/sbin:/bin:/usr/bin\n"
                        "export PATH\n"
                        "\n"
                        "LOGDEVICE=$6\n"
                        "REALDEVICE=$1\n"
                        "\n"
                        "exit 0\n");

                /* creat PPP_3G_IP_DOWN file*/
                init_file(PPP_3G_IP_DOWN);
                save2file(PPP_3G_IP_DOWN,
                        "echo \"Hello ip-down\"\n"
                        "PATH=/sbin:/usr/sbin:/bin:/usr/bin\n"
                        "export PATH\n"
                        "\n"
                        "LOGDEVICE=$6\n"
                        "REALDEVICE=$1\n"
                        "\n"
                        "exit 0\n");

			
                /* creat PPP_3G_DIAL file*/
                if(usb3g_auth==1){//pap
                        if(strlen(nvram_safe_get("usb3g_password")))
				strcpy(dial_auth,"refuse-chap");//-chap
                        else
				strcpy(dial_auth,"");
                }
                else if(usb3g_auth==2){//chap
                        if(strlen(nvram_safe_get("usb3g_password")))
				strcpy(dial_auth,"refuse-pap");//-pap
                        else
				strcpy(dial_auth,"");
                }
                else{//auto
			strcpy(dial_auth,"");
                }

		if(strlen(nvram_safe_get("usb3g_wan_mtu")))
			sprintf(usb3g_mtu, "mtu %s", nvram_safe_get("usb3g_wan_mtu"));		
		else
			strcpy(usb3g_mtu,"");	

                init_file(PPP_3G_DIAL);
                save2file(PPP_3G_DIAL,
                        "-detach\n"
                        "ipcp-max-failure 30\n"
                        "lcp-echo-failure 0\n"
                        "/dev/ttyUSB0\n"
                        "115200\n"
                        "debug\n"
			"novj\n"//NickChou
                        "defaultroute\n"
                        "%s\n"
                        "\n"
                        "ipcp-accept-local\n"
                        "ipcp-accept-remote\n"
                        "\n"
                        "user \"%s\"\n"
                        "password \"%s\"\n"
                        "crtscts\n"
                        "lock\n"
                        "%s\n"
                        "%s\n"
                        "connect \'/sbin/chat -v -t6 -f %s\'\n"
                        "persist\n"
                        ,use_peerdns
                        ,strlen(nvram_safe_get("usb3g_username")) ? nvram_safe_get("usb3g_username") : ""
                        ,strlen(nvram_safe_get("usb3g_password")) ? nvram_safe_get("usb3g_password") : ""
			,dial_auth
			,usb3g_mtu
                        ,PPP_3G_CONNECT_CHAT);

                if(nvram_match("usb3g_reconnect_mode", "always_on") == 0)
                {
                        start_redial(wan_proto, wan_eth, "0", "0");
                }
                else if(nvram_match("usb3g_reconnect_mode", "on_demand") == 0)
                {
                        start_monitor(0, wan_proto, wan_eth);
#ifdef DLINK_ROUTER_LED_DEFINE
                        kill(read_pid(WANTIMER_PID), SIGTSTP);
                }
                else /*manual mode*/
                        kill(read_pid(WANTIMER_PID), SIGALRM);
#else
                }
#endif
        }
#endif //#ifdef CONFIG_USER_3G_USB_CLIENT

#if defined(CONFIG_USER_3G_USB_CLIENT_WM5) || defined(CONFIG_USER_3G_USB_CLIENT_IPHONE)
/*	Date: 2010-01-13
*	Name: Jimmy Huang
*	Reason:	Add support for Windows Mobile 5
*	Note:
*			Currently support CHT 9100 with Windows Mobile Phone 5
*				Before activate WM5's Internet Sharing
*				VendorID: 0bb4
*				ProductID:0b05
*				Once activate WM5's Internet Sharing
*				VendorID: 0bb4
*				ProductID:0303
*				We using ProductID:0303 to check if the device is ready
*			Pid, Vid is generated by AR7161/mips-linux-2.6.15/drivers/usb/core/hub.c
*			driver status is generated by AR7161/mips-linux-2.6.15/drivers/usb/net/usbnet.c
*			driver is using rndis_host
*/
	else if( strcmp(wan_proto, "usb3g_phone") == 0 && strcmp(wlan0_mode, "rt") == 0 ) /* usb3g */
	{
#ifdef CONFIG_USER_3G_USB_CLIENT_WM5
/*	Date:	2010-02-05
 *	Name:	Cosmo Chang
 *	Reason:	add usb_type=5 for Android Phone RNDIS feature
 */
		if( nvram_match("usb_type", "3") == 0 || nvram_match("usb_type", "5") == 0)// we don't call udhcpc, let wantimer to call udhcpc
		{
			printf("%s (%d) start_usb_mobile !!\n",__func__,__LINE__);
			start_usb_mobile_phone(3);
		}
#endif
#ifdef CONFIG_USER_3G_USB_CLIENT_IPHONE
		if( nvram_match("usb_type", "4") == 0)// we don't call udhcpc, let wantimer to call udhcpc
		{
			printf("%s (%d) start_usb_mobile !!\n",__func__,__LINE__);
			start_usb_mobile_phone(4);
		}
#endif
	}
#endif

        mtu = nvram_safe_get("wan_mtu");
        if(strlen(mtu)>2)
        {
                char *lan_eth=nvram_safe_get("lan_eth");
#ifndef AR9100
                _system("ifconfig %s down", lan_eth);
#endif
                _system("ifconfig %s mtu %s", wan_eth, mtu);
#ifndef AR9100
                _system("ifconfig %s up", lan_eth);
#endif
        }

        return 0;
}

/* NickChou, 071210, for fix PPTP/L2TP in multi-router environment */
int add_host_gw(char *wan_proto, char *server_ip, char *wan_eth)
{
        /* server_ip support host name format, ex. www.dlink.com.tw*/

        FILE *fp;
        char new_gw[16];

        if(strcmp(wan_proto, "pptp") == 0)
        {
                if(nvram_match("wan_pptp_dynamic", "1") == 0)
                {
                        fp = fopen("/var/tmp/dhcp_gateway.txt","r");
                        if(fp==NULL)
                        {
                                printf("open /var/tmp/dhcp_gateway.txt fail\n");
                                return 0;
                        }
                        fread(new_gw, 1, 15, fp);
                                _system("route add -host %s gw %s dev %s", server_ip, new_gw, wan_eth);
                                fclose(fp);
                }
                else
                        _system("route add -host %s gw %s dev %s", server_ip, nvram_safe_get("wan_pptp_gateway"), wan_eth);
        }
        else if(strcmp(wan_proto, "l2tp") == 0)
        {
                if(nvram_match("wan_l2tp_dynamic", "1") == 0)
                {
                        fp = fopen("/var/tmp/dhcp_gateway.txt","r");
                        if(fp==NULL)
                        {
                                printf("open /var/tmp/dhcp_gateway.txt fail\n");
                                return 0;
                        }
                        fread(new_gw, 1, 15, fp);
                        _system("route add -host %s gw %s dev %s", server_ip, new_gw, wan_eth);
                fclose(fp);
        }
        else
                        _system("route add -host %s gw %s dev %s", server_ip, nvram_safe_get("wan_l2tp_gateway"), wan_eth);
        }

    return 0;
}

int start_pptp(void)
{
        FILE *fp_pptp;
        FILE *fp_server;
        FILE *fp_gw;
#ifdef RPPPOE
        FILE *fp_russia;
#endif
        char *wan_proto = nvram_safe_get("wan_proto");
        char *wan_eth = nvram_safe_get("wan_eth");
        char *wan_pptp_server_ip = nvram_safe_get("wan_pptp_server_ip");
        struct in_addr eth_addr;
        struct in_addr mask_addr;
        struct in_addr pptp_addr;
        int hostname_check_flag = 0;
        int dhcp_try_count = 0;

        DEBUG_MSG("Start PPTP\n");

        system("MODULES=`lsmod | grep pptp` && [ -n \"$MODULES\" ] || insmod /lib/modules/pptp.ko");

	 /* Delete iptable rule,when can't connect pptp server. */
        _system("iptables -D OUTPUT -p icmp --icmp-type port-unreachable -d %s -j DROP",nvram_safe_get("wan_pptp_server_ip"));
        /* In order to prevent the pptp server(linux) send icmp(port-unreachable) packet due can't connect*/
        _system("iptables -A OUTPUT -p icmp --icmp-type port-unreachable -d %s -j DROP",nvram_safe_get("wan_pptp_server_ip"));

        if(nvram_match("wan_pptp_dynamic", "1") == 0 )
        {
                while(dhcp_try_count < 3)
                {
                        sleep(5);//NickChou, wait for udhcpc get wan_eth IP
                        if(!get_ipaddr(wan_eth))
                        {
                                printf("PPTP : wan_eth don't have IP Address:%d\n",dhcp_try_count);
                                dhcpc_release();
                                dhcpc_renew();
                                dhcp_try_count++;

                        }
                        else
                        {
                                printf("PPTP : wan_eth have IP Address!!!");
                                break;
                        }
                        if(dhcp_try_count >= 3)
                                return 0;

                }

        }
        else // Static PPTP
        {
#ifdef RPPPOE
                if( nvram_match("wan_pptp_russia_enable", "0") == 0)
#endif
                {
                        /*
                                NickChou,For PPP adding host gateway routing rule in ppp\ipcp.c
                                when PPTP Server Eth IP = PPP Gateway IP
                        */
                        fp_gw = fopen("/var/tmp/pptp_staic_gw.txt", "w+");
                        if(fp_gw)
                        {
                                DEBUG_MSG("save pptp server ip to /var/tmp/pptp_staic_gw.txt\n");
                                fputs(nvram_safe_get("wan_pptp_gateway"), fp_gw);
                                fclose(fp_gw);
                        }
                        else
                                printf("open /var/tmp/pptp_staic_gw.txt fail\n");
                }
        }

        if( inet_aton(get_ipaddr(wan_eth), &eth_addr) )
        {
                printf("wan eth addr = %s\n", inet_ntoa(eth_addr));
                if( inet_aton(get_netmask(wan_eth), &mask_addr) )
        {
                        printf("wan mask addr =%s\n", inet_ntoa(mask_addr));

                        if( inet_aton(wan_pptp_server_ip, &pptp_addr) == 0 )
                        {
                                /*
                                        ex. if wan_pptp_server_ip = azaitsev.int.msk.dlink.ru
                                */
                                struct hostent *host = gethostbyname(wan_pptp_server_ip);

                                printf("gethostbyname: pptp server addr\n");
                                hostname_check_flag = 1;

                                if (host == NULL)
                                {
                                        dhcpc_release();
                                        dhcpc_renew();
                                        sleep(3);

                                        host=gethostbyname(wan_pptp_server_ip);
                                        if(host==NULL)
                                        {
                                        perror("start_pptp");
                                        return 0;
                                }
                                        else
                                        {
                                                memset(&pptp_addr, 0, sizeof(pptp_addr));
                                                memcpy(&pptp_addr.s_addr, host->h_addr, 4);

                                        }
                                }
                                else if (host->h_addrtype != AF_INET)
                                {
                                        printf("%s has non-internet address\n", wan_pptp_server_ip);
                                        return 0;
                                }
                                else
                                {
                                         memset(&pptp_addr, 0, sizeof(pptp_addr));
                                         memcpy(&pptp_addr.s_addr, host->h_addr, 4);
                                }
                        }
                        printf("pptp server addr = %s\n", inet_ntoa(pptp_addr));
                        printf("pptp server domain name = %s\n", wan_pptp_server_ip);

                if( (eth_addr.s_addr & mask_addr.s_addr) != (pptp_addr.s_addr & mask_addr.s_addr) )
                {
                                printf("pptp server is not in the same subnet with pptp client\n");
                                /* NickChou modify
                                   if we convert server host name to IP => hostname_check_flag == 1
                                   we add routing with IP address => inet_ntoa(pptp_addr)
                                   becauese russia server has many IP,
                                   if we use host name to add routing or dial, it sometime error
                                */
                                if(hostname_check_flag == 1)
                                        add_host_gw(wan_proto, inet_ntoa(pptp_addr), wan_eth);
                                else
                        add_host_gw(wan_proto, wan_pptp_server_ip, wan_eth);

                        sleep(1);
                }
                        else
                        {
                                /*NickChou, Save wan_pptp_server_ip to /var/tmp/pptp_server_ip.txt
                                For PPP adding host gateway routing rule.
                                For RUSSIA PPTP or PPP in multi-router enviroment
                                or wan_pptp_server_ip = PPP gateway ip ( in ppp\pppd\ipcp.c )
                                */
                                fp_server = fopen("/var/tmp/pptp_server_ip.txt", "w+");
                                if(fp_server)
                                {
                                        DEBUG_MSG("save wan_pptp_server_ip to /var/tmp/pptp_server_ip.txt\n");
                                        fputs(wan_pptp_server_ip, fp_server);
                                        fclose(fp_server);
                                }
                                else
                                        printf("open /var/tmp/pptp_server_ip.txt fail\n");
                        }
                }
        }
/* Chun add: record if russia enable for ppp/ipcp.c to decide how to modify resolve.conf */
#ifdef RPPPOE
        fp_russia = fopen("/var/tmp/russia_enable.txt", "w+");
        if(fp_russia)
        {
                if ((!nvram_match("wan_pptp_russia_enable", "1") && !nvram_match("wan_proto", "pptp")))
                        fprintf(fp_russia, "1");
                else
                        fprintf(fp_russia, "0");
                fclose(fp_russia);
        }
        else
                printf("open /var/tmp/russia_enable.txt fail\n");
#endif

        /*NickChou, prevent PPTP is dialing, but ppp0 still don't have ip*/
        fp_pptp = fopen(PPTP_PID, "r");
        if(fp_pptp==NULL)
        {
                //_system("pppd linkname pptp unit 0 pty \"pptp %s --nolaunchpppd \" &", wan_pptp_server_ip );
                /*albert for using pptp plugin we chage to below script */

                /*murat: to use the pptp-client package, which is already ported for no-mmu processors, we need to use the following call*/
                //_system("pptp %s linkname pptp unit 0 &", wan_pptp_server_ip );

                if(hostname_check_flag == 1)
                        _system("pppd linkname pptp unit 0 plugin \"/lib/pptp.so\" pptp_server %s &", inet_ntoa(pptp_addr) );
                else
                        _system("pppd linkname pptp unit 0 plugin \"/lib/pptp.so\" pptp_server %s &", wan_pptp_server_ip );

            sleep(1);
        }
        else
        {
                printf("PPTP is already connecting\n");
                fclose(fp_pptp);
        }

        return 0;
}

int stop_pptp(void)
{
        DEBUG_MSG("Stop PPTP\n");
        KILL_APP_ASYNCH("pppd");
        sleep(1);//NickChou wait for delete /var/run/ppp0.pid
        unlink("/var/tmp/pptp_server_ip.txt");
        unlink("/var/tmp/pptp_staic_gw.txt");
		nvram_set("support_pptp_mppe","0"); 
#ifdef RPPPOE
        unlink("/var/tmp/russia_enable.txt");
        if ((!nvram_match("wan_pptp_russia_enable", "1") && !nvram_match("wan_proto", "pptp")))
        {
                /*NickChou, 20080717
                  when ppp disconnect, we have to keep original wan physical dns and delete ppp dns
                */
                _system("cp -f %s %s", RUSSIA_PHY_RESOLVFILE, RESOLVFILE);
        	if(nvram_match("wan_pptp_dynamic", "0") == 0 )
		{
			 _system("route add default gw %s dev %s",nvram_safe_get("wan_pptp_gateway"),nvram_safe_get("wan_eth"));

		}
        }
#endif
        return 0;
}

int start_l2tp(void)
{
        FILE *fp_l2tp;
        FILE *fp_server;
        FILE *fp_gw;
#ifdef RPPPOE
        FILE *fp_russia;
#endif
        char *wan_proto = nvram_safe_get("wan_proto");
        char *wan_eth = nvram_safe_get("wan_eth");
        char *wan_l2tp_server_ip = nvram_safe_get("wan_l2tp_server_ip");
        struct in_addr eth_addr;
        struct in_addr mask_addr;
        struct in_addr l2tp_addr;
        int hostname_check_flag = 0;

        DEBUG_MSG("Start L2TP\n");

        if(nvram_match("wan_l2tp_dynamic", "1") == 0 )
        {
                sleep(3);//NickChou, wait for udhcpc get wan_eth IP
                if(!get_ipaddr(wan_eth))
                {
                        printf("L2TP : wan_eth don't have IP Address\n");
                        dhcpc_release();
                        dhcpc_renew();
                        return 0;
                }
                }
        else // Static L2TP
        {
#ifdef RPPPOE
                if( nvram_match("wan_l2tp_russia_enable", "1") == 0)
#endif
                {
                        /*
                                NickChou,For PPP adding host gateway routing rule in ppp\ipcp.c
                                when L2TP Server Eth IP = PPP Gateway IP
                        */
                        fp_gw = fopen("/var/tmp/l2tp_staic_gw.txt", "w+");
                        if(fp_gw)
                        {
                                DEBUG_MSG("save l2tp server ip to /var/tmp/l2tp_staic_gw.txt\n");
                                fputs(nvram_safe_get("wan_l2tp_gateway"), fp_gw);
                                fclose(fp_gw);
                        }
                        else
                                printf("open /var/tmp/l2tp_staic_gw.txt fail\n");
                }
        }

        if( inet_aton(get_ipaddr(wan_eth), &eth_addr) )
        {
                if( inet_aton(get_netmask(wan_eth), &mask_addr) )
        {
        /* jimmy modified 20080721, to support l2tp server is a domain not ip */
        //      inet_aton(wan_l2tp_server_ip, &l2tp_addr);
                        if( inet_aton(wan_l2tp_server_ip, &l2tp_addr) == 0 )
                        {
                        /*
                           ex. if wan_pptp_server_ip = azaitsev.int.msk.dlink.ru
                                   ===>inet_aton(wan_l2tp_server_ip, &l2tp_addr) == 0 means get hostname
                         */

                        struct hostent *host = gethostbyname(wan_l2tp_server_ip);
                        printf("gethostbyname: l2tp server addr\n");
                        hostname_check_flag = 1;

                        if (host == NULL)
                        {

                                dhcpc_release();
                                dhcpc_renew();
                                sleep(3);

                                host=gethostbyname(wan_l2tp_server_ip);
                                if(host==NULL)
                                {
                                perror("start_l2tp");
                                return 0;
                                }
                                else
                                {
                                        memset(&l2tp_addr, 0, sizeof(l2tp_addr));
                                        memcpy(&l2tp_addr.s_addr, host->h_addr, 4);

                                }
                        }
                        else if (host->h_addrtype != AF_INET){
                                printf("%s has non-internet address\n", wan_l2tp_server_ip);
                                return 0;
                        }else{
                                memset(&l2tp_addr, 0, sizeof(l2tp_addr));
                                memcpy(&l2tp_addr.s_addr, host->h_addr, 4);
                        }
                }
                printf("l2tp server addr = %s\n", inet_ntoa(l2tp_addr));
                printf("l2tp server domain name = %s\n", wan_l2tp_server_ip);
        /* ------------------------------------------------------------------ */
                if( (eth_addr.s_addr & mask_addr.s_addr) != (l2tp_addr.s_addr & mask_addr.s_addr) )
        {
                        printf("l2tp server is not in the same subnet as l2tp client\n");
                        if(hostname_check_flag == 1)
                                add_host_gw(wan_proto, inet_ntoa(l2tp_addr), wan_eth);
                        else
                        add_host_gw(wan_proto, wan_l2tp_server_ip, wan_eth);

                        sleep(1);
                }
                        else
                        {
                                /*NickChou, Save l2tp server ip to /var/tmp/l2tp_server_ip.txt
                                For PPP adding host gateway routing rule.
                                For RUSSIA L2TP or PPP in multi-router enviroment or server ip = gateway ip
                                */
                                fp_server = fopen("/var/tmp/l2tp_server_ip.txt", "w+");
                                if(fp_server)
                                {
                                        DEBUG_MSG("save pptp server ip to /var/tmp/l2tp_server_ip.txt\n");
                                        fputs(wan_l2tp_server_ip, fp_server);
                                        fclose(fp_server);
                                }
                                else
                                        printf("open /var/tmp/l2tp_server_ip.txt fail\n");
                        }
                }
        }

/* NickChou add: record if russia enable for ppp/ipcp.c to decide how to modify resolve.conf */
#ifdef RPPPOE
        fp_russia = fopen("/var/tmp/russia_enable.txt", "w+");
        if(fp_russia)
        {
                if ((!nvram_match("wan_l2tp_russia_enable", "1") && !nvram_match("wan_proto", "l2tp")))
                        fprintf(fp_russia, "1");
                else
                        fprintf(fp_russia, "0");
                fclose(fp_russia);
        }
        else
                printf("open /var/tmp/russia_enable.txt fail\n");
#endif

        /*NickChou, prevent L2TP is dialing, but ppp0 still don't have ip*/
        fp_l2tp = fopen(L2TP_PID, "r");
        if(fp_l2tp==NULL)
        {
                char t_l2tp_ip[32]={};
                if (hostname_check_flag == 1)
                        sprintf(t_l2tp_ip, "%s", inet_ntoa(l2tp_addr));
                else
                        sprintf(t_l2tp_ip, "%s", wan_l2tp_server_ip);
//Albert modify //2008/10/21
//-----
                modify_l2tp_conf(t_l2tp_ip);
//-----
                system("l2tpd &");
                sleep(1);//NickChou, if no sleep => show error message
                _system("l2tp-control \"start-session %s\"", t_l2tp_ip);
        }
        else
        {
                printf("L2TP is already connecting\n");
                fclose(fp_l2tp);
        }

        return 0;
}

int stop_l2tp(void)
{
        DEBUG_MSG("Stop L2TP\n");
        KILL_APP_ASYNCH("l2tpd");
        sleep(1);//NickChou wait for delete /var/run/ppp0.pid
        unlink("/var/tmp/l2tp_server_ip.txt");
        unlink("/var/tmp/l2tp_staic_gw.txt");
#ifdef RPPPOE
        unlink("/var/tmp/russia_enable.txt");
        if ((!nvram_match("wan_l2tp_russia_enable", "1") && !nvram_match("wan_proto", "l2tp")))
        {
                /*NickChou, 20080717
                  when ppp disconnect, we have to keep original wan physical dns and delete ppp dns
                */
                _system("cp -f %s %s", RUSSIA_PHY_RESOLVFILE, RESOLVFILE);
        }
#endif
        return 0;
}

/*
*       Date: 2009-1-19
*       Name: Ken_Chiang
*       Reason: modify for 3g usb client card to used.
*       Notice :
*/
#ifdef CONFIG_USER_3G_USB_CLIENT
#if defined(CONFIG_USER_3G_USB_CLIENT_WM5) || defined(CONFIG_USER_3G_USB_CLIENT_IPHONE)
/*	Date: 2010-01-13
*	Name: Jimmy Huang
*	Reason:	Add support for Windows Mobile 5
*	Note:
*			Currently support CHT 9100 with Windows Mobile Phone 5
*				Before activate WM5's Internet Sharing
*				VendorID: 0bb4
*				ProductID:0b05
*				Once activate WM5's Internet Sharing
*				VendorID: 0bb4
*				ProductID:0303
*				We using ProductID:0303 to check if the device is ready
*			Pid, Vid is generated by AR7161/mips-linux-2.6.15/drivers/usb/core/hub.c
*			driver status is generated by AR7161/mips-linux-2.6.15/drivers/usb/net/usbnet.c
*			driver is using rndis_host
*/
int start_usb_mobile_phone(int phone_type){
/*
	system("insmod /lib/modules/2.6.28.10/kernel/drivers/usb/net/usbnet.ko");
	system("insmod /lib/modules/2.6.28.10/kernel/drivers/usb/net/cdc_ether.ko");	
	system("insmod /lib/modules/2.6.28.10/kernel/drivers/usb/net/rndis_host.ko");
	system("insmod /lib/modules/2.6.28.10/kernel/drivers/usb/net/iphone.ko");
	system("[ -n \"`ps | grep udhcpc`\" ] && killall udhcpc");
*/
/*	Date:	2010-02-05
 *	Name:	Cosmo Chang
 *	Reason:	add usb_type=5 for Android Phone RNDIS feature
 */
	if(phone_type == 3 || phone_type == 5){ // we using usb_type as phone_type
		//system("[ -n \"`lsmod | grep NetUSB`\" ] && rmmod NetUSB");
#ifdef IP8000
		system("[ -n \"`lsmod | grep rndis_host`\" ] || insmod /lib/modules/2.6.36+/kernel/drivers/usb/net/rndis_host.ko");
#else
		system("[ -n \"`lsmod | grep rndis_host`\" ] || insmod /lib/modules/2.6.28.10/kernel/drivers/usb/net/rndis_host.ko");
#endif
	}else if(phone_type == 4){ // we using usb_type as phone_type
		//system("[ -n \"`lsmod | grep NetUSB`\" ] && rmmod NetUSB");
		system("[ -n \"`mount | grep /proc/bus/usb`\" ] || mount -t usbfs none /proc/bus/usb");
		system("[ -n \"`lsmod | grep iphone`\" ] || insmod /lib/modules/iphone.ko");
	}

	return 1;
}

int stop_usb_mobile_phone(void){
	system("[ -n \"`lsmod | grep rndis_host`\" ] && rmmod rndis_host.ko");
	system("[ -n \"`lsmod | grep iphone`\" ] && rmmod iphone.ko");
	//system("[ -n \"`lsmod | grep cdc_ether`\" ] && rmmod cdc_ether.ko");
	system("[ -n \"`mount | grep /proc/bus/usb`\" ] && mount -t usbfs none /proc/bus/usb");
	return 1;
}
#endif // end defined(CONFIG_USER_3G_USB_CLIENT_WM5) || defined(CONFIG_USER_3G_USB_CLIENT_IPHONE)
int ttyUSB_Num_old=0;
int start_usb3g(void)
{
        FILE *fp_usb3g;
        FILE *fp_usb_proid;
        FILE *fp_usb_venid;
        char usbProductID[10];
        char usbVendorID[10];
        usb_3g_device_table_t *p = NULL;
        int find_3g_usb=0;
	int is_dwm_152_156 = 0;
	int ttyUSB_Num=0;

        DEBUG_MSG("Start usb3g\n");

        fp_usb_venid = fopen(USB_DEVICE_VendorID,"r");
        if(fp_usb_venid==NULL)
                {
                printf("start_usb3g: open %s fail\n",USB_DEVICE_VendorID);
                return 0;
                        }

        fp_usb_proid = fopen(USB_DEVICE_ProductID,"r");
        if(fp_usb_proid==NULL)
                        {
                printf("start_usb3g: open %s fail\n",USB_DEVICE_ProductID);
                                fclose(fp_usb_venid);
                                return 0;
                        }

        fread(usbVendorID, sizeof(usbVendorID), 1, fp_usb_venid);
        if(strlen(usbVendorID) < 2)
                {
                        printf("start_usb3g: check usbVendorID fail\n");
                        fclose(fp_usb_venid);
                        return 0;
                }

                fread(usbProductID, sizeof(usbProductID), 1, fp_usb_proid);
        if(strlen(usbProductID) < 2)
                {
                printf("start_usb3g: check usbProductID fail\n");
                fclose(fp_usb_venid);
                                fclose(fp_usb_proid);
                                return 0;
                        }

        for(p = usb_3g_device_table; strncmp(p->vendorID,"NULL",4) ;p++)
                {
                DEBUG_MSG("%s, supported vid = %s, pid = %s\n",__FUNCTION__,p->vendorID,p->productID);
                if((strncmp(usbVendorID,p->vendorID,strlen(p->vendorID))==0) &&
                   (strncmp(usbProductID,p->productID,strlen(p->productID))==0))
                {
                        DEBUG_MSG("%s, Match 3G USB Device !\n",__FUNCTION__);
                        find_3g_usb=1;
			if(strncmp(usbProductID,"3e01",strlen(p->productID))==0
			|| strncmp(usbProductID,"3e02",strlen(p->productID))==0 )
				is_dwm_152_156 = 1;

			if(strncmp(usbProductID,"7e11",strlen(p->productID))==0 )//DWM-152/156A3
				ttyUSB_Num=2;

                        break;
                }
        }
	if(ttyUSB_Num != ttyUSB_Num_old)
	{
		_system("sed -i 's/ttyUSB%d/ttyUSB%d/' %s",ttyUSB_Num_old,ttyUSB_Num,PPP_3G_DIAL);
		ttyUSB_Num_old = ttyUSB_Num;

	}
        if(!find_3g_usb)
        {
                printf("%s, 3G USB Device not found !\n",__FUNCTION__);
                DEBUG_MSG("usbVendorID=%s, usbProductIDpid=%s\n", usbVendorID, usbProductID);
                fclose(fp_usb_venid);
                fclose(fp_usb_proid);
                return 0;
        }

        fclose(fp_usb_venid);
        fclose(fp_usb_proid);

#if 0
                _system("insmod /lib/modules/2.6.28.10/kernel/drivers/usb/serial/usbserial.ko product=%s vendor=%s",
                        usbProductID, usbVendorID);
#else
#ifdef IP8000
        system("insmod /lib/modules/2.6.36+/kernel/drivers/usb/serial/usbserial.ko");
#else
        system("insmod /lib/modules/2.6.28.10/kernel/drivers/usb/serial/usbserial.ko");
#endif
#endif

#ifdef IP8000
                system("insmod /lib/modules/2.6.36+/kernel/drivers/usb/serial/option.ko");
#else
                system("insmod /lib/modules/2.6.28.10/kernel/drivers/usb/serial/option.ko");
#endif
        sleep(1);

        fp_usb3g = fopen(USB_3G_PID, "r");
        if(fp_usb3g==NULL)
        {
		char usb3g_pin_cmd[40] = {};
		char *usb3g_pin = nvram_safe_get("usb3g_pin");
		int use_pin = 0;
		int need_pin = 0;
		FILE *fp_use_pin;
		
		/*For SIM Card need PIN number*/
		if(strlen(usb3g_pin) > 0)
		{
			use_pin = 1;
		 	sprintf(usb3g_pin_cmd, "OK \'AT+CPIN=%s\'\n", usb3g_pin);
		 	
			/*
			Only enter PIN once for un-lock SIM Card
			If PPP_3G_USE_PIN exist => we have entered PIN 
			Remove it when un-plug USB Card (check by wantimer) 
			*/
			fp_use_pin = fopen(PPP_3G_USE_PIN, "r");
			if(fp_use_pin == NULL)
			{
				need_pin = 1;
				init_file(PPP_3G_USE_PIN); 
			}	
		}
		
                /*NickChou Add, 2009.05.19
                Because PPPoE/L2TP/PPTP use /var/etc/options for options_from_file().
                3G use /var/etc/options and /var/etc/ppp/peers/3g-dial for options_from_file().
                So if WAN Proto change from PPPoE/L2TP/PPTP to 3G,
                we need to clear /var/etc/options (prevent options setting conflict)
                */
                init_file(PPP_OPTIONS);

		/* creat PPP_3G_CONNECT_CHAT file*/
		init_file(PPP_3G_CONNECT_CHAT);
	
		save2file(PPP_3G_CONNECT_CHAT, 
			"\'\' AT\n"
			"%s"
			"OK ATZ\n"
			"%s" //Add for DWM-152/156 1.04 driver
			"%s"
			"OK \'AT+CGDCONT=1,\"IP\",\"%s\"\'\n"
			"OK ATD%s\n"
			"CONNECT \'\'\n"
			,(use_pin && need_pin) ? usb3g_pin_cmd : "\n"
			,is_dwm_152_156? "OK \'AT+CFUN=1\'\n" : "\n"
			,is_dwm_152_156? "OK \'AT*QSTART=0\'\n" : "\n"
			,nvram_safe_get("usb3g_apn_name")
			,nvram_safe_get("usb3g_dial_num"));	
				
                system("/sbin/pppd call 3g-dial &");
        }
        else
        {
                printf("USB 3G is already connecting\n");
                fclose(fp_usb3g);
        }

        return 0;
}

int stop_usb3g(void)
{
        printf("Stop usb3g\n");
        KILL_APP_ASYNCH("pppd");
        /*
                sleep for
                1. delete /var/run/ppp0.pid
                2.  USB module deregister USB card.
        */
        sleep(1);
        system("rmmod option");
        system("rmmod usbserial");
        sleep(1);
        return 0;
}
#endif

#ifdef MPPPOE
int start_pppoe(int ppp_ifunit)
{
        char service_name[128] = "";
        char tmp_service_name[128] = "";
        char *sn;

        DEBUG_MSG("Start PPPoE %d\n", ppp_ifunit);
        sprintf(service_name,"wan_pppoe_service_%02d",ppp_ifunit);
        sn = nvram_safe_get(service_name);
        strcpy(tmp_service_name, sn);

        if(strcmp(sn,"") == 0){
        _system("pppd linkname %02d file /var/etc/options_%02d unit %d %s &", ppp_ifunit, ppp_ifunit, ppp_ifunit, nvram_get("wan_eth"));
        }else{
        _system("pppd linkname %02d file /var/etc/options_%02d unit %d rp_pppoe_service \"%s\" rp_pppoe_ac \"%s\" %s  &", ppp_ifunit, ppp_ifunit, ppp_ifunit, parse_special_char(tmp_service_name),parse_special_char(tmp_service_name), nvram_get("wan_eth"));
        }
        sleep(5);
        return 0;
}
#else //#ifdef MPPPOE
//albert modify 20081024
//int start_pppoe(int ppp_ifunit, char *wan_eth, char *service_name)
int start_pppoe(int ppp_ifunit, char *wan_eth, char *service_name, char *ac_name)
{
        char tmp_service_name[128] = "";
//Albert add
        char tmp_ac_name[128] = "";
//---
        FILE *fp_pppoe;
#ifdef RPPPOE
        FILE *fp_russia;
#endif
        DEBUG_MSG("Start PPPoE %d\n", ppp_ifunit);

/* Chun add: record if russia enable for ppp/ipcp.c to decide how to modify resolve.conf */
#ifdef RPPPOE
        fp_russia = fopen("/var/tmp/russia_enable.txt", "w+");
        if(fp_russia)
        {
                if (!nvram_match("wan_pppoe_russia_enable", "1") && !nvram_match("wan_proto", "pppoe"))
                        fprintf(fp_russia, "1");
                else
                        fprintf(fp_russia, "0");
                fclose(fp_russia);
        }
        else
                printf("open /var/tmp/russia_enable.txt fail\n");
#endif

        //fp_pppoe = fopen("/var/run/ppp0.pid", "r");
        //if(fp_pppoe==NULL)
        if (!checkServiceAlivebyName("pppd"))
        {
                if( (strncmp(service_name, "null_sn", strlen("null_sn")) == 0))
                {
                        if (strncmp(ac_name, "null_ac", strlen("null_ac")) == 0)
                        {
                        _system("pppd linkname %02d file /var/etc/options_%02d unit %d %s &",
                                ppp_ifunit, ppp_ifunit, ppp_ifunit, wan_eth);
                        }
                        else
                        {
                                _system("pppd linkname %02d file /var/etc/options_%02d unit %d rp_pppoe_ac \"%s\" %s &",
                                        ppp_ifunit, ppp_ifunit, ppp_ifunit, parse_special_char(ac_name), wan_eth);
                }
                }

    else
    {
                strcpy(tmp_service_name, service_name);
                strcpy(tmp_ac_name, ac_name);

                        _system("pppd linkname %02d file /var/etc/options_%02d unit %d rp_pppoe_service \"%s\" rp_pppoe_ac \"%s\" %s &",
                                ppp_ifunit, ppp_ifunit, ppp_ifunit, tmp_service_name, tmp_ac_name, wan_eth);
        }

                sleep(1);//NickChou, wait for dial
        return 0;
    }
        else
        {
                printf("PPPoE is already connecting\n");
                //fclose(fp_pppoe);
                return 0;
        }
}
#endif

int stop_pppoe(int ppp_ifunit)
{
#ifdef MPPPOE
    struct mp_info mpppoe;
#endif

        DEBUG_MSG("Stop PPPoE %d\n", ppp_ifunit);
#ifdef MPPPOE
        mpppoe = check_mpppoe_info(ppp_ifunit);
        if( mpppoe.pid > 0)
                _system("kill %d &",mpppoe.pid);
#else
        KILL_APP_ASYNCH("pppd");
        sleep(1);//NickChou wait for delete /var/run/ppp0.pid
#ifdef RPPPOE
        unlink("/var/tmp/russia_enable.txt");
        if (!nvram_match("wan_pppoe_russia_enable", "1") && !nvram_match("wan_proto", "pppoe"))
        {
                /*NickChou, 20080717
                  when ppp disconnect, we have to keep original wan physical dns and delete ppp dns
                */
                _system("cp -f %s %s", RUSSIA_PHY_RESOLVFILE, RESOLVFILE);
                if(nvram_match("wan_pppoe_russia_dynamic", "0") == 0 )
		{
			 _system("route add default gw %s dev %s",nvram_safe_get("wan_pppoe_russia_gateway"),nvram_safe_get("wan_eth"));

		}

        }
#endif
#endif

        return 0;
}

int start_unnumbered(int ppp_ifunit)
{
        char service_name[128] = "";
        char tmp_service_name[128] = "";
        char *sn;

        DEBUG_MSG("Start Unnumbered %d\n", ppp_ifunit);
        sprintf(service_name,"wan_pppoe_service_%02d",ppp_ifunit);
        sn = nvram_safe_get(service_name);
        strcpy(tmp_service_name, sn);

        if(strcmp(sn,"") == 0){
                _system("pppd linkname %02d file /var/etc/options_%02d unit %d %s &", ppp_ifunit, ppp_ifunit, ppp_ifunit, nvram_get("wan_eth"));
        }else{
                 _system("pppd linkname %02d file /var/etc/options_%02d unit %d rp_pppoe_service \"%s\" rp_pppoe_ac \"%s\" %s &", ppp_ifunit, ppp_ifunit, ppp_ifunit, parse_special_char(tmp_service_name),parse_special_char(tmp_service_name), nvram_get("wan_eth"));
        }

        sleep(5);
        return 0;
}

int stop_unnumbered(int ppp_ifunit)
{
        struct mp_info mpppoe;

        DEBUG_MSG("Stop Unnumbered %d\n", ppp_ifunit);
        mpppoe = check_mpppoe_info(ppp_ifunit);
        if( mpppoe.pid > 0)
                _system("kill %d &",mpppoe.pid);

        return 0;
}

int stop_wan(void)
{
        char *wan_eth, *wan_proto;
        if(!( action_flags.all  || action_flags.wan ) )
                return 1;

        DEBUG_MSG("Stop WAN\n");
        wan_proto = nvram_safe_get("wan_proto");
        wan_eth = nvram_safe_get("wan_eth");

        KILL_APP_ASYNCH("udhcpc");
        /* charles : don't kill wantimer */
        //system("killall wantimer &");
        unlink("/var/tmp/bandwidth_result.txt");
        stop_redial();
        stop_monitor();

#ifdef CONFIG_USER_WAN_8021X
        stop_wan_supplicant();
#endif

#ifdef MPPPOE
        stop_ppptimer();
    for(i=0; i<MAX_PPPOE_CONNECTION; i++)
                stop_pppoe(i);
            _system("rm -f %s",DNS_FILE_00);
        _system("rm -f %s",DNS_FILE_01);
            _system("rm -f %s",DNS_FILE_02);
            _system("rm -f %s",DNS_FILE_03);
            _system("rm -f %s",DNS_FILE_04);
#else
           stop_pppoe(0);
#endif
        stop_pptp();
        stop_l2tp();
        stop_bigpond();

        /* ken added to clean wan addrs for wrong source IP for PPTP / L2TP / Russia PPTP/L2TP/PPPoE when Dynamic IP chang to Static IP */
        if( strcmp(wan_proto, "pptp") == 0 || strcmp(wan_proto, "l2tp") == 0 ){
                _system("ifconfig %s 0.0.0.0",wan_eth);
        }
        //stop_route(); //move to stop_firewall
#ifdef CONFIG_USER_3G_USB_CLIENT
        stop_usb3g();
        system("rmmod usbserial");
        system("rmmod option");
/*	Date: 2010-01-13
*	Name: Jimmy Huang
*	Reason:	Add support for Windows Mobile 5
*	Note:
*			Currently support CHT 9100 with Windows Mobile Phone 5
*				Before activate WM5's Internet Sharing
*				VendorID: 0bb4
*				ProductID:0b05
*				Once activate WM5's Internet Sharing
*				VendorID: 0bb4
*				ProductID:0303
*				We using ProductID:0303 to check if the device is ready
*			Pid, Vid is generated by AR7161/mips-linux-2.6.15/drivers/usb/core/hub.c
*			driver status is generated by AR7161/mips-linux-2.6.15/drivers/usb/net/usbnet.c
*			driver is using rndis_host
*/
#if defined(CONFIG_USER_3G_USB_CLIENT_WM5) || defined(CONFIG_USER_3G_USB_CLIENT_IPHONE)
	stop_usb_mobile_phone();
#endif // end CONFIG_USER_3G_USB_CLIENT_WM5 || CONFIG_USER_3G_USB_CLIENT_IPHONE
#endif
        return 0;
}



int ipup_main(int argc, char **argv)
{
#ifdef MPPPOE
        char ppp_unit[10] = {};
#endif
        int connected = 0;

        if( !strcmp(argv[1], "dhcp") )
                connected = wan_connected_cofirm( nvram_safe_get("wan_eth") );
        else
        {
#ifdef MPPPOE
                if( argv[2] )
                {
                        sprintf(ppp_unit, "ppp%d", atoi(argv[2]) );
                        connected = wan_connected_cofirm(ppp_unit);
                }
                else
#endif
                connected = wan_connected_cofirm("ppp0");
        }

        if(!connected)
        {
                if( !strcmp(argv[1], "dhcp") )
                        dhcpc_renew();
                else if( !strcmp(argv[1], "pppoe") )
#ifdef MPPPOE
                        start_pppoe( atoi(argv[2]));
#else
		{
			if(nvram_match("wan_pppoe_connect_mode_00","manual")!=0)
			{
				action_flags.wan=1;
				stop_wan();
				start_wan();
			}
			
			if(nvram_match("wan_pppoe_connect_mode_00","always_on")!=0)
                        start_pppoe( atoi(argv[2]), argv[3], argv[4], argv[5]);
		}
#endif
                else if( !strcmp(argv[1], "pptp") )
		{
			if(nvram_match("wan_pptp_connect_mode","manual")!=0)
			{
        			action_flags.wan=1;
				stop_wan();
				start_wan();
			}	
			
			if(nvram_match("wan_pptp_connect_mode","always_on")!=0)
                        start_pptp();
		}
                else if( !strcmp(argv[1], "l2tp") )
		{	
			if(nvram_match("wan_l2tp_connect_mode","manual")!=0)
			{
        			action_flags.wan=1;
				stop_wan();
				start_wan();
			}
			
			if(nvram_match("wan_l2tp_connect_mode","always_on")!=0)
                        start_l2tp();
		}
                else if( !strcmp(argv[1], "bigpond") )
		{	
                        start_bigpond();
                /*      Date: 2009-01-21
                *       Name: jimmy huang
                *       Reason: for usb-3g connect/disconnect via UI
                *       Note:
                *
                */
		}
#ifdef CONFIG_USER_3G_USB_CLIENT
                else if( strcmp(argv[1], "usb3g") == 0)
		{
			if(nvram_match("usb3g_reconnect_mode","manual")!=0)
			{
        			action_flags.wan=1;
				stop_wan();
				start_wan();
			}
			
			if(nvram_match("usb3g_reconnect_mode","always_on")!=0)
			{

                        //_system("/var/sbin/ip-up usb3g %d %s %s &", pppoe_unit, wan_eth, sn);
			KILL_APP_ASYNCH("pppd"); // prevent that Device ttyUSB0 is locked by pppd
                        start_usb3g();
                }
                }
#endif
        }

        return 0;
}

int ipdown_main(int argc, char **argv)
{
        if( !strcmp(argv[1], "dhcp") )
                dhcpc_release();
        else if( !strcmp(argv[1], "pppoe") )
        {
		if(nvram_match("wan_pppoe_connect_mode_00","manual")!=0)
		{
			action_flags.wan=1;
			stop_wan();
		}
		else
		{
                if( argv[2] )
                        stop_pppoe( atoi(argv[2]));
        }
        }
        else if( !strcmp(argv[1], "pptp") )
	{
		if(nvram_match("wan_pptp_connect_mode","manual")!=0)
        	{	
			action_flags.wan=1;
			stop_wan();
		}
		else
                stop_pptp();
	}
        else if( !strcmp(argv[1], "l2tp") )
	{
		if(nvram_match("wan_l2tp_connect_mode","manual")!=0)
		{
        		action_flags.wan=1;
			stop_wan();
		}
		else
                stop_l2tp();
	}
        else if( !strcmp(argv[1], "bigpond") )
                stop_bigpond();
        /*      Date: 2009-01-21
        *       Name: jimmy huang
        *       Reason: for usb-3g connect/disconnect via UI
        *       Note:
        *
        */
#ifdef CONFIG_USER_3G_USB_CLIENT
        else if( strcmp(argv[1], "usb3g") == 0)
	{
		if(nvram_match("usb3g_reconnect_mode","manual")!=0)
		{
        		action_flags.wan=1;
			stop_wan();
		}
		else

                /*
                Date: 2009-2-05
                Name: Nick Chou
                Reason: We only remove option.ko and usbserial.ko in two condition
                                        1. 3G USB Card disconnect from USB port.
                                        2. Change WAN type form USB3g to other
                                So we only kill pppd here to prevent "Connect script failed" error.
                */
                KILL_APP_ASYNCH("pppd");//stop_usb3g();
	}
#endif

        return 0;
}

#ifdef CONFIG_USER_WAN_8021X
int start_wan_supplicant(void)
{
        char *wan;
        char driver[10];
        char pid_file[20];

        char *wan_8021x_enable;
        char *wan_8021x_auth_type;
        char *wan_8021x_eap_method;
        char *wan_8021x_eap_tls_key_passwd;
        char *wan_8021x_eap_tls_username;
        char wan_8021x_eap_tls_username_tmp[20];
        char *wan_8021x_eap_ttls_inner_proto;
        char wan_8021x_eap_ttls_inner_proto_tmp[20];
        char *wan_8021x_eap_ttls_username;
        char *wan_8021x_eap_ttls_passwd;
        char *wan_8021x_eap_peap_inner_proto;
        char wan_8021x_eap_peap_inner_proto_tmp[20];
        char *wan_8021x_eap_peap_username;
        char *wan_8021x_eap_peap_passwd;

        /* prepare for wimax */
        char *wan_8021x_auth_anon_user;
        char *wan_8021x_auth_user;
        char *wan_8021x_auth_passwd;
        char *wan_8021x_auth_key_passwd;

        DEBUG_MSG("enter start_wan_supplicant\n");

        /* Date: 2009-05-20
         * kill supplicant first
         */

        wan = nvram_safe_get("wan_eth");
        wan_8021x_enable = nvram_safe_get("wan_8021x_enable");


        if(strcmp(wan_8021x_enable, "1") == 0) {
                DEBUG_MSG("wan_8021x_enable=1\n");
                wan_8021x_eap_method = nvram_safe_get("wan_8021x_eap_method");

                if (strcmp(wan_8021x_eap_method, "TLS") == 0) {
                        DEBUG_MSG("wan_8021x_eap_method=TLS\n");

        /*
         * generate and read conf/cert
         */

                        wan_8021x_eap_tls_username = nvram_safe_get("wan_8021x_eap_tls_username");
                        wan_8021x_eap_tls_key_passwd = nvram_safe_get("wan_8021x_eap_tls_key_passwd");

                        if(strlen(wan_8021x_eap_tls_username) == 0) {
                                memset(wan_8021x_eap_tls_username_tmp, 0, sizeof(wan_8021x_eap_tls_username_tmp));
                                strcpy(wan_8021x_eap_tls_username_tmp, "test@example.com");
                                wan_8021x_eap_tls_username = wan_8021x_eap_tls_username_tmp;
                        }

                        _system("nvram readfile %s", CA_CERTIFICATION);
                        _system("nvram readfile %s", CLIENT_CERTIFICATION);
                        _system("nvram readfile %s", PRIVATE_KEY);

                        if( strlen(wan_8021x_eap_tls_key_passwd)==0 ||
                                access(CA_CERTIFICATION, F_OK)!=0 ||
                                access(CLIENT_CERTIFICATION, F_OK)!=0 ||
                                access(PRIVATE_KEY, F_OK)!=0
                                ) {
                                printf("missing information, won't start wpa_supplicant\n");
                                return 0;
                        }

        /*
         * replace conf file with correct information
         */

                        _system("sed -e 's/eap=\"\"/eap=%s/; \
                                s/identity=\"\"/identity=\"%s\"/; \
                                s/private_key_passwd=\"\"/private_key_passwd=\"%s\"/; \
                                ' %s > %s",
                                wan_8021x_eap_method,
                                wan_8021x_eap_tls_username,
                                wan_8021x_eap_tls_key_passwd,
                                WAN_WPA_CONFIG_TPL, WPASUP_CONFIG);

                } else if (strcmp(wan_8021x_eap_method, "TTLS") == 0) {
                        DEBUG_MSG("wan_8021x_eap_method=TTLS\n");

        /*
         * generate and read conf/cert
         */

                        wan_8021x_eap_ttls_username = nvram_safe_get("wan_8021x_eap_ttls_username");
                        wan_8021x_eap_ttls_passwd = nvram_safe_get("wan_8021x_eap_ttls_passwd");
                        wan_8021x_eap_ttls_inner_proto = nvram_safe_get("wan_8021x_eap_ttls_inner_proto");

                        _system("nvram readfile %s", CA_CERTIFICATION);

                        if( strlen(wan_8021x_eap_ttls_username) == 0 ||
                                strlen(wan_8021x_eap_ttls_passwd) == 0 ||
                                access(CA_CERTIFICATION, F_OK)!=0 ) {
                                printf("missing information, won't start wpa_supplicant\n");
                                return 0;
                        }

                        if( strlen(wan_8021x_eap_ttls_inner_proto) > 0 ) {
                                memset(wan_8021x_eap_ttls_inner_proto_tmp, 0, sizeof(wan_8021x_eap_ttls_inner_proto_tmp));
                                sprintf(wan_8021x_eap_ttls_inner_proto_tmp, "auth=%s", wan_8021x_eap_ttls_inner_proto);
                        } else {
                                memset(wan_8021x_eap_ttls_inner_proto_tmp, 0, sizeof(wan_8021x_eap_ttls_inner_proto_tmp));
                                strcpy(wan_8021x_eap_ttls_inner_proto_tmp, wan_8021x_eap_ttls_inner_proto);
                        }

        /*
         * replace conf file with correct information
         */

                        _system("sed -e 's/eap=\"\"/eap=%s/; \
                                s/identity=\"\"/identity=\"%s\"/; \
                                s/password=\"\"/password=\"%s\"/; \
                                s/client_cert=/#client_cert=/; \
                                s/private_key=/#private_key=/; \
                                s/private_key_passwd=/#private_key_passwd=/; \
                                s/phase2=\"\"/phase2=\"%s\"/; \
                                ' %s > %s",
                                wan_8021x_eap_method,
                                wan_8021x_eap_ttls_username,
                                wan_8021x_eap_ttls_passwd,
                                wan_8021x_eap_ttls_inner_proto_tmp,
                                WAN_WPA_CONFIG_TPL, WPASUP_CONFIG);


                } else if (strcmp(wan_8021x_eap_method, "PEAP") == 0) {
                        DEBUG_MSG("wan_8021x_eap_method=PEAP\n");

        /*
         * generate and read conf/cert
         */

                        wan_8021x_eap_peap_username = nvram_safe_get("wan_8021x_eap_peap_username");
                        wan_8021x_eap_peap_passwd = nvram_safe_get("wan_8021x_eap_peap_passwd");
                        wan_8021x_eap_peap_inner_proto = nvram_safe_get("wan_8021x_eap_peap_inner_proto");

                        _system("nvram readfile %s", CA_CERTIFICATION);

                        if( strlen(wan_8021x_eap_peap_username) == 0 ||
                                strlen(wan_8021x_eap_peap_passwd) == 0 ||
                                access(CA_CERTIFICATION, F_OK)!=0 ) {
                                printf("missing information, won't start wpa_supplicant\n");
                                return 0;
                        }

                        if( strlen(wan_8021x_eap_peap_inner_proto) > 0 ) {
                                memset(wan_8021x_eap_peap_inner_proto_tmp, 0, sizeof(wan_8021x_eap_peap_inner_proto_tmp));
                                sprintf(wan_8021x_eap_peap_inner_proto_tmp, "auth=%s", wan_8021x_eap_peap_inner_proto);
                        } else {
                                memset(wan_8021x_eap_peap_inner_proto_tmp, 0, sizeof(wan_8021x_eap_peap_inner_proto_tmp));
                                strcpy(wan_8021x_eap_peap_inner_proto_tmp, wan_8021x_eap_peap_inner_proto);
                        }

        /*
         * replace conf file with correct information
         */

                        _system("sed -e 's/eap=\"\"/eap=%s/; \
                                s/identity=\"\"/identity=\"%s\"/; \
                                s/password=\"\"/password=\"%s\"/; \
                                s/client_cert=/#client_cert=/; \
                                s/private_key=/#private_key=/; \
                                s/private_key_passwd=/#private_key_passwd=/; \
                                s/phase2=\"\"/phase2=\"%s\"/; \
                                ' %s > %s",
                                wan_8021x_eap_method,
                                wan_8021x_eap_peap_username,
                                wan_8021x_eap_peap_passwd,
                                wan_8021x_eap_peap_inner_proto_tmp,
                                WAN_WPA_CONFIG_TPL, WPASUP_CONFIG);
                }

                usleep (500);

                if (strcmp(wan, "eth0") == 0 || strcmp(wan, "eth1") == 0) { // Driver=wired
                        memset(driver, 0, sizeof(driver));
                        strcpy(driver, "wired");
                } else if (strcmp(wan, "ath0") == 0 || strcmp(wan, "ath1") == 0 ) { //Driver=others
                        // empty function
                }

        /*
         * start supplicant for wan
         */
                _system("wpa_supplicant -i %s -D %s -c %s -P %s -B &", wan, driver, WPASUP_CONFIG, WPASUP_PID);
                return 1;
        } else { // wan_8021x_enable= 0
                DEBUG_MSG("wan_8021x_enable=0\n");

                return 0;
        }
}

int stop_wan_supplicant(void) {
        _system("rm /var/etc/wpa.conf");
        KILL_APP_ASYNCH("wpa_supplicant");
        return 0;
}
#endif


#ifdef CONFIG_USER_TC
void get_measure_bandwidth(char *bandwidth)
{
        FILE *fp;
        char returnVal[32] = {0};

        fp = fopen("/var/tmp/bandwidth_result.txt","r");
        if (fp)
        {
                fread(returnVal, sizeof(returnVal), 1, fp);
                DEBUG_MSG("%s", returnVal);
                if(strlen(returnVal) > 1)
                        strcpy(bandwidth, returnVal);
                else
                        strcpy(bandwidth, "0");
                fclose(fp);
        }
        else
        {
                strcpy(bandwidth, "0");
        }

        return ;
}

#endif
