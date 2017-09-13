



#include "mld.h"
int gpcnt;




//
// return:  0, NOT found;
//          1, found;
//         -2, qrplist has no head, empty;
//
int findgp(struct in6_addr *ma, struct ifrx *pi, struct mldgroupx **pp) {
	struct mldgroupx *p=pgmg;

	if (!pgmg) {
		*pp = pgmg;
		return -2;
	}

	while (1) {
		if (IN6_ARE_ADDR_EQUAL(ma,(&(p->ma)))) {
			if (pi) {
				if (pi->ifidx!=(p->ifx->ifidx)) {
					goto nextgp;
				}
			}
			*pp = p; //found
			return 1;
		}
nextgp:
		if (p->nxt) {
			p = p->nxt;
		}else{
			*pp = p; //reach end of list
			return 0; //not found
		}
	}
	return 0; //not found
}


//
// return:  0, add OK;
//          1, existing;
//         -1, Fail;
//
int addgp(struct in6_addr *ma, struct ifrx *pi, struct mldgroupx **pp) {
	static unsigned char gcnt;
	int ret=0;
	struct mldgroupx *p=0, *p1=0;

//	if (128<=gcnt) {
//		printf("too many groups\n");
//		return -1;
//	}

	if (!pi)
		return -1;

	ret = findgp(ma, pi, &p);
	if (1==ret) {
		*pp = p;
//		++((*pp)->cnt);
		return 1;
	}

	if (!(p1=(struct mldgroupx*)malloc(sizeof(struct mldgroupx)))) {
		return -1;
	}

	memset(p1, 0, sizeof(struct mldgroupx));
	memcpy(&(p1->ma), ma, sizeof(struct in6_addr));
	p1->ifx=pi;

	if (0==ret) {
		p->nxt = p1;
		p1->pre = p;
	}else if (-2==ret) {
		pgmg = p1;
	}

//	++((*pp)->cnt);
	++gcnt;
	++gpcnt;

	init_timer(&(p1->tm_masqry), htime_masqry, (void*)p1);
	init_timer(&(p1->tm_alive),  htime_galive, (void*)p1);

	*pp = p1;
	return 0;
}


int delgp(struct in6_addr *ma, struct ifrx *pi) {
	int ret=0, delh=0;
	struct mldgroupx *p=0;

	if (!pi)
		return -1;

	ret = findgp(ma, pi, &p);
	if (1==ret) {
//		if (0<(--(p->cnt)))
//			return 0;
		if (p->pre) {
			p->pre->nxt = p->nxt;
		}else
			delh = 1;	//should be Head

		if (p->nxt) {
			p->nxt->pre = p->pre;
		}

		if (delh) { //del Head
			pgmg = pgmg->nxt;
		}

		clear_timer(&(p->tm_masqry));
		clear_timer(&(p->tm_alive));
		memset(p, 0, sizeof(struct mldgroupx));
		free(p);
		--gpcnt;
		return 0;
	}
	return -1;
}

