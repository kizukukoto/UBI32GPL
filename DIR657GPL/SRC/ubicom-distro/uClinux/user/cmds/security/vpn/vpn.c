#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "ddns_append_fqdn.h"
#include "shutils.h"
#include "vpn.h"
#include "nvram.h"
#include "libcameo.h"
#define cprintf(fmt, args...) do { \
        FILE *fp = fopen("/dev/console", "w"); \
        if (fp) { \
                fprintf(fp, fmt, ## args); \
                fclose(fp); \
        } \
} while (0)
//#define DEBUG(fmt, a...) fprintf(stderr, fmt, ##a)
#define DEBUG(fmt, a...) do { } while(0)

/*
 * Functions to crate, append, execute script for IPSec
 * */

#ifdef CONFIG_UBICOM_ARCH
#warning "*********************************************************"
#warning "We should not define CONFIG_UBICOM_ARCH for compatibility"
#warning "FIXME: plsase delete this dirty code"
#warning "*********************************************************"
#endif
static inline u_int32_t get_v4netaddr(u_int32_t addr, u_int32_t mask)
{
	return addr & mask;
}


static char *a2ip(const char *s, char *out)
{
	return strcpy(out, s);
}
static char *a2fqdn(const char *s, char *out)
{
	return a2ip(s, out);
}
static char *a2asn(const char *s, char *out)
{
	sprintf(out, "\"%s\"", s);
	return out;
}
static char *a2string(const char *s, char *out)
{
	strcpy(out, "@");
	strcat(out, s);
	return out;
}

static void extraparm_id_alg(struct vpn_rule *vr, char *leftid, char *rightid)
{
	char *e = strdup(vr->extraparm);
	char *st[24];
	int i, idx;
	struct {
		char *type;
		char *(*convert)(const char *, char *);
	} *p, idt[] = {
		{ "#default", NULL },
		{ "#ip", a2ip},
		{ "#FQDN", a2fqdn},
		{ "#ANS", a2asn},
		{ "#string", a2string},
		{ NULL, NULL},
	};
#define RID "rightid="
#define LID "leftid="	
	*leftid = '\0';
	*rightid = '\0';
	idx = split(e, ",", st, 1);
	for (i = 0; i < idx; i ++) {
		for (p = idt; p->type != NULL; p++) {
			char *b;

			if (strncmp(st[i], p->type, strlen(p->type)) != 0)
				continue;
			if (i == (idx - 1)) //last, over, UI bug
				continue;
			b = st[i+1];
			if (strncmp(b, LID, strlen(LID)) == 0) {
				if (!(strlen(b) >= strlen(LID) &&
					p->convert != NULL))
					continue;
				p->convert(b + strlen(LID), leftid);
			}
			if (strncmp(b, RID, strlen(RID)) == 0)  {
				if (!(strlen(b) >= strlen(RID) &&
					p->convert != NULL))
					continue;
				p->convert(b + strlen(RID), rightid);
			}
		}
	}
#undef RID
#undef LID
	free(e);
}

static int vpn_conn_format(struct vpn_rule *vr, char *buf)
{
	char *type, *lip, *rip, *lnet, *rnet;

	fprintf(stderr, "[%s] buf:[%s]\n", __FUNCTION__, buf);
	
	type = strsep(&buf, ",");
	lip = strsep(&buf, "-");
	rip = strsep(&buf, ",");
	lnet = strsep(&buf, "-");
	rnet = buf;

	if (!(type && lip && rip && lnet && rnet))
		return -1;
	vr->conn.type = type;
	vr->conn.leftip = lip;
	vr->conn.leftsubnet = lnet;
	vr->conn.rightip = rip;
	vr->conn.rightsubnet = rnet;
	extraparm_id_alg(vr, vr->conn.leftid, vr->conn.rightid);
	fprintf(stderr, "[%s] conn:%s to %s, %s to %s\n", __FUNCTION__, lip, rip, lnet, rnet);
	cprintf(stderr, "[%s] conn:%s to %s, %s to %s\n", __FUNCTION__, lip, rip, lnet, rnet);
	return 0;
}

