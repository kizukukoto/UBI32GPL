/*
 * Router rc control script
 *
 * $Id: rc.c,v 1.82 2009/08/17 09:07:03 ken_chiang Exp $
 */

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>
#include <syslog.h>
#include <signal.h>
#include <string.h>
#include <sys/klog.h>
#include <sys/types.h>
#include <sys/mount.h>
#include <sys/reboot.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>
#include <sys/time.h>
#include <sys/utsname.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <net/if_arp.h>
#include <dirent.h>

#include <build.h>
#include <rc.h>
#include <sutil.h>
#include <nvram.h>
#include <shvar.h>
#include <fcntl.h>
#include <sys/ipc.h>
#include <sys/shm.h>

//#define RC_DEBUG
#ifdef RC_DEBUG
#define DEBUG_MSG(fmt, arg...)       printf("%s [%d]: "fmt, __FILE__, __LINE__, ##arg)
#else
#define DEBUG_MSG(fmt, arg...)
#endif

struct action_flag action_flags;

static void sysinit(void);
static void rc_signal(int sig);
static int wlan0_domain_flag = 0;

#ifdef IPv6_SUPPORT
int inet6_br0_status=0;
int inet6_eth1_status=0;
#endif //IPv6_SUPPORT

#ifdef CONFIG_USER_TC
#define RESET_ALL       action_flags.all = action_flags.lan = action_flags.wan = action_flags.wlan = action_flags.app = action_flags.firewall = action_flags.lan_app = action_flags.qos = 0
#define QOS_SET                 RESET_ALL;action_flags.qos=1
#define ONLY_QOS_SET    action_flags.qos=1
#else
#define RESET_ALL       action_flags.all = action_flags.lan = action_flags.wan = action_flags.wlan = action_flags.app = action_flags.firewall = action_flags.lan_app = 0
#endif

#define APP_SET                 RESET_ALL;action_flags.app =1
#define FIREWALL_SET    RESET_ALL;action_flags.firewall=1
#define WAN_SET                 RESET_ALL;action_flags.wan=1;action_flags.lan_app =1
#define LAN_SET                 RESET_ALL;action_flags.lan=1
#define WLAN_SET                RESET_ALL;action_flags.wlan=1
#define ALL_SET                 RESET_ALL;action_flags.all=1
#define STOP_SET                RESET_ALL;action_flags.wan=1;action_flags.app =1;action_flags.firewall=1
#define AP_MODE_SET             RESET_ALL;action_flags.app =1;action_flags.lan=1
#define LAN_APP_SET             action_flags.lan_app =1
#define ONLY_WLAN_SET   action_flags.wlan=1

/* States */
enum {
        RESTART,
        STOP,
        START,
        IDLE,
        UPGRADE_STOP,
};
static int state = START;
static int signalled = -1;
//char http_bind_ipaddr[16]= "192.168.0.1";
extern const char VER_FIRMWARE[];

#define key IPC_PRIVATE

int alloc_shm(void)
{
    int shmid;
    struct shmid_ds buf;
    FILE *shmfile;

    shmid = shmget(IPC_PRIVATE, FW_BUF_SIZE+0x10000, IPC_CREAT | 0600);
    shmctl(shmid, IPC_STAT, &buf);
    DEBUG_MSG("shmid = %d\n", shmid);
#if 0
    shmfile = fopen("var/etc/shm_id", "w");
    if (shmfile) {
                fprintf(shmfile, "%d\n", shmid);
                fclose(shmfile);
     }
#endif
    return 0;
}

int create_pid(void)
{
        FILE *pidfile;
        // Create PID
        pidfile = fopen(RC_PID, "w");
        if (pidfile) {
                fprintf(pidfile, "%lu\n", (unsigned long) getpid());
                fclose(pidfile);
        }
        return 0;
}

void set_flag(char* current_state)
{
        FILE *fp = NULL;

        if(!strncmp(current_state,"b",1))
                printf("rc is BUSY now!\n");
        else
                printf("rc is IDLE now!\n");

        fp = fopen(RC_FLAG_FILE,"w");
        if(fp)
        {
                fwrite(current_state,1,strlen(current_state),fp);
                fclose(fp);
        }
        else
                printf("Open %s failed\n",RC_FLAG_FILE);
}


int mount_fs(void)
{
        mkdir("/tmp/var", 0777);
        mkdir("/var/log", 0777);
        mkdir("/var/run", 0777);
        mkdir("/var/tmp", 0777);
        mkdir("/var/misc", 0777);
        mkdir("/var/etc", 0777);
/*
*       Date: 2009-1-19
*       Name: Ken_Chiang
*       Reason: modify for 3g usb client card to used.
*       Notice :
*/
#ifdef CONFIG_USER_3G_USB_CLIENT
        mkdir("/var/etc/ppp", 0777);
        mkdir("/var/etc/ppp/peers", 0777);
        mkdir("/var/lock", 0777);
        mkdir("/var/etc/peers", 0777);
#endif
        mkdir("/var/sbin", 0777);
        mkdir("/var/firm", 0777);
#ifdef MPPPOE
        mkdir("/var/iproute2", 0777);
#endif
#ifdef CONFIG_USER_WAN_8021X
        mkdir("/var/etc/cert", 0777);
#endif
        return 0;
}

void create_symlink(void)
{
#ifdef CONFIG_VLAN_ROUTER
        /* ipup_main */
        symlink("/sbin/rc", "/var/sbin/ip-up");
        /* ipdown_main */
        symlink("/sbin/rc", "/var/sbin/ip-down");
        /* monitor_main */
        symlink("/sbin/rc", "/var/sbin/monitor");
	/* ipv6_monitor_main */
	symlink("/sbin/rc", "/var/sbin/mon6");
	/* redial_main */
        symlink("/sbin/rc", "/var/sbin/redial");
        /* ipv6_redial_main */
        symlink("/sbin/rc", "/var/sbin/red6");
#ifdef MPPPOE
        /* ppptimer_main */
        symlink("/sbin/rc", "/var/sbin/ppptimer");
#endif
        /* process_monitor_main */
        symlink("/sbin/rc", "/var/sbin/psmon");
        /* wan_port_connection_time_monitor_main */
        symlink("/sbin/rc", "/var/sbin/wantimer");
#else
        symlink("/sbin/rc", "/var/sbin/lanmon");
#endif
        symlink("/sbin/rc", "/var/sbin/fwupgrade");
#ifdef MPPPOE
        tempnam("/var/iproute2", "ip");
    symlink("/var/iproute2", "/etc/iproute2");
#endif
/* jimmy marked 20080603 */
#ifdef CONFIG_USER_TC
//      /* measure bandwidth */
//      symlink("/sbin/rc", "/var/sbin/mbandwidth");
#endif
/* -------------------------- */
}

