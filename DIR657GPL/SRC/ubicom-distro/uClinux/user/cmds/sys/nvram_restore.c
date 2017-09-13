#include <stdio.h>
#include <nvramcmd.h>

#define NV_RESET	1
#define NV_NOSAVE	(1 << 1)

struct items_range{
	char *key;
	char *_default;
	unsigned int _flags;
	int start;
	int end;
	struct items_range *next;
};

#include "nvram/dir-730.h"

extern int save2user_fn(struct items_range *ir, void *data);

static void dump_items_range(struct items_range *ir, void *len)
{
	printf("%*s%s=%s\n", *((int *)len), "", ir->key, ir->_default);
	//(*(int *)len) += 2;
}

static void __extrack_items_range_then_callback(
	const struct items_range *ir_in,
	struct items_range *ir_out,
	void (*fn)(struct items_range *, void *),
	void *data)
{
	return;
}

/*
 * walk through all structure items_renge
 * and call @fn for each of entry(node)
 * */
static void walk_items_range(
	struct items_range *ir,
	void (*fn)(struct items_range *, void *),
	void *data)
{
	struct items_range *p;
	for (p = ir; p->key != NULL;p++) {
		if (p->next != NULL) {
			walk_items_range(p->next, fn, data);
		} else
			fn(p, (void *) data);
	}
}
#if 0
int main(int argc, char *argv[])
{
	walk_items_range(nvram, dump_items_range, 0);
}
#endif

static void item_info(struct items_range *ir)
{
	printf("ir->key: %s\n", ir->key);
	printf("ir->_default: %s\n", ir->_default);
	printf("ir->_flags: %d\n", ir->_flags);
	printf("ir->start: %d\n", ir->start);
	printf("ir->end: %d\n", ir->end);

	if (ir->next)
		printf("ir->next: has subtree\n");
	else
		printf("ir->next: no subtree\n");
}

static void restore_items_range(struct items_range *ir, void *unused)
{
	int i;
	char k[256];
	
	if (ir->_flags != NV_RESET)
		return;
	if (ir->_default == NULL)
		return;
	if (ir->start == ir->end) {
		nvram_set(ir->key, ir->_default);
		return;
	}

	for (i = ir->start;i <= ir->end; i++) {
		sprintf(k, "%s%d", ir->key, i);
		nvram_set(k, ir->_default);
	}
	return;
}

#if 0
static int save2user_fn(struct items_range *ir, void *data)
{
	int i;
	char k[256], *p;

	if (ir->_flags & NV_NOSAVE)
		return;
	if (ir->_default == NULL)
		return;
	if (ir->start == ir->end) {

	if((p = nvram_get(ir->key)) != NULL)
		printf("%s=%d-%s\n",ir->key, strlen(p), p);
	else
		printf("%s=%d-%s\n",ir->key, strlen(ir->_default), ir->_default);
		return;
	}

	for (i = ir->start;i <= ir->end; i++) {
		sprintf(k, "%s%d", ir->key, i);
		if ((p = nvram_get(k)) != NULL)
			printf("%s=%d-%s\n", k, strlen(p), p);
		else
			printf("%s=%d-%s\n", k, strlen(ir->_default), ir->_default);
	}
	return;
}
#endif
/* used to DIR730 style, 
 * nvram is built-in program. DIR652 is locates in filesystem 
 * */
void param_save_conf(int argc, char *argv[])
{
	walk_items_range(nvram, (void *)save2user_fn, (void *)0);
	return ;
}


int restore_main(int argc, char *argv[])
{
	walk_items_range(nvram, restore_items_range, 0);
	nvram_commit();
	return 0;
}

int dump_default_nvram(int argc, char *argv[])
{
	int indent = 0;
	walk_items_range(nvram, dump_items_range, &indent);
	return 0;
}

void check_then_update_nvram(struct items_range *ir, void *counter)
{
	int i;
	char k[256];

	if (ir->_flags != NV_RESET)
		return;
	if (ir->_default == NULL)
		return;
	if (ir->start == ir->end) {
		if (nvram_get(ir->key) == NULL) {
			fprintf(stderr, "restore: %s:%s\n", ir->key, ir->_default);
			nvram_set(ir->key, ir->_default);
			(*(int *)counter) ++;
		}
		return;
	}
	for (i = ir->start;i <= ir->end; i++) {
		sprintf(k, "%s%d", ir->key, i);
		if (nvram_get(k) == NULL) {
			fprintf(stderr, "restore: %s:%s\n", k, ir->_default);
			nvram_set(k, ir->_default);
			(*(int *)counter) ++;
		}
	}
	return;
}
static void __init_nvram_static_check()
{
	
}
int init_nvram_main(int argc, char  *argv[])
{
	int commit = 0;
	char *built_time = VALUE_NVRAM_RELEASE_TIME;
	
	if (nvram_init(NULL) != 0)
		goto out;
	/* XXX: this is for developper, delete them when release */
	if (strcmp(OS_BUILD_VER,
		nvram_safe_get("__os_version")) != 0)
	{
		fprintf(stderr, "force restore (%s) != %s\n",
				OS_BUILD_VER, nvram_safe_get("__os_version"));
		nvram_set("os_version", OS_VERSION);
		nvram_set("__os_version", OS_BUILD_VER);
		nvram_set("os_date", __DATE__);
		//goto force_restore;
	}

	/* XXX: model_area cannot be modified, so I reset model_area here */
	nvram_set("model_area", HW_AREA);

	walk_items_range(nvram, check_then_update_nvram, &commit);
	if (!NVRAM_MATCH("restore_defaults", "0")) {
		fprintf(stderr, "restore default\n");
force_restore:
		restore_main(argc, argv);
		commit = 0;
	}

	/* Now Initail some nvram at booting */
	ddns_restore_main(argc, argv);
	net_restore_main();
	walk_items_range(statistics, restore_items_range, &commit);
	
	if (commit != 0)
		nvram_commit();
out:
	return 0;
}
void __show_version()
{
	printf("Build %s %s\n", __DATE__ , __TIME__);
	printf("Firmware:%s HWID:%s\n", OS_BUILD_VER, HW_ID);
	printf("Model:%s %s-%s\n", nvram_get("model_number"),
		nvram_get("model_name"), nvram_get("model_area"));
}
#if 0
struct nvram_tuple router_defaults[] = {
/* system settings */
/* zone sutuff */
	{ "zone_alias0", "LAN", 1},	/* Zone name */
	{ "zone_if_list0", "LAN", 1},	/* a list of if_aliasX */
	{ "zone_if_attached0", "", 1},/* a already initized list of if_aliasX */

	{ "zone_alias1", "DMZ", 1},
	{ "zone_if_list1", "DMZ", 1},
	{ "zone_if_attached1", "", 1},

	{ "zone_alias2", "WAN", 1},	/* Zone name */
	{ "zone_if_list2", "WAN_primary", 1},
	{ "zone_if_attached2", "", 1},

	{ "zone_if_proto2", "dhcpc", 1}, /* For UI to record the protocol of WAN */
	
/* interfaces stuff */
	{ "if_alias0", "LAN", 1},
	{ "if_dev0", "br0", 1},
	{ "if_phy0", "eth0", 1},
	{ "if_proto0", "static", 1},	/* static|dhcpc|pppoe */
	{ "if_proto_alias0", "LAN_static", 1},	/* alias of proto */
	{ "if_mode0", "route", 1},	/* route|nat XXX: up on ifup */
	{ "if_trust0", "", 1},		/* trust packet from interface alias */
	{ "if_services0", LAN_SERVICES , 1},
	{ "if_route0", "", 1},		/* fmt: NAME1/NET/MASK/GW/Metric,NAME2/NET..*/
	{ "if_icmp0", "1", 1},		/* 1: enable. 0: disable*/

	{ "if_alias1", "DMZ", 1},
	{ "if_dev1", "eth2", 1},
	{ "if_phy1", "eth2", 1},
	{ "if_proto1", "static", 1},	/* static|dhcpc|pppoe */
	{ "if_proto_alias1", "DMZ_static", 1},	/* alias of proto */
	{ "if_mode1", "route", 1},	/* route|nat */
	{ "if_trust1", "LAN", 1},
	{ "if_services1", DMZ_SERVICES, 1},
	{ "if_route2", "", 1},
	{ "if_icmp2", "1", 1},		/* 1: enable. 0: disable*/

	{ "if_alias2", "WAN_primary", 1},
	{ "if_dev2", "eth1", 1},
	{ "if_phy2", "eth1", 1},
	{ "if_proto2", "dhcpc", 1},	/* static|dhcpc|pppoe */
	{ "if_proto_alias2", "WAN_dhcpc", 1},	/* alias of proto */
	{ "if_mode2", "nat", 1},	/* route|nat */
	{ "if_trust2", "LAN,DMZ", 1},	
	{ "if_services2", PROTO_SINGLE_SERVICES, 1},
	{ "if_route2", "", 1},
	{ "if_icmp2", "1", 1},		/* 1: enable. 0: disable*/
	
	{ "if_alias3", "WAN_slave", 1},
	{ "if_dev3", "ppp3", 1},
	{ "if_phy3", "eth1", 1},
	{ "if_proto3", "", 1},	/* pppoe|pptp|l2tp|rupppoe|rupptp|rul2tp */
	{ "if_proto_alias3", "WAN_pppoe_ru", 1},	/* alias of proto */
	{ "if_mode3", "nat", 1},	/* route|nat */
	{ "if_trust3", "LAN,DMZ", 1},		/* 0|1 */
	{ "if_services3", PROTO_DUAL1_SERVICES, 1},
	{ "if_route3", "", 1},
	{ "if_icmp3", "1", 1},		/* 1: enable. 0: disable*/

/* protocol static stuff */
	{ "static_alias0", "LAN_static", 1},
	{ "static_hostname0", "DIR-730", 1},
	{ "static_addr0", "192.168.0.1", 1},
	{ "static_netmask0", "255.255.255.0", 1},
	{ "static_gateway0", "", 1},
	{ "static_primary_dns0", "", 1},
	{ "static_secondary_dns0", "", 1},
	{ "static_mac0", "", 1},
	{ "static_mtu0", "1500", 1},
	{ "static_iplist0", "", 1}, /* smtp@192.168.3.2,dns@192.168.3.3 */
	
	{ "static_alias1", "DMZ_static", 1},
	{ "static_hostname1", "DIR-730", 1},
	{ "static_addr1", "172.17.1.1", 1},
	{ "static_netmask1", "255.255.255.0", 1},
	{ "static_gateway1", "", 1},
	{ "static_primary_dns1", "", 1},
	{ "static_secondary_dns1", "", 1},
	{ "static_mac1", "", 1},
	{ "static_mtu1", "1500", 1},
	{ "static_iplist1", "", 1}, /* smtp@192.168.3.2,dns@192.168.3.3 */
	
	{ "static_alias2", "WAN_static", 1},
	{ "static_hostname2", "DIR-730", 1},
	{ "static_addr2", "", 1},
	{ "static_netmask2", "", 1},
	{ "static_gateway2", "", 1},
	{ "static_primary_dns2", "", 1},
	{ "static_secondary_dns2", "", 1},
	{ "static_mac2", "", 1},
	{ "static_mtu2", "1500", 1},
	{ "static_iplist2", "", 1}, /* smtp@192.168.3.2,dns@192.168.3.3 */

	{ "static_alias3", "WAN_static_rupppoe", 1},
	{ "static_hostname3", "DIR-730", 1},
	{ "static_addr3", "192.168.3.1", 1},
	{ "static_netmask3", "255.255.255.0", 1},
	{ "static_gateway3", "192.168.3.254", 1},
	{ "static_primary_dns3", "192.168.3.254", 1},
	{ "static_secondary_dns3", "", 1},
	{ "static_mac3", "", 1},
	{ "static_mtu3", "", 1},

	{ "static_alias4", "WAN_static_rupptp", 1},
	{ "static_hostname4", "DIR-730", 1},
	{ "static_addr4", "192.168.3.1", 1},
	{ "static_netmask4", "255.255.255.0", 1},
	{ "static_gateway4", "192.168.3.254", 1},
	{ "static_primary_dns4", "192.168.3.254", 1},
	{ "static_secondary_dns4", "", 1},
	{ "static_mac4", "", 1},
	{ "static_mtu4", "", 1},


/* protocol DHCPC stuff */
	{ "dhcpc_alias0", "LAN_dhcpc", 1},
	{ "dhcpc_hostname0", "DIR-730", 1},
	{ "dhcpc_addr0", "", 1},
	{ "dhcpc_netmask0", "", 1},
	{ "dhcpc_gateway0", "", 1},
	{ "dhcpc_primary_dns0", "", 1},
	{ "dhcpc_secondary_dns0", "", 1},
	{ "dhcpc_mac0", "", 1},
	{ "dhcpc_mtu0", "1500", 1},
	
	{ "dhcpc_alias1", "DMZ_dhcpc", 1},
	{ "dhcpc_hostname1", "DIR-730", 1},
	{ "dhcpc_addr1", "", 1},
	{ "dhcpc_netmask1", "", 1},
	{ "dhcpc_gateway1", "", 1},
	{ "dhcpc_primary_dns1", "", 1},
	{ "dhcpc_secondary_dns1", "", 1},
	{ "dhcpc_mac1", "", 1},
	{ "dhcpc_mtu1", "1500", 1},
	
