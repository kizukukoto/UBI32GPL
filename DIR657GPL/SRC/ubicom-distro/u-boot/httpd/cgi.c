/**
 * \addtogroup httpd
 * @{
 */

/**
 * \file
 * HTTP server script language C functions file.
 * \author Adam Dunkels <adam@dunkels.com>
 *
 * This file contains functions that are called by the web server
 * scripts. The functions takes one argument, and the return value is
 * interpreted as follows. A zero means that the function did not
 * complete and should be invoked for the next packet as well. A
 * non-zero value indicates that the function has completed and that
 * the web server should move along to the next script line.
 *
 */

/*
 * Copyright (c) 2001, Adam Dunkels.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote
 *    products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * This file is part of the uIP TCP/IP stack.
 *
 * $Id: cgi.c,v 1.4 2009/09/24 08:01:31 norp_huang Exp $
 *
 */
#include <common.h>
#include <command.h>
#include <linux/ctype.h>
#include <config.h>
#include <flash.h>
#include "uip.h"
#include "cgi.h"
#include "httpd.h"
#include "fs.h"
#include "bsp.h"

/*
 * Debug message define
 */
//#define DEBUG_CGI
#ifdef DEBUG_CGI
#define DEBUG_MSG        printf
#else
#define DEBUG_MSG(args...)
#endif

#define LOADER_HWID_LEN		22
#define KERNEL_HWID_LEN		24
#define HWID_LEN			24

int content_length = 0;
int startSector = 0;
int endSector = 0;

// Predefine function
char *get_request_line(int *request_len, int *request_idx);
/*static*/ u8_t upgrade_cgi(u8_t next);

cgifunction cgitab[] = {
	upgrade_cgi      /* CGI function "c" */
};

static const char closed[] =   /*  "CLOSED",*/
{0x43, 0x4c, 0x4f, 0x53, 0x45, 0x44, 0};
static const char syn_rcvd[] = /*  "SYN-RCVD",*/
{0x53, 0x59, 0x4e, 0x2d, 0x52, 0x43, 0x56,
	0x44,  0};
static const char syn_sent[] = /*  "SYN-SENT",*/
{0x53, 0x59, 0x4e, 0x2d, 0x53, 0x45, 0x4e,
	0x54,  0};
static const char established[] = /*  "ESTABLISHED",*/
{0x45, 0x53, 0x54, 0x41, 0x42, 0x4c, 0x49, 0x53, 0x48,
	0x45, 0x44, 0};
static const char fin_wait_1[] = /*  "FIN-WAIT-1",*/
{0x46, 0x49, 0x4e, 0x2d, 0x57, 0x41, 0x49,
	0x54, 0x2d, 0x31, 0};
static const char fin_wait_2[] = /*  "FIN-WAIT-2",*/
{0x46, 0x49, 0x4e, 0x2d, 0x57, 0x41, 0x49,
	0x54, 0x2d, 0x32, 0};
static const char closing[] = /*  "CLOSING",*/
{0x43, 0x4c, 0x4f, 0x53, 0x49,
	0x4e, 0x47, 0};
static const char time_wait[] = /*  "TIME-WAIT,"*/
{0x54, 0x49, 0x4d, 0x45, 0x2d, 0x57, 0x41,
	0x49, 0x54, 0};
static const char last_ack[] = /*  "LAST-ACK"*/
{0x4c, 0x41, 0x53, 0x54, 0x2d, 0x41, 0x43,
	0x4b, 0};


extern char null_board_mac[];

#define CFG_FLASH_BASE 0x60000000
#define CFG_MAX_FLASH_BANKS 1

#define BUFF_LOCATION 	 0x40100000
#define LOADER_SIZE		(256 << 10)	//256k uboot
#define LOADER_LOCATION  CFG_FLASH_BASE
#define KERNEL_LOCATION  LOADER_LOCATION + LOADER_SIZE
#define FLASH_BASE       KERNEL_LOCATION

