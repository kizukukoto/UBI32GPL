/*
 *  Point-to-Point Tunneling Protocol for Linux
 *
 *	Authors: Kozlov D. (xeb@mail.ru)
 *
 *	This program is free software; you can redistribute it and/or
 *	modify it under the terms of the GNU General Public License
 *	as published by the Free Software Foundation; either version
 *	2 of the License, or (at your option) any later version.
 *
 */

#include <linux/string.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/errno.h>
#include <linux/netdevice.h>
#include <linux/net.h>
#include <linux/skbuff.h>
#include <linux/init.h>
#include <linux/ppp_channel.h>
#include <linux/ppp_defs.h>
#include <linux/if_ppp.h>
#include "if_pppox.h"
#include <linux/notifier.h>
#include <linux/file.h>
#include <linux/proc_fs.h>
#include <linux/in.h>
#include <linux/ip.h>
#include <linux/netfilter.h>
#include <linux/netfilter_ipv4.h>
#include <linux/version.h>
#include <linux/spinlock.h>

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
#include <linux/tqueue.h>
#include <linux/timer.h>
#else
#include <linux/workqueue.h>
#endif

#include <net/sock.h>
#include <net/protocol.h>
#include <net/ip.h>
#include <net/icmp.h>
#include <net/route.h>

#include <asm/uaccess.h>

#define PPTP_DRIVER_VERSION "0.7.12"

MODULE_DESCRIPTION("Point-to-Point Tunneling Protocol for Linux");
MODULE_AUTHOR("Kozlov D. (xeb@mail.ru)");
MODULE_LICENSE("GPL");

static int log_level=0;
static int log_packets=10;
static int rx_stop=0;
static int min_window=5;
static int max_window=100;
static int statistics=0;
static int stat_collect_time=3;

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
MODULE_PARM(min_window,"i");
MODULE_PARM(max_window,"i");
MODULE_PARM(log_level,"i");
MODULE_PARM(log_packets,"i");
MODULE_PARM(statistics,"i");
MODULE_PARM(stat_collect_time,"i");
#else
module_param(min_window,int,0);
module_param(max_window,int,0);
module_param(log_level,int,0);
module_param(log_packets,int,0);
module_param(statistics,int,0);
module_param(stat_collect_time,int,0);
#endif
MODULE_PARM_DESC(min_window,"Minimum sliding window size (default=3)");
MODULE_PARM_DESC(max_window,"Maximum sliding window size (default=100)");
MODULE_PARM_DESC(log_level,"Logging level (default=0)");
MODULE_PARM_DESC(statistics,"Performance statistics collection");
MODULE_PARM_DESC(stat_collect_time,"Statistics collect time (sec)");

#define SC_RCV_BITS	(SC_RCV_B7_1|SC_RCV_B7_0|SC_RCV_ODDP|SC_RCV_EVNP)

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
#define __wait_event_timeout(wq, condition, ret)			\
do {									\
	wait_queue_t __wait;						\
	init_waitqueue_entry(&__wait, current);				\
									\
	for (;;) {							\
		set_current_state(TASK_UNINTERRUPTIBLE);		\
		if (condition)						\
			break;						\
		ret = schedule_timeout(ret);				\
		if (!ret)						\
			break;						\
	}								\
	current->state = TASK_RUNNING;					\
	remove_wait_queue(&wq, &__wait);				\
} while (0)
#define wait_event_timeout(wq, condition, timeout)			\
({									\
	long __ret = timeout;						\
	if (!(condition)) 						\
		__wait_event_timeout(wq, condition, __ret);		\
	__ret;								\
})

#define INIT_TIMER(_timer,_routine,_data) \
do { \
	(_timer)->function=_routine; \
	(_timer)->data=_data; \
	init_timer(_timer); \
} while (0); 

static inline void *kzalloc(size_t size,int gfp)
{
	void *p=kmalloc(size,gfp);
	memset(p,0,size);
	return p;
}

static inline void skb_get_timestamp(const struct sk_buff *skb, struct timeval *stamp)
{
	*stamp = skb->stamp;
}
static inline void __net_timestamp(struct sk_buff *skb, const struct timeval *stamp)
{
	struct timeval tv;
	do_gettimeofday(&tv);
	skb->stamp = *stamp;
}
#endif

//============= performance statistics =============
//static struct timeval stat_last_update={0,0};
unsigned long stat_last_update=0;
static unsigned int tx_packets_avg=0;
static unsigned int rx_packets_avg=0;
static unsigned int ack_timeouts_avg=0;
static unsigned int ack_works_avg=0;
static unsigned int buf_works_avg=0;
static unsigned int tx_packets_cur=0;
static unsigned int rx_packets_cur=0;
static unsigned int ack_timeouts_cur=0;
static unsigned int ack_works_cur=0;
static unsigned int buf_works_cur=0;
static unsigned int stat_tx_packets=0;
static unsigned int stat_rx_packets=0;
static unsigned int stat_tx_errors=0;
static unsigned int stat_rx_errors=0;
static unsigned int stat_rx_buffered=0;
static rwlock_t stat_lock=RW_LOCK_UNLOCKED;
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
struct tq_struct stat_work;
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,20)
struct delayed_work stat_work;
#else
struct work_struct stat_work;
#endif
static void inc_stat_counter(unsigned int *counter)
{
	if (statistics)
	{
		write_lock_bh(&stat_lock);
		(*counter)++;
		write_unlock_bh(&stat_lock);		
	}
}
#define INC_TX_PACKETS {inc_stat_counter(&tx_packets_cur); inc_stat_counter(&stat_tx_packets);}
#define INC_RX_PACKETS {inc_stat_counter(&rx_packets_cur); inc_stat_counter(&stat_rx_packets);}
#define INC_ACK_TIMEOUTS inc_stat_counter(&ack_timeouts_cur)
#define INC_ACK_WORKS inc_stat_counter(&ack_works_cur)
#define INC_BUF_WORKS inc_stat_counter(&buf_works_cur)
#define INC_TX_ERRORS inc_stat_counter(&stat_tx_errors)
#define INC_RX_ERRORS inc_stat_counter(&stat_rx_errors)
#define INC_RX_BUFFERED inc_stat_counter(&stat_rx_buffered)

static void do_stat_work(void*p)
{
	unsigned long delta_jiff;
	
	write_lock_bh(&stat_lock);
	
	delta_jiff=jiffies-stat_last_update;
	stat_last_update=jiffies;
	if (delta_jiff<=0) goto exit;
	
	tx_packets_avg=(tx_packets_avg+tx_packets_cur*HZ/delta_jiff)/2;
	rx_packets_avg=(rx_packets_avg+rx_packets_cur*HZ/delta_jiff)/2;
	ack_timeouts_avg=(ack_timeouts_avg+ack_timeouts_cur*HZ/delta_jiff)/2;
	ack_works_avg=(ack_works_avg+ack_works_cur*HZ/delta_jiff)/2;
	buf_works_avg=(buf_works_avg+buf_works_cur*HZ/delta_jiff)/2;
	
	tx_packets_cur=0;
	rx_packets_cur=0;
	ack_timeouts_cur=0;
	ack_works_cur=0;
	buf_works_cur=0;
	
exit:
	write_unlock_bh(&stat_lock);		

	schedule_delayed_work(&stat_work,stat_collect_time*HZ);
}
//==================================================



