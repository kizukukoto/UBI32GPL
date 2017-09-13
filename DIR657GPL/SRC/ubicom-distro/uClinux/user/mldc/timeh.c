



#include "mld.h"





void htime_galive(void *data) {
	delgp(&(((struct mldgroupx*)data)->ma), ((struct mldgroupx*)data)->ifx);
}


void htime_masqry(void *data) {
	struct mldgroupx *p = (struct mldgroupx*)data;
	if ((p->ifx->last_lisenr_qry_cnt)<=(p->s_masqry_cnt)) {
		delgp(&(p->ma), p->ifx);
		return;
	}
	Smldqry(p->ifx->ifidx, &(p->ma), p->ifx->last_lisenr_qry_itvl);
	(p->s_masqry_cnt)++;
	set_timer(&(p->tm_masqry), p->ifx->last_lisenr_qry_itvl);
}


void htime_gquery(void *data) {
	struct ifrx *pifrx=(struct ifrx*)data;

	if (!(pifrx->isquerier))
		return;

	if ((pifrx->isstartupq) && (pifrx->start_qry_cnt)<=(pifrx->s_qry_cnt)) {
		pifrx->isstartupq = 0;
	}

	Smldqry(pifrx->ifidx, 0, 0);
	set_timer(&(pifrx->tm_qry), (pifrx->isstartupq)?(pifrx->start_qry_itvl):(pifrx->qry_itvl));
}


void htime_nonquerier(void *data) {
	querier((struct ifrx*)data, 0, 0);
}

