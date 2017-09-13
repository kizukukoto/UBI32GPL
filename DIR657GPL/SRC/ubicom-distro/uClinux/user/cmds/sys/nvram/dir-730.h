#include "admin.def.h"
#include "dhcpd.def.h"
#include "dir330.def.h"
#include "interfaces.def.h"
#include "ips.def.h"
#include "iptables.def.h"
#include "log_smtp.def.h"
#include "misc.def.h"
#include "protocols.def.h"
#include "route.def.h"
#include "sys.def.h"
#include "time.def.h"
#include "vlan_bw.def.h"
#include "vpn.def.h"
#include "wireless.def.h"
#include "advances.def.h"
#include "statistics.def.h"

#define HW_WL				"2880"
#define HW_PLATFORM			"SL3516"
#define HW_WL_VDR			"RT"
#define HW_DATE				"080901"
#define HW_VENDER			"00"
#define HW_AREA				"WW"
#define HW_ID				HW_WL "-" HW_PLATFORM "-" HW_WL_VDR "-" HW_DATE "-" HW_VENDER "-" HW_AREA


#define KEY_NVRAM_RELEASE_TIME		"release_time"
#define VALUE_NVRAM_RELEASE_TIME	__DATE__ " " __TIME__
#define OS_VERSION			"1.00"
#define OS_BUILD_VER	OS_VERSION	"b24"

struct items_range nvram[] = {
	/* THIS IS SYSTEM ROOT CONFIGURE, MODIFIED CAREFULLY */
	{ "os_version", OS_VERSION, NV_RESET},
	{ "__os_version", OS_BUILD_VER, NV_RESET},
	{ "os_date", __DATE__, NV_RESET},
	{ "model_name", "DIR-730", NV_RESET},
	{ "model_number", "D-Link Router", NV_RESET},
	{ "model_area", HW_AREA, NV_RESET},
	
	{ "release_time", VALUE_NVRAM_RELEASE_TIME, NV_RESET},	
	{ "hostname", "D-Link DIR-730", 1},	//For log E-Mail's Subject
	{ "sys_cmds_debug", "/dev/null", NV_RESET},
	{ "restore_defaults", "0", NV_RESET},
	{ "time_zone", "GMT+8", NV_RESET},
	{ "ntp_server", "ntp1.dlink.com", NV_RESET},	//ntp server
	{ "timer_interval", "3600" , NV_RESET},	//ntp parameter 
	{ "upnp_enable", "1", NV_RESET}, //upnp
	{ "ipsec_lock", "1", NV_RESET},
	/* SBUTREE */
	{ "admin", NULL, 0, 0, 0, admin}, 
	{ "dhcpd", NULL, 0, 0, 0, dhcpd}, 
	{ "dir_330", NULL, 0, 0, 0, dir_330}, 
	{ "interfaces", NULL, 0, 0, 0, interfaces}, 
	{ "ips", NULL, 0, 0, 0, ips},
	{ "advanced", NULL, 0, 0, 0, inbound_filter},
	{ "iptables", NULL, 0, 0, 0, iptables}, 
	{ "log_smtp", NULL, 0, 0, 0, log_smtp}, 
	{ "misc", NULL, 0, 0, 0, misc}, 
	{ "protocols", NULL, 0, 0, 0, protocols}, 
	{ "route", NULL, 0, 0, 0, route}, 
	{ "time", NULL, 0, 0, 0, time}, 
	{ "vlan_bw_qos", NULL, 0, 0, 0, vlan_bw_qos}, 
	{ "vpn", NULL, 0, 0, 0, vpn}, 
	{ "wireless", NULL, 0, 0, 0, wireless},
	{ "statistics", NULL, 0, 0, 0, statistics},
	{ NULL, NULL}
};
