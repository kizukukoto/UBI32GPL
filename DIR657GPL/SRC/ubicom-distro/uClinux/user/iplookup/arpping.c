/*
 * arpping.c
 *
 * Mostly stolen from: dhcpcd - DHCP client daemon
 * by Yoichi Hariguchi <yoichi@fore.com>
 */

#include <sys/time.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/if_ether.h>
#include <net/if_arp.h>
#include <net/if.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include <errno.h>
#include <sys/sysinfo.h>
#include <sys/ioctl.h>
#include <shvar.h>
#include <stdlib.h>

#include "arpping.h"
#include "ip_lookup.h"

struct lan_pc_list lan_pc[255];
int pc_count=0;

//#define ARPPING_DEBUG  1
#ifdef ARPPING_DEBUG
#define DBGMSG(fmt, arg...)       printf("%s [%d]: "fmt, __FILE__, __LINE__, ##arg)
#else
#define DBGMSG(fmt, arg...)
#endif

#define MAC_BCAST_ADDR		(uint8_t *) "\xff\xff\xff\xff\xff\xff"
#define SIOCGIFHWADDR	0x8927		/* Get hardware address	*/

uint8_t device_mac[6] = {0};
unsigned long device_ip;
unsigned long start_ip;
unsigned long end_ip;
int ip_count = 0;

/*
 * Reason: modify for not senting arpping to wlan.
 * Modified: John Huang
 * Date: 2010.02.02
 */
char br_interface[6]={0},arp_interface[6]={0};


void send_arpping(void)
{
	unsigned long arpping_ip = 0;

	while (1)
	{
		if (ip_count > 255) {
			DBGMSG("arpping done\n");
			break;
		}

		DBGMSG("ip_count = %x\n", ip_count);

		arpping_ip = start_ip + ip_count;
		if ((arpping_ip) == device_ip || (arpping_ip) > end_ip) {
			/* Nothing to do */
			ip_count++;
			continue;
		}

		ip_count++;
		arpping(htonl(arpping_ip), htonl(device_ip) ,device_mac, arp_interface);
	}
}

int read_interface_mac(char *interface, uint8_t *device_mac)
{
	int fd;
	struct ifreq ifr;

	memset(&ifr, 0, sizeof(struct ifreq));
	if((fd = socket(AF_INET, SOCK_RAW, IPPROTO_RAW)) >= 0) {
		ifr.ifr_addr.sa_family = AF_INET;
		strcpy(ifr.ifr_name, interface);

		if (ioctl(fd, SIOCGIFHWADDR, &ifr) == 0) {
			memcpy(device_mac, ifr.ifr_hwaddr.sa_data, 6);
			DBGMSG("adapter hardware address %02x:%02x:%02x:%02x:%02x:%02x\n",
				device_mac[0], device_mac[1], device_mac[2], device_mac[3], device_mac[4], device_mac[5]);
		}
		else {
			DBGMSG( "SIOCGIFHWADDR failed! \n");
			return -1;
		}
	}
	else {
		DBGMSG( "socket failed! \n");
		return -1;
	}

	close(fd);
	return 0;
}

int _system(const char *fmt, ...)
{
	va_list args;
	int i;
	char buf[512];

	va_start(args, fmt);
	i = vsprintf(buf, fmt,args);
	va_end(args);

	system(buf);
	return i;
}

char *get_nvram(const char *name, char *value)
{
    char buf[128], *ptr;
    FILE *fp;

    _system("nvram get %s > /var/tmp/%s", name, name);

    sprintf(buf, "/var/tmp/%s", name);
    if ((fp=fopen(buf,"r")) == NULL)
        goto final;

    if (fgets(buf, sizeof(buf), fp) == NULL)
        goto final;

    if ((ptr=strchr(buf, '=' )) != NULL)
        strcpy(value, ptr+2);
    else
        strcpy(value, buf);

    // replace '\n' to '\0'
    value[strlen(value)-1] = '\0';

    fclose(fp);
    _system("rm -f /var/tmp/%s", name);
    return value;

final:
    if (fp)
        fclose(fp);

    _system("rm -f /var/tmp/%s", name);
    return NULL;
}