static struct proc_dir_entry* proc_dir;

#define HASH_SIZE  32
#define HASH(addr) ((addr^(addr>>3))&0x1F)

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
static rwlock_t chan_lock=RW_LOCK_UNLOCKED;
#define SK_STATE(sk) (sk)->state
#else
static DEFINE_RWLOCK(chan_lock);
#define SK_STATE(sk) (sk)->sk_state
#endif
static struct pppox_sock *chans[HASH_SIZE];

static int pptp_xmit(struct ppp_channel *chan, struct sk_buff *skb);
static int pptp_ppp_ioctl(struct ppp_channel *chan, unsigned int cmd,
			   unsigned long arg);
static int read_proc(char *page, char **start, off_t off,int count,
                     int *eof, void *data);
static int __pptp_rcv(struct pppox_sock *po,struct sk_buff *skb,int new);

static struct ppp_channel_ops pptp_chan_ops= {
	.start_xmit = pptp_xmit,
	.ioctl=pptp_ppp_ioctl,
};


#define MISSING_WINDOW 20
#define WRAPPED( curseq, lastseq) \
    ((((curseq) & 0xffffff00) == 0) && \
     (((lastseq) & 0xffffff00 ) == 0xffffff00))

/* gre header structure: -------------------------------------------- */

#define PPTP_GRE_PROTO  0x880B
#define PPTP_GRE_VER    0x1

#define PPTP_GRE_FLAG_C	0x80
#define PPTP_GRE_FLAG_R	0x40
#define PPTP_GRE_FLAG_K	0x20
#define PPTP_GRE_FLAG_S	0x10
#define PPTP_GRE_FLAG_A	0x80

#define PPTP_GRE_IS_C(f) ((f)&PPTP_GRE_FLAG_C)
#define PPTP_GRE_IS_R(f) ((f)&PPTP_GRE_FLAG_R)
#define PPTP_GRE_IS_K(f) ((f)&PPTP_GRE_FLAG_K)
#define PPTP_GRE_IS_S(f) ((f)&PPTP_GRE_FLAG_S)
#define PPTP_GRE_IS_A(f) ((f)&PPTP_GRE_FLAG_A)

struct pptp_gre_header {
  u_int8_t flags;		/* bitfield */
  u_int8_t ver;			/* should be PPTP_GRE_VER (enhanced GRE) */
  u_int16_t protocol;		/* should be PPTP_GRE_PROTO (ppp-encaps) */
  u_int16_t payload_len;	/* size of ppp payload, not inc. gre header */
  u_int16_t call_id;		/* peer's call_id for this session */
  u_int32_t seq;		/* sequence number.  Present if S==1 */
  u_int32_t ack;		/* seq number of highest packet recieved by */
  				/*  sender in this session */
};
#define PPTP_HEADER_OVERHEAD (2+sizeof(struct pptp_gre_header))

struct gre_statistics {
  /* statistics for GRE receive */
  unsigned int rx_accepted;  // data packet accepted
  unsigned int rx_lost;      // data packet did not arrive before timeout
  unsigned int rx_underwin;  // data packet was under window (arrived too late
                             // or duplicate packet)
  unsigned int rx_buffered;  // data packet arrived earlier than expected,
                             // packet(s) before it were lost or reordered
  unsigned int rx_errors;    // OS error on receive
  unsigned int rx_truncated; // truncated packet
  unsigned int rx_invalid;   // wrong protocol or invalid flags
  unsigned int rx_acks;      // acknowledgement only

  /* statistics for GRE transmit */
  unsigned int tx_sent;      // data packet sent
  unsigned int tx_failed;    //
  unsigned int tx_acks;      // sent packet with just ACK

  __u32 pt_seq;
  struct timeval pt_time;
  unsigned int rtt;
};

static struct pppox_sock * lookup_chan(__u16 call_id)
{
	struct pppox_sock *po;
	read_lock_bh(&chan_lock);
	for(po=chans[HASH(call_id)]; po; po=po->next)
		if (po->proto.pptp.src_addr.call_id==call_id)
			break;
	read_unlock_bh(&chan_lock);
	return po;
}

static void add_chan(struct pppox_sock *po)
{
	write_lock_bh(&chan_lock);
	po->next=chans[HASH(po->proto.pptp.src_addr.call_id)];
	chans[HASH(po->proto.pptp.src_addr.call_id)]=po;
	write_unlock_bh(&chan_lock);
}

static void add_free_chan(struct pppox_sock *po)
{
	static __u16 call_id=0;
	struct pppox_sock *p;

	write_lock_bh(&chan_lock);
	while (1) {
		if (++call_id==0) continue;
		for(p=chans[HASH(call_id)]; p; p=p->next)
			if (p->proto.pptp.src_addr.call_id==call_id)
				break;
		if (!p){
			po->proto.pptp.src_addr.call_id=call_id;
			po->next=chans[HASH(call_id)];
			chans[HASH(call_id)]=po;
			break;
		}
	}
	write_unlock_bh(&chan_lock);
}

static void del_chan(struct pppox_sock *po)
{
	struct pppox_sock *p1,*p2;

	write_lock_bh(&chan_lock);
	for(p2=NULL,p1=chans[HASH(po->proto.pptp.src_addr.call_id)]; p1 && p1!=po;
				p2=p1,p1=p1->next);
	if (p1){
	    if (p2) p2->next=p1->next;
	    else chans[HASH(po->proto.pptp.src_addr.call_id)]=p1->next;
	}
	write_unlock_bh(&chan_lock);
}

