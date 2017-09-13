/* Masquerade.  Simple mapping which alters range to a local IP address
   (depending on route). */
#include <linux/types.h>
#include <linux/ip.h>
#include <linux/udp.h>
//#include <linux/timer.h>
#include <linux/netfilter.h>
#include <linux/netfilter_ipv4.h>
#include <linux/module.h>
#include <net/protocol.h>
#include <net/checksum.h>
#include <net/ip.h>
#include <linux/tcp.h>

#define ASSERT_READ_LOCK(x) 
#define ASSERT_WRITE_LOCK(x) 


#include <net/netfilter/nf_conntrack.h>
#include <net/netfilter/nf_conntrack_core.h>
#include <net/netfilter/nf_nat_rule.h>
#include <linux/netfilter/x_tables.h>
#include <linux/netfilter_ipv4/ipt_NATTYPE.h>

MODULE_LICENSE("GPL");


#if 0
#define DEBUGP printk
#else
#define DEBUGP(format, args...)
#endif

struct ipt_nattype {
	struct list_head list;		
	struct timer_list timeout;	
	unsigned int dst_addr;
	unsigned short nat_port;	/* Router src port */
	unsigned short proto;
	struct nf_nat_multi_range_compat mr;
};

LIST_HEAD(nattype_list);

static unsigned int
del_nattype_rule(struct ipt_nattype *nattype)
{
	NF_CT_ASSERT(nattype);
	spin_lock_bh(&nf_conntrack_lock);
	list_del(&nattype->list);
    spin_unlock_bh(&nf_conntrack_lock);
	kfree(nattype);
	return 0;
}


static void 
refresh_timer(struct ipt_nattype *nattype)
{
	NF_CT_ASSERT(nattype);
	spin_lock_bh(&nf_conntrack_lock);
	
	if (del_timer(&nattype->timeout)) {
		nattype->timeout.expires = jiffies + NATTYPE_TIMEOUT * HZ;
		add_timer(&nattype->timeout);
	}
	spin_unlock_bh(&nf_conntrack_lock);
}

static void timer_timeout(unsigned long in_nattype)
{
	struct ipt_nattype *nattype= (void *) in_nattype;
	spin_lock_bh(&nf_conntrack_lock);
	del_nattype_rule(nattype);
	spin_unlock_bh(&nf_conntrack_lock);
}


static inline int 
packet_in_match(const struct ipt_nattype *nattype, struct sk_buff *skb, const struct ipt_nattype_info *info)
{
	const struct iphdr *iph = ip_hdr(skb);
	struct tcphdr _tcph, *tcph;
	struct udphdr _udph, *udph;
	u_int16_t dst_port;

	tcph = skb_header_pointer(skb, ip_hdrlen(skb), sizeof(_tcph), &_tcph);
	udph = skb_header_pointer(skb, ip_hdrlen(skb), sizeof(_udph), &_udph);

	if( info->type == TYPE_ENDPOINT_INDEPEND) {
		if( iph->protocol == IPPROTO_TCP)
			dst_port = tcph->dest;
		else if( iph->protocol == IPPROTO_UDP)
			dst_port = udph->dest;
		else
			return 0;
				
		DEBUGP("packet_in_match: nat port :%d\n", ntohs(nattype->nat_port));
		return ( (nattype->proto==iph->protocol) && (nattype->nat_port == dst_port) );

	} else if( info->type == TYPE_ADDRESS_RESTRICT) {
		DEBUGP("packet_in_match fail: nat port :%d, dest_port: %d\n", ntohs(nattype->nat_port), ntohs(dst_port));	
		return (nattype->dst_addr== iph->saddr);
	}
	
	return -1;
}

