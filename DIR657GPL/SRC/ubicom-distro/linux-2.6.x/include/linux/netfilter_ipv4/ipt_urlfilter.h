#ifndef _IPT_URLFILTER_H
#define _IPT_URLFILTER_H

/* *** PERFORMANCE TWEAK ***
 * Packet size and search string threshold,
 * above which sublinear searches is used. */
#define IPT_STRING_HAYSTACK_THRESH	100
#define IPT_STRING_NEEDLE_THRESH	20

#define BM_MAX_NLEN 256
#define BM_MAX_HLEN 1024

typedef char *(*proc_ipt_search) (char *, char *, int, int);

struct ipt_urlfilter_info {
	char url[BM_MAX_NLEN + 1];
	u_int16_t invert;
	u_int16_t url_len;
	u_int16_t host_len;
	char host[BM_MAX_NLEN + 1];
};

#endif /* _IPT_URLFILTER_H */