	{ "dhcpc_alias2", "WAN_dhcpc", 1},
	{ "dhcpc_hostname2", "", 1},
	{ "dhcpc_addr2", "", 1},
	{ "dhcpc_netmask2", "", 1},
	{ "dhcpc_gateway2", "", 1},
	{ "dhcpc_mac2", "", 1},
	{ "dhcpc_primary_dns2", "", 1},
	{ "dhcpc_secondary_dns2", "", 1},
	{ "dhcpc_mtu2", "1500", 1},

	{ "dhcpc_alias3", "WAN_dhcpc_rupppoe", 1},
	{ "dhcpc_hostname3", "DIR-730", 1},
	{ "dhcpc_addr3", "", 1},
	{ "dhcpc_netmask3", "", 1},
	{ "dhcpc_gateway3", "", 1},
	{ "dhcpc_mac3", "", 1},
	{ "dhcpc_primary_dns3", "", 1},
	{ "dhcpc_secondary_dns3", "", 1},
	{ "dhcpc_mtu3", "", 1},

	{ "dhcpc_alias4", "WAN_dhcpc_rupptp", 1},
	{ "dhcpc_hostname4", "DIR-730", 1},
	{ "dhcpc_addr4", "", 1},
	{ "dhcpc_netmask4", "", 1},
	{ "dhcpc_gateway4", "", 1},
	{ "dhcpc_mac4", "", 1},
	{ "dhcpc_primary_dns4", "", 1},
	{ "dhcpc_secondary_dns4", "", 1},
	{ "dhcpc_mtu4", "", 1},


/* protocol ppp stuff */


/* protocol pppoe stuff */
	{ "pppoe_alias0", "LAN_pppoe", 1},
	{ "pppoe_hostname0", "wan_pppoe", 1},
	{ "pppoe_servicename0", "wan_pppoe", 1},
	{ "pppoe_addr0", "", 1},
	{ "pppoe_netmask0", "", 1},
	{ "pppoe_primary_dns0", "", 1},
	{ "pppoe_secondary_dns0", "", 1},
	{ "pppoe_mtu0", "1492", 1},
	{ "pppoe_user0", "vincent", 1},
	{ "pppoe_pass0", "1234", 1},		/* password */
	{ "pppoe_idle0", "300", 1},	/* idle time by sec */
	{ "pppoe_unit0", "0", 1},	/* the index of pppX, ex ppp3 */
	{ "pppoe_mru0", "1496", 1},
	{ "pppoe_dial0", "1", 1},	/* "0" manual, "1" always, "2" demand, "3" */
	{ "pppoe_mode0", "1", 1},	/* "1" dynamic, "0" static*/
	{ "pppoe_mac0", "", 1},

	{ "pppoe_alias1", "DMZ_pppoe", 1},
	{ "pppoe_hostname1", "wan_pppoe", 1},
	{ "pppoe_servicename1", "wan_pppoe", 1},
	{ "pppoe_addr1", "", 1},
	{ "pppoe_netmask1", "", 1},
	{ "pppoe_primary_dns1", "", 1},
	{ "pppoe_secondary_dns1", "", 1},
	{ "pppoe_mtu1", "1492", 1},
	{ "pppoe_user1", "vincent", 1},
	{ "pppoe_pass1", "1234", 1},		/* password */
	{ "pppoe_idle1", "300", 1},	/* idle time by sec */
	{ "pppoe_unit1", "0", 1},	/* the index of pppX, ex ppp3 */
	{ "pppoe_mru1", "1496", 1},
	{ "pppoe_dial1", "1", 1},	/* "0" manual, "1" always, "2" demand, "3" */
	{ "pppoe_mode1", "1", 1},	/* "1" dynamic, "0" static*/
	{ "pppoe_mac1", "", 1},
	
	{ "pppoe_alias2", "WAN_pppoe", 1},
	{ "pppoe_hostname2", "wan_pppoe", 1},
	{ "pppoe_servicename2", "", 1},
	{ "pppoe_addr2", "", 1},
	{ "pppoe_netmask2", "", 1},
	{ "pppoe_primary_dns2", "", 1},
	{ "pppoe_secondary_dns2", "", 1},
	{ "pppoe_mtu2", "1492", 1},
	{ "pppoe_user2", "", 1},
	{ "pppoe_pass2", "", 1},		/* password */
	{ "pppoe_idle2", "300", 1},	/* idle time by sec */
	{ "pppoe_unit2", "0", 1},	/* the index of pppX, ex ppp3 */
	{ "pppoe_mru2", "1496", 1},
	{ "pppoe_dial2", "1", 1},	/* "0" manual, "1" always, "2" demand, "3" */
	{ "pppoe_mode2", "1", 1},	/* "1" dynamic, "0" static*/
	{ "pppoe_mac2", "", 1},

	{ "pppoe_alias3", "WAN_pppoe_ru", 1},
	{ "pppoe_hostname3", "wan_pppoe_ru", 1},
	{ "pppoe_servicename3", "wan_pppoe_ru", 1},
	{ "pppoe_addr3", "", 1},
	{ "pppoe_netmask3", "", 1},
	{ "pppoe_primary_dns3", "", 1},
	{ "pppoe_secondary_dns3", "", 1},
	{ "pppoe_mtu3", "1492", 1},
	{ "pppoe_user3", "", 1},
	{ "pppoe_pass3", "1234", 1},		/* password */
	{ "pppoe_idle3", "300", 1},	/* idle time by sec */
	{ "pppoe_unit3", "0", 1},	/* the index of pppX, ex ppp3 */
	{ "pppoe_mru3", "1496", 1},
	{ "pppoe_dial3", "1", 1},	/* "0" manual, "1" always, "2" demand, "3" */
	{ "pppoe_mode3", "1", 1},	/* "1" dynamic, "0" static*/
	{ "pppoe_mac3", "", 1},

	{ "pppoe_alias4", "multi_pppoe_primary", 1},
	{ "pppoe_hostname4", "multi_pppoe_primary", 1},
	{ "pppoe_servicename4", "multi_pppoe_primary", 1},
	{ "pppoe_addr4", "", 1},
	{ "pppoe_netmask4", "", 1},
	{ "pppoe_primary_dns4", "", 1},
	{ "pppoe_secondary_dns4", "", 1},
	{ "pppoe_mtu4", "1492", 1},
	{ "pppoe_user4", "", 1},
	{ "pppoe_pass4", "1234", 1},		/* password */
	{ "pppoe_idle4", "300", 1},	/* idle time by sec */
	{ "pppoe_unit4", "0", 1},	/* the index of pppX, ex ppp3 */
	{ "pppoe_mru4", "1496", 1},
	{ "pppoe_dial4", "1", 1},	/* "0" manual, "1" always, "2" demand, "3" */
	{ "pppoe_mode4", "1", 1},	/* "1" dynamic, "0" static*/
	{ "pppoe_mac4", "", 1},
/*Just for multi-pppoe primary*/
	{ "pppoe_unnum_enable4", "", 1},	/* "1" Enable, "0" disable */
	{ "pppoe_unnum_ip4", "", 1},	/* for unnumber ip */
	{ "pppoe_unnum_netmask4", "", 1},

	{ "pppoe_alias5", "multi_pppoe_secondary", 1},
	{ "pppoe_hostname5", "multi_pppoe_secondary", 1},
	{ "pppoe_servicename5", "multi_pppoe_secondary", 1},
	{ "pppoe_addr5", "", 1},
	{ "pppoe_netmask5", "", 1},
	{ "pppoe_primary_dns5", "", 1},
	{ "pppoe_secondary_dns5", "", 1},
	{ "pppoe_mtu5", "1492", 1},
	{ "pppoe_user5", "", 1},
	{ "pppoe_pass5", "1234", 1},		/* password */
	{ "pppoe_idle5", "300", 1},	/* idle time by sec */
	{ "pppoe_unit5", "0", 1},	/* the index of pppX, ex ppp3 */
	{ "pppoe_mru5", "1496", 1},
	{ "pppoe_dial5", "1", 1},	/* "0" manual, "1" always, "2" demand, "3" */
	{ "pppoe_mode5", "1", 1},	/* "1" dynamic, "0" static*/
	{ "pppoe_mac5", "", 1},
/*Just for multi-pppoe secondary*/
	{ "pppoe_ntt5", "1492", 1},	/* "0" East, "1" West */
	{ "pppoe_policy_type5", "", 1}, /* "0" ip/domain, "1" tcp/udp protocol */

/* protocol pptp stuff */
	{ "pptp_alias0", "LAN_pptp", 1},
	{ "pptp_hostname0", "lan_pptp", 1},
	{ "pptp_addr0", "", 1},
	{ "pptp_netmask0", "", 1},
	{ "pptp_gateway0", "", 1},
	{ "pptp_dns0", "", 1},
	{ "pptp_mtu0", "1450", 1},
	{ "pptp_serverip0", "", 1},
	{ "pptp_user0", "vincent", 1},
	{ "pptp_pass0", "1234", 1},		/* password */
	{ "pptp_idle0", "300", 1},	/* idle time by sec */
	{ "pptp_unit0", "0", 1},	/* the index of pppX, ex ppp3 */
	{ "pptp_mru0", "1496", 1},
	{ "pptp_dial0", "1", 1},	/* "0" manual, "1" always, "2" demand, "3" */
	{ "pptp_remote0", "10.0.0.1", 1},
	{ "pptp_mode0", "1", 1},	/* "1" dynamic, "0" static*/
	{ "pptp_mppe0", "nomppe", 1},	/* require-mppe|nomppe */
	
	{ "pptp_alias1", "DMZ_pptp", 1},
	{ "pptp_hostname1", "dmz_pptp", 1},
	{ "pptp_addr1", "", 1},
	{ "pptp_netmask1", "", 1},
	{ "pptp_gateway1", "", 1},
	{ "pptp_dns1", "", 1},
	{ "pptp_mtu1", "1450", 1},
	{ "pptp_serverip1", "", 1},
	{ "pptp_user1", "vincent", 1},
	{ "pptp_pass1", "1234", 1},		/* password */
	{ "pptp_idle1", "300", 1},	/* idle time by sec */
	{ "pptp_unit1", "0", 1},	/* the index of pppX, ex ppp3 */
	{ "pptp_mru1", "1496", 1},
	{ "pptp_dial1", "1", 1},	/* "0" manual, "1" always, "2" demand, "3" */
	{ "pptp_remote1", "10.0.0.1", 1},
	{ "pptp_mode1", "1", 1},	/* "1" dynamic, "0" static*/
	{ "pptp_mppe1", "nomppe", 1},	/* require-mppe|nomppe */

	{ "pptp_alias2", "WAN_pptp", 1},
	{ "pptp_hostname2", "wan_pptp", 1},
	{ "pptp_addr2", "", 1},
	{ "pptp_netmask2", "", 1},
	{ "pptp_gateway2", "", 1},
	{ "pptp_dns2", "", 1},
	{ "pptp_mtu2", "1450", 1},
	{ "pptp_serverip2", "", 1},
	{ "pptp_user2", "", 1},
	{ "pptp_pass2", "1234", 1},		/* password */
	{ "pptp_idle2", "300", 1},	/* idle time by sec */
	{ "pptp_unit2", "0", 1},	/* the index of pppX, ex ppp3 */
	{ "pptp_mru2", "1496", 1},
	{ "pptp_dial2", "1", 1},	/* "0" manual, "1" always, "2" demand, "3" */
	{ "pptp_remote2", "10.0.0.1", 1},
	{ "pptp_mode2", "1", 1},	/* "1" dynamic, "0" static*/
	{ "pptp_mppe2", "nomppe", 1},	/* require-mppe|nomppe */

	
	{ "pptp_alias3", "WAN_pptp_ru", 1},
	{ "pptp_hostname3", "wan_pptp_ru", 1},
	{ "pptp_addr3", "", 1},
	{ "pptp_netmask3", "", 1},
	{ "pptp_gateway3", "", 1},
	{ "pptp_dns3", "", 1},
	{ "pptp_mtu3", "1450", 1},
	{ "pptp_serverip3", "", 1},
	{ "pptp_user3", "", 1},
	{ "pptp_pass3", "1234", 1},		/* password */
	{ "pptp_idle3", "300", 1},	/* idle time by sec */
	{ "pptp_unit3", "0", 1},	/* the index of pppX, ex ppp3 */
	{ "pptp_mru3", "1496", 1},
	{ "pptp_dial3", "1", 1},	/* "0" manual, "1" always, "2" demand, "3" */
	{ "pptp_remote3", "10.0.0.1", 1},
	{ "pptp_mode3", "1", 1},	/* "1" dynamic, "0" static*/
	{ "pptp_mppe3", "nomppe", 1},	/* require-mppe|nomppe */
	{ "pptp_mac3", "", 1},


/* protocol/service pptp/l2tp stuff */
	{ "l2tp_alias2", "WAN_l2tp", 1},
	{ "l2tp_hostname2", "WAN_l2tp", 1},
	{ "l2tp_addr2", "", 1},
	{ "l2tp_netmask2", "", 1},
	{ "l2tp_gateway2", "", 1},
	{ "l2tp_dns2", "", 1},
	{ "l2tp_mtu2", "1450", 1},
	{ "l2tp_serverip2", "", 1},
	{ "l2tp_user2", "", 1},
	{ "l2tp_pass2", "1234", 1},		/* password */
	{ "l2tp_idle2", "300", 1},	/* idle time by sec */
	{ "l2tp_unit2", "0", 1},	/* the index of pppX, ex ppp3 */
	{ "l2tp_mru2", "1496", 1},
	{ "l2tp_dial2", "1", 1},	/* "0" manual, "1" always, "2" demand, "3" */
	{ "l2tp_remote2", "10.0.0.1", 1},
	{ "l2tp_mode2", "1", 1},	/* "1" dynamic, "0" static*/
	{ "l2tp_mppe2", "nomppe", 1},	/* require-mppe|nomppe */
	
	
/* DHCPD stuff */
	{ "dhcpd_alias0", "LAN_dhcpd", 1},	/* DHCP server assign addr*/
	{ "dhcpd_start0", "192.168.0.100", 1},	/* DHCP server assign addr*/
	{ "dhcpd_end0", "192.168.0.200", 1},	
	{ "dhcpd_lease0", "86400", 1},		/* lease time in seconds */
	{ "dhcpd_dns0", "192.168.0.1", 1},
	{ "dhcpd_reserve0", "", 1},
	{ "dhcpd_dns_relay0", "1", 1},
	{ "dhcpd_enable0", "1", 1},	/* "1" enable DHCP Server, "0" disable DHCP server , "2" dhcp relay */

