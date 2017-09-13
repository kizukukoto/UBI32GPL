/*
 * drivers/net/ubi32-eth.c
 *   Ubicom32 ethernet TIO interface driver.
 *
 * (C) Copyright 2009, Ubicom, Inc.
 *
 * This file is part of the Ubicom32 Linux Kernel Port.
 *
 * The Ubicom32 Linux Kernel Port is free software: you can redistribute
 * it and/or modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation, either version 2 of the
 * License, or (at your option) any later version.
 *
 * The Ubicom32 Linux Kernel Port is distributed in the hope that it
 * will be useful, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
 * the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with the Ubicom32 Linux Kernel Port.  If not,
 * see <http://www.gnu.org/licenses/>.
 *
 * Ubicom32 implementation derived from (with many thanks):
 *   arch/m68knommu
 *   arch/blackfin
 *   arch/parisc
 */
/*
 * ubi32_eth.c
 * Ethernet driver for Ip5k/Ip7K
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/moduleparam.h>

#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/interrupt.h>

#include <linux/in.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/mii.h>
#include <linux/if_vlan.h>
#include <linux/ip.h>
#include <linux/tcp.h>
#include <linux/skbuff.h>
#include <asm/devtree.h>

// IGMP LOCK ISSUE 2009.12.03
#ifdef CONFIG_UBICOM_SWITCH_AR8316
#define ATHEROS_IGMP_LOCK 1
#endif

#ifdef ATHEROS_IGMP_LOCK
u32_t igmp_lock_enable = 0;
#endif

#define UBICOM32_USE_NAPI	/* define this to use NAPI instead of tasklet */
//#define UBICOM32_USE_POLLING	/* define this to use polling instead of interrupt */
#include "ubi32-eth.h"

/*
 * TODO:
 * mac address from flash
 * multicast filter
 * ethtool support
 * sysfs support
 * skb->nrfrag support
 * ioctl
 * monitor phy status
 */

extern int ubi32_ocm_skbuf_max, ubi32_ocm_skbuf, ubi32_ddr_skbuf;
static const char *eth_if_name[UBI32_ETH_NUM_OF_DEVICES] =
	{"eth_lan", "eth_wan"};
static struct net_device *ubi32_eth_devices[UBI32_ETH_NUM_OF_DEVICES] =
	{NULL, NULL};
static u8_t mac_addr[UBI32_ETH_NUM_OF_DEVICES][ETH_ALEN] = {
	{0x00, 0x03, 0x64, 'l', 'a', 'n'},
	{0x00, 0x03, 0x64, 'w', 'a', 'n'}};

#if (defined(CONFIG_ZONE_DMA) && defined(CONFIG_UBICOM32_OCM_FOR_SKB))
static inline struct sk_buff *ubi32_alloc_skb_ocm(struct net_device *dev, unsigned int length)
{
	return __dev_alloc_skb(length, GFP_ATOMIC | __GFP_NOWARN | __GFP_NORETRY | GFP_DMA);
}
#endif

static inline struct sk_buff *ubi32_alloc_skb(struct net_device *dev, unsigned int length)
{
	return __dev_alloc_skb(length, GFP_ATOMIC | __GFP_NOWARN);
}

static void ubi32_eth_vp_rxtx_enable(struct net_device *dev)
{
	struct ubi32_eth_private *priv = netdev_priv(dev);
	priv->regs->command = UBI32_ETH_VP_CMD_RX_ENABLE | UBI32_ETH_VP_CMD_TX_ENABLE;
	priv->regs->int_mask = (UBI32_ETH_VP_INT_RX | UBI32_ETH_VP_INT_TX);
	ubicom32_set_interrupt(priv->vp_int_bit);
}

static void ubi32_eth_vp_rxtx_stop(struct net_device *dev)
{
	struct ubi32_eth_private *priv = netdev_priv(dev);
	priv->regs->command = 0;
	priv->regs->int_mask = 0;
	ubicom32_set_interrupt(priv->vp_int_bit);

	/* Wait for graceful shutdown */
	while (priv->regs->status & (UBI32_ETH_VP_STATUS_RX_STATE | UBI32_ETH_VP_STATUS_TX_STATE));
}

/*
 * ubi32_eth_tx_done()
 */
static int ubi32_eth_tx_done(struct net_device *dev)
{
	struct ubi32_eth_private *priv;
	struct sk_buff *skb;
	volatile void *pdata;
	struct ubi32_eth_dma_desc *desc;
	u32_t 	count = 0;

	priv = netdev_priv(dev);

 	priv->regs->int_status &= ~UBI32_ETH_VP_INT_TX;
	while (priv->tx_tail != priv->regs->tx_out) {
		pdata = priv->regs->tx_dma_ring[priv->tx_tail];
		BUG_ON(pdata == NULL);

		skb = container_of((void *)pdata, struct sk_buff, cb);
		desc = (struct ubi32_eth_dma_desc *)pdata;
		if (unlikely(!(desc->status & UBI32_ETH_VP_TX_OK))) {
			dev->stats.tx_errors++;
		} else {
			dev->stats.tx_packets++;
			dev->stats.tx_bytes += skb->len;
		}
		dev_kfree_skb_any(skb);
		priv->regs->tx_dma_ring[priv->tx_tail] = NULL;
		priv->tx_tail = (priv->tx_tail + 1) & TX_DMA_RING_MASK;
		count++;
	}

	if (unlikely(priv->regs->status & UBI32_ETH_VP_STATUS_TX_Q_FULL)) {
		spin_lock(&priv->lock);
		if (priv->regs->status & UBI32_ETH_VP_STATUS_TX_Q_FULL) {
			priv->regs->status &= ~UBI32_ETH_VP_STATUS_TX_Q_FULL;
			netif_wake_queue(dev);
		}
		spin_unlock(&priv->lock);
	}
	return count;
}

#ifdef CONFIG_UBICOM_SWITCH_AR8316
#ifdef ATHEROS_IGMP_LOCK
// debug use only
#if 0
static void DEBUG_PRINTHEX(char *s,int len)
{
	int i;
	char *c;
	c = s;
	for(i=0;i<len;i++) {
		if( (i != 0) && ((i % 16)== 0 )) 
			printk("\n");
			
		printk("%02x ",*c++);
	}
	printk("\n");
}
#endif

static struct timer_list igmp_poll_timer;
u32_t igmp_poll_count;

//#define IGMP_TABLE_MAX_ITEM 128
#define IGMP_TABLE_MAX_ITEM 256 // consider ipv6
#define IGMP_TABLE_ADD_COUNT 300  // add how maybe count 




// only support 128 table
#define IGMP_TABLE_FLAG_USED   1
#define IGMP_TABLE_FLAG_FREE   0

struct igmp_table_t_item {
	unsigned char lastmac[4]; // ipv6 need 4 bytes
	unsigned char flag;
	u32_t timecount;		
	u32_t ipv6;
	struct igmp_table_t_item *next;
};

struct igmp_table_t {
	u16_t count;		
	struct igmp_table_t_item item[IGMP_TABLE_MAX_ITEM];	
};

struct igmp_table_t igmp_table_implememt;
struct igmp_table_t *igmp_table_resouce = &igmp_table_implememt;

struct igmp_table_t_item *igmp_list=NULL; // link-list

void athrs16_reg_write(uint32_t reg_addr, uint32_t reg_val);
uint32_t athrs16_reg_read(uint32_t reg_addr);


// only layer 2 Check
// if LAN need receive ipv6 specific multicast address, please add  
int ipv6_AllPort(char *mac)
{
    if( mac[0] == 0x33	&& mac[1] == 0x33) {
    }
    else {
    	// not ipv6 multicast
    	return 0;
    }
 
// FF:xx:xx:xx
    if( mac[2] == 0xff) {
    	return 1;
    }

// 00:00:00:xx    	
    else
    if( mac[2] == 0 && mac[3] == 0 && mac[4] == 0) {
    	return 1;
    } 

// 00:00:01:01    	
//ff0X::101 All Network Time Protocol (NTP) servers Available in all scopes 
//ff0x::108 Network Information Service 
    else
    if( mac[2] == 0 && mac[3] == 0 && mac[4] == 0x01 ) {
    	if( mac[5] == 0x01)
    	    return 1;
    	else if( mac[5] == 0x08)
    	    return 1;
    }


// 00:01:00:0x

//ff02::1:1 Link Name 2 (link-local) 
//ff02::1:2 All-dhcp-agents 2 (link-local) 
//ff02::1:3 Link-local Multicast Name Resolution 2 (link-local) 
//ff05::1:3 
    else
    if( mac[2] == 0 && mac[3] == 1 && mac[4] == 0x00) {
    	if( mac[5] == 0x01)
    	    return 1;
    	else if( mac[5] == 0x02)
    	    return 1;
    	else if( mac[5] == 0x03)
    	    return 1;
    }
    	
    	
    return 0;
    	
}



