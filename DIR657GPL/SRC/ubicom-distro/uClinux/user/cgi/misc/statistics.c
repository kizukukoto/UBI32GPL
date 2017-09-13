#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "nvram.h"
#include "sutil.h"
#include "libdb.h"
#include "debug.h"

#define ifconfig_info "/var/misc/ifconfig_info.txt"

#define DEBUG_MSG(fmt, args...)

static unsigned long rx_packets_on_resetting[4] = {0, 0, 0, 0}; /* wan/lan/wlan/wlan1 */
static unsigned long tx_packets_on_resetting[4] = {0, 0, 0, 0};
static unsigned long rx_dropped_on_resetting[4] = {0, 0, 0, 0};
static unsigned long tx_dropped_on_resetting[4] = {0, 0, 0, 0};
static unsigned long collisions_on_resetting[4] = {0, 0, 0, 0};
static unsigned long error_on_resetting[4]      = {0, 0, 0, 0};

typedef enum IfType
{
        IFTYPE_BRIDGE = 0,
        IFTYPE_ETH_LAN = 1,
        IFTYPE_WLAN = 2,
        IFTYPE_ETH_WAN = 3,
        IFTYPE_WLAN1 = 4  //5G interface 2
}eIfType;

typedef enum PktType
{
        TXPACKETS =1,
        RXPACKETS,
        LOSSPACKETS
}ePktType;

struct __statistics_struct {
        const char *param;
        const char *desc;
        int (*fn)(int, char *[]);
};

/* wan/lan/wlan/wlan1 
 * tx_packet = "0/0/0/0"
 * rx_packet = "0/0/0/0"
 * tx_drop = "0/0/0/0"
 * rx_drop = "0/0/0/0"
 * collision_packet = "0/0/0/0"
 * error_packet = "0/0/0/0"
 */

static int get_packets_from_nvram()
{
	char tmp[256];
	memset(tx_packets_on_resetting, '\0', sizeof(tx_packets_on_resetting));
	memset(rx_packets_on_resetting, '\0', sizeof(rx_packets_on_resetting));
	memset(tx_dropped_on_resetting, '\0', sizeof(tx_dropped_on_resetting));
	memset(rx_dropped_on_resetting, '\0', sizeof(rx_dropped_on_resetting));
	memset(error_on_resetting, '\0', sizeof(error_on_resetting));
	memset(collisions_on_resetting, '\0', sizeof(collisions_on_resetting));

	sprintf(tmp, "%s", nvram_safe_get("tx_packet"));
	sscanf(tmp, "%lu/%lu/%lu/%lu", 
		&tx_packets_on_resetting[0], &tx_packets_on_resetting[1],
		&tx_packets_on_resetting[2], &tx_packets_on_resetting[3]);

	sprintf(tmp, "%s", nvram_safe_get("rx_packet"));
	sscanf(tmp, "%lu/%lu/%lu/%lu", 
		&rx_packets_on_resetting[0], &rx_packets_on_resetting[1],
		&rx_packets_on_resetting[2], &rx_packets_on_resetting[3]);

	sprintf(tmp, "%s", nvram_safe_get("tx_drop"));
	sscanf(tmp, "%lu/%lu/%lu/%lu", 
		&tx_dropped_on_resetting[0], &tx_dropped_on_resetting[1],
		&tx_dropped_on_resetting[2], &tx_dropped_on_resetting[3]);

	sprintf(tmp, "%s", nvram_safe_get("rx_drop"));
	sscanf(tmp, "%lu/%lu/%lu/%lu", 
		&rx_dropped_on_resetting[0], &rx_dropped_on_resetting[1],
		&rx_dropped_on_resetting[2], &rx_dropped_on_resetting[3]);

	sprintf(tmp, "%s", nvram_safe_get("error_packet"));
	sscanf(tmp, "%lu/%lu/%lu/%lu", 
		&error_on_resetting[0], &error_on_resetting[1],
		&error_on_resetting[2], &error_on_resetting[3]);

	sprintf(tmp, "%s", nvram_safe_get("collision_packet"));
	sscanf(tmp, "%lu/%lu/%lu/%lu", 
		&collisions_on_resetting[0], &collisions_on_resetting[1],
		&collisions_on_resetting[2], &collisions_on_resetting[3]);

	cprintf( "rx packets :: %lu/%lu/%lu/%lu\n", 
		rx_packets_on_resetting[0], rx_packets_on_resetting[1],
		rx_packets_on_resetting[2], rx_packets_on_resetting[3]);
	return 0;	
}