	{ "dhcpd_alias1", "DMZ_dhcpd", 1},
//	{ "dhcpd_dev1", "", 1},
	{ "dhcpd_start1", "172.17.1.100", 1},	/* DHCP server assign addr*/
	{ "dhcpd_end1", "172.17.1.200", 1},	
	{ "dhcpd_lease1", "86400", 1},		/* lease time in seconds */
	{ "dhcpd_dns1", "", 1},
	{ "dhcpd_reserve1", "", 1},
	{ "dhcpd_dns_relay1", "1", 1},
	{ "dhcpd_enable1", "1", 1},	/* "1" enable DHCP Server, "0" disable DHCP server */

	{ "dhcpd_alias4", "WLAN_dhcpd", 1},	/* DHCP server assign addr*/
	{ "dhcpd_start4", "192.168.10.100", 1},	/* DHCP server assign addr*/
	{ "dhcpd_end4", "192.168.10.200", 1},	
	{ "dhcpd_lease4", "86400", 1},		/* lease time in seconds */
	{ "dhcpd_dns4", "192.168.10.1", 1},
	{ "dhcpd_reserve4", "", 1},
	{ "dhcpd_dns_relay4", "1", 1},
	{ "dhcpd_enable4", "1", 1},	/* "1" enable DHCP Server, "0" disable DHCP server */

/* httpd profile */
	{ "httpd_alias0", "httpd_LAN", 1},
	{ "httpd_wanif0", "eth1", 1},
	{ "httpd_cfg0", "/tmp/password", 1},
	{ "httpd_port0", "80", 1},

/* https profile */
	{ "https_alias0", "https_REMOTE", 1},
	{ "https_lanif0", "br0", 1},
	{ "https_cfg0", "/tmp/https.conf", 1},
	{ "https_port0", "443", 1},

/* dns resolve*/

/* remote_admin */
	{ "remote_enable", "0", 1},
	{ "remote_ipaddr0", "", 1},
	{ "remote_ipaddr1", "", 1},
	{ "remote_ipaddr2", "", 1},
	{ "remote_ipaddr3", "", 1},
	{ "remote_ipaddr4", "", 1},
	{ "remote_port", "0", 1},
	{ "remote_schedule", "always", 1},
/* misc */
	{ "restore_defaults", "0", 1},

/* wireless rt*/
	{ "wlan0_eth", "ra0", 1},
	{ "wlan0_radio_mode", "0", 1},
	{ "wlan0_enable", "1", 1},
	{ "wlan0_emi_test", "0", 1},
	{ "wlan0_ssid", "DIR-730", 1},
	{ "wlan0_supper_g", "disable", 1},//
	{ "wlan0_domain", "US/NA", 1},
	{ "wlan0_channel", "6", 1},
	//{ "wlan0_channel_list", "1,2,3,4,5,6,7,8,9,10,11,12,13", 1},
	{ "wlan0_channel_list", "1,2,3,4,5,6,7,8,9,10,11", 1},
	{ "wlan0_auto_channel_enable", "1", 1},
	{ "wlan0_dot11_mode", "11bgn", 1},
	{ "wlan0_ssid_broadcast", "1", 1},
	{ "wlan0_security", "disable", 1},
	{ "wlan0_wep64_key_1", "0000000000", 1},
	{ "wlan0_wep64_key_2", "0000000000", 1},
	{ "wlan0_wep64_key_3", "0000000000", 1},
	{ "wlan0_wep64_key_4", "0000000000", 1},
	{ "wlan0_wep128_key_1", "00000000000000000000000000", 1},
	{ "wlan0_wep128_key_2", "00000000000000000000000000", 1},
	{ "wlan0_wep128_key_3", "00000000000000000000000000", 1},
	{ "wlan0_wep128_key_4", "00000000000000000000000000", 1},
	{ "wlan0_wep152_key_1", "00000000000000000000000000000000", 1},
	{ "wlan0_wep152_key_2", "00000000000000000000000000000000", 1},
	{ "wlan0_wep152_key_3", "00000000000000000000000000000000", 1},
	{ "wlan0_wep152_key_4", "00000000000000000000000000000000", 1},
	{ "wlan0_wep_display", "hex", 1},
	{ "wlan0_wep_default_key", "1", 1},
	{ "wlan0_psk_cipher_type", "tkip", 1},
	{ "wlan0_psk_pass_phrase", "1234567890", 1},
	{ "wlan0_eap_radius_server_0", "0.0.0.0/1812/", 1},
	{ "wlan0_eap_radius_server_1", "0.0.0.0/1812/", 1},
	{ "wlan0_eap_reauth_period", "60", 1},
	{ "wlan0_gkey_rekey_time", "3600", 1},
	{ "wlan0_auto_txrate", "33", 1},
	{ "wlan0_11b_txrate", "11", 1},
	{ "wlan0_11g_txrate", "54", 1},
	{ "wlan0_11bg_txrate", "54", 1},
	{ "wlan0_txpower", "19", 1},
	{ "wlan0_beacon_interval", "100", 1},
	{ "wlan0_rts_threshold", "2346", 1},
	{ "wlan0_fragmentation", "2346", 1},
	{ "wlan0_dtim", "1", 1},
	{ "wlan0_cts", "1", 1},
	{ "wlan0_11d_enable", "0", 1},
	{ "wlan0_partition", "0", 1},
	{ "wlan0_short_gi", "1", 1},
	{ "wlan0_wmm_enable", "1", 1},
	{ "wlan0_mode", "rt", 1},
	{ "wlan0_11n_protection", "20", 1},
	{ "wlan0_eap_mac_auth", "3", 1},
	{ "wlan0_countrycode", "842", 1},
	
	{ "wlan1_ssid", "DIR-730", 1},
	{ "wlan1_channel", "36", 1},
	//{ "wlan1_channel_list", "36,40,44,48,52,56,60,64,149,153,157,161,165", 1},
	{ "wlan1_channel_list", "36,40,44,48,149,153,157,161,165", 1},
	{ "wlan1_auto_channel_enable", "1", 1},
	{ "wlan1_dot11_mode", "11an", 1},
	{ "wlan1_ssid_broadcast", "1", 1},
	{ "wlan1_security", "disable", 1},
	{ "wlan1_wep64_key_1", "0000000000", 1},
	{ "wlan1_wep64_key_2", "0000000000", 1},
	{ "wlan1_wep64_key_3", "0000000000", 1},
	{ "wlan1_wep64_key_4", "0000000000", 1},
	{ "wlan1_wep128_key_1", "00000000000000000000000000", 1},
	{ "wlan1_wep128_key_2", "00000000000000000000000000", 1},
	{ "wlan1_wep128_key_3", "00000000000000000000000000", 1},
	{ "wlan1_wep128_key_4", "00000000000000000000000000", 1},
	{ "wlan1_wep152_key_1", "00000000000000000000000000000000", 1},
	{ "wlan1_wep152_key_2", "00000000000000000000000000000000", 1},
	{ "wlan1_wep152_key_3", "00000000000000000000000000000000", 1},
	{ "wlan1_wep152_key_4", "00000000000000000000000000000000", 1},
	{ "wlan1_wep_display", "hex", 1},
	{ "wlan1_wep_default_key", "1", 1},
	{ "wlan1_psk_cipher_type", "tkip", 1},
	{ "wlan1_psk_pass_phrase", "1234567890", 1},
	{ "wlan1_eap_radius_server_0", "0.0.0.0/1812/", 1},
	{ "wlan1_eap_radius_server_1", "0.0.0.0/1812/", 1},
	{ "wlan1_eap_reauth_period", "60", 1},
	{ "wlan1_gkey_rekey_time", "3600", 1},
	{ "wlan1_auto_txrate", "33", 1},
	{ "wlan1_beacon_interval", "100", 1},
	{ "wlan1_rts_threshold", "2346", 1},
	{ "wlan1_fragmentation", "2346", 1},
	{ "wlan1_dtim", "1", 1},
	{ "wlan1_cts", "1", 1},
	{ "wlan1_11d_enable", "1", 1},
	{ "wlan1_partition", "0", 1},
	{ "wlan1_short_gi", "1", 1},
	{ "wlan1_wmm_enable", "1", 1},
	{ "wlan1_11n_protection", "auto", 1},
	{ "wlan1_eap_mac_auth", "3", 1},
	{ "wlan1_countrycode", "842", 1},
	{ "wps_enable", "1", 1},
	{ "wps_configured_mode", "0", 1},

	{ "wps_lock", "0", 1},
	{ "wps_pin", "00000000", 1},
	{ "wps_default_pin", "00000000", 1},
	{ "wps_enable", "1", 1},
	{ "wps_1_enable", "1", 1},
	{ "wps_1_configured_mode", "1", 1},
	{ "wps_1_lock", "0", 1},
	{ "wps_1_pin", "00000000", 1},
	{ "tmp_wep_key_1", "0000000000", 1},
	{ "tmp_wep_key_2", "0000000000", 1},
	{ "tmp_wep_key_3", "0000000000", 1},
	{ "tmp_wep_key_4", "0000000000", 1},	

/* upnp */
	{ "upnp_enable", "1", 1},
	{ "model_url", "http://support.dlink.com", 1},
/* ddns */
	{ "ddns_enable", "0", 1},
	{ "ddns_type", "dlinkddns.com", 1},
	{ "ddns_hostname", "", 1},
	{ "ddns_username", "", 1},
	{ "ddns_password", "", 1},	
	{ "ddns_timeout", "86400", 1},//ddns_sync_interval
	{ "ddns_wildcards_enable", "0", 1},
	{ "ddns_status", "Disconnect", 1},
/* */
	{ "ips_enable", "0", 1},
	{ "dos_enable", "1", 1},
	{ "tcp_syn_flooding", "800", 1},
        { "udp_flooding", "500", 1},
        { "icmp_flooding", "10", 1},
        { "block_land_attack", "1", 1},
        { "block_ping_death", "1", 1},
        { "block_all_fragments", "1", 1},
        { "block_teardrop_attack", "1", 1},
        { "block_smurf_attack", "1", 1},
        { "icmp_filter_enable", "0", 1},
        { "icmp_setting", "", 1}, /*echo_reply, echo_request, unreachable, time_exceeded, redirection, source_quenching, parameter_problem*/

/*
 * FOR DIR-330 test
 *	It should only for test.
 *
 * */

	{ "DeviceName", "DeviceName", 0 },
	{ "AdminPassword", "", 0 },
	/* WAN settings */
	{ "Type", " ", 0 },
	{ "Username", "", 0 },
	{ "Password", "", 0 },
	{ "MaxIdleTime", "", 0 },
	{ "MTU", "", 0 },
	{ "ServiceName", "", 0 },
	{ "AutoReconnect", "False", 0 },
	{ "IPAddress", "", 0 },
	{ "SubnetMask", "", 0 },
	{ "Gateway", "", 0 },
	{ "Primary", "", 0 },
	{ "Secondary", "", 0 },
	{ "MacAddress", "00:00:00:00:00:00", 0 },
	/* WLAN settings include to use MacAddress parameter */
	{ "Enabled", "true", 0 },
	{ "SSID", "dlink", 0 },
	{ "SSIDBroadcast", "true", 0 },
	{ "Channel", "6", 0 },
	/* WLAN Security setting parameter */
	{ "WEPKeyBits", "", 0},
	{ "Key", "", 0},
	{ "RadiusIP1", "", 0},
	{ "RadiusPort1", "0", 0},
	{ "RadiusIP2", "", 0},
	{ "RadiusPort2", "0", 0},
	/* MACFilters2 */	
	{ "IsAllowList", "false", 0 },	/* true - allowd list// false - denied list  */
	/* RouterLanSettings*/
	{ "RouterIPAddress", "", 0},
	{ "RouterSubnetMask", "", 0},
	{ "DHCPServerEnabled", "", 0},
	/* PortMapping for Del and Add PortMapping */
	{ "PortMappingDescription", "", 0},
	{ "InternalClient", "", 0},
	{ "PortMappingProtocol", "", 0},
	{ "ExternalPort", "", 0},
	{ "InternalPort", "", 0},
	
