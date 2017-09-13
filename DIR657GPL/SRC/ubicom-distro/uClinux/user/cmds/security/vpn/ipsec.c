#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
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
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <strings.h>
#include <string.h>

#include "cmds.h"
#include "shutils.h"
#include <nvramcmd.h>
#define uint32 unsigned long
#define uint unsigned int
#define uint8 unsigned char
#define uint64 unsigned long long

#define IPSEC_CONF_FILE		"/tmp/ipsec.conf"
#define IPSEC_SECRETS_FILE	"/tmp/ipsec.secrets"

char *info_getwan_interface()
{
	char *wan_proto = nvram_safe_get("wan_proto");

	if (strcmp(wan_proto, "static") == 0 || strcmp(wan_proto, "dhcpc") == 0)
		return "eth0.1";
	return "ppp0";
}

static char *strsep_with_validation(char **, char *, char *);

#define MAX_NVRAM_IPSEC_VALUE_SIZE 1024
#define STRSEP strsep_with_validation
#include "ipsec_bcm.h"
static char * strsep_with_validation(char **ptr,char *sep,char * retchar)	
{ 
	char * strptr ;	
	char *cptr;

	if (*ptr == NULL)
		return retchar;	

	strptr  =strsep(ptr,sep);	
	if (strptr != NULL) {

		/* change eny last \n to NULL */	
		cptr = strchr(strptr,'\r');;
		if (cptr != NULL)
			*cptr =' '; 


		if (*strptr == 0x00)		
			return retchar;	
	
	}else if (strptr == NULL)
		return retchar;
	
	return	strptr;	
}

static int ipsec_update_wan_addr(char *buf, const char *ip)
{
	char *t, *n, *p;
	int rev = -1;

	//cprintf("update_wan_addr:%s\n", ip);
	t = buf;
	if ((t = strchr(t, ';')) == NULL) goto out; t++; /* skip action field */
	if ((t = strchr(t, ';')) == NULL) goto out; t++; /* skip name field */
	if ((t = strchr(t, ',')) == NULL) goto out; t++; /* skip tunnel field*/
	if ((n = strchr(t, '-')) == NULL) goto out;
	//cprintf("Updating:%s\n%s\n", t, n);
	p = strdup(n);
	strcpy(t, ip); /* It would overwrite bufffer @buf field of Left */
	strcat(t, p);
	free(p);
	rev = 0;
	//cprintf("BUF[%s]\n", buf);
out:
	return rev;
}

/* replace 'key' as 'rep' from 'org' string */
static void
string_replace(char *org, const char *key, const char *rep)
{
	char buf[512];
	char *idx = strstr(org, key);

	if (idx == NULL)
		return;

	bzero(buf, sizeof(buf));
	strncpy(buf, org, idx - org);
	strcat(buf, rep);
	strcat(buf, idx + strlen(key));
	strcpy(org, buf);
}
static FILE *get_and_clear_ipsec_conf(const char *secrets, const char *conf)
{
	FILE *fd;

	/* clear ipsec.secrets); */
	if (!(fd = fopen(secrets, "w"))) {
		perror(secrets);
		return NULL;
	}
	fclose(fd);

	/* Write to ipsec.conf  file based on current information */
	if (!(fd = fopen(conf, "w"))) {
		perror(conf);
		return NULL;
	}
	return fd;
}