static void ATHRS_mcast(u16_t add,unsigned char *mac,int port)
{
	u32_t *value;
	u16_t *value16;
	u32_t tmp;
	
	value = (u32_t *) mac;

	athrs16_reg_write(0x0054,*value);
//	athrs16_reg_write(0x0058,0x000f0002);
	if( add) {
//		athrs16_reg_write(0x0058,0x000f0001); // only CPU Port
//		athrs16_reg_write(0x0058,0x000f0021); // only CPU Port
//		athrs16_reg_write(0x0058,0x000f0029); // only CPU Port
//		athrs16_reg_write(0x0058,0x000f003f); // only CPU Port
		if( ipv6_AllPort(mac) ) {
		athrs16_reg_write(0x0058,0x000f003f); // ALL Port
		port = -1; // debug use
		}
		else
		athrs16_reg_write(0x0058,0x000f0021 | (1 << port) ); // only CPU Port
	}
	else {
		athrs16_reg_write(0x0058,0x000f0000); // remove
	}
	
	value16 = (u16_t *) (unsigned char*) (mac+4);

	tmp = *value16;
	tmp <<= 16;
	
	if( add ) {
		tmp += 0x000a;
	}
	else {
		tmp += 0x000b;	
	}
	
	athrs16_reg_write(0x0050,tmp);

	printk(KERN_INFO "m_table count=%2d %s %08x %08x port=%d\n",igmp_table_resouce->count,add?"Add":"Del",*value,tmp,port);

//	printk(KERN_INFO "%x\n",	athrs16_reg_read(0x2c));
//	printk(KERN_INFO "%x\n",	athrs16_reg_read(0x3c));

//	printk("static mac 01 00 5e 01 01 01\n");
//	athrs16_reg_write(0x0054,0x01005e01);
//	athrs16_reg_write(0x0058,0x000f0002);
//	athrs16_reg_write(0x0050,0x0101000a);
}


static void igmp_add_new_poll(unsigned long morepoll)
{
	igmp_poll_timer.data = (unsigned long)morepoll;
	igmp_poll_timer.expires = jiffies + HZ;
	add_timer(&igmp_poll_timer);
	
}

static void igmp_poll(unsigned long arg)
{
//	printk("igmp_poll\n");
//	u16_t i;
	struct igmp_table_t_item *item;
	struct igmp_table_t_item **prev_item;

	igmp_poll_count++;
	
//	printk("%d\n",(int) sizeof(igmp_table->item[0]));

	item = igmp_list;
	prev_item = &igmp_list;
	
	while( item ) {

		if( item->timecount < igmp_poll_count) {			
			{
				char mac[6];
				if( item->ipv6 ) {
					mac[0] = 0x33;
					mac[1] = 0x33;
					mac[2] = item->lastmac[3];
					mac[3] = item->lastmac[0];
					mac[4] = item->lastmac[1];
					mac[5] = item->lastmac[2];
					ATHRS_mcast(0,mac,0);					
				}
				else {
					mac[0] = 0x01;
					mac[1] = 0x00;
					mac[2] = 0x5e;
					mac[3] = item->lastmac[0];
					mac[4] = item->lastmac[1];
					mac[5] = item->lastmac[2];
					ATHRS_mcast(0,mac,0);
				}
			}

			// remove list
			item->flag = IGMP_TABLE_FLAG_FREE;
			
			// avoid some issue, dec dec...
			if( igmp_table_resouce->count > 0) {
				igmp_table_resouce->count--;	
			}					
//			printk("count=%d\n",igmp_table_resouce->count);
			
			*prev_item = item->next;	
			// one poll only delete one
			break;			
		}

		prev_item = &(item->next);
		item = item->next;
	}
	
		

	
/*
			if( i > (igmp_table->count) ) {
				// don't move, only delete
			}
			else {
				// move last to here
				memcpy(	&igmp_table->item[i],&igmp_table->item[igmp_table->count-1],sizeof(igmp_table->item[i]));
			}
			igmp_table->count--;
			printk("count=%d\n",igmp_table->count);
*/
		if( igmp_lock_enable && arg ) {
	 		igmp_add_new_poll(arg);
		}	
}


static void IGMP_Init(void)
{
	// maybe alloc memory for...
	printk("IGMP_Init\n");

	igmp_poll_count = 0;
	
	init_timer(&igmp_poll_timer);
	igmp_poll_timer.function = igmp_poll;
	igmp_poll_timer.data = (unsigned long)0;
	igmp_poll_timer.expires = jiffies + HZ;
//	add_timer(&igmp_poll_timer);
	
}


// MLD
#define	SUPPORT_MLD 1


#ifdef SUPPORT_MLD

static void igmp_del_mac(char *dst_mac,int del)
{
//	printk("igmp_poll\n");
//	u16_t i;
	struct igmp_table_t_item *item;
	struct igmp_table_t_item **prev_item;

//	igmp_poll_count++;
	
//	printk("%d\n",(int) sizeof(igmp_table->item[0]));

	item = igmp_list;
	prev_item = &igmp_list;
	
	while( item ) {

		if( item->ipv6 && item->lastmac[3] == dst_mac[2]
			&& item->lastmac[0] == dst_mac[3]
			&& item->lastmac[1] == dst_mac[4]
			&& item->lastmac[2] == dst_mac[5] ) {
			
//		if( item->timecount < igmp_poll_count) {			

			// remove list
			item->flag = IGMP_TABLE_FLAG_FREE;
			
			// avoid some issue, dec dec...
			if( igmp_table_resouce->count > 0) {
				igmp_table_resouce->count--;	
			}					
//			printk("count=%d\n",igmp_table_resouce->count);
			if( del) 
			ATHRS_mcast(0,dst_mac,0);					

			
			*prev_item = item->next;	
			// one poll only delete one
			break;			
		}

		prev_item = &(item->next);
		item = item->next;
	}
	
}
	

static int igmp_Find_mac(char *dst_mac, int update)
{
//	printk("igmp_poll\n");
//	u16_t i;
	struct igmp_table_t_item *item;
//	struct igmp_table_t_item **prev_item;

//	igmp_poll_count++;
	
//	printk("%d\n",(int) sizeof(igmp_table->item[0]));

	item = igmp_list;
//	prev_item = &igmp_list;
	
	while( item ) {

		if( item->ipv6 && item->lastmac[3] == dst_mac[2]
			&& item->lastmac[0] == dst_mac[3]
			&& item->lastmac[1] == dst_mac[4]
			&& item->lastmac[2] == dst_mac[5] ) {
			
//		if( item->timecount < igmp_poll_count) {			

			// remove list
//			item->flag = IGMP_TABLE_FLAG_FREE;
			
			// avoid some issue, dec dec...
//			if( igmp_table_resouce->count > 0) {
//				igmp_table_resouce->count--;	
//			}					
//			printk("count=%d\n",igmp_table_resouce->count);
			
//			*prev_item = item->next;	
			// one poll only delete one
			
			if( update ) {
			  	item->timecount = igmp_poll_count + IGMP_TABLE_ADD_COUNT ;				
			}
			
			return 1;
			break;			
		}

//		prev_item = &(item->next);
		item = item->next;
	}
	
	
	return 0;
}

	
#endif


#ifdef SUPPORT_MLD

struct ipv6hdr {
#if defined(__LITTLE_ENDIAN_BITFIELD)
	__u8			priority:4,
				version:4;
#elif defined(__BIG_ENDIAN_BITFIELD)
	__u8			version:4,
				priority:4;
#else
#error	"Please fix <asm/byteorder.h>"
#endif
	__u8			flow_lbl[3];

	__be16			payload_len;
	__u8			nexthdr;
	__u8			hop_limit;

	struct	in6_addr	saddr;
	struct	in6_addr	daddr;
};

struct ipv6_opt_hdr {
	__u8 		nexthdr;
	__u8 		hdrlen;
	/* 
	 * TLV encoded option data follows.
	 */
} __attribute__ ((packed));	/* required for some archs */

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

#define MLD2_MODE_IS_INCLUDE	1
#define MLD2_MODE_IS_EXCLUDE	2
#define MLD2_CHANGE_TO_INCLUDE	3
#define MLD2_CHANGE_TO_EXCLUDE	4
#define MLD2_ALLOW_NEW_SOURCES	5
#define MLD2_BLOCK_OLD_SOURCES	6


