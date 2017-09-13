/*
 * This is a module which is used for rejecting packets.
 */

/* (C) 1999-2001 Paul `Rusty' Russell
 * (C) 2002-2004 Netfilter Core Team <coreteam@netfilter.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt
#include <linux/module.h>
#include <linux/skbuff.h>
#include <linux/slab.h>
#include <linux/ip.h>
#include <linux/udp.h>
#include <linux/icmp.h>
#include <net/icmp.h>
#include <net/ip.h>
#include <net/tcp.h>
#include <net/route.h>
#include <net/dst.h>
#include <linux/netfilter/x_tables.h>
#include <linux/netfilter_ipv4/ip_tables.h>
#include <linux/netfilter_ipv4/ipt_REJECT.h>
#ifdef CONFIG_BRIDGE_NETFILTER
#include <linux/netfilter_bridge.h>
#endif

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Netfilter Core Team <coreteam@netfilter.org>");
MODULE_DESCRIPTION("Xtables: packet \"rejection\" target for IPv4");

#if 0
char *url_data = 				
"HTTP/1.1 200 OK\r\n"
"Server: Embedded HTTP Server ,Firmware Version 1.02\r\n"
"Date: Fri, 17 Aug 2012 04:12:53 GMT\r\n"
"Content-Type: text/html\r\n"
"cache-Control: max-age=1\r\n"
"\r\n"
"<html>\nHello\n</html>\n";
#endif

char *url_data1 = 				
"HTTP/1.0 200 Ok\r\n"
"Server: httpd\r\n"
"Date: Mon, 20 Aug 2012 04:25:03 GMT\r\n"
"Cache-Control: no-cache\r\n"
"Pragma: no-cache\r\n"
"Expires: 0\r\n"
"Content-Type: text/html\r\n"
"Connection: close\r\n"
"\r\n"
"<script>\n"
"window.location.href = \"http://";

char *url_data2 =
"/reject.html\"\n"	
"</script>\n";


//"<html>\n"
//"Hello\n"
//"<html>\n";

#include <linux/inet.h>
#include <linux/inetdevice.h>
#include <linux/netdevice.h>


#define MY_PROC_IP 1
#ifdef MY_PROC_IP
char my_router_ip[128]="192.168.0.1";

static int myproc_init_module(void);
static int myproc_exit_module(void);

#endif


/* Send RST reply */
static void send_reset(struct sk_buff *oldskb, int hook,long mytype)
{
	struct sk_buff *nskb;
	const struct iphdr *oiph;
	struct iphdr *niph;
	const struct tcphdr *oth;
	struct tcphdr _otcph, *tcph;
	unsigned int addr_type;

	long old_tsval;
	long old_tsecr;

//printk("send_reset\n");

	/* IP header checks: fragment. */
	if (ip_hdr(oldskb)->frag_off & htons(IP_OFFSET))
		return;

	oth = skb_header_pointer(oldskb, ip_hdrlen(oldskb),
				 sizeof(_otcph), &_otcph);
	if (oth == NULL)
		return;

	/* No RST for RST. */
	if (oth->rst)
		return;

	/* Check checksum */
	if (nf_ip_checksum(oldskb, hook, ip_hdrlen(oldskb), IPPROTO_TCP))
		return;
	oiph = ip_hdr(oldskb);


// need to size 32 bytes
// need to search find id = 08
// now only try
	{
		char *option;
		long *l_ptr;
		
		option = (char *) oiph;
		option += sizeof(struct iphdr);
		option += sizeof(struct tcphdr);
		option += 4; // 01 01 08 0a
		
		l_ptr = (long *) &option[0];
		old_tsval = *l_ptr;
		
		l_ptr = (long *) &option[4];
		
		old_tsecr = *l_ptr;
//		printk("old_tsval=%ld %ld\n",(long) old_tsval,old_tsecr);
		
	}



	nskb = alloc_skb(sizeof(struct iphdr) + sizeof(struct tcphdr) +
			 LL_MAX_HEADER + 300 + 12, GFP_ATOMIC);
	if (!nskb)
		return;

	skb_reserve(nskb, LL_MAX_HEADER);

	skb_reset_network_header(nskb);
	niph = (struct iphdr *)skb_put(nskb, sizeof(struct iphdr));
	niph->version	= 4;
	niph->ihl	= sizeof(struct iphdr) / 4;
	niph->tos	= 0;
	niph->id	= 0;
	niph->frag_off	= htons(IP_DF);
	niph->protocol	= IPPROTO_TCP;
	niph->check	= 0;
	niph->saddr	= oiph->daddr;
	niph->daddr	= oiph->saddr;

	tcph = (struct tcphdr *)skb_put(nskb, sizeof(struct tcphdr)+12);
	memset(tcph, 0, sizeof(*tcph));
	tcph->source	= oth->dest;
	tcph->dest	= oth->source;
	tcph->doff	= (sizeof(struct tcphdr)+ 12) / 4;

	if (oth->ack)
		tcph->seq = oth->ack_seq;
	else {
		tcph->ack_seq = htonl(ntohl(oth->seq) + oth->syn + oth->fin +
				      oldskb->len - ip_hdrlen(oldskb) -
				      (oth->doff << 2));
		tcph->ack = 1;
	}

	{
	char *option = (char *) tcph;
	long *tsval = (long *) &option[4];
	long *tsecr = (long *) &option[8];
	
	option += sizeof(struct tcphdr);

	tsval = (long *) &option[4];
	tsecr = (long *) &option[8];

	option[0] = 0x01;
	option[1] = 0x01;
	option[2] = 0x08;
	option[3] = 0x0a;

	if( mytype == 1) {
		*tsval = old_tsecr + 1;
		*tsecr = old_tsval; // ???
	}
	else {

		*tsval = old_tsecr+ 1 + strlen(url_data1) + strlen(my_router_ip) + strlen(url_data2);
		*tsval = old_tsecr+ 1 + 8;
		*tsecr = old_tsval; // ???
	}
	}

if( mytype == 3) {
	tcph->seq = oth->ack_seq + 2;
	tcph->seq = oth->ack_seq + strlen(url_data1) + strlen(my_router_ip) + strlen(url_data2);
}
else {
	tcph->seq = oth->ack_seq;
}
	tcph->ack_seq = htonl(ntohl(oth->seq) + oth->syn + oth->fin +
			      oldskb->len - ip_hdrlen(oldskb) -
			      (oth->doff << 2));
			      
	if( mytype == 1) {			      
			      
		tcph->ack = 1;
	}
	else if( mytype == 3) {			      
			      
		tcph->ack = 1;
		tcph->fin = 1;
	}
	else 
	{

		tcph->ack = 1;
		tcph->psh = 1;
	}




//	tcph = (struct tcphdr *)skb_put(nskb, sizeof(struct tcphdr));
	if( mytype == 1) {			      
	}
	else if( mytype == 3) {			      		      
	}
	else
	{
#if 0		
		char *data = "HTTP/1.0 200 Ok\r\n"
				"Server: httpd\r\n"
				"Date: Thu, 16 Aug 2012 09:38:28 GMT\r\n"
				"Cache-Control: no-cache\r\n"
				"Pragma: no-cache\r\n"
				"Expires: 0\r\n"
				"Content-Type: text/html\r\n"
				"Connection: close\r\n"
				"\r\n"
				"<html></html>"
				;
#endif				
#if 0			
		{
	__be32 saddr;
	struct net *net;
			
			struct rtable *rt = skb_rtable(oldskb);
			
			if( rt) {
			
	saddr = 0; //iph->daddr;
	net = dev_net(rt->dst.dev);
	if (!(rt->rt_flags & RTCF_LOCAL)) {
		struct net_device *dev = NULL;

		rcu_read_lock();
		if (rt->fl.iif &&
			net->ipv4.sysctl_icmp_errors_use_inbound_ifaddr)
			dev = dev_get_by_index_rcu(net, rt->fl.iif);

		if (dev)
			saddr = inet_select_addr(dev, 0, RT_SCOPE_LINK);
		else
			saddr = 0;
		rcu_read_unlock();
	}
	printk("%x\n",saddr);
	}
			
			
		}
		
		   icmp_send(oldskb, 1, 1,0);
#endif		

		{

		char *data;
		char *newdata;
		long len;


				
		data = url_data1;
		len = strlen(data);
		newdata = (char *)skb_put(nskb, len);		
		if( newdata) {
			memcpy(newdata,data,len);
		}

		data = my_router_ip;
		len = strlen(data);
		newdata = (char *)skb_put(nskb, len);		
		if( newdata) {
			memcpy(newdata,data,len);
		}


		data = url_data2;
		len = strlen(data);
		newdata = (char *)skb_put(nskb, len);		
		if( newdata) {
			memcpy(newdata,data,len);
		}


		}	
	}





	tcph->window = 215;

	tcph->check = 0; //~tcp_v4_check(sizeof(struct tcphdr), niph->saddr,
			//	    niph->daddr, 0);


//	th->check = tcp_v4_check(skb->len, saddr, daddr,
//				 csum_partial(th,
//					      th->doff << 2,
//					      skb->csum));


//	tcph->check = tcp_v4_check(nskb->len, niph->saddr, niph->daddr,
//				 csum_partial(tcph,
//					      tcph->doff << 2,
//					      nskb->csum));

#if 0
	nskb->ip_summed = CHECKSUM_COMPLETE;
//	nskb->ip_summed = CHECKSUM_PARTIAL;
	nskb->ip_summed = 0;
	nskb->csum_start = (unsigned char *)tcph - nskb->head;
	nskb->csum_offset = offsetof(struct tcphdr, check);
#endif

	tcph->check = ~tcp_v4_check(sizeof(struct tcphdr) + 12, niph->saddr,
				    niph->daddr, 0);
	nskb->ip_summed = CHECKSUM_PARTIAL;
	nskb->csum_start = (unsigned char *)tcph - nskb->head;
	
	if( mytype == 2) {			      
//		nskb->csum_offset = offsetof(struct tcphdr, check) + strlen(url_data);

			long datalen;
			nskb->ip_summed = 0;
			datalen = nskb->len - niph->ihl*4;
			tcph->check = 0;
			tcph->check = tcp_v4_check(datalen,
						   niph->saddr, niph->daddr,
						   csum_partial(tcph,
								datalen, 0));


	}
	else {
		nskb->csum_offset = offsetof(struct tcphdr, check);
	}


	addr_type = RTN_UNSPEC;
	if (hook != NF_INET_FORWARD
#ifdef CONFIG_BRIDGE_NETFILTER
	    || (nskb->nf_bridge && nskb->nf_bridge->mask & BRNF_BRIDGED)
#endif
	   )
		addr_type = RTN_LOCAL;

	/* ip_route_me_harder expects skb->dst to be set */
	skb_dst_set_noref(nskb, skb_dst(oldskb));

	nskb->protocol = htons(ETH_P_IP);
	if (ip_route_me_harder(nskb, addr_type))
		goto free_nskb;

	niph->ttl	= dst_metric(skb_dst(nskb), RTAX_HOPLIMIT);

	/* "Never happens" */
	if (nskb->len > dst_mtu(skb_dst(nskb)))
		goto free_nskb;

	nf_ct_attach(nskb, oldskb);

	ip_local_out(nskb);
	return;

 free_nskb:
	kfree_skb(nskb);
}

