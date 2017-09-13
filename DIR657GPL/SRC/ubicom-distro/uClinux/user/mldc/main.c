

#include "mld.h"


#ifdef IP8000
#define UBICOM_IP8K  1
#endif


struct ifrx  aifr[MAX_IF_NUM];
struct mldgroupx *pgmg;
       int  ifcnt;
       int  sock0, sock1;
       char  recvbuf[1600];

int opensock(void);
int loop(void);
int ismld(struct sockaddr_ll *psa, char *data, int dlen);
int test(struct sockaddr_ll* psa, char *data, int dlen);

//#include <linux/mroute6.h>

#include <stdarg.h>

long debug_print = 0;


void enable_debugprint(void)
{
	debug_print = 1;
}

void _dprintf(const char *fmt, ...)
{
	va_list args;
	char buf[1024];
	
	if( debug_print == 0)
		return;

	va_start(args, fmt);
	vsprintf(buf, fmt,args);
	va_end(args);

	printf("%s",buf);

}




#define dprintf _dprintf

#define MRT6_BASE	200
#define MRT6_INIT	(MRT6_BASE)	/* Activate the kernel mroute code 	*/
#define MRT6_DONE	(MRT6_BASE+1)	/* Shutdown the kernel mroute		*/
#define MRT6_ADD_MIF	(MRT6_BASE+2)	/* Add a virtual interface		*/
#define MRT6_DEL_MIF	(MRT6_BASE+3)	/* Delete a virtual interface		*/
#define MRT6_ADD_MFC	(MRT6_BASE+4)	/* Add a multicast forwarding entry	*/
#define MRT6_DEL_MFC	(MRT6_BASE+5)	/* Delete a multicast forwarding entry	*/
#define MRT6_VERSION	(MRT6_BASE+6)	/* Get the kernel multicast version	*/
#define MRT6_ASSERT	(MRT6_BASE+7)	/* Activate PIM assert mode		*/
#define MRT6_PIM	(MRT6_BASE+8)	/* enable PIM code	*/


#define MIFF_REGISTER	0x1	/* register vif	*/

typedef unsigned short mifi_t;

struct mif6ctl {
	mifi_t	mif6c_mifi;		/* Index of MIF */
	unsigned char mif6c_flags;	/* MIFF_ flags */
	unsigned char vifc_threshold;	/* ttl limit */
	u_short	 mif6c_pifi;		/* the index of the physical IF */
	unsigned int vifc_rate_limit;	/* Rate limiter values (NI) */
};


#ifndef IF_SETSIZE
#define IF_SETSIZE	256
#endif

typedef	__u32		if_mask;
#define NIFBITS (sizeof(if_mask) * 8)        /* bits per mask */

#if !defined(__KERNEL__) && !defined(DIV_ROUND_UP)
#define	DIV_ROUND_UP(x,y)	(((x) + ((y) - 1)) / (y))
#endif

typedef struct if_set {
	if_mask ifs_bits[DIV_ROUND_UP(IF_SETSIZE, NIFBITS)];
} if_set;

struct mf6cctl
{
	struct sockaddr_in6 mf6cc_origin;		/* Origin of mcast	*/
	struct sockaddr_in6 mf6cc_mcastgrp;		/* Group in question	*/
	mifi_t	mf6cc_parent;			/* Where it arrived	*/
	struct if_set mf6cc_ifset;		/* Where it is going */
};


#define IF_SET(n, p)    ((p)->ifs_bits[(n)/NIFBITS] |= (1 << ((n) % NIFBITS)))
#include <arpa/inet.h>

#if 1
struct ipv6_mreq_kern {
	/* IPv6 multicast address of group */
	struct in6_addr ipv6mr_multiaddr;

	/* local IPv6 address of interface */
	int		ipv6mr_ifindex;
};
#endif

int mrouter_s6;


static void DEBUG_PRINTHEX(char *s,int len)
{
	int i;
	char *c;
	c = s;

	if( debug_print == 0)
		return;
	
	for(i=0;i<len;i++) {
		if( (i != 0) && ((i % 16)== 0 )) 
			dprintf("\n");
			
		dprintf("%02x ",*c++);
	}
	dprintf("\n");
}



struct mld2_grec {
	__u8		grec_type;
	__u8		grec_auxwords;
	__be16		grec_nsrcs;
	struct in6_addr	grec_mca;
	struct in6_addr	grec_src[0];
};

struct mld2_report {
	__u8	type;
	__u8	resv1;
	__sum16	csum;
	__be16	resv2;
	__be16	ngrec;
	struct mld2_grec grec[0];
};

#define ICMPV6_MGM_QUERY		130
#define ICMPV6_MGM_REPORT       	131
#define ICMPV6_MGM_REDUCTION    	132
#define ICMPV6_MLD2_REPORT		143



