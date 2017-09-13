/*
 * RTSP extension for TCP NAT alteration
 * (C) 2003 by Tom Marshall <tmarshall at real.com>
 * based on ip_nat_irc.c
 *
 *      This program is free software; you can redistribute it and/or
 *      modify it under the terms of the GNU General Public License
 *      as published by the Free Software Foundation; either version
 *      2 of the License, or (at your option) any later version.
 *
 * Module load syntax:
 *      insmod nf_nat_rtsp.o ports=port1,port2,...port<MAX_PORTS>
 *                           stunaddr=<address>
 *                           destaction=[auto|strip|none]
 *
 * If no ports are specified, the default will be port 554 only.
 *
 * stunaddr specifies the address used to detect that a client is using STUN.
 * If this address is seen in the destination parameter, it is assumed that
 * the client has already punched a UDP hole in the firewall, so we don't
 * mangle the client_port.  If none is specified, it is autodetected.  It
 * only needs to be set if you have multiple levels of NAT.  It should be
 * set to the external address that the STUN clients detect.  Note that in
 * this case, it will not be possible for clients to use UDP with servers
 * between the NATs.
 *
 * If no destaction is specified, auto is used.
 *   destaction=auto:  strip destination parameter if it is not stunaddr.
 *   destaction=strip: always strip destination parameter (not recommended).
 *   destaction=none:  do not touch destination parameter (not recommended).
 */

#include <linux/module.h>
#include <net/tcp.h>
#include <net/netfilter/nf_nat_helper.h>
#include <net/netfilter/nf_nat_rule.h>
#include <linux/netfilter/nf_conntrack_rtsp.h>
#include <net/netfilter/nf_conntrack_expect.h>

#include <linux/inet.h>
#include <linux/ctype.h>
#define NF_NEED_STRNCASECMP
#define NF_NEED_STRTOU16
#include <linux/netfilter/netfilter_helpers.h>
#define NF_NEED_MIME_NEXTLINE
#include <linux/netfilter/netfilter_mime.h>
#include <linux/version.h>

#define MAX_PORTS       8
#define DSTACT_AUTO     0
#define DSTACT_STRIP    1
#define DSTACT_NONE     2

static char*    stunaddr = NULL;
static char*    destaction = "auto";

static u_int32_t extip = 0;
static int       dstact = 0;

MODULE_AUTHOR("Tom Marshall <tmarshall at real.com>");
MODULE_DESCRIPTION("RTSP network address translation module");
MODULE_LICENSE("GPL");
module_param(stunaddr, charp, 0644);
MODULE_PARM_DESC(stunaddr, "Address for detecting STUN");
module_param(destaction, charp, 0644);
MODULE_PARM_DESC(destaction, "Action for destination parameter (auto/strip/none)");

#define SKIP_WSPACE(ptr,len,off) while(off < len && isspace(*(ptr+off))) { off++; }

/*** helper functions ***/

static void
get_skb_tcpdata(struct sk_buff* skb, char** pptcpdata, uint* ptcpdatalen)
{
    struct iphdr*   iph  = ip_hdr(skb);
    struct tcphdr*  tcph = (void *)iph + ip_hdrlen(skb);

    *pptcpdata = (char*)tcph +  tcph->doff*4;
    *ptcpdatalen = ((char*)skb_transport_header(skb) + skb->len) - *pptcpdata;
}

/*** nat functions ***/
/* patch ... */
static int replace_contents(unsigned char *addr, uint old_len, char *new_data, uint new_len, uint total_len)
{
	int diff = new_len - old_len;

	memmove(addr+new_len, addr+old_len, total_len - old_len);
	memcpy(addr, new_data, new_len);

	return 1;
}

/*
 * Mangle the "Transport:" header:
 *   - Replace all occurences of "client_port=<spec>"
 *   - Handle destination parameter
 *
 * In:
 *   ct, ctinfo = conntrack context
 *   skb        = packet
 *   tranoff    = Transport header offset from TCP data
 *   tranlen    = Transport header length (incl. CRLF)
 *   rport_lo   = replacement low  port (host endian)
 *   rport_hi   = replacement high port (host endian)
 *
 * Returns packet size difference.
 *
 * Assumes that a complete transport header is present, ending with CR or LF
 */
