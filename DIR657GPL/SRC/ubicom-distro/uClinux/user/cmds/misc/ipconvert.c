#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define END_TUNNEL	        1	
#define SRC_LAN_IP      	2		
#define SRC_LAN_NETMASK     	3	
#define SRC_GATEWAY	     	4	
#define DST_LAN_IP              5
#define DST_LAN_NETMASK        	6
#define DST_GATEWAY	 	7	
#define SRC_WAN_IP	        8	
#define DST_WAN_IP 	        9	

static char szTmpBuf[128+1];
static int convertIP2Hex(char *IPstr)
{
	char *str1;
	unsigned long IP_value = 0;

	strcpy(szTmpBuf, IPstr);
	str1 = strtok(szTmpBuf, ".");
	IP_value = atoi(str1);

	while (1)
	{
		str1 = strtok(NULL, ".");
		if (str1 == NULL)
			break;
		IP_value = atoi(str1) + IP_value * 256;
	}
	printf("%lu ",IP_value);
//	printf("%x ",IP_value);
	return 0;
}

int ipconvert_main(int argc, char *argv[])
{
	if (argc < 10)
	{
		printf("usage: IPconvert {0|1} {src LAN IP} {src LAN netmask} {src gw IP} {dst LAN IP} {dst LAN netmask} {dst gw IP} {src WAN IP} {dst WAN IP}\r\n");
		exit(-1);
	}

	printf("1 0 ");
	convertIP2Hex(argv[SRC_LAN_IP]);
        convertIP2Hex(argv[SRC_LAN_NETMASK]);	
        convertIP2Hex(argv[SRC_GATEWAY]);
        convertIP2Hex(argv[DST_LAN_IP]);
        convertIP2Hex(argv[DST_LAN_NETMASK]);
        convertIP2Hex(argv[SRC_WAN_IP]);
        convertIP2Hex(argv[DST_WAN_IP]);

	printf("1 1 ");
	convertIP2Hex(argv[DST_LAN_IP]);
        convertIP2Hex(argv[DST_LAN_NETMASK]);
        convertIP2Hex(argv[DST_GATEWAY]);
        convertIP2Hex(argv[SRC_LAN_IP]);
        convertIP2Hex(argv[SRC_LAN_NETMASK]);
        convertIP2Hex(argv[DST_WAN_IP]);
        convertIP2Hex(argv[SRC_WAN_IP]);

//	if (atoi(argv[END_TUNNEL]))
//		printf("\r\n");
	return 0;
	
}