static char *strcat_ipsec_string(int vpn_idx, char *buf)
{
	char vpn_conn[] = "vpn_connXXXXXX";
	char vpn_extra[] = "vpn_extraXXXXXX";
	char *vp;
	sprintf(vpn_conn, "vpn_conn%d", vpn_idx);
	sprintf(vpn_extra, "vpn_extra%d", vpn_idx);

	if (*(vp = nvram_safe_get(vpn_conn)) == '\0')
		return NULL;
	/* Skip disabled rule */
	if ((strncmp(vp, "ignore", 6)) == 0)
		return NULL;
	
	strcpy(buf, vp);
	strcat(buf, ";");
	if (*(vp = nvram_safe_get(vpn_extra)) != "")
		strcat(buf, vp);
	return buf;

}
/*
* create/recreate the ipsec.conf file in /etc
*/
int
create_ipsec_conf(void)
{
	FILE *fd;
	char *wan_ifname=NULL;
	char *lan_ifname=NULL;
	char *wl_ifname=NULL;
	char wanif0[IF_NAMESIZE] = "";
	char wanif1[IF_NAMESIZE] = "";
	char host0[32] = "", host1[32] = "", mask0[32] = "", mask1[32] = "";
	
	//ipsec_Conn_t ipsec_Conn;
	int conn_id=1; /* connections start from 1. not 0 */

	fd = get_and_clear_ipsec_conf(IPSEC_SECRETS_FILE, IPSEC_CONF_FILE);

#ifdef CONFIG_BCM_IPSEC
	lan_ifname= nvram_safe_get("lan_ifname");
	wl_ifname= nvram_safe_get("wl_ifname");
	/* DIR130/330 style */
	dual_wans_info(host0, mask0, wanif0, host1, mask1, wanif1);
#else
	/* FIXME: hard code in DIR730*/
	lan_ifname= "br0";
	wl_ifname= "eth1";
#endif	
#ifdef CONFIG_BCM_IPSEC
	if (*wanif0 != 0x00 && *wanif1 != 0x00) {
		wan_ifname = wanif0;
		/* XXX: If daul wans up. overwirte wireless IFNAME
		 * After all, Ipses does not bind wireless.
		 * */
		wl_ifname = wanif1;
	} else if (*wanif0 != 0x00) {
		wan_ifname = wanif0;
	} else if (*wanif1 != 0x00)  {
		wan_ifname = wanif1;
	} else {
		fprintf(stderr, "IPSec: interfaces error!\n");
		return -1;
	}
#else
	/* FIXME: bad hard code */
	strcpy(wanif0, info_getwan_interface());
	get_ip(wanif0, host0);
#endif
	fprintf(fd, "config setup\n");

#ifdef CONFIG_BCM_IPSEC
	__ipsec_bcm_setup(fd, wan_ifname, lan_ifname, wl_ifname);
	if (strcmp(nvram_safe_get("vpn_nat_traversal"), "1") == 0 ) {
		fputs("\tnat_traversal=yes\n", fd);
		fputs("\tvirtual_private=%v4:0.0.0.0/1,%v:128.0.0.0/1,%all\n", fd);
	}

	/* default for subsequent connections */
	fputs("\nconn %default\n\
		\tkeyingtries=0\n\
		\tdisablearrivalcheck=no\n\
		\tauthby=rsasig\n\
		\tleftrsasigkey=%dnsondemand\n\
		\trightrsasigkey=%dnsondemand\n",fd);
#else
	fprintf(fd,"\tnat_traversal=yes\n"
	"\tvirtual_private=%v4:10.0.0.0/8,%v4:192.168.0.0/16,%4:172.16.0.0/12\n"
	"\tprotostack=netkey\n\n");
						
#endif
	conn_id=1;
	{
#define VPN_BUFFER_SIZE	1024
#define VPN_CONN_MAX	26   //99//26
		FILE *authfd;
		char *vp, buf[VPN_BUFFER_SIZE];
		int vpn_idx;
		
		if (!(authfd = fopen(IPSEC_SECRETS_FILE, "a+"))) {
			perror(IPSEC_CONF_FILE);
			return -1;
		};		
		//init_ipsec_script(); /* obsolete */
		for (vpn_idx = 0; vpn_idx <= VPN_CONN_MAX; vpn_idx++) {
			if ((vp = strcat_ipsec_string(vpn_idx, buf)) == NULL)
				continue;
			fprintf(stderr, "wanif0: %s, wanif1: %s\n", wanif0, wanif1);
			if (*wanif0 != 0x00 || (*wanif0 == 0x00 && (NVRAM_MATCH("wan0_proto", "rupptp") ||
				NVRAM_MATCH("wan0_proto", "rupppoe")))) {
				char route[8], uname[16], tunnel[16];
				char range[64];
				char leftip[32], rightip[256];

				bzero(route, sizeof(route));
				bzero(uname, sizeof(uname));
				bzero(tunnel, sizeof(tunnel));
				bzero(range, sizeof(range));
				bzero(leftip, sizeof(leftip));
				bzero(rightip, sizeof(rightip));
				/* route;s2s;tunnel,0.0.0.0-172.21.33.23,192.168.... */
				/* add;ruser;tunnel,10.0.0.2-any%,192.168.... */
				sscanf(vp, "%[^;];%[^;];%[^,],%[^,]", route, uname, tunnel, range);
				sscanf(range, "%[^-]-%s", leftip, rightip);
				if (*wanif1 != 0x00) {	// dual access
                                        /* XXX: DUAL ACCESS 2010/4/21 copy from DIR130, not verify */
					if (strcmp(rightip, "%any") == 0) {	// remote user
						char buf2[VPN_BUFFER_SIZE];
						char comm_name1[VPN_BUFFER_SIZE];
						char comm_name2[VPN_BUFFER_SIZE];

						sscanf(buf, "%[^;];%[^;]", route, uname);
						bzero(buf2, sizeof(buf2));
						bzero(comm_name1, sizeof(comm_name1));
						bzero(comm_name2, sizeof(comm_name2));

						sprintf(comm_name1, "%s_russia_remote_ppp", uname);
						sprintf(comm_name2, "%s_russia_remote_phy", uname);
						strncpy(buf2, buf, sizeof(buf2));
						//cprintf("original buf: %s\n", buf);
						//cprintf("original buf2: %s\n", buf2);
						string_replace(buf, uname, comm_name1);
						string_replace(buf2, uname, comm_name2);

						//cprintf("uname: %s\n", uname);
						cprintf("comm_name1: %s\n", comm_name1);
						cprintf("comm_name2: %s\n", comm_name2);
						//cprintf("processed buf: %s\n", buf);
						//cprintf("processed buf2: %s\n", buf2);
						cprintf("(dual access remote user)Init IPSec: %s:%s\n", wanif0, host0);
						cprintf("(dual access remote user)Init IPSec: %s:%s\n", wanif1, host1);
						ipsec_update_wan_addr(buf, host0);
						if (conf_vpn_connection(buf, fd, authfd) == 0)
							conn_id++;

						ipsec_update_wan_addr(buf2, host1);
						if (conf_vpn_connection(buf2, fd, authfd) == 0)
							conn_id++;
                                                /***********END DUAL ACCESS***************/

					} else {	// site to site
						char dev[8], dev_gw[16], dev_ip[16];

						//getGatewayInterfaceByIP(host1, dev, dev_gw);
						bzero(dev, sizeof(dev));
						bzero(dev_gw, sizeof(dev_gw));
						bzero(dev_ip, sizeof(dev_ip));

						sleep(3);
						getGatewayInterfaceByIP(rightip, dev, dev_gw);
						getIPbyDevice(dev, dev_ip);

						if (strlen(dev) == 0 || strlen(dev_ip) == 0)
							continue;

						cprintf("(dual access site to site)Init IPSec: %s:%s\n", dev, dev_ip);
						ipsec_update_wan_addr(buf, dev_ip);
						if (conf_vpn_connection(buf, fd, authfd) == 0)
							conn_id++;
					}
				} else { // single access
					cprintf("Init IPSec: %s:%s,[%s]\n", wanif0, host0, buf);
					ipsec_update_wan_addr(buf, host0);
					if (conf_vpn_connection(buf, fd, authfd) == 0)
						conn_id++;
				}
			}
		}
		fclose(authfd);
	}
	fclose(fd);
	return (conn_id-1);
}
#ifndef CONFIG_BCM_IPSEC
/*
 * Start IPSec.
 */
