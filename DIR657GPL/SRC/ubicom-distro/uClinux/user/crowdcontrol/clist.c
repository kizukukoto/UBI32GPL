#include "clist.h"

#define MAXADDRS 100

struct iplist{
	u_int32_t v;
	struct list_head list;
};

static LIST_HEAD(head);
static int ipsize = 0;

#define unless(c) if(!(c))

int 
for_each_ips(int (*fn)(struct iplist *, void *), void *args){
	struct iplist *p = NULL;
	int rev = 0;
	list_for_each_entry(p, &head , list){
		if ((rev = fn(p, args))) {
			list_move(&p->list, &head);
			return rev;
		}
	}
	return rev;
}

/* 0 as not matched */
int 
fn_cmp(struct iplist *p, void *ptr){
	u_int32_t *v;
	v = (u_int32_t *)ptr;
	//memcpy(&v, ptr, sizeof(u_int32_t));
	//printf("p->v: %x == v: %x\n", p->v, v);
	if (p->v == *v){
		return *v;
	}
	return 0;
}

/* 
 * Return -1: as fail, and otherwise value is returned 
 */
u_int32_t
search_ips(u_int32_t v){
	
	if (for_each_ips(fn_cmp, &v) == 0)
		return -1;
	return v;
}

void
add_ips(u_int32_t v){
	struct iplist *p;
	
	if (v == 0xFFFFFFFF || v == 0)
		return;
	if (search_ips(v) != -1) {
		return;
	}
	if( ipsize == MAXADDRS) {
		p = container_of(head.prev ,struct iplist, list);
		list_del(&p->list);
		free(p);
		ipsize--;
	}
	
	unless (p = malloc(sizeof(struct iplist)))
		return;
	p->v = v;
	list_add(&p->list, &head);
	ipsize++;
}
/////////////////////////

int
fn_dump(struct iplist *p, void *ptr) {
	struct in_addr in;
	in.s_addr = p->v;
	printf("%s\n", inet_ntoa(in));
	return 0;
}

int 
search_addr(const char *addr){
	in_addr_t in;
	
	if ((in = inet_addr(addr)) == -1) {
		syslog(LOG_INFO, "search_addr: %s Invalid format", addr);
		return -1;
	}
	if (search_ips(in) != -1) {
		syslog(LOG_INFO, "search_addr: %s found", addr);
		return 1;
	}
	syslog(LOG_INFO, "search_addr: %s no found", addr);
	return 0;
}

void
add_domain2ip(const char *p)
{
	struct hostent *hp;
	if (inet_addr(p) != -1)
		return;
	if (!(hp = gethostbyname(p)))
		return;
	if (hp->h_addrtype != AF_INET)
		return;
	if (hp->h_length != 4)
		return;
	syslog(LOG_INFO, "dom2ip proccess: [%s]", p);
	add_ips(((struct in_addr *)(hp->h_addr))->s_addr);
}
#if 0
int
main(int argc, char *argv[]){
	u_int32_t v;
	/*
	add_ips(1);
	add_ips(2);
	add_ips(3);
	add_ips(3);
	add_ips(5);
	add_ips(4);
	add_ips(4);
	add_ips(1);
	*/
	add_domain2ip("www.pchome.com.tw");
	add_domain2ip("tw.yahoo.com");
	add_domain2ip("www.hinet.net");
	for_each_ips(fn_dump, NULL);
	if (search_addr("202.43.19.52") == 1)
		printf("find 202.43.195.52\n");
	v = 5;
	if (search_ips(v) != -1)
		printf("Find %d\n", v);
	return 0;
}
#endif
