/* $Id: upnpsoap.c,v 1.8 2009/06/29 10:00:02 jimmy_huang Exp $ */
/* MiniUPnP project
 * http://miniupnp.free.fr/ or http://miniupnp.tuxfamily.org/
 * (c) 2006-2008 Thomas Bernard 
 * This software is subject to the conditions detailed
 * in the LICENCE file provided within the distribution */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <syslog.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>

#include "config.h"
#include "upnpglobalvars.h"
#include "upnphttp.h"
#include "upnpsoap.h"
#include "upnpreplyparse.h"
#include "upnpredirect.h"
#include "getifaddr.h"
#include "getifstats.h"

#include "upnpevents.h"

#ifdef MINIUPNPD_DEBUG_SOAP
#define DEBUG_MSG(fmt, arg...)       printf("%s [%d]: "fmt, __FILE__, __LINE__, ##arg)
#else
#define DEBUG_MSG(fmt, arg...) 
#endif

static void
BuildSendAndCloseSoapResp(struct upnphttp * h,
                          const char * body, int bodylen)
{
/*	Date:	2009-04-29
*	Name:	jimmy huang
*	Reason:	Add utf-8
*	Note:	Refer to Ubicom DIR-655
*/
/*
	static const char beforebody[] =
		"<?xml version=\"1.0\"?>\r\n"
		"<s:Envelope xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\" "
		"s:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\">"
		"<s:Body>";
*/
	static const char beforebody[] =
		"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\r\n"
		"<s:Envelope xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\" "
		"s:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\">"
		"<s:Body>";

	static const char afterbody[] =
		"</s:Body>"
		"</s:Envelope>\r\n";

	BuildHeader_upnphttp(h, 200, "OK",  sizeof(beforebody) - 1
		+ sizeof(afterbody) - 1 + bodylen );

	memcpy(h->res_buf + h->res_buflen, beforebody, sizeof(beforebody) - 1);
	h->res_buflen += sizeof(beforebody) - 1;

	memcpy(h->res_buf + h->res_buflen, body, bodylen);
	h->res_buflen += bodylen;

	memcpy(h->res_buf + h->res_buflen, afterbody, sizeof(afterbody) - 1);
	h->res_buflen += sizeof(afterbody) - 1;

	SendResp_upnphttp(h);
	CloseSocket_upnphttp(h);
}

static void
GetConnectionTypeInfo(struct upnphttp * h, const char * action)
{
	static const char resp[] =
		"<u:GetConnectionTypeInfoResponse "
		"xmlns:u=\"urn:schemas-upnp-org:service:WANIPConnection:1\">"
		"<NewConnectionType>IP_Routed</NewConnectionType>\n"
		"<NewPossibleConnectionTypes>IP_Routed</NewPossibleConnectionTypes>\n"
		"</u:GetConnectionTypeInfoResponse>";
	BuildSendAndCloseSoapResp(h, resp, sizeof(resp)-1);
}

static void
GetTotalBytesSent(struct upnphttp * h, const char * action)
{
	int r;

	static const char resp[] =
		"<u:%sResponse "
		"xmlns:u=\"%s\">"
		"<NewTotalBytesSent>%llu</NewTotalBytesSent>"
		"</u:%sResponse>";

	char body[512];
	int bodylen;
	struct ifdata data;

	r = getifstats(ext_if_name, &data);
	bodylen = snprintf(body, sizeof(body), resp,
	         action, "urn:schemas-upnp-org:service:WANCommonInterfaceConfig:1",
             r<0?0:data.obytes, action);
	DEBUG_MSG("\nupnp send(%d)-%s-%llu-%s\n",bodylen,ext_if_name,(r<0?0:data.obytes),body);
	BuildSendAndCloseSoapResp(h, body, bodylen);
}

static void
GetTotalBytesReceived(struct upnphttp * h, const char * action)
{
	int r;

	static const char resp[] =
		"<u:%sResponse "
		"xmlns:u=\"%s\">"
		"<NewTotalBytesReceived>%llu</NewTotalBytesReceived>"
		"</u:%sResponse>";

	char body[512];
	int bodylen;
	struct ifdata data;

	r = getifstats(ext_if_name, &data);
	bodylen = snprintf(body, sizeof(body), resp,
	         action, "urn:schemas-upnp-org:service:WANCommonInterfaceConfig:1",
	         r<0?0:data.ibytes, action);
	DEBUG_MSG("\nupnp rcv(%d)-%s-%llu-%s\n",bodylen,ext_if_name,r<0?0:data.ibytes,body);
	BuildSendAndCloseSoapResp(h, body, bodylen);
}

static void
GetTotalPacketsSent(struct upnphttp * h, const char * action)
{
	int r;

	static const char resp[] =
		"<u:%sResponse "
		"xmlns:u=\"%s\">"
		"<NewTotalPacketsSent>%lu</NewTotalPacketsSent>"
		"</u:%sResponse>";

	char body[512];
	int bodylen;
	struct ifdata data;

	r = getifstats(ext_if_name, &data);
	bodylen = snprintf(body, sizeof(body), resp,
	         action, "urn:schemas-upnp-org:service:WANCommonInterfaceConfig:1",
	         r<0?0:data.opackets, action);
	BuildSendAndCloseSoapResp(h, body, bodylen);
}

static void
GetTotalPacketsReceived(struct upnphttp * h, const char * action)
{
	int r;

	static const char resp[] =
		"<u:%sResponse "
		"xmlns:u=\"%s\">"
		"<NewTotalPacketsReceived>%lu</NewTotalPacketsReceived>"
		"</u:%sResponse>";

	char body[512];
	int bodylen;
	struct ifdata data;

	r = getifstats(ext_if_name, &data);
	bodylen = snprintf(body, sizeof(body), resp,
	         action, "urn:schemas-upnp-org:service:WANCommonInterfaceConfig:1",
	         r<0?0:data.ipackets, action);
	BuildSendAndCloseSoapResp(h, body, bodylen);
}