static int pptp_xmit(struct ppp_channel *chan, struct sk_buff *skb)
{
	struct sock *sk = (struct sock *) chan->private;
	struct pppox_sock *po = pppox_sk(sk);
	struct pptp_opt *opt=&po->proto.pptp;
	struct pptp_gre_header *hdr;
	unsigned int header_len=sizeof(*hdr);
	int len=skb?skb->len:0;
	int err=0;
	int window;

	struct rtable *rt;     			/* Route to the other host */
	struct net_device *tdev;			/* Device to other host */
	struct iphdr  *iph;			/* Our new IP header */
	int    max_headroom;			/* The extra header space needed */

	INC_TX_PACKETS;

	spin_lock_bh(&opt->xmit_lock);
	
	window=WRAPPED(opt->ack_recv,opt->seq_sent)?(__u32)0xffffffff-opt->seq_sent+opt->ack_recv:opt->seq_sent-opt->ack_recv;

	if (!skb){
	    if (opt->ack_sent == opt->seq_recv) goto exit;
	}else if (window>opt->window){
		__set_bit(PPTP_FLAG_PAUSE,(unsigned long*)&opt->flags);
		#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
		mod_timer(&opt->ack_timeout_timer,opt->stat->rtt/100*HZ/10000);
		#else
		schedule_delayed_work(&opt->ack_timeout_work,opt->stat->rtt/100*HZ/10000);
		#endif
		goto exit;
	}

	#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
	{
		struct rt_key key = {
			.dst=opt->dst_addr.sin_addr.s_addr,
			.src=opt->src_addr.sin_addr.s_addr,
			.tos=RT_TOS(0),
		};
		if ((err=ip_route_output_key(&rt, &key))) {
			goto tx_error;
		}
	}
	#else
	{
		struct flowi fl = { .oif = 0,
				    .nl_u = { .ip4_u =
					      { .daddr = opt->dst_addr.sin_addr.s_addr,
						.saddr = opt->src_addr.sin_addr.s_addr,
						.tos = RT_TOS(0) } },
				    .proto = IPPROTO_GRE };
		if ((err=ip_route_output_key(&rt, &fl))) {
			goto tx_error;
		}
	}
	#endif
	tdev = rt->u.dst.dev;
	
	#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
	max_headroom = ((tdev->hard_header_len+15)&~15) + sizeof(*iph)+sizeof(*hdr)+2;
	#else
	max_headroom = LL_RESERVED_SPACE(tdev) + sizeof(*iph)+sizeof(*hdr)+2;
	#endif
	

	if (!skb){
		skb=dev_alloc_skb(max_headroom);
		if (!skb) {
			ip_rt_put(rt);
			goto tx_error;
		}
		skb_reserve(skb,max_headroom-skb_headroom(skb));
	}else if (skb_headroom(skb) < max_headroom ||
						skb_cloned(skb) || skb_shared(skb)) {
		struct sk_buff *new_skb = skb_realloc_headroom(skb, max_headroom);
		if (!new_skb) {
			ip_rt_put(rt);
			goto tx_error;
		}
		if (skb->sk)
		skb_set_owner_w(new_skb, skb->sk);
		kfree_skb(skb);
		skb = new_skb;
	}
	
	if (skb->len){
		int islcp;
		unsigned char *data=skb->data;
		islcp=((data[0] << 8) + data[1])== PPP_LCP && 1 <= data[2] && data[2] <= 7;		
		
		/* compress protocol field */
		if ((opt->ppp_flags & SC_COMP_PROT) && data[0]==0 && !islcp)
			skb_pull(skb,1);
		
		/*
		 * Put in the address/control bytes if necessary
		 */
		if ((opt->ppp_flags & SC_COMP_AC) == 0 || islcp) {
			data=skb_push(skb,2);
			data[0]=0xff;
			data[1]=0x03;
		}
	}
	len=skb->len;

	if (len==0) header_len-=sizeof(hdr->seq);
	if (opt->ack_sent == opt->seq_recv) header_len-=sizeof(hdr->ack);

	// Push down and install GRE header
	skb_push(skb,header_len);
	hdr=(struct pptp_gre_header *)(skb->data);

	hdr->flags       = PPTP_GRE_FLAG_K;
	hdr->ver         = PPTP_GRE_VER;
	hdr->protocol    = htons(PPTP_GRE_PROTO);
	hdr->call_id     = htons(opt->dst_addr.call_id);

	if (!len){
		hdr->payload_len = 0;
		hdr->ver |= PPTP_GRE_FLAG_A;
		/* ack is in odd place because S == 0 */
		hdr->seq = htonl(opt->seq_recv);
		opt->ack_sent = opt->seq_recv;
		opt->stat->tx_acks++;
	}else {
		hdr->flags |= PPTP_GRE_FLAG_S;
		hdr->seq    = htonl(opt->seq_sent++);
		if (log_level>=3 && opt->seq_sent<=log_packets)
			printk(KERN_INFO"PPTP[%i]: send packet: seq=%i",opt->src_addr.call_id,opt->seq_sent);
		if (opt->ack_sent != opt->seq_recv)	{
		/* send ack with this message */
			hdr->ver |= PPTP_GRE_FLAG_A;
			hdr->ack  = htonl(opt->seq_recv);
			opt->ack_sent = opt->seq_recv;
			if (log_level>=3 && opt->seq_sent<=log_packets)
				printk(" ack=%i",opt->seq_recv);
		}
		hdr->payload_len = htons(len);
		if (log_level>=3 && opt->seq_sent<=log_packets)
			printk("\n");
	}

	/*
	 *	Push down and install the IP header.
	 */

	#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,22)
	skb->transport_header = skb->network_header;
	skb_push(skb, sizeof(*iph));
	skb_reset_network_header(skb);
	#else
	skb->h.raw = skb->nh.raw;
	skb->nh.raw = skb_push(skb, sizeof(*iph));
	#endif
	memset(&(IPCB(skb)->opt), 0, sizeof(IPCB(skb)->opt));
	#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,16)
	IPCB(skb)->flags &= ~(IPSKB_XFRM_TUNNEL_SIZE | IPSKB_XFRM_TRANSFORMED |
			      IPSKB_REROUTED);
	#endif

	#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,22)
	iph 			=	ip_hdr(skb);
	#else
	iph 			=	skb->nh.iph;
	#endif
	iph->version		=	4;
	iph->ihl		=	sizeof(struct iphdr) >> 2;
	iph->frag_off		=	0;//df;
	iph->protocol		=	IPPROTO_GRE;
	iph->tos		=	0;
	iph->daddr		=	rt->rt_dst;
	iph->saddr		=	rt->rt_src;
	#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
	iph->ttl = sysctl_ip_default_ttl;
	#else
	iph->ttl = dst_metric(&rt->u.dst, RTAX_HOPLIMIT);
	#endif
	iph->tot_len = htons(skb->len);

	dst_release(skb->dst);
	skb->dst = &rt->u.dst;
	
	nf_reset(skb);

	skb->ip_summed = CHECKSUM_NONE;
	ip_select_ident(iph, &rt->u.dst, NULL);
	ip_send_check(iph);

	err = NF_HOOK(PF_INET, NF_IP_LOCAL_OUT, skb, NULL, rt->u.dst.dev, dst_output);
	
	wake_up(&opt->wait);

	if (err == NET_XMIT_SUCCESS || err == NET_XMIT_CN) {
		opt->stat->tx_sent++;
		if (!opt->stat->pt_seq){
			opt->stat->pt_seq  = opt->seq_sent;
			do_gettimeofday(&opt->stat->pt_time);
		}
	}else{
		INC_TX_ERRORS;
		opt->stat->tx_failed++;	
	}

	spin_unlock_bh(&opt->xmit_lock);
	return 1;

tx_error:
	INC_TX_ERRORS;
	opt->stat->tx_failed++;
	if (!len) kfree_skb(skb);
	spin_unlock_bh(&opt->xmit_lock);
	return 1;
exit:
	spin_unlock_bh(&opt->xmit_lock);
	return 0;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,20)
