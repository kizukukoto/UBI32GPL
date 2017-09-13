
#include <ctype.h>
#include <errno.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <nvram.h>

unsigned char lan_mac_addr[8], wan_mac_addr[8];
char lan_mac[32], wan_mac[32];

int hexit( char ch)
{
    int val;

    if ( isxdigit( ch) == 0)
        return  -1;

    if ( isdigit( ch))
        val = (int)ch - (int)'0';
    else
        val = toupper(ch) - (int)'A' + 10;
    
    return  val;
}

int parse_macaddr_argv(unsigned char mac_addr[], char *mac_str)
{
    int i, val;

    if (strlen(mac_str) != 12)
        return  -1;

    for ( i=0; i<12; i++)
    {
        if ( ( val = hexit( mac_str[i])) == -1)
            return  -1;

        if ( ( i % 2) == 0)
            val = val * 16;
        
        mac_addr[i/2] += val;
    }

    return  0;
}

static unsigned long conver_to_interger(char *src)
{
        unsigned long val =0;
        int base =16;
        unsigned char c;

        c = *src;
        for (;;)
        {
                if (isdigit(c))
                {
                        val = (val * base) + (c - '0');
                        c = *++src;
                }
                else if (isxdigit(toupper(c)))
                {
                        val = (val << 4) | (toupper(c) + 10 - 'A');
                        c = *++src;
                }
                else
                        break;
        }

        return (val);
}

static int add_one_to_macaddr(char *mac_str)
{
    unsigned long mac_number[3] ={0};
    int i;
    char temp[5], mac_temp[32];

    for (i= 0 ; i<3 ; i++) {
        memset(temp, 0, 5);
        memcpy(temp, mac_str+i*4 , 4);
        mac_number[i] = conver_to_interger(temp);
    }

    mac_number[2] = mac_number[2] + 1;
    if ((mac_number[2]) > 65535) {
        mac_number[2] = 0;
        mac_number[1] = mac_number[1] + 1;
        if ((mac_number[1]) > 65535) {
            mac_number[1] = 0;
            mac_number[0] = mac_number[0] + 1;
            if ((mac_number[0]) > 65535) {
                mac_number[2] = 0;
                mac_number[1] = 0;
                mac_number[0] = 0;
            }
        }
    }
    
    sprintf((char *)mac_temp, "%04x%04x%04x", mac_number[0], mac_number[1], mac_number[2]);
    return (parse_macaddr_argv(wan_mac_addr, mac_temp));
}

int main(int argc, char *argv[])
{
	if ((argc == 3) && (strncmp(argv[1], "-lmac", 5) == 0)) {
		if (parse_macaddr_argv(lan_mac_addr, argv[2]) == -1) {
			printf("Ex: smac -lmac 001122334455\n");
			goto __exit;
		}	

		if (add_one_to_macaddr(argv[2]) == -1)
			goto __exit;
	
		sprintf(lan_mac, "%02x:%02x:%02x:%02x:%02x:%02x",
			*(lan_mac_addr + 0), *(lan_mac_addr + 1), *(lan_mac_addr + 2),
			*(lan_mac_addr + 3), *(lan_mac_addr + 4), *(lan_mac_addr + 5));
		printf("lan_mac = %s\n", lan_mac);

		sprintf(wan_mac, "%02x:%02x:%02x:%02x:%02x:%02x",
			*(wan_mac_addr + 0), *(wan_mac_addr + 1), *(wan_mac_addr + 2),
			*(wan_mac_addr + 3), *(wan_mac_addr + 4), *(wan_mac_addr + 5));
		printf("wan_mac = %s\n", wan_mac);

#ifdef SET_GET_FROM_ARTBLOCK
		artblock_set("lan_mac", lan_mac);
		artblock_set("wan_mac", wan_mac);
#endif

		nvram_set("lan_mac", lan_mac);
		nvram_set("wan_mac", wan_mac);
		nvram_commit();
	}

__exit:
    exit(1);
    return 0;
} /* main() */


