#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <vct.h>

/* only support for wan port */
int setPortSpeed(char *ifname, int  portspeed)
{
	int sock;
	struct ifreq ifr;
	struct parse_t portstatus;

	portstatus.port = WAN_PORT;
	portstatus.svalue = 0;
	ifr.ifr_data = (char *)&portstatus;	
	portstatus.svalue = portspeed;

	if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
		return -1;
	strncpy(ifr.ifr_name, ifname, sizeof(ifr.ifr_name));
	if( ioctl(sock, SIOCDEVPRIVATESETPORTSPEEDANDDUPLEX, &ifr) < 0) {
		close(sock);
		return -1;
	}

	close(sock);
	return 0;
}


/* return 1 if connected else return 0 */
int getPortConnectState(char *ifname, int portNum)
{
	int sock;
	struct ifreq ifr;
	struct parse_t portstatus, *p_port;
	portstatus.port = portNum;
	portstatus.gvalue = 0;
	ifr.ifr_data = (char *)&portstatus;	
	
	if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
	{
		printf("getPortConnectState: create socket error\n");
		return 0;
	}

	strncpy(ifr.ifr_name, ifname, sizeof(ifr.ifr_name));
	if( ioctl(sock, SIOCDEVPRIVATEGETPORTLINKED, &ifr) < 0) 
	{
		printf("getPortConnectState: SIOCDEVPRIVATEGETPORTLINKED error\n");
		close(sock);
		return 0;
	}

	p_port = (struct parse_t *)(ifr.ifr_data);

	close(sock);

	return p_port->gvalue;

}


int getPortSpeedState(char *ifname, int portNum, char *portSpeed, char *duplexMode)
{
	int sock;
	struct ifreq ifr;
	struct parse_t portstatus, *p_port;

	portstatus.port = portNum;

	ifr.ifr_data = (char *)&portstatus;	


	if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
		return 0;

	strncpy(ifr.ifr_name, ifname, sizeof(ifr.ifr_name));
	if( ioctl(sock, SIOCDEVPRIVATEGETPORTSPEED, &ifr) < 0) {
		close(sock);
		return 0;
	}
	p_port = (struct parse_t *)(ifr.ifr_data);
	if(p_port->gvalue == 0)          /* 10 Mbps */
		strcpy(portSpeed,"10");
	else if(p_port->gvalue == 1)     /* 100 Mbps */
		strcpy(portSpeed,"100");
	else                             /* 1000 Mbps */
		strcpy(portSpeed,"1000");


	if( ioctl(sock, SIOCDEVPRIVATEGETPORTDUPLEX, &ifr) < 0) {
		close(sock);
		return 0;
	}

	p_port = (struct parse_t *)(ifr.ifr_data);

	strcpy(duplexMode, (p_port->gvalue == 1)? "full":"half");

	close(sock);
	return 0;
}