static int set_packets_to_nvram(const char *rx_packet, const char *tx_packet, 
		const char *rx_drop, const char *tx_drop, 
		const char *error_packet, const char *collision_packet)
{
	nvram_set("rx_packet", rx_packet);
	nvram_set("tx_packet", tx_packet);
	nvram_set("rx_drop", rx_drop);
	nvram_set("tx_drop", tx_drop);
	nvram_set("error_packet", error_packet);
	nvram_set("collision_packet", collision_packet);
	nvram_commit();
	return 0;
}

static int misc_statistics_help(struct __statistics_struct *p)
{
        cprintf("Usage:\n");

        for (; p && p->param; p++)
                cprintf("\t%s: %s\n", p->param, p->desc);

        cprintf("\n");
        return 0;
}


static int create_ifconfig(int IfType, int type)
{
        switch (IfType) {
                case IFTYPE_ETH_WAN:
                        DEBUG_MSG("IFTYPE_ETH_WAN\n");
                        if (NVRAM_MATCH("wan_proto", "usb3g") != 0){
                                DEBUG_MSG("usb3g\n");
                                _system("ifconfig ppp0 > %s", ifconfig_info);
                        } else {
                                DEBUG_MSG("ethernet\n");
                                _system("ifconfig %s > %s", \
					nvram_safe_get("wan_eth"), ifconfig_info);
                        }
                        type = 0;
                        break;
                case IFTYPE_ETH_LAN:
                        DEBUG_MSG("IFTYPE_ETH_LAN\n");
                        _system("ifconfig %s > %s", 
					nvram_safe_get("lan_eth"), ifconfig_info);
                        type = 1;
                        break;
                case IFTYPE_WLAN:
                        DEBUG_MSG("IFTYPE_WLAN\n");
                        if (NVRAM_MATCH("wlan0_enable", "0") != 0)
                                return -1;
                        else {
                                _system("ifconfig %s > %s", 
					nvram_safe_get("wlan0_eth"), ifconfig_info);
                                type = 2;
                        }
                        break;
                case IFTYPE_WLAN1: // 5G
                        DEBUG_MSG("IFTYPE_WLAN1\n");
                        if (NVRAM_MATCH("wlan1_enable","0") == 0)
                                return -1;
                        else {
                                if (NVRAM_MATCH("wlan0_enable", "1") != 0)
                                        _system("ifconfig %s > %s", \
						"ath1", ifconfig_info);
                                else
                                        _system("ifconfig %s > %s", \
						"ath0", ifconfig_info);
                                type = 3;
                        }
                        break;
                default:
                        return -1;
        }
	return type;
}