static int
rtsp_mangle_tran(enum ip_conntrack_info ctinfo,
                 struct nf_conntrack_expect* rtp_exp,
                 struct nf_conntrack_expect* rtcp_exp,
								 struct ip_ct_rtsp_expect* prtspexp,
                 struct sk_buff* skb, uint tranoff, uint tranlen)
{
    char*       ptcp;
    char*       ptcp_seq;
    uint        tcplen;
    char*       ptran;
    char        rbuf1[16];      /* Replacement buffer (one port) */
    uint        rbuf1len;       /* Replacement len (one port) */
    char        rbufa[16];      /* Replacement buffer (all ports) */
    uint        rbufalen;       /* Replacement len (all ports) */
    u_int32_t   newip;
    u_int16_t   loport, hiport;
    uint        off = 0;
    uint        diff;           /* Number of bytes we removed */

int diff_len = 0;

#define MAX_RTSP_MANGLE_BUFFER 1400
char mangle_buffer[MAX_RTSP_MANGLE_BUFFER];
uint len_old = 0, len_new = 0;

    struct nf_conn *ct = rtp_exp->master;
    struct nf_conntrack_tuple *t_rtp, *t_rtcp;

    char    szextaddr[15+1];
    uint    extaddrlen;
    int     is_stun;

#if 1 /* patch ...*/
    ptcp = mangle_buffer;
    memset(ptcp, 0, MAX_RTSP_MANGLE_BUFFER);
    get_skb_tcpdata(skb, &ptcp_seq, &tcplen);
    memcpy(mangle_buffer, ptcp_seq, tcplen);
#else
    get_skb_tcpdata(skb, &ptcp, &tcplen);
#endif
    ptran = ptcp+tranoff;

    if (tranoff+tranlen > tcplen || tcplen-tranoff < tranlen ||
        tranlen < 10 || !iseol(ptran[tranlen-1]) ||
        nf_strncasecmp(ptran, "Transport:", 10) != 0)
    {
        pr_info("sanity check failed\n");
        return 0;
    }
    off += 10;
    SKIP_WSPACE(ptcp+tranoff, tranlen, off);

    newip = ct->tuplehash[IP_CT_DIR_REPLY].tuple.dst.u3.ip;

#ifdef CONFIG_NF_NAT_NEEDED
    rtp_exp->saved_proto.udp.port = rtp_exp->tuple.dst.u.udp.port;
    rtcp_exp->saved_proto.udp.port = rtcp_exp->tuple.dst.u.udp.port;
#endif

    t_rtp = &rtp_exp->tuple;
    t_rtcp = &rtcp_exp->tuple;

    t_rtp->dst.u3.ip = newip;
    t_rtcp->dst.u3.ip = newip;

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,36)
    extaddrlen = extip ? sprintf(szextaddr, "%u.%u.%u.%u", NIPQUAD(extip))
                       : sprintf(szextaddr, "%u.%u.%u.%u", NIPQUAD(newip));
#else
    extaddrlen = extip ? sprintf(szextaddr, "%pI4", &extip)
                       : sprintf(szextaddr, "%pI4", &newip);