#define FLASH_SECTOR_SIZE (256 << 10) //256k per sector
#define LOADER_START_SECTOR	0
#define LOADER_END_SECTOR LOADER_START_SECTOR + (LOADER_SIZE/FLASH_SECTOR_SIZE) -1
#define KERNEL_SIZE		(13312 << 10)	//7552K kernel
#define IMAGE_START_SECTOR LOADER_END_SECTOR+1
#define IMAGE_END_SECTOR IMAGE_START_SECTOR + ((KERNEL_SIZE)/FLASH_SECTOR_SIZE) -1

//#define HWID_LOCATION    CFG_FLASH_BASE + 0x400
#define BUFFER_SIZE      16*1024*1024

volatile unsigned long total_size=0;
unsigned long total_filesize=0;
unsigned long total_packets=0;
unsigned long flash_base = KERNEL_LOCATION;
unsigned long supportFireFox = 0;
unsigned long supportSafari = 0;
unsigned long supportChrome = 0;
unsigned long supportOpera = 0;

int buffer_ptr=0;


u8_t *ptr;
extern timer_t fin;
int bufsize = 0;

/* **********************************************************************
* Fucntion    : flash_in_which_sec
* Description : Search offset in which sector
* Input		  :
*				flash_information table
*				offset address
*************************************************************************/
#if 0
static unsigned int flash_in_which_sec(flash_info_t *fl,unsigned int offset)
{
	unsigned int sec_num;
	if(NULL == fl)
		return 0xFFFFFFFF;

	for( sec_num = 0; sec_num < fl->sector_count ; sec_num++){
		/* If last sector*/
		if (sec_num == fl->sector_count -1)
		{
			if((offset >= fl->start[sec_num] - fl->start[0]) &&
			   (offset < fl->size) )
			{
				return sec_num;
			}

		}
		else
		{
			if((offset >= fl->start[sec_num] - fl->start[0]) &&
			   (offset < fl->start[sec_num + 1] - fl->start[0]) )
			{
				return sec_num;
			}

		}
	}
	/* return illegal sector Number */
	return 0xFFFFFFFF;

}
#endif



/*-----------------------------------------------------------------------------------*/
/*  *********************************************************************
 *  cgi_get_flashbuf(bufptr, bufsize)
 *
 *  Figure out the location and size of the staging buffer.
 *
 *  Input parameters:
 *	   bufptr - address to return buffer location
 *	   bufsize - address to return buffer size
 ********************************************************************* */


static void cgi_get_flash_buf(u8_t **bufptr, int *bufsize)
{
	int size = BUFFER_SIZE;

	if (size > 0) {
		*bufptr = (u8_t *) BUFF_LOCATION;
		*bufsize = size;
	}
}


char *get_request_line(int *request_len, int *request_idx)
{
	int i;
	char c;

	for ( i = *request_idx; *request_idx < *request_len; ++(*request_idx) )
	{
		c = uip_appdata[*request_idx];
		if ( c == '\n' || c == '\r' )
		{
			uip_appdata[*request_idx] = '\0';
			++(*request_idx);
			if ( c == '\r' && *request_idx < *request_len && uip_appdata[*request_idx] == '\n' )
			{
				uip_appdata[*request_idx] = '\0';
				++(*request_idx);
			}
			return (char *)&(uip_appdata[i]);
		}
	}
	return (char*) 0;
}

