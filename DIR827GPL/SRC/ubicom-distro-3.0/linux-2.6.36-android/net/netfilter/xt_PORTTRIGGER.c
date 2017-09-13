//#include <linux/config.h>
#include <linux/types.h>
#include <linux/icmp.h>
#include <linux/ip.h>
#include <linux/timer.h>
#include <linux/netfilter.h>
#include <linux/netfilter_ipv4.h>
#include <linux/module.h>
#include <linux/skbuff.h>
#include <linux/proc_fs.h>
#include <linux/vmalloc.h>
#include <net/checksum.h>
#include <net/ip.h>
#include <linux/stddef.h>
#include <linux/sysctl.h>
#include <linux/slab.h>
#include <linux/random.h>
#include <linux/jhash.h>
#include <linux/err.h>
#include <linux/percpu.h>
#include <linux/moduleparam.h>
#include <linux/notifier.h>
#include <linux/tcp.h>

/* nf_conntrack_lock protects the main hash table, protocol/helper/expected
   registrations, conntrack timers*/
//#define ASSERT_READ_LOCK(x)
//#define ASSERT_WRITE_LOCK(x)
//#define ASSERT_READ_LOCK(x) 
//#define ASSERT_WRITE_LOCK(x) 

#include <net/netfilter/nf_conntrack.h>
#include <net/netfilter/nf_conntrack_core.h>
#include <net/netfilter/nf_nat.h>
#include <linux/netfilter_ipv4/ip_tables.h>
#include <linux/netfilter_ipv4/lockhelp.h>
#include <linux/netfilter/xt_PORTTRIGGER.h>

// copy from include/linux/netfilter_ipv4.h
/* IP Hooks */
/* After promisc drops, checksum checks. */
#define NF_IP_PRE_ROUTING       0
/* If the packet is destined for this box. */
#define NF_IP_LOCAL_IN          1
/* If the packet is destined for another interface. */
#define NF_IP_FORWARD           2
/* Packets coming from a local process. */
#define NF_IP_LOCAL_OUT         3
/* Packets about to hit the wire. */
#define NF_IP_POST_ROUTING      4
#define NF_IP_NUMHOOKS          5

MODULE_LICENSE("GPL");


#if 0
#define DEBUGP printk
#else
#define DEBUGP(format, args...)
#endif

#ifdef CONFIG_NETFILTER_DEBUG
#define IP_NF_ASSERT(x)						\
do {								\
	if (!(x))						\
		printk("IP_NF_ASSERT: %s:%s:%u\n",		\
			__func__, __FILE__, __LINE__);   \
} while(0)
#else
#define IP_NF_ASSERT(x)
#endif

#define ASSERT_READ_LOCK(x) MUST_BE_READ_LOCKED(&nf_conntrack_lock)
#define ASSERT_WRITE_LOCK(x) MUST_BE_WRITE_LOCKED(&nf_conntrack_lock)
#include <linux/netfilter/listhelp.h>

struct xt_porttrigger {
	struct list_head list;		
	struct timer_list timeout;	
	unsigned int src_ip;		
	unsigned int dst_ip;		
	unsigned short trigger_proto;
	unsigned short forward_proto;
	unsigned int timer;
	struct xt_mport trigger_ports;
	struct xt_mport forward_ports;
	struct nf_nat_range range;
};

static LIST_HEAD(trigger_list);

static unsigned int
del_porttrigger_rule(struct xt_porttrigger *trigger)
{
	IP_NF_ASSERT(trigger);
//	spin_lock_bh(&nf_conntrack_lock);
	MUST_BE_WRITE_LOCKED(&nf_conntrack_lock);
	DEBUGP("del rule src_ip=%x,proto=%x,dst_ip=%x,proto=%x\n",trigger->src_ip,trigger->trigger_proto,trigger->dst_ip,trigger->forward_proto);
	list_del(&trigger->list);
//	spin_unlock_bh(&nf_conntrack_lock);
	kfree(trigger);
//	return 0;
}