static void dump_vpn_auth(struct vpn_rule *vr)
{
	struct vpn_auth *ah = &vr->auth;
	
	switch (ah->type) {
	case PSK:
		printf("auth: [%s]\n", vr->auth.extra.psk.psk);
		break;
	case X509:
		printf("x509: cert[%s]\n", vr->auth.extra.x509.cert);
		printf("x509: ca[%s]\n", vr->auth.extra.x509.ca);
		printf("x509: pki[%s]\n", vr->auth.extra.x509.pki);
		break;
	default:
		printf("Unsupport authentication type!\n");
	}
	
}
static void dump_vpn_rule(struct vpn_rule *vr)
{
	printf("Conn: %s, action %s, Type: %s, IP: %s to %s\nNet: %s to %s\n", 
		vr->name, vr->action, vr->conn.type, vr->conn.leftip, vr->conn.rightip,
		vr->conn.leftsubnet, vr->conn.rightsubnet);
	printf("IKE: %s, aggress mode = %s\n", vr->ph1.ike, vr->ph1.aggrmode);
	printf("ESP: %s\n", vr->ph2.esp);
	printf("extra: %s\n", vr->extraparm);
	dump_vpn_auth(vr);
}

static int vpn_ph1_format(struct vpn_rule *vr, char *buf)
{
	char *p;
	
	p = strsep(&buf, ",");
	
	if (!(p && buf))
		return -1;
	
	vr->ph1.aggrmode = p;
	vr->ph1.ike = buf;
	//printf("phase1:%s, %s\n", p, buf);
	return 0;
}

static int vpn_ph2_format(struct vpn_rule *vr, char *buf)
{
	char *p;

	p = strsep(&buf, ",");
	if (!(p && buf))
		return -1;
	vr->ph2.pfs = p;
	vr->ph2.esp = buf;
	return 0;
}

static int conf_vpn_ph1(struct vpn_rule *vr, FILE *fd)
{
	struct vpn_phase1 *ph1;

	ph1 = &vr->ph1;

	if (strcmp(ph1->aggrmode, "yes") == 0)
		fputs("\taggrmode=yes\n", fd);
	fprintf(fd, "\tike=\"%s\"\n", ph1->ike);
	return 0;
}

static int conf_vpn_ph2(struct vpn_rule *vr, FILE *fd)
{
	struct vpn_phase2 *ph2;

	ph2 = &vr->ph2;
	fprintf(fd, "\tpfs=%s\n", ph2->pfs);
#ifdef CONFIG_BCM_IPSEC //openswan 1.0.10
	fprintf(fd, "\tesp=\"%s\"\n", ph2->esp);
#else
	fprintf(fd, "\tphase2alg=\"%s\"\n", ph2->esp);
#endif
	return 0;
}

static int vpn_auth_format_psk(struct vpn_rule *vr, char *buf)
{
	vr->auth.extra.psk.psk = buf;
	return 0;
}
#include "x509.h"
static int vpn_auth_format_xauth(struct vpn_rule *vr, char *buf)
{
	vr->auth.extra.xauth.serverkey = buf;
	return 0;
}

static int vpn_auth_format_ext(struct vpn_rule *vr, char *buf)
{
	struct {
		char *key;
		char **vv;
	}*p, mcfg[] = {
		{ "XAUTHSRV", &vr->auth.mcfg.xauthsrv},
		{ "XAUTHCLI", &vr->auth.mcfg.xauthcli},
		{ NULL, NULL},
	};
       	char *v, *k;
	DEBUG("========%s(FOO, %s)=============\n", __FUNCTION__, buf);
	v = buf;
	k = strsep(&v, ":");
	if (v == NULL || *v == '\0')
		goto out;
	for (p = mcfg; p->key != NULL; p++) {
		*(p->vv) = (strncmp(p->key, k, strlen(p->key)) == 0)? v : NULL;
	}
out:
	return 0;
}