static char *get_nvram(const char *name, char *value)
{
    char buf[128], *ptr;
    FILE *fp;

    _system("nvram get %s > /var/tmp/%s", name, name);

    sprintf(buf, "/var/tmp/%s", name);
    if ((fp=fopen(buf,"r")) == NULL)
        goto final;

    if (fgets(buf, sizeof(buf), fp) == NULL)
        goto final;

    if ((ptr=strchr(buf, '=' )) != NULL)
        strcpy(value, ptr+2);
    else
        strcpy(value, buf);

    // replace '\n' to '\0'
    value[strlen(value)-1] = '\0';

    fclose(fp);
    _system("rm -f /var/tmp/%s", name);
    return value;

final:
    if (fp)
        fclose(fp);

    _system("rm -f /var/tmp/%s", name);
    return NULL;
}

static void
GetCommonLinkProperties(struct upnphttp * h, const char * action)
{
	#include <linux_vct.h>
	
	/* WANAccessType : set depending on the hardware :
	 * DSL, POTS (plain old Telephone service), Cable, Ethernet */
	static const char resp[] =
		"<u:%sResponse "
		"xmlns:u=\"%s\">"
		/*"<NewWANAccessType>DSL</NewWANAccessType>"*/
		"<NewWANAccessType>Ethernet</NewWANAccessType>\n"
		"<NewLayer1UpstreamMaxBitRate>%lu</NewLayer1UpstreamMaxBitRate>\n"
		"<NewLayer1DownstreamMaxBitRate>%lu</NewLayer1DownstreamMaxBitRate>\n"
		"<NewPhysicalLinkStatus>%s</NewPhysicalLinkStatus>\n"
		"</u:%sResponse>";

	char body[2048];
	int bodylen;
	const char * status = "Up";	/* Up, Down (Required) */

    DEBUG_MSG("\n%s (%d): Begin\n",__FUNCTION__,__LINE__);

	char speed[16] = {0};
	char duplex[16]= {0};
	char link[16]= {0};
	char wan_eth[16];

	get_nvram("wan_eth", wan_eth);

	VCTGetPortConnectState(wan_eth, VCTWANPORT0, link);
	if (strcmp(link, "disconnect") == 0) {
		status = "Down";
	}
	else {
		unsigned long rate = 10000000;

		VCTGetPortSpeedState(wan_eth, VCTWANPORT0, speed, duplex);
		if (strcmp(speed, "giga") == 0) {
			rate = 1000000000;
		}
		else if (strcmp(speed, "100") == 0) {
			rate = 100000000;
		}
		downstream_bitrate = upstream_bitrate = rate;
	}

	bodylen = snprintf(body, sizeof(body), resp,
	    action, "urn:schemas-upnp-org:service:WANCommonInterfaceConfig:1",
		upstream_bitrate, downstream_bitrate,
	    status, action);
	BuildSendAndCloseSoapResp(h, body, bodylen);
}

static void
GetStatusInfo(struct upnphttp * h, const char * action)
{
	static const char resp[] =
		"<u:%sResponse "
		"xmlns:u=\"%s\">"
		"<NewConnectionStatus>%s</NewConnectionStatus>\n"
		"<NewLastConnectionError>ERROR_NONE</NewLastConnectionError>\n"
		"<NewUptime>%ld</NewUptime>\n"
		"</u:%sResponse>";
	
	DEBUG_MSG("\n%s (%d): Begin\n",__FUNCTION__,__LINE__);
	
	char body[512];
	int bodylen;
	time_t uptime;
	const char * status = "Connected";
	/* ConnectionStatus possible values :
	 * Unconfigured, Connecting, Connected, PendingDisconnect,
	 * Disconnecting, Disconnected */
	char ext_ip_addr[INET_ADDRSTRLEN];

	DEBUG_MSG("%s (%d): ext_if_name = %s\n",__FUNCTION__,__LINE__,ext_if_name);
	if(getifaddr(ext_if_name, ext_ip_addr, INET_ADDRSTRLEN) < 0) {
		/*	Date:	2010-08-10
		*	Name:	jimmy huang
		*	Reason:	For Vista / Win 7 icon in "View Computers and Devices"
		*			Only when we tell UPnP Control Point that our "ConnectionStatus" is "Connecting", the CP will open browser
		*			to our router.
		*	Note:	This way is not totally meet the UPnP spec
		*			UPnP_IGD_WANIPConnection 1.0.pdf, page 9, 17 ~ 20, has precisely definition
		*/
#ifdef ICON_DOUBLE_CLICK_FORCE_TO_GUI
		DEBUG_MSG("%s (%d): status = Connecting\n",__FUNCTION__,__LINE__);
		status = "Connecting";
#else
		DEBUG_MSG("%s (%d): status = Disconnected\n",__FUNCTION__,__LINE__);
		status = "Disconnected";
#endif
		//jimmy test for record wan connection status
		// we should not update the global variable here (last_WanConnectionStatus and last_ext_ip_addr)
		// if we do, in WanConnectionStatus_changed(), upnpevents.c
		// may miss the changing ...
		//strncpy(ext_ip_addr,"0.0.0.0\0",strlen("0.0.0.0\0"));
		//last_WanConnectionStatus = 0;
		
		// if we really want update here, we need check if the status has changed and
		// create notify right here
		//if(last_WanConnectionStatus == 1){
		//	upnp_event_var_change_notify(EWanIPC);//2
		//	last_WanConnectionStatus = 0;
		//}
		//---------------
	}else{
#ifdef ICON_DOUBLE_CLICK_FORCE_TO_GUI
		DEBUG_MSG("%s (%d): status = Connecting\n",__FUNCTION__,__LINE__);
		status = "Connecting";
#else
		DEBUG_MSG("%s (%d): status = Connected\n",__FUNCTION__,__LINE__);
#endif
		//jimmy test for record wan connection status
		// we should not update the global variable here (last_WanConnectionStatus and last_ext_ip_addr)
		// if we do, in WanConnectionStatus_changed(), upnpevents.c
		// may miss the changing ...
		//strncpy(last_ext_ip_addr,ext_ip_addr,INET_ADDRSTRLEN);
		//last_WanConnectionStatus = 1;
		
		// if we really want update here, we need check if the status has changed and
		// create notify right here
		//if(last_WanConnectionStatus == 0){
		//	last_WanConnectionStatus = 1;
		//	upnp_event_var_change_notify(EWanIPC);//2
		//}
		//---------------
	}
	
	
	uptime = (time(NULL) - startup_time);
	bodylen = snprintf(body, sizeof(body), resp,
		action, "urn:schemas-upnp-org:service:WANIPConnection:1",
		status, (long)uptime, action);	
	BuildSendAndCloseSoapResp(h, body, bodylen);
}

