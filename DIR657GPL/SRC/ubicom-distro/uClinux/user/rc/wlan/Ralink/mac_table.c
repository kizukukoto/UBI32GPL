#include <stdio.h>			// for printf() etc.
#include <stdlib.h> 			// for malloc() free() atoi() etc.
#include <string.h>			// for strcpy() memcpy() etc.
#include <sys/socket.h>			/* for "struct sockaddr" et al	*/
#include <linux/types.h>                /* for __u* and __s* typedefs */
#include <linux/socket.h>               /* for "struct sockaddr" et al  */
#include <linux/if.h>                   /* for IFNAMSIZ and co... */
#include <linux/wireless.h>
#include <sys/ioctl.h>
#include <unistd.h>

//=============================================================================
#if WIRELESS_EXT <= 11
#ifndef SIOCDEVPRIVATE
#define SIOCDEVPRIVATE 0x8BE0
#endif
#define SIOCIWFIRSTPRIV SIOCDEVPRIVATE
#endif

#define RTPRIV_IOCTL_GET_MAC_TABLE (SIOCIWFIRSTPRIV + 0x0F)
#define MAC_ADDR_LEN 6
#define MAX_LEN_OF_MAC_TABLE 128

typedef union _MACHTTRANSMIT_SETTING 
{
	union
	{
		unsigned short MCS:7; // MCS
		unsigned short BW:1; //channel bandwidth 20MHz or 40 MHz
		unsigned short ShortGI:1;
		unsigned short STBC:2; //SPACE
		unsigned short rsv:3;
		unsigned short MODE:2; // Use definition MODE_xxx.
	}__attribute__((__packed__)) field;
	unsigned short word;
}__attribute__((__packed__)) MACHTTRANSMIT_SETTING, *PMACHTTRANSMIT_SETTING;


typedef struct _RT_802_11_MAC_ENTRY {
    unsigned char       Addr[MAC_ADDR_LEN];
    unsigned char       Aid;
    unsigned char       Psm;     // 0:PWR_ACTIVE, 1:PWR_SAVE
    unsigned char  	MimoPs;  // 0:MMPS_STATIC, 1:MMPS_DYNAMIC, 3:MMPS_Enabled
    signed char		AvgRssi0;
    signed char		AvgRssi1;
    signed char		AvgRssi2;
    unsigned int		ConnectedTime;
    MACHTTRANSMIT_SETTING	TxRate;
    MACHTTRANSMIT_SETTING	MaxTxRate;
    unsigned char		apidx;
} RT_802_11_MAC_ENTRY, *PRT_802_11_MAC_ENTRY;

typedef struct _RT_802_11_MAC_TABLE {
    unsigned long       Num;
    RT_802_11_MAC_ENTRY Entry[MAX_LEN_OF_MAC_TABLE];
} RT_802_11_MAC_TABLE, *PRT_802_11_MAC_TABLE;

static int get_dhcpc_pid(char *dhcp)
{
	char pid[24];
	FILE *fp = NULL;

	memset(pid, '\0', sizeof(pid));

	/* check the udhcpd server will start or not*/
	if (access("/var/run/udhcpd.pid", F_OK) != 0)
		return -1;

	if ((fp = popen("cat /var/run/udhcpd.pid", "r")) == NULL)
		return -1;

	fgets(pid, sizeof(pid), fp);
	//fread(pid, 1, 5, fp);
	pclose(fp);

	strncpy(dhcp, pid, strlen(pid)-1);

	return 0;
}