	/*  Datatypes Structure Key for Pure Network */
	{ "filter_mac_name_list", "", 0},
	{ "MACInfo", "", 0 },		/* MacAddress/DeviceName# */
	{ "PortMapping", "", 0},	/* PortMapping Description/InternalClient/
					   PortMappingProtocol/ExternalPort/InternalPort# */
	{ "ConnectedClient", "", 0},	/* ConnectTime/MacAddress/DeviceName/PortName/
					   Wireless/Active# */
	{ "NetworkStats", "", 0},	/* PortName/PacketsReceived/PacketsSent/
					   BytesEceived/BytesSent# */

	/* Miscellaneous parameters */
	{ "wan_port_speed", "auto", 1 },	/* wan port speed */
	{ "log_level", "0", 1 },		/* Bitmask 0:off 1:denied 2:accepted */
	{ "ezc_enable", "1", 0 },		/* Enable EZConfig updates */
	{ "is_default", "1", 0 },		/* is it default setting: 1:yes 0:no*/
	{ "os_server", "", 0 },			/* URL for getting upgrades */
	{ "stats_server", "", 0 },		/* URL for posting stats */
	{ "console_loglevel", "1", 0 },		/* Kernel panics only */

	/* Big switches */
	{ "router_disable", "0", 0 },		/* lan_proto=static lan_stp=0 wan_proto=disabled */
	{ "ure_disable", "1", 0 },		/* sets APSTA for radio and puts wirelesss											 interfaces in correct lan */
	{ "fw_disable", "0", 1 },		/* Disable firewall (allow new connections from the WAN) */
		
	/* LAN H/W parameters */
	{ "lan_ifname", "", 0 },		/* LAN interface name */
	{ "lan_ifnames", "", 0 },		/* Enslaved LAN interfaces */
	{ "lan_hwnames", "", 0 },		/* LAN driver names (e.g. et0) */
	{ "lan_hwaddr", "", 0 },		/* LAN interface MAC address */
	
	/* LAN TCP/IP parameters */
	{ "lan_dhcp", "0", 1 },			/* DHCP client [static|dhcp] */
	{ "lan_ipaddr", "192.168.0.1", 1 },	/* LAN IP address */
	{ "lan_netmask", "255.255.255.0", 1 },	/* LAN netmask */
	{ "lan_gateway", "192.168.0.1", 1 },	/* LAN gateway */
	{ "lan_proto", "dhcp", 1 },		/* DHCP server [static|dhcp] */
	{ "lan_wins", "", 1 },			/* x.x.x.x x.x.x.x ... */
	{ "lan_domain", "", 1 },		/* LAN domain name */
	{ "lan_lease", "86400", 1 },		/* LAN lease time in seconds */
	{ "lan_stp", "0", 0 },			/* LAN spanning tree protocol */
	{ "lan_route", "", 0 },			/* Static routes (ipaddr:netmask:gateway:metric:ifname ...) */
#define __CONFIG_NAT__	//testing
#ifdef __CONFIG_NAT__
	/* WAN H/W parameters */
	
	/* DHCP server parameters */
	{ "dhcp_start", "192.168.0.100", 1 },	/* First assignable DHCP address */
	{ "dhcp_end", "192.168.0.150", 1 },	/* Last assignable DHCP address */
	{ "dns_relay", "1", 1 },		/* Use dnsmasq to do dns relay if enable ___by Tony*/
	{ "dhcpd_reserve_00", "", 1},		/* Static DHCP reserve rule */
	{ "dhcpd_reserve_01", "", 1},
	{ "dhcpd_reserve_02", "", 1},
	{ "dhcpd_reserve_03", "", 1},
	{ "dhcpd_reserve_04", "", 1},
	{ "dhcpd_reserve_05", "", 1},
	{ "dhcpd_reserve_06", "", 1},
	{ "dhcpd_reserve_07", "", 1},
	{ "dhcpd_reserve_08", "", 1},
	{ "dhcpd_reserve_09", "", 1},
	{ "dhcpd_reserve_10", "", 1},
	{ "dhcpd_reserve_11", "", 1},
	{ "dhcpd_reserve_12", "", 1},
	{ "dhcpd_reserve_13", "", 1},
	{ "dhcpd_reserve_14", "", 1},
	{ "dhcpd_reserve_15", "", 1},
	{ "dhcpd_reserve_16", "", 1},
	{ "dhcpd_reserve_17", "", 1},
	{ "dhcpd_reserve_18", "", 1},
	{ "dhcpd_reserve_19", "", 1},
	{ "dhcpd_reserve_20", "", 1},
	{ "dhcpd_reserve_21", "", 1},
	{ "dhcpd_reserve_22", "", 1},
	{ "dhcpd_reserve_23", "", 1},
	{ "dhcpd_reserve_24", "", 1},
	{ "dhcpd_reserve_25", "", 1},
	{ "dhcpd_reserve_26", "", 1},
	{ "dhcpd_reserve_27", "", 1},
	{ "dhcpd_reserve_28", "", 1},
	{ "dhcpd_reserve_29", "", 1},
	{ "dhcpd_reserve_30", "", 1},
	{ "dhcpd_reserve_31", "", 1},
	{ "dhcpd_reserve_32", "", 1},
	{ "dhcpd_reserve_33", "", 1},
	{ "dhcpd_reserve_34", "", 1},
	{ "dhcpd_reserve_35", "", 1},
	{ "dhcpd_reserve_36", "", 1},
	{ "dhcpd_reserve_37", "", 1},
	{ "dhcpd_reserve_38", "", 1},
	{ "dhcpd_reserve_39", "", 1},
	{ "dhcpd_reserve_40", "", 1},
	{ "dhcpd_reserve_41", "", 1},
	{ "dhcpd_reserve_42", "", 1},
	{ "dhcpd_reserve_43", "", 1},
	{ "dhcpd_reserve_44", "", 1},
	{ "dhcpd_reserve_45", "", 1},
	{ "dhcpd_reserve_46", "", 1},
	{ "dhcpd_reserve_47", "", 1},
	{ "dhcpd_reserve_48", "", 1},
	{ "dhcpd_reserve_49", "", 1},
	/* Filters */
	{ "filter_maclist", "", 1 },		/* xx:xx:xx:xx:xx:xx ... */
	{ "filter_macmode", "disabled", 1 },	/* "allow" only, "deny" only, or "disabled" (allow all) */
	{ "filter_client0", "", 1 },		/* [lan_ipaddr0-lan_ipaddr1|*]:lan_port0-lan_port1,proto,enable,day_start-day_end,sec_start-sec_end,desc */

