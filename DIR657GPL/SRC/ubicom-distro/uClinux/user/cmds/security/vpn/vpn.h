#ifndef __VPN_H
#define __VPN_H

#ifdef CONFIG_BCM_IPSEC
#include "nvram.h"
#endif

struct vpn_conn{
	char *type; 		/* "tunnel" || "transport" */
	char *leftip;
	char *leftsubnet;	/* Ex: "192.168.0.0/24" or "255.255.255.255/32" for ignored that */
	char *rightip;
	char *rightsubnet;
	char leftid[512];
	char rightid[512];
};

struct vpn_phase1{
	char *aggrmode;		/* "yes" or "no"	*/
	char *ike;		/* Value of key "ike"	*/
};

struct vpn_phase2{
	char *pfs;		/* pfs= "yes" or "no"	*/
	char *esp;		/* Value of key "esp"	*/
};

struct vpn_auth{
	char *psk;		/* Value of secret PSK, obsolete */
	enum {
		NONE = 0,
		PSK,
		X509,
		XAUTH,
		AUTH_MAX,
	} type;		/* [PSK|X509|XAUTH] */
	
	union {
		union {
			char *psk;
		} psk;
		union {
			char *serverkey;
			char *clientkey;
		} xauth;	/* obsolete */
		struct {
			char *ca;
			char *cert;
			char *pki;
		} x509;
	} extra;
	struct {
		char *xauthsrv;
		char *xauthcli;
	} mcfg;
};

struct vpn_rule{
	char *action;	/* Key of "auto", Valid values: "start", "add", "ignored", "route" */
	char *name;	/* Rule name */
	struct vpn_conn conn;
	struct vpn_phase1 ph1;
	struct vpn_phase2 ph2;
	struct vpn_auth auth;
	char *extraparm;
	char garbage[128];	/* It is useful to avoid of malloc() and free() */
	char *private;
};

extern int conf_vpn_connection(const char *conf, FILE *cfg, FILE *auth);
extern void init_ipsec_script();
extern void eval_ipsec_script();
static inline int multi_section(struct vpn_conn *vc)
{
	return (strchr(vc->leftsubnet,  ',') != NULL ||
		strchr(vc->rightsubnet, ',') != NULL) ? 1 : 0;
}
#endif //__VPN_H