static inline void send_unreach(struct sk_buff *skb_in, int code)
{
	icmp_send(skb_in, ICMP_DEST_UNREACH, code, 0);
}

static unsigned int
reject_tg(struct sk_buff *skb, const struct xt_action_param *par)
{
	const struct ipt_reject_info *reject = par->targinfo;

	switch (reject->with) {
	case IPT_ICMP_NET_UNREACHABLE:
		send_unreach(skb, ICMP_NET_UNREACH);
		break;
	case IPT_ICMP_HOST_UNREACHABLE:
		send_unreach(skb, ICMP_HOST_UNREACH);
		break;
	case IPT_ICMP_PROT_UNREACHABLE:
		send_unreach(skb, ICMP_PROT_UNREACH);
		break;
	case IPT_ICMP_PORT_UNREACHABLE:
		send_unreach(skb, ICMP_PORT_UNREACH);
		break;
	case IPT_ICMP_NET_PROHIBITED:
		send_unreach(skb, ICMP_NET_ANO);
		break;
	case IPT_ICMP_HOST_PROHIBITED:
		send_unreach(skb, ICMP_HOST_ANO);
		break;
	case IPT_ICMP_ADMIN_PROHIBITED:
		send_unreach(skb, ICMP_PKT_FILTERED);
		break;
	case IPT_TCP_RESET:
		send_reset(skb, par->hooknum,1);
		send_reset(skb, par->hooknum,2);
		send_reset(skb, par->hooknum,3);
	case IPT_ICMP_ECHOREPLY:
		/* Doesn't happen. */
		break;
	}

	return NF_DROP;
}

