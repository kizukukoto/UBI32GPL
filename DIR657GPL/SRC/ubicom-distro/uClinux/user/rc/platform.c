#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <sys/ioctl.h>

#include <nvram.h>
#include <rc.h>

#ifdef RC_DEBUG
#define DEBUG_MSG(fmt, arg...)       printf("%s [%d]: "fmt, __FILE__, __LINE__, ##arg)
#else
#define DEBUG_MSG(fmt, arg...) 
#endif

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
	char *wan_eth;

		portstatus.port = WANPORT;
		portstatus.svalue = 0;
		ifr.ifr_data = &portstatus;	
	
	wan_eth = nvram_safe_get("wan_eth");	
	pWantype = nvram_safe_get("wan_port_speed");

  if(strcmp(pWantype, "auto") == 0)
  	portstatus.svalue = 0;
  else if (strcmp(pWantype, "10half") == 0)
  	portstatus.svalue = 3;
  else if (strcmp(pWantype, "10full") == 0)
  	portstatus.svalue = 3;
  else if (strcmp(pWantype, "100half") == 0)
  	portstatus.svalue = 2;
  else if (strcmp(pWantype, "100full") == 0)
  	portstatus.svalue = 2;   	  	  	
  		
	if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
		return 0;

	strncpy(ifr.ifr_name, wan_eth, sizeof(ifr.ifr_name));
	if( ioctl(sock, SIOCDEVPRIVATESETPORTSPEEDANDDUPLEX, &ifr) < 0) {
		close(sock);
		return 0;
	}

	close(sock);
	return 1;
}

