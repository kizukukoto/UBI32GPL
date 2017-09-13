#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <signal.h>
#include <fcntl.h>

#include "libdb.h"
#include "sutil.h"
#include "debug.h"
#include "hnap.h"
#include "hnapconnection.h"


struct dhcpd_leased_table_s *p_st_dhcpd_list;          //array of dhcpd_leased_table_s info
T_WLAN_STAION_INFO wlan_staion_info;

wireless_stations_table_t wlan_sta_table[M_MAX_STATION];
struct html_leases_s leases[M_MAX_DHCP_LIST];
struct dhcpd_leased_table_s client_list[M_MAX_DHCP_LIST];
cnted_client_list_t cnted_client_table[4];
T_DHCPD_LIST_INFO dhcpd_list;

static char *pure_dhcp_time_format(char *data,char *res)
{
	char *year,*month,*temp_date,*temp_time;
	char date[16] = "";
	char time_tmp[32] = "";	
	
	year = data;
	month = NULL;

	if((temp_date = strstr(data,"Jan")))
		month = "01";
	else if((temp_date = strstr(data,"Feb")))
		month = "02";
	else if((temp_date = strstr(data,"Mar")))
		month = "03";
	else if((temp_date = strstr(data,"Apr")))
		month = "04";
	else if((temp_date = strstr(data,"May")))
		month = "05";
	else if((temp_date = strstr(data,"Jun")))
		month = "06";
	else if((temp_date = strstr(data,"Jul")))
		month = "07";
	else if((temp_date = strstr(data,"Aug")))
		month = "08";
	else if((temp_date = strstr(data,"Sep")))
		month = "09";
	else if((temp_date = strstr(data,"Oct")))
		month = "10";
	else if((temp_date = strstr(data,"Nov")))
		month = "11";
	else if((temp_date = strstr(data,"Dec")))
		month = "12";

	temp_date = temp_date + 4;
	strncpy(date,temp_date,2);
	if(strcmp(date," 1") == 0)
		strcpy(date,"01");
	if(strcmp(date," 2") == 0)
		strcpy(date,"02");
	if(strcmp(date," 3") == 0)
		strcpy(date,"03");
	if(strcmp(date," 4") == 0)
		strcpy(date,"04");
	if(strcmp(date," 5") == 0)
		strcpy(date,"05");
	if(strcmp(date," 6") == 0)
		strcpy(date,"06");
	if(strcmp(date," 7") == 0)
		strcpy(date,"07");
	if(strcmp(date," 8") == 0)
		strcpy(date,"08");
	if(strcmp(date," 9") == 0)
		strcpy(date,"09");
	temp_time = temp_date + 3;
	strncpy(time_tmp,temp_time,8);
	year = year + 20;
	sprintf(res,"%s-%s-%sT%s",year,month,date,time_tmp);
	return res;
}

int rssi_to_ratio(char *rssi)
{
	int nRSSI=atoi(rssi);
	int signalStrength=0;

	if (nRSSI >= 40) {
		signalStrength = 100;// 40 dB SNR or greater is 100%

	} else if (nRSSI >= 20) {
		// 20 dB SNR is 80% 
		signalStrength = nRSSI - 20;
        	signalStrength = (signalStrength * 26) / 30;
	        signalStrength += 80; // Between 20 and 40 dB, linearize between 80 and 100
	} else
		signalStrength = (nRSSI * 75) / 20;

	return signalStrength;
}