u8_t upgrade_firmware(void) {

	unsigned int mem_addr;
	void *err_addr;
	unsigned int flash_addr;
	int stat;

	flash_addr = (unsigned int)flash_base;
	mem_addr = (unsigned int)ptr;

//	total_filesize = total_filesize - HWID_LEN;

	/*	Start to copy firmware from buffer to FLASH	*/
	printf("Upgrade Firmware.........\n");
	printf("entry point = %x, flash base = %x total_filesize = %x\n ",mem_addr, flash_addr, total_filesize );

	/* The Hardware ID only Check is illegal */ // HWID???
	{
		extern flash_info_t flash_info[];	/* info for FLASH chips */
		flash_info_t *info = &flash_info[0]; //depend on platform, commonly only have one flash.
		int i;

		if (CFG_MAX_FLASH_BANKS != 1) //more than one flash...
		{
			for(i=0;i<CFG_MAX_FLASH_BANKS;i++)
			{
				if (flash_info[i].flash_id && flash_info[i].flash_id != 0xffff)
				{
					DEBUG_MSG("current working flash id is 0x%x\n", flash_info[i].flash_id);
					info = &flash_info[i];
					break;
				}
			}
		}

    	if (stat = flash_erase(info, startSector, endSector) != 0) {
        	//diag_printf("Can't erase region at %p: %s\n", err_addr, flash_errmsg(stat));
    	}

    	if (stat = write_buff(info, (void *)mem_addr, flash_addr, total_filesize) != 0) {
        	//diag_printf("Can't program region at %p: %s\n", err_addr, flash_errmsg(stat));
    	}
	}
}