void add_del_cache_table(int add,char *group) 
{
	int i;
	long ret;
	struct sockaddr_in6 source_addr;
	struct sockaddr_in6 group_addr;

	inet_pton(AF_INET6, "::", &(source_addr.sin6_addr));

	for(i=0;i<16;i++)
		group_addr.sin6_addr.in6_u.u6_addr8[i] = group[i];
		
	int maxvifs = 255;
	struct mf6cctl mc;

	if( add == 0) {
		memset(&mc, 0, sizeof(mc));
		memcpy(&mc.mf6cc_origin, &source_addr, sizeof(mc.mf6cc_origin));
		memcpy(&mc.mf6cc_mcastgrp, &group_addr, sizeof(mc.mf6cc_mcastgrp));
		ret = setsockopt(mrouter_s6, IPPROTO_IPV6, MRT6_DEL_MFC,
                (void *)&mc, sizeof(mc));

		dprintf("ret=%d\n",ret);
			return;
	}

	memset(&mc, 0, sizeof(mc));
	memcpy(&mc.mf6cc_origin, &source_addr, sizeof(mc.mf6cc_origin));
	memcpy(&mc.mf6cc_mcastgrp, &group_addr, sizeof(mc.mf6cc_mcastgrp));
	mc.mf6cc_parent = 3; //iif_index;
	for (i = 0; i < maxvifs; i++)
		IF_SET(i, &mc.mf6cc_ifset);
	ret = setsockopt(mrouter_s6, IPPROTO_IPV6, MRT6_ADD_MFC,
		(void *)&mc, sizeof(mc));
	dprintf("ret=%d\n",ret);
	
}


int add_join_leave(int join,int s,int interface,char *ip)
{
	long ret;
	int i;
	long cmd;
	struct ipv6_mreq_kern mreq;
	mreq.ipv6mr_ifindex = interface;

	for(i=0;i<16;i++) 
		mreq.ipv6mr_multiaddr.in6_u.u6_addr8[i] = ip[i];

	if( join)
		cmd = IPV6_ADD_MEMBERSHIP;
	else
		cmd = IPV6_DROP_MEMBERSHIP;
		
	ret = setsockopt(s, IPPROTO_IPV6, cmd,
           (void *)&mreq, sizeof(mreq));

	dprintf("add_join_leave ret=%d\n",ret);
	return ret; 
	
}



// phy number (linux network)

int phy_br0;     
int phy_eth; // wan
int phy_lan;

#ifdef UBICOM_IP8K
char name_br0[16]="br0";
char name_eth[16]="eth1";
char name_lan[16]="eth0";
#else
char name_br0[16]="br0";
char name_eth[16]="eth0.1";
char name_lan[16]="eth0";
#endif


char host_ff02__1[16]={0xff,0x02,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01};


long TimeCount()
{
    struct  timeval  curtime; //, lasttime, difftime, tv; 
    gettimeofday(&curtime, NULL);
    return curtime.tv_sec;
}



struct lanpc {
    struct lanpc *next;
    char ip[16];

// issue 2011.07.19
    int                 timeValue;       // Downcounter for death.          

};


    
/**
*   Routing table structure definition. Double linked list...
*/
struct RouteTable {
    struct RouteTable   *next;     // Pointer to the next group in line.
    char group[16];          // The group to route

    int                 timeValue;       // Downcounter for death.          

// 2011.04.27
    struct lanpc        *lan;
};

struct RouteTable *routing_table;
long table_count;

struct RouteTable *findNextTable(char *group)
{
	struct RouteTable *routing;
	
	routing = routing_table;
	
	while( routing) {
		if( memcmp(routing->group,group,16) == 0) {
			return routing;
		}
		routing = routing->next;
	}
	
	return NULL;
}

// update lan 
void updateTable_Lan(struct RouteTable *routing,char *src)
{
	struct lanpc *lan;
	struct lanpc *newlan;
	
	lan = routing->lan;
	
	while( lan) {
		if( memcmp(lan->ip,src,16) == 0) {
			break;
		}		
		lan = lan->next;
	}
	
	if( lan  ) {
		// exist
		// 2011.07.19
		lan->timeValue = TimeCount(); // wait to do
		return;
	}

	// not exist, need update
	newlan = (struct lanpc *) malloc(sizeof(*newlan));
	if( newlan == NULL) {
		// memory issue
		//break;
		return;
	}

// 2011.07.19	
	newlan->timeValue = TimeCount(); // wait to do

	memcpy(newlan->ip,src,16);
	newlan->next = routing->lan;
	routing->lan = newlan;
	
	
} 



long int get_file_len(FILE *fp)
{
	long int len, cur;

	cur = ftell(fp); //Record the "current position"
	fseek(fp, 0, SEEK_END);
	len = ftell(fp);
	fseek(fp, cur, SEEK_SET); //Restore the file pointer to the "current position"

	return len;
}




// issue
int check_exist_igmp6_table(char *group)
{
	
	FILE *fp;
	char buf[256];
	char buf2[256];
//	char *s;
	unsigned char *src;
//	char devname[32];
	int ret;
	
	ret = 0;
	
	src = (unsigned char *)group;

	sprintf(buf2,"%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
		src[0],src[1],src[2],src[3],src[4],src[5],src[6],src[7],src[8],src[9],src[10],src[11],src[12],src[13],src[14],src[15]);

	
	sprintf(buf,"cat /proc/net/igmp6 | grep %s | grep %s  > /var/tmp/igmp6_mld",name_eth,buf2);
dprintf("[%s]\n",buf);

	system(buf);

	fp = fopen("/var/tmp/igmp6_mld", "r");

	if ( fp ) {
		if( get_file_len(fp) > 0) {
			ret = 1;
		}

		fclose(fp);

        	
	}
	

	return ret;
	
	
	
}


#define CHECK_LOCAL_MCAST 1