static void display_ifconfig_info(eIfType IfType, int idx){
        FILE *fp = NULL;
        char buf[128] = {0},collision[16] = {0};
        char rx_packets[16] = {0},rx_error[16] = {0},rx_drop[16] = {0};
        char tx_packets[16] = {0},tx_error[16] = {0},tx_drop[16] = {0};
        char *packets, *error, *drop, *overrun, *collisions, *txqueuelen;
	char p_ifconfig_info[64];

        int type=0;

	if (create_ifconfig(IfType, &type) == -1)
		return NULL;

	if (access(ifconfig_info, R_OK) != 0)
		return NULL;
        if ((fp = fopen(ifconfig_info, "rb")) == NULL) {
                DEBUG_MSG("ifconfig_info.txt open fail.\n");
                return NULL;
        }

        while (fgets(buf,sizeof(buf), fp)) {

                if (packets = strstr(buf,"RX packets")) {
                        DEBUG_MSG("RX packets\n");
                        if (error = strstr(buf,"errors")) {
                                DEBUG_MSG("error\n");
                                packets = packets + 11;
                                error = error - 1;
                                strncpy(rx_packets,packets,error - packets);
                                DEBUG_MSG("rx_packets=%s\n",rx_packets);
                                if (drop = strstr(buf,"dropped")) {
                                        DEBUG_MSG("dropped\n");
                                        error = error + 8;
                                        drop = drop - 1;
                                        strncpy(rx_error,error,drop - error);
                                        DEBUG_MSG("rx_error=%s\n",rx_error);
                                        if(overrun = strstr(buf,"overruns")) {
                                                DEBUG_MSG("overruns\n");
                                                drop = drop + 9;
                                                overrun = overrun - 1;
                                                strncpy(rx_drop,drop,overrun - drop);
                                                DEBUG_MSG("rx_drop=%s\n",rx_drop);
                                        }
                                }
                        }
                }

                if (packets = strstr(buf,"TX packets")) {
                        if(error = strstr(buf,"errors")) {
                                packets = packets + 11;
                                error = error - 1;
                                strncpy(tx_packets,packets,error - packets);
                                if (drop = strstr(buf,"dropped")) {
                                        error = error + 8;
                                        drop = drop - 1;
                                        strncpy(tx_error,error,drop - error);
                                        if(overrun = strstr(buf,"overruns")) {
                                                drop = drop + 9;
                                                overrun = overrun - 1;
                                                strncpy(tx_drop,drop,overrun - drop);
                                        }
                                }
                        }
                }

                if (collisions = strstr(buf,"collisions")) {
                        if(txqueuelen = strstr(buf,"txqueuelen")) {
                                collisions = collisions + 11;
                                txqueuelen = txqueuelen - 1;
                                strncpy(collision, collisions, txqueuelen - collisions);
                        }
                }

        }

        sprintf(p_ifconfig_info, "%lu/%lu/%lu/%lu/%lu/%lu", \
			atol(rx_packets) - rx_packets_on_resetting[idx], \
			atol(tx_packets) - tx_packets_on_resetting[idx], \
			atol(rx_drop) - rx_dropped_on_resetting[idx], \
			atol(tx_drop) - tx_dropped_on_resetting[idx], \
			atol(collision) - collisions_on_resetting[idx], \
			atol(rx_error) + atol(tx_error) - error_on_resetting[idx]);

        DEBUG_MSG("p_ifconfig_info=%s\n",p_ifconfig_info);
        fclose(fp);
	printf("%s", p_ifconfig_info);
        return 0;
}

static int get_interface_statistics(int argc, char *argv[])
{
	if (argc < 1)
		return 0;

	get_packets_from_nvram();

	if (strcmp(argv[0], "wan") == 0)
		display_ifconfig_info(IFTYPE_ETH_WAN, 0);

	if (strcmp(argv[0], "lan") == 0)
		display_ifconfig_info(IFTYPE_ETH_LAN, 1);

	if (strcmp(argv[0], "wlan") == 0)
		display_ifconfig_info(IFTYPE_WLAN, 2);
	return 0;
}

