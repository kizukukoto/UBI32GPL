/*
 * $Id: app.c,v 1.144 2009/08/19 03:29:23 peter_pan Exp $
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
#include <linux_vct.h>
#include <rc.h>

extern struct action_flag action_flags;

#ifdef RC_DEBUG
#define DEBUG_MSG(fmt, arg...)       printf("%s [%d]: "fmt, __FILE__, __LINE__, ##arg)
#else
#define DEBUG_MSG(fmt, arg...)
#endif

#ifdef IPv6_TEST
int first_send_dadns_flag = 1;
#endif

int start_dhcpc(void) /*for Pure AP*/
{
        char *hostname;
        char tmp_hostname[40];
        char tmp_netbios[3]={0};
        char tmp_ucast[3]={0};
        char tmp_apmode[3]={0};
        char tmp_automodeselect[3]={0};

        DEBUG_MSG("start_dhcpc\n");

        hostname = nvram_safe_get("hostname");
        if (strlen(hostname) > 1)
                strcpy(tmp_hostname, hostname);
        else
                strcpy(tmp_hostname, nvram_safe_get("model_number"));

        /* dhcpc Relay */
        if( !nvram_match( "dhcpc_enable", "1" ) )
        {
                DEBUG_MSG("start_dhcpc and start_lanmon\n");

#ifdef  UDHCPD_NETBIOS
                if((nvram_match("dhcpd_netbios_enable", "1") == 0 ) && (nvram_match("dhcpd_netbios_learn", "1") == 0 ))
                        strcpy(tmp_netbios,"-N");
#endif
#ifdef  UDHCPC_UNICAST
                if(nvram_match("dhcpc_use_ucast", "1") == 0 )
                        strcpy(tmp_ucast,"-u");
#endif
#ifdef  CONFIG_BRIDGE_MODE
                if(nvram_match("wlan0_mode", "ap") == 0 )
                        strcpy(tmp_apmode,"-a");
/*      Date: 2009-04-10
*       Name: Ken Chiang
*       Reason: Add support for enable auto mode select(router mode/ap mode).
*       Note:
*/
#ifdef AUTO_MODE_SELECT
                DEBUG_MSG("AUTO_MODE_SELECT\n");
                if(nvram_match("auto_mode_select", "1") == 0 ){
                        DEBUG_MSG("automodeselect enable\n");
                        strcpy(tmp_automodeselect,"-A");
                }
#endif
#endif
/*      Date: 2009-04-10
*       Name: Ken Chiang
*       Reason: modify support for enable auto mode select(router mode/ap mode).
*       Note:
*/
/*
                _system("udhcpc -w dhcpc -i %s -H \"%s\" -s %s %s %s %s &",
                        nvram_safe_get("lan_bridge"), parse_special_char(tmp_hostname),DHCPC_DNS_SCRIPT,
                        tmp_netbios, tmp_ucast, tmp_apmode);
*/
                DEBUG_MSG("automodeselect=%s\n",tmp_automodeselect);
                _system("udhcpc -w dhcpc -i %s -H \"%s\" -s %s %s %s %s %s &",
                        nvram_safe_get("lan_bridge"), parse_special_char(tmp_hostname),DHCPC_DNS_SCRIPT,
                        tmp_netbios, tmp_ucast, tmp_apmode, tmp_automodeselect);

                start_lanmon();
        }

        return 0;
}

int stop_dhcpc(int flag)
{
        DEBUG_MSG("Stop dhcpc\n");

        if( !nvram_match("dhcpc_enable", "0") || flag )
        {
                KILL_APP_SYNCH("udhcpc");
                stop_lanmon(0);
        }
        return 0;
}

int start_dnsmasq(void)
{
        char *lan_bridge=nvram_safe_get("lan_bridge");
	char *lan_ipaddr=nvram_safe_get("lan_ipaddr");
//        char open_dns_opt[10]={0};
// opendns need more memory
// IFdef OPENDNS
        char open_dns_opt[128]={0};
	char w4r_opt[40]={0};


#if defined(RPPPOE) || defined(CONFIG_WEB_404_REDIRECT)
        char *wan_proto=NULL;
#endif

#ifdef MPPPOE
        char main_session_resolve_file[] = "/var/etc/resolve_XY";
        sprintf(main_session_resolve_file,"/var/etc/resolve_%02d",atoi(nvram_safe_get("wan_pppoe_main_session")));
        _system("cp -f %s %s",main_session_resolve_file,RESOLVFILE);
#endif

                /* DNS Relay */
        if( !nvram_match( "dns_relay", "1" ) )
        {
#ifdef OPENDNS
			int opendns;

			opendns=0;

		if( nvram_match("opendns_enable","1")==0 && 
			nvram_match("wan_specify_dns","1")==0 &&
			nvram_match("wan_primary_dns","204.194.232.200")==0 &&
			nvram_match("wan_secondary_dns","204.194.234.200")==0 
		) {
                        sprintf(open_dns_opt, " -I %s", nvram_safe_get("wan_eth"));
			opendns = 1;
		}

// Family Sield
		if( nvram_match("opendns_enable","1")==0 && 
			nvram_match("wan_specify_dns","1")==0 &&
			nvram_match("wan_primary_dns","208.67.222.123")==0 &&
			nvram_match("wan_secondary_dns","208.67.220.123")==0 
		) {
                        sprintf(open_dns_opt, " -I %s", nvram_safe_get("wan_eth"));
			opendns = 1;
		}
// Partial Control
		if( nvram_safe_get("opendns_mode"))
		if( nvram_match("opendns_enable","1")==0 && 
			nvram_match("opendns_mode","2")==0 &&
			nvram_match("wan_specify_dns","1")==0 //&&
//			nvram_match("wan_primary_dns","204.194.232.200")==0 &&
//			nvram_match("wan_secondary_dns","204.194.234.200")==0 
		) {
			if( nvram_safe_get("opendns_deviceid") ) {
				if( strlen(nvram_safe_get("opendns_deviceid")) > 1) {
                        sprintf(open_dns_opt, " -I %s -F 2 -G %s -J %s", nvram_safe_get("wan_eth"),nvram_safe_get("lan_mac"),nvram_safe_get("opendns_deviceid"));
				}
				else {
                        sprintf(open_dns_opt, " -I %s -F 2 -G %s ", nvram_safe_get("wan_eth"),nvram_safe_get("lan_mac"));
				}
				opendns = 1;
			}
		}

		// add =d for debug use
		if( opendns) {
			sprintf(open_dns_opt,"%s -W %s -d",open_dns_opt,nvram_safe_get("wan_proto"));
		}


// 2011.01.05
// dlinkrouter.cameo.com.tw issue 
// printf("->200\n");
#if 1
		{

			FILE *fp = fopen("/var/tmp/dhcpc_dns.tmp","r");
			char dns_name_input[64]={0};
			char dns_name_all[128]={0};

			//dns_name_input[0] = 0; // null string
			//dns_name_input[32] = 0; // null string

			if(fp ) {
				/* save lease value for dhcp client */						
				fgets(dns_name_input, 32, fp);
				fclose(fp);
			}
			
			strcpy(dns_name_all,nvram_safe_get("lan_device_name"));

			if( strlen(dns_name_input) > 0) {
				sprintf(dns_name_all,"%s.%s",nvram_safe_get("lan_device_name"),dns_name_input);
			}

///var/tmp/hosts
			fp = fopen("/var/tmp/hosts","w+");
			if(fp ) {
				fprintf(fp,"%s %s\n",nvram_safe_get("lan_ipaddr"),dns_name_all);
				fclose(fp);				
			}

			sprintf(open_dns_opt,"%s -H %s",open_dns_opt,"/var/tmp/hosts");

		
		}
#endif
#endif

#ifdef CONFIG_WEB_404_REDIRECT
		wan_proto = nvram_safe_get("wan_proto");
		if (strcmp(wan_proto,"static")==0 ||
		    strcmp(wan_proto,"dhcpc")==0 ||
		    strcmp(wan_proto,"usb3g_phone")==0)
			sprintf(w4r_opt, " --w4r-wan-interface=%s", nvram_safe_get("wan_eth"));
		else if(strcmp(wan_proto,"pppoe")==0 ||
		    strcmp(wan_proto,"pptp")==0 ||
		    strcmp(wan_proto,"l2tp")==0 ||
		    strcmp(wan_proto,"usb3g")==0 )
			strcpy(w4r_opt, " --w4r-wan-interface=ppp0");
#endif

#ifdef RPPPOE /* jimmy modified 20080611 */
                wan_proto = nvram_safe_get("wan_proto");
                if( ( !nvram_match("wan_pptp_russia_enable", "1")  && !strcmp(wan_proto, "pptp")) ||
                        ( !nvram_match("wan_pppoe_russia_enable", "1") && !strcmp(wan_proto, "pppoe")) ||
                        ( !nvram_match("wan_l2tp_russia_enable", "1") && !strcmp(wan_proto, "l2tp"))
                )
                {
#ifdef CONFIG_USER_DLINK_ROUTER_URL
			_system("dnsmasq -o -i %s -P 4096 %s -A /router.dlink.com/%s &", lan_bridge, open_dns_opt, lan_ipaddr);
#else
                        _system("dnsmasq -o -i %s -P 4096 %s &", lan_bridge, open_dns_opt);
#endif
                }
                else
                {
#ifdef CONFIG_USER_DLINK_ROUTER_URL
			_system("dnsmasq -i %s -P 4096 %s -A /router.dlink.com/%s %s &", lan_bridge, open_dns_opt, lan_ipaddr, w4r_opt);
#else
			_system("dnsmasq -i %s -P 4096 %s %s &", lan_bridge, open_dns_opt, w4r_opt);
#endif
                }
#else //#ifdef RPPPOE /* jimmy modified 20080611 */
#ifdef CONFIG_USER_DLINK_ROUTER_URL
		  _system("dnsmasq -i %s -P 4096 %s -A /router.dlink.com/%s %s &", lan_bridge, open_dns_opt, lan_ipaddr, w4r_opt);
#else
		_system("dnsmasq -i %s -P 4096 %s %s &", lan_bridge, open_dns_opt, w4r_opt);
#endif
#endif //#ifdef RPPPOE /* jimmy modified 20080611 */
                }

        return 0;
}

int stop_dnsmasq(int flag)
{
        DEBUG_MSG("Stop dnsmasq\n");

        if( !nvram_match("dns_relay", "0") || flag )
                KILL_APP_ASYNCH("dnsmasq");
        return 0;
}

int start_tftpd(void)
{
        _system("tftpd &");
        return 0;
}


int stop_tftpd(int flag)
{
        DEBUG_MSG("Stop tftp\n");

        /* tftpd, for smac only, no upgrade function */
        if (action_flags.wan==1 && action_flags.app == 1 && action_flags.firewall==1)
         return 0;

        KILL_APP_SYNCH("tftpd");
        return 0;
}

#ifdef CONFIG_USER_DROPBEAR
int start_ssh_server(void)
{
        char *username, *password;
        username = nvram_safe_get("admin_username");
        password = nvram_safe_get("admin_password");

#if 1
    /* /etc/passwd should have Admin entry with root permission */
        if (strcmp("Admin", username) )
                username = "Admin";
        if (strlen(password) == 0) /* empty password */
                password = "password";
#endif

        if(nvram_readfile("/tmp/dropbear_key")) {
                _system("dropbear -r /tmp/dropbear_key -U %s -u %s",
                        username,
                        password);
                return 0;
        } else {
                _system("nvram initfile"); // call nvram_initfile() directly will fail

                _system("dropbearkey -t rsa -f /tmp/dropbear_key");
                _system("dropbear -r /tmp/dropbear_key -U %s -u %s",
                        username,
                        password);
                _system("nvram writefile /tmp/dropbear_key"); // call nvram_writefile() directly will fail

                return 0;
        }
}


int stop_ssh_server(int flag)
{
        KILL_APP_ASYNCH("dropbear");
        return 0;
}
#endif

#ifdef CONFIG_IPV6_LLMNR
int start_llmnr_server(void)
{
    char *lan_device_name = nvram_safe_get("lan_device_name");
    if (lan_device_name == NULL)
        lan_device_name = nvram_safe_get("hostname");
    if (lan_device_name == NULL)
        lan_device_name = "llmnr-host";
#if 0
    char *lan_eth;
    lan_eth = nvram_safe_get("lan_eth");
    if (lan_eth != NULL)
        _system("llmnr %s %s &", lan_eth, lan_device_name);
#else
    char *wlan_enable;
    wlan_enable = nvram_safe_get("wlan0_enable");

    // Jery Lin modify 2009.12.02
    // Fixed that llmnr select fault interface when disable wireless
    _system("llmnr %s %s &", nvram_safe_get("lan_bridge"), lan_device_name);
#endif
        return 0;
}

int stop_llmnr_server(int flag)
{
        KILL_APP_ASYNCH("llmnr");
        return 0;
}
#endif

int start_ntp(void)
{
        char wan_eth[10]={0};
        FILE *fp;
        int pid;
        char *ntp_client_enable=NULL;
        char *time_daylight_saving_enable=NULL;

        if((fp = fopen("/var/etc/ntp.conf", "w+")) != NULL)
        {
                ntp_client_enable = nvram_safe_get("ntp_client_enable");
                time_daylight_saving_enable = nvram_safe_get("time_daylight_saving_enable");

                fprintf(fp, "ntp_client_enable=%s\n", ntp_client_enable);

                if( !strcmp(ntp_client_enable, "1")  )
                {
                        if( !nvram_match("wan_proto", "dhcpc") || !nvram_match("wan_proto", "static") )
                                strcpy( wan_eth, nvram_safe_get("wan_eth") );
                        else
                                strcpy( wan_eth, "ppp0" );

                        fprintf(fp, "time_zone=%s\n", nvram_safe_get("time_zone"));
                        fprintf(fp, "wan_eth=%s\n", wan_eth);
                        fprintf(fp, "ntp_server=%s\n", nvram_safe_get("ntp_server"));
                        fprintf(fp, "ntp_sync_interval=%d\n", atoi(nvram_safe_get("ntp_sync_interval"))*60*60);
                }
                else
                        fprintf(fp, "system_time=%s\n", nvram_safe_get("system_time"));

                fprintf(fp, "time_daylight_saving_enable=%s\n", time_daylight_saving_enable);
                if( !strcmp(time_daylight_saving_enable, "1")  )
                {
                        fprintf(fp, "start_month=%s\n", nvram_safe_get("time_daylight_saving_start_month"));
                        fprintf(fp, "start_time=%s\n", nvram_safe_get("time_daylight_saving_start_time"));
                        fprintf(fp, "start_day_of_week=%s\n", nvram_safe_get("time_daylight_saving_start_day_of_week"));
                        fprintf(fp, "start_week=%s\n", nvram_safe_get("time_daylight_saving_start_week"));
                        fprintf(fp, "end_month=%s\n", nvram_safe_get("time_daylight_saving_end_month"));
                        fprintf(fp, "end_time=%s\n", nvram_safe_get("time_daylight_saving_end_time"));
                        fprintf(fp, "end_day_of_week=%s\n", nvram_safe_get("time_daylight_saving_end_day_of_week"));
                        fprintf(fp, "end_week=%s\n", nvram_safe_get("time_daylight_saving_end_week"));
                }

                fclose(fp);

                pid = read_pid(TIMER_PID);
                if((pid > 0) && !kill(pid, 0)) {
                    /* send different signal to timer for Ubicom */
                    FILE *sig_fp;
                    if ((sig_fp = fopen(SIG_TMP_FILE, "w")) != NULL) {
                        fputs("ntp", sig_fp);
                    }
                    fclose(sig_fp);
                    kill(pid, SIGHUP);
                }
        }

        return 0;
}

