#include "errno.h"
#include <stdio.h>	// for printf() etc.
#include <stdlib.h> 	// for malloc() free() atoi() etc.
#include <string.h>	// for strcpy() memcpy() etc.
#include <sys/socket.h>	/* for "struct sockaddr" et al	*/
#include "wireless.h"

#include <sys/ioctl.h>
#include <unistd.h>

int	global_skfd;
char 	global_extra[8192];
struct iwreq	global_wrq;

/* ---------------------- SOCKET SUBROUTINES -----------------------*/
static int	iw_sockets_open(void);
static void iw_sockets_close(int	skfd);

/*
 * Open a socket.
 * Depending on the protocol present, open the right socket. The socket
 * will allow us to talk to the driver.
 */
static int iw_sockets_open(void)
{
  static const int families[] = {
    AF_INET, AF_IPX, AF_AX25, AF_APPLETALK
  };
  unsigned int	i;
  int		sock;

  /* Try all families we support */
  for(i = 0; i < sizeof(families)/sizeof(int); ++i) {
      /* Try to open the socket, if success returns it */
      sock = socket(families[i], SOCK_DGRAM, 0);
      if(sock >= 0)
		return sock;
  }

  return -1;
}
 
static void iw_sockets_close(int	skfd)
{
  close(skfd);
}

#if WIRELESS_EXT <= 11
#ifndef SIOCDEVPRIVATE
#define SIOCDEVPRIVATE                          0x8BE0
#endif
#ifndef SIOCIWFIRSTPRIV
#define SIOCIWFIRSTPRIV				SIOCDEVPRIVATE
#endif
#endif

/* for AP */
#define RT_PRIV_IOCTL				(SIOCIWFIRSTPRIV + 0x01)
/* for STA */
#define RT_PRIV_STA_IOCTL			(SIOCIWFIRSTPRIV + 0x0E)

//#define UCOS
#define PACKED 
#define UINT32 	unsigned int
#define USHORT 	unsigned short
#define UCHAR	unsigned char
#define CHAR 	char
#define ULONG 	unsigned long


#define    OID_802_11_STATISTICS                0x060E
#define 	RTPRIV_IOCTL_GET_MAC_TABLE 	(SIOCIWFIRSTPRIV + 0x0F)

typedef struct PACKED __RT_MBSS_STAT_ENTRY
{
	UINT32 RxCount;
	UINT32 TxCount;
	UINT32 ReceivedByteCount;
	UINT32 TransmittedByteCount;
	UINT32 RxErrorCount;
	UINT32 RxDropCount;
} RT_MBSS_STAT_ENTRY;

typedef struct PACKED __RT_MBSS_STATISTICS_TABLE
{
	int Num;
	RT_MBSS_STAT_ENTRY	MbssEntry[8];
} RT_MBSS_STATISTICS_TABLE;

#define MAC_ADDR_LENGTH                 6
#define MAX_NUMBER_OF_MAC		128 // if MAX_MBSSID_NUM is 8, this value can't be larger than 211

/* little_endian
typedef union  PACKED _MACHTTRANSMIT_SETTING {
	struct	{
		USHORT   	MCS:7;  // MCS
		USHORT		BW:1;	//channel bandwidth 20MHz or 40 MHz
		USHORT		ShortGI:1;
		USHORT		STBC:2;	//SPACE 
		USHORT		rsv:3;	 
		USHORT		MODE:2;	// Use definition MODE_xxx.  
	} field;

	USHORT word;
} MACHTTRANSMIT_SETTING, *PMACHTTRANSMIT_SETTING;
*/

/* big_endian */
typedef union  PACKED _MACHTTRANSMIT_SETTING {
	struct	{
		USHORT   	MODE:2; //Use definition MODE_xxx
		USHORT		rsv:3;
		USHORT		STBC:2; //SPACE
		USHORT		ShortGI:1;
		USHORT		BW:1;   //Channel bandwidth 20 MHz or 40 MHz
		USHORT		MCS:7;	//MCS
	} field;

	USHORT word;
} MACHTTRANSMIT_SETTING, *PMACHTTRANSMIT_SETTING;

typedef struct _RT_802_11_MAC_ENTRY {
    UCHAR       Addr[MAC_ADDR_LENGTH]; 
    UCHAR       Aid; 
    UCHAR       Psm;     // 0:PWR_ACTIVE, 1:PWR_SAVE
    UCHAR	MimoPs;  // 0:MMPS_STATIC, 1:MMPS_DYNAMIC, 3:MMPS_Enabled
    CHAR	AvgRssi0;
    CHAR	AvgRssi1;
    CHAR	AvgRssi2;
    UINT32	ConnectedTime;
    MACHTTRANSMIT_SETTING	TxRate;
#ifdef UCOS
    MACHTTRANSMIT_SETTING	MaxTxRate;
#endif // UCOS //
    UCHAR	apidx;
} RT_802_11_MAC_ENTRY, *PRT_802_11_MAC_ENTRY;

typedef struct _RT_802_11_MAC_TABLE {
    ULONG       Num;
    RT_802_11_MAC_ENTRY Entry[MAX_NUMBER_OF_MAC];
} RT_802_11_MAC_TABLE, *PRT_802_11_MAC_TABLE;

#define ifr_name        ifr_ifrn.ifrn_name      /* interface name */

static UINT32 convert(UINT32 i)
{
        return  (i >> 24) + ((i & 0x00ff0000) >> 8) + ((i & 0x0000ff00) << 8)
                + (i << 24);
}

