/*=======================================================================
  
       Copyright(c) 2009, Works Systems, Inc. All rights reserved.
  
       This software is supplied under the terms of a license agreement 
       with Works Systems, Inc, and may not be copied nor disclosed except 
       in accordance with the terms of that agreement.
  
  =======================================================================*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <time.h>

#include "war_thread.h"
#include "xml.h"
#include "tr.h"
#include "log.h"
#include "tr_strings.h"
#include "tr_lib.h"
#include "war_string.h"
#include "war_time.h"
#include "war_errorcode.h"
#include "event.h"
#include "sendtocli.h"

#include <bsp.h> 

#ifdef TR196
#include "cm.h"
#endif




#include "my_httpd.h"
extern int getStrtok(char *input, char *token, char *fmt, ...);

char *read_ipaddr(char *if_name);
char* get_internetonline_check(char *buf);
T_WLAN_STAION_INFO* get_stalist();

#define dprintf printf

int nvram_need_commit=0;


// 2010.07.07 
#define _NEW_ADD 1

//struct api_table {
//	char cmd[16];
//	char type[16];
//	char field[64];
//	char path[256];
//
//};

// libversion
extern const char VER_FIRMWARE[];
extern const char VER_DATE[];
extern const char HW_VERSION[];
extern const char VER_BUILD[];
extern const char SVN_TAG[];



#ifdef __V4_2
	#ifndef _GNU_SOURCE
		#define _GNU_SOURCE
	#endif
	#ifdef USE_DYNAMIC
		#ifndef __USE_GNU
			#define __USE_GNU
		#endif
		#include <dlfcn.h>
	#else
		#ifndef __DYNAMIC_H
			#include "dynamic.h"
		#endif
	#endif
#endif


static struct node *root = NULL; //MOT root node
static int count = 0; // the session reference counter
static char xml_file_path[256];
static int change = 0; //Flag indicate if or not the MOT has been changed
static int factory_resetted = 0; //Flag indicate if or not factory is required,
//in real world device, it MUST be a permanent
//flag that can cross reboot.

static void xml_tag2node(struct xml *tag, struct node *n);
static struct node *last_child(struct node *parent);
static struct node * xml2tree(const char *file_path);
static unsigned int nocc_str2code(const char *str);
//static unsigned int type_str2code(const char *str);
//static const char *type_code2str(unsigned int code);
static int __tree2xml(struct node *tree, FILE *fp, int *level);
static int tree2xml(struct node *tree, const char *file_path);
static void lib_destroy_tree(node_t node);

// 2010.07.07
#define PATH_LEN 256

#ifdef __V4_2

#define MAX_LOCATE_DEPTH 4
//#define PATH_LEN 256

void *handle; /**<Dynamic library handle*/

/*Define data_type*/
#define VALUE_TYPE_ANY                  0x00
#define VALUE_TYPE_STRING               0x01
#define VALUE_TYPE_INT                  0x02
#define VALUE_TYPE_UNSIGNED_INT         0x03
#define VALUE_TYPE_BOOLEAN              0x04
#define VALUE_TYPE_DATE_TIME            0x05
#define VALUE_TYPE_BASE_64              0x06

/*!
 * \brief To generate a \b locate from a path
 *
 * To generate a \b locate from \a path, the result will be saved in the \a locate
 *
 * \param path The path which the calculation basing on
 * \param locate The pointer pointing to the memory that the result will be saved
 * \depth depth There are how many locate cells in the memory pointed by \a locate
 *
 * \return 0
 */
int path_2_locate(const char *path, int *locate, int depth)
{
        int i;
        char *d;
        char tmp_name[PATH_LEN];

        for(i = 0; i < depth; i++)
                locate[i] = 0;

        i = 0;

        d = strchr(path, '.');
        while(d && i < depth) {
                if(*path <= '9' && *path > '0') {
                        if(d - path >= PATH_LEN) {
                                tr_log(LOG_NOTICE, "Too long object/paraemter node name");
                                break;
                        }

                        memcpy(tmp_name, path, d - path);
                        tmp_name[d - path] = '\0';

                        locate[i] = atoi(tmp_name);
                        i++;
                }

                path = d + 1;
                d = strchr(path, '.');
        }

        return 0;
}

#endif

// 2010.07.07 add this function 
/*!
 * \brief Generate the full name
 *
 * Generate the full name
 *
 * \param p-- The parameter which will be search
 * \param full_name--Recive the full name
 *
 * \return AGENT_SUCCESS
 */
int get_param_full_name(struct node *p, char full_name[])
{
    struct node *o;
    char tmp_name[PATH_LEN];

    sprintf(full_name,"%s",p->name);
    o = p->parent;
    while (o) {
        sprintf(tmp_name,"%s.", o->name);
        strcat(tmp_name, full_name);
        sprintf(full_name, "%s", tmp_name);
        o = o->parent;
    }
    return 0;
}




// ipv4 convert, reference ubicom 
typedef unsigned short int bool_t;
typedef signed char s8_t;
typedef unsigned char u8_t;
typedef signed short int s16_t;
typedef unsigned short int u16_t;
typedef signed long int s32_t;
typedef unsigned long int u32_t;
typedef signed long long int s64_t;
typedef unsigned long long int u64_t;





typedef unsigned short int fast_bool_t;
typedef signed int fast_s8_t;
typedef unsigned int fast_u8_t;
typedef signed int fast_s16_t;
typedef unsigned int fast_u16_t;
typedef signed int fast_s32_t;
typedef unsigned int fast_u32_t;
typedef signed long long int fast_s64_t;
typedef unsigned long long int fast_u64_t;

#define FALSE 0
#define TRUE 1

static inline u32_t be_u32_to_arch_u32(u32_t v)
{
	return v;
}

static inline u32_t arch_u32_to_be_u32(u32_t v)
{
	return v;
}


typedef u32_t ipv4_addr_t;		

typedef union {
	u8_t bytes[4];
	ipv4_addr_t lgword;
} lgword_t;

/*
 * ipv4_addr_from_str
 *	convert a dot string representation to an ipv4_addr_t rep
 */
bool_t ipv4_addr_from_str(ipv4_addr_t *ip, const char *s)
{
	lgword_t retval;
	retval.lgword = 0;
	fast_u8_t dots = 0;
	fast_u8_t i = 0;
	fast_u16_t accum = 0;
	u8_t c = s[i];
	while (c != 0) {
		if (c == '.') {
			retval.bytes[dots++] = (u8_t)accum;
			if (dots > 3) {
				/*
				 * stop if we have too many octets
				 */
				return FALSE;
			}
			accum = 0;
			c = s[++i];
			continue;
		}
		if (!isdigit(c)) {
			return FALSE;
		}
		accum = accum * 10 + (c - '0');
		/*
		 * we want to stop if, for example, we get something like
		 * 1.2.3.455.  We need to do this here because we don't want
		 * the u16 to roll over.
		 */
		if (accum > 255) {
			return FALSE;
		}
		c = s[++i];
	}
	if (dots != 3) {
		return FALSE;
	}
	retval.bytes[dots] = accum;
	*ip = be_u32_to_arch_u32 (retval.lgword);
	return TRUE;
}

/*
 * ipv4_addr_to_str()
 *	convert a ipv4_addr_t ip address into a string
 *
 * The caller is responsabile for ensuring that enough
 * space exists to hold a complete dot representation
 * plus the string termination character (16 chars)
 */
void ipv4_addr_to_str(ipv4_addr_t ip, char *s)
{
	lgword_t iplong;
	iplong.lgword = arch_u32_to_be_u32 (ip);
//	for (fast_u8_t i = 0; i < 4; i++) {
	fast_u8_t i;
	
	for (i = 0; i < 4; i++) {
		u8_t val = iplong.bytes[i];
		if (val >= 100) {
			*s++ = '0' + (val / 100);
			val %= 100;
			*s++ = '0' + (val / 10);
			val %= 10;
			*s++ = '0' + (val);
		} else if (val >= 10) {
			*s++ = '0' + (val / 10);
			val %= 10;
			*s++ = '0' + (val);
		} else {
			*s++ = '0' + (val);
		}
		*s++ = '.';
	}
	*--s = 0;
}


/*
 * is_valid_host_ip
 *  Verify that an IP address is valid.
 */
bool_t is_valid_host_ip(ipv4_addr_t ip)
{
    /*
     * Don't allow non-unicast or loopback IP addresses.
     */
    return ((ip != 0) && (ip < 0xe0000000) && ((ip & 0x7f000000) != 0x7f000000));
}



/*
 * is_valid_host_ip_with_subnet
 *  Verify that an IP address is valid.
 */
bool_t is_valid_host_ip_with_subnet(ipv4_addr_t ip, ipv4_addr_t mask)
{
    if (!is_valid_host_ip(ip)) {
        return FALSE;
    }

    /*
     * Network address.
     */
    if ((ip & mask) == ip) {
        return FALSE;
    }

    /*
     * Broadcast address.
     */
    ipv4_addr_t wildcard = ~mask;
    if ((ip & wildcard) == wildcard) {
        return FALSE;
    }
    return TRUE;
}

/*
 * is_valid_subnet_mask()
 *  Verify that a subnet mask is valid.
 */
static int is_valid_subnet_mask(unsigned long mask)
{
    if (!mask || (mask == 0xffffffff)) {
        return 0;
    }

    /*
     * Ensure that we don't have any more significant zero bits after the least
     * significant one bit.
     */
    mask = (~mask) + 1;
    if ((mask & (-mask)) != mask) {
        return 0;
    }
    return 1;
}

/*
 * is_valid_network_ip_with_subnet
 *  Verify that an IP network address is valid.
 */
bool_t is_valid_network_ip_with_subnet(ipv4_addr_t ip, ipv4_addr_t mask)
{
    if (!is_valid_host_ip(ip)) {
        return FALSE;
    }

    /*
     * Mask must be valid (may also be a network of one host)
     */
    if (!is_valid_subnet_mask(mask) && (mask != 0xffffffff)) {
        return FALSE;
    }

    /*
     * Must not be to a host (except where mask defines a single host).
     */
    if ((ip & mask) != ip) {
        return FALSE;
    }

    return TRUE;
}


// return 0 if ok
long check_ip_mask(char *ip2_str,char *ip_str,char *mask_str)
{
	ipv4_addr_t	ip,mask;
	ipv4_addr_t	ip2;
	long ret;
	
	ret = ipv4_addr_from_str(&ip,	ip_str);
	dprintf("%lx %lx\n",ip,ret);
	
	if( !ret) 
		return -1;
	
	ret = ipv4_addr_from_str(&ip2,	ip2_str);
	dprintf("%lx %lx\n",ip2,ret);
	
	if( !ret) 
		return -1;

	
	ret = ipv4_addr_from_str(&mask, mask_str);
	dprintf("%lx %lx\n",mask,ret);
	if( !ret) 
		return -1;

	if( (ip & mask) == (ip2 & mask) ) {
		return 0;
	}
	else {
		return -1;
	}
	

}


long check_mask(char *mask)
{
	ipv4_addr_t	mask_ip;
	long ret;
	
	ret = ipv4_addr_from_str(&mask_ip,	mask);
	dprintf("%lx %lx\n",mask_ip,ret);
	if( !ret) 
		return -1;
	
	if( is_valid_subnet_mask(mask_ip) ) {
		return 0;
	}
	
	return -1;

}


long is_ip_same(char *ip2_str,char *ip_str)
{
	ipv4_addr_t	ip;
	ipv4_addr_t	ip2;
	long ret;
	
	ret = ipv4_addr_from_str(&ip,	ip_str);
	dprintf("%lx %lx\n",ip,ret);
	
	if( !ret) 
		return -1;
	
	ret = ipv4_addr_from_str(&ip2,	ip2_str);
	dprintf("%lx %lx\n",ip2,ret);
	
	if( !ret) 
		return -1;

	

	if( ip  ==  ip2 ) {
		return 0;
	}
	else {
		return -1;
	}
	

}

long is_ip(char *ip_str)
{
	ipv4_addr_t	ip;
//	ipv4_addr_t	ip2;
	long ret;
	
	ret = ipv4_addr_from_str(&ip,	ip_str);
	dprintf("%lx %lx\n",ip,ret);
	
	if( !ret) 
		return -1;

	return 0;	
}


/*
 * is_valid_hex_string()
 */
bool_t is_valid_hex_string(char *s)
{
    while (*s != '\0') {
        char c = tolower(*s++);
        if (!((c >= 'a') && (c <= 'f')) && !((c >= '0') && (c <= '9'))) {
            return FALSE;
        }
    }
    return TRUE;
}













//
#include <nvram.h>

#include <linux_vct.h>



// reference httpd ajax.c

char* get_internetonline_check(char *buf)
{
    char wan_interface[8] = {};
    char status[15] = {};
    char *wan_eth = NULL;
    char *wan_proto = NULL;

    wan_eth = nvram_safe_get("wan_eth");
    wan_proto = nvram_safe_get("wan_proto");

    if(strcmp(wan_proto, "dhcpc") == 0 || strcmp(wan_proto, "static") == 0)
        strcpy(wan_interface, wan_eth);
    else
        strcpy(wan_interface, "ppp0");
/*  Date: 2009-01-20
*   Name: jimmy huang
*   Reason: for usb-3g detect status
*   Note:   0: disconnected
            1: connected
*/
#ifdef CONFIG_USER_3G_USB_CLIENT
    if(strncmp(wan_proto, "usb3g",strlen("usb3g")) == 0){
        DEBUG_MSG("%s,wan_proto = usb_3g\n",__FUNCTION__);
        if(is_3g_device_connect()){
            DEBUG_MSG("%s,usb_3g is connect\n",__FUNCTION__);
            if(read_ipaddr(wan_interface) == NULL){
                strcpy(status,"disconnect");/*Internet Offline*/
            }else{
                strcpy(status,"connect");/*Internet Online*/
            }
        }else{
            strcpy(status,"disconnect");/*Internet Offline*/
            DEBUG_MSG("%s,usb_3g is not connect\n",__FUNCTION__);
        }
    }else{
        /* Check phy connect statue */
        VCTGetPortConnectState( wan_eth, VCTWANPORT0, status);
    }

        if( strncmp("disconnect", status, 10) == 0)
            strcpy(buf,"0");/*Internet Offline*/
        else if(read_ipaddr(wan_interface) == NULL)
            strcpy(buf,"0"); /*Internet Offline*/
        else
            strcpy(buf,"1");/*Internet Online*/

#else
    /* Check phy connect statue */
    VCTGetPortConnectState( wan_eth, VCTWANPORT0, status);

    if( strncmp("disconnect", status, 10) == 0)
        strcpy(buf,"0");/*Internet Offline*/
    else if(read_ipaddr(wan_interface) == NULL)
        strcpy(buf,"0"); /*Internet Offline*/
    else
        strcpy(buf,"1");/*Internet Online*/
#endif
    return buf;
}




// reference httpd widget.c
struct statistics {
	char  rx_packets[20];
	char  rx_errors[20];
	char  rx_dropped[20];
	char  rx_bytes[20];
	char  tx_packets[20];
	char  tx_errors[20];
	char  tx_dropped[20];
	char  tx_collisions[20];
	char  tx_bytes[20];	
};


#define WLAN0_ETH nvram_safe_get("wlan0_eth")

int _system(const char *fmt, ...)
{
	va_list args;
	int i;
	char buf[512];

	va_start(args, fmt);
	vsprintf(buf, fmt,args);
	va_end(args);

	dprintf("%s\n",buf);
//	DEBUG_MSG("%s\n",buf);
	i = system(buf);
	return i;

}

// static
 void *get_statistics(char *interface, struct statistics *st)
{
	#define FILE_SIZE 2048 // or more
    FILE *fp = NULL;
    char data[FILE_SIZE], *now_ptr, *p_pkts_s = NULL, *p_pkts_e = NULL;
//    int file_len = 0;
//    char p_tx_lossbytes[sizeof(long)*8+1] ={0};
//    char p_rx_lossbytes[sizeof(long)*8+1] ={0};
//    unsigned long total_loss_bytes = 0;

    if ((strcmp(interface, WLAN0_ETH) == 0) && (nvram_match("wlan0_enable", "0") == 0))
        return NULL;

    _system("ifconfig %s > /var/misc/pkts069.txt", interface);
    fp = fopen("/var/misc/pkts069.txt", "r");
    if (fp == NULL)
        return NULL;

    fread(data, FILE_SIZE, 1, fp);
    fclose(fp);
    memset(st, 0, sizeof(struct statistics));
    
    /* RX packets */
    p_pkts_s = strstr(data, "RX packets:") + strlen("RX packets:");
    p_pkts_e = strstr(p_pkts_s, " ");
    if (p_pkts_s == NULL || p_pkts_e == NULL){
        dprintf("RX pkts not found\n");
        return NULL;
    }
    memcpy(st->rx_packets, p_pkts_s, p_pkts_e - p_pkts_s);
    now_ptr = p_pkts_s;

    /* RX errors */
    p_pkts_s = strstr(now_ptr, "errors:") + strlen("errors:");
    p_pkts_e = strstr(p_pkts_s, " ");
    if (p_pkts_s == NULL || p_pkts_e == NULL){
        dprintf("RX errors not found\n");
        return NULL;
    }
    memcpy(st->rx_errors, p_pkts_s, p_pkts_e - p_pkts_s);
    now_ptr = p_pkts_s;

    /* RX dropped */
    p_pkts_s = strstr(now_ptr, "dropped:") + strlen("dropped:");
    p_pkts_e = strstr(p_pkts_s, " ");
    if (p_pkts_s == NULL || p_pkts_e == NULL){
        dprintf("RX dropped not found\n");
        return NULL;
    }
    memcpy(st->rx_dropped, p_pkts_s, p_pkts_e - p_pkts_s);
    now_ptr = p_pkts_s;
        
    /* TX packets */
    p_pkts_s = strstr(data, "TX packets:") + strlen("TX packets:");
    p_pkts_e = strstr(p_pkts_s, " ");
    if (p_pkts_s == NULL || p_pkts_e == NULL){
        dprintf("TX pkts not found\n");
        return NULL;
    }
    memcpy(st->tx_packets, p_pkts_s, p_pkts_e - p_pkts_s);
    now_ptr = p_pkts_s;

    /* TX dropped */
    p_pkts_s = strstr(now_ptr, "dropped:") + strlen("dropped:");
    p_pkts_e = strstr(p_pkts_s, " ");
    if (p_pkts_s == NULL || p_pkts_e == NULL){
        dprintf("TX dropped not found\n");
        return NULL;
    }
    memcpy(st->tx_dropped, p_pkts_s, p_pkts_e - p_pkts_s);
    now_ptr = p_pkts_s;
    
    /* TX collisions */
    p_pkts_s = strstr(now_ptr, "collisions:") + strlen("collisions:");
    p_pkts_e = strstr(p_pkts_s, " ");
    if (p_pkts_s == NULL || p_pkts_e == NULL){
        dprintf("TX collisions not found\n");
        return NULL;
    }
    memcpy(st->tx_collisions, p_pkts_s, p_pkts_e - p_pkts_s);
    now_ptr = p_pkts_s;
        
    /* RX bytes */
    p_pkts_s = strstr(now_ptr, "RX bytes:") + strlen("RX bytes:");
    p_pkts_e = strstr(p_pkts_s, " ");
    if (p_pkts_s == NULL || p_pkts_e == NULL){
        dprintf("RX bytes not found\n");
        return NULL;
    }
    memcpy(st->rx_bytes, p_pkts_s, p_pkts_e - p_pkts_s);
    now_ptr = p_pkts_s;
    
    /* TX bytes */
    p_pkts_s = strstr(now_ptr, "TX bytes:") + strlen("TX bytes:");
    p_pkts_e = strstr(p_pkts_s, " ");
    if (p_pkts_s == NULL || p_pkts_e == NULL){
        dprintf("TX bytes not found\n");
        return NULL;
    }
    memcpy(st->tx_bytes, p_pkts_s, p_pkts_e - p_pkts_s);
    
    return st;
}	