int stop_ntp(int flag)
{
        return 0;
}

#ifdef UPNP_ATH_LIBUPNP
/* jimmy added 20080215 for test avoid multilpe call upnpd
        2 setting still not integrate, left as hardcode in source code
        advertisements period = 30, in linuxigd-1.0/main.c line 256
        advertisement_ttl = 4, in upnp/src/ssdp/ssdp_device.c, line 219
*/
#define UPNP_XML_FILE "/var/etc/gatedesc.xml"

void mod_upnpd_xml_desc(void){
        FILE *fp;
        int wps_enable=0;
/*  Date: 2009-3-31
*   Name: Ken Chiang
*   Reason: Modify for nvram get string length is over buffer issue.
*   Notice :
*/
/*
        char friendlyName[20];
        char manufacturer[20];
        char modelName[30];
        char modelURL[30];
        char manufacturerURL[40];
        char modelNumber[20];
        char serialNumber[20];
        char lan_ip[20];
*/
        char friendlyName[20]={0};
        char modelName[32]={0};
        char manufacturer[40]={0};
        char modelURL[52]={0};
        char manufacturerURL[52]={0};
        char modelNumber[20]={0};
        char serialNumber[20]={0};
        char lan_ip[20]={0};

        /* the directory /var/etc is created by other application */
        system("cp -raf /etc/linuxigd/* /var/etc/");

        memset(friendlyName,'0',sizeof(friendlyName));
        memset(manufacturer,'0',sizeof(manufacturer));
        memset(modelName,'0',sizeof(modelName));
        memset(modelURL,'0',sizeof(modelURL));
        memset(manufacturerURL,'0',sizeof(manufacturerURL));
        memset(modelNumber,'0',sizeof(modelNumber));
        memset(serialNumber,'0',sizeof(serialNumber));
        memset(lan_ip,'0',sizeof(lan_ip));

        wps_enable=atoi(nvram_safe_get("wps_enable"));
/*  Date: 2009-3-31
*   Name: Ken Chiang
*   Reason: Modify for nvram get string length is over buffer issue.
*   Notice :
*/
/*
        strncpy(friendlyName,nvram_safe_get("friendlyname"),sizeof(friendlyName));
        strncpy(manufacturer,nvram_safe_get("manufacturer"),sizeof(manufacturer));
        strncpy(modelName,nvram_safe_get("model_name"),sizeof(modelName));
        strncpy(modelURL,nvram_safe_get("model_url"),sizeof(modelURL));
        strncpy(manufacturerURL,nvram_safe_get("model_url"),sizeof(manufacturerURL));
        strncpy(modelNumber,nvram_safe_get("model_number"),sizeof(modelNumber));
        strncpy(serialNumber,nvram_safe_get("serial_number"),sizeof(serialNumber));
        strncpy(lan_ip,nvram_safe_get("lan_ipaddr"),sizeof(lan_ip));
*/
        strncpy(friendlyName,nvram_safe_get("friendlyname"),sizeof(friendlyName)-1);
        strncpy(manufacturer,nvram_safe_get("manufacturer"),sizeof(manufacturer)-1);
        strncpy(modelName,nvram_safe_get("model_name"),sizeof(modelName)-1);
        strncpy(modelURL,nvram_safe_get("model_url"),sizeof(modelURL)-1);
        strncpy(manufacturerURL,nvram_safe_get("model_url"),sizeof(manufacturerURL)-1);
        strncpy(modelNumber,nvram_safe_get("model_number"),sizeof(modelNumber)-1);
        strncpy(serialNumber,nvram_safe_get("serial_number"),sizeof(serialNumber)-1);
        strncpy(lan_ip,nvram_safe_get("lan_ipaddr"),sizeof(lan_ip)-1);

        unlink(UPNP_XML_FILE);
        if((fp=fopen(UPNP_XML_FILE,"w"))!=NULL){
                fprintf(fp,"<?xml version=\"1.0\"?>\n");
                fprintf(fp,"<root xmlns=\"urn:schemas-upnp-org:device-1-0\">\n");
                fprintf(fp,"    <specVersion>\n");
                fprintf(fp,"            <major>1</major>\n");
                fprintf(fp,"            <minor>0</minor>\n");
                fprintf(fp,"    </specVersion>\n");
                fprintf(fp,"    <device>\n");
                fprintf(fp,"            <deviceType>urn:schemas-upnp-org:device:InternetGatewayDevice:1</deviceType>\n");
                fprintf(fp,"            <presentationURL>http://%s:80</presentationURL>\n",lan_ip);
                fprintf(fp,"            <friendlyName>%s</friendlyName>\n",friendlyName);
                fprintf(fp,"            <manufacturer>%s</manufacturer>\n",manufacturer);
                fprintf(fp,"            <manufacturerURL>%s</manufacturerURL>\n",manufacturerURL);
                fprintf(fp,"            <modelDescription>D-Link Router</modelDescription>\n");
                fprintf(fp,"            <modelName>%s</modelName>\n",modelName);
                fprintf(fp,"            <modelNumber>%s</modelNumber>\n",modelNumber);
                fprintf(fp,"            <modelURL>%s</modelURL>\n",modelURL);
                fprintf(fp,"            <serialNumber>%s</serialNumber>\n",serialNumber);
                fprintf(fp,"            <UDN>uuid:11111111-1111-1111-1111-111111111111</UDN>\n");
                fprintf(fp,"            <serviceList>\n");
                fprintf(fp,"                    <service>\n");
                fprintf(fp,"                            <serviceType>urn:schemas-dummy-com:service:Dummy:1</serviceType>\n");
                fprintf(fp,"                            <serviceId>urn:dummy-com:serviceId:dummy1</serviceId>\n");
                fprintf(fp,"                            <controlURL>/dummy</controlURL>\n");
                fprintf(fp,"                            <eventSubURL>/dummy</eventSubURL>\n");
                fprintf(fp,"                            <SCPDURL>/dummy.xml</SCPDURL>\n");
                fprintf(fp,"                    </service>\n");
                fprintf(fp,"            </serviceList>\n");
                fprintf(fp,"            <deviceList>\n");
                fprintf(fp,"                    <device>\n");
                fprintf(fp,"                            <deviceType>urn:schemas-upnp-org:device:WANDevice:1</deviceType>\n");
                fprintf(fp,"                            <friendlyName>WANDevice</friendlyName>\n");
                fprintf(fp,"                            <manufacturer>%s</manufacturer>\n",manufacturer);
                fprintf(fp,"                            <manufacturerURL>%s</manufacturerURL>\n",manufacturerURL);
                fprintf(fp,"                            <modelDescription>Internet Access Router</modelDescription>\n");
                fprintf(fp,"                            <modelName>WAN Device</modelName>\n");
                fprintf(fp,"                            <modelNumber>%s</modelNumber>\n",modelNumber);
                fprintf(fp,"                            <modelURL>%s</modelURL>\n",modelURL);
                fprintf(fp,"                            <serialNumber>%s</serialNumber>\n",serialNumber);
                fprintf(fp,"                            <UDN>uuid:22222222-2222-2222-2222-222222222222</UDN>\n");
                fprintf(fp,"                            <UPC>0001</UPC>\n");
                fprintf(fp,"                            <serviceList>\n");
                fprintf(fp,"                                    <service>\n");
                fprintf(fp,"                                            <serviceType>urn:schemas-upnp-org:service:WANCommonInterfaceConfig:1</serviceType>\n");
                fprintf(fp,"                                            <serviceId>urn:upnp-org:serviceId:WANCommonIFC1</serviceId>\n");
                fprintf(fp,"                                            <controlURL>/upnp/control/WANCommonIFC1</controlURL>\n");
                fprintf(fp,"                                            <eventSubURL>/upnp/control/WANCommonIFC1</eventSubURL>\n");
                fprintf(fp,"                                            <SCPDURL>/gateicfgSCPD.xml</SCPDURL>\n");
                fprintf(fp,"                                    </service>\n");
                fprintf(fp,"                            </serviceList>\n");
                fprintf(fp,"                            <deviceList>\n");
                fprintf(fp,"                                    <device>\n");
                fprintf(fp,"                                            <deviceType>urn:schemas-upnp-org:device:WANConnectionDevice:1</deviceType>\n");
                fprintf(fp,"                                            <friendlyName>WANConnectionDevice</friendlyName>\n");
                fprintf(fp,"                                            <manufacturer>%s</manufacturer>\n",manufacturer);
                fprintf(fp,"                                            <manufacturerURL>%s</manufacturerURL>\n",manufacturerURL);
                fprintf(fp,"                                            <modelDescription>WANConnectionDevice</modelDescription>\n");
                fprintf(fp,"                                            <modelName>WANConnectionDevice</modelName>\n");
                fprintf(fp,"                                            <modelNumber>%s</modelNumber>\n",modelNumber);
                fprintf(fp,"                                            <modelURL>%s</modelURL>\n",modelURL);
                fprintf(fp,"                                            <serialNumber>%s</serialNumber>\n",serialNumber);
                fprintf(fp,"                                            <UDN>uuid:33333333-3333-3333-3333-333333333333</UDN>\n");
                fprintf(fp,"                                            <UPC>0001</UPC>\n");
                fprintf(fp,"                                            <serviceList>\n");
                fprintf(fp,"                                                    <service>\n");
                fprintf(fp,"                                                            <serviceType>urn:schemas-upnp-org:service:WANIPConnection:1</serviceType>\n");
                fprintf(fp,"                                                            <serviceId>urn:upnp-org:serviceId:WANIPConn1</serviceId>\n");
                fprintf(fp,"                                                            <controlURL>/upnp/control/WANIPConn1</controlURL>\n");
                fprintf(fp,"                                                            <eventSubURL>/upnp/control/WANIPConn1</eventSubURL>\n");
                fprintf(fp,"                                                            <SCPDURL>/gateconnSCPD.xml</SCPDURL>\n");
                fprintf(fp,"                                                    </service>\n");
                fprintf(fp,"                                            </serviceList>\n");
                fprintf(fp,"                                    </device>\n");
                fprintf(fp,"                            </deviceList>\n");
                fprintf(fp,"                    </device>\n");
        /*                      WFA Description                 */
                if(wps_enable == 1){
                        fprintf(fp,"                    <device>\n");
                        fprintf(fp,"                            <deviceType>urn:schemas-wifialliance-org:device:WFADevice:1</deviceType>\n");
                        fprintf(fp,"                            <presentationURL>http://%s:80</presentationURL>\n",lan_ip);
                        fprintf(fp,"                            <friendlyName>WFADevice</friendlyName>\n");
                        fprintf(fp,"                            <manufacturer>%s</manufacturer>\n",manufacturer);
                        fprintf(fp,"                            <manufacturerURL>%s</manufacturerURL>\n",manufacturerURL);
                        fprintf(fp,"                            <modelDescription>WFADevice</modelDescription>\n");
                        fprintf(fp,"                            <modelName>WFADevice</modelName>\n");
                        fprintf(fp,"                            <modelURL>%s</modelURL>\n",modelURL);
                        fprintf(fp,"                            <modelNumber>%s</modelNumber>\n",modelNumber);
                        fprintf(fp,"                            <serialNumber>%s</serialNumber>\n",serialNumber);
                        fprintf(fp,"                            <UDN>uuid:565aa949-67c1-4c0e-aa8f-f349e6f59311</UDN>\n");
                        fprintf(fp,"                            <serviceList>\n");
                        fprintf(fp,"                                    <service>\n");
                        fprintf(fp,"                                            <serviceType>urn:schemas-wifialliance-org:service:WFAWLANConfig:1</serviceType>\n");
                        fprintf(fp,"                                            <serviceId>urn:wifialliance-org:serviceId:WFAWLANConfig1</serviceId>\n");
                        fprintf(fp,"                                            <controlURL>http://%s:60001/WFAWLANConfig/control</controlURL>\n",lan_ip);
                        fprintf(fp,"                                            <eventSubURL>http://%s:60001/WFAWLANConfig/event</eventSubURL>\n",lan_ip);
                        fprintf(fp,"                                            <SCPDURL>http://%s:60001/WFAWLANConfig/scpd.xml</SCPDURL>\n",lan_ip);
                        fprintf(fp,"                                    </service>\n");
                        fprintf(fp,"                            </serviceList>\n");
                        fprintf(fp,"                    </device>\n");
                }
        /*---------------------- WFA end ------------------*/
                fprintf(fp,"            </deviceList>\n");
                fprintf(fp,"    </device>\n");
                fprintf(fp,"</root>\n");
                fclose(fp);
        }else{
                printf("Can't open %s with write permission\n",UPNP_XML_FILE);
        }
}
#endif

static int check_mem()
{
	char mfree[32];
	int memfree = 0, ret = 0;

	if (!_GetMemInfo("MemFree:", mfree))
		goto out;
	if ((memfree = strtol(mfree, NULL, 10)) < 7) {
		printf("Memory free : %d \n", memfree);
		goto out;
	}
	ret = 1;
out:
	return ret;
}

/* -------------------------------------------------------- */
#ifdef CONFIG_USER_IP_LOOKUP
int start_arpping(void)
{
/*  Date: 2010/10/08
 *  Name: Robert Chen
 *  Reason: We need to add check free memory becasue arpping demand need 6MB space.
 */ 
     if (!check_mem()) {
	printf("Memory is not enough to exec arpping command!\n");
	goto out;
     }
     _system("arpping %s %s %s %s %s &",
                nvram_safe_get("lan_ipaddr"), nvram_safe_get("dhcpd_start"),
                nvram_safe_get("dhcpd_end"), nvram_safe_get("lan_bridge"),
                nvram_safe_get("lan_eth"));
out:
     return 0;
}

int stop_arpping(int flag)
{
        KILL_APP_SYNCH("arpping");
        return 0;
}
#endif