static void ack_work(struct work_struct *work)
{
    struct pptp_opt *opt=container_of(work,struct pptp_opt,ack_work);
    struct pppox_sock *po=container_of(opt,struct pppox_sock,proto.pptp);
#else
static void ack_work(struct pppox_sock *po)
{
	struct pptp_opt *opt=&po->proto.pptp;
#endif
	INC_ACK_WORKS;
	pptp_xmit(&po->chan,0);

	if (!test_and_set_bit(PPTP_FLAG_PROC,(unsigned long*)&opt->flags)){
		if (ppp_unit_number(&po->chan)!=-1){
			char unit[10];
			sprintf(unit,"ppp%i",ppp_unit_number(&po->chan));
			create_proc_read_entry(unit,0,proc_dir,read_proc,po);
		}else clear_bit(PPTP_FLAG_PROC,(unsigned long*)&opt->flags);
	}
}


static void do_ack_timeout_work(struct pppox_sock *po)
{
    struct pptp_opt *opt=&po->proto.pptp;
    int paused;
    
    spin_lock_bh(&opt->xmit_lock);
    paused=__test_and_clear_bit(PPTP_FLAG_PAUSE,(unsigned long*)&opt->flags);
    if (paused){
			if (opt->window>min_window) --opt->window;
			opt->ack_recv=opt->seq_sent;
    }
    spin_unlock_bh(&opt->xmit_lock);
    if (paused) ppp_output_wakeup(&po->chan);
}
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,20)
static void _ack_timeout_work(struct work_struct *work)
{
    struct pptp_opt *opt=container_of(work,struct pptp_opt,ack_timeout_work.work);
    struct pppox_sock *po=container_of(opt,struct pppox_sock,proto.pptp);
		INC_ACK_TIMEOUTS;
    do_ack_timeout_work(po);
}
#else
static void _ack_timeout_work(struct pppox_sock *po)
{
	INC_ACK_TIMEOUTS;
  do_ack_timeout_work(po);
}
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
static void ack_timeout_timer(struct pppox_sock *po)
{
    struct pptp_opt *opt;
    opt=&po->proto.pptp;
    schedule_task(&opt->ack_timeout_work);
}
#endif

static int get_seq(struct sk_buff *skb)
{
	struct iphdr *iph;
	u8 *payload;
	struct pptp_gre_header *header;

	iph = (struct iphdr*)skb->data;
	payload = skb->data + (iph->ihl << 2);
	header = (struct pptp_gre_header *)(payload);

	return ntohl(header->seq);
}
static void do_buf_work(struct pppox_sock *po)
{
	struct timeval tv1,tv2;
	struct sk_buff *skb;
	struct pptp_opt *opt=&po->proto.pptp;
	unsigned int t;

	spin_lock_bh(&opt->rcv_lock);
	do_gettimeofday(&tv1);
	while((skb=skb_dequeue(&opt->skb_buf))){
		if (!__pptp_rcv(po,skb,0)){
			skb_get_timestamp(skb,&tv2);
			t=(tv1.tv_sec-tv2.tv_sec)*1000000+(tv1.tv_usec-tv2.tv_usec);
			if (t<opt->stat->rtt){
				skb_queue_head(&opt->skb_buf,skb);
				#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
				mod_timer(&opt->buf_timer,t/100*HZ/10000);
				#else
				schedule_delayed_work(&opt->buf_work,t/100*HZ/10000);
				#endif
				goto exit;
			}
			t=get_seq(skb)-1;
			opt->stat->rx_lost+=t-opt->seq_recv;
			opt->seq_recv=t;
			if (log_level>=2)
				printk(KERN_INFO"PPTP[%i]: unbuffer packet %i\n",opt->src_addr.call_id,t+1);
			__pptp_rcv(po,skb,0);
		}
	}
exit:
	spin_unlock_bh(&opt->rcv_lock);
}
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,20)
static void inline _buf_work(struct work_struct *work)
{
	struct pptp_opt *opt=container_of(work,struct pptp_opt,buf_work.work);
	struct pppox_sock *po=container_of(opt,struct pppox_sock,proto.pptp);
	
	INC_BUF_WORKS;
	do_buf_work(po);
}
#else
static void buf_work(struct pppox_sock *po)
{
	INC_BUF_WORKS;
	do_buf_work(po);
}
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
static void buf_timer(struct pppox_sock *po)
{
    struct pptp_opt *opt;
    opt=&po->proto.pptp;
    schedule_task(&opt->buf_work);
}
#endif

static int __pptp_rcv(struct pppox_sock *po,struct sk_buff *skb,int new)
{
	struct pptp_opt *opt=&po->proto.pptp;
	int headersize,payload_len,seq;
	__u8 *payload;
	struct pptp_gre_header *header;

	header = (struct pptp_gre_header *)(skb->data);

	if (log_level>=4) printk(KERN_INFO"PPTP[%i]: __pptv_rcv %i\n",opt->src_addr.call_id,new);

	if (new){
		/* test if acknowledgement present */
		if (PPTP_GRE_IS_A(header->ver)){
				int paused;
				__u32 ack = (PPTP_GRE_IS_S(header->flags))?
						header->ack:header->seq; /* ack in different place if S = 0 */
				
				ack = ntohl( ack);

				spin_lock_bh(&opt->xmit_lock);

				if (ack > opt->ack_recv) opt->ack_recv = ack;
				/* also handle sequence number wrap-around  */
				if (WRAPPED(ack,opt->ack_recv)) opt->ack_recv = ack;

				paused=__test_and_clear_bit(PPTP_FLAG_PAUSE,(unsigned long*)&opt->flags);
				spin_unlock_bh(&opt->xmit_lock);
				
				if (paused){
						#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
						del_timer(&opt->ack_timeout_timer);
						#else
				    cancel_delayed_work(&opt->ack_timeout_work);
				    #endif
				    ppp_output_wakeup(&po->chan);
				}else if (opt->window<opt->max_window) ++opt->window;


				if (opt->stat->pt_seq && opt->ack_recv > opt->stat->pt_seq){
					struct timeval tv;
					unsigned int rtt;
					do_gettimeofday(&tv);
					rtt = (tv.tv_sec - opt->stat->pt_time.tv_sec)*1000000+
						tv.tv_usec-opt->stat->pt_time.tv_usec;
					opt->stat->rtt = (opt->stat->rtt + rtt) / 2;
					if (opt->stat->rtt>opt->timeout) opt->stat->rtt=opt->timeout;
					opt->stat->pt_seq=0;
				}
		}else do_ack_timeout_work(po);

		/* test if payload present */
		if (!PPTP_GRE_IS_S(header->flags)){
			opt->stat->rx_acks++;
			goto drop;
		}
	}

	headersize  = sizeof(*header);
	payload_len = ntohs(header->payload_len);
	seq         = ntohl(header->seq);

	/* no ack present? */
	if (!PPTP_GRE_IS_A(header->ver)) headersize -= sizeof(header->ack);
	/* check for incomplete packet (length smaller than expected) */
	if (skb->len- headersize < payload_len){
		if (log_level>=1)
			printk(KERN_INFO"PPTP: discarding truncated packet (expected %d, got %d bytes)\n",
						payload_len, skb->len- headersize);
		opt->stat->rx_truncated++;
		goto drop;
	}

	payload=skb->data+headersize;
	/* check for expected sequence number */
	if ((seq == opt->seq_recv + 1) || (!opt->timeout &&
			(seq > opt->seq_recv + 1 || WRAPPED(seq, opt->seq_recv)))){
		if ( log_level >= 3 && opt->seq_sent<=log_packets)
			printk(KERN_INFO"PPTP[%i]: accepting packet %d size=%i (%02x %02x %02x %02x %02x %02x)\n",opt->src_addr.call_id, seq,payload_len,
				*(payload +0),
				*(payload +1),
				*(payload +2),
				*(payload +3),
				*(payload +4),
				*(payload +5));
		opt->stat->rx_accepted++;
		opt->stat->rx_lost+=seq-(opt->seq_recv + 1);
		opt->seq_recv = seq;
		#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
		schedule_task(&opt->ack_work);
		#else
		schedule_work(&opt->ack_work);
		#endif

		skb_pull(skb,headersize);

		if (payload[0] == PPP_ALLSTATIONS && payload[1] == PPP_UI){
			/* chop off address/control */
			if (skb->len < 3)
				goto drop;
			skb_pull(skb,2);
		}

		if ((*skb->data) & 1){
			/* protocol is compressed */
			skb_push(skb, 1)[0] = 0;
		}
		
		skb->ip_summed=CHECKSUM_NONE;
		ppp_input(&po->chan,skb);

		INC_RX_PACKETS;
	
		return 1;
	/* out of order, check if the number is too low and discard the packet.
	* (handle sequence number wrap-around, and try to do it right) */
	}else if ( seq < opt->seq_recv + 1 || WRAPPED(opt->seq_recv, seq) ){
		if ( log_level >= 1)
			printk(KERN_INFO"PPTP[%i]: discarding duplicate or old packet %d (expecting %d)\n",opt->src_addr.call_id,
							seq, opt->seq_recv + 1);
		opt->stat->rx_underwin++;
	/* sequence number too high, is it reasonably close? */
	}else /*if ( seq < opt->seq_recv + MISSING_WINDOW ||
						 WRAPPED(seq, opt->seq_recv + MISSING_WINDOW) )*/{
		opt->stat->rx_buffered++;
		if ( log_level >= 2 && new )
				printk(KERN_INFO"PPTP[%i]: buffering packet %d (expecting %d, lost or reordered)\n",opt->src_addr.call_id,
						seq, opt->seq_recv+1);
		INC_RX_BUFFERED;
		return 0;
	/* no, packet must be discarded */
	}/*else{
		if ( log_level >= 1 )
			printk(KERN_INFO"PPTP[%i]: discarding bogus packet %d (expecting %d)\n",opt->src_addr.call_id,
							seq, opt->seq_recv + 1);
	}*/
drop:
	INC_RX_ERRORS;
	kfree_skb(skb);
	return -1;
}

