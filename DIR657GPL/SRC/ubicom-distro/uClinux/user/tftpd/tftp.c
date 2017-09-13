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

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <syslog.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <unistd.h>
//#include <stropts.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sutil.h>
#include <signal.h>
#include <bsp.h>

#include <nvram.h>
#include <shvar.h>
#include "tftp.h"

//#define TFTPD_DEBUG 1
#ifdef TFTPD_DEBUG
#define DEBUG_MSG(fmt, arg...)       printf("%s [%d]: "fmt, __FILE__, __LINE__, ##arg)
#else
#define DEBUG_MSG(fmt, arg...)
#endif

#define HWID_LEN                             24
/*   Date: 2009-2-18
*   Name: Ken Chiang
*   Reason: To use different hardware country ID for US and other regions for D-Link.
*   Notice :
*/
#ifdef HWCOUNTRYID
#define HWCOUNTRYID_LEN                       2
#endif
#define sys_reboot() kill(1, SIGTERM)

#define ART_FILE           "/var/firm/art.out" //"/bin/mdk_client.out"
static char calCheckId[]={0x35,0x33,0x31,0x31,0x13,0x3e,0x00,0x05,0x41,0x74,0x68,0x65,0x72,0x6f,0x73,0x20};

#define lswap(x) ((((x) & 0xff000000) >> 24) | \
                  (((x) & 0x00ff0000) >>  8) | \
                  (((x) & 0x0000ff00) <<  8) | \
                  (((x) & 0x000000ff) << 24))

static char *check_mac_scope(char *file_buf);
static char *add_one_to_macaddr(char mac_addr[]);
static unsigned long conver_to_interger(char *src);
static char* remove_tftp_mac_sep(char *p_ori_mac_addr);
int upgrade_firmware(char *p_file_buff, int image_len);

extern int create_socket(int type, int *p_port);
extern int TimeOut, NumberTimeOut;

static char* check_mac_scope(char *file_buf)
{
	char *tokenPtr ;
	char *mapping_string ="ffffff";
	char temp[4];
	char macString[20];
	int counter = 5 ;
	memset(temp,0,sizeof(temp));
	memset(macString,0,sizeof(macString));
	tokenPtr = strtok(file_buf,":");
	while(tokenPtr != NULL){
		/*if search the token has the "ffffff" string then cat the substring  ex: file_buf is 00:11:22:33:44:80 ,the '80' of file_buf exceed 128 in the decimal presentation , then it will show  ffffff80 in the hexdecimal presentation */
        	if(strstr(tokenPtr,mapping_string) != NULL){
                	strncpy(temp,strstr(tokenPtr,(tokenPtr+6)),2);
			strcat(macString,temp);
			if(counter > 0)
                        strcat(macString,":");
        	}else{
			strcat(macString,tokenPtr);
			if(counter > 0)
			   strcat(macString,":");
		}
		tokenPtr = strtok(NULL,":");
		counter--;
	}
	return macString;
}

/*
 *  converts a string containing a hex number(exceeds unsigned long integer range)
 *  into a decimal interger.
 */
static char *add_one_to_macaddr(char mac_addr[])
{
	unsigned long mac_number[3] ={0};
	char *mac_final;
	int i;
	char temp[5];

	mac_final = (char*)malloc(18);
	if(mac_final == NULL)
		return NULL;

	memset(mac_final, 0, 18);
	for(i= 0 ; i<3 ; i++)
	{
		memset(temp, 0, 5);
		memcpy(temp, mac_addr+i*4 , 4);
		mac_number[i] = conver_to_interger(temp);
	}

	mac_number[2] = mac_number[2] + 1 ;
	if((mac_number[2]) > 65535)
	{
		mac_number[2] = 0;
		mac_number[1] = mac_number[1] + 1 ;
		if((mac_number[1]) > 65535)
		{
			mac_number[1] = 0;
			mac_number[0] = mac_number[0] + 1 ;
			if((mac_number[0]) > 65535)
			{
				mac_number[2] = 0;
				mac_number[1] = 0;
				mac_number[0] = 0;
			}
		}
	}

	for(i= 0 ; i<3 ; i++)
		sprintf(mac_final, "%s%04x", mac_final, mac_number[i]);

	return mac_final;
}

/*
 *  converts a string containing hex number into a decimal interger.
 */
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

/* remove mac separator Ex: 11:22:33:44:55:66 => 112233445566 */
static char* remove_tftp_mac_sep(char *p_ori_mac_addr)
{
	int i;

	for(i =0 ; i< 5; i++ )
	{
		memmove(p_ori_mac_addr+2+(i*2), p_ori_mac_addr+3+(i*3), 2);
	}
	memset(p_ori_mac_addr+12, 0, strlen(p_ori_mac_addr)-12);
	return p_ori_mac_addr;

}

