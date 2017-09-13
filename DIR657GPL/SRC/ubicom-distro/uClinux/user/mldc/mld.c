



#include "mld.h"



struct in6_addr anma = {{{0xFF, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1}}};




void querier(struct ifrx *pifrx, int sec, char start) {
	pifrx->isquerier = 1;
	pifrx->isstartupq = start?((0==start)?0:1):1;
	pifrx->r_qry_cnt = 0;
	pifrx->s_qry_cnt = 0;

	Smldqry(pifrx->ifidx, 0, 0);
	set_timer(&(pifrx->tm_qry), (sec)?(sec):(pifrx->start_qry_itvl));
}


void nonquerier(struct ifrx *pifrx, int sec) {
	pifrx->isquerier = 0;
	pifrx->r_qry_cnt = 0;
	set_timer(&(pifrx->tm_nonqry), (sec)?(sec):(pifrx->oqrier_itvl));
}


int Smldqry(        int       ifidx,
             struct in6_addr *ma,
                    uint16_t  maxdelay) {
	char  buf[256];
	struct ifrx *pifrx=0;
	struct ip6_hdr *ph6=(struct ip6_hdr*)buf;
	struct ip6ex *pex=(struct ip6ex*)(buf+sizeof(struct ip6_hdr));
	struct ip6orx *por0=(struct ip6orx*)(buf+sizeof(struct ip6_hdr)+sizeof(struct ip6ex));
	struct ip6orx *por1=(struct ip6orx*)(buf+sizeof(struct ip6_hdr)+sizeof(struct ip6ex)+sizeof(struct ip6orx));
	struct mld1hx *pm1x=(struct mld1hx*)(buf+sizeof(struct ip6_hdr)+sizeof(struct ip6ex)+sizeof(struct ip6orx)+2);
	struct sockaddr_ll  sa;
	int sd, len, ret;

	if (!(pifrx=getifrx(ifidx))) {
		return -1;
	}

	if (-1==(sd=socket(PF_PACKET, SOCK_DGRAM, htons(ETH_P_IPV6)))) {
		return -1;
	}

	memset((void*)&sa, 0, sizeof(sa));
	memset((void*)buf, 0, sizeof(buf));
	memcpy((void*)&(ph6->ip6_src),  (void*)&(pifrx->addr), sizeof(struct in6_addr));

	if (ma) {
		memcpy((void*)&(pm1x->maddr),  (void*)ma, sizeof(struct in6_addr));
		memcpy((void*)&(ph6->ip6_dst), (void*)ma, sizeof(struct in6_addr));
	}else
		memcpy((void*)&(ph6->ip6_dst), (void*)&anma, sizeof(struct in6_addr));

	ph6->ip6_vfc = 96;
	ph6->ip6_plen = sizeof(struct ip6ex)+sizeof(struct ip6orx)+2+sizeof(struct mld1hx);
	ph6->ip6_hlim = 1;
	pex->ip6e_nxt = 58;
	por0->ip6or_type = 5;
	por0->ip6or_len = 2;
	por1->ip6or_type = 1;
	pm1x->type = MLD_LISTENER_QUERY;
	pm1x->cksum = hsum(ph6, sizeof(struct mld1hx), 58);
	pm1x->maxdelay = (maxdelay?maxdelay:DFT_QUERY_RESP_ITVL);
	pm1x->cksum = ~(cksum((void*)pm1x, (sizeof(struct mld1hx)+1)/2));

	sa.sll_family = AF_PACKET;
	sa.sll_protocol = htons(ETH_P_IPV6);
	sa.sll_ifindex = ifidx;
	sa.sll_halen = 6;
	sa.sll_addr[0] = 0x33;
	sa.sll_addr[1] = 0x33;
	memcpy((sa.sll_addr)+2, (ph6->ip6_dst.s6_addr)+12, 4);

	len = sizeof(struct ip6_hdr) + (ph6->ip6_plen);
	if (0>=(ret=sendto(sd, (const void*)buf, len, 0, (const struct sockaddr*)&sa, (socklen_t)sizeof(struct sockaddr_ll)))) {
		close(sd);
		return -1;
	}
	(pifrx->s_qry_cnt)++;
	close(sd);
	return 0;
}


int Rmldqry(struct sockaddr_ll *psa,
                   char        *data,
                   int          dlen,
            struct mld1hx      *pm1h)
{
	struct in6_addr *psrc  = &(((struct ip6_hdr*)data)->ip6_src);
	struct ifrx     *pifrx =  getifrx(psa->sll_ifindex);

