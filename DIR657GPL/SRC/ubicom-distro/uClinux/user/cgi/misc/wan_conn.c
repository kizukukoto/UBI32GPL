#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "ssi.h"
#include "libdb.h"
#include "debug.h"
#include "nvram.h"
#include "sutil.h"
#include "shvar.h"

#define PPP0 "/var/run/ppp0.pid"
#define IPHONE_INIT_RESULT "/var/tmp/iphone"
#define DHCPC_DNS_SCRIPT "/usr/share/udhcpc/default.bound-dns"

struct __wan_conn_struct {
	const char *param;
	const char *desc;
	int (*fn)(int, char *[]);
};

static int dhcp_renew(int argc, char *argv[])
{
	system("killall -SIGUSR1 udhcpc");
	return 0;
}

static int dhcp_release(int argc, char *argv[])
{
	system("killall -SIGUSR2 udhcpc");
	return 0;
}

static int _pppoe_connect(int pppoe_unit)
{
        FILE *fp_pppoe;
        char sn[128], ac[128], cmd[128];
        char *wan_eth=NULL, *service_name=NULL, 
	     *wan_pppoe_connect_mode_00 = NULL , *ac_name=NULL;

	memset(sn, '\0', sizeof(sn));
	memset(ac, '\0', sizeof(ac));
	memset(cmd, '\0', sizeof(cmd));

        wan_pppoe_connect_mode_00 = nvram_safe_get("wan_pppoe_connect_mode_00");
        if (strcmp(wan_pppoe_connect_mode_00, "always_on")) {
                if (strcmp(wan_pppoe_connect_mode_00, "on_demand")) {
                        if ((fp_pppoe = fopen(PPP0, "r")) != NULL) {
				cprintf("PPPoE is already connecting\n");
                                fclose(fp_pppoe);
			} else {
                                wan_eth = nvram_safe_get("wan_eth");
                                service_name = nvram_safe_get("wan_pppoe_service_00");
                                if(strcmp(service_name, "") == 0)
                                        strncpy(sn, "null_sn", strlen("null_sn"));
                                else
                                        strncpy(sn, service_name, strlen(service_name));

                                ac_name = nvram_safe_get("wan_pppoe_ac_00");

                                if(strcmp(ac_name, "") == 0)
                                        strncpy(ac, "null_ac", strlen("null_ac"));
                                else
                                        strncpy(ac, ac_name, strlen(ac_name));

				sprintf(cmd, "/var/sbin/ip-up pppoe %d %s \"%s\" \"%s\" &",
                                        pppoe_unit, wan_eth, parse_special_char(sn), 
					parse_special_char(ac));
				system(cmd);
                        }                
		 } else {
                        wan_eth = nvram_safe_get("wan_eth");
                        service_name = nvram_safe_get("wan_pppoe_service_00");

                        if (strcmp(service_name, "") == 0)
                                strncpy(sn, "null_sn", strlen("null_sn"));
                        else
                                strncpy(sn, service_name, strlen(service_name));

                        ac_name = nvram_safe_get("wan_pppoe_ac_00");

                        if(strcmp(ac_name, "") == 0)
                                strncpy(ac, "null_ac", strlen("null_ac"));
                        else
                                strncpy(ac, ac_name, strlen(ac_name));

			sprintf(cmd, "/var/sbin/ip-up pppoe %d %s \"%s\" \"%s\" &",
				pppoe_unit, wan_eth, parse_special_char(sn), parse_special_char(ac));
			system(cmd);
                }
        }
	return 0;
}

static int pppoe_connect(int argc, char *argv[])
{
        int count = 0;
	char ppp6_ip[64]={};
	_pppoe_connect(0);
	while (count < 8) {
		if(wan_connected_cofirm("ppp0"))
			break;
		else {
			sleep(1);
			count++;
		}
	}
	while (count < 18 && NVRAM_MATCH("ipv6_wan_proto","ipv6_autodetect")) {
		if(read_ipv6addr("ppp0", SCOPE_GLOBAL, ppp6_ip, sizeof(ppp6_ip)))
			break;
		else {
			sleep(1);
			count++;
		}
		if(count == 18){
			cprintf("IPv6 PPPoE share with IPv4 is Failed, Change to Autoconfiguration Mode\n");
			system("cli ipv6 rdnssd stop");
			system("cli ipv6 autoconfig stop");
			_system("cli ipv6 rdnssd start %s", nvram_safe_get("wan_eth"));
		}
	}
	cprintf("httpd wait %d Secs for PPPoE dial\n", count);
	return 0;
}