#endif

    pr_debug("stunaddr=%s (%s)\n", szextaddr, (extip?"forced":"auto"));

    rbuf1len = rbufalen = 0;
    switch (prtspexp->pbtype)
    {
    case pb_single:
        for (loport = prtspexp->loport; loport != 0; loport++) /* XXX: improper wrap? */
        {
            t_rtp->dst.u.udp.port = htons(loport);
            if (nf_ct_expect_related(rtp_exp) == 0)
            {
                printk("using port %hu\n", loport);
                break;
            }
        }
        if (loport != 0)
        {
            rbuf1len = sprintf(rbuf1, "%hu", loport);
            rbufalen = sprintf(rbufa, "%hu", loport);
        }
        break;
    case pb_range:
        for (loport = prtspexp->loport; loport != 0; loport += 2) /* XXX: improper wrap? */
        {
            t_rtp->dst.u.udp.port = htons(loport);
            if (nf_ct_expect_related(rtp_exp) == 0)
            {
                t_rtcp->dst.u.udp.port = htons(loport+1);
                if (nf_ct_expect_related(rtcp_exp) == 0)
                {
                    hiport = loport+1;
                    pr_debug("using ports %d-%d\n", loport, loport+1);
                }
                else
                {
                    hiport = 0;
                }
                break;
            }
        }
        if (loport != 0 && hiport != 0)
        {
            rbuf1len = sprintf(rbuf1, "%hu", loport);
            rbufalen = sprintf(rbufa, "%hu-%hu", loport, hiport);
        }
        break;
    case pb_discon:
        for (loport = prtspexp->loport; loport != 0; loport++) /* XXX: improper wrap? */
        {
            t_rtp->dst.u.udp.port = htons(loport);
            if (nf_ct_expect_related(rtp_exp) == 0)
            {
                pr_debug("using port %hu (1 of 2)\n", loport);
                break;
            }
        }
        for (hiport = prtspexp->hiport; hiport != 0; hiport++) /* XXX: improper wrap? */
        {
            t_rtcp->dst.u.udp.port = htons(hiport);
            if (nf_ct_expect_related(rtcp_exp) == 0)
            {
                pr_debug("using port %hu (2 of 2)\n", hiport);
                break;
            }
        }
        if (loport != 0 && hiport != 0)
        {
            rbuf1len = sprintf(rbuf1, "%hu", loport);
            if (hiport == loport+1)
            {
                rbufalen = sprintf(rbufa, "%hu-%hu", loport, hiport);
            }
            else
            {
                rbufalen = sprintf(rbufa, "%hu/%hu", loport, hiport);
            }
        }
        break;
    }

    if (rbuf1len == 0)
    {
        return 0;   /* cannot get replacement port(s) */
    }

    /* Transport: tran;field;field=val,tran;field;field=val,... */
    while (off < tranlen)
    {
        uint        saveoff;
        const char* pparamend;
        uint        nextparamoff;

        pparamend = memchr(ptran+off, ',', tranlen-off);
        pparamend = (pparamend == NULL) ? ptran+tranlen : pparamend+1;
        //nextparamoff = pparamend-ptcp;
nextparamoff = pparamend-ptran;
        /*
         * We pass over each param twice.  On the first pass, we look for a
         * destination= field.  It is handled by the security policy.  If it
         * is present, allowed, and equal to our external address, we assume
         * that STUN is being used and we leave the client_port= field alone.
         */
        is_stun = 0;
        saveoff = off;
        while (off < nextparamoff)
        {
            const char* pfieldend;
            uint        nextfieldoff;
            uint        replen;

            pfieldend = memchr(ptran+off, ';', nextparamoff-off);
            nextfieldoff = (pfieldend == NULL) ? nextparamoff : pfieldend-ptran+1;

            if (dstact != DSTACT_NONE && strncmp(ptran+off, "destination=", 12) == 0)
            {
                if (strncmp(ptran+off+12, szextaddr, extaddrlen) == 0)
                {
                    is_stun = 1;
                }
                if (dstact == DSTACT_STRIP || (dstact == DSTACT_AUTO && !is_stun))
                {
                    off += 12;
                    replen = (pfieldend == NULL) ? nextfieldoff-off : nextfieldoff-off-1;
                    diff = replen - extaddrlen;

#if 1
		    if( replace_contents(ptran+off, replen, szextaddr, extaddrlen, tcplen-off-(ptran-ptcp)) == 0) 
                    {
                        // mangle failed, all we can do is bail 
			nf_ct_unexpect_related(rtp_exp);
			nf_ct_unexpect_related(rtcp_exp);
                        return 0;
                    }

		    tcplen = tcplen + extaddrlen - replen;
		    diff_len = diff_len + extaddrlen - replen;

#else

                    if (!nf_nat_mangle_tcp_packet(skb, ct, ctinfo,
                                                         off, diff, NULL, 0))
                    {
                        // mangle failed, all we can do is bail 
			nf_ct_unexpect_related(rtp_exp);
			nf_ct_unexpect_related(rtcp_exp);
                        return 0;
                    }
#endif
#if 0
                    get_skb_tcpdata(skb, &ptcp, &tcplen);
#endif
                    ptran = ptcp+tranoff;
                    tranlen -= diff;
                    nextparamoff -= diff;
                    nextfieldoff -= diff;
                }
            }

            off = nextfieldoff;
        }
        if (is_stun)
        {
            continue;
        }
        off = saveoff;
        while (off < nextparamoff)
        {
            const char* pfieldend;
            uint        nextfieldoff;

            pfieldend = memchr(ptran+off, ';', nextparamoff-off);
            nextfieldoff = (pfieldend == NULL) ? nextparamoff : pfieldend-ptran+1;

            if (strncmp(ptran+off, "client_port=", 12) == 0)
            {
                u_int16_t   port;
                uint        numlen;
                uint        origoff;
                uint        origlen;
                char*       rbuf    = rbuf1;
                uint        rbuflen = rbuf1len;
                uint off1;

                off += 12;
                off1 = off;
                origoff = (ptran-ptcp)+off;
                origlen = 0;
                numlen = nf_strtou16(ptran+off, &port);
                off += numlen;
                origlen += numlen;
                if (port != prtspexp->loport)
                {
                    pr_debug("multiple ports found, port %hu ignored\n", port);
                }
                else
                {
                    if (ptran[off] == '-' || ptran[off] == '/')
                    {
                        off++;
                        origlen++;
                        numlen = nf_strtou16(ptran+off, &port);
                        off += numlen;
                        origlen += numlen;
                        rbuf = rbufa;
                        rbuflen = rbufalen;
                    }

                    /*
                     * note we cannot just memcpy() if the sizes are the same.
                     * the mangle function does skb resizing, checks for a
                     * cloned skb, and updates the checksums.
                     *
                     * parameter 4 below is offset from start of tcp data.
                     */
                    diff = origlen-rbuflen;
#if 1

		    if( replace_contents(ptran+off1, origlen, rbuf, rbuflen, tcplen-off1-(ptran-ptcp)) == 0) 
                    {
                        /* mangle failed, all we can do is bail */
			nf_ct_unexpect_related(rtp_exp);
			nf_ct_unexpect_related(rtcp_exp);
                        return 0;
                    }
		    tcplen = tcplen + rbuflen - origlen;
		    diff_len = diff_len + rbuflen - origlen;


#else
                    if (!nf_nat_mangle_tcp_packet(skb, ct, ctinfo,
                                              origoff, origlen, rbuf, rbuflen))
                    {
                        /* mangle failed, all we can do is bail */
			nf_ct_unexpect_related(rtp_exp);
			nf_ct_unexpect_related(rtcp_exp);
                        return 0;
                    }
#endif
#if 0
                    get_skb_tcpdata(skb, &ptcp, &tcplen);
#endif
                    ptran = ptcp+tranoff;
                    tranlen -= diff;
                    nextparamoff -= diff;
                    nextfieldoff -= diff;
                }
            }

            off = nextfieldoff;
        }

        off = nextparamoff;
    }

	if (!nf_nat_mangle_tcp_packet(skb, ct, ctinfo, 0, tcplen-diff_len, mangle_buffer, tcplen))
	{
		/* mangle failed, all we can do is bail */
		//DEBUGP(" mangle failed client_port\n");
		nf_ct_unexpect_related(rtp_exp);
			nf_ct_unexpect_related(rtcp_exp);
		return 0;
	}
    return 1;
}