//#include <net/if.h>
//#include <netinet/in.h>
#include <sys/timeb.h>
//#include <netdb.h>
//#include <netinet/in.h>
//#include <arpa/inet.h>
#include <time.h>


//#define DEBUG_MSG(A,...)
#define DEBUG_MSG dprintf

// reference httpd ajax.c
#define ICMP_ECHO       8
#define ICMP_ECHOREPLY  0
#define ICMP_MIN        8        // Minimum 8 byte icmp packet (just header)

//
// IP header
//
struct iphdr
{
  unsigned int h_len:4;          // length of the header
  unsigned int version:4;        // Version of IP
  unsigned char tos;             // Type of service
  unsigned short total_len;      // total length of the packet
  unsigned short ident;          // unique identifier
  unsigned short frag_and_flags; // flags
  unsigned char  ttl;
  unsigned char proto;           // protocol (TCP, UDP etc)
  unsigned short checksum;       // IP checksum

  unsigned int source_ip;
  unsigned int dest_ip;
} __attribute__ ((packed));

//
// ICMP header
//
struct icmphdr
{
  unsigned char i_type;
  unsigned char i_code; // type sub code
  unsigned short i_cksum;
  unsigned short i_id;
  unsigned short i_seq;
} __attribute__ ((packed));

#define DEF_PACKET_SIZE 32
#define MAX_PACKET_SIZE 256

static unsigned short ping_id = 0;
static unsigned short seq_no = 0;
static struct timeb start, end;

static unsigned short checksum(unsigned short *buffer, int size)
{
    unsigned long cksum = 0;

    while (size > 1) {
        cksum += *buffer++;
        size -= sizeof(unsigned short);
    }

    if (size) cksum += *(unsigned char *) buffer;

    cksum = (cksum >> 16) + (cksum & 0xffff);
    cksum += (cksum >> 16);
    return (unsigned short) (~cksum);
}

static void fill_icmp_data(char *icmp_data, int datasize)
{
    struct icmphdr *icmp_hdr;
    char *datapart;

    icmp_hdr = (struct icmphdr *) icmp_data;

    icmp_hdr->i_type = ICMP_ECHO;
    icmp_hdr->i_code = 0;
    icmp_hdr->i_id = ++ping_id;
    icmp_hdr->i_cksum = 0;
    icmp_hdr->i_seq = ++seq_no;

    datapart = icmp_data + sizeof(struct icmphdr);
    memset(datapart, 'E', datasize - sizeof(struct icmphdr));
}

struct ping_diag {
	unsigned long MaximumResponseTime;
	unsigned long MinimumResponseTime;
	unsigned long AverageResponseTime;
	unsigned long FailureCount;
	unsigned long SuccessCount;
	unsigned long DSCP;          // ???
	unsigned long DataBlockSize; // ???
	unsigned long Timeout;
	unsigned long NumberOfRepetitions;	
	//char Host
//	Interface
//	DiagnosticsState
	
};


int start_ping4(char *host, int *ttl,struct ping_diag *diag)
{
    int sockraw;
    struct sockaddr_in dest, from;
    int datasize = DEF_PACKET_SIZE;
    socklen_t fromlen = sizeof(from);
    struct timeval timeout;
    char icmp_data[MAX_PACKET_SIZE];
    char recvbuf[MAX_PACKET_SIZE];

    struct addrinfo hints, *res;
    struct in_addr addr;

	int min_update;
	
	min_update = 0;

    ftime(&start);
    memset(&dest, 0, sizeof(dest));
    memset(&hints, 0, sizeof(hints));
    memset(icmp_data, 0, MAX_PACKET_SIZE);
    memset(recvbuf, 0, MAX_PACKET_SIZE);

    hints.ai_socktype = SOCK_STREAM;
    hints.ai_family = AF_INET;


// init
	diag->MaximumResponseTime = 0;
	diag->MinimumResponseTime = 0;
	diag->AverageResponseTime = 0;
	diag->SuccessCount = 0;
	diag->FailureCount = 0;


//dprintf("1\n");
    if (getaddrinfo(host, NULL, &hints, &res) != 0) {
        dprintf("Ping: getaddrinfo error\n");
        return -1;
    }

//dprintf("2\n");
    addr.s_addr = ((struct sockaddr_in *)(res->ai_addr))->sin_addr.s_addr;
    DEBUG_MSG("Ping ip address : %s\n", inet_ntoa(addr));
    freeaddrinfo(res);

    dest.sin_addr.s_addr = addr.s_addr;
    dest.sin_family = AF_INET;
    dest.sin_port = htons(53);


    sockraw = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if (sockraw < 0) {
        dprintf("Ping: Error socket\n");
        return -1;
    }


	

    timeout.tv_sec = diag->Timeout / 1000;
    timeout.tv_usec = (diag->Timeout % 1000)*1000; // 100ms

//    timeout.tv_sec = 1;
    
    if( (timeout.tv_usec == 0) && (timeout.tv_sec == 0) ) {
    	timeout.tv_sec = 1;    	
    }
    
    if (setsockopt(sockraw, SOL_SOCKET, SO_RCVTIMEO, (void *) &timeout, sizeof(timeout)) < 0) {
        dprintf("Set recv socket error\n");
        return -1;
    }

    if (setsockopt(sockraw, SOL_SOCKET, SO_SNDTIMEO, (void *) &timeout, sizeof(timeout)) < 0) {
        dprintf("Set send socket error\n");
        return -1;
    }

    DEBUG_MSG("PING %s : %d data bytes\n\n", host, datasize);


	dprintf("%ld\n",sizeof(struct timeb));
	//struct timeb

int i;

for(i=0;i< diag->NumberOfRepetitions;i++) {


    ftime(&start);

    struct icmphdr *icmphdr = (struct icmphdr *) &icmp_data;
    fill_icmp_data(icmp_data, datasize + sizeof(struct icmphdr));
    icmphdr->i_cksum = checksum((unsigned short *) icmp_data, datasize + sizeof(struct icmphdr));

    if (sendto(sockraw, icmp_data, datasize + sizeof(struct icmphdr), 0, (struct sockaddr *) &dest, sizeof(dest)) < 0) {
        dprintf("Sendto() timed out\n");
        return -1;
    }

read:
    if (recvfrom(sockraw, recvbuf, MAX_PACKET_SIZE, 0, (struct sockaddr *) &from, &fromlen) < 0) {
        dprintf("Recvfrom() timed out\n");
//        return -1;
		diag->FailureCount++;
		continue;
    }

//    close(sockraw);

    /* decode packet */
    struct iphdr *iphdr = (struct iphdr *) recvbuf;
    icmphdr = (struct icmphdr *) (recvbuf + sizeof(struct iphdr));

    if (icmphdr->i_type == ICMP_ECHO) {
        if (iphdr->source_ip != iphdr->dest_ip) {
            DEBUG_MSG("Invalid echo pkt recvd\n");
            return -1;
        }
    }
    else if (icmphdr->i_type != ICMP_ECHOREPLY) {
        dprintf("Non-echo type %d recvd\n", icmphdr->i_type);
        return -1;
    }

    if (icmphdr->i_id != ping_id) {
        dprintf("Wrong ID : %d   %d\n", icmphdr->i_id, ping_id);
        //return -1;
		diag->FailureCount++;
        goto read;
//        continue;
    }

    DEBUG_MSG("Ping: TTL=%d\n", iphdr->ttl);
    *ttl = iphdr->ttl;
    ftime(&end);


	long ret = ((end.time - start.time) * 1000 + (end.millitm - start.millitm));
	
	if( ret > diag->MaximumResponseTime)
		 diag->MaximumResponseTime = ret;
		 
	if( min_update == 0) {
		min_update = 1;
		diag->MinimumResponseTime = ret;
	}	 
	else {	
		if( ret < diag->MinimumResponseTime)
			diag->MinimumResponseTime = ret;
	}
	
    DEBUG_MSG("Ping: time=%ld ms\n", (end.time - start.time) * 1000 + (end.millitm - start.millitm));
//    return ((end.time - start.time) * 1000 + (end.millitm - start.millitm));

	diag->SuccessCount++;

	diag->AverageResponseTime += ret;

}
    close(sockraw);

	int total_count;
	
	total_count = diag->SuccessCount + diag->FailureCount;
	
	if( total_count > 0) {
		diag->AverageResponseTime /= total_count;
	}

	DEBUG_MSG("%ld %ld %ld\n",diag->MaximumResponseTime,diag->MinimumResponseTime,diag->AverageResponseTime);
	DEBUG_MSG("%ld %ld \n",diag->SuccessCount,diag->FailureCount);

// update

	return 0;
}





unsigned long uptime(void);


static void trim_string_char(char *s,char c)
{
	char *src;
	char *dst;

	dst = src = s;

	while( *src ) {

		if( *src == c) {
			src++;
			continue;
		}
		*dst = *src;
		src++;
		dst++;
	}

	*dst = 0;
}
 
 
 
static int sub_nvram_get(struct node *n, char *data)
{
	char *field = n->nvram_field;
//	dprintf(" -->nvram\n");
	strcpy(data,nvram_safe_get(field));
	return 0;
}

static int sub_nvramOUI_get(struct node *n, char *data)
{
	char *field = n->nvram_field;
//	dprintf(" -->nvramOUI\n");
	strcpy(data,nvram_safe_get(field));
	trim_string_char(data,':');
	data[6] = 0; // only 6 bytes
	return 0;
}
static int sub_nvramMAC_get(struct node *n, char *data)
{
	char *field = n->nvram_field;
//	dprintf(" -->nvramOUI\n");
	strcpy(data,nvram_safe_get(field));
	trim_string_char(data,':');
	return 0;
}
static int sub_version_get(struct node *n, char *data)
{
	char *field = n->nvram_field;
//	char *s;
	if(war_strcasecmp(field, "HW_VERSION") == 0)	{
		strcpy(data,HW_VERSION);
		return 0;
//		update_value = 1;	
	}
	else if(war_strcasecmp(field, "SW_VERSION") == 0)	{
		
		strcpy(data,nvram_safe_get("model_number"));
		trim_string_char(data,'-');
		strcat(data,HW_VERSION);

		strcat(data,"_FW");

		strcat(data,VER_FIRMWARE);
		strcat(data,"B");
		strcat(data,VER_BUILD);
		trim_string_char(data,'.');
		return 0;
//		update_value = 1;	
	}


	return -1;
}

#define SCOPE_LOCAL     0
#define SCOPE_GLOBAL    1

#define MAX_IPV6_IP_LEN	45
int get_wan_ipv6(char *ipv6);
char *read_ipv6addr(const char *if_name,const int scope, char *ipv6addr,const int length);

int get_lan_ipv6(char *ipv6)
{
    if (read_ipv6addr(nvram_safe_get("lan_bridge"), SCOPE_GLOBAL, ipv6, MAX_IPV6_IP_LEN)==NULL)
    	return 0;
//        strcpy(global_lanip,"(null)"); //avoiding ui get empty string cause show error
    return 1;
}

static int sub_waninfo_get(struct node *n, char *data)
{
	char *field = n->nvram_field;
	char wan_ip[192]={0}; // this is buff for other use
	char wan_proto[16];


	if(war_strcasecmp(field, "DNSServers") == 0)	{
		// 2010.03.05
//		char wan_ip[192]={0};
		char *wanip, *netmask, *gateway, *primary_dns, *secondary_dns;

		memcpy(wan_ip, read_current_ipaddr(0), sizeof(wan_ip));
		getStrtok(wan_ip, "/", "%s %s %s %s %s", &wanip, &netmask, &gateway, &primary_dns, &secondary_dns);

		data[0] = 0;
		if( primary_dns ) {		
	    	strcat(data, primary_dns);
		}
		if( secondary_dns) {
			strcat(data,",");
        	strcat(data, secondary_dns);
		}


		return 0;
	}
	else if(war_strcasecmp(field, "DefaultGateway") == 0)	{
//		char wan_ip[192]={0};
		char *wanip, *netmask, *gateway, *primary_dns, *secondary_dns;

		memcpy(wan_ip, read_current_ipaddr(0), sizeof(wan_ip));
		getStrtok(wan_ip, "/", "%s %s %s %s %s", &wanip, &netmask, &gateway, &primary_dns, &secondary_dns);

		if( gateway)
  	      strcpy(data, gateway);
        return 0;
		
	}
	
	else if(war_strcasecmp(field, "SubnetMask") == 0)	{
	
//		char wan_ip[192]={0};
		char *wanip, *netmask, *gateway, *primary_dns, *secondary_dns;

		memcpy(wan_ip, read_current_ipaddr(0), sizeof(wan_ip));
		getStrtok(wan_ip, "/", "%s %s %s %s %s", &wanip, &netmask, &gateway, &primary_dns, &secondary_dns);

		if( netmask)
	        strcpy(data, netmask);
        return 0;
	
	}
	
	
	else if(war_strcasecmp(field, "ExternalIPAddress") == 0)	{
		// 2010.03.05
//		char wan_ip[192]={0};
		char *wanip, *netmask, *gateway, *primary_dns, *secondary_dns;
    char global_wanip[5*MAX_IPV6_IP_LEN+4]={0};
    int ipv6_exist;
    
    ipv6_exist = get_wan_ipv6(global_wanip);



		memcpy(wan_ip, read_current_ipaddr(0), sizeof(wan_ip));
		getStrtok(wan_ip, "/", "%s %s %s %s %s", &wanip, &netmask, &gateway, &primary_dns, &secondary_dns);

			if( ipv6_exist ) {
				trim_string_char(global_wanip,' ');
					
					char *s;
					s = strchr(global_wanip,'/');
					if( s)
						*s = 0;
						
					
        	strcpy(data,global_wanip);
				}
				else

		if( wanip )
        	strcpy(data, wanip);
        return 0;
	}	
	
	else if(war_strcasecmp(field, "AddressingType") == 0)	{
		// check mode @@
		int dhcp;
		dhcp = 0; // static
		if (strcmp(nvram_safe_get("wan_proto"), "static") == 0){
			dhcp = 0;
		}
		else if (strcmp(nvram_safe_get("wan_proto"), "dhcpc") == 0){
			dhcp = 1;
		}
		else if (strcmp(nvram_safe_get("wan_proto"), "pppoe") == 0){

			if (strcmp(nvram_get("wan_pppoe_dynamic_00"), "0") == 0) {
				//wan_settings.protocol = "StaticPPPoE";
				//type = 2;
				dhcp = 0;
			}
			else if (strcmp(nvram_get("wan_pppoe_dynamic_00"), "1") == 0) {
				//wan_settings.protocol = "DHCPPPPoE";
				//type = 3;
				dhcp = 1;
			}
		}
		else if (strcmp(nvram_safe_get("wan_proto"), "pptp") == 0){
		
			if (strcmp(nvram_get("wan_pptp_dynamic"), "0") == 0){
				//wan_settings.protocol = "StaticPPTP";
				//type = 5;
				dhcp = 0;
			}
			else if (strcmp(nvram_get("wan_pptp_dynamic"), "1") == 0){
				//wan_settings.protocol = "DynamicPPTP";
				//type = 6;
				dhcp = 1;
			}
		}

		else if (strcmp(nvram_safe_get("wan_proto"), "l2tp") == 0){
			if (strcmp(nvram_get("wan_l2tp_dynamic"),"0") == 0) {
				//wan_settings.protocol = "StaticL2TP";
				//type = 7;
				dhcp = 0;
				
			}
			else if(strcasecmp(nvram_get("wan_l2tp_dynamic"),"1") == 0) {
				//wan_settings.protocol = "DynamicL2TP";
				//type = 8;
				dhcp = 1;
			}
		}

		
		if( dhcp ) {
			strcpy(data,"DHCP");
		}
		else {
			strcpy(data,"Static");			
		}


		return 0;
	}	
	
		
	else if(war_strcasecmp(field, "ConnectionStatus") == 0)	{
		
		char network_status_buf[64]={0};
		char *s_ret;
		long status;		
		
		s_ret = get_internetonline_check(network_status_buf);
		
		status = atoi( s_ret);

		if( status == 1) {		
        	strcpy( (char *)data,"Connected"); 		
    	}
    	else {
        	strcpy( (char *)data,"Disconnected"); 		
    	}
		
        return 0;
	}
		
	else if(war_strcasecmp(field, "wanip_enable") == 0)	{
		// 
		int enable;
		enable = 0;
		if (strcmp(nvram_safe_get("wan_proto"), "static") == 0){
			enable = 1;
		}
		else if (strcmp(nvram_safe_get("wan_proto"), "dhcpc") == 0){
			enable = 1;
		}
		
		if( enable) {
			strcpy(data,"true");
		}
		else {
			strcpy(data,"false");
		}
		return 0;
		
	}
	else if(war_strcasecmp(field, "wanppp_enable") == 0)	{
		// 
		int enable;
		enable = 1;
		if (strcmp(nvram_safe_get("wan_proto"), "static") == 0){
			enable = 0;
		}
		else if (strcmp(nvram_safe_get("wan_proto"), "dhcpc") == 0){
			enable = 0;
		}
		
		if( enable) {
			strcpy(data,"true");
		}
		else {
			strcpy(data,"false");
		}
		return 0;
		
	}
	else if(war_strcasecmp(field, "PossibleConnectionTypes") == 0)	{
		strcpy(data,"IP_Routed");
		return 0;
	}
	else if(war_strcasecmp(field, "ConnectionType") == 0)	{
		strcpy(data,"IP_Routed");
		return 0;
	}
		
	else if(war_strcasecmp(field, "Uptime") == 0)	{
		
//		char wan_proto[16];
		memset(wan_proto, 0 , sizeof(wan_proto));
		strcpy(wan_proto, nvram_safe_get("wan_proto"));
		sprintf(data,"%lu", get_wan_connect_time(wan_proto, 1));
		return 0;
	}	
		
	else if(war_strcasecmp(field, "ConnectionTrigger") == 0)	{
//		char wan_proto[16];
		memset(wan_proto, 0 , sizeof(wan_proto));
		strcpy(wan_proto, nvram_safe_get("wan_proto"));
	
        int demand = 0, alwayson = 0;
        if(strncmp(wan_proto, "pppoe", 5) ==0) {
                if(nvram_match("wan_pppoe_connect_mode_00","on_demand")==0)
                        demand = 1;
                else if(nvram_match("wan_pppoe_connect_mode_00","always_on")==0)
                        alwayson = 1;
        } else if(strcmp(wan_proto, "pptp") ==0 ) {
                if( nvram_match("wan_pptp_connect_mode", "on_demand")==0 )
                        demand = 1;
                else if(nvram_match("wan_pptp_connect_mode", "always_on")==0)
                        alwayson = 1;
        } else if(strcmp(wan_proto, "l2tp") ==0){
                if( nvram_match("wan_l2tp_connect_mode", "on_demand")==0 )
                        demand = 1;
                else if( nvram_match("wan_l2tp_connect_mode", "always_on")==0 )
                        alwayson = 1;
        }

		if( demand ) {
			sprintf(data,"%s","OnDemand");
		}
		else if( alwayson) {
			sprintf(data,"%s","AlwaysOn");		
		}
		else {
			sprintf(data,"%s","Manual");		
		}
		return 0;

	}	
	
	
	else if(war_strcasecmp(field, "PPPoEServiceName") == 0)	{
//		char wan_proto[16];
		memset(wan_proto, 0 , sizeof(wan_proto));
		strcpy(wan_proto, nvram_safe_get("wan_proto"));

        if(strncmp(wan_proto, "pppoe", 5) ==0) {
			sprintf(data,"%s",nvram_safe_get("wan_pppoe_service_00"));        	
		}	
		else {
			sprintf(data,"%s","");		
		}
		return 0;	
	}


	else if(war_strcasecmp(field, "RemoteIPAddress") == 0)	{
		memset(wan_proto, 0 , sizeof(wan_proto));
		strcpy(wan_proto, nvram_safe_get("wan_proto"));

        if(strcmp(wan_proto, "pptp") ==0 ) {
			sprintf(data,"%s",nvram_safe_get("wan_pptp_server_ip"));        	
 			
        } else if(strcmp(wan_proto, "l2tp") ==0){
			sprintf(data,"%s",nvram_safe_get("wan_l2tp_server_ip"));        	
        }
		else {
			sprintf(data,"%s","");		

			//???
			char *wanip, *netmask, *gateway, *primary_dns, *secondary_dns;

			memcpy(wan_ip, read_current_ipaddr(0), sizeof(wan_ip));
			getStrtok(wan_ip, "/", "%s %s %s %s %s", &wanip, &netmask, &gateway, &primary_dns, &secondary_dns);

			if( wanip )
        		strcpy(data, wanip);

		}
		return 0;	


	}

	else if(war_strcasecmp(field, "Username") == 0)	{
		memset(wan_proto, 0 , sizeof(wan_proto));
		strcpy(wan_proto, nvram_safe_get("wan_proto"));

        if(strncmp(wan_proto, "pppoe", 5) ==0) {
			sprintf(data,"%s",nvram_safe_get("wan_pppoe_username_00"));        	
		}	
        else if(strcmp(wan_proto, "pptp") ==0 ) {
			sprintf(data,"%s",nvram_safe_get("wan_pptp_username"));        	
 			
        }
        else if(strcmp(wan_proto, "l2tp") ==0){
			sprintf(data,"%s",nvram_safe_get("wan_l2tp_username"));        	
        }
		else {
			sprintf(data,"%s","");		
		}
		return 0;	
	}		




	else if(war_strcasecmp(field, "IdleDisconnectTime") == 0)	{
		memset(wan_proto, 0 , sizeof(wan_proto));
		strcpy(wan_proto, nvram_safe_get("wan_proto"));
		
		long t;
		
        if(strncmp(wan_proto, "pppoe", 5) ==0) {
			t = atol(nvram_safe_get("wan_pppoe_max_idle_time_00"));        	
		}
		else if(strcmp(wan_proto, "pptp") ==0 ) {
			t = atol(nvram_safe_get("wan_pptp_max_idle_time"));        				
        }
        else if(strcmp(wan_proto, "l2tp") ==0){
			t = atol(nvram_safe_get("wan_l2tp_max_idle_time"));        	
        }
		else {
			t = 0;
		}
		
		t *= 60; // seconds
		
		sprintf(data,"%ld",t);
		
		return 0;	
	}		
		
		
	else if( (war_strcasecmp(field, "Layer1DownstreamMaxBitRate") == 0) ||
			 (war_strcasecmp(field, "Layer1UpstreamMaxBitRate") == 0) ){
	
        char speed[5] ={0};
        char duplex[5] ={0};
		char status[15]= {0};
		long rate;

    	rate = 0;
	  	VCTGetPortConnectState( nvram_safe_get("wan_eth"), VCTWANPORT0, status);
  		
  		if(strcmp(status, "disconnect") == 0) {
  			rate = 0;
  		}
  		else {
			VCTGetPortSpeedState( nvram_safe_get("wan_eth"), VCTWANPORT0, speed, duplex);
			if( strcmp(speed,"giga")==0)  {
				rate = 1000;
			}	
			else
				rate = atol(speed);
		}
		
		rate *= 1000000;
		
		sprintf(data,"%ld",rate);
		return 0;
	
	}

	else if( war_strcasecmp(field, "PhysicalLinkStatus") == 0 ) {
		char status[15]= {0};
	  	VCTGetPortConnectState( nvram_safe_get("wan_eth"), VCTWANPORT0, status);
  		
  		if(strcmp(status, "disconnect") == 0) {
  			sprintf(data,"%s","Down");
  		}
  		else 
  			sprintf(data,"%s","Up");
  		
  		return 0;
	}

	else if( war_strcasecmp(field, "wan_proto") == 0 ) {

		sprintf(data,"%s",nvram_safe_get("wan_proto"));
		return 0;
	}
		
	
	return -1;
}