/* upgrade firmware */
int upgrade_firmware(char *p_file_buff, int image_len)
{
    int ret = 0, offset= 0;
    char imageHWID[HWID_LEN+2] = {0};
    char *p_hwid ;

#ifdef HWCOUNTRYID
	char imageHWCOUNTRYID[HWCOUNTRYID_LEN+2] = {0};
	char *p_hwcountryid ;
#endif
    printf("sys_upgrade \n");
    DEBUG_MSG("Run time Image size is %02d\n", image_len);
    offset = image_len%SEGSIZE;
    if(!offset)
    	 offset = SEGSIZE;
	DEBUG_MSG("offset size is %02d\n", offset);
#ifdef HWCOUNTRYID
    /* Hardware ID Check */
    p_hwid = p_file_buff + offset - HWID_LEN;
    p_hwcountryid = p_file_buff + offset - HWID_LEN - HWCOUNTRYID_LEN;
	DEBUG_MSG("file_buffer now address 0x%x, Hardware ID address 0x%x Hardware Country ID address 0x%x\n",
                        p_file_buff, p_hwid, p_hwcountryid);
    DEBUG_MSG("Find Match String\n");
    memcpy(imageHWID, p_hwid,HWID_LEN);
    imageHWID[HWID_LEN] = '\0';
    DEBUG_MSG("Image Hardware ID is %s\n",imageHWID);
    DEBUG_MSG("System Hardware ID is %s\n",HWID);
    memcpy(imageHWCOUNTRYID, p_hwcountryid,HWCOUNTRYID_LEN);
    imageHWCOUNTRYID[HWCOUNTRYID_LEN] = '\0';
    DEBUG_MSG("Image Hardware Country ID is %s\n",imageHWCOUNTRYID);
    DEBUG_MSG("System Hardware Country ID is %s\n",HWCOUNTRYID);
    if( (strncmp(p_hwid, HWID, HWID_LEN) == 0) && (strncmp(p_hwcountryid, HWCOUNTRYID, HWCOUNTRYID_LEN) == 0)){
        printf("HWID and HWCountryID is correct.\n");
	}
    else
    {
        if(strncmp(p_hwid, HWID, HWID_LEN) == 0){
        	printf("HWID is correct but HWCountryID is not correct.\n");
        }
        else{
        	printf("HWID and HWCountryID is not correct.\n");
    	}
    	return -1;
   	}
#else
    /* Hardware ID Check */
    p_hwid = p_file_buff + offset - HWID_LEN;
    DEBUG_MSG("file_buffer now address 0x%x\n, Hardware ID address 0x%x \n",
                        p_file_buff, p_hwid);
    DEBUG_MSG("Find Match String\n");
    memcpy(imageHWID, p_hwid,HWID_LEN);
    imageHWID[HWID_LEN] = '\0';
    DEBUG_MSG("Image Hardware ID is %s\n",imageHWID);
    DEBUG_MSG("System Hardware ID is %s\n",HWID);
    if( strncmp(p_hwid, HWID, HWID_LEN) == 0){
        printf("HWID is correct.\n");
    }
    else
    {
    	printf("HWID is not correct. \n");
	    return -1;
   	}
#endif
    /* Write to Flash System */
    printf("Write to Flash System\n");

	/* Kill GPIO and System log procedure */
	//kill(read_pid(RC_PID), SIGINT);
	system("killall gpio");
	system("killall syslogd");
	system("killall klogd");
	system("killall lld2d");
	system("killall dcc");
    sleep(1);
    system("/var/sbin/fwupgrade fw &");
    return ret;
}

/* run art client */
int run_art_client(unsigned char *ptr, unsigned int len)
{
    FILE *fp;
    int count=0;

	printf("run_art_client image_length = 0x%x\n",len);
	system("mv /var/firm/image.bin /var/firm/art.out");

//    kill(read_pid(RC_PID), SIGINT);
    chmod(ART_FILE, S_IXUSR);
    system("ifconfig br0 192.168.0.2");
    system("[ -n \"`lsmod | grep ath_pci`\" ] && rmmod ath_pci");
    system("[ -n \"`lsmod | grep ath_pci`\" ] && rmmod ath_pktlog");
    system("mknod /dev/dk0 c 63 0");
    system("mknod /dev/dk1 c 63 1");
    system("chmod 666 /dev/dk0");
    system("chmod 666 /dev/dk1");
    system("insmod /lib/modules/art.ko");
    system(ART_FILE);//var/firm/art.out
}

/* return 0 if no error.*/
char TFTPswrite(char *p_data,long p_data_amount,char first,void *f)
{
    int result;
    result = fwrite(p_data, p_data_amount,1,(FILE *)f);
    if(result < 0)
        return result;
    else
        return 0;
}

/* return 0 if no error.*/
char TFTPsread(char *p_data,long *p_data_amount,char first,void *f)
{
    *p_data_amount=fread(p_data,1,SEGSIZE,(FILE *)f);
	if(*p_data_amount < 0)
        return *p_data_amount;
 	else
        return 0;
}

void nak(int peer,struct sockaddr_in *p_to_addr,int error,char *p_error_msg)
{
	char buf[PKTSIZE];
	int length, status;
	size_t tolen=sizeof(struct sockaddr_in);
	struct tftphdr *p_tftp_pkt=(struct tftphdr *)&buf;

	printf("error mesg: %s \n", p_error_msg);
	p_tftp_pkt->th_opcode = htons((u_short)ERROR);
	p_tftp_pkt->th_code = htons((u_short)error);
	strcpy(p_tftp_pkt->th_msg, p_error_msg);
	length = strlen(p_tftp_pkt->th_msg);
	p_tftp_pkt->th_msg[length] = '\0';
	length += 5;

	status = sendto(peer, buf, length, 0,(struct sockaddr*)p_to_addr, tolen);
	if ( status!= length)
	{
		printf("send nak failed: %d\n", errno);
	}

}