static uint
help_out(struct sk_buff *skb, enum ip_conntrack_info ctinfo,
	 unsigned int matchoff, unsigned int matchlen, struct ip_ct_rtsp_expect* prtspexp, 
	 struct nf_conntrack_expect* rtp_exp, struct nf_conntrack_expect *rtcp_exp)
{
    char*   ptcp;
    uint    tcplen;
    uint    hdrsoff;
    uint    hdrslen;
    uint    lineoff;
    uint    linelen;
    uint    off;

    //struct iphdr* iph = (struct iphdr*)(*pskb)->nh.iph;
    //struct tcphdr* tcph = (struct tcphdr*)((void*)iph + iph->ihl*4);

    get_skb_tcpdata(skb, &ptcp, &tcplen);
    hdrsoff = matchoff;//exp->seq - ntohl(tcph->seq);
    hdrslen = matchlen;
    off = hdrsoff;
    pr_debug("NAT rtsp help_out\n");

    while (nf_mime_nextline(ptcp, hdrsoff+hdrslen, &off, &lineoff, &linelen))
    {
        if (linelen == 0)
        {
            break;
        }
        if (off > hdrsoff+hdrslen)
        {
            pr_info("!! overrun !!");
            break;
        }
        pr_debug("hdr: len=%u, %.*s", linelen, (int)linelen, ptcp+lineoff);

        if (nf_strncasecmp(ptcp+lineoff, "Transport:", 10) == 0)
        {
            uint oldtcplen = tcplen;
	    pr_debug("hdr: Transport\n");
            if (!rtsp_mangle_tran(ctinfo, rtp_exp, rtcp_exp, prtspexp, skb, lineoff, linelen))
            {
		pr_debug("hdr: Transport mangle failed");
                break;
            }
            get_skb_tcpdata(skb, &ptcp, &tcplen);
            hdrslen -= (oldtcplen-tcplen);
            off -= (oldtcplen-tcplen);
            lineoff -= (oldtcplen-tcplen);
            linelen -= (oldtcplen-tcplen);
            pr_debug("rep: len=%u, %.*s", linelen, (int)linelen, ptcp+lineoff);
        }
    }