#ifdef CHECK_LOCAL_MCAST
long check_local_multicast(char *group)
{
	// ff02:: ...
	if( group[0] == 0xff && group[1] == 0x02)
		return 1;
	return 0;	
}
#endif

void add_igmpv6(char *src,char *group)
{
	long ret;
	struct RouteTable *routing;
	
#ifdef CHECK_LOCAL_MCAST
	if( check_local_multicast(group) )
		return;
#endif
	
	routing = findNextTable(group);
	
	ret = check_exist_igmp6_table(group);
//	ret = add_join_leave(1,mrouter_s6,phy_eth,group);
	dprintf("check_exist_igmp6_table=%d\n",ret);
	
	if( !ret ) {
		add_join_leave(0,mrouter_s6,phy_eth,group);
		if( routing ) {
			add_join_leave(1,mrouter_s6,phy_eth,group);
		}		
	}
	
	if( routing != NULL) {
		// exist, so update/check src
		routing->timeValue = TimeCount(); // wait to do
		updateTable_Lan(routing,src);
		return;
	}
	
	// add new table & src
	routing = (struct RouteTable *)malloc(sizeof(struct RouteTable));
	if( routing == NULL) {
		return;
	}

	table_count++;
	
	routing->next = routing_table;
	routing->timeValue = TimeCount(); // wait to do
	routing->lan = NULL;
	memcpy(routing->group,group,16);
	updateTable_Lan(routing,src);
	
	routing_table = routing;
	
dprintf("add igmp6\n");
	add_join_leave(1,mrouter_s6,phy_eth,group);
	add_del_cache_table(1,(char *) group);

}



int delTable_Lan(struct RouteTable *routing,char *src)
{

	// del 
	struct lanpc *uei = routing->lan;
	struct lanpc **prev_uei = &routing->lan;

	while (uei) {
		if ( memcmp(uei->ip,src,16) == 0) {
			break;
		}
		prev_uei = &(uei->next);
		uei = uei->next;
	}

	if (!uei) {
		// cannot find
		return 0;
	}	

	*prev_uei = uei->next;
	free(uei);
	
	return 1;
	
}


void del_igmpv6(char *src,char *group)
{
	struct RouteTable *routing;


#ifdef CHECK_LOCAL_MCAST
	if( check_local_multicast(group) )
		return;
#endif		

// siwtch issue
//void updateMLD(void);
//extern long MLDNextTime; // very 90 seconds

  // immedialtwe
//	MLDNextTime = TimeCount() - 1; 
//	updateMLD();

#define MLD_QUERY_SPECIFIC	2

void Send_MLD_V2_Query(int mode,char *m_ipv6);
	Send_MLD_V2_Query(MLD_QUERY_SPECIFIC,group);


	
	routing = findNextTable(group);

	if( routing == NULL) {
		return;
	}

	// del lan info.
	
	// del 
	struct RouteTable *uei = routing_table;
	struct RouteTable **prev_uei = &routing_table;

	while (uei) {
		if ( memcmp(uei->group,group,16) == 0) {
			break;
		}
		prev_uei = &(uei->next);
		uei = uei->next;
	}

	if (!uei) {
		// cannot find
		return;
	}	


// del lan ip
	delTable_Lan(uei,src);
	
	if( uei->lan != NULL) {
		// other ip exist
		return;
	}

	*prev_uei = uei->next;
	free(uei);
	table_count--;
	
dprintf("del igmp6\n");
							add_join_leave(0,mrouter_s6,phy_eth,group);
							add_del_cache_table(0,(char *) group);

	

}



// 2011.07.19
// prototype define
int RTTable_Lan_check(struct RouteTable *routing);

long MLDQueryTime=60; // very 90 seconds
long MLDNextTime; // very 90 seconds

void updateRT(void)
{
	//dprintf("updateRT\n");
	
//    struct  timeval  curtime; //, lasttime, difftime, tv; 
//    gettimeofday(&curtime, NULL);
	
//		dprintf("%d %8d %8d %8d \n",TimeCount(),table_count,curtime.tv_sec,curtime.tv_usec );

// limit: one time only delete one


	// del 
	struct RouteTable *uei = routing_table;
	struct RouteTable **prev_uei = &routing_table;

	long stoptime;
	long buftime;
	
// 1.5 	MLDQueryTime
	stoptime = TimeCount();
	
	buftime =  MLDQueryTime + MLDQueryTime/2; 
	

	while (uei) {
		if ( (uei->timeValue + buftime) < stoptime  ) {
			break;
		}
		
		// check lan time
		// 2011.07.19
		RTTable_Lan_check(uei);
		
		prev_uei = &(uei->next);
		uei = uei->next;
	}

	if (!uei) {
		// cannot find
		return;
	}	
			
// try del 
	{
		// free all ->lan memory
		struct lanpc *lan = uei->lan;
		
		while( lan ) {
			struct lanpc *nextlan;
			nextlan = lan->next;
			free(lan);
			lan = nextlan;
		}

	}
	
	dprintf("delete RT\n");
							add_join_leave(0,mrouter_s6,phy_eth,uei->group);
							add_del_cache_table(0,(char *) uei->group);
							
	*prev_uei = uei->next;
	free(uei);
	table_count--;
	
}


