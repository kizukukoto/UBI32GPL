#ifdef CONFIG_UBICOM_ARCH
#include <sys/types.h>
#include <sys/wait.h>
#include <nvramcmd.h>

#include "ddns_append_fqdn.h"
/* 
 * Many scripts in openswan can not run on None-MMU, because of bash can not 
 * support in None-MMU. so we need to manpulate whack manually.
 * 
 * samples: 
 * whack --name x509 --ipv4 --host 172.21.33.199 --ikeport 500 \
 * --client 192.168.99.0/24 --srcip 192.168.99.1 \
 *  --id "C=TW, ST=Taiwan, O=CAMEO, OU=BU1, CN=dir655, E=dir655@cameo.com.tw" \
 *  --cert /etc/ipsec.d/certs/dir655.pem \
 *  --to --host 172.21.33.8 --ikeport 500 --client 192.168.0.0/24 \
 *  --id "C=TW, ST=Taiwan, O=CAMEO, OU=BU1, CN=mandrake, E=mandrake@cameo.com.tw" \
 *  --cert /etc/ipsec.d/certs/mandrake.pem --rsasig --encrypt --pfs --debug-all
 *
 *  whack --name x509 --initiate
 */
static char *argv[48];
static int argc;


/****************************************************
 *		Helper
 ****************************************************/
static void dump_argv()
{
	char **args;

	for (args = argv; *args != NULL; args++)
		printf("### %s\n", *args);
}

static int argv_find(const char *k)
{
	int c;

	for (c = 0; c < argc; c++) {
		if (strcmp(argv[c], k) != 0)
			continue;
		return c;
	}
	return -1;
}

static int argv_shift(int start)
{
	int e;

	for (e = argc + 1; e >= start; e--)
		argv[e] = argv[e -1];
	argc++;
	return 0;
}

/*
 * insert new @arg into @argv
 * eg: if @argv = {"whack", "--a1", "--a2", "a3", NULL}
 * 	call argv_insert("--a2", "new_arg");
 * 	then @argv = {"whack", "--a1", "new_arg", "--a2", "a3", NULL}
 * return: if no @k found in @argv -1 returned.
 * 	On sucess, return index of new element
 */
static int argv_insert(const char *k, const char *arg)
{
	int c;

	if ((c = argv_find(k)) == -1)
		return -1;
	argv_shift(c);
	argv[c] = arg;
	return c;
}

static int argv_append(const char *k, const char *arg)
{
	int c, e;

	if ((c = argv_find(k)) == -1)
		return -1;
	c++;
	argv_shift(c);
	argv[c] = arg;
	return c;
}

static char ARGVBUF[24][128];
static char ARGVBUF_INDEX;
static int argv_append_tail_buffer(const char *arg)
{
	strcpy(ARGVBUF[ARGVBUF_INDEX], arg);
	argv[argc++] = ARGVBUF[ARGVBUF_INDEX];
	ARGVBUF_INDEX++;
	return 0;
}


static FILE *AUTHFD;		/* used in multi_section */
static void conf_whack_reinit_multi_section(
	struct vpn_rule *vr,
	FILE *fd,
	const char *name)
{
	char *rs, *ls, *hr, *hl, *r, *l, _name[128];
	int idx = 1;
	struct vpn_conn *vc = &vr->conn;
	
	hr = rs = strdup(vc->rightsubnet);
	hl = ls = strdup(vc->leftsubnet);

	while ((r = strsep(&rs, ",")) != NULL) {
		while ((l = strsep(&ls, ",")) != NULL) {

			sprintf(_name, "%s_%d", name, idx);
			vr->name = _name;
			vc->rightsubnet = r;
			vc->leftsubnet = l;
			conf_whack_rule_conn(vr, fd, AUTHFD);
			if (config_whack_negotiate(vr) == -1)
				return;
			idx++;
		}
	}
	free(hr);
	free(hl);
}

/*****************************************************************/

