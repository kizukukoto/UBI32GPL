/* dhcpd.c
 *
 * udhcp Server
 * Copyright (C) 1999 Matthew Ramsay <matthewr@moreton.com.au>
 *                      Chris Trew <ctrew@moreton.com.au>
 *
 * Rewrite by Russ Dill <Russ.Dill@asu.edu> July 2001
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <time.h>
#include <sys/time.h>

#include "dhcpd.h"
#include "arpping.h"
#include "socket.h"
#include "options.h"
#include "files.h"
#include "serverpacket.h"
#include "common.h"
#include "signalpipe.h"
#include "static_leases.h"
#include <shvar.h>
/* jimmy added 0908 */
//#include <nvram.h>
//#include <sutil.h>
#include <stdarg.h>

char nvram_lan_ip[20];
char nvram_lan_mask[20];

extern char dns_name[];
extern int getDomainNameFromGUI;

/* globals */
struct dhcpOfferedAddr *leases;
struct server_config_t server_config;
#ifdef UDHCPD_REVOKE
struct dhcpd_revoke_t dhcpd_revoke;
extern int read_revoke(const char *file);//in files.c
#endif


/* ----------------- */
/*      Date: 2009-01-07
*       Name: jimmy huang
*       Reason: Avoid compiler warning messages
*       Note:   Add the function prototype
*/
extern void get_dns_name(char *dns_name_input);//serverpacket.c

/* ----------------- */
/*      Date: 2009-01-07
*       Name: jimmy huang
*       Reason: Remove the dependency of nvram
*       Note:
*/
#define NVRAM_FILE "/var/etc/nvram.conf"
#define KEYWORD "mac_filter_type"
#define TMP_FILE "/tmp/useless.txt"

/*      Date: 2009-01-07
*       Name: jimmy huang
*       Input:
*       Output: mac_filter_type in nvram
*               0: disable
*               1: enable with list_allow
*               2: enable with list_deny
*/
int get_mac_filter_type(void){
    char buff[256];
    char *ptr = NULL;
    int status = 0;
    FILE *fp;

    sprintf(buff,"cat %s |grep \"%s\" > %s",NVRAM_FILE,KEYWORD,TMP_FILE);
    system(buff);

    if((fp = fopen(TMP_FILE,"r"))!=NULL){
        while(!feof(fp)){
            memset(buff,'\0',sizeof(buff));
            fgets(buff,sizeof(buff),fp);
            if(buff[0] != '\n' && buff[0] != '\0' && buff[0] != '#'){   //ignore #!/bin.sh
                if( strncmp(buff,KEYWORD,strlen(KEYWORD)) == 0){
                    ptr = strchr(buff,'=');
                    ptr++;
                    if(strncmp(ptr,"list_allow",strlen("list_allow")) == 0){
                        status = 1;
                        break;
                    }
                    if(strncmp(ptr,"list_deny",strlen("list_deny")) == 0){
                        status = 2;
                        break;
                    }
                }
            }
        }
        fclose(fp);
    }
    unlink(TMP_FILE);
    return status;
}

/*      Date: 2009-01-07
*       Name: jimmy huang
*       Input:
*       Output:
*       Note:   Copy from sutil.c, getStrtok()
*/
int getStrtok(char *input, char *token, char *fmt, ...)
{
        va_list  ap;
        int arg, count = 0;
        char *c, *tmp;

        if (!input)
                return 0;
        tmp = input;
        for(count=0; (tmp=strpbrk(tmp, token)); tmp+=1, count++);
        count +=1;

        va_start(ap, fmt);
        for (arg = 0, c = fmt; c && *c && arg < count;) {
                if (*c++ != '%')
                        continue;
                switch (*c) {
                        case 'd':
                                if(!arg)
                                        *(va_arg(ap, int *)) = atoi(strtok(input, token));
                                else
                                        *(va_arg(ap, int *)) = atoi(strtok(NULL, token));
                                break;
                        case 's':
                                if(!arg) {
                                        *(va_arg(ap, char **)) = strtok(input, token);
                                } else
                                        *(va_arg(ap, char **)) = strtok(NULL, token);
                                break;
                }
                arg++;
        }
        va_end(ap);

        return arg;
}


