/* $Id: upnpdescgen.c,v 1.51 2009/09/06 21:26:36 nanard Exp $ */
/* MiniUPnP project
 * http://miniupnp.free.fr/ or http://miniupnp.tuxfamily.org/
 * (c) 2006-2009 Thomas Bernard 
 * This software is subject to the conditions detailed
 * in the LICENCE file provided within the distribution */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#ifdef ENABLE_EVENTS
#include "getifaddr.h"
#include "upnpredirect.h"
#endif
#include "upnpdescgen.h"
#include "miniupnpdpath.h"
#include "upnpglobalvars.h"
#include "upnpdescstrings.h"

#ifdef MINIUPNPD_DEBUG_DESCRIPTION
#define DEBUG_MSG(fmt, arg...)       printf("%s [%d]: "fmt, __FILE__, __LINE__, ##arg)
#else
#define DEBUG_MSG(fmt, arg...) 
#endif
char modelname[MODELNAME_MAX_LEN];
char modelurl[MODELURL_MAX_LEN];
char manufacturername[MANUFACTURERNAME_MAX_LEN];
char manufacturerurl[MANUFACTURERURL_MAX_LEN];
char friendlyname[FRIENDLYNAME_MAX_LEN];
static const char * const upnptypes[] =
{
	"string",
	"boolean",
	"ui2",
	"ui4"
};

static const char * const upnpdefaultvalues[] =
{
	0,
	"Unconfigured",
	0,
	"IP_Routed",
	0,
	"3600"
};

static const char * const upnpallowedvalueslist[] =
{
	0,		/* 0 */
	"DSL",	/* 1 */
	"POTS",
	"Cable",
	"Ethernet",
	0,
	"Up",	/* 6 */
	"Down",
	"Initializing",
	"Unavailable",
	0,
	"TCP",	/* 11 */
	"UDP",
	0,
	"Unconfigured",	/* 14 */
	"IP_Routed",
	"IP_Bridged",
	0,
	"Unconfigured",	/* 18 */
	"Connecting",
	"Connected",
	"PendingDisconnect",
	"Disconnecting",
	"Disconnected",
	0,
	"ERROR_NONE",	/* 25 */
	0,
	"",		/* 27 */
	0
};

static const char * const upnpallowedvaluesrange[] =
{
	0,		/* 0 */
	"0",	/* 1 */
	"604800",
	0,
	"1",	/* 4 */
	"65535",
	0,
	"1",	/* 7 */
	"86400",
	0
};

static const char xmlver[] = 
	"<?xml version=\"1.0\"?>\r\n";
static const char root_service[] =
	"scpd xmlns=\"urn:schemas-upnp-org:service-1-0\"";
static const char root_device[] = 
	"root xmlns=\"urn:schemas-upnp-org:device-1-0\"";

/* root Description of the UPnP Device 
 * fixed to match UPnP_IGD_InternetGatewayDevice 1.0.pdf 
 * presentationURL is only "recommended" but the router doesn't appears
 * in "Network connections" in Windows XP if it is not present. */