	/* DMZ setting */
	{ "dmz_ipaddr", "0.0.0.0", 1 },		/* x.x.x.x (equivalent to 0-60999>dmz_ipaddr:0-60999) */
	{ "dmz_enable", "0", 1 },		/* x.x.x.x (equivalent to 0-60999>dmz_ipaddr:0-60999) */
	{ "dmz_display", "0.0.0.0", 1 },	/* x.x.x.x (equivalent to 0-60999>dmz_ipaddr:0-60999) */
	{ "dmz_schedule", "always", 1 },	/* "always" or index of schedule [0-19] */
	/* Pass thruogh */
	{ "pptp_pt", "1", 1},
	{ "l2tp_pt", "1", 1},
	{ "ipsec_pt", "1", 1},
	{ "wan0_ping_enable", "1", 1},		/* Enable ping to WAN port from WIDE WAN */
	{ "upnp_enable", "1", 1},		/* Enable UPNP */
	{ "forward_port0", "", 1 },		/* Port Forwarding*/ /* wan_port0-wan_port1>lan_ipaddr:lan_port0-lan_port1[:,]proto[:,]enable[:,]desc */
        { "forward_port1", "", 1},		
        { "forward_port2", "", 1},
        { "forward_port3", "", 1},
        { "forward_port4", "", 1},
        { "forward_port5", "", 1},
        { "forward_port6", "", 1},
        { "forward_port7", "", 1},
        { "forward_port8", "", 1},
        { "forward_port9", "", 1},
        { "forward_port10", "", 1},
        { "forward_port11", "", 1},
        { "forward_port12", "", 1},
        { "forward_port13", "", 1},
        { "forward_port14", "", 1},
        { "forward_port15", "", 1},
        { "forward_port16", "", 1},
        { "forward_port17", "", 1},
        { "forward_port18", "", 1},
        { "forward_port19", "", 1},
	{ "forward_port20", "", 1},
	{ "forward_port21", "", 1},
	{ "forward_port22", "", 1},
	{ "forward_port23", "", 1},
	{ "forward_port24", "", 1},
	/* Port forwards */
	{ "forward_port25", "", 1},
	{ "forward_port26", "", 1},
	{ "forward_port27", "", 1},
	{ "forward_port28", "", 1},
	{ "forward_port29", "", 1},
	{ "forward_port30", "", 1},
	{ "forward_port31", "", 1},
	{ "forward_port32", "", 1},
	{ "forward_port33", "", 1},
	{ "forward_port34", "", 1},
	{ "forward_port35", "", 1},
	{ "forward_port36", "", 1},
	{ "forward_port37", "", 1},
	{ "forward_port38", "", 1},
	{ "forward_port39", "", 1},
	{ "forward_port40", "", 1},
	{ "forward_port41", "", 1},
	{ "forward_port42", "", 1},
	{ "forward_port43", "", 1},
	{ "forward_port44", "", 1},
	{ "forward_port45", "", 1},
	{ "forward_port46", "", 1},
	{ "forward_port47", "", 1},
        { "fw_forward_port0", "", 1},		
        { "fw_forward_port1", "", 1},		
        { "fw_forward_port2", "", 1},
        { "fw_forward_port3", "", 1},
        { "fw_forward_port4", "", 1},
        { "fw_forward_port5", "", 1},
        { "fw_forward_port6", "", 1},
        { "fw_forward_port7", "", 1},
        { "fw_forward_port8", "", 1},
        { "fw_forward_port9", "", 1},
        { "fw_forward_port10", "", 1},
        { "fw_forward_port11", "", 1},
        { "fw_forward_port12", "", 1},
        { "fw_forward_port13", "", 1},
        { "fw_forward_port14", "", 1},
        { "fw_forward_port15", "", 1},
        { "fw_forward_port16", "", 1},
        { "fw_forward_port17", "", 1},
        { "fw_forward_port18", "", 1},
        { "fw_forward_port19", "", 1},
	{ "fw_forward_port20", "", 1},
	{ "fw_forward_port21", "", 1},
	{ "fw_forward_port22", "", 1},
	{ "fw_forward_port23", "", 1},
	{ "fw_forward_port24", "", 1},
	/* Advance Firewalls */
	{ "app_port0", "", 1},	/* out_proto:out_ports>in_ports,enable,desc,sched */
	{ "app_port1", "", 1},
	{ "app_port2", "", 1},
	{ "app_port3", "", 1},
	{ "app_port4", "", 1},
	{ "app_port5", "", 1},
	{ "app_port6", "", 1},
	{ "app_port7", "", 1},
	{ "app_port8", "", 1},
	{ "app_port9", "", 1},
	{ "app_port10", "", 1},
	{ "app_port11", "", 1},
	{ "app_port12", "", 1},
	{ "app_port13", "", 1},
	{ "app_port14", "", 1},
	{ "app_port15", "", 1},
	{ "app_port16", "", 1},
	{ "app_port17", "", 1},
	{ "app_port18", "", 1},
	{ "app_port19", "", 1},
	{ "app_port20", "", 1},
	{ "app_port21", "", 1},
	{ "app_port22", "", 1},
	{ "app_port23", "", 1},
	{ "app_port24", "", 1},
//#ifdef __CONFIG_IPSEC_FREESWAN__ 
	{ "httpd_authmethod", "DB", 1},
	{ "vpn_nat_traversal", "1", 1},
	{ "vpn_conn1", "", 1},		/* VPN connection rules */
	{ "vpn_conn2", "", 1},
	{ "vpn_conn3", "", 1},
	{ "vpn_conn4", "", 1},
	{ "vpn_conn5", "", 1},
	{ "vpn_conn6", "", 1},
	{ "vpn_conn7", "", 1},
	{ "vpn_conn8", "", 1},
	{ "vpn_conn9", "", 1},
	{ "vpn_conn10", "", 1},
	{ "vpn_conn11", "", 1},
	{ "vpn_conn12", "", 1},
	{ "vpn_conn13", "", 1},
	{ "vpn_conn14", "", 1},
	{ "vpn_conn15", "", 1},
	{ "vpn_conn16", "", 1},
	{ "vpn_conn17", "", 1},
	{ "vpn_conn18", "", 1},
	{ "vpn_conn19", "", 1},
	{ "vpn_conn20", "", 1},
	{ "vpn_conn21", "", 1},
	{ "vpn_conn22", "", 1},
	{ "vpn_conn23", "", 1},
	{ "vpn_conn24", "", 1},
	{ "vpn_conn25", "", 1},
	{ "vpn_conn26", "", 1},
	{ "vpn_conn27", "", 1},
	{ "vpn_conn28", "", 1},
	{ "vpn_conn29", "", 1},
	{ "vpn_conn30", "", 1},
	{ "vpn_conn31", "", 1},
	{ "vpn_conn32", "", 1},
	{ "vpn_conn33", "", 1},
	{ "vpn_conn34", "", 1},
	{ "vpn_conn35", "", 1},
	{ "vpn_conn36", "", 1},
	{ "vpn_conn37", "", 1},
	{ "vpn_conn38", "", 1},
	{ "vpn_conn39", "", 1},
	{ "vpn_conn40", "", 1},
	{ "vpn_conn51", "", 1},
	{ "vpn_conn52", "", 1},
	{ "vpn_conn53", "", 1},
	{ "vpn_conn54", "", 1},
	{ "vpn_conn55", "", 1},
	{ "vpn_conn56", "", 1},
	{ "vpn_conn57", "", 1},
	{ "vpn_conn58", "", 1},
	{ "vpn_conn59", "", 1},
	{ "vpn_conn60", "", 1},
	{ "vpn_conn61", "", 1},
	{ "vpn_conn62", "", 1},
	{ "vpn_conn63", "", 1},
	{ "vpn_conn64", "", 1},
	{ "vpn_conn65", "", 1},
	{ "vpn_conn66", "", 1},
	{ "vpn_conn67", "", 1},
	{ "vpn_conn68", "", 1},
	{ "vpn_conn69", "", 1},
	{ "vpn_conn70", "", 1},
	{ "vpn_conn71", "", 1},
	{ "vpn_conn72", "", 1},
	{ "vpn_conn73", "", 1},
	{ "vpn_conn74", "", 1},
	{ "vpn_conn75", "", 1},
	{ "vpn_conn76", "", 1},
	{ "vpn_conn77", "", 1},
	{ "vpn_conn78", "", 1},
	{ "vpn_conn79", "", 1},
	{ "vpn_conn80", "", 1},
	{ "vpn_conn81", "", 1},
	{ "vpn_conn82", "", 1},
	{ "vpn_conn83", "", 1},
	{ "vpn_conn84", "", 1},
	{ "vpn_conn85", "", 1},
	{ "vpn_conn86", "", 1},
	{ "vpn_conn87", "", 1},
	{ "vpn_conn88", "", 1},
	{ "vpn_conn89", "", 1},
	{ "vpn_conn90", "", 1},
	{ "vpn_conn91", "", 1},
	{ "vpn_conn92", "", 1},
	{ "vpn_conn93", "", 1},
	{ "vpn_conn94", "", 1},
	{ "vpn_conn95", "", 1},
	{ "vpn_conn96", "", 1},
	{ "vpn_conn97", "", 1},
	{ "vpn_conn98", "", 1},
	{ "vpn_conn99", "", 1},
	{ "vpn_conn100", "", 1},
	{ "vpn_extra1", "", 1},		/* VPN connection rules */
	{ "vpn_extra2", "", 1},
	{ "vpn_extra3", "", 1},
	{ "vpn_extra4", "", 1},
	{ "vpn_extra5", "", 1},
	{ "vpn_extra6", "", 1},
	{ "vpn_extra7", "", 1},
	{ "vpn_extra8", "", 1},
	{ "vpn_extra9", "", 1},
	{ "vpn_extra10", "", 1},
	{ "vpn_extra11", "", 1},
	{ "vpn_extra12", "", 1},
	{ "vpn_extra13", "", 1},
	{ "vpn_extra14", "", 1},
	{ "vpn_extra15", "", 1},
	{ "vpn_extra16", "", 1},
	{ "vpn_extra17", "", 1},
	{ "vpn_extra18", "", 1},
	{ "vpn_extra19", "", 1},
	{ "vpn_extra20", "", 1},
	{ "vpn_extra21", "", 1},
	{ "vpn_extra22", "", 1},
	{ "vpn_extra23", "", 1},
	{ "vpn_extra24", "", 1},
	{ "vpn_extra25", "", 1},
	{ "vpn_extra26", "", 1},
   	{ "vpn_extra27", "", 1},
	{ "vpn_extra28", "", 1},
	{ "vpn_extra29", "", 1},
	{ "vpn_extra30", "", 1},
   	{ "vpn_extra31", "", 1},
	{ "vpn_extra32", "", 1},
	{ "vpn_extra33", "", 1},
	{ "vpn_extra34", "", 1},
	{ "vpn_extra35", "", 1},
	{ "vpn_extra36", "", 1},
   	{ "vpn_extra37", "", 1},
	{ "vpn_extra38", "", 1},
	{ "vpn_extra39", "", 1},
	{ "vpn_extra40", "", 1},
    	{ "vpn_extra41", "", 1},
	{ "vpn_extra42", "", 1},
	{ "vpn_extra43", "", 1},
	{ "vpn_extra44", "", 1},
	{ "vpn_extra45", "", 1},
	{ "vpn_extra46", "", 1},
    	{ "vpn_extra47", "", 1},
	{ "vpn_extra48", "", 1},
	{ "vpn_extra49", "", 1},
	{ "vpn_extra50", "", 1},
	{ "vpn_extra51", "", 1},
	{ "vpn_extra52", "", 1},
	{ "vpn_extra53", "", 1},
	{ "vpn_extra54", "", 1},
	{ "vpn_extra55", "", 1},
	{ "vpn_extra56", "", 1},
	{ "vpn_extra57", "", 1},
	{ "vpn_extra58", "", 1},
	{ "vpn_extra59", "", 1},
	{ "vpn_extra60", "", 1},
	{ "vpn_extra61", "", 1},
	{ "vpn_extra62", "", 1},
	{ "vpn_extra63", "", 1},
	{ "vpn_extra64", "", 1},
	{ "vpn_extra65", "", 1},
	{ "vpn_extra66", "", 1},
	{ "vpn_extra67", "", 1},
	{ "vpn_extra68", "", 1},
	{ "vpn_extra69", "", 1},
	{ "vpn_extra70", "", 1},
	{ "vpn_extra71", "", 1},
	{ "vpn_extra72", "", 1},
	{ "vpn_extra73", "", 1},
	{ "vpn_extra74", "", 1},
	{ "vpn_extra75", "", 1},
	{ "vpn_extra76", "", 1},
	{ "vpn_extra77", "", 1},
	{ "vpn_extra78", "", 1},
	{ "vpn_extra79", "", 1},
	{ "vpn_extra80", "", 1},
	{ "vpn_extra81", "", 1},
	{ "vpn_extra82", "", 1},
	{ "vpn_extra83", "", 1},
	{ "vpn_extra84", "", 1},
	{ "vpn_extra85", "", 1},
	{ "vpn_extra86", "", 1},
	{ "vpn_extra87", "", 1},
	{ "vpn_extra88", "", 1},
	{ "vpn_extra89", "", 1},
	{ "vpn_extra90", "", 1},
	{ "vpn_extra91", "", 1},
	{ "vpn_extra92", "", 1},
	{ "vpn_extra93", "", 1},
	{ "vpn_extra94", "", 1},
	{ "vpn_extra95", "", 1},
	{ "vpn_extra96", "", 1},
	{ "vpn_extra97", "", 1},
	{ "vpn_extra98", "", 1},
	{ "vpn_extra99", "", 1},
	{ "vpn_extra100", "", 1},
	{ "fw_vpn_conn0", "", 1},		
        { "fw_vpn_conn1", "", 1},		
        { "fw_vpn_conn2", "", 1},		
        { "fw_vpn_conn3", "", 1},		
        { "fw_vpn_conn4", "", 1},		
        { "fw_vpn_conn5", "", 1},		
        { "fw_vpn_conn6", "", 1},		
        { "fw_vpn_conn7", "", 1},		
        { "fw_vpn_conn8", "", 1},		
        { "fw_vpn_conn9", "", 1},		
        { "fw_vpn_conn10", "", 1},		
        { "fw_vpn_conn11", "", 1},		
        { "fw_vpn_conn12", "", 1},		
        { "fw_vpn_conn13", "", 1},		
        { "fw_vpn_conn14", "", 1},		
        { "fw_vpn_conn15", "", 1},		
        { "fw_vpn_conn16", "", 1},		
        { "fw_vpn_conn17", "", 1},		
        { "fw_vpn_conn18", "", 1},		
        { "fw_vpn_conn19", "", 1},		
        { "fw_vpn_conn20", "", 1},		
        { "fw_vpn_conn21", "", 1},		
        { "fw_vpn_conn22", "", 1},		
        { "fw_vpn_conn23", "", 1},		
        { "fw_vpn_conn24", "", 1},		
        { "fw_vpn_conn25", "", 1},		
//#endif	/* __CONFIG_IPSEC_FREESWAN__ */
#endif	/* __CONFIG_NAT__ */

	{ "sslvpn1", "", 1},	//sslvpn settings
	{ "sslvpn2", "", 1},
	{ "sslvpn3", "", 1},
	{ "sslvpn4", "", 1},
	{ "sslvpn5", "", 1},
	{ "sslvpn6", "", 1},
	{ "https_pem", "", 1},
	/* Web server parameters */
	{ "ro_username", "user", 1 },		/* read only username */
	{ "ro_password", "", 1 },		/* read only password */
	{ "http_username", "admin", 1 },	/* Username */
	{ "admin_password", "", 1},		/* Password */
	{ "http_wanport", "80", 1 },		/* WAN port to listen on */
	{ "http_lanport", "80", 1 },		/* LAN port to listen on */
	{ "http_group", "", 1 },		/* Which admin group can login */
	{ "http_group_enable", "", 1 },		/* Let admin group enable */
	{ "rdonly_group", "", 1 },              /* readonly account groups */
	{ "rdonly_group_enable", "0", 1 },      /* Allow readonly account login */
	{ "remote_ssl", "0", 1 },		/* allow admin page login from https */

	/* Wireless parameters */

	{ "wl_ifname", "", 0 },			/* Interface name */
	{ "wl_hwaddr", "", 0 },			/* MAC address */
	{ "wl_phytype", "b", 0 },		/* Current wireless band ("a" (5 GHz), "b" (2.4 GHz), or "g" (2.4 GHz)) */
	{ "wl_corerev", "", 0 },		/* Current core revision */
	{ "wl_phytypes", "", 0 },		/* List of supported wireless bands (e.g. "ga") */
	{ "wl_radioids", "", 0 },		/* List of radio IDs */
	{ "wl_mssid_extended", "off", 0 },	/* MSSID extended mode (off|on) */
	{ "wl_country", "", 0 },		/* Country (default obtained from driver) */
	{ "wl_country_code", "", 0 },		/* Country Code (default obtained from driver) */
        { "wl_ap_isolate", "0", 0 },            /* AP isolate mode */
	{ "wl_mode", "ap", 0 },			/* AP mode (ap|sta|wds) */
	{ "wl_lazywds", "0", 0 },		/* Enable "lazy" WDS mode (0|1) */
	{ "wl_wds", "", 0 },			/* xx:xx:xx:xx:xx:xx ... */
	{ "wl_wds_timeout", "1", 0 },		/* WDS link detection interval defualt 1 sec*/
	{ "wl_radio", "1", 0 },			/* Enable (1) or disable (0) radio */
	{ "wl_channel", "6", 0 },		/* Channel number */
	{ "wl_closed", "0", 0 },		/* Closed (hidden) network */
	{ "wl_wep", "disabled", 0 },		/* WEP data encryption (enabled|disabled) */
	{ "wl_ssid", "dlink", 0 },		/* Service set ID (network name) */
	{ "wl_auth", "0", 0 },			/* Shared key authentication optional (0) or required (1) */
	{ "wl_key", "1", 0 },			/* Current WEP key */
	{ "wl_key1", "", 0 },			/* 5/13 char ASCII or 10/26 char hex */
	{ "wl_key2", "", 0 },			/* 5/13 char ASCII or 10/26 char hex */
	{ "wl_key3", "", 0 },			/* 5/13 char ASCII or 10/26 char hex */
	{ "wl_key4", "", 0 },			/* 5/13 char ASCII or 10/26 char hex */
	{ "wl_maclist", "", 0 },		/* xx:xx:xx:xx:xx:xx ... */
	{ "wl_macmode", "disabled", 0 },	/* "allow" only, "deny" only, or "disabled" (allow all) */
	{ "wlan0_auto_channel_enable", "1", 1 },	/* Auto Channel setting */
	{ "wl_reg_mode", "off", 0 },		/* Regulatory: 802.11H(h)/802.11D(d)/off(off) */
	{ "wl_dfs_preism", "60", 0 },		/* 802.11H pre network CAC time */
	{ "wl_dfs_postism", "60", 0 },		/* 802.11H In Service Monitoring CAC time */
	{ "wl_rate", "0", 0 },			/* Rate (bps, 0 for auto) */
	{ "wl_mrate", "0", 0 },			/* Mcast Rate (bps, 0 for auto) */
	{ "wl_rateset", "default", 0 },		/* "default" or "all" or "12" */
	{ "wl_frag", "2346", 0 },		/* Fragmentation threshold */
	{ "wl_rts", "2346", 0 },		/* RTS threshold */
	{ "wl_dtim", "1", 0 },			/* DTIM period */
	{ "wl_bcn", "100", 0 },			/* Beacon interval */
	{ "wl_plcphdr", "long", 0 },		/* 802.11b PLCP preamble type */
	{ "wl_gmode_protection", "auto", 0 },	/* 802.11g RTS/CTS protection (off|auto) */
	{ "wl_afterburner", "off", 0 },		/* AfterBurner */
	{ "wl_frameburst", "off", 0 },		/* BRCM Frambursting mode (off|on) */
	{ "wl_wme", "off", 0 },			/* WME mode (off|on) */
	{ "wl_antdiv", "-1", 0 },		/* Antenna Diversity (-1|0|1|3) */
	{ "wl_infra", "1", 0 },			/* Network Type (BSS/IBSS) */
	{ "wep_key_len", "5", 1},		/* WEP key length with 5 or 13 ___by Tony */
	{ "wlan0_wep_display", "hex", 1},	/* WEP key type with ASCII or HEX ___by Tony */

