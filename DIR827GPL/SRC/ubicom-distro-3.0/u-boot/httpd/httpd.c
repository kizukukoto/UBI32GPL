/**
 * \addtogroup exampleapps
 * @{
 */

/**
 * \defgroup httpd Web server
 * @{
 *
 * The uIP web server is a very simplistic implementation of an HTTP
 * server. It can serve web pages and files from a read-only ROM
 * filesystem, and provides a very small scripting language.
 *
 * The script language is very simple and works as follows. Each
 * script line starts with a command character, either "i", "t", "c",
 * "#" or ".".  The "i" command tells the script interpreter to
 * "include" a file from the virtual file system and output it to the
 * web browser. The "t" command should be followed by a line of text
 * that is to be output to the browser. The "c" command is used to
 * call one of the C functions from the httpd-cgi.c file. A line that
 * starts with a "#" is ignored (i.e., the "#" denotes a comment), and
 * the "." denotes the last script line.
 *
 * The script that produces the file statistics page looks somewhat
 * like this:
 *
 \code
 i /header.html
 t <h1>File statistics</h1><br><table width="100%">
 t <tr><td><a href="/index.html">/index.html</a></td><td>
 c a /index.html
 t </td></tr> <tr><td><a href="/cgi/files">/cgi/files</a></td><td>
 c a /cgi/files
 t </td></tr> <tr><td><a href="/cgi/tcp">/cgi/tcp</a></td><td>
 c a /cgi/tcp
 t </td></tr> <tr><td><a href="/404.html">/404.html</a></td><td>
 c a /404.html
 t </td></tr></table>
 i /footer.plain
 .
 \endcode
 *
 */


/**
 * \file
 * HTTP server.
 * \author Adam Dunkels <adam@dunkels.com>
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
 * $Id: httpd.c,v 1.4 2009/09/24 08:01:31 norp_huang Exp $
 *
 */

#include "uip.h"
#include "httpd.h"
#include "fs.h"
#include "fsdata.h"
#include "cgi.h"
#include "uip_arp.h"
#include "uipopt.h"
#include "defaultConf.h"
#include "bsp.h"
#include <common.h>
#include "../include/asm-ubicom32/gpio.h"
//extern void gpio_set_value(unsigned gpio, int value);

/*
 * Debug message define
 */
//#define DEBUG_HTTP
#ifdef DEBUG_HTTP
#define DEBUG_MSG        printf
#else
#define DEBUG_MSG(args...)
#endif

#define SWAP_16(X)  X
#define BUF ((struct uip_eth_hdr *)&uip_buf[0])

/* The HTTP server states: */
#define HTTP_NOGET        0
#define HTTP_FILE         1
#define HTTP_TEXT         2
#define HTTP_FUNC         3
#define HTTP_END          4

struct httpd_state *hs;

extern const struct fsdata_file file_index_html;
extern const struct fsdata_file file_404_html;
extern const struct fsdata_file file_error_html;
extern const struct fsdata_file file_counter_html;
extern void do_reset(int argc, char *argv[]);
extern  u8_t upgrade_cgi(u8_t next);

extern u8_t upgrade_firmware(void);

static void next_scriptline(void);
static void next_scriptstate(void);

extern void init_led(int gpio);
extern int flashwrite;
extern int supportSafari;
extern int supportChrome;
int httpd_no_hwid=0;

#define ISO_G        0x47
#define ISO_E        0x45
#define ISO_T        0x54
#define ISO_P        0x50
#define ISO_O        0x4F
#define ISO_S        0x53
#define ISO_T        0x54
#define ISO_slash    0x2f
#define ISO_c        0x63
#define ISO_g        0x67
#define ISO_i        0x69
#define ISO_space    0x20
#define ISO_nl       0x0a
#define ISO_cr       0x0d
#define ISO_a        0x61
#define ISO_t        0x74
#define ISO_hash     0x23
#define ISO_period   0x2e
#define NULL (void *)0
#define ETH_MAX_PKTLEN 1522