static void
GetNATRSIPStatus(struct upnphttp * h, const char * action)
{
	static const char resp[] =
		"<u:GetNATRSIPStatusResponse "
		"xmlns:u=\"urn:schemas-upnp-org:service:WANIPConnection:1\">"
		"<NewRSIPAvailable>0</NewRSIPAvailable>\n"
		"<NewNATEnabled>1</NewNATEnabled>\n"
		"</u:GetNATRSIPStatusResponse>\n";
	/* 2.2.9. RSIPAvailable
	 * This variable indicates if Realm-specific IP (RSIP) is available
	 * as a feature on the InternetGatewayDevice. RSIP is being defined
	 * in the NAT working group in the IETF to allow host-NATing using
	 * a standard set of message exchanges. It also allows end-to-end
	 * applications that otherwise break if NAT is introduced
	 * (e.g. IPsec-based VPNs).
	 * A gateway that does not support RSIP should set this variable to 0. */
	BuildSendAndCloseSoapResp(h, resp, sizeof(resp)-1);
}

static void
GetExternalIPAddress(struct upnphttp * h, const char * action)
{
	static const char resp[] =
		"<u:%sResponse "
		"xmlns:u=\"%s\">"
		"<NewExternalIPAddress>%s</NewExternalIPAddress>"
		"</u:%sResponse>";

	char body[512];
	int bodylen;
	char ext_ip_addr[INET_ADDRSTRLEN];

#ifndef MULTIPLE_EXTERNAL_IP
	if(use_ext_ip_addr)
	{
		strncpy(ext_ip_addr, use_ext_ip_addr, INET_ADDRSTRLEN);
	}
	else if(getifaddr(ext_if_name, ext_ip_addr, INET_ADDRSTRLEN) < 0)
	{
		syslog(LOG_ERR, "Failed to get ip address for interface %s",
			ext_if_name);
		strncpy(ext_ip_addr, "0.0.0.0", INET_ADDRSTRLEN);
	}
#else
	int i;
	strncpy(ext_ip_addr, "0.0.0.0", INET_ADDRSTRLEN);
	for(i = 0; i<n_lan_addr; i++)
	{
		if( (h->clientaddr.s_addr & lan_addr[i].mask.s_addr)
		   == (lan_addr[i].addr.s_addr & lan_addr[i].mask.s_addr))
		{
			strncpy(ext_ip_addr, lan_addr[i].ext_ip_str, INET_ADDRSTRLEN);
			break;
		}
	}
#endif
	bodylen = snprintf(body, sizeof(body), resp,
	              action, "urn:schemas-upnp-org:service:WANIPConnection:1",
				  ext_ip_addr, action);
	BuildSendAndCloseSoapResp(h, body, bodylen);
}

