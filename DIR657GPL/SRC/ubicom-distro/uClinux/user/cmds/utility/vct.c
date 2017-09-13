#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <net/if_arp.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#if 1//set/get port status
#include <net/if.h>
#endif
#include "cmds.h"
#if 1//set/get port status
#define SIOCDEVPRIVATEGETPORTSPEED    (SIOCDEVPRIVATE + 1)
#define SIOCDEVPRIVATEGETPORTDUPLEX   (SIOCDEVPRIVATE + 2)
#define SIOCDEVPRIVATEGETPORTLINKED   (SIOCDEVPRIVATE + 3)
#define SIOCDEVPRIVATESETPORTSPEEDANDDUPLEX    (SIOCDEVPRIVATE + 7)

#define SETPORTSPEED_AUTO		0x10
#define SETPORTSPEED_10HALF		0x20
#define SETPORTSPEED_10FULL		0x21
#define SETPORTSPEED_100HALF	0x30
#define SETPORTSPEED_100FULL	0x31
#define SETPORTSPEED_1000FULL	0x41

#define GMAC_SPEED_10			0
#define GMAC_SPEED_100			1
#define GMAC_SPEED_1000			2

enum {
	DMZPORT0,//eth2
	LANPORT1,//eth0
	LANPORT2,//eth0
	LANPORT3,//eth0
	LANPORT4,//eth0
	WANPORT0//eth1
};

struct parse_t
{
	int port;   //port number
	int svalue; //set value
	int gvalue; //get value
	unsigned char mac[6];
};

/* only support for wan port */
static int setPortSpeed(char *ifname, int  portspeed)
{
	int sock;
	struct ifreq ifr;
	struct parse_t portstatus;
	
	//printf("setPortSpeed:\n");
	portstatus.port = 0;
	portstatus.gvalue = 0;	
	portstatus.svalue = 0;	
	
	portstatus.port = WANPORT0;
		
	portstatus.gvalue = 0;	
	portstatus.svalue = 0;
	ifr.ifr_data = (char *)&portstatus;	
	portstatus.svalue = portspeed;

	if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0){
		printf("setPortSpeed socket fail\n");
		return -1;
	}	
	strncpy(ifr.ifr_name, ifname, sizeof(ifr.ifr_name));
	if( ioctl(sock, SIOCDEVPRIVATESETPORTSPEEDANDDUPLEX, &ifr) < 0) {
		close(sock);
		printf("setPortSpeed ioctl fail\n");
		return -1;
	}

	close(sock);
	return 0;
}

/* return 1 if connected else return 0 */
static int getPortConnectState(char *ifname, int portNum, char *connect_State)
{
	int sock;
	struct ifreq ifr;
	struct parse_t portstatus, *p_port;
	
	//printf("getPortConnectState:\n");
	//if(!strcmp(ifname,"eth2")){
		//printf("getPortConnectState: change to eth0\n");
		//ifname="eth0";
	//}	
	portstatus.port = 0;
	portstatus.gvalue = 0;	
	portstatus.svalue = 0;
	
	portstatus.port = portNum;	
	ifr.ifr_data = (char *)&portstatus;	

	if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0){
		printf("getPortConnectState socket fail\n");
		return 0;
	}	

	strncpy(ifr.ifr_name, ifname, sizeof(ifr.ifr_name));
	if( ioctl(sock, SIOCDEVPRIVATEGETPORTLINKED, &ifr) < 0) {
		close(sock);
		printf("getPortConnectState ioctl fail\n");
		return 0;
	}

	p_port = (struct parse_t *)(ifr.ifr_data);

	close(sock);
	printf("connect gvalue=%x\n",p_port->gvalue);
	
	if(p_port->gvalue == 1){
		strcpy(connect_State,"connect");			
	}	
	else{ 
		strcpy(connect_State,"disconnect");		
	}	
	printf("connect_State=%s\n",connect_State);	

	return 0;
}

