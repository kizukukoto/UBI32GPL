#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "libdb.h"
#include "ssi.h"

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

static inline int have_sched(const char *sched)
{
	return (strncasecmp(sched, "alway", 5) != 0) ? 1 : 0;
}
static inline int have_rule(const char *rule)
{
	return (strlen(rule) > 0) ? 1 : 0;
}

void schedule_inused(const char *key)
{

	char buf[128], *p;

	if (query_record(key, buf, sizeof(buf)) != 0)
		return;
	if (have_rule(buf) != 1)
		return;
	
	if ((p = strrchr(buf, ',')) == NULL)
		p = buf;
	else
		p++;

	if (have_sched(p) == 0)
		return;
	fprintf(stdout, "%s=%s,", key, p);
}

int echo_schedules_inused(int argc, char *argv[])
{

	static struct {
		char *name;
		char *prefix;
		int min;
		int max;
	} *p, schd[] = {
		{ "Virtual server", "forward_portXX", 0, 23},
		{ "Port forwarding", "forward_portXX", 24, 47},
		{ "Application", "app_portXX", 0, 24},
		{ "URL filter", "url_domain_filter_schedule_XX", 0, 39},
		{ "DMZ", "dmz_schedule", -1 , -1},
		{ "Remote Admin", "remote_schedule", -1, -1},
		{ NULL, NULL}
	};

	
	char buf[128];
	int i;
	
	if (nvram_init(NULL) < 0)
		return -1;
	
	for (p = schd; p->name != NULL; p++) {
		if (p->min == -1 && p->max == -1) {
			schedule_inused(p->prefix);
		} else {
			for (i = p->min; i <= p->max; i++) {
				strcpy(buf, p->prefix);
				sprintf(buf + strlen(buf) - 2, "%d", i);
				
				schedule_inused(buf);
			}
		}
	}
	
	return 0;
}
/*
 * split string @buf by deimitor, such as split utility
 * Input:
 * 	@buf:	string
 * 	@strip:	not zero for strip repeated delimitor
 * Output:
 * 	@tok:	Stored each of results from strsep()
 * Retrun:
 * 	Retrun length of @tok
 * 
 * FIXME:
 * 	buf = "1bbb2bb3", strip = 0
 * 	return [1],[],[],2,[],[3]:
 * 		which output each slice of delimitor - 1 while paser with @stip = 0
 * 		it seems due to the behavior of @strsep()
 * */
