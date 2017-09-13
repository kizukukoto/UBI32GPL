#ifndef __PROTO_H
#define __PROTO_H

struct proto_base {
	int index;
	char *alias;
	char *hostname;
	char *addr;
	char *netmask;
	char *gateway;
	char *dns;
	char *mac;
	char *mtu;
	char *status;
};

enum {
	PROTO_STATIC = 1,
	PROTO_DHCPC,
	PROTO_PPPOE,
	PROTO_PPTP,
	PROTO_L2TP,
	PROTO_USB3G,
	PROTO_MAX
};

#define PROTO_STATIC_STR	"static"
#define PROTO_DHCPC_STR		"dhcpc"
#define PROTO_PPPOE_STR		"pppoe"
#define PROTO_PPTP_STR		"pptp"
#define PROTO_L2TP_STR		"l2tp"
#define PROTO_USB3G_STR		"usb3g"


/*
 * ethX 
 * */
struct proto_static {
	struct proto_base base;
};

struct proto_dhcpc {
	struct proto_base base;
	char *pid;
};
/*
 * pppX
 * */

#define PPP_OPTIONS 			"/var/ppp/options"
#define PPP_STUFF struct {			\
	char *mru, *mtu, *idle, *unit, *dial;	\
	char *user, *pass, *servicename;	\
	char *ip, *dns1, *dns2, *mode, *mppe;	\
}
struct proto_ppp {
	struct proto_base base;
	PPP_STUFF;
};
struct proto_pptp {
	struct proto_base base;
	PPP_STUFF;
	char *serverip;
};
struct proto_pppoe {
	struct proto_base base;
	PPP_STUFF;
	char *serverip;
};
struct proto_l2tp {
	struct proto_base base;
	PPP_STUFF;
	char *serverip;
};
struct proto_usb3g {
	struct proto_base base;
	PPP_STUFF;
	char *apn_name;
	char *dial_num;
};
struct proto_struct {
	char *name;
	int (*start)(int, char **);
	int (*stop)(int, char **);
	int (*status)(int, char **);
	int (*showconfig)(int, char **);
};

extern int init_proto_struct(int proto, struct proto_base *pb, int size, int idx);

/* display nic info by ioctl()*/
extern int proto_xxx_status(const char *dev, const char *phy);

extern int init_proto_static(struct proto_base *pb, int size, int idx);
extern int init_proto_dhcpc(struct proto_base *pb, int size, int idx);
extern int init_proto_pppoe(struct proto_base *pb, int size, int idx);
extern int init_proto_pptp(struct proto_base *pb, int size, int idx);
extern int init_proto_l2tp(struct proto_base *pb, int size, int idx);
extern int init_proto_usb3g(struct proto_base *pb, int size, int idx);
extern int proto_showconfig_main(int proto, int argc, char **argv);
extern void dump_proto_struct(int proto, struct proto_base *pb, FILE *fd);
#endif //__PROTO_H
