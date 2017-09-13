#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "libdb.h"
#include "debug.h"
#include "hnap.h"
#include "hnapnetwork.h"

char p_pkts[sizeof(long)*8+1] ;
char p_byte_bytes[sizeof(long)*8+1] ;

/* common variable for initial packets*/
static char p_wlan_txbytes_on_resetting[sizeof(long)*8+1] ={0};
static char p_lan_txbytes_on_resetting[sizeof(long)*8+1] ={0};
static char p_wan_txbytes_on_resetting[sizeof(long)*8+1] ={0};
static char p_wlan_rxbytes_on_resetting[sizeof(long)*8+1] ={0};
static char p_lan_rxbytes_on_resetting[sizeof(long)*8+1] ={0};
static char p_wan_rxbytes_on_resetting[sizeof(long)*8+1] ={0};
static char p_wlan_lossbytes_on_resetting[sizeof(long)*8+1]={0};
static char p_lan_lossbytes_on_resetting[sizeof(long)*8+1] ={0};
static char p_wan_lossbytes_on_resetting[sizeof(long)*8+1] ={0};


static int do_set_geteway()
{
        char *ip= NULL;
        char *mask = NULL;
	int rangeStart=10,rangeEnd=20;
	char s_ip[16] = {};
	char e_ip[16] = {};

        struct in_addr start_addr, end_addr, myaddr, mymask;

	ip = nvram_safe_get("lan_ipaddr");
	mask = nvram_safe_get("lan_netmask");
	strcpy(s_ip, nvram_safe_get("dhcpd_start"));
	strcpy(e_ip, nvram_safe_get("dhcpd_end"));
	inet_aton(s_ip, &start_addr);
	inet_aton(e_ip, &end_addr);
	inet_aton(ip, &myaddr);
        inet_aton(mask, &mymask);

	if(!strcmp(mask,"255.255.255.248")) {
		rangeStart=2;
		rangeEnd=6;
	
	} else if(!strcmp(mask,"255.255.255.252")) {
		rangeStart=1;
		rangeEnd=2;
	
	} else {
		rangeStart=10;
		rangeEnd=20;
	}
	
	
        start_addr.s_addr = (myaddr.s_addr & mymask.s_addr) + rangeStart/*(myaddr.s_addr & ~mymask.s_addr)+ 1*/;
        end_addr.s_addr = (myaddr.s_addr & mymask.s_addr) + rangeEnd/*(myaddr.s_addr & ~mymask.s_addr) - 2*/;
        nvram_set("dhcpd_start",inet_ntoa(start_addr));
        nvram_set("dhcpd_end",inet_ntoa(end_addr));


        return 1;
}

int do_getrouter_lansettings(const char *key, char *raw)
{
	char xml_resraw[2048], xml_resraw2[1024];

	bzero(xml_resraw, sizeof(xml_resraw));
	do_xml_create_mime(xml_resraw);

	sprintf(xml_resraw2, getrouter_lansettings_result,
	nvram_safe_get("lan_ipaddr"),
	nvram_safe_get("lan_netmask"),
	NVRAM_MATCH("dhcpd_enable", "1")? "true": "false");

	strcat(xml_resraw, xml_resraw2);

	return do_xml_response(xml_resraw);
}

static int do_save_key(const char *key, char *ct)
{
	//cprintf("XXX %s(%d) %s='%s'\n", __FUNCTION__, __LINE__, key, ct);
	sleep(2); /* To correct value can be stored */
	nvram_set(key, ct);
	return 0;
}

static int do_save_dhcpd_key(const char *key, char *ct)
{
	return do_save_key(key, (strcmp(ct, "true") == 0)? "1": "0");
}

static char* display_bytes(eByteType byte_type,char *interface)

{
	FILE *fp = NULL;
	char *data = NULL;
	char *p_bytes_s = NULL;
	char *p_bytes_e = NULL; 
	int file_len = 0;
	char cmd[128];
  
	memset(p_byte_bytes, 0, sizeof(long)*8+1);
	if( (nvram_match("wlan0_enable", "0") == 0) && (strcmp(interface, nvram_safe_get("wlan0_eth")) == 0) )
		return NULL;
	else {
		sprintf(cmd,"ifconfig %s > /var/misc/bytes.txt",interface);
		system(cmd);
	}
		
	fp = fopen("/var/misc/bytes.txt","rb");
	if (fp == NULL){
		return NULL;
	}
    
	fseek(fp, 0, SEEK_END);
	file_len = ftell(fp);
	data = (char*)malloc(file_len);
	if(data == NULL || file_len == 0){
		fclose(fp);
		cprintf("display bytes memory allocate failed\n");
		return NULL;
	}
	fseek(fp, 0, SEEK_SET);
	fread(data, file_len, 1, fp);
  
	switch (byte_type){ 
		case TXBYTES: 
			p_bytes_s = strstr(data, "TX bytes:") + 9;	//9 is len of TX bytess:
			p_bytes_e = strstr(p_bytes_s, "(") ;			
			if(p_bytes_s == NULL || p_bytes_e == NULL){
				cprintf("tx bytes not found\n");
				return NULL;
			}     			
			memcpy(p_byte_bytes, p_bytes_s, p_bytes_e - p_bytes_s);
			break; 
		case RXBYTES: 
			p_bytes_s = strstr(data, "RX bytes:") + 9;	//11 is len of RX packets:
			p_bytes_e = strstr(p_bytes_s, "(") ;			
			if(p_bytes_s == NULL || p_bytes_e == NULL)
			{
				cprintf("rx bytes not found\n");
				return NULL;
			} 
			memcpy(p_byte_bytes, p_bytes_s, p_bytes_e - p_bytes_s);
			break; 
		default : 
			cprintf("bytes not found\n");
			fclose(fp);
			return NULL;
	}       
  
	free(data);
	fclose(fp);
	return p_byte_bytes;
}