/*      Date:   2009-04-16
*       Name:   jimmy huang
*       Reason: Add function to convert netmask str to cidr integer
*       Note:   Used for miniupnpd-1.3
*/
int netmask_str_2_cidr(char *netmask_ip_str){
        int i = 0, cidr = 0;
        char *p = NULL;
        char c;
        int int_ip_a = 0,int_ip_b = 0,int_ip_c = 0,int_ip_d = 0;
        unsigned int new_mask;

        if(!netmask_ip_str)
                return 0;

        for(p = netmask_ip_str ; (c = *p) ; p++ ){
                if(c == '.')
                        cidr ++;
        }

        if(cidr != 3){
                return 0;
        }
        cidr = 0;
        sscanf(netmask_ip_str,"%d.%d.%d.%d",&int_ip_a,&int_ip_b,&int_ip_c,&int_ip_d);
        ((unsigned char *)(&new_mask))[0]=int_ip_a;
        ((unsigned char *)(&new_mask))[1]=int_ip_b;
        ((unsigned char *)(&new_mask))[2]=int_ip_c;
        ((unsigned char *)(&new_mask))[3]=int_ip_d;

        for(i=0 ; i < 32 ; i++){
                cidr += (new_mask >> i) & 0x1;
        }

        return cidr;
}

int start_upnp(void)
{
        char *lan_bridge=nvram_safe_get("lan_bridge");

//#ifndef UPNP_ATH_MINIUPNPD_RC
#if !defined(UPNP_ATH_MINIUPNPD_RC) && !defined(UPNP_ATH_MINIUPNPD_1_3)

#ifdef UPNP_ATH_LIBUPNP
/* jimmy modified 20080214 , replace miniupnpd with upnpd */

        if( !nvram_match("upnp_enable", "1")  ) {
                int pid;
                printf("lib upnp init\n");
                mod_upnpd_xml_desc();
                route_add( lan_bridge, "239.0.0.0", "255.0.0.0", "0.0.0.0", 0);
                //_system("route add -net 239.0.0.0 netmask 255.0.0.0 %s", nvram_get("lan_bridge") );
/*  Date: 2009-4-02
*   Name: Ken Chiang
*   Reason: lib upnpd will acquire too many cpu time then cause booting time too long
                        so start lib upnpd by timer to avoid upnpd delay other app process , root cause still not found
*   Notice :
*/
/*
                _system("upnpd %s %s &",nvram_safe_get("wan_eth"), lan_bridge);
*/
                //start timer to call upnpd
                pid = read_pid(TIMER_PID);
                if((pid > 0) && !kill(pid, 0))
                        printf("lib upnp timer start\n");
                        kill(pid, SIGILL); /*start_tracegw_timer*/
                }
                else{
                        printf("lib upnp can't timer start\n");
                }
#endif

#else // !defined(UPNP_ATH_MINIUPNPD_RC) && !defined(UPNP_ATH_MINIUPNPD_1_3)

#ifdef MPPPOE
        char mpppoe_main_session[8] = {};
        char *wan_eth;
        sprintf(mpppoe_main_session,"ppp%d",atoi(nvram_safe_get("wan_pppoe_main_session")));
        if(nvram_match("wan_proto","pppoe") == 0)
                wan_eth = mpppoe_main_session;//only support main session
        else if(nvram_match("wan_proto","unnumbered") == 0)
                wan_eth = "ppp0";
        else
                wan_eth = nvram_safe_get("wan_eth");
#else
        char wan_eth[10]={0};	// fix *wan_eth and wan_eth[10]={0}; conflict
#endif
        char buf[100];
        FILE *fp=NULL;
#ifdef CONFIG_BRIDGE_MODE
        char *lan_ip=NULL;
#endif

        /* UPNP */
					if (!nvram_match("upnp_enable", "1") ) {
                //_system("route add -net 239.0.0.0 netmask 255.0.0.0 %s", nvram_get("lan_bridge") );
                route_add( lan_bridge, "239.0.0.0", "255.0.0.0", "0.0.0.0", 0);
                //save nvram configuration into miniupnpd.conf
                fp = fopen(UPNP_CONF_FILE,"w");
                if(fp)
                {
                        sprintf( buf,"wps_enable=%s\n", nvram_safe_get("wps_enable") );
                        fwrite(buf,1,strlen(buf),fp);
                        sprintf( buf,"friendly_name=%s\n", nvram_safe_get("friendlyname") );
                        fwrite(buf,1,strlen(buf),fp);
                        sprintf( buf,"manufacturer_name=%s\n", nvram_safe_get("manufacturer") );
                        fwrite(buf,1,strlen(buf),fp);
                        sprintf( buf,"model_name=%s\n", nvram_safe_get("model_name") );
                        fwrite(buf,1,strlen(buf),fp);
                        sprintf( buf,"manufacturer_url=%s\n", nvram_safe_get("manufacturer_url") );
                        fwrite(buf,1,strlen(buf),fp);
                        sprintf( buf,"model_url=%s\n", nvram_safe_get("model_url") );
                        fwrite(buf,1,strlen(buf),fp);
                        sprintf( buf,"model_number=%s\n", nvram_safe_get("model_number") );
                        fwrite(buf,1,strlen(buf),fp);
                        sprintf( buf,"serial=%s\n", nvram_safe_get("serial_number") );
                        fwrite(buf,1,strlen(buf),fp);

#ifdef CONFIG_BRIDGE_MODE
                if(nvram_match("wlan0_mode", "ap") == 0)
                {
                                sprintf( buf,"ext_ifname=%s\n", lan_bridge );
                                fwrite(buf,1,strlen(buf),fp);

                                lan_ip = get_ipaddr(lan_bridge);
#ifndef UPNP_ATH_MINIUPNPD_1_3
                                if(lan_ip == NULL)
                                        sprintf( buf,"listening_ip=%s\n", nvram_safe_get("lan_ipaddr") );
                        else
                                        sprintf( buf,"listening_ip=%s\n", lan_ip );
#else
                                // UPNP_ATH_MINIUPNPD_1.3 used
                        if(lan_ip == NULL){
                                sprintf( buf,"listening_ip=%s/%d\n", nvram_safe_get("lan_ipaddr"),netmask_str_2_cidr(nvram_safe_get("lan_netmask")) );
                        }else{
                                sprintf( buf,"listening_ip=%s/%d\n", lan_ip ,netmask_str_2_cidr(nvram_safe_get("lan_netmask")));
                        }
#endif

                                fwrite(buf,1,strlen(buf),fp);
                }
                else
#endif // CONFIG_BRIDGE_MODE
                {
#ifndef UPNP_ATH_MINIUPNPD_1_3
                        sprintf( buf,"listening_ip=%s\n", nvram_safe_get("lan_ipaddr") );
                        fwrite(buf,1,strlen(buf),fp);
#endif
                        if( !nvram_match("wan_proto", "dhcpc") || !nvram_match("wan_proto", "static") )
                                strcpy( wan_eth, nvram_safe_get("wan_eth") );
                        else
                                strcpy( wan_eth, "ppp0" );
#ifndef UPNP_ATH_MINIUPNPD_1_3
                        if(get_ipaddr(wan_eth))
                                        sprintf( buf,"ext_ifname=%s\n", wan_eth );
                        else
                                sprintf( buf,"ext_ip=0.0.0.0\n" );

                                fwrite(buf,1,strlen(buf),fp);
#endif
                        }

#ifdef UPNP_ATH_MINIUPNPD_1_3
                        fprintf(fp,"listening_ip=%s/%d\n",  nvram_safe_get("lan_ipaddr") ,netmask_str_2_cidr(nvram_safe_get("lan_netmask")));
                        fprintf(fp,"ext_ifname=%s\n", wan_eth);
                        fprintf(fp,"int_ifname=%s\n", lan_bridge);
                        fprintf(fp,"port=65530\n"); // miniupnpd's listening port
                        fprintf(fp,"enable_upnp=yes\n");
                        fprintf(fp,"bitrate_up=1000000\n");
                        fprintf(fp,"bitrate_down=10000000\n");
                        fprintf(fp,"secure_mode=no\n");
                        fprintf(fp,"system_uptime=yes\n");
                        fprintf(fp,"notify_interval=30\n");
                        /*      Date:   2009-06-09
                        *       Name:   jimmy huang
                        *       Reason: Modified clean_ruleset_interval to 0
                        *                       which means disable this function
                        */
                        fprintf(fp,"clean_ruleset_interval=0\n");
#endif
                        fclose(fp);
                }
                _system("miniupnpd &");
        }
#endif // !defined(UPNP_ATH_MINIUPNPD_RC) && !defined(UPNP_ATH_MINIUPNPD_1_3)
/* ----------------------------------- */
        return 0;
};

int stop_upnp(int flag)
{
        char *lan_bridge=nvram_safe_get("lan_bridge");

//#ifndef UPNP_ATH_MINIUPNPD_RC
#if !defined(UPNP_ATH_MINIUPNPD_RC) && !defined(UPNP_ATH_MINIUPNPD_1_3)
        /* jimmy added 20080918, add libupnp-1.6.3 flag */
#ifdef UPNP_ATH_LIBUPNP
/*
        jimmy modified 20080214 , replace miniupnpd with upnpd
        Only when lan setting changes, upnpd need to restart
*/

        /* when receiving SIGIPE from dhcpc, no need to stop upnpd
                it's due to dhcpd receive ACK from dhcpd server on WAN
        */
        if( (action_flags.lan_app == 1) && (!nvram_match("upnp_enable", "1")) ){
                return 0;
        }

        /* we need to stop upnpd when
                turn off upnpd service,
                turn on/off wps service, then start upnpd
                rebooting       */
        printf("lib upnp stop\n");
        _system("route del -net 239.0.0.0 netmask 255.0.0.0 %s", lan_bridge);
        KILL_APP_ASYNCH("upnpd");
        unlink(UPNP_XML_FILE);
#endif

#else
         if( !nvram_match("upnp_enable", "0")  || flag)
         {
                /* UPNP */
        _system("route del -net 239.0.0.0 netmask 255.0.0.0 %s", lan_bridge);
                KILL_APP_ASYNCH("miniupnpd");
                unlink("/var/etc/miniupnpd.conf");
        }
#endif
/* ----------------------------------- */
        return 0;
}


void modify_igmpproxy_conf(unsigned int *russia_mode)
{
        char wan_eth[5]="";
        char *enable, *name, *dest_addr, *dest_mask, *gateway, *ifname, *metric;
        char static_route[]="static_routing_XX", route_data[128];
        int i=0;
        char *wan_proto=NULL;

        wan_proto = nvram_safe_get("wan_proto");

        if( !strcmp(wan_proto, "dhcpc")
        || !strcmp(wan_proto, "static")
#ifdef RPPPOE
        || ( !nvram_match("wan_pptp_russia_enable", "1")  && !strcmp(wan_proto, "pptp") )
        || ( !nvram_match("wan_pppoe_russia_enable", "1") && !strcmp(wan_proto, "pppoe") )
        || ( !nvram_match("wan_l2tp_russia_enable", "1") && !strcmp(wan_proto, "l2tp") )
#endif
        ){
                strcpy(wan_eth, nvram_safe_get("wan_eth") );
                *russia_mode = 1;

        }
        else{
                strcpy(wan_eth, "ppp0");
                *russia_mode = 0;
        }
        init_file(IGMPPROXY);

#if 0 //#ifdef RPPPOE
        if(((!nvram_match("wan_pptp_russia_enable", "1")) && ( !nvram_match("wan_proto", "pptp"))) ||
                ((!nvram_match("wan_pppoe_russia_enable", "1")) && ( !nvram_match("wan_proto", "pppoe"))) )
        {
                save2file(IGMPPROXY,"quickleave\n"
                                "phyint eth0 physical upstream  ratelimit 0\n"
                                "phyint %s virtual upstream  ratelimit 0 \n"
                                "phyint %s downstream  ratelimit 0 \n",
                                wan_eth, nvram_safe_get("lan_bridge"));



                /* static route */
                for(i=0; i<MAX_STATIC_ROUTING_NUMBER; i++)
                {
                        snprintf(static_route, sizeof(static_route), "static_routing_%02d",i) ;
                        if ( nvram_match(static_route, "") == 0 )
                                continue;
                        else
                                strcpy( route_data, nvram_safe_get(static_route));

                        enable = strtok(route_data, "/");
                        name = strtok(NULL, "/");
                        dest_addr = strtok(NULL, "/");
                        dest_mask = strtok(NULL, "/");
                        gateway = strtok(NULL, "/");
                        ifname = strtok(NULL, "/");
                        metric = strtok(NULL, "/");

                        if(( strcmp(enable, "1") == 0 ) && ( strcmp(ifname, "WAN_PHY") == 0 ) )
                                save2file(IGMPPROXY, "route addr %s mask %s \n", dest_addr, dest_mask);
                }

        }
        else
#endif
        {
                save2file(IGMPPROXY,"quickleave\n"
                                "phyint %s upstream  ratelimit 0\n"
                                "phyint %s downstream  ratelimit 0\n ",
                                wan_eth, nvram_safe_get("lan_bridge"));
        }
}


// add 2009.12.11
#include <net/if.h>
#include <sys/socket.h>
#include <linux/sockios.h>

void set_eth0_igmp(int flags)
{
        int sockfd;
        struct ifreq ifreq_ioctl;

        sockfd=socket(AF_INET, SOCK_DGRAM, 0);

        if( !sockfd) {
                printf("no memory or socket fail!!!\n");
                return;
        }

        strcpy(ifreq_ioctl.ifr_name,  "eth0");

        ifreq_ioctl.ifr_ifru.ifru_flags= flags;

        ioctl(sockfd, SIOCDEVPRIVATE, &ifreq_ioctl);

        close(sockfd);
}


int start_mcast(void)
{
        int wan_mode=0;
	int multicast_flag=0;
        /*multicast stream*/
/*
*       Date: 2009-7-20
*       Name: Albert_CHen
*       Reason: modify for support discard igmp packet to tunnel site
*       Notice :
*/

        if( !nvram_match("multicast_stream_enable", "1")  ){
                modify_igmpproxy_conf(&wan_mode);
                _system("igmpproxy %s &", wan_mode? "-r":"");

		multicast_flag=1;

        }

        /* support ipv6 multicast */
        if( !nvram_match("ipv6_multicast_stream_enable", "1")  ) {
#if 0
                system("mldc br0 eth0.1 &");
#else
		if( !checkServiceAlivebyName("mldc") ) {
                system("mldc -z &");
		}
#endif
		multicast_flag=1;
	}

                // add 2009.12.11
        if( multicast_flag == 1)
                set_eth0_igmp(1);

        return 0;
}

int stop_mcast(int flag)
{
        DEBUG_MSG("Stop igmp\n");

        if( !nvram_match("multicast_stream_enable", "0")  || flag )
                KILL_APP_ASYNCH("igmpproxy");

        /* support ipv6 multicast */
#if 0
        if( !nvram_match("ipv6_multicast_stream_enable", "0")  || flag )
                system("killall mldc");
#else
                system("killall mldc");
#endif
        // add 2009.12.11
        set_eth0_igmp(0);

        return 0;
}


int start_psmon(void)
{
        _system("/var/sbin/psmon 5");
        return 0;
}

int stop_psmon(int flag)
{
        return 0;
}