#define IGMP_SNOOP_CMD_JOIN 	1
#define IGMP_SNOOP_CMD_LEAVE 	2


static void IGMP_Check(unsigned char *mac,u16_t len, u16_t vlan,int direct_insert);

static void igmpv6_join_leave(int cmd,char *srcAddr,char *ipv6_src,char *ipv6_group,int tx_mode,int port)
{
//	struct ieee80211vap *vap = ni->ni_vap;
//	int			cmd = IGMP_SNOOP_CMD_OTHER;
	long bypass;
	char *mac;

#define IEEE80211_ADDR_LEN 	6
	u_int8_t	groupAddrL2[IEEE80211_ADDR_LEN];

//	u_int32_t	groupAddr = 0;
//	u_int32_t	saddr = 0;
	
//	cmd = IGMP_SNOOP_CMD_JOIN;
	//cmd = IGMP_SNOOP_CMD_LEAVE;

	groupAddrL2[0] = 0x33;
	groupAddrL2[1] = 0x33;
	groupAddrL2[2] = ipv6_group[12];
	groupAddrL2[3] = ipv6_group[13];
	groupAddrL2[4] = ipv6_group[14];
	groupAddrL2[5] = ipv6_group[15];



	mac = groupAddrL2;
	bypass = 0;

#if 1
//		if( mac[0] == 0x33 && mac[1] == 0x33 )
				if( mac[2] == 00 && mac[3] == 00 && mac[4] == 00 ) {
					if( mac[5] == 1) 
						bypass = 1;		
					if( mac[5] == 2) 
						bypass = 1;		
					if( mac[5] == 16) 
						bypass = 1;		
				}
#endif

	if( bypass)
		return;

//	ieee80211_me_SnoopListUpdate(vap, cmd, &groupAddrL2[0], srcAddr, ni, groupAddr, saddr,ipv6_group,ipv6_src,0);

	if( cmd == IGMP_SNOOP_CMD_LEAVE) {
		
		
		// try 
		
//		if( ipv6_group[1] == 0x3e) {
//			ATHRS_mcast(0,groupAddrL2,0);					
//			ATHRS_mcast(1,groupAddrL2,2);	
			igmp_del_mac(groupAddrL2,1);
//			ATHRS_mcast(0,groupAddrL2,0);					
//			printk("del\n");

// try
//			ATHRS_mcast(1,groupAddrL2,0);	
//		}
		
	}
	else {
//		if( ipv6_group[1] == 0x3e) {
			
			if( igmp_Find_mac(groupAddrL2,1) ) {
				
			} 
			else {
				//ATHRS_mcast(1,groupAddrL2,0);					
//			printk("add\n");
				if( tx_mode) {
					IGMP_Check(groupAddrL2,6,0,1); // this is insert group mac
				}
				else {
					IGMP_Check(groupAddrL2,6,port,1); // this is insert group mac
				}
			
			}
//			ATHRS_mcast(1,groupAddrL2,2);	

//		}
		
	}


}



static void add_igmpv6(char *srcAddr,char *ipv6_src,char *ipv6_group,int tx_mode,int port)
{
	igmpv6_join_leave(IGMP_SNOOP_CMD_JOIN,srcAddr,ipv6_src,ipv6_group,tx_mode,port);
//	printk("add_igmpv6\n");

}

static void del_igmpv6(char *srcAddr,char *ipv6_src,char *ipv6_group,int tx_mode,int port)
{
	igmpv6_join_leave(IGMP_SNOOP_CMD_LEAVE,srcAddr,ipv6_src,ipv6_group,tx_mode,port);
//	printk("del_igmpv6\n");
}

#endif






#ifdef SUPPORT_MLD

#define	ETHER_ADDR_LEN		6	/* length of an Ethernet address */

struct	ether_header {
	u_char	ether_dhost[ETHER_ADDR_LEN];
	u_char	ether_shost[ETHER_ADDR_LEN];
	u_short	ether_type;
} __packed;



