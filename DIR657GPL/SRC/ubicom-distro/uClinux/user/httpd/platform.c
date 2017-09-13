#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <sys/ioctl.h>

#include <nvram.h>
#include <cmoapi.h>

#define SIOCDEVPRIVATEGETPORTSPEED    (SIOCDEVPRIVATE + 0)
#define SIOCDEVPRIVATESETPORTSPEEDANDDUPLEX    (SIOCDEVPRIVATE + 1)
#define SIOCDEVPRIVATEGETPORTDUPLEX   (SIOCDEVPRIVATE + 2)
#define SIOCDEVPRIVATEGETPORTLINKED   (SIOCDEVPRIVATE + 3)
#define SIOCDEVPRIVATELOCKPORTMAC       (SIOCDEVPRIVATE + 4)
#define SIOCDEVPRIVATELOCKPORTDB       (SIOCDEVPRIVATE + 5)
#define SIOCDEVPRIVATETESTSWITCH      (SIOCDEVPRIVATE + 6)

struct parse_t
{
	int port;   //port number
	int svalue; //set value
	int gvalue; //get value
	unsigned char mac[6];
};

#define WANPORT 4
#define CPU_PORT_NUM 5

int wan_port_type_setting(void)
{
	int sock;
	struct ifreq ifr;
	char *pWantype;
	struct parse_t portstatus;

	portstatus.port = WANPORT;
	portstatus.svalue = 0;
	ifr.ifr_data = (char *)&portstatus;	


	pWantype = nvram_get("wan_port_speed");

	if(strcmp(pWantype, "auto") == 0)
		portstatus.svalue = 0;
	else if (strcmp(pWantype, "10half") == 0)
		portstatus.svalue = 0x00;
	else if (strcmp(pWantype, "10full") == 0)
		portstatus.svalue = 0x01;
	else if (strcmp(pWantype, "100half") == 0)
		portstatus.svalue = 0x20;
	else if (strcmp(pWantype, "100full") == 0)
		portstatus.svalue = 0x21;   	  	  	
	else if (strcmp(pWantype, "1000half") == 0)
		portstatus.svalue = 7;
	else if (strcmp(pWantype, "1000full") == 0)
		portstatus.svalue = 6;

	if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
		return 0;

	strncpy(ifr.ifr_name, nvram_get("wan_eth"), sizeof(ifr.ifr_name));
	if( ioctl(sock, SIOCDEVPRIVATESETPORTSPEEDANDDUPLEX, &ifr) < 0) {
		close(sock);
		return 0;
	}

	close(sock);
	return 1;
}

int get_port_connected_state(char *ifname, int portNum, char *connect_state)
{
	int sock;
	struct ifreq ifr;
	struct parse_t portstatus, *p_port;

	portstatus.port = portNum;
	portstatus.gvalue = 0;
	ifr.ifr_data = (char *)&portstatus;	

	printf("portstatus->port = %d\n",portstatus.port);

	if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
		return 0;

	strncpy(ifr.ifr_name, ifname, sizeof(ifr.ifr_name));
	if( ioctl(sock, SIOCDEVPRIVATEGETPORTLINKED, &ifr) < 0) {
		close(sock);
		return 0;
	}

	p_port = (struct parse_t *)(ifr.ifr_data);

	strcpy(connect_state,(p_port->gvalue == 1)? "connect":"disconnect");

	close(sock);
	return 1;

}

int get_port_current_state(char *ifname, int portNum, char *portSpeed, char *duplexMode)
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
	return 1;
}