static void 
refresh_timer(struct xt_porttrigger *trigger, unsigned long extra_jiffies)
{
	IP_NF_ASSERT(trigger);
	spin_lock_bh(&nf_conntrack_lock);

	if(extra_jiffies == 0)
		extra_jiffies = TRIGGER_TIMEOUT * HZ;

	if (del_timer(&trigger->timeout)) {
		trigger->timeout.expires = jiffies + extra_jiffies;
		add_timer(&trigger->timeout);
	}
	spin_unlock_bh(&nf_conntrack_lock);
}

static void timer_timeout(unsigned long in_trigger)
{
	struct xt_porttrigger *trigger= (void *) in_trigger;

	spin_lock_bh(&nf_conntrack_lock);
	del_porttrigger_rule(trigger);
	DEBUGP("timer out, del trigger rule\n");
	spin_unlock_bh(&nf_conntrack_lock);
}


static inline int
ports_match(const struct xt_mport *minfo, u_int16_t port)
{
	unsigned int i, m;
	u_int16_t s, e;
	u_int16_t pflags = minfo->pflags;
	
	for (i=0, m=1; i<IPT_MULTI_PORTS; i++, m<<=1) {
		if (pflags & m  && minfo->ports[i] == 65535){
			DEBUGP("port%x don't match=%d\n",port,i);
			return 0;
		}	

		s = minfo->ports[i];
		if (pflags & m) {
			e = minfo->ports[++i];
			m <<= 1;
		} else
			e = s;

		if ( port >= s && port <= e){ 
			//DEBUGP("s=%x,e=%x\n",s,e);
			return 1;
		}	
	}
	DEBUGP("ports=%x don't match\n",port);
	return 0;
}


static inline int 
packet_in_match(const struct xt_porttrigger *trigger,
	const unsigned short proto, 
	const unsigned short dport,
	const unsigned int src_ip)
{
	/* 
	  Modification: for protocol type==all(any) can't work
      Modified by: ken_chiang 
      Date:2007/8/21
    */
#if 0	
	u_int16_t forward_proto = trigger->forward_proto;
	
	if (!forward_proto)
		forward_proto = proto;
	return ( (forward_proto == proto) && (ports_match(&trigger->forward_ports, dport)) );
#else
	u_int16_t forward_proto = trigger->forward_proto;
	DEBUGP("src_ip=%x,trigger->src_ip=%x in match\n",src_ip,trigger->src_ip);
	/* 
	  Modification: for trigge port==incomeing port can't work
      Modified by: ken_chiang 
      Date:2007/9/7
    */
	if(src_ip==trigger->src_ip){
		return 0;
	}	
	DEBUGP("proto=%x,dport=%x in match\n",proto,dport);
	if (!forward_proto){
		DEBUGP("forward_proto=null\n");
		return ( ports_match(&trigger->forward_ports, dport) );
	}
	else{
		DEBUGP("forward_proto=%x\n",forward_proto);
		return ( (trigger->forward_proto == proto) && (ports_match(&trigger->forward_ports, dport)) );
	}	
#endif
}

static inline int 
packet_out_match(const struct xt_porttrigger *trigger,
	const unsigned short proto, 
	unsigned short dport)
{
	/* 
	  Modification: for protocol type==all(any) can't work
      Modified by: ken_chiang 
      Date:2007/8/21
    */
    u_int16_t trigger_proto = trigger->trigger_proto;
    DEBUGP("proto=%x,dport=%x out match\n",proto,dport);
	if (!trigger_proto){
		DEBUGP("trigger_proto=null\n");
		return ( ports_match(&trigger->trigger_ports, dport) );
	}	
	else{
		DEBUGP("trigger_proto=%x\n",trigger_proto);
		return ( (trigger->trigger_proto == proto) && (ports_match(&trigger->trigger_ports, dport)) );
	}	
}