static int conf_whack_conn(struct vpn_rule *vr, FILE *fd, const char *name)
{
	struct vpn_conn *vc;
	static char buf[128];	/* FIXME: bad idea, use static buffer for global args */

	vc = &vr->conn;
	argv[argc++] = "--name";
	sprintf(buf, "conn_%s", name);
	argv[argc++] = buf;

	/* whack need not specify "--tunnel", "--tunnelipv4", "--tunnelipv6" ...*/
	argv[argc++] = "--ipv4";
	//argv[argc++] = "--tunnel";
	argv[argc++] = "--ikeport"; argv[argc++] = "500";

	/* config left(our) */
	argv[argc++] = "--host"; argv[argc++] = vc->leftip;
	if (strstr(vc->type, "transport") != NULL) {
		argv[argc++] = "--clientprotoport"; argv[argc++] = "17/1701";
	} else {
		argv[argc++] = "--client"; argv[argc++] = vc->leftsubnet;
	}
	//argv[argc++] = "--srcip"; argv[argc++] = ?;
	
	/* config right(remote peer) */
	argv[argc++] = "--to";
	argv[argc++] = "--host"; argv[argc++] = vc->rightip;

	if (strcmp(vc->rightip, "%any") == 0) {
		static char dev[8], dev_gw[16];

		argv_insert("--to", "--nexthop");
		getGatewayInterfaceByIP(vr->conn.rightip, dev, dev_gw);

		/* FIXME: if remote-user NAT-T is in the same subnet
		 * it must use '%direc't for '--nexthop'...
		 */
		//argv_insert("--to", dev_gw);
		argv_insert("--to", "%direct");
		if (nat_traversal()) {
			argv[argc++] = "--client"; argv[argc++] = "vhost:%no,%priv";
		}
	} else {
		//if (nat_traversal()) {
		//	argv[argc++] = "--id"; argv[argc++] = "%any";
		//}
		argv[argc++] = "--client"; argv[argc++] = vc->rightsubnet;
		ddns_append_fqdn(vr->conn.rightip, NS_PATH);
	}
	if (strstr(vc->type, "transport") != NULL) {
		argv[argc++] = "--clientprotoport"; argv[argc++] = "17/%any";
	}
	
	argv[argc++] = "--ike"; argv[argc++] = vr->ph1.ike;
	argv[argc++] = "--esp"; argv[argc++] = vr->ph2.esp;
	/*
 	 * NOTE: Transport Mode only used in L2TP over IPSec.
 	 * L2TP over IPSec disallow PFS from Windows by default.
	 */
	if (strcmp(vr->ph2.pfs, "yes") == 0) {
		argv[argc++] = "--pfs";
	}
	
	if (strcmp(vr->ph1.aggrmode, "yes") == 0) {
		argv[argc++] = "--aggrmode";
	}
	return 0;
}
/*
 * copy from "int conf_vpn_auth_x509(struct vpn_rule *vr, FILE *fd)"
 * */
static int conf_whack_auth_x509(struct vpn_rule *vr, FILE *fd)
{
	struct vpn_auth *ah = &vr->auth;
	char buf[256], *pki, *cert, *cacert;

	pki = ah->extra.x509.pki;
	cert = ah->extra.x509.cert;
	cacert = ah->extra.x509.ca;

	argv[argc++] = "--rsasig";
	fprintf(stderr, "X509:PKI=%s|CERT=%s|\n", pki, cert);
	if (pki) {
		sprintf(buf, "%s/%s/%s", CA_ROOT_DIR, CERTS, pki);
		fprintf(fd, "\tleftcert=%s\n", buf);
		argv_insert("--to", "--cert");
		strcpy(ARGVBUF[ARGVBUF_INDEX], buf);
		argv_insert("--to", ARGVBUF[ARGVBUF_INDEX]);
		ARGVBUF_INDEX++;
		if (cert_subject(pki, buf) == 0) {
			fprintf(fd, "\tleftid=\"%s\"\n", buf);
			DEBUG("\tleftid=\"%s\"\n", buf);
			argv_insert("--to", "--id");
			strcpy(ARGVBUF[ARGVBUF_INDEX], buf);
			argv_insert("--to", ARGVBUF[ARGVBUF_INDEX]);
			ARGVBUF_INDEX++;
		}
	}
	if (cert) {
		fprintf(stderr, "# %s:%d# X509:PKI=%s|CERT=%s|\n", __FUNCTION__, __LINE__, pki, cert);
		sprintf(buf, "%s/%s/%s", CA_ROOT_DIR, CERTS, cert);
		fprintf(fd, "\trightcert=%s\n", buf);
		strcpy(ARGVBUF[ARGVBUF_INDEX], buf);
		argv_append("--to", ARGVBUF[ARGVBUF_INDEX]);
		ARGVBUF_INDEX++;
		
		argv_append("--to", "--cert");
		
		if (cert_subject(cert, buf) == 0) {
			fprintf(fd, "\trightid=\"%s\"\n", buf);
			DEBUG("\trightid=\"%s\"\n", buf);
			strcpy(ARGVBUF[ARGVBUF_INDEX], buf);
			argv_append("--to", ARGVBUF[ARGVBUF_INDEX]);
			ARGVBUF_INDEX++;
			
			argv_append("--to", "--id");
		}
	} else
		fputs("\trightrsasigkey=%cert\n", fd);
	create_ca_dir();
	ca_create();
	dump_argv();
	return 0;
}