T_DHCPD_LIST_INFO* get_dhcp_client_list(void)
{
	FILE *fp;
	FILE *fp_pid;
	int i;

	// Jery Lin Added for Fix DHCP Reservations issue 2010/02/04
	char res_rule[] = "dhcpd_reserve_XX";
	char *enable="", *name="", *ip="", *mac="";
	char reserve_ip[MAX_DHCPD_RESERVATION_NUMBER][16];
	char *p_dhcp_reserve;
	char fw_value[192]={0};
	int j;

	if(NVRAM_MATCH("wlan0_mode","ap") || NVRAM_MATCH("dhcpd_enable","0"))
		return NULL;

	dhcpd_list.num_list = 0; 
	memset(leases, 0, sizeof(struct html_leases_s)*M_MAX_DHCP_LIST);

	fp_pid = fopen(UDHCPD_PID,"r");
	if(fp_pid) {	
	/*write dhcp list into file /var/misc/udhcpd.leases & html.leases */
		kill(read_pid(UDHCPD_PID), SIGUSR1);
		fclose(fp_pid);

	} else {
		cprintf("get_dhcp_client_list /var/run/udhcpd.pid error\n");
		return NULL;
	}	

	cprintf("get_dhcp_client_list: wait for  open udhcpd.leases\n");
	/*Albert: take off sleep for GUI get information lag */
//	sleep(1);
	usleep(1000);
	fp = fopen("/var/misc/html.leases","r");
	if(fp == NULL) {
		cprintf("get_dhcp_client_list: open html.leases error\n");
		return NULL;
	}

	// Jery Lin Added for save to reserve_ip array from nvram dhcpd_reserve_XX
	memset(reserve_ip,0,sizeof(reserve_ip));
	for (j=0; j<MAX_DHCPD_RESERVATION_NUMBER; j++) {
		snprintf(res_rule, sizeof(res_rule), "dhcpd_reserve_%02d",j);
		p_dhcp_reserve = nvram_safe_get(res_rule);
		if (p_dhcp_reserve == NULL || (strlen(p_dhcp_reserve) == 0))
			continue;
		memcpy(fw_value, p_dhcp_reserve, strlen(p_dhcp_reserve));
		getStrtok(fw_value, "/", "%s %s %s %s ", &enable, &name, &ip, &mac);
		if (atoi(enable)==0)
			continue;
		strcpy(reserve_ip[j],ip);
		cprintf("get_dhcp_client_list: reserve_ip[%d]=%s\n",j,reserve_ip[j]);
	}

	
	for( i = 0; i < M_MAX_DHCP_LIST; i++) {
		fread(&leases[i], sizeof(struct html_leases_s), 1, fp);
		if(leases[i].ip_addr[0] != 0x00) { 
			dhcpd_list.num_list++; 
			client_list[i].hostname = leases[i].hostname;
			client_list[i].ip_addr = leases[i].ip_addr;
			client_list[i].mac_addr = leases[i].mac_addr;

			// Jery Lin Added for check client_list[i] whether is in reserve_ip array
			for (j=0; j<MAX_DHCPD_RESERVATION_NUMBER; j++)
				if (strcmp(reserve_ip[j], client_list[i].ip_addr)==0)
					break;

			client_list[i].expired_at = (j<MAX_DHCPD_RESERVATION_NUMBER) ? 
						    "Never":
						    leases[i].expires;

			cprintf("%s",leases[i].hostname);
			cprintf("%s",client_list[i].ip_addr);
			cprintf("%s",leases[i].mac_addr);
			cprintf("%s",client_list[i].expired_at);
			cprintf("\n");
		}
		else
			break;
	}	
	cprintf("dhcpd_list.num_list = %d \n",dhcpd_list.num_list);
	fclose(fp);
	dhcpd_list.p_st_dhcpd_list = &client_list[0];
	return &dhcpd_list;
}