static  int MPcount = 0;
int httpd_flag = 0;

/* move to httpd.h */
/*typedef struct timer {
   unsigned int start;
   unsigned int delay;
}timer_t;*/

timer_t fin;

typedef struct gpio_info {
     timer_t gtimer;
     int tgl;
     int setbit;
}led_t;

led_t statLed;

typedef struct{
	unsigned char *dst;
	unsigned char *src;
	unsigned int  type;
}eth_header_t;

/*-----------------------------------------------------------------------------------*/
/**
 * Initialize the web server.
 *
 * Starts to listen for incoming connection requests on TCP port 80.
 */
/*-----------------------------------------------------------------------------------*/
void
httpd_init(void)
{
	fs_init();

	/* Listen to port 80. */
	uip_listen(HTONS(80));
}
/*-----------------------------------------------------------------------------------*/
	void
httpd_appcall(void)
{
	struct fs_file fsfile;
	int ret = 0;
	int send_error_flag = 0;
	int send_count_down = 0;

	//u8_t i;
	int i;
	int offset=0;
	DEBUG_MSG("http_appcall () : Entry \n");


	switch(uip_conn->lport) {
		/* This is the web server: */
		case HTONS(80):
			/* Pick out the application state from the uip_conn structure. */
			hs = (struct httpd_state *)(uip_conn->appstate);

			/* We use the uip_ test functions to deduce why we were
				called. If uip_connected() is non-zero, we were called
				because a remote host has connected to us. If
				uip_newdata() is non-zero, we were called because the
				remote host has sent us new data, and if uip_acked() is
				non-zero, the remote host has acknowledged the data we
				previously sent to it. */
			if(uip_connected()) {
				/* Since we have just been connected with the remote host, we
					reset the state for this connection. The ->count variable
					contains the amount of data that is yet to be sent to the
					remote host, and the ->state is set to HTTP_NOGET to signal
					that we haven't received any HTTP GET request for this
					connection yet. */
				hs->state = HTTP_NOGET;
				hs->count = 0;
				return;

			} else if(uip_poll()) {
				/* If we are polled ten times, we abort the connection. This is
					because we don't want connections lingering indefinately in
					the system. */
				if(hs->count++ >= 10) {
					uip_abort();
				}
				return;
			} else if(uip_newdata() && hs->state == HTTP_NOGET) {
				/* This is the first data we receive, and it should contain a GET. */

				/* Check for GET. */
#if 0
				DEBUG_MSG("uip_appdata \n");
				for(i = 0; i < 16; i++)
					DEBUG_MSG("0x%2x ",uip_appdata[i]);
				DEBUG_MSG("\n");
#endif
				if(uip_appdata[0] != ISO_G || uip_appdata[1] != ISO_E ||uip_appdata[2] != ISO_T || uip_appdata[3] != ISO_space) {
					offset=1;
					/* If it isn't a GET, we abort the connection. */
					if(uip_appdata[0] != ISO_P || uip_appdata[1] != ISO_O ||uip_appdata[2] != ISO_S || uip_appdata[3] != ISO_T ||
							uip_appdata[4] != ISO_space){
						/* If it isn't a POST, we abort the connection. */
						uip_abort();
						return;
					}
				}
				DEBUG_MSG("offset is %d, offset = 1 is GET , = 0 is POST\n",offset);
				/* Find the file we are looking for. */
				for(i = (4+offset); i < (40+offset); ++i) {
					if(uip_appdata[i+offset] == ISO_space || uip_appdata[i+offset] == ISO_cr ||uip_appdata[i+offset] == ISO_nl) {
						uip_appdata[i+offset] = 0;
						break;
					}
				}

				/* Check for a request for "/". */
				if(uip_appdata[4+offset] == ISO_slash && uip_appdata[5+offset] == 0) {
					DEBUG_MSG("Download index.html file \n");
					fs_open(file_index_html.name, &fsfile);
				}
				else if((!memcmp(&uip_appdata[4+offset], "/test_nohwid.html", 17)) && (uip_appdata[21+offset] == 0)) {
					printf("Do not check HW ID\n");
					httpd_no_hwid = 1;
					fs_open(file_index_html.name, &fsfile);
				}
				else {
					if(!fs_open((const char *)&uip_appdata[4+offset], &fsfile)) {
						DEBUG_MSG("couldn't open file\n");
						fs_open(file_404_html.name, &fsfile);
					}
				}

send_error:
				if(send_count_down == 1) {
					send_count_down = 0;
					if(fs_open(file_counter_html.name,&fsfile))
						DEBUG_MSG("http_appcall() : send count down page \n");
					else
					   DEBUG_MSG("http_appcall() : open count down page fail \n");
				}
				if(send_error_flag == 1) {
					send_error_flag = 0;
					if(fs_open(file_error_html.name, &fsfile))
					   DEBUG_MSG("http_appcall () : send error file\n" );
					else
						DEBUG_MSG("http_appcall () : open error file page fail \n");
				}
				if(uip_appdata[4+offset] == ISO_slash && uip_appdata[5+offset] == ISO_c &&
						uip_appdata[6+offset] == ISO_g && uip_appdata[7+offset] == ISO_i &&	uip_appdata[8+offset] == ISO_slash) {
					/* If the request is for a file that starts with "/cgi/", we prepare for invoking a script. */
					hs->script = fsfile.data;
					next_scriptstate();

				} else {
					hs->script = NULL;
					/* The web server is now no longer in the HTTP_NOGET state, but
						in the HTTP_FILE state since is has now got the GET from
						the client and will start transmitting the file. */
					hs->state = HTTP_FILE;

					/* Point the file pointers in the connection state to point to
						the first byte of the file. */
					hs->dataptr = fsfile.data;
					hs->count = fsfile.len;
				}
			}

			DEBUG_MSG("http_appcall () : Check http client has ACK \n" );

			if(hs->state != HTTP_FUNC) {
				DEBUG_MSG("hs->state != HTTP_FUNC \n" );
				/* Check if the client (remote end) has acknowledged any data that
					we've previously sent. If so, we move the file pointer further
					into the file and send back more data. If we are out of data to
					send, we close the connection. */

				if(uip_acked()) {
					if(hs->count >= uip_conn->len) {
						hs->count -= uip_conn->len;
						hs->dataptr += uip_conn->len;
					} else {
						hs->count = 0;
					}

					if(hs->count == 0) {
						if(hs->script != NULL) {
							next_scriptline();
							next_scriptstate();
						} else {
							uip_close();
						}
					}
				}

			} else {
				/* Call the CGI function. */
				DEBUG_MSG("http_appcall () : Call CGI function \n" );
				/*ret = cgitab[hs->script[2] - ISO_a]( uip_acked());*/
				ret = upgrade_cgi( uip_acked());
				if((ret > 0) && (ret < 2)) {
					/* If the function returns non-zero, we jump to the next line
						in the script. */
					send_count_down = 1;
					goto send_error;
				}
				else if(ret == 2)
				{
					send_error_flag = 1;
					goto send_error;
				}
			}

			if(hs->state != HTTP_FUNC && !uip_poll()) {
				// Send a piece of data, but not more than the MSS of the connection.
				DEBUG_MSG("http_appcall () : send data \n" );
				uip_send(hs->dataptr, hs->count);
			}

			/* Finally, return to uIP. Our outgoing packet will soon be on its
				way... */

			DEBUG_MSG("http_appcall () : return \n" );
			return;

		default:
			/* Should never happen. */
			DEBUG_MSG("http_appcall () : uip abort() \n" );
			uip_abort();
			break;
	}
}
/*-----------------------------------------------------------------------------------*/
/* next_scriptline():
 *
 * Reads the script until it finds a newline. */
	static void
