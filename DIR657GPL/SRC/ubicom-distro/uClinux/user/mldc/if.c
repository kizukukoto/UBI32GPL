



#include "mld.h"





void* getifrx(int ifidx) {
	int i=0;
	for (i=0; i<MAX_IF_NUM; i++) {
		if (ifidx==aifr[i].ifidx)
			return (void*)&(aifr[i]);
	}
	return 0;
}


int getaddr(char *name, struct in6_addr *pa) {
	char  buf[64];
	FILE *f;

	if (!(f=fopen(_PATH_ADDRX_, "r"))) {
		return -1;
	}

	if (!pa) {
		return -1;
	}

	memset(buf, 0, sizeof(buf));
	while (fgets(buf, sizeof(buf), f)) {
		static int cnt0;

		if (strstr(buf, name)) {
			int i=0, j=0;
			static int cnt1;
			for (i=0; i<32; i++) {
				if ((48<=buf[i]) && (57>=buf[i])) {
					j += buf[i] - 48;
				}else if ((65<=buf[i]) && (70>=buf[i])) {
					j += buf[i] - 55;
				}else if ((97<=buf[i]) && (102>=buf[i])) {
					j += buf[i] - 87;
				}
				if (0==(i%2)) {
					j *= 16;
				}else{
					if ( ((1==i)&&(254!=j)) || ((3==i)&&(128!=(192&j))) ) {
						memset(pa,  0, sizeof(struct in6_addr));
						memset(buf, 0, sizeof(buf));
						break;
					}
					pa->s6_addr[(i-1)/2] = j;
					j = 0;
				}
			}

			if (32>i)
				continue;

			fclose(f);
			return 0;
		}
	}
	fclose(f);
	return -1;
}


int ifinit(char *lan, char *wan) {

	ifconf(lan, &(aifr[0]));
	sleep(1);
	ifconf(wan, &(aifr[1]));
	ifcnt = 2;

	return 0;
}


int ifconf(char *ifname, struct ifrx *pifrx) {
	int sd;
	struct ifreq ifr;

	if (0>(sd=socket(AF_INET, SOCK_RAW, IPPROTO_RAW)))
		return -1;
	memset((void*)pifrx,  0, sizeof(struct ifrx));

	// name
	strncpy(pifrx->name, ifname, IFNAMSIZ);

	// ifidx
	memset((void*)&ifr,  0, sizeof(ifr));
	strncpy(ifr.ifr_name, pifrx->name, IFNAMSIZ);
	ioctl(sd, SIOCGIFINDEX, &ifr);
	pifrx->ifidx = ifr.ifr_ifindex;

	// addr
	getaddr(pifrx->name, &(pifrx->addr));

	// var
	pifrx->robust_var        = DFT_ROBUST_VAR;
	pifrx->qry_itvl          = DFT_QUERY_ITVL;
	pifrx->qry_resp_itvl     = DFT_QUERY_RESP_ITVL;
	pifrx->mcast_lisenr_itvl = ((pifrx->robust_var)*(pifrx->qry_itvl)) + (pifrx->qry_resp_itvl);
	pifrx->oqrier_itvl       = ((pifrx->robust_var)*(pifrx->qry_itvl)) + ((pifrx->qry_resp_itvl)/2);
	pifrx->start_qry_itvl    = (pifrx->qry_itvl)/4;
	pifrx->start_qry_cnt     = pifrx->robust_var;
	pifrx->last_lisenr_qry_itvl = DFT_LAST_LISTENER_QUERY_ITVL;
	pifrx->last_lisenr_qry_cnt  = pifrx->robust_var;

	init_timer(&(pifrx->tm_qry),    htime_gquery,     (void*)pifrx);
	init_timer(&(pifrx->tm_nonqry), htime_nonquerier, (void*)pifrx);

	querier(pifrx, 0, 1);

	close(sd);
	return 0;
}


int getmtu(char *ifname) {
	int sd;
	struct ifreq ifr;

	if (0>(sd=socket(AF_INET, SOCK_RAW, IPPROTO_RAW)))
		return -1;
	memset((void*)&ifr,  0, sizeof(ifr));
	strncpy(ifr.ifr_name, ifname, IFNAMSIZ);
	if (0!=ioctl(sd, SIOCGIFMTU, &ifr)) {
		close(sd);
		return -1;
	}
	close(sd);
	return ifr.ifr_mtu;
}

