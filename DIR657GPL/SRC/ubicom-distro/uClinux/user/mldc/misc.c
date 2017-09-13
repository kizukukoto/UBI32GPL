



#include "mld.h"









int chk_hbhopt(char *data, int dlen) {
	if (IPPROTO_HOPOPTS==((struct ip6_hdr*)data)->ip6_nxt) {
		uint8_t *ptr = data + sizeof(struct ip6_hdr) + 2;
		if (ptr<(uint8_t*)(data+sizeof(struct ip6_hdr)+(((((struct ip6ex*)(data+sizeof(struct ip6_hdr)))->ip6e_len)+1)*8))) {
			switch (((struct ip6orx*)ptr)->ip6or_type) {
			case 5:
			case 1:
			case 0:
				return 0;
			default:
				if (0xc0&(((struct ip6orx*)ptr)->ip6or_type)) {
					return -1;
				}
				return 0;
			}
		}
	}
	return 0;
}


int ismld(struct sockaddr_ll *psa, char *data, int dlen) {
	struct ip6_hdr *ph6=(struct ip6_hdr*)data;
	struct icmp6_hdr *pic=0;
	struct ip6ex *pex=(struct ip6ex*)(data+sizeof(struct ip6_hdr));
	struct ip6orx *por=0;
	uint32_t  loc=sizeof(struct ip6_hdr), icmplen=0;
	uint8_t  nxt=ph6->ip6_nxt;
	uint8_t  ismra=0;

	while ((loc<dlen) && (IPPROTO_ICMPV6!=nxt)) {
		if (IPPROTO_HOPOPTS==nxt) {
			uint8_t *ptr    = data + loc + 2;
			while (ptr < (uint8_t*)(data+loc+((pex->ip6e_len)+1)*8)) {
				por = (struct ip6orx*)ptr;
				if (5==por->ip6or_type) {
					if ((2==por->ip6or_len) && (0==por->ip6or_value[0]) && (0==por->ip6or_value[1])) {
						ismra = 1;
					}
				}
				ptr = ptr + por->ip6or_len + 2;
			}
		}
		nxt  = pex->ip6e_nxt;
		loc += (pex->ip6e_len + 1) * 8;
		pex  = (struct ip6ex*)(data + loc);
	}

	if (!ismra)
		return 0;

	if (IPPROTO_ICMPV6!=nxt)
		return 0;

	pic = (struct icmp6_hdr*)pex;
	icmplen = (uint32_t)((char*)pic - data);

	switch (pic->icmp6_type) {
	case MLD_LISTENER_QUERY:
		if (24>icmplen)
			return 0;
		Rmldqry(psa, data, dlen, (struct mld1hx*)pic);
		return 1;

	case MLD_LISTENER_REPORT:
		//printf("XXX %s[%d] MLD_LISTENER_REPORT:%d icmplen:%d XXX\n",__func__,__LINE__,MLD_LISTENER_REPORT,icmplen);
		if (24>icmplen)
			return 0;
		Rmldrpt(psa, data, dlen, (struct mld1hx*)pic);
		return 1;

	case MLDv2_LISTENER_REPORT:
		//printf("XXX %s[%d] MLDv2_LISTENER_REPORT:%d icmplen:%d XXX\n",__func__,__LINE__,MLDv2_LISTENER_REPORT,icmplen);
		if (24>icmplen)
			return 0;
		

		Rmldrptv2(psa, data, dlen, (struct mld2hx*)pic);
		return 1;

	case MLD_LISTENER_REDUCTION:
		if (24>icmplen)
			return 0;
		Rmlddne(psa, data, dlen, (struct mld1hx*)pic);
		return 1;
	}
	return 0;
}


int sendd2big(char *data, int dlen, int mtu) {
	char  buf[1280];
	struct ip6_hdr *ph6=(struct ip6_hdr*)data;
	struct icmp6_hdr *pi6=(struct icmp6_hdr*)buf;
	struct sockaddr_in6  sin6;
	int sd, len0, len1, ret;

	memset(buf, 0, sizeof(buf));
	sd = socket(PF_INET6, SOCK_RAW, IPPROTO_ICMPV6);
	if (-1==sd) {
		return -1;
	}

	memset(&sin6, 0, sizeof(sin6));
	sin6.sin6_family = AF_INET6;
	memcpy(&(sin6.sin6_addr), &(ph6->ip6_src), sizeof(sin6.sin6_addr));

	pi6->icmp6_type = ICMP6_PACKET_TOO_BIG;
	pi6->icmp6_mtu = mtu;

	len0 = sizeof(buf)-sizeof(struct ip6_hdr)-sizeof(struct icmp6_hdr);
	len1 = (dlen>len0)?len0:dlen;
	memcpy(buf+sizeof(struct icmp6_hdr), data, len1);

	ret = sendto(sd, (const void*)buf, len1, 0, (const struct sockaddr*)&sin6, (socklen_t)sizeof(sin6));
	if (0>=ret) {
		close(sd);
		return -1;
	}
	close(sd);
	return 0;
}


uint16_t cksum(void *ptr, int count) {
	uint16_t  checksum;
	uint16_t *addr=(uint16_t*)ptr;
	uint32_t  sum=0;

	count += count;

	while (count>1) {
		sum += *addr++;
		count -= 2;
	}

	if (count>0)
		sum += *(uint8_t*)addr;

	while (sum>>16)
		sum = (sum & 0xffff) + (sum>>16);

	checksum = (uint16_t)sum;
	return checksum;
}


uint16_t hsum(struct ip6_hdr *ph6, uint32_t len, uint8_t nh) {
	uint32_t  tmp;
	uint32_t  len0=htonl(len);

	/* add the various fields to the 32 bit tmp sum */
	tmp = (uint32_t)cksum(&(ph6->ip6_src), sizeof(struct in6_addr));
	tmp += (uint32_t)(htons(nh));

	tmp += len0 & 0x0000FFFF;
	tmp += len0 >> 16;

	/* wrap carry bits back into lower 16 bits */
	while(tmp & 0xFFFF0000)
		tmp = (tmp & 0x0000FFFF) + (tmp >> 16);

	return (uint16_t)tmp;
}