static void sysinit(void)
{
        mount_fs();
        //alloc_shm();
        create_pid();
        create_symlink();
}

/* Signal handling */
static void rc_signal(int sig)
{
/*
 * Reason: Let MP smooth.
 * Modified: John Huang
 * Date: 2010.02.09
 */
#ifdef UBICOM_MP_SPEED
        FILE *fp = NULL;
        int mp_testing=0;
#endif
        if (state == IDLE) {
                switch (sig) {
                        case SIGHUP:
                                DEBUG_MSG("signalling RESTART\n");
/*
 * Reason: Let MP smooth.
 * Modified: John Huang
 * Date: 2010.02.09
 */
#if 0 //def UBICOM_MP_SPEED
        			fp = fopen("/var/tmp/mp_testing.tmp","r");
        			if(fp)
        			{
        				fscanf(fp,"%d",&mp_testing);
        				fclose(fp);
        			}
        			if(mp_testing == 1) start_get_wps_default_pin();
#endif
                                ALL_SET;
                                signalled = RESTART;
#ifdef CONFIG_USER_WISH
                                system("/etc/init.d/stop_wish &");
#endif
                                break;
                        case SIGUSR2:
                                DEBUG_MSG("signalling START\n");
                                ALL_SET;
                                signalled = START;
                                break;
                        case SIGINT:
                                DEBUG_MSG("signalling STOP\n");
                                ALL_SET;
                                signalled = STOP;
                                break;
                        case SIGALRM:
                                DEBUG_MSG  ("signalling TIMER\n");
                                STOP_SET;
                                signalled = STOP;
                                break;
                                /* re-run specify part */
#ifdef CONFIG_VLAN_ROUTER
                        case SIGUSR1:
                                DEBUG_MSG  ("signalling RESTART WAN\n");
                                WAN_SET;
                                signalled = RESTART;
                                break;
#endif
                        case SIGQUIT:
                                DEBUG_MSG  ("signalling RESTART LAN\n");
                                LAN_SET;
                                signalled = RESTART;
                                break;
                        case SIGTSTP:
                                DEBUG_MSG  ("signalling RESTART WLAN\n");
                                WLAN_SET;
                                signalled = RESTART;
#ifdef CONFIG_USER_WISH
                                system("/etc/init.d/stop_wish &");
#endif
                                break;
                        case SIGILL:
                                DEBUG_MSG  ("signalling RESTART WLAN+APP\n");
#ifdef  CONFIG_BRIDGE_MODE
                                if(nvram_match("wlan0_mode", "ap") == 0)
                                {
                                        printf("rc_sinal: RESTART APP : %s\n",
                                                get_ipaddr(nvram_safe_get("lan_bridge")) );
                                        AP_MODE_SET;
                                }
                                else if(nvram_match("wlan0_mode", "rt") == 0)
#endif
                                {
                                        APP_SET;
#ifdef CONFIG_USER_TC
                                        ONLY_QOS_SET;
#endif
                                }
                                ONLY_WLAN_SET;
                                signalled = RESTART;
#ifdef CONFIG_USER_WISH
                                system("/etc/init.d/stop_wish &");
#endif
                                break;
#ifdef CONFIG_VLAN_ROUTER
                        case SIGSYS:
                                DEBUG_MSG  ("signalling RESTART FIREWALL\n");
                                FIREWALL_SET;
/*
 *      Date: 2009-10-2
 *      Name: Gareth Williams <gareth.williams@ubicom.com>
 *      Reason: Fix streamengine not restarting on firewall state change
 */
#ifdef CONFIG_USER_STREAMENGINE
#ifdef CONFIG_USER_TC
                                ONLY_QOS_SET;
#endif
#endif
                                signalled = RESTART;
                                break;
#endif

#ifdef CONFIG_USER_TC
                        case SIGTTIN:
                                DEBUG_MSG  ("signalling SIGTTIN RESTART QOS\n");
                                QOS_SET;
                                signalled = RESTART;
                                break;
#endif

                        case SIGTERM:
                                DEBUG_MSG  ("signalling RESTART APP\n");

#ifdef  CONFIG_BRIDGE_MODE
                                if(nvram_match("wlan0_mode", "ap") == 0)
                                {
                                        printf("rc_sinal: RESTART APP : %s\n",
                                                get_ipaddr(nvram_safe_get("lan_bridge")) );
                                        AP_MODE_SET;
                                }
                                else if(nvram_match("wlan0_mode", "rt") == 0)
#endif
                                {
                                APP_SET;
#ifdef CONFIG_USER_TC
                                        ONLY_QOS_SET;
#endif
#ifdef CONFIG_USER_WISH
                                        system("/etc/init.d/stop_wish &");
#endif

                                }

                                signalled = RESTART;
                                break;
                        case SIGPIPE:
                                DEBUG_MSG  ("signalling signalling RESTART APP, not include DHCPD\n");
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
        			if(mp_testing == 1) break;
#endif
#ifdef  CONFIG_BRIDGE_MODE
                                if(nvram_match("wlan0_mode", "ap") == 0)
                                {
                                        printf("rc_sinal: RESTART APP : %s\n",
                                                get_ipaddr(nvram_safe_get("lan_bridge")) );
                                        AP_MODE_SET;
                                }
                                else if(nvram_match("wlan0_mode", "rt") == 0)
#endif
                                {
                                APP_SET;
                                        LAN_APP_SET;
#ifdef CONFIG_USER_TC
                                        ONLY_QOS_SET;
#endif
                                }
                                signalled = RESTART;
                                break;
                }
        }
}

int init_nvram(void)
{

        FILE *fp;
        int mtd_block;
        int count = 0;
        char *file_buffer;
        char *eof_tok;
#ifdef UBICOM_JFFS2
	int fp_1;
#endif

#ifdef CONFIG_USER_LINUX_NVRAM
	_system("cli sys init nvram_check");
#else
#ifdef UBICOM_JFFS2
	if ((fp_1=open(SYS_CONF_MTDBLK, O_RDONLY)) < 0) {
		printf("mtd block %s open fail \n",SYS_CONF_MTDBLK);
	}
	else
	{
		close(fp_1);
		system("cp -rf /etc/nvram /var/etc/nvram.conf");
	}
#else
        file_buffer = malloc(CONF_BUF); //30K
        if(file_buffer==NULL)
                printf("init_nvram: malloc fail\n");
	else
	{
        	mtd_block = open(SYS_CONF_MTDBLK, O_RDONLY);

        	if(mtd_block < 0 )
                	printf("mtd block %s open fail \n",SYS_CONF_MTDBLK);
		else
		{
        		count = read(mtd_block,file_buffer,CONF_BUF);
        		if(count <= 0)
                		printf("read from %s fail\n",SYS_CONF_MTDBLK);
        		else
        		{
                		DEBUG_MSG("%02d data read from %s success\n",count,SYS_CONF_MTDBLK);

        			// eof_tok=index(file_buffer, '~');
        			eof_tok=strstr(file_buffer, EOF_NVRAM);

        			fp =fopen (NVRAM_FILE, "w+");
        			if(fp==NULL)
                			printf("init_nvram: open NVRAM_FILE fail\n");
				else
				{
        				fwrite(file_buffer, 1, CONF_BUF, fp);
        				fclose(fp);
        			}
        		}
        		close(mtd_block);
        	}
        	free(file_buffer);
        }
#endif
        nvram_upgrade();

#endif //CONFIG_USER_LINUX_NVRAM
        return 0;
}