// 2011.07.19
int RTTable_Lan_check(struct RouteTable *routing)
{

	// del 
	struct lanpc *uei = routing->lan;
	struct lanpc **prev_uei = &routing->lan;


	long stoptime;
	long buftime;
	
// 1.5 	MLDQueryTime
	stoptime = TimeCount();
	
	buftime =  MLDQueryTime + MLDQueryTime/2; 


	while (uei) {
		if ( (uei->timeValue + buftime) < stoptime  ) {
			break;
		}
		prev_uei = &(uei->next);
		uei = uei->next;
	}

	if (!uei) {
		// cannot find
		return 0;
	}	

	*prev_uei = uei->next;
	free(uei);
	
	return 1;
	
}



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



typedef unsigned short int bool_t;
typedef signed char s8_t;
typedef unsigned char u8_t;
typedef signed short int s16_t;
typedef unsigned short int u16_t;
typedef signed long int s32_t;
typedef unsigned long int u32_t;
typedef signed long long int s64_t;
typedef unsigned long long int u64_t;
#define FALSE 0
#define TRUE 1



/*
 * hextobuf()
 *  Takes an ASCII string as a hex number and fills buff[len].
 */
static bool_t hex2buf(u8_t *buf, const char *s, u16_t len)
{
    u8_t j = 0;
    while (*s) {
        u8_t nbl;
        if ((*s >= '0') && (*s <= '9')) {
            nbl = *s - '0';
        } else if ((*s >= 'A') && (*s <= 'F')) {
            nbl = *s - 'A' + 10;
        } else if ((*s >= 'a') && (*s <= 'f')) {
            nbl = *s - 'a' + 10;
        } else {
            return FALSE;
        }

        if (j & 1) {    /* odd hex nibbles */
            *buf++ |= nbl;
        } else {    /* even hex nibbles */
            *buf = nbl << 4;
        }
        j++;
        if (j >= len*2) {
            break;
        }
        s++;
    }

    return TRUE;
}



void UpdateWLAN_IGMP6_single(char *filename)
{

	FILE *fp;
	char buf[256];
	
//	int i;

	char *s;

	char src[16];
	char group[16];

#if 0
	sprintf(buf,"/proc/%s",filename);
	fp = fopen(buf, "r");
	if( fp == NULL)
		return;
	close(fp);
#endif
	
	sprintf(buf,"cat /proc/%s > /var/tmp/igmp6_wlan",filename);

	system(buf);

	memset(src,0,16);
	memset(group,0,16);

	fp = fopen("/var/tmp/igmp6_wlan", "r");

	if ( fp ) {
		while ( fgets(buf, sizeof(buf), fp) ) {

//group_ipv6 ff 02 00 00 00 00 00 00 00 00 00 01 ff 64 f7 84
//group_saddr fe 80 00 00 00 00 00 00 02 1b 11 ff fe 64 f7 84
//  node 00:1b:11:64:f7:84


//			printf("%s",buf);


			s = strstr(buf,"group_ipv6");
			if( s) {
				s+= strlen("group_ipv6");
				while( *s && *s==' ')
				  s++;
			  trim_string_char(s,' ');
//				dprintf("%s\n",s);  
			  hex2buf((unsigned char *)group,s,16);
			}

			s = strstr(buf,"group_saddr");
			if( s) {
				s+= strlen("group_saddr");
				while( *s && *s==' ')
				  s++;
				  trim_string_char(s,' ');
				  hex2buf((unsigned char *)src,s,16);
			}

			s = strstr(buf,"node");
			if( s) {
				//acceptGroupReport(src, group,IGMP_V2_MEMBERSHIP_REPORT);
//				dprintf("%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
//					src[0],src[1],src[2],src[3],src[4],src[5],src[6],src[7],src[8],src[9],src[10],src[11],src[12],src[13],src[14],src[15]);
//				dprintf("%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
//					group[0],group[1],group[2],group[3],group[4],group[5],group[6],group[7],group[8],group[9],group[10],group[11],group[12],group[13],group[14],group[15]);
				
				add_igmpv6(src,group);
				
			}


		}
		fclose(fp);
	}

	
	
	//        acceptGroupReport(src, group, igmp->igmp_type);
}



#define __BIG_ENDIAN_BITFIELD
struct mld2_query {
	__u8 type;
	__u8 code;
	__sum16 csum;
	__be16 mrc;
	__be16 resv1;
	struct in6_addr mca;
#if defined(__LITTLE_ENDIAN_BITFIELD)
	__u8 qrv:3,
	     suppress:1,
	     resv2:4;
#elif defined(__BIG_ENDIAN_BITFIELD)
	__u8 resv2:4,
	     suppress:1,
	     qrv:3;
#else
#error "Please fix <asm/byteorder.h>"
#endif
	__u8 qqic;
	__be16 nsrcs;
	struct in6_addr srcs[0];
};


//
#define MLD_QUERY_GENERAL   1
#define MLD_QUERY_SPECIFIC	2