static int vpn_auth_format(struct vpn_rule *vr, char *buf)
{
	struct {
		char *key;
		int flag;
		int (*fn)(struct vpn_rule *, char *);	
	} auth_type[] = {
		{ "PSK", PSK, vpn_auth_format_psk},
		{ "X509", X509, vpn_auth_format_x509},
		//{ "XAUTH", XAUTH, vpn_auth_format_xauth},
		{ NULL, 0}
	}, *a;
	struct vpn_auth *ah;
	char *mcfg = buf;
	DEBUG("VPN_AUTH_FORMAT\n");
	ah = &vr->auth;
	/* @mcfg: "#XAUTHSRV:dbname"
	 * @mcfg: "#XAUTHCLI:u1,p1"
	 */
	if (strstr(mcfg, "#XAUTHCLI") ||strstr(mcfg, "#XAUTHSRV"))
		strsep(&mcfg, "#");
	else
		mcfg = NULL;
	/* 
	 * Parser: <KEY>:<VALUE> format of authentication
	 * ex: "PSK:123456" for PreShareKey with secret 123456
	 * ex: "X509:pki=x509_1,cert=x509_2"
	 * ex: "XAUTH:peter,123,usr,234,#,#"
	 * ex: "PSK:123456#XAUTHSRV:db1" for PreShareKey with secret 123456 and XAUTH server
	 * ex: "X509:pki=x509_1,cert=x509_2#XAUTHCLI:user,passwd"
	 * */
	for (a = auth_type; a->key != NULL; a++) {
		if (strncmp(buf, a->key, strlen(a->key)) == 0) {
			//printf("AUTH STR:%s\n", buf);
			ah->type = a->flag;
			a->fn(vr, buf + strlen(a->key) + 1); /* 1 = ':'*/
			break;
		}
	}
	if (a->key == NULL) {
		ah->type = PSK;
		vpn_auth_format_psk(vr, buf);
	}
	DEBUG("MODE CFG__\n");
	if (mcfg == NULL || *(mcfg) == '\0')
		goto out;
	/* @mcfg: "#XAUTHSRV:dbname"
	 * @mcfg: "#XAUTHCLI:u1,p1"
	 * */
	vpn_auth_format_ext(vr, mcfg);
out:
	return 0;
}

static int vpn_format(struct vpn_rule *vr, char *buf)
{
	char *action, *name, *conn, *ph1, *ph2, *auth, *extra;
	
	fprintf(stderr, "[%s:%d] buf:[%s]\n", __FUNCTION__, __LINE__, buf);
	/* fetch each of section by token ";"  from @buf */
	action = strsep(&buf, ";");
	name = strsep(&buf, ";");
	conn = strsep(&buf, ";");
	ph1 = strsep(&buf, ";");
//#ifdef CONFIG_BCM_IPSEC
	fprintf(stderr, "[%s:%d] buf:[%s]\n", __FUNCTION__, __LINE__, buf);
#if 1	//test...
	ph2 = strsep(&buf, ";");
#else // openswan 2.6.X
	ph2 = buf;
	buf = strchr(buf, ';');
	buf ++;
	printf("%s\n", buf);
	strsep(&buf, ";");
#endif
	auth = strsep(&buf, ";");
	extra = strsep(&buf, ";");

	if (!(action && conn && ph1 && ph2 && auth && extra))
		return -1;

	vr->extraparm = extra;
	vr->action = action;
	vr->name = name;
	vpn_conn_format(vr, conn);
	vpn_ph1_format(vr, ph1);
	vpn_ph2_format(vr, ph2);
	vpn_auth_format(vr, auth);
	//dump_vpn_rule(vr);
	return 0;
}

static inline int in_netmask(u_int32_t ifip, u_int32_t ifmask, u_int32_t destip)
{
	/*
	 * We ignored this kind of error which transport mode with
	 * multi-leftsubnet or multi-rightsubnet.
	 * UI should handle (block) it.
	 * */
	return (ifip & ifmask) == (destip & ifmask) ? 1 : 0;
}