	if (pifrx) {
		int i=0;
		(pifrx->r_qry_cnt)++;
		for (i=0;i<16;i++) {
			if ((psrc->s6_addr[i])<(pifrx->addr.s6_addr[i])) {
				nonquerier(pifrx, 0);
				break;
			}else if ((psrc->s6_addr[i])>(pifrx->addr.s6_addr[i])) {
				break;
			}
		}
	}

	if (!(pifrx->isquerier) && (pm1h->maddr.s6_addr[1])) {
		struct mldgroupx *p=0;
		int ret=0;

		ret = findgp(&(pm1h->maddr), pifrx, &p);
		if (1==ret) {
			set_timer(&(p->tm_alive), pm1h->maxdelay);
		}
	}

	return 0;
}


int Rmlddne(struct sockaddr_ll* psa, char *data, int dlen, struct mld1hx *pm1h) {
	struct mldgroupx *p=0;
	struct ifrx *pifrx=getifrx(psa->sll_ifindex);
	int ret=0;

	if (!pifrx) {
		return -1;
	}else if (!(pifrx->isquerier)) {
		return -1;
	}

	ret = findgp(&(pm1h->maddr), pifrx, &p);
	if (1==ret) {
		Smldqry(p->ifx->ifidx, &(p->ma), p->ifx->last_lisenr_qry_itvl);
		p->s_masqry_cnt = 1;
		usleep(5000);
		set_timer(&(p->tm_masqry), p->ifx->last_lisenr_qry_itvl);
		usleep(5000);
	}else
		return -1;

	return 0;
}


int Rmldrpt( struct sockaddr_ll *psa,
                    char        *data,
                    int          dlen,
             struct mld1hx      *pm1h)
{
	int ret=0, retadd=0, sd=0;
	struct mldgroupx *pmg=0;
	struct greqx greq;
	struct ifrx *pifrx=getifrx(psa->sll_ifindex);;

	if (!pifrx) {
		return -1;
	}

	if (3>=(pm1h->maddr.s6_addr[1]&0xf)) {
		return -1;
	}

	if (-1==(retadd=addgp((struct in6_addr*)&(pm1h->maddr), pifrx, &pmg))) {
		return -1;
	}
	++(pmg->r_rpt_cnt);

	clear_timer(&(pmg->tm_masqry));
	set_timer(&(pmg->tm_alive), pifrx->mcast_lisenr_itvl);

	if (1==retadd)
		return 0;

	if (-1==(sd=socket(AF_INET6, SOCK_RAW, IPPROTO_IPV6))) {
		return -1;
	}

	memset(&greq, 0, sizeof(greq));
	greq.ifidx = ((psa->sll_ifindex)==(aifr[0].ifidx))?(aifr[1].ifidx):(aifr[0].ifidx);
	greq.sai6.sin6_family = AF_INET6;

	memcpy( (void*)&(greq.sai6.sin6_addr), (void*)&(pm1h->maddr), sizeof(pm1h->maddr) );

	ret = setsockopt(sd, SOL_IPV6, MCAST_JOIN_GROUP, (void*)&greq, sizeof(greq));
	if (0!=ret) {
		close(sd);
		return -1;
	}
	close(sd);

	return 0;
}

/* MLDv2 */

int Rmldrptv2( struct sockaddr_ll *psa,
                    char        *data,
                    int          dlen,
             struct mld2hx      *pm2h)
{
	int ret=0, retadd=0, sd=0;
	struct mldgroupx *pmg=0;
	struct greqx greq;
	struct ifrx *pifrx=getifrx(psa->sll_ifindex);


	
	int i;
	for(i=0;i<16;i++)
		printf("%x",pm2h->maddr.s6_addr[i]);
	

	if (!pifrx)
		return -1;

	if (3>=(pm2h->maddr.s6_addr[1]&0xf)) 
		return -1;
	