void tftp_free(void * ptr)
{
    if(ptr)
        free(ptr);
}

int tftp_receive_ext(struct sockaddr_in *p_to_addr,char *p_name,char *p_mode,int InClient,
                     char (*TFTPwrite)(char *,long ,char,void *),
                     void *argu,int vPKTSIZE)
{
    char *p_data_pkt,*p_ack_pkt,*p_data_buf,*p_copy_buf;
    struct tftphdr *p_tftpdata_pkt,*p_tftpack_pkt;
	unsigned long wan_mac;
	unsigned char *p_wan_mac = &wan_mac;
	//added for set hw version
#ifdef SET_GET_FROM_ARTBLOCK
	char bv_cmd[2] = {0x42, 0x56};
#endif
	char wps_cmd[3] = {0x57, 0x70, 0x73};
	char preamble_mac[6] = {0xfe, 0xfe, 0xfe, 0xfe, 0xfe, 0xfe};
	char null_mac[6] = {0, 0, 0, 0, 0, 0};
	char macString[20] = {0};
	char config_buf[30] = {0};
	char mpmode[2][1] = {{0x01}, {0x02}};
	char func[15][1] = {{0x01}, {0x02}, {0x03}, {0x04}, {0x05}, {0x06}, {0x07}, {0x08}, {0x09}, {0x0a}, {0x0b}, {0x0c}, {0x0d}, {0x0e}, {0x0f}};
    int result = 0 , bytes;
    int size, ntimeout, peer, port_no;
    struct timeval tv;
    u_short nextBlockNumber;
    fd_set lecture;
    struct sockaddr_in from,to=*p_to_addr;
    size_t fromlen=sizeof(from),tolen=fromlen;
    char *file_buffer;
    int  count = 0,image_length = 0;
	int rc_restart_reqired = 0; //0=nothing 1=reboot 2=rc_restart
/*
 * Reason: Let MP smooth.
 * Modified: John Huang
 * Date: 2010.02.09
 */
#ifdef UBICOM_MP_SPEED
	int nvram_defaultstore_reqired = 0; //0=nothing 1=restore default
#endif
	FILE *fp = fopen(FW_UPGRADE_FILE,"w+");

    p_data_pkt=(char*)malloc(vPKTSIZE);
    p_ack_pkt=(char*)malloc(vPKTSIZE);
    file_buffer=(char*)malloc(vPKTSIZE);

    if(p_data_pkt == NULL || p_ack_pkt == NULL || file_buffer == NULL || fp == NULL)
    {
        DEBUG_MSG("TFTP: out of memory.\n");
        tftp_free(p_data_pkt);
        tftp_free(p_ack_pkt);
        tftp_free(file_buffer);
        if(fp)
        	fclose(fp);
        return 255;
    }
    printf("TFTP receving..... \n");
    memset(macString,0,sizeof(macString));
    memset(p_data_pkt, 0, vPKTSIZE);
    memset(p_ack_pkt, 0, vPKTSIZE);
    memset(file_buffer, 0, vPKTSIZE);

    p_tftpdata_pkt=(struct tftphdr *)p_data_pkt;
    p_tftpack_pkt=(struct tftphdr *)p_ack_pkt;
    p_data_buf=(char*)p_tftpdata_pkt+4;
    p_copy_buf=(char*)&p_tftpack_pkt->th_stuff[0];

    port_no = 0;
    if ((peer=create_socket(SOCK_DGRAM, &port_no))<0)
    {
        DEBUG_MSG("create socket failure %d:\n", errno);
        tftp_free(p_data_pkt);
        tftp_free(p_ack_pkt);
        tftp_free(file_buffer);
        if(fp)
        	fclose(fp);
        return 255;
    }

    if (InClient)
    {
        p_tftpack_pkt->th_opcode=htons((u_short)RRQ);
        strcpy(p_copy_buf, p_name);
        p_copy_buf += strlen(p_name);
        *p_copy_buf++ = '\0';
        strcpy(p_copy_buf, p_mode);
        p_copy_buf += strlen(p_mode);
        *p_copy_buf++ = '\0';
        size=(unsigned long)p_copy_buf-(unsigned long)p_ack_pkt;
    }
    else
    {
        p_tftpack_pkt->th_opcode=htons((u_short)ACK);
        p_tftpack_pkt->th_block=0;
        size=4;
    }
    nextBlockNumber=1;

    do
    {
        /*next ACK's timeout */
        ntimeout=0;
        do
        {
            if (ntimeout==NumberTimeOut)
            {
                DEBUG_MSG("TFTP timeout");
                close(peer);
                tftp_free(p_data_pkt);
                tftp_free(p_ack_pkt);
                tftp_free(file_buffer);
                if(fp)
        			fclose(fp);
                return 255;
            }
            if (sendto(peer,p_tftpack_pkt,size,0,(struct sockaddr *)&to,tolen)!=size)
            {
                DEBUG_MSG("tftp: write error : %d", errno);
                close(peer);
                tftp_free(p_data_pkt);
                tftp_free(p_ack_pkt);
                tftp_free(file_buffer);
                if(fp)
        			fclose(fp);
                return 255;
            }

        	do
        	{
            	bytes = -1;
            	FD_ZERO(&lecture);
            	FD_SET(peer,&lecture);
            	tv.tv_sec=TimeOut;
           		tv.tv_usec=0;

            	if ((result=select(peer+1, &lecture, NULL, NULL, &tv))==-1)
            	{
                	DEBUG_MSG("tftp: select error : %d", errno);
                	close(peer);
                	tftp_free(p_data_pkt);
                	tftp_free(p_ack_pkt);
                	tftp_free(file_buffer);
                	if(fp)
        				fclose(fp);
                	return 255;
            	}
            	if (result > 0)
            	{
                	memset(p_tftpdata_pkt, 0, vPKTSIZE);
                	bytes = recvfrom(peer, p_tftpdata_pkt, vPKTSIZE, 0,(struct sockaddr *)&from, &fromlen);
            	}
        	} while ((bytes < 0)&&(result > 0));    //recv nothing but select>0, keep recv

         	if (result > 0)
         	{
            	to.sin_port=from.sin_port;
            	p_tftpdata_pkt->th_opcode = ntohs((u_short)p_tftpdata_pkt->th_opcode);
            	p_tftpdata_pkt->th_block = ntohs((u_short)p_tftpdata_pkt->th_block);
            	if (p_tftpdata_pkt->th_opcode != DATA)
            	{
                	DEBUG_MSG("tftp: op code is not correct \n");
                	close(peer);
                	tftp_free(p_data_pkt);
                	tftp_free(p_ack_pkt);
                	tftp_free(file_buffer);
                	if(fp)
        				fclose(fp);
                	return 255;
            	}
            	if (p_tftpdata_pkt->th_block != nextBlockNumber)
            	{
               		/* Re-synchronize with the other side */
               		ioctl(peer, FIONREAD, &bytes);
               		while (bytes)
               		{
                  		recv(peer, p_tftpdata_pkt, vPKTSIZE, 0);
                  		ioctl(peer, FIONREAD, &bytes);
               		}
              	 	p_tftpdata_pkt->th_block=nextBlockNumber+1;
            	}
         	}
         	ntimeout++;
      	} while (p_tftpdata_pkt->th_block!=nextBlockNumber);

        p_tftpack_pkt->th_block=htons(nextBlockNumber);
        nextBlockNumber++;

        /*only use in client */
        if (nextBlockNumber==2)
        {
            p_tftpack_pkt->th_opcode=htons((u_short)ACK);
            size=4;
        }
        if (bytes-4 > 0)
        {
            //printf("bytes-4 = %d\n", bytes-4);
            count = fwrite(p_data_buf,1,bytes-4,fp);
            memcpy(file_buffer, p_data_buf, bytes-4);
            memset(p_data_buf, 0, bytes-4);
            image_length = image_length + bytes-4;
            result =0 ;
        }

    } while (bytes == vPKTSIZE);

    printf("TFTP receive successfully \n");
    /* send the "final" ack */
    sendto(peer, p_tftpack_pkt, 4, 0,(struct sockaddr *)&to,tolen);

    /* upgrade firmware */
	printf("Image length = %02d \n",image_length);

    if (strncmp(p_name, "mdk_client.out", 14) == 0) {
    	// art client
        printf("*****************************************************run_art_client \n");
        system("rc stop");
        system("killall timer");
		system("killall httpd");
		system("killall radvd");
    	if (fp)
    		fclose(fp);
    	fp = NULL;
        run_art_client(file_buffer, image_length);
	}
	else if (image_length > IMG_LOWER_BOUND && image_length < IMG_UPPER_BOUND) {
		//upgrade firmware
		printf("************************************************upgrade firmware \n");
		upgrade_firmware(file_buffer, image_length);
	}
	else {
        //check smac command
        if (memcmp(preamble_mac, file_buffer, 6) == 0){
            printf("*****************************************************smac command \n");
/*
 * Reason: Let MP smooth.
 * Modified: John Huang
 * Date: 2010.02.09
 */
#ifdef UBICOM_MP_SPEED
	    system("echo x > /var/tmp/mp_cmd.tmp");	    
#endif
            //set mac address
            if (memcmp(file_buffer+6, null_mac, 6) && (*(file_buffer+6) & 0x1) == 0)
            {
                char *wan_mac_buf;
                int i;
				printf("************************************************set mac address\n");
                //get lan mac
                sprintf(config_buf, "%02x:%02x:%02x:%02x:%02x:%02x",
                *(file_buffer + 6), *(file_buffer + 7), *(file_buffer + 8),
                *(file_buffer + 9), *(file_buffer + 10), *(file_buffer + 11));
				strcpy(macString,check_mac_scope(config_buf));
				memset(config_buf,0,sizeof(config_buf));
				strcpy(config_buf,macString);
                printf("lan_mac = %s\n", config_buf);
#ifdef SET_GET_FROM_ARTBLOCK
				artblock_set("lan_mac", config_buf);
				nvram_set("lan_mac", config_buf);
#else//SET_GET_FROM_ARTBLOCK
                nvram_set("lan_mac", config_buf);
#endif//SET_GET_FROM_ARTBLOCK
                //get wan mac
                remove_tftp_mac_sep(config_buf);
                wan_mac_buf = add_one_to_macaddr(config_buf);
                memset(config_buf, 0, 30);
                for(i=0 ; i<6 ; i++) {
                    if( i == 5) {
                        sprintf(config_buf+i*3, "%c%c", wan_mac_buf[i*2], wan_mac_buf[i*2+1]);
                        break;
                    }
                    sprintf(config_buf+i*3, "%c%c:", wan_mac_buf[i*2], wan_mac_buf[i*2+1]);
                }
                printf("wan_mac = %s\n", config_buf);
#ifdef SET_GET_FROM_ARTBLOCK
				artblock_set("wan_mac", config_buf);
				nvram_set("wan_mac", config_buf);
#else//SET_GET_FROM_ARTBLOCK
                nvram_set("wan_mac", config_buf);
#endif//SET_GET_FROM_ARTBLOCK
				tftp_free(wan_mac_buf);

/*
 * Reason: Let MP smooth.
 * Modified: John Huang
 * Date: 2010.02.09
 */
#ifdef UBICOM_MP_SPEED
				nvram_defaultstore_reqired = 0; //0=nothing 1=restore default
#else
/*
* 	Date: 2009-5-22
* 	Name: Ken_Chiang
* 	Reason: modify for get wps_default_pin when set mac to do restore default.
* 	Notice :
*/	      		
				system("nvram restore_default");
#endif
#ifdef UBICOM_MP_SPEED
				rc_restart_reqired = 0; 
#else
				rc_restart_reqired = 2;
#endif				
/*
 * Reason: Let MP smooth.
 * Modified: John Huang
 * Date: 2010.02.09
 */
#ifdef UBICOM_MP_SPEED
	    	sprintf(config_buf, "echo %01x > /var/tmp/mp_cmd.tmp", *(file_buffer + 11) & 0xf);
	    	system(config_buf);	    
#endif
            }
            else
            {
                printf("!!!mac addr can not be null & should start with 00!!!\n");
            }

            //set wirelsee domain
            if (*(file_buffer+12) != 0)
            {
                printf("************************************************set wireless domain \n");
                sprintf(config_buf, "0x%02x", *(file_buffer + 12));
                printf("wlan0_domain = %s\n", config_buf);
#ifdef SET_GET_FROM_ARTBLOCK
				artblock_set("wlan0_domain", config_buf);
				nvram_set("wlan0_domain", config_buf);
#else//SET_GET_FROM_ARTBLOCK
				nvram_set("wlan0_domain", config_buf);
#endif//SET_GET_FROM_ARTBLOCK

#ifdef UBICOM_MP_SPEED
				rc_restart_reqired = 0;
#else
				rc_restart_reqired = 2;
#endif        		
/*
 * Reason: Let MP smooth.
 * Modified: John Huang
 * Date: 2010.02.09
 */
#ifdef UBICOM_MP_SPEED
	    	sprintf(config_buf, "echo %01x > /var/tmp/mp_cmd.tmp", *(file_buffer + 12) & 0xf);
	    	system(config_buf);	    
#endif
            }

            //set mpmode and func
            if (memcmp(file_buffer+21, mpmode[0], 1) == 0)
            {
                printf("************************************************set mpmode 1\n");
/*
 * Reason: Let MP smooth.
 * Modified: John Huang
 * Date: 2010.02.09
 */
#ifdef UBICOM_MP_SPEED
	    	sprintf(config_buf, "echo 1 > /var/tmp/mp_cmd.tmp");
	    	system(config_buf);	    
#endif
            }
            else if (memcmp(file_buffer+21, mpmode[1], 1) == 0)
            {
                printf("************************************************set mpmode 2 \n");
/*
 * Reason: Let MP smooth.
 * Modified: John Huang
 * Date: 2010.02.09
 */
#ifdef UBICOM_MP_SPEED
	    	sprintf(config_buf, "echo %d > /var/tmp/mp_cmd.tmp", *(file_buffer + 23) % 10);
	    	system(config_buf);	    
#endif
                if ( memcmp(file_buffer+23,  func[0],  1) == 0){
                	  printf("************************************************set func 1 \n");
                }
            	else if ( memcmp(file_buffer+23, func[1], 1) == 0){
                	  printf("************************************************set func 2 \n");
                	  system("sbin/gpio POWER_LED off");
/*
 * Reason: Let MP smooth.
 * Modified: John Huang
 * Date: 2010.02.09
 */
#ifdef UBICOM_MP_SPEED
#ifdef CONFIG_USER_WISH
                        system("/etc/init.d/stop_wish");
#endif
#ifdef CONFIG_USER_STREAMENGINE2
		  	system("killall se_restart");		  	
		  	system("killall se_do_rate_estimation");
#else
		  	system("killall start_rating");
		  	system("killall streamengine_restart");
		  	system("killall rc_streamengine_restart");
		  	system("killall timer");
		  	system("killall udhcpc");
#endif
		  	system("echo 1 > /var/tmp/mp_testing.tmp");
#endif
                }
            	else if ( memcmp(file_buffer+23, func[2], 1) == 0){
                	  printf("************************************************set func 3 \n");
                	  system("sbin/gpio POWER_LED on");
                }
            	else if ( memcmp(file_buffer+23, func[3], 1) == 0){
                	  printf("************************************************set func 4 \n");
                	  system("sbin/gpio STATUS_LED off");
                }
            	else if ( memcmp(file_buffer+23, func[4], 1) == 0){
                	  printf("************************************************set func 5 \n");
                	  system("sbin/gpio STATUS_LED on");
                }
            	else if ( memcmp(file_buffer+23, func[5], 1) == 0){
                	printf("************************************************set func 6 \n");
                	  system("killall gpio");
                	  system("sbin/gpio WLAN_LED off");
                }
            	else if ( memcmp(file_buffer+23, func[6], 1) == 0){
                	  printf("************************************************set func 7 \n");
                	  system("sbin/gpio WLAN_LED on");
                }
            	else if ( memcmp(file_buffer+23, func[7], 1) == 0){
                	  printf("************************************************set func 8 \n");
                	  system("sbin/gpio WPS off");
/*
 * Reason: Let MP smooth.
 * Modified: John Huang
 * Date: 2010.02.09
 */
#ifdef UBICOM_MP_SPEED
			  system("echo 1 > /var/tmp/mp_testing.tmp");
#endif
                }
            	else if ( memcmp(file_buffer+23, func[8], 1) == 0){
                	  printf("************************************************set func 9 \n");
                	  system("sbin/gpio WPS on");
                }
            	else if ( memcmp(file_buffer+23, func[9], 1) == 0){
                	  printf("************************************************set func 10 \n");
                	  system("sbin/gpio USB_LED off");
                }
            	else if ( memcmp(file_buffer+23, func[10], 1) == 0){
                	  printf("************************************************set func 11 \n");
                	  system("sbin/gpio USB_LED on");
                }
            	else if ( memcmp(file_buffer+23, func[11], 1) == 0){
                	  printf("************************************************set func 12 \n");
                	  system("killall gpio");
                	  system("sbin/gpio INTERNET_LED off green");
                }
            	else if ( memcmp(file_buffer+23, func[12], 1) == 0){
                	  printf("************************************************set func 13 \n");
                	  system("sbin/gpio INTERNET_LED on green");
                }
            	else if ( memcmp(file_buffer+23, func[13], 1) == 0){
                	  printf("************************************************set func 14 \n");
                	  system("sbin/gpio INTERNET_LED off amber");
                }
            	else if ( memcmp(file_buffer+23, func[14], 1) == 0){
                	printf("************************************************set func 15 \n");
                	  system("sbin/gpio INTERNET_LED on amber");
                }
                else{
                	printf("************************************************set unknow func\n");
                }
            }

#ifdef SET_GET_FROM_ARTBLOCK
			//set hw version
			if (memcmp(file_buffer+24, bv_cmd, 2) == 0)
            {
                printf("************************************************set hw version \n");
/*
 * Reason: Let MP smooth.
 * Modified: John Huang
 * Date: 2010.02.09
 */
#ifdef UBICOM_MP_SPEED
	    	sprintf(config_buf, "echo %c > /var/tmp/mp_cmd.tmp", *(file_buffer + 29));
	    	system(config_buf);	    
#endif
                sprintf(config_buf, "%c%c", *(file_buffer + 28),*(file_buffer + 29));
                printf("bv = %s\n", config_buf);
                artblock_set("hw_version", config_buf);
#ifdef UBICOM_MP_SPEED                
                rc_restart_reqired = 0;
#else
                rc_restart_reqired = 2;
#endif                                
            }
#endif
        	//set wps command
        	if (memcmp(file_buffer+36, wps_cmd, 3) == 0)
            {
            	printf("wps_cmd = %s\n", file_buffer+36);
/*
 * Reason: Let MP smooth.
 * Modified: John Huang
 * Date: 2010.02.09
 */
#ifdef UBICOM_MP_SPEED
	    	sprintf(config_buf, "echo %c > /var/tmp/mp_cmd.tmp", *(file_buffer + 41));
	    	system(config_buf);	    
#endif
                if(strstr(file_buffer+37, "Getpin")){//-wps Getpin
                	printf("wps_cmd Getpin\n");
                	system("nvram get wps_default_pin > /var/tmp/wps_default_pin.txt");
                	rc_restart_reqired = 0;
                }
                else if(strstr(file_buffer+37, "Wenable")){//-wps Wenable
                		printf("wps_cmd wirelee enable\n");
                		nvram_set("wlan0_enable", "1");
                		nvram_commit();
                		rc_restart_reqired = 2;
                	}
                	else if(strstr(file_buffer+37, "Wdisable")){//-wps Wdisable
                		printf("wps_cmd wirelee disable\n");
                		nvram_set("wlan0_enable", "0");
                		nvram_commit();
                		rc_restart_reqired = 2;
                	}
                	else if(strstr(file_buffer+37, "Enable")){//-wps Enable
                		printf("wps_cmd wps enable\n");
                		nvram_set("wps_enable", "1");
                		nvram_commit();
                		rc_restart_reqired = 2;
                	}
                	else if(strstr(file_buffer+37, "Disable")){//-wps Disable
                		printf("wps_cmd wps disable\n");
                		nvram_set("wps_enable", "0");
                		nvram_commit();
                		rc_restart_reqired = 2;
                	}
                	else if(strstr(file_buffer+37, "WPSNoNo")){//-wps Disable
                		printf("wps button does not run\n");
				system("echo 1 > /var/tmp/WPSNoNo");
/*
 * Reason: Let MP smooth.
 * Modified: John Huang
 * Date: 2010.02.09
 */
#ifdef UBICOM_MP_SPEED
			  	system("echo 1 > /var/tmp/mp_testing.tmp");
#endif
                		rc_restart_reqired = 0;
                	}
#ifdef UBICOM_MP_SPEED                	
                	else if(strstr(file_buffer+37, "re-rc")){//hidden command rc_restart
                		printf("cmd rc_restart\n");
                		rc_restart_reqired = 2;
                	}
                	else if(strstr(file_buffer+37, "re-sys")){//hidden sys_reboot
                		printf("cmd sys_reboot\n");
                		sys_reboot();
                	}
                	else if(strstr(file_buffer+37, "re-nvram")){//hidden nvram_re_def
                		printf("cmd nvram restore_default\n");
                		system("nvram restore_default");
                	}                    	     
#endif                	            	
			
            }
            else
            	nvram_commit();

/*
 * Reason : MP to test host wirelss for Multi-SSID
 * Modified : Robert Chen
 * Date : 2011/01/20
 */
#ifdef DIR652V
	    if (*(file_buffer+365) == 48) {
		system("nvram set mp_wlan=\"0\"");
		system("nvram set wlan0_security=\"wpa2auto_psk\"");
            	nvram_commit();
	    } else if (*(file_buffer+365) == 49) {
		system("nvram set mp_wlan=\"1\"");
		system("nvram set wlan0_security=\"disable\"");
            	nvram_commit();
	    }
#endif
        }
    }//tftp type end

/*
 * Reason: Let MP smooth.
 * Modified: John Huang
 * Date: 2010.02.09
 */
#ifdef UBICOM_MP_SPEED
	if(nvram_defaultstore_reqired == 1) //0=nothing 1=restore default
	{
		sleep(2);
		system("nvram restore_default");
		sleep(2);
	}
#endif

    if(rc_restart_reqired == 1) {
        printf("*****************************************************system reboot \n");
        sys_reboot();
    }
	else if(rc_restart_reqired == 2) {
	    printf("*****************************************************rc restart \n");
    	system("rc restart");
    }
    else{
    	printf("*****************************************************nothing \n");
    }

    close(peer);
    tftp_free(p_data_pkt);
    tftp_free(p_ack_pkt);
    tftp_free(file_buffer);
    if(fp)
    	fclose(fp);
    return 0;
}

