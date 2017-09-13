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
#include <netdb.h>
#ifdef RC_DEBUG
#define DEBUG_MSG(fmt, arg...)       printf("%s [%d]: "fmt, __FILE__, __LINE__, ##arg)
#else
#define DEBUG_MSG(fmt, arg...)
#endif

int tunnel_check_flag = 1;

#ifdef IPV6_PPPoE
int ipv6_redial_main(int argc, char **argv)
{
	int need_redial = 0;
	char *ipv6_wan_proto = NULL;
	int ipv6_need_redial_count = 0;
	char ppp6_ip[MAX_IPV6_IP_LEN]={};
	
        ipv6_wan_proto = argv[2];

	if (access(RED6_PID, F_OK) == 0)
        {
		printf("%s is already active !\n",RED6_PID);
		return 0;
        }
	else
                {
		// create empty RED6_PID file
		fclose(fopen(RED6_PID, "w+"));
		}
 
	while(1)
                {
		// check ipv6 wizard is not running
		if (access(WIZARD_IPV6, F_OK)!=0) {
			read_ipv6addr("ppp6", SCOPE_LOCAL, ppp6_ip, sizeof(ppp6_ip));
                        if(strlen(ppp6_ip) < 1)
                        {
                                DEBUG_MSG("ipv6_redial_main: ppp6 IP = NULL\n");
				if((access(PPP6_PID, F_OK) != 0) || (ipv6_need_redial_count == 2) )
                                {
                                        DEBUG_MSG("ipv6_redial_main: PPPoE6 need_redial\n");
                                        need_redial = 1;
					ipv6_need_redial_count = 0 ;
                                }
                                else
                                {
                                        DEBUG_MSG("ipv6_redial_main: PPPoE6 deaman is running\n");
                                        ipv6_need_redial_count ++ ;
                                }
                        }
                	if(need_redial)
                	{
				if(strncmp(ipv6_wan_proto, "ipv6_pppoe", 10) == 0 )
				{		
                                	DEBUG_MSG("redial ipv6 pppoe\n");
					system("cli ipv6 pppoe dial");
				}	
                        	need_redial = 0;
                	}
		}
		sleep(atoi(argv[1]));
        }
}

int ipv6_monitor_main(int argc, char** argv)
{
        int connected=0, not_LocalPacket=1;
	char *lan_bridge=NULL;
	char *ipv6_sn=NULL, *ipv6_ac=NULL;
	char ppp6_ip[MAX_IPV6_IP_LEN]={};

	lan_bridge = nvram_safe_get("lan_bridge");

/*Albert Chen: please do not remove this sleep, it will cause dail on demand fail */
        sleep(1);
	
	if( access(WANTIMER_PID, F_OK) == 0 )
	{
		printf("********** Monitor IPv6 PPP Connection State ( In Dial on Demand Mode)**********\n");
	}
	else
        {
                printf("********** wan_timer fail => stop wan monitor **********\n");
                return 0;
        }

	if (access(MON6_PID, F_OK) == 0)
	{
		printf("%s is already active !\n",MON6_PID);
		return 0;
	}
        else
        {
		//create empty MON6_PID file
		fclose(fopen(MON6_PID, "w+"));
        }

        while(1)
        {
		sleep(5);
		// check ipv6 wizard is not running
		if (access(WIZARD_IPV6, F_OK)!=0)
                {
                if(connected)
                {
				read_ipv6addr("ppp6", SCOPE_LOCAL, ppp6_ip, sizeof(ppp6_ip));
				if( (strlen(ppp6_ip) < 1) && (access(PPP6_PID, F_OK) != 0) )
                                {
                                        printf("ipv6_monitor_main: PPPoE ppp6 is disconnected\n");
                                        connected = 0;
                                }

                        if( tunnel_check_flag )
                        {
                                kill(read_pid(WANTIMER_PID), SIGTRAP); /*start_idle_timer*/
                                sleep(2);
                                tunnel_check_flag = 0;
                        }

                        if(connected)
                        {
					not_LocalPacket = listen_socket6(lan_bridge);
				if( not_LocalPacket )
                                        kill(read_pid(WANTIMER_PID), SIGABRT);/*reset_idle_timer*/
                        }
                }
                else //no connected
                {
				read_ipv6addr("ppp6", SCOPE_LOCAL, ppp6_ip, sizeof(ppp6_ip));
				if((strlen(ppp6_ip) > 1)){
                                                printf("PPPoE ppp6 is connected\n");
                                                connected = 1;
                                        }

                        if( tunnel_check_flag == 0 )
                        {
                                tunnel_check_flag = 1;
                                kill(read_pid(WANTIMER_PID), SIGILL);/*stop_idle_timer*/
				sleep(2);
                        }

                        if(!connected)
                        {
					not_LocalPacket = listen_socket6(lan_bridge);
				if( not_LocalPacket )
                                {
                                                printf("ipv6_monitor: Dial IPv6 PPPoE\n");
						system("cli ipv6 pppoe dial");
						char ppp6_ip[MAX_IPV6_IP_LEN]={};
						read_ipv6addr("ppp6", SCOPE_LOCAL, ppp6_ip, sizeof(ppp6_ip));
                                                if (strlen(ppp6_ip) > 1)         
							connected = 1;
                                        /* wait for connect */
                                        sleep(5);
                                }
                        }
                }
        }
	}
        return 0;
}
#endif //IPV6_PPPoE