// wan stat 
static int sub_wanstat_get(struct node *n, char *data)
{
	char *field = n->nvram_field;

	struct statistics  wan_st;

	get_statistics(nvram_safe_get("wan_eth"), &wan_st);

	if(war_strcasecmp(field, "EthernetBytesSent") == 0)	{
		sprintf(data,"%s",wan_st.tx_bytes);
		return 0;
	}
	
	else if(war_strcasecmp(field, "EthernetBytesReceived") == 0)	{
		sprintf(data,"%s",wan_st.rx_bytes);
		return 0;
	}
	else if(war_strcasecmp(field, "EthernetPacketsSent") == 0)	{
		sprintf(data,"%s",wan_st.tx_packets);
		return 0;
	}
	else if(war_strcasecmp(field, "EthernetPacketsReceived") == 0)	{
		sprintf(data,"%s",wan_st.rx_packets);
		return 0;
	}

// other name
	else if(war_strcasecmp(field, "TotalBytesSent") == 0)	{
		sprintf(data,"%s",wan_st.tx_bytes);
		return 0;
	}
	
	else if(war_strcasecmp(field, "TotalBytesReceived") == 0)	{
		sprintf(data,"%s",wan_st.rx_bytes);
		return 0;
	}
	else if(war_strcasecmp(field, "TotalPacketsSent") == 0)	{
		sprintf(data,"%s",wan_st.tx_packets);
		return 0;
	}
	else if(war_strcasecmp(field, "TotalPacketsReceived") == 0)	{
		sprintf(data,"%s",wan_st.rx_packets);
		return 0;
	}


	return -1;
}



// lan info
static int sub_laninfo_get(struct node *n, char *data)
{
	char *field = n->nvram_field;
	char wan_ip[192]={0}; // this is buff for other use
//	char wan_proto[16];


	if(war_strcasecmp(field, "DNSServers") == 0)	{
		int relay;

		relay = atoi( nvram_safe_get("dns_relay"));

		if( relay ) {
			strcpy(data, nvram_safe_get("lan_ipaddr"));			
		}
		else {
			// get wan dns???
	//		char wan_ip[192]={0};
			char *wanip, *netmask, *gateway, *primary_dns, *secondary_dns;

			memcpy(wan_ip, read_current_ipaddr(0), sizeof(wan_ip));
			getStrtok(wan_ip, "/", "%s %s %s %s %s", &wanip, &netmask, &gateway, &primary_dns, &secondary_dns);


			strcpy(data,"");
			if( primary_dns ) {
				strcat(data,primary_dns);
			}
				
			if( secondary_dns ) {
				strcat(data,",");
				strcat(data,secondary_dns);
			}
		}
		return 0;	
	}


	else if(war_strcasecmp(field, "DHCPLeaseTime") == 0)	{
		
		long t;
		t = atol( nvram_safe_get("dhcpd_lease_time"));
		t *= 60;
		sprintf(data,"%ld",t);
		return 0;

	}
	

	else if(war_strcasecmp(field, "ReservedAddresses") == 0)	{
	
		int i;
		char buf[17];
		char *p_dhcp_reserve;
		char fw_value[192]={0};
		
		char *enable="", *name="", *ip="", *mac="";		

        strcpy((char *)data, "");

		for (i=0; i < 25; i++){ 
			sprintf(buf, "dhcpd_reserve_%02d", i);
			p_dhcp_reserve = nvram_safe_get(buf);
			if (p_dhcp_reserve == NULL || (strlen(p_dhcp_reserve) == 0))
				continue;
			memcpy(fw_value, p_dhcp_reserve, sizeof(fw_value));
			getStrtok(fw_value, "/", "%s %s %s %s ", &enable, &name, &ip, &mac);
			//xmlWrite(widget_xml+strlen(widget_xml),"<dhcpd_reserve_entry_%02d>",i);
			//xmlWrite(widget_xml+strlen(widget_xml),"<enable>%s</enable>",enable);
			//xmlWrite(widget_xml+strlen(widget_xml),"<name>%s</name>",name); 
			//xmlWrite(widget_xml+strlen(widget_xml),"<ip>%s</ip>",ip);	
			//xmlWrite(widget_xml+strlen(widget_xml),"<mac>%s</mac>",mac);
			//xmlWrite(widget_xml+strlen(widget_xml),"</dhcpd_reserve_entry_%02d>",i);

			// memory check
			if( strlen(data) >= 240) 
				break;

			if( strlen((char *)data) == 0) {
				strcpy((char *)data, ip);
			}
			else {
				strcat((char *)data, ",");
				strcat((char *)data, ip);
			}			
		}
		return 0;	
	}	


		
	
	
		

	return -1;
}

// lan stat 
static int sub_lanstat_get(struct node *n, char *data)
{
	char *field = n->nvram_field;

	struct statistics  wan_st;

	get_statistics(nvram_safe_get("lan_eth"), &wan_st);


	if(war_strcasecmp(field, "BytesSent") == 0)	{
		sprintf(data,"%s",wan_st.tx_bytes);
		return 0;
	}
	
	else if(war_strcasecmp(field, "BytesReceived") == 0)	{
		sprintf(data,"%s",wan_st.rx_bytes);
		return 0;
	}
	else if(war_strcasecmp(field, "PacketsSent") == 0)	{
		sprintf(data,"%s",wan_st.tx_packets);
		return 0;
	}
	else if(war_strcasecmp(field, "PacketsReceived") == 0)	{
		sprintf(data,"%s",wan_st.rx_packets);
		return 0;
	}


	return -1;
}


// reference chklst_version.c
void get_wlan_domain ( char* wlan_domain )
{
        char *hex_wlan_domain = NULL ;

#ifdef SET_GET_FROM_ARTBLOCK
        if(artblock_get("wlan0_domain"))
                hex_wlan_domain = artblock_safe_get("wlan0_domain");
        else
                hex_wlan_domain = nvram_safe_get("wlan0_domain");
	sprintf(wlan_domain,"%s", hex_wlan_domain);
#else
        hex_wlan_domain = nvram_safe_get("wlan0_domain");
        sprintf(wlan_domain,"%s", hex_wlan_domain);
#endif
}



// reference wireless.asp
//    var txrate_11b = new Array('Best (automatic)',11, 5.5, 2, 1);
//    var txrate_11g = new Array('Best (automatic)',54, 48, 36, 24, 18, 12, 9, 6);
//    var txrate_11bg = new Array('Best (automatic)',54, 48, 36, 24, 18, 12, 9, 6, 11, 5.5, 2, 1);
//    var txrate_11n = new Array('Best (automatic)','MCS 15 - 130 [270]', 'MCS 14 - 117 [243]', 'MCS 13 - 104 [216]', 'MCS 12 - 78 [162]', 'MCS 11 - 52 [108]', 'MCS 10 - 39 [81]', 'MCS 9 - 26 [54]', 'MCS 8 - 13 [27]', 'MCS 7 - 65 [135]', 'MCS 6 - 58.5 [121.5]', 'MCS 5 - 52 [108]', 'MCS 4 - 39 [81]', 'MCS 3 - 26 [54]', 'MCS 2 - 19.5 [40.5]', 'MCS 1 - 13 [27]', 'MCS 0 - 6.5 [13.5]');
//    var txrate_11gn = new Array('Best (automatic)','MCS 15 - 130 [270]', 'MCS 14 - 117 [243]', 'MCS 13 - 104 [216]', 'MCS 12 - 78 [162]', 'MCS 11 - 52 [108]', 'MCS 10 - 39 [81]', 'MCS 9 - 26 [54]', 'MCS 8 - 13 [27]', 'MCS 7 - 65 [135]', 'MCS 6 - 58.5 [121.5]', 'MCS 5 - 52 [108]', 'MCS 4 - 39 [81]', 'MCS 3 - 26 [54]', 'MCS 2 - 19.5 [40.5]', 'MCS 1 - 13 [27]', 'MCS 0 - 6.5 [13.5]', 54, 48, 36, 24, 18, 12, 9, 6);
//    var txrate_11bgn = new Array('Best (automatic)','MCS 15 - 130 [270]', 'MCS 14 - 117 [243]', 'MCS 13 - 104 [216]', 'MCS 12 - 78 [162]', 'MCS 11 - 52 [108]', 'MCS 10 - 39 [81]', 'MCS 9 - 26 [54]', 'MCS 8 - 13 [27]', 'MCS 7 - 65 [135]', 'MCS 6 - 58.5 [121.5]', 'MCS 5 - 52 [108]', 'MCS 4 - 39 [81]', 'MCS 3 - 26 [54]', 'MCS 2 - 19.5 [40.5]', 'MCS 1 - 13 [27]', 'MCS 0 - 6.5 [13.5]', 54, 48, 36, 24, 18, 12, 9, 6, 11, 5.5, 2, 1);
//    var txrate_11b = new Array('Best (automatic)',11, 5.5, 2, 1);
//    var txrate_11g = new Array('Best (automatic)',54, 48, 36, 24, 18, 12, 9, 6);
//    var txrate_11bg = new Array('Best (automatic)',54, 48, 36, 24, 18, 12, 9, 6, 11, 5.5, 2, 1);
//    var txrate_11n = new Array('Best (automatic)','MCS 15 - 130 [270]', 'MCS 14 - 117 [243]', 'MCS 13 - 104 [216]', 'MCS 12 - 78 [162]', 'MCS 11 - 52 [108]', 'MCS 10 - 39 [81]', 'MCS 9 - 26 [54]', 'MCS 8 - 13 [27]', 'MCS 7 - 65 [135]', 'MCS 6 - 58.5 [121.5]', 'MCS 5 - 52 [108]', 'MCS 4 - 39 [81]', 'MCS 3 - 26 [54]', 'MCS 2 - 19.5 [40.5]', 'MCS 1 - 13 [27]', 'MCS 0 - 6.5 [13.5]');
//   var txrate_11gn = new Array('Best (automatic)','MCS 15 - 130 [270]', 'MCS 14 - 117 [243]', 'MCS 13 - 104 [216]', 'MCS 12 - 78 [162]', 'MCS 11 - 52 [108]', 'MCS 10 - 39 [81]', 'MCS 9 - 26 [54]', 'MCS 8 - 13 [27]', 'MCS 7 - 65 [135]', 'MCS 6 - 58.5 [121.5]', 'MCS 5 - 52 [108]', 'MCS 4 - 39 [81]', 'MCS 3 - 26 [54]', 'MCS 2 - 19.5 [40.5]', 'MCS 1 - 13 [27]', 'MCS 0 - 6.5 [13.5]', 54, 48, 36, 24, 18, 12, 9, 6);
//    var txrate_11bgn = new Array('Best (automatic)','MCS 15 - 130 [270]', 'MCS 14 - 117 [243]', 'MCS 13 - 104 [216]', 'MCS 12 - 78 [162]', 'MCS 11 - 52 [108]', 'MCS 10 - 39 [81]', 'MCS 9 - 26 [54]', 'MCS 8 - 13 [27]', 'MCS 7 - 65 [135]', 'MCS 6 - 58.5 [121.5]', 'MCS 5 - 52 [108]', 'MCS 4 - 39 [81]', 'MCS 3 - 26 [54]', 'MCS 2 - 19.5 [40.5]', 'MCS 1 - 13 [27]', 'MCS 0 - 6.5 [13.5]', 54, 48, 36, 24, 18, 12, 9, 6, 11, 5.5, 2, 1);

char *txrate_11b[]   = {"Best (automatic)","11","5.5","2", "1",NULL};
char *txrate_11g[]   = {"Best (automatic)","54","48","36","24","18","12","9","6",NULL};
char *txrate_11bg[]  = {"Best (automatic)","54","48","36","24","18","12","9","6","11","5.5","2","1",NULL };
char *txrate_11n[]   = {"Best (automatic)","MCS 15 - 130 [270]", "MCS 14 - 117 [243]", "MCS 13 - 104 [216]", "MCS 12 - 78 [162]", "MCS 11 - 52 [108]", "MCS 10 - 39 [81]", "MCS 9 - 26 [54]", "MCS 8 - 13 [27]", "MCS 7 - 65 [135]", "MCS 6 - 58.5 [121.5]", "MCS 5 - 52 [108]", "MCS 4 - 39 [81]", "MCS 3 - 26 [54]", "MCS 2 - 19.5 [40.5]", "MCS 1 - 13 [27]", "MCS 0 - 6.5 [13.5]", NULL };
char *txrate_11gn[]  = {"Best (automatic)","MCS 15 - 130 [270]", "MCS 14 - 117 [243]", "MCS 13 - 104 [216]", "MCS 12 - 78 [162]", "MCS 11 - 52 [108]", "MCS 10 - 39 [81]", "MCS 9 - 26 [54]", "MCS 8 - 13 [27]", "MCS 7 - 65 [135]", "MCS 6 - 58.5 [121.5]", "MCS 5 - 52 [108]", "MCS 4 - 39 [81]", "MCS 3 - 26 [54]", "MCS 2 - 19.5 [40.5]", "MCS 1 - 13 [27]", "MCS 0 - 6.5 [13.5]", "54", "48", "36", "24", "18", "12", "9", "6",NULL};
char *txrate_11bgn[] = {"Best (automatic)","MCS 15 - 130 [270]", "MCS 14 - 117 [243]", "MCS 13 - 104 [216]", "MCS 12 - 78 [162]", "MCS 11 - 52 [108]", "MCS 10 - 39 [81]", "MCS 9 - 26 [54]", "MCS 8 - 13 [27]", "MCS 7 - 65 [135]", "MCS 6 - 58.5 [121.5]", "MCS 5 - 52 [108]", "MCS 4 - 39 [81]", "MCS 3 - 26 [54]", "MCS 2 - 19.5 [40.5]", "MCS 1 - 13 [27]", "MCS 0 - 6.5 [13.5]", "54", "48", "36", "24", "18", "12", "9", "6", "11", "5.5", "2", "1",NULL};