char* display_pkts(ePktType packet_type, char *interface)
{
	FILE *fp = NULL;
	char *data = NULL, *p_pkts_s = NULL, *p_pkts_e = NULL; 
	int file_len = 0;
	char p_tx_lossbytes[sizeof(long)*8+1] ={0};
	char p_rx_lossbytes[sizeof(long)*8+1] ={0};
	char cmd[128];
	unsigned long total_loss_bytes = 0;
	unsigned long after_reset_bytes = 0;

	memset(p_pkts, 0, sizeof(long)*8+1);
	if( (nvram_match("wlan0_enable", "0") == 0) && (strcmp(interface, nvram_safe_get("wlan0_eth")) == 0) )
		return NULL;
	else {
		sprintf(cmd,"ifconfig %s > /var/misc/pkts.txt",interface);
		system(cmd);
	}
		
	fp = fopen("/var/misc/pkts.txt","rb");
	if (fp == NULL)
		return NULL;

	fseek(fp, 0, SEEK_END);
	file_len = ftell(fp);
	data = (char*)malloc(file_len);
	if(data == NULL || file_len == 0){
		fclose(fp);
		cprintf("display pkts memory allocate failed\n");
		return NULL;
	}
	fseek(fp, 0, SEEK_SET);
	fread(data, file_len, 1, fp);


	switch (packet_type){ 
		case TXPACKETS: 
			p_pkts_s = strstr(data, "TX packets:") + 11;	//11 is len of TX packets:
			p_pkts_e = strstr(p_pkts_s, "errors:") ;			
			if(p_pkts_s == NULL || p_pkts_e == NULL){
				cprintf("tx pkts not found\n");
				return NULL;
			}     			
			memcpy(p_pkts, p_pkts_s, p_pkts_e - p_pkts_s);
#ifndef DEVICE_NO_ROUTING
			if(strcmp(interface,nvram_safe_get("wan_eth")) == 0)
				after_reset_bytes = atoi(p_pkts) - atoi(p_wan_txbytes_on_resetting);
			else
#endif			
			 if(strcmp(interface,nvram_safe_get("lan_eth")) == 0)
				after_reset_bytes = atoi(p_pkts) - atoi(p_lan_txbytes_on_resetting);
			else if(strcmp(interface,nvram_safe_get("wlan0_eth")) == 0)
				after_reset_bytes = atoi(p_pkts) - atoi(p_wlan_txbytes_on_resetting);
			memset(p_pkts, 0, sizeof(long)*8+1);
			sprintf(p_pkts, "%lu", after_reset_bytes);
			break; 
		case RXPACKETS: 
			p_pkts_s = strstr(data, "RX packets:") + 11;	//11 is len of RX packets:
			p_pkts_e = strstr(p_pkts_s, "errors:") ;			
			if(p_pkts_s == NULL || p_pkts_e == NULL){
				cprintf("rx pkts not found\n");
				return NULL;
			} 
			memcpy(p_pkts, p_pkts_s, p_pkts_e - p_pkts_s);
#ifndef DEVICE_NO_ROUTING
			if(strcmp(interface,nvram_safe_get("wan_eth")) == 0)
				after_reset_bytes = atoi(p_pkts) - atoi(p_wan_rxbytes_on_resetting);
			else 
#endif
			if(strcmp(interface,nvram_safe_get("lan_eth")) == 0)
				after_reset_bytes = atoi(p_pkts) - atoi(p_lan_rxbytes_on_resetting);
			else if(strcmp(interface,nvram_safe_get("wlan0_eth")) == 0)
				after_reset_bytes = atoi(p_pkts) - atoi(p_wlan_rxbytes_on_resetting);
			memset(p_pkts, 0, sizeof(long)*8+1);
			sprintf(p_pkts, "%lu", after_reset_bytes);
			break; 
		case LOSSPACKETS: 
			p_pkts_s = strstr(data, "dropped") + 8;			//8 is len of dropped:
			p_pkts_e = strstr(p_pkts_s, "overruns") ;			
			if(p_pkts_s == NULL || p_pkts_e == NULL){
				cprintf("tx loss pkts not found\n");
				return NULL;
			} 
			memcpy(p_tx_lossbytes, p_pkts_s, p_pkts_e - p_pkts_s);
			p_pkts_s = strstr(p_pkts_e, "dropped") + 8;			//8 is len of dropped:
			p_pkts_e = strstr(p_pkts_s, "overruns") ;			
			if(p_pkts_s == NULL || p_pkts_e == NULL){
				cprintf("rx loss pkts not found\n");
				return NULL;
			} 
			memcpy(p_rx_lossbytes, p_pkts_s, p_pkts_e - p_pkts_s);
			total_loss_bytes = atoi(p_tx_lossbytes)+atoi(p_rx_lossbytes);
			sprintf(p_pkts, "%lu", total_loss_bytes);
#ifndef DEVICE_NO_ROUTING
			if(strcmp(interface,nvram_safe_get("wan_eth")) == 0)
				after_reset_bytes = atoi(p_pkts) - atoi(p_wan_lossbytes_on_resetting);
			else 
#endif
			if(strcmp(interface,nvram_safe_get("lan_eth")) == 0)
				after_reset_bytes = atoi(p_pkts) - atoi(p_lan_lossbytes_on_resetting);
			else if(strcmp(interface,nvram_safe_get("wlan0_eth")) == 0)
				after_reset_bytes = atoi(p_pkts) - atoi(p_wlan_lossbytes_on_resetting);
			memset(p_pkts, 0, sizeof(long)*8+1);
			sprintf(p_pkts, "%lu", after_reset_bytes);            
			break; 
		default : 
			cprintf("pkts not found\n");
			fclose(fp);
			return NULL;
	}       

	free(data);
	fclose(fp);
	return p_pkts;
}