#ifdef DIR652V
int start_dhcp_relay_652V(void)
{
	if (nvram_match("dhcpd_enable", "1" ) == 0)
		return -1;
	system("cli services dhcprelay start");
	system("cli sys vif up");

	return 0;
}

int stop_dhcp_relay_652V(void)
{
	system("cli services dhcprelay stop");
	system("cli sys vif down");

	return 0;
}
#endif

int start_dhcp_relay(void)
{
        FILE *fp;

        char *p, dhcpd_leases[128], buf[1024];
        char *lan_if, *wan_if;

        lan_if = "br0";
        wan_if = "eth1";

        if ((fp = fopen(DHCPRELAY_CONFIG, "w")) == NULL)
        {
                printf("open dhcp_relay conf file error");
                return -1;
        }

        fprintf(fp,"logfile %s\n",DHCPRELAY_CONFIG);
        fprintf(fp,"loglevel 1\n");
        fprintf(fp,"pidfile %s\n",DHCPRELAY_PID);
                                                        /* IFNAME  clients servers bcast */
        fprintf(fp,"if    %s    true    false    true\n", lan_if);//br0
        fprintf(fp,"if    %s    false    true    true\n",wan_if);//eth1

        /* IFNAME  agent-idname */
        fprintf(fp,"name    %s    DIR-825HS_B1\n",lan_if);//br0

        /*   TYPE    address */
        //fprintf(fp,"server  ip  192.168.8.66\n");
        fprintf(fp,"server bcast   %s\n",wan_if);//eth1
        fclose(fp);

        //_system("%s %s %s %s %s %s %s %s %s %s %s %s %s","iptables", "-A", "INPUT", "-i", "eth1", "-p", "udp","-m", "mport", "--dports", "53,67:68", "-j", "ACCEPT");
        _system("%s %s %s","dhcp-fwd", "-c", DHCPRELAY_CONFIG);
        return 0;

}
int start_ipsec(void)
{
	stop_ipsec(0);
#ifdef CONFIG_USER_OPENSWAN
	system("cli security vpn ipsec start");
	system("cli security vpn pptpd start");
	system("cli security vpn l2tpd start");
	/* XXX: avoid async issue. fix it in future*/
#endif //CONFIG_USER_OPENSWAN
	return 0;
}
int stop_ipsec(int flag)
{
#ifdef CONFIG_USER_OPENSWAN
	system("cli security vpn ipsec stop");
	system("cli security vpn pptpd stop");
	system("cli security vpn l2tpd stop");
	/* XXX: avoid async issue. fix it in future*/
	sleep(5);
#endif //CONFIG_USER_OPENSWAN
	return 0;
}
int stop_dhcp_relay(int flag)
{
        FILE *fp_dhcp_relay;

        if( !nvram_match("dhcpd_enable", "2")  || flag )
        {
                fp_dhcp_relay = fopen(DHCPRELAY_PID, "r");
                if(fp_dhcp_relay)
                {
                        KILL_APP_SYNCH("dhcp-fwd");
                        sleep(1);
                        fclose(fp_dhcp_relay);
                }
        }

        return 0;
}

/*
*       Date: 2009-05-26
*       Name: Jimmy Huang
*       Reason: for lcm collecting info used
*       Notice : package "infod" is within www/model_name/project_apps
*/
#ifdef LCM_FEATURE_INFOD
int start_infod(void){
        system("infod &");
        return 1;
}
int stop_infod(void){
        KILL_APP_ASYNCH("infod");
        return 1;
}
#endif

int start_dhcpd(void)
{
        char *enable, *name, *ip, *mac;
        char reserved_rule[]="dhcpd_reserve_XX", reserved_data[128];
        int i=0;

        char *tok=NULL;
        char wins_server[80]={0};
        char node_type[2]={0};
        char scope[32]={0};
        char *lan_ipaddr = NULL;

#ifdef  CONFIG_BRIDGE_MODE
#ifndef AP_NOWAN_HASBRIDGE
                if(nvram_match("wlan0_mode", "ap") == 0)
                        return 0;
#else
                if(nvram_match("dhcpc_enable","1") == 0)
                        return 0;
#endif
#endif

        if( action_flags.lan_app == 1 &&
            ( !nvram_match("dhcpd_netbios_enable", "0") || strlen(nvram_safe_get("dhcpd_netbios_enable"))==0 ) &&
            !nvram_match("dns_relay", "1") )
        {
                DEBUG_MSG("when dhcpc get ip and netbios is disable and DNS Relay is enabled, dhcpd don't re-start\n");
                return 0;
        }

        /* WINS SERVER */
        if( nvram_match("dhcpd_netbios_enable", "1") ==0 )
                {
                        /* Learn NetBIOS info from WAN */
                if( nvram_match("dhcpd_netbios_learn", "1") == 0 )
                        {
                                strcpy(wins_server, nvram_safe_get("dhcpd_dynamic_wins_server"));
                                strcpy(node_type, nvram_safe_get("dhcpd_dynamic_node_type"));
                                strcpy(scope, nvram_safe_get("dhcpd_dynamic_scope"));
                        }

                        /* Use static NetBIOS info */
                        else
                        {
                                strcpy(wins_server, nvram_safe_get("dhcpd_static_wins_server"));
                                strcpy(node_type, nvram_safe_get("dhcpd_static_node_type"));
                                strcpy(scope, nvram_safe_get("dhcpd_static_scope"));
                        }

                        /* If Broadcast(B-node) is used, we don't encapsulate thr info of WINS server into DHCP packets.
                         * That is because when client receive DHCP packets which cantains both of B-node and WIN server,
                         * It will transfer to Hybrid(H-node) automatically.
                         */
                        /* B-node: Clean WINS server*/
                        if(!strcmp(node_type,"1"))
                        {
                                memset(wins_server,0,sizeof(wins_server));
                        }
                        /* Other node: Transfer wins server format from 1.1.1.1/2.2.2.2/3.3.3.3 to 1.1.1.1 2.2.2.2 3.3.3.3 */
                        else
                        {
                                if(strlen(wins_server))
                                {
                                        do
                                        {
                                                tok = strchr(wins_server,'/');
                                                if(tok)
                                                        *tok = ' ';
                                        }while(tok);
                                }
                        }
                }

        /* DHCPD */
#ifdef DIR652V
	if (nvram_match("dhcpd_enable", "1" ) == 0 && nvram_match("dhcp_relay", "0") == 0) {
#else
	if (nvram_match("dhcpd_enable", "1" ) == 0) {
#endif
		//***********
		// Jery Lin modified 2009/11/11
		// fixed error message "Failure parsing /var/etc/udhcpd.conf"
		// examine whether wins_server, node_type, scope is empty
		char tmp_wins_server[100], tmp_node_type[100], tmp_scope[100];
		if (strcmp(wins_server,"")==0)
			tmp_wins_server[0]='\0';
		else
			sprintf(tmp_wins_server, "option wins	%s\n", wins_server);

		if (strcmp(node_type,"")==0)
			tmp_node_type[0]='\0';
		else
			sprintf(tmp_node_type, "option nodetype	%s\n", node_type);

		if (strcmp(scope,"")==0)
			tmp_scope[0]='\0';
		else
			sprintf(tmp_scope, "option scope	%s\n", scope);
		//***********

                printf("dhcpd_enabled::::::\n");
                lan_ipaddr = nvram_safe_get("lan_ipaddr");
                init_file(DHCPD_CONF);
                save2file(DHCPD_CONF, "#Sample udhcpd configuration\n"
                                "start %d.%d.%d.%d\n"
                                "end %d.%d.%d.%d\n"
                                "interface %s\n"
                                "max_leases 254\n"
                                "remaining yes\n"
                                "auto_time 7200\n"
                                "decline_time 3600\n"
                                "conflict_time 3600\n"
                                "offer_time 60\n"
                                "min_lease 60\n"
                                "lease_file /var/misc/udhcpd.leases\n"
                                "pidfile /var/run/udhcpd.pid\n"
                                "option domain %s\n"
                                "option dns %s\n"
                                "option subnet %s\n"
                                "option router %s\n"
                                "option lease %d\n"
                                "%s%s%s"	// Jery Lin modified 2009/11/11
                                "always_bcast %d\n"
                                "static_lease %s %s\n",
#ifdef SET_GET_FROM_ARTBLOCK
/* jimmy modified 20080528 */
#if 0
                                get_single_ip(lan_ipaddr, 0), get_single_ip(lan_ipaddr, 1),\
                                get_single_ip(lan_ipaddr, 2), get_single_ip(nvram_safe_get("dhcpd_start"), 3),\
                                get_single_ip(lan_ipaddr, 0), get_single_ip(lan_ipaddr, 1),\
                                get_single_ip(lan_ipaddr, 2), get_single_ip(nvram_safe_get("dhcpd_end"), 3),\

#endif
                                get_single_ip(nvram_safe_get("dhcpd_start"), 0), get_single_ip(nvram_safe_get("dhcpd_start"), 1),\
                                get_single_ip(nvram_safe_get("dhcpd_start"), 2), get_single_ip(nvram_safe_get("dhcpd_start"), 3),\
                                get_single_ip(nvram_safe_get("dhcpd_end"), 0), get_single_ip(nvram_safe_get("dhcpd_end"), 1),\
                                get_single_ip(nvram_safe_get("dhcpd_end"), 2), get_single_ip(nvram_safe_get("dhcpd_end"), 3),\
/* ------------------------ */
                                nvram_safe_get("lan_bridge"),\
                                strlen(nvram_safe_get("dhcpd_domain_name")) > 0 ? nvram_safe_get("dhcpd_domain_name") : get_domain(),\
                                nvram_match("dns_relay", "1") == 0 ? lan_ipaddr : get_dns(),\
                                nvram_safe_get("lan_netmask"),\

#ifdef  AP_NOWAN_HASBRIDGE
                                nvram_safe_get("lan_gateway"),\

#else
                                lan_ipaddr,\

#endif
                                60 * atoi(nvram_safe_get("dhcpd_lease_time")), \
                                tmp_wins_server ,\
                                tmp_node_type,\
                                tmp_scope, \
                                nvram_match("dhcpd_always_bcast", "1") == 0 ?  1 : 0, \
                                artblock_get("lan_mac") == NULL ? nvram_safe_get("lan_mac") : artblock_safe_get("lan_mac"), \
                                lan_ipaddr
                                );
#else//SET_GET_FROM_ARTBLOCK


/* jimmy modified 20080528 */
#if 0
                                get_single_ip(lan_ipaddr, 0), get_single_ip(lan_ipaddr, 1),\
                                get_single_ip(lan_ipaddr, 2), get_single_ip(nvram_safe_get("dhcpd_start"), 3),\
                                get_single_ip(lan_ipaddr, 0), get_single_ip(lan_ipaddr, 1),\
                                get_single_ip(lan_ipaddr, 2), get_single_ip(nvram_safe_get("dhcpd_end"), 3),\

#endif
                                get_single_ip(nvram_safe_get("dhcpd_start"), 0), get_single_ip(nvram_safe_get("dhcpd_start"), 1),\
                                get_single_ip(nvram_safe_get("dhcpd_start"), 2), get_single_ip(nvram_safe_get("dhcpd_start"), 3),\
                                get_single_ip(nvram_safe_get("dhcpd_end"), 0), get_single_ip(nvram_safe_get("dhcpd_end"), 1),\
                                get_single_ip(nvram_safe_get("dhcpd_end"), 2), get_single_ip(nvram_safe_get("dhcpd_end"), 3),\
/* ------------------------ */
                                nvram_safe_get("lan_bridge"),\
                                strlen(nvram_safe_get("dhcpd_domain_name")) > 0 ? nvram_safe_get("dhcpd_domain_name") : get_domain(),\
                                nvram_match("dns_relay", "1") == 0 ? lan_ipaddr : get_dns(),\
                                nvram_safe_get("lan_netmask"),\

#ifdef  AP_NOWAN_HASBRIDGE
                                nvram_safe_get("lan_gateway"),\

#else
                                lan_ipaddr,\

#endif
                                60 * atoi(nvram_safe_get("dhcpd_lease_time")), \
                                wins_server ,\
                                node_type,\
                                scope, \
                                nvram_match("dhcpd_always_bcast", "1") == 0 ?  1 : 0, \
                                nvram_safe_get("lan_mac"), \
                                lan_ipaddr
                                );

#endif//SET_GET_FROM_ARTBLOCK

#ifdef AP_NOWAN_HASBRIDGE
        save2file(DHCPD_CONF,"static_lease %s %s\n","11:22:33:44:55:66",nvram_safe_get("lan_gateway"));
#endif


                /* Reserved IP Address */
                if( nvram_match( "dhcpd_reservation", "1" ) == 0 )
                {
                        for(i=0; i<MAX_DHCPD_RESERVATION_NUMBER; i++)
                        {
                                snprintf(reserved_rule, sizeof(reserved_rule), "dhcpd_reserve_%02d",i) ;
                                if ( nvram_match(reserved_rule, "") == 0 )
                                        continue;
                                else
                                        strcpy( reserved_data, nvram_safe_get(reserved_rule));

                                enable = strtok(reserved_data, "/");
                                name = strtok(NULL, "/");
                                ip = strtok(NULL, "/");
                                mac = strtok(NULL, "/");

                                if( strcmp(enable, "1") == 0 )
                                        save2file(DHCPD_CONF, "static_lease     %s      %s\n", mac, ip);
                        }
                }

                _system("udhcpd &");
        }
        return 0;
        }

int stop_dhcpd(int flag)
{
        FILE *fp_udhcpd;

        if( action_flags.lan_app == 1 &&
                (!nvram_match("dhcpd_netbios_enable", "0") || strlen(nvram_safe_get("dhcpd_netbios_enable"))==0)&&
                !nvram_match("dns_relay", "1") )
        {
                DEBUG_MSG("when dhcpc get ip and netbios is disabled and DNS Relay is enabled, dhcpd don't stop\n");
                return 0;
        }

        if( !nvram_match("dhcpd_enable", "0")  || flag )
        {
                fp_udhcpd = fopen(UDHCPD_PID, "r");
                if(fp_udhcpd)
                {
                        KILL_APP_ASYNCH("udhcpd");
                        unlink(HTML_LEASES_FILE);
                        unlink(LEASES_FILE);
                        sleep(1);
                        fclose(fp_udhcpd);
                }
        }

        return 0;
        }