static int get_lan_ip(const char *pid, char *lan_ip, int *cnt)
{
	char cmds[512], buf[512], ppid[128];
	char ipaddr[128], hwtype[12], flag[12], 
	     macaddr[128], mask[24], device[24],
	     *pdata;
	FILE *fp;
	int cflag = 0;

	memset(buf, '\0', sizeof(buf));
	memset(ppid, '\0', sizeof(ppid));
	memset(ipaddr, '\0', sizeof(ipaddr));
	memset(hwtype, '\0', sizeof(hwtype));
	memset(flag, '\0', sizeof(flag));
	memset(macaddr, '\0', sizeof(macaddr));
	memset(mask, '\0', sizeof(mask));
	memset(device, '\0', sizeof(device));

	strncpy(ppid, pid, strlen(pid));
	sprintf(cmds, "cat /proc/%s/net/arp", ppid);
	pdata = lan_ip;

	if ((fp = popen(cmds, "r")) == NULL)
		return -1;
	
	while (fgets(buf, sizeof(buf), fp)) {
		if (cflag == 0) {
			cflag++;
			continue;
		} else {
			sscanf(buf, "%s %s %s %s %s %s", 
			ipaddr, hwtype, flag, 
			macaddr, mask, device);
			if (strcmp(device, "br0") != 0)
				continue;
			if (cflag == 1) {
				sprintf(pdata, "%s;%s", ipaddr, macaddr);
				pdata+=35;
				(*cnt)++;
			} else {
				sprintf(pdata, "%s%s;%s", 
					pdata, ipaddr, macaddr);
				pdata+=35;
				(*cnt)++;
			}
		}
	}
	pclose(fp);
	return 0;
}

static int print_msg(const char *mac, const char *ip, 
	int mode, int rate,
	int annta0, int annta1, int annta2)
{
	char status[512];

	if (strlen(mac) == 0 || strlen(ip) == 0)
		return -1;

	sprintf(status, "%s", mac);
	sprintf(status, "%s,%s", status, ip);

	if(rate)
		sprintf(status, "%s,%s", status, "40M");
	else
		sprintf(status, "%s,%s", status, "20M");

	if(mode == 0)
		sprintf(status, "%s,%s", status, "CCK");
	else if(mode == 1)
		sprintf(status, "%s,%s", status, "OFDM");
	else
		sprintf(status, "%s,%s", status, "HT");

	sprintf(status, "%s,%d", status, annta0);
	sprintf(status, "%s,%d", status, annta1);
	sprintf(status, "%s,%d", status, annta2);

	printf("%s\n", status);
	return 0;
}


int get_mac_table(const char *iface)
{
	int socket_id;
	struct iwreq wrq;
	int ret;
	RT_802_11_MAC_TABLE data, *pMacEntry;
	
	// open socket based on address family: AF_NET ----------------------------
	socket_id = socket(AF_INET, SOCK_DGRAM, 0);
	if(socket_id < 0)
	{
		printf("\nrtuser::error::Open socket error!\n\n");
		return -1;
	}

	// ioctl
	pMacEntry = &data;
	memset(&data, 0x00, sizeof(RT_802_11_MAC_TABLE));
	strcpy(wrq.ifr_name, iface);
	wrq.u.data.length = sizeof(RT_802_11_MAC_TABLE);
	wrq.u.data.pointer = pMacEntry;
	wrq.u.data.flags = 0;
	ret = ioctl(socket_id, RTPRIV_IOCTL_GET_MAC_TABLE, &wrq);

	printf("\n=======Gat Associated MAC Table========");
	{
		int i;
		printf("\n%-4s%-20s%-6s%-6s%-6s%-6s%-6s%-6s%-6s\n", "AID", "MAC_Address", "MCS", "BW", "SGI", "MODE", "RSSI0", "RSSI1", "RSSI2");
		for(i = 0 ; i < pMacEntry->Num ; i++)
		{
			printf("%-4d", pMacEntry->Entry[i].Aid);
			printf("%02X:%02X:%02X:%02X:%02X:%02X\t",
			pMacEntry->Entry[i].Addr[0], pMacEntry->Entry[i].Addr[1],
			pMacEntry->Entry[i].Addr[2], pMacEntry->Entry[i].Addr[3],
			pMacEntry->Entry[i].Addr[4], pMacEntry->Entry[i].Addr[5]);
			printf("%-6d", pMacEntry->Entry[i].TxRate.field.MCS);
			if(pMacEntry->Entry[i].TxRate.field.BW)
				printf("%-6s","40M");
			else
				printf("%-6s","20M");
			printf("%-6d", pMacEntry->Entry[i].TxRate.field.ShortGI);
			if(pMacEntry->Entry[i].TxRate.field.MODE == 0)
				printf("%-6s", "CCK");
			else if(pMacEntry->Entry[i].TxRate.field.MODE == 1)
				printf("%-6s", "OFDM");
			else
				printf("%-6s", "HT");
			printf("%-6d", pMacEntry->Entry[i].AvgRssi0);
			printf("%-6d", pMacEntry->Entry[i].AvgRssi1);
			printf("%-6d", pMacEntry->Entry[i].AvgRssi2);
			printf("\n");
		}
		printf("\n");			
	}	

	close(socket_id);

	return 0;
}

