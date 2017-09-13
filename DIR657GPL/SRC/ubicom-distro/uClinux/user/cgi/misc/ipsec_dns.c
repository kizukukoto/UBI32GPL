#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define NAME_SIZE 256
#define IN_ADDR_SIZE 12
struct ns{
	char *name;
	struct in_addr in[IN_ADDR_SIZE];
};
static int debug;
#define dbg(fmt, a...) { if (debug) printf(fmt, ##a);}
static void ns_dump(struct ns *n)
{
	while (n->name != NULL) {
		printf("[%s]\n", n->name);
		n++;
	}
}
static char *chop(char *io)
{
	char *p;

	p = io + strlen(io) - 1;
	while ( *p == '\n' || *p == '\r') {
		*p = '\0';
		p--;
	}
	return io;
}
/*
 * Init structure ns to @n from @n->name.
 * 
 * Return 0 on success, -1 on error or name resolve failure.
 * */
static int ns_init_resolv(struct ns *n)
{
	struct hostent *h;
	int i;
	
	if ((h = gethostbyname(n->name)) == NULL) {
		dbg("can not resolve %s\n", n->name);
		goto out;
	}
	if (h->h_length != 4)
		goto out;	
	if (h->h_aliases == NULL)
		dbg("alias: %s\n", h->h_aliases);
	dbg("%s\n", n->name);
	for (i = 0; h->h_addr_list[i] != NULL && i < IN_ADDR_SIZE; i++) {
		n->in[i] = *(struct in_addr *)h->h_addr_list[i];
		dbg("\t[%s]\n", inet_ntoa(n->in[i]));
	}
	return 0;
out:
	return -1;
}

struct ns* ns_init(FILE *fd)
{
	char n[NAME_SIZE];
	int i = 0;
	struct ns *np, *h = NULL;
	
	/* test array elements should be allocate */
	while (fgets(n, sizeof(n), fd)) {
		if (strlen(n) < 3) /* simply filter */
			continue;
		chop(n);
		if (strlen(n) > NAME_SIZE - 1)
			continue;
		//dbg("[%s]\n", n);
		i++;
	}
	if (fseek(fd, 0L, SEEK_SET) == -1)
		goto out; /* can not seek, from stdio? */
	if ((h = malloc(sizeof(struct ns) * i + 1)) == NULL)
		goto out;
	memset(h, sizeof(struct ns) * i + 1, 0);
	np = h;
	while (fgets(n, sizeof(n), fd)) {
		if (strlen(n) < 3) /* simply filter */
			continue;
		chop(n);
		if (strlen(n) > NAME_SIZE - 1)
			continue;
		np->name = strdup(n);
		/* if error, @np->in will be zero
		 * and update when name resolve is valid.
		 * */
		ns_init_resolv(np); /* init ip to np->in */
		np++;
	}
	np->name = NULL;
	//ns_dump(h);
out:
	return h;
}

static int ns_same_addr(struct ns *n, struct ns *old)
{
	int x, y, same = 0;
	
	dbg("=====check %s\n", n->name);
	for (x = 0; x < IN_ADDR_SIZE && n->in[x].s_addr != 0; x++) {
		same = 0;
		/*
		 * XXX: Actually. we only compare the frist entry of @n
		 * with each entries of @old.
		 * */
		for (y = 0; y < IN_ADDR_SIZE && old->in[y].s_addr != 0; y++) {
			dbg("\t%s ==", inet_ntoa(n->in[x]));
			dbg(" %s", inet_ntoa(old->in[y]));
			
			//same = old->in[y].s_addr == n->in[x].s_addr ? 1 : 0;
			same = n->in[x].s_addr == old->in[y].s_addr ? 1 : 0;
			dbg("\t%s\n", same?"m":"e");
			if (same)
				break;
		}
		return same;
	}
	return 0;
}

/*
 * update structure ns, and call event hanlder
 * */
static int ns_event_update(struct ns *h, struct ns *n)
{
	system("ipsec setup restart");
	memcpy(h->in, n->in, sizeof(struct in_addr) * IN_ADDR_SIZE);
	return 0;
}

int ns_check_and_update(struct ns *h)
{
	struct ns nt;
	int i;
	
	while (h->name != NULL) {
		memset(&nt, sizeof(struct ns), 0);
		nt.name = h->name;
		//nt.name = "www.google.com.tw";
		if (ns_init_resolv(&nt) == -1) {
			h++;
			continue;
		}
		if (!ns_same_addr(&nt, h)) {
			ns_event_update(h, &nt);
			sleep(120);
			return 1; //return if any ip changed;
		}
		h++;
	}
out:
	return 0;
}

static void ipsec_dns_help(char *argv0)
{
	printf("use %s <filename>\n"
		" file format is a list line of doamin eg:\n"
		"www.foo.com\n"
		"ipsec.foo.org\n\n"
		"dump message if environ DEBUG is exist, for example\n"
		"\tDEBUG=1 %s /tmp/ns.res\n", argv0, argv0);
}
#ifdef DEMO
int main(int argc, char *argv[])
#else
int ipsec_dns_main(int argc, char *argv[])
#endif
{
	int rev = -1;
	FILE *fd;
	struct ns *hp;
	char *res = "ns.res";
	int interval = 30;
	if (argc == 1 || *argv[1] == '-') {
		ipsec_dns_help(argv[0]);
		return 0;
	}
	
	if (argc > 1)
		res = argv[1];
	if (getenv("DEBUG"))
		debug = 1;
	if (getenv("IPSEC_DNS_TIMER")) {
		interval = atoi(getenv("IPSEC_DNS_TIMER"));
		printf("interval is:%d\n", interval);
	}
	if ((fd = fopen(res, "r")) == NULL)
		goto out;
	if ((hp = ns_init(fd)) == NULL)
		goto out;
	fclose(fd);
	while (1) {
		sleep(60);
		ns_check_and_update(hp);
	}
out:
	return rev;
}