void query_hostname_for_wlan_clients(void)
{
    FILE *fp, *fp1;
    char buff[512], mac_addr[20], ip_addr[20], buff1[32];
    unsigned char hostname[20] = {};
    char wlan0_enable[4] = {}, wlan0_vap1_enable[4] = {};

    get_nvram("wlan0_enable", wlan0_enable);
    get_nvram("wlan0_vap1_enable", wlan0_vap1_enable);

#ifdef USER_WL_ATH_5GHZ
    char wlan1_enable[4] = {}, wlan1_vap1_enable[4] = {};

    get_nvram("wlan1_enable", wlan1_enable);
    get_nvram("wlan1_vap1_enable", wlan1_vap1_enable);
#endif

    if (strcmp(wlan0_enable, "1") == 0) {
        _system("wlanconfig ath0 list sta > /var/misc/stalist_arp.txt");
        if (strcmp(wlan0_vap1_enable, "1") == 0)
        {
#ifdef USER_WL_ATH_5GHZ
            if (strcmp(wlan1_enable, "1") == 0)
                system("wlanconfig ath2 list sta >> /var/misc/stalist_arp.txt"); //2.4GHz GuestZone
            else
#endif
                system("wlanconfig ath1 list sta >> /var/misc/stalist_arp.txt"); //2.4GHz GuestZone
        }
    }

#ifdef USER_WL_ATH_5GHZ
    if (strcmp(wlan1_enable, "1") == 0) {
        if (strcmp(wlan0_enable, "1") == 0) {
            system("wlanconfig ath1 list sta >> /var/misc/stalist_arp.txt");
            if (strcmp(wlan1_vap1_enable, "1") == 0)  { /* Add 5GHz Wireless GuestZone STA List */
                if (strcmp(wlan0_vap1_enable, "1") == 0)
                    system("wlanconfig ath3 list sta >> /var/misc/stalist_arp.txt"); //5GHz GuestZone
                else
                    system("wlanconfig ath2 list sta >> /var/misc/stalist_arp.txt"); //5GHz GuestZone
            }
        }
        else {
            system("wlanconfig ath0 list sta > /var/misc/stalist5g.txt");
            if (strcmp("wlan1_vap1_enable", "1") == 0)
                system("wlanconfig ath1 list sta >> /var/misc/stalist5g.txt");//5GHz GuestZone
        }
    }
#endif

    fp = fopen("/var/misc/stalist_arp.txt", "rb");
    if (fp == NULL) {
        DBGMSG("stalist file open failed (No station connected). \n");
        return;
    }

    while (fgets(buff, 1024, fp)) {
        if (strstr(buff, "ID CHAN RATE RSSI IDLE"))
            continue;

        if (strstr(buff, "UAPSD QoSInfo")) {
            continue;
        }

        memset(mac_addr, 0, sizeof(mac_addr));
        strncpy(mac_addr, buff, 17);

        _system("/bin/cat /proc/net/arp | grep '%s'| cut -d ' ' -f 1  > /tmp/var/arpinfo_arp.txt", mac_addr);
        fp1 = fopen("/tmp/var/arpinfo_arp.txt", "rb");
        if (fp1 == NULL) {
            DBGMSG("arpinfo file open failed.\n");
            continue;
        }

        if (fgets(buff1, 32, fp1) == NULL) {
            fclose(fp1);
            continue;
        }
        fclose(fp1);

        if (strlen(buff1))
            strncpy(ip_addr, buff1, 15);
        else
            continue;

        memset(hostname, 0, sizeof(hostname));
        strncpy(lan_pc[pc_count].mac, mac_addr, 20);
        lan_pc[pc_count].ip.s_addr = inet_addr(ip_addr);
        pc_count++;
        query_hostname(mac_addr, inet_addr(ip_addr), hostname);
    }
}

int main(int argc, char *argv[])
{
	device_ip = htonl(inet_addr(argv[1]));
	start_ip = htonl(inet_addr(argv[2]));
	end_ip = htonl(inet_addr(argv[3]));
	int count=0;
	memcpy(br_interface, argv[4], strlen(argv[4]));
	memcpy(arp_interface, argv[5], strlen(argv[5]));

	DBGMSG("device_ip = %s . star = %s . end = %s\n", argv[1], argv[2], argv[3]);

	_system("cp %s %s", CLIENT_LIST_FILE , CLIENT_LIST_TMP_FILE);
	init_file(CLIENT_LIST_FILE);
	read_interface_mac(br_interface, device_mac);
	memset(lan_pc, 0, sizeof(struct lan_pc_list)*255);

	//query_hostname_for_wlan_clients();
	send_arpping();

	for(count=0; count<pc_count; count++)
	{
		DBGMSG("save2file: lan_pc[%d].ip=%s, mac=%s, name=%s\n",
			count, inet_ntoa(lan_pc[count].ip), lan_pc[count].mac, lan_pc[count].name);

		save2file(CLIENT_LIST_FILE,"%s/%s/%s\n",
			inet_ntoa(lan_pc[count].ip), lan_pc[count].name, lan_pc[count].mac);
	}

	_system("cp %s %s", CLIENT_LIST_FILE , CLIENT_LIST_TMP_FILE);
	DBGMSG("arpping finished\n");

    return 1;
}