static inline int 
packet_out_match(const struct ipt_nattype *nattype, struct sk_buff *skb, const u_int16_t nat_port, const struct ipt_nattype_info *info)
{
	const struct iphdr *iph = ip_hdr(skb);
	struct tcphdr _tcph, *tcph;
	struct udphdr _udph, *udph;

	__be16 src_port;

	tcph = skb_header_pointer(skb, ip_hdrlen(skb), sizeof(_tcph), &_tcph);
	udph = skb_header_pointer(skb, ip_hdrlen(skb), sizeof(_udph), &_udph);

	if( info->type == TYPE_ENDPOINT_INDEPEND) {
		if( iph->protocol == IPPROTO_TCP)
			src_port = tcph->source;
		else if( iph->protocol == IPPROTO_UDP)
			src_port = udph->source;
		else
			return 0;
		DEBUGP("packet_out_match: nat port :%d, mr.port:%d\n", ntohs(nattype->nat_port), ntohs(nattype->mr.range[0].min.all));
		return ( (nattype->proto==iph->protocol) && (nattype->mr.range[0].min.all == src_port) && (nattype->nat_port == nat_port));
	} else if( info->type == TYPE_ADDRESS_RESTRICT) {
		return (nattype->dst_addr == iph->daddr );
	}

	return -1;
}

static unsigned int
add_nattype_rule(struct ipt_nattype *nattype)
{
	struct ipt_nattype *rule;

	spin_lock_bh(&nf_conntrack_lock);
	rule = (struct ipt_nattype *)kmalloc(sizeof(struct ipt_nattype), GFP_ATOMIC);

	if (!rule) {
		spin_unlock_bh(&nf_conntrack_lock);
		return -ENOMEM;
	}
	
	memset(rule, 0, sizeof(*nattype));
	INIT_LIST_HEAD(&rule->list);
	memcpy(rule, nattype, sizeof(*nattype));
	list_add(&rule->list, &nattype_list);
	init_timer(&rule->timeout);
	rule->timeout.data = (unsigned long)rule;
	rule->timeout.function = timer_timeout;
	rule->timeout.expires = jiffies + (NATTYPE_TIMEOUT  * HZ);
	add_timer(&rule->timeout);	
	spin_unlock_bh(&nf_conntrack_lock);
	return 0;
}


static unsigned int
nattype_nat(struct sk_buff *skb, const struct xt_target_param *par)
{
	struct nf_conn *ct;
	enum ip_conntrack_info ctinfo;
	struct nf_nat_range newrange;
	struct ipt_nattype *found;

	NF_CT_ASSERT(par->hooknum == NF_INET_PRE_ROUTING);
	list_for_each_entry(found, &nattype_list, list) {
		if (packet_in_match(found, skb, par->targinfo)) {
			DEBUGP("NAT nat port :%d, mr.port:%d\n", ntohs(found->nat_port), ntohs(found->mr.range[0].min.all));

			/* cat /proc/net/ip_conntrack */
			ct = nf_ct_get(skb, &ctinfo);
	
			newrange = ((struct nf_nat_range)
				{ found->mr.range[0].flags | IP_NAT_RANGE_MAP_IPS,
				  found->mr.range[0].min_ip, found->mr.range[0].min_ip,
				  found->mr.range[0].min, found->mr.range[0].max });		 

			return nf_nat_setup_info(ct, &newrange, IP_NAT_MANIP_DST); 
		}
	}

	DEBUGP("nattype_nat: not found\n");
	return XT_CONTINUE;
}