// wlan
static int sub_wlaninfo_get(struct node *n, char *data)
{
	char *field = n->nvram_field;
//	char wan_ip[192]={0}; // this is buff for other use
//	char wan_proto[16];


	if(war_strcasecmp(field, "Status") == 0)	{
		if( atol(nvram_safe_get("wlan0_enable"))) {
			sprintf(data,"%s","Up");
		}
		else {
			sprintf(data,"%s","Disabled");
		}
		return 0;		
	}

// reference ui wireless.asp
	else if(war_strcasecmp(field, "MaxBitRate") == 0)	{
		char *mode;
		long rate;
		char *ratestr;
		ratestr = NULL;
		rate = 0; // Default Auto
		mode = nvram_safe_get("wlan0_dot11_mode");
		if( war_strcasecmp(mode,"11b") == 0 ) {
			ratestr = nvram_safe_get("wlan0_11b_txrate");		
		}
		else if( war_strcasecmp(mode,"11g") == 0) {
			ratestr = nvram_safe_get("wlan0_11g_txrate");					
		}	
		else if( war_strcasecmp(mode,"11bg") == 0) {
			ratestr = nvram_safe_get("wlan0_11bg_txrate");					
		}	
		else if( war_strcasecmp(mode,"11n") == 0) {
			ratestr = nvram_safe_get("wlan0_11n_txrate");					
		}	
		else if( war_strcasecmp(mode,"11gn") == 0) {
			ratestr = nvram_safe_get("wlan0_11gn_txrate");					
		}	
		else if( war_strcasecmp(mode,"11bgn") == 0) {
			ratestr = nvram_safe_get("wlan0_11bgn_txrate");					
		}	

		if( !ratestr) {
			dprintf("ERROR ratestr\n");
			sprintf(data,"%s","ERR");
			return 0;
		}

		if( strncmp(ratestr,"MCS",3) == 0 ) {
			// find - or [
			char *tmp;
			if( atol(nvram_safe_get("wlan0_11n_protection")) == 20) {
				// 20MHZ
				tmp = strstr(ratestr,"-");
				if( tmp ) {
					tmp++;
					rate = atol(tmp);
				}
				else {
					dprintf("ERROR ratestr\n");
					sprintf(data,"%s","ERR1");
					return 0;
				}
			}
			else {
				// Auto 20/40
				tmp = strstr(ratestr,"[");
				if( tmp ) {
					tmp++;
					rate = atol(tmp);
				}
				else {
					dprintf("ERROR ratestr\n");
					sprintf(data,"%s","ERR2");
					return 0;
				}
			}
		}
		else {
			rate = atol(ratestr);
		}
		
		if( rate == 0) {
			sprintf(data,"Auto");
		}
		else 
			sprintf(data,"%ld",rate);
		
		return 0;
	}


	else if(war_strcasecmp(field, "WEPEncryptionLevel") == 0)	{
#if 0		
		if (strstr(nvram_safe_get("wlan0_security"), "disable")){
			sprintf(data,"%s","Disabled");
		}else if (strstr(nvram_safe_get("wlan0_security"), "64")){
			sprintf(data,"%s","40-bit");
		}else if (strstr(nvram_safe_get("wlan0_security"), "128")){
			sprintf(data,"%s","104-bit");		
		}else if (strstr(nvram_safe_get("wlan0_security"), "psk")){
			if (strcmp(nvram_safe_get("wlan0_security"), "wpa2auto_psk") == 0){
				sprintf(data,"%s","wpa_auto_psk");		
   	    	}else{
				sprintf(data,"%s",nvram_safe_get("wlan0_security"));
   	    	}			
		}else if (strstr(nvram_safe_get("wlan0_security"), "eap")){
		    if (strcmp(nvram_safe_get("wlan0_security"), "wpa2auto_eap") == 0){
				sprintf(data,"%s","wpa_auto_eap");
  	    	}else{
				sprintf(data,"%s",nvram_safe_get("wlan0_security"));
  	    	}
		}
#else
		sprintf(data,"Disabled,40-bit,104-bit");
#endif	
		return 0;
	}


	else if(war_strcasecmp(field, "BasicEncryptionModes") == 0)	{
		if (strstr(nvram_safe_get("wlan0_security"), "64")){
			sprintf(data,"%s","WEPEncryption");
		}else if (strstr(nvram_safe_get("wlan0_security"), "128")){
			sprintf(data,"%s","WEPEncryption");
		}
		else {
			sprintf(data,"%s","None");
		}		
		return 0;
	}
	

	else if(war_strcasecmp(field, "BasicAuthenticationMode") == 0)	{
		sprintf(data,"%s","None");
		if (strstr(nvram_safe_get("wlan0_security"), "64")){
			if (strstr(nvram_safe_get("wlan0_security"), "open")){
			}
			else {
				sprintf(data,"%s","SharedAuthentication")			;
			}
			
		}else if (strstr(nvram_safe_get("wlan0_security"), "128")){
			if (strstr(nvram_safe_get("wlan0_security"), "open")){
			}
			else {
				sprintf(data,"%s","SharedAuthentication")			;
			}
		}
		else {
			sprintf(data,"%s","None");
		}		
		return 0;
	}

	else if(war_strcasecmp(field, "WPAEncryptionModes") == 0)	{

		sprintf(data,"%s","WEPEncryption");
		if (strstr(nvram_safe_get("wlan0_security"), "disable")){
			sprintf(data,"%s","WEPEncryption");
		}else if (strstr(nvram_safe_get("wlan0_security"), "64")){
			sprintf(data,"%s","WEPEncryption");
		}else if (strstr(nvram_safe_get("wlan0_security"), "128")){
			sprintf(data,"%s","WEPEncryption");		
		}else if (strstr(nvram_safe_get("wlan0_security"), "psk")){
			if (strcmp(nvram_safe_get("wlan0_security"), "wpa2auto_psk") == 0){
				sprintf(data,"%s","TKIPandAESEncryption");		
   	    	}else{
				sprintf(data,"%s","TKIPEncryption");
   	    	}			
		}else if (strstr(nvram_safe_get("wlan0_security"), "eap")){
		    if (strcmp(nvram_safe_get("wlan0_security"), "wpa2auto_eap") == 0){
				sprintf(data,"%s","TKIPandAESEncryption");
  	    	}else{
				sprintf(data,"%s","AESEncryption");
  	    	}
		}
		return 0;
	}
	
	
	else if(war_strcasecmp(field, "WPAAuthenticationMode") == 0)	{
	
		sprintf(data,"%s","PSKAuthentication");
		if (strstr(nvram_safe_get("wlan0_security"), "disable")){
//			sprintf(data,"%s","WEPEncryption");
		}else if (strstr(nvram_safe_get("wlan0_security"), "64")){
//			sprintf(data,"%s","WEPEncryption");
		}else if (strstr(nvram_safe_get("wlan0_security"), "128")){
//			sprintf(data,"%s","WEPEncryption");		
		}else if (strstr(nvram_safe_get("wlan0_security"), "psk")){
			if (strcmp(nvram_safe_get("wlan0_security"), "wpa2auto_psk") == 0){
				sprintf(data,"%s","PSKAuthentication");		
   	    	}else{
				sprintf(data,"%s","PSKAuthentication");
   	    	}			
		}else if (strstr(nvram_safe_get("wlan0_security"), "eap")){
		    if (strcmp(nvram_safe_get("wlan0_security"), "wpa2auto_eap") == 0){
				sprintf(data,"%s","PSKAuthentication");
  	    	}else{
				sprintf(data,"%s","PSKAuthentication");
  	    	}
		}
		return 0;
		
	
	}	


	else if(war_strcasecmp(field, "PossibleChannels") == 0)	{
	
	
		char wlan_domain[30] = {};
		int domain;
		char channel_list_24G[128];
	
    	
#define WIRELESS_BAND_24G   0
extern int GetDomainChannelList(unsigned short number, int band, char wlan_channel_list[], int show_used_space);
	
		get_wlan_domain ( wlan_domain );	

		domain = 0x00;
	   	sscanf(wlan_domain,"%x",&domain);
	
		GetDomainChannelList(domain,WIRELESS_BAND_24G,channel_list_24G,1);
	
		sprintf(data,"%s",channel_list_24G);
		return 0;
	}
	

	else if(war_strcasecmp(field, "TotalAssociations") == 0)	{
	

	    T_WLAN_STAION_INFO* wireless_status = NULL;

	    wireless_status = get_stalist();
	    
	    if( wireless_status ) {
			sprintf(data,"%d",wireless_status->num_sta? : 0);
		}
		else {
			sprintf(data,"%d", 0);
		}
		return 0;			
		
	
	}




	return -1;
}



// wlan stat 
static int sub_wlanstat_get(struct node *n, char *data)
{
	char *field = n->nvram_field;

	struct statistics  wan_st;

	get_statistics(nvram_safe_get(WLAN0_ETH), &wan_st);


	if(war_strcasecmp(field, "TotalBytesSent") == 0)	{
		sprintf(data,"%s",wan_st.tx_bytes);
		return 0;
	}
	
	else if(war_strcasecmp(field, "TotalBytesReceived") == 0)	{
		sprintf(data,"%s",wan_st.rx_bytes);
		return 0;
	}
	else if(war_strcasecmp(field, "TotalPacketsSent") == 0)	{
		sprintf(data,"%s",wan_st.tx_packets);
		return 0;
	}
	else if(war_strcasecmp(field, "TotalPacketsReceived") == 0)	{
		sprintf(data,"%s",wan_st.rx_packets);
		return 0;
	}


	return -1;
}



// time 
static int sub_time_get(struct node *n, char *data)
{
	char *field = n->nvram_field;

	if(war_strcasecmp(field, "uptime") == 0)	{
		sprintf(data,"%lu",uptime());			
		return 0;
	}
	else if(war_strcasecmp(field, "CurrentLocalTime") == 0)	{

		sprintf(data,"%s",lib_current_time());
		return 0;
	}
	
	
	
	return -1;

}


//#define SCOPE_LOCAL     0
//#define SCOPE_GLOBAL    1

//#define MAX_IPV6_IP_LEN	45



int get_wan_ipv6(char *ipv6)
{
    char wan_interface[8] = {};
    char *wan_eth = NULL;
    char wan_proto[50];

//	char dhcp_pd_enable[10]={0};
//	char dhcp_pd[100]={0};
	char ppp6_ip[64] = {}; 
	char autoconfig_ip[64] = {}; 

    strcpy(wan_proto, nvram_safe_get("ipv6_wan_proto"));
//strcpy(lan_bridge, nvram_safe_get("lan_bridge"));
    wan_eth = nvram_safe_get("wan_eth");

    if (strcmp(wan_proto, "ipv6_6to4") == 0)
        strcpy(wan_interface, "tun6to4");
    else if (strcmp(wan_proto, "ipv6_6in4") == 0)
        strcpy(wan_interface, "sit1");
    else if (strcmp(wan_proto, "ipv6_pppoe") == 0)
    		if(nvram_match("ipv6_pppoe_share", "1") == 0)
						strcpy(wan_interface, "ppp0");
				else
        strcpy(wan_interface, "ppp6");
        else if(strcmp(wan_proto, "ipv6_6rd") == 0)
        	strcpy(wan_interface, "sit2");
        else if(strcmp(wan_proto, "ipv6_autodetect") == 0) 
		if(read_ipv6addr("ppp0", SCOPE_GLOBAL, ppp6_ip, sizeof(ppp6_ip))){ 
			strcpy(wan_interface, "ppp0"); 
			strcpy(wan_proto, "ipv6_pppoe"); 
		}else if(read_ipv6addr(wan_eth, SCOPE_GLOBAL, autoconfig_ip, sizeof(autoconfig_ip))){ 
			 strcpy(wan_interface, wan_eth); 
			 strcpy(wan_proto, "ipv6_autoconfig"); 
		 }else{ 
			 strcpy(wan_interface, wan_eth); 
			 strcpy(wan_proto, "ipv6_autodetect"); 
		} 
        else{
	strcpy(wan_interface, wan_eth);
	}


    if (read_ipv6addr(wan_interface, SCOPE_GLOBAL, ipv6, 5*MAX_IPV6_IP_LEN+4 )!=NULL)
    	return 1;

	return 0;	
}

// time 
static int sub_server_get(struct node *n, char *data)
{
	char *field = n->nvram_field;
	char wan_ip[192]={0}; // this is buff for other use


	if(war_strcasecmp(field, "ConnectionRequestURL") == 0)	{

    char global_wanip[5*MAX_IPV6_IP_LEN+4]={0};
    int ipv6_exist;
    
    ipv6_exist = get_wan_ipv6(global_wanip);



		// get wan ip
		char *wanip, *netmask, *gateway, *primary_dns, *secondary_dns;

		memcpy(wan_ip, read_current_ipaddr(0), sizeof(wan_ip));
		getStrtok(wan_ip, "/", "%s %s %s %s %s", &wanip, &netmask, &gateway, &primary_dns, &secondary_dns);

		if( wanip ) {
				if( ipv6_exist ) {
					trim_string_char(global_wanip,' ');
					
					char *s;
					s = strchr(global_wanip,'/');
					if( s)
						*s = 0;
						
					
        	sprintf(data,"http://[%s]%s",global_wanip,nvram_safe_get("tr069_cpe_url"));
				}
				else
        	sprintf(data,"http://%s%s",wanip,nvram_safe_get("tr069_cpe_url"));
		}
		else {
			// error ???
			sprintf(data,"http://192.168.0.1/%s",nvram_safe_get("tr069_cpe_url"));
		}
		return 0;
		
	}

	return -1;
}









// setting 


static int sub_nvram_set(struct node *n)
{
	char *field = n->nvram_field;
//	dprintf(" -->nvram\n");
//	strcpy(data,nvram_safe_get(field));

	// if type == boolean, we should change to 0 or 1
	if( war_strcasecmp(n->type,"boolean") == 0) {
		if( war_strcasecmp(n->value,"true") == 0) {
			nvram_set(field,"1");			
		}
		else if( war_strcasecmp(n->value,"false") == 0) {
			nvram_set(field,"0");	
		}
		else {
			nvram_set(field,n->value);
		}			
	}
	else {
		nvram_set(field,n->value);
	}
	return 0;
}


// lan info
static int sub_laninfo_set(struct node *n)
{
	char *field = n->nvram_field;
//	char wan_ip[192]={0}; // this is buff for other use
//	char wan_proto[16];
	char buff[128];

	if(war_strcasecmp(field, "DNSServers") == 0)	{
		// by pass
		return 0;	
	}


	else if(war_strcasecmp(field, "DHCPLeaseTime") == 0)	{
		
		// don't care
		long t;
		t = atol(n->value);
		t /= 60;
		if( t < 0) {
			t = 1440 * 365 * 10;  // 10 year
		}
		
		sprintf(buff,"%ld",t);
		
		nvram_set("dhcpd_lease_time",buff);

		return 0;

	}
	

	else if(war_strcasecmp(field, "ReservedAddresses") == 0)	{
		// by pass
			
		return 0;	
	}	


		
	
	
		

	return -1;
}



//wlan0_11n_protection   auto
static int sub_wlaninfo_set(struct node *n)
{
	char *field = n->nvram_field;
//	char wan_ip[192]={0}; // this is buff for other use
//	char wan_proto[16];
	char buff[128];

	if(war_strcasecmp(field, "MaxBitRate") == 0)	{
		
		nvram_set("wlan0_dot11_mode","11bgn");
		nvram_set("wlan0_11n_protection","20");
		
//		wlan0_11bgn_txrate
		
		if( war_strcasecmp(n->value,"Auto") == 0) {
			// not set 20 or auto
			nvram_set("wlan0_11bgn_txrate",txrate_11bgn[0]);
			return 0;
		}
		int i;
		int rate;
		rate = atol(n->value);
		i = 1;
		while( txrate_11bgn[i] != NULL) {
			if( memcmp(txrate_11bgn[i], "MCS", 3) == 0) {
				char *tmp;
				tmp = strstr( txrate_11bgn[i],"-");
				if( tmp ) {
					if( rate == atol(tmp+1)) {
						// width 20 
						nvram_set("wlan0_11bgn_txrate",txrate_11bgn[i]);
						return 0; // ok
					}
				}
				tmp = strstr( txrate_11bgn[i],"[");
				if( tmp ) {
					if( rate == atol(tmp+1)) {
						// width 40 or auto 
						nvram_set("wlan0_11bgn_txrate",txrate_11bgn[i]);
						nvram_set("wlan0_11n_protection","auto");
						
						return 0; // ok
					}
				}					
			}
			else {
				//
				if( rate == atol(txrate_11bgn[i]) ) {
					nvram_set("wlan0_11bgn_txrate",txrate_11bgn[i]);
					return 0;
				}
			}
			
			i++;
		}
		
		
	}
	else if( (war_strcasecmp(field, "wepkey1") == 0) ||
			 (war_strcasecmp(field, "wepkey2") == 0) ||
			 (war_strcasecmp(field, "wepkey3") == 0) ||
			 (war_strcasecmp(field, "wepkey4") == 0) ) {

		nvram_set("wlan0_wep_display","hex");
		int keyindex;
		keyindex = 1;
		if( strstr(field,"1")) 
			keyindex = 1;
		else if( strstr(field,"2")) 
			keyindex = 2;
		else if( strstr(field,"3")) 
			keyindex = 3;
		else if( strstr(field,"4")) 
			keyindex = 4;

		buff[0] = 0;
		if( strlen(n->value) == 10 ) {
			sprintf(buff,"wlan0_wep64_key_%d",keyindex);			
		}
		else if( strlen(n->value) == 26 ) {
			sprintf(buff,"wlan0_wep128_key_%d",keyindex);						
		}
		
		if( strlen(buff) == 0) {
			return -1;
		}
		
		nvram_set(buff,n->value);
		return 0;
	}

	else if(war_strcasecmp(field, "BasicEncryptionModes") == 0)	{
		// don't check?
		return 0;
	}
	else if(war_strcasecmp(field, "BasicAuthenticationMode") == 0)	{
		return 0;
	}
	else if(war_strcasecmp(field, "WPAEncryptionModes") == 0)	{
		return 0;
	}	
	else if(war_strcasecmp(field, "WPAAuthenticationMode") == 0)	{
		return 0;
	}	



	return -1;
}



static int sub_waninfo_set(struct node *n)
{
	char *field = n->nvram_field;
	char buff[256];

	if(war_strcasecmp(field, "DNSServers") == 0)	{

		char *tmp;
//		long ret;
		char *data;
		data = n->value;
		
		tmp = strstr(data,",");
		if( tmp == NULL) {
			// data is only ip
			if( war_strcasecmp(data,"0.0.0.0")==0) {
				nvram_set("wan_specify_dns","0");
				nvram_set("wan_primary_dns","0.0.0.0");
				nvram_set("wan_secondary_dns","0.0.0.0");
			}
			else {
				nvram_set("wan_specify_dns","1");
				nvram_set("wan_primary_dns",data);
				nvram_set("wan_secondary_dns","0.0.0.0");			
			}
			return 0;
		}	
		else {
			memcpy(buff,data,tmp-data);
			buff[tmp-data]=0;
			tmp++;
			//ret = is_ip(buff);
dprintf("1->[%s]\n",buff);			
			//if( ret != 0) 
			//	return -1;
			//ret = is_ip(tmp);
dprintf("2->[%s]\n",tmp);			
			//return ret;						
			// two ip 
			if( (war_strcasecmp(data,"0.0.0.0")==0) && (war_strcasecmp(tmp,"0.0.0.0")==0) ) {
				nvram_set("wan_specify_dns","0");
				nvram_set("wan_primary_dns","0.0.0.0");
				nvram_set("wan_secondary_dns","0.0.0.0");

			}
			else {
				nvram_set("wan_specify_dns","1");
				nvram_set("wan_primary_dns",buff);
				nvram_set("wan_secondary_dns",tmp);

			}
			
		}	
		return 0;
	}
	else if( war_strcasecmp(field, "wan_proto") == 0 ) {
		// by pass
		return 0;
	}



	return -1;

}







// nvram check setting table
struct checklist {
	char *para;
	long type;  // 1 number, 2 string, 3 boolean
	union {
		struct {
			long min;
			long max;
		} num;
		struct {
			long len;
		} str;
	};
		
};