void netBios_sock_init(netbiosMsg *nbmsg)
{
	int i;

	nbmsg->Flag = htons((unsigned short)0);
	nbmsg->QDCount = htons((unsigned short)1);
	nbmsg->ANCount = htons((unsigned short)0);		/* number of answer resource records */
	nbmsg->NSCount = htons((unsigned short)0);		/* number of name service resource records */
	nbmsg->ARCount = htons((unsigned short)0);

	/* form the name "*<00><00><00><00><00><00><00><00><00><00><00><00><00><00><00>" */
	nbmsg->question.Name[0] = 0x20;	/* SPACE */
	nbmsg->question.Name[1] = 0x43;	/* C */
	nbmsg->question.Name[2] = 0x4b;	/* K */
	for(i=3; i<sizeof(nbmsg->question.Name)-1; i++){
		nbmsg->question.Name[i] = 0x41;	/* A */
	}
	nbmsg->question.Name[i] = 0x00;
	nbmsg->question.Type = htons(QUESTION_TYPE_NBSTAT);
	nbmsg->question.Class = htons(QUESTION_CLASS_IN);

	DBGMSG("netBios_sock_init finished\n");
	return;
}

int query_hostname(unsigned char *mac, unsigned long query_ip ,char *hostname)
{
	int ret = 0;
	struct sockaddr_in client_socka, server_socka;
	int server_port = 4097;	/* Any one port */
	int client_port = 137;		/* NetBios port */
	char *ptr;

	unsigned long time_to_live;
	unsigned short data_length;
	unsigned char number_of_name;
	char *name = hostname;	/* hostname */
	netbiosMsg nbmsg;
	int server_socket = -1;	/* socket number */
	fd_set fdset;
	struct timeval tm_val;
	int retstatus = 0;
	char recv_buffer[1024]={0};
	unsigned int addr_len = sizeof(struct sockaddr_in);
	int enable = 1;
	int count=0;
	struct in_addr query_addr = { .s_addr = query_ip };

	netBios_sock_init(&nbmsg);

	memset((char *)&server_socka, 0, sizeof(server_socka));
	server_socka.sin_family = AF_INET;
	server_socka.sin_addr.s_addr = htonl(INADDR_ANY);
	server_socka.sin_port = htons(server_port);

	server_socket = socket(AF_INET, SOCK_DGRAM, 0);
	if(server_socket<0){
		printf("Create Server Socket Fail!!");
	}
	else{
		if(setsockopt( server_socket, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable)) != 0){
			printf("Server socket setsockopt Fail!!");
			close(server_socket);
		}
		else{
			if( bind(server_socket, (struct sockaddr *)&server_socka, sizeof(server_socka)) < 0 ){
				printf("Server socket bind Fail!!");
				close(server_socket);
			}
		}
	}

	memset((char *)&client_socka, 0, sizeof(client_socka));
	client_socka.sin_family = AF_INET;
	client_socka.sin_port = htons(client_port);
	client_socka.sin_addr.s_addr = query_ip;
	nbmsg.TransactionID = rand();	// assign rand number to transaction id in netbios message.

	if (sendto(server_socket, (char *)&nbmsg, sizeof(nbmsg), 0, (struct sockaddr *) &client_socka, sizeof(struct sockaddr_in)) == -1) {
		printf("Send NetBIOS data Fail!!");
		close(server_socket);
		ret = -1;
	}
	else {
		DBGMSG("Send NetBIOS to %s success!!\n", inet_ntoa(query_addr));

		FD_ZERO(&fdset);
		FD_SET(server_socket, &fdset);
		tm_val.tv_sec = 0;
		tm_val.tv_usec = 100000;    /* 0.1 seconds */

		if (select(server_socket+1, &fdset, (fd_set *)NULL, (fd_set *)NULL, &tm_val) == -1)
			printf("select() fail\n");

		if (FD_ISSET(server_socket, &fdset)) {
			retstatus = recvfrom(server_socket, recv_buffer, sizeof(recv_buffer), 0, (struct sockaddr *) &server_socka, &addr_len);

			//receive the response and then parse it.
			//finally, store the result to the temp file.
			DBGMSG("Recv NetBIOS data, length = %d\n", retstatus);
			if (retstatus > 0) {
				ptr = recv_buffer;
				//get host name
				ptr += sizeof(netbiosMsg) + sizeof(time_to_live) + sizeof(data_length) + sizeof(number_of_name);
				memcpy(name, ptr, 16);

				DBGMSG("NetBIOS name = %s, pc_count=%d\n", name, pc_count);

				for (count=0; count<pc_count; count++) {
					DBGMSG("lan_pc[%d].ip=%s, mac=%s\n",
						count, inet_ntoa(lan_pc[count].ip), lan_pc[count].mac);

					if (server_socka.sin_addr.s_addr==lan_pc[count].ip.s_addr) {
						strncpy(lan_pc[count].name, name, 16);
						DBGMSG("found lan_pc[%d].ip=%s, name=%s\n",
							count, inet_ntoa(lan_pc[count].ip), lan_pc[count].name);
						break;
					}
				}
			}
			else
				printf("Rcv nothing about NetBIOS from %s\n", inet_ntoa(query_addr));
		}

   		close(server_socket);
	}

	return ret;
}