int flashwrite = 0;
int finishflag = 0;
int hwiderror = 0;
extern int httpd_no_hwid;
/*static*/ u8_t upgrade_cgi(u8_t next)
{
	//printf("upgrade_cgi\n");
	char *line;
	char *cp;

	char imageHWID[HWID_LEN];
	u8_t *phwid;

	int request_len,request_idx;
	static int shift=0; /* Chun add for mime packet*/
	int len;
	int i;
	int tmp_idx = 0;
	cgi_get_flash_buf(&ptr, &bufsize);

	if(next)
		return 1;
	if (total_packets == 0)
	{
  		request_len = uip_datalen();
  		request_idx = 0;

  		while ( ( line = get_request_line(&request_len, &request_idx) ) != (char*) 0 ) {
  			DEBUG_MSG("%s \n",line);
  			if ( line[0] == '\0' )
			{
 				break;
 			}
 			/* STOP if we reach "Referer" in Firefox packet */
 			if( supportFireFox && ( strstr(line,"Referer") )  )
			{
 				break;

			/*	Get "POST /cgi/index"	and reset all 	*/
			}
			else if(strncmp(line, "POST /cgi/index", 15) ==0)
			{
				total_size=0;
				total_filesize=0;
				total_packets = 1;
				DEBUG_MSG("Get POST cgi index\n");
			}
			else if (strncmp(line, "User-Agent: ", 12) ==0)
			{
				cp = &line[12];
				if (strncmp(cp, "Mozilla/5.0", 11) ==0 )
				{
					if (strstr(cp, "Firefox")){
						supportFireFox = 1;
						DEBUG_MSG("supportFireFox\n");
					} else if (strstr(cp, "Chrome")) {
						supportChrome = 1;
						DEBUG_MSG("supportChrome\n");
					} else if (strstr(cp, "Safari")) {
						supportSafari = 1;
						DEBUG_MSG("supportSafari\n");
					}
					/*Chun modify: Opera version may be different*/
					//}else if (strncmp(cp, "Opera/9.21", 10) ==0 ){
				}
				else if (strncmp(cp, "Opera", 5) ==0 )
				{
				   supportOpera = 1;
				   DEBUG_MSG("supportOpera\n");
				}
				else
				   DEBUG_MSG("default browser using IE \n");
			}
			else if ( strncmp( line, "Content-Length:", 15 ) == 0 ) 			{
	    			cp = &line[16];
	    			total_filesize= ( unsigned long )simple_strtol(cp, NULL, 0);
				DEBUG_MSG("total_filesize is %02d\n",total_filesize);
			}
  		}
  		total_packets++;
	}
	if (total_packets == 2)
	{

		/* Chun modify:
		 * When IE or Opera uses MIME packet,
		 * which means it will add boundary info in POST /cgi/index packet instead of a standalone packet,
		 * we need to record the shift index.
		 */
		shift = request_idx;
		if ((!supportFireFox) &&((line = get_request_line(&request_len, &request_idx) ) != (char*) 0) &&(strncmp( line, "----------", 10) == 0 ))//opera and ie
		{
			DEBUG_MSG("shift: %s \n",line);
    			content_length = strlen(line);
    			DEBUG_MSG("content_length is %02d\n",content_length);
		}
		else
		{
			request_len = uip_datalen();
			request_idx = 0;
			shift = 0;
		}
		DEBUG_MSG("request_len=%d\n",request_len);
		DEBUG_MSG("request_idx=%d\n",request_idx);
		DEBUG_MSG("shift=%d\n",shift);

		while ((line = get_request_line(&request_len, &request_idx) ) != (char*) 0 )
		{
			DEBUG_MSG("while: request_len=%d\n",request_len);
			DEBUG_MSG("while: request_idx=%d\n",request_idx);
			DEBUG_MSG("while: content_length=%d\n",content_length);
			DEBUG_MSG("%s \n",line);

			if (supportSafari && !content_length)
			{ /*Safari 2nd packet is MIME*/
				total_filesize -= (request_len + (request_idx + 4));//0x2d 0x2d 0x0d 0x0a
				total_size = 0;
				total_packets = 3;
				break;
			}

  			if ( line[0] == '\0' )
    				break;
    			if(!supportFireFox)
			{
    				if (strncmp( line, "-----------------------------", 29) == 0 )
				{
    					DEBUG_MSG("support IE %s \n",line);
    					content_length = strlen(line);
    					DEBUG_MSG("content_length is %02d\n",content_length);
    				}
				else if (strncmp( line, "------------", 12 ) == 0 )
				{
    					content_length = strlen(line);
    					DEBUG_MSG("content_length is %02d\n",content_length);
				}
				else if (strncmp(line, "------", 6) == 0)
				{
	   				DEBUG_MSG("support Chorme %s \n",line);
    					content_length = strlen(line);
    					DEBUG_MSG("content_length is %02d\n",content_length);
				}
				if ( strncmp( line, "Content-Type:", 13 ) == 0 ) 				{
					cp = (char *)&uip_appdata[request_idx+2];
		   			if(uip_datalen() != 1446)
					{
		   				DEBUG_MSG("!1446: content_length=%d\n",content_length);
	    					total_filesize = total_filesize - (request_idx+2) - content_length - 6 + shift;
	    					DEBUG_MSG("!1446: total_filesize is %02d\n",total_filesize);
					}
					else
					{
	    					total_filesize = total_filesize - (request_idx+2);
	    					DEBUG_MSG("=1446: total_filesize is %02d\n",total_filesize);
				 	}
					len = uip_datalen() - (request_idx+2);
					total_size = len;
					DEBUG_MSG("total_size=%d\n",total_size);
					memcpy(ptr,cp,len);
					buffer_ptr=len;
	                		total_packets++;
		     			break;
				}
			}
			else
			{
				DEBUG_MSG("supportFireFox %s \n",line);
				/* Chun modify:
				 * When Firefox uses MIME packet,
		 		 * which means it will add boundary info in POST /cgi/index packet instead of a standalone packet,
		 		 * we need to record index has been shited.
				 */
				if ( strncmp( line, "POST /cgi/index", 15 ) == 0 )
				{
					shift = 1;
					DEBUG_MSG("supportFireFox shift=%d \n",shift);
				}
				else if ( strncmp( line, "Content-Length:", 15 ) == 0 )
				{
		    			cp = &line[16];
		    			total_filesize= ( unsigned long )simple_strtol(cp, NULL, 0);
					DEBUG_MSG("Content-Length: total_filesize is %02d\n",total_filesize);
					/*get boundry len*/
					request_idx +=2;
					tmp_idx = request_idx;
					line = get_request_line(&request_len, &request_idx);
					DEBUG_MSG("tmp_idx =%d\n",tmp_idx);
					DEBUG_MSG("pkt 2 line = %s \n", line);
					content_length = strlen(line);
					line = get_request_line(&request_len, &request_idx);
					DEBUG_MSG("pkt 2 line = %s \n", line);
					line = get_request_line(&request_len, &request_idx);
					cp = (char *)&uip_appdata[request_idx+2];
		    			if(uip_datalen() != 1446)
					{
		    				total_filesize = total_filesize - (request_idx+2) - content_length - 6 + tmp_idx;
		    				DEBUG_MSG("total_filesize = %d request_idx = %d content_length = %d\n", total_filesize, request_idx+2, content_length);
					}
					else
					{
		    				total_filesize = total_filesize - (request_idx+2);
					}
					len = uip_datalen() - (request_idx+2);
					total_size = len;
					DEBUG_MSG("total_size = %d \n", total_size);
					memcpy(ptr,cp,len);
					buffer_ptr=len;
               				total_packets++;
	     				break;
				}
			}
		}
	}
	else if(((total_size != 0) || supportSafari) && (total_packets == 3))		{
		request_len = uip_datalen();
		len = request_len;
		total_size += len;
		printf(".");
		cp = (char *)&uip_appdata[0];

		//printf("len=%d total_size=%d \n",len,total_size);
   		if(total_size < total_filesize)
		{
    			/* Chun modify
    		 	* If the index has been shifted, the first packet(POST /cgi/index) need be ignored
    		 	* or the last packet will be missed.
    		 	*/
    			if(shift)
    			{
    				printf("ENTER shift\n");
    				total_size -= len;
    				shift = 0;
    			}
    			else
    			{
				memcpy( (ptr+buffer_ptr), cp, len );
				buffer_ptr += len;
			}
		}
		else
		{
			printf("\n");
			memcpy( (ptr+buffer_ptr), cp, len);
			buffer_ptr += len;

			/* Check Loader Hardware ID information */
			if(total_filesize <= (128*1024))
			{ //use image size to seprate loader or normal FW??
			//phwid = (void *) (ptr + HWID_ADDR_OFFSET);
				phwid = (void *) (ptr + total_filesize - 24);
				memcpy(imageHWID,phwid,HWID_LEN);
				imageHWID[HWID_LEN] = '\0';
				printf("total_filesize is %02d,0x%02x\n",total_filesize,total_filesize);
	 	    		printf("Image Hardware ID is %s\n",imageHWID);
	 	    		//printf(" HWID_LOCATION = 0x%x \n", HWID_LOCATION);
				if(httpd_no_hwid == 0)
				{
				    if(strncmp(imageHWID, HWID, LOADER_HWID_LEN))
				    {
					printf("\n Loader Hardware ID Check Error\n");
					total_size=0;
					total_filesize=0;
					total_packets=0;
					hwiderror = 1;
					return 2;
				    }
				}
				startSector = LOADER_START_SECTOR;
				endSector = LOADER_END_SECTOR; //norp, exclude nvram, need to check restore default!!
				flash_base = LOADER_LOCATION;

			}
			else
			{
				 /*  Check Hardware ID */
				phwid = (void *) (ptr + total_filesize - strlen(HWID));
				memcpy(imageHWID,phwid,HWID_LEN);
				imageHWID[HWID_LEN] = '\0';
				printf("total_filesize is %02d,0x%02x\n",total_filesize,total_filesize);
		 	   	printf("Image Hardware ID is %s\n",imageHWID);
		 	   	//printf(" HWID_LOCATION = 0x%x \n", HWID_LOCATION);
				if(httpd_no_hwid == 0)
				{
				    if(strncmp(imageHWID, HWID, KERNEL_HWID_LEN)) {
					printf("\n Kernel Hardware ID Check Error\n");
					total_size=0;
					total_filesize=0;
					total_packets=0;
					hwiderror = 1;
					return 2;
				    }
				}
				startSector = IMAGE_START_SECTOR;
				endSector = IMAGE_END_SECTOR;
				flash_base = KERNEL_LOCATION;
			}
			if (supportSafari || supportChrome)
			{
				fin.start = get_ticks();
				fin.delay = 1 * get_tbclk();//1 second to reply FIN.
			}
			else
			{ //others: just initial to zero.
				fin.start = 0;
				fin.delay = 0;
			}
			flashwrite = 1;
			return 1;
		}

	}
	return 0;
}





/*-----------------------------------------------------------------------------------*/