static void
AddPortMapping(struct upnphttp * h, const char * action)
{
	int r;

	static const char resp[] =
		"<u:AddPortMappingResponse "
		"xmlns:u=\"urn:schemas-upnp-org:service:WANIPConnection:1\"/>";

	struct NameValueParserData data;
	char * int_ip, * int_port, * ext_port, * protocol, * desc = NULL;
	char * leaseduration;

	char * ext_ip=NULL;
	char * Enabled = NULL;

	unsigned short iport, eport;

	struct hostent *hp; /* getbyhostname() */
	char ** ptr; /* getbyhostname() */
	struct in_addr result_ip;/*unsigned char result_ip[16];*/ /* inet_pton() */

	DEBUG_MSG("\n%s (%d): begin\n",__FUNCTION__,__LINE__);
	
	ParseNameValue(h->req_buf + h->req_contentoff, h->req_contentlen, &data);
	int_ip = GetValueFromNameValueList(&data, "NewInternalClient");

	if (!int_ip)
	{
		ClearNameValueList(&data);
		SoapError(h, 402, "Invalid Args");
		return;
	}
	/*	Date:	2009-06-29
	*	Name:	jimmy huang
	*	Reason:	We do not support internal ip is our router's local ip
	*/
	//if(strncmp(int_ip,lan_addr[0].str,strlen(int_ip)) == 0){
	if(strcmp(int_ip,lan_addr[0].str) == 0){
		ClearNameValueList(&data);
		SoapError(h, 402, "Invalid Args");
		return;
	}

	/* if ip not valid assume hostname and convert */
	if (inet_pton(AF_INET, int_ip, &result_ip) <= 0) 
	{
		hp = gethostbyname(int_ip);
		if(hp && hp->h_addrtype == AF_INET) 
		{ 
			for(ptr = hp->h_addr_list; ptr && *ptr; ptr++)
		   	{
				int_ip = inet_ntoa(*((struct in_addr *) *ptr));
				result_ip = *((struct in_addr *) *ptr);
				/* TODO : deal with more than one ip per hostname */
				break;
			}
		} 
		else 
		{
			syslog(LOG_ERR, "Failed to convert hostname '%s' to ip address", int_ip); 
			ClearNameValueList(&data);
			SoapError(h, 402, "Invalid Args");
			return;
		}				
	}

	/* check if NewInternalAddress is the client address */
	if(GETFLAG(SECUREMODEMASK))
	{
		if(h->clientaddr.s_addr != result_ip.s_addr)
		{
			syslog(LOG_INFO, "Client %s tried to redirect port to %s",
			       inet_ntoa(h->clientaddr), int_ip);
			ClearNameValueList(&data);
			SoapError(h, 718, "ConflictInMappingEntry");
			return;
		}
	}

	int_port = GetValueFromNameValueList(&data, "NewInternalPort");
	ext_port = GetValueFromNameValueList(&data, "NewExternalPort");
	protocol = GetValueFromNameValueList(&data, "NewProtocol");
	desc = GetValueFromNameValueList(&data, "NewPortMappingDescription");
	
	/*	Date:	2009-06-19
	*	Name:	jimmy huang
	*	Reason:	If we want to discard skype's action AddPortmapping
	*			Maybe we can drop it here
	*			Skype will add port 13316 for TCP and UDP with desc 
	*			"Skype TCP at xxx.xxx.xxx.xxx (900)" and "Skype UDP at xxx.xxx.xxx.xxx (900)"
	*	Note:	
	*			1. Not test
	*			2. Error code refer to 
	*				UPnP-DeviceArchitecture-v1.0.pdf, page56
	*				or
	*				UPnP-arch-DeviceArchitecture-v1 1-20081015.pdf, page89
	*			3. MSN seems still has other issue, by now, msn does not detect our UPnP
	*/
#ifdef DROP_SKYPE
	if(desc && (strncasecmp(desc,"Skype ",6) == 0)){
		ClearNameValueList(&data);
		// tell skype that our current state of service prevents invoking AddPortMapping action
		SoapError(h, 501, "ActionFailed");
		return;
	}
#endif
	// --------------------------------------------------------
	
	
	/*	Date:	2009-04-20
	*	Name:	jimmy huang
	*	Reason:	if no desc , force to set it as miniupnpd, if we do not force set it,
	*			within add_redirect_desc(), will ignore it, then we can't find the rule
	*			in memory's link list
	*/
	if(desc == NULL){
		desc = default_NewPortMappingDescription;
	}
	// ----------
	//Chun add: for CD_ROUTER
	ext_ip = GetValueFromNameValueList(&data, "NewRemoteHost");
	Enabled = GetValueFromNameValueList(&data, "NewEnabled");
	DEBUG_MSG("%s (%d): Enabled: %s\n",__FUNCTION__,__LINE__,Enabled);
	//if(strcasencmp(Enabled,"enabled",) == 0)
	// -----------
	leaseduration = GetValueFromNameValueList(&data, "NewLeaseDuration");

	if (!int_port || !ext_port || !protocol)
	{
		ClearNameValueList(&data);
		SoapError(h, 402, "Invalid Args");
		return;
	}

	eport = (unsigned short)atoi(ext_port);
	iport = (unsigned short)atoi(int_port);

	if(leaseduration && atoi(leaseduration)) {
		syslog(LOG_WARNING, "NewLeaseDuration=%s", leaseduration);
	}

	syslog(LOG_INFO, "%s: ext port %hu to %s:%hu protocol %s for: %s",
			action, eport, int_ip, iport, protocol, desc);
	DEBUG_MSG("%s:%s (%d), NewRemoteHost = %s\n",__FILE__,__FUNCTION__,__LINE__,ext_ip ? ext_ip : "");
	DEBUG_MSG("%s:%s (%d), NewExternalPort = %hu\n",__FILE__,__FUNCTION__,__LINE__,eport);
	DEBUG_MSG("%s:%s (%d), NewProtocol = %s\n",__FILE__,__FUNCTION__,__LINE__,protocol);
	DEBUG_MSG("%s:%s (%d), NewInternalPort = %hu\n",__FILE__,__FUNCTION__,__LINE__,iport);
	DEBUG_MSG("%s:%s (%d), NewInternalClient = %s\n",__FILE__,__FUNCTION__,__LINE__,int_ip ? int_ip : "");
	DEBUG_MSG("%s:%s (%d), NewEnabled = %s\n",__FILE__,__FUNCTION__,__LINE__,Enabled ? Enabled : "");
	DEBUG_MSG("%s:%s (%d), NewPortMappingDesc = %s\n",__FILE__,__FUNCTION__,__LINE__,desc ? desc : "");
	DEBUG_MSG("%s:%s (%d), NewLeaseDuration = %s\n",__FILE__,__FUNCTION__,__LINE__,leaseduration);

	/* Chun modify: for CD_ROUTER */
	//r = upnp_redirect(eport, int_ip, iport, protocol, desc);

	//r = upnp_redirect(eport, int_ip, iport, protocol, desc, ext_ip);
	if(Enabled && (atoi(Enabled) == 1)){
		r = upnp_redirect(eport, int_ip, iport, protocol, desc, ext_ip, atoi(Enabled) ,atol(leaseduration));
	}else{
		// try to find out if rules has been added already, if so, remove it
		DEBUG_MSG("%s (%d): Enabled is false so try to delete this rule\n",__FUNCTION__,__LINE__);
		if(upnp_delete_redirection(eport, protocol) == -1){
			DEBUG_MSG("%s (%d): delete success\n",__FUNCTION__,__LINE__);
		}else{
			DEBUG_MSG("%s (%d): No such rule\n",__FUNCTION__,__LINE__);
		}
		r = 0;
	}
	//--------------------------------

	ClearNameValueList(&data);

	/* possible error codes for AddPortMapping :
	 * 402 - Invalid Args
	 * 501 - Action Failed
	 * 715 - Wildcard not permited in SrcAddr
	 * 716 - Wildcard not permited in ExtPort
	 * 718 - ConflictInMappingEntry
	 * 724 - SamePortValuesRequired
     * 725 - OnlyPermanentLeasesSupported
             The NAT implementation only supports permanent lease times on
             port mappings
     * 726 - RemoteHostOnlySupportsWildcard
             RemoteHost must be a wildcard and cannot be a specific IP
             address or DNS name
     * 727 - ExternalPortOnlySupportsWildcard
             ExternalPort must be a wildcard and cannot be a specific port
             value */
	switch(r)
	{
	case 0:	/* success */
		BuildSendAndCloseSoapResp(h, resp, sizeof(resp)-1);
		break;
	case -2:	/* already redirected */
	case -3:	/* not permitted */
		SoapError(h, 718, "ConflictInMappingEntry");
		break;
	default:
		SoapError(h, 501, "ActionFailed");
	}
/*	Date:	2009-04-29
*	Name:	jimmy huang
*	Reason:	Currently, due to miniupnpd's architecture(select,noblock),
*			we may not immediately send notify after variable changed (Add / Del PortMapping...)
*			This definition is used to force when after soap, we use sock with block
*			to send notify right after AddPortMapping, search this definition within codes
*			for more detail
*	Note:	Not test this feature well
*/
#ifdef IMMEDIATELY_SEND_NOTIFY
 		DEBUG_MSG("%s (%d): go check_notify_right_now() at %ld  !! \n",__FUNCTION__,__LINE__,time(NULL));
 		check_notify_right_now(EWanIPC);
#endif
	// ----------
}