int get_mtable(const char *iface)
{
	int socket_id, res = 0;
	struct iwreq wrq;
	int ret, lan_ip_cnt = 0, i = 0, j = 0;
	char dhcp_pid[24], lan_ip[1024], 
	     *ptr_lan_ip, *plan_mac, *tmp_ip,
	     wclient_mac[128], tmp_lan[128];
	RT_802_11_MAC_TABLE data, *pMacEntry;
	
	memset(dhcp_pid, '\0', sizeof(dhcp_pid));
	memset(lan_ip, '\0', sizeof(lan_ip));
	memset(wclient_mac, '\0', sizeof(wclient_mac));
	memset(tmp_lan, '\0', sizeof(tmp_lan));

	res = get_dhcpc_pid(dhcp_pid);

	// open socket based on address family: AF_NET

	socket_id = socket(AF_INET, SOCK_DGRAM, 0);
	if(socket_id < 0)
	{
		printf("\nrtuser::error::Open socket error!\n\n");
		return -1;
	}

	// ioctl
	pMacEntry = &data;
	memset(&data, 0x00, sizeof(RT_802_11_MAC_TABLE));
	strcpy(wrq.ifr_name, iface);
	wrq.u.data.length = sizeof(RT_802_11_MAC_TABLE);
	wrq.u.data.pointer = pMacEntry;
	wrq.u.data.flags = 0;
	ret = ioctl(socket_id, RTPRIV_IOCTL_GET_MAC_TABLE, &wrq);

	for ( i = 0; i < pMacEntry->Num; i++) {
	
		sprintf(wclient_mac, "%02X:%02X:%02X:%02X:%02X:%02X",
		pMacEntry->Entry[i].Addr[0], pMacEntry->Entry[i].Addr[1],
		pMacEntry->Entry[i].Addr[2], pMacEntry->Entry[i].Addr[3],
		pMacEntry->Entry[i].Addr[4], pMacEntry->Entry[i].Addr[5]);
	
		if (res == -1) {
			print_msg(wclient_mac, "unknow",
				pMacEntry->Entry[i].TxRate.field.BW,
				pMacEntry->Entry[i].TxRate.field.MODE,
				pMacEntry->Entry[i].AvgRssi0,
				pMacEntry->Entry[i].AvgRssi1,
				pMacEntry->Entry[i].AvgRssi2
			);
		} else {
			get_lan_ip(dhcp_pid, &lan_ip, &lan_ip_cnt);
			ptr_lan_ip = lan_ip;
			for ( j = 0; j < lan_ip_cnt; j++, ptr_lan_ip+=35) {
				sprintf(tmp_lan, "%s", ptr_lan_ip);
				plan_mac = tmp_lan;
				tmp_ip = strsep(&plan_mac, ";");
				if (strcasecmp(plan_mac, wclient_mac) == 0) {
					print_msg(wclient_mac, tmp_ip,
					pMacEntry->Entry[i].TxRate.field.BW,
					pMacEntry->Entry[i].TxRate.field.MODE,
					pMacEntry->Entry[i].AvgRssi0,
					pMacEntry->Entry[i].AvgRssi1,
					pMacEntry->Entry[i].AvgRssi2
					);
				}
			}
		}
		lan_ip_cnt = 0;
		memset(lan_ip, '\0', sizeof(lan_ip));
	}

	close(socket_id);

	return 0;
}

int get_wlan_mac_table(int argc, char *argv[])
{
	if (argc < 2)
		return -1;

	if (argv[2] != NULL && strcmp(argv[2], "mtable") == 0)
		return get_mtable(argv[1]);
	else
		return get_mac_table(argv[1]);
}