#define CHECK_NUM  			1
#define CHECK_STR 			2
#define CHECK_BOOL 			3
#define CHECK_LANIP_MASK 	4
#define CHECK_MASK 			5
#define CHECK_CHANNEL 		6
#define CHECK_MAC	 		7

struct checklist check_table[]={
// ManagementServer
	{"tr069_inform_interval",	CHECK_NUM,			{{5,86400}} },
	{"tr069_inform_enable",		CHECK_BOOL			},
	{"tr069_acs_url",			CHECK_STR,			{{0}}	},
	{"tr069_acs_pwd",			CHECK_STR,			{{0}}	},
	{"tr069_acs_usr",			CHECK_STR,			{{0}}	},
	{"tr069_inform_time",		CHECK_STR,			{{0}}	},
	{"tr069_upgrade_managed",	CHECK_BOOL			},
// TIME	
	{"ntp_server",				CHECK_STR,			{{0}}	},
// LANHostConfigManagement
	{"dhcpd_domain_name",		CHECK_STR,			{{30}}	},

	{"dhcpd_enable",			CHECK_BOOL			},
	{"dhcpd_start",				CHECK_LANIP_MASK	},
	{"dhcpd_end",				CHECK_LANIP_MASK	},

	{"dns_relay",				CHECK_BOOL			},

	{"lan_netmask",				CHECK_MASK			},

// WLAN
	{"wlan0_enable",				CHECK_BOOL			},

	{"wlan0_channel",				CHECK_CHANNEL		},
	{"wlan0_auto_channel_enable",	CHECK_BOOL			},

	{"wlan0_ssid",					CHECK_STR,			{{32}}	},

	{"wlan0_wep_default_key",		CHECK_NUM,			{{1,4}} },


//
//
	{"wan_mac",						CHECK_MAC			},


	
	
	
	{NULL,0}
};

//tr069_inform_interval


// check value
static int sub_nvram_checkset(struct node *n,char *data)
{
	char *field = n->nvram_field;
	char buff[128];
	int i;
	
	i = 0;

	while( check_table[i].para != NULL) {		
		if( war_strcasecmp(field,check_table[i].para) == 0) {
			//dprintf("1\n");
			if( check_table[i].type == CHECK_NUM) {
				long l;
				
				l = atol(data);
			//dprintf("2 %ld\n",l);
				if( (l >= check_table[i].num.min ) && (l <= check_table[i].num.max)) {
					return 0;
				}
			}
			else if( check_table[i].type == CHECK_STR ) {
				if( check_table[i].str.len == 0) {
					return 0; // don't check len
				}
				else if( strlen(data) <= check_table[i].str.len) {
					return 0;
				}
			}
			else if( check_table[i].type == CHECK_BOOL ) {
				//if( check_table[i].str.len == 0) {
					return 0; // don't check
				//}
			}
			else if( check_table[i].type == CHECK_LANIP_MASK ) {
				if( check_ip_mask(data,nvram_safe_get("lan_ipaddr"),nvram_safe_get("lan_netmask")) == 0 ) {
					if( war_strcasecmp(data,nvram_safe_get("lan_ipaddr")) == 0) {
						// same ip @@
					}
					else {
						return 0;
					}
				}	
			}
			else if( check_table[i].type == CHECK_MASK ) {
				if( check_mask(data) == 0 ) {
						return 0;
				}	
			}
			else if( check_table[i].type == CHECK_CHANNEL ) {
				
				char wlan_domain[30] = {};
				int domain;
				char channel_list_24G[128];
				
				//
				channel_list_24G[0] = 0; // null string
    	
#define WIRELESS_BAND_24G   0
extern int GetDomainChannelList(unsigned short number, int band, char wlan_channel_list[], int show_used_space);
	
				get_wlan_domain ( wlan_domain );	

				domain = 0x00;
			   	sscanf(wlan_domain,"%x",&domain);
	
				GetDomainChannelList(domain,WIRELESS_BAND_24G,channel_list_24G,1);
	
				int channel;
				char *tmp;
				
				channel = atol(data);
dprintf("%d\n",channel);
				if( channel == atol(channel_list_24G)) {
					return 0;
				}
					
				tmp = strstr(channel_list_24G,",");
				while( tmp ) {
					tmp++;
					if( channel == atol(tmp)) {
						return 0;
					}
					tmp = strstr(tmp,",");					
				}
				//break;
				
			}
			else if( check_table[i].type == CHECK_MAC ) {
	
	
				char *mac;
				mac = data;
				dprintf(" %s\n",mac);
				if( strlen(mac) != 17) {
					return -1;
				}
				buff[0] = 0;
				strcpy(buff,mac);
				trim_string_char(buff,':');

				if( is_valid_hex_string(buff)) 
					return 0;
			}

			break;
		}
		
		i++;
	}
	
	

	return -1;
}



// lan info
static int sub_laninfo_checkset(struct node *n, char *data)
{
	char *field = n->nvram_field;
//	char wan_ip[192]={0}; // this is buff for other use
//	char wan_proto[16];


	if(war_strcasecmp(field, "DNSServers") == 0)	{
		// by pass
		return 0;	
	}


	else if(war_strcasecmp(field, "DHCPLeaseTime") == 0)	{
		
		long t;
		t = atol( data);
		// don't care
		return 0;

	}
	

	else if(war_strcasecmp(field, "ReservedAddresses") == 0)	{
		// by pass
			
		return 0;	
	}	


		
	
	
		

	return -1;
}


//char *txrate_11bgn[] = {"Best (automatic)","MCS 15 - 130 [270]", "MCS 14 - 117 [243]", "MCS 13 - 104 [216]", "MCS 12 - 78 [162]", "MCS 11 - 52 [108]", "MCS 10 - 39 [81]", "MCS 9 - 26 [54]", "MCS 8 - 13 [27]", "MCS 7 - 65 [135]", "MCS 6 - 58.5 [121.5]", "MCS 5 - 52 [108]", "MCS 4 - 39 [81]", "MCS 3 - 26 [54]", "MCS 2 - 19.5 [40.5]", "MCS 1 - 13 [27]", "MCS 0 - 6.5 [13.5]", "54", "48", "36", "24", "18", "12", "9", "6", "11", "5.5", "2", "1",NULL};

static int sub_wlaninfo_checkset(struct node *n, char *data)
{
	char *field = n->nvram_field;
//	char wan_ip[192]={0}; // this is buff for other use
//	char wan_proto[16];


	if(war_strcasecmp(field, "MaxBitRate") == 0)	{
		if( war_strcasecmp(data,"Auto") == 0) {
			return 0;
		}
		int i;
		int rate;
		rate = atol(data);
		i = 1;
		while( txrate_11bgn[i] != NULL) {
			if( memcmp(txrate_11bgn[i], "MCS", 3) == 0) {
				char *tmp;
				tmp = strstr( txrate_11bgn[i],"-");
				if( tmp ) {
					if( rate == atol(tmp+1)) {
						// width 20 
						return 0; // ok
					}
				}
				tmp = strstr( txrate_11bgn[i],"[");
				if( tmp ) {
					if( rate == atol(tmp+1)) {
						// width 40 or auto 
						return 0; // ok
					}
				}					
			}
			else {
				//
				if( rate == atol(txrate_11bgn[i]) ) {
					return 0;
				}
			}
			
			i++;
		}
		
	}
	else if( (war_strcasecmp(field, "wepkey1") == 0) ||
			 (war_strcasecmp(field, "wepkey2") == 0) ||
			 (war_strcasecmp(field, "wepkey3") == 0) ||
			 (war_strcasecmp(field, "wepkey4") == 0) ) {

		if( (strlen(data) == 10) ||
			(strlen(data) == 26) ) {
			return 0;
		}	
		
	}

	else if(war_strcasecmp(field, "BasicEncryptionModes") == 0)	{
		// don't check?
		return 0;
	}
	else if(war_strcasecmp(field, "BasicAuthenticationMode") == 0)	{
		return 0;
	}
	else if(war_strcasecmp(field, "WPAEncryptionModes") == 0)	{
		return 0;
	}	
	else if(war_strcasecmp(field, "WPAAuthenticationMode") == 0)	{
		return 0;
	}	
	
	return -1;
}




static int sub_waninfo_checkset(struct node *n, char *data)
{
	char *field = n->nvram_field;
//	char wan_ip[192]={0}; // this is buff for other use
//	char wan_proto[16];
	char buff[128];

	if(war_strcasecmp(field, "DNSServers") == 0)	{

		char *tmp;
		long ret;
		
		tmp = strstr(data,",");
		if( tmp == NULL) {
			// this is only ip
			return is_ip(data);			
		}	
		else {
			memcpy(buff,data,tmp-data);
			buff[tmp-data]=0;
			tmp++;
			ret = is_ip(buff);
dprintf("1->[%s]%ld\n",buff,ret);			
			if( ret != 0) 
				return -1;
			ret = is_ip(tmp);
dprintf("2->[%s]%ld\n",tmp,ret);			
			return ret;						
		}	

//wan_specify_dns 1
//wan_primary_dns
//wan_secondary_dns



	}
	else if( war_strcasecmp(field, "wan_proto") == 0 ) {
		// by pass
		return 0;
	}


	return -1;
	
}



// wlaninfo commit
void wlaninfo_commit(void) {

    node_t n;
    char tmp_path[256];
	
	sprintf(tmp_path,"InternetGatewayDevice.LANDevice.1.WLANConfiguration.1.BasicEncryptionModes");
	lib_resolve_node(tmp_path,&n);			
	
	if( war_strcasecmp(n->value,"WEPEncryption") == 0) {
		// WEP
		sprintf(tmp_path,"InternetGatewayDevice.LANDevice.1.WLANConfiguration.1.BasicAuthenticationMode");
		lib_resolve_node(tmp_path,&n);			
		
		dprintf("auth %s\n",n->value);
		// don't care auth, we set open
		
		sprintf(tmp_path,"InternetGatewayDevice.LANDevice.1.WLANConfiguration.1.WEPKey.1.WEPKey");
		lib_resolve_node(tmp_path,&n);			
		
		if( strlen(n->value) == 10) {
			// wep 64
			
			nvram_set("wlan0_security","wep_open_64");
		}
		else {
			// wep 128?
			nvram_set("wlan0_security","wep_open_128");
		}
		
		
	}
	else {
		// disable
		nvram_set("wlan0_security","disable");
	}
	


	dprintf(" wlan commit security: %s\n",nvram_safe_get("wlan0_security"));
}




struct nvram {
	char *command;
    int (*get)(struct node *n, char *);
    int (*set)(struct node *n);
    int (*checkset)(struct node *n,char *);
    
};
 
struct nvram nvram_table[] = {
	{"nvram",sub_nvram_get,      sub_nvram_set,     sub_nvram_checkset},
	{"nvramOUI",sub_nvramOUI_get,NULL,NULL},
	{"nvramMAC",sub_nvramMAC_get,NULL,NULL},
	{"version",sub_version_get,  NULL,NULL},
	{"waninfo",sub_waninfo_get,  sub_waninfo_set,	sub_waninfo_checkset},
	{"wanstat",sub_wanstat_get,  NULL,NULL},

	{"laninfo",sub_laninfo_get,  sub_laninfo_set,	sub_laninfo_checkset},
	{"lanstat",sub_lanstat_get,  NULL,NULL},

	{"wlaninfo",sub_wlaninfo_get,sub_wlaninfo_set,	sub_wlaninfo_checkset},
	{"wlanstat",sub_wlanstat_get,NULL,NULL},

	{"time",sub_time_get,        NULL,NULL},

	{"server",sub_server_get,    NULL,NULL},
	
	{NULL,NULL}
	};
 

// 2010.07.30
// need to check or set 
// DNSServers 
 
 
//static char myurl[256];
int acs_url_update_nvram=1; // need update acs_url from nvram

static void process_nvram_get(struct node *n)
{
// 2010.07.07
    char full_path[256];
    char *cmd;
    char *field;
    char update_value;
    
//    int locate[MAX_LOCATE_DEPTH];
	char data[256];

    get_param_full_name(n, full_path);

//	dprintf(" path=%s\n",full_path);
	if( acs_url_update_nvram ) {
		// update nvram
	}
	else
	if( war_strcasecmp(full_path,ACS_URL) == 0) {
		// check ACS_URL + not update ACS_URL
//		war_snprintf(n->value, sizeof(n->value), "%s", myurl);
		return;
	}
		
//	dprintf("path=%s\n",full_path);
//	dprintf(" field=%s\n",n->nvram_field);
//	dprintf("  cmd=%s\n",n->nvram_cmd);

	cmd = n->nvram_cmd;
	field = n->nvram_field;
	
	update_value = 0;

	int i;
	int find;

	i = 0;
	find = 0;
	while( nvram_table[i].command != NULL) {
		
		if( war_strcasecmp(cmd,nvram_table[i].command) == 0) {
			find = 1;	
			if( nvram_table[i].get ) {
				if( nvram_table[i].get(n,data) == 0) {
					update_value = 1;
				}
				//break;
			}
			break; // find, so we should break...
		}
		
		i++;
	}


	if( find == 0) {
		dprintf(" -->unknow command!!!\n");
	}






	if( update_value ) {
		war_snprintf(n->value, sizeof(n->value), "%s", data);
	}

//	dprintf("%s\n",VER_FIRMWARE);
	
}

static void xml_tag2node(struct xml *tag, struct node *n)
{
	int i;

		for(i = 0; i < tag->attr_count; i++) {
		if(war_strcasecmp(tag->attributes[i].attr_name, "name") == 0) {
			war_snprintf(n->name, sizeof(n->name), "%s", tag->attributes[i].attr_value);
		} else if(war_strcasecmp(tag->attributes[i].attr_name, "rw") == 0) {
			if(war_strcasecmp(tag->attributes[i].attr_value, "1") == 0 || war_strcasecmp(tag->attributes[i].attr_value, "true") == 0)
				n->rw = 1;
		} else if(war_strcasecmp(tag->attributes[i].attr_name, "getc") == 0) {
			if(war_strcasecmp(tag->attributes[i].attr_value, "1") == 0 || war_strcasecmp(tag->attributes[i].attr_value, "true") == 0)
				n->getc = 1;
		} else if(war_strcasecmp(tag->attributes[i].attr_name, "noc") == 0) {
			n->noc = atoi(tag->attributes[i].attr_value);
		} else if(war_strcasecmp(tag->attributes[i].attr_name, "nocc") == 0) {
			n->nocc = nocc_str2code(tag->attributes[i].attr_value);
		} else if(war_strcasecmp(tag->attributes[i].attr_name, "nin") == 0) {
			n->nin = atoi(tag->attributes[i].attr_value);
		} else if(war_strcasecmp(tag->attributes[i].attr_name, "il") == 0) {
			n->il = atoi(tag->attributes[i].attr_value);
		} else if(war_strcasecmp(tag->attributes[i].attr_name, "acl") == 0) {
			war_snprintf(n->acl, sizeof(n->acl), "%s", tag->attributes[i].attr_value ? xml_xmlstr2str(tag->attributes[i].attr_value, tag->attributes[i].attr_value) : "");
		} else if(war_strcasecmp(tag->attributes[i].attr_name, "type") == 0) {
		    //n->type = type_str2code(tag->attributes[i].attr_value);
		    war_snprintf(n->type, sizeof(n->type), "%s", tag->attributes[i].attr_value);

// 2010.07.07
		} else if(war_strcasecmp(tag->attributes[i].attr_name, "field") == 0) {
			war_snprintf(n->nvram_field, sizeof(n->nvram_field), "%s", tag->attributes[i].attr_value ? xml_xmlstr2str(tag->attributes[i].attr_value, tag->attributes[i].attr_value) : "");
		} else if(war_strcasecmp(tag->attributes[i].attr_name, "mode") == 0) {
			
			int j;
			j = 0;
			while( nvram_table[j].command != NULL) {
				if( war_strcasecmp(tag->attributes[i].attr_value,nvram_table[j].command) == 0) {
					n->nvram_mode = 1;				
					break;
				}
				j++;
			}

			
			if( n->nvram_mode == 1) {
				war_snprintf(n->nvram_cmd, sizeof(n->nvram_cmd), "%s", tag->attributes[i].attr_value ? xml_xmlstr2str(tag->attributes[i].attr_value, tag->attributes[i].attr_value) : "");				
			}
				

			
		#ifdef __V4_2
		} else if(war_strcasecmp(tag->attributes[i].attr_name, "add") == 0) {
		    n->dev.obj.add = dlsym(handle, tag->attributes[i].attr_value);
		} else if(war_strcasecmp(tag->attributes[i].attr_name, "del") == 0) {
		    n->dev.obj.del= dlsym(handle, tag->attributes[i].attr_value);
		} else if(war_strcasecmp(tag->attributes[i].attr_name, "get") == 0) {
		    n->dev.param.get= dlsym(handle, tag->attributes[i].attr_value);
		} else if(war_strcasecmp(tag->attributes[i].attr_name, "set") == 0) {
		    n->dev.param.set= dlsym(handle, tag->attributes[i].attr_value);
		#endif
		}
		#ifdef __V4_2
		if( dlerror() != NULL){
			tr_log(LOG_ERROR,"dlerror");
			dlclose(handle);
			exit(-1);
		}
		#endif
	}

	// if(tag->value)  //gspring: segmentation fault
	if(tag->value[0] != '\0')
		war_snprintf(n->value, sizeof(n->value), "%s", xml_xmlstr2str(tag->value, tag->value));


// print all path
    char full_path[256];

    get_param_full_name(n, full_path);

//	if( n->rw) {
//		dprintf("path=%s\n",full_path);
//		
//	}


// 2010.07.07
	if( n->nvram_mode == 1) {
		process_nvram_get(n);
	}


}

static struct node *last_child(struct node *parent)
{
	struct node *child = parent->children;

	while(child && child->brother)
		child = child->brother;

	return child;
}

static struct node * xml2tree(const char *file_path)
{
	struct node *root = NULL;
	struct node *cur = NULL;
	FILE *fp;
	char *buf = NULL;
	size_t len = 0;

#ifdef __V4_2
	char dlpath[PATH_LEN];
	tr_full_name("libdev.so", dlpath, sizeof(dlpath));
        handle = dlopen(dlpath, RTLD_LAZY);
	if(handle == NULL) {
	    exit(-1);
	}
#endif

	fp = tr_fopen(file_path, "r");
	if(fp == NULL) {
		char bak[256];
		war_snprintf(bak, sizeof(bak), "%s.bak", file_path);
		fp = tr_fopen(bak, "r");
	}

	if(fp) {
		fseek(fp, 0, SEEK_END);
		len = ftell(fp);
		fseek(fp, 0, SEEK_SET);

// consider u8 code issue
#if 1
int u8_issue;
int i;
		u8_issue = 0;
		unsigned char u8_data[3];
		fread(u8_data, 1, 3, fp);
		for(i=0;i<3;i++) {
			if( u8_data[i] > 0x80) 
				u8_issue++;			
		}		

		fseek(fp, 0, SEEK_SET);
		if( u8_issue ) {
			len -= u8_issue;
			fseek(fp, u8_issue, SEEK_SET);
		}
#endif



		buf = malloc(len + 1);
		if(buf == NULL) {
			tr_log(LOG_ERROR, "Out of memory!");
		} else {
			struct xml tag;
			char *left;

			fread(buf, 1, len, fp);
			buf[len] = '\0';

			left = buf;
			while(xml_next_tag(&left, &tag) == XML_OK) {
				if(war_strcasecmp(tag.name, "node") == 0) {
					if(root == NULL) {
						root = calloc(1, sizeof(*root));
						if(root == NULL) {
							tr_log(LOG_ERROR, "Out of memory!");
							break;
						} else {
							xml_tag2node(&tag, root);
						}

						cur = root;
					} else {
						struct node *n;
						struct node *brother;


						n = calloc(1, sizeof(*n));
						if(n == NULL) {
							tr_log(LOG_ERROR, "Out of memory!");
							break;
						} else {

// 2010.07.07 add parent to n
						n->parent = cur;
							xml_tag2node(&tag, n);
						}

						n->parent = cur;

						brother = last_child(n->parent);
						if(brother) {
							brother->brother = n;
						} else {
							cur->children = n;
						}

						cur = n;
					}
				} else if(war_strcasecmp(tag.name, "/node") == 0) {
					cur = cur->parent;
					if(cur == NULL)
						break;
				} else if(war_strcasecmp(tag.name, "?xml")) {
					tr_log(LOG_WARNING, "Invalid XML tag!");
					break;
				}
			}
		}
	}