	{ "wl0_radio", "1", 1 },			/* Enable (1) or disable (0) radio */
	{ "wl0_channel", "6", 1 },		/* Channel number */
	{ "wl0_user_channel", "6", 1 },		/* user define Channel number */
	{ "wl0_closed", "0", 1 },		/* Closed (hidden) network */
	{ "wl0_wep", "disabled", 1 },		/* WEP data encryption (enabled|disabled) */
	{ "wl0_ssid", "dlink", 1 },		/* Service set ID (network name) */
	{ "wl0_auth", "0", 1 },			/* Shared key authentication optional (0) or required (1) */
	{ "wl0_key", "1", 1 },			/* Current WEP key */
	{ "wl0_key1", "", 1 },			/* 5/13 char ASCII or 10/26 char hex */
	{ "wl0_key2", "", 1 },			/* 5/13 char ASCII or 10/26 char hex */
	{ "wl0_key3", "", 1 },			/* 5/13 char ASCII or 10/26 char hex */
	{ "wl0_key4", "", 1 },			/* 5/13 char ASCII or 10/26 char hex */
	{ "wl0_rate", "0", 1 },			/* Rate (bps, 0 for auto) */
	{ "wl0_mrate", "0", 1 },			/* Mcast Rate (bps, 0 for auto) */
	{ "wl0_frag", "2346", 1 },		/* Fragmentation threshold */
	{ "wl0_rts", "2346", 1 },		/* RTS threshold */
	{ "wl0_dtim", "1", 1 },			/* DTIM period */
	{ "wl0_bcn", "100", 1 },			/* Beacon interval */
	{ "wl0_plcphdr", "long", 1 },		/* 802.11b PLCP preamble type */
	{ "wl0_gmode_protection", "auto", 1 },	/* 802.11g RTS/CTS protection (off|auto) */
	{ "wl0_wme", "off", 1 },		/* WME mode (off|on) */
	{ "wl_wme", "off", 1 },			/* WME mode (off|on) */
	{ "wl0_auth_mode", "none", 1 },		/* Network authentication mode (radius|none) */
	{ "wl0_wpa_psk", "", 1 },		/* WPA pre-shared key */
	{ "wl0_wpa_gtk_rekey", "0", 1 },		/* GTK rotation interval */
	{ "wl0_radius_ipaddr", "", 1 },		/* RADIUS server IP address */
	{ "wl0_radius_key", "", 1 },		/* RADIUS shared secret */
	{ "wl0_radius_port", "1812", 1 },	/* RADIUS server UDP port */
	{ "wl0_crypto", "tkip", 1 },		/* WPA data encryption */
	{ "wl0_net_reauth", "36000", 1 },	/* Network Re-auth/PMK caching duration */
	{ "wl0_akm", "", 1 },			/* WPA akm list */

	/* WPA parameters */
	{ "wl_auth_mode", "none", 0 },		/* Network authentication mode (radius|none) */
	{ "wl_wpa_psk", "", 0 },		/* WPA pre-shared key */
	{ "wl_wpa_gtk_rekey", "0", 0 },		/* GTK rotation interval */
	{ "wl_radius_ipaddr", "", 0 },		/* RADIUS server IP address */
	{ "wl_radius_key", "", 0 },		/* RADIUS shared secret */
	{ "wl_radius_port", "1812", 0 },	/* RADIUS server UDP port */
	{ "wl_crypto", "tkip", 0 },		/* WPA data encryption */
	{ "wl_net_reauth", "36000", 0 },	/* Network Re-auth/PMK caching duration */
	{ "wl_akm", "", 0 },			/* WPA akm list */

 	/* SES parameters */
 	{ "ses_enable", "1", 0 },		/* enable ses */
 	{ "ses_event", "2", 0 },		/* initial ses event */
 	{ "ses_wds_mode", "1", 0 },		/* enabled if ses cfgd */
 
 	/* SES client parameters */
 	{ "ses_cl_enable", "1", 0 },		/* enable ses */
 	{ "ses_cl_event", "0", 0 },		/* initial ses event */
 
 	/* WSC parameters */
 	{ "wl_wsc_mode", "enabled", 1 },		/* disable wsc */
 	{ "wl0_wsc_mode", "enabled", 1 },		/* disable wsc */
 	{ "wsc_mode", "enabled", 1 },		/* disable wsc */
 	{ "wsc_config_command", "0", 1 },		/* config state unconfiged */
 	{ "wsc_config_state", "0", 1 },		/* config state unconfiged */
 	{ "wsc_method", "1", 1 },		/* config state unconfiged */
 	{ "wsc_device_pin", "12345670", 1 },
 	{ "wsc_modelname", "Broadcom", 1 },
 	{ "wsc_mfstring", "Broadcom", 1 },
 	{ "wsc_device_name", "D-Link DIR-330", 1 },
 	{ "wl_wsc_reg", "disabled", 1 },
 	{ "wsc_sta_pin", "0", 1 },
 	{ "wsc_modelnum", "123456", 1 },
 	{ "wsc_timeout_enable", "1", 1 },
 	{ "wsc_addER", "0", 1 },
 	{ "wsc_uuid", "0x000102030405060708090A0B0C0D0EBB", 1 },
	{ "wps_gpio", "7", 1},
	{ "wps_led_gpio", "6", 1},
	{ "wps_pre_restart", "0", 1},
	
 	{ "wl_wsc_mode", "enabled", 0 },		/* disable wsc */
 	{ "wl0_wsc_mode", "enabled", 0 },		/* disable wsc */
 	{ "wsc_mode", "enabled", 0 },		/* disable wsc */
 	{ "wsc_config_command", "0", 0 },		/* config state unconfiged */
 	{ "wsc_config_state", "0", 0 },		/* config state unconfiged */
 	{ "wsc_method", "1", 0 },		/* config state unconfiged */
 	{ "wsc_device_pin", "12345670", 0 },
 	{ "wsc_modelname", "Broadcom", 0 },
 	{ "wsc_mfstring", "Broadcom", 0 },
 	{ "wsc_device_name", "D-Link DIR-330", 0 },
 	{ "wl_wsc_reg", "disabled", 0 },
 	{ "wsc_sta_pin", "0", 0 },
 	{ "wsc_modelnum", "123456", 0 },
 	{ "wsc_timeout_enable", "1", 0 },
 	{ "wsc_addER", "0", 0 },
 	{ "wsc_uuid", "0x000102030405060708090A0B0C0D0EBB", 0 },
	{ "wps_gpio", "7", 0},
	{ "wps_led_gpio", "6", 0},
	{ "wps_pre_restart", "0", 0},


	/* WME parameters (cwmin cwmax aifsn txop_b txop_ag adm_control oldest_first) */
	/* EDCA parameters for STA */
	{ "wl_wme_sta_be", "15 1023 3 0 0 off off", 0 },	/* WME STA AC_BE parameters */
	{ "wl_wme_sta_bk", "15 1023 7 0 0 off off", 0 },	/* WME STA AC_BK parameters */
	{ "wl_wme_sta_vi", "7 15 2 6016 3008 off off", 0 },	/* WME STA AC_VI parameters */
	{ "wl_wme_sta_vo", "3 7 2 3264 1504 off off", 0 },	/* WME STA AC_VO parameters */

	/* EDCA parameters for AP */
	{ "wl_wme_ap_be", "15 63 3 0 0 off off", 0 },		/* WME AP AC_BE parameters */
	{ "wl_wme_ap_bk", "15 1023 7 0 0 off off", 0 },		/* WME AP AC_BK parameters */
	{ "wl_wme_ap_vi", "7 15 1 6016 3008 off off", 0 },	/* WME AP AC_VI parameters */
	{ "wl_wme_ap_vo", "3 7 1 3264 1504 off off", 0 },	/* WME AP AC_VO parameters */

	{ "wl_wme_no_ack", "off", 0},		/* WME No-Acknowledgment mode */
	{ "wl_wme_apsd", "on", 0},		/* WME APSD mode */

	{ "wl_maxassoc", "128", 0},		/* Max associations driver could support */

	{ "wl_unit", "0", 0 },			/* Last configured interface */
	{ "wl_sta_retry_time", "5", 0 }, /* Seconds between association attempts */