static const struct XMLElt rootDesc[] =
{
/* 0 */
	{root_device, INITHELPER(1,2)},
	{"specVersion", INITHELPER(3,2)},
#if defined(ENABLE_L3F_SERVICE) || defined(HAS_DUMMY_SERVICE)
	{"device", INITHELPER(5,13)},
#else
	{"device", INITHELPER(5,12)},
#endif
	{"/major", "1"},
	{"/minor", "0"},
/* 5 */
	{"/deviceType", IGDstname},
	{"/friendlyName", ROOTDEV_FRIENDLYNAME},	/* required */
	{"/manufacturer", ROOTDEV_MANUFACTURER},		/* required */
/* 8 */
	{"/manufacturerURL", ROOTDEV_MANUFACTURERURL},	/* optional */
	{"/modelDescription", ROOTDEV_MODELDESCRIPTION}, /* recommended */
	{"/modelName", ROOTDEV_MODELNAME},	/* required */
	{"/modelNumber", modelnumber},
	{"/modelURL", ROOTDEV_MODELURL},
	{"/serialNumber", serialnumber},
	{"/UDN", uuidvalue},	/* required */
#if defined(ENABLE_L3F_SERVICE) || defined(HAS_DUMMY_SERVICE)
	{"serviceList", INITHELPER(63,1)},
	{"deviceList", INITHELPER(18,1)},
	{"/presentationURL", presentationurl},	/* recommended */
#else
	{"deviceList", INITHELPER(18,1)},
	{"/presentationURL", presentationurl},	/* recommended */
	{0,0},
#endif
/* 18 */
	{"device", INITHELPER(19,13)},
/* 19 */
	{"/deviceType", "urn:schemas-upnp-org:device:WANDevice:2"}, /* required */
	{"/friendlyName", WANDEV_FRIENDLYNAME},
	{"/manufacturer", WANDEV_MANUFACTURER},
	{"/manufacturerURL", WANDEV_MANUFACTURERURL},
	{"/modelDescription" , WANDEV_MODELDESCRIPTION},
	{"/modelName", WANDEV_MODELNAME},
	{"/modelNumber", WANDEV_MODELNUMBER},
	{"/modelURL", WANDEV_MODELURL},
	{"/serialNumber", serialnumber},
	{"/UDN", wandevuuid},
	{"/UPC", WANDEV_UPC},
/* 30 */
	{"serviceList", INITHELPER(32,1)},
	{"deviceList", INITHELPER(38,1)},
/* 32 */
	{"service", INITHELPER(33,5)},
/* 33 */
	{"/serviceType",
			"urn:schemas-upnp-org:service:WANCommonInterfaceConfig:1"},
	/*{"/serviceId", "urn:upnp-org:serviceId:WANCommonInterfaceConfig"}, */
	{"/serviceId", "urn:upnp-org:serviceId:WANCommonIFC1"}, /* required */
	{"/controlURL", WANCFG_CONTROLURL},
	{"/eventSubURL", WANCFG_EVENTURL},
	{"/SCPDURL", WANCFG_PATH},
/* 38 */
	{"device", INITHELPER(39,12)},
/* 39 */
	{"/deviceType", "urn:schemas-upnp-org:device:WANConnectionDevice:2"},
	{"/friendlyName", WANCDEV_FRIENDLYNAME},
	{"/manufacturer", WANCDEV_MANUFACTURER},
	{"/manufacturerURL", WANCDEV_MANUFACTURERURL},
	{"/modelDescription", WANCDEV_MODELDESCRIPTION},
	{"/modelName", WANCDEV_MODELNAME},
	{"/modelNumber", WANCDEV_MODELNUMBER},
	{"/modelURL", WANCDEV_MODELURL},
	{"/serialNumber", serialnumber},
	{"/UDN", wanconuuid},
	{"/UPC", WANCDEV_UPC},
	{"serviceList", INITHELPER(51,2)},
/* 51 */
	{"service", INITHELPER(53,5)},
	{"service", INITHELPER(58,5)},
/* 52 */
	{"/serviceType", "urn:schemas-upnp-org:service:WANIPConnection:2"},
	/* {"/serviceId", "urn:upnp-org:serviceId:WANIPConnection"}, */
	{"/serviceId", "urn:upnp-org:serviceId:WANIPConn2"},
	{"/controlURL", WANIPC_CONTROLURL},
	{"/eventSubURL", WANIPC_EVENTURL},
	{"/SCPDURL", WANIPC_PATH},
/* 57 */
	{"/serviceType", "urn:schemas-upnp-org:service:WANIPv6FirewallControl:1"},
 	{"/serviceId", "urn:upnp-org:serviceId:WANIPv6FC1"},
 	{"/controlURL", WANIP6FC_CONTROLURL},
 	{"/eventSubURL", WANIP6FC_EVENTURL},
 	{"/SCPDURL", WANIP6FC_PATH},
#ifdef HAS_DUMMY_SERVICE
	{"service", INITHELPER(64,5)},
/* 63 */
	{"/serviceType", "urn:schemas-dummy-com:service:Dummy:1"},
	{"/serviceId", "urn:dummy-com:serviceId:dummy1"},
	{"/controlURL", "/dummy"},
	{"/eventSubURL", "/dummy"},
	{"/SCPDURL", DUMMY_PATH},
#endif
#ifdef ENABLE_L3F_SERVICE
	{"service", INITHELPER(64,5)},
/* 63 */
	{"/serviceType", "urn:schemas-upnp-org:service:Layer3Forwarding:1"},
	{"/serviceId", "urn:upnp-org:serviceId:Layer3Forwarding1"},
	{"/controlURL", L3F_CONTROLURL}, /* The Layer3Forwarding service is only */
	{"/eventSubURL", L3F_EVENTURL}, /* recommended, not mandatory */
	{"/SCPDURL", L3F_PATH},
#endif
	{0, 0}
};

/* WANIPCn.xml */
/* see UPnP_IGD_WANIPConnection 1.0.pdf
static struct XMLElt scpdWANIPCn[] =
{
	{root_service, {INITHELPER(1,2)}},
	{0, {0}}
};
*/
static const struct argument AddPortMappingArgs[] =
{
	{"NewRemoteHost", 1, 11},
	{"NewExternalPort", 1, 12},
	{"NewProtocol", 1, 14},
	{"NewInternalPort", 1, 13},
	{"NewInternalClient", 1, 15},
	{"NewEnabled", 1, 9},
	{"NewPortMappingDescription", 1, 16},
	{"NewLeaseDuration", 1, 10},
	{"", 0, 0}
};

static const struct argument AddAnyPortMappingArgs[] =
{
	{"NewRemoteHost", 1, 11},
	{"NewExternalPort", 1, 12},
	{"NewProtocol", 1, 14},
	{"NewInternalPort", 1, 13},
	{"NewInternalClient", 1, 15},
	{"NewEnabled", 1, 9},
	{"NewPortMappingDescription", 1, 16},
	{"NewLeaseDuration", 1, 10},
	{"NewReservedPort", 2, 12},
	{"", 0, 0}
};// Added for certification purpose

static const struct argument GetExternalIPAddressArgs[] =
{
	{"NewExternalIPAddress", 2, 7},
	{"", 0, 0}
};

static const struct argument DeletePortMappingArgs[] = 
{
	{"NewRemoteHost", 1, 11},
	{"NewExternalPort", 1, 12},
	{"NewProtocol", 1, 14},
	{"", 0, 0}
};