T_WLAN_STAION_INFO* get_stalist()
{
	FILE *fp;
	char *ptr, *rssi_ptr, *rate_ptr, *nego_rate;
	char buff[1024], addr[20], rssi[4], mode[16], time[16], rate[8], ip_addr[16]; 
	T_DHCPD_LIST_INFO* dhcpd_status = get_dhcp_client_list();
	int i;
	char *ptr_htcaps = NULL;
	int sta_is_n_mode = 0;
	char cmd[256];

	if(NVRAM_MATCH("wlan0_enable", "0"))
		return NULL;	
	wlan_staion_info.num_sta = 0;
	memset(wlan_sta_table, 0, sizeof(wireless_stations_table_t)*M_MAX_STATION);

	sprintf(cmd,"wlanconfig %s list sta > /var/misc/stalist.txt",nvram_safe_get("wlan0_eth"));
	system(cmd);
	
	fp=fopen("/var/misc/stalist.txt","rb");
	if (fp == NULL) {
		cprintf("stalist file open failed (No station connected). \n");
		return NULL;
	}

	fgets(buff, 1024, fp); 
    	//buf = ADDR AID CHAN RATE RSSI IDLE  TXSEQ  RXSEQ CAPS ACAPS ERP    STATE   TYPE  CIPHER   MODE A_TIME NEGO_RATES
	
	//AR9100(AP81), AR7161(AP94)
	//buf = ADDR AID CHAN RATE RSSI IDLE  TXSEQ  RXSEQ CAPS ACAPS ERP    STATE HTCAPS   A_TIME NEGO_RATES
	memset(buff, 0, sizeof(buff));
	
	while( fgets(buff, 1024, fp)) {
		ptr_htcaps = NULL;
		sta_is_n_mode = 0;
	    	ptr = buff;
    	
		/* Add 2.GHz Wireless GuestZone STA List */
    		if(strstr(buff, "ADDR")) {
			cprintf("get_stalist(): Skip GuestZone first line\n");
			memset(buff, 0, sizeof(buff));
			continue;
		}	
    	
		/*UAPSD QoSInfo: 0x0f, (VO,VI,BE,BK) = (1,1,1,1), MaxSpLimit = NoLimit*/
	    	if(strstr(buff, "UAPSD QoSInfo"))  {
			cprintf("get_stalist(): Having VISTA Client QoSInfo.\n");
			memset(buff, 0, sizeof(buff));
			continue;
		}	
	    	memset(addr, 0, 20);
    		memset(rssi, 0, 4);
	    	memset(mode, 0, 16);
    		memset(time, 0, 16);
	    	memset(rate, 0, 8);
    		memset(ip_addr, 0, 16);
  	    	
    		//00:13:46:11:6b:bd    1    4  54M   55  135     25  18464 ESs          0       27   open    none Normal 00:00:56 12        WME
	    	//%s %4u  %4d  %3dM %4d  %4d  %6d    %6d    %4s %5s   %3x  %8x      %6s    %7s     %6s    %-8.8s   %-10d 	
    	
	    	//AR9100(AP81), AR7161(AP94)
	    	//00:19:d2:36:f7:98    3   11 130M   62  120    451     96 ES           0       27 Q      01:27:06 12        WME
	    	//%s                  %4u  %4d  %3dM %4d  %4d  %6d    %6d    %4s %5s   %3x  %8x      %6s    %-8.8s   %-10d          
	    
	    	strncpy(addr, ptr, 17);    	
    		ptr += strlen(addr);
    	
	    	/* skip one space + AID *//* skip one space + CHAN */
    		ptr += 10; 
    	
	    	ptr += 1; /* skip one space */
    		rate_ptr = ptr;
	    	while(*rate_ptr == ' ')
    			rate_ptr++;
	    	strncpy(rate, rate_ptr, 4-(rate_ptr-ptr) ); 	
    		ptr += 4; /* skip RATE */
    	
	    	ptr += 1; /* skip one space */   	
    		rssi_ptr = ptr;
	    	while(*rssi_ptr == ' ')
    			rssi_ptr++;
	    	strncpy(rssi, rssi_ptr, 4-(rssi_ptr-ptr) );
    	
    	
    		/* skip RSSI */  /* skip one space + IDLE */ /* skip one space + TXSEQ */
	    	/* skip one space + RXSEQ */ /* skip one space + CAPS */ /* skip one space + ACAPS */
    		/* skip one space + ERP */ /* skip one space + STATE */
	    	ptr += 47; 	
 
		/*	Date: 2010-02-22
		*	Name: Jimmy Huang
		*	Reason:	Fixed the bug that sometimes wireless sta mode showing in WEB GUI is not correct
		*	Note:	We using HTCAPS to determine the mode
		*			If has any one of G,M,P,S,W, we consider it as n-mode client
		*/
		ptr_htcaps = ptr + 1;
		ptr += 7; /* skip one space + HTCAPS */

    		ptr += 1; /* skip one space */
    		strncpy(time, ptr, 8);
	    	ptr += strlen(time);
    		ptr += 1; /* skip one space */
	    	if(!strlen(time))
    			strcpy(time, "0");
    		
	    	nego_rate = ptr;
    		while(*nego_rate != ' ')
    			nego_rate++;

		/*NEGO_RATES = 12*/
	    	if(nego_rate - ptr == 2) {
			/*	Date: 2010-02-22
			*	Name: Jimmy Huang
			*	Reason:	Fixed the bug that sometimes wireless sta mode showing in WEB GUI is not correct
			*	Note:	We using HTCAPS to determine the mode
			*			If has any one of G,M,P,S,W, we consider it as n-mode client
			*/
			strcpy(mode, "802.11g");
			while(*ptr_htcaps != ' '){
				switch(ptr_htcaps[0]){
					case 'G':
					case 'M':
					case 'P':
					case 'S':
					case 'W':
						memset(mode,'\0',sizeof(mode));
						strcpy(mode, "802.11n");
						sta_is_n_mode = 1;
						break;
					default:
						break;
				}
				if(sta_is_n_mode){
					break;
				}
				ptr_htcaps++;
			}
    		} else if(nego_rate - ptr == 1) /*NEGO_RATES = 4*/
    			strcpy(mode, "802.11b");
    	
   	
	    	strcpy(ip_addr, "0.0.0.0");	
    		if(dhcpd_status != NULL) {		

		    	for(i = 0 ; i < dhcpd_status->num_list; i++) {

				if(strcmp(dhcpd_status->p_st_dhcpd_list[i].mac_addr, addr) == 0) {
					strcpy(ip_addr, dhcpd_status->p_st_dhcpd_list[i].ip_addr);
					break;
				}
			}
		}

		memset(wlan_sta_table[wlan_staion_info.num_sta].list, 0, 128);
		memset(wlan_sta_table[wlan_staion_info.num_sta].info, 0, 128);

		sprintf(wlan_sta_table[wlan_staion_info.num_sta].list, "%s/%d/%s/%s/%s/%s", addr, rssi_to_ratio(rssi), mode, time, rate, ip_addr);// for GUI
    		sprintf(wlan_sta_table[wlan_staion_info.num_sta].info, "%s/%d/%s/%s/%s/%s", addr, rssi_to_ratio(rssi), mode, time, rate, ip_addr);
		wlan_sta_table[wlan_staion_info.num_sta].mac_addr = strtok(wlan_sta_table[wlan_staion_info.num_sta].info, "/");
		wlan_sta_table[wlan_staion_info.num_sta].rssi = strtok(NULL,"/");
		wlan_sta_table[wlan_staion_info.num_sta].mode = strtok(NULL,"/");
		wlan_sta_table[wlan_staion_info.num_sta].connected_time = strtok(NULL,"/");
		wlan_sta_table[wlan_staion_info.num_sta].rate = strtok(NULL,"/");
		wlan_sta_table[wlan_staion_info.num_sta].ip_addr = strtok(NULL,"/");
		wlan_staion_info.num_sta++;
		
		memset(buff, 0, sizeof(buff));
	}
		
	fclose(fp);
	wlan_staion_info.p_st_wlan_sta = &wlan_sta_table[0];
	
	return &wlan_staion_info;
}