int split(char *buf, const char *delim, char *tok[], int strip)
{
	int i = 0;
	char *p;
	
	while ((p = strsep(&buf, delim)) != NULL) {
		if (strip && (strchr(delim, *p) != NULL)) {
			continue;
		}
		tok[i++] = p;
	}
	if (buf)
		tok[i++] = buf;
	return i;
}
#define CA_MAX	25
#define VPN_MAX 26
#define X509_AUTH	"X509"
#define DEBUG(fmt, a...)	do{} while(0)
int ca_inused_main(int argc, char *argv[])
{
	
	char b[512], *buf = b;
	char vpn_name[CA_MAX][64];
	int i;
	
	if (nvram_init(NULL) < 0)
		return -1;
	for (i = 0; i < CA_MAX; i++)
		vpn_name[i][0] = '\0';

	/* parser each nvram to tail of field: "xxxx;X509:pki=x509_2,cert=x509_3" */
	for (i = 1; i <= VPN_MAX; i++) {
		char *p, *pki, *cert, *name;
		char vpn[] = "vpn_connXX";
		int idx;

		sprintf(vpn, "vpn_conn%d", i);
		bzero(b, sizeof(b));
		buf = b;
		if (query_record(vpn, buf, sizeof(b)) != 0) continue;
		if (strlen(buf) == 0)
			continue;
		
		if (strsep(&buf, ";") == NULL ||
		   (name = strsep(&buf, ";")) == NULL)
			continue;
		DEBUG("[Name: %s]\n", name);
		if ((p = strrchr(buf, ';')) == NULL) continue;
		
		p++; /* p = ";pki=x509_2,cert=x509_3" */
		DEBUG("[p:%s]\n", p);
		if (strncmp(p, X509_AUTH, strlen(X509_AUTH)) != 0)
			continue;
		
		if ((p = strchr(p, ':')) == NULL) continue;
		p++;
		/* split "pki=x509_2,cert=x509_3" by ',' */
		pki = strsep(&p, ",");
		if (pki == NULL || p == NULL) continue;
		DEBUG("pki:[%s], cert:[%s]\n", pki, p);
		/* fetch @pki = "x509_2", @cert = "x509_3" */
		pki = strchr(pki, '=');
		cert = strchr(p, '=');
		if (pki == NULL || cert == NULL) continue;

		pki++; cert++;
		idx = atoi(pki + strlen("x509_"));
		if (idx >= CA_MAX)
			continue;
		DEBUG("add:%d\n", idx);
		strncpy(vpn_name[idx - 1], name, 63);

		idx = atoi(cert + strlen("x509_"));
		if (idx >= CA_MAX)
			continue;
		DEBUG("add:%d\n", idx);
		strncpy(vpn_name[idx - 1], name, 63);
	}
	
	for (i = 0; i < CA_MAX; i++) {
		fprintf(stdout, "\"%s\",", vpn_name[i]);
	}
	fprintf(stdout, "\"\"");
	return 0;
}
#define GROUP_MAX	6
#define XAUTHSRV	"XAUTHSRV"
int group_inused_main(int argc, char *argv[])
{
	
	char b[512], *buf = b;
	char vpn_name[GROUP_MAX][64];
	int i, j;
	
	if (nvram_init(NULL) < 0)
		return -1;
	for (i = 0; i < GROUP_MAX; i++)
		vpn_name[i][0] = '\0';

	/* parser each nvram to tail of field: ";PSK:aaa#XAUTHSRV:auth_group1" */
	for (i = 1; i <= VPN_MAX; i++) {
		char *p, *group, *name;
		char vpn[] = "vpn_connXX";
		int idx;

		sprintf(vpn, "vpn_conn%d", i);
		bzero(b, sizeof(b));
		buf = b;
		if (query_record(vpn, buf, sizeof(b)) != 0) continue;
		if (strlen(buf) == 0)
			continue;
		
		if (strsep(&buf, ";") == NULL ||
		   (name = strsep(&buf, ";")) == NULL)
			continue;
		DEBUG("[Name: %s]\n", name);
		if ((p = strrchr(buf, ';')) == NULL) continue;
		
		p++; /* p = "PSK:aaa#XAUTHSRV:auth_group1" */
		DEBUG("[p:%s]\n", p);

		if ((p = strchr(p, '#')) == NULL) continue;
		p++;

		if (strncmp(p, XAUTHSRV, strlen(XAUTHSRV)) != 0)
			continue;

		if ((group = strchr(p, ':')) == NULL) continue;
		group++; /* group = auth_group1 */
	 	DEBUG("[group:%s]\n", group);
		
		idx = atoi(group + strlen("auth_group"));
		if (idx >= GROUP_MAX)
			continue;
		DEBUG("add:%d\n", idx);
		strncpy(vpn_name[idx - 1], name, 63);
	}

	query_record("auth_pptpd_l2tpd", buf, sizeof(b));
	j = atoi(buf) - 1;
	
	for (i = 0; i < GROUP_MAX; i++) {
		if (i == j){
			fprintf(stdout, "\"%s\",", "in_used");
		}else{
			fprintf(stdout, "\"%s\",", vpn_name[i]);
		}
	}
	fprintf(stdout, "\"\"");
	return 0;
}