static void
GetSpecificPortMappingEntry(struct upnphttp * h, const char * action)
{
	int r;

	static const char resp[] =
		"<u:%sResponse "
		"xmlns:u=\"%s\">"
		"<NewInternalPort>%u</NewInternalPort>\n"
		"<NewInternalClient>%s</NewInternalClient>\n"
		"<NewEnabled>1</NewEnabled>\n"
		"<NewPortMappingDescription>%s</NewPortMappingDescription>\n"
		"<NewLeaseDuration>%d</NewLeaseDuration>\n"
		"</u:%sResponse>\n";

	char body[1024];
	int bodylen;
	struct NameValueParserData data;
	const char * r_host, * ext_port, * protocol;
	unsigned short eport, iport;
	char int_ip[32];
	char desc[64];
	// jimmy added
	long int expire_time = 0;
	// ----------

	ParseNameValue(h->req_buf + h->req_contentoff, h->req_contentlen, &data);
	r_host = GetValueFromNameValueList(&data, "NewRemoteHost");
	ext_port = GetValueFromNameValueList(&data, "NewExternalPort");
	protocol = GetValueFromNameValueList(&data, "NewProtocol");

	if(!ext_port || !protocol)
	{
		ClearNameValueList(&data);
		SoapError(h, 402, "Invalid Args");
		return;
	}

	eport = (unsigned short)atoi(ext_port);

	r = upnp_get_redirection_infos(eport, protocol, &iport,
	                               int_ip, sizeof(int_ip),
	                               desc, sizeof(desc),&expire_time);

	if(r < 0)
	{		
		SoapError(h, 714, "NoSuchEntryInArray");
	}
	else
	{
		syslog(LOG_INFO, "%s: rhost='%s' %s %s found => %s:%u desc='%s'",
		       action,
		       r_host, ext_port, protocol, int_ip, (unsigned int)iport, desc);

		if(expire_time > 0){
			expire_time = expire_time - time(NULL);
		}
		if(expire_time < 0){
			expire_time = 0;
		}

		bodylen = snprintf(body, sizeof(body), resp,
				action, "urn:schemas-upnp-org:service:WANIPConnection:1",
				(unsigned int)iport, int_ip, desc,
				//(int)(expire_time - time(NULL)),
				(int)expire_time,
				action);
		BuildSendAndCloseSoapResp(h, body, bodylen);
	}

	ClearNameValueList(&data);
}

static void
DeletePortMapping(struct upnphttp * h, const char * action)
{
	int r;

	static const char resp[] =
		"<u:DeletePortMappingResponse "
		"xmlns:u=\"urn:schemas-upnp-org:service:WANIPConnection:1\">"
		"</u:DeletePortMappingResponse>";

	struct NameValueParserData data;
	const char * r_host, * ext_port, * protocol;
	unsigned short eport;

	DEBUG_MSG("\n%s (%d): begin\n",__FUNCTION__,__LINE__);

	ParseNameValue(h->req_buf + h->req_contentoff, h->req_contentlen, &data);
	r_host = GetValueFromNameValueList(&data, "NewRemoteHost");
	ext_port = GetValueFromNameValueList(&data, "NewExternalPort");
	protocol = GetValueFromNameValueList(&data, "NewProtocol");

	if(!ext_port || !protocol)
	{
		ClearNameValueList(&data);
		SoapError(h, 402, "Invalid Args");
		return;
	}

	eport = (unsigned short)atoi(ext_port);

	/* TODO : if in secure mode, check the IP */

	syslog(LOG_INFO, "%s: external port: %hu, protocol: %s", 
		action, eport, protocol);

	DEBUG_MSG("%s (%d): go upnp_delete_redirection()\n",__FUNCTION__,__LINE__);
	
	r = upnp_delete_redirection(eport, protocol);
	
	DEBUG_MSG("%s (%d): upnp_delete_redirection() return r = %d\n",__FUNCTION__,__LINE__,r);

	if(r < 0)
	{	
		SoapError(h, 714, "NoSuchEntryInArray");
	}
	else
	{
		BuildSendAndCloseSoapResp(h, resp, sizeof(resp)-1);
	}

	DEBUG_MSG("%s (%d): go  ClearNameValueList() \n",__FUNCTION__,__LINE__);
	ClearNameValueList(&data);
	DEBUG_MSG("%s (%d): end\n",__FUNCTION__,__LINE__);
/*	Date:	2010-08-20
*	Name:	jimmy huang
*	Reason:	Currently, due to miniupnpd's architecture(select,noblock),
*			we may not immediately send notify after variable changed (Add / Del PortMapping...)
*			This definition is used to force when after soap, we use sock with block
*			to send notify right after AddPortMapping, search this definition within codes
*			for more detail
*	Note:	Not test this feature well
*/
#ifdef IMMEDIATELY_SEND_NOTIFY
 		DEBUG_MSG("%s (%d): go check_notify_right_now() at %ld  !! \n",__FUNCTION__,__LINE__,time(NULL));
 		check_notify_right_now(EWanIPC);
#endif
}