static const struct argument DeletePortMappingRangeArgs[] = 
{
	{"NewStartPort", 1, 12},
	{"NewEndPort", 1, 12},
	{"NewProtocol", 1, 14},
	{"NewManage", 1, 18},
	{"", 0, 0}
};// Added for certification purpose

static const struct argument SetConnectionTypeArgs[] =
{
	{"NewConnectionType", 1, 0},
	{"", 0, 0}
};

static const struct argument GetConnectionTypeInfoArgs[] =
{
	{"NewConnectionType", 2, 0},
	{"NewPossibleConnectionTypes", 2, 1},
	{"", 0, 0}
};

static const struct argument GetStatusInfoArgs[] =
{
	{"NewConnectionStatus", 2, 2},
	{"NewLastConnectionError", 2, 4},
	{"NewUptime", 2, 3},
	{"", 0, 0}
};

static const struct argument GetNATRSIPStatusArgs[] =
{
	{"NewRSIPAvailable", 2, 5},
	{"NewNATEnabled", 2, 6},
	{"", 0, 0}
};

static const struct argument GetGenericPortMappingEntryArgs[] =
{
	{"NewPortMappingIndex", 1, 8},
	{"NewRemoteHost", 2, 11},
	{"NewExternalPort", 2, 12},
	{"NewProtocol", 2, 14},
	{"NewInternalPort", 2, 13},
	{"NewInternalClient", 2, 15},
	{"NewEnabled", 2, 9},
	{"NewPortMappingDescription", 2, 16},
	{"NewLeaseDuration", 2, 10},
	{"", 0, 0}
};

static const struct argument GetSpecificPortMappingEntryArgs[] =
{
	{"NewRemoteHost", 1, 11},
	{"NewExternalPort", 1, 12},
	{"NewProtocol", 1, 14},
	{"NewInternalPort", 2, 13},
	{"NewInternalClient", 2, 15},
	{"NewEnabled", 2, 9},
	{"NewPortMappingDescription", 2, 16},
	{"NewLeaseDuration", 2, 10},
	{"", 0, 0}
};

static const struct argument GetListOfPortMappingsArgs[] =
{
	{"NewStartPort", 1, 12},
	{"NewEndPort", 1, 12},
	{"NewProtocol", 1, 14},
	{"NewManage", 1, 18},
	{"NewNumberOfPorts", 1, 8},
	{"NewPortListing", 2, 19},
	{"", 0, 0}
};// Added for certification purpose

static const struct action WANIPCnActions[] =
{
	{"SetConnectionType", SetConnectionTypeArgs}, /* R */
	{"GetConnectionTypeInfo", GetConnectionTypeInfoArgs}, /* R */
	{"RequestConnection", 0}, /* R */
	{"ForceTermination", 0}, /* R */
	{"GetStatusInfo", GetStatusInfoArgs}, /* R */
	{"GetNATRSIPStatus", GetNATRSIPStatusArgs}, /* R */
	{"GetGenericPortMappingEntry", GetGenericPortMappingEntryArgs}, /* R */
	{"GetSpecificPortMappingEntry", GetSpecificPortMappingEntryArgs}, /* R */
	{"AddPortMapping", AddPortMappingArgs}, /* R */
	{"AddAnyPortMapping", AddAnyPortMappingArgs}, /* R */ // Added for certification purpose
	{"DeletePortMapping", DeletePortMappingArgs}, /* R */
	{"DeletePortMappingRange", DeletePortMappingRangeArgs}, /* R */ // Added for certification purpose
	{"GetExternalIPAddress", GetExternalIPAddressArgs}, /* R */
	{"GetListOfPortMappings", GetListOfPortMappingsArgs}, /* R */ // Added for certification purpose
	{0, 0}
};
/* R=Required, O=Optional */

static const struct stateVar WANIPCnVars[] =
{
/* 0 */
	{"ConnectionType", 0, 3, 14, 0/*1*/}, /* required */
	{"PossibleConnectionTypes", 0|0x80, 0, 0, 0, 15},
	 /* Required
	  * Allowed values : Unconfigured / IP_Routed / IP_Bridged */
	{"ConnectionStatus", 0|0x80, 0/*1*/, 18, 0, 20}, /* required */
	 /* Allowed Values : Unconfigured / Connecting(opt) / Connected
	  *                  PendingDisconnect(opt) / Disconnecting (opt)
	  *                  Disconnected */
	{"Uptime", 3, 0},	/* Required */
	{"LastConnectionError", 0, 0, 25},	/* required : */
	 /* Allowed Values : ERROR_NONE(req) / ERROR_COMMAND_ABORTED(opt)
	  *                  ERROR_NOT_ENABLED_FOR_INTERNET(opt)
	  *                  ERROR_USER_DISCONNECT(opt)
	  *                  ERROR_ISP_DISCONNECT(opt)
	  *                  ERROR_IDLE_DISCONNECT(opt)
	  *                  ERROR_FORCED_DISCONNECT(opt)
	  *                  ERROR_NO_CARRIER(opt)
	  *                  ERROR_IP_CONFIGURATION(opt)
	  *                  ERROR_UNKNOWN(opt) */
	{"RSIPAvailable", 1, 0}, /* required */ //5
	{"NATEnabled", 1, 0},    /* required */
	{"ExternalIPAddress", 0|0x80, 0, 0, 0, 254}, /* required. Default : empty string */
	{"PortMappingNumberOfEntries", 2|0x80, 0, 0, 0, 253}, /* required >= 0 */
	{"PortMappingEnabled", 1, 0}, /* Required */
	{"PortMappingLeaseDuration", 3, 5, 0, 1, 0}, /* required */ //10
	{"RemoteHost", 0, 0},   /* required. Default : empty string */
	{"ExternalPort", 2, 0}, /* required */
	{"InternalPort", 2, 0, 0, 4, 0}, /* required */
	{"PortMappingProtocol", 0, 0, 11}, /* required allowedValues: TCP/UDP */
	{"InternalClient", 0, 0}, /* required */ //15
	{"PortMappingDescription", 0, 0}, /* required default: empty string */
	{"SystemUpdateID", 3|0x80, 0}, // Added for certification purpose
	{"A_ARG_TYPE_Manage", 1, 0}, // Added for certification purpose
	{"A_ARG_TYPE_PortListing", 0, 0}, // Added for certification purpose
	{0, 0}
};