int start_ddns(void)
{
        char wan_eth[32]={0};
        FILE *fp;
        int pid = -1;
#ifdef  CONFIG_BRIDGE_MODE
                if(nvram_match("wlan0_mode", "ap") == 0)
                        return 0;
#endif
	if(!nvram_match("ddns_type","www.oray.cn"))
	{
/* jimmy modified 20081217, for add oray ddns support */
//orayddns.h: #define ORAY_CONFIG  "/var/etc/oray.conf"
        if((fp = fopen("/var/etc/oray.conf", "w+")) != NULL){
                fprintf(fp, "username=%s\n", nvram_safe_get("ddns_username"));
                fprintf(fp, "password=%s\n", nvram_safe_get("ddns_password"));
                fprintf(fp, "register_name=%s\n", nvram_safe_get("ddns_hostname"));
                fclose(fp);
                if( !nvram_match("ddns_enable", "1")  ){
                        _system("/sbin/orayddns &");
                }
		}
	}
	else
	{
        	if((fp = fopen("/var/etc/ddns.conf", "w+")) != NULL)
        	{
                fprintf(fp, "ddns_enable=%s\n", nvram_safe_get("ddns_enable"));

                if( !nvram_match("ddns_enable", "1")  )
                {
                        if( !nvram_match("wan_proto", "dhcpc") || !nvram_match("wan_proto", "static") )
                        strcpy( wan_eth, nvram_safe_get("wan_eth") );
                else
                        strcpy( wan_eth, "ppp0" );

                        fprintf(fp, "ddns_sync_interval=%d\n", atoi(nvram_safe_get("ddns_sync_interval"))*60*60);
                        fprintf(fp, "wan_eth=%s\n", wan_eth);
                        fprintf(fp, "ddns_hostname=%s\n", nvram_safe_get("ddns_hostname"));
                        fprintf(fp, "ddns_type=%s\n", nvram_safe_get("ddns_type"));
                        fprintf(fp, "ddns_username=%s\n", nvram_safe_get("ddns_username"));
                        fprintf(fp, "ddns_password=%s\n", nvram_safe_get("ddns_password"));
                        fprintf(fp, "ddns_wildcards_enable=%s\n", nvram_safe_get("ddns_wildcards_enable"));
                        fprintf(fp, "ddns_dyndns_kinds=%s\n", nvram_safe_get("ddns_dyndns_kinds"));
                        fprintf(fp, "model_name=%s\n", nvram_safe_get("model_name"));
                        fprintf(fp, "model_number=%s\n", nvram_safe_get("model_number"));

                }
                fclose(fp);
                // jimmy modified 20090729, in case when process timer is not exist,
                // kill will send signal to non-expected process ... ex:udhcpd
                //kill(read_pid(TIMER_PID),SIGINT);
                pid = read_pid(TIMER_PID);
                if ((pid > 0) && !kill(pid,0)) {
                    /* send different signal to timer for Ubicom */
                    FILE *sig_fp;
                    if ((sig_fp = fopen(SIG_TMP_FILE, "w")) != NULL) {
                        fputs("ddns", sig_fp);
                    }
                    fclose(sig_fp);
                    kill(pid, SIGHUP);
                }
        }
	}
/* ------------------------------------------------------- */
        return 0;
}

int stop_ddns(int flag)
{
	if(!nvram_match("ddns_type","www.oray.cn"))
        KILL_APP_ASYNCH("orayddns");
	else
	{
        if( !nvram_match("ddns_enable", "0")  || flag )
        {

                /* Since noip will retry 5 times if fail, it may still run
                 * even after we shutdown noiptime. To avoid it, we kill inadyn also here.
                 */
                KILL_APP_ASYNCH("inadyn");
        }
        unlink("/var/tmp/ddns_status");
	}
/* -------------------------------------- */
        return 0;
}

int start_maillog(void)
{
        char *onlogfull,*p_set_hour,*p_set_weekday,*log_sche_name;
        int sche_rule_count;
        char tmp_sche_list[64] = {}, log_email_schedule[32] ={};
        char schedule_list[]="schedule_rule_00";
        char *sche_name = NULL;
        char *schedule = nvram_safe_get("log_email_schedule");

        /* Mail-On-Schedule */
        /* Ken modify for Uibcome log behavior */
        if((nvram_match("log_email_enable", "1") == 0))
        {
                strcpy(log_email_schedule, nvram_safe_get("log_email_schedule"));
                onlogfull = strtok(log_email_schedule, "/") ? : "";
                p_set_hour = strtok(NULL, "/") ? : "";
                p_set_weekday = strtok(NULL, "/") ? : "";
                log_sche_name = strtok(NULL, "/") ? : "";

                init_file(SMTP_CONF);
                save2file(SMTP_CONF, "log_email_auth=%s\n" "log_email_recipien=%s\n" "log_email_username=%s\n"
                                "log_email_password=%s\n" "log_email_server=%s\n" "log_email_sender=%s\n" "log_email_server_port=%s\n",
                                nvram_safe_get("log_email_auth"),\
                                nvram_safe_get("log_email_recipien"),\
                                nvram_safe_get("log_email_username"), \
                                nvram_safe_get("log_email_password"),\
                                nvram_safe_get("log_email_server"),\
                                nvram_safe_get("log_email_sender"),\
                                strlen(nvram_safe_get("log_email_server_port")) > 0 ? nvram_safe_get("log_email_server_port") : "25"
                                );
                DEBUG_MSG("onlogfull=%s\n",onlogfull);
                DEBUG_MSG("onschedule=%s\n",p_set_hour);
                if(strcmp("1", onlogfull) == 0)
                {
                        DEBUG_MSG("onlogfull enable\n");
                        if(strcmp("25", p_set_hour) == 0){//if p_set_hour==25 then onschedule
                                DEBUG_MSG("onschedule enable\n");
                                if(strcmp("Never", log_sche_name) == 0){
                                        DEBUG_MSG("onschedule Never\n"); //log full
                                        _system("mailosd \"%s\" &", schedule);

                                }
                                else{
                                        DEBUG_MSG("onschedule other\n");
                                        for(sche_rule_count=0; sche_rule_count<MAX_SCHEDULE_NUMBER; sche_rule_count++)
                                        {
                                                memset(tmp_sche_list, 0, 64);
                                                snprintf(schedule_list, sizeof(schedule_list), "schedule_rule_%02d", sche_rule_count);
                                                strcpy(tmp_sche_list, nvram_safe_get(schedule_list));

                                                if ( tmp_sche_list == NULL || strcmp(tmp_sche_list, "") == 0)
                                                        continue;

                                                sche_name = strtok(tmp_sche_list, "/");
                                                if(strcmp(sche_name, log_sche_name) == 0)
							_system("mailosd \"%s\" %s &", schedule, nvram_safe_get(schedule_list));
                                                else
                                                        continue;

                                                break;  /*find log schedule from schedule rules*/
                                        }
                                }
                        }
                        else{//log full
                                DEBUG_MSG("onschedule disable\n");
                        _system("mailosd \"%s\" &", schedule);
                        }
                }
                else
                {
                        DEBUG_MSG("onlogfull disable\n");
                        if(strcmp("25", p_set_hour) == 0)
                        {
                                DEBUG_MSG("onschedule enable\n");
                                if(strcmp("Never", log_sche_name) != 0)//on schedule
                                {
                                        DEBUG_MSG("onschedule other\n");
                                        for(sche_rule_count=0; sche_rule_count<MAX_SCHEDULE_NUMBER; sche_rule_count++)
                                        {
                                                memset(tmp_sche_list, 0, 64);
                                                snprintf(schedule_list, sizeof(schedule_list), "schedule_rule_%02d", sche_rule_count);
                                                strcpy(tmp_sche_list, nvram_safe_get(schedule_list));

                                                if ( tmp_sche_list == NULL || strcmp(tmp_sche_list, "") == 0)
                                                        continue;

                                                sche_name = strtok(tmp_sche_list, "/");
                                                if(strcmp(sche_name, log_sche_name) == 0)
                                                        _system("mailosd \"%s\" %s &", schedule, nvram_safe_get(schedule_list));
                                                else
                                                        continue;

                                                break;  /*find log schedule from schedule rules*/
                                        }
                                }
                        }
                        else
                        {
                                if(strlen(log_sche_name))
                                {
                                        DEBUG_MSG("onschedule disable\n");
                    /*NickChou, mailosd must up in any condition,
                      if no schedule, mailosd can rcv signal for sending log
                    */
                                        if( strlen(nvram_safe_get("log_email_recipien")) && strlen(nvram_safe_get("log_email_sender")) )
                                                _system("mailosd manual &");
                                }
                                else//for other customer ex:TRENNET.... do log full
                                {
                                        DEBUG_MSG("onschedule disable other customer\n");
                                        if( strlen(nvram_safe_get("log_email_recipien")) && strlen(nvram_safe_get("log_email_sender")) )
                                                _system("mailosd \"%s\" &", schedule);
                                }
                        }
                }
        }
        return 0;
}

int stop_maillog(int flag)
{
        DEBUG_MSG("Stop maillog\n");

        if( !nvram_match("log_email_enable", "0")  || flag )
                KILL_APP_ASYNCH("mailosd");
        return 0;
}

/* start wan port connection time mointor */
int start_wantimer(void){
        _system("/var/sbin/wantimer &");
        return 0;
}

/* stop wan port connection time mointor */
int stop_wantimer(int flag){
        DEBUG_MSG("Stop wantimer\n");

        if(flag)
                KILL_APP_SYNCH("wantimer");
        return 0;
}