/* FIXME: match response against chaddr */
int arpping(uint32_t yiaddr, uint32_t ip, uint8_t *mac, char *interface)
{
	int optval = 1;
	int	s;			/* socket */
	int	rv = 1;			/* return value */
	struct sockaddr addr;		/* for interface name */
	struct arpMsg	arp;
	fd_set fdset;
	struct timeval	tm;
	char client_hostname[20] = {};

	if ((s = socket (PF_PACKET, SOCK_PACKET, htons(ETH_P_ARP))) == -1) {
		close(s);
		return -1;
	}

	if (setsockopt(s, SOL_SOCKET, SO_BROADCAST, &optval, sizeof(optval)) == -1) {
		close(s);
		return -1;
	}

	/* send arp request */
	memset(&arp, 0, sizeof(arp));
	memcpy(arp.h_dest, MAC_BCAST_ADDR, 6);		/* MAC DA */
	memcpy(arp.h_source, mac, 6);			/* MAC SA */
	arp.h_proto = htons(ETH_P_ARP);			/* protocol type (Ethernet) */
	arp.htype = htons(ARPHRD_ETHER);		/* hardware type */
	arp.ptype = htons(ETH_P_IP);			/* protocol type (ARP message) */
	arp.hlen = 6;					/* hardware address length */
	arp.plen = 4;					/* protocol address length */
	arp.operation = htons(ARPOP_REQUEST);		/* ARP op code */
	memcpy(arp.sInaddr, &ip, sizeof(ip));		/* source IP address */
	memcpy(arp.sHaddr, mac, 6);			/* source hardware address */
	memcpy(arp.tInaddr, &yiaddr, sizeof(yiaddr));	/* target IP address */

	memset(&addr, 0, sizeof(addr));
	strcpy(addr.sa_data, interface);
	if (sendto(s, &arp, sizeof(arp), 0, &addr, sizeof(addr)) < 0)
		rv = 0;

	FD_ZERO(&fdset);
	FD_SET(s, &fdset);
  	tm.tv_sec = 0;
  	tm.tv_usec = 100000;    /* 0.1 second */

	if (select(s + 1, &fdset, (fd_set *) NULL, (fd_set *) NULL, &tm) < 0) {
		if (errno != EINTR)
			rv = 0;
	}
	else if (FD_ISSET(s, &fdset)) {
		if (recv(s, &arp, sizeof(arp), 0) < 0)
			rv = 0;

		if (arp.operation == htons(ARPOP_REPLY) &&
			bcmp(arp.tHaddr, mac, 6) == 0 &&
			*((uint32_t *) arp.sInaddr) == yiaddr)
		{
			rv = 0;

			sprintf(lan_pc[pc_count].mac, "%02X:%02X:%02X:%02X:%02X:%02X",
					arp.sHaddr[0],arp.sHaddr[1], arp.sHaddr[2], arp.sHaddr[3], arp.sHaddr[4], arp.sHaddr[5]);
			lan_pc[pc_count].ip.s_addr = yiaddr;

			DBGMSG("get arpping: lan_pc[%d].ip=%s, mac=%s\n",
					pc_count, inet_ntoa(lan_pc[pc_count].ip), lan_pc[pc_count].mac);

			pc_count++;
			query_hostname(arp.sHaddr, yiaddr, client_hostname);
		}
	}

	close(s);
	return rv;
}