#define UNSET_LEFTSUBNET	"255.255.255.255/32"
/* config HUB & Spoke style */
static void conf_vpn_conn_subsection(struct vpn_rule *vr, FILE *fd, const char *name)
{
	char *l = NULL, *r = NULL, *rsub;
	int idx = 1;
	struct vpn_conn *vc = &vr->conn;
	
	rsub = malloc(strlen(vc->rightsubnet) + 1);
	
	do  {
		if ((l = strrchr(vc->leftsubnet, ',')) == NULL)
			l = vc->leftsubnet;
		else
			*l++ = '\0';
		r = NULL;
		strcpy(rsub, vc->rightsubnet);
		do {
			if ((r = strrchr(rsub, ',')) == NULL)
				r = rsub;
			else
				*r++ = '\0';
			fprintf(fd,
"conn conn_%s_%d\n"
"\tauto=%s\n"
"\tleftsubnet=%s\n"
"\t%srightsubnet=%s\n"
"\talso=conn_%s_base\n\n",
			name, idx, vr->action, l,
			strncmp(r, UNSET_LEFTSUBNET, strlen(UNSET_LEFTSUBNET)) == 0 ? "#":"",
			r, name);
			idx++;
		} while (rsub != r);
	
	} while (vc->leftsubnet != l);
	free(rsub);
}

static void conf_vpn_extraparm(struct vpn_rule *vr, FILE *fd)
{
	
	char *p, *e, *m;

	e = strdup(vr->extraparm);
	m = e;
	while ((p = strsep(&e, ",")) != NULL) {
		if (!(*p == '#' ||
		    strncmp(p, "leftid=", strlen("leftid=")) == 0 || 
		    strncmp(p, "rightid=", strlen("rightid=")) == 0))
		{
			if ((strncmp(p, "dpdaction", strlen("dpdaction")) == 0 ||
			     strncmp(p, "dpdtimeout", strlen("dpdtimeout")) == 0) &&
			     vr->auth.mcfg.xauthcli != NULL)
				continue;
			fprintf(fd, "\t%s\n", p);
		}
	}
	free(m);
	//fputs("\tdpdaction=clear\n", fd);
	//fputs("\tdpdtimeout=60\n", fd);
}

static inline int nat_traversal()
{
	return strcmp(nvram_safe_get("vpn_nat_traversal"), "1") == 0 ? 1: 0;
}

static inline void masq_alg(struct vpn_rule *vr, FILE *fd)
{
#ifdef CONFIG_BCM_IPSEC
	char cmd[128];
	char *ifn = NULL;
	
	//DEBUG("========LEFTSUBNET:[%s]\n", vr->conn.leftsubnet);
	if (!(strstr(vr->conn.leftsubnet, "0.0.0.0/0") != NULL ||
	     (strstr(vr->conn.leftsubnet, "0.0.0.0/1") != NULL &&
	      strstr(vr->conn.leftsubnet, "128.0.0.0/1") != NULL)))
	{
		return;
	}
	ifn = nvram_safe_get("wan0_ifname");
	sprintf(cmd, "iptables -t nat -D POSTROUTING -o %s -j MASQUERADE", ifn);
	system(cmd);
	sprintf(cmd, "iptables -t nat -A POSTROUTING -o %s -j MASQUERADE", ifn);
	system(cmd);
#endif
	return;
}