static int do_clear_statistics(int argc, char *argv[])
{
        FILE *fp = NULL;
        char buf[80], cmds[256], wlan_eth[32];
	char tx_packet[128], rx_packet[128], tx_drop[128], rx_drop[128], 
	     collision_packet[128], error_packet[128];
        unsigned long rx_error, tx_error;
        int if_type = 0;
	
	memset(buf, '\0', sizeof(buf));
	memset(cmds, '\0', sizeof(cmds));
	memset(tx_packet, '\0', sizeof(tx_packet));
	memset(rx_packet, '\0', sizeof(rx_packet));
	memset(rx_drop, '\0', sizeof(rx_drop));
	memset(tx_drop, '\0', sizeof(tx_drop));
	memset(collision_packet, '\0', sizeof(collision_packet));
	memset(error_packet, '\0', sizeof(error_packet));
	sprintf(wlan_eth, "%s", nvram_safe_get("wlan0_eth"));
	
        for (if_type = 0; if_type < 4; if_type++) {
                switch (if_type) {
                        case 0:
                                DEBUG_MSG("%s: Lan\n",__func__);
                                sprintf(cmds, "ifconfig %s > /var/misc/ifconfig_info.txt", 
					nvram_safe_get("wan_eth"));
                                break;
                        case 1:
                                DEBUG_MSG("%s: Wan\n",__func__);
                                sprintf(cmds, "ifconfig %s > /var/misc/ifconfig_info.txt",
					nvram_safe_get("lan_eth"));
                                break;
                        //wireless interface 1(2.4G)
                        case 2:
                                if(nvram_match("wlan0_enable","0") == 0)
                                        return NULL;
                                else
                                        sprintf(cmds, "ifconfig %s > /var/misc/ifconfig_info.txt", 
						wlan_eth);//WLAN0_ETH);
                                break;
                        //wireless interface 2(5G)
                        case 3:
				if(nvram_match("wlan0_enable", "1")==0)
					sprintf(cmds, "ifconfig %s > /var/misc/ifconfig_info.txt", 
					"ath1");
				else
					sprintf(cmds, "ifconfig %s > /var/misc/ifconfig_info.txt", 
					"ath0");
                                break;
                }
		//cprintf("cmds :: %s\n", cmds);
		system(cmds);

                if ((fp = fopen("/var/misc/ifconfig_info.txt","rb")) == NULL )
                        return NULL;

                while(fgets(buf, sizeof(buf),fp)) {
			if(strstr(buf,"RX packets")) {
                                if ( sscanf(buf, "           RX packets:%lu errors:%lu dropped:%lu overruns:%*lu frame:%*lu",
                                        &rx_packets_on_resetting[if_type], &rx_error, &rx_dropped_on_resetting[if_type]) != 3)
                                        printf("sscanf: error 1\n");
                        }
			if(strstr(buf,"TX packets")) {
                                if( sscanf(buf, "           TX packets:%lu errors:%lu dropped:%lu overruns:%*lu carrier:%*lu",
                                        &tx_packets_on_resetting[if_type], &tx_error, &tx_dropped_on_resetting[if_type]) != 3)
                                        printf("sscanf: error 2\n");
                        }
                        if(strstr(buf,"collisions")) {
                                if( sscanf(buf, "           collisions:%lu txqueuelen:%*lu",
                                        &collisions_on_resetting[if_type]) != 1)
                                        printf("sscanf: error 3\n");
                        }
                        memset(buf,0 , sizeof(buf));
                }
                error_on_resetting[if_type] = rx_error + tx_error;
		if (if_type == 0) {
			sprintf(rx_packet, "%lu", rx_packets_on_resetting[if_type]);
			sprintf(tx_packet, "%lu", tx_packets_on_resetting[if_type]);
			sprintf(rx_drop, "%lu", rx_dropped_on_resetting[if_type]);
			sprintf(tx_drop, "%lu", tx_dropped_on_resetting[if_type]);
			sprintf(error_packet, "%lu", error_on_resetting[if_type]);
			sprintf(collision_packet, "%lu", collisions_on_resetting[if_type]);
		} else {
			sprintf(rx_packet, "%s/%lu", rx_packet, rx_packets_on_resetting[if_type]);
			sprintf(tx_packet, "%s/%lu", tx_packet, tx_packets_on_resetting[if_type]);
			sprintf(rx_drop, "%s/%lu", rx_drop, rx_dropped_on_resetting[if_type]);
			sprintf(tx_drop, "%s/%lu", tx_drop, tx_dropped_on_resetting[if_type]);
			sprintf(error_packet, "%s/%lu", error_packet, error_on_resetting[if_type]);
			sprintf(collision_packet, "%s/%lu", collision_packet, collisions_on_resetting[if_type]);
		}
                fclose(fp);
        }

	set_packets_to_nvram(rx_packet, tx_packet, 
			     rx_drop, tx_drop,
			     error_packet, collision_packet);
	return 0;
}

int statistics_main(int argc, char *argv[])
{
        int ret = -1;
        struct __statistics_struct *p, list[] = {
			{"wan", "get wan statistics information", get_interface_statistics},
			{"lan", "get lan statistics information", get_interface_statistics},
			{"wlan", "get wireless statistics information", get_interface_statistics},
			{"clean", "Clear all statistics information", do_clear_statistics},
                        { NULL, NULL, NULL }
        };

        if (argc == 1 || strcmp(argv[1], "help") == 0)
                goto help;

        for (p = list; p && p->param; p++) {
                if (strcmp(argv[1], p->param) == 0) {
                        ret = p->fn(argc - 1, argv + 1);
                        goto out;
                }
        }
help:
        ret = misc_statistics_help(list);
out:
        return ret;

}