    return NF_ACCEPT;
}

static unsigned int
help(struct sk_buff *skb, enum ip_conntrack_info ctinfo, 
     unsigned int matchoff, unsigned int matchlen, struct ip_ct_rtsp_expect* prtspexp,
     struct nf_conntrack_expect* rtp_exp, struct nf_conntrack_expect* rtcp_exp)
{
    int dir = CTINFO2DIR(ctinfo);
    int rc = NF_ACCEPT;

    switch (dir)
    {
    case IP_CT_DIR_ORIGINAL:
        rc = help_out(skb, ctinfo, matchoff, matchlen, prtspexp, rtp_exp, rtcp_exp);
        break;
    case IP_CT_DIR_REPLY:
	pr_debug("unmangle ! %u\n", ctinfo);
    	/* XXX: unmangle */
	rc = NF_ACCEPT;
        break;
    }
    //UNLOCK_BH(&ip_rtsp_lock);

    return rc;
}

static void expected(struct nf_conn* ct, struct nf_conntrack_expect *exp)
{
    struct nf_nat_multi_range_compat mr;
    u_int32_t newdstip, newsrcip, newip;

    struct nf_conn *master = ct->master;

    newdstip = master->tuplehash[IP_CT_DIR_ORIGINAL].tuple.src.u3.ip;
    newsrcip = ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple.src.u3.ip;
    //FIXME (how to port that ?)
    //code from 2.4 : newip = (HOOK2MANIP(hooknum) == IP_NAT_MANIP_SRC) ? newsrcip : newdstip;
    newip = newdstip;

    //pr_debug("newsrcip=%u.%u.%u.%u, newdstip=%u.%u.%u.%u, newip=%u.%u.%u.%u\n",
      //     NIPQUAD(newsrcip), NIPQUAD(newdstip), NIPQUAD(newip));
    pr_debug("newsrcip=%pI4, newdstip=%pI4, newip=%pI4\n",
           &newsrcip, &newdstip, &newip);

    mr.rangesize = 1;
#ifdef CONFIG_NF_NAT_NEEDED
    mr.range[0].flags = (IP_NAT_RANGE_MAP_IPS | IP_NAT_RANGE_PROTO_SPECIFIED);
    mr.range[0].min = mr.range[0].max = exp->saved_proto;
#else
    mr.range[0].flags = IP_NAT_RANGE_MAP_IPS;
#endif
    mr.range[0].min_ip = mr.range[0].max_ip = newip;

    nf_nat_setup_info(ct, &mr.range[0], IP_NAT_MANIP_DST);
}


static void __exit fini(void)
{
	nf_nat_rtsp_hook = NULL;
        nf_nat_rtsp_hook_expectfn = NULL;
	synchronize_net();
}

static int __init init(void)
{
	printk("nf_nat_rtsp v" IP_NF_RTSP_VERSION " loading\n");

	BUG_ON(nf_nat_rtsp_hook);
	nf_nat_rtsp_hook = help;
        nf_nat_rtsp_hook_expectfn = &expected;

	if (stunaddr != NULL)
		extip = in_aton(stunaddr);

	if (destaction != NULL) {
	        if (strcmp(destaction, "auto") == 0)
			dstact = DSTACT_AUTO;

		if (strcmp(destaction, "strip") == 0)
			dstact = DSTACT_STRIP;

		if (strcmp(destaction, "none") == 0)
			dstact = DSTACT_NONE;
	}

	return 0;
}

module_init(init);
module_exit(fini);