void Send_MLD_V2_Query_interface(int mode,char *m_ipv6,int interface)
{
	long ret;

// MLD Alert, it need, RFC told
	char opton[8] = { 0x00,0x00,0x05,0x02,0x00,0x00,0x01,0x00};


	struct mld2_query *mld_query;

	char query[24+4]; // v2 is 28 bytes	

	struct sockaddr_in6 dest_addr; //set up dest address info

	int val;
	
	val = phy_br0;

#ifdef UBICOM_IP8K	
	// here we use lan
	val = phy_lan;
#endif

	val = interface;

	dprintf("br0 val=%d\n",val);

	ret = setsockopt(mrouter_s6, IPPROTO_IPV6, IPV6_MULTICAST_IF,
           (void *)&val, sizeof(val));
	dprintf("ret=%d\n",ret);
	
// MLD Alert, it need	
	ret = setsockopt(mrouter_s6, IPPROTO_IPV6, IPV6_HOPOPTS, &opton,
		    8);

	dprintf("ret=%d\n",ret);
	

	memset(&dest_addr,0,sizeof(dest_addr));

// add len	
//	dest_addr.sin6_len = sizeof(dest_addr);
	
	dest_addr.sin6_family = AF_INET6;
	inet_pton (AF_INET6,"ff02::1",  &dest_addr.sin6_addr);


	memset(query,0,24+4);

	mld_query = (struct mld2_query *) query;
	mld_query->mrc = 1000; 
	
	// v2 
	mld_query->qrv = 2; // Robustness Default: 2 

	if( mode == MLD_QUERY_SPECIFIC ) {
		memcpy(&mld_query->mca,m_ipv6,16);
	}

	
	mld_query->type = ICMPV6_MGM_QUERY;

	ret = sendto(mrouter_s6, &query, 
               sizeof(query) , 0,
               (struct sockaddr *)&dest_addr, sizeof(dest_addr));

	dprintf("ret=%d\n",ret);

	
}



void Send_MLD_V2_Query(int mode,char *m_ipv6)
{

	Send_MLD_V2_Query_interface(mode,m_ipv6,phy_br0);
//	Send_MLD_V2_Query_interface(mode,m_ipv6,phy_lan);
//	Send_MLD_V2_Query_interface(mode,m_ipv6,10);


}



int get_num_ath(void)
{

	FILE *fp;
	char buf[256];
	char *s;
//	char devname[32];
	int count;
	
	count = 0;

	system("ifconfig > /var/tmp/igmp6_ifconfig");

	fp = fopen("/var/tmp/igmp6_ifconfig", "r");

	if ( fp ) {

		while( fgets(buf, 255, fp)) {
			if( (s = strstr(buf,"ath")) ) {
//				dprintf("[%s]\n",s);
				int num;
				num = s[3] - '0' + 1;
				if( num > 0 && num < 10) {
					if( num > count) 
						count = num;
				}
	    }
		}
		
		fclose(fp);

        	
	}
	

	return count;
}


void updateMLD(void)
{
	int num;
	int i;
	char buf[128];
	
	//dprintf("updateRT\n");
	if( TimeCount() <= MLDNextTime ) 
		return;
		
	MLDNextTime = TimeCount() + MLDQueryTime;
	Send_MLD_V2_Query(MLD_QUERY_GENERAL,NULL);
	
	
	num = get_num_ath();
	dprintf("num=%d\n",num);
	
	for(i=0;i<num;i++) {
#ifdef UBICOM_IP8K
		sprintf(buf,"igmp6_%d",i);
#else		
		sprintf(buf,"igmp6_ath%d",i);
#endif		
		dprintf("filename=%s\n",buf);	
			
		UpdateWLAN_IGMP6_single(buf);
	}	
}


/*
void my_alarm_handler(int a)
{
	Send_MLD_V2_Query();
	alarm(30);
	        
}
*/


void Init_MLD_Query(void)
{
 //  signal( SIGALRM, my_alarm_handler );
 //  alarm(1);	

	MLDNextTime = TimeCount() + 1;
}