next_scriptline(void)
{
	/* Loop until we find a newline character. */
	do {
		++(hs->script);
	} while(hs->script[0] != ISO_nl);

	/* Eat up the newline as well. */
	++(hs->script);
}
/*-----------------------------------------------------------------------------------*/
/* next_sciptstate:
 *
 * Reads one line of script and decides what to do next.
 */

	static void
next_scriptstate(void)
{
	struct fs_file fsfile;
	u8_t i;

again:
	switch(hs->script[0]) {
		case ISO_t:
			/* Send a text string. */
			hs->state = HTTP_TEXT;
			hs->dataptr = &hs->script[2];

			/* Calculate length of string. */
			for(i = 0; hs->dataptr[i] != ISO_nl; ++i);
			hs->count = i;
			break;
		case ISO_c:
			DEBUG_MSG("ISO_c %d \n",hs->script[2] - ISO_a);
			/* Call a function. */
			hs->state = HTTP_FUNC;
			hs->dataptr = NULL;
			hs->count = 0;
			/*cgitab[hs->script[2] - ISO_a](0);*/
			upgrade_cgi(0);
			break;
		case ISO_i:
			/* Include a file. */
			hs->state = HTTP_FILE;
			if(!fs_open(&hs->script[2], &fsfile)) {
				uip_abort();
			}
			hs->dataptr = fsfile.data;
			hs->count = fsfile.len;
			break;
		case ISO_hash:
			/* Comment line. */
			next_scriptline();
			goto again;
			break;
		case ISO_period:
			/* End of script. */
			hs->state = HTTP_END;
			uip_close();
			break;
		default:
			uip_abort();
			break;
	}
}

