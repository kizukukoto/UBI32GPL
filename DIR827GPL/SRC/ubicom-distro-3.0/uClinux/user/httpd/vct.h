#define SIOCDEVPRIVATEGETPORTSPEED	  (SIOCDEVPRIVATE + 0)
#define SIOCDEVPRIVATESETPORTSPEEDANDDUPLEX	  (SIOCDEVPRIVATE + 1)
#define SIOCDEVPRIVATEGETPORTDUPLEX	  (SIOCDEVPRIVATE + 2)
#define SIOCDEVPRIVATEGETPORTLINKED	  (SIOCDEVPRIVATE + 3)
#define SIOCDEVPRIVATELOCKPORTMAC	    (SIOCDEVPRIVATE + 4)
#define SIOCDEVPRIVATELOCKPORTDB			(SIOCDEVPRIVATE + 5)
#define SIOCDEVPRIVATENETSTATS      (SIOCDEVPRIVATE + 6)
//extern GT_QD_DEV* qd_dev;

struct parse_t
{
	int port;   //port number
	int svalue; //set value
	int gvalue; //get value
	unsigned char mac[6];
};

#define CPU_PORT_NUM 5

int setPortSpeed(char *ifname, int  portspeed);
int getPortConnectState(char *ifname, int portNum);
int getPortSpeedState(char *ifname, int portNum, char *portSpeed, char *duplexMode);


/* below define for compact */
#define VCTSETPORTSPEED_AUTO		0x10
#define VCTSETPORTSPEED_10HALF	0x80
#define VCTSETPORTSPEED_10FULL	0x81
#define VCTSETPORTSPEED_100HALF	0xa0
#define VCTSETPORTSPEED_100FULL	0xa1
#define VCTSETPORTSPEED_1000HALF	7
#define VCTSETPORTSPEED_1000FULL	6

#define VCTGETPORTSPEED_10			0
#define VCTGETPORTSPEED_100		1
#define VCTGETPORTSPEED_1000		2


#ifdef DIR635G
#define WAN_PORT		5
#define LAN_PORT0		7
#define LAN_PORT1		0
#define LAN_PORT2		1
#define LAN_PORT3		2
#else
#define WAN_PORT		4
#define LAN_PORT0		3
#define LAN_PORT1		2
#define LAN_PORT2		1
#define LAN_PORT3		0
#endif