char* display_lan_bytes(eByteType byte_type)
{
	return display_bytes(byte_type,nvram_safe_get("lan_eth"));
}

char* display_wlan_bytes(eByteType byte_type)
{
	return display_bytes(byte_type,nvram_safe_get("wlan0_eth"));
} 

char* display_lan_pkts(ePktType packet_type){
	return display_pkts(packet_type,nvram_safe_get("lan_eth"));
}

char* display_wlan_pkts(ePktType packet_type){
	return display_pkts(packet_type,nvram_safe_get("wlan0_eth"));
}

static void do_get_network_stats_to_xml(char *dest_xml, char *general_xml)
{
	/* initial network stats */
	sprintf(pure_atrs.lanTxPkts,"%s",display_lan_pkts(TXPACKETS) ? : "0");
        sprintf(pure_atrs.lanRxPkts,"%s",display_lan_pkts(RXPACKETS) ? : "0");
        sprintf(pure_atrs.lanRxBytes,"%s",display_lan_bytes(RXBYTES) ? : "0");
        sprintf(pure_atrs.lanTxBytes,"%s",display_lan_bytes(TXBYTES) ? : "0");
        sprintf(pure_atrs.wlanTxPkts,"%s",display_wlan_pkts(TXPACKETS) ? : "0");
        sprintf(pure_atrs.wlanRxPkts,"%s",display_wlan_pkts(RXPACKETS) ? : "0");
        sprintf(pure_atrs.wlanTxBytes,"%s",display_wlan_bytes(TXBYTES) ? : "0");
        sprintf(pure_atrs.wlanRxBytes,"%s",display_wlan_bytes(RXBYTES) ? : "0");

        /* add stats to xml */
        sprintf(dest_xml,general_xml,
                        pure_atrs.lanRxPkts, pure_atrs.lanTxPkts, pure_atrs.lanRxBytes, pure_atrs.lanTxBytes,
                        pure_atrs.wlanRxPkts, pure_atrs.wlanTxPkts, pure_atrs.wlanRxBytes, pure_atrs.wlanTxBytes);
}

int do_setrouter_lansettings(const char *key, char *raw)
{
	char xml_raw[2048], xml_raw2[1024];
	hnap_entry_s ls[] = {
		{ "RouterIPAddress",   "lan_ipaddr",   do_save_key },
		{ "RouterSubnetMask",  "lan_netmask",  do_save_key },
		{ "DHCPServerEnabled", "dhcpd_enable", do_save_dhcpd_key },
		{ NULL, NULL, NULL }
	};

	
	bzero(xml_raw, sizeof(xml_raw));
	do_xml_create_mime(xml_raw);
	do_xml_mapping(raw, ls);

	sprintf(xml_raw2, setrouter_lansettings_result, 
			do_set_geteway() ? "REBOOT": "ERROR");
	
	strcat(xml_raw, xml_raw2);

	do_xml_response(xml_raw);

	return rc_restart();
}

int do_get_network_stats(const char *key, char *raw)
{
	char xml_resraw[2048],xml_resraw2[1024];
	bzero(xml_resraw, sizeof(xml_resraw));
	
	do_xml_create_mime(xml_resraw);
	do_get_network_stats_to_xml(xml_resraw2,get_network_stats_result);
	strcat(xml_resraw, xml_resraw2);

	return do_xml_response(xml_resraw);

}
