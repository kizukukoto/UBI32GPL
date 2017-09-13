

#include <string.h>
#include <strings.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <time.h> 
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <linux/rtnetlink.h>
#include <net/ethernet.h>
#include <netpacket/packet.h>
#include <netinet/in.h>
#include <netinet/ip6.h>
#include <netinet/icmp6.h>
#include <net/if.h>
#include <errno.h> 

//
//#define ICMP6_ECHO_REQUEST          128
//#define ICMP6_ECHO_REPLY            129
//
#ifndef MLD_LISTENER_QUERY
#define MLD_LISTENER_QUERY          130
#endif
#ifndef MLD_LISTENER_REPORT
#define MLD_LISTENER_REPORT         131
#endif
#ifndef MLD_LISTENER_REDUCTION
#define MLD_LISTENER_REDUCTION      132
#endif

#ifndef MLDv2_LISTENER_REPORT
#define MLDv2_LISTENER_REPORT	    143
#define MODE_IS_INCLUDE		    0x03
#define MODE_IS_EXCLUDE		    0x04
#endif
//
//#define ND_ROUTER_SOLICIT           133
//#define ND_ROUTER_ADVERT            134
//#define ND_NEIGHBOR_SOLICIT         135
//#define ND_NEIGHBOR_ADVERT          136
//#define ND_REDIRECT                 137
//
#ifndef MCAST_JOIN_GROUP
#define MCAST_JOIN_GROUP             42
#endif
#ifndef MCAST_LEAVE_GROUP
#define MCAST_LEAVE_GROUP            45
#endif


#define DFT_ROBUST_VAR                 2
#define DFT_QUERY_ITVL                 125
#define DFT_QUERY_RESP_ITVL            10
#define DFT_LAST_LISTENER_QUERY_ITVL   1


#define MAX_IF_NUM  4

#define NLMSG_OK2(nlh,len)	((len) >= (int)sizeof(struct nlmsghdr) && \
				 (nlh)->nlmsg_len >= sizeof(struct nlmsghdr) && \
				 (int)(nlh)->nlmsg_len <= (len))


/*
 * ifconfig
 */
struct timer_lst {
	struct timeval     expires;
	       void        (*handler)(void*);
	       void       *data;
	struct timer_lst  *next;
	struct timer_lst  *prev;	
};

struct ifrx {
	char  name[IFNAMSIZ];
	int  ifidx;
	struct in6_addr  addr;

	int robust_var;
	int qry_itvl;
	int qry_resp_itvl;
	int mcast_lisenr_itvl;
	int oqrier_itvl;
	int start_qry_itvl;
	int start_qry_cnt;
	int last_lisenr_qry_itvl;
	int last_lisenr_qry_cnt;

	char  isquerier;
	char  isstartupq;
	int   r_qry_cnt;
	int   s_qry_cnt;
	struct timer_lst  tm_qry;
	struct timer_lst  tm_nonqry;
};

struct ip6ex {
	uint8_t  ip6e_nxt;
	uint8_t  ip6e_len;
};

struct ip6orx {
	uint8_t  ip6or_type;
	uint8_t  ip6or_len;
	uint8_t  ip6or_value[2];
};

struct greqx {
	int  ifidx;
	struct sockaddr_in6  sai6;
};

/*
 * group
 */
struct mldgroupx {
	struct in6_addr   ma;
	struct ifrx      *ifx;
//	struct ifrx      *ifx[MAX_IF_NUM];
	       int        r_rpt_cnt;
	       int        s_masqry_cnt;
	struct timer_lst  tm_masqry;
	struct timer_lst  tm_alive;
	struct mldgroupx *pre;
	struct mldgroupx *nxt;
};

/*
 * MLDv1 header.
 */
struct mld1hx {
	uint8_t  type;
	uint8_t  code;
	uint16_t  cksum;
	uint16_t  maxdelay;
	uint16_t  reserved;
	struct in6_addr  maddr;
};

/*
 * MLDv2 header.
 */
struct mld2hx {
	uint8_t  type;
	uint8_t  code;
	uint16_t cksum;
	uint16_t  reserved;
	uint16_t number;
	uint8_t record;
	uint8_t aux;
	uint16_t number_of_Source;
	struct in6_addr  maddr;
};

#define _PATH_ADDRX_  "/proc/net/if_inet6"
extern struct ifrx       aifr[MAX_IF_NUM];
extern struct mldgroupx *pgmg;
extern int  ifcnt;
extern int  gpcnt;

extern int sendd2big(char *data, int dlen, int mtu);
extern int Smldqry(int ifidx, struct in6_addr *ma, uint16_t maxdelay);
extern int Rmldqry(struct sockaddr_ll* psa, char *data, int dlen, struct mld1hx *pm1h);
extern int Rmldrpt(struct sockaddr_ll* psa, char *data, int dlen, struct mld1hx *pm1h);
extern int Rmldrptv2(struct sockaddr_ll* psa, char *data, int dlen, struct mld2hx *pm2h);
extern int mldv2_send_include_mode(int ifidx,struct in6_addr *ma);
extern int Rmlddne(struct sockaddr_ll* psa, char *data, int dlen, struct mld1hx *pm1h);


extern int ifinit(char *lan, char *wan);
extern int ifconf(char *ifname, struct ifrx *pifrx);
extern void* getifrx(int ifidx);
extern int getmtu(char *ifname);
extern int getaddr(char *name, struct in6_addr *addr);
extern int findgp(struct in6_addr *ma, struct ifrx *pi, struct mldgroupx **pp);
extern int addgp(struct in6_addr *ma, struct ifrx *pi, struct mldgroupx **pp);
extern int delgp(struct in6_addr *ma, struct ifrx *pi);

extern uint16_t cksum(void *ptr, int count);
extern uint16_t hsum(struct ip6_hdr *ph6, uint32_t len, uint8_t nexthdr);
extern int chk_hbhopt(char *data, int dlen);

extern void querier(struct ifrx *pifrx, int sec, char start);
extern void nonquerier(struct ifrx *pifrx, int sec);
extern void htime_gquery(void *data);
extern void htime_nonquerier(void *data);
extern void htime_galive(void *data);
extern void htime_masqry(void *data);

void clear_timer(struct timer_lst *tm);
void init_timer( struct timer_lst *tm, void (*handler)(void*), void *data);
void set_timer(struct timer_lst *tm, int secs);

extern int printaddr(struct in6_addr *paddr);