static int conf_whack_auth_psk(struct vpn_rule *vr, FILE *fd)
{
	argv[argc++] = "--psk";
	return 0;
}
static int conf_whack_auth(struct vpn_rule *vr, FILE *fd)
{

	struct vpn_auth *ah = &vr->auth;
	int rev = -1;

	switch (ah->type) {
	case PSK:
		argv[argc++] = "--psk";
		rev = 0;
		break;
	case X509:
		conf_whack_auth_x509(vr, stderr);
		break;
#if 0
	case XAUTH: 	/* obsolete */
		conf_vpn_auth_xauth(vr, fd);
		break;
#endif
	default:
		fprintf(fd, "UNKNOW AUTH\n");
	}
	if (ah->mcfg.xauthsrv != NULL)  {
		argv_insert("--to", "--xauthserver");
		argv_append("--to", "--xauthclient");
	}
	if (ah->mcfg.xauthcli != NULL) {
		argv_append("--to", "--xauthserver");
		argv_insert("--to", "--xauthclient");
	}
	return rev;
}

static int conf_if_whack_extra_id(struct vpn_rule *vr, char *s, char **e)
{
	/*
 	 * FIXME: support string type only,
 	 * 	FQDN, ASN1, need support.
 	 * */
	char *idtype[] = {
		"#string",
		"#ip",
		"#FQDN",
		NULL,
	}, **p, *id;

	printf("%s:%d:%s\n", __FUNCTION__, __LINE__, s);
	for (p = idtype; *p != NULL; p++) {
		if (strcmp(*p, s) != 0)
			continue;
		if ((id = strsep(e, ",")) == NULL)
			break;	/* FIXME: error on ~ */
		if (strncmp(id, "leftid=", 7) == 0) {
			printf("%s:%d:%s\n", __FUNCTION__, __LINE__, id);
			argv_insert("--to", "--id");
			argv_insert("--to", vr->conn.leftid);
			
		} else if (strncmp(id, "rightid=", 8) == 0) {
			printf("%s:%d:%s\n", __FUNCTION__, __LINE__, id);
			argv_append("--to", vr->conn.rightid);
			argv_append("--to", "--id");
		}
	}  

	return 0;
}
static void conf_whack_extraparm(struct vpn_rule *vr, FILE *fd)
{
	char *p, *e, *m;
	/* 
	 * parser/init extraparms to whack from buffer @vr->extraparm 
	 * eg: @e as ...
	 * "pfsgroup=modp1024,ikelifetime=28800s,keylife=3600s,"
	 * "#default,#leftid=,#default,#rightid=,dpdaction=hold,dpdtimeout=120s"
	 * 
	 * in new style:(except DIR130/330), id will declare they type.
	 * eg: "vpn_extra1=pfsgroup=modp768,ikelifetime=28800s,keylife=3600s," \
	 * 	"#string,leftid=dir652,#string,rightid=dir130,dpdaction=hold,dpdtimeout=120s"
 	 */
	e = strdup(vr->extraparm);
	m = e;
	fprintf(stderr, "e:%s\n", e);
	//fprintf(stderr, "lid, :%s, rid:%s\n", vr->conn.leftid, vr->conn.rightid);
	while ((p = strsep(&e, ",")) != NULL) {
	//	fprintf(stderr, "XXX p:%s\n", p);
		if (conf_if_whack_extra_id(vr, p, &e))
			continue;
		/* 
		 * skip those keys, they are configured somewhere
		 * (per-node left and right) 
 		 */
		if (!(*p == '#' ||
			strncmp(p, "leftid=", strlen("leftid=")) == 0 || 
			strncmp(p, "rightid=", strlen("rightid=")) == 0 ||
			strncmp(p, "rightprotoport=", strlen("rightprotoport=")) == 0 ||
			strncmp(p, "leftprotoport=", strlen("leftprotoport=")) == 0)
		)
		{
			char *k, arg[128];
			struct {
				char *orig;
				char *arg;
				int sec_convert; /* format time eg: "30s" as "30" */
			} t[] = {
				{"ikelifetime", "--ikelifetime", 1},
				{"keylife", "--ipseclifetime", 1},
				{"dpdtimeout", "--dpdtimeout", 1},
				{NULL, NULL, 0}
			}, *s;
			k = strsep(&p, "=");

			for (s = t; s->orig != NULL; s++) {
				if (strcmp(s->orig, k) != 0)
					continue;
				k = s->arg;
				if (s->sec_convert)
					p[strlen(p) - 1] = '\0';
				break;
			}
			/* if prefix is not "--", do it */
			if (!(k[0] == '-' && k[1] == '-'))
				sprintf(arg, "--%s", k);
			else
				sprintf(arg, "%s", k);

			/* add extra parm and value to @argv */
			argv_append_tail_buffer(arg);
			argv_append_tail_buffer(p);
			/* in openswan whack 2.6.24, Must have 3th args if DPD enable */
			if (strcmp("--dpdtimeout", arg) == 0) {
				argv[argc++] = "--dpddelay";
				argv[argc++] = "30";
			}
		}
	}
	free(m);
}
int conf_whack_rule_conn(struct vpn_rule *vr, FILE *fd, FILE *auth)
{
	/* reset */
	bzero(argv, sizeof(argv));
	bzero(ARGVBUF, sizeof(ARGVBUF));
	ARGVBUF_INDEX = 0;
	AUTHFD = auth; /* used in multi_section */
	argv[0] = "whack";
	argc = 1;
	/* inital pluto once only */
	conf_whack_start_pluto();
	if (multi_section(&vr->conn) == 1) {
		write_secret(vr, AUTHFD);
		fflush(AUTHFD);
		conf_whack_reinit_multi_section(vr, fd, vr->name);
		fprintf(stderr, "%s: whack support multiple session\n", __FUNCTION__);
		return 1;
	}
	if (conf_whack_conn(vr, fd, vr->name) == -1)
		return -1;
	//conf_vpn_ph1(vr, fd);
	//conf_vpn_ph2(vr, fd);
	conf_whack_extraparm(vr, fd);
	conf_whack_auth(vr, fd);
	//conf_vpn_extraparm(vr, fd);
	return 0;
}