	/* Schedule rule */
	{ "schedule_rule0", "", 1},
	{ "schedule_rule1", "", 1},
	{ "schedule_rule2", "", 1},
	{ "schedule_rule3", "", 1},
	{ "schedule_rule4", "", 1},
	{ "schedule_rule5", "", 1},
	{ "schedule_rule6", "", 1},
	{ "schedule_rule7", "", 1},
	{ "schedule_rule8", "", 1},
	{ "schedule_rule9", "", 1},
	{ "schedule_rule10", "", 1},
	{ "schedule_rule11", "", 1},
	{ "schedule_rule12", "", 1},
	{ "schedule_rule13", "", 1},
	{ "schedule_rule14", "", 1},
	{ "schedule_rule15", "", 1},
	{ "schedule_rule16", "", 1},
	{ "schedule_rule17", "", 1},
	{ "schedule_rule18", "", 1},
	{ "schedule_rule19", "", 1},
	{ "schedule_rule20", "", 1},
	{ "schedule_rule21", "", 1},
	{ "schedule_rule22", "", 1},
	{ "schedule_rule23", "", 1},
	{ "schedule_rule24", "", 1},
	/* Restore defaults */
	{ "restore_defaults", "0", 0 },		/* Set to 0 to not restore defaults on boot */
	/* ntp and system time*/		
	{ "timer_interval", "3600", 1 },	/* Timer interval in seconds default:1 hours*/
	{ "ntp_server", "ntp1.dlink.com", 1 },		/* NTP server */	
	{ "time_zone", "GMT+8", 1 },		/* Time zone (GNU TZ format) */
	{ "sel_time_zone", "-128", 1},
	{ "ntp_client_enable", "0", 1},
	{ "daylight_en", "0", 1},
	{ "daylight_smonth", "1", 1},
	{ "daylight_sweek", "1", 1},
	{ "daylight_sday", "1", 1},
	{ "daylight_stime", "1", 1},
	{ "daylight_emonth", "1", 1},
	{ "daylight_eweek", "1", 1},
	{ "daylight_eday", "1", 1},
	{ "daylight_etime", "1", 1},
	{ "system_time", "2002/01/01/00/00/00", 1},
	/* url_domain_filter */
	{ "url_domain_filter_type", "0", 1},	/*  */
	{ "url_domain_filter_0", "", 1},	/*  */
	{ "url_domain_filter_1", "", 1},
	{ "url_domain_filter_2", "", 1},
	{ "url_domain_filter_3", "", 1},
	{ "url_domain_filter_4", "", 1},
	{ "url_domain_filter_5", "", 1},
	{ "url_domain_filter_6", "", 1},
	{ "url_domain_filter_7", "", 1},
	{ "url_domain_filter_8", "", 1},
	{ "url_domain_filter_9", "", 1},
	{ "url_domain_filter_10", "", 1},
	{ "url_domain_filter_11", "", 1},
	{ "url_domain_filter_12", "", 1},
	{ "url_domain_filter_13", "", 1},
	{ "url_domain_filter_14", "", 1},
	{ "url_domain_filter_15", "", 1},
	{ "url_domain_filter_16", "", 1},
	{ "url_domain_filter_17", "", 1},
	{ "url_domain_filter_18", "", 1},
	{ "url_domain_filter_19", "", 1}, 
	{ "url_domain_filter_20", "", 1},	/* Port Forwarding*/
	{ "url_domain_filter_21", "", 1},
	{ "url_domain_filter_22", "", 1},
	{ "url_domain_filter_23", "", 1},
	{ "url_domain_filter_24", "", 1},
	{ "url_domain_filter_25", "", 1},
	{ "url_domain_filter_26", "", 1},
	{ "url_domain_filter_27", "", 1},
	{ "url_domain_filter_28", "", 1},
	{ "url_domain_filter_29", "", 1},
	{ "url_domain_filter_30", "", 1},
	{ "url_domain_filter_31", "", 1},
	{ "url_domain_filter_32", "", 1},
	{ "url_domain_filter_33", "", 1},
	{ "url_domain_filter_34", "", 1},
	{ "url_domain_filter_35", "", 1},
	{ "url_domain_filter_36", "", 1},
	{ "url_domain_filter_37", "", 1},
	{ "url_domain_filter_38", "", 1},
	{ "url_domain_filter_39", "", 1},
	{ "url_domain_filter_40", "", 1},
	{ "url_domain_filter_41", "", 1},
	{ "url_domain_filter_42", "", 1},
	{ "url_domain_filter_43", "", 1},
	{ "url_domain_filter_44", "", 1},
	{ "url_domain_filter_45", "", 1},
	{ "url_domain_filter_46", "", 1},
	{ "url_domain_filter_47", "", 1},
	{ "url_domain_filter_48", "", 1},
	{ "url_domain_filter_49", "", 1},
	{ "url_domain_filter_schedule_0", "", 1},	/*  */
	{ "url_domain_filter_schedule_1", "", 1},
	{ "url_domain_filter_schedule_2", "", 1},
	{ "url_domain_filter_schedule_3", "", 1},
	{ "url_domain_filter_schedule_4", "", 1},
	{ "url_domain_filter_schedule_5", "", 1},
	{ "url_domain_filter_schedule_6", "", 1},
	{ "url_domain_filter_schedule_7", "", 1},
	{ "url_domain_filter_schedule_8", "", 1},
	{ "url_domain_filter_schedule_9", "", 1},
	{ "url_domain_filter_schedule_10", "", 1},
	{ "url_domain_filter_schedule_11", "", 1},
	{ "url_domain_filter_schedule_12", "", 1},
	{ "url_domain_filter_schedule_13", "", 1},
	{ "url_domain_filter_schedule_14", "", 1},
	{ "url_domain_filter_schedule_15", "", 1},
	{ "url_domain_filter_schedule_16", "", 1},
	{ "url_domain_filter_schedule_17", "", 1},
	{ "url_domain_filter_schedule_18", "", 1},
	{ "url_domain_filter_schedule_19", "", 1}, 
	{ "url_domain_filter_schedule_20", "", 1},	/* Port Forwarding*/
	{ "url_domain_filter_schedule_21", "", 1},
	{ "url_domain_filter_schedule_22", "", 1},
	{ "url_domain_filter_schedule_23", "", 1},
	{ "url_domain_filter_schedule_24", "", 1},
	{ "url_domain_filter_schedule_25", "", 1},
	{ "url_domain_filter_schedule_26", "", 1},
	{ "url_domain_filter_schedule_27", "", 1},
	{ "url_domain_filter_schedule_28", "", 1},
	{ "url_domain_filter_schedule_29", "", 1},
	{ "url_domain_filter_schedule_30", "", 1},
	{ "url_domain_filter_schedule_31", "", 1},
	{ "url_domain_filter_schedule_32", "", 1},
	{ "url_domain_filter_schedule_33", "", 1},
	{ "url_domain_filter_schedule_34", "", 1},
	{ "url_domain_filter_schedule_35", "", 1},
	{ "url_domain_filter_schedule_36", "", 1},
	{ "url_domain_filter_schedule_37", "", 1},
	{ "url_domain_filter_schedule_38", "", 1},
	{ "url_domain_filter_schedule_39", "", 1},
	{ "url_domain_filter_schedule_40", "", 1},
	{ "url_domain_filter_schedule_41", "", 1},
	{ "url_domain_filter_schedule_42", "", 1},
	{ "url_domain_filter_schedule_43", "", 1},
	{ "url_domain_filter_schedule_44", "", 1},
	{ "url_domain_filter_schedule_45", "", 1},
	{ "url_domain_filter_schedule_46", "", 1},
	{ "url_domain_filter_schedule_47", "", 1},
	{ "url_domain_filter_schedule_48", "", 1},
	{ "url_domain_filter_schedule_49", "", 1},
	/* vlan parameters */
	{ "vlan_enable", "0", 1},
	{ "vlan2ports", "", 1},
	{ "vlan3ports", "", 1},
	{ "vlan4ports", "", 1},
	{ "vlan5ports", "", 1},
	{ "vlan6ports", "", 1},
	{ "vlan7ports", "", 1},
	{ "vlan8ports", "", 1},
	{ "vlan9ports", "", 1},
	{ "vlan10ports", "", 1},
	{ "vlan11ports", "", 1},
	{ "vlan12ports", "", 1},
	{ "vlan13ports", "", 1},
	{ "vlan14ports", "", 1},
	{ "vlan15ports", "", 1},
	{ "vlanpriority0", "3", 1},
	{ "vlanpriority1", "3", 1},
	{ "vlanpriority2", "2", 1},
	{ "vlanpriority3", "2", 1},
	{ "vlanpriority4", "1", 1},
	{ "vlanpriority5", "1", 1},
	{ "vlanpriority6", "0", 1},
	{ "vlanpriority7", "0", 1},
	{ "lan_tx", "0", 0},
	{ "lan_rx", "0", 0},
	{ "wan_tx", "0", 0},
	{ "wan_rx", "0", 0},
	{ "wl_tx", "0", 0},
	{ "wl_rx", "0", 0},
	/* Bandwidth Control parameters */
	{ "bw_enable", "0", 1},
	{ "bw_uplink", "512", 1},
	{ "bw_downlink", "512", 1},
	{ "bw_ctrl_0", "", 1},
	{ "bw_ctrl_1", "", 1},
	{ "bw_ctrl_2", "", 1},
	{ "bw_ctrl_3", "", 1},
	{ "bw_ctrl_4", "", 1},
	{ "bw_ctrl_5", "", 1},
	{ "bw_ctrl_6", "", 1},
	{ "bw_ctrl_7", "", 1},
	{ "bw_ctrl_8", "", 1},
	{ "bw_ctrl_9", "", 1},
	{ "bw_ctrl_10", "", 1},
	{ "bw_ctrl_11", "", 1},
	{ "bw_ctrl_12", "", 1},
	{ "bw_ctrl_13", "", 1},
	{ "bw_ctrl_14", "", 1},
	{ "bw_ctrl_15", "", 1},
	{ "bw_ctrl_16", "", 1},
	{ "bw_ctrl_17", "", 1},
	{ "bw_ctrl_18", "", 1},
	{ "bw_ctrl_19", "", 1},
	{ "bw_ctrl_20", "", 1},
	{ "bw_ctrl_21", "", 1},
	{ "bw_ctrl_22", "", 1},
	{ "bw_ctrl_23", "", 1},
	{ "bw_ctrl_24", "", 1},
	/* PPTP client parameters */
	{ "wan_pptp_dynamic", "1", 0},
	{ "wan_pptp_server_ipaddr", "", 0},
	{ "wan_pptp_username", "", 0},
	{ "wan_pptp_passwd", "", 0},
	{ "wan_pptp_idletime", "300", 0},
	{ "wan_pptp_mtu", "1450", 0},
	{ "wan_pptp_demand", "2", 0},
	{ "wan0_pptp_dynamic", "1", 1},
	{ "wan0_pptp_server_ipaddr", "", 1},
	{ "wan0_pptp_username", "", 1},
	{ "wan0_pptp_passwd", "", 1},
	{ "wan0_pptp_idletime", "300", 1},
	{ "wan0_pptp_mtu", "1450", 1},
	{ "wan0_pptp_demand", "2", 1},
	{ "wan0_pptp_encrypt", "nomppe", 1},
	/* Russia PPTP client parameters */
	{ "wan_rupptp_dynamic", "1", 0},
	{ "wan_rupptp_server_ipaddr", "", 0},
	{ "wan_rupptp_username", "", 0},
	{ "wan_rupptp_passwd", "", 0},
	{ "wan_rupptp_idletime", "300", 0},
	{ "wan_rupptp_mtu", "1450", 0},
	{ "wan_rupptp_demand", "2", 0},
	{ "wan0_rupptp_dynamic", "1", 1},
	{ "wan0_rupptp_server_ipaddr", "", 1},
	{ "wan0_rupptp_username", "", 1},
	{ "wan0_rupptp_passwd", "", 1},
	{ "wan0_rupptp_idletime", "300", 1},
	{ "wan0_rupptp_mtu", "1450", 1},
	{ "wan0_rupptp_demand", "2", 1},
	{ "wan0_rupptp_encrypt", "nomppe", 1},
	/* L2TP client parameters */
	{ "wan_l2tp_dynamic", "1", 0},
	{ "wan_l2tp_server_ipaddr", "", 0},
	{ "wan_l2tp_username", "", 0},
	{ "wan_l2tp_passwd", "", 0},
	{ "wan_l2tp_idletime", "300", 0},
	{ "wan_l2tp_mtu", "1450", 0},
	{ "wan_l2tp_demand", "2", 0},
	{ "wan0_l2tp_dynamic", "1", 1},
	{ "wan0_l2tp_server_ipaddr", "", 1},
	{ "wan0_l2tp_username", "", 1},
	{ "wan0_l2tp_passwd", "", 1},
	{ "wan0_l2tp_idletime", "300", 1},
	{ "wan0_l2tp_mtu", "1450", 1},
	{ "wan0_l2tp_demand", "2", 1},
	/* BigPond parameters */
	/*
	{ "wan_bigpond_username", "", 0},
	{ "wan_bigpond_passwd", "", 0},
	{ "wan_bigpond_auth", "sm-server", 0},
	{ "wan_bigpond_server", "", 0},
	*/
	{ "wan0_bigpond_username", "", 1},
	{ "wan0_bigpond_passwd", "", 1},
	{ "wan0_bigpond_auth", "sm-server", 1},
	{ "wan0_bigpond_server", "", 1},
	{ "wan0_bigpond_dynamic", "1" , 1},
	/* L2TPD parameters */
	{"l2tpd_enable", "0", 1},
	{"l2tpd_servername", "", 1},
	{"l2tpd_local_ip", "", 1},
	{"l2tpd_remote_ip", "", 1},
	{"l2tpd_auth_proto", "pap", 1},
	{"l2tpd_auth_source", "local", 1},
	{"l2tpd_authmethod", "LDAP", 1},
	{"l2tpd_conn_1", "", 1},
	{"l2tpd_conn_2", "", 1},
	{"l2tpd_conn_3", "", 1},
	{"l2tpd_conn_4", "", 1},
	{"l2tpd_conn_5", "", 1},
	{"l2tpd_conn_6", "", 1},
	{"l2tpd_conn_7", "", 1},
	{"l2tpd_conn_8", "", 1},
	{"l2tpd_conn_9", "", 1},
	{"l2tpd_conn_10", "", 1},
	{"l2tpd_conn_11", "", 1},
	{"l2tpd_conn_12", "", 1},
	{"l2tpd_conn_13", "", 1},
	{"l2tpd_conn_14", "", 1},
	{"l2tpd_conn_15", "", 1},
	{"l2tpd_conn_16", "", 1},
	{"l2tpd_conn_17", "", 1},
	{"l2tpd_conn_18", "", 1},
	{"l2tpd_conn_19", "", 1},
	{"l2tpd_conn_20", "", 1},
	{"l2tpd_conn_21", "", 1},
	{"l2tpd_conn_22", "", 1},
	{"l2tpd_conn_23", "", 1},
	{"l2tpd_conn_24", "", 1},
	{"l2tpd_conn_25", "", 1},
	{"l2tpd_overipsec_enable", "0", 1},
	/* PPTPD parameters */
	{"pptpd_enable", "0", 1},
	{"pptpd_servername", "", 1},
	{"pptpd_local_ip", "", 1},
	{"pptpd_remote_ip", "", 1},
	{"pptpd_auth_proto", "pap", 1},
	{"pptpd_encryption", "mppe-40", 1},
	{"pptpd_auth_source", "local", 1},
	{"pptpd_authmethod", "DB", 1},
	{"pptpd_conn_1", "", 1},
	{"pptpd_conn_2", "", 1},
	{"pptpd_conn_3", "", 1},
	{"pptpd_conn_4", "", 1},
	{"pptpd_conn_5", "", 1},
	{"pptpd_conn_6", "", 1},
	{"pptpd_conn_7", "", 1},
	{"pptpd_conn_8", "", 1},
	{"pptpd_conn_9", "", 1},
	{"pptpd_conn_10", "", 1},
	{"pptpd_conn_11", "", 1},
	{"pptpd_conn_12", "", 1},
	{"pptpd_conn_13", "", 1},
	{"pptpd_conn_14", "", 1},
	{"pptpd_conn_15", "", 1},
	{"pptpd_conn_16", "", 1},
	{"pptpd_conn_17", "", 1},
	{"pptpd_conn_18", "", 1},
	{"pptpd_conn_19", "", 1},
	{"pptpd_conn_20", "", 1},
	{"pptpd_conn_21", "", 1},
	{"pptpd_conn_22", "", 1},
	{"pptpd_conn_23", "", 1},
	{"pptpd_conn_24", "", 1},
	{"pptpd_conn_25", "", 1},
	/* PPTPD and L2TPD links */
	{"vpn_link", "", 0},
	/* VPN keys*/
	{"edit_vpn", "-1", 1},
	{"edit_type", "-1", 1},
	/* log status */
    { "current_page", "1", 0},
    /* log email setting */
    { "log_email_recipient", "", 1},	
	{ "log_email_sender", "", 1},
	{ "log_email_server", "", 1},
	/* esmtp authentication */
	{ "auth_active", "0", 1},
	{ "auth_acname", "", 1},
	{ "auth_passwd", "", 1},	
	/* log type setting */
	{ "log_system_activity", "1", 1},
	{ "log_debug_information", "0", 1},
	{ "log_attacks", "1", 1},
	{ "log_dropped_packets", "0", 1},
	{ "log_notice", "1", 1},
	/* log server setting */	
	{ "log_server", "0" ,1},		/* ebanle/disable log to remote server */
	{ "log_ipaddr_1", "", 1 },		/* syslog recipient 1 */
	{ "log_ipaddr_2", "", 1 },		/* syslog recipient 2 */
	{ "log_ipaddr_3", "", 1 },		/* syslog recipient 3 */
	{ "log_ipaddr_4", "", 1 },		/* syslog recipient 4 */
	{ "log_ipaddr_5", "", 1 },		/* syslog recipient 5 */
	{ "log_ipaddr_6", "", 1 },		/* syslog recipient 6 */
	{ "log_ipaddr_7", "", 1 },		/* syslog recipient 7 */
	{ "log_ipaddr_8", "", 1 },		/* syslog recipient 8 */
	/* DDNS keys*/
	{"ddns_enable", "0", 1},
	{"ddns_type", "Dynamic@dlinkddns.com", 1},	
	{"ddns_hostname", "", 1},
	{"ddns_username", "", 1},
	{"ddns_password", "", 1},
	{"ddns_timeout", "864000", 1},
	{"ddns_status", "Disconnect", 1},
	/* authentication */
	{"auth_enable", "0", 1},
	{"auth_iprange", "", 1},
	{"auth_group1", "Group1/", 1},
	{"auth_group2", "Group2/", 1},
	{"auth_group3", "Group3/", 1},
	{"auth_group4", "Group4/", 1},
	{"auth_group5", "Group5/", 1},
	{"auth_group6", "Group6/", 1},
	{"auth_group7", "Group7/", 1},
	{"auth_group8", "Group8/", 1},
	{"auth_xauth", "", 1},		
	{"auth_pptpd_l2tpd", "", 1},	/* pptpd user */
	{"auth_user_auth", "", 1},	/* lan to wan auth */
	{"auth_status", "", 1},		/* lan to wan auth */
	/* end authentication */
	{"x509_1", "", 1 },
	{"x509_2", "", 1 },
	{"x509_3", "", 1 },
	{"x509_4", "", 1 },
	{"x509_5", "", 1 },
	{"x509_6", "", 1 },
	{"x509_7", "", 1 },
	{"x509_8", "", 1 },
	{"x509_9", "", 1 },
	{"x509_10", "", 1 },
	{"x509_11", "", 1 },
	{"x509_12", "", 1 },
	{"x509_13", "", 1 },
	{"x509_14", "", 1 },
	{"x509_15", "", 1 },
	{"x509_16", "", 1 },
	{"x509_17", "", 1 },
	{"x509_18", "", 1 },
	{"x509_19", "", 1 },
	{"x509_20", "", 1 },
	{"x509_21", "", 1 },
	{"x509_22", "", 1 },
	{"x509_23", "", 1 },
	{"x509_24", "", 1 },
	{"x509_ca_1", "", 1 },
	{"x509_ca_2", "", 1 },
	{"x509_ca_3", "", 1 },
	{"x509_ca_4", "", 1 },
	{"x509_ca_5", "", 1 },
	{"x509_ca_6", "", 1 },
	{"x509_ca_7", "", 1 },
	{"x509_ca_8", "", 1 },
	{"x509_ca_9", "", 1 },
	{"x509_ca_10", "", 1 },
	{"x509_ca_11", "", 1 },
	{"x509_ca_12", "", 1 },
	{"x509_ca_13", "", 1 },
	{"x509_ca_14", "", 1 },
	{"x509_ca_15", "", 1 },
	{"x509_ca_16", "", 1 },
	{"x509_ca_17", "", 1 },
	{"x509_ca_18", "", 1 },
	{"x509_ca_19", "", 1 },
	{"x509_ca_20", "", 1 },
	{"x509_ca_21", "", 1 },
	{"x509_ca_22", "", 1 },
	{"x509_ca_23", "", 1 },
	{"x509_ca_24", "", 1 },
	//route
	{"static_route", "", 1},
	{"russia_route", "", 1},
	{"rip_enable", "0", 1},	
	{"rip_tx_version", "", 1},
	{"rip_rx_version", "", 1},
	{"ospf_enable", "0", 1},	
	{"ospf_if_area0", "0", 1},
	{"ospf_if_area1", "0", 1},
	{"ospf_if_area2", "0", 1},
	{"ospf_if_cost0", "1", 1},	
	{"ospf_if_cost1", "10", 1},	
	{"ospf_if_cost2", "1", 1},	
	{"ospf_if_priority0", "1", 1},
	{"ospf_if_priority1", "1", 1},
	{"ospf_if_priority2", "1", 1},		
	// firewall rule 0~74
	{"firewall_rule0", "", 1},
	{"firewall_rule1", "", 1},
	{"firewall_rule2", "", 1},
	{"firewall_rule3", "", 1},
	{"firewall_rule4", "", 1},
	{"firewall_rule5", "", 1},
	{"firewall_rule6", "", 1},
	{"firewall_rule7", "", 1},
	{"firewall_rule8", "", 1},
	{"firewall_rule9", "", 1},
	{"firewall_rule10", "", 1},
	{"firewall_rule11", "", 1},
	{"firewall_rule12", "", 1},
	{"firewall_rule13", "", 1},
	{"firewall_rule14", "", 1},
	{"firewall_rule15", "", 1},
	{"firewall_rule16", "", 1},
	{"firewall_rule17", "", 1},
	{"firewall_rule18", "", 1},
	{"firewall_rule19", "", 1},
	{"firewall_rule20", "", 1},
	{"firewall_rule21", "", 1},
	{"firewall_rule22", "", 1},
	{"firewall_rule23", "", 1},
	{"firewall_rule24", "", 1},
	{"firewall_rule25", "", 1},
	{"firewall_rule26", "", 1},
	{"firewall_rule27", "", 1},
	{"firewall_rule28", "", 1},
	{"firewall_rule29", "", 1},
	{"firewall_rule30", "", 1},
	{"firewall_rule31", "", 1},
	{"firewall_rule32", "", 1},
	{"firewall_rule33", "", 1},
	{"firewall_rule34", "", 1},
	{"firewall_rule35", "", 1},
	{"firewall_rule36", "", 1},
	{"firewall_rule37", "", 1},
	{"firewall_rule38", "", 1},
	{"firewall_rule39", "", 1},
	{"firewall_rule40", "", 1},
	{"firewall_rule41", "", 1},
	{"firewall_rule42", "", 1},
	{"firewall_rule43", "", 1},
	{"firewall_rule44", "", 1},
	{"firewall_rule45", "", 1},
	{"firewall_rule46", "", 1},
	{"firewall_rule47", "", 1},
	{"firewall_rule48", "", 1},
	{"firewall_rule49", "", 1},
	{"firewall_rule50", "", 1},
	{"firewall_rule51", "", 1},
	{"firewall_rule52", "", 1},
	{"firewall_rule53", "", 1},
	{"firewall_rule54", "", 1},
	{"firewall_rule55", "", 1},
	{"firewall_rule56", "", 1},
	{"firewall_rule57", "", 1},
	{"firewall_rule58", "", 1},
	{"firewall_rule59", "", 1},
	{"firewall_rule60", "", 1},
	{"firewall_rule61", "", 1},
	{"firewall_rule62", "", 1},
	{"firewall_rule63", "", 1},
	{"firewall_rule64", "", 1},
	{"firewall_rule65", "", 1},
	{"firewall_rule66", "", 1},
	{"firewall_rule67", "", 1},
	{"firewall_rule68", "", 1},
	{"firewall_rule69", "", 1},
	{"firewall_rule70", "", 1},
	{"firewall_rule71", "", 1},
	{"firewall_rule72", "", 1},
	{"firewall_rule73", "", 1},
	{"firewall_rule74", "", 1},
	// mac filter 0~99
	{"mac_filter0", "", 1},
	{"mac_filter1", "", 1},
	{"mac_filter2", "", 1},
	{"mac_filter3", "", 1},
	{"mac_filter4", "", 1},
	{"mac_filter5", "", 1},
	{"mac_filter6", "", 1},
	{"mac_filter7", "", 1},
	{"mac_filter8", "", 1},
	{"mac_filter9", "", 1},
	{"mac_filter10", "", 1},
	{"mac_filter11", "", 1},
	{"mac_filter12", "", 1},
	{"mac_filter13", "", 1},
	{"mac_filter14", "", 1},
	{"mac_filter15", "", 1},
	{"mac_filter16", "", 1},
	{"mac_filter17", "", 1},
	{"mac_filter18", "", 1},
	{"mac_filter19", "", 1},
	{"mac_filter20", "", 1},
	{"mac_filter21", "", 1},
	{"mac_filter22", "", 1},
	{"mac_filter23", "", 1},
	{"mac_filter24", "", 1},
	{"mac_filter25", "", 1},
	{"mac_filter26", "", 1},
	{"mac_filter27", "", 1},
	{"mac_filter28", "", 1},
	{"mac_filter29", "", 1},
	{"mac_filter30", "", 1},
	{"mac_filter31", "", 1},
	{"mac_filter32", "", 1},
	{"mac_filter33", "", 1},
	{"mac_filter34", "", 1},
	{"mac_filter35", "", 1},
	{"mac_filter36", "", 1},
	{"mac_filter37", "", 1},
	{"mac_filter38", "", 1},
	{"mac_filter39", "", 1},
	{"mac_filter40", "", 1},
	{"mac_filter41", "", 1},
	{"mac_filter42", "", 1},
	{"mac_filter43", "", 1},
	{"mac_filter44", "", 1},
	{"mac_filter45", "", 1},
	{"mac_filter46", "", 1},
	{"mac_filter47", "", 1},
	{"mac_filter48", "", 1},
	{"mac_filter49", "", 1},
	{"mac_filter50", "", 1},
	{"mac_filter51", "", 1},
	{"mac_filter52", "", 1},
	{"mac_filter53", "", 1},
	{"mac_filter54", "", 1},
	{"mac_filter55", "", 1},
	{"mac_filter56", "", 1},
	{"mac_filter57", "", 1},
	{"mac_filter58", "", 1},
	{"mac_filter59", "", 1},
	{"mac_filter60", "", 1},
	{"mac_filter61", "", 1},
	{"mac_filter62", "", 1},
	{"mac_filter63", "", 1},
	{"mac_filter64", "", 1},
	{"mac_filter65", "", 1},
	{"mac_filter66", "", 1},
	{"mac_filter67", "", 1},
	{"mac_filter68", "", 1},
	{"mac_filter69", "", 1},
	{"mac_filter70", "", 1},
	{"mac_filter71", "", 1},
	{"mac_filter72", "", 1},
	{"mac_filter73", "", 1},
	{"mac_filter74", "", 1},
	{"mac_filter75", "", 1},
	{"mac_filter76", "", 1},
	{"mac_filter77", "", 1},
	{"mac_filter78", "", 1},
	{"mac_filter79", "", 1},
	{"mac_filter80", "", 1},
	{"mac_filter81", "", 1},
	{"mac_filter82", "", 1},
	{"mac_filter83", "", 1},
	{"mac_filter84", "", 1},
	{"mac_filter85", "", 1},
	{"mac_filter86", "", 1},
	{"mac_filter87", "", 1},
	{"mac_filter88", "", 1},
	{"mac_filter89", "", 1},
	{"mac_filter90", "", 1},
	{"mac_filter91", "", 1},
	{"mac_filter92", "", 1},
	{"mac_filter93", "", 1},
	{"mac_filter94", "", 1},
	{"mac_filter95", "", 1},
	{"mac_filter96", "", 1},
	{"mac_filter97", "", 1},
	{"mac_filter98", "", 1},
	{"mac_filter99", "", 1},