static int igmp6_parse(unsigned char *mac,u16_t len, u16_t vlan,int tx_mode)
{
	// must ipv6 packet.
	// mac[12] == 0x86
	// mac[13] == 0xdd
	
	struct ether_header *eh = (struct ether_header *) mac;
	struct ipv6_opt_hdr *opt_hdr;
	char *opt;
	struct mld2_report	*report;
	unsigned char *ipv6_dst;
	unsigned char *ipv6_src;



				struct ipv6hdr *ip6h; // = (struct ipv6hdr *) (mac + sizeof (struct ether_header));
				
				if( tx_mode ) {
					ip6h = (struct ipv6hdr *) (mac + sizeof (struct ether_header));
				}
				else {
					ip6h = (struct ipv6hdr *) (mac + sizeof (struct ether_header) + 6);
				}
				

				if(	ip6h->nexthdr != 0) {
					goto exit_1;
				}

				if( tx_mode ) {
					opt_hdr = (struct ipv6_opt_hdr *) (mac + sizeof (struct ether_header) 
						+ sizeof(*ip6h) );
				}
				else {
					opt_hdr = (struct ipv6_opt_hdr *) (mac + sizeof (struct ether_header) 
						+ sizeof(*ip6h) + 6);
				}

				// check ICMPV6(0x3a)
				if( opt_hdr->nexthdr != 0x3a) {
					goto exit_1;
				}				

				opt = (char *) opt_hdr;
				// check MLD 
				// 05 02 00 00 
				if( opt[2] == 5 && opt[3]==2 && opt[4] == 0 && opt[5] == 0) {
//					printk("found MLD\n");
				}
				else {
					goto exit_1;
				}

				opt += (opt[1]) * 8;
				opt += 8;

// maybe check  (opt - skb->data) > (skb->len) 

				// this is MLD data
//				printk("%d \n",opt[0]);


				report = (struct mld2_report *) opt;		



		ipv6_src = ip6h->saddr.in6_u.u6_addr8;
		ipv6_dst = ip6h->daddr.in6_u.u6_addr8;

		//ipv6_dst = eh->ether_dhost;
		//ipv6_src = eh->ether_shost;

// v2
		if( report->type == ICMPV6_MLD2_REPORT) {
				int count;
				char *group;
				count = report->ngrec;
				
				if( count >= 0) {
					struct mld2_grec *grec;					
					grec = (struct mld2_grec *) report->grec;
					
					while( count) {
						char *s;
						if( grec->grec_auxwords != 0) {
//							dprintf("error format\n");
							goto exit_1;
							//goto next;
							break;
							//continue;
						}
						
						group = (char *) &grec->grec_mca;
		
//			dprintf("v2[%s]%d\n",str,status);

//			dprintf("report:%d ",report->type);
//			DEBUG_PRINTHEX(group,16);
				
						if( grec->grec_type == MLD2_CHANGE_TO_EXCLUDE || 
							grec->grec_type == MLD2_MODE_IS_EXCLUDE
							) {
							//join
							add_igmpv6(eh->ether_shost,ipv6_src,group,tx_mode,mac[13] & 0x0f);
							//add_join_leave(1,mrouter_s6,phy_eth,group);
							//add_del_cache_table(1,(char *) group);
						}
						else if( grec->grec_type == MLD2_CHANGE_TO_INCLUDE ||
							grec->grec_type == MLD2_MODE_IS_INCLUDE ) {
							// leave
							del_igmpv6(eh->ether_shost,ipv6_src,group,tx_mode,mac[13] & 0x0f);
							//add_join_leave(0,mrouter_s6,phy_eth,group);
							//add_del_cache_table(0,(char *) group);
						}
					
						s = (char *) grec;
						s += sizeof( *grec); // - 16; // dec last ipv6
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

//			dprintf("v1[%s]%d\n",str,status);

//			dprintf("report:%d ",report->type);
			group = (char *) report->grec;
//DEBUG_PRINTHEX(group,16);

#if 0
			if( memcmp((char *) report->grec,host_ff02__1,16)==0 ) {					
				  //dprintf("found ff02::1\n");
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
				
			
				add_igmpv6(eh->ether_shost,ipv6_src,group,tx_mode,mac[13] & 0x0f);
				//add_igmpv6(ipv6_src,group);
				//add_join_leave(1,mrouter_s6,phy_eth,group);
				//add_del_cache_table(1,(char *) group);
			}
			else {
				// leave
//				add_join_leave(0,mrouter_s6,phy_br0,group);
				del_igmpv6(eh->ether_shost,ipv6_src,group,tx_mode,mac[13] & 0x0f);

				//add_join_leave(0,mrouter_s6,phy_eth,group);
				//add_del_cache_table(0,(char *) group);
			}
			
		
		}









exit_1:
	return 0;
			



	
}




#endif



// TX Use
//  1 == forward
static int IGMP_Tx_Check(unsigned char *mac,u16_t len, u16_t vlan)
{

//	u16_t i;
	
	struct igmp_table_t_item *item;

	if( vlan != 2) {
//	DEBUG_PRINTHEX(mac,32);
		goto pass;
	}	



// IPV6 check
	if( mac[12] == 0x86 &&  mac[13] == 0xdd) {
		
		igmp6_parse(mac,len,vlan,1);
		
		
		if( ipv6_AllPort(mac) ) {
			goto pass;
		}
		
#if 0		
		long bypass;
		
		bypass = 0;
		
		if( mac[0] == 0x33 && mac[1] == 0x33 )
				if( mac[2] == 00 && mac[3] == 00 && mac[4] == 00 ) {
					if( mac[5] == 1) 
						bypass = 1;		
					if( mac[5] == 2) 
						bypass = 1;		
					if( mac[5] == 16) 
						bypass = 1;		
				}
			
		if( bypass ) {
		}
		else {					
			igmp6_parse(mac,len,vlan);
		}
#endif
		
	}	





	

#if 0	
	if( mac[17] != 0x02) {
		// this is wan packet 
		goto pass;
//		return;
	}
#endif
	
//	DEBUG_PRINTHEX(mac,32);
//	printk("lan IGMP\n");
	
	//skip 224.0.0.1
	//skip 224.0.0.2
	//skip 224.0.0.22
// disable this 	
#if 1	
	if( mac[0] == 0x01) // ipv4 multicast mac
	if( ( (mac[3] ^ 0) | (mac[4] ^ 0) ) == 0) {
		if( (mac[5] ^ 1 ) == 0) {
			goto pass;
//			return;
		}
		else if( (mac[5] ^ 2 ) == 0) {
			goto pass;
//			return;
		}
		else if( (mac[5] ^ 22 ) == 0) {
			goto pass;
//			return;
		}
		else if( (mac[5] ^ 0x16 ) == 0) {
			goto pass;
//			return;
		}
	}
#endif	
	
	
	// check mac[3]
	
// look at existing mac

	item = igmp_list;
	while( item ) {

		if( item->ipv6 ) {
			if( ( (mac[3] ^ item->lastmac[0]) |
				  (mac[4] ^ item->lastmac[1]) |

				  (mac[2] ^ item->lastmac[3]) |

				  (mac[5] ^ item->lastmac[2]) ) == 0) {
				  	// find 
			  		// update timer
				  	//item->timecount = igmp_poll_count + IGMP_TABLE_ADD_COUNT ;
				  	// do nothing
				  	goto pass;
//				  	return; //
			}  					

		}
		else		
		if( ( (mac[3] ^ item->lastmac[0]) |
			  (mac[4] ^ item->lastmac[1]) |
			  (mac[5] ^ item->lastmac[2]) ) == 0) {
			  	// find 
			  	// update timer
			  	//item->timecount = igmp_poll_count + IGMP_TABLE_ADD_COUNT ;
			  	// do nothing
			  	goto pass;
//			  	return; //
		}  					
		
		item = item->next;
	}
	
	return 0;
pass:	
	return 1;
}


// RX Use
// direct_insert = 1 -> vlan = port
static void IGMP_Check(unsigned char *mac,u16_t len, u16_t vlan,int direct_insert)
{
//	struct igmp_table_t *igmp_table;
	u16_t i;
	u32 port;
	
	struct igmp_table_t_item *item;

	if( direct_insert ) 
		goto insert;
//	DEBUG_PRINTHEX(mac,32);


// try stop ipv6 multicast snoppy
//	if( mac[0] == 0x33) /
//	/	return;


#if 0
	if( vlan != 2)  {
		return;
	}
#endif
#if 1
	if( mac[17] != 0x02) {
		return;
	}
#endif


// rx need add 6 bytes,  control issue 

	if( mac[12+6] == 0x86 &&  mac[13+6] == 0xdd) {
	   if( mac[2] == 0 && mac[3] == 0 && mac[4] == 0 && mac[5] == 0x16) {
//	   	printk("go\n");	
				igmp6_parse(mac,len,vlan,0);
		}
	}


//	DEBUG_PRINTHEX(mac,32);
	
//	DEBUG_PRINTHEX(mac,32);
//	printk("lan IGMP\n");
	
	//skip 224.0.0.1
	//skip 224.0.0.2
	//skip 224.0.0.22
// disable	
#if 0	
	if( ( (mac[3] ^ 0) | (mac[4] ^ 0) ) == 0) {
		if( (mac[5] ^ 1 ) == 0) {
			return;
		}
		else if( (mac[5] ^ 2 ) == 0) {
			return;
		}
		else if( (mac[5] ^ 22 ) == 0) {
			return;
		}
//		else if( (mac[5] ^ 0x16 ) == 0) {
//			return;
//		}
	}
#endif	
	
	
	// check mac[3]
	
// look at existing mac

	item = igmp_list;
	while( item ) {
		
		if( item->ipv6 ) {
			if( ( (mac[3] ^ item->lastmac[0]) |
				  (mac[4] ^ item->lastmac[1]) |
				  
				//item->lastmac[3] = mac[2];
				  (mac[2] ^ item->lastmac[3]) |
				  
				  (mac[5] ^ item->lastmac[2]) ) == 0) {
				  	// find 
			  		// update timer
			  		item->timecount = igmp_poll_count + IGMP_TABLE_ADD_COUNT ;
			  		// do nothing
			  		return; //
			}  					
			
		}		
		else 

		if( ( (mac[3] ^ item->lastmac[0]) |
			  (mac[4] ^ item->lastmac[1]) |
			  (mac[5] ^ item->lastmac[2]) ) == 0) {
			  	// find 
			  	// update timer
			  	item->timecount = igmp_poll_count + IGMP_TABLE_ADD_COUNT ;
			  	// do nothing
			  	return; //
		}  					
		
		item = item->next;
	}


insert:
	// not find in table

// search free memory	
	
	if( igmp_table_resouce->count >=  IGMP_TABLE_MAX_ITEM ) {
		printk("table overflow %d\n",igmp_table_resouce->count);
		return;
	}

// find free 
	for(i=0;i<IGMP_TABLE_MAX_ITEM;i++) {
		if( igmp_table_resouce->item[i].flag == IGMP_TABLE_FLAG_FREE) {
			break;
		}		
	}
	
	if( i == IGMP_TABLE_MAX_ITEM) {
		//
		printk("no free table\n");
		return;
	}

	// make list
	
//	printk("i=%d\n",i);
	
	item = &igmp_table_resouce->item[i];
		
	
	item->lastmac[0] = mac[3];
	item->lastmac[1] = mac[4];
	item->lastmac[2] = mac[5];
	item->flag = IGMP_TABLE_FLAG_USED;
	
	item->ipv6 = 0; // should set
	if( mac[0] == 0x33) {
		item->ipv6 = 1;
		item->lastmac[3] = mac[2];
	}
		
	
	item->timecount = igmp_poll_count + IGMP_TABLE_ADD_COUNT ;
	item->next = NULL;

	igmp_table_resouce->count++;
	
	
	// make list
#if 0	
	if( igmp_list) {
		igmp_list->next = item;
	}
	else {
		igmp_list = item;
	}
#else
	item->next = igmp_list;
	igmp_list = item;

#endif
	

	if( direct_insert ) 
		port = vlan; // 
	else
	
	// guest port value
	port = mac[12+1] & 0xf;

//printk("port=%d\n",port);
	
	ATHRS_mcast(1,mac,port);	
//	printk("count=%d\n",igmp_table_resouce->count);
	
	
}

#endif // ATHEROS_IGMP_LOCK
#endif // CONFIG_UBICOM_SWITCH_AR8316


/*
 * ubi32_eth_receive()
 *	To avoid locking overhead, this is called only
 *	by tasklet when not using NAPI, or
 *	by NAPI poll when using NAPI.
 *	return number of frames processed
 */
static int ubi32_eth_receive(struct net_device *dev, int quota)
{
	struct ubi32_eth_private *priv = netdev_priv(dev);
	unsigned short rx_in = priv->regs->rx_in;
	struct sk_buff *skb;
	struct ubi32_eth_dma_desc *desc = NULL;
	volatile void *pdata;

	int extra_reserve_adj;
	int extra_alloc = UBI32_ETH_RESERVE_SPACE + UBI32_ETH_TRASHED_MEMORY;
	int replenish_cnt, count = 0;
	int replenish_max = RX_DMA_MAX_QUEUE_SIZE;
#if (defined(CONFIG_ZONE_DMA) && defined(CONFIG_UBICOM32_OCM_FOR_SKB))
	if (likely(dev == ubi32_eth_devices[0]))
		replenish_max = min(ubi32_ocm_skbuf_max, RX_DMA_MAX_QUEUE_SIZE);;
#endif
#ifdef CONFIG_UBICOM_SWITCH_AR8316
	if (likely(dev == ubi32_eth_devices[0])) {
		BUG_ON((priv->rx_tail != priv->regs->rx_out) && !priv->vlgrp);
	}
#endif

	if (unlikely(rx_in == priv->regs->rx_out))
		priv->vp_stats.rx_q_full_cnt++;

	priv->regs->int_status &= ~UBI32_ETH_VP_INT_RX;
#ifdef CONFIG_UBICOM_SWITCH_AR8316
	if (likely(dev == ubi32_eth_devices[0])) {
		while (priv->rx_tail != priv->regs->rx_out) {
			if (unlikely(count == quota)) {
				/* There is still frame pending to be processed */
				priv->vp_stats.rx_throttle++;
				break;
			}

			pdata = priv->regs->rx_dma_ring[priv->rx_tail];
			BUG_ON(pdata == NULL);

			desc = (struct ubi32_eth_dma_desc *)pdata;
			skb = container_of((void *)pdata, struct sk_buff, cb);
			count++;
			priv->regs->rx_dma_ring[priv->rx_tail] = NULL;
			priv->rx_tail = ((priv->rx_tail + 1) & RX_DMA_RING_MASK);

			/*
			 * Check only RX_OK bit here.
			 * The rest of status word is used as timestamp
			 */
			if (unlikely(!(desc->status & UBI32_ETH_VP_RX_OK))) {
				dev->stats.rx_errors++;
				dev_kfree_skb_any(skb);
				continue;
			}


/// try
#ifdef ATHEROS_IGMP_LOCK
//		printk("--> %x %d\n",(char *)dev,(long) desc->data_len);
		
		//
		if( igmp_lock_enable ) 	{
			unsigned char *mac;
			mac = (char *)desc->data_pointer;
			if( (*mac & 1) && (*mac != 0xff) ) {
				
				
//		if( mac[0] == 0x33)		{
//			printk("rx");
//		DEBUG_PRINTHEX(desc->data_pointer,32);
//		}
				// we know IGMP  use mac from 01 00 5e 
				//ipv6 use mac form 33 33,and avoid ff00::1 ff00::2 drop with device
//				if( ((mac[0] == 0x01) && (mac[1]==0x00) && (mac[2]==0x5e) ) || ((mac[0] == 0x33) && (mac[1]==0x33) && (mac[5] !=0x01) && (mac[5] != 0x02) )) {			
  // make 0x33 0x33 00 00 00 01  
  // 00 00 00 is need check

				if( ((mac[0] == 0x01) && (mac[1]==0x00) && (mac[2]==0x5e) ) || 
					((mac[0] == 0x33) && (mac[1]==0x33) )  ) {			
					//printk("%d\n",skb->vlan_tci);
					//DEBUG_PRINTHEX(desc->data_pointer,32);
					IGMP_Check(mac,desc->data_len,skb->vlan_tci,0);
				}
			}
		}
#endif		


			asm volatile (
			"	move.2	%0, 16(%1)	\n\t"
			"	move.2	16(%1), 10(%1)	\n\t"
			"	move.2	14(%1), 8(%1)	\n\t"
			"	move.2	12(%1), 6(%1)	\n\t"
			"	move.2	10(%1), 4(%1)	\n\t"
			"	move.2	8(%1), 2(%1)	\n\t"
			"	move.2	6(%1), 0(%1)	\n\t"
			: "=m" (skb->vlan_tci)
			: "a" (desc->data_pointer)
			: "memory"
			);
			skb_put(skb, desc->data_len - 2);	// Remove AR8316 CPU header
			skb_pull(skb, VLAN_HLEN);		// Remove VLAN tag
			skb->dev = dev;
			skb->protocol = eth_type_trans(skb, dev);
			skb->ip_summed = CHECKSUM_NONE;
			dev->stats.rx_bytes += skb->len;
			dev->stats.rx_packets++;

			vlan_hwaccel_receive_skb(skb,
				priv->vlgrp, skb->vlan_tci);
			continue;
		}

		goto do_replenish;
	}
#endif

	while (priv->rx_tail != priv->regs->rx_out) {
		if (unlikely(count == quota)) {
			/* There is still frame pending to be processed */
			priv->vp_stats.rx_throttle++;
			break;
		}

		pdata = priv->regs->rx_dma_ring[priv->rx_tail];
		BUG_ON(pdata == NULL);

		desc = (struct ubi32_eth_dma_desc *)pdata;
		skb = container_of((void *)pdata, struct sk_buff, cb);
		count++;
		priv->regs->rx_dma_ring[priv->rx_tail] = NULL;
		priv->rx_tail = ((priv->rx_tail + 1) & RX_DMA_RING_MASK);

		/*
		 * Check only RX_OK bit here.
		 * The rest of status word is used as timestamp
		 */
		if (unlikely(!(desc->status & UBI32_ETH_VP_RX_OK))) {
			dev->stats.rx_errors++;
			dev_kfree_skb_any(skb);
			continue;
		}

		skb_put(skb, desc->data_len);
		skb->dev = dev;
		skb->protocol = eth_type_trans(skb, dev);
		skb->ip_summed = CHECKSUM_NONE;
		dev->stats.rx_bytes += skb->len;
		dev->stats.rx_packets++;
#ifndef UBICOM32_USE_NAPI
		netif_rx(skb);
#else
		netif_receive_skb(skb);
#endif
	}

do_replenish:
	/* fill in more descripor for VP*/
	replenish_cnt =  replenish_max -
		((RX_DMA_RING_SIZE + rx_in - priv->rx_tail) & RX_DMA_RING_MASK);
	if (replenish_cnt > 0) {
#if (defined(CONFIG_ZONE_DMA) && defined(CONFIG_UBICOM32_OCM_FOR_SKB))
		/*
		 * black magic for perforamnce:
		 *   Try to allocate skb from OCM only for first Ethernet I/F.
		 *   Also limit number of RX buffers to 21 due to limited OCM.
		 */
		if (likely(dev == ubi32_eth_devices[0])) {
			do {
				skb = ubi32_alloc_skb_ocm(dev, RX_BUF_SIZE + extra_alloc);
				if (!skb) {
					break;
				}
				/* set up dma descriptor */
				ubi32_ocm_skbuf++;
				desc = (struct ubi32_eth_dma_desc *)skb->cb;
				extra_reserve_adj =
					((u32)skb->data + UBI32_ETH_RESERVE_SPACE + ETH_HLEN) &
					(CACHE_LINE_SIZE - 1);
				skb_reserve(skb, UBI32_ETH_RESERVE_SPACE - extra_reserve_adj);
				desc->data_pointer = skb->data;
#ifdef CONFIG_UBICOM_SWITCH_AR8316				// must be ubi32_eth_devices[0] already
				desc->data_pointer -= 2;	// make room for AR8316 CPU header
#endif
				desc->buffer_len = RX_BUF_SIZE + UBI32_ETH_TRASHED_MEMORY;
				desc->data_len = 0;
				desc->status = 0;
				priv->regs->rx_dma_ring[rx_in] = desc;
				rx_in = (rx_in + 1) & RX_DMA_RING_MASK;
			} while (--replenish_cnt > 0);
		}
#endif

		while (replenish_cnt-- > 0) {
			skb = ubi32_alloc_skb(dev, RX_BUF_SIZE + extra_alloc);
			if (!skb) {
				priv->vp_stats.rx_alloc_err++;
				break;
			}
			/* set up dma descriptor */
			ubi32_ddr_skbuf++;
			desc = (struct ubi32_eth_dma_desc *)skb->cb;
			extra_reserve_adj =
				((u32)skb->data + UBI32_ETH_RESERVE_SPACE + ETH_HLEN) &
				(CACHE_LINE_SIZE - 1);
			skb_reserve(skb, UBI32_ETH_RESERVE_SPACE - extra_reserve_adj);
			desc->data_pointer = skb->data;
#ifdef CONFIG_UBICOM_SWITCH_AR8316				// make room for AR8316 CPU header
			desc->data_pointer -= (dev == ubi32_eth_devices[0]) ? 2 : 0;
#endif
			desc->buffer_len = RX_BUF_SIZE + UBI32_ETH_TRASHED_MEMORY;
			desc->data_len = 0;
			desc->status = 0;
			priv->regs->rx_dma_ring[rx_in] = desc;
			rx_in = (rx_in + 1) & RX_DMA_RING_MASK;
		}

		wmb();
		priv->regs->rx_in = rx_in;
		ubicom32_set_interrupt(priv->vp_int_bit);
	}

	if (likely(count > 0)) {
		dev->last_rx = jiffies;
	}
	return count;
}

#ifdef UBICOM32_USE_NAPI
static int ubi32_eth_napi_poll(struct napi_struct *napi, int budget)
{
	struct ubi32_eth_private *priv = container_of(napi, struct ubi32_eth_private, napi);
	struct net_device *dev = priv->dev;
	u32_t count;

	if (priv->tx_tail != priv->regs->tx_out) {
		ubi32_eth_tx_done(dev);
	}

	count = ubi32_eth_receive(dev, budget);

	if (count < budget) {
		netif_rx_complete(dev, napi);
		priv->regs->int_mask |= (UBI32_ETH_VP_INT_RX | UBI32_ETH_VP_INT_TX);
		if ((priv->rx_tail != priv->regs->rx_out) || (priv->tx_tail != priv->regs->tx_out)) {
			if (netif_rx_reschedule(dev, napi)) {
				priv->regs->int_mask = 0;
			}
		}
	}
	return count;
}

#else
static void ubi32_eth_do_tasklet(unsigned long arg)
{
	struct net_device *dev = (struct net_device *)arg;
	struct ubi32_eth_private *priv = netdev_priv(dev);

	if (priv->tx_tail != priv->regs->tx_out) {
		ubi32_eth_tx_done(dev);
	}

	/* always call receive to process new RX frame as well as replenish RX buffers */
	ubi32_eth_receive(dev, UBI32_RX_BOUND);

	priv->regs->int_mask |= (UBI32_ETH_VP_INT_RX | UBI32_ETH_VP_INT_TX);
	if ((priv->rx_tail != priv->regs->rx_out) || (priv->tx_tail != priv->regs->tx_out)) {
		priv->regs->int_mask = 0;
		tasklet_schedule(&priv->tsk);
	}
}
#endif

#if defined(UBICOM32_USE_POLLING)
static struct timer_list eth_poll_timer;

static void ubi32_eth_poll(unsigned long arg)
{
	struct net_device *dev;
	struct ubi32_eth_private *priv;
	int i;

	for (i = 0; i < UBI32_ETH_NUM_OF_DEVICES; i++) {
		dev = ubi32_eth_devices[i];
		if (dev && (dev->flags & IFF_UP)) {
			priv = netdev_priv(dev);
#ifdef UBICOM32_USE_NAPI
			netif_rx_schedule(dev, &priv->napi);
#else
			tasklet_schedule(&priv->tsk);
#endif
		}
	}

	eth_poll_timer.expires = jiffies + 2;
	add_timer(&eth_poll_timer);
}

#else
static irqreturn_t ubi32_eth_interrupt(int irq, void *dev_id)
{
	struct ubi32_eth_private *priv;

	struct net_device *dev = (struct net_device *)dev_id;
	BUG_ON(irq != dev->irq);

	priv = netdev_priv(dev);
	if (unlikely(!(priv->regs->int_status & priv->regs->int_mask))) {
		return IRQ_NONE;
	}

	/*
	 * Disable port interrupt
	 */
#ifdef UBICOM32_USE_NAPI
	if (netif_rx_schedule_prep(dev, &priv->napi)) {
		priv->regs->int_mask = 0;
		__netif_rx_schedule(dev, &priv->napi);
	}
#else
	priv->regs->int_mask = 0;
	tasklet_schedule(&priv->tsk);
#endif
	return IRQ_HANDLED;
}
#endif

/*
 * ubi32_eth_open
 */
static int ubi32_eth_open(struct net_device *dev)
{
	struct ubi32_eth_private *priv = netdev_priv(dev);
	int err;

	printk(KERN_INFO "eth open %s\n",dev->name);
#ifndef UBICOM32_USE_POLLING
	/* request_region() */
	err = request_irq(dev->irq, ubi32_eth_interrupt, IRQF_DISABLED, dev->name, dev);
	if (err) {
		printk(KERN_WARNING "fail to request_irq %d\n",err);
		 return -ENODEV;
	}
#endif
#ifdef  UBICOM32_USE_NAPI
	napi_enable(&priv->napi);
#else
	tasklet_init(&priv->tsk, ubi32_eth_do_tasklet, (unsigned long)dev);
#endif

	/* call receive to supply RX buffers */
	ubi32_eth_receive(dev, RX_DMA_MAX_QUEUE_SIZE);

	/* check phy status and call netif_carrier_on */
	ubi32_eth_vp_rxtx_enable(dev);
	netif_start_queue(dev);
	return 0;
}

static int ubi32_eth_close(struct net_device *dev)
{
	struct ubi32_eth_private *priv = netdev_priv(dev);
	volatile void *pdata;
	struct sk_buff *skb;

#ifndef UBICOM32_USE_POLLING
	free_irq(dev->irq, dev);
#endif
	netif_stop_queue(dev); /* can't transmit any more */
#ifdef UBICOM32_USE_NAPI
	napi_disable(&priv->napi);
#else
	tasklet_kill(&priv->tsk);
#endif
	ubi32_eth_vp_rxtx_stop(dev);

	/*
	 * RX clean up
	 */
	while (priv->rx_tail != priv->regs->rx_in) {
		pdata = priv->regs->rx_dma_ring[priv->rx_tail];
		skb = container_of((void *)pdata, struct sk_buff, cb);
		priv->regs->rx_dma_ring[priv->rx_tail] = NULL;
		dev_kfree_skb_any(skb);
		priv->rx_tail = ((priv->rx_tail + 1) & RX_DMA_RING_MASK);
	}
	priv->regs->rx_in = 0;
	priv->regs->rx_out = priv->regs->rx_in;
	priv->rx_tail = priv->regs->rx_in;

	/*
	 * TX clean up
	 */
	BUG_ON(priv->regs->tx_out != priv->regs->tx_in);
	ubi32_eth_tx_done(dev);
	BUG_ON(priv->tx_tail != priv->regs->tx_in);
	priv->regs->tx_in = 0;
	priv->regs->tx_out = priv->regs->tx_in;
	priv->tx_tail = priv->regs->tx_in;

	return 0;
}

/*
 * ubi32_eth_set_config
 */
static int ubi32_eth_set_config(struct net_device *dev, struct ifmap *map)
{
	/* if must to down to config it */
	printk(KERN_INFO "set_config %x\n", dev->flags);
	if (dev->flags & IFF_UP)
		return -EBUSY;

	/* I/O and IRQ can not be changed */
	if (map->base_addr != dev->base_addr) {
		printk(KERN_WARNING "%s: Can't change I/O address\n", dev->name);
		return -EOPNOTSUPP;
	}

#ifndef UBICOM32_USE_POLLING
	if (map->irq != dev->irq) {
		printk(KERN_WARNING "%s: Can't change IRQ\n", dev->name);
		return -EOPNOTSUPP;
	}
#endif

	/* ignore other fields */
	return 0;
}

static int ubi32_eth_start_xmit(struct sk_buff *skb, struct net_device *dev)
{
	struct ubi32_eth_private *priv = netdev_priv(dev);
	struct ubi32_eth_dma_desc *desc = NULL;
	unsigned short space, tx_in;
#ifdef CONFIG_UBICOM_SWITCH_AR8316
#ifdef ATHEROS_IGMP_LOCK
	unsigned char *mac;
#endif
#endif

	tx_in = priv->regs->tx_in;

	dev->trans_start = jiffies; /* save the timestamp */
	space = TX_DMA_RING_MASK - ((TX_DMA_RING_SIZE + tx_in - priv->tx_tail) & TX_DMA_RING_MASK);

	if (unlikely(space == 0)) {
		if (!(priv->regs->status & UBI32_ETH_VP_STATUS_TX_Q_FULL)) {
			spin_lock(&priv->lock);
			if (!(priv->regs->status & UBI32_ETH_VP_STATUS_TX_Q_FULL)) {
				priv->regs->status |= UBI32_ETH_VP_STATUS_TX_Q_FULL;
				priv->vp_stats.tx_q_full_cnt++;
				netif_stop_queue(dev);
			}
			spin_unlock(&priv->lock);
		}

		/* give both HW and this driver an extra trigger */
		priv->regs->int_mask |= UBI32_ETH_VP_INT_TX;
#ifndef UBICOM32_USE_POLLING
		ubicom32_set_interrupt(dev->irq);
#endif
		ubicom32_set_interrupt(priv->vp_int_bit);

		return NETDEV_TX_BUSY;
	}

	desc = (struct ubi32_eth_dma_desc *)skb->cb;
	desc->data_pointer = skb->data;
	desc->data_len = skb->len;

// try
//	DEBUG_PRINTHEX(desc->data_pointer,32);
#if 1
#ifdef CONFIG_UBICOM_SWITCH_AR8316
#ifdef ATHEROS_IGMP_LOCK
//	printk("--> %x %d\n",(char *)dev,(long) desc->data_len);
//	DEBUG_PRINTHEX(desc->data_pointer,32);
		
	//
	mac = (char *)desc->data_pointer;
	if( igmp_lock_enable ) 	{
		if( (*mac & 1) && (*mac != 0xff) ) {
			
//		if( mac[0] == 0x33)		{
//			printk(">tx");
//		DEBUG_PRINTHEX(desc->data_pointer,96);
//		}
//	DEBUG_PRINTHEX(desc->data_pointer,32);
			// we know IGMP  use mac from 01 00 5e 
			// ipv6 use mac form 33 33,and avoid ff00::1 ff00::2 drop with device
//			if( ((mac[0] == 0x01) && (mac[1]==0x00) && (mac[2]==0x5e) ) || (mac[0] == 0x33 && mac[1]== 0x33 && mac[5] != 0x01 && mac[5] != 0x02 )) {	
			if( ((mac[0] == 0x01) && (mac[1]==0x00) && (mac[2]==0x5e) ) || (mac[0] == 0x33 && mac[1]== 0x33 )) {	
				if( !IGMP_Tx_Check(mac,desc->data_len, skb->vlan_tci)) {
					// drop this packet
// 2011.05.04
// memory leak issue WLAN
					dev_kfree_skb_any(skb);

					return NETDEV_TX_OK;
				}
//	printk("pass!!!\n");			
//	DEBUG_PRINTHEX(desc->data_pointer,32);
				
			}
		
//			if( mac[0] == 0x33)		{
//				printk(">pass %d\n",skb->vlan_tci);
//			}
		
		}
	}
#endif		
#endif		
#endif

	/*still have room */
#ifdef CONFIG_UBICOM_SWITCH_AR8316
	if (likely(dev == ubi32_eth_devices[0])) {
#ifdef ATHEROS_IGMP_LOCK
	    int igmp_check=0;
    	    if(mac[0] == 0x01 || (mac[0] == 0x33 && mac[5] != 0x01 && mac[5] != 0x02 )) igmp_check = 1;
#endif
		/* insert VLAN tag and make room for AR8316 CPU header */
		if (unlikely(skb_headroom(skb) < (VLAN_HLEN + 2))) {
			if (!skb_cow_head(skb, (VLAN_HLEN + 2))) {
				return NETDEV_TX_BUSY;
			}
		}

#ifdef ATHEROS_IGMP_LOCK
	    if(igmp_check == 0) {
#endif
               /* make skb->data private */ 
               if (unlikely(skb_cloned(skb))) { 
                       struct sk_buff *skb_dup; 
                       skb_dup = skb_copy(skb, GFP_ATOMIC | __GFP_NOWARN); 
                       if (unlikely(!skb_dup)) { 
                               return NETDEV_TX_BUSY; 
                       } 
 
                       /* Free the old skb (reduce its reference count) */ 
                       dev_kfree_skb_any(skb); 
                       skb = skb_dup; 
               } 
#ifdef ATHEROS_IGMP_LOCK
	    }
#endif 

		skb_push(skb, (VLAN_HLEN + 2));
		asm volatile (
		"	btst	%0, #0		\n\t"
		"	jmpeq.t	1f		\n\t"

		"	move.1	0(%0), 6(%0)	\n\t"	// perform 8-bit op.
		"	move.1	1(%0), 7(%0)	\n\t"
		"	move.1	2(%0), 8(%0)	\n\t"
		"	move.1	3(%0), 9(%0)	\n\t"
		"	move.1	4(%0), 10(%0)	\n\t"
		"	move.1	5(%0), 11(%0)	\n\t"
		"	move.1	6(%0), 12(%0)	\n\t"
		"	move.1	7(%0), 13(%0)	\n\t"
		"	move.1	8(%0), 14(%0)	\n\t"
		"	move.1	9(%0), 15(%0)	\n\t"
		"	move.1	10(%0), 16(%0)	\n\t"
		"	move.1	11(%0), 17(%0)	\n\t"
		"	move.1	12(%0), #0x80	\n\t"
		"	move.1	13(%0), #0x20	\n\t"
		"	move.1	14(%0), #0x81	\n\t"
		"	move.1	15(%0), #0x00	\n\t"
		"	move.1	17(%0), %1	\n\t"
		"	lsr.4	%1, %1, #8	\n\t"
		"	move.1	16(%0), %1	\n\t"
		"	jmpt.t	2f		\n\t"

		"1:	move.2	0(%0), 6(%0)	\n\t"	// perform 16-bit op.
		"	move.2	2(%0), 8(%0)	\n\t"
		"	move.2	4(%0), 10(%0)	\n\t"
		"	move.2	6(%0), 12(%0)	\n\t"
		"	move.2	8(%0), 14(%0)	\n\t"
		"	move.2	10(%0), 16(%0)	\n\t"
		"	movei	12(%0), #0x8020	\n\t"
		"	movei	14(%0),	#0x8100	\n\t"
		"	move.2	16(%0), %1	\n\t"

		"2:				\n\t"
		:
		: "a" (skb->data), "d" (skb->vlan_tci)
		: "memory"
		);
	}
#endif
	desc = (struct ubi32_eth_dma_desc *)skb->cb;
	desc->data_pointer = skb->data;
	desc->data_len = skb->len;

// try
//	DEBUG_PRINTHEX(desc->data_pointer,32);
#if 0
#ifdef CONFIG_UBICOM_SWITCH_AR8316
#ifdef ATHEROS_IGMP_LOCK
//		printk("--> %x %d\n",(char *)dev,(long) desc->data_len);
//		DEBUG_PRINTHEX(desc->data_pointer,32);
		
		//
		if( igmp_lock_enable ) 	{
			unsigned char *mac;
			mac = (char *)desc->data_pointer;
			if( (*mac & 1) && (*mac != 0xff) ) {
				
				// we know IGMP  use mac from 01 00 5e 
				if( (mac[0] == 0x01) && (mac[1]==0x00) && (mac[2]==0x5e) ) {			
					if( !IGMP_Tx_Check(mac,desc->data_len)) {
						dev_kfree_skb_any(skb);
						// drop this packet
						return NETDEV_TX_OK;
					}
				}
			}
		}
#endif		
#endif		
#endif
	
	priv->regs->tx_dma_ring[tx_in] = desc;
	tx_in = ((tx_in + 1) & TX_DMA_RING_MASK);
	wmb();
	priv->regs->tx_in = tx_in;
	/* kick the HRT */
	ubicom32_set_interrupt(priv->vp_int_bit);

	return NETDEV_TX_OK;
}

/*
 * Deal with a transmit timeout.
 */
static void ubi32_eth_tx_timeout (struct net_device *dev)
{
	struct ubi32_eth_private *priv = netdev_priv(dev);
	dev->stats.tx_errors++;
	priv->regs->int_mask |= UBI32_ETH_VP_INT_TX;
#ifndef UBICOM32_USE_POLLING
	ubicom32_set_interrupt(dev->irq);
#endif
	ubicom32_set_interrupt(priv->vp_int_bit);
}

static int ubi32_eth_ioctl(struct net_device *dev, struct ifreq *rq, int cmd)
{
	struct ubi32_eth_private *priv = netdev_priv(dev);
	struct mii_ioctl_data *data = if_mii(rq);

	printk(KERN_INFO "ioctl %s, %d\n", dev->name, cmd);
	switch (cmd) {
	case SIOCGMIIPHY:
		data->phy_id = 0;
		break;

	case SIOCGMIIREG:
		if ((data->reg_num & 0x1F) == MII_BMCR) {
			/* Make up MII control register value from what we know */
			data->val_out = 0x0000
			| ((priv->regs->status & UBI32_ETH_VP_STATUS_DUPLEX)
					? BMCR_FULLDPLX : 0)
			| ((priv->regs->status & UBI32_ETH_VP_STATUS_SPEED100)
					? BMCR_SPEED100 : 0)
			| ((priv->regs->status & UBI32_ETH_VP_STATUS_SPEED1000)
					? BMCR_SPEED1000 : 0);
		} else if ((data->reg_num & 0x1F) == MII_BMSR) {
			/* Make up MII status register value from what we know */
			data->val_out =
			(BMSR_100FULL|BMSR_100HALF|BMSR_10FULL|BMSR_10HALF)
			| ((priv->regs->status & UBI32_ETH_VP_STATUS_LINK)
					? BMSR_LSTATUS : 0);
		} else {
			return -EIO;
		}
		break;

	case SIOCSMIIREG:
		return -EOPNOTSUPP;
		break;

#ifdef ATHEROS_IGMP_LOCK
	case SIOCDEVPRIVATE:
		{
			short flags;
			int i;
			
			flags = rq->ifr_ifru.ifru_flags;
			printk(KERN_INFO "flags=%d\n", flags);
			
			if( flags == 0 ) {
				igmp_lock_enable = 0;
				del_timer(&igmp_poll_timer);
				
				igmp_poll_count += IGMP_TABLE_ADD_COUNT;
				igmp_poll_count++;
				igmp_poll_count++;
				
				// for
				for(i=0;i<IGMP_TABLE_MAX_ITEM;i++) {
					igmp_poll(0); // do delete 
					if( igmp_table_resouce->count == 0) 
						break;
				}
                                // Disable wan port hardware IGMP snooping                         
                                athrs16_reg_write(0x104 + (5 << 8),  
                                athrs16_reg_read(0x104 + (5 << 8)) & ~(3 << 20));       

			}
			else {							
				igmp_lock_enable = 1;
				igmp_add_new_poll(1);			
                                // Enable wan port hardware IGMP snooping                         
                                athrs16_reg_write(0x104 + (5 << 8),  
                                athrs16_reg_read(0x104 + (5 << 8)) | (3 << 20));       
			}
			
			
			
		}
		break;
#endif

	default:
		return -EOPNOTSUPP;
	}

	return 0;
}

/*
 * Return statistics to the caller
 */
static struct net_device_stats *ubi32_eth_get_stats(struct net_device *dev)
{
	return &dev->stats;
}


static int ubi32_eth_change_mtu(struct net_device *dev, int new_mtu)
{
	struct ubi32_eth_private *priv = netdev_priv(dev);
	unsigned long flags;

	if ((new_mtu < 68) || (new_mtu > 1500))
		return -EINVAL;

	spin_lock_irqsave(&priv->lock, flags);
	dev->mtu = new_mtu;
	spin_unlock_irqrestore(&priv->lock, flags);
	printk(KERN_INFO "set mtu to %d", new_mtu);
	return 0;
}

#ifdef CONFIG_UBICOM_SWITCH_AR8316
/*
 * ubi32_eth_vlan_rx_register: regsiter vlan for RX acceleration
 */
static void ubi32_eth_vlan_rx_register(struct net_device *dev, struct vlan_group *grp)
{
	struct ubi32_eth_private *priv = netdev_priv(dev);

	BUG_ON(dev != ubi32_eth_devices[0]);
	printk(KERN_INFO "%s: register VLAN for HW acceleration\n", dev->name);
	priv->vlgrp = grp;
}
#endif

/*
 * ubi32_eth_cleanup: unload the module
 */
void ubi32_eth_cleanup(void)
{
	struct ubi32_eth_private *priv;
	struct net_device *dev;
	int i;

	for (i = 0; i < UBI32_ETH_NUM_OF_DEVICES; i++) {
		dev = ubi32_eth_devices[i];
		if (dev) {
			priv = netdev_priv(dev);
			kfree(priv->regs->tx_dma_ring);
			unregister_netdev(dev);
			free_netdev(dev);
			ubi32_eth_devices[i] = NULL;
		}
	}
}

int ubi32_eth_init_module(void)
{
	struct ethtionode *eth_node;
 	struct net_device *dev;
	struct ubi32_eth_private *priv;
	int i, err;

	/*
	 * Device allocation.
	 */
	err = 0;
	for (i = 0; i < UBI32_ETH_NUM_OF_DEVICES; i++) {
		/*
		 * See if the eth_vp is in the device tree.
		 */
		eth_node = (struct ethtionode *)devtree_find_node(eth_if_name[i]);
		if (!eth_node) {
			printk(KERN_INFO "%s does not exist\n", eth_if_name[i]);
			continue;
		}

		eth_node->tx_dma_ring = (struct ubi32_eth_dma_desc **)kmalloc(
				sizeof(struct ubi32_eth_dma_desc *) *
				(TX_DMA_RING_SIZE + RX_DMA_RING_SIZE),
				GFP_ATOMIC | __GFP_NOWARN | __GFP_NORETRY | GFP_DMA);

		if (eth_node->tx_dma_ring == NULL) {
			eth_node->tx_dma_ring = (struct ubi32_eth_dma_desc **)kmalloc(
				sizeof(struct ubi32_eth_dma_desc *) *
				(TX_DMA_RING_SIZE + RX_DMA_RING_SIZE), GFP_KERNEL);
			printk(KERN_INFO "fail to allocate from OCM\n");
		}

		if (!eth_node->tx_dma_ring) {
			err = -ENOMEM;
			break;
		}
		eth_node->rx_dma_ring = eth_node->tx_dma_ring + TX_DMA_RING_SIZE;
		eth_node->tx_sz = TX_DMA_RING_SIZE - 1;
		eth_node->rx_sz = RX_DMA_RING_SIZE - 1;

		dev = alloc_etherdev(sizeof(struct ubi32_eth_private));
		if (!dev) {
			kfree(eth_node->tx_dma_ring);
			err = -ENOMEM;
			break;
		}
		priv = netdev_priv(dev);
		priv->dev = dev;

		/*
		 * This just fill in some default Ubicom MAC address
		 */
		memcpy(dev->perm_addr, mac_addr[i], ETH_ALEN);
		memcpy(dev->dev_addr, mac_addr[i], ETH_ALEN);
		memset(dev->broadcast, 0xff, ETH_ALEN);

		priv->regs = eth_node;
		priv->regs->command = 0;
		priv->regs->int_mask = 0;
		priv->regs->int_status = 0;
		priv->regs->tx_out = 0;
		priv->regs->rx_out = 0;
		priv->regs->tx_in = 0;
		priv->regs->rx_in = 0;
		priv->rx_tail = 0;
		priv->tx_tail = 0;

		priv->vp_int_bit = eth_node->dn.sendirq;
		dev->irq = eth_node->dn.recvirq;

		spin_lock_init(&priv->lock);

		dev->open		= ubi32_eth_open;
		dev->stop		= ubi32_eth_close;
		dev->hard_start_xmit	= ubi32_eth_start_xmit;
		dev->tx_timeout		= ubi32_eth_tx_timeout;
		dev->watchdog_timeo	= UBI32_ETH_VP_TX_TIMEOUT;

		dev->set_config		= ubi32_eth_set_config;
		dev->do_ioctl		= ubi32_eth_ioctl;
		dev->get_stats		= ubi32_eth_get_stats;
		dev->change_mtu		= ubi32_eth_change_mtu;
#ifdef UBICOM32_USE_NAPI
		netif_napi_add(dev, &priv->napi, ubi32_eth_napi_poll, UBI32_ETH_NAPI_WEIGHT);
#endif
#ifdef CONFIG_UBICOM_SWITCH_AR8316
		if (i == 0) {
			dev->features		|= NETIF_F_HW_VLAN_RX | NETIF_F_HW_VLAN_TX;
			dev->vlan_rx_register	= ubi32_eth_vlan_rx_register;
		}
#endif
		err = register_netdev(dev);
		if (err) {
			printk(KERN_WARNING "Failed to register netdev %s\n", eth_if_name[i]);
			//release_region();
			free_netdev(dev);
			kfree(eth_node->tx_dma_ring);
			break;
		}

		ubi32_eth_devices[i] = dev;
		printk(KERN_INFO "%s vp_base:0x%p, tio_int:%d irq:%d feature:0x%lx\n",
			dev->name, priv->regs, eth_node->dn.sendirq, dev->irq, dev->features);
	}

	if (err) {
		ubi32_eth_cleanup();
		return err;
	}

	if (!ubi32_eth_devices[0] && !ubi32_eth_devices[1]) {
		return -ENODEV;
	}

#if defined(UBICOM32_USE_POLLING)
	init_timer(&eth_poll_timer);
	eth_poll_timer.function = ubi32_eth_poll;
	eth_poll_timer.data = (unsigned long)0;
	eth_poll_timer.expires = jiffies + 2;
	add_timer(&eth_poll_timer);
#endif


#ifdef CONFIG_UBICOM_SWITCH_AR8316
#ifdef ATHEROS_IGMP_LOCK

	IGMP_Init();

#endif
#endif

	return 0;
}

module_init(ubi32_eth_init_module);
module_exit(ubi32_eth_cleanup);

MODULE_AUTHOR("Kan Yan, Greg Ren");
MODULE_LICENSE("GPL");