void init_network(void)
{

        char *lan_eth=nvram_safe_get("lan_eth");
        char *wan_eth=nvram_safe_get("wan_eth");
        char *lan_bridge=nvram_safe_get("lan_bridge");
        char *nvram_lan_mac=nvram_safe_get("lan_mac");
        char *nvram_wan_mac=nvram_safe_get("wan_mac");

#ifdef ATHEROS_11N_DRIVER
// Atheros Nic driver bugs, if we turn on eth1 early than eth0
// then ths driver will be crash.
/*#ifdef SUPPORT_IGMP_SNOOPY
        printf("SUPPORT_IGMP_SNOOPY\n");
        if( !nvram_match("multicast_stream_enable", "1")  ){
                printf("multicast_stream_enable\n");
*/
/*
*       Date: 2009-8-17
*       Name: Ken_Chiang
*       Reason: modify for fixed when enable igmp snopping the lan unicast will be flooding to all port issue.
*       Notice : we can used SET_GW_MAC to added the gw mac address to cpu port to fix the issue
*            or used the enable cpu port learning to fixed the issue.
*/
/* We don`t have this module, ag7100_mod.ko.
#ifdef SET_GW_MAC
#ifdef SET_GET_FROM_ARTBLOCK
        printf("SET_GW_MAC\n");
        if(artblock_get("lan_mac"))
                _system("insmod /lib/modules/2.6.15/net/ag7100_mod.ko wan_speed=%s enable_igmp_snoopy=1 gw_addr=%s", nvram_safe_get("wan_port_speed"), artblock_safe_get("lan_mac"));
        else
                _system("insmod /lib/modules/2.6.15/net/ag7100_mod.ko wan_speed=%s enable_igmp_snoopy=1 gw_addr=%s", nvram_safe_get("wan_port_speed"),nvram_lan_mac);
#else
                _system("insmod /lib/modules/2.6.15/net/ag7100_mod.ko wan_speed=%s enable_igmp_snoopy=1 gw_addr=%s", nvram_safe_get("wan_port_speed"),nvram_lan_mac);
#endif
#else//SET_GW_MAC
                _system("insmod /lib/modules/2.6.15/net/ag7100_mod.ko wan_speed=%s enable_igmp_snoopy=1" , nvram_safe_get("wan_port_speed"));
#endif//SET_GW_MAC
        }
        else{
                printf("multicast_stream_disable\n");
                _system("insmod /lib/modules/2.6.15/net/ag7100_mod.ko wan_speed=%s", nvram_safe_get("wan_port_speed"));
        }
#else
        printf("NO SUPPORT_IGMP_SNOOPY\n");
        _system("insmod /lib/modules/2.6.15/net/ag7100_mod.ko wan_speed=%s", nvram_safe_get("wan_port_speed"));
#endif
*/
        /* Only brctl */
#ifdef SET_GET_FROM_ARTBLOCK
        char *art_lan_mac=artblock_get("lan_mac");
        if(art_lan_mac != NULL) {
                ifconfig_mac( lan_eth, art_lan_mac );
                nvram_set("lan_mac", art_lan_mac); //Set LAN MAC at nvram the same as in artblock.
        } else {
                ifconfig_mac( lan_eth, nvram_lan_mac );
        }

        ifconfig_up( lan_eth, "0.0.0.0", "0.0.0.0");

#ifdef CONFIG_VLAN_ROUTER
        char *art_wan_mac=artblock_get("wan_mac");
        if(art_wan_mac != NULL) {
                ifconfig_mac( wan_eth, art_wan_mac );
                nvram_set("wan_mac", art_wan_mac); //Set WAN MAC at nvram the same as in artblock.
        } else {
                ifconfig_mac( wan_eth, nvram_wan_mac );
        }

        ifconfig_up( wan_eth, "0.0.0.0", "0.0.0.0");
#endif
#else//SET_GET_FROM_ARTBLOCK

        ifconfig_mac( lan_eth, nvram_lan_mac );
        ifconfig_up( lan_eth, "0.0.0.0", "0.0.0.0");

#ifdef CONFIG_VLAN_ROUTER
        ifconfig_mac( wan_eth, nvram_wan_mac );
        ifconfig_up( wan_eth, "0.0.0.0", "0.0.0.0");
#endif

#endif//SET_GET_FROM_ARTBLOCK

#else //ATHEROS_11N_DRIVER

        /* Only brctl */
#ifdef SET_GET_FROM_ARTBLOCK
#ifdef CONFIG_VLAN_ROUTER
        if(artblock_get("wan_mac"))
                ifconfig_mac( wan_eth, artblock_safe_get("wan_mac") );
        else
                ifconfig_mac( wan_eth, nvram_wan_mac );
        ifconfig_up( wan_eth, "0.0.0.0", "0.0.0.0");
#endif

        if(artblock_get("lan_mac"))
                ifconfig_mac( lan_eth, artblock_safe_get("lan_mac") );
        else
                ifconfig_mac( lan_eth, nvram_lan_mac );

        ifconfig_up( lan_eth, "0.0.0.0", "0.0.0.0");
#else//SET_GET_FROM_ARTBLOCK

#ifdef CONFIG_VLAN_ROUTER
        ifconfig_mac( wan_eth, nvram_wan_mac );
        ifconfig_up( wan_eth, "0.0.0.0", "0.0.0.0");
#endif

        ifconfig_mac( lan_eth, nvram_lan_mac );
        ifconfig_up( lan_eth, "0.0.0.0", "0.0.0.0");

#endif//SET_GET_FROM_ARTBLOCK
#endif //ATHEROS_11N_DRIVER

#ifdef CONFIG_VLAN_ROUTER
        printf("CONFIG_VLAN_ROUTER defined!!!");
#endif

#ifndef IPv6_TEST
        _system("brctl addbr %s", lan_bridge );
        _system("brctl stp %s 0", lan_bridge );
        _system("brctl setfd %s 1", lan_bridge);
        _system("brctl addif %s %s", lan_bridge, lan_eth);
        ifconfig_up(lan_bridge, nvram_safe_get("lan_ipaddr"), nvram_safe_get("lan_netmask")  );
#endif

//#endif

}

