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
#define  RT_PRIV_IOCTL					(SIOCIWFIRSTPRIV + 0x01)
#define	OID_802_11_CURRENTCHANNEL			0x0712

typedef struct _NDIS_802_11_CONFIGURATION_FH
{
   unsigned long           Length;            // Length of structure
   unsigned long           HopPattern;        // As defined by 802.11, MSB set 
   unsigned long           HopSet;            // to one if non-802.11
   unsigned long           DwellTime;         // units are Kusec
} NDIS_802_11_CONFIGURATION_FH, *PNDIS_802_11_CONFIGURATION_FH;

typedef struct _NDIS_802_11_CONFIGURATION
{
   unsigned long                           Length;             // Length of structure
   unsigned long                           BeaconPeriod;       // units are Kusec
   unsigned long                           ATIMWindow;         // units are Kusec
   unsigned long                           DSConfig;           // Frequency, units are kHz
   NDIS_802_11_CONFIGURATION_FH    FHConfig;
} NDIS_802_11_CONFIGURATION, *PNDIS_802_11_CONFIGURATION;

int get_channel(const char *iface)
{
	int socket_id;
	struct iwreq wrq;
	int ret=0;
	unsigned char data = 0;
	//NDIS_802_11_CONFIGURATION pConfig;
	
	// open socket based on address family: AF_NET ----------------------------
	socket_id = socket(AF_INET, SOCK_DGRAM, 0);
	if(socket_id < 0)
	{
		printf("\nrtuser::error::Open socket error!\n\n");
		return -1;
	}

	/////////
	memset(&wrq, 0, sizeof(wrq));
	strcpy(wrq.ifr_name, iface);
	wrq.u.data.length = sizeof(unsigned char);
	wrq.u.data.pointer = &data;
	wrq.u.data.flags = OID_802_11_CURRENTCHANNEL;
	ret = ioctl(socket_id, RT_PRIV_IOCTL ,&wrq);
	if(ret==0)
		printf("Current AP Channel: %ld\n", data);
	else	
		printf("IOCTL fail\n", ret);

	if(socket_id>=0)
		close(socket_id);

	return 1;
}

int _get_channel(const char *iface)
{
	int socket_id;
	struct iwreq wrq;
	int ret=0;
	unsigned char data = 0;

	socket_id = socket(AF_INET, SOCK_DGRAM, 0);
	if(socket_id < 0)
	{
		printf("\nrtuser::error::Open socket error!\n\n");
		return -1;
	}

	memset(&wrq, 0, sizeof(wrq));
	strcpy(wrq.ifr_name, iface);
	wrq.u.data.length = sizeof(unsigned char);
	wrq.u.data.pointer = &data;
	wrq.u.data.flags = OID_802_11_CURRENTCHANNEL;
	ret = ioctl(socket_id, RT_PRIV_IOCTL ,&wrq);
	if(ret==0)
		printf("%ld", data);
	else	
		printf("IOCTL fail\n", ret);

	if(socket_id>=0)
		close(socket_id);

	return 1;
}


int get_wlan_channel(int argc, char *argv[])
{
 	if (argc < 2)
                return -1;
	if (argv[2] != NULL && strcmp(argv[2], "channel") == 0)
		return _get_channel(argv[1]);
	else
		return get_channel(argv[1]);
}