int vpn_main(int argc, char *argv[])
{
	FILE *fd;
	char buf[256], tmp[256], *p;
	int rev = -1;

	if (argc < 2)
		goto out;
	
	if (strcmp(argv[1], "eroute") == 0) {
		sprintf(buf, "/usr/local/sbin/ipsec %s 2>&1", argv[1]);
		fd = popen(buf, "r");
	/* 
	 * Parser: output of format of "ipsec eroute"
	 * "0	192.168.0.0/24		->	192.168.1.0/24	=> tun0x12345678@172.21.17.68"
	 * Output:
	 * "tunnel/0,IPSec,192.168.0.0/24,192.168.1.0/24"
	 * */
		while (fgets(tmp, sizeof(tmp), fd)) {
			if ((p = strstr(tmp, "=>")) != NULL) {
				char *toks[24];
				char *mode;
				if (strstr(p, "%trap"))
					continue;
				if (strstr(p, "%hold"))
					continue;
				if (strstr(p, "%pass"))
					continue;
				split(tmp, " \t@", toks, 1);
				if (strstr(toks[5], "tun") != NULL)
					mode = "tunnel";
				else
					mode = "transport";

				printf("%s/%s,IPSec,%s,%s#", mode, toks[0], toks[1], toks[3]);
			} 
		}
		pclose(fd);
	}
	/* openvpn.key.*.st was generated from openvpn.sh */
	if ((fd =  popen("cat /tmp/openvpn.key.*.st 2>/dev/null", "r")) == NULL)
		goto out;
	while (fgets(buf, sizeof(buf), fd)) {
		buf[strlen(buf) - 1] = '\0';
		printf("tunnel/0,SSL,%s#", buf);
	}
	pclose(fd);
	rev = 0;
out:
	return rev;
}

static int vpn_openvpn_exist()
{
	struct dirent *ptr = NULL;
	DIR *dir = opendir("/tmp");

	cprintf("REMOTE_ADDR: %s\n", getenv("REMOTE_ADDR"));

	while ((ptr = readdir(dir)) != NULL) {
		if (ptr->d_name[0] == '.')
			continue;

		if (strstr(ptr->d_name, ".st")) {
			FILE *fp = fopen(ptr->d_name, "r");
			char hostnet[24], remote_addr[24];

			if (fp == NULL)
				continue;

			bzero(hostnet, sizeof(hostnet));
			bzero(remote_addr, sizeof(remote_addr));

			fscanf(fp, "%[^,],%s", hostnet, remote_addr);
			cprintf("%s: %s, %s\n", ptr->d_name, hostnet, remote_addr);

			if (strcmp(getenv("REMOTE_ADDR"), remote_addr) == 0) {
				fclose(fp);
				return 1;
			}

			fclose(fp);
		}
	}

	closedir(dir);
	return 0;
}

static int vpn_ipsec_exist()
{
	FILE *fd = NULL;
	char cmd[] = "/usr/local/sbin/ipsec eroute";
	int n_packet;
	char ip1[24], ip2[24], gt[8], gt2[8], status[128];

	if ((fd = popen(cmd, "r")) == NULL)
		goto out;

	bzero(ip1, sizeof(ip1));
	bzero(ip2, sizeof(ip2));
	bzero(gt, sizeof(gt));
	bzero(gt2, sizeof(gt2));
	bzero(status, sizeof(status));

	while(fscanf(fd, "%d %s %s %s %s %s",
		&n_packet, ip1, gt, ip2, gt2, status) != EOF) {
		cprintf("n_packet: %d, ip1: %s, ip2: %s, status: %s, addr: %s\n",
			n_packet, ip1, ip2, status, getenv("REMOTE_ADDR"));
		if (strstr(ip2, getenv("REMOTE_ADDR")) && strstr(status, "tun0x")) {
			pclose(fd);
			return 1;
		}
	}

	pclose(fd);
out:
	return 0;
}

static int vpn_exist_main(int argc, char *argv[])
{
	if (vpn_openvpn_exist() == 1 || vpn_ipsec_exist() == 1) {
		fputs("alert(\"relogin!\");\nreturn;", stdout);
	}

	return 1;
}