int start_ipsec(void)
{
	int noofcons=0;
	noofcons =create_ipsec_conf();
#ifndef CONFIG_UBICOM_ARCH
	if (noofcons > 0) {
		eval("ipsec", "setup", "start");
		return 0;
	}
#endif //CONFIG_UBICOM_ARCH
	/* no tunnel profile */
	return -1;
}

/*
 * Stop IPSec.
 */
static int stop_ipsec(void)
{
#ifdef CONFIG_UBICOM_ARCH
	config_whack_stop_pluto();
	eval("killall", "-9", "ipsec_dns");
#else
	eval("ipsec", "setup", "stop");
#endif
	return 0;
}
#endif //CONFIG_BCM_IPSEC
/*
 * Returns an IP address as an unsinged long 
 */
inline unsigned long ipaddr2hex(char *str)
{
	int d1,d2,d3,d4;
	char *s;

	s = str; d1 = atoi (s);
	s = strchr(s + 1, '.'); d2 = atoi (s+1);
	s = strchr(s + 1, '.'); d3 = atoi (s+1);
	s = strchr(s + 1, '.'); d4 = atoi (s+1);
	return ((unsigned long) d1 << 24 | (unsigned long)d2 << 16 
			| (unsigned long) d3 <<8 | (unsigned long) d4 );
}

