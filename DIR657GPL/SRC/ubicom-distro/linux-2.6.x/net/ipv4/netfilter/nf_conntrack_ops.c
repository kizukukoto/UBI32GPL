#include <linux/types.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/percpu.h>
#include <net/net_namespace.h>

#include <linux/netfilter.h>
#include <net/netfilter/nf_conntrack_core.h>
#include <net/netfilter/nf_conntrack_l3proto.h>
#include <net/netfilter/nf_conntrack_l4proto.h>
#include <net/netfilter/nf_conntrack_expect.h>
#include <net/netfilter/nf_conntrack_acct.h>

/*
 * This is derive from nf_conntrack_l3proto_ipv4_compat.c
 * and base on Kernel 2.6.28x
 * */
static int bucket_index;
static int nf_timeout;
static struct net *NET = NULL;
//#define __DEBUG__
#ifdef __DEBUG__
#define NF_DBG(fmt, a...)	printk(fmt, ##a)
#else
#define NF_DBG(fmt, a...)	do{}while(0)
#endif //__DEBUG__
static void nf_set_expire(struct net *net, int expire);
static int nf_settimeout(const char *val, struct kernel_param *kp)
{
	nf_timeout = simple_strtoul(val, NULL, 0);
	if (nf_timeout >= 25 && NET != NULL) {
		nf_set_expire(NET, nf_timeout);
	} else
		nf_timeout = 0;
	return 0;
}

MODULE_DESCRIPTION("set nf_conntrack timeout by HZ.");

module_param_call(nf_timeout, nf_settimeout, param_get_uint, &nf_timeout, 0600);

static struct hlist_node *ct_get_first(struct net *net)
{
	int bucket;
	struct hlist_node *n;

	NF_DBG("%s,%d, bucket size:%d\n", __FUNCTION__, __LINE__, nf_conntrack_htable_size);
	for (bucket = 0; bucket < nf_conntrack_htable_size; bucket++) {
                n = rcu_dereference(net->ct.hash[bucket].first);
		if (n) {
			NF_DBG("%s,%d, %d\n", __FUNCTION__, __LINE__, bucket);
			bucket_index = bucket;
			return n;
		}
	}
	return NULL;
}

static struct hlist_node *ct_get_next(struct net *net,
				      struct hlist_node *head)
{

	head = rcu_dereference(head->next);
	while (head == NULL) {
		if (++bucket_index >= nf_conntrack_htable_size)
			return NULL;
		head = rcu_dereference(net->ct.hash[bucket_index].first);
	}
	return head;
}

static struct hlist_node *ct_get_idx(struct net *net, loff_t pos)
{
	struct hlist_node *head = ct_get_first(net);

	if (head)
		while (pos && (head = ct_get_next(net, head)))
			pos--;
	return pos ? NULL : head;
}


static void nf_set_expire(struct net *net, int expire)
{
	struct hlist_node *head;

	head = ct_get_idx(net, 0);
	for (;head != NULL; head = ct_get_next(net, head)) {
		const struct nf_conntrack_tuple_hash *hash = (void *)head;
		struct nf_conn *ct = nf_ct_tuplehash_to_ctrack(hash);
		const struct nf_conntrack_l3proto *l3proto;
		const struct nf_conntrack_l4proto *l4proto;
		NF_CT_ASSERT(ct);
		/* we only want to print DIR_ORIGINAL */
		if (NF_CT_DIRECTION(hash))
			continue;
		if (nf_ct_l3num(ct) != AF_INET)
			continue;
		/* I only care INET ipv4 family */
		l3proto = __nf_ct_l3proto_find(nf_ct_l3num(ct));
		NF_CT_ASSERT(l3proto);
		l4proto = __nf_ct_l4proto_find(nf_ct_l3num(ct), nf_ct_protonum(ct));
		NF_CT_ASSERT(l4proto);
		
		if (strcmp(l4proto->name, "icmp") != 0)
			continue;
		NF_DBG("%-8s %u timer_pending: %ld\n",
		      l4proto->name, nf_ct_protonum(ct),
		      timer_pending(&ct->timeout)
		      ? (long)(ct->timeout.expires - jiffies)/HZ : 0);
		
		mod_timer(&ct->timeout, jiffies + expire);
	}
	
}
static int __net_init ip_conntrack_net_init(struct net *net)
{
	NF_DBG("%s,%d\n", __FUNCTION__, __LINE__);
	NET = net;
	return 0;
}
static void __net_exit ip_conntrack_net_exit(struct net *net)
{
	NF_DBG("%s,%d\n", __FUNCTION__, __LINE__);
}
static struct pernet_operations nf_clean_ops = {
	.init = ip_conntrack_net_init,
	.exit = ip_conntrack_net_exit,
};
static int __init nf_clean_init(void)
{
	return register_pernet_subsys(&nf_clean_ops);
}

static void __exit nf_clean_fini(void)
{
	unregister_pernet_subsys(&nf_clean_ops);
	return;
}
MODULE_LICENSE("GPL");
module_init(nf_clean_init);
module_exit(nf_clean_fini);