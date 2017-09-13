/*
 * TFTP library 
 * copyright (c) 2004 Vanden Berghen Frank  
 *
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 * 
 */
 
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h> 
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <syslog.h>
#include <netinet/in.h>
//#include <sutil.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include "nmbd.h"

#if 0
#define DEBUG_MSG(fmt, arg...)       printf(fmt, ##arg)
#else
#define DEBUG_MSG(fmt, arg...) 
#endif

int TimeOut,NumberTimeOut,PortNMBD;

#define MAX_MSG 200

int create_socket(int type, int *p_port)
{
    int socketID; 					
    unsigned int len=sizeof(struct sockaddr_in);	
    struct sockaddr_in adresse;
    /* create a socket */
    if ((socketID=socket(AF_INET, type, 0))==-1)
    {
        printf("Creation socket failure\n");
        return -1;
    };
  
    adresse.sin_family=AF_INET;
    adresse.sin_addr.s_addr=htonl(INADDR_ANY);
    adresse.sin_port=htons(*p_port); 

    /* Bind the socket. */
    if (bind(socketID,(struct sockaddr *)&adresse,len)==-1)
    {
        printf("bind socket failure.\n");
        close(socketID);
        return -1;
    };
  
  return socketID;
}

/*
 * Write the encoded form of a name into a buffer.
 *	A NETBIOS name can be up to 15 characters.  The 16th character is a special type indicator.
 */
static void netbios_write_name(u8_t *buf, u8_t *name, u8_t type, char pad_char)
{
	u8_t i;
	u8_t name_len = strlen(name);

	/*
	 * All names are encoded into 32 bytes.
	 */
	buf[0] = 0x20;

	/*
	 * Write the name.
	 */
	for (i = 0; i < name_len; i++) {
       		buf[(2 * i) + 1] = (name[i] >> 4) + 'A';
       		buf[(2 * i) + 2] = (name[i] & 0x0f) + 'A';
	}

	/*
	 * Pad to 15.
	 */
	for (i = name_len; i < 15; i++) {
		buf[(2 * i) + 1] = (pad_char >> 4) + 'A';
		buf[(2 * i) + 2] = (pad_char & 0x0f) + 'A';
	}

	/*
	 * Write the encoded type indicator and the terminating nul.
	 * GGG Need to find out more about this and what this is doing, STD19 says nothing about this.
	 */
	buf[31] = (type >> 4) + 'A';
	buf[32] = (type & 0x0f) + 'A';
	buf[33] = 0;
}

/////////////////////////////////////////////////////////////////////////////
int getInAddr( char *interface, ADDR_T type, void *pAddr )
{
    struct ifreq ifr;
    int skfd, found=0;
    struct sockaddr_in *addr;

    skfd = socket(AF_INET, SOCK_DGRAM, 0);

    strcpy(ifr.ifr_name, interface);
    if (ioctl(skfd, SIOCGIFFLAGS, &ifr) < 0)
	return (0);

    if (type == HW_ADDR) {
    	if (ioctl(skfd, SIOCGIFHWADDR, &ifr) >= 0) {
		memcpy(pAddr, &ifr.ifr_hwaddr, sizeof(struct sockaddr));
		found = 1;
	}
    }
    else if (type == IP_ADDR) {
	if (ioctl(skfd, SIOCGIFADDR, &ifr) == 0) {
		addr = ((struct sockaddr_in *)&ifr.ifr_addr);
		*((struct in_addr *)pAddr) = *((struct in_addr *)&addr->sin_addr);
		found = 1;
	}
    }
    else if (type == SUBNET_MASK) {
	if (ioctl(skfd, SIOCGIFNETMASK, &ifr) >= 0) {
		addr = ((struct sockaddr_in *)&ifr.ifr_addr);
		*((struct in_addr *)pAddr) = *((struct in_addr *)&addr->sin_addr);
		found = 1;
	}
    }
    close( skfd );
    return found;

}