static void
GetGenericPortMappingEntry(struct upnphttp * h, const char * action)
{
	int r;
	
	static const char resp[] =
		"<u:%sResponse "
		"xmlns:u=\"%s\">"
		"<NewRemoteHost></NewRemoteHost>\n"
		"<NewExternalPort>%u</NewExternalPort>\n"
		"<NewProtocol>%s</NewProtocol>\n"
		"<NewInternalPort>%u</NewInternalPort>\n"
		"<NewInternalClient>%s</NewInternalClient>\n"
		"<NewEnabled>1</NewEnabled>\n"
		"<NewPortMappingDescription>%s</NewPortMappingDescription>\n"
		"<NewLeaseDuration>%d</NewLeaseDuration>\n"
		"</u:%sResponse>\n";

	int index_local = 0;
	unsigned short eport, iport;
	const char * m_index;
	char protocol[4], iaddr[32];
	char desc[64];
	struct NameValueParserData data;
	// jimmy added
	long int expire_time = 0;
	// ----------
	

	DEBUG_MSG("\n%s (%d): begin\n",__FUNCTION__,__LINE__);
	
	ParseNameValue(h->req_buf + h->req_contentoff, h->req_contentlen, &data);
	m_index = GetValueFromNameValueList(&data, "NewPortMappingIndex");

	if(!m_index)
	{
		ClearNameValueList(&data);
		SoapError(h, 402, "Invalid Args");
		return;
	}	

	index_local = (int)atoi(m_index);

	syslog(LOG_INFO, "%s: index_local=%d", action, index_local);
	
    DEBUG_MSG("%s (%d): go upnp_get_redirection_infos_by_index()\n",__FUNCTION__,__LINE__);
	r = upnp_get_redirection_infos_by_index(index_local, &eport, protocol, &iport,
                                            iaddr, sizeof(iaddr),
	                                        desc, sizeof(desc)
											,&expire_time
											);
	if(r < 0)
	{
		DEBUG_MSG("%s (%d): upnp_get_redirection_infos_by_index() failed\n",__FUNCTION__,__LINE__);
		SoapError(h, 713, "SpecifiedArrayIndexInvalid");
	}
	else
	{
		int bodylen;
		char body[2048];

		DEBUG_MSG("%s:%s (%d), NewRemoteHost = %s\n",__FILE__,__FUNCTION__,__LINE__,"");
		DEBUG_MSG("%s:%s (%d), NewExternalPort = %hu\n",__FILE__,__FUNCTION__,__LINE__,eport);
		DEBUG_MSG("%s:%s (%d), NewProtocol = %s\n",__FILE__,__FUNCTION__,__LINE__,protocol);
		DEBUG_MSG("%s:%s (%d), NewInternalPort = %hu\n",__FILE__,__FUNCTION__,__LINE__,iport);
		DEBUG_MSG("%s:%s (%d), NewInternalClient = %s\n",__FILE__,__FUNCTION__,__LINE__,iaddr ? iaddr : "");
		//DEBUG_MSG("%s:%s (%d), NewEnabled = %d\n",__FILE__,__FUNCTION__,__LINE__,Enabled);
		DEBUG_MSG("%s:%s (%d), NewPortMappingDesc = %s\n",__FILE__,__FUNCTION__,__LINE__,desc ? desc : "");
		DEBUG_MSG("%s:%s (%d), NewLeaseDuration = %ld\n",__FILE__,__FUNCTION__,__LINE__,expire_time);

		if(expire_time > 0){
			expire_time = expire_time - time(NULL);
		}
		if(expire_time < 0){
			expire_time = 0;
		}

		bodylen = snprintf(body, sizeof(body), resp,
			action, "urn:schemas-upnp-org:service:WANIPConnection:1",
			(unsigned int)eport, protocol, (unsigned int)iport, iaddr, desc,
			//(int)(expire_time - time(NULL)),
			(int)expire_time,
			action);
		BuildSendAndCloseSoapResp(h, body, bodylen);
	}

	ClearNameValueList(&data);
}

#ifdef ENABLE_L3F_SERVICE
static void
SetDefaultConnectionService(struct upnphttp * h, const char * action)
{
	static const char resp[] =
		"<u:SetDefaultConnectionServiceResponse "
		"xmlns:u=\"urn:schemas-upnp-org:service:Layer3Forwarding:1\">"
		"</u:SetDefaultConnectionServiceResponse>";
	struct NameValueParserData data;
	char * p;
	ParseNameValue(h->req_buf + h->req_contentoff, h->req_contentlen, &data);
	p = GetValueFromNameValueList(&data, "NewDefaultConnectionService");
	if(p) {
		syslog(LOG_INFO, "%s(%s) : Ignored", action, p);
	}
	ClearNameValueList(&data);
	BuildSendAndCloseSoapResp(h, resp, sizeof(resp)-1);
}