static const struct serviceDesc scpdWANIPCn =
{ WANIPCnActions, WANIPCnVars };

/* WANCfg.xml */
/* See UPnP_IGD_WANCommonInterfaceConfig 1.0.pdf */

static const struct argument GetCommonLinkPropertiesArgs[] =
{
	{"NewWANAccessType", 2, 0},
	{"NewLayer1UpstreamMaxBitRate", 2, 1},
	{"NewLayer1DownstreamMaxBitRate", 2, 2},
	{"NewPhysicalLinkStatus", 2, 3},
	{"", 0, 0}
};

static const struct argument GetTotalBytesSentArgs[] =
{
	{"NewTotalBytesSent", 2, 4},
	{"", 0, 0}
};

static const struct argument GetTotalBytesReceivedArgs[] =
{
	{"NewTotalBytesReceived", 2, 5},
	{"", 0, 0}
};

static const struct argument GetTotalPacketsSentArgs[] =
{
	{"NewTotalPacketsSent", 2, 6},
	{"", 0, 0}
};

static const struct argument GetTotalPacketsReceivedArgs[] =
{
	{"NewTotalPacketsReceived", 2, 7},
	{"", 0, 0}
};

static const struct action WANCfgActions[] =
{
	{"GetCommonLinkProperties", GetCommonLinkPropertiesArgs}, /* Required */
	{"GetTotalBytesSent", GetTotalBytesSentArgs},             /* optional */
	{"GetTotalBytesReceived", GetTotalBytesReceivedArgs},     /* optional */
	{"GetTotalPacketsSent", GetTotalPacketsSentArgs},         /* optional */
	{"GetTotalPacketsReceived", GetTotalPacketsReceivedArgs}, /* optional */
	{0, 0}
};

/* See UPnP_IGD_WANCommonInterfaceConfig 1.0.pdf */
static const struct stateVar WANCfgVars[] =
{
	{"WANAccessType", 0, 0, 1},
	/* Allowed Values : DSL / POTS / Cable / Ethernet 
	 * Default value : empty string */
	{"Layer1UpstreamMaxBitRate", 3, 0},
	{"Layer1DownstreamMaxBitRate", 3, 0},
	{"PhysicalLinkStatus", 0|0x80, 0, 6, 0, 6},
	/*  allowed values : 
	 *      Up / Down / Initializing (optional) / Unavailable (optionnal)
	 *  no Default value 
	 *  Evented */
	{"TotalBytesSent", 3, 0},	   /* Optional */
	{"TotalBytesReceived", 3, 0},  /* Optional */
	{"TotalPacketsSent", 3, 0},    /* Optional */
	{"TotalPacketsReceived", 3, 0},/* Optional */
	/*{"MaximumActiveConnections", 2, 0},	// allowed Range value // OPTIONAL */
	{0, 0}
};

static const struct serviceDesc scpdWANCfg =
{ WANCfgActions, WANCfgVars };

#ifdef ENABLE_6FC_SERVICE
/* WANIP6FC.xml */
static const struct argument GetFirewallStatusArgs[] =
{
	{"FirewallEnabled", 2, 0},
	{"InboundPinholeAllowed", 2, 1},
	{"", 0, 0}
};

static const struct argument GetOutboundPinholeTimeoutArgs[] =
{
	{"RemoteHost", 1, 3},
	{"RemotePort", 1, 4},
	{"InternalClient", 1, 3},
	{"InternalPort", 1, 4},
	{"Protocol", 1, 5},
	{"OutboundPinholeTimeout", 2, 2},
	{"", 0, 0}
};

static const struct argument AddPinholeArgs[] =
{
	{"RemoteHost", 1, 3},
	{"RemotePort", 1, 4},
	{"InternalClient", 1, 3},
	{"InternalPort", 1, 4},
	{"Protocol", 1, 5},
	{"LeaseTime", 1, 6},
	{"UniqueID", 2, 7},
	{"", 0, 0}
};

static const struct argument UpdatePinholeArgs[] =
{
	{"UniqueID", 1, 7},
	{"NewLeaseTime", 1, 6},
	{"", 0, 0}
};

static const struct argument DeletePinholeArgs[] = 
{
	{"UniqueID", 1, 7},
	{"", 0, 0}
};