void Do_MLD(void)
{
//	int i;

for(;;) {
	
	int status;
	
	char str[INET_ADDRSTRLEN];
	struct mld2_report	*report;
	struct sockaddr_in6	dst;
	
	struct iovec iov;
	struct msghdr msg = {
		.msg_name = &dst,
		.msg_namelen = sizeof(dst),
		.msg_iov = &iov,
		.msg_iovlen = 1,
	};
	static char buf[16384];



int MRouterFD;
    int     MaxFD, Rt; //, secs;
    fd_set  ReadFDS;
//    int     dummy = 0;
    struct  timeval  tv; 
    // The timeout is a pointer in order to set it to NULL if nessecary.
    struct  timeval  *timeout = &tv;

    timeout->tv_usec = 0;
    timeout->tv_sec = 1; //secs;

//timeout = NULL;
MRouterFD = mrouter_s6;
        // Prepare for select.
        MaxFD = MRouterFD;

        FD_ZERO( &ReadFDS );
        FD_SET( MRouterFD, &ReadFDS );

        // wait for input
        Rt = select( MaxFD +1, &ReadFDS, NULL, NULL, timeout );

        // log and ignore failures
        if( Rt < 0 ) {
           // log_msg( LOG_WARNING, errno, "select() failure" );
           dprintf( "select() failure" );
           goto next;
            continue;
        }
        else if( Rt > 0 ) {

            // Read IGMP request, and handle it...
            if( FD_ISSET( MRouterFD, &ReadFDS ) ) {
            }
            else {
            	goto next;
            	continue;
            }

				}
				else {			
						goto next;
						continue;
				}
        	






	iov.iov_base = buf;
//		struct nlmsghdr *h;

		iov.iov_len = sizeof(buf);
		
		status = recvmsg(mrouter_s6, &msg, MSG_DONTWAIT);
	
//		dprintf("%d\n",status);
		
		if( status <= 0) 
			goto next;

		inet_ntop(AF_INET6, &(dst.sin6_addr), str, INET6_ADDRSTRLEN);

//dprintf("6\n");	
	
//		dprintf("[%s]%d\n",str,len);
//		DEBUG_PRINTHEX(recvbuf,len);
		
		report = (struct mld2_report *) buf;



//#define ICMPV6_MGM_REPORT       	131
//#define ICMPV6_MGM_REDUCTION    	132
#if 0
struct mld2_grec {
	__u8		grec_type;
	__u8		grec_auxwords;
	__be16		grec_nsrcs;
	struct in6_addr	grec_mca;
	struct in6_addr	grec_src[0];
};

struct mld2_report {
	__u8	type;
	__u8	resv1;
	__sum16	csum;
	__be16	resv2;
	__be16	ngrec;
	struct mld2_grec grec[0];
};
#endif

#define MLD2_MODE_IS_INCLUDE	1
#define MLD2_MODE_IS_EXCLUDE	2
#define MLD2_CHANGE_TO_INCLUDE	3
#define MLD2_CHANGE_TO_EXCLUDE	4
#define MLD2_ALLOW_NEW_SOURCES	5
#define MLD2_BLOCK_OLD_SOURCES	6

// private type 199
		DEBUG_PRINTHEX(buf,status);
//		dprintf("%d %d\n",report->resv2,phy_br0);
		if( report->type != 199) 
			goto next;
					
		
//		DEBUG_PRINTHEX(buf,status);
//		dprintf("%d %d\n",report->resv2,phy_br0);
		
		if( report->resv2 != 1) // br0 use vif index 1 
			goto next;

		char *s;
		char *ipv6_dst;
		char *ipv6_src;
		
		s = (char *) report;
		s += 8; // icmpv6 header
		ipv6_src = s;
		s +=16; // src ipv6
		ipv6_dst = s;
		s +=16; // dst ipv6
		report = (struct mld2_report *) s;		
		status -= 40; 
		if( status <= 0) 
			goto next;



		dprintf("[%s]%d\n",str,status);
		dprintf("type=%d\n",report->type);
		DEBUG_PRINTHEX(s,status);
		
//		DEBUG_PRINTHEX(ipv6_src,16);
//		DEBUG_PRINTHEX(ipv6_dst,16);


// v2
		if( report->type == ICMPV6_MLD2_REPORT) {
				int count;
				count = report->ngrec;
				char *group;
				
				if( count >= 0) {
					struct mld2_grec *grec;					
					grec = (struct mld2_grec *) report->grec;
					
					while( count) {
						char *s;
						if( grec->grec_auxwords != 0) {
							dprintf("error format\n");
							goto next;
							break;
							//continue;
						}
						
						group = (char *) &grec->grec_mca;
		
			dprintf("v2[%s]%d\n",str,status);

			dprintf("report:%d ",report->type);
			DEBUG_PRINTHEX(group,16);
				
						if( grec->grec_type == MLD2_CHANGE_TO_EXCLUDE || 
							  grec->grec_type == MLD2_MODE_IS_EXCLUDE ) {
							//join
							add_igmpv6(ipv6_src,group);
							//add_join_leave(1,mrouter_s6,phy_eth,group);
							//add_del_cache_table(1,(char *) group);
						}
						else if( grec->grec_type == MLD2_CHANGE_TO_INCLUDE || 
							grec->grec_type ==  MLD2_MODE_IS_INCLUDE ) {
							// leave
							del_igmpv6(ipv6_src,group);
							//add_join_leave(0,mrouter_s6,phy_eth,group);
							//add_del_cache_table(0,(char *) group);
						}
					
						s = (char *) grec;
						s += sizeof( *grec); // - 16; // dec last ipv6
						//dprintf("%d\n",sizeof( *grec) );
						s += grec->grec_nsrcs * 16;
						grec = (struct mld2_grec *) s;						
						count--;
					}
					
					
					
				}
				
		}			
		else 
// v1
		if( report->type == ICMPV6_MGM_REPORT ||
			  report->type == ICMPV6_MGM_REDUCTION ) { 		
			char *group;

			dprintf("v1[%s]%d\n",str,status);

			dprintf("report:%d ",report->type);
			group = (char *) report->grec;
DEBUG_PRINTHEX(group,16);

#if 0
			if( memcmp((char *) report->grec,host_ff02__1,16)==0 ) {					
				  dprintf("found ff02::1\n");
					continue;
			}
			
			{
				char *addr = (char *) report->grec;
				if( *(addr+1) != 0x3e) {
					continue;
				}
			}
#endif			

			if( report->type == ICMPV6_MGM_REPORT ) {
				//join
//				add_join_leave(1,mrouter_s6,phy_br0,group);
				
			
				add_igmpv6(ipv6_src,group);
				//add_join_leave(1,mrouter_s6,phy_eth,group);
				//add_del_cache_table(1,(char *) group);
			}
			else {
				// leave
//				add_join_leave(0,mrouter_s6,phy_br0,group);
				del_igmpv6(ipv6_src,group);

				//add_join_leave(0,mrouter_s6,phy_eth,group);
				//add_del_cache_table(0,(char *) group);
			}
			
		
		}


		next:
			updateRT();
			updateMLD();
			continue;
	
	//sleep(1);
}
	
}