#ifdef CONFIG_USER_SNMP
/* start snmp*/
int start_snmpd(void){
        char temp[8] = "";
        char temp_auth[8] = "";
        char temp_priv[8] = "";

        if(nvram_match("snmpv3_enable","1") == 0)
        {
        if(nvram_match("auth_protocol","none") == 0)
                strcpy(temp,"noauth");
        else
        {
                if(nvram_match("privacy_protocol","none") == 0)
                        strcpy(temp,"auth");
                else
                        strcpy(temp,"priv");
        }

        if(nvram_match("auth_protocol","md5") == 0)
                strcpy(temp_auth,"MD5");
        if(nvram_match("auth_protocol","sha1") == 0)
                strcpy(temp_auth,"SHA");
        if(nvram_match("privacy_protocol","des") == 0)
                strcpy(temp_priv,"DES");
        if(nvram_match("privacy_protocol","aes") == 0)
                strcpy(temp_priv,"AES");

                init_file(SNMPV3_CONF_FILE);
                save2file(SNMPV3_CONF_FILE,"rwuser %s %s",nvram_safe_get("snmpv3_security_user_name"),temp);
                init_file(SNMPV3_AUTH_FILE);

                if(nvram_match("auth_protocol","none") == 0 )
                        save2file(SNMPV3_AUTH_FILE,"createUser %s ",nvram_safe_get("snmpv3_security_user_name"));
                else
                {
                        if(nvram_match("privacy_protocol","none") == 0 && nvram_match("auth_protocol","none") != 0)
                        {
                                save2file(SNMPV3_AUTH_FILE,"createUser %s %s \"%s\" %s",\
                                nvram_safe_get("snmpv3_security_user_name"),\
                                temp_auth,nvram_safe_get("auth_password"),temp_priv);
                                printf("createUser %s %s \"%s\" %s\n",\
                                nvram_safe_get("snmpv3_security_user_name"),\
                                temp_auth,nvram_safe_get("auth_password"),temp_priv);
                        }
                        if(nvram_match("privacy_protocol","none") != 0 && nvram_match("auth_protocol","none") != 0)
                        {
                                save2file(SNMPV3_AUTH_FILE,"createUser %s %s \"%s\" %s %s",\
                                nvram_safe_get("snmpv3_security_user_name"),\
                                temp_auth,nvram_safe_get("auth_password"),temp_priv,nvram_safe_get("privacy_password"));
                                printf("createUser %s %s \"%s\" %s %s\n",\
                                nvram_safe_get("snmpv3_security_user_name"),\
                                temp_auth,nvram_safe_get("auth_password"),temp_priv,nvram_safe_get("privacy_password"));
                        }
                }

#ifdef CONFIG_USER_SNMP_TRAP
                        if(strcmp(nvram_safe_get("snmp_trap_receiver_1"),"0.0.0.0") != 0 && nvram_match("snmp_trap_receiver_1","")!=0)
                        {
                                if(nvram_match("auth_protocol","none") == 0 )
                                        save2file(SNMPV3_CONF_FILE,"\ntrapsess -v 3 -u %s -l noauthNoPriv %s:162",nvram_safe_get("snmpv3_security_user_name"),nvram_safe_get("snmp_trap_receiver_1"));
                                if( (nvram_match("auth_protocol","md5") == 0 )&& (nvram_match("privacy_protocol","none") == 0) )
                                        save2file(SNMPV3_CONF_FILE,"\ntrapsess -v 3 -u %s -a MD5 -A %s -l authNoPriv %s:162",nvram_safe_get("snmpv3_security_user_name"),nvram_safe_get("auth_password"),nvram_safe_get("snmp_trap_receiver_1"));
                                if( (nvram_match("auth_protocol","md5") == 0 )&& (nvram_match("privacy_protocol","des") == 0) )
                                        save2file(SNMPV3_CONF_FILE,"\ntrapsess -v 3 -u %s -a MD5 -A %s -x DES -X %s -l authPriv %s:162",nvram_safe_get("snmpv3_security_user_name"),nvram_safe_get("auth_password"),nvram_safe_get("privacy_password"),nvram_safe_get("snmp_trap_receiver_1"));
                                if( (nvram_match("auth_protocol","md5") == 0 )&& (nvram_match("privacy_protocol","aes") == 0) )
                                        save2file(SNMPV3_CONF_FILE,"\ntrapsess -v 3 -u %s -a MD5 -A %s -x AES128 -X %s -l authPriv %s:162",nvram_safe_get("snmpv3_security_user_name"),nvram_safe_get("auth_password"),nvram_safe_get("privacy_password"),nvram_safe_get("snmp_trap_receiver_1"));
                                if( (nvram_match("auth_protocol","sha1") == 0 )&& (nvram_match("privacy_protocol","none") == 0) )
                                        save2file(SNMPV3_CONF_FILE,"\ntrapsess -v 3 -u %s -a SHA -A %s -l authNoPriv %s:162",nvram_safe_get("snmpv3_security_user_name"),nvram_safe_get("auth_password"),nvram_safe_get("snmp_trap_receiver_1"));
                                if( (nvram_match("auth_protocol","sha1") == 0 )&& (nvram_match("privacy_protocol","des") == 0) )
                                        save2file(SNMPV3_CONF_FILE,"\ntrapsess -v 3 -u %s -a SHA -A %s -x DES -X %s -l authPriv %s:162",nvram_safe_get("snmpv3_security_user_name"),nvram_safe_get("auth_password"),nvram_safe_get("privacy_password"),nvram_safe_get("snmp_trap_receiver_1"));
                                if( (nvram_match("auth_protocol","sha1") == 0 )&& (nvram_match("privacy_protocol","aes") == 0) )
                                        save2file(SNMPV3_CONF_FILE,"\ntrapsess -v 3 -u %s -a SHA -A %s -x AES128 -X %s -l authPriv %s:162",nvram_safe_get("snmpv3_security_user_name"),nvram_safe_get("auth_password"),nvram_safe_get("privacy_password"),nvram_safe_get("snmp_trap_receiver_1"));
                        }

                        if(strcmp(nvram_safe_get("snmp_trap_receiver_2"),"0.0.0.0") != 0 && nvram_match("snmp_trap_receiver_2","")!=0)
                        {
                                if(nvram_match("auth_protocol","none") == 0 )
                                        save2file(SNMPV3_CONF_FILE,"\ntrapsess -v 3 -u %s -l noauthNoPriv %s:162",nvram_safe_get("snmpv3_security_user_name"),nvram_safe_get("snmp_trap_receiver_2"));
                                if( (nvram_match("auth_protocol","md5") == 0 )&& (nvram_match("privacy_protocol","none") == 0) )
                                        save2file(SNMPV3_CONF_FILE,"\ntrapsess -v 3 -u %s -a MD5 -A %s -l authNoPriv %s:162",nvram_safe_get("snmpv3_security_user_name"),nvram_safe_get("auth_password"),nvram_safe_get("snmp_trap_receiver_2"));
                                if( (nvram_match("auth_protocol","md5") == 0 )&& (nvram_match("privacy_protocol","des") == 0) )
                                        save2file(SNMPV3_CONF_FILE,"\ntrapsess -v 3 -u %s -a MD5 -A %s -x DES -X %s -l authPriv %s:162",nvram_safe_get("snmpv3_security_user_name"),nvram_safe_get("auth_password"),nvram_safe_get("privacy_password"),nvram_safe_get("snmp_trap_receiver_2"));
                                if( (nvram_match("auth_protocol","md5") == 0 )&& (nvram_match("privacy_protocol","aes") == 0) )
                                        save2file(SNMPV3_CONF_FILE,"\ntrapsess -v 3 -u %s -a MD5 -A %s -x AES128 -X %s -l authPriv %s:162",nvram_safe_get("snmpv3_security_user_name"),nvram_safe_get("auth_password"),nvram_safe_get("privacy_password"),nvram_safe_get("snmp_trap_receiver_2"));
                                if( (nvram_match("auth_protocol","sha1") == 0 )&& (nvram_match("privacy_protocol","none") == 0) )
                                        save2file(SNMPV3_CONF_FILE,"\ntrapsess -v 3 -u %s -a SHA -A %s -l authNoPriv %s:162",nvram_safe_get("snmpv3_security_user_name"),nvram_safe_get("auth_password"),nvram_safe_get("snmp_trap_receiver_2"));
                                if( (nvram_match("auth_protocol","sha1") == 0 )&& (nvram_match("privacy_protocol","des") == 0) )
                                        save2file(SNMPV3_CONF_FILE,"\ntrapsess -v 3 -u %s -a SHA -A %s -x DES -X %s -l authPriv %s:162",nvram_safe_get("snmpv3_security_user_name"),nvram_safe_get("auth_password"),nvram_safe_get("privacy_password"),nvram_safe_get("snmp_trap_receiver_2"));
                                if( (nvram_match("auth_protocol","sha1") == 0 )&& (nvram_match("privacy_protocol","aes") == 0) )
                                        save2file(SNMPV3_CONF_FILE,"\ntrapsess -v 3 -u %s -a SHA -A %s -x AES128 -X %s -l authPriv %s:162",nvram_safe_get("snmpv3_security_user_name"),nvram_safe_get("auth_password"),nvram_safe_get("privacy_password"),nvram_safe_get("snmp_trap_receiver_2"));
                        }

                        if(strcmp(nvram_safe_get("snmp_trap_receiver_3"),"0.0.0.0") != 0 && nvram_match("snmp_trap_receiver_3","")!=0)
                        {
                                if(nvram_match("auth_protocol","none") == 0 )
                                        save2file(SNMPV3_CONF_FILE,"\ntrapsess -v 3 -u %s -l noauthNoPriv %s:162",nvram_safe_get("snmpv3_security_user_name"),nvram_safe_get("snmp_trap_receiver_3"));
                                if( (nvram_match("auth_protocol","md5") == 0 )&& (nvram_match("privacy_protocol","none") == 0) )
                                        save2file(SNMPV3_CONF_FILE,"\ntrapsess -v 3 -u %s -a MD5 -A %s -l authNoPriv %s:162",nvram_safe_get("snmpv3_security_user_name"),nvram_safe_get("auth_password"),nvram_safe_get("snmp_trap_receiver_3"));
                                if( (nvram_match("auth_protocol","md5") == 0 )&& (nvram_match("privacy_protocol","des") == 0) )
                                        save2file(SNMPV3_CONF_FILE,"\ntrapsess -v 3 -u %s -a MD5 -A %s -x DES -X %s -l authPriv %s:162",nvram_safe_get("snmpv3_security_user_name"),nvram_safe_get("auth_password"),nvram_safe_get("privacy_password"),nvram_safe_get("snmp_trap_receiver_3"));
                                if( (nvram_match("auth_protocol","md5") == 0 )&& (nvram_match("privacy_protocol","aes") == 0) )
                                        save2file(SNMPV3_CONF_FILE,"\ntrapsess -v 3 -u %s -a MD5 -A %s -x AES128 -X %s -l authPriv %s:162",nvram_safe_get("snmpv3_security_user_name"),nvram_safe_get("auth_password"),nvram_safe_get("privacy_password"),nvram_safe_get("snmp_trap_receiver_3"));
                                if( (nvram_match("auth_protocol","sha1") == 0 )&& (nvram_match("privacy_protocol","none") == 0) )
                                        save2file(SNMPV3_CONF_FILE,"\ntrapsess -v 3 -u %s -a SHA -A %s -l authNoPriv %s:162",nvram_safe_get("snmpv3_security_user_name"),nvram_safe_get("auth_password"),nvram_safe_get("snmp_trap_receiver_3"));
                                if( (nvram_match("auth_protocol","sha1") == 0 )&& (nvram_match("privacy_protocol","des") == 0) )
                                        save2file(SNMPV3_CONF_FILE,"\ntrapsess -v 3 -u %s -a SHA -A %s -x DES -X %s -l authPriv %s:162",nvram_safe_get("snmpv3_security_user_name"),nvram_safe_get("auth_password"),nvram_safe_get("privacy_password"),nvram_safe_get("snmp_trap_receiver_3"));
                                if( (nvram_match("auth_protocol","sha1") == 0 )&& (nvram_match("privacy_protocol","aes") == 0) )
                                        save2file(SNMPV3_CONF_FILE,"\ntrapsess -v 3 -u %s -a SHA -A %s -x AES128 -X %s -l authPriv %s:162",nvram_safe_get("snmpv3_security_user_name"),nvram_safe_get("auth_password"),nvram_safe_get("privacy_password"),nvram_safe_get("snmp_trap_receiver_3"));
                        }
#endif

                _system("snmpd -f -c %s &",SNMPV3_CONF_FILE);
        }
}
#endif

#ifdef CONFIG_USER_SNMP
/* stop snmp*/
int stop_snmpd(int flag){
        DEBUG_MSG("Stop snmpd\n");

        if(flag)
                KILL_APP_SYNCH("snmpd");
        return 0;
}
#endif

int start_lanmon(void)
{
        char *lan_ipaddr, *lan_bridge;

        lan_ipaddr = nvram_safe_get("lan_ipaddr");
        lan_bridge = nvram_safe_get("lan_bridge");

        _system("/var/sbin/lanmon 5 %s %s &", lan_ipaddr, lan_bridge);
        return 0;
}

int stop_lanmon(int flag)
{
        KILL_APP_SYNCH("lanmon");
        return 0;
}

int start_syslogd(void)
{
        char *hostname, *nvram_syslog_server, *syslog_enable, *syslog_ip;
        char tmp_hostname[40]={};
        char tmp_syslog_server[20]={};

        if(action_flags.lan_app == 1)
        {
                DEBUG_MSG("when dhcpc get ip, syslogd don't re-start\n");
                return 0;
        }

        hostname = nvram_safe_get("hostname");
        if (strlen(hostname) > 1)
                strcpy(tmp_hostname, hostname);
        else
                strcpy(tmp_hostname, nvram_safe_get("model_number"));

        _system("hostname \"%s\"", parse_special_char(tmp_hostname) );

        nvram_syslog_server = nvram_safe_get("syslog_server");
        strcpy(tmp_syslog_server, nvram_syslog_server);
        if( strlen(nvram_syslog_server) )
        {
            syslog_enable = strtok(tmp_syslog_server, "/");
            syslog_ip = strtok(NULL, "/");

            if(syslog_enable == NULL || syslog_ip == NULL)
            {
                printf("start_syslogd: syslog_server error\n");
/* jimmy modified for IPC syslog */
/* Note:
                The codes here and ej_cmo_get_log() in /httpd/cmobasicapi.c
                must be identically !!
*/
//              _system("syslogd -s %d -b 0 &", LOG_MAX_SIZE);
                        _system("syslogd -O %s -b 0 &",LOG_FILE_BAK);
/* ---------------------------------*/
            }
            else
            {
/* jimmy modified for IPC syslog */
//                  if( strcmp(syslog_enable, "1") == 0 )
//                              _system("syslogd -s %d -b 0 -L -R %s:514 &", LOG_MAX_SIZE, syslog_ip);
//                  else
//                              _system("syslogd -s %d -b 0 &", LOG_MAX_SIZE);
                    if( strcmp(syslog_enable, "1") == 0 )
                                _system("syslogd -O %s -L -R %s:514 &",LOG_FILE_BAK , syslog_ip);
                    else
				_system("syslogd -O %s &",LOG_FILE_BAK);
/* ---------------------------------*/
                }
        }
        else
/* jimmy modified for IPC syslog */
//              _system("syslogd -s %d -b 0 &", LOG_MAX_SIZE);
                _system("syslogd -O %s -b 0 &",LOG_FILE_BAK);
/* -------------------------*/

        return 0;
}

int stop_syslogd(int flag)
{
        if(action_flags.lan_app == 1)
        {
                DEBUG_MSG("when dhcpc get ip, syslogd don't stop\n");
                return 0;
        }

        if(flag)
                KILL_APP_ASYNCH("syslogd");
        return 0;
}

int start_klogd(void)
{
        if(action_flags.lan_app == 1)
        {
                DEBUG_MSG("when dhcpc get ip, klogd don't re-start\n");
                return 0;
        }

        system("klogd &");
        return 0;
}

int stop_klogd(int flag)
{
        if(action_flags.lan_app == 1)
        {
                DEBUG_MSG("when dhcpc get ip, klogd don't stop\n");
                return 0;
        }

        if(flag)
                KILL_APP_ASYNCH("klogd");
        return 0;
}

#ifdef MPPPOE
void parse_mpppoe_dns_server(void)
{
        FILE *fp;
        char dns_ip[32] = {};
        init_file(DNS_FINAL_FILE);
        fp = fopen(DNS_FILE_00,"r");
        if(fp)
        {
                save2file(DNS_FINAL_FILE,"ppp0\n");
                while(fgets(dns_ip,32,fp))
                {
                        save2file(DNS_FINAL_FILE,"%s",dns_ip);
                        memset(dns_ip,0,sizeof(dns_ip));
                }
                fclose(fp);
        }
        fp = fopen(DNS_FILE_01,"r");
        if(fp)
        {
                save2file(DNS_FINAL_FILE,"ppp1\n");
                while(fgets(dns_ip,32,fp))
                {
                        save2file(DNS_FINAL_FILE,"%s",dns_ip);
                        memset(dns_ip,0,sizeof(dns_ip));
                }
                fclose(fp);
        }
        fp = fopen(DNS_FILE_02,"r");
        if(fp)
        {
                save2file(DNS_FINAL_FILE,"ppp2\n");
                while(fgets(dns_ip,32,fp))
                {
                        save2file(DNS_FINAL_FILE,"%s",dns_ip);
                        memset(dns_ip,0,sizeof(dns_ip));
                }
                fclose(fp);
        }
        fp = fopen(DNS_FILE_03,"r");
        if(fp)
        {
                save2file(DNS_FINAL_FILE,"ppp3\n");
                while(fgets(dns_ip,32,fp))
                {
                        save2file(DNS_FINAL_FILE,"%s",dns_ip);
                        memset(dns_ip,0,sizeof(dns_ip));
                }
                fclose(fp);
        }
        fp = fopen(DNS_FILE_04,"r");
        if(fp)
        {
                save2file(DNS_FINAL_FILE,"ppp4\n");
                while(fgets(dns_ip,32,fp))
                {
                        save2file(DNS_FINAL_FILE,"%s",dns_ip);
                        memset(dns_ip,0,sizeof(dns_ip));
                }
                fclose(fp);
        }
}