static int pptp_rcv(struct sk_buff *skb)
{
	struct pptp_gre_header *header;
	struct pppox_sock *po;
	struct pptp_opt *opt;

	if (log_level>=4) printk(KERN_INFO"PPTP: pptp_rcv rx_stop=%i\n",rx_stop);
	
	if (rx_stop) goto drop;

  if (!pskb_may_pull(skb, 12))
		goto drop;
		
	if (!(skb=skb_share_check(skb,GFP_ATOMIC)))
		goto out;

	header = (struct pptp_gre_header *)skb->data;

	if (    /* version should be 1 */
					((header->ver & 0x7F) != PPTP_GRE_VER) ||
					/* PPTP-GRE protocol for PPTP */
					(ntohs(header->protocol) != PPTP_GRE_PROTO)||
					/* flag C should be clear   */
					PPTP_GRE_IS_C(header->flags) ||
					/* flag R should be clear   */
					PPTP_GRE_IS_R(header->flags) ||
					/* flag K should be set     */
					(!PPTP_GRE_IS_K(header->flags)) ||
					/* routing and recursion ctrl = 0  */
					((header->flags&0xF) != 0)){
			/* if invalid, discard this packet */
		if (log_level>=1)
			printk(KERN_INFO"PPTP: Discarding GRE: %X %X %X %X %X %X\n",
							header->ver&0x7F, ntohs(header->protocol),
							PPTP_GRE_IS_C(header->flags),
							PPTP_GRE_IS_R(header->flags),
							PPTP_GRE_IS_K(header->flags),
							header->flags & 0xF);
		goto drop;
	}

	dst_release(skb->dst);
	skb->dst = NULL;
	nf_reset(skb);

	if ((po=lookup_chan(htons(header->call_id)))) {
		opt=&po->proto.pptp;
		if (!(SK_STATE(sk_pppox(po))&PPPOX_BOUND))
			goto drop;
		if (!po->chan.ppp){
			printk(KERN_INFO"PPTP: received packed, but ppp is down\n");
			goto drop;
		}
		spin_lock_bh(&opt->rcv_lock);
		if (__pptp_rcv(po,skb,1)){
			spin_unlock_bh(&opt->rcv_lock);
			do_buf_work(po);
		}else{
			__net_timestamp(skb);
			skb_queue_tail(&opt->skb_buf, skb);
			#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
			mod_timer(&opt->buf_timer,opt->stat->rtt/100*HZ/100000);
			#else
			schedule_delayed_work(&opt->buf_work,opt->stat->rtt/100*HZ/10000);
			#endif
			spin_unlock_bh(&opt->rcv_lock);
		}
		return NET_RX_SUCCESS;
	}else {
		if (log_level>=1)
			printk(KERN_INFO"PPTP: Discarding packet from unknown call_id %i\n",header->call_id);
		icmp_send(skb, ICMP_DEST_UNREACH, ICMP_PROT_UNREACH, 0);
	}

drop:
	INC_RX_ERRORS;	
	kfree_skb(skb);
out:
	return NET_RX_DROP;
}


//=============== PROC =======================
static int proc_output (struct pppox_sock *po,char *buf)
{
	struct pptp_opt *opt=&po->proto.pptp;
	struct gre_statistics *stat=opt->stat;
	char *p=buf;
	p+=sprintf(p,"rx accepted  = %d\n",stat->rx_accepted);
	p+=sprintf(p,"rx lost      = %d\n",stat->rx_lost);
	p+=sprintf(p,"rx under win = %d\n",stat->rx_underwin);
	p+=sprintf(p,"rx buffered  = %d\n",stat->rx_buffered);
	p+=sprintf(p,"rx invalid   = %d\n",stat->rx_invalid);
	p+=sprintf(p,"rx acks      = %d\n",stat->rx_acks);
	p+=sprintf(p,"tx sent      = %d\n",stat->tx_sent);
	p+=sprintf(p,"tx failed    = %d\n",stat->tx_failed);
	p+=sprintf(p,"tx acks      = %d\n",stat->tx_acks);
	p+=sprintf(p,"rtt          = %d\n",stat->rtt);
	p+=sprintf(p,"timeout      = %d\n",opt->timeout);
	p+=sprintf(p,"window       = %d\n",opt->window);
	p+=sprintf(p,"max window   = %d\n",opt->max_window);

	return p-buf;
}
static int read_proc(char *page, char **start, off_t off,int count, int *eof, void *data)
{
	struct pppox_sock *po = data;
	int len = proc_output (po,page);
	if (len <= off+count) *eof = 1;
	*start = page + off;
	len -= off;
	if (len>count) len = count;
	if (len<0) len = 0;
	return len;
}