static int conf_vpn_conn(struct vpn_rule *vr, FILE *fd, const char *name)
{
	struct vpn_conn *vc;

	vc = &vr->conn;
	
	masq_alg(vr, fd);

	if (multi_section(vc) == 1) {
		/* parser subsection for each of leftsubnets & rightsubnets */
		conf_vpn_conn_subsection(vr, fd, name);
		/* declare base section for reference by above sections */
		fprintf(fd, "conn conn_%s_base\n", name);
	} else {
		fprintf(fd, "conn conn_%s\n", name);
		/* XXX: BAD BAD BAD! */
		fprintf(fd, "\tauto=%s\n", vr->action);
		//fprintf(fd, "\tauto=%s\n", strcmp(vc->rightip, "%any") == 0 ?
		//		"add":"start");
		/* XXX: BAD BAD BAD! DIR730...*/
		//eval("iptables", "-I", "FORWARD", "-d", vc->leftsubnet, "-j", "ACCEPT");
	       
		if (strstr(vc->type, "tunnel") != NULL) {
			if (strncmp(vc->leftsubnet, UNSET_LEFTSUBNET,
				strlen(UNSET_LEFTSUBNET)) != 0)
			{
				fprintf(fd, "\tleftsubnet=%s\n", vc->leftsubnet);
			}
			
			if (nat_traversal() && strcmp(vc->rightip, "%any") == 0
			&& strcmp(vc->rightsubnet, "255.255.255.255/32") == 0)
				fputs("\trightsubnet=vhost:%no,%priv,%all\n", fd);
			else if (strncmp(vc->rightsubnet, UNSET_LEFTSUBNET,
						strlen(UNSET_LEFTSUBNET)) != 0)
				fprintf(fd, "\trightsubnet=%s\n", vc->rightsubnet);
		}
	}

	fprintf(fd, "\ttype=%s\n\tleft=%s\n\tright=%s\n",
			vc->type, vc->leftip, vc->rightip);
	ddns_append_fqdn(vc->rightip, NS_PATH);
	if (vc->leftid[0] != '\0')
		fprintf(fd, "\tleftid=%s\n", vc->leftid);
	if (vc->rightid[0] != '\0')
		fprintf(fd, "\trightid=%s\n", vc->rightid);
#ifdef CONFIG_BCM_IPSEC
	{
		struct in_addr addr, mask;

		inet_aton(nvram_safe_get("wan0_ipaddr"), &addr);
		inet_aton(nvram_safe_get("wan0_netmask"), &mask);

		char dev[8], dev_gw[16];

		bzero(dev, sizeof(dev));
		bzero(dev_gw, sizeof(dev_gw));

		/* if vc->rightip = "any" and still find gateway use getGatewayInterfaceByIP(..),
	 	 * here has segmentation fault (fredhung, 2008/9/24) */
		if (strstr(vc->rightip, "any"))
			return 0;

		getGatewayInterfaceByIP(vr->conn.rightip, dev, dev_gw);

		if ((strstr(vc->rightip, "any")) ||
			(in_netmask(addr.s_addr, mask.s_addr, inet_addr(vc->rightip)) != 1))
		{
				fprintf(fd, "\t%sleftnexthop=%s\n",
				(strcmp(dev_gw, "0.0.0.0") == 0 || strlen(dev_gw) == 0)? "#" : "",
				dev_gw);
		}
	}
#endif
	return 0;
}

static int conf_vpn_auth_xauth(struct vpn_rule *vr, FILE *fd)
{
	fputs("\txauth=yes\n", fd);
	fputs("\tauthby=secret\n", fd);
	return 0;
}

static int conf_vpn_auth(struct vpn_rule *vr, FILE *fd)
{
	struct vpn_auth *ah = &vr->auth;

	DEBUG("XAUTH support\n");
	switch (ah->type) {
	case PSK:
		fputs("\tauthby=secret\n", fd);
		break;
	case X509:
		conf_vpn_auth_x509(vr, fd);
		break;
#if 0
	case XAUTH: 	/* obsolete */
		conf_vpn_auth_xauth(vr, fd);
		break;
#endif
	default:
		fprintf(fd, "UNKNOW AUTH\n");
	}
	if (ah->mcfg.xauthsrv != NULL)
		fputs("\tleftxauthserver=yes\n", fd);
	if (ah->mcfg.xauthcli != NULL)
		fputs("\tleftxauthclient=yes\n", fd);
	return 0;
}

static int conf_vpn_other(FILE *fd)
{
	fputs("\trekeymargin=1799s\n", fd);
	fputs("\trekeyfuzz=0%\n", fd);

	return 0;
}

static int conf_vpn_rule(struct vpn_rule *vr, FILE *fd)
{
	conf_vpn_conn(vr, fd, vr->name);
	conf_vpn_ph1(vr, fd);
	conf_vpn_ph2(vr, fd);
	conf_vpn_auth(vr, fd);
	conf_vpn_extraparm(vr, fd);
	conf_vpn_other(fd);
	fputs("\n", fd);
	return 0;
}