static void
GetDefaultConnectionService(struct upnphttp * h, const char * action)
{
	static const char resp[] =
		"<u:%sResponse "
		"xmlns:u=\"urn:schemas-upnp-org:service:Layer3Forwarding:1\">"
		"<NewDefaultConnectionService>%s:WANConnectionDevice:1,"
		"urn:upnp-org:serviceId:WANIPConn1</NewDefaultConnectionService>"
		"</u:%sResponse>";
	/* example from UPnP_IGD_Layer3Forwarding 1.0.pdf :
	 * uuid:44f5824f-c57d-418c-a131-f22b34e14111:WANConnectionDevice:1,
	 * urn:upnp-org:serviceId:WANPPPConn1 */
	char body[1024];
	int bodylen;

	bodylen = snprintf(body, sizeof(body), resp,
	                   action, uuidvalue, action);
	BuildSendAndCloseSoapResp(h, body, bodylen);
}
#endif

/*
If a control point calls QueryStateVariable on a state variable that is not
buffered in memory within (or otherwise available from) the service,
the service must return a SOAP fault with an errorCode of 404 Invalid Var.

QueryStateVariable remains useful as a limited test tool but may not be
part of some future versions of UPnP.
*/
static void
QueryStateVariable(struct upnphttp * h, const char * action)
{
	static const char resp[] =
        "<u:%sResponse "
        "xmlns:u=\"%s\">"
		"<return>%s</return>"
        "</u:%sResponse>";

	char body[512];
	int bodylen;
	struct NameValueParserData data;
	const char * var_name;

	ParseNameValue(h->req_buf + h->req_contentoff, h->req_contentlen, &data);
	/*var_name = GetValueFromNameValueList(&data, "QueryStateVariable"); */
	/*var_name = GetValueFromNameValueListIgnoreNS(&data, "varName");*/
	var_name = GetValueFromNameValueList(&data, "varName");

	/*syslog(LOG_INFO, "QueryStateVariable(%.40s)", var_name); */

	if(!var_name)
	{
		SoapError(h, 402, "Invalid Args");
	}
	else if(strcmp(var_name, "ConnectionStatus") == 0)
	{	
		const char * status = "Connected";
		char ext_ip_addr[INET_ADDRSTRLEN];
		if(getifaddr(ext_if_name, ext_ip_addr, INET_ADDRSTRLEN) < 0) {
			/*	Date:	2010-08-10
			*	Name:	jimmy huang
			*	Reason:	For Vista / Win 7 icon in "View Computers and Devices"
			*			Only when we tell UPnP Control Point that our "ConnectionStatus" is "Connecting", the CP will open browser
			*			to our router.
			*	Note:	This way is not totally meet the UPnP spec
			*			UPnP_IGD_WANIPConnection 1.0.pdf, page 9, 17 ~ 20, has precisely definition
			*/
#ifdef ICON_DOUBLE_CLICK_FORCE_TO_GUI
			status = "Connecting";
#else
			status = "Disconnected";
#endif
		}else{
#ifdef ICON_DOUBLE_CLICK_FORCE_TO_GUI
			status = "Connecting";
#endif
		}
		bodylen = snprintf(body, sizeof(body), resp,
                           action, "urn:schemas-upnp-org:control-1-0",
		                   status, action);
		BuildSendAndCloseSoapResp(h, body, bodylen);
	}
#if 0
	/* not usefull */
	else if(strcmp(var_name, "ConnectionType") == 0)
	{	
		bodylen = snprintf(body, sizeof(body), resp, "IP_Routed");
		BuildSendAndCloseSoapResp(h, body, bodylen);
	}
	else if(strcmp(var_name, "LastConnectionError") == 0)
	{	
		bodylen = snprintf(body, sizeof(body), resp, "ERROR_NONE");
		BuildSendAndCloseSoapResp(h, body, bodylen);
	}
#endif
	else if(strcmp(var_name, "PortMappingNumberOfEntries") == 0)
	{
		char strn[10];
		/*	Date:	2009-04-27
		*	Name:	jimmy huang
		*	Reason:	
		*			1. Refer to DIR-855
		*			2. If no rules, response empty value
		*/
		/*
		snprintf(strn, sizeof(strn), "%i",
		         upnp_get_portmapping_number_of_entries());
		*/
		int nums = 0;
		nums = upnp_get_portmapping_number_of_entries();
		if(nums){
			snprintf(strn, sizeof(strn), "%i",nums);
		}else{
			memset(strn,'\0',sizeof(strn));
		}

		bodylen = snprintf(body, sizeof(body), resp,
                           action, "urn:schemas-upnp-org:control-1-0",
		                   strn, action);
		BuildSendAndCloseSoapResp(h, body, bodylen);
	}
	else
	{
		syslog(LOG_NOTICE, "%s: Unknown: %s", action, var_name?var_name:"");
		SoapError(h, 404, "Invalid Var");
	}

	ClearNameValueList(&data);	
}

/*
* 
*/
static void
ForceTermination ( struct upnphttp * h )
{
	static const char resp[] =
	    "<u:ForceTerminationResponse "
	    "xmlns:u=\"urn:schemas-upnp-org:service:WANIPConnection:1\">"
	    "</u:ForceTerminationResponse>";

	char body[512];
	int bodylen;
	time_t uptime;

	uptime = ( time ( NULL ) - startup_time );
	bodylen = snprintf ( body, sizeof ( body ), resp, ( long ) uptime );
	BuildSendAndCloseSoapResp ( h, body, bodylen );
}

static void
RequestConnection ( struct upnphttp * h )
{
	static const char resp[] =
	    "<u:RequestConnectionResponse "
	    "xmlns:u=\"urn:schemas-upnp-org:service:WANIPConnection:1\">"
	    "</u:RequestConnectionResponse>";

	char body[512];
	int bodylen;
	time_t uptime;

	uptime = ( time ( NULL ) - startup_time );
	bodylen = snprintf ( body, sizeof ( body ), resp, ( long ) uptime );
	BuildSendAndCloseSoapResp ( h, body, bodylen );
}