int ctrl_write_proc(struct file* file,const char __user *buffer,unsigned long count,void *data)
{
	int res=count;
	char *tmp_buf=kmalloc(count+1,GFP_KERNEL);
	if (copy_from_user(tmp_buf,buffer,count))
		res=-EFAULT;
	else{
		int val;
		tmp_buf[count]=0;
		if (sscanf(tmp_buf," log_level = %i",&val)==1) log_level=val;
		else if (sscanf(tmp_buf," log_packets = %i",&val)==1) log_packets=val;
		else if (sscanf(tmp_buf," rx_stop = %i",&val)==1) rx_stop=val;
		else res=-EINVAL;
	}
	kfree(tmp_buf);
	return res;
}
int stat_write_proc(struct file* file,const char __user *buffer,unsigned long count,void *data)
{
	int res=count;
	char *tmp_buf=kmalloc(count+1,GFP_KERNEL);
	if (copy_from_user(tmp_buf,buffer,count))
		res=-EFAULT;
	else{
		int val;
		tmp_buf[count]=0;
		if (sscanf(tmp_buf," enable = %i",&val)==1){
			if (!statistics && val){
				stat_last_update=jiffies;
				schedule_delayed_work(&stat_work,stat_collect_time*HZ);
			}
			else if (statistics && !val){
				cancel_delayed_work(&stat_work);
			}
			statistics=val;
		}
		else if (sscanf(tmp_buf," collect_time = %i",&val)==1){
			if (val<=0) res=-EINVAL;
			else stat_collect_time=val;
		}
		else res=-EINVAL;
	}
	kfree(tmp_buf);
	return res;
}
static int stat_read_proc(char *page, char **start, off_t off,int count, int *eof, void *data)
{
	int len;
	char *p=page;
	p+=sprintf(p,"enabled      = %i\n",statistics);
	p+=sprintf(p,"collect_time = %i\n",stat_collect_time);
	p+=sprintf(p,"\nperformance counters per sec:\n");
	p+=sprintf(p,"tx_packets   = %i\n",tx_packets_avg);
	p+=sprintf(p,"rx_packets   = %i\n",rx_packets_avg);
	p+=sprintf(p,"ack_timeouts = %i\n",ack_timeouts_avg);
	p+=sprintf(p,"ack_works    = %i\n",ack_works_avg);
	p+=sprintf(p,"buf_works    = %i\n",buf_works_avg);
	p+=sprintf(p,"\noverall statistics:\n");
	p+=sprintf(p,"tx_packets   = %i\n",stat_tx_packets);
	p+=sprintf(p,"rx_packets   = %i\n",stat_rx_packets);
	p+=sprintf(p,"tx_errors    = %i\n",stat_tx_errors);
	p+=sprintf(p,"rx_errors    = %i\n",stat_rx_errors);
	p+=sprintf(p,"rx_buffered  = %i\n",stat_rx_buffered);
	len=p-page;
	
	if (len <= off+count) *eof = 1;
	*start = page + off;
	len -= off;
	if (len>count) len = count;
	if (len<0) len = 0;
	return len;
}
//============================================



static int pptp_bind(struct socket *sock,struct sockaddr *uservaddr,int sockaddr_len)
{
	struct sock *sk = sock->sk;
	struct sockaddr_pppox *sp = (struct sockaddr_pppox *) uservaddr;
	struct pppox_sock *po = pppox_sk(sk);
	struct pptp_opt *opt=&po->proto.pptp;
	int error=0;

	if (log_level>=1)
		printk(KERN_INFO"PPTP: bind: addr=%X call_id=%i\n",sp->sa_addr.pptp.sin_addr.s_addr,
						sp->sa_addr.pptp.call_id);
	lock_sock(sk);

	opt->src_addr=sp->sa_addr.pptp;
	if (sp->sa_addr.pptp.call_id){
		if (lookup_chan(sp->sa_addr.pptp.call_id)){
			error=-EBUSY;
			goto end;
		}
		add_chan(po);
	}else{
		add_free_chan(po);
		if (!opt->src_addr.call_id)
			error=-EBUSY;
		if (log_level>=1)
			printk(KERN_INFO"PPTP: using call_id %i\n",opt->src_addr.call_id);
	}

 end:
	release_sock(sk);
	return error;
}

static int pptp_connect(struct socket *sock, struct sockaddr *uservaddr,
		  int sockaddr_len, int flags)
{
	struct sock *sk = sock->sk;
	struct sockaddr_pppox *sp = (struct sockaddr_pppox *) uservaddr;
	struct pppox_sock *po = pppox_sk(sk);
	struct pptp_opt *opt=&po->proto.pptp;
	struct rtable *rt;     			/* Route to the other host */
	int error=0;

	if (log_level>=1)
		printk(KERN_INFO"PPTP[%i]: connect: addr=%X call_id=%i\n",opt->src_addr.call_id,
						sp->sa_addr.pptp.sin_addr.s_addr,sp->sa_addr.pptp.call_id);

	lock_sock(sk);

	if (sp->sa_protocol != PX_PROTO_PPTP){
		error = -EINVAL;
		goto end;
	}

	/* Check for already bound sockets */
	if (SK_STATE(sk) & PPPOX_CONNECTED){
		error = -EBUSY;
		goto end;
	}

	/* Check for already disconnected sockets, on attempts to disconnect */
	if (SK_STATE(sk) & PPPOX_DEAD){
		error = -EALREADY;
		goto end;
	}

	if (!opt->src_addr.sin_addr.s_addr || !sp->sa_addr.pptp.sin_addr.s_addr){
		error = -EINVAL;
		goto end;
	}

	po->chan.private=sk;
	po->chan.ops=&pptp_chan_ops;
	
	#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
	{
		struct rt_key key = {
			.dst=opt->dst_addr.sin_addr.s_addr,
			.src=opt->src_addr.sin_addr.s_addr,
			.tos=RT_TOS(0),
		};
		if (ip_route_output_key(&rt, &key)) {
			return -EHOSTUNREACH;
		}
	}
	#else
	{
		struct flowi fl = {
				    .nl_u = { .ip4_u =
					      { .daddr = opt->dst_addr.sin_addr.s_addr,
						.saddr = opt->src_addr.sin_addr.s_addr,
						.tos = RT_CONN_FLAGS(sk) } },
				    .proto = IPPROTO_GRE };
// jimmy marked, 20080429
//		security_sk_classify_flow(sk, &fl);
// -------------
		if (ip_route_output_key(&rt, &fl))
			return -EHOSTUNREACH;
		sk_setup_caps(sk, &rt->u.dst);
	}
	#endif
	po->chan.mtu=dst_mtu(&rt->u.dst);
	if (!po->chan.mtu) po->chan.mtu=PPP_MTU;
	po->chan.mtu-=PPTP_HEADER_OVERHEAD;
	
	po->chan.hdrlen=2+sizeof(struct pptp_gre_header);
	error = ppp_register_channel(&po->chan);
	if (error){
		printk(KERN_ERR "PPTP: failed to register PPP channel (%d)\n",error);
		goto end;
	}

	opt->dst_addr=sp->sa_addr.pptp;
	SK_STATE(sk) = PPPOX_CONNECTED;

 end:
	release_sock(sk);
	return error;
}

static int pptp_getname(struct socket *sock, struct sockaddr *uaddr,
		  int *usockaddr_len, int peer)
{
	int len = sizeof(struct sockaddr_pppox);
	struct sockaddr_pppox sp;

	sp.sa_family	= AF_PPPOX;
	sp.sa_protocol	= PX_PROTO_PPTP;
	sp.sa_addr.pptp=pppox_sk(sock->sk)->proto.pptp.src_addr;

	memcpy(uaddr, &sp, len);

	*usockaddr_len = len;

	return 0;
}