static int pppoe_disconnect(int argc, char *argv[])
{
	char cmd[128];
	sprintf(cmd, "/var/sbin/ip-down pppoe 0 &");
	system(cmd);
	cprintf("wait 3 Secs for PPPoE Disconnect\n");
	sleep(3);
	return 0;
}

static int pptp_connect(int argc, char *argv[])
{ 
        int count = 0;
	char *wan_pptp_connect_mode = nvram_safe_get("wan_pptp_connect_mode");
	if (strcmp(wan_pptp_connect_mode, "always_on") != 0)
		system("/var/sbin/ip-up pptp &");
	while(count < 8) {
		if(wan_connected_cofirm("ppp0"))
			break;
		else {
			sleep(1);
			count++;
		}
	}
	cprintf("httpd wait %d Secs for PPTP dial\n", count);
	return 0;
}

static int pptp_disconnect(int argc, char *argv[])
{
        system("/var/sbin/ip-down pptp &");
        cprintf("wait 3 Secs for PPTP Disconnect\n");
        sleep(3);
	return 0;
}

static int l2tp_connect(int argc, char *argv[])
{
	char *wan_l2tp_connect_mode = nvram_safe_get("wan_l2tp_connect_mode");
        int count=0;

	if (strcmp(wan_l2tp_connect_mode, "always_on") != 0)
		system("/var/sbin/ip-up l2tp &");
	
	while(count < 8) {
		if(wan_connected_cofirm("ppp0"))
			break;
		else {
			sleep(1);
			count++;
		}
	}
	cprintf("httpd wait %d Secs for L2TP dial\n", count);
	return 0;
}

static int l2tp_disconnect(int argc, char *argv[])
{
	system("/var/sbin/ip-down l2tp &");
        cprintf("wait 3 Secs for L2TP Disconnect\n");
        sleep(3);
	return 0;
}

static int _usb3g_connect(int pppoe_unit)
{
        char sn[128];
        char *wan_eth, *usb3g_reconnect_mode;
        FILE *fp_pppoe;

	memset(sn, '\0', sizeof(sn));

        usb3g_reconnect_mode = nvram_safe_get("usb3g_reconnect_mode");
        strncpy(sn, "null_sn", strlen("null_sn"));

        if ((strlen(usb3g_reconnect_mode) > 0) && 
		(strcmp(usb3g_reconnect_mode,"always_on")!=0)) {
                if (strcmp(usb3g_reconnect_mode, "on_demand") != 0) {
                        //Manual
                        if ((fp_pppoe = fopen(PPP0, "r")) == NULL) {
                                wan_eth = nvram_safe_get("wan_eth");
                                system("/var/sbin/ip-up usb3g &");
                        } else {
                                cprintf("3G USB is already connecting\n");
                                fclose(fp_pppoe);
                        }
                } else {
                        //On demand
                        wan_eth = nvram_safe_get("wan_eth");
                        strncpy(sn, "null_sn", strlen("null_sn"));
                        system("/var/sbin/ip-up usb3g &");
                }
        }
        return 0;
}

static int usb3g_connect(int argc, char *argv[])
{
	_usb3g_connect(0);
	return 0;
}

static int _usb3g_disconnect(int pppoe_unit)
{
	char cmd[128];
        sprintf(cmd, "/var/sbin/ip-down usb3g %d &", pppoe_unit);
	system(cmd);
        return 0;
}

static int usb3g_disconnect(int argc, char *argv[])
{
	_usb3g_disconnect(0);
	return 0;
}

static int init_iphone(void)
{
        FILE *fp;
	char cmd[128];

        memset(cmd,'\0',sizeof(cmd));
        unlink(IPHONE_INIT_RESULT);
        sprintf(cmd,"[ -n \"`iphone | grep Success`\" ] && echo 1 > %s",
		IPHONE_INIT_RESULT);
        system(cmd);

        if ((fp = fopen(IPHONE_INIT_RESULT,"r")) != NULL) {
                fclose(fp);
                return 1;
        }
        return 0;
}