static void
SetConnectionType(struct upnphttp * h, const char * action)
{
	const char * connection_type;
	struct NameValueParserData data;

	ParseNameValue(h->req_buf + h->req_contentoff, h->req_contentlen, &data);
	connection_type = GetValueFromNameValueList(&data, "NewConnectionType");
	/* Unconfigured, IP_Routed, IP_Bridged */
	ClearNameValueList(&data);
	/* always return a ReadOnly error */
	SoapError(h, 731, "ReadOnly");
}

/* Windows XP as client send the following requests :
 * GetConnectionTypeInfo
 * GetNATRSIPStatus
 * ? GetTotalBytesSent - WANCommonInterfaceConfig
 * ? GetTotalBytesReceived - idem
 * ? GetTotalPacketsSent - idem
 * ? GetTotalPacketsReceived - idem
 * GetCommonLinkProperties - idem
 * GetStatusInfo - WANIPConnection
 * GetExternalIPAddress
 * QueryStateVariable / ConnectionStatus!
 */
static const struct 
{
	const char * methodName; 
	void (*methodImpl)(struct upnphttp *, const char *);
}
soapMethods[] =
{
	{ "SetConnectionType", SetConnectionType},
	{ "GetConnectionTypeInfo", GetConnectionTypeInfo },
	{ "GetNATRSIPStatus", GetNATRSIPStatus},
	{ "GetExternalIPAddress", GetExternalIPAddress},
	{ "AddPortMapping", AddPortMapping},
	{ "DeletePortMapping", DeletePortMapping},
	{ "GetGenericPortMappingEntry", GetGenericPortMappingEntry},
	{ "GetSpecificPortMappingEntry", GetSpecificPortMappingEntry},
	{ "QueryStateVariable", QueryStateVariable},
	{ "GetTotalBytesSent", GetTotalBytesSent},
	{ "GetTotalBytesReceived", GetTotalBytesReceived},
	{ "GetTotalPacketsSent", GetTotalPacketsSent},
	{ "GetTotalPacketsReceived", GetTotalPacketsReceived},
	{ "GetCommonLinkProperties", GetCommonLinkProperties},
	{ "GetStatusInfo", GetStatusInfo},
#ifdef ENABLE_L3F_SERVICE
	{ "SetDefaultConnectionService", SetDefaultConnectionService},
	{ "GetDefaultConnectionService", GetDefaultConnectionService},
#endif
	{ "ForceTermination", ForceTermination},
	{ "RequestConnection", RequestConnection},
	{ 0, 0 }
};

void
ExecuteSoapAction(struct upnphttp * h, const char * action, int n)
{
	char * p;
	char * p2;
	int i, len, methodlen;

	i = 0;
	p = strchr(action, '#');

	if(p)
	{
		p++;
		p2 = strchr(p, '"');
		if(p2)
			methodlen = p2 - p;
		else
			methodlen = n - (p - action);
		/*syslog(LOG_DEBUG, "SoapMethod: %.*s", methodlen, p);*/
		while(soapMethods[i].methodName)
		{
			len = strlen(soapMethods[i].methodName);
			if(strncmp(p, soapMethods[i].methodName, len) == 0)
			{
				DEBUG_MSG("\n**************************************************\n");
				DEBUG_MSG("%s (%d): begin , %s !!\n",__FUNCTION__,__LINE__,soapMethods[i].methodName);
				DEBUG_MSG("\n**************************************************\n\n");
				soapMethods[i].methodImpl(h, soapMethods[i].methodName);
				return;
			}
			i++;
		}

		syslog(LOG_NOTICE, "SoapMethod: Unknown: %.*s", methodlen, p);
	}

	SoapError(h, 401, "Invalid Action");
}

/* Standard Errors:
 *
 * errorCode errorDescription Description
 * --------	---------------- -----------
 * 401 		Invalid Action 	No action by that name at this service.
 * 402 		Invalid Args 	Could be any of the following: not enough in args,
 * 							too many in args, no in arg by that name, 
 * 							one or more in args are of the wrong data type.
 * 403 		Out of Sync 	Out of synchronization.
 * 501 		Action Failed 	May be returned in current state of service
 * 							prevents invoking that action.
 * 600-699 	TBD 			Common action errors. Defined by UPnP Forum
 * 							Technical Committee.
 * 700-799 	TBD 			Action-specific errors for standard actions.
 * 							Defined by UPnP Forum working committee.
 * 800-899 	TBD 			Action-specific errors for non-standard actions. 
 * 							Defined by UPnP vendor.
*/
void
SoapError(struct upnphttp * h, int errCode, const char * errDesc)
{
	static const char resp[] = 
		"<s:Envelope "
		"xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\" "
		"s:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\">"
		"<s:Body>"
		"<s:Fault>"
		"<faultcode>s:Client</faultcode>"
		"<faultstring>UPnPError</faultstring>"
		"<detail>"
		"<UPnPError xmlns=\"urn:schemas-upnp-org:control-1-0\">"
		"<errorCode>%d</errorCode>"
		"<errorDescription>%s</errorDescription>"
		"</UPnPError>"
		"</detail>"
		"</s:Fault>"
		"</s:Body>"
		"</s:Envelope>";

	char body[2048];
	int bodylen;

	syslog(LOG_INFO, "Returning UPnPError %d: %s", errCode, errDesc);
	bodylen = snprintf(body, sizeof(body), resp, errCode, errDesc);
	BuildResp2_upnphttp(h, 500, "Internal Server Error", body, bodylen);
	SendResp_upnphttp(h);
	CloseSocket_upnphttp(h);
}

