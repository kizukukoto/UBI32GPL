/* $Id: upnpglobalvars.c,v 1.18 2008/10/06 13:22:02 nanard Exp $ */
/* MiniUPnP project
 * http://miniupnp.free.fr/ or http://miniupnp.tuxfamily.org/
 * (c) 2006 Thomas Bernard 
 * This software is subject to the conditions detailed
 * in the LICENCE file provided within the distribution */

#include <sys/types.h>
#include <netinet/in.h>

#include "config.h"
#include "upnpglobalvars.h"

/* network interface for internet */
const char * ext_if_name = 0;

/* file to store leases */
#ifdef ENABLE_LEASEFILE
const char* lease_file = 0;
#endif

/* forced ip address to use for this interface
 * when NULL, getifaddr() is used */
const char * use_ext_ip_addr = 0;

const char * linklocal_ip_addr = 0;
int linklocal_index = 0;

/* LAN address */
/*const char * listen_addr = 0;*/

unsigned long downstream_bitrate = 0;
unsigned long upstream_bitrate = 0;

/* startup time */
time_t startup_time = 0;

#if 0
/* use system uptime */
int sysuptime = 0;

/* log packets flag */
int logpackets = 0;

#ifdef ENABLE_NATPMP
int enablenatpmp = 0;
#endif
#endif

int runtime_flags = 0;

// ** Following variables enables to store the UDA 1.1 header value
int upnp_bootid = 0;
int upnp_configid = 1337;
//int upnp_searchport = 0;
//int upnp_next_bootid = 0;
// **

const char * pidfilename = "/var/run/miniupnpd6.pid";

char uuidvalue[256] = "uuid:00000000-0000-0000-0000-000000000000";
char wandevuuid[256] = "uuid:8930ac12-ce99-4214-ab5f-67407262443r";
char wanconuuid[256] = "uuid:8930ac12-ce99-4214-ab5f-67407262443k";
char serialnumber[SERIALNUMBER_MAX_LEN] = "none";

char modelnumber[MODELNUMBER_MAX_LEN] = "1";

/* presentation url :
 * v4: http://nnn.nnn.nnn.nnn:ppppp/  => max 30 bytes including terminating 0
 * v6: http://[nnnn:nnnn:nnnn:nnnn:nnnn:nnnn:nnnn:nnnn]:ppppp/  => max 55 bytes including terminating 0 */
char presentationurl[PRESENTATIONURL_MAX_LEN];

char IGDstname[ST_MAX_LEN] = "urn:schemas-upnp-org:device:InternetGatewayDevice:2";

/* UPnP permission rules : */
struct upnpperm * upnppermlist = 0;
unsigned int num_upnpperm = 0;

#ifdef ENABLE_NATPMP
/* NAT-PMP */
unsigned int nextnatpmptoclean_timestamp = 0;
unsigned short nextnatpmptoclean_eport = 0;
unsigned short nextnatpmptoclean_proto = 0;
#endif

#ifdef USE_PF
const char * queue = 0;
const char * tag = 0;
#endif

#ifdef USE_NETFILTER
/* chain name to use, both in the nat table
 * and the filter table */
const char * miniupnpd_nat_chain = "MINIUPNPD";
const char * miniupnpd_forward_chain = "MINIUPNPD";
#endif

int n_lan_addr = 0;
struct lan_addr_s lan_addr[MAX_LAN_ADDR];

/* Path of the Unix socket used to communicate with MiniSSDPd */
const char * minissdpdsocketpath = "/var/run/minissdpd6.sock";

int firewallEnabled = 1;
int inboundPinholeAllowed = 1;
int inboundPinholeSpace = 1;
int systemUpdateID = 0;
char nextpinholetoclean_uid[5] = "";
unsigned int nextpinholetoclean_timestamp = 0;

int ip_version = 0;
char tmp_char_1[40]="";
char tmp_char_2[40]="";
int count_1 = 0;
int count_2 = 0;

const char * rulefilename = "rule-file6.txt";
int line_number = 1;