static void dhcpc_kill_renew(char *interface)
{
        int i = 0;
	char cmd[128];

        memset(cmd,'\0',sizeof(cmd));

        system("[ -n \"`ps | grep udhcpc`\" ] && killall udhcpc");

        if(strcmp(interface,"rndis0") == 0){
                system("[ -n \"`ifconfig | grep rndis0`\" ] || ifconfig rndis0 up");
        } else if(strcmp(interface,"iph0") == 0){
                // other mobile phone will bring different interface
                if(init_iphone() == 1){
                        cprintf("%s (%d): init iphone Success\n", __func__, __LINE__);
                }else{
                        cprintf("%s (%d): init iphone failed, try again\n", __func__, __LINE__);
                        init_iphone();
                }
                //sleep(1); // wait iphone to send init request to iphone device
                cprintf(" %s (%d): bring up interface %s\n", __func__, __LINE__, interface);
                // if interface rndis0 is not up, bring it up
                system("[ -n \"`ifconfig | grep iph0`\" ] || ifconfig iph0 up");
        } else {
		cprintf("No interface match!\n");
	}

        sprintf(cmd,"udhcpc -i %s -s %s &", interface, DHCPC_DNS_SCRIPT);
        system(cmd);
}

static int usb3g_phone_connect(int argc, char *argv[])
{
	dhcpc_kill_renew(nvram_safe_get("wan_eth"));
	return 0;
}

static int usb3g_phone_disconnect(int argc, char *argv[])
{
	char *usb_type;
	system("killall udhcpc 2>&1 > /dev/null");
	usb_type = nvram_safe_get("usb_type");
/* CONFIG_3G_USB_CLIENT_WM5 */
	if (strcmp(usb_type, "3") == 0 || strcmp(usb_type, "5") == 0) {
		system("ifconfig rndis0 0.0.0.0");
		system("ifconfig rndis0 down");
	}
/* CONFIG_3G_USB_CLIENT_IPHONE */
	if (strcmp(usb_type, "4") == 0) {
		system("ifconfig iph0 0.0.0.0");
		system("ifconfig iph0 down");
		if(init_iphone() != 0) {
			init_iphone();
		}
	}
	return 0;
}

static int misc_wan_conn_help(struct __wan_conn_struct *p)
{
	cprintf("Usage:\n");
	for (; p && p->param; p++)
		cprintf("\t%s: %s\n", p->param, p->desc);

	cprintf("\n");
	return 0;
}

int wan_conn_main(int argc, char *argv[])
{
	cprintf("%s(%d)\n", __func__, __LINE__);
	int ret = -1;
	struct __wan_conn_struct *p, list[] = {
		{ "dhcp_renew", "DHCP Renew" , dhcp_renew},
		{ "dhcp_release", "DHCP Release" , dhcp_release},
		{ "pppoe_00_connect", "PPPOE Connect" , pppoe_connect},
		{ "pppoe_00_disconnect", "PPPOE Disconnect" , pppoe_disconnect},
		{ "pptp_connect", "PPTP Connect" , pptp_connect},
		{ "pptp_disconnect", "PPTP Disconnect" , pptp_disconnect},
		{ "l2tp_connect", "L2TP Connect" , l2tp_connect},
		{ "l2tp_disconnect", "L2TP Disconnect" , l2tp_disconnect},
		{ "usb3g_init_connect", "USB3G Connect" , usb3g_connect},
		{ "usb3g_disconnect", "USB3G Disconnect" , usb3g_disconnect},
		{ "usb3g_phone_connect", "3G Phone Connect" , usb3g_phone_connect},
		{ "usb3g_phone_disconnect", "3G Phone Disconnect" , usb3g_phone_disconnect},
		{ NULL, NULL, NULL }
	};

	if (argc == 1 || strcmp(argv[1], "help") == 0)
		goto help;

	for (p = list; p && p->param; p++) {
		if (strcmp(argv[1], p->param) == 0) {
			ret = p->fn(argc - 1, argv + 1);
			goto out;
		}
	}
help:
	ret = misc_wan_conn_help(list);
out:
	return ret;
}
