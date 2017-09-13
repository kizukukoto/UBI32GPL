#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <unistd.h> /* for close */
#include <linux/wireless.h>

#include "cmds.h"

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
	struct 
	{
		unsigned short MCS:7; // MCS
		unsigned short BW:1; //channel bandwidth 20MHz or 40 MHz
		unsigned short ShortGI:1;
		unsigned short STBC:2; //SPACE
		unsigned short rsv:3;
		unsigned short MODE:2; // Use definition MODE_xxx.
	
	}field;
	unsigned short word;
} MACHTTRANSMIT_SETTING, *PMACHTTRANSMIT_SETTING;


typedef struct _RT_802_11_MAC_ENTRY {
    unsigned char       Addr[MAC_ADDR_LEN];
    unsigned char       Aid;
    unsigned char       Psm;     // 0:PWR_ACTIVE, 1:PWR_SAVE
    unsigned char  	MimoPs;  // 0:MMPS_STATIC, 1:MMPS_DYNAMIC, 3:MMPS_Enabled
    char		AvgRssi0;
    char		AvgRssi1;
    char		AvgRssi2;
    unsigned int		ConnectedTime;
    MACHTTRANSMIT_SETTING	TxRate;
    MACHTTRANSMIT_SETTING	MaxTxRate;
    unsigned char		apidx;
} RT_802_11_MAC_ENTRY, *PRT_802_11_MAC_ENTRY;

typedef struct _RT_802_11_MAC_TABLE {
    unsigned long       Num;
    RT_802_11_MAC_ENTRY Entry[MAX_LEN_OF_MAC_TABLE];
} RT_802_11_MAC_TABLE, *PRT_802_11_MAC_TABLE;

int utility_wclient_main(int argc, char** argv)
{

	char name[25];
	char data[sizeof(RT_802_11_MAC_TABLE)];
	int socket_id, ret = -1;
	struct iwreq wrq;

	socket_id = socket(AF_INET,SOCK_DGRAM,0);
	if(socket_id < 0) {
		err_msg("rtuser::error::Open Socket error\n");
		goto out;
	}
	/* get_mac_table */
	sprintf(name, "ra0");
	memset(data, 0x00, sizeof(RT_802_11_MAC_TABLE));
	strcpy(data, "get_mac_table");
	strcpy(wrq.ifr_name, name);
	wrq.u.data.length = sizeof(RT_802_11_MAC_TABLE);
	wrq.u.data.pointer = data;
	wrq.u.data.flags = 0;
	
	ret = ioctl(socket_id, RTPRIV_IOCTL_GET_MAC_TABLE, &wrq);
	if(ret != 0){
		err_msg("rtuser::error::get mac table\n");
		goto out;
	}
	
	//printf("\n=======Gat Associated MAC Table========");
	{
		RT_802_11_MAC_TABLE *mp;
		int i;
		mp=(RT_802_11_MAC_TABLE*)wrq.u.data.pointer;
		//printf("\n%-4s%-20s%-4s%-10s%-10s%-10s\n", "AID", "MAC_Address", "PSM", "LastTime", "RxByte", "TxByte");
		
		for(i = 0 ; i < mp->Num ; i++)
		{
			//printf("%-4d", mp->Entry[i].Aid);
			printf("%02X:%02X:%02X:%02X:%02X:%02X",
			mp->Entry[i].Addr[0], mp->Entry[i].Addr[1],
			mp->Entry[i].Addr[2], mp->Entry[i].Addr[3],
			mp->Entry[i].Addr[4], mp->Entry[i].Addr[5]);
			//printf("  %-4d", mp->Entry[i].Psm);
			printf("/%us", mp->Entry[i].ConnectedTime);
			//printf("%-10u", (unsigned int)mp->Entry[i].HSCounter.LastDataPacketTime);
			//printf("%-10u", (unsigned int)mp->Entry[i].HSCounter.TotalRxByteCount);
			//printf("%-10u", (unsigned int)mp->Entry[i].HSCounter.TotalTxByteCount);
			printf(",");
		}
		//printf("\n");			
		
	}	
	ret = 0;
	out:
		if(socket_id>=0)
			close(socket_id);
		return ret;
}