	if(pm2h->record == MODE_IS_EXCLUDE) {
		if (-1==(retadd=addgp((struct in6_addr*)&(pm2h->maddr), pifrx, &pmg))) {
			return -1;
		}
		++(pmg->r_rpt_cnt);

		clear_timer(&(pmg->tm_masqry));
		set_timer(&(pmg->tm_alive), pifrx->mcast_lisenr_itvl);

		if (1==retadd)
			return 0;

		if (-1==(sd=socket(AF_INET6, SOCK_RAW, IPPROTO_IPV6))) {
			return -1;
		}

		memset(&greq, 0, sizeof(greq));
		greq.ifidx = ((psa->sll_ifindex)==(aifr[0].ifidx))?(aifr[1].ifidx):(aifr[0].ifidx);
		greq.sai6.sin6_family = AF_INET6;

		memcpy( (void*)&(greq.sai6.sin6_addr), (void*)&(pm2h->maddr), sizeof(pm2h->maddr) );

		ret = setsockopt(sd, SOL_IPV6, MCAST_JOIN_GROUP, (void*)&greq, sizeof(greq));
		if (0!=ret) {
			close(sd);
			return -1;
		}
		close(sd);

		return 0;
	} else if(pm2h->record == MODE_IS_INCLUDE) {
		if (!(pifrx->isquerier)) 
			return -1;

		ret = findgp(&(pm2h->maddr), pifrx, &pmg);
		if (1==ret) {
			mldv2_send_include_mode(pmg->ifx->ifidx, &(pmg->ma));
			pmg->s_masqry_cnt = 1;
			usleep(5000);
			set_timer(&(pmg->tm_masqry), pmg->ifx->last_lisenr_qry_itvl);
			usleep(5000);
		}else
			return -1;

		return 0;
		
	} 
}

int mldv2_send_include_mode(int ifidx,struct in6_addr *ma) 
{
	char  buf[256];
	struct ifrx *pifrx=0;
	struct ip6_hdr *ph6=(struct ip6_hdr*)buf;
	struct ip6ex *pex=(struct ip6ex*)(buf+sizeof(struct ip6_hdr));
	struct ip6orx *por0=(struct ip6orx*)(buf+sizeof(struct ip6_hdr)+sizeof(struct ip6ex));
	struct ip6orx *por1=(struct ip6orx*)(buf+sizeof(struct ip6_hdr)+sizeof(struct ip6ex)+sizeof(struct ip6orx));
	struct mld2hx *pm2x=(struct mld1hx*)(buf+sizeof(struct ip6_hdr)+sizeof(struct ip6ex)+sizeof(struct ip6orx)+2);
	struct sockaddr_ll  sa;
	int sd, len, ret;

	if (!(pifrx=getifrx(ifidx))) {
		return -1;
	}

	if (-1==(sd=socket(PF_PACKET, SOCK_DGRAM, htons(ETH_P_IPV6)))) {
		return -1;
	}

	memset((void*)&sa, 0, sizeof(sa));
	memset((void*)buf, 0, sizeof(buf));
	memcpy((void*)&(ph6->ip6_src),  (void*)&(pifrx->addr), sizeof(struct in6_addr));

	if (ma) {
		memcpy((void*)&(pm2x->maddr),  (void*)ma, sizeof(struct in6_addr));
		memcpy((void*)&(ph6->ip6_dst), (void*)ma, sizeof(struct in6_addr));
	}else
		memcpy((void*)&(ph6->ip6_dst), (void*)&anma, sizeof(struct in6_addr));

	ph6->ip6_vfc = 96;
	ph6->ip6_plen = sizeof(struct ip6ex)+sizeof(struct ip6orx)+2+sizeof(struct mld1hx);
	ph6->ip6_hlim = 1;
	pex->ip6e_nxt = 58;
	por0->ip6or_type = 5;
	por0->ip6or_len = 2;
	por1->ip6or_type = 1;
	pm2x->type = MLDv2_LISTENER_REPORT;
	pm2x->cksum = hsum(ph6, sizeof(struct mld2hx), 58);
	pm2x->cksum = ~(cksum((void*)pm2x, (sizeof(struct mld2hx)+1)/2));
	pm2x->record = MODE_IS_INCLUDE;

	sa.sll_family = AF_PACKET;
	sa.sll_protocol = htons(ETH_P_IPV6);
	sa.sll_ifindex = ifidx;
	sa.sll_halen = 6;
	sa.sll_addr[0] = 0x33;
	sa.sll_addr[1] = 0x33;
	memcpy((sa.sll_addr)+2, (ph6->ip6_dst.s6_addr)+12, 4);

	len = sizeof(struct ip6_hdr) + (ph6->ip6_plen);
	
	
	if (0>=(ret=sendto(sd, (const void*)buf, len, 0, (const struct sockaddr*)&sa, (socklen_t)sizeof(struct sockaddr_ll)))) {
		close(sd);
		return -1;
	}
	(pifrx->s_qry_cnt)++;
	close(sd);
	return 0;
}