	if(fp)
		fclose(fp);

	if(buf)
		free(buf);
#if 0 /*printf xml config file in console*/
	if(root) {
		int level = 0;
		__tree2xml(root, stdout, &level);
	}
#endif


	return root;
}

static char * nocc_table[] = {"", "0", "1", "2", "!0", "!1", "!2"};

static unsigned int nocc_str2code(const char *str)
{
	unsigned int i;

	for(i = sizeof(nocc_table) / sizeof(nocc_table[0]) - 1; i > 0; i--) {
		if(strcmp((char *)str, nocc_table[i]) == 0) {
			return i;
		}
	}

	return 0;
}

static const char *nocc_code2str(unsigned int code)
{
	if(code < sizeof(nocc_table) / sizeof(nocc_table[0]) && code >= 0)
		return nocc_table[code];
	else
		return nocc_table[0];
}



/*
static char * type_table[] = {"string", "int", "unsignedInt", "boolean", "dateTime", "base64", "node", "any"};

static unsigned int type_str2code(const char *str)
{
	int i;

	for(i = sizeof(type_table) / sizeof(type_table[0]) - 1; i >= 0; i--) {
		if(strcmp((char *)str, type_table[i]) == 0) {
			return i;
		}
	}

	tr_log(LOG_WARNING, "type of %s incorrect", str);
	return 0;
}

static const char *type_code2str(unsigned int code)
{
	if(code < sizeof(type_table) / sizeof(type_table[0]) && code >= 0)
		return type_table[code];
	else
		return type_table[0];
}
*/

static int __tree2xml(struct node *tree, FILE *fp, int *level)
{
#ifdef __V4_2
	int i;
	Dl_info info1, info2;
	info1.dli_fname = 0;
	info1.dli_sname = 0;
	info1.dli_fbase = 0;
	info1.dli_saddr = 0;
	info2.dli_fname = 0;
	info2.dli_sname = 0;
	info2.dli_fbase = 0;
	info2.dli_saddr = 0;
	if( !tree || !fp )
		return -1;
	for(i = *level; i > 0; i--)
		fprintf(fp, "    ");
	if(strcmp(tree->type, "node") == 0) {
		struct node *n;
		if(tree->rw) {
		    if(war_strcasecmp(tree->name, "template") == 0)
			    fprintf(fp, "<node name='template' rw='1' type='node'>\n");
		    else{
			    if( !tree->dev.obj.add || !tree->dev.obj.del ){
				    tr_log(LOG_NOTICE,"add del function pointer is null!");
				    fprintf(fp,"<node name='%s' rw='1' type='node'>\n'",tree->name);
			    }
			    else
			    {
			    if(dladdr(tree->dev.obj.add, &info1) == 0||info1.dli_saddr != tree->dev.obj.add) {
				    tr_log(LOG_ERROR, "Resole add() function name failed!");
			    } else if(dladdr(tree->dev.obj.del, &info2) == 0 || info2.dli_saddr != tree->dev.obj.del) {
				    tr_log(LOG_ERROR, "Resole del() function name failed!");
			    } else {
				    fprintf(fp, "<node name='%s' rw='1' nin='%d' il='%d' type='node' add='%s' del='%s'>\n", tree->name, tree->nin, tree->il, info1.dli_sname, info2.dli_sname);
			    }
				}
		    }
		} else {
		    fprintf(fp, "<node name='%s' rw='0' type='node'>\n", tree->name);
		}

		(*level)++;
		for(n = tree->children; n; n = n->brother) {
			__tree2xml(n, fp, level);
		}

		(*level)--;
		for(i = *level; i > 0; i--)
			fprintf(fp, "    ");
		fprintf(fp, "</node>\n");
	} else {
		if(dladdr((void*)tree->dev.param.get, &info1) && dladdr((void*)tree->dev.param.set, &info2)) {                // info1.dli_saddr != tree->dev.param.get) 
	   		 fprintf(fp, "<node name='%s' rw='%d' getc='%d' noc='%d' nocc='%s' acl='%s' type='%s' get='%s' set='%s'>%s</node>\n",
		    		tree->name, tree->rw, tree->getc, tree->noc, nocc_code2str(tree->nocc), tree->acl, tree->type, info1.dli_sname, info2.dli_sname,tree->value);
		} else {
	    		fprintf(fp, "<node name='%s' rw='%d' getc='%d' noc='%d' nocc='%s' acl='%s' type='%s' get='%s'>%s</node>\n",
				tree->name, tree->rw, tree->getc, tree->noc, nocc_code2str(tree->nocc), tree->acl, tree->type, info1.dli_sname,tree->value);
		}
	}

	return 0;
#else
	int i;

	for(i = *level; i > 0; i--)
		fprintf(fp, "    ");

	//if(tree->type == TYPE_NODE) {
	if(strcmp(tree->type, "node") == 0) {
		struct node *n;

		if(tree->rw)
			fprintf(fp, "<node name='%s' rw='1' nin='%d' il='%d' type='node'>\n", tree->name, tree->nin, tree->il);
		else
			fprintf(fp, "<node name='%s' rw='0' type='node'>\n", tree->name);

		(*level)++;
		for(n = tree->children; n; n = n->brother) {
			__tree2xml(n, fp, level);
		}

		(*level)--;
		for(i = *level; i > 0; i--)
			fprintf(fp, "    ");
		fprintf(fp, "</node>\n");
	} else {

// 2010.07.07
		if( tree->nvram_mode == 1) {
		fprintf(fp, "<node name='%s' rw='%d' getc='%d' noc='%d' nocc='%s' acl='%s' type='%s' mode='%s' field='%s'>%s</node>\n",
				tree->name, tree->rw, tree->getc, tree->noc, nocc_code2str(tree->nocc), tree->acl, tree->type, tree->nvram_cmd,tree->nvram_field,tree->value);
		}
		else {		
		fprintf(fp, "<node name='%s' rw='%d' getc='%d' noc='%d' nocc='%s' acl='%s' type='%s'>%s</node>\n",
				tree->name, tree->rw, tree->getc, tree->noc, nocc_code2str(tree->nocc), tree->acl, tree->type, tree->value);
				//tree->name, tree->rw, tree->getc, tree->noc, nocc_code2str(tree->nocc), tree->acl, type_code2str(tree->type), tree->value);
		}
		
//
		if( tree->nvram_changed	) {
			dprintf(" %s %s\n",tree->name,tree->value);
			// try write to nvram & other 

			int i;
			int find;

			i = 0;
			find = 0;
			while( nvram_table[i].command != NULL) {
		
				if( war_strcasecmp(tree->nvram_cmd,nvram_table[i].command) == 0) {
					find = 1;	
					if( nvram_table[i].set ) {
						if( nvram_table[i].set(tree) == 0) {
							//update_value = 1;
							nvram_need_commit = 1;
						}
						break;
					}
				}
		
				i++;
			}


			if( find == 0) {
				dprintf(" -->unknow command!!!\n");
			}

			
			
			tree->nvram_changed = 0; // free it
		}			
		
		
		
	}

	return 0;
#endif
}