static unsigned int
add_porttrigger_rule(struct xt_porttrigger *trigger)
{
	struct xt_porttrigger *rule;

	spin_lock_bh(&nf_conntrack_lock);
	rule = (struct xt_porttrigger *)kmalloc(sizeof(struct xt_porttrigger), GFP_ATOMIC);

	if (!rule) {
		spin_unlock_bh(&nf_conntrack_lock);
		return -ENOMEM;
	}

	memset(rule, 0, sizeof(*trigger));
	INIT_LIST_HEAD(&rule->list);
	memcpy(rule, trigger, sizeof(*trigger));
	DEBUGP("add rule src_ip=%x,proto=%x,dst_ip=%x,proto=%x\n\n\n",rule->src_ip,rule->trigger_proto,rule->dst_ip,rule->forward_proto);
	list_prepend(&trigger_list, &rule->list);
	init_timer(&rule->timeout);
	rule->timeout.data = (unsigned long)rule;
	rule->timeout.function = timer_timeout;
	DEBUGP("rule->timer=%x\n",rule->timer);
	DEBUGP("rule->src_ip=%x\n",rule->src_ip);
	/* 
	  Modification: for protocol type==all(any) sometime can't work if timer = 0
      Modified by: ken_chiang 
      Date:2007/8/31
    */
    	/* Name: KK Huang
 	 * Date: 2011-07-13
	 * Reason: Replace 300 with TRIGGER_TIMEOUT
 	 */
	if(rule->timer<TRIGGER_TIMEOUT)
		rule->timer =TRIGGER_TIMEOUT;
	DEBUGP("rule->timer2=%x\n",rule->timer);	
	rule->timeout.expires = jiffies + (rule->timer * HZ);
	add_timer(&rule->timeout);
	spin_unlock_bh(&nf_conntrack_lock);
	return 0;
}


static unsigned int
porttrigger_nat(const struct sk_buff *skb,
		const struct net_device *in,
		const struct net_device *out,
		unsigned int hooknum,
		const void *targinfo,
		void *userinfo)
{
	struct nf_conn *ct;
	enum ip_conntrack_info ctinfo;
	const struct iphdr *iph = (const struct iphdr*)skb->network_header;
	struct tcphdr *tcph = (void *)iph + iph->ihl*4;
	struct nf_nat_range newrange;
	struct xt_porttrigger *found;

	IP_NF_ASSERT(hooknum == NF_IP_PRE_ROUTING);
	/* 
	  Modification: for trigge port==incomeing port can't work
      Modified by: ken_chiang 
      Date:2007/9/7
    */
	found = LIST_FIND(&trigger_list, packet_in_match, struct xt_porttrigger *, iph->protocol, ntohs(tcph->dest),iph->saddr);
	if( !found || !found->src_ip ){
		DEBUGP("DNAT: no find\n");
		return IPT_CONTINUE;
	}	

//	DEBUGP("DNAT: src IP %u.%u.%u.%u\n", NIPQUAD(found->src_ip));
	ct = nf_ct_get(skb, &ctinfo);
	newrange = ((struct nf_nat_range)
		{ IP_NAT_RANGE_MAP_IPS, found->src_ip, found->src_ip, 
		found->range.min, found->range.max });

	return nf_nat_setup_info(ct, &newrange, HOOK2MANIP(hooknum));
}