static int do_get_connected_devices_to_xml(char *dest_xml, char *general_xml, char *cnted_client_xml)
{
	T_DHCPD_LIST_INFO* dhcpd_status = NULL;
	T_WLAN_STAION_INFO* wireless_status = NULL;
	int i = 0, j = 0;
	char clients_xml[MAX_RES_LEN] = {};
	char clients_xml_tmp[1000] = {};	
	char res[1024] = {}; 
	int find = 0;
	int wireless_enable=1;
	
	dhcpd_status = get_dhcp_client_list();
	wireless_status = get_stalist();

	if(wireless_status == NULL || wireless_status->num_sta == 0)
		wireless_enable=0;
	
	if((dhcpd_status == NULL) && (wireless_status == NULL))		
		return 0;   
	else if(dhcpd_status->num_list > 0) {
	
		cprintf("dhcpd_status->num_list = %d\n", dhcpd_status->num_list);
		if(wireless_enable)
			cprintf("wireless_status->num_sta = %d\n", wireless_status->num_sta);
			
		for(i = 0 ; i < dhcpd_status->num_list; i++) {
			cnted_client_table[i].cnted_time = pure_dhcp_time_format(dhcpd_status->p_st_dhcpd_list[i].expired_at,res);
			cnted_client_table[i].mac_add = dhcpd_status->p_st_dhcpd_list[i].mac_addr;
			cnted_client_table[i].dev_name = dhcpd_status->p_st_dhcpd_list[i].hostname;		
				
			if(wireless_enable) {			
				find = 0;
				for(j = 0 ; j < wireless_status->num_sta; j++) {				
					if(strncmp(dhcpd_status->p_st_dhcpd_list[i].mac_addr ,wireless_status->p_st_wlan_sta[j].mac_addr, 18) == 0) {
						cnted_client_table[i].port_name = "WLAN 802.11g";
						cnted_client_table[i].wireless = "true";
						cnted_client_table[i].active = "false";
						find = 1;
						break;
					}
				}
		
				if(!find) {
					cnted_client_table[i].port_name = "LAN";
					cnted_client_table[i].wireless = "false";
					cnted_client_table[i].active = "true";				
				}

			} else {
				cnted_client_table[i].port_name = "LAN";
				cnted_client_table[i].wireless = "false";
				cnted_client_table[i].active = "true";				
			}
		}		
	}
	
	for(i = 0; i < dhcpd_status->num_list; i++) {
		
		sprintf(clients_xml_tmp, cnted_client_xml,\
		cnted_client_table[i].cnted_time,\
		cnted_client_table[i].mac_add,\
		cnted_client_table[i].dev_name,\
		cnted_client_table[i].port_name,\
		cnted_client_table[i].wireless,\
		cnted_client_table[i].active);		
	
		sprintf(clients_xml, "%s%s", clients_xml, clients_xml_tmp);
		memset(clients_xml_tmp, 0, 1000);
	}
	
	sprintf(dest_xml, general_xml, "OK", clients_xml);	
	return 1;	
}


int do_get_connected_devices(const char *key, char *raw)
{
	int ret;
	char xml_resraw[2048];
	char xml_resraw2[1024];
	
	bzero(xml_resraw, sizeof(xml_resraw));
        do_xml_create_mime(xml_resraw);
	ret = do_get_connected_devices_to_xml(xml_resraw2, get_connected_devices_result, cnted_client);
	strcat(xml_resraw, xml_resraw2);
	return do_xml_response(xml_resraw);
}