int check_wlan0_domain(int flag)
{
        if(flag == 2)
                return wlan0_domain_flag;
        else
                wlan0_domain_flag = flag;
}

/*
*       Date: 2009-5-20
*       Name: Ken_Chiang
*       Reason: modify for New WPS algorithm.
*       Notice :
*
*       Date: 2009-7-3
*       Name: Jerry_Shyu
*       Modify: 1. Use NVRAM entry "wps_pin_code_gen_interface"
*                  to select the interface to generate PIN code.
*/
void generate_pin_by_wan_mac(char *wps_pin)
{
        unsigned long int rnumber = 0;
        unsigned long int pin1 = 0,pin2 = 0;
        unsigned long int accum = 0;
        int i,digit = 0;
        char wan_mac[32]={0};
        char *mac_ptr = NULL;
        char *pin_if=NULL; //John 2010.04.23 remove unused nvram_get("wps_pin_code_gen_interface");

        //printf("%s:\n",__func__);
        //get mac
#ifdef SET_GET_FROM_ARTBLOCK
        if (!pin_if) {
                if(artblock_get("wan_mac"))
                        mac_ptr = artblock_safe_get("wan_mac");
                else
                        mac_ptr = nvram_safe_get("wan_mac");
        }else {
                if (0==nvram_match("wan_eth", pin_if)) {
                        if(artblock_get("wan_mac"))
                                mac_ptr = artblock_safe_get("wan_mac");
                        else
                                mac_ptr = nvram_safe_get("wan_mac");
                }else { //if (0==nvram_match("lan_eth", pin_if)) {
                        if(artblock_get("lan_mac"))
                                mac_ptr = artblock_safe_get("lan_mac");
                        else
                                mac_ptr = nvram_safe_get("lan_mac");
                }
        }
#else
        if (!pin_if) {
                mac_ptr = nvram_safe_get("wan_mac");
        }else {
                if (0==nvram_match("wan_eth", pin_if))
                        mac_ptr = nvram_safe_get("wan_mac");
                else //if (0==nvram_match("lan_eth", pin_if))
                        mac_ptr = nvram_safe_get("lan_mac");
        }
#endif
        //printf("%s:mac_ptr=%s\n",__func__,mac_ptr);
        strncpy(wan_mac, mac_ptr, 18);
        mac_ptr = wan_mac;
        //remove ':'
        for(i =0 ; i< 5; i++ )
        {
                memmove(wan_mac+2+(i*2), wan_mac+3+(i*3), 2);
        }
        memset(wan_mac+12, 0, strlen(wan_mac)-12);
        //printf("%s:wan_mac=%x%x%x%x%x%x%x%x%x%x%x%x\n",__func__,
                        //wan_mac[0],wan_mac[1],wan_mac[2],wan_mac[3],wan_mac[4],wan_mac[5],
                        //wan_mac[6],wan_mac[7],wan_mac[8],wan_mac[9],wan_mac[10],wan_mac[11]);
        sscanf(wan_mac,"%06X%06X",&pin1,&pin2);

        pin2 ^= 0x55aa55;
        pin1 = pin2 & 0x00000F;
        pin1 = (pin1<<4) + (pin1<<8) + (pin1<<12) + (pin1<<16) + (pin1<<20) ;
        pin2 ^= pin1;
        pin2 = pin2 % 10000000; // 10^7 to get 7 number

        if( pin2 < 1000000 )
                pin2 = pin2 + (1000000*((pin2%9)+1)) ;

        //ComputeChecksum
        rnumber = pin2*10;
        accum += 3 * ((rnumber / 10000000) % 10);
        accum += 1 * ((rnumber / 1000000) % 10);
        accum += 3 * ((rnumber / 100000) % 10);
        accum += 1 * ((rnumber / 10000) % 10);
        accum += 3 * ((rnumber / 1000) % 10);
        accum += 1 * ((rnumber / 100) % 10);
        accum += 3 * ((rnumber / 10) % 10);
        digit = (accum % 10);
        pin1 = (10 - digit) % 10;

        pin2 = pin2 * 10;
        pin2 = pin2 + pin1;
        sprintf( wps_pin, "%08d", pin2 );
        //printf("%s:wps_pin=%s\n",__func__,wps_pin);
        return;
}

start_get_wps_default_pin(void)
{
        if((nvram_match("wps_pin", "00000000") == 0) && (nvram_match("wps_default_pin", "00000000") == 0)){
                char wps_pin[10];
                printf("wps_pin==00000000 and wps_default_pin==00000000\n");
                generate_pin_by_wan_mac(wps_pin);
                nvram_set("wps_pin",wps_pin);
                nvram_set("wps_default_pin",wps_pin);
        }
}

static int updown_httpd(int up)
{

	if (up){
/* maybe use httpd build-in busybox */
#if (CONFIG_USER_HTTPD_HTTPD == 1)
		system("httpd &");
#else
		//system("cgi services httpd start");
		system("cli sys account");
		system("cli services https stop");
		system("cli services https start");
		system("cli security vpn sslvpn stop");
		system("cli security vpn sslvpn start");
		system("/bin/httpd -h /www -p 80");
		if(strcmp(nvram_safe_get("sys_lang_en"), "none") != 0)
			system("lang init");
#endif
	} else {
		printf("unknow how to stop httpd yet. need?\n");
	}
	return 0;
}

#ifdef NVRAM_CONF_VER_CTL
extern const char CONF_VERSION[];
void configuration_version_control(void)
{
	char nv_cv[8]={0}, cv[8]={0};
	char *nv_cv_major=NULL, *nv_cv_minor=NULL;
	char *cv_major=NULL, *cv_minor=NULL;

	strcpy(nv_cv, nvram_safe_get("configuration_version"));
	strcpy(cv, nvram_default_search("configuration_version"));
	printf("configuration_version=%s in NVRAM\n", nv_cv);
	printf("configuration_version=%s in System\n", cv);

	nv_cv_major = strtok(nv_cv, ".");
	nv_cv_minor = strtok(NULL, ".");
	cv_major = strtok(cv, ".");
	cv_minor = strtok(NULL, ".");
		
	if((nv_cv_major!=NULL) && (nv_cv_minor!=NULL) && (cv_major!=NULL) && (cv_minor!=NULL))
	{
		if(atoi(cv_major) > atoi(nv_cv_major)){
			/* Udate nvram deault value, item was already in NVRAM
			ex. change wlan0_ssid=a to wlan0_ssid=b in nvram.default
			*/	
			nvram_restore_default();
			nvram_flag_reset();
			printf("Update configuration_version=%s in NVRAM and restore default\n"
				, nvram_safe_get("configuration_version"));
		}			
	}
}
#endif //NVRAM_CONF_VER_CTL