void NMBD_process(int sd, struct sockaddr_in *p_to_addr, char *packet, int len)
{
	char ret_info[100];
	int ret_len=0;
	char want_return[] = { 0x85, 0x80, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00 };
	char pad_return[] = { 0x00, 0x03, 0xf4, 0x80, 0x00, 0x06, 0x00, 0x00 };
	char ip_return[] = { 0xc0, 0xa8, 0x01, 0x1e };
	int tolen=sizeof(struct sockaddr_in);
	struct in_addr	intaddr,intmask;
	unsigned long ip_t,mask_t,ip_from;
	
	memcpy(ret_info, packet, len);
	ret_len = len;
	memcpy(&ret_info[2], want_return, 10);
	memcpy(&ret_info[len], pad_return, 8);
	if ( getInAddr("br0", IP_ADDR, (void *)&intaddr ) )
	{
		if( getInAddr("br0", SUBNET_MASK, (void *)&intmask ) )
			mask_t = *((unsigned long *)&intmask);
		else
			mask_t = 0xffffff00;
		ip_from = *((unsigned long *)&p_to_addr->sin_addr);
		ip_t = *((unsigned long *)&intaddr);
		if((ip_from & mask_t) != (ip_t & mask_t))
		{
			printf("Not the same Network %x-%x-%x!\n",ip_from,ip_t,mask_t);
			return;
		}
		ret_info[len+8] = (ip_t >> 24) & 0xff;
		ret_info[len+9] = (ip_t >> 16) & 0xff;
		ret_info[len+10] = (ip_t >> 8) & 0xff;
		ret_info[len+11] = ip_t & 0xff;
	}
	else
		memcpy(&ret_info[len+8], ip_return, 4);
	ret_len += 12;
	DEBUG_MSG("Send NMBD Response!\n");
	sendto(sd, ret_info, ret_len, 0,(struct sockaddr*)p_to_addr, tolen);
}

int main(int argc, char *argv[])
{
    struct sockaddr_in adresse;
    int length=sizeof(adresse);
    int result,i;
    int desc_socket;
  	char line[MAX_MSG];
    char name[17],name_t[17];
    char nname[34];
    
    TimeOut=3;
    NumberTimeOut=7;
    PortNMBD = 137;      //server listen on port 137

    DEBUG_MSG("NMBD main\n");

		{
			FILE *pid_fp;
			/* Daemonize and log PID */
//			if (daemon(1, 1) == -1) {
//				perror("daemon");
//				exit(errno);
//			}
			if (!(pid_fp = fopen("/var/run/nmbd.pid", "w"))) {
				perror("/var/run/nmbd.pid");
				return errno;
			}
			fprintf(pid_fp, "%d", getpid());
			fclose(pid_fp);
		}

    /* create a socket */
    if ((desc_socket=create_socket(SOCK_DGRAM, &PortNMBD))==-1)
    {
	    printf("create socket error %d", errno);
	    exit(2);
    };
 
    DEBUG_MSG("NMBD launched on port %d.\n",PortNMBD);

	memset(name,0,17);
	memset(name_t,0,17);
	if(argc == 2)
	{
		if(strlen(argv[1]) >= 16)
			strcpy(name_t,"router");
		else
			strcpy(name_t,argv[1]);
	}
	else
		strcpy(name_t,"router");
		
	for(i=0;i<strlen(name_t);i++) name[i] = toupper(name_t[i]);
	DEBUG_MSG("name:%s\n",name);
	netbios_write_name(nname, name, NETBIOS_WORKSTATION, ' ');
			
    while (1)
    {
    	/* init line */
    	memset(line,0x0,MAX_MSG);
    
    	/* receive segments */
        do
            result=recvfrom(desc_socket,line,MAX_MSG,0,(struct sockaddr *)&adresse,&length);
        while (result<0);
	//DEBUG_MSG("\nNMBD: received from %s:UDP %d\n",inet_ntoa(adresse.sin_addr),ntohs(adresse.sin_port));
	//for(i=0;i<result;i++) 
	//	DEBUG_MSG("%x ",line[i]);
	//DEBUG_MSG("\n");
	if(!memcmp(&line[12],nname,31))
	{
		if(line[5] & 1)
		{
			DEBUG_MSG("Get NMBD Request!\n");
			NMBD_process(desc_socket,&adresse,line,result);
		}
		//else
		//	DEBUG_MSG("NMBD Abort!\n");
	}
    };
}