	/* add for phase_1 1.5 version */
	{"username1", "", 1},
	{"passwd1", "", 1},
	{"username2", "", 1},
	{"passwd2", "", 1},
	{"username3", "", 1},
	{"passwd3", "", 1},
	{"tftpd_enable", "1", 1},
	/* add external user setting */
	{"enable_ext_server", "0", 1},
	{"ext_server_mode", "0", 1},
	{"radius0", "", 1},
	{"radius1", "", 1},
	{"enable_wpa_ent", "0", 1},
	{"radio_RADIUS", "", 1},
	{"ldap", "", 1},
	{"ad", "", 1},
	/* Qos Engine Setup */
	{"is_enable_qos", "1" , 1},
	{"qos_auto_classif", "1" , 1},
	{"qos_dynamic_frag", "1" , 1},
	/* Qos Rule */
	{"qos_rule1", "0", 1},
	{"qos_rule2", "0", 1},
	{"qos_rule3", "0", 1},
	{"qos_rule4", "0", 1},
	{"qos_rule5", "0", 1},
	{"qos_rule6", "0", 1},
	{"qos_rule7", "0", 1},
	{"qos_rule8", "0", 1},
	{"qos_rule9", "0", 1},
	{"qos_rule10", "0", 1},
	/* Qos Weight*/
	{"qos_weight_rule", "", 1},
	/* check list checksun */
	{"checksum", "", 1},
	/* usb setting start */
	{"usb_mode", "0", 1},
	{"usb_type_1", "1", 1},
	{"usb_type_2", "0", 1},
	{"usb_det_sec1", "", 1},
	{"usb_det_sec2", "", 1},
	/* usb setting end */
	{ "usb3g_username", "", 1},
        { "usb3g_password", "", 1},
        { "usb3g_auth", "", 1},
        { "usb3g_country", "", 1},
        { "usb3g_isp", "", 1},
        { "usb3g_dial_num", "", 1},
        { "usb3g_apn_name", "", 1},
        { "usb3g_reconnect_mode", "0", 1},
        { "usb3g_max_idle_time", "5", 1},
        { "usb3g_wan_mtu", "1492", 1},

	/* ipsec lock */
	{"ipsec_lock", "1", 1},
	{ 0, 0, 0 },
	{ NULL, NULL, NULL}
};
#endif