void mrt_6(void)
{
	
	mrouter_s6 = socket(AF_INET6, SOCK_RAW, IPPROTO_ICMPV6);

/* IPv6 */
	int v = 1;        /* 1 to enable, or 0 to disable */
	setsockopt(mrouter_s6, IPPROTO_IPV6, MRT6_INIT, (void *)&v, sizeof(v));



	int j = 1;
	if(setsockopt(mrouter_s6, SOL_SOCKET, SO_REUSEADDR, &j, sizeof(j)) < 0)
	{
		printf("re use fail\n");
	}


	int Sock;
	struct ifreq IfReq;
            
	long ret;            
	int  val=256*1024;
	ret = setsockopt(mrouter_s6, SOL_SOCKET, SO_RCVBUF, &val, sizeof(val));
	dprintf("%d\n",ret);



	if ( (Sock = socket( AF_INET, SOCK_DGRAM, 0 )) < 0 ) {
		//   log_msg( LOG_ERR, errno, "RAW socket open" );
	}

	memset(&IfReq,0,sizeof(IfReq));
	memcpy( IfReq.ifr_name, name_br0, strlen(name_br0) );


	// Get the physical index of the Interface
	if (ioctl(Sock, SIOCGIFINDEX, &IfReq ) < 0) {
		//    log_msg(LOG_ERR, errno, "ioctl SIOCGIFINDEX for %s", IfReq.ifr_name);
	}

	dprintf("br0=%d\n",IfReq.ifr_ifindex);

	phy_br0 = IfReq.ifr_ifindex;


	memset(&IfReq,0,sizeof(IfReq));
	memcpy( IfReq.ifr_name, name_eth, strlen(name_eth) );


	// Get the physical index of the Interface
	if (ioctl(Sock, SIOCGIFINDEX, &IfReq ) < 0) {
		//    log_msg(LOG_ERR, errno, "ioctl SIOCGIFINDEX for %s", IfReq.ifr_name);
	}

	dprintf("wan=%d\n",IfReq.ifr_ifindex);
	phy_eth = IfReq.ifr_ifindex;


	memset(&IfReq,0,sizeof(IfReq));
	memcpy( IfReq.ifr_name, name_lan, strlen(name_lan) );


	// Get the physical index of the Interface
	if (ioctl(Sock, SIOCGIFINDEX, &IfReq ) < 0) {
		//    log_msg(LOG_ERR, errno, "ioctl SIOCGIFINDEX for %s", IfReq.ifr_name);
	}

	dprintf("lan=%d\n",IfReq.ifr_ifindex);
	phy_lan = IfReq.ifr_ifindex;


	close(Sock);






	mifi_t mifi = 1;
	setsockopt(mrouter_s6, IPPROTO_IPV6, MRT6_DEL_MIF, (void *)&mifi,
           sizeof(mifi));
	mifi = 3;
	setsockopt(mrouter_s6, IPPROTO_IPV6, MRT6_DEL_MIF, (void *)&mifi,
        	sizeof(mifi));


#define MIFF_UDPINFO	0x4	/* register vif	*/
#define MIFF_NO_ORG		0x8	/* register vif	*/
#define MIFF_MLD_INFO	0x10	/* register vif	*/

	struct mif6ctl mc;
	memset(&mc, 0, sizeof(mc));
	/* Assign all mif6ctl fields as appropriate */
	mc.mif6c_mifi = 1; //mif_index;
	mc.mif6c_flags = 0; //MIFF_REGISTER; //mif_flags;
	mc.mif6c_flags = MIFF_MLD_INFO;
//	mc.mif6c_flags = 0; //MIFF_REGISTER; //mif_flags;
	mc.mif6c_pifi = phy_br0; //pif_index;
	ret = setsockopt(mrouter_s6, IPPROTO_IPV6, MRT6_ADD_MIF, (void *)&mc,
           sizeof(mc));
	dprintf("ret=%d\n",ret);


	memset(&mc, 0, sizeof(mc));
	/* Assign all mif6ctl fields as appropriate */
	mc.mif6c_mifi = 3; //mif_index;
	mc.mif6c_flags = 0; //MIFF_REGISTER; //mif_flags;
	mc.mif6c_flags = 4; // kernel add //MIFF_REGISTER; //mif_flags;
	mc.mif6c_flags = MIFF_NO_ORG;
//	mc.mif6c_flags = 0; //MIFF_REGISTER; //mif_flags;
	mc.mif6c_pifi = phy_eth; //pif_index;
	ret = setsockopt(mrouter_s6, IPPROTO_IPV6, MRT6_ADD_MIF, (void *)&mc,
           sizeof(mc));

	dprintf("ret=%d\n",ret);


#ifdef UBICOM_IP8K
// new linux kernel, mc_forwarding not enable/disable 
#else
// old linux kernel, mc_forwarding is switch(enable/disable) 
	system("echo 1 > /proc/sys/net/ipv6/conf/all/mc_forwarding");
#endif

	Init_MLD_Query();

	Do_MLD();


}




int main1(int argc, char **argv)
{
	mrt_6();

	return 0;
}