char *marge_dns_server(void)
{
        FILE *fp;
        char content[256] = {};
        char dns_server_list[256] = {};
        char dns_server_1[32] = {};
        char dns_server_2[32] = {};
        char temp[128] = {};
        //just for nslookup at mpppoe
        init_file(RESOLVFILE);
        fp = fopen(DNS_FINAL_FILE,"r");
        if(fp)
        {
                while(fgets(content,256,fp))
                {
                        if(strstr(content,"ppp0"))
                        {
                                fgets(dns_server_1,32,fp);
                                dns_server_1[strlen(dns_server_1)-1] = '\0';
                                fgets(dns_server_2,32,fp);
                                dns_server_2[strlen(dns_server_2)-1] = '\0';

                                if(strlen(dns_server_1) > 0)
                                {
                                        strcat(temp," -s ");
                                        strcat(temp,dns_server_1);
                                        save2file(RESOLVFILE,"nameserver %s\n",dns_server_1);
                                }
                                if(strlen(dns_server_2) > 0)
                                {
                                        strcat(temp," -s ");
                                        strcat(temp,dns_server_2);
                                        save2file(RESOLVFILE,"nameserver %s\n",dns_server_2);
                                }

                                strcat(dns_server_list,temp);
                                DEBUG_MSG(" ppp0 dns_server_list = %s\n",dns_server_list);
                                memset(dns_server_1,0,sizeof(dns_server_1));
                                memset(dns_server_2,0,sizeof(dns_server_2));
                                memset(temp,0,sizeof(temp));
                        }
                        else if(strstr(content,"ppp1"))
                        {
                                fgets(dns_server_1,32,fp);
                                dns_server_1[strlen(dns_server_1)-1] = '\0';
                                fgets(dns_server_2,32,fp);
                                dns_server_2[strlen(dns_server_2)-1] = '\0';

                                if(strlen(dns_server_1) > 0)
                                {
                                        strcat(temp," -s ");
                                        strcat(temp,dns_server_1);
                                        save2file(RESOLVFILE,"nameserver %s\n",dns_server_1);
                                }
                                if(strlen(dns_server_2) > 0)
                                {
                                        strcat(temp," -s ");
                                        strcat(temp,dns_server_2);
                                        save2file(RESOLVFILE,"nameserver %s\n",dns_server_2);
                                }

                                strcat(dns_server_list,temp);
                                DEBUG_MSG(" ppp1 dns_server_list = %s\n",dns_server_list);
                                memset(dns_server_1,0,sizeof(dns_server_1));
                                memset(dns_server_2,0,sizeof(dns_server_2));
                                memset(temp,0,sizeof(temp));

                        }
                        else if(strstr(content,"ppp2"))
                        {
                                fgets(dns_server_1,32,fp);
                                dns_server_1[strlen(dns_server_1)-1] = '\0';
                                fgets(dns_server_2,32,fp);
                                dns_server_2[strlen(dns_server_2)-1] = '\0';

                                if(strlen(dns_server_1) > 0)
                                {
                                        strcat(temp," -s ");
                                        strcat(temp,dns_server_1);
                                        save2file(RESOLVFILE,"nameserver %s\n",dns_server_1);
                                }
                                if(strlen(dns_server_2) > 0)
                                {
                                        strcat(temp," -s ");
                                        strcat(temp,dns_server_2);
                                        save2file(RESOLVFILE,"nameserver %s\n",dns_server_2);
                                }

                                strcat(dns_server_list,temp);
                                DEBUG_MSG(" ppp2 dns_server_list = %s\n",dns_server_list);
                                memset(dns_server_1,0,sizeof(dns_server_1));
                                memset(dns_server_2,0,sizeof(dns_server_2));
                                memset(temp,0,sizeof(temp));

                        }
                        else if(strstr(content,"ppp3"))
                        {
                                fgets(dns_server_1,32,fp);
                                dns_server_1[strlen(dns_server_1)-1] = '\0';
                                fgets(dns_server_2,32,fp);
                                dns_server_2[strlen(dns_server_2)-1] = '\0';

                                if(strlen(dns_server_1) > 0)
                                {
                                        strcat(temp," -s ");
                                        strcat(temp,dns_server_1);
                                        save2file(RESOLVFILE,"nameserver %s\n",dns_server_1);
                                }
                                if(strlen(dns_server_2) > 0)
                                {
                                        strcat(temp," -s ");
                                        strcat(temp,dns_server_2);
                                        save2file(RESOLVFILE,"nameserver %s\n",dns_server_2);
                                }

                                strcat(dns_server_list,temp);
                                DEBUG_MSG(" ppp3 dns_server_list = %s\n",dns_server_list);
                                memset(dns_server_1,0,sizeof(dns_server_1));
                                memset(dns_server_2,0,sizeof(dns_server_2));
                                memset(temp,0,sizeof(temp));

                        }
                        else if(strstr(content,"ppp4"))
                        {
                                fgets(dns_server_1,32,fp);
                                dns_server_1[strlen(dns_server_1)-1] = '\0';
                                fgets(dns_server_2,32,fp);
                                dns_server_2[strlen(dns_server_2)-1] = '\0';

                                if(strlen(dns_server_1) > 0)
                                {
                                        strcat(temp," -s ");
                                        strcat(temp,dns_server_1);
                                        save2file(RESOLVFILE,"nameserver %s\n",dns_server_1);
                                }
                                if(strlen(dns_server_2) > 0)
                                {
                                        strcat(temp," -s ");
                                        strcat(temp,dns_server_2);
                                        save2file(RESOLVFILE,"nameserver %s\n",dns_server_2);
                                }

                                strcat(dns_server_list,temp);
                                DEBUG_MSG(" ppp4 dns_server_list = %s\n",dns_server_list);
                                memset(dns_server_1,0,sizeof(dns_server_1));
                                memset(dns_server_2,0,sizeof(dns_server_2));
                                memset(temp,0,sizeof(temp));
                        }
                }
                fclose(fp);
        }
        DEBUG_MSG("COMMAND = %s\n",dns_server_list);
        return dns_server_list;
}
#endif

#ifdef MRD_ENABLE
int start_mrd(void)
{
        write_mrd_conf();
        if(nvram_match("ipv6_mrd_status","on") == 0)
                _system("mrd -f %s &",MRD_CONF_FILE);
        return 0;
}

int stop_mrd(int flag)
{
        DEBUG_MSG("Stop mrd\n");
        if(flag)
                KILL_APP_ASYNCH("mrd");
        return 0;
}
#endif

#ifdef CONFIG_USER_OWERA
int start_owera(void)
{
        char uid[32]  = "0123456789abcdef0000";
        char lan_mac[18] = {0};
        char *mac_ptr = NULL;
        int i;
#ifdef SET_GET_FROM_ARTBLOCK
        if(artblock_get("lan_mac"))
                strncpy(lan_mac, artblock_safe_get("lan_mac"), 18);
        else
                strncpy(lan_mac, nvram_safe_get("lan_mac"), 18);
#else
        strncpy(lan_mac, nvram_safe_get("lan_mac"), 18);
#endif
        mac_ptr = lan_mac;

        //remove ':'
        for(i =0 ; i< 5; i++ )
                memmove(lan_mac+2+(i*2), lan_mac+3+(i*3), 2);

        strncat(uid, lan_mac, 12);
        printf("OWERA Unit_ID=%s\n", uid);

        _system("oppc -a 67.130.140.39 -p 9117 -y 000200 -i %s &", uid);
}

int stop_owera(void)
{
        KILL_APP_ASYNCH("oppc");
        return 0;
}
#endif

#ifdef CONFIG_USER_NMBD
int start_nmbd(void)
{
/* jimmy modified 20080623  */
//      char *name="dlinkrouter";
//      DEBUG_MSG("start_nmbd name=%s\n", name);
//      _system("nmbd %s &", name);
#ifdef USE_SILEX
#define SAMBA_CONFIG_FILE "/etc/samba/smb.conf"
	FILE *fp;
#endif
        char *name = NULL;

        name=nvram_safe_get("lan_device_name");
        if(name){
                _system("nmbd %s &", name);
                DEBUG_MSG("start_nmbd name=%s\n", name);
        }else{
                _system("nmbd dlinkrouter &");
                DEBUG_MSG("start_nmbd name = dlinkrouter\n");
        }
#ifdef USE_SILEX
	fp=fopen( SAMBA_CONFIG_FILE, "w");
        if (fp) {
            fprintf(fp,
                "[global]\n"
                "	workgroup = WORKGROUP\n"
                "	netbios name = %s\n"
                "	server string = D-Link %s\n"
                "	security = SHARE\n"
                "	load printers = no\n"
                "	guest ok = yes\n"
                "\n"
                "include = /etc/samba/smb.def.conf\n"
                "include = /tmp/smb.dir.conf\n"
                , nvram_safe_get("lan_device_name"), nvram_safe_get("model_number"));

            fclose(fp);
       }
#endif

/* -------------------------- */
}

int stop_nmbd(void)
{
        KILL_APP_ASYNCH("nmbd");
        return 0;
}
#endif




#ifdef CONFIG_USER_WCN


int start_wcnd(void)
{
/*
*  Date 2009-03-09
*  Name Albert Chen
*  Detail get lan from new block
*/
        char *lan_mac = NULL;
        char commBuf[30] ={0};
        char *usb_type = nvram_safe_get("usb_type");
        char *wan_proto = nvram_safe_get("wan_proto");

#ifdef SET_GET_FROM_ARTBLOCK
        if(!(lan_mac = artblock_get("lan_mac")))
                lan_mac = nvram_safe_get("lan_mac");
#else
        lan_mac = nvram_safe_get("lan_mac");
#endif

        sprintf(commBuf, "wcnd -m %s &", lan_mac);


/*
*  Date: 2010-12-03
*  Name: Jerry Chen
*  Reason:Follow NEW Silex spec usb_type = SharePort.
*/
        //if(strcmp(usb_type, "0")==0 && strcmp(wan_proto, "usb3g")) //Using Network USB Utility (SharePort Utility)
        if(1) //always rum SharePort
        {
#ifndef IP8000
#ifdef CONFIG_USER_WL_ATH_GUEST_ZONE
                char *lan_bridge = nvram_safe_get("lan_bridge");
                char *lan_ipaddr = nvram_safe_get("lan_ipaddr");
                char *netusb_guest_zone = nvram_safe_get("netusb_guest_zone");
                int wlan0_enable = 0;
                int wlan0_vap1_enable = 0;
                int wlan0_vap_guest_zone = 0;
                int wlan1_enable = 0;
                int wlan1_vap1_enable = 0;
                int wlan1_vap_guest_zone = 0;
#endif //CONFIG_USER_WL_ATH_GUEST_ZONE
#endif

#ifdef USE_KCODES
                system("insmod /lib/modules/GPL_NetUSB.ko");
                system("insmod /lib/modules/NetUSB.ko");
#endif
#ifndef IP8000
#ifdef CONFIG_USER_WL_ATH_GUEST_ZONE
                wlan0_enable = atoi(nvram_safe_get("wlan0_enable"));
                //John 2010.04.23 remove unused wlan1_enable = atoi(nvram_safe_get("wlan1_enable"));
                wlan0_vap1_enable = atoi(nvram_safe_get("wlan0_vap1_enable"));
                //John 2010.04.23 remove unused wlan1_vap1_enable = atoi(nvram_safe_get("wlan1_vap1_enable"));
                wlan0_vap_guest_zone = atoi(nvram_safe_get("wlan0_vap_guest_zone"));
                //John 2010.04.23 remove unused wlan1_vap_guest_zone = atoi(nvram_safe_get("wlan1_vap_guest_zone"));

                //setguestzone <bridge> <device>:<Enable_SharePort>:<Enable_RouteInLan> <bridge ip>
                if(wlan0_enable==1 && wlan0_vap1_enable==1)
                {
                        if(wlan1_enable==1)
                                _system("brctl setguestzone %s ath2:%s:%d %s", lan_bridge, netusb_guest_zone, !wlan0_vap_guest_zone, lan_ipaddr);
                        else
                                _system("brctl setguestzone %s ath1:%s:%d %s", lan_bridge, netusb_guest_zone, !wlan0_vap_guest_zone, lan_ipaddr);
                }

                if(wlan1_enable==1 && wlan1_vap1_enable==1)
                {
                        if(wlan0_enable==0)
                                _system("brctl setguestzone %s ath1:%s:%d %s", lan_bridge, netusb_guest_zone,!wlan1_vap_guest_zone,lan_ipaddr);
                        else
                        {
                                if(wlan0_vap1_enable==0)
                                        _system("brctl setguestzone %s ath2:%s:%d %s", lan_bridge, netusb_guest_zone,!wlan1_vap_guest_zone,lan_ipaddr);
                                else
                                        _system("brctl setguestzone %s ath3:%s:%d %s", lan_bridge, netusb_guest_zone,!wlan1_vap_guest_zone,lan_ipaddr);
                        }
                }
#endif //#ifdef CONFIG_USER_WL_ATH_GUEST_ZONE
#endif
        }
        return 0;
}

int stop_wcnd(int flag)
{

#ifdef USE_KCODES
        system("rmmod NetUSB");
        system("rmmod GPL_NetUSB");
#endif

        system("umount /var/usb > /dev/null 2>&1");//For WCN

        /*NickChou,
        Remove usb-storage when using SharePort/WCN.
        If we use usb3g, wantimer will opendir("/proc/scsi/usb-storage").
        So we can not remove usb-storage to prevent wantimer crash.
        */
#ifdef CONFIG_USER_3G_USB_CLIENT
        if(nvram_match("usb_type", "2"))
#endif
/*
 * Date: 2009-08-25
 * Name: Ubicom
 * Reason: USB support is not built as kernel modules; therefore the
 *              following line is commented
 */
//              system("rmmod usb-storage");
//                system("[ -n \"`lsmod | grep usb_storage`\" ] && rmmod usb-storage");

        KILL_APP_ASYNCH("wcnd");
        return 0;

}
#endif //#ifdef CONFIG_USER_WCN


int start_dlna(void)
{
#define DLNA_CONF_FILE   "/etc/minidlna.conf"
	
    if (!nvram_match("media_server_enable", "1")) {
    	char buf[512];
    	FILE *fp;
    	
        remove(DLNA_CONF_FILE);
        fp = fopen(DLNA_CONF_FILE, "w");
        if (fp) {
            fprintf(fp, 
                "port=8200\n"
                "network_interface=%s\n" 
                "media_dir=/mnt/shared\n"
                "friendly_name=%s\n"
                "album_art_names=Cover.jpg/cover.jpg/AlbumArtSmall.jpg/albumartsmall.jpg/AlbumArt.jpg/albumart.jpg/Album.jpg/album.jpg/Folder.jpg/folder.jpg/Thumb.jpg/thumb.jpg\n"
                "inotify=yes\n"
                "enable_tivo=no\n"
                "strict_dlna=yes\n"
                "notify_interval=900\n"
                "serial=12345678\n"
                "model_number=1\n"
                , nvram_safe_get("lan_bridge"), nvram_safe_get("media_server_name"));

       	    fclose(fp);
       }
//	_system("/sbin/minidlna -d -P /var/run/minidlna.pid -f %s > /var/log/minidlna.log &", DLNA_CONF_FILE);
    }

    return 0;
}

int stop_dlna(int flag)
{
//	if (!nvram_match("media_server_enable", "0"))
//    KILL_APP_ASYNCH("minidlna");

	return 0;
}


#ifdef CONFIG_USER_OPENAGENT
/*
 * Date: 2009-03-31
 * Name: Norp Huang
 * Description: TR069 client start API, called by rc.
 *                              source code is from Works Systems, OpenAgent2.0, LINCENSED
 *
 */
int start_tr069(void)
{
//        if (!nvram_match("tr069_enable", "1"))
        if ( !nvram_match("tr069_enable", "1") && !nvram_match("tr069_ui", "1"))
        {
        	// copy events
//        		_system("cp /etc/tr/events /home/tr/conf/events -f");
 //       		_system("rm /etc/tr/events");
                _system("mkdir /etc/tr &");
//                _system("tr069 -d /home/tr/conf &");
        	
#if 0        	
                char *acs_usr = NULL, *acs_pwd = NULL, *acs_url = NULL, *acs_def_url = NULL;
                char *cpe_url = NULL, *cpe_usr = NULL, *cpe_pwd = NULL;

                DEBUG_MSG("Start TR069 agent\n");
                _system("agent -F /var/etc/ &");
                //need some delay??
                sleep(2);
                acs_usr = nvram_safe_get("tr069_acs_usr");
                if (strlen(acs_usr))
                        _system("tr069 mngsrv acsuser %s", acs_usr);
                acs_pwd = nvram_safe_get("tr069_acs_pwd");
                if (strlen(acs_pwd))
                        _system("tr069 mngsrv acspwd %s", acs_pwd);
                acs_def_url = nvram_safe_get("tr069_acs_def_url");
                if (strlen(acs_def_url))
                        _system("tr069 mngsrv acsurl %s", acs_def_url);
                acs_url = nvram_safe_get("tr069_acs_url");
                if (strlen(acs_url))
                        _system("tr069 mngsrv acsurl %s", acs_url);
                cpe_url = nvram_safe_get("tr069_cpe_url");
                if (strlen(cpe_url)) //parse port and URI of CPE.
                {
                        char *p = NULL;
                        char port[12]={}, uri[128]={};

                        p = strstr(cpe_url, "/");
                        if (p)
                        {
                                if (cpe_url[0] == 0x3A) // char : as having specify port
                                        strncpy(port, cpe_url + 1, p - (cpe_url + 1));
                                else
                                        strncpy(port, cpe_url, p - cpe_url); // TODO: only having uri(no specify port)
                                strncpy(uri, p+1, strlen(p));
                                _system("tr069 mngsrv reqport %s", port);
                                _system("tr069 mngsrv requri %s", uri);
                        }
                }
                cpe_usr = nvram_safe_get("tr069_cpe_usr");
                if (strlen(cpe_usr))
                        _system("tr069 mngsrv requser %s", cpe_usr);
                cpe_pwd = nvram_safe_get("tr069_cpe_pwd");
                if (strlen(cpe_pwd))
                        _system("tr069 mngsrv reqpwd %s", cpe_pwd);

                if (!nvram_match("tr069_inform_enable", "1")) //TR069 periodic inform enable.
                {
                        int inform_interval = 0;
                        char *inform_time = NULL;


                        DEBUG_MSG("set periodic inform parameters of TR069.\n");
                        _system("tr069 mngsrv enable %d", 1);
                        inform_interval = atoi(nvram_safe_get("tr069_inform_interval"));
                        if (inform_interval)
                                _system("tr069 mngsrv interval %d", inform_interval);
                        inform_time = nvram_safe_get("tr069_inform_time");
                        if (inform_time)
                                _system("tr069 mngsrv time %s", inform_time);
                }
                else
                        _system("tr069 mngsrv enable %d", 0);
#endif

                return 0;
        }
        else
        {
                DEBUG_MSG("Start TR069 fail, please check GUI/nvram\n");
                return 1;
        }
}