//#define SECOND 1750000
//#define STATUS_LED_TIMEOUT 2*SECOND


void led_blinking(led_t *led)
{
	//printf("get_ticks=%d, led->gtimer.start=%d, led->gtimer.delay=%d\n",get_ticks(),led->gtimer.start,led->setbit);
    	if (get_ticks() - led->gtimer.start >= led->gtimer.delay)
	{
    		led->gtimer.start = get_ticks();
	    	if (led->tgl)
	    	{
			gpio_set_value(led->setbit,1);
	    	}
	    	else
	    	{
			gpio_set_value(led->setbit,0);
	    	}
    	    	led->tgl ^=1;
    	}


/*	static int lightoff = 0;
        static int blinking_timer = 0;

        blinking_timer++;

        if(blinking_timer == STATUS_LED_TIMEOUT)//about 2 Secs
        {
                if (!lightoff)
                {
			printf("lightoff=0\n");
			lightoff=1;
			gpio_set_value(led->setbit,0);

		}
		else
		{
			printf("lightoff=1\n");
			lightoff=0;
			gpio_set_value(led->setbit,1);

		}
		blinking_timer=0;
	}*/
}

void led_dark(led_t *led)
{
	gpio_set_value(led->setbit,1);
}


#ifdef ETH_TEST
extern void uip_arp_send(void);
#endif