int main2(int argc, char **argv) 
{
	int ret=0;

	printf("Enter mldc, ==== argc=%d, argv[0]=%s, argv[1]=%s, argv[2]=%s  ====\n", argc, argv[0], argv[1], argv[2]);
	if (3!=argc) {
		printf("usage: mldc [lan-if] [wan-if] &\n");
		return -1;
	}

	memset((void*)aifr, 0, sizeof(aifr));
	ifinit(argv[1], argv[2]);

	ret = opensock();
	if (-1==ret) {
		return -1;
	}

	loop();

	return 0;
}

int main(int argc, char **argv)
{	
	long newway;
	
	newway = 0;
	
	if( argc > 1) {
		
		if( strcmp(argv[1],"-d")==0 ) {
			enable_debugprint();
		}
		
		if( strcmp(argv[1],"-z")==0 ) {
			newway = 1;
		}
	}
	
	if( argc > 2) {
		
		if( strcmp(argv[2],"-d")==0 ) {
			enable_debugprint();
		}
		
		if( strcmp(argv[2],"-z")==0 ) {
			newway = 1;
		}
	}
	

	if( newway ) {
		main1(argc,argv);
		return 0;
	}

	main2(argc,argv);

	return 0;	
}


int opensock(void) {
	int  ret=0, val=256*1024;

	sock0 = socket(PF_PACKET, SOCK_DGRAM, htons(ETH_P_IPV6));
	if (-1==sock0) {
		return -1;
	}

	ret = setsockopt(sock0, SOL_SOCKET, SO_RCVBUF, &val, sizeof(val));
	if (0!=ret) {
		close(sock0);
		return -1;
	}

	sock1 = socket(PF_PACKET, SOCK_DGRAM, htons(ETH_P_IPV6));
	if (-1==sock1) {
		close(sock0);
		return -1;
	}

	return 0;
}


int loop() {
	int     len=0, ls, ism=0;
	fd_set  fdr, fdw;
	struct sockaddr_ll  sa;
	socklen_t  salen=sizeof(sa);

//	ls = (sock0>sock1)?sock0:sock1;
	ls = sock0;
	memset(recvbuf, 0, sizeof(recvbuf));

	FD_ZERO(&fdr);
	FD_ZERO(&fdw);
	FD_SET(sock0, &fdr);
//	FD_SET(sock1, &fdr);

	while (1) {
//		Smldqry(aifr[0].ifidx, 0, 0); // test mld query
		if (0>=select(ls+1, &fdr, NULL, NULL, NULL)) {
			continue;
		}

		if (FD_ISSET(sock0, &fdr)) {
			struct ip6_hdr *ph6;

			memset(&sa, 0, sizeof(sa));
			len = recvfrom(sock0, recvbuf, sizeof(recvbuf), 0, (struct sockaddr*)&sa, (socklen_t*)&salen);
			if (0>=len) {
				continue;
			}

			ph6 = (struct ip6_hdr*)recvbuf;

			// useless pkt (ucast, NS.NA.RS.RA.Redirect, ...)
			if (  (htons(ETH_P_IPV6)!=sa.sll_protocol)
			   || (PACKET_OUTGOING==sa.sll_pkttype) ) {
				continue;
			}

			if (!IN6_IS_ADDR_MULTICAST(&(ph6->ip6_dst))) {
				continue;
			}

			if (  IN6_IS_ADDR_UNSPECIFIED(&(ph6->ip6_src))
			   || IN6_IS_ADDR_MULTICAST(&(ph6->ip6_src)) ) {
				continue;
			}

			if (IN6_IS_ADDR_LINKLOCAL(&(ph6->ip6_src))) {
				//check ismld here
				ism = ismld(&sa, recvbuf, len);
				continue;
			}

			if (3>=(ph6->ip6_dst.s6_addr[1]&0xf)) {
				continue;
			}

			if (0==(ph6->ip6_hlim)--) {
				continue;
			}

			if (0<gpcnt)
				test(&sa, recvbuf, len);

		}
	}
	close(sock0);
	close(sock1);
	return 0;
}


int test( struct sockaddr_ll *psa,
                 char        *data,
                 int          dlen)
{
	int ret=0; //, oldifidx=psa->sll_ifindex;
	struct ip6_hdr *ph6=(struct ip6_hdr*)data;
	struct mldgroupx *p=0;
	int sd=0;

	if (1!=findgp((struct in6_addr*)&(ph6->ip6_dst), 0, &p)) {
		return -1;
	}

	if (!(p->ifx->isquerier)) {
		return -1;
	}

	if (-1==chk_hbhopt(data, dlen)) {
		return -1;
	}

	if (-1==(sd=socket(PF_PACKET, SOCK_DGRAM, htons(ETH_P_IPV6)))) {
		return -1;
	}

	*((uint16_t*)&(psa->sll_addr[0])) = 0x3333;
	memcpy(psa->sll_addr+2, ph6->ip6_dst.s6_addr+12, 4);

	psa->sll_ifindex = p->ifx->ifidx;

	if (0>=(ret=sendto(sd, (const void*)data, dlen, 0, (const struct sockaddr*)psa, (socklen_t)sizeof(struct sockaddr_ll)))) {
		if (EMSGSIZE==errno) {
			int mtu=0;
			if (-1!=(mtu=getmtu(p->ifx->name)))
				sendd2big(data, dlen, mtu); //send too big
		}
		close(sd);
		return -1;
	}

	close(sd);
	return 0;
}