static int reject_tg_check(const struct xt_tgchk_param *par)
{
	const struct ipt_reject_info *rejinfo = par->targinfo;
	const struct ipt_entry *e = par->entryinfo;

	if (rejinfo->with == IPT_ICMP_ECHOREPLY) {
		pr_info("ECHOREPLY no longer supported.\n");
		return -EINVAL;
	} else if (rejinfo->with == IPT_TCP_RESET) {
		/* Must specify that it's a TCP packet */
		if (e->ip.proto != IPPROTO_TCP ||
		    (e->ip.invflags & XT_INV_PROTO)) {
			pr_info("TCP_RESET invalid for non-tcp\n");
			return -EINVAL;
		}
	}
	return 0;
}

static struct xt_target reject_tg_reg __read_mostly = {
	.name		= "REJECT",
	.family		= NFPROTO_IPV4,
	.target		= reject_tg,
	.targetsize	= sizeof(struct ipt_reject_info),
	.table		= "filter",
	.hooks		= (1 << NF_INET_LOCAL_IN) | (1 << NF_INET_FORWARD) |
			  (1 << NF_INET_LOCAL_OUT),
	.checkentry	= reject_tg_check,
	.me		= THIS_MODULE,
};

static int __init reject_tg_init(void)
{

#ifdef MY_PROC_IP
	myproc_init_module();
#endif

	return xt_register_target(&reject_tg_reg);
}