static int getPortSpeedState(char *ifname, int portNum, char *portSpeed, char *duplexMode)
{
	int sock;
	struct ifreq ifr;
	struct parse_t portstatus, *p_port;
	
	//printf("getPortSpeedState:\n");
	//if(!strcmp(ifname,"eth2")){
		//printf("getPortSpeedState: change to eth0\n");
		//ifname="eth0";
	//}	
	portstatus.port = 0;
	portstatus.gvalue = 0;	
	portstatus.svalue = 0;

	portstatus.port = portNum;
	ifr.ifr_data = (char *)&portstatus;

	if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0){
		printf("getPortSpeedState socket fail\n");
		return 0;
	}	
	//speed
	strncpy(ifr.ifr_name, ifname, sizeof(ifr.ifr_name));
	if( ioctl(sock, SIOCDEVPRIVATEGETPORTSPEED, &ifr) < 0) {
		close(sock);
		printf("getPortSpeedState socket fail\n");
		return 0;
	}
	p_port = (struct parse_t *)(ifr.ifr_data);
	//printf("speed gvalue=%x\n",p_port->gvalue);
	if(p_port->gvalue == GMAC_SPEED_10)          /* 10 Mbps */
		strcpy(portSpeed,"10");
	else if(p_port->gvalue == GMAC_SPEED_100)    /* 100 Mbps */
		strcpy(portSpeed,"100");
	else if(p_port->gvalue == GMAC_SPEED_1000)   /* 1000 Mbps */
		strcpy(portSpeed,"1000");
	else{
		//printf("getPortSpeedState speed fail\n");
		strcpy(portSpeed,"10");
	}	
	//printf("portSpeed=%s\n",portSpeed);
	//duplex
	if( ioctl(sock, SIOCDEVPRIVATEGETPORTDUPLEX, &ifr) < 0) {
		close(sock);
		//printf("getPortSpeedState socket fail\n");
		return 0;
	}	
	p_port = (struct parse_t *)(ifr.ifr_data);
	//printf("duplex gvalue=%x\n",p_port->gvalue);
	strcpy(duplexMode, (p_port->gvalue == 1)? "full":"half");
	//printf("duplexMode=%s\n",duplexMode);
	close(sock);
	return 0;
}


static int VctSetPortSpeed(char *ifname, char *portspeed)
{
	int ret = -1;
	int speedmode= -1;
	
	//printf("VctSetPortSpeed\n");	
	if( !strcmp(portspeed, "auto"))
		speedmode =  SETPORTSPEED_AUTO;
	if( !strcmp(portspeed, "10half"))
		speedmode =  SETPORTSPEED_10HALF;
	if( !strcmp(portspeed, "10full"))
		speedmode =  SETPORTSPEED_10FULL;
	if( !strcmp(portspeed, "100half"))
		speedmode =  SETPORTSPEED_100HALF;
	if( !strcmp(portspeed, "100full"))
		speedmode =  SETPORTSPEED_100FULL;
	if( !strcmp(portspeed, "giga"))
		speedmode =  SETPORTSPEED_1000FULL;

	if( speedmode >=0)
		ret = setPortSpeed(ifname, speedmode);
	else
		printf("SetPortSpeed:speedmode fail\n");		
	return ret;
}

static int VctGetPortConnectState(char *ifname, int portNum, char *connect_State)
{
	int state;
	int port=0;
	printf("VctGetPortConnectState:ifname=%s,portNum=%d\n",ifname,portNum);	
	switch(portNum){
		case DMZPORT0:
			port = DMZPORT0;
			break;
		case LANPORT1://3
			port = LANPORT1;
			break;
		case LANPORT2://2
			port = LANPORT2;
			break;
		case LANPORT3://1
			port = LANPORT3;
			break;
		case LANPORT4://0
			port = LANPORT4;
			break;
		case WANPORT0:
			port = WANPORT0;	
			break;
		default:
			printf("portNum=%d fail\n",portNum);
		return 0;
	}
	
	return getPortConnectState(ifname, port, connect_State);	
}

static int VctGetPortSpeedState(char *ifname, int portNum, char *portSpeed, char *duplexMode)
{
	int port=0;
	//printf("VctGetPortSpeedState:ifname=%s,portNum=%d\n",ifname,portNum);	
	switch(portNum){
		case DMZPORT0:
			port = DMZPORT0;
			break;
		case LANPORT1://3
			port = LANPORT1;
			break;
		case LANPORT2://2
			port = LANPORT2;
			break;
		case LANPORT3://1
			port = LANPORT3;
			break;
		case LANPORT4://0
			port = LANPORT4;
			break;
		case WANPORT0:
			port = WANPORT0;	
			break;
		default:
			printf("portNum=%d fail\n",portNum);
		return -1;
	}
	
	return getPortSpeedState(ifname, port, portSpeed, duplexMode);
}

static int speed_main(int argc, char *argv[])
{
	int rev = -1;
	
	if (argc < 2)
		printf("valid arguments: [10full | 100full | giga | auto]\n");
	else
		rev = VctSetPortSpeed("eth1", argv[1]);
	
	return rev;
}
static int status_main(int argc, char *argv[])
{
	int rev = -1, i;
	char portSpeed[128], duplex[128];

	/* 5:WAN 0:DMZ 1:LAN1, 2:LAN2, 3:LAN3, 4:LAN4 */
	for (i = 0; i <= 5; i++) {
		if (VctGetPortSpeedState("eth1", i, portSpeed, duplex) == -1)
			continue;
		printf("port %d, speed: %s, duplex:%s\n", i, portSpeed, duplex);
	}
	return rev;
}

int vct_main(int argc, char *argv[])
{
	struct subcmd cmds[] = {
		{ "speed", speed_main, "set speed of ports WAN"},
		{ "status", status_main, "get status of ports WAN/LANs/DMZ"},
		{ NULL, NULL}
	};
	return lookup_subcmd_then_callout(argc, argv, cmds);
}

#endif