static unsigned int
porttrigger_forward(struct sk_buff *skb,
		  unsigned int hooknum,
		  const struct net_device *in,
		  const struct net_device *out,
		  const void *targinfo,
		  void *userinfo,
		  int mode)
{
	const struct xt_porttrigger_info *info = targinfo;
	const struct iphdr *iph = (const struct iphdr*)skb->network_header;
	struct tcphdr *tcph = (void *)iph + iph->ihl*4;
	struct xt_porttrigger trigger, *found, match;

	switch(mode)
	{
		case MODE_FORWARD_IN:
			/* 
	  			Modification: for trigge port==incomeing port can't work
      			Modified by: ken_chiang 
      			Date:2007/9/7
    		*/
			found = LIST_FIND(&trigger_list, packet_in_match, struct xt_porttrigger *, iph->protocol, ntohs(tcph->dest),iph->saddr);
			if (found) {
				refresh_timer(found, info->timer * HZ);
				DEBUGP("FORWARD_IN found\n");		
				return NF_ACCEPT;
			}
			DEBUGP("FORWARD_IN no found\n");
			break;

		/* MODE_FORWARD_OUT */
		case MODE_FORWARD_OUT:
			found = LIST_FIND(&trigger_list, packet_out_match, struct xt_porttrigger *, iph->protocol, ntohs(tcph->dest));
			if (found) {
				refresh_timer(found, info->timer * HZ);
				found->src_ip = iph->saddr;
				DEBUGP("FORWARD_OUT found ip=%x\n",found->src_ip);
			} else {
				DEBUGP("FORWARD_OUT no found\n");
				//memcpy(&match.trigger_ports, &info->trigger_ports, sizeof(struct xt_mport));
				match.trigger_ports = info->trigger_ports;
				match.trigger_proto = info->trigger_proto;
					
				if( packet_out_match(&match, iph->protocol, ntohs(tcph->dest)) ) {
					DEBUGP("FORWARD_OUT_MATCH\n");
					memset(&trigger, 0, sizeof(trigger));
					trigger.src_ip = iph->saddr;
					DEBUGP("FORWARD_OUT trigger ip=%x\n",trigger.src_ip);
					trigger.trigger_proto = iph->protocol;					
					trigger.forward_proto = info->forward_proto;
					memcpy(&trigger.trigger_ports, &info->trigger_ports, sizeof(struct xt_mport));
					memcpy(&trigger.forward_ports, &info->forward_ports, sizeof(struct xt_mport));
					add_porttrigger_rule(&trigger);
				}

			}
			break;
	}

	return IPT_CONTINUE;
}

unsigned int
porttrigger_target(struct sk_buff *skb, const struct xt_action_param *par)
{
	const struct xt_porttrigger_info *info = par->targinfo;
	const struct iphdr *iph = (const struct iphdr*)skb->network_header;

	if ((iph->protocol != IPPROTO_TCP) && (iph->protocol != IPPROTO_UDP))
		return IPT_CONTINUE;

	if (info->mode == MODE_DNAT)
		return porttrigger_nat(skb, par->in, par->out, par->hooknum, par->targinfo, NULL);
	else if (info->mode == MODE_FORWARD_OUT)
		return porttrigger_forward(skb, par->hooknum, par->in, par->out, par->targinfo, NULL, MODE_FORWARD_OUT);
	else if (info->mode == MODE_FORWARD_IN)
		return porttrigger_forward(skb, par->hooknum, par->in, par->out, par->targinfo, NULL, MODE_FORWARD_IN);

	return IPT_CONTINUE;
}

static int
porttrigger_check(const struct xt_tgchk_param *par)
{
	const struct xt_porttrigger_info *info = par->targinfo;
	struct list_head *cur, *tmp;

	if( info->mode == MODE_DNAT && strcmp(par->table, "nat") != 0) {
		DEBUGP("porttrigger_check: bad table `%s'.\n", par->table);
		//return 0;
		return -EINVAL;
	}
	/*
	if (targinfosize != XT_ALIGN(sizeof(*info))) {
		DEBUGP("porttrigger_check: size %u != %u.\n",
		       targinfosize, sizeof(*info));
		return 0;
	}*/
	if (par->hook_mask & ~((1 << NF_IP_PRE_ROUTING) | (1 << NF_IP_FORWARD))) {
		DEBUGP("porttrigger_check: bad hooks %x.\n", par->hook_mask);
		return -EINVAL;
	}
	if ( info->forward_proto != IPPROTO_TCP && info->forward_proto != IPPROTO_UDP && info->forward_proto != 0) {
		DEBUGP("porttrigger_check: bad trigger proto.\n");
		return -EINVAL;
	}

	list_for_each_safe(cur, tmp, &trigger_list) {
		struct xt_porttrigger *trigger = (void *)cur;
		del_timer(&trigger->timeout);
		del_porttrigger_rule(trigger);
	}

	return 0;
}

static struct xt_target porttrigger = {
	.name		    = "PORTTRIGGER",
	.target		  = porttrigger_target,
  .checkentry = porttrigger_check,
	.targetsize	= sizeof(struct xt_porttrigger_info),
//	.table		= "nat",
	.me		= THIS_MODULE,
};

static int __init init(void)
{
	return xt_register_target(&porttrigger);
}

static void __exit fini(void)
{
	xt_unregister_target(&porttrigger);
}

module_init(init);
module_exit(fini);