static int tree2xml(struct node *tree, const char *file_path)
{
	FILE *fp;
	int res;
	int level = 0;

	if(factory_resetted) {
		/*
		 * Factory reset the device. In this sample, we just replace the xml file
		 * with the default xml file.
		 */
		char bak[512];
		FILE *src, *dst;

		war_snprintf(bak, sizeof(bak), "%s.bak", file_path);
		dst = tr_fopen(file_path, "w");
		src = tr_fopen(bak, "r");

		if(dst && src) {
			int len;

			while((len = fread(bak, 1, sizeof(bak), src)) > 0) {
				if(fwrite(bak, 1, len, dst) != len) {
					tr_log(LOG_ERROR, "Write xml file failed: %s", war_strerror(war_geterror()));
					break;
				}
			}
		} else {
			tr_log(LOG_ERROR, "fopen xml_file fail: %s", war_strerror(war_geterror()));
		}

		if(dst)
			fclose(dst);
		if(src)
			fclose(src);

		factory_resetted = 0;
		return 0;
	}

	fp = tr_fopen(file_path, "w");
	if(fp == NULL)
		return -1;

	fprintf(fp, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
	res = __tree2xml(tree, fp, &level);
	fflush(fp);
	fclose(fp);

	return res;
}




#if 0
// static
int updatexml(struct node *tree, char *target,char *value)
{

// 
//	char tmp[256];


	char *path;
	char *find;
	int len;
	char name[256];	

	struct node *n;

		
	path = target;



	while( tree ) {

		find = strchr(path,'.');

//		dprintf("1 %s\n",find);

		if( find) {
			int match;
			len = find-path;
		
			memcpy(name,path,len);
			name[len] = 0;

//			dprintf("2 %s\n",name);

			match = 0;
			for(n=tree; n; n = n->brother) {
				if(strcmp(n->type, "node") == 0) {
					if( strcasecmp(n->name,name) == 0) {
						tree = n->children;
						match = 1;
						break;
					}
				}
		
			}

			// cannot find
			if( match) {
				path = find+1; // find next
				//tree = tree->children;			
			}
			else {
				return -1;
			}
		}
		else {
			// find value?
//			dprintf("3 %s\n",path);

			for(n=tree; n; n = n->brother) {
				if(strcmp(n->type, "node") != 0) {
					if( strcasecmp(n->name,path) == 0) {
					
						//dprintf("%s\n",n->value);
						war_snprintf(n->value, sizeof(n->value), "%s", value);

						return 0;	
						break;
					}
				}
		
			}


			return -1;		
		}



	}

	return -1;



}
#endif


// update 
static int extra_porting_mapping_init(char *interface)
{
	
// create port mapping	
    node_t node;
    char root_path[256] = "InternetGatewayDevice.WANDevice.1.WANConnectionDevice.1.WANIPConnection.1.PortMapping";
    char tmp_path[256];
    int i;
	int update_count;

	strcpy(root_path,"InternetGatewayDevice.WANDevice.1.WANConnectionDevice.1.");
	strcat(root_path,interface);
	strcat(root_path,".1.PortMapping");

    node_t root_node;

	char fw_value[192]={0};
	char buf[16];
    
	char *enable="", *name="", *protocol_type="", *pub_port="", *pri_port="", *ip="", *sch="", *inbound="";

	update_count = 0;
	
	sprintf(tmp_path,"%s",root_path);
	lib_resolve_node(tmp_path,&root_node);
//	dprintf("%s\n",tmp_path);
//	exit(1);
	if( root_node ) {


		// virtual server

		for(i=0;i<24;i++) {
						
			sprintf(buf, "vs_rule_%02d",i);
			strncpy(fw_value, nvram_safe_get(buf), sizeof(fw_value));
			if (strlen(fw_value) == 0) 
				continue;

			sprintf(tmp_path,"%s.%d",root_path,i+1);
			// find exist???
			lib_resolve_node(tmp_path,&node);


			if (!strcmp(fw_value, "0//6/0/0/0.0.0.0/Always/Allow_All")) {
				if( node) {
					lib_do(node);				
				}
			   	continue;
			}

			getStrtok(fw_value, "/", "%s %s %s %s %s %s %s %s", &enable, &name, &protocol_type, &pub_port \
				, &pri_port, &ip, &sch, &inbound);

			if( !node) {
				lib_ao(root_node,i+1);
			}			
		
			// read again
			lib_resolve_node(tmp_path,&node);
			if( node) {
				node_t n;

				// update value
				// enable
				sprintf(tmp_path,"%s.%d.PortMappingEnabled",root_path,i+1);
				lib_resolve_node(tmp_path,&n);			
				war_snprintf(n->value, sizeof(n->value), "%s", enable);

				// protocol
				sprintf(tmp_path,"%s.%d.PortMappingProtocol",root_path,i+1);
				lib_resolve_node(tmp_path,&n);			

		  		if (strcmp(protocol_type, "6") == 0){
					war_snprintf(n->value, sizeof(n->value), "TCP");
		  		}else if (strcmp(protocol_type, "17") == 0){
					war_snprintf(n->value, sizeof(n->value), "UDP");
		  		}else if (strcmp(protocol_type, "256") == 0){
					war_snprintf(n->value, sizeof(n->value), "TCP,UDP");		  			
				}else{
					war_snprintf(n->value, sizeof(n->value), "%s",protocol_type);
				}

				// PortMappingLeaseDuration
				sprintf(tmp_path,"%s.%d.PortMappingLeaseDuration",root_path,i+1);
				lib_resolve_node(tmp_path,&n);			
				war_snprintf(n->value, sizeof(n->value), "%d",0);

				// RemoteHost
				sprintf(tmp_path,"%s.%d.RemoteHost",root_path,i+1);
				lib_resolve_node(tmp_path,&n);			
				war_snprintf(n->value, sizeof(n->value), "");

				
				//ExternalPort
				sprintf(tmp_path,"%s.%d.ExternalPort",root_path,i+1);
				lib_resolve_node(tmp_path,&n);			
				war_snprintf(n->value, sizeof(n->value), "%s",pub_port);
				
				//ExternalPortEndRange
				sprintf(tmp_path,"%s.%d.ExternalPortEndRange",root_path,i+1);
				lib_resolve_node(tmp_path,&n);			
				war_snprintf(n->value, sizeof(n->value), "0");

				//InternalPort
				sprintf(tmp_path,"%s.%d.InternalPort",root_path,i+1);
				lib_resolve_node(tmp_path,&n);			
				war_snprintf(n->value, sizeof(n->value), "%s",pri_port);
				
				//InternalClient
				sprintf(tmp_path,"%s.%d.InternalClient",root_path,i+1);
				lib_resolve_node(tmp_path,&n);			
				war_snprintf(n->value, sizeof(n->value), "%s",ip);

				//PortMappingDescription
				sprintf(tmp_path,"%s.%d.PortMappingDescription",root_path,i+1);
				lib_resolve_node(tmp_path,&n);			
				war_snprintf(n->value, sizeof(n->value), "%s %s %s",name,sch,inbound);

				update_count++;
			}
		
		
		}
	
	}
	
	// maybe update nin or il ?	

	// update PortMappingNumberOfEntries
	
	sprintf(tmp_path,"InternetGatewayDevice.WANDevice.1.WANConnectionDevice.1.%s.1.PortMappingNumberOfEntries",interface);
	lib_resolve_node(tmp_path,&root_node);
	
	if( root_node) {
		war_snprintf(root_node->value, sizeof(root_node->value), "%d",update_count);
		
	}





	
		
	return 0;	
}





//Parameter: InternetGatewayDevice.Layer3Forwarding.Forwarding.1.ForwardingMetric
//Parameter: InternetGatewayDevice.Layer3Forwarding.Forwarding.1.Interface
//Parameter: InternetGatewayDevice.Layer3Forwarding.Forwarding.1.GatewayIPAddress
//Parameter: InternetGatewayDevice.Layer3Forwarding.Forwarding.1.ForwardingPolicy
//Parameter: InternetGatewayDevice.Layer3Forwarding.Forwarding.1.SourceSubnetMask
//Parameter: InternetGatewayDevice.Layer3Forwarding.Forwarding.1.SourceIPAddress
//Parameter: InternetGatewayDevice.Layer3Forwarding.Forwarding.1.DestSubnetMask
//Parameter: InternetGatewayDevice.Layer3Forwarding.Forwarding.1.DestIPAddress
//Parameter: InternetGatewayDevice.Layer3Forwarding.Forwarding.1.Type
//Parameter: InternetGatewayDevice.Layer3Forwarding.Forwarding.1.StaticRoute
//Parameter: InternetGatewayDevice.Layer3Forwarding.Forwarding.1.Status
//Parameter: InternetGatewayDevice.Layer3Forwarding.Forwarding.1.Enable
//Parameter: InternetGatewayDevice.Layer3Forwarding.ForwardNumberOfEntries
//Parameter: InternetGatewayDevice.Layer3Forwarding.DefaultConnectionService

static int extra_routing_table_init(void)
{
    char root_path[256] = "InternetGatewayDevice.Layer3Forwarding.Forwarding";
    char tmp_path[256];
//    int i;
//	int update_count;

    node_t root_node;
    node_t node;

	routing_table_t *routing_table_list;
	routing_table_list = read_route_table();
	
	int i;
	int count;
	
	count = 0;
	sprintf(tmp_path,"%s",root_path);
	lib_resolve_node(tmp_path,&root_node);
	if( root_node ) {
		for(i=0; strlen(routing_table_list[i].dest_addr) >0; i++ ){


			sprintf(tmp_path,"%s.%d",root_path,i+1);
			// find exist???
			lib_resolve_node(tmp_path,&node);

			if( !node) {
				lib_ao(root_node,i+1);
			}					

			// read again
			lib_resolve_node(tmp_path,&node);
			if( node) {
			    node_t n;

				sprintf(tmp_path,"%s.%d.Enable",root_path,i+1);
				lib_resolve_node(tmp_path,&n);			
				war_snprintf(n->value, sizeof(n->value), "%s","true");

				sprintf(tmp_path,"%s.%d.GatewayIPAddress",root_path,i+1);
				lib_resolve_node(tmp_path,&n);			
				war_snprintf(n->value, sizeof(n->value), "%s",routing_table_list[i].gateway ? routing_table_list[i].gateway : "");

				sprintf(tmp_path,"%s.%d.DestIPAddress",root_path,i+1);
				lib_resolve_node(tmp_path,&n);			
				war_snprintf(n->value, sizeof(n->value), "%s",routing_table_list[i].dest_addr ? routing_table_list[i].dest_addr : "");

				sprintf(tmp_path,"%s.%d.DestSubnetMask",root_path,i+1);
				lib_resolve_node(tmp_path,&n);			
				war_snprintf(n->value, sizeof(n->value), "%s",routing_table_list[i].dest_mask ? routing_table_list[i].dest_mask : "");

				sprintf(tmp_path,"%s.%d.ForwardingMetric",root_path,i+1);
				lib_resolve_node(tmp_path,&n);			
				war_snprintf(n->value, sizeof(n->value), "%d",routing_table_list[i].metric);


				sprintf(tmp_path,"%s.%d.Interface",root_path,i+1);
				lib_resolve_node(tmp_path,&n);			
				
				char interface[16];
				
				strcpy(interface,"WAN");
				
				
//	wan_eth
				if( strcasecmp(routing_table_list[i].interface ? routing_table_list[i].interface : "",nvram_safe_get("lan_bridge")) == 0) {
					strcpy(interface,"LAN");				
				}
				else if( strcasecmp(routing_table_list[i].interface ? routing_table_list[i].interface : "","lo") == 0) {
					strcpy(interface,"Local");				
				}
				
				
				
				war_snprintf(n->value, sizeof(n->value), "%s",interface);
				




				sprintf(tmp_path,"%s.%d.Status",root_path,i+1);
				lib_resolve_node(tmp_path,&n);							
				war_snprintf(n->value, sizeof(n->value), "%s","Enabled");

				sprintf(tmp_path,"%s.%d.StaticRoute",root_path,i+1);
				lib_resolve_node(tmp_path,&n);							
				war_snprintf(n->value, sizeof(n->value), "%s","false");

				sprintf(tmp_path,"%s.%d.ForwardingPolicy",root_path,i+1);
				lib_resolve_node(tmp_path,&n);							
				war_snprintf(n->value, sizeof(n->value), "%s","-1");


				sprintf(tmp_path,"%s.%d.SourceIPAddress",root_path,i+1);
				lib_resolve_node(tmp_path,&n);							
				war_snprintf(n->value, sizeof(n->value), "%s","0.0.0.0");

				sprintf(tmp_path,"%s.%d.SourceSubnetMask",root_path,i+1);
				lib_resolve_node(tmp_path,&n);							
				war_snprintf(n->value, sizeof(n->value), "%s","0.0.0.0");

				sprintf(tmp_path,"%s.%d.Type",root_path,i+1);
				lib_resolve_node(tmp_path,&n);							
				war_snprintf(n->value, sizeof(n->value), "%s","Network");
				
			}
	
			count++;		
		}	
		
	}


	// update ForwardNumberOfEntries
	
	sprintf(tmp_path,"InternetGatewayDevice.Layer3Forwarding.ForwardNumberOfEntries");
	lib_resolve_node(tmp_path,&root_node);
	
	if( root_node) {
		war_snprintf(root_node->value, sizeof(root_node->value), "%d",count);
		
	}


	
	return 0;
}	

#if 1
static int extra_acs_dhcp_issue(int v4)
{
    	char tmp_path[256];
	    node_t root_node;
	char my_url[128];	
	memset(my_url,0,128);
	
	FILE *fp=NULL;

	if( v4 == 4) {
		fp = fopen("/var/tmp/acs.txt", "r");
	}
	else {
		fp = fopen("/var/tmp/acs6.txt", "r");
	}

#if 1
	if( fp ) {
	    //fread(my_url, 128, 1, fp);
	    fgets(my_url,128,fp);
printf("---[%s]\n",my_url);	    
printf("---[%ld]\n",strlen(my_url));	    
	    
		

		sprintf(tmp_path,ACS_URL);
		lib_resolve_node(tmp_path,&root_node);
//printf("---[%s]\n",tmp_path);	    
	
		if( root_node) {
//printf("---2 [%s]\n",root_node->value);	    
//printf("---3 [%s]\n",myurl);	    
			//war_snprintf(root_node->value, sizeof(root_node->value), "%s",my_url);
			//strcpy(root_node->value,my_url);

			
			char *s;
			s = my_url;
			while(*s) {
				if( *s == '\n') { 
					*s = 0;
					break;
				}
				s++;
			}
printf("---[%s]\n",my_url);	    
printf("---[%ld]\n",strlen(my_url));	    

			strcpy(root_node->value,"http://60.250.64.86:80/comserver/node1/tr069");
			war_snprintf(root_node->value, sizeof(root_node->value), "%s",my_url);

			acs_url_update_nvram = 0; // don't update ACS_URL from nvram

//printf("---4 [%s]\n",root_node->value);	    
printf("---4 [%ld]\n",strlen(root_node->value));	    
	    goto end;
		
		}
		
		
	}
#endif	
end:	
	if( fp) 
		fclose(fp);

	
	return 0;
}
#endif

static int extra_init(void)
{

	extra_porting_mapping_init("WANIPConnection");
	extra_porting_mapping_init("WANPPPConnection");
	
	
	extra_routing_table_init();
	
	extra_acs_dhcp_issue(4);
	extra_acs_dhcp_issue(6);
	
	return 0;

}

#if 0



int try()
{
	
	char mac[]="00:03:21:55:56:g1";
	char buff[128];	
	
	dprintf("%ld\n",strlen(mac));
	
	if( strlen(mac) != 17) {
		return -1;
	}

	strcpy(buff,mac);
	trim_string_char(buff,':');

	dprintf("%d\n",is_valid_hex_string(buff));	

	
	
	exit(0);

	
}
#endif

TR_LIB_API int lib_init(const char *arg)
{

//	try();


	
	war_snprintf(xml_file_path, sizeof(xml_file_path), "%s", arg);
	if((root = xml2tree(arg)) != NULL) {
		// create ok.
		// but we need update some data
		extra_init();			
		
		return 0;
	}
	else
		return -1;
}

TR_LIB_API int lib_start_session(void)
{
	//Maybe need to lock the MO Tree and/or open the database
	return ++count;
}

TR_LIB_API int lib_end_session(void)
{
	//Maybe need to unlock the MO Tree and/or close the database
	if(count > 0)
		count--;

	if(count == 0) {
		change = 0;
		tree2xml(root, xml_file_path);
	}

	return 0;
}

TR_LIB_API int lib_factory_reset(void)
{
	tr_log(LOG_NOTICE, "Fctory reset");
	dprintf("lib_factory_reset\n");
	factory_resetted = 1;
	//tr_remove(xml_file_path);

    char cmd[40];
    memset(cmd,0,sizeof(cmd));
    fprintf(stderr,"restore_default\n");
    sprintf(cmd,"nvram restore_default &" );
    fprintf(stderr,"%s",cmd);
    _system(cmd);
	
	
	return 0;
}

TR_LIB_API int lib_reboot(void)
{
	dprintf("lib_reboot\n");
	tr_log(LOG_NOTICE, "Reboot system");
	
//	_system("mkdir /etc/tr");
//	_system("cp /home/tr/conf/events /etc/tr/events -f");

//	_system("cp /home/tr/conf/tr.xml /etc/tr/tr.xml -f");
	
#define sys_reboot()      _system("reboot -d2 &")
	sys_reboot();
	
	return 0;
}

#ifdef TR196
TR_LIB_API int lib_get_parent_node(node_t child, node_t *parent)
{
	*parent = child->parent;
	return 0;
}
#endif

TR_LIB_API int lib_get_child_node(node_t parent, const char *name, node_t *child)
{
	*child = NULL;

	if(name == NULL) {
		*child = parent->children;
		return 0;
	} else {
		node_t n;

		for(n = parent->children; n; n = n->brother) {
			if(strcmp(n->name, name) == 0) {
				*child = n;
				return 0;
			}
		}

		return -1;
	}		
}

static node_t resolve_node(char *path, node_t from)
{
	char *dot;
	node_t n;

	dot = strchr(path, '.');
	if(dot) {
		*dot = '\0';
		dot++;
	}
	if (from) {//bug 1118
	    for(n = from; n; n = n->brother) {
		if(strcmp(n->name, path) == 0) {
		    if(dot == NULL || *dot == '\0')
			return n;
		    else
			return resolve_node(dot, n->children);
		}
	    }
	}
	return NULL;
}

TR_LIB_API int lib_resolve_node(const char *path, node_t *node)
{
	if(path[0] == '\0') {
		*node = root;
		return 0;
	} else {
		char _path[256];
		war_snprintf(_path, sizeof(_path), "%s", path);

		*node = resolve_node(_path, root);

		if(*node)
			return 0;
		else
			return 1;
	}
}


TR_LIB_API int lib_get_property(node_t node, const char *name, char prop[PROPERTY_LENGTH])
{
	if(war_strcasecmp(name, "rw") == 0) {
		war_snprintf(prop, PROPERTY_LENGTH, "%d", node->rw);
	} else if(war_strcasecmp(name, "getc") == 0) {
		war_snprintf(prop, PROPERTY_LENGTH, "%d", node->getc);
	} else if(war_strcasecmp(name, "nin") == 0) {
		war_snprintf(prop, PROPERTY_LENGTH, "%d", node->nin);
	} else if(war_strcasecmp(name, "il") == 0) {
		war_snprintf(prop, PROPERTY_LENGTH, "%d", node->il);
	} else if(war_strcasecmp(name, "acl") == 0) {
		war_snprintf(prop, PROPERTY_LENGTH, "%s", node->acl);
	} else if(war_strcasecmp(name, "type") == 0) {
		war_snprintf(prop, PROPERTY_LENGTH, "%s", node->type);
	} else if(war_strcasecmp(name, "noc") == 0) {
		war_snprintf(prop, PROPERTY_LENGTH, "%d", node->noc);
	} else if(war_strcasecmp(name, "nocc") == 0) {
		war_snprintf(prop, PROPERTY_LENGTH, "%s", nocc_code2str(node->nocc));
	} else if(war_strcasecmp(name, "name") == 0) {
		war_snprintf(prop, PROPERTY_LENGTH, "%s", node->name);
	} else {
		return -1;
	}

	return 0;
}

TR_LIB_API int lib_get_value(node_t node, char **value)
{
	int len;

// get again
	if( node->nvram_mode == 1) {
		process_nvram_get(node);
	}

dprintf("GET value=%s [%s]\n",node->name,node->value);

	len = strlen(node->value);

	*value = malloc(len + 1);
	if(*value == NULL) {
		tr_log(LOG_ERROR, "Out of memory!");
		return -1;
	}

	war_snprintf(*value, len + 1, "%s", node->value);

	return 0;
}

TR_LIB_API void lib_destroy_value(char *value)
{
	if(value)
		free(value);
}

static  void free_tree(struct node *tree)
{
    if (tree) {

        if(tree->children) {
            free_tree(tree->children);
            tree->children = NULL;
        }

        if(tree->brother) {
            free_tree(tree->brother);
            tree->brother = NULL;
        }

        free(tree);
    }
}

static struct node *duplicate_tree(struct node *node)
{
        int len;
        int error = 0;
        struct node *to = NULL;
        struct node *from = NULL;
        struct node *tmp = NULL;

        len = sizeof(struct node); 
        to = malloc(len);
        if(to == NULL){
	    tr_log(LOG_ERROR, "Out memory!");
        } else {
                memcpy(to, node, len);
		to->parent = NULL;
                to->brother = NULL;
                to->children = NULL;

                for (from = node->children; from; from = from->brother) {
		    tmp = duplicate_tree(from);
                    if (tmp == NULL) {
			error = 1;
			break;
		    }
		    tmp->brother = to->children;
		    to->children = tmp;
		    tmp->parent = to;
		}
        }
        if(error == 1){
                free_tree(to);
                to = NULL;
        }
        return to;
}

node_t lib_get_child(node_t parent, char *name)
{
    node_t cur, next;

    for (cur = parent->children; cur; cur = next) {
        if (strcmp(cur->name, name) == 0) {
	    break;
	}
        next = cur->brother;
    }
    return cur?cur:NULL;
}

TR_LIB_API int lib_ao(node_t parent, int nin)
{

    int res = 0;
    node_t node0;
    node_t to;
#ifdef __V4_2
    char full_path[256];
    int locate[MAX_LOCATE_DEPTH];

    get_param_full_name(parent, full_path);
    path_2_locate(full_path, locate, sizeof(locate)/sizeof(locate[0]));
    if( parent->dev.obj.add )
	    res = parent->dev.obj.add(locate, sizeof(locate)/sizeof(locate[0]), nin);
    else
	   return -1;
#endif

    node0 = lib_get_child(parent, "template");
    if (node0) {//template
	to = duplicate_tree(node0);        
	if (to) {
	    war_snprintf(to->name, sizeof(to->name), "%d", nin);
	    to->brother = parent->children;
	    parent->children = to;
	    to->parent = parent;
	    res = 0;
	}
    }
    return res;

}


static void lib_destroy_tree(node_t node)
{
	node_t child;

	for(;node->children;) {
		child = node->children;
		node->children = child->brother;

		lib_destroy_tree(child);
	}

	free(node);
}

TR_LIB_API int lib_do(node_t node)
{
    int res = 0;
#ifdef __V4_2
    int ins_num ;
    char full_path[256];
    char *ins = NULL;
    int locate[MAX_LOCATE_DEPTH];
    get_param_full_name(node, full_path);
    ins = strrchr(full_path, '.');
    if(ins == NULL)
        return -1;

    ins_num = atoi(ins + 1);
    *(ins + 1)= '\0';

    path_2_locate(full_path, locate, sizeof(locate)/sizeof(locate[0]));
    if( node->parent->dev.obj.del)
	    res = node->parent->dev.obj.del(locate, sizeof(locate)/sizeof(locate[0]), ins_num);
    else
	    res = -1;
#endif
	if(node->parent) {
		node_t prev;

		if(node->parent->children == node) {
			node->parent->children = node->brother;
			change = 1;
		} else {
			for(prev = node->parent->children; prev; prev = prev->brother) {
				if(prev->brother == node) {
					prev->brother = node->brother;
					change = 1;
					break;
				}
			}
		}

		if(change) {
			node->brother = NULL;
			lib_destroy_tree(node);
		}
	} else {
		return -1;
	}

    return res;
/*
#else
	if(node->parent) {
		node_t prev;

		if(node->parent->children == node) {
			node->parent->children = node->brother;
			change = 1;
		} else {
			for(prev = node->parent->children; prev; prev = prev->brother) {
				if(prev->brother == node) {
					prev->brother = node->brother;
					change = 1;
					break;
				}
			}
		}

		if(change) {
			node->brother = NULL;
			lib_destroy_tree(node);
		}
	} else {
		return -1;
	}

	return 0;
#endif
*/
}

TR_LIB_API int lib_set_property(node_t node, const char *name, const char prop[PROPERTY_LENGTH])
{
	if(war_strcasecmp(name, "noc") == 0) {
		unsigned int tmp;

		tmp = atoi(prop);
		if(node->noc != tmp) {
			node->noc = tmp;
			change = 1;
		}
	} else if(war_strcasecmp(name, "acl") == 0) {
		if(strcmp(node->acl, prop)) {
			tr_log(LOG_ERROR, "New acl: %s", prop);
			war_snprintf(node->acl, sizeof(node->acl), "%s", prop);
			change = 1;
		}
	} else if(war_strcasecmp(name, "nin") == 0) {
		node->nin = atoi(prop);
		change = 1;
	} else {
		return -1;
	}

	return 0;
}

TR_LIB_API int lib_set_value(node_t node, const char *value)
{
#ifdef __V4_2
    //transfer node to device API todo
    char full_path[256];
    int res = 0;
    int locate[MAX_LOCATE_DEPTH];
    int  value_type = 0;

    get_param_full_name(node, full_path);
    path_2_locate(full_path, locate, sizeof(locate)/sizeof(locate[0]));
    if (strcmp(value, node->value)) {
	    if (war_strcasecmp(full_path, PARAMETERKEY) != 0) {
		    if( node->dev.param.set ) {
			    res = node->dev.param.set(locate, sizeof(locate)/sizeof(locate[0]), (char *)value, strlen(value), value_type);
			    if (res != -1)
				res = 0;
		    } else
			    return -1;
	    }
	    tr_log(LOG_NOTICE,"set value:%s",value);    
	    war_snprintf(node->value,sizeof(node->value),"%s",value);
	    tr_log(LOG_NOTICE,"set node value:%s",node->value);
	    change = 1;
    }
    return res;
#else




    if(strcmp(value, node->value)) {
    	
//  value check
		if( node->nvram_mode ) {
			int i;
			i = 0;
			while( nvram_table[i].command != NULL) {
		
				if( war_strcasecmp(node->nvram_cmd,nvram_table[i].command) == 0) {
					//find = 1;	
					if( nvram_table[i].checkset ) {
						if( nvram_table[i].checkset(node,(char *)value) == -1) {
							//update_value = 1;
							printf("set value fail %s [%s]\n",node->name,value);
							return -1;
						}
						break;
					}
				}
		
				i++;
			}
		}
    	
    	
    	
    	
	    war_snprintf(node->value, sizeof(node->value), "%s", value);
	    change = 1;
	    
// 2010.07.07
		if( node->nvram_mode == 1) {
			node->nvram_changed = 1;
		}	    


	    
    }

	dprintf("set value=%s [%s]\n",node->name,node->value);


#endif
    return 0;
}


TR_LIB_API const char *lib_current_time()
{
    static char str_time[32] = "";
    /*	char buf[20];

	struct tm *tm;
	time_t t, tz;
	char minus;

	war_time(&t);


	tm = war_gmtime(&t);
	tz = war_mktime(tm);
	tm = war_localtime(&t);
	t = war_mktime(tm);

	tz = t - tz;

	war_strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%S", tm);

	if(tz < 0) {
	minus = '-';
	tz = -tz;
	} else {
		minus = '+';
	}

	war_snprintf(cur, sizeof(cur), "%s%c%02d:%02d", buf, minus, (int)(tz / 3600), (int)((tz / 60) % 60));
*/
    struct tm *tp;
    char *format = "%Y-%m-%dT%H:%M:%S";
    int minus_value;
    int local_hour, local_min, local_yday;
    time_t t;

    //for WINCE 1109 inform time CurrentTime>2009-11-03T15:57:12-08:00</CurrentTime> format
    war_time(&t);
    tp = war_localtime(&t);
    if (war_strftime(str_time, 20, format, tp) == 0) {
        tr_log(LOG_ERROR, "Don't copy any string to buffer");
        return str_time;
    }

    local_hour = tp->tm_hour;
    local_min = tp->tm_min;
    local_yday = tp->tm_yday;

    tp = war_gmtime(&t);
    minus_value = (local_hour * 60 + local_min) - (tp->tm_hour * 60 + tp->tm_min);
    if(tp->tm_yday > local_yday) {
        minus_value -= 24 * 60;
    } else if (tp->tm_yday < local_yday) {
        minus_value += 24 * 60;
    }

    if(minus_value > 0) {
        war_snprintf(str_time + strlen(str_time), 7, "+%02d:%02d", minus_value / 60, minus_value % 60);
    } else if (minus_value < 0) {
        minus_value = abs(minus_value);
        war_snprintf(str_time + strlen(str_time), 7, "-%02d:%02d", minus_value / 60, minus_value % 60);
    }

	return str_time;
}


TR_LIB_API char * lib_node2path(node_t node)
{
	static char path[256];
	int index;
	int len;

	memset(path, 0, sizeof(path));


	for(index = sizeof(path) - 1; node; node = node->parent) {
		//if(node->type == TYPE_NODE) {
		if(strcmp(node->type , "node") == 0) {
			path[--index] = '.';
		}

		len = strlen(node->name);
		if(index >= len) {
			memcpy(path + index - len, node->name, len);
			index -= len;
		} else {
			return NULL;
		}
	}

	return path + index;
}

TR_LIB_API int lib_get_children(node_t node, node_t **children)
{
	int count = 0;
	node_t c;

	for(c = node->children; c; c = c->brother) {
	    if (strcmp(c->name, "template") != 0)
		count++;
	}
	if(count > 0) {
		int i;

		*children = calloc(count, sizeof(node_t));
		if(*children == NULL)
			return -1;

		for(i = 0, c = node->children; c; c = c->brother) {
		    if (strcmp(c->name, "template") != 0)
			(*children)[i++] = c;
		}
	}

	return count;
}


TR_LIB_API void lib_destroy_children(node_t *children)
{
	if(children)
		free(children);
}

TR_LIB_API int lib_disk_free_space(const char *type)
{
	if(war_strcasecmp(type, "1 Firmware Upgrade Image") == 0)
		return 50 * 1024 * 1024; //50M
	else if(war_strcasecmp(type, "2 Web Content") == 0)
		return 10 * 1024 * 1024; //10 M
	else if(war_strcasecmp(type, "3 Vendor Configuration File") == 0)
		return 100 * 1024; //100 K
     	else if(war_strcasecmp(type, "X 00256D 3GPP Configuration File") == 0)
	    return 1024 * 1024; //1M
	else
		return -1;
}


// meet slash to double slash
void process_slash(char *dst,char *src)
{
	char *s1;
	char *s2;
	s1 = src;
	s2 = dst;

//printf("[%s] [%s]\n",s1,s2);	
	
	while( *s1) {
		if( *s1 == '\\') {
			*s2 = *s1;
			s2++;
		}
		*s2 = *s1;

		s1++;
		s2++;		
		
	}

	*s2 = 0;	
}

#define TMPFS 1
extern char *get_value_from_upload_config(char *path , char *name , char *value);
extern char *upload_configuration_init(char *path , char *name_string,char *boundry);
extern int in_cksum(unsigned short *buf, int sz);
//#define HWID                    "AP94-AR7161-RT-080619-02"
//#define HWCOUNTRYID             "01"//00:usa, 01:others country all the same
#define HWID_LEN					24
#define HWCOUNTRYID_LEN             2
//#define DOWNLOAD_FW_FILE        "/home/tr/conf/firmware.bin"
char DOWNLOAD_FW_FILE[256]="/home/tr/conf/firmware.bin";
#define FW_UPGRADE_FILE					"/var/firm/image.bin"
//#define NVRAM_UPGRADE_BIN	"/home/tr/conf/config.conf"
char NVRAM_UPGRADE_BIN[256]="/home/tr/conf/config.conf";
#define NVRAM_CONFIG_FILE "/var/tmp/config.bin"
#define NVRAM_SIZE				1024

#define NVRAM_UPGRADE_TMP    "/var/tmp/nvram.tmp" /* XXX: copy from shvar.h */

#define WORKING_PATH  "/home/tr/conf"

int firmware_upgrade=0; // default not do

TR_LIB_API int lib_upgradefirmware(void)
{
	return firmware_upgrade;
}

TR_LIB_API int lib_stop_tr069_for_upgradefirmware(void)
{
//    char buff[128];
printf("kill wantimer, tr069 stop\n");

    system("killall wantimer");
    sleep(1);
    system("killall wantimer");
    
//    system("kill tr069&");
	
		return 0;
}

TR_LIB_API int lib_download_available(const char * type)
{
    if(war_strcasecmp(type, "1 Firmware Upgrade Image") == 0)  {
    	if( firmware_upgrade )
    		return 0;
    	else 
    		return 1;
    }
	
		return 1;
}


//TR_LIB_API int lib_download_complete(const char * type, const char *path)
TR_LIB_API int lib_download_complete(const char * type, const char *path, char *url)

{
    char buff[128];
        FILE *fp;
        
    dprintf("path=[%s]",path);    
    sprintf(NVRAM_UPGRADE_BIN,"%s/%s",WORKING_PATH,path); // path should random
    sprintf(DOWNLOAD_FW_FILE,"%s/%s",WORKING_PATH,path); // path should random

    if(war_strcasecmp(type, "1 Firmware Upgrade Image") == 0)  
    {
        //char *pCountryID = NULL, *pHwID = NULL;	
        char ImageCountryID[HWCOUNTRYID_LEN+1], ImageHwID[HWID_LEN + 1]; 
//        char buff[64];
        //int i;
//        FILE *fp;

        fp = fopen(DOWNLOAD_FW_FILE,"r");
        fseek(fp,-26 ,SEEK_END);
        fread(ImageCountryID,sizeof(char), HWCOUNTRYID_LEN, fp);
        ImageCountryID[HWCOUNTRYID_LEN] = '\0';
        tr_log(LOG_DEBUG, "ImageCountryID=%s\n", ImageCountryID); //Brian+ for debug
        
        fread(ImageHwID, sizeof(char), HWID_LEN, fp);
        ImageHwID[HWID_LEN] = '\0';
        fclose(fp);
        tr_log(LOG_DEBUG, "ImageHwID=%s\n", ImageHwID); //Brian+ for debug
        
        if((!strncmp(ImageHwID, HWID, HWID_LEN)) && (!strncmp(ImageCountryID, HWCOUNTRYID, HWCOUNTRYID_LEN)))
{
        
            tr_log(LOG_DEBUG,"CountryID and HwID are matched!!!!!\n");	

#ifdef TMPFS
            //system("mount -t tmpfs tmpfs /var/firm");
            char tmp[256];
            process_slash(tmp,DOWNLOAD_FW_FILE);
            printf("%s\n",tmp);
            sprintf(buff,"/bin/mv %s %s", tmp, FW_UPGRADE_FILE);
            printf("%s\n",buff);
            system(buff);

// add stop wish & qos            
        system("/etc/init.d/stop_wish &");
        system("/etc/init.d/streamengine_stop &");
            
	        system("/var/sbin/fwupgrade fw &");
	        
	        firmware_upgrade = 1;
#endif
        }     
        else
        {
            tr_log(LOG_ERROR, "CountryID or HwID is not match!!!\n");
            return 9018; //error message: file is corrupted!!
	    }

        tr_log(LOG_DEBUG, "FIRMWARE UPGRADE is DONE!!!!!\n");
		tr_log(LOG_WARNING, "Need reboot after complete download: %s", type);

        return 0;
    }

    else if(war_strcasecmp(type, "3 Vendor Configuration File") == 0)
    {
        /* check upload file checksum */
	    char upload_config_string[1024 * 32] = {};     // max size : 32K
	    char checksum_value_string[32] = {};
	    char checksum_string[32] = {};
	    unsigned long chksum = 0;
	    char *upload_config = NULL;
	    char *checksum_value = NULL;
        struct stat buf;

// rm some file
        sprintf(buff,"rm %s", NVRAM_UPGRADE_TMP);
        system(buff);


        
        stat(NVRAM_UPGRADE_BIN, &buf);
        int size = buf.st_size;
        tr_log(LOG_NOTICE, "The config file size=%d", size);

        //check config file size, can't > 32k
        if (size > 32768) //32k
        {
            char command[64] = {0};
            sprintf(command, "%s%s", "rm -f ", NVRAM_UPGRADE_BIN);
            system(command);
            tr_log(LOG_ERROR, "The config file is > 32k!!");
            return 9018;
	    }    

        /* get checksum from upload configuration file */
	    checksum_value = get_value_from_upload_config(NVRAM_UPGRADE_BIN,"config_checksum",checksum_value_string);
	    if(!checksum_value)
	    {
            char command[64] = {0};
            sprintf(command, "%s%s", "rm -f ", NVRAM_UPGRADE_BIN);
            system(command);
            tr_log(LOG_ERROR, "Can't get cheksum value from config file!!");
            return 9018;
	    }

	    /* init upload configuration with format : A=B\nC=D\nE=F\n..... */
	    upload_config = upload_configuration_init(NVRAM_UPGRADE_BIN,upload_config_string,"config_checksum");
	    if(!upload_config)
        {
		    char command[64] = {0};
            sprintf(command, "%s%s", "rm -f ", NVRAM_UPGRADE_BIN);
            system(command);
            tr_log(LOG_ERROR, "config file init fail!!");
            return 9002;
	    }

	    /* calculate the checksum according to upload configuration */
	    chksum = in_cksum((unsigned short *)upload_config, strlen(upload_config));
        //chksum = checksum((unsigned short *)upload_config, strlen(upload_config)); //Brian+: use chksum in tr_lib.c
        sprintf(checksum_string,"%lu",chksum);
	    
	    /* validate checksum consistently */
	    if( strcmp(checksum_string,checksum_value) != 0)
        {
		    char command[64] = {0};
            sprintf(command, "%s%s", "rm -f ", NVRAM_UPGRADE_BIN);
            system(command);
            tr_log(LOG_ERROR, "config file checksum validation fail!!");
            tr_log(LOG_ERROR, "checksum_string=%s, checksum_value=%s", checksum_string, checksum_value);
            return 9018;
	    }
        
        //Update VendorConfigFile node value

        char *p=strrchr(url,'/');
        tr_log(LOG_NOTICE, "config file name=%s", p+1);    

        //nvram_set("tr069_dl_conf_name", p+1); 
        //nvram_set("tr069_dl_conf_time", lib_current_time());
        //nvram_commit();

	unlink(NVRAM_UPGRADE_BIN);
	fp = fopen(NVRAM_UPGRADE_TMP, "w+");
	if (fp == NULL)
		return 0;

	fwrite(upload_config_string , 1 , sizeof(upload_config_string) , fp);
	fclose(fp);



//        sprintf(buff,"mv %s %s",NVRAM_UPGRADE_BIN,NVRAM_UPGRADE_TMP);
//		system(buff);

        /* Read default configuration file */
	    system("nvram upgrade &");

        return 0;
	}

#ifdef TR196
	else if(war_strcasecmp(type, "X 00256D 3GPP Configuration File") == 0){
		process_cm(path);	
		return 1;
	}
#endif		
	else {
		return 0;
	}
}


void httpd_nvram_reset(void)
{
	char buff[256];
	// not use &
	sprintf(buff,"wget http://%s/nvram_reset.txt -s",nvram_safe_get("lan_ipaddr"));
	
	_system(buff);
}


TR_LIB_API int lib_commit_transaction(void)
{
	tr_log(LOG_DEBUG, "Commit transaction!");
	
	dprintf("lib_commit_transaction\n");
	
	if(change) {
		change = 0;
		int ret;		
		ret = tree2xml(root, xml_file_path);

		if( nvram_need_commit ) {
			nvram_need_commit = 0;
			
// we need to change wlan_setting issue 
			wlaninfo_commit();
			
			nvram_commit();
			
			httpd_nvram_reset();
		}
				
		return ret;
	}

	return 0;
}



//path=InternetGatewayDevice.IPPingDiagnostics.MaximumResponseTime
//path=InternetGatewayDevice.IPPingDiagnostics.MinimumResponseTime
//path=InternetGatewayDevice.IPPingDiagnostics.AverageResponseTime
//path=InternetGatewayDevice.IPPingDiagnostics.FailureCount
//path=InternetGatewayDevice.IPPingDiagnostics.SuccessCount
//path=InternetGatewayDevice.IPPingDiagnostics.DSCP
//path=InternetGatewayDevice.IPPingDiagnostics.DataBlockSize
//path=InternetGatewayDevice.IPPingDiagnostics.Timeout
//path=InternetGatewayDevice.IPPingDiagnostics.NumberOfRepetitions
//path=InternetGatewayDevice.IPPingDiagnostics.Host
//path=InternetGatewayDevice.IPPingDiagnostics.Interface
//path=InternetGatewayDevice.IPPingDiagnostics.DiagnosticsState


static int ip_ping()
{
    node_t node;

    tr_log(LOG_DEBUG, "Start IP Ping test");
    war_sleep(10);
    
    
	int ttl,ret;
	struct ping_diag diag;
	
	memset(&diag,0,sizeof(diag));

// don't need to check node null
    node_t host_node;
    lib_resolve_node("InternetGatewayDevice.IPPingDiagnostics.Host", &host_node);
    
	lib_resolve_node("InternetGatewayDevice.IPPingDiagnostics.DSCP",&node);	
	diag.DSCP = atol(node->value);
	
	lib_resolve_node("InternetGatewayDevice.IPPingDiagnostics.DataBlockSize",&node);	
	diag.DataBlockSize = atol(node->value);

	lib_resolve_node("InternetGatewayDevice.IPPingDiagnostics.Timeout",&node);	
	diag.Timeout = atol(node->value);

	lib_resolve_node("InternetGatewayDevice.IPPingDiagnostics.NumberOfRepetitions",&node);	
	diag.NumberOfRepetitions = atol(node->value);	

//	ret = start_ping4("www.hinet.net",&ttl);
//	ret = start_ping4("www.abc.com",&ttl);
//	ret = start_ping4("199.181.132.250",&ttl,&diag);
	ret = start_ping4(host_node->value,&ttl,&diag);
//	dprintf("%d %d\n",ret,ttl);
//	exit(1);
    
    // update
    lib_resolve_node("InternetGatewayDevice.IPPingDiagnostics.MaximumResponseTime", &node);
	war_snprintf(node->value, sizeof(node->value), "%ld", diag.MaximumResponseTime);

    lib_resolve_node("InternetGatewayDevice.IPPingDiagnostics.MinimumResponseTime", &node);
	war_snprintf(node->value, sizeof(node->value), "%ld", diag.MinimumResponseTime);

    lib_resolve_node("InternetGatewayDevice.IPPingDiagnostics.AverageResponseTime", &node);
	war_snprintf(node->value, sizeof(node->value), "%ld", diag.AverageResponseTime);

    lib_resolve_node("InternetGatewayDevice.IPPingDiagnostics.SuccessCount", &node);
	war_snprintf(node->value, sizeof(node->value), "%ld", diag.SuccessCount);

    lib_resolve_node("InternetGatewayDevice.IPPingDiagnostics.FailureCount", &node);
	war_snprintf(node->value, sizeof(node->value), "%ld", diag.FailureCount);
    
    
    
    
    tr_log(LOG_DEBUG, "IP Ping test over");
    lib_start_session();
    lib_resolve_node(IP_PING, &node);
    lib_set_value(node, "Complete");
    lib_end_session();
    
    tr069_cli("http://127.0.0.1:1234/add/event/", "code=8 DIAGNOSTICS COMPLETE");
  
    return 0;
}

void lib_start_ip_ping(void)
{
#if 1
    pthread_t id;
    pthread_create(&id, NULL, (void *)ip_ping, NULL);
#endif

#if 0
    HANDLE id;
    id = CreateThread(NULL, 0, ip_ping, NULL, 0, NULL);
#endif

#if 0
    taskSpawn("task_IPPING", 90, 0, TASK_STACK_SIZE, (FUNCPTR)ip_ping, 0, 0, 0, 0, 0, 0, 0, 0, 0 ,0);
#endif
    return ;
}

#if 0
void lib_start_ip_ping(void)
{
        node_t node;

        tr_log(LOG_DEBUG, "Start IP Ping test");
        lib_start_session();
        lib_resolve_node(IP_PING, &node);
        lib_set_value(node, "Complete");
        lib_end_session();
        system("./sendtocli http://127.0.0.1:1234/add/event/ \"code=8 DIAGNOSTICS COMPLETE&cmdkey=\"");
}
#endif

void lib_stop_ip_ping(void)
{
	tr_log(LOG_DEBUG, "Stop IP Ping test");
}

static int trace_route()
{
    node_t node;

    tr_log(LOG_DEBUG, "Start trace route test!");
    war_sleep(10);
    tr_log(LOG_DEBUG, "trace route test over!");
    lib_start_session();
    lib_resolve_node(TRACE_ROUTE, &node);
    lib_set_value(node, "Complete");
    lib_end_session();
    

    tr069_cli("http://127.0.0.1:1234/add/event/", "code=8 DIAGNOSTICS COMPLETE");
    return 0;
}

void lib_start_trace_route(void)
{
#if 1
    pthread_t id;
    pthread_create(&id, NULL, (void *)trace_route, NULL);
#endif
    
#if 0
    HANDLE id;
    id = CreateThread(NULL, 0, trace_route, NULL, 0, NULL);
#endif

#if 0
    taskSpawn("task_TRACEROUTE", 90, 0, TASK_STACK_SIZE, (FUNCPTR)trace_route, 0, 0, 0, 0, 0, 0, 0, 0, 0 ,0);
#endif
    return;
}

#if 0
void lib_start_trace_route(void)
{
        node_t node;

        tr_log(LOG_DEBUG, "Start trace route test");
        lib_start_session();
        lib_resolve_node(TRACE_ROUTE, &node);
        lib_set_value(node, "Complete");
        lib_end_session();
        system("./sendtocli http://127.0.0.1:1234/add/event/ \"code=8 DIAGNOSTICS COMPLETE&cmdkey=\"");
}
#endif

void lib_stop_trace_route(void)
{
    tr_log(LOG_DEBUG, "Stop trace route test");
}
//add to DSL DIAGNOSTICS
static int dsl_diagnostics(char *path)
{
    node_t node;

    tr_log(LOG_DEBUG, "Start dsl_diagnostics test %s", path);
    war_sleep(10);
    tr_log(LOG_DEBUG, "dsl_dignostic test over");
    lib_start_session();
    lib_resolve_node(path, &node);
    lib_set_value(node, "Complete");
    lib_end_session();


    tr069_cli("http://127.0.0.1:1234/add/event/", "code=8 DIAGNOSTICS COMPLETE");
    return 0;
}
void lib_start_dsl_diagnostics(char *path)
{
#if 1
    pthread_t id;
    pthread_create(&id, NULL, (void *)dsl_diagnostics, path);
#endif

#if 0
    HANDLE id;
    id = CreateThread(NULL, 0, dsl_diagnostics, path, 0, NULL);
#endif

#if 0
    taskSpawn("task_DSLDIAG", 90, 0, TASK_STACK_SIZE, (FUNCPTR)dsl_diagnostics, path, 0, 0, 0, 0, 0, 0, 0, 0 ,0);
#endif
    return; 
}
void lib_stop_dsl_diagnostics(char *path)
{
    tr_log(LOG_DEBUG, "Stop dsl_diagnostics test %s", path);
}

static int atm_diagnostics(char *path)
{
    node_t node;

    tr_log(LOG_DEBUG, "Start atm_diagnostics test %s", path);
    war_sleep(10);
    tr_log(LOG_DEBUG, "atm_dignostic test over");
    lib_start_session();
    lib_resolve_node(path, &node);
    lib_set_value(node, "Complete");
    lib_end_session();


    tr069_cli("http://127.0.0.1:1234/add/event/", "code=8 DIAGNOSTICS COMPLETE");
    return 0;
}
void lib_start_atm_diagnostics(char *path)
{
#if 1
    pthread_t id;
    pthread_create(&id, NULL, (void *)atm_diagnostics, path);
#endif

#if 0
    HANDLE id;
    id = CreateThread(NULL, 0, atm_diagnostics, path, 0, NULL);
#endif

#if 0
    taskSpawn("task_ATMDIAG", 90, 0, TASK_STACK_SIZE, (FUNCPTR)atm_diagnostics, path, 0, 0, 0, 0, 0, 0, 0, 0 ,0);
#endif
    return ;
}
void lib_stop_atm_diagnostics(char *path)
{
    tr_log(LOG_DEBUG, "Stop atm_diagnostics test %s", path);
}


TR_LIB_API void lib_get_interface_ip(const char *inter, char *ip, int ip_len)
{
	war_snprintf(ip, ip_len, "0.0.0.0");
}

TR_LIB_API unsigned int lib_get_interface_traffic(const char *inter, int direction)
{
	static unsigned int dummy_traffic = 0;

	dummy_traffic += 30000;
	return dummy_traffic;
}


TR_LIB_API int lib_get_wib_session_timer()
{
	//time to trigger wib
	return 5;
}

TR_LIB_API int lib_get_wib_url(char *wib_url, int len)
{
	char *mac = "000102030405";
	war_snprintf(wib_url, len, "http://10.10.1.12/wib/bootstrap?version=0&msid=%s&protocol={1}", mac);
	return 0;
}

TR_LIB_API int lib_get_emsk(char **emsk)
{
	//wib decrypt EMSK
	char *tmp = malloc(5);
	war_snprintf(tmp, 5, "%s", "wkss");
	*emsk = tmp;
	return 0;
}

/*interface to generate the pm file*/
TR_LIB_API int lib_gen_pm_file(const char* path)
{
	return 0;
}