#ifdef COMBINED_BINARY
int udhcpd_main(int argc, char *argv[])
#else
int main(int argc, char *argv[])
#endif
{
        fd_set rfds;
        struct timeval tv;
        int server_socket = -1;
        int bytes, retval;
        struct dhcpMessage packet;
        uint8_t *state;
        uint8_t *server_id, *requested;
        uint32_t server_id_align, requested_align;
        unsigned long timeout_end;
        struct option_set *option;
        struct dhcpOfferedAddr *lease;
        struct dhcpOfferedAddr static_lease;
        int max_sock;
        unsigned long num_ips;
        struct in_addr lan_ip,lan_mask,result_nvram_lan,result_dhcp;
        uint32_t static_lease_ip;
//      struct option_set *curr_opt;
        /* ----------------- */
        /*      Date: 2009-01-07
        *       Name: jimmy huang
        *       Reason: Record the type of mac_filter_type
        *       Note: variable mac_filter_type
        */
        int mac_filter_type = 0;
    /*  Date: 2009-03-05
    *   Name: Jackey Chen
    *   Reason: DHCPD will disappear with unknown reason when recieve a signal that not include 
        *                       SIGUSR1 & SIGUSR2 in RTL Soultion. To fix it, check a file when dhcpd enable.
    *   Note:Only RTL will create this file.So it does not effect other model.
        */
        FILE *read_cl_fp;

        memset(&server_config, 0, sizeof(struct server_config_t));
        read_config(argc < 2 ? DHCPD_CONF_FILE : argv[1]);

        if ((option = find_option(server_config.options, DHCP_ROUTER)))
                sprintf(nvram_lan_ip, "%d.%d.%d.%d", option->data[2], option->data[3], option->data[4], option->data[5]);

        if ((option = find_option(server_config.options, DHCP_SUBNET)))
                sprintf(nvram_lan_mask, "%d.%d.%d.%d", option->data[2], option->data[3], option->data[4], option->data[5]);

        LOG(LOG_INFO, "DHCP server start.");
        LOG(LOG_INFO, "device_lan_ip=%s , device_lan_subnet_mask=%s", nvram_lan_ip, nvram_lan_mask);

        inet_aton(nvram_lan_ip, &lan_ip);
    inet_aton(nvram_lan_mask, &lan_mask);
        result_nvram_lan.s_addr = (lan_ip.s_addr & lan_mask.s_addr);

        /* Start the log, sanitize fd's, and write a pid file */
        start_log_and_pid("udhcpd", server_config.pidfile);

        if ((option = find_option(server_config.options, DHCP_LEASE_TIME))) {
                memcpy(&server_config.lease, option->data + 2, 4);
                server_config.lease = ntohl(server_config.lease);
        }
        else server_config.lease = LEASE_TIME;

        /* Sanity check */
        num_ips = ntohl(server_config.end) - ntohl(server_config.start) + 1;
        if (server_config.max_leases > num_ips) {
                LOG(LOG_ERR, "max_leases value (%lu) not sane, "
                                "setting to %lu instead",
                                server_config.max_leases, num_ips);
                server_config.max_leases = num_ips;
        }

        leases = xcalloc(server_config.max_leases, sizeof(struct dhcpOfferedAddr));
        read_leases(server_config.lease_file);

        if (read_interface(server_config.interface, &server_config.ifindex,
                                &server_config.server, server_config.arp) < 0)
                return 1;

#ifndef UDHCP_DEBUG
        background(server_config.pidfile); /* hold lock during fork. */
#endif

        /* Setup the signal pipe */
        udhcp_sp_setup();

        timeout_end = time(0) + server_config.auto_time;
        /*  Date: 2009-03-05
    *   Name: Jackey Chen
    *   Reason: DHCPD will disappear with unknown reason when recieve a signal that not include
    *           SIGUSR1 & SIGUSR2 in RTL Soultion. To fix it, check a file when dhcpd enable.
    *   Note:Only RTL will create this file.So it does not effect other model.
    */
        if(read_cl_fp = fopen("/var/tmp/dhcpd_clinet_list_info","r"))
        {
                read_leases("/var/tmp/dhcpd_clinet_list_info");
                fclose(read_cl_fp);
        }

        while(1) { /* loop until universe collapses */

                if (server_socket < 0)
                        if ((server_socket = listen_socket(INADDR_ANY, SERVER_PORT, server_config.interface)) < 0) {
                                LOG(LOG_ERR, "FATAL: couldn't create server socket, %m");
                                return 2;
                        }

                max_sock = udhcp_sp_fd_set(&rfds, server_socket);
                if (server_config.auto_time) {
                        tv.tv_sec = timeout_end - time(0);
                        tv.tv_usec = 0;
                }
                if (!server_config.auto_time || tv.tv_sec > 0) {
                        retval = select(max_sock + 1, &rfds, NULL, NULL,
                                        server_config.auto_time ? &tv : NULL);
                } else retval = 0; /* If we already timed out, fall through */

                if (retval == 0) {
                        timeout_end = time(0) + server_config.auto_time;
                        continue;
                } else if (retval < 0 && errno != EINTR) {
                        DEBUG(LOG_INFO, "error on select");
                        continue;
                }

                switch (udhcp_sp_read(&rfds)) {
                        case SIGUSR1:
                                //LOG(LOG_DEBUG, "UDHCPD Received a SIGUSR1");
                                write_leases();
                                /* why not just reset the timeout, eh */
                                timeout_end = time(0) + server_config.auto_time;
                                continue;
#ifdef UDHCPD_REVOKE
                        case SIGUSR2:
                                LOG(LOG_DEBUG, "UDHCPD Received a SIGUSR2");
                                read_revoke(DHCPD_REVOKE_FILE);
                                clear_lease(dhcpd_revoke.mac, dhcpd_revoke.ip);
                                continue;
#endif
/* Date: 2009-01-22
*  Name: Jackey Chen
*  Reason: Keep dhpc client list info when dhcp server was killed by unknow reason
*/
                        case SIGSYS:
                LOG(LOG_DEBUG, "UDHCPD Received a SIGSYS"); 
                read_leases("/var/tmp/dhcpd_clinet_list_info"); 
                                continue;
                        case SIGTERM:
                                LOG(LOG_DEBUG, "UDHCPD Received a SIGTERM");
                                return 0;
                        case 0:
                                break;          /* no signal */
                        default:
                                continue;       /* signal or error (probably EINTR) */
                }

                if ((bytes = get_packet(&packet, server_socket)) < 0) { /* this waits for a packet - idle */
                        if (bytes == -1 && errno != EINTR) {
                                DEBUG(LOG_INFO, "error on read, %m, reopening socket");
                                close(server_socket);
                                server_socket = -1;
                        }
                        continue;
                }

                if ((state = get_option(&packet, DHCP_MESSAGE_TYPE)) == NULL) {
                        DEBUG(LOG_ERR, "couldn't get option from packet, ignoring");
                        continue;
                }

                /* Look for a static lease */
                static_lease_ip = getIpByMac(server_config.static_leases, &packet.chaddr);

/* 
        jimmy added 0908 , to prevent during booting time, when mac filter is list allow,
        dhcpd response to the client which we should not response
*/
#if !(ONLY_FILTER_WLAN_MAC)
//              if((nvram_match("mac_filter_type", "list_allow") ==0) || (nvram_match("mac_filter_type", "list_deny") ==0)){
//                      int list_allow = 0;
//                      int i = 0, ignore = 0;
//                      char *getValue, *mac = NULL, *name = NULL, *enable = NULL, *schedule = NULL;
//                      char fw_value[192]={0};
//                      char fw_rule[]="fw_XXXXXXXXXXXXXXXXXXXXXXXXXX";
//                      int mac_tmp[6];

//                      if(nvram_match("mac_filter_type", "list_allow") == 0){
//                              list_allow = 1;
//                              ignore = 1;
//                      }

//              for(i=0; i<MAX_MAC_FILTER_NUMBER; i++){
//                              snprintf(fw_rule, sizeof(fw_rule), "mac_filter_%02d",i) ;
//                              getValue = nvram_safe_get(fw_rule);
//                              if ( getValue == NULL ||  strlen(getValue) == 0 )
//                                      continue;
//                              else
//                                      strncpy(fw_value, getValue, sizeof(fw_value));

//                              if( getStrtok(fw_value, "/", "%s %s %s %s", &enable, &name, &mac, &schedule) !=4 )
//                                      continue;

//                              if(atoi(enable) != 1){
//                                      continue;
//                              }

//                              if(strcmp(schedule, "Never") == 0)
//                                      continue;

//                              if(strcmp("00:00:00:00:00:00", mac) == 0)
//                                      continue;

//                              sscanf(mac,"%02x:%02x:%02x:%02x:%02x:%02x",
//                                              &mac_tmp[0],&mac_tmp[1],&mac_tmp[2],&mac_tmp[3],&mac_tmp[4],&mac_tmp[5]);
//                              DEBUG(LOG_DEBUG,"mac_filter_%02d  = %02x:%02x:%02x:%02x:%02x:%02x \n",
//                              i,
//                              mac_tmp[0],mac_tmp[1],mac_tmp[2],
//                              mac_tmp[3],mac_tmp[4],mac_tmp[5]);

//                              DEBUG(LOG_DEBUG, "Receive packet from mac = %02x:%02x:%02x:%02x:%02x:%02x \n",
//                              packet.chaddr[0],packet.chaddr[1],packet.chaddr[2],
//                              packet.chaddr[3],packet.chaddr[4],packet.chaddr[5]);

//                              if(list_allow){
//                                      /* list_allow */
//                                      if((mac_tmp[0] == packet.chaddr[0]) && (mac_tmp[1] == packet.chaddr[1]) && (mac_tmp[2] == packet.chaddr[2])
//                                                       && (mac_tmp[3] == packet.chaddr[3]) && (mac_tmp[4] == packet.chaddr[4]) && (mac_tmp[5] == packet.chaddr[5])
//                                              ){
//                                              ignore = 0;
//                                              break;
//                                      }
//                              }else{
//                                      /* list_deny */
//                                      if((mac_tmp[0] == packet.chaddr[0]) && (mac_tmp[1] == packet.chaddr[1]) && (mac_tmp[2] == packet.chaddr[2])
//                                                       && (mac_tmp[3] == packet.chaddr[3]) && (mac_tmp[4] == packet.chaddr[4]) && (mac_tmp[5] == packet.chaddr[5])
//                                              ){
//                                                      DEBUG(LOG_DEBUG, "mac  %02x:%02x:%02x:%02x:%02x:%02x is not allow in mac_filter , ignore this request !!!\n\n\n",
//                                                      mac_tmp[0],mac_tmp[1],mac_tmp[2],
//                                                      mac_tmp[3],mac_tmp[4],mac_tmp[5]);
//                                              ignore = 1;
//                                              break;
//                                      }
//                              }
//              }
//                      if(ignore){
//                              DEBUG(LOG_DEBUG, "mac  %02x:%02x:%02x:%02x:%02x:%02x is not allow in mac_filter , ignore this request !!!\n\n\n",
//                              mac_tmp[0],mac_tmp[1],mac_tmp[2],
//                              mac_tmp[3],mac_tmp[4],mac_tmp[5]);
//                              continue;
//                      }
//          }
                /*      Date: 2009-01-07
                *       Name: jimmy huang
                *       Reason: to remove the dependency of nvram
                *       Note: Modified the codes above to the new codes below
                */
                mac_filter_type = get_mac_filter_type();
                if(mac_filter_type != 0){
                        FILE *fp;
                        int i = -1;
                        char *mac = NULL, *name = NULL, *enable = NULL, *schedule = NULL;
                        char fw_value[192]={0};
                        char *ptr = NULL;
                        int mac_tmp[6];
                        char buff[256];
			int ignore;
			
			// Jery Lin modify 2009/11/17
			// fixed that staion which is not in list can acquire ip from dhcpd when mac_filter_type is list_allow
			if (mac_filter_type==1)		//if mac_filter_type is list_allow
				ignore=1;		//   default set deny
			else if(mac_filter_type==2)	//else if mac_filter_type is list_deny
				ignore=0;		//   default set allow
			
                        memset(buff,'\0',sizeof(buff));

                        sprintf(buff,"cat %s |grep \"mac_filter_\" |grep -v \"type\" > %s",NVRAM_FILE,TMP_FILE);
                system(buff);

                        if((fp = fopen(TMP_FILE,"r"))!=NULL){
                                while(!feof(fp)){
                                        memset(buff,'\0',sizeof(buff));
                                        fgets(buff,sizeof(buff),fp);
                                        ptr = NULL;
                                        i++;
                                        if(buff[0] != '\n' && buff[0] != '\0'){
                                                ptr = strchr(buff,'=');
                                                if(!ptr){
                                                        continue;
                                                }
                                                ptr++;
                                                strncpy(fw_value,ptr,sizeof(fw_value));
                                                if( getStrtok(fw_value, "/", "%s %s %s %s", &enable, &name, &mac, &schedule) !=4 ){
                                                        continue;
                                                }
                                                if(atoi(enable) != 1){
                                                        continue;
                                                }
                                                if(strcmp(schedule, "Never") == 0)
                                                        continue;
                                                if(strcmp("00:00:00:00:00:00", mac) == 0)
                                                        continue;
                                                sscanf(mac,"%02x:%02x:%02x:%02x:%02x:%02x",
                                                        &mac_tmp[0],&mac_tmp[1],&mac_tmp[2],&mac_tmp[3],&mac_tmp[4],&mac_tmp[5]);
                                                DEBUG(LOG_DEBUG,"mac_filter_%02d  = %02x:%02x:%02x:%02x:%02x:%02x \n",
                                                        i,
                                                        mac_tmp[0],mac_tmp[1],mac_tmp[2],
                                                        mac_tmp[3],mac_tmp[4],mac_tmp[5]);

                                                DEBUG(LOG_DEBUG, "Receive packet from mac = %02x:%02x:%02x:%02x:%02x:%02x \n",
                                                        packet.chaddr[0],packet.chaddr[1],packet.chaddr[2],
                                                        packet.chaddr[3],packet.chaddr[4],packet.chaddr[5]);
                                                if(mac_filter_type == 1){
                                                        /* list_allow */
                                                        if((mac_tmp[0] == packet.chaddr[0]) && (mac_tmp[1] == packet.chaddr[1]) && (mac_tmp[2] == packet.chaddr[2])
                                                                        && (mac_tmp[3] == packet.chaddr[3]) && (mac_tmp[4] == packet.chaddr[4]) && (mac_tmp[5] == packet.chaddr[5])
                                                                ){
                                                                ignore = 0;
                                                                break;
                                                        }
                                                }else{
                                                        /* list_deny */
                                                        if((mac_tmp[0] == packet.chaddr[0]) && (mac_tmp[1] == packet.chaddr[1]) && (mac_tmp[2] == packet.chaddr[2])
                                                                        && (mac_tmp[3] == packet.chaddr[3]) && (mac_tmp[4] == packet.chaddr[4]) && (mac_tmp[5] == packet.chaddr[5])
                                                                ){
                                                                        DEBUG(LOG_DEBUG, "mac  %02x:%02x:%02x:%02x:%02x:%02x is not allow in mac_filter , ignore this request !!!\n\n\n",
                                                                        mac_tmp[0],mac_tmp[1],mac_tmp[2],
                                                                        mac_tmp[3],mac_tmp[4],mac_tmp[5]);
                                                                ignore = 1;
                                                                break;
                                                        }
                                                }
                                        }
                                }
                        }
                        if(ignore){
                                DEBUG(LOG_DEBUG, "mac  %02x:%02x:%02x:%02x:%02x:%02x is not allow in mac_filter , ignore this request !!!\n\n\n",
                                mac_tmp[0],mac_tmp[1],mac_tmp[2],
                                mac_tmp[3],mac_tmp[4],mac_tmp[5]);
                                continue;
                        }
            }
#endif
/* ------------------------------------------------ */

                if(static_lease_ip)
                {
                        memcpy(&static_lease.chaddr, &packet.chaddr, 16);
                        static_lease.yiaddr = static_lease_ip;
                        static_lease.expires = 0;
                        lease = &static_lease;
                        DEBUG(LOG_INFO,"DHCPD: find static_lease_ip");
                }
                else
                {
                        lease = find_lease_by_chaddr(packet.chaddr);
                }

                switch (state[0]) {
                        case DHCPDISCOVER:
                                DEBUG(LOG_INFO,"received DISCOVER");
                                if (sendOffer(&packet) < 0)
                                        LOG(LOG_ERR, "send OFFER failed");
                                break;
                        case DHCPREQUEST:
                                DEBUG(LOG_INFO, "received REQUEST");
                                if(!getDomainNameFromGUI)
                                //get_dns_name(&dns_name);
                                /* ----------------- */
                                /*      Date: 2009-01-07
                                *       Name: jimmy huang
                                *       Reason: Avoid compiler warning message
                                *                       warning: passing arg 1 of `get_dns_name' from incompatible pointer type
                                *       Note:   Modified the codes above
                                */
                                get_dns_name(dns_name);
                                requested = get_option(&packet, DHCP_REQUESTED_IP);
                                server_id = get_option(&packet, DHCP_SERVER_ID);

                                if (requested)
                                        memcpy(&requested_align, requested, 4);
                                if (server_id)
                                        memcpy(&server_id_align, server_id, 4);

                                if (lease)
                                {
                                        if (server_id)
                                        {
                                                /* SELECTING State */
                                                DEBUG(LOG_INFO, "server_id = %08x", ntohl(server_id_align));
                                                if (server_id_align == server_config.server &&
                                                        requested &&
                                                        requested_align == lease->yiaddr)
                                                {
                                                        sendACK(&packet, lease->yiaddr);
                                                }
                                        }
                                        else
                                        {
                                                if (requested)
                                                {
                                                        /* INIT-REBOOT State */
                                                        if (lease->yiaddr == requested_align)
                                                                sendACK(&packet, lease->yiaddr);
                                                        else
                                                                sendNAK(&packet);
                                                }
                                                else
                                                {
                                                        /* RENEWING or REBINDING State */
                                                        if (lease->yiaddr == packet.ciaddr)
                                                                sendACK(&packet, lease->yiaddr);
                                                        else    /* don't know what to do!!!! */
                                                                sendNAK(&packet);
                                                        }
                                                }
                                        }
#if 0
                                else if (server_id) /* what to do if we have no record of the client */
                                {
                                        /* SELECTING State */
                                        LOG(LOG_INFO, "DHCPREQUEST: no record of the client\n");
                                        sendNAK(&packet);
                                }
#endif
                                else if (requested)
                                {
                                        /* INIT-REBOOT State */
                                        if ((lease = find_lease_by_yiaddr(requested_align)))
                                        {
                                                if (lease_expired(lease))
                                                {
                                                        /* probably best if we drop this lease */
                                                        memset(lease->chaddr, 0, 16);
                                                        /* make some contention for this address */
                                                }
                                                else
                                                sendNAK(&packet);
                                        }
                                        else if (requested_align < server_config.start ||
                                                         requested_align > server_config.end)
                                        {
                                                sendNAK(&packet);
                                        }
                                        else/* else remain silent */
                                                sendNAK(&packet);
                                }
                                        else
                                {
                                        /* RENEWING or REBINDING State */
                                        result_dhcp.s_addr = (packet.ciaddr & lan_mask.s_addr);
                                        if(result_nvram_lan.s_addr == result_dhcp.s_addr)
                                        {
                                                LOG(LOG_DEBUG,"send ack in RENEWING or REBINDING State");
                                                sendACK(&packet, packet.ciaddr);
                                }
                                        else
                                        {
                                                LOG(LOG_DEBUG,"send NAK");
                                                sendNAK(&packet);
                                        }
                                }
                                break;
                        case DHCPDECLINE:
                                DEBUG(LOG_INFO,"received DECLINE");
                                if (lease) {
                                        memset(lease->chaddr, 0, 16);
                                        lease->expires = time(0) + server_config.decline_time;
                                }
                                break;
                        case DHCPRELEASE:
                                DEBUG(LOG_INFO,"received RELEASE");
                                if (lease) lease->expires = time(0);
                                break;
                        case DHCPINFORM:
                                DEBUG(LOG_INFO,"received INFORM");
                                get_dns_name(dns_name);
                                send_inform(&packet);
                                break;
                        default:
                                LOG(LOG_WARNING, "unsupported DHCP message (%02x) -- ignoring", state[0]);
                }
        }

        return 0;
}