void
httpd(int argc, char *argv[])
{
	u8_t arptimer;
	int i;
	unsigned short uip_addr;
	//static char line[CYGPKG_REDBOOT_MAX_CMD_LINE];
	unsigned long bi_ip_addr;
	unsigned char localMac[6];
	int ret = 0;

	statLed.tgl = 0;
#ifdef DLINK_ROUTER_LED_DEFINE
    statLed.setbit = STATUS_LED;
#else
	statLed.setbit = 14; //please sepcify for your board.
#endif
	statLed.gtimer.start = get_ticks();
	statLed.gtimer.delay = 2 * get_tbclk(); //every 2 seconds.
/* check it please....*/
//	init_led(statLed.setbit); //init led.


	httpd_flag = 1;

	eth_init ();

	printf("httpd start\n");

	/* Take Network Environment Setting*/
	//bi_ip_addr= string_to_ip(getenv("ipaddr"));
	bi_ip_addr = DEFAULT_IP_ADDRESS;

//printf("bi_ip_addr = %x\n", bi_ip_addr);

	/*
	 * Reason: Get lan mac from artblock, it can auto detect the artblock address.
	 * Modified: Yufa Huang
	 * Date: 2010.11.12
	 */
	// Get Lan MAC addr
	struct hw_lan_wan_t {
       	char hwversion[4];
       	char lanmac[20];
       	char wanmac[20];
       	char domain[4];
	};

#if defined(IP5000) || defined(IP7000)
	#include "./../../ultra/projects/bootexec/config/config.h"
#elif defined(IP8000)
	#include "./../../ultra_public/projects/bootexec/config/config.h"
#else
#error "Unknown Ubicom32 processor"
#endif
    
	#define FW_CAL_BUF_SIZE  0x10000
	#define HW_LAN_WAN_OFFSET  (FW_CAL_BUF_SIZE - 100)

#if defined(IP5000) || defined(IP7000)
	#define FLASH_BASE_ADDR  0x60000000
#elif defined(IP8000)
	#define FLASH_BASE_ADDR  0xb0000000
#else
#error "Unknown Ubicom32 processor"
#endif

	unsigned char lan_mac[24];
	struct hw_lan_wan_t *header_start;

	unsigned long artblock_addr = FLASH_BASE_ADDR + (EXTFLASH_MIN_TOTAL_SIZE_KB - EXTFLASH_MAX_PAGE_SIZE_KB) * 1024;

	header_start = (struct hw_lan_wan_t*)(artblock_addr + HW_LAN_WAN_OFFSET);
	memcpy(lan_mac, header_start->lanmac, sizeof(header_start->lanmac));
	lan_mac[sizeof(header_start->lanmac)] = '\0';

	if (strlen(lan_mac) && (lan_mac[0] != 0xff) && (lan_mac[1] != 0xff)) {
		printf("get lan_mac = %s\n", lan_mac);
		int len = strlen(lan_mac);
		
		for (i=0 ;i < len; i++) {
			if ((lan_mac[i] >= 'A') && (lan_mac[i] <= 'F')) {
				lan_mac[i] = lan_mac[i] - 'A' + 10;
				continue;
			}
		
			if ((lan_mac[i] >= 'a') && (lan_mac[i] <= 'f')) {
				lan_mac[i] = lan_mac[i] - 'a' + 10;
				continue;
			}
			
			if ((lan_mac[i] >= '0') && (lan_mac[i] <= '9')) {
				lan_mac[i] -= '0';
			}	
		}

		localMac[0] = lan_mac[0];
		localMac[0] <<= 4;
		localMac[0] |= lan_mac[1];

		localMac[1] = lan_mac[3];
		localMac[1] <<= 4;
		localMac[1] |= lan_mac[4];

		localMac[2] = lan_mac[6];
		localMac[2] <<= 4;
		localMac[2] |= lan_mac[7];

		localMac[3] = lan_mac[9];
		localMac[3] <<= 4;
		localMac[3] |= lan_mac[10];

		localMac[4] = lan_mac[12];
		localMac[4] <<= 4;
		localMac[4] |= lan_mac[13];

		localMac[5] = lan_mac[15];
		localMac[5] <<= 4;
		localMac[5] |= lan_mac[16];
	}
	else {
		eth_getenv_enetaddr("ethaddr", localMac);
	}

	/* uip init*/
	DEBUG_MSG("uip init\n");
	uip_init();

	/* httpd init */
	DEBUG_MSG("http init\n");
	httpd_init();

	uip_addr = htons( (bi_ip_addr >> 16) & 0xffff);
	uip_addr = SWAP_16(uip_addr);
	uip_hostaddr[0]=  uip_addr;
	uip_addr = htons( (bi_ip_addr & 0x0000ffff));
	uip_addr = SWAP_16(uip_addr);
	uip_hostaddr[1]=  uip_addr;

	DEBUG_MSG("hostmac = ");
	for(i=0;i<6;i++)
	{
		uip_ethaddr.addr[i]=localMac[i];
		DEBUG_MSG("%c%02x", i ? ':' : ' ', localMac[i]);
	}

#if BYTE_ORDER == BIG_ENDIAN
    printf("\nhostaddr = 0x%04x%04x\n",uip_hostaddr[0], uip_hostaddr[1]);
#else //LITTLE_ENDIAN
    printf("\nhostaddr = 0x%04x%04x\n",uip_hostaddr[1], uip_hostaddr[0]);
#endif

	uip_arp_netmask[0] = 0xffff;
	uip_arp_netmask[1] = htons(0xff00);

	arptimer = 0;
	uip_len=0;

	while(1)
	{
		i++;
#ifdef ETH_TEST
		i++;
		if(!(i%10000000))
		{
			DEBUG_MSG("\nTest Send\n");
			uip_arp_send();
			NetSendPacket(uip_buf,uip_len);
        	}
#endif
		//if(!flashwrite) //alway blinking
			led_blinking(&statLed);

		uip_len=0;

		ret = eth_rx();
		// Abort if ctrl-c was pressed.
		if (ctrlc())
		{
			eth_halt();
			DEBUG_MSG("\nAbort\n");
			httpd_flag = 0;
			break;
		}

		if(finishflag)
		{
             		/* Reset system */
	        	printf("System Reset \n");
	        	do_reset(0, NULL);
			break;
		}

		if(flashwrite)
		{
			printf("flashwrite\n");
			if ((!supportSafari && !supportChrome) || (fin.delay < get_ticks() - fin.start))
			{
				led_dark(&statLed);
				upgrade_firmware();
				finishflag = 1;
			}
		}

		if(hwiderror)
		{
			statLed.gtimer.delay = 3 * get_tbclk(); //every 3 seconds
		}

	   	if(uip_len == 0)
		{
			// Call the ARP timer function every 10 seconds.
			if(++arptimer == 20)
			{
				uip_arp_timer();
				arptimer = 0;
			}
		}
		else
		{

			DEBUG_MSG("BUF->type=0x%x, len=%d\n", BUF->type, uip_len);
			if(BUF->type == htons(UIP_ETHTYPE_IP))
			{
				DEBUG_MSG("UIP_ETHTYPE_IP \n");
				uip_arp_ipin();
				DEBUG_MSG("BUF->type=0x%x, len=%d\n", BUF->type, uip_len);
				uip_input();
				DEBUG_MSG("BUF->type=0x%x, len=%d\n", BUF->type, uip_len);
				/* If the above function invocation resulted in data that
				* should be sent out on the network, the global variable
				* uip_len is set to a value > 0. */
				if(uip_len > 0)
				{
					uip_arp_out();
					DEBUG_MSG("uip_len=%d\n",uip_len);
					NetSendPacket(uip_buf,uip_len);
				}
				DEBUG_MSG("UIP_ETHTYPE_IP OUT\n");
			}
			else if(BUF->type == htons(UIP_ETHTYPE_ARP))
			{
				DEBUG_MSG("UIP_ETHTYPE_ARP \n");
				uip_arp_arpin();
				/* If the above function invocation resulted in data that
				* should be sent out on the network, the global variable
				* uip_len is set to a value > 0. */
				if(uip_len > 0)
				{
					NetSendPacket(uip_buf,uip_len);
				}
				DEBUG_MSG("UIP_ETHTYPE_ARP OUT \n");
			}
			else
				DEBUG_MSG("UNKNOWN PACKET?? \n");
		}
	}
}