static const struct argument CheckPinholeWorkingArgs[] =
{
	{"UniqueID", 1, 7},
	{"IsWorking", 2, 8},
	{"", 0, 0}
};

static const struct argument GetPinholePacketsArgs[] =
{
	{"UniqueID", 1, 7},
	{"IsWorking", 2, 9},
	{"", 0, 0}
};

static const struct action WANIP6FCActions[] =
{
	{"AddPinhole", AddPinholeArgs}, /* R */
	{"DeletePinhole", DeletePinholeArgs}, /* R */
	{"GetFirewallStatus", GetFirewallStatusArgs}, /* R */
	{"UpdatePinhole", UpdatePinholeArgs}, /* R */
	{"GetOutboundPinholeTimeout", GetOutboundPinholeTimeoutArgs}, /* R */
	{"CheckPinholeWorking", CheckPinholeWorkingArgs}, /* R */
	{"GetPinholePackets", GetPinholePacketsArgs}, /* O */
	{0, 0}
};

static const struct stateVar WANIP6FCVars[] =
{
/* 0 */
	{"FirewallEnabled", 1|0x80, 0, 0, 0, 251}, /* required */
	{"InboundPinholeAllowed", 1|0x80, 0, 0, 0, 252}, /* required */
	{"A_ARG_TYPE_OutboundPinholeTimeout", 3, 0, 0, 1, 0},
	{"A_ARG_TYPE_IPv6Address", 0, 0/*1*/}, /* required */
	{"A_ARG_TYPE_Port", 2, 0}, /* required */
	{"A_ARG_TYPE_Protocol", 2, 0}, /* required */
	{"A_ARG_TYPE_LeaseTime", 3, 5, 0, 7, 0}, /* required */
	{"A_ARG_TYPE_UniqueID", 2, 0}, /* required */
	{"A_ARG_TYPE_Boolean", 1, 0}, /* required */
	{"A_ARG_TYPE_PinholePackets", 3, 0},
	{0, 0}
};

static const struct serviceDesc scpdWANIP6FC =
{ WANIP6FCActions, WANIP6FCVars };
#endif

#ifdef ENABLE_L3F_SERVICE
/* Read UPnP_IGD_Layer3Forwarding_1.0.pdf */
static const struct argument SetDefaultConnectionServiceArgs[] =
{
	{"NewDefaultConnectionService", 1, 0}, /* in */
	{"", 0, 0}
};

static const struct argument GetDefaultConnectionServiceArgs[] =
{
	{"DefaultConnectionService", 2, 0}, /* out */
	{"", 0, 0}
};

static const struct action L3FActions[] =
{
	{"SetDefaultConnectionService", SetDefaultConnectionServiceArgs}, /* Req */
	{"GetDefaultConnectionService", GetDefaultConnectionServiceArgs}, /* Req */
	{0, 0}
};

static const struct stateVar L3FVars[] =
{
	{"DefaultConnectionService", 0|0x80, 0, 0, 0, 255}, /* Required */
	{0, 0}
};

static const struct serviceDesc scpdL3F =
{ L3FActions, L3FVars };
#endif

/* strcat_str()
 * concatenate the string and use realloc to increase the
 * memory buffer if needed. */
static char *
strcat_str(char * str, int * len, int * tmplen, const char * s2)
{
	int s2len;
	s2len = (int)strlen(s2);
	if(*tmplen <= (*len + s2len))
	{
		if(s2len < 256)
			*tmplen += 256;
		else
			*tmplen += s2len + 1;
		str = (char *)realloc(str, *tmplen);
	}
	/*strcpy(str + *len, s2); */
	memcpy(str + *len, s2, s2len + 1);
	*len += s2len;
	return str;
}

/* strcat_char() :
 * concatenate a character and use realloc to increase the
 * size of the memory buffer if needed */
static char *
strcat_char(char * str, int * len, int * tmplen, char c)
{
	if(*tmplen <= (*len + 1))
	{
		*tmplen += 256;
		str = (char *)realloc(str, *tmplen);
	}
	str[*len] = c;
	(*len)++;
	return str;
}

/* iterative subroutine using a small stack
 * This way, the progam stack usage is kept low */
static char *
genXML(char * str, int * len, int * tmplen,
                   const struct XMLElt * p)
{
	unsigned short i, j;
	unsigned long k;
	int top;
	const char * eltname, *s;
	char c;
	struct {
		unsigned short i;
		unsigned short j;
		const char * eltname;
	} pile[16]; /* stack */
	top = -1;
	i = 0;	/* current node */
	j = 1;	/* i + number of nodes*/
	for(;;)
	{
		eltname = p[i].eltname;
		if(!eltname)
			return str;
		if(eltname[0] == '/')
		{
			if(p[i].data && p[i].data[0])
			{
				/*printf("<%s>%s<%s>\n", eltname+1, p[i].data, eltname); */
				str = strcat_char(str, len, tmplen, '<');
				str = strcat_str(str, len, tmplen, eltname+1);
				str = strcat_char(str, len, tmplen, '>');
				str = strcat_str(str, len, tmplen, p[i].data);
				str = strcat_char(str, len, tmplen, '<');
				str = strcat_str(str, len, tmplen, eltname);
				str = strcat_char(str, len, tmplen, '>');
			}
			for(;;)
			{
				if(top < 0)
					return str;
				i = ++(pile[top].i);
				j = pile[top].j;
				/*printf("  pile[%d]\t%d %d\n", top, i, j); */
				if(i==j)
				{
					/*printf("</%s>\n", pile[top].eltname); */
					str = strcat_char(str, len, tmplen, '<');
					str = strcat_char(str, len, tmplen, '/');
					s = pile[top].eltname;
					for(c = *s; c > ' '; c = *(++s))
						str = strcat_char(str, len, tmplen, c);
					str = strcat_char(str, len, tmplen, '>');
					top--;
				}
				else
					break;
			}
		}
		else
		{
			/*printf("<%s>\n", eltname); */
			str = strcat_char(str, len, tmplen, '<');
			str = strcat_str(str, len, tmplen, eltname);
			str = strcat_char(str, len, tmplen, '>');
			k = (unsigned long)p[i].data;
			i = k & 0xffff;
			j = i + (k >> 16);
			top++;
			/*printf(" +pile[%d]\t%d %d\n", top, i, j); */
			pile[top].i = i;
			pile[top].j = j;
			pile[top].eltname = eltname;
		}
	}
}

