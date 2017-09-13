#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <nvram.h>
#include <sutil.h>

void init_wlan0_mac_to_nvram(void)
{
        /*
         * Read WLAN MAC from the wireless card and save it in nvram. We are not saving "lan_mac"
         * read from nvram as wlan0 MAC, because set_wlan_mac() may fail to set "lan_mac" as the new WLAN MAC.
         * So, we are always saving the actual MAC on the card to nvram.
         * Thus, the correct MAC will be displayed at status page
         */
        FILE *in;
        char wlan0_mac_on_card[18];

        if (!(in = popen("ifconfig ath0 | grep HWaddr | awk ' {print $5} '", "r"))) {
                exit(1);
        }

        fgets(wlan0_mac_on_card, sizeof(wlan0_mac_on_card), in);

        /* close the pipe */
        pclose(in);

        nvram_set("wlan0_mac", wlan0_mac_on_card);
}


static int get_wlan_mac(char *mac)
{
	char *lan_mac = NULL;
#ifdef SET_GET_FROM_ARTBLOCK
        lan_mac = artblock_get("lan_mac");
        if(lan_mac == NULL)
                lan_mac = nvram_safe_get("lan_mac");
/*
        wan_mac = artblock_get("wan_mac");
        if(wan_mac == NULL)
                wan_mac = nvram_safe_get("wan_mac");
*/
#else
        lan_mac = nvram_safe_get("lan_mac");
//        wan_mac = nvram_safe_get("wan_mac");
#endif
	strcpy(mac, lan_mac);
	return 0;
}

/* Date:2010/11/18 Robert
 * # It seems that we do not need wan mac.
 * # So I remove wan mac setting.
 */
void set_wlan_mac(void)
{
        char *p_lmac=NULL, p_lan_mac[18], wlan0_mac[13];
        int  lan_mac[6]={0};

	get_wlan_mac(p_lan_mac);
	p_lmac = p_lan_mac;
        sscanf(p_lmac, "%02x:%02x:%02x:%02x:%02x:%02x",
                &lan_mac[0],&lan_mac[1],&lan_mac[2],&lan_mac[3],&lan_mac[4],&lan_mac[5]);
        sprintf(wlan0_mac, "%02x%02x%02x%02x%02x%02x",
                lan_mac[0],lan_mac[1],lan_mac[2],lan_mac[3],lan_mac[4],lan_mac[5]);
        _system("ifconfig wifi0 hw ether %s", wlan0_mac);
        /* (Ubicom) We have a single radio. Comment out wifi1 setting.
        sscanf(p_wan_mac,"%02x:%02x:%02x:%02x:%02x:%02x",&wan_mac[0],&wan_mac[1],&wan_mac[2],&wan_mac[3],&wan_mac[4],&wan_mac[5]);
        if(wan_mac[5]!= 0xff)
                sprintf(wlan1_mac, "%02x%02x%02x%02x%02x%02x",wan_mac[0],wan_mac[1],wan_mac[2],wan_mac[3],wan_mac[4],wan_mac[5]+1);
        else if(wan_mac[4]!= 0xff)
                sprintf(wlan1_mac, "%02x%02x%02x%02x%02x%02x",wan_mac[0], wan_mac[1], wan_mac[2], wan_mac[3], wan_mac[4]+1, 0);
        else if(wan_mac[3]!= 0xff)
                sprintf(wlan1_mac, "%02x%02x%02x%02x%02x%02x",wan_mac[0],wan_mac[1],wan_mac[2],wan_mac[3]+1, 0, 0);
        else if(wan_mac[2]!= 0xff)
                sprintf(wlan1_mac, "%02x%02x%02x%02x%02x%02x",wan_mac[0],wan_mac[1],wan_mac[2]+1, 0, 0, 0);
        else if(wan_mac[1]!= 0xff)
                sprintf(wlan1_mac, "%02x%02x%02x%02x%02x%02x",wan_mac[0],wan_mac[1]+1, 0, 0, 0, 0);
        else
                sprintf(wlan1_mac, "%02x%02x%02x%02x%02x%02x",wan_mac[0]+1, 0, 0, 0, 0, 0);
        _system("ifconfig wifi1 hw ether %s", wlan1_mac);
        */
        //DEBUG_MSG("set_wlan_mac: end\n");
}