static int pptp_setsockopt(struct socket *sock, int level, int optname,
	char* optval, int optlen)
{
	struct sock *sk = sock->sk;
	struct pppox_sock *po = pppox_sk(sk);
	struct pptp_opt *opt=&po->proto.pptp;
	int val;
	
	if (optlen!=sizeof(int))
		return -EINVAL;

	if (get_user(val,(int __user*)optval))
		return -EFAULT;

	switch(optname) {
		case PPTP_SO_TIMEOUT:
			opt->timeout=val;
			break;
		case PPTP_SO_WINDOW:
			opt->max_window=val;
			opt->window=val/2;
			break;
		default:
				return -ENOPROTOOPT;
	}

	return 0;
}

static int pptp_getsockopt(struct socket *sock, int level, int optname,
	char* optval, int *optlen)
{
	struct sock *sk = sock->sk;
	struct pppox_sock *po = pppox_sk(sk);
	struct pptp_opt *opt=&po->proto.pptp;
	int len,val;

	if (get_user(len,(int __user*)optlen))
		return -EFAULT;

	if (len<sizeof(int))
		return -EINVAL;

	switch(optname) {
		case PPTP_SO_TIMEOUT:
			val=opt->timeout;
			break;
		case PPTP_SO_WINDOW:
			val=opt->window;
			break;
		default:
				return -ENOPROTOOPT;
	}

	if (put_user(sizeof(int),(int __user*)optlen))
		return -EFAULT;

	if (put_user(val,(int __user*)optval))
		return -EFAULT;

	return 0;
}

static int pptp_release(struct socket *sock)
{
	struct sock *sk = sock->sk;
	struct pppox_sock *po;
	struct pptp_opt *opt;
	int error = 0;

	if (!sk)
	    return 0;

	lock_sock(sk);

	#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)	
	if (sk->dead)
	#else
	if (sock_flag(sk, SOCK_DEAD))
	#endif
	{
	    release_sock(sk);
	    return -EBADF;
	}
	po = pppox_sk(sk);
	opt=&po->proto.pptp;

	if (log_level>=1)
		printk(KERN_INFO"PPTP[%i]: release\n",opt->src_addr.call_id);

	wake_up(&opt->wait);

	if (opt->src_addr.sin_addr.s_addr) {
		#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
		del_timer(&opt->buf_timer);
		del_timer(&opt->ack_timeout_timer);
		flush_scheduled_tasks();
		#else
		cancel_delayed_work(&opt->buf_work);
		cancel_delayed_work(&opt->ack_timeout_work);
		flush_scheduled_work();
		#endif
		skb_queue_purge(&opt->skb_buf);
		del_chan(po);

		if (test_bit(PPTP_FLAG_PROC,(unsigned long*)&opt->flags)) {
			char unit[10];
			sprintf(unit,"ppp%i",ppp_unit_number(&po->chan));
			remove_proc_entry(unit,proc_dir);
		}
	}

	pppox_unbind_sock(sk);
	SK_STATE(sk) = PPPOX_DEAD;

	kfree(opt->stat);

	sock_orphan(sk);
	sock->sk = NULL;

	#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)	
	skb_queue_purge(&sk->receive_queue);
	#else
	skb_queue_purge(&sk->sk_receive_queue);
	#endif
	release_sock(sk);
	sock_put(sk);

	return error;
}


#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,0)
static struct proto pptp_sk_proto = {
	.name	  = "PPTP",
	.owner	  = THIS_MODULE,
	.obj_size = sizeof(struct pppox_sock),
};
#endif

static struct proto_ops pptp_ops = {
    .family		= AF_PPPOX,
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,0)
    .owner		= THIS_MODULE,
#endif    
    .release		= pptp_release,
    .bind		=  pptp_bind,
    .connect		= pptp_connect,
    .socketpair		= sock_no_socketpair,
    .accept		= sock_no_accept,
    .getname		= pptp_getname,
    .poll		= sock_no_poll,
    .listen		= sock_no_listen,
    .shutdown		= sock_no_shutdown,
    .setsockopt		= pptp_setsockopt,
    .getsockopt		= pptp_getsockopt,
    .sendmsg		= sock_no_sendmsg,
    .recvmsg		= sock_no_recvmsg,
    .mmap		= sock_no_mmap,
    #if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,16)
    .ioctl		= pppox_ioctl,
    #endif
};


#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
static void pptp_sock_destruct(struct sock *sk)
{
	if (sk->protinfo.destruct_hook)
		kfree(sk->protinfo.destruct_hook);
		
	MOD_DEC_USE_COUNT;
}

static int pptp_create(struct socket *sock)
{
	int error = -ENOMEM;
	struct sock *sk;
	struct pppox_sock *po;
	struct pptp_opt *opt;

	MOD_INC_USE_COUNT;

	sk = sk_alloc(PF_PPPOX, GFP_KERNEL, 1);
	if (!sk)
		goto out;

	sock_init_data(sock, sk);

	sock->state = SS_UNCONNECTED;
	sock->ops   = &pptp_ops;

	//sk->sk_backlog_rcv = pppoe_rcv_core;
	sk->state	   = PPPOX_NONE;
	sk->type	   = SOCK_STREAM;
	sk->family	   = PF_PPPOX;
	sk->protocol	   = PX_PROTO_PPTP;

	sk->protinfo.pppox=kzalloc(sizeof(struct pppox_sock),GFP_KERNEL);
	sk->destruct=pptp_sock_destruct;
	sk->protinfo.destruct_hook=sk->protinfo.pppox;
	
	po = pppox_sk(sk);
	po->sk=sk;
	opt=&po->proto.pptp;

	opt->window=max_window/2;
	opt->max_window=max_window;
	opt->timeout=0;
	opt->flags=0;
	opt->seq_sent=0; opt->seq_recv=-1;
	opt->ack_recv=0; opt->ack_sent=-1;
	skb_queue_head_init(&opt->skb_buf);
  INIT_TQUEUE(&opt->ack_work,(void(*)(void*))ack_work,po);
	INIT_TQUEUE(&opt->buf_work,(void(*)(void*))buf_work,po);
	INIT_TQUEUE(&opt->ack_timeout_work,(void(*)(void*))ack_timeout_work,po);
	INIT_TIMER(&opt->buf_timer,(void(*)(unsigned long))buf_timer,(long int)po);
	INIT_TIMER(&opt->ack_timeout_timer,(void(*)(unsigned long))ack_timeout_timer,(long int)po);
	opt->stat=kzalloc(sizeof(*opt->stat),GFP_KERNEL);
	init_waitqueue_head(&opt->wait);
	spin_lock_init(&opt->xmit_lock);
	spin_lock_init(&opt->rcv_lock);

	error = 0;
out:
	return error;
}
#else
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,24)
static int pptp_create(struct socket *sock)
#else
static int pptp_create(struct net *net, struct socket *sock)
#endif
{
	int error = -ENOMEM;
	struct sock *sk;
	struct pppox_sock *po;
	struct pptp_opt *opt;

	#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,24)
	sk = sk_alloc(PF_PPPOX, GFP_KERNEL, &pptp_sk_proto, 1);
	#else
	sk = sk_alloc(net,PF_PPPOX, GFP_KERNEL, &pptp_sk_proto);
	#endif
	if (!sk)
		goto out;

	sock_init_data(sock, sk);

	sock->state = SS_UNCONNECTED;
	sock->ops   = &pptp_ops;

	//sk->sk_backlog_rcv = pptp_rcv_core;
	sk->sk_state	   = PPPOX_NONE;
	sk->sk_type	   = SOCK_STREAM;
	sk->sk_family	   = PF_PPPOX;
	sk->sk_protocol	   = PX_PROTO_PPTP;

	po = pppox_sk(sk);
	#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
	po->sk=sk;
	#endif
	opt=&po->proto.pptp;

	opt->window=max_window/2;
	opt->max_window=max_window;
	opt->timeout=0;
	opt->flags=0;
	opt->seq_sent=0; opt->seq_recv=-1;
	opt->ack_recv=0; opt->ack_sent=-1;
	skb_queue_head_init(&opt->skb_buf);
  #if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,20)
  INIT_WORK(&opt->ack_work,(work_func_t)ack_work);
	INIT_DELAYED_WORK(&opt->buf_work,(work_func_t)_buf_work);
	INIT_DELAYED_WORK(&opt->ack_timeout_work,(work_func_t)_ack_timeout_work);
  #else
	INIT_WORK(&opt->ack_work,(void(*)(void*))ack_work,po);
	INIT_WORK(&opt->buf_work,(void(*)(void*))buf_work,po);
	INIT_WORK(&opt->ack_timeout_work,(void(*)(void*))_ack_timeout_work,po);
  #endif
	opt->stat=kzalloc(sizeof(*opt->stat),GFP_KERNEL);
	init_waitqueue_head(&opt->wait);
	spin_lock_init(&opt->xmit_lock);
	spin_lock_init(&opt->rcv_lock);

	error = 0;
out:
	return error;
}
#endif