/*
 * Date: 2009-03-31
 * Name: Norp Huang
 * Description: TR069 client stop API
 */
int stop_tr069(void)
{
                DEBUG_MSG("Stop TR069 agent\n");
//                KILL_APP_SYNCH("agent");
//				_system("mkdir /etc/tr");
//				_system("cp /home/tr/conf/events /etc/tr/events -f");

                KILL_APP_SYNCH("tr069");
                return 0;
}
#endif

/*
*       Date: 2009-4-30
*       Name: Ken_Chiang
*       Reason: modify for support heartbeat feature.
*       Notice :
*/
#ifdef CONFIG_USER_HEARTBEAT
#define HEARTBEAT_CONFIG  "/var/etc/heattbeat.conf"
int start_heartbeat(void)
{
        FILE *fp;
        DEBUG_MSG("%s: \n",__func__);
#ifdef  CONFIG_BRIDGE_MODE
                if(nvram_match("wlan0_mode", "ap") == 0)
                        return 0;
#endif

        if((fp = fopen(HEARTBEAT_CONFIG, "w+")) != NULL){
                DEBUG_MSG("%s: open heartbeat config\n",__func__);
                fprintf(fp, "server=%s\n", nvram_safe_get("heartbeat_server"));
                fprintf(fp, "interval=%s\n", nvram_safe_get("heartbeat_interval"));
                fprintf(fp, "wanproto=%s\n", nvram_safe_get("wan_proto"));
                fclose(fp);

                if( nvram_match("heartbeat_interval", "0")  ){
                        DEBUG_MSG("%s: start heartbeat\n",__func__);
                        _system("/sbin/heartbeat &");
                }
        }

        return 0;
}

int stop_heartbeat(void)
{
        DEBUG_MSG("%s: \n",__func__);
        KILL_APP_ASYNCH("heartbeat");

        return 0;
}
#endif

/*
*       Date: 2009-12-01
*       Name: Jimmy Huang
*       Reason: Add support for LCD GUI program " lcdshowd "
*       Notice :
*/
#ifdef CONFIG_USER_LCDSHOWD
int start_lcdshowd(void){
        // when rc restart, we don't let lcdshowd re-run
        if(read_pid(LCDSHOWD_PID) <= 0){
                _system("/bin/lcdshowd &");
        }
}

int stop_lcdshowd(void){
        return 1;
}
#endif

#ifdef PPPOE_RELAY
#define PPPOE_RELAY_MAX_SESSION 100
int start_pppoe_relay(void)
{
        char *lan_if, *wan_if;
        printf("%s: \n",__func__);
#ifdef  CONFIG_BRIDGE_MODE
        if(nvram_match("wlan0_mode", "ap") == 0)
                return 0;
#endif

        lan_if = "br0";
        wan_if = nvram_safe_get("wan_eth");
        if( nvram_match("pppoe_pass_through", "1") == 0 ){
                printf("%s: pppoe_pass_through\n",__func__);
                _system("%s -S %s -C %s -n %d -F &","pppoe-relay", wan_if,lan_if, PPPOE_RELAY_MAX_SESSION);
        }
        return 0;

}

int stop_pppoe_relay(int flag)
{
        printf("%s: \n",__func__);
        KILL_APP_ASYNCH("pppoe-relay");

        return 0;
}
#endif

#ifdef IPV6_TUNNEL
int start_tunnel(void)
{
	char *ipv6_wan_proto = nvram_safe_get("ipv6_wan_proto");
	if (strcmp(ipv6_wan_proto,"ipv6_6in4") == 0){
		system("cli ipv6 6in4 start");
		if (nvram_match("ipv6_dhcp_pd_enable","1") == 0){
			system("cli ipv6 dhcp6c start sit1 oset");
	}
	}

	if (strcmp(ipv6_wan_proto,"ipv6_6to4") == 0){
		system("cli ipv6 6to4 start");
		system("cli ipv6 6to4 diagnose");
		system("cli ipv6 dhcp6d start");
		system("cli ipv6 radvd start");
	}

	if (strcmp(ipv6_wan_proto,"ipv6_6rd") == 0 && nvram_match("ipv6_6rd_dhcp", "0") == 0){
		system("cli ipv6 6rd start");
		system("cli ipv6 6rd diagnose");
		system("cli ipv6 dhcp6d start");
		system("cli ipv6 radvd start");
	}
	return 0;
}

int stop_tunnel(void)
{
	char *ipv6_wan_proto = nvram_safe_get("ipv6_wan_proto");
	if (strcmp(ipv6_wan_proto,"ipv6_6in4") == 0){
		system("cli ipv6 6in4 stop");
		if (nvram_match("ipv6_dhcp_pd_enable","1") == 0){
			_system("ip -6 addr flush scope global dev %s", nvram_safe_get("lan_bridge"));
			system("cli ipv6 dhcp6c stop");
		}
	}

	if (strcmp(ipv6_wan_proto,"ipv6_6to4") == 0){
		system("cli ipv6 6to4 stop");
		system("cli ipv6 dhcp6d stop");
		system("cli ipv6 radvd stop");
	}

	if (strcmp(ipv6_wan_proto,"ipv6_6rd") == 0 && nvram_match("ipv6_6rd_dhcp", "0") == 0){
		system("cli ipv6 6rd stop");
		system("cli ipv6 dhcp6d stop");
		system("cli ipv6 radvd stop");
	}
	return 0;
}
#endif

/* individual demand */
struct demand AppServices[] = {
#ifdef MRD_ENABLE
        { "mrd", 1, stop_mrd, start_mrd, NULL},
#endif//#ifdef MRD_ENABLE
#ifdef CONFIG_USER_DROPBEAR
        { "ssh", 1, stop_ssh_server, start_ssh_server, NULL},
#endif
#ifdef CONFIG_IPV6_LLMNR
        { "llmnr", 1, stop_llmnr_server, start_llmnr_server, NULL},
#endif
#ifdef IPV6_TUNNEL
        { "tunnel",1,stop_tunnel,start_tunnel,NULL},
#endif
/*
*       Date: 2009-05-26
*       Name: Jimmy Huang
*       Reason: for lcm collecting info used
*       Notice : package "infod" is within www/model_name/project_apps
*/
#ifdef LCM_FEATURE_INFOD
        { "infod", 1, stop_infod, start_infod, NULL},
#endif
//--------------------------------
#ifdef CONFIG_VLAN_ROUTER
        { "syslogd", 1, stop_syslogd, start_syslogd, NULL},
        { "klogd", 1, stop_klogd, start_klogd, NULL},
        { "tftpd", 1, stop_tftpd, start_tftpd, NULL},
        { "upnpd", 1, stop_upnp, start_upnp, NULL},
        { "dnsmasq", 1, stop_dnsmasq, start_dnsmasq, NULL},
        { "noiptime", 1, stop_ddns, start_ddns, NULL},
//       { "ntptime", 1, stop_ntp, start_ntp, NULL},
        { "maillog", 1, stop_maillog, start_maillog, NULL},
        { "igmpproxy", 1, stop_mcast, start_mcast, NULL},
#ifdef CONFIG_USER_IP_LOOKUP
        { "arpping", 1, stop_arpping, start_arpping, NULL},
#endif
#ifdef CONFIG_USER_SNMP
        { "snmpd", 1, stop_snmpd, start_snmpd, NULL},
#endif
#ifdef CONFIG_USER_NMBD
        { "nmbd", 1, stop_nmbd, start_nmbd, NULL},
#endif
#ifdef CONFIG_USER_WCN
        { "wcnd", 1, stop_wcnd, start_wcnd, NULL},
#endif//#ifdef CONFIG_USER_WCN

//        { "minidlna", 1, stop_dlna, start_dlna, NULL},
/*
*       Date: 2009-4-30
*       Name: Ken_Chiang
*       Reason: modify for support heartbeat feature.
*       Notice :
*/
#ifdef CONFIG_USER_HEARTBEAT
        { "heartbeat", 1, stop_heartbeat, start_heartbeat, NULL},
#endif
/*
*       Date: 2009-12-01
*       Name: Jimmy Huang
*       Reason: Add support for LCD GUI program " lcdshowd "
*       Notice :
*/
#ifdef CONFIG_USER_LCDSHOWD
        { "lcdshowd", 1, stop_lcdshowd, start_lcdshowd, NULL},
#endif
/*
*       Date: 2009-7-14
*       Name: Ken_Chiang
*       Reason: modify for support pppoe-relay feature.
*       Notice :
*/
#ifdef PPPOE_RELAY
        { "pppoe-relay", 1, stop_pppoe_relay, start_pppoe_relay, NULL},
#endif
#else //#ifdef CONFIG_VLAN_ROUTER
        { "tftpd", 1, stop_tftpd, start_tftpd, NULL},
        { "udhcpd", 1, stop_dhcpd, start_dhcpd, NULL},
        { "udhcpc",1, stop_dhcpc, start_dhcpc, NULL},
        { "syslogd", 1, stop_syslogd, start_syslogd, NULL},
        { "klogd", 1, stop_klogd, start_klogd, NULL}
#endif //#ifdef CONFIG_VLAN_ROUTER
#ifdef DHCP_RELAY
        { "dhcp-fwd",1,stop_dhcp_relay,start_dhcp_relay,NULL}
#endif
#ifdef DIR652V
	{ "dhcprelay", 1, stop_dhcp_relay_652V, start_dhcp_relay_652V, NULL}
#endif
	{ "minidlna", 1, stop_dlna, start_dlna, NULL},
};


/* Function Name: start_app
 * Author: Solo Wu
 * Comment:
 *      Start the applications if they are setted. e.g. dhcpd, dnsmasq.
 */
int start_app(void)
{
        struct demand *d;
        char *wan_eth = nvram_safe_get("wan_eth");
        char *wan_speed = nvram_safe_get("wan_port_speed");
        int apps_count = 0;
        int i=0;
	char *ipv6_wan_proto = nvram_safe_get("ipv6_wan_proto");
/*
 * Reason: Let MP smooth.
 * Modified: John Huang
 * Date: 2010.02.09
 */
#ifdef UBICOM_MP_SPEED
        FILE *fp = NULL;
        int mp_testing=0;
#endif

#ifdef MPPPOE
        char *hostname;

        hostname = nvram_safe_get("hostname");
        if (strlen(hostname) < 1)
                hostname = nvram_safe_get("model_number");

        init_file(INTERFACE_INFO);
    save2file(INTERFACE_INFO, "%s/%s/%s/%s/%s", nvram_safe_get("lan_eth"), wan_eth, nvram_safe_get("wlan0_eth"), hostname, nvram_safe_get("wan_proto"));
#endif

        if(!( action_flags.all || action_flags.wan || action_flags.lan || action_flags.app ) )
                return 1;
        DEBUG_MSG("Start APP\n");

/*
 * Reason: Let MP smooth.
 * Modified: John Huang
 * Date: 2010.02.09
 */
#ifdef UBICOM_MP_SPEED
	fp = fopen("/var/tmp/mp_testing.tmp","r");
	if(fp)
	{
		fscanf(fp,"%d",&mp_testing);
		fclose(fp);
	}
	if(mp_testing != 1)
#endif
        /* set wan port speed */
        VCTSetPortSpeed(wan_eth, wan_speed );

        apps_count = sizeof(AppServices) / sizeof(AppServices[0]);
        DEBUG_MSG("Start APP: Apps List Count = %d\n", apps_count);

        for(i=0; i<apps_count; i++)
        {
                d = &AppServices[i];
                DEBUG_MSG("%d/%d: Checking %s to start\n", i+1, apps_count, d->name);
                if( !checkServiceAlivebyName(d->name) )
                        d->start();
        }

#ifdef IPv6_TEST
        if(checkServiceAlivebyName("radvd"))
                nvram_set("ipv6_ra_status","on");
        else
                nvram_set("ipv6_ra_status","off");

    _system("killall -SIGUSR2 httpd");

        //Because network driver has vlan, so at the first time,WAN does not send DADNS.
        //We sent it only once after rebooting
        if(first_send_dadns_flag)
        {
                _system("ifconfig %s down",nvram_safe_get("wan_eth"));
                usleep(500);
                _system("ifconfig %s up",nvram_safe_get("wan_eth"));
                first_send_dadns_flag = 0;
        }
#endif

        return 0;
}

int stop_app(void)
{
        struct demand *d;
        int apps_count=0;
        int i=0;

        /* NickChou, there are one reason to move action_flags.wan
           1. if wan proto change then wan get ip, and send signal to restart app
        if(!( action_flags.all || action_flags.wan || action_flags.lan || action_flags.app ) )
        */
        if(!( action_flags.all || action_flags.lan || action_flags.app ) )
        {
                if(action_flags.wan)
                {
                        /* Nickchou, when wan proto is static, it would not send signal to rc to restart app.*/
                        if(nvram_match("wan_proto", "static"))
                        return 1;
                }
                else
                        return 1;
        }

        DEBUG_MSG("Stop APP\n");

        apps_count = sizeof(AppServices)/ sizeof(AppServices[0]);

        for(i=0; i<apps_count; i++)
        {
                d=&AppServices[i];
                DEBUG_MSG("%d/%d: Stopping %s\n", i+1, apps_count, d->name);
                d->stop( d->reStartbyRc );
        }

        stop_dhcpd(1);//NickChou move, 20080523

#ifdef AR9100
        /*  NickChou 2007.7.16 for AR9100 running too fast,
                so make sure APPs are already fully stopping.
        */
        sleep(1);
#endif

        return 0;
}