int strip_main(int argc, char *argv[])
{
	int rev = -1, i;
	char cmd[512] = "", *p;
	FILE *fd;
	
	if (argc < 3) 
		goto out;
	//if (access(argv[2], X_OK) == -1)
	//	goto out;
	
	for (i = 2; i < argc; i++) {
		strcat(cmd, " ");
		strcat(cmd, argv[i]);
	}
	
	if ((fd = popen(cmd, "r")) == NULL)
		goto out;
	p = cmd;

	while (fgets(p, sizeof(cmd), fd) != NULL) {
		chop(p);
		strcat(p, argv[1]);
		fputs(p, stdout);
	}
	
	rev = 0;
out:
	return rev;
}

int get_time(int argc, char *argv[])
{
	FILE *fd;
	char time[512] = "", *p;
	
	if ((fd = popen("/bin/date \"+%b %d, %Y %H:%M:%S\"", "r")) == NULL)
		return -1;

	p = time;
	
	while (fgets(p, sizeof(time), fd) != NULL) {
		chop(p);
		fputs(p, stdout);
	}
	pclose(fd);
	return 0;
}

/* The parameter string 'username' is a contents of nvram key 'current_user'.
 * This key was set by httpd, it means current login user. But the contents
 * of this key is base64 encoded. So, the compared target should be base64
 * encoded too. In this function, 'username' will compared with 'usr'. But
 * variable 'usr' is not encoding string. We translate 'usr' to 'usr_b64',
 * the variable 'usr_b64' is encoded 'usr'.
 *
 * usr -> (base64 encode) -> usr_b64
 *
 * */
extern int base64_encode(char *OrgString, char *Base64String, int OrgStringLen);
static int __username_in_auth_group(const char *group, const char *username)
{
	char buf[2048], usr_b64[128], *usr, *pass, *p;

	/* format of value of @group = "Groupname/user1,pass1,user2,pass2 " */
	if (query_record(group, buf, sizeof(buf)) != 0)
		return 0;
	if (strlen(buf) == 0)
		return 0;
	p = buf;
	strsep(&p, "/");
	do {
		if ((usr = strsep(&p, ",")) == NULL)
			break;
		if ((pass = strsep(&p, "/")) == NULL)
			break;
		base64_encode(usr, usr_b64, strlen(usr));
		if (strcmp(usr_b64, username) == 0)
			return 1;
	} while (p != NULL);
	return 0;
}

/*
 * @group : "auth_group1:auth_group2:auth_group6"
 * @name  : "peter"
 * Return : 1 as success.
 * */
static int username_in_auth_groups(const char *groups, const char *username)
{
	char *p, *g, *m;
	int rev = -1;
	if (strchr(groups, ':') == NULL){
		rev = __username_in_auth_group(groups, username);
		goto out;
	}

	/*
	 * @group support multi-group
	 * ex: "auth_group1:auth_group3:auth_group4"
	 * */
	if ((g = strdup(groups)) == NULL)
		goto out;

	m = g;
	while (g != NULL && *g != '\0' && 
		(p = strsep(&g, ":")) != NULL && *p != '\0')
	{
		if ((rev = __username_in_auth_group(p, username)) == 1)
			goto out; //got it!
	}
	free(m);
out:
	return rev;
}

static void gen_random(unsigned char *out, size_t len)
{
	unsigned long r, i;

	srand(time((time_t *)NULL));
	for (i = 0; i < len; i++) {
		r = rand();
		out[i] = (unsigned char)r;
	}
	return;
}

static void key_gen(int c1, int c2, unsigned char *out, size_t len)
{
	int i;
	for (i = 0; i < len; i++) {
		out[i] %= (c2 - c1);
		out[i] += c1;
	}
}

#define cprintf(fmt, args...) do { \
	FILE *fp = fopen("/dev/console", "w"); \
	if (fp) { \
				fprintf(fp, fmt, ## args); \
				fclose(fp); \
			} \
} while (0)