static int write_secret_x509(struct vpn_rule *vr, FILE *fd)
{
	char *pki = vr->auth.extra.x509.pki;
	char key[] = "x509_XX";
	
	/* @pki = "x509_XX"; XX=0-99 */
	sprintf(key, "x509_%d", x509_index(pki));
#ifdef CONFIG_BCM_IPSEC	/* FIXME: openswan 1.99 */
	fprintf(fd, ": RSA %s %%prompt\n", key);
#else
	fprintf(fd, ": RSA /etc/ipsec.d/private/%s\n", key);
#endif
	return 0;
}
static int write_secret_psk(struct vpn_rule *vr, FILE *fd)
{
	char *rightip = vr->conn.rightip;

#ifdef CONFIG_UBICOM_ARCH
	/* for openswann 2.4.x: the ipsec.secrets file for 
 	 *   RaodWarrior format like this:
 	 *   1.2.3.4  : PSK "psk_secrets"
 	 * for openswan 1.9x (bcm)
 	 *   1.2.3.4 %any : PSK "psk_secrets"
 	 */
	if (strcmp(rightip, "%any") == 0)
		rightip = "";
#endif //CONFIG_UBICOM_ARCH

	fprintf(fd, "%s %s : PSK \"%s\"\n",
		strlen(vr->conn.leftid) == 0 ? vr->conn.leftip : vr->conn.leftid,
		strlen(vr->conn.rightid) == 0 ? rightip : vr->conn.rightid,
		vr->auth.extra.psk.psk);

	return 0;
}

static int write_secret_xauth(struct vpn_rule *vr, FILE *fd)
{
	struct vpn_conn *vc;
	char *p, *psk;

	vc = &vr->conn;
	/* We only support xauth server now! */
	fputs("#XAUTH:", fd);
	fputs(vr->auth.extra.xauth.serverkey, fd);
	fputc('\n', fd);
	DEBUG("%s\n", vr->auth.extra.xauth.serverkey);
	if ((p = strchr(vr->auth.extra.xauth.serverkey, ',')) == NULL) {
		DEBUG("PSK + XAUTH NO PASSWROD FOUND\n");
		return 0;
	}
	p++;
	psk = strsep(&p, ",");
	fprintf(fd, "%s %s : PSK \"%s\"\n", vc->leftip,
		vc->rightip, psk);
	return 0;
}

static int write_secret_xauthsrv(struct vpn_rule *vr, FILE *fd)
{
	char *h, *e;

	DEBUG("%s(%s)\n", __FUNCTION__, vr->auth.mcfg.xauthsrv);

	/* old style from DIR-330/130 v1.10 */
	if (strchr(vr->auth.mcfg.xauthsrv, ',') != NULL) {
		/* XXX: Bad idea for DIR-130 USA without USER/PASSWD DB. */
		fprintf(fd, "#XAUTH:conn_%s/%s/\n", vr->name, vr->auth.mcfg.xauthsrv);
		return 0;
	}
	
	/* XXX: how about Hub spoke ? 
	 * ex: mcfg.xauthsrv="auth_group1"
	 * */
	h = strdup(nvram_safe_get(vr->auth.mcfg.xauthsrv));
	if ((e = strchr(h, '/')) == NULL)
		return -1;
	e++;
	fprintf(fd, "#XAUTH:conn_%s/%s\n", vr->name ,*e == '\0' ? NULL : e);
	free(h);
	return 0;
}

static int write_secret_xauthcli(struct vpn_rule *vr, FILE *fd)
{
	char buf[256];
	
	system("[ -d /tmp/ipsec.d ] || mkdir /tmp/ipsec.d");
	sprintf(buf, "echo \"%s\" > /tmp/ipsec.d/conn_%s.xauthcli",
		vr->auth.mcfg.xauthcli, vr->name);
	DEBUG(buf);
	system(buf);
	return 0;
	
}

static int write_secret(struct vpn_rule *vr, FILE *fd)
{
	int type = vr->auth.type;

	switch (type) {
#if 0
	case XAUTH:
		write_secret_xauth(vr, fd);
		break;
#endif
	case PSK:
		write_secret_psk(vr, fd);
		break;
	case X509:
		write_secret_x509(vr, fd);
		break;
	}
	if (vr->auth.mcfg.xauthsrv != NULL) {
		DEBUG("WRITE SECRETE XAUTHSRV\n");
		write_secret_xauthsrv(vr, fd);
	}
	if (vr->auth.mcfg.xauthcli != NULL) {
		DEBUG("WRITE SECRETE XAUTHCLI\n");
		write_secret_xauthcli(vr, fd);
	}
	return 0;
}

static inline int vpn_enable(struct vpn_rule *vr)
{
	return (strcmp(vr->action, "start") == 0 ||
		strcmp(vr->action, "route") == 0 ||
		strcmp(vr->action, "add") == 0) ? 1 : 0;
}