static int pptp_ioctl(struct socket *sock, unsigned int cmd, unsigned long arg)
{
	struct sock *sk = sock->sk;
	struct pppox_sock *po = pppox_sk(sk);
	struct pptp_opt *opt=&po->proto.pptp;
	int res=-EINVAL;

	switch (cmd) {
	case PPPTPIOWFP:
		release_sock(sk);
		res=wait_event_timeout(opt->wait,opt->seq_sent||SK_STATE(sk) == PPPOX_DEAD,arg*HZ);
		res=(res&&res<HZ)?1:res/HZ;
		lock_sock(sk);
		break;
	}
	
	return res;
}

static int pptp_ppp_ioctl(struct ppp_channel *chan, unsigned int cmd,
			   unsigned long arg)
{
	struct sock *sk = (struct sock *) chan->private;
	struct pppox_sock *po = pppox_sk(sk);
	struct pptp_opt *opt=&po->proto.pptp;
	void __user *argp = (void __user *)arg;
	int __user *p = argp;
	int err, val;

	err = -EFAULT;
	switch (cmd) {
	case PPPIOCGFLAGS:
		val = opt->ppp_flags;
		if (put_user(val, p))
			break;
		err = 0;
		break;
	case PPPIOCSFLAGS:
		if (get_user(val, p))
			break;
		opt->ppp_flags = val & ~SC_RCV_BITS;
		err = 0;
		break;
	default:
		err = -ENOTTY;
	}

	return err;
}


static struct pppox_proto pppox_pptp_proto = {
    .create	= pptp_create,
    .ioctl	= pptp_ioctl,
	  #if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,15)
    .owner	= THIS_MODULE,
    #endif
};


#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
static struct inet_protocol net_pptp_protocol = {
	.handler	= pptp_rcv,
	//.err_handler	=	pptp_err,
	.protocol = IPPROTO_GRE,
	.name     = "PPTP",
};
#else
static struct net_protocol net_pptp_protocol = {
	.handler	= pptp_rcv,
	//.err_handler	=	pptp_err,
};
#endif

static int pptp_init_module(void)
{
	struct proc_dir_entry *res;
	int err=0;
	printk(KERN_INFO "PPTP driver version " PPTP_DRIVER_VERSION "\n");

	if (stat_collect_time==0)
	{
		printk(KERN_ERR "PPTP: stat_collect_time must be >0\n");
		return -EINVAL;
	}

	#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
	inet_add_protocol(&net_pptp_protocol);
	#else
	if (inet_add_protocol(&net_pptp_protocol, IPPROTO_GRE) < 0) {
		printk(KERN_INFO "PPTP: can't add protocol\n");
		goto out;
	}
	#endif

	#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,0)
	err = proto_register(&pptp_sk_proto, 0);
	if (err){
		printk(KERN_INFO "PPTP: can't register sk_proto\n");
		goto out_inet_del_protocol;
	}
	#endif

 	err = register_pppox_proto(PX_PROTO_PPTP, &pppox_pptp_proto);
	if (err){
		printk(KERN_INFO "PPTP: can't register pppox_proto\n");
		goto out_unregister_sk_proto;
	}

	proc_dir=proc_mkdir("net/pptp",NULL);
	if (proc_dir){
	    proc_dir->owner=THIS_MODULE;
	    
	    res=create_proc_entry("ctrl",0,proc_dir);
	    if (!res)printk(KERN_ERR "PPTP: failed to create ctrl proc entry\n");
	    else res->write_proc=ctrl_write_proc;
	
	    res=create_proc_entry("stat",0,proc_dir);
	    if (!res)printk(KERN_ERR "PPTP: failed to create stat proc entry\n");
	    else{
	    	res->write_proc=stat_write_proc;
	    	res->read_proc=stat_read_proc;
	    }
	}else printk(KERN_ERR "PPTP: failed to create proc dir\n");
	//console_verbose();

  #if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
  // TODO: not implemented
  #elif LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,20)
	INIT_DELAYED_WORK(&stat_work,(work_func_t)do_stat_work);
  #else
	INIT_WORK(&stat_work,(void(*)(void*))do_stat_work,NULL);
  #endif
	
	stat_last_update=jiffies;
	if (statistics)
		schedule_delayed_work(&stat_work,stat_collect_time*HZ);

out:
	return err;
out_unregister_sk_proto:
	#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,0)
	proto_unregister(&pptp_sk_proto);
	#endif
out_inet_del_protocol:
	
	#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
	inet_del_protocol(&net_pptp_protocol);
	#else
	inet_del_protocol(&net_pptp_protocol, IPPROTO_GRE);
	#endif
	goto out;
}

static void pptp_exit_module(void)
{
	cancel_delayed_work(&stat_work);
	flush_scheduled_work();
	
	unregister_pppox_proto(PX_PROTO_PPTP);
	#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
	inet_del_protocol(&net_pptp_protocol);
	#else
	proto_unregister(&pptp_sk_proto);
	inet_del_protocol(&net_pptp_protocol, IPPROTO_GRE);
	#endif

	if (proc_dir)
	{
		remove_proc_entry("ctrl",proc_dir);
		remove_proc_entry("stat",proc_dir);
		remove_proc_entry("pptp",NULL);
	}
}


module_init(pptp_init_module);
module_exit(pptp_exit_module);