void filetype(int fd)
{
	struct stat stus;
	if (fstat(fd, &stus) == -1) {
		cprintf("Unable stat of %d:%s\n", fd, strerror(errno));
		return;
	}
	cprintf("FILE TYPE OF (%d):%o IS ", fd, 077770000);
	if (S_ISFIFO(stus.st_mode))
		cprintf("FIFO");
	else if (S_ISCHR(stus.st_mode))
		cprintf("CHAR");
	else if (S_ISDIR(stus.st_mode))
		cprintf("DIR");
	else if (S_ISREG(stus.st_mode))
		cprintf("REG");
	else if (S_ISBLK(stus.st_mode))
		cprintf("BLK");
	else if (S_ISLNK(stus.st_mode))
		cprintf("LNK");
	else if (S_ISSOCK(stus.st_mode))
		cprintf("SOCK");
	else
		cprintf("UNKNOW?");
	cprintf("\n");
}

static void sslvpn_exec()
{
	pid_t pid;
	int rev, st;
#ifdef DIR652V
	if ((pid = vfork()) == -1) {
#else
	if ((pid = fork()) == -1) {
#endif
		cprintf("FORK Failure\n");
		return;
	}
	if (pid == 0) {
		int m;

		setsid();
		for (m = 0;m < 256; m++){
			close(m);
		}
		execlp("sslvpn", "sslvpn", NULL);
		cprintf("EXEC sslvpn failure\n");
		exit(-1);
	}
	if ((rev = waitpid(pid, &st, 0)) == -1)
		cprintf("NO PID return:%s\n", strerror(errno));
	else if (rev == 0)
		cprintf("NO CHILD return:%s\n", strerror(errno));
	else {
		if (WIFEXITED(st))
			cprintf("Normoal Exit(%d)\n", WEXITSTATUS(st));
		else
			cprintf("Exit on abnormal!\n");
	}
}
/*
 * Add, Delete a record of ip address.
 * key "vpn_ip" for record each of byte which 
 * either '0' for unused or '1' on inused. 
 * 
 * */
int vpn_ip_main(int argc, char *argv[])
{
#define SSLVPN_MAX_TUNNEL	254
	int i, rev = -1;
	char b[SSLVPN_MAX_TUNNEL + 1], *k = "vpn_ip";

	if (argc < 2)
		goto help;
	
	if (nvram_init(NULL) < 0)
		goto out;

	if (query_record(k, b, sizeof(b)) != 0) {
		bzero(b, sizeof(b));
		system("nvram set vpn_ip=\"\"");
	}
	
	if (strcmp(argv[1], "new") == 0) {
		for (i = 0; i < SSLVPN_MAX_TUNNEL; i++) {
			if (!(b[i] == '0' || b[i] == '\0'))
				continue;
			b[i] = '1';
			printf("10.1.%d.1\n",i + 1);
			break;
		}
	} else if (strcmp(argv[1], "del") == 0 && argc == 3) {
		unsigned char a[4];
		printf("new delete:%s, %d\n", argv[2], i = atoi(argv[2]));
		
		sscanf(argv[2], "%d.%d.%d.%d", (int *)&a[0], (int *)&a[1], (int *)&a[2], (int *)&a[3]);
		i = a[2];
		if (b[i - 1] != '1')
			goto out;
		b[i - 1] = '0';
	} else if (strcmp(argv[1], "dump") == 0) {
		for (i = 0; i < SSLVPN_MAX_TUNNEL && b[i] != '\0'; i++) {
			if (b[i] == '1')
				printf("10.1.%d.1\n", i + 1);
		}
	} else {
		goto help;
	}
	update_record(k, b);
	rev = 0;
	goto out;
help:
	fprintf(stderr,
		"vpn_ip [new | del | dump]\n"
		"vpn_ip new #For assign a new ip address\n"
		"vpn_ip del ipaddr # To delete a assinged ip address\n"
		"vpn_ip dump # Dump ip inused\n");
out:
	return rev;
#undef SSLVPN_MAX_TUNNEL
}

static int gen_p2p_ip(char *ip1, char *ip2)
{
	int rev = -1;
	FILE *fp;
	int sip[4];
	char b[128], *cmd = "vpn_ip new";
	
	if ((fp = popen(cmd, "r")) == NULL)
		goto out;
	while (fgets(b, sizeof(b), fp) != NULL) {
		strcpy(ip1, b);
		ip1[strlen(ip1) - 1] = '\0';
	}
	pclose(fp);
	sscanf(ip1, "%d.%d.%d.%d", &sip[0], &sip[1], &sip[2], &sip[3]);
	sprintf(ip2, "10.1.%d.2", sip[2]);
	//strcpy(ip2, "10.1.0.2");
	rev = 0;
out:
	return rev;
}
#ifdef DIR652V
/* ipv6 @addr := [::ffff:172.21.33.239]
 * ipv4 @addr := 172.21.33.239
 * */
static void sslvpn_addr_formatting(const char *addr, char *buf)
{
	char *lp;

	if (strstr(addr, ":") == NULL)	/* it's means @addr is ipv4 */
		return;

	lp = strrchr(addr, ':');
	strcpy(buf, lp + 1);
	buf[strlen(buf) - 1] = '\0';	/* replace ']' to '\0' */
}
#endif

/*
 * Generate SSLVPN and push to stdout then execute sslvpn
 * @masq: "0" or "1" to setup SNAT when tunnel established.
 * */
void sslvpn_init(
	FILE *out,
	const char *user,
	const char *subnet,
	const char *masq)
{
	unsigned char key[5], *host;
	char *ovpn_cmd, buf[1024], b1[128];
	char ovpn_key[] = "/tmp/openvpn.key.XXXXXX";
	FILE *fp;
	unsigned char ip1[16], ip2[16], r;
#ifdef DIR652V
	char wanip[128], format_addr[128];
	char *rm_addr = getenv("REMOTE_ADDR");
	extern int get_ip(const char *, char *);
#endif

	/* For openvpn VPN */
	ovpn_cmd = buf;
	mktemp(ovpn_key);
	sprintf(ovpn_cmd, "openvpn --genkey --secret %s", ovpn_key);
	system(ovpn_cmd);
	if ((fp = fopen(ovpn_key, "r")) == NULL)
		return;

	gen_p2p_ip(ip1, ip2);
	setenv("SSLKEY", ovpn_key, 1);
	setenv("SSLIFCONFIG", ip1, 1);
	setenv("SSLIFCONFIG_PEER", ip2, 1);
	sscanf(ip1, "%d.%d.%d.%d", (int *)&b1[0], (int *)&b1[1], (int *)&r, (int *)&b1[3]);
	sprintf(b1, "%d", r + 1000);
	setenv("SSLPORT", b1, 1);
	setenv("SSLNETMASK", "255.255.255.0", 1);
	/* For IPSec VPN */
	bzero(key, sizeof(key));
	// "REMOTE_ADDR", "REMOTE_USER", had be set already.
	gen_random(key, sizeof(key) - 1);
	key_gen('0', '9', key, sizeof(key) - 1);
#ifdef DIR652V
	if (strcmp(nvram_safe_get("wan_proto"), "static") == 0)
		strcpy(wanip, nvram_safe_get("wan_static_ipaddr"));
	else if (strcmp(nvram_safe_get("wan_proto"), "dhcpc") == 0)
		get_ip("eth0.1", wanip);
	else if (strcmp(nvram_safe_get("wan_proto"), "pppoe") == 0 ||
		 strcmp(nvram_safe_get("wan_proto"), "pptp") == 0 ||
		 strcmp(nvram_safe_get("wan_proto"), "l2tp") == 0)
		get_ip("ppp0", wanip);
	else
		cprintf("XXX unknown wan type: '%s'\n", nvram_safe_get("wan_proto"));

	host = wanip;
	sslvpn_addr_formatting(rm_addr, format_addr);
	cprintf("XXX %s(%d) rm_addr: '%s' format_addr: '%s'\n",
		__FUNCTION__, __LINE__, rm_addr, format_addr);
	setenv("REMOTE_ADDR", format_addr, 1);
#else
	host = nvram_safe_get("wan0_ipaddr");
#endif
	setenv("PRESHARED", key, 1);
	setenv("HOSTNET", subnet, 1);
	setenv("HOST", host, 1);
	setenv("LPORT", nvram_safe_get("sslport"), 1);

	setenv("MASQ", (masq == NULL || masq == '\0') ? "" : masq, 1);
	/* Pass to SSI/SSC */
	fprintf(out, "SSLVPN=HOST:%s,HOSTNET:%s,PRESHARED:%s,"
			"REMOTE_ADDR:%s,REMOTE_USER:%s,HOSTNET_ADDR:%s,"
			"SSLIFCONFIG:%s+%s,SSLNETMASK:255.255.255.0,LPORT:%d,SSLONLY:%d,\n",
			host, subnet, key, getenv("REMOTE_ADDR"),
			getenv("REMOTE_ADDR"), nvram_safe_get("lan_ipaddr"),
	      		ip1, ip2, //atoi(nvram_safe_get("sslport")),
			atoi(b1), 
			strcmp(nvram_safe_get("sslonly"), "1") ? 0:1);
	fputs("SSLKEY:", out);
	while (fgets(b1, sizeof(b1), fp))
		fputs(b1, out);
	fclose(fp);
	
	fflush(out);
	sslvpn_exec();
	return;
}

static void http_userinfo(const char *item, char *buf)
{
	FILE *fp;
	char info_s[128], info_t[16], info_i[128], info_c[128];

	buf[0] = '\0';
	sprintf(info_s, "/tmp/graph/%s_allow", getenv("REMOTE_ADDR"));

	if ((fp = fopen(info_s, "r")) == NULL)
		goto out;

	while(fscanf(fp, "%s %s %s", info_t, info_i, info_c) != EOF) {
		if (strcmp(item, info_i) == 0) {
			strcpy(buf, info_c);
			break;
		}
	}

out:
	fclose(fp);
}

/*
 * http_login():
 * Pass http login user and sslvpn info to sslvpn daemon and stdout for SSC.
 * o Find sslvpnXX to get subnet
 * o Setup environment
 * o Pass info stdout for SSC to remote
 * o execv sslvpn daemon
 * */
int http_login(int argc, char *argv[])
{
	int i;
	char user[128], sslvpn[24], rule[128], *p, *g, *masq;

	http_userinfo("REMOTE_USER", user);
	cprintf("XXX REMOTE_USER [%s]\n", user);

	//if ((user = getenv("REMOTE_USER")) == NULL)
	if (*user == '\0')
		return -1;
	if (getenv("REMOTE_ADDR") == NULL)
		return -1;
	if (nvram_init(NULL) < 0)
		return -1;
	/* sslvpnX: <enable=1|0>,<name>,<groups>,<subnet>,<masq=1|0> */
	for (i = 1; i <= 6; i++) {
		sprintf(sslvpn, "sslvpn%d", i);
		if (query_record(sslvpn, rule, sizeof(rule)) != 0)
			continue;
		//printf("<%s:%s>", sslvpn, rule);
		if (strlen(rule) == 0)
			continue;
		p = rule;
		if (p[0] != '1') //skip disabled rule
			continue;
		if (strsep(&p, ",") == NULL)
			continue;
		if (strsep(&p, ",") == NULL)
			continue;
		if ((g = strsep(&p, ",")) == NULL)
			continue;
		//printf("<g:%s>", g);
		if (username_in_auth_groups(g, user) != 1)
			continue;
		if ((g = strsep(&p, ",")) == NULL)
			continue;

		if ((masq = strsep(&p, ",")) == NULL)
			continue;
		cprintf("USER:%s, SUBNET:%s\n", user, g);
		sslvpn_init(stdout, user, g, masq);
		break;
	}
	return 0;
}

void main_using(struct subcmd *cmds)
{
	fprintf(stderr, "Valid sub command:\n");
	for (;cmds->action != NULL; cmds++) {
		fprintf(stderr, "\t%s\n", cmds->action);
	}
}