int tftp_receive(struct sockaddr_in *p_to_addr,char *p_name,char *p_mode,int InClient,
                 char (*TFTPwrite)(char *,long ,char,void *),
                 void *argu)
{
    return tftp_receive_ext(p_to_addr, p_name, p_mode, InClient, TFTPwrite,argu, PKTSIZE);
}

int tftp_send_ext(struct sockaddr_in *p_to_addr,char *p_name,char *p_mode,int InClient,
                  char (*TFTPread)(char *,long *,char,void *),
                  void *argu, int vPKTSIZE)
{
    char *p_data_pkt,*p_ack_pkt,*p_data_buf,*p_copy_buf;
    struct tftphdr *p_tftpdata_pkt,*p_tftpack_pkt;
    int result =0, bytes;
    int size,Oldsize=vPKTSIZE;
    int ntimeout,peer, port_no;
    ushort nextBlockNumber;
    struct timeval tv;
    fd_set lecture;
    struct sockaddr_in from,to=*p_to_addr;
    size_t fromlen=sizeof(from),tolen=fromlen;

    p_data_pkt=(char*)malloc(vPKTSIZE);
    if (p_data_pkt==NULL)
    {
        DEBUG_MSG("TFTP: out of memory.\n");
        return 255;
    }
    p_ack_pkt=(char*)malloc(vPKTSIZE);
    if (p_ack_pkt==NULL)
    {
        DEBUG_MSG("TFTP: out of memory.\n");
        tftp_free(p_data_pkt); return 255;
    }
    p_tftpdata_pkt=(struct tftphdr *)p_data_pkt;
    p_tftpack_pkt=(struct tftphdr *)p_ack_pkt;
    p_data_buf=(char*)&p_tftpdata_pkt->th_data[0];
    p_copy_buf=(char*)&p_tftpdata_pkt->th_stuff[0];

    port_no = 0;
    if ((peer = create_socket(SOCK_DGRAM, &port_no))<0)
    {
        DEBUG_MSG("create socket failure %d:\n", errno);
        tftp_free(p_data_pkt); tftp_free(p_ack_pkt);
        return 255;
    }

    if (InClient)
    {
        p_tftpdata_pkt->th_opcode=htons((u_short)WRQ);
        strcpy(p_copy_buf, p_name);
        p_copy_buf += strlen(p_name);
        *p_copy_buf++ = '\0';
        strcpy(p_copy_buf, p_mode);
        p_copy_buf += strlen(p_mode);
        *p_copy_buf++ = '\0';
        size=(unsigned long)p_copy_buf-(unsigned long)p_data_pkt;
        nextBlockNumber=0;
    }
    else
    {
        p_tftpdata_pkt->th_opcode=(u_short)htons((u_short)DATA);
        p_tftpdata_pkt->th_block=(u_short)htons((ushort)1);
        /*The actual size read from file will be returned in "size" */
        if ((*TFTPread)(p_data_buf,(long*)(&size),1,argu)!=0)
        {

            DEBUG_MSG("TFTPread error : %d", errno);
            nak(peer, &to, errno,strerror(errno));
            close(peer); tftp_free(p_data_pkt); tftp_free(p_ack_pkt);
            return 255;
        }
 /*
* 	Date: 2009-10-21
* 	Name: Ken_Chiang
* 	Reason: modify for get wps_default_pin.txt the size is too small so
*			the pin code will has loss last number.
* 	Notice :
*/
        if(strstr(p_name,"wps_default_pin.txt")){
        	//printf("p_name=%s,size=%d\n",p_name,size);
        	size+=6;
        }
        else{
        	size+=4;
        }
        nextBlockNumber=1;
    }
    //printf("p_data_buf=%s,size=%d\n",p_data_buf,size);

    do
    {
        /*next ACK's timeout */
        ntimeout=0;
        do
        {
            if (ntimeout==NumberTimeOut) { close(peer); tftp_free(p_data_pkt); tftp_free(p_ack_pkt); return 255;}

            if (sendto(peer,p_tftpdata_pkt,size,0,(struct sockaddr *)&to,tolen)!=size)
            {
                DEBUG_MSG("sendto failure %d", errno);
                close(peer); tftp_free(p_data_pkt); tftp_free(p_ack_pkt);
                return 255;
            }

        do
        {
            bytes = -1;
            FD_ZERO(&lecture);
            FD_SET(peer,&lecture);
            tv.tv_sec=TimeOut; tv.tv_usec=0;

            if ((result=select(peer+1, &lecture, NULL, NULL, &tv))==-1)
            {
                DEBUG_MSG("tftp: select error : %d", errno);
                close(peer); tftp_free(p_data_pkt); tftp_free(p_ack_pkt);
                return 255;
            }
            if (result >0) bytes = recvfrom(peer, p_tftpack_pkt, vPKTSIZE, 0,(struct sockaddr *)&from, &fromlen);
        } while ((bytes < 0)&&(result > 0)); //recv nothing but select>0, keep recv

         if (result > 0)
         {
            to.sin_port=from.sin_port;
            p_tftpack_pkt->th_opcode = ntohs((u_short)p_tftpack_pkt->th_opcode);
            p_tftpack_pkt->th_block = ntohs((u_short)p_tftpack_pkt->th_block);
            if (p_tftpack_pkt->th_opcode != ACK)
            {
                   DEBUG_MSG("opcode not correct");
                	close(peer); tftp_free(p_data_pkt); tftp_free(p_ack_pkt);
                	return 255;
            }

            if (p_tftpack_pkt->th_block != nextBlockNumber)
            {
               /* Re-synchronize with the other side */
               ioctl(peer, FIONREAD, &bytes);
               while (bytes)
               {
                  recv(peer, p_tftpack_pkt, vPKTSIZE, 0);
                  ioctl(peer, FIONREAD, &bytes);
               };
               p_tftpack_pkt->th_block=nextBlockNumber+1;
            }
         }
         ntimeout++;
      } while (p_tftpack_pkt->th_block!=nextBlockNumber);

        if ((size<vPKTSIZE)&&(nextBlockNumber!=0)) break;

        nextBlockNumber++;
        p_tftpdata_pkt->th_block=htons(nextBlockNumber);

        /*only use in client*/
        if (nextBlockNumber==1)
        {
            p_tftpdata_pkt->th_opcode=htons((u_short)DATA);
            result=(*TFTPread)(p_data_buf,(long*)(&size),1,argu);
        }
        else
        {
            Oldsize=size;
            if (Oldsize==vPKTSIZE) result =(*TFTPread)(p_data_buf,(long*)(&size),0,argu);
            else result=0;
        }

        if (result < 0)
        {
            DEBUG_MSG("TFTPread error : %d", errno);
            nak(peer, &from, errno,strerror(errno));
            close(peer); tftp_free(p_data_pkt); tftp_free(p_ack_pkt);
            return result;
        }
        size+=4;
    } while (Oldsize==vPKTSIZE);
    printf("TFTP send successfully \n");
    close(peer); tftp_free(p_data_pkt); tftp_free(p_ack_pkt);
    return 0;
}

int tftp_send(struct sockaddr_in *p_to_addr,char *p_name,char *p_mode,int InClient,
              char (*TFTPread)(char *,long *,char,void *),
              void *argu)
{
    return tftp_send_ext(p_to_addr, p_name, p_mode, InClient, TFTPread, argu, PKTSIZE);
}