extern char *info_getwan_interface();
int conf_whack_start_pluto()
{
	static int initiated = 0;
	char cmds[512], *nat_args="--nat_traversal --virtual_private "
			"%v4:192.168.0.0/16,%v4:10.0.0.0/8,%v4:172.16.0.0/12";
			//"%v4:192.168.0.0/24,%v4:0.0.0.0/1,%v4:128.0.0/1,%all,%v4:192.168.52.0/24";
	if (initiated == 1)
		return 0;
	initiated++;

	if (access("/var/run/pluto/", F_OK) != 0)
		mkdir("/var/run/pluto", 0755);
	sprintf(cmds, "pluto --nofork --ikeport 500 "
		"--secretsfile /tmp/ipsec.secrets "
		"--ctlbase /var/run/pluto/pluto "
		"--interface %s %s %s  --stderrlog &",
		info_getwan_interface(),			/* eg: "eth0.1" or "ppp0" */
		nvram_safe_get("pluto_extraparm"),		/* eg: "--debug-all" */
		nat_traversal() ? nat_args : "");
	printf(cmds);
	system(cmds);

	/* wait pluto execurted and initicated */
	sleep(2);
	return 0;
}

int config_whack_negotiate(struct vpn_rule *vr)
{
	int st, pid;
	char cmds[128], *act, *extra_args;

	argv[argc++] = "--encrypt";

	/* extram args if we want to debug or test newer options in
 	 * whack for profile , eg: "--debug-all"
 	 * */
	if (strlen((extra_args = nvram_safe_get("whack_extraparm"))) != 0)
		argv[argc++] = extra_args;

	dump_argv();
	system("whack --listen");
	switch ((pid = vfork())){
	case -1:
		perror("vfork() to exec whack ");
		return -1;
	case 0:
		/*
		 *  Befor config IPSec profile. Make sure pluto is running.
 		 */
		execvp("whack", argv);
		perror("execvp(whack) ");
		exit(-1);
	}
	if (waitpid(pid, &st, 0) < 0)
		goto syserr;
	if (!WIFEXITED(st))
		goto syserr;
	if (WEXITSTATUS(st) != 0)
		goto syserr;


	/* action */
	if (strcmp(vr->action, "route") == 0)
		act = "--route";
	else if (strcmp(vr->action, "start") == 0)
		act = "--initiate";
	else if (strcmp(vr->action, "add") == 0)
		act = "--route";	/* for remote any? */
	else {
		fprintf(stderr, "%s %d: FIXME: whack -- Unknow action: %s\n",
			__FUNCTION__, __LINE__, vr->action);
		return -1;
	}
		
	/* XXX: need reread? */
	sprintf(cmds, "whack --name conn_%s %s &", vr->name, act);
	printf("%s", cmds);
	system(cmds);
	system("whack  --rereadsecrets");

	return 0;
syserr:
	perror("waitpid or whack return an error ");
	return -1;
}

int config_whack_stop_pluto()
{
	system("killall pluto");
	system("killall _pluto_adns");
	unlink(NS_PATH);
	return 0;
}
#endif //CONFIG_UBICOM_ARCH