int service_init(void)
{
        char *hostname, *system_time, *syslog_server;
        char tmp_hostname[40];
        char *syslog_enable, *syslog_ip;
        /* jimmy added 20080822, in case syslog server IP miss */
        char tmp_syslog_server[20]={0};
	int year = 2006,month = 4, date = 12, hour = 12, min = 24, sec = 12;
        /* --------------------------------------------------- */

        system("cp -f /etc/nvram.default /var/etc");

        DEBUG_MSG("rc:service_init:init_nvram:start\n");
        init_nvram();
        DEBUG_MSG("rc:service_init:init_nvram:end\n");
	
#if 0 //def NVRAM_CONF_VER_CTL	
	configuration_version_control();
#endif	

/*
*       Date: 2009-06-12
*       Name: Albert Chen
*       Reason: for lcm module
*       Notice : package "keypad.ko" is within www/model_name/project_apps
*/
#ifdef LCM_FEATURE_INFOD
        system("insmod /lib/modules/2.6.15/io/keypad.ko");
#endif

#ifdef IPv6_TEST
                        system("echo 0 > /proc/sys/net/ipv6/conf/all/router_solicitations");
                  system("echo 0 > /proc/sys/net/ipv6/conf/eth0.2/router_solicitations");
                  system("echo 0 > /proc/sys/net/ipv6/conf/eth0.1/router_solicitations");
                  system("echo 0 > /proc/sys/net/ipv6/conf/br0/router_solicitations");
                  system("echo 1 > /proc/sys/net/ipv6/conf/all/forwarding");
                        system("echo 1 > /proc/sys/net/ipv6/conf/all/mc_forwarding");
                        system("echo 1 > /proc/sys/net/ipv6/conf/eth0.2/mc_forwarding");
                        system("echo 1 > /proc/sys/net/ipv6/conf/eth0.1/mc_forwarding");
                        system("echo 1 > /proc/sys/net/ipv6/conf/br0/mc_forwarding");
                        system("echo 0 > /proc/sys/net/ipv6/conf/all/accept_ra");
                  system("echo 0 > /proc/sys/net/ipv6/conf/eth0.2/accept_ra");
                  system("echo 0 > /proc/sys/net/ipv6/conf/eth0.1/accept_ra");
                  system("echo 0 > /proc/sys/net/ipv6/conf/br0/accept_ra");
#endif //IPv6_SUPPORT
        init_network();
        system("./etc/rc.d/pre_customer.sh");
#ifdef AR7161
        /*NickChou for ART MP more fast to upload art.ko*/
    system("tftpd &");
#endif

/* ken added for get image checksum
   move from http init for if rc restart run flash_get_checksum again
*/
#ifdef  CONFIG_USER_SHOW_CHECKSUM
        flash_get_checksum();
#endif
#ifdef CONFIG_LP
        lp_get_checksum();
#endif

#ifdef IPv6_SUPPORT
#ifdef IPv6_TEST
        system("ifconfig eth0.2 3ffe:501:ffff:100:201:23ff:fe45:6789/64");
        system("ifconfig br0 3ffe:501:ffff:100:201:23ff:fe45:6789/64");
        system("ifconfig eth0.1 3ffe:501:ffff:101:201:23ff:fe45:678a/64");
		set_ipv6_param();
#endif //IPv6_TEST
#endif //IPv6_SUPPORT

        hostname = nvram_safe_get("hostname");
        if (strlen(hostname) > 1)
                strcpy(tmp_hostname, hostname);
        else
                strcpy(tmp_hostname, nvram_safe_get("model_number"));

        _system("hostname \"%s\"", parse_special_char(tmp_hostname));

        system_time = nvram_safe_get("system_time");
        if(strlen(system_time) < 1)
        {
                DEBUG_MSG("set system_time = __BUILD_DATE__\n");
                _system("date -s %s", __BUILD_DATE__);
        }
  	else 
	{
		sscanf(system_time,"%d/%d/%d/%d/%d/%d",&year,&month,&date,&hour,&min,&sec);
		_system("date -s %02d%02d%02d%02d%04d",month,date,hour,min,year);
		//_system("date -s %s", system_time);
	}

        syslog_server = nvram_safe_get("syslog_server");
        /* jimmy added, 20080822, in case syslog server IP miss */
        strcpy(tmp_syslog_server,syslog_server);
        /* --------------------------------------------------- */
/* jimmy modified, 20080822, in case syslog server IP miss */
        if( strlen(tmp_syslog_server) )
        {
                                syslog_enable = strtok(tmp_syslog_server, "/");
#if 0
        if( strlen(syslog_server) )
        {
                syslog_enable = strtok(syslog_server, "/");
#endif
/* ------------------------------------------------------- */
                syslog_ip = strtok(NULL, "/");
                if(syslog_enable == NULL || syslog_ip == NULL)
                {
                        printf("service_init: syslog_server error\n");
/* jimmy modified for IPC syslog */
//                      _system("syslogd -s %d -b 0 &", LOG_MAX_SIZE);
                        _system("syslogd -O %s -b 0 &",LOG_FILE_BAK);
/* ---------------------------------*/
                }
                else
                {
/* jimmy modified for IPC syslog */
//                      if( strcmp(syslog_enable, "1") == 0 )
//                              _system("syslogd -s %d -b 0 -L -R %s:514 &", LOG_MAX_SIZE, syslog_ip);
//                      else
//                              _system("syslogd -s %d -b 0 &", LOG_MAX_SIZE);
/* ---------------------------------*/
                        if( strcmp(syslog_enable, "1") == 0 )
                                _system("syslogd -O %s -L -R %s:514 &",LOG_FILE_BAK , syslog_ip);
                    else
                                _system("syslogd -O %s &",LOG_FILE_BAK);
                }
        }
        else
/* jimmy modified for IPC syslog */
//              _system("syslogd -s %d -b 0 &", LOG_MAX_SIZE);
                _system("syslogd -O %s -b 0 &",LOG_FILE_BAK);
/* ---------------------------------*/

        system("klogd &");
/* Alper (Ubicom), GPIO driver modules are installed by default */
#ifdef KERNEL_2_6_15
//      _system("insmod %s", GPIO_DRIVER_PATH);
#else
//      _system("modprobe %s", GPIO_DRIVER_PATH);
//      _system("modprobe %s", WPS_LED_GPIO_DRIVER_PATH);
#endif
        init_file(AP_RT_INFO);
        save2file(AP_RT_INFO,"%s", nvram_safe_get("wlan0_mode"));
        system("/sbin/gpio SYSTEM check &");
#ifdef IP8000 
	system("/sbin/gpio SD_CHECK check &");
#endif
	updown_httpd(1);
        sleep(1);//prevent httpd dead
#ifdef CONFIG_USER_WEB_FILE_ACCESS
if(nvram_match("file_access_enable", "1") == 0)
	lighttpd_start();
#endif
#ifdef USE_SILEX
	system("/etc/rc.d/startSilex start&");
#endif
        system("timer &");

        init_file(LOG_FILE);
        syslog(LOG_INFO,"[Initialized, firmware version: %s ] \n",VER_FIRMWARE);

#ifdef CONFIG_VLAN_ROUTER
#ifndef IPv6_TEST
        system("dcc &");
        _system("lld2d %s &",nvram_safe_get("lan_bridge"));
#endif
#endif

        /*load driver module => iwconfig wifi0*/
#ifndef KERNEL_2_6_15
//      system("modprobe ath_ahb");
#endif

        check_wlan0_domain(1);
//Albert : we need change default pin when system init
//#ifdef AP_NOWAN_HASBRIDGE
/*
*       Date: 2009-5-20
*       Name: Ken_Chiang
*       Reason: modify for New WPS algorithm.
*       Notice :
*/
/*
        generate_pin_by_mac(wps_pin);
*/
/*
*       Date: 2009-5-20
*       Name: Ken_Chiang
*       Reason: modify just when wps_pin and wps_default_pin is default value "00000000"
*           to do generate_pin_by_wan_mac() and set the value to nvram
*                       for fixed the wps_pin value is changed when device is reboot.
*       Notice :
*/
/*
*       Date: 2009-5-22
*       Name: Ken_Chiang
*       Reason: remove to main_loop START for if when wps_pin and wps_default_pin is default value "00000000"
*           don't must power down device just rc restart then to do generate_pin_by_wan_mac().
*       Notice :
*/
/*
        start_get_wps_default_pin();
*/
//#endif
#ifdef CONFIG_USER_SNMP
        system("mkdir /var/net-snmp"); //create the net-snmp folder
#endif

/*
 * Date: 2009-08-25
 * Name: Ubicom
 * Reason: USB support is not built as kernel modules; therefore the
 *              followings (enclosed with CONFIG_USER_WCN) are commented
 */

//#ifdef CONFIG_USER_WCN
//#ifdef AR7161
    //system("insmod /lib/modules/2.6.15--LSDK-7.2.0.156/kernel/drivers/usb/core/usbcore.ko");
    //system("insmod /lib/modules/2.6.15--LSDK-7.2.0.156/kernel/drivers/usb/host/ehci-hcd.ko");//support USB 2.0 driver => permanent service
/*
*       Date: 2009-1-19
*       Name: Ken_Chiang
*       Reason: modify for 3g usb client card to used.
*       Notice :
*/
//#ifdef CONFIG_USER_3G_USB_CLIENT
    //system("insmod /lib/modules/2.6.15--LSDK-7.2.0.156/kernel/drivers/usb/host/ohci-hcd.ko");//support USB 1.0 driver => permanent service
//#endif //CONFIG_USER_3G_USB_CLIENT
/*
*       Date: 2009-1-19
*       Name: Ken_Chiang
*       Reason: modify for kernel usb change to modules.
*       Notice :
*/
//#else //AR7161
         //system("insmod /lib/modules/2.6.15--LSDK-7.1.2.27/kernel/drivers/usb/core/usbcore.ko");
     //system("insmod /lib/modules/2.6.15--LSDK-7.1.2.27/kernel/drivers/usb/host/ehci-hcd.ko");//support USB 2.0 driver => permanent service
//#endif //AR7161
//#endif //CONFIG_USER_WCN

        return 0;
}

#ifdef IPv6_SUPPORT
void get_inet6_status(void) {
        FILE *f;
        char devname[20];
        int plen, scope, dad_status, if_idx;
        char addr6p[8][5];

        if ((f = fopen(_PROCNET_IFINET6, "r")) != NULL) {
                while (fscanf(f, "%4s%4s%4s%4s%4s%4s%4s%4s %02x %02x %02x %02x %20s\n",
                                addr6p[0], addr6p[1], addr6p[2], addr6p[3], addr6p[4], addr6p[5], addr6p[6], addr6p[7],
                                &if_idx, &plen, &scope, &dad_status, devname) != EOF) {
                        switch (scope & IPV6ADDR_SCOPE_MASK) {
//                      case 0:
//                              printf("Global\n");
//                              break;
                        case IPV6ADDR_LINKLOCAL:
//                              printf("Link\n");
                                if (strcmp(devname,"br0") == 0)
                                        inet6_br0_status = dad_status;
                                else if (strcmp(devname, "eth1") == 0)
                                        inet6_eth1_status = dad_status;
                                DEBUG_MSG("inet6_if=%s, dad_status=%02x\n", devname, dad_status);
                                break;
//                      case IPV6ADDR_SITELOCAL:
//                              printf("Site\n");
//                              break;
//                      case IPV6ADDR_COMPATv4:
//                              printf("Compat\n");
//                              break;
//                      case IPV6ADDR_LOOPBACK:
//                              printf("Host\n");
//                              break;
//                      default:
//                              printf("Unknown\n");
                        }
                }
                fclose(f);
        }
        return;
}
#ifdef IPv6_TEST
int set_ipv6_param(void)
{
        int i=0;
        for (i=0;i<4;i++) {
                usleep(1000000);
                get_inet6_status();
                if (inet6_eth1_status == 0x80) {
                        if(strlen(nvram_safe_get("ipv6_static_wan_ip")) != 0)
                                _system("ifconfig %s add %s/%s",nvram_get("wan_eth"),nvram_get("ipv6_static_wan_ip"),nvram_get("ipv6_static_prefix_length"));
//                               system("ifconfig eth0.1 3ffe:501:ffff:101:201:23ff:fe45:678a/64");
                }                // NOTE: ifconfig should assign "/prefix",or prefix would be "/0" 

                if (inet6_br0_status == 0x80) {
                        if(strlen(nvram_safe_get("ipv6_static_lan_ip")) != 0)
                                _system("ifconfig %s add %s/64",nvram_get("lan_bridge"), nvram_get("ipv6_static_lan_ip"));
//                               system("ifconfig eth0.1 3ffe:501:ffff:101:201:23ff:fe45:678a/64");
                }		 // NOTE: ifconfig should assign "/prefix",or prefix would be "/0"

                if ((0x80==inet6_eth1_status)&&(0x80==inet6_br0_status))
                        break;

//       system("ifconfig eth0.1 3ffe:501:ffff:101:201:23ff:fe45:678a/64");
        }
        return 0;
}
#endif //IPV6_TEST
#endif //IPv6_SUPPORT

/*
 * rc_streamengine_stop_qos()
 */
static void rc_streamengine_stop_qos(void)
{
	int wantimer_pid;

	if (!action_flags.all && !action_flags.qos) {
		return;
	}

	/*
	 *	Date: 2010-May-10
	 *	Name: Gareth Williams <gareth.williams@ubicom.com>
	 * 	Reason: When streamengine is active qos is controlled by wantimer (because we also need to start/stop qos on wan state changes)
	 *	for this reason we SIGNAL to the wantimer to stop.
	 */
	wantimer_pid = read_pid(WANTIMER_PID);
	if (wantimer_pid == -1) {
		DEBUG_MSG("Unable to read wantimer pid\n");
	} else {
		DEBUG_MSG("Send down signal to wan timer\n");
		kill(wantimer_pid, SIGILL);
	}
}

/*
 * rc_streamengine_start_qos()
 */
static void rc_streamengine_start_qos(void)
{
	int wantimer_pid;

	if (!action_flags.all && !action_flags.qos) {
		return;
	}

	/*
	 *	Date: 2010-May-10
	 *	Name: Gareth Williams <gareth.williams@ubicom.com>
	 * 	Reason: When streamengine is active qos is controlled by wantimer (because we also need to start/stop qos on wan state changes)
	 *	for this reason we SIGNAL to the wantimer to start.
	 */
	wantimer_pid = read_pid(WANTIMER_PID);
	if (wantimer_pid == -1) {
		DEBUG_MSG("Unable to read wantimer pid\n");
	} else {
		DEBUG_MSG("Send up signal to wan timer\n");
		kill(wantimer_pid, SIGSYS);
	}
}

static void main_loop(void)
{
        sigset_t sigset;
#ifdef CONFIG_USER_FASTNAT_APPS
        static int first_init = 1;
#endif
/*
 * Reason: Let MP smooth.
 * Modified: John Huang
 * Date: 2010.02.09
 */
#ifdef UBICOM_MP_SPEED
        FILE *fp = NULL;
        int mp_testing=0;
#endif

        /* Basic initialization */
        sysinit();
        /* Setup signal handlers */
        //      signal_init();

        signal(SIGHUP, rc_signal);      //RC RESTART
        signal(SIGUSR2, rc_signal);     //RC START
        signal(SIGINT, rc_signal);      //RC STOP
        signal(SIGALRM, rc_signal);     //TIMER
        signal(SIGUSR1, rc_signal);     //WAN
        signal(SIGQUIT, rc_signal);     //LAN
        signal(SIGTSTP, rc_signal);     //WLAN
#ifdef CONFIG_VLAN_ROUTER
        signal(SIGSYS, rc_signal);      //FIREWALL
#endif
        signal(SIGTERM, rc_signal);     //APP
        signal(SIGILL, rc_signal);      //WLAN+APP
        signal(SIGPIPE, rc_signal);     //DHCPD
#ifdef CONFIG_USER_TC
        signal(SIGTTIN, rc_signal);     //QOS
#endif

        sigemptyset(&sigset);

        ALL_SET;
        service_init();
        for (;;) {
                switch (state) {
                        case RESTART:
                                DEBUG_MSG("RESTART\n");
                                /* Chun: set flag to announce rc is busy restarting now.
                                 * Since udhcpd and pppd both call rc restart once they obtain IP,
                                 * I need to set flag here to inform one of them that rc is restart now
                                 * and please wait until rc is idle. This protects rc from receiving multiple
                                 * restart signal at the same time.
                                 */
                                set_flag("b");
/*
 * Reason: Let MP smooth.
 * Modified: John Huang
 * Date: 2010.02.09
 */
#ifdef UBICOM_MP_SPEED
				system("echo b > /var/tmp/rc_idle.tmp");
#endif
#ifdef CONFIG_USER_CLI
				system("cli sys account");
				system("cli services https stop");
				system("cli services https start");
				system("cli security vpn sslvpn stop");
				system("cli security vpn sslvpn start");
#endif
                                nvram_flag_reset();
                                kill(read_pid(HTTPD_PID), SIGUSR2);
#ifdef CONFIG_USER_OWERA
                                stop_owera();
#endif

#ifdef CONFIG_USER_FASTNAT_APPS
                                if (first_init)
                                        break;
#endif

                                /* Fall through */
                        case STOP:
                                DEBUG_MSG("STOP\n");
                                set_flag("b");
                                if(read_pid(GPIO_PID) > 0)
                                kill(read_pid(GPIO_PID), SIGHUP);

#ifdef CONFIG_MODULE_UBICOM_STREAMENGINE2_IP8K
	_system("/etc/init.d/na_stop");
#endif

#ifdef CONFIG_USER_TC
#ifdef CONFIG_USER_WISH
				ubicom_wish_stop();
#endif
#ifndef CONFIG_USER_STREAMENGINE
				/*
				 *	Date: 2010-Jan-20
				 *	Name: Gareth Williams <gareth.williams@ubicom.com>
				 *	Reason: start_qos() called only when streamengine is NOT active - start and stopping of QoS is done by wantimer when streamengine is active.
				 */
                                stop_qos();
#else
                                rc_streamengine_stop_qos();
#endif
#endif
#ifdef CONFIG_VLAN_ROUTER
                                stop_wan();
                                stop_firewall();
#endif

#ifdef CONFIG_USER_OPENAGENT
                                stop_tr069();
#endif
                                stop_app();
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
        			if(mp_testing != 1) {
#endif
                                stop_wlan();
                                stop_wps();
#ifdef UBICOM_MP_SPEED
				}
#endif
                                stop_lan();
#ifdef IPv6_SUPPORT
				if( action_flags.all || action_flags.wan )
					system("cli ipv6 stop");
#endif
                                if (state == STOP) {
                                        state = IDLE;
                                        break;
                                }
                                /* Fall through */
                        case START:
                                DEBUG_MSG("START\n");
                /* Chun: set flag to announce rc is busy restarting now.
                                 * In the case of booting, router only "start rc" instaed of "restart rc".
                                 * I need to set flag here as well to prevent dhcpd and pppd from sending signal
                                 * when rc is booting.
                                 */
                                set_flag("b");

/*
 * Reason: Let MP smooth.
 * Modified: John Huang
 * Date: 2010.02.09
 */
#ifdef UBICOM_MP_SPEED
				system("echo b > /var/tmp/rc_idle.tmp");
#endif

/*
*       Date: 2009-5-22
*       Name: Ken_Chiang
*       Reason: remove from service_init main_loop START for if when wps_pin and wps_default_pin is default value "00000000"
*           don't must power down device just rc restart then to do generate_pin_by_wan_mac().
*       Notice :
*/
                                start_get_wps_default_pin();

#ifndef DLINK_ROUTER_LED_DEFINE
                            system("gpio STATUS_LED blink &");
#endif

#ifdef CONFIG_VLAN_ROUTER
                                start_wan();
#endif
#ifdef IPv6_TEST
                        system("ifconfig eth0.1 3ffe:501:ffff:101:201:23ff:fe45:678a/64");
#endif
                                start_lan();
#if defined (DLINK_ROUTER_LED_DEFINE) || defined (AP_NOWAN_HASBRIDGE)
                                system("gpio POWER_LED on");
#endif

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
                                start_wlan();

                                start_app();
/*
 * Reason: Let MP smooth.
 * Modified: John Huang
 * Date: 2010.02.09
 */
#ifdef UBICOM_MP_SPEED
				system("echo o > /var/tmp/rc_idle.tmp");
#endif

#ifdef CONFIG_VLAN_ROUTER
                                start_firewall();
#endif

#ifdef DHCP_RELAY
                                start_dhcp_relay();
#endif

#ifdef CONFIG_USER_OPENAGENT
                                start_tr069();
#endif

/*
 * Reason: Let MP smooth.
 * Modified: John Huang
 * Date: 2010.02.09
 */
#ifdef UBICOM_MP_SPEED
        			if(mp_testing != 1)
#endif
                                start_wps();

/*
 * Reason: Let MP smooth.
 * Modified: John Huang
 * Date: 2010.02.09
 */
#ifdef UBICOM_MP_SPEED
				system("echo k > /var/tmp/rc_idle.tmp");
#endif

#ifdef CONFIG_USER_TC
/*
 * Reason: Let MP smooth.
 * Modified: John Huang
 * Date: 2010.02.09
 */
#ifdef CONFIG_USER_WISH
#ifdef UBICOM_MP_SPEED
        			if(mp_testing == 1)
        			_system("echo 0 > /proc/br_nf_wish");
        			else
#endif
				ubicom_wish_start();
#else
				_system("echo 0 > /proc/br_nf_wish");
#endif

#ifndef CONFIG_USER_STREAMENGINE
				/*
				 *	Date: 2010-Jan-20
				 *	Name: Gareth Williams <gareth.williams@ubicom.com>
				 *	Reason: start_qos() called only when streamengine is NOT active - start and stopping of QoS is done by wantimer when streamengine is active.
				 */
/*
 * Reason: Let MP smooth.
 * Modified: John Huang
 * Date: 2010.02.09
 */
#ifdef UBICOM_MP_SPEED
        			if(mp_testing != 1)
#endif
                                start_qos();
#else
                                rc_streamengine_start_qos();
#endif
#endif

#ifdef CONFIG_USER_OWERA
                                start_owera();
#endif
#ifdef IPv6_SUPPORT
	        		if( action_flags.all || action_flags.wan )
					system("cli ipv6 start");
#endif
			/*
			 * Robert 20100128 : Check idel time
			 * Sometimes it could stop by other demand.
			 */
				check_wantimer();

start_ntp();

                        /* Fall through */
                        case IDLE:
                                DEBUG_MSG("IDLE\n");
                                /* Chun: set flag to announce rc is idle now.
                                 * At the status, either dhcpd or pppd can send signal to rc.
                                 */
#ifdef IPv6_SUPPORT
#ifdef IPv6_TEST
                                set_ipv6_param();
#endif //IPv6_TEST
#endif //IPv6_SUPPORT
                                set_flag("i");
                                state = IDLE;
/*
 * Reason: Let MP smooth.
 * Modified: John Huang
 * Date: 2010.02.09
 */
#ifdef UBICOM_MP_SPEED
				system("echo i > /var/tmp/rc_idle.tmp");
#endif
                                system("./etc/rc.d/post_customer.sh");
                                /* Wait for user input or state change */
                                while (signalled == -1) {
                                        sigsuspend(&sigset);
                                }
                                state = signalled;
                                signalled = -1;
#ifdef CONFIG_USER_FASTNAT_APPS
                                first_init = 0;
#endif
                                break;
                        default:
                                DEBUG_MSG("UNKNOWN\n");
                                return;
                }
        }
}



int main(int argc, char **argv)
{
        char *base = strrchr(argv[0], '/');

        base = base ? base + 1 : argv[0];

        /* init */
        if (strstr(base, "init")) {
                main_loop();
                return 0;
        }

        /* rc [stop|start|restart ] */
        if (strstr(base, "rc")) {
                if (argv[1]) {
                        if (strncmp(argv[1], "start", 5) == 0) {
                                return kill(read_pid(RC_PID), SIGUSR2);
                        } else if (strncmp(argv[1], "stop", 4) == 0) {
                                return kill(read_pid(RC_PID), SIGINT);
                        } else if (strncmp(argv[1], "restart", 7) == 0) {
                                return kill(read_pid(RC_PID), SIGHUP);
                        } else if (strncmp(argv[1], "init", 4) == 0) {
                                main_loop();
                        }
                } else {
                        fprintf(stderr, "usage: rc [start|stop|restart|init]\n");
                        return EINVAL;
                }
        }
#ifdef CONFIG_VLAN_ROUTER
        else if (strstr(base, "redial"))
                return redial_main(argc, argv);
        else if (strstr(base, "monitor"))
                return monitor_main(argc, argv);
#ifdef IPv6_SUPPORT                
        else if (strstr(base, "mon6"))	//monitor for ipv6 pppoe
                return ipv6_monitor_main(argc, argv);
	else if (strstr(base, "red6"))	//redial for ipv6 pppoe
                return ipv6_redial_main(argc, argv);
#endif                
#ifdef MPPPOE
        else if (strstr(base, "ppptimer"))
                return ppptimer_main(argc, argv);
#endif
        else if (strstr(base, "ip-up"))
                return ipup_main(argc, argv);
        else if (strstr(base, "ip-down"))
                return ipdown_main(argc, argv);
        else if (strstr(base, "psmon"))
                return psmon_main(argc, argv);
        else if (strstr(base, "fwupgrade"))
                return fwupgrade_main(argc, argv);
        else if (strstr(base, "wantimer"))
                return wantimer_main(argc, argv);
#else
        else if (strstr(base, "fwupgrade"))
                return fwupgrade_main(argc, argv);
        else if (strstr(base, "lanmon"))
                return lanmon_main(argc, argv);
#endif
#ifdef CONFIG_USER_WL_RALINK
        else if (strstr(base, "cmds"))
                return cmds_main(argc, argv);
#endif
#ifdef DIR865
        else if (strstr(base, "wlan-config"))
                return wconfig_main(argc, argv);
#endif

/* jimmy marked 20080603 */
//#ifdef CONFIG_USER_TC
//      else if (strstr(base, "mbandwidth"))
 //               return mbandwidth_main(argc, argv);
//#endif
/* --------------------- */
        return EINVAL;
}
