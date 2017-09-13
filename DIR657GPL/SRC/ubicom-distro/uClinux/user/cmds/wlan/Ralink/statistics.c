#include "errno.h"
#include <stdio.h>	// for printf() etc.
#include <stdlib.h> 	// for malloc() free() atoi() etc.
#include <string.h>	// for strcpy() memcpy() etc.
#include <sys/socket.h>	/* for "struct sockaddr" et al	*/
#include "wireless.h"

#include <sys/ioctl.h>
#include <unistd.h>

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

#define ifr_name        ifr_ifrn.ifrn_name      /* interface name */

static UINT32 convert(UINT32 i)
{
        return  (i >> 24) + ((i & 0x00ff0000) >> 8) + ((i & 0x0000ff00) << 8)
                + (i << 24);
}

int _get_wlan_statistics(const char *iface)
{
	int 	skfd;		/* generic raw socket desc. */
	int	i;
	struct iwreq	wrq;
	int 	err;

	RT_MBSS_STATISTICS_TABLE statistics_table;

	if((skfd =  socket(AF_INET, SOCK_DGRAM, 0)) < 0)
	{
		printf("\nrtuser::error::Open socket error!\n\n");		
		return -1;
	}

	strcpy(wrq.ifr_name, iface);

	/* get OID_802_11_STATISTICS. USE OID method */
	wrq.u.data.length = sizeof(RT_MBSS_STATISTICS_TABLE);
	wrq.u.data.pointer = &statistics_table;
	wrq.u.data.flags = OID_802_11_STATISTICS;
	err = ioctl(skfd, RT_PRIV_IOCTL, &wrq);

	statistics_table.Num = convert(statistics_table.Num);
	for (i = 0; i < statistics_table.Num; i++) {
		printf("RxCount: %u\n", convert(statistics_table.MbssEntry[i].RxCount));
		printf("TxCount: %u\n", convert(statistics_table.MbssEntry[i].TxCount));
		printf("ReceivedByteCount: %u\n", convert(statistics_table.MbssEntry[i].ReceivedByteCount));
		printf("TransmittedByteCount: %u\n", convert(statistics_table.MbssEntry[i].TransmittedByteCount));
		printf("RxErrorCount: %u\n", convert(statistics_table.MbssEntry[i].RxErrorCount));
		printf("RxDropCount: %u\n", convert(statistics_table.MbssEntry[i].RxDropCount));
		printf("\n");
	}

	/* Close the socket. */
  	close(skfd);
	return 0;
}

int get_wstatistics(const char *iface)
{
	int 	skfd;		/* generic raw socket desc. */
	int	i;
	struct iwreq	wrq;
	int 	err;

	RT_MBSS_STATISTICS_TABLE statistics_table;

	if((skfd =  socket(AF_INET, SOCK_DGRAM, 0)) < 0)
	{
		printf("\nrtuser::error::Open socket error!\n\n");		
		return -1;
	}

	strcpy(wrq.ifr_name, iface);

	/* get OID_802_11_STATISTICS. USE OID method */
	wrq.u.data.length = sizeof(RT_MBSS_STATISTICS_TABLE);
	wrq.u.data.pointer = &statistics_table;
	wrq.u.data.flags = OID_802_11_STATISTICS;
	err = ioctl(skfd, RT_PRIV_IOCTL, &wrq);

	/*RxCount,TxCount,RxByteCount,TxByteCount,RxErrCount,RXDropCount*/
	statistics_table.Num = convert(statistics_table.Num);
	for (i = 0; i < statistics_table.Num; i++) {
		printf("%u,%u,%u,%u,%u,%u\n", 
			convert(statistics_table.MbssEntry[i].RxCount),
			convert(statistics_table.MbssEntry[i].TxCount),
			convert(statistics_table.MbssEntry[i].ReceivedByteCount),
			convert(statistics_table.MbssEntry[i].TransmittedByteCount),
			convert(statistics_table.MbssEntry[i].RxErrorCount),
			convert(statistics_table.MbssEntry[i].RxDropCount));
	}

	/* Close the socket. */
  	close(skfd);
	return 0;
}

int get_wlan_statistics(int argc, char *argv[])
{
	if (argc < 2)
		return -1;

	if (argv[2] != NULL && strcmp(argv[2], "statistics") == 0)
		return get_wstatistics(argv[1]);
	else
		return _get_wlan_statistics(argv[1]);
}