/* genRootDesc() :
 * - Generate the root description of the UPnP device.
 * - the len argument is used to return the length of
 *   the returned string. 
 * - tmp_uuid argument is used to build the uuid string */
char *
genRootDesc(int * len)
{
	char * str;
	int tmplen;
	tmplen = 2048;
	str = (char *)malloc(tmplen);
	if(str == NULL)
		return NULL;
	* len = strlen(xmlver);
	/*strcpy(str, xmlver); */
	memcpy(str, xmlver, *len + 1);
	str = genXML(str, len, &tmplen, rootDesc);
	str[*len] = '\0';
	return str;
}

/* genServiceDesc() :
 * Generate service description with allowed methods and 
 * related variables. */
static char *
genServiceDesc(int * len, const struct serviceDesc * s)
{
	int i, j;
	const struct action * acts;
	const struct stateVar * vars;
	const struct argument * args;
	const char * p;
	char * str;
	int tmplen;
	tmplen = 2048;
	str = (char *)malloc(tmplen);
	if(str == NULL)
		return NULL;
	/*strcpy(str, xmlver); */
	*len = strlen(xmlver);
	memcpy(str, xmlver, *len + 1);
	
	acts = s->actionList;
	vars = s->serviceStateTable;

	str = strcat_char(str, len, &tmplen, '<');
	str = strcat_str(str, len, &tmplen, root_service);
	str = strcat_char(str, len, &tmplen, '>');

	str = strcat_str(str, len, &tmplen,
		"<specVersion><major>1</major><minor>0</minor></specVersion>");

	i = 0;
	str = strcat_str(str, len, &tmplen, "<actionList>");
	while(acts[i].name)
	{
		str = strcat_str(str, len, &tmplen, "<action><name>");
		str = strcat_str(str, len, &tmplen, acts[i].name);
		str = strcat_str(str, len, &tmplen, "</name>");
		/* argument List */
		args = acts[i].args;
		if(args)
		{
			str = strcat_str(str, len, &tmplen, "<argumentList>");
			j = 0;
			while(args[j].dir)
			{
				str = strcat_str(str, len, &tmplen, "<argument><name>");
				p = args[j].name;
				str = strcat_str(str, len, &tmplen, p);
				str = strcat_str(str, len, &tmplen, "</name><direction>");
				str = strcat_str(str, len, &tmplen, args[j].dir==1?"in":"out");
				str = strcat_str(str, len, &tmplen,
						"</direction><relatedStateVariable>");
				p = vars[args[j].relatedVar].name;
				str = strcat_str(str, len, &tmplen, p);
				str = strcat_str(str, len, &tmplen,
						"</relatedStateVariable></argument>");
				j++;
			}
			str = strcat_str(str, len, &tmplen,"</argumentList>");
		}
		str = strcat_str(str, len, &tmplen, "</action>");
		/*str = strcat_char(str, len, &tmplen, '\n'); // TEMP ! */
		i++;
	}
	str = strcat_str(str, len, &tmplen, "</actionList><serviceStateTable>");
	i = 0;
	while(vars[i].name)
	{
		str = strcat_str(str, len, &tmplen,
				"<stateVariable sendEvents=\"");
#ifdef ENABLE_EVENTS
		str = strcat_str(str, len, &tmplen, (vars[i].itype & 0x80)?"yes":"no");
#else
		/* for the moment allways send no. Wait for SUBSCRIBE implementation
		 * before setting it to yes */
		str = strcat_str(str, len, &tmplen, "no");
#endif
		str = strcat_str(str, len, &tmplen, "\"><name>");
		str = strcat_str(str, len, &tmplen, vars[i].name);
		str = strcat_str(str, len, &tmplen, "</name><dataType>");
		str = strcat_str(str, len, &tmplen, upnptypes[vars[i].itype & 0x0f]);
		str = strcat_str(str, len, &tmplen, "</dataType>");
		if(vars[i].iallowedlist)
		{
		  str = strcat_str(str, len, &tmplen, "<allowedValueList>");
		  for(j=vars[i].iallowedlist; upnpallowedvalueslist[j]; j++)
		  {
		    str = strcat_str(str, len, &tmplen, "<allowedValue>");
		    str = strcat_str(str, len, &tmplen, upnpallowedvalueslist[j]);
		    str = strcat_str(str, len, &tmplen, "</allowedValue>");
		  }
		  str = strcat_str(str, len, &tmplen, "</allowedValueList>");
		}
		/*if(vars[i].defaultValue) */
		if(vars[i].idefault)
		{
		  str = strcat_str(str, len, &tmplen, "<defaultValue>");
		  /*str = strcat_str(str, len, &tmplen, vars[i].defaultValue); */
		  str = strcat_str(str, len, &tmplen, upnpdefaultvalues[vars[i].idefault]);
		  str = strcat_str(str, len, &tmplen, "</defaultValue>");
		}
		if(vars[i].iallowedrange) // Added for certification purpose
		{
		  str = strcat_str(str, len, &tmplen, "<allowedValueRange>");
		  str = strcat_str(str, len, &tmplen, "<minimum>");
		  str = strcat_str(str, len, &tmplen, upnpallowedvaluesrange[vars[i].iallowedrange]);
		  str = strcat_str(str, len, &tmplen, "</minimum>");
		  str = strcat_str(str, len, &tmplen, "<maximum>");
		  str = strcat_str(str, len, &tmplen, upnpallowedvaluesrange[vars[i].iallowedrange+1]);
		  str = strcat_str(str, len, &tmplen, "</maximum>");
		  str = strcat_str(str, len, &tmplen, "</allowedValueRange>");
		}
		str = strcat_str(str, len, &tmplen, "</stateVariable>");
		/*str = strcat_char(str, len, &tmplen, '\n'); // TEMP ! */
		i++;
	}
	str = strcat_str(str, len, &tmplen, "</serviceStateTable></scpd>");
	str[*len] = '\0';
	return str;
}