static float convert_signal(int rssi)
{
	if ( rssi >= -50)
		return 1.0;
	 else if (rssi <= -51 && rssi >= -80)
		return (24 + (rssi + 80) * 2.6);
	 else if (rssi <= -81 && rssi >= -90)
		return ((rssi + 90) * 2.6);
	 else
		return 0.0;
}

#if 0
static void dump_hex(u_char *p, int len)
{
        int i, off = 0;
        u_char buf[60 * 3 + 1];

        if (len > 60)
                len = 60;
        memset(buf, 0, 60 *3 +1);
        for (i = 0; i < len; i++) {
                off += sprintf(buf + off, "%.2X ", p[i]);
	}
        printf("dump: [%s]\n", buf);
}
#endif

int main(int argc, char *argv[])
{
	int 	skfd;		/* generic raw socket desc. */
	int	i;
	struct iwreq	wrq;
	char 	name[100];
	unsigned int data;
	int 	err;
	char *interface, *type;
	RT_MBSS_STATISTICS_TABLE statistics_table;
	RT_802_11_MAC_TABLE mac_table;

	if (argc < 3) {
		fprintf(stderr, "Usage :: ./stalist interface mac_list|trx_rate\n");
		return 0;
	} else {
		interface = argv[1];
		type = argv[2];
	}

	if((skfd =  socket(AF_INET, SOCK_DGRAM, 0)) < 0)
	{
		printf("\nrtuser::error::Open socket error!\n\n");		
		return(-1);
	}

	if (interface)
		sprintf(name , interface);

	strcpy(wrq.ifr_name, name);

	/* get OID_802_11_STATISTICS. USE OID method */
	if ( strcmp(type, "trx_rate") == 0) {
		wrq.u.data.length = sizeof(RT_MBSS_STATISTICS_TABLE);
		wrq.u.data.pointer = &statistics_table;
		wrq.u.data.flags = OID_802_11_STATISTICS;
		err = ioctl(skfd, RT_PRIV_IOCTL, &wrq);

		statistics_table.Num = convert(statistics_table.Num);
		for (i = 0; i < statistics_table.Num; i++) {
			printf("RxCount: %d\n", convert(statistics_table.MbssEntry[i].RxCount));
			printf("TxCount: %d\n", convert(statistics_table.MbssEntry[i].TxCount));
			printf("ReceivedByteCount: %d\n", convert(statistics_table.MbssEntry[i].ReceivedByteCount));
			printf("TransmittedByteCount: %d\n", convert(statistics_table.MbssEntry[i].TransmittedByteCount));
			printf("RxErrorCount: %d\n", convert(statistics_table.MbssEntry[i].RxErrorCount));
			printf("RxDropCount: %d\n", convert(statistics_table.MbssEntry[i].RxDropCount));
			printf("\n");
		}
	}

	/* RTPRIV_IOCTL_GET_MAC_TABLE. USE iwpriv commadn */
	if ( strcmp(type, "mac_list") == 0){
		wrq.u.data.length = sizeof(RT_802_11_MAC_TABLE);
		wrq.u.data.pointer = &mac_table;
		wrq.u.data.flags = 0;
		err = ioctl(skfd, RTPRIV_IOCTL_GET_MAC_TABLE, &wrq);

		mac_table.Num = convert(mac_table.Num);
		for(i = 0; i < mac_table.Num; i++) {
			int rssi0 = 0, rssi1 = 0, rssi2 = 0;
			char tmp[24];
			printf("MAC %02x%02x%02x%02x%02x%02x\n", 
				mac_table.Entry[i].Addr[0],mac_table.Entry[i].Addr[1],
				mac_table.Entry[i].Addr[2],mac_table.Entry[i].Addr[3],
				mac_table.Entry[i].Addr[4],mac_table.Entry[i].Addr[5]);
			
			sprintf(tmp, "%u", mac_table.Entry[i].AvgRssi0);
			rssi0 = atoi(tmp) - 255;
			sprintf(tmp, "%u", mac_table.Entry[i].AvgRssi1);
			rssi1 = atoi(tmp) - 255;
			sprintf(tmp, "%u", mac_table.Entry[i].AvgRssi2);
			rssi2 = atoi(tmp) -255;
/*
			printf("AvgRssi0 :: %u\n", mac_table.Entry[i].AvgRssi0);
			printf("AvgRssi1 :: %u\n", mac_table.Entry[i].AvgRssi1);
			printf("AvgRssi2 :: %u\n", mac_table.Entry[i].AvgRssi2);
*/
			printf("AvgRssi0 :: %.2f\n", convert_signal(rssi0)*100);
			printf("AvgRssi1 :: %.2f\n", convert_signal(rssi1)*100);
			printf("AvgRssi2 :: %.2f\n", convert_signal(rssi2)*100);
			printf("ConnectedTime :: %d\n", convert(mac_table.Entry[i].ConnectedTime));
			printf("TxRate MODE :: %d\n", mac_table.Entry[i].TxRate.field.MODE);
			printf("TxRate rsv :: %d\n", mac_table.Entry[i].TxRate.field.rsv);
			printf("TxRate STBC :: %d\n", mac_table.Entry[i].TxRate.field.STBC);
			printf("TxRate ShortGI :: %d\n", mac_table.Entry[i].TxRate.field.ShortGI);
			printf("TxRate BW :: %d\n", mac_table.Entry[i].TxRate.field.BW);
			printf("TxRate MCS :: %d\n", mac_table.Entry[i].TxRate.field.MCS);
		}	

	}
	/* Close the socket. */
	iw_sockets_close(skfd);
	return 0;
}