#ifdef CONFIG_UBICOM_ARCH
#include "ipsec_whack.h"
#endif
/*
 * (Open/Free)swan configuration file generator.
 * INPUT:
 * 	@conf: config string format.
 * 	@cfg: Opened stream for output the config (usually /etc/ipsec.conf)
 * 	@auth: Opened stream for ouput the secrets for PSK.
 * */
int conf_vpn_connection(const char *conf, FILE *cfg, FILE *auth)
{
	struct vpn_rule vr;
	int rev = -1;
	char *p;

	if ((p = strdup(conf)) == NULL)
		goto out2;

	bzero(&vr, sizeof(struct vpn_rule));
	if (vpn_format(&vr, p) == -1)
		goto out;

	if (vpn_enable(&vr) != 1)
		goto out;
#ifdef CONFIG_UBICOM_ARCH
	if (conf_whack_rule_conn(&vr, cfg, auth) != 0) {
		/* in multi-section, they config by themslve,
		 * and return 1, then we just do the next porfile.
		 */
		rev = 0;
		goto out;
	}
#else
	if (conf_vpn_rule(&vr, cfg) == -1)
		goto out;
#endif
	if (write_secret(&vr, auth) == -1)
		goto out;
#ifdef CONFIG_UBICOM_ARCH
	fflush(auth);
	if (config_whack_negotiate(&vr) == -1)
		goto out;
#endif
	rev = 0;
out:
	free(p);
out2:
	return rev;
}

/*
 * Those APIs is for an ancient freeswan to openswan in 
 * DIR130/330 v1.0 upgrade to 1.1.
 * We will not use them in other projects
 * */
#include "ipsec_convert.h"

#if 0
#define IPSEC_CMD "/usr/local/sbin/ipsec"
int main_vpn_test(int argc, char *argv[])
{
	char cmd[128];
	int rev = -1;
	FILE *fd;

	sprintf(cmd, "%s auto --up %s", IPSEC_CMD, "conn_test");
	if ((fd = popen(cmd)) == NULL)
		goto out;
	
out:
	return rev;
}
#endif
#if 0
/*
 * Sample
 * 
 * 2008/08/15:
 * vpn_conn1="route;remoete;tunnel,1.1.1.1-172.21.34.6,192.168.0.0/24-192.168.3.0/24;no,3des-md5-modp1024,aes-md5-modp1024,aes128-md5-modp1024,aes192-md5-modp1024;yes,3des-md5,aes-md5,aes128-md5,aes192-md5;PSK:123456"
 * vpn_extra2="pfsgroup=modp1024,ikelifetime=28800s,keylife=3600s,#default,#leftid=,#default,#rightid=,dpdaction=clear,dpdtimeout=120s"
 * */
int main(int argc, char *argv[])
{
	//FILE *fd;
	//struct vpn_rule vr;
	char p[] =
	/* start|add|ignore|route;name;tunnel|transport; */
	"add;Name1;tunnel,"
	/* left-right,leftsubnet[,leftsubnet...]-rightsubnet[,rightsubnet...]; */
	//"192.168.0.1-192.168.0.2,192.168.1.0/24,1.2.3.0/24-10.0.0.0/24,2.2.2.0/16;"
	"192.168.0.1-192.168.0.2,192.168.1.0/24-10.0.0.0/24;"
	/* PHASE1: aggressive[yes|no],ike_proposal,ike_proposal,ike_proposal,ike_proposal;*/
	"yes,3des-md5-96,3des-sha1-96,aes-md5-96,aes-sha1-96;"
	/* PHASE2: pfs[yes|no],ipsec_proposal,ipsec_proposal,ipsec_proposal,ipsec_proposal;*/
	"yes,3des-md5-96,3des-sha1-96,aes-md5-96,aes-sha1-96;"
	/* PSK secert; */
	"1234;";
	//"X509:pki=x509_1,cert=x509_2;"
	//"X509:pki=x509_1;"
	/* #extra data */
	"pdpaction=%hold,pdpdely=180,dpdtimeout=19600,rekey=yes,keylife=8h";
	init_db(NULL);
	create_ca_dir();
	ca_create();
	return conf_vpn_connection(p, stdout, stdout);

}
#endif 