/* genWANIPCn() :
 * Generate the WANIPConnection xml description */
char *
genWANIPCn(int * len)
{
	return genServiceDesc(len, &scpdWANIPCn);
}

/* genWANCfg() :
 * Generate the WANInterfaceConfig xml description. */
char *
genWANCfg(int * len)
{
	return genServiceDesc(len, &scpdWANCfg);
}

#ifdef ENABLE_6FC_SERVICE
/* genWANIP6FC() :
 * Generate the WANIPv6FirewallControl xml description */
char *
genWANIP6FC(int * len)
{
	return genServiceDesc(len, &scpdWANIP6FC);
}
#endif

#ifdef ENABLE_L3F_SERVICE
char *
genL3F(int * len)
{
	return genServiceDesc(len, &scpdL3F);
}
#endif

#ifdef ENABLE_EVENTS
static char *
genEventVars(int * len, const struct serviceDesc * s, const char * servns)
{
	char tmp[40], tmpStatus[4]; // IPv6 Modification
	const struct stateVar * v;
	char * str;
	int tmplen;
	tmplen = 512;
	str = (char *)malloc(tmplen);
	if(str == NULL)
		return NULL;
	*len = 0;
	v = s->serviceStateTable;
	str = strcat_str(str, len, &tmplen, "<e:propertyset xmlns:e=\"urn:schemas-upnp-org:event-1-0\" xmlns:s=\"");
	str = strcat_str(str, len, &tmplen, servns);
	str = strcat_str(str, len, &tmplen, "\">");
	while(v->name)
	{
		if(v->itype & 0x80)
		{
			str = strcat_str(str, len, &tmplen, "<e:property><s:");
			str = strcat_str(str, len, &tmplen, v->name);
			str = strcat_str(str, len, &tmplen, ">");
			//printf("<e:property><s:%s>", v->name);
			printf("event value: %d\n", v->ieventvalue);
			switch(v->ieventvalue)
			{
			case 0:
				break;
			case 250:	/* SystemUpdateID Magical Value */
				snprintf(tmpStatus, sizeof(tmpStatus), "%d", systemUpdateID);
				str = strcat_str(str, len, &tmplen, tmpStatus);
				break;
			case 251:	/* FirewallEnabled Magical Value */
				snprintf(tmpStatus, sizeof(tmpStatus), "%d", firewallEnabled);
				str = strcat_str(str, len, &tmplen, tmpStatus);
				break;
			case 252:	/* InboundPinholeAllowed Magical Value */
				snprintf(tmpStatus, sizeof(tmpStatus), "%d", inboundPinholeAllowed);
				str = strcat_str(str, len, &tmplen, tmpStatus);
				break;
			case 253:	/* Port mapping number of entries magical value */
				snprintf(tmp, sizeof(tmp), "%d", upnp_get_portmapping_number_of_entries());
				str = strcat_str(str, len, &tmplen, tmp);
				break;
			case 254:	/* External ip address magical value */
				if(use_ext_ip_addr)
					str = strcat_str(str, len, &tmplen, use_ext_ip_addr);
				else {
					char ext_ip_addr[INET6_ADDRSTRLEN]; // IPv6 Modification
					if(getifaddr(ext_if_name, ext_ip_addr, INET6_ADDRSTRLEN) < 0) // IPv6 Modification
					{
						str = strcat_str(str, len, &tmplen, "::"); // IPv6 Modification
					}
					else
					{
						str = strcat_str(str, len, &tmplen, ext_ip_addr);
					}
				}
				/*str = strcat_str(str, len, &tmplen, "0.0.0.0");*/
				break;
			case 255:	/* DefaultConnectionService magical value */
				str = strcat_str(str, len, &tmplen, wanconuuid);
				str = strcat_str(str, len, &tmplen, ":WANConnectionDevice:1,urn:upnp-org:serviceId:WANIPConn1");
				//printf("%s:WANConnectionDevice:1,urn:upnp-org:serviceId:WANIPConn1", uuidvalue);
				break;
			default:
				str = strcat_str(str, len, &tmplen, upnpallowedvalueslist[v->ieventvalue]);
				//printf("%s", upnpallowedvalueslist[v->ieventvalue]);
			}
			str = strcat_str(str, len, &tmplen, "</s:");
			str = strcat_str(str, len, &tmplen, v->name);
			str = strcat_str(str, len, &tmplen, "></e:property>");
			//printf("</s:%s></e:property>\n", v->name);
		}
		v++;
	}
	str = strcat_str(str, len, &tmplen, "</e:propertyset>");
	//printf("</e:propertyset>\n");
	//printf("\n");
	//printf("%d\n", tmplen);
	str[*len] = '\0';
	return str;
}

