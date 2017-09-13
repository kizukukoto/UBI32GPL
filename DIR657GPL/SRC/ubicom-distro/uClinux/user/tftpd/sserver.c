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
#include <sutil.h>
#include <signal.h>
#include "tftp.h" 

#ifdef TFTPD_DEBUG
#define DEBUG_MSG(fmt, arg...)       printf("%s [%d]: "fmt, __FILE__, __LINE__, ##arg)
#else
#define DEBUG_MSG(fmt, arg...) 
#endif

int TimeOut,NumberTimeOut,PortTFTP;
    
typedef struct connect_info_tag_S 
{
    char *p_name;
    struct sockaddr_in adresse;
    short job;
} connect_info_S;

int create_socket(int type, int *p_port)
{
    int socketID; 					
    unsigned int len=sizeof(struct sockaddr_in);	
    struct sockaddr_in adresse;
    /* create a socket */
    if ((socketID=socket(AF_INET, type, 0))==-1)
    {
        DEBUG_MSG("Creation socket failure");
        return -1;
    };
  
    adresse.sin_family=AF_INET;
    adresse.sin_addr.s_addr=htonl(INADDR_ANY);
    adresse.sin_port=htons(*p_port); 

    /* Bind the socket. */
    if (bind(socketID,(struct sockaddr *)&adresse,len)==-1)
    {
        DEBUG_MSG("bind socket failure.\n");
        close(socketID);
        return -1;
    };
  
  return socketID;
}
       
void tftpd_general(void *p_info, int socket_ID)
{
    connect_info_S *p_con_info=(connect_info_S *)p_info;
    FILE *f;
    int result=255;

    if (p_con_info->job==RRQ)
    { 
/*
* 	Date: 2009-5-22 
* 	Name: Ken_Chiang
* 	Reason: modify for get wps_default_pin.
* 	Notice :
*/	
/*
		f=fopen(p_con_info->p_name,"rb");
*/
        if(strstr(p_con_info->p_name,"wps_default_pin.txt"))
        	f=fopen("/var/tmp/wps_default_pin.txt","rb");
        else
        	f=fopen(p_con_info->p_name,"rb");	
        if(f == NULL)
        {
        	printf("can't open name=%s\n",p_con_info->p_name);
        	nak(socket_ID, &p_con_info->adresse, errno,strerror(errno));
        	tftp_free(p_con_info->p_name);
        	tftp_free(p_con_info);
        	close(socket_ID);
        	return ;
        }
        result=tftp_send(&p_con_info->adresse,p_con_info->p_name,"octet",0,TFTPsread,f);
        fclose(f);
    } 
    else if (p_con_info->job==WRQ)
    {
        result=tftp_receive(&p_con_info->adresse,p_con_info->p_name,"octet",0,TFTPswrite,f);
    }
    else
    {
        DEBUG_MSG("TFTP op code is not correct \n");
    }

    if (result!=0) DEBUG_MSG("TFTP error\n");
    tftp_free(p_con_info->p_name);
    tftp_free(p_con_info);
    return ;
}

int tftp_connection(struct sockaddr_in _adresse, short _job, char *_name, int socket_ID)
{
    
    char *p_copy_buf=_name;
    connect_info_S *p_con_info;
    int pid;

    printf("tftp_connection\n");
    while ((p_copy_buf<_name+SEGSIZE-5)&&(*p_copy_buf!=' ')&&(*p_copy_buf!='\0')) p_copy_buf++;
     /* Corrupt packet - no final NULL */
    if ((*p_copy_buf!= ' ')&&(*p_copy_buf!='\0'))
    {
        DEBUG_MSG("file name is corrupted. \n");
        nak(socket_ID, &_adresse, (int)EBADOP,strerror(EBADOP));
        close(socket_ID);
        return 255;
    };
    *p_copy_buf='\0';
    p_con_info=(connect_info_S*)malloc(sizeof(connect_info_S));
    p_con_info->p_name=(char*)malloc(p_copy_buf-_name+1);
    strcpy(p_con_info->p_name,_name);
    p_con_info->adresse=_adresse; 
    p_con_info->job=_job; 

/*
 Reason: change fork to vfork, because fork is not working
 Modified: John Huang
 Date: 2009.08.11
*/ 
    pid = vfork();
    if(pid < 0)
    	printf("TFTP fork error");
    else if(pid == 0)    //child process
    { 
        tftpd_general(p_con_info, socket_ID);
        _exit(0);   
    }
    else                    //parent process
        ;

    return 0;
}

int main(int argc, char *argv[])
{
    struct sockaddr_in adresse;
    size_t length=sizeof(adresse);
    int result;
    int desc_socket;
    char   buf[PKTSIZE];
    struct tftphdr *p_tftp_pkt=(struct tftphdr *)&buf;
    
    TimeOut=3;
    NumberTimeOut=7;
    PortTFTP = 69;      //server listen on port 69

    DEBUG_MSG("TFTP main\n");
    /* create a socket */
    if ((desc_socket=create_socket(SOCK_DGRAM, &PortTFTP))==-1)
    {
	    DEBUG_MSG("create socket error %d", errno);
	    exit(2);
    };
 
    DEBUG_MSG("standard_tftp_server launched on port %d.\n",PortTFTP);

    while (1)
    {
        do
            result=recvfrom(desc_socket,p_tftp_pkt,PKTSIZE,0,(struct sockaddr *)&adresse,&length);
        while (result<0);
        p_tftp_pkt->th_opcode=htons((u_short)p_tftp_pkt->th_opcode);
        if ((p_tftp_pkt->th_opcode == WRQ)||(p_tftp_pkt->th_opcode == RRQ))
            tftp_connection(adresse,p_tftp_pkt->th_opcode,&p_tftp_pkt->th_stuff[0], desc_socket);
    };
}