static unsigned int
nattype_forward(struct sk_buff *skb, const struct xt_target_param *par)
{
	const struct iphdr *iph = ip_hdr(skb);
	//void *protoh = proto_hdr(skb);
	void *protoh = (void *)iph + iph->ihl * 4;
	struct ipt_nattype nattype, *found;
	struct nf_conn *ct;
	enum ip_conntrack_info ctinfo;
	const struct ipt_nattype_info *info = par->targinfo;

	NF_CT_ASSERT(hooknum == NF_INET_FORWARD);

	switch(info->mode)
	{
		case MODE_FORWARD_IN:
			list_for_each_entry(found, &nattype_list, list) {
				if (packet_in_match(found, skb, info)) {
					refresh_timer(found);
					DEBUGP("FORWARD_IN_ACCEPT\n");
					return NF_ACCEPT;
				}				
			}

			//Not found
			DEBUGP("FORWARD_IN_FAIL\n");
			break;

		case MODE_FORWARD_OUT:
			ct = nf_ct_get(skb, &ctinfo);
			/* Albert add : check ct if get null */
			if(ct == NULL)
			  return XT_CONTINUE;

			NF_CT_ASSERT(ct && (ctinfo == IP_CT_NEW || ctinfo == IP_CT_RELATED));
			
			list_for_each_entry(found, &nattype_list, list) {
				if (packet_out_match(found, skb, ct->tuplehash[IP_CT_DIR_REPLY].tuple.dst.u.all, info)) {
					refresh_timer(found);
					return XT_CONTINUE;
				}
			}

			DEBUGP("FORWARD_OUT ADD\n");
			if( iph->daddr == iph->saddr)
				return XT_CONTINUE;
				
			/*
			1. Albert Add
			2. NickChou marked for pass VISTA IGD data, but may cause kernel panic in BT test
			  2007.10.11 	
			*/
			if (iph->daddr == (__be32)0 || iph->saddr == (__be32)0 || (iph->protocol != IPPROTO_TCP && iph->protocol != IPPROTO_UDP) ){
					if(iph->daddr == (__be32)0)
						DEBUGP("iph->daddr null point \n");
					else if(iph->saddr == (__be32)0)
						DEBUGP("iph->saddr null point \n");
					else //if(iph->protocol == NULL )
						DEBUGP("iph->protocol null point \n");					       
			       return XT_CONTINUE;
			}       
			memset(&nattype, 0, sizeof(nattype));
			nattype.mr.rangesize = 1;
			nattype.mr.range[0].flags |= IP_NAT_RANGE_PROTO_SPECIFIED;
			
			nattype.dst_addr = iph->daddr;
			nattype.mr.range[0].min_ip = iph->saddr;
			//nattype.nat_port = ct->tuplehash[IP_CT_DIR_REPLY].tuple.dst.u.all;
			//nattype.nat_port = ct->tuplehash[IP_CT_DIR_REPLY].tuple.dst.u.udp.port;
			nattype.proto = iph->protocol;
			
			if( iph->protocol == IPPROTO_TCP) {
				nattype.nat_port = ct->tuplehash[IP_CT_DIR_REPLY].tuple.dst.u.tcp.port;
				nattype.mr.range[0].max.tcp.port = nattype.mr.range[0].min.tcp.port =  ((struct tcphdr *) protoh)->source ;
				DEBUGP("ADD: TCP nat port: %d\n", ntohs(nattype.nat_port));
				DEBUGP("ADD: TCP Source Port: %d\n", ntohs(nattype.mr.range[0].min.tcp.port));
			} else if( iph->protocol == IPPROTO_UDP) {
				nattype.nat_port = ct->tuplehash[IP_CT_DIR_REPLY].tuple.dst.u.udp.port;
				nattype.mr.range[0].max.udp.port = nattype.mr.range[0].min.udp.port = ((struct udphdr *) protoh)->source;
				//nattype.nat_port = nattype.mr.range[0].min.udp.port;
				DEBUGP("ADD: UDP NAT Port: %d\n", ntohs(nattype.nat_port));
				//DEBUGP("ADD: UDP IP_CT_DIR_ORIGINAL port: %d\n", ntohs(ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple.dst.u.all));
				//DEBUGP("ADD: UDP IP_CT_DIR_REPLY port: %d\n", ntohs(ct->tuplehash[IP_CT_DIR_REPLY].tuple.dst.u.all));
				//DEBUGP("ADD: UDP IP_CT_DIR_REPLY dst port: %d\n", ntohs(ct->tuplehash[IP_CT_DIR_REPLY].tuple.dst.u.udp.port));
				DEBUGP("ADD: UDP IP_CT_DIR_ORIGINAL dst port: %d\n", ntohs(ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple.dst.u.udp.port));
				DEBUGP("ADD: UDP Source Port: %d\n", ntohs(nattype.mr.range[0].min.udp.port));
			} else
				return XT_CONTINUE;

			add_nattype_rule(&nattype);

			break;
	}

	return XT_CONTINUE;
}