char *
getVarsWANIPCn(int * l)
{
	return genEventVars(l,
                        &scpdWANIPCn,
	                    "urn:schemas-upnp-org:service:WANIPConnection:1");
}

char *
getVarsWANCfg(int * l)
{
	return genEventVars(l,
	                    &scpdWANCfg,
	                    "urn:schemas-upnp-org:service:WANCommonInterfaceConfig:1");
}

#ifdef ENABLE_6FC_SERVICE
char *
getVarsWANIP6FC(int * l)
{
	return genEventVars(l,
                        &scpdWANIP6FC,
	                    "urn:schemas-upnp-org:service:WANIPv6FirewallControl:1");
}
#endif

#ifdef ENABLE_L3F_SERVICE
char *
getVarsL3F(int * l)
{
	return genEventVars(l,
	                    &scpdL3F,
	                    "urn:schemas-upnp-org:service:Layer3Forwarding:1");
}
#endif
#endif

char *
genPresentationPage()
{
	char str[5096];
	char tmp_str[2048];
	int i, len, tmplen;
	const struct stateVar * vars;
	printf("strcat issue ?\n");
	strcpy(str, presentationpage_begin);
	len = strlen(presentationpage_begin);
	printf("str prepared: \n%s\n", str);

	printf("starting with IPCn\n");
	vars = WANIPCnVars;
	i = 0;
	/* Adding the WANIPConnection part */
	tmplen = sprintf(tmp_str, presentationpage_table_begin, "WANIPConnection", WANIPC_PATH);
	strcat(str, tmp_str);
	len += tmplen;
	memset (tmp_str, 0, sizeof (tmp_str));
	tmplen=0;
	while(vars[i].name && !strncmp(vars[i].name, "A_ARG", 5)){
		printf("%s\n", vars[i].name);
		tmplen = sprintf(tmp_str, presentationpage_table_content, vars[i].name, vars[i].itype);
		strcat(str, tmp_str);
		len += tmplen;
		memset (tmp_str, 0, sizeof (tmp_str));
		tmplen=0;
		i++;
	}
	strcat(str, presentationpage_table_end);
	len += strlen(presentationpage_table_end);

	printf("Now with Cfg\n");
	vars = WANCfgVars;
	i = 0;
	/* Adding the WANCommonInterfaceConfig part */
	tmplen = sprintf(tmp_str, presentationpage_table_begin, "WANCommonInterfaceConfig", WANCFG_PATH);
	strcat(str, tmp_str);
	len += tmplen;
	memset (tmp_str, 0, sizeof (tmp_str));
	tmplen=0;
	while(vars[i].name && !strncmp(vars[i].name, "A_ARG", 5)){
		tmplen = sprintf(tmp_str, presentationpage_table_content, vars[i].name, "");
		strcat(str, tmp_str);
		len += tmplen;
		memset (tmp_str, 0, sizeof (tmp_str));
		tmplen=0;
		i++;
	}
	strcat(str, presentationpage_table_end);
	len += strlen(presentationpage_table_end);

	printf("Now with IP6FC\n");
	vars = WANIP6FCVars;
	i = 0;
#ifdef ENABLE_6FC_SERVICE
	/* Adding the WANIPv6FirewallControl part */
	tmplen = sprintf(tmp_str, presentationpage_table_begin, "WANIPv6FirewallControl", WANIP6FC_PATH);
	strcat(str, tmp_str);
	len += tmplen;
	memset (tmp_str, 0, sizeof (tmp_str));
	tmplen=0;
	while(vars[i].name && !strncmp(vars[i].name, "A_ARG", 5)){
		tmplen = sprintf(tmp_str, presentationpage_table_content, vars[i].name, "");
		strcat(str, tmp_str);
		len += tmplen;
		memset (tmp_str, 0, sizeof (tmp_str));
		tmplen=0;
		i++;
	}
	strcat(str, presentationpage_table_end);
	len += strlen(presentationpage_table_end);
#endif

	printf("finishing\n");
	strcat(str, presentationpage_end);
	len += strlen(presentationpage_end);
	str[len+1] = '\0';
	return str;
}