static void __exit reject_tg_exit(void)
{
#ifdef MY_PROC_IP
	myproc_exit_module();
#endif


	xt_unregister_target(&reject_tg_reg);
}




//
#ifdef MY_PROC_IP
// add proc
#include <linux/module.h>       /* Specifically, a module */
#include <linux/kernel.h>       /* We're doing kernel work */
#include <linux/proc_fs.h>      /* Necessary because we use the proc fs */
#include <asm/uaccess.h>        /* for copy_from_user */
#define PROCFS_MAX_SIZE         128
#define PROCFS_NAME             "router_ip"
/**
 * This structure hold information about the /proc file
 *
 */
static struct proc_dir_entry *ipmr_Our_Proc_File;
/**
 * The buffer used to store character for this module
 *
 */
static char ipmr_procfs_buffer[PROCFS_MAX_SIZE];
/**
 * The size of the buffer
 *
 */
static unsigned long ipmr_procfs_buffer_size = 0;
/**
 * This function is called then the /proc file is read
 *
 */
static int
procfile_read(char *buffer,
              char **buffer_location,
              off_t offset, int buffer_length, int *eof, void *data)
{
        int ret;
        printk(KERN_INFO "procfile_read (/proc/%s) called\n", PROCFS_NAME);
        if (offset > 0) {
                /* we have finished to read, return 0 */
                ret = 0;
        } else {
                /* fill the buffer, return the buffer size */
                sprintf(buffer,"%s",my_router_ip);
                ret = strlen(buffer);
        }
        return ret;
}
/**
 * This function is called with the /proc file is written
 *
 */
static int procfile_write(struct file *file, const char *buffer, unsigned long count,
                    void *data)
{
        /* get buffer size */
        ipmr_procfs_buffer_size = count;
        if (ipmr_procfs_buffer_size > PROCFS_MAX_SIZE ) {
                ipmr_procfs_buffer_size = PROCFS_MAX_SIZE;
        }
        /* write data to the buffer */
        if ( copy_from_user(ipmr_procfs_buffer, buffer, ipmr_procfs_buffer_size) ) {
                return -EFAULT;
        }
        
        ipmr_procfs_buffer[ipmr_procfs_buffer_size] = 0;
//        sscanf(ipmr_procfs_buffer,"%d",&ipmr_ttl);
        strcpy(my_router_ip,ipmr_procfs_buffer);
        {
        	char *s;
        	s = my_router_ip;
        	while(*s) {
        		if( *s == '\r' || *s == '\n') {
        			*s = 0;
        			break;
        		}
        		s++;
        	}
        }
        
        
        printk(KERN_INFO "procfile_write (/proc/%s) called [%s]\n", PROCFS_NAME,my_router_ip);
        
        return ipmr_procfs_buffer_size;
}
/**
 *This function is called when the module is loaded
 *
 */
static int myproc_init_module(void)
{
        /* create the /proc file */
        ipmr_Our_Proc_File = create_proc_entry(PROCFS_NAME, 0644, NULL);
        if (ipmr_Our_Proc_File == NULL) {
                printk(KERN_ALERT "Error: Could not initialize /proc/%s\n",
                         PROCFS_NAME);
                return -ENOMEM;
        }
        ipmr_Our_Proc_File->read_proc   = procfile_read;
        ipmr_Our_Proc_File->write_proc  = procfile_write;
        //ipmr_Our_Proc_File->owner       = THIS_MODULE;
        ipmr_Our_Proc_File->mode        = S_IFREG | S_IRUGO;
        ipmr_Our_Proc_File->uid         = 0;
        ipmr_Our_Proc_File->gid         = 0;
        ipmr_Our_Proc_File->size        = 128;
        printk(KERN_INFO "/proc/%s created\n", PROCFS_NAME);
        return 0;        /* everything is ok */
}

static int myproc_exit_module(void)
{
	remove_proc_entry(PROCFS_NAME, NULL); 
	printk(KERN_INFO "/proc/%s removed\n", PROCFS_NAME); 
	
	return 0;
}


#endif




module_init(reject_tg_init);
module_exit(reject_tg_exit);