static unsigned int
nattype_target(struct sk_buff *skb, const struct xt_target_param *par)
{
	const struct ipt_nattype_info *info = par->targinfo;
	const struct iphdr *iph = ip_hdr(skb);
	
	if ((iph->protocol != IPPROTO_TCP) && (iph->protocol != IPPROTO_UDP))
        return XT_CONTINUE;
	
	DEBUGP("nattype_target, info->mode=%d, info->type=%d\n", info->mode, info->type);	

	if (info->mode == MODE_DNAT)
	{
		DEBUGP("nattype_target: MODE_DNAT\n");
		return nattype_nat(skb, par);
	}	
	else if (info->mode == MODE_FORWARD_OUT)
	{
		DEBUGP("nattype_target: MODE_FORWARD_OUT\n");
		/*NickChou marked for pass VISTA IGD data, but may cause kernel panic in BT test
		  2007.10.11
		  KenChiang masked uesrinfo == NULL for fixed nat TYPE =1 or 2 cannot work bug
		  2008.01.11	
		*/
		if (par->in == NULL || par->out == NULL || par->targinfo == NULL /*|| userinfo == NULL*/)
		{
			if(par->in == NULL)
				DEBUGP("in null point \n");
			else if(par->out == NULL)
				DEBUGP("out null point \n");
			else if(par->targinfo == NULL)
				DEBUGP("targinfo null point \n");
			else //if(userinfo == NULL)			
				DEBUGP("userinfo point \n");
			return XT_CONTINUE;
		}
		return nattype_forward(skb, par);
	}	
	else if (info->mode == MODE_FORWARD_IN)
	{
		DEBUGP("nattype_target: MODE_FORWARD_IN\n");
		return nattype_forward(skb, par);
	}
	
	return XT_CONTINUE;
}

static bool nattype_check(const struct xt_tgchk_param *par)
{
	const struct ipt_nattype_info *info = par->targinfo;
	struct list_head *cur, *tmp;
	
	if( info->type == TYPE_PORT_ADDRESS_RESTRICT) {
		DEBUGP("For Port and Address Restricted. You do not need to insert the rule\n");
		return false;
	}
	else if( info->type == TYPE_ENDPOINT_INDEPEND)
		DEBUGP("NAT_Tyep = TYPE_ENDPOINT_INDEPEND.\n");
	else if( info->type == TYPE_ADDRESS_RESTRICT)
		DEBUGP("NAT_Tyep = TYPE_ADDRESS_RESTRICT.\n");	
		
	if ( info->mode != MODE_DNAT && info->mode != MODE_FORWARD_IN && info->mode != MODE_FORWARD_OUT) {
		DEBUGP("nattype_check: bad nat mode.\n");
		return false;
	}	
	
/*
	if (targinfosize != IPT_ALIGN(sizeof(*info))) {
		DEBUGP("nattype_check: size %u != %u.\n",
		       targinfosize, sizeof(*info));
		return 0;
	}
*/
	if (par->hook_mask & ~((1 << NF_INET_PRE_ROUTING) | (1 << NF_INET_FORWARD))) {
		DEBUGP("nattype_check: bad hooks %x.\n", par->hook_mask);
		return false;
	}
	list_for_each_safe(cur, tmp, &nattype_list) {
		struct ipt_nattype *nattype = (void *)cur;
		del_timer(&nattype->timeout);
		del_nattype_rule(nattype);
	}

	return true;
}

static struct ipt_target nattype = {
	.name		= "NATTYPE",
	.family		= NFPROTO_IPV4,
	.target		= nattype_target,
	.checkentry	= nattype_check,
	.targetsize	= sizeof(struct ipt_nattype_info),
	.hooks		= ((1 << NF_INET_PRE_ROUTING) | (1 << NF_INET_FORWARD)),
	.me			= THIS_MODULE,
};

static int __init init(void)
{
	return xt_register_target(&nattype);
}

static void __exit fini(void)
{
	xt_unregister_target(&nattype);
}

module_init(init);
module_exit(fini);