/*
 * Translate ip mask into bits 
 */
static int mask2bits (char *str)
{
	unsigned long int x;
	int bits = 0;

	x = ipaddr2hex(str);
	while (!(x & 1) && (bits < 31)) { bits++; x >>= 1; }

	return 32 - bits;
}

extern int hw_ipsec_clear();
static int ipsec_main_start(int argc, char *argv[])
{
	if (start_ipsec() == -1)
		return -1;
	//system("iptables -t nat -D POSTROUTING -o eth1 -j MASQUERADE >/dev/null 2>&1");
	//system("iptables -t nat -I POSTROUTING -o eth1 -j MASQUERADE >/dev/null 2>&1");
	//system("echo \"1\" > /proc/sys/net/ipv4/ip_forward");
	//hw_ipsec_clear();
	/* FIXME: if IPSec ready, delete this rule. This rule be added from
	 * rc start_firewall, because of to prevent src IP is DUT LAN. But
	 * this is not good idea, conntrack problem still have.
	 *
	 * FredHung, 2010/6/10
	 * */

	unlink("/tmp/ns.res");
	if (fork() == 0) {
		sleep(10);
		system("iptables -t nat -D POSTROUTING -o ipsec0 -j MASQUERADE");
		exit(-1);
	}

	if (access("/tmp/ns.res", F_OK) == 0)
                system("ipsec_dns /tmp/ns.res &");

	return 0;
}

static int ipsec_main_stop(int argc, char *argv[])
{
	system("iptables -t nat -D POSTROUTING -o eth1 -j MASQUERADE");
	unlink("/tmp/ns.res");
	stop_ipsec();
}

extern int ipsec_main_monitor(int argc, char *argv[]);
int ipsec_main(int argc, char *argv[])
{
	struct subcmd cmds[] = {
		{ "start", ipsec_main_start, "start IPSec: rev0.00"},
		{ "stop", ipsec_main_stop, "stop IPSec: rev0.00"},
		{ "monitor", ipsec_main_monitor, "ike rekey timer" },
		{ NULL, NULL}
	};
	int rev, lock;
	setenv("PATH", "/bin:/sbin:/usr/bin:/usr/sbin:/usr/local/bin:/usr/local/sbin", 1);
	if (nvram_get("ipsec_lock") == NULL)
		lock = 0;
	else
		lock = 1;
	rd(1, "lock:%d", lock);
	lock_prog(argc, argv, lock);
	rev = lookup_subcmd_then_callout(argc, argv, cmds);
	unlock_prog();
	return rev;
}
