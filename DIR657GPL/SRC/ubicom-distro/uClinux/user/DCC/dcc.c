#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <net/if.h>
#include <net/ethernet.h>
#include <arpa/inet.h>
#include <nvram.h>
#include <sutil.h>
#include <shvar.h>
#include "dcc.h"
#include <linux_vct.h>

//#define DCC_DEBUG 1
#ifdef DCC_DEBUG
#define DEBUG_MSG(fmt, arg...)       printf("%s [%d]: "fmt, __FILE__, __LINE__, ##arg)
#else
#define DEBUG_MSG(fmt, arg...)
#endif

#define AUTH_RUN_TIMER 1
#ifdef AUTH_RUN_TIMER
#include <sys/time.h>
static int auth_check_if=0;

struct support_login {
        unsigned long log_ip;
	unsigned long sticks;
        unsigned int auth_if;
        char clientMAC[20];
};
#define S_LOGIN_MAX 10
#define S_LOGIN_MAINTAIN 3*3
struct support_login sp_login[S_LOGIN_MAX];
struct support_login *sp_login_p;

#endif

#define RESOLVCONF				"/var/etc/resolv.conf"
#define reboot()	kill(1, SIGTERM);

char szDebug[64];
unsigned char g_nWANType = WAN_TYPE_UNKNOWN;
static char *parse_mac(char *mac_addr, char *parse);

__inline unsigned short bswap(unsigned short u)
{
	/* Big Endian */
	return ((((u) >> 8) & 0xff) | (((u) << 8) & 0xff00));

	/* Little Endian */
//	return u;
}

__inline void Extract2Chars(unsigned char* pch, const short int n)
{
	*pch = (unsigned short) (n & 0x00ff);
	*(pch + 1) = (unsigned short) ((n >> 8) & 0x00ff);
}

__inline void ExtractShortInt(const unsigned char* pch, unsigned short int *pui)
{
	*pui = (unsigned short) (*pch) & 0x00ff;
	*pui |= (((unsigned short) (*(pch + 1))) & 0x00ff) << 8L;
}

/* transfer hex to 10 */
int hexTO10(char *hex_1,char *hex_0)
{
	int hexType_c;
	char 	hexType_lower[16]={'0','1','2','3','4','5','6','7','8','9','a','b','c','d','e','f'};
	char 	hexType_upper[16]={'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'};
	int hexTypeMap[16]={0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15};
	char 	*hexTypeLower_ptr = hexType_lower;
	char 	*hexTypeUpper_ptr = hexType_upper;
	int 	h_1 = -1;
	int 	h_0 = -1;

	for(hexType_c = 0;hexType_c < 16;hexType_c++)
	{
		if(*(hexTypeLower_ptr+hexType_c) == *hex_1 || *(hexTypeUpper_ptr+hexType_c) == *hex_1)
		{
			h_1 = hexTypeMap[hexType_c];
			break;
		}
	}

	for(hexType_c = 0;hexType_c < 16;hexType_c++)
	{
		if(*(hexTypeLower_ptr+hexType_c) == *hex_0 || *(hexTypeUpper_ptr+hexType_c) == *hex_0)
		{
			h_0 = hexTypeMap[hexType_c];
			break;
		}
	}

	return (h_1*16 + h_0);
}

char* ascii2hex(char *nvram_wepkey_value, char *returnValue)
{
	int i;
	//char tmp_key[256] = {};

	for(i =0 ; i < strlen(nvram_wepkey_value); i++)
		sprintf(returnValue, "%s%x", returnValue,*(nvram_wepkey_value + i));

	return returnValue;
}

int nvram_set_n(const char *target, const unsigned char *value, int len)
{
	char *tmp_value;
	int ret=-1;

	tmp_value = (char *) malloc(len+1);
	memset(tmp_value, 0, len+1);
	strncpy(tmp_value, value, len);

	ret = nvram_set(target, tmp_value);
	free(tmp_value);
	return  ret;

}



char *read_dns(int dns_num)
{
	FILE *fp;
	char *buff;
	static char dns[20];
	int num, i=0, len=0;

	fp = fopen (RESOLVCONF, "r");
	if(!fp) {
		perror(RESOLVCONF);
		return 0;
	}
	buff = (char *) malloc(1024);
	memset(buff, 0, 1024);
	while( fgets(buff, 1024, fp)) {
		if (strstr(buff, "nameserver") ) {
			num = strspn(buff+10, " ");
			len = strlen( (buff + 10 + num) );
			if ( i == dns_num) {
				strncpy(dns, (buff + 10 + num), len-1+i );  // +i is for \n case  (first line)
				break;
			}
			i++;
		}
	}

	free(buff);
	fclose(fp);
	return dns;
}


/*
 *  *	create command header
 */
void CreateHeader(unsigned char *pchBuf,
		const unsigned short cmd, const short ret_code, const unsigned short client_id, const unsigned short seq_no)
{
	PPACKETHEADER pPkt = (PPACKETHEADER) pchBuf;

	assert(pchBuf != NULL);

	pPkt->cmd = bswap(cmd);
	pPkt->ret_code = bswap(ret_code);
	pPkt->client_id = bswap(client_id);
	if (seq_no == 0xffff) {
		pPkt->seq_no = bswap(0x0001);
	} else {
		pPkt->seq_no = bswap(seq_no + 1);
	}
}


/*
 *	appends a tag or TLV set to the QuickConfig packet
 */
int AppendTag(unsigned char* pchBuf, int* pnbBufLen,
		const unsigned short nValueType, const void* pvValue, const int nbValueLen)
{
	unsigned char *pch;

	assert(pchBuf != NULL && pnbBufLen != NULL && pvValue != NULL);

	if ((*pnbBufLen + QC_TAG_HDR_LEN + nbValueLen) > MAX_BUF_SIZE) {
		DEBUG_MSG("Quick Config: packet length is too large...");
		return FALSE;
	}

	pch = pchBuf + *pnbBufLen;

	Extract2Chars(pch, nValueType);
	pch += sizeof(short int);
	Extract2Chars(pch, nbValueLen);
	pch += sizeof(short int);

	*pnbBufLen += QC_TAG_HDR_LEN;

	memcpy(pch, pvValue, nbValueLen);
	*pnbBufLen += nbValueLen;

	return TRUE;
}


int HandleDiscoveryCommand(const unsigned char *pchBuf, const int nbLen)
{
	//int i;
	int nbValueLen;
	int PassLen;		//by sandy
	const unsigned char * pkchValue;
	PKTAGHEADER pkTagHdr;
	assert(pchBuf != NULL);

	if (nbLen < (QC_PKT_HDR_LEN + QC_TAG_HDR_LEN)) {
		return QC_DIS_ERR;
	}
	pkTagHdr = (PKTAGHEADER) (pchBuf + QC_PKT_HDR_LEN);

	if (bswap(pkTagHdr->type) != TYPE_PASSWORD) {
		//return QC_DIS_AUTH_ERR;
	}

	nbValueLen = pkTagHdr->len;
	pkchValue = pchBuf + QC_PKT_HDR_LEN + QC_TAG_HDR_LEN;

	/* TODO - decrypt password here with RC4 */
	/*
		for (i = 0; i < nbValueLen; ++i) {
		if (cfg.device.man_pass[0].pass[i] != pkchValue[i])  {
		return QC_DIS_AUTH_ERR;
		}
		}
		*/
	/*	if (MAN_PASS_LEN < nbValueLen)
		{
	//return QC_DIS_AUTH_ERR;
	}
	*/
	//if (strncmp ((char*) cfg.device.man_pass[0].pass, (char*) pkchValue, MAN_PASS_LEN) != 0)
	//if (strncmp ((char*) cfg.device.man_pass[0].pass, DEFAULT_PASSWORD, MAN_PASS_LEN) != 0)

	//PassLen = strlen((char *) cfg.device.man_pass[0].pass);
	PassLen = strlen( nvram_get("admin_password"));

	//if (strncmp((char *) cfg.device.man_pass[0].pass, (char *) pkchValue, PassLen) != 0) //change by sandy
	if( strncmp( nvram_get("admin_password"), (char *)pkchValue,  PassLen)  != 0 ) //change by sandy
	{
		sprintf(szDebug, "No pass! password:\n");
		//PutLogMsg(MLOG_DEBUG, szDebug, "QC");
		DEBUG_MSG("%s\n", szDebug);

		return QC_DIS_AUTH_ERR;
	}

	return QC_SUCCESS;
}


int HandleRegisterCommand(const unsigned char *pchBuf, const int nbLen)
{
	PTAGHEADER pTagHdr = NULL;
	unsigned char fDhcpClient;
	char *WanProto;

	assert(pchBuf != NULL);

	if (nbLen < (QC_PKT_HDR_LEN + QC_TAG_HDR_LEN + sizeof(unsigned char)))
		return QC_REG_ERR;

	pTagHdr = (PTAGHEADER) (pchBuf + QC_PKT_HDR_LEN);

	// client must tell us is PC DHCP or static
	// if client doesn't send this information, send error
	if (bswap(pTagHdr->type) != TYPE_PC_DHCP_OR_NOT)
		return QC_REG_PC_DHCP_ERR;
	// GetIFSType by sandy
	// respone DCC WAN setting by web setting, not by true internet detecting
#if 0
	GetIFSType();
	return QC_SUCCESS;
#endif

	// by Savy Code
	// respone DCC WAN setting by true internet detecting

#if 1
	fDhcpClient = *(pchBuf + QC_PKT_HDR_LEN + QC_TAG_HDR_LEN);

	WanProto =  nvram_get("wan_proto");


	if ( !strncmp( WanProto, "dhcpc", 5) ) {
		g_nWANType = WAN_TYPE_DHCP;
	} else if ( !strncmp( WanProto, "static", 6) ) {
		g_nWANType = WAN_TYPE_STATIC_IP;
	} else if ( !strncmp( WanProto, "pppoe", 5) ) {
		if (fDhcpClient)
			g_nWANType = WAN_TYPE_DYNAMIC_PPPOE;
		else
			g_nWANType = WAN_TYPE_STATIC_PPPOE;
	} else if ( !strncmp( WanProto, "pptp", 4) ) {
		if( !nvram_match("wan_l2tp_dynamic", "1") )
			g_nWANType = WAN_TYPE_DYNAMIC_PPTP;
		else
			g_nWANType = WAN_TYPE_STATIC_PPTP;

	} else if ( !strncmp( WanProto, "l2tp", 4) ) {
		if( !nvram_match("wan_l2tp_dynamic", "1") )
			g_nWANType = WAN_TYPE_DYNAMIC_L2TP;
		else
			g_nWANType = WAN_TYPE_STATIC_L2TP;

	} else if ( !strncmp( WanProto, "bigpond", 7) ) {
		g_nWANType = WAN_TYPE_BIGPOND;

	} else {

		if (fDhcpClient)
			g_nWANType = WAN_TYPE_UNKNOWN;
		else
			g_nWANType = WAN_TYPE_STATIC_IP;
	}


	sprintf(szDebug, "WAN Port Type: %d", g_nWANType);
	//PutLogMsg(MLOG_DEBUG, szDebug, "QC");
	DEBUG_MSG("%s\n", szDebug);

	return QC_SUCCESS;
#endif

}

/*
* Date: 2008-01-16
* Name: NickChou
* Detail: Add list_channels() and ieee80211_mhz2ieee() for wireless domanin is not 0x10 or 0x30.
		  So I use IOCTL to get wireless channel from wireless driver.
		  Modify from "wlanconfig ath0 list chan" command.
*/
int ieee80211_mhz2ieee(int freq)
{
	if (freq == 2484)
		return 14;

	if (freq < 2484)
		return (freq - 2407) / 5;

	if (freq < 5000)
		return 15 + ((freq - 2512) / 20);

	return (freq - 5000) / 5;
}

static void list_channels(const char *ifname, char *channel_list)
{
	struct ieee80211req_chaninfo chans;
	int i, half, len, skfd;
	struct iwreq wrq;
	char channel_1[64]={0};
	char channel_2[64]={0};
	int channel=0;

	if(ifname==NULL || strlen(ifname)==0)
	{
		printf("DCC: list_channels: ifname is NULL\n");
		return;
	}

	/* Create a channel to the NET kernel. */
	skfd = socket(AF_INET, SOCK_DGRAM, 0);
	if(skfd < 0)
	{
		perror("list_channels");
		close(skfd);
		return;
	}

	len = sizeof(chans);
	DEBUG_MSG("DCC: list_channels: Create Socket: OK: ifname=%s\n", ifname);

	/* Set device name */
	strncpy(wrq.ifr_name, ifname, IFNAMSIZ);

	/*
	 * Argument data too big for inline transfer; setup a
	 * parameter block instead; the kernel will transfer
	 * the data for the driver.
	 */
	wrq.u.data.pointer = &chans;
	wrq.u.data.length = len;

	/* Do the request */
	if( ioctl(skfd, 0x8BE7/*IEEE80211_IOCTL_GETCHANINFO*/, &wrq) >= 0) //(SIOCIWFIRSTPRIV+7)
	{
		half = chans.ic_nchans / 2;
		DEBUG_MSG("DCC: list_channels: number of channel info=%d\n", chans.ic_nchans);

		if (chans.ic_nchans % 2)
			half++;

		for (i = 0; i < chans.ic_nchans / 2; i++)
		{
			channel = ieee80211_mhz2ieee(chans.ic_chans[i].ic_freq);
			if( (channel<52) || (channel>140) ) /*NickChou: DFS Channel = 52~140*/
				sprintf(channel_1, "%s%d,", channel_1, channel);

			channel = ieee80211_mhz2ieee(chans.ic_chans[half+i].ic_freq);
			if( (channel<52) || (channel>140) ) /*NickChou: DFS Channel = 52~140*/
				sprintf(channel_2, "%s%d,", channel_2, channel);
		}

		DEBUG_MSG("DCC: list_channels: channel_1: %s\n", channel_1);
		DEBUG_MSG("DCC: list_channels: channel_2: %s\n", channel_2);

		if (chans.ic_nchans % 2) {
			channel = ieee80211_mhz2ieee(chans.ic_chans[i].ic_freq);
			if( (channel<52) || (channel>140) ) /*NickChou: DFS Channel = 52~140*/
				sprintf(channel_1, "%s%d,", channel_1, channel);
		}

		sprintf(channel_list, "%s%s", channel_1, channel_2);

		DEBUG_MSG("DCC: list_channels: final channel_list=%s\n", channel_list);
	}
	else
	{
		printf("DCC:list_channels: ioctl :IEEE80211_IOCTL_GETCHANINFO: error\n");
	}

  	close(skfd);
  	return;

}


int HandleGetCommand(const unsigned char *pchRecvBuf, const int nbRecvLen, unsigned char *pchSendBuf, int *pnbSendLen)
{
	int i, j;
	int nTag = 0;
	PTAGHEADER pTagHdr = NULL;
	char *Wan_eth=NULL, *WanProto=NULL;
	char *ul = NULL;
	char encryption_type;
	char encryption_type2;
	char value_tmp[64];
	int _10code;
	int total_len = nbRecvLen - QC_PKT_HDR_LEN;
	int count=0;
	int tag_len = QC_TAG_HDR_LEN;
	char *ConnectState = NULL;

	if (nbRecvLen < (QC_PKT_HDR_LEN + QC_TAG_HDR_LEN)){
		printf("%s: RecvLen Fail!",__func__);
		return QC_GET_ERR;
	}

	nTag = (nbRecvLen - QC_PKT_HDR_LEN) / QC_TAG_HDR_LEN;

	if (nTag == 0){
		printf("%s: Tag Fail!",__func__);
		return QC_GET_ERR;
	}
	pTagHdr = (PTAGHEADER) (pchRecvBuf + QC_PKT_HDR_LEN);
	DEBUG_MSG("GET Tag :%d \n", nTag);
	while(total_len != 0){

		switch ( bswap(pTagHdr->type) ) {
			/*
			 * MUST items: WAN Port Type & Status Information
			 */
			case TYPE_WAN_STATUS:
			{
				DEBUG_MSG("TYPE_WAN_STATUS \n");
				char fConnected = 1;

				/* not support multi-pppoe yet*/
				if( !nvram_match("wan_proto", "static") || !nvram_match("wan_proto", "dhcpc") )
					Wan_eth = nvram_get("wan_eth");
				else
					Wan_eth = "ppp0";

				fConnected = wan_connected_cofirm(Wan_eth);
				if (AppendTag(pchSendBuf, pnbSendLen, TYPE_WAN_STATUS, &fConnected, sizeof(char)) == FALSE) {
					return QC_BUF_ERR;
				}
				break;
			}
			case TYPE_IFS_1_TYPE:
			{
				DEBUG_MSG("TYPE_IFS_1_TYPE \n");
				g_nWANType = WAN_TYPE_UNKNOWN;
				WanProto =  nvram_get("wan_proto");
				if ( !strncmp( WanProto, "dhcpc", 5) ) {
					g_nWANType = WAN_TYPE_DHCP;
				} else if ( !strncmp( WanProto, "static", 6) ) {
					g_nWANType = WAN_TYPE_STATIC_IP;
				} else if ( !strncmp( WanProto, "pppoe", 5) ) {
					g_nWANType = WAN_TYPE_DYNAMIC_PPPOE;
					//g_nWANType = WAN_TYPE_STATIC_PPPOE;
				} else if ( !strncmp( WanProto, "pptp", 4) ) {
					if( !nvram_match("wan_l2tp_dynamic", "1") )
						g_nWANType = WAN_TYPE_DYNAMIC_PPTP;
					else
						g_nWANType = WAN_TYPE_STATIC_PPTP;

				} else if ( !strncmp( WanProto, "l2tp", 4) ) {
					if( !nvram_match("wan_l2tp_dynamic", "1") )
						g_nWANType = WAN_TYPE_DYNAMIC_L2TP;
					else
						g_nWANType = WAN_TYPE_STATIC_L2TP;
				} else if ( !strncmp( WanProto, "bigpond", 7) ) {
					g_nWANType = WAN_TYPE_BIGPOND;
				} else {
					g_nWANType = WAN_TYPE_STATIC_IP;
				}
				if (AppendTag(pchSendBuf, pnbSendLen,	TYPE_IFS_1_TYPE, (void*) &g_nWANType, UC_LEN) == FALSE)
					return QC_BUF_ERR;
				break;
			}
			/* Wireless Info */
			case TYPE_WLAN_DOMAIN:
			{
				DEBUG_MSG("TYPE_WLAN_DOMAIN \n");
				char *wlanDomain;
				unsigned short domain;
#ifdef SET_GET_FROM_ARTBLOCK
				if(!(wlanDomain = artblock_get("wlan0_domain")))
				wlanDomain = nvram_safe_get("wlan0_domain");
#else
				wlanDomain = nvram_safe_get("wlan0_domain");
#endif
				if(!strcmp(wlanDomain, "0x10"))
					domain=0x10;
				else if(!strcmp(wlanDomain, "0x30"))
					domain=0x30;
				if (AppendTag(pchSendBuf, pnbSendLen,
							TYPE_WLAN_DOMAIN, &domain, sizeof(unsigned short)) == FALSE)
					return QC_BUF_ERR;
				break;
			}
//SP 2.0
			case TYPE_WLAN_MODE:
			{
				/* ken_chiang
					The wirelee one is assumption runing in 2.4G so 11n mode is response value 3
					not 7. if your wireless is runing in 5G you must to chang the response value.
					0: b/g/n - 1: b only - 2. g only - 3. n only (2.4GHz) - 4: b/g mixed
					- 5: g/n mixed - 6: a/n - 7. n only (5GHz)- 8. a only
				*/
				DEBUG_MSG("TYPE_WLAN_MODE \n");
				char *wlan_mode;
				char mode;
				wlan_mode = nvram_get("wlan0_dot11_mode");
				if (!strcmp(wlan_mode, "11bgn"))
					mode = 0;
				else if (!strcmp(wlan_mode, "11b"))
					mode = 1;
				else if (!strcmp(wlan_mode, "11g"))
					mode = 2;
				else if (!strcmp(wlan_mode, "11n"))
					mode = 3;
				else if (!strcmp(wlan_mode, "11bg"))
					mode = 4;
				else if (!strcmp(wlan_mode, "11gn"))
					mode = 5;
				else if (!strcmp(wlan_mode, "11na"))
					mode = 6;
				//else if (!strcmp(wlan_mode, "11n"))
					//mode = 7;
				else if (!strcmp(wlan_mode, "11a"))
					mode = 8;

				if (AppendTag(pchSendBuf, pnbSendLen,
							TYPE_WLAN_MODE, &mode, sizeof(char)) == FALSE)
					return QC_BUF_ERR;
				break;
			}
			case TYPE_WLAN_CHANNEL_LIST:
			{
				DEBUG_MSG("TYPE_WLAN_CHANNEL_LIST \n");
				char *wlanDomain;
				char *channel;
				char channel_list[64]={0};
#ifdef SET_GET_FROM_ARTBLOCK
				if(!(wlanDomain = artblock_get("wlan0_domain")))
				wlanDomain = nvram_safe_get("wlan0_domain");
#else
				wlanDomain = nvram_safe_get("wlan0_domain");
#endif
				if(!strcmp(wlanDomain, "0x10"))
					channel="1,2,3,4,5,6,7,8,9,10,11,";
				else if(!strcmp(wlanDomain, "0x30"))
					channel="1,2,3,4,5,6,7,8,9,10,11,12,13,";
				else
				{
					DEBUG_MSG("TYPE_WLAN_CHANNEL_LIST: using IOCTL to get channel list\n");
#ifdef AR7161
					if(nvram_match("wlan0_enable", "1")==0)
						list_channels(nvram_safe_get("wlan0_eth"), &channel_list[0]);
					else
						DEBUG_MSG("TYPE_WLAN_CHANNEL_LIST: 2.4GHz is disable\n");
#else
					if(nvram_match("wlan0_enable", "1")==0)
						list_channels(nvram_safe_get("wlan0_eth"), &channel_list[0]);
#endif
					channel=&channel_list[0];
					DEBUG_MSG("TYPE_WLAN_CHANNEL_LIST: channel list=%s\n", channel);
				}

				if (AppendTag(pchSendBuf, pnbSendLen,
							TYPE_WLAN_CHANNEL_LIST, channel, strlen(channel)+1) == FALSE)
					return QC_BUF_ERR;
				break;
			}
			/* Interface Information */
			case TYPE_IFS_1_IP:
			{
				/* For Fixed IP */
				DEBUG_MSG("TYPE_IFS_1_IP \n");
				ul =  nvram_get("wan_static_ipaddr") ;
				if (AppendTag(pchSendBuf, pnbSendLen, TYPE_IFS_1_IP, (void*) ul, strlen(ul)) == FALSE)
					return QC_BUF_ERR;
				break;
			}
			case TYPE_IFS_1_MASK:
			{
				/* For Fixed IP */
				DEBUG_MSG("TYPE_IFS_1_MASK \n");
				ul =  nvram_get("wan_static_netmask") ;
				if (AppendTag(pchSendBuf, pnbSendLen, TYPE_IFS_1_IP, (void*) ul, strlen(ul)) == FALSE)
					return QC_BUF_ERR;
				break;
			}
			case TYPE_IFS_1_GATEWAY:
			{
				/* For Fixed IP */
				DEBUG_MSG("TYPE_IFS_1_GATEWAY \n");
				ul =  nvram_get("wan_static_gateway") ;
				if (AppendTag(pchSendBuf, pnbSendLen, TYPE_IFS_1_IP, (void*) ul, strlen(ul)) == FALSE)
					return QC_BUF_ERR;
				break;
			}
			case TYPE_IFS_1_MTU:
			{
				DEBUG_MSG("TYPE_IFS_1_MTU \n");
				break;
			}
			case TYPE_IFS_1_DNS_1:
			{
				DEBUG_MSG("TYPE_IFS_1_DNS_1 \n");
				ul = read_dns(0);
				if (AppendTag(pchSendBuf, pnbSendLen,	TYPE_IFS_1_DNS_1, (void*) ul, strlen(ul)) == FALSE)
					return QC_BUF_ERR;
				break;
			}
			case TYPE_IFS_1_DNS_2:
			{
				DEBUG_MSG("TYPE_IFS_1_DNS_2 \n");
				ul = read_dns(1);
				if (AppendTag(pchSendBuf, pnbSendLen,	TYPE_IFS_1_DNS_1, (void*) ul, strlen(ul)) == FALSE)
					return QC_BUF_ERR;
				break;
			}
			/* Other Trivial Information */
			case TYPE_HWID:
			{
				DEBUG_MSG("TYPE_HWID \n");
				break;
			}
			case TYPE_MODELNAME:
			{
				DEBUG_MSG("TYPE_MODELNAME \n");
				if (AppendTag(pchSendBuf, pnbSendLen,	TYPE_MODELNAME, (void*) nvram_safe_get("model_number"), strlen(nvram_safe_get("model_number"))) == FALSE)
					return QC_BUF_ERR;
				break;
			}
			case TYPE_MAC:
			{
				DEBUG_MSG("TYPE_MAC \n");
				break;
			}
			case TYPE_IFS_1_SERVICES_NAME:
			{
				DEBUG_MSG("TYPE_IFS_1_SERVICES_NAME \n");
				break;
			}
			case TYPE_IFS_1_IDLE_TIME:
			{
				DEBUG_MSG("TYPE_IFS_1_IDLE_TIME \n");
				break;
			}
			case TYPE_IFS_1_USERNAME:
			{
				char *UserName = NULL;
				DEBUG_MSG("TYPE_IFS_1_USERNAME \n");
				/* not support multi-pppoe */
				if ( !nvram_match("wan_proto", "pppoe") )
					UserName = nvram_get("wan_pppoe_username_00");
				else if ( !nvram_match("wan_proto", "pptp") )
					UserName = nvram_get("wan_pptp_username");
				else if ( !nvram_match("wan_proto", "l2tp") )
					UserName = nvram_get("wan_l2tp_username");
				else if ( !nvram_match("wan_proto", "bigpond") )
					UserName = nvram_get("wan_bigpond_username");

				if (AppendTag(pchSendBuf, pnbSendLen,	TYPE_IFS_1_USERNAME, (void*) UserName, strlen(UserName)) == FALSE)
					return QC_BUF_ERR;
				break;
			}
			case TYPE_IFS_1_PASSWORD:
			{
				char *PassWord = NULL;
				DEBUG_MSG("TYPE_IFS_1_PASSWORD \n");
				/* not support multi-pppoe */
				if ( !nvram_match("wan_proto", "pppoe") )
					PassWord = nvram_get("wan_pppoe_password_00");
				else if ( !nvram_match("wan_proto", "pptp") )
					PassWord = nvram_get("wan_pptp_password");
				else if ( !nvram_match("wan_proto", "l2tp") )
					PassWord = nvram_get("wan_l2tp_password");
				else if ( !nvram_match("wan_proto", "bigpond") )
					PassWord = nvram_get("wan_pptp_password");

				if (AppendTag(pchSendBuf, pnbSendLen,	TYPE_IFS_1_PASSWORD, (void*) PassWord, strlen(PassWord)) == FALSE)
					return QC_BUF_ERR;
				break;
			}
			case TYPE_PPTP_SERVER_IP:
			{
				ul = nvram_safe_get("wan_pptp_server_ip");
				if(strlen(ul) == 0)
					ul = "0.0.0.0";
				if (AppendTag(pchSendBuf, pnbSendLen,	TYPE_PPTP_SERVER_IP, (void*) ul, strlen(ul)) == FALSE)
					return QC_BUF_ERR;
				break;
			}
			case TYPE_L2TP_SERVER_IP:
			{
				ul = nvram_safe_get("wan_l2tp_server_ip");
				if(strlen(ul) == 0)
					ul = "0.0.0.0";
				if (AppendTag(pchSendBuf, pnbSendLen,	TYPE_L2TP_SERVER_IP, (void*) ul, strlen(ul)) == FALSE)
					return QC_BUF_ERR;
				break;
			}
			case TYPE_IFS_1_ON_DEMAND:
			{
				DEBUG_MSG("TYPE_IFS_1_ON_DEMAND \n");
				break;
			}
			case TYPE_IFS_1_AUTO_RECONNECT:
			{
				DEBUG_MSG("TYPE_IFS_1_AUTO_RECONNECT \n");
				break;
			}
			case TYPE_DETECT_ETHERNET_CABLE_CONNECTED:
			{
				short auto_detect_wan;
				char status[15] = {0};
				TAGHEADER_ETH *ethHdrTag;
				ethHdrTag = (TAGHEADER_ETH *)pTagHdr;

				if (ConnectState != NULL)
					ethHdrTag->val = *ConnectState;
				DEBUG_MSG("pTagHdr->val = %d \n", ethHdrTag->val);
				if (ethHdrTag->val == 4)
				{
					VCTGetPortConnectState(nvram_get("wan_eth"),VCTWANPORT0,status);
					if( strncmp("disconnect", status, 10) == 0)
						auto_detect_wan = 0;
					else
						auto_detect_wan = 1;
				}
				else
				{
					VCTGetPortConnectState(nvram_get("lan_eth"),VCTLANPORT0+ethHdrTag->val,status);
					if( strncmp("disconnect", status, 10) == 0)
						auto_detect_wan = 0;
				else
						auto_detect_wan = 1;
				}

			  	DEBUG_MSG("TYPE_AUTODETECT_WAN  %d\n", auto_detect_wan);

				if (AppendTag(pchSendBuf, pnbSendLen, TYPE_DETECT_ETHERNET_CABLE_CONNECTED, &auto_detect_wan, sizeof(short)) == FALSE) {
					return QC_BUF_ERR;
				}
				tag_len = QC_TAG_HDR_LEN + 1;
				break;
			}
			/* Wireless Info */
			case TYPE_WLAN_SSID:
			{
				DEBUG_MSG("TYPE_WLAN_SSID \n");
				char *WlanSSID;
				WlanSSID = nvram_get("wlan0_ssid");
				if (AppendTag(pchSendBuf, pnbSendLen,	TYPE_WLAN_SSID, WlanSSID, strlen(WlanSSID)+1) == FALSE) {
					return QC_BUF_ERR;
				}
				break;
			}
			case TYPE_WLAN_CHANNEL:
			{
				DEBUG_MSG("TYPE_WLAN_CHANNEL \n");
				char ch;
				ch = atoi(nvram_safe_get("wlan0_channel"));
				if (AppendTag(pchSendBuf, pnbSendLen,	TYPE_WLAN_CHANNEL, &ch, sizeof(char)) == FALSE) {
					return QC_BUF_ERR;
				}
				break;
			}
			//case TYPE_AUTOCHANSELECT_MODE:
			case TYPE_WLAN_AUTOCHANSELECT_MODE://SP 2.0
			{
				DEBUG_MSG("TYPE_AUTOCHANSELECT_MODE \n");
				char auto_channel;
				if(nvram_match("wlan0_auto_channel_enable", "1") == 0)
					auto_channel = 0x1;
				else
					auto_channel = 0x0;
				if (AppendTag(pchSendBuf, pnbSendLen,	TYPE_WLAN_AUTOCHANSELECT_MODE, &auto_channel, sizeof(char)) == FALSE) {
					return QC_BUF_ERR;
				}
				break;
			}
			case TYPE_WLAN_ENCRYPTION_TYPE:
			{
				DEBUG_MSG("TYPE_WLAN_ENCRYPTION_TYPE \n");
				char *sec_type;

				sec_type = nvram_get("wlan0_security");
				if(strcmp(sec_type, "wpa_psk") == 0){
				encryption_type = AUTHTYPE_WPAPSK;
				}else if(strcmp(sec_type, "wep_open_64") == 0){
					encryption_type = AUTHTYPE_WEP64;
				}else if (strcmp(sec_type, "wpa2auto_psk") == 0){
					encryption_type = AUTHTYPE_WPAPSK;
				}
				if (AppendTag(pchSendBuf, pnbSendLen,
							TYPE_WLAN_ENCRYPTION_TYPE, &encryption_type,
								sizeof(char)) == FALSE)
					return QC_BUF_ERR;
				break;
			}
/*
* Date: 2008-01-19
* Name: NickChou
* Detail: DCC 2.0 allow to get key
*/
			case TYPE_WLAN_KEY:
			{
				DEBUG_MSG("TYPE_WLAN_KEY \n");
				char tmp_wep_key[128] = {};
				char *tmp_key;
				if(encryption_type == AUTHTYPE_WPAPSK){
					tmp_key = nvram_get("wlan0_psk_pass_phrase");
					strncpy(tmp_wep_key, tmp_key, strlen(tmp_key));
				}else if(encryption_type == AUTHTYPE_WEP64){
					tmp_key = nvram_get("wlan0_wep64_key_1");
					memset(value_tmp, 0, sizeof(value_tmp));
					for(j = 0; j < strlen(tmp_key); j = j + 2){
						_10code = hexTO10(tmp_key + j, (tmp_key + j + 1));
						if(_10code < 0 || _10code > 128)
						{
							DEBUG_MSG("hexto10 exceeds ASCII code\n");
							_10code =0;
						}
						sprintf(value_tmp, "%s%c", value_tmp, _10code);
					}
					strncpy(tmp_wep_key, value_tmp, strlen(value_tmp));
					DEBUG_MSG("tmp_wep_key:%s\nlen is %d\n", tmp_wep_key, strlen(tmp_wep_key));
				}
				if (AppendTag(pchSendBuf, pnbSendLen,
							TYPE_WLAN_KEY, tmp_wep_key,
							strlen(tmp_wep_key)) == FALSE) {
					return QC_BUF_ERR;
				}
				break;
			}
			case TYPE_SUPERG_MODE:
			{
				DEBUG_MSG("TYPE_SUPERG_MODE \n");
				unsigned short tmp_SuperG;
				// if no support SuperG; send 0xff
				tmp_SuperG = 0xff;
				if (AppendTag(pchSendBuf, pnbSendLen, TYPE_SUPERG_MODE, &tmp_SuperG, sizeof(short)) == FALSE)
					return QC_BUF_ERR;
				break;
			}
//SP 2.0
			case TYPE_WLAN_MODE_CAPABILITY:
			{
				/* ken_chiang
				The option is to indicate the wireless capability and the default value
				   is 1(bgn) if your wirless is other type you must to used #define or
				   new nvram value like wlan0_cpanility=0(bg) to cange the response value.
				0: 11b/g - 1: 11b/g/n - 2. 11a/n - 3. 11a - 4. a/b/g/n
				*/
				DEBUG_MSG("TYPE_WLAN_MODE_CAPABILITY \n");
				char mode=1;
				if (AppendTag(pchSendBuf, pnbSendLen,
							TYPE_WLAN_MODE_CAPABILITY, &mode, sizeof(char)) == FALSE)
					return QC_BUF_ERR;
				break;
			}
			case TYPE_WLAN_NUMBER:
			{
				char wlan_number;
				if(nvram_safe_get("wlan1_enable"))
					wlan_number = 0x2;
				else
					wlan_number = 0x1;
				if (AppendTag(pchSendBuf, pnbSendLen,	TYPE_WLAN_NUMBER, &wlan_number, sizeof(char)) == FALSE) {
					return QC_BUF_ERR;
				}
				break;
			}
//5G interface 2
			case TYPE_WLAN2_SSID:
			{
				DEBUG_MSG("TYPE_WLAN2_SSID \n");
				char *WlanSSID;
				if(nvram_safe_get("wlan1_ssid")){
					WlanSSID = nvram_get("wlan1_ssid");
					if (AppendTag(pchSendBuf, pnbSendLen,	TYPE_WLAN2_SSID, WlanSSID, strlen(WlanSSID)+1) == FALSE) {
						return QC_BUF_ERR;
					}
				}
				else{
					return QC_TYPE_ERR;
				}
				break;
			}
			case TYPE_WLAN2_CHANNEL:
			{
				DEBUG_MSG("TYPE_WLAN2_CHANNEL \n");
				char ch;
				if(nvram_get("wlan1_channel"))
				{
					ch = atoi(nvram_get("wlan1_channel"));
					if (AppendTag(pchSendBuf, pnbSendLen,	TYPE_WLAN2_CHANNEL, &ch, sizeof(char)) == FALSE) {
						return QC_BUF_ERR;
					}
					break;
				}
				else{
					return QC_TYPE_ERR;
				}
			}
			case TYPE_WLAN2_ENCRYPTION_TYPE:
			{
				DEBUG_MSG("TYPE_WLAN2_ENCRYPTION_TYPE \n");
				char *sec_type;
				if(nvram_safe_get("wlan1_security")){
					sec_type = nvram_get("wlan1_security");
					if(strcmp(sec_type, "wpa_psk") == 0){
						encryption_type2 = AUTHTYPE_WPAPSK;
					}else if(strcmp(sec_type, "wep_open_64") == 0){
						encryption_type2 = AUTHTYPE_WEP64;
					}else if (strcmp(sec_type, "wpa2auto_psk") == 0){
						encryption_type2 = AUTHTYPE_WPAPSK;
					}
					if (AppendTag(pchSendBuf, pnbSendLen,
							TYPE_WLAN2_ENCRYPTION_TYPE, &encryption_type2,
								sizeof(char)) == FALSE)
						return QC_BUF_ERR;
				}
				else{
					return QC_TYPE_ERR;
				}
				break;
			}
/*
* Date: 2008-01-19
* Name: NickChou
* Detail: DCC 2.0 allow to get key
*/
			case TYPE_WLAN2_KEY:
			{
				DEBUG_MSG("TYPE_WLAN2_KEY \n");
				char tmp_wep_key[128] = {};
				char *tmp_key;
				if(nvram_safe_get("wlan1_wep64_key_1")){
					if(encryption_type2 == AUTHTYPE_WPAPSK){
						tmp_key = nvram_get("wlan1_psk_pass_phrase");
						strncpy(tmp_wep_key, tmp_key, strlen(tmp_key));
					}else if(encryption_type2 == AUTHTYPE_WEP64){
						tmp_key = nvram_get("wlan1_wep64_key_1");
						memset(value_tmp, 0, sizeof(value_tmp));
						for(j = 0; j < strlen(tmp_key); j = j + 2){
							_10code = hexTO10(tmp_key + j, (tmp_key + j + 1));
							if(_10code < 0 || _10code > 128)
							{
								DEBUG_MSG("hexto10 exceeds ASCII code\n");
								_10code =0;
							}
							sprintf(value_tmp, "%s%c", value_tmp, _10code);
						}
						strncpy(tmp_wep_key, value_tmp, strlen(value_tmp));
						DEBUG_MSG("tmp_wep_key:%s\nlen is %d\n", tmp_wep_key, strlen(tmp_wep_key));
					}
					if (AppendTag(pchSendBuf, pnbSendLen,
								TYPE_WLAN2_KEY, tmp_wep_key,
								strlen(tmp_wep_key)) == FALSE) {
						return QC_BUF_ERR;
					}
				}
				else{
					return QC_TYPE_ERR;
				}
				break;
			}
			case TYPE_WLAN2_AUTOCHANSELECT_MODE://SP 2.0
			{
				DEBUG_MSG("TYPE_WLAN2_AUTOCHANSELECT_MODE \n");
				char auto_channel;
				if(nvram_get("wlan1_auto_channel_enable"))
				{
					if(nvram_match("wlan1_auto_channel_enable", "1") == 0)
						auto_channel = 0x1;
					else
						auto_channel = 0x0;
					if (AppendTag(pchSendBuf, pnbSendLen,	TYPE_WLAN2_AUTOCHANSELECT_MODE, &auto_channel, sizeof(char)) == FALSE) {
						return QC_BUF_ERR;
					}
				}
				else{
					return QC_TYPE_ERR;
				}
				break;
			}
			case TYPE_WLAN2_MODE_CAPABILITY:
			{
				/* ken_chiang
				The option is to indicate the wireless2 capability and the default value
				   is 2(an) if your wirless2 is other type you must to used #define or
				   new nvram value like wlan1_cpanility=3(a) to cange the response value.
				0: 11b/g - 1: 11b/g/n - 2. 11a/n - 3. 11a - 4. a/b/g/n
				*/
				if(nvram_safe_get("wlan0_dot11_mode")){
					char mode=2;
					if (AppendTag(pchSendBuf, pnbSendLen,
								TYPE_WLAN2_MODE_CAPABILITY, &mode, sizeof(char)) == FALSE)
						return QC_BUF_ERR;
				}
				else{
					return QC_TYPE_ERR;
				}
				break;
			}
			case TYPE_WLAN2_MODE:
			{
				/* ken_chiang
					The wirelee2 is assumption runing in 5G so 11n mode is response value 7
					not 3. if your wireless2 is runing in 2.4G you must to chang the response value.
					0: b/g/n - 1: b only - 2. g only - 3. n only (2.4GHz) - 4: b/g mixed
					- 5: g/n mixed - 6: a/n - 7. n only (5GHz)- 8. a only
				*/
				DEBUG_MSG("TYPE_WLAN2_MODE \n");
				char *wlan_mode;
				char mode;
				if(nvram_safe_get("wlan0_dot11_mode")){
					wlan_mode = nvram_get("wlan1_dot11_mode");
					if (!strcmp(wlan_mode, "11bgn"))
						mode = 0;
					else if (!strcmp(wlan_mode, "11b"))
						mode = 1;
					else if (!strcmp(wlan_mode, "11g"))
						mode = 2;
					//else if (!strcmp(wlan_mode, "11n"))
						//mode = 3;
					else if (!strcmp(wlan_mode, "11bg"))
						mode = 4;
					else if (!strcmp(wlan_mode, "11gn"))
						mode = 5;
					else if (!strcmp(wlan_mode, "11na"))
						mode = 6;
					else if (!strcmp(wlan_mode, "11n"))
						mode = 7;
					else if (!strcmp(wlan_mode, "11a"))
						mode = 8;

					if (AppendTag(pchSendBuf, pnbSendLen,
								TYPE_WLAN2_MODE, &mode, sizeof(char)) == FALSE)
						return QC_BUF_ERR;
				}
				else{
					return QC_TYPE_ERR;
				}
				break;
			}
			case TYPE_WLAN2_CHANNEL_LIST:
			{
				DEBUG_MSG("TYPE_WLAN2_CHANNEL_LIST \n");
				char *wlanDomain;
				char *channel;
				char channel_list[64]={0};
#ifdef SET_GET_FROM_ARTBLOCK
				if(!(wlanDomain = artblock_get("wlan0_domain")))
					wlanDomain = nvram_safe_get("wlan0_domain");
#else
				wlanDomain = nvram_safe_get("wlan0_domain");
#endif
				if(nvram_safe_get("wlan1_enable"))
				{
					if(!strcmp(wlanDomain, "0x10"))
						channel="36,40,48,52,56,60,64,149,153,157,161,165,";
					else if(!strcmp(wlanDomain, "0x30"))
						channel="36,40,44,48,52,56,60,64,";
					else
					{
						DEBUG_MSG("TYPE_WLAN2_CHANNEL_LIST: using IOCTL to get channel list\n");
#ifdef AR7161
						if(nvram_match("wlan0_enable", "1")==0)
						{
							if(nvram_match("wlan1_enable", "1")==0)
								list_channels("ath1", &channel_list[0]);
							else
								DEBUG_MSG("TYPE_WLAN_CHANNEL_LIST: 5GHz is disable\n");
						}
						else
						{
							if(nvram_match("wlan1_enable", "1")==0)
								list_channels("ath0", &channel_list[0]);
							else
								DEBUG_MSG("TYPE_WLAN_CHANNEL_LIST: 5GHz is disable\n");
						}
#else
						list_channels(nvram_safe_get("wlan0_eth"), &channel_list[0]);
#endif
						channel=&channel_list[0];
						DEBUG_MSG("TYPE_WLAN2_CHANNEL_LIST: channel list=%s\n", channel);
					}

					if (AppendTag(pchSendBuf, pnbSendLen,
								TYPE_WLAN2_CHANNEL_LIST, channel, strlen(channel)+1) == FALSE)
						return QC_BUF_ERR;
				}
				else{
					return QC_TYPE_ERR;
				}
				break;
			}
//5G interface 2
			default:
				DEBUG_MSG("default:\n");
				return QC_TYPE_ERR;
				break;
		}/* switch */

		if(bswap(pTagHdr->type) != TYPE_DETECT_ETHERNET_CABLE_CONNECTED)
			tag_len = QC_TAG_HDR_LEN;
		count++;
		total_len -= tag_len;
/*
* Date: 2008-01-07
* Name: Albert
* Detail: modify for support DCC 2.0
*/
//---------
		if((tag_len != QC_TAG_HDR_LEN) && (total_len != 0))
		{
		   unsigned char *tmp1,*tmp2;
		   tmp1 = (unsigned char *)(pchRecvBuf + QC_PKT_HDR_LEN + count * tag_len);
		   tmp2 = (unsigned char *)(pchRecvBuf + QC_PKT_HDR_LEN + count * tag_len + 1);
		   DEBUG_MSG("tmp1 = %x tmp2 = %x \n", *(unsigned char*)tmp1, *(unsigned char*)tmp2);
		   if (bswap(((*(unsigned char*)tmp1 << 8)|*(unsigned char*)tmp2)) != TYPE_DETECT_ETHERNET_CABLE_CONNECTED)
		   {
		   	tmp1 = (unsigned char *)(pchRecvBuf + QC_PKT_HDR_LEN + count * tag_len -1);
		   	tmp2 = (unsigned char *)(pchRecvBuf + QC_PKT_HDR_LEN + count * tag_len);
		   	pTagHdr->type = ((*(unsigned char*)tmp1 << 8)|*(unsigned char*)tmp2);
		   }
		   else{
		   	pTagHdr->type = ((*(unsigned char*)tmp1 << 8)|*(unsigned char*)tmp2);
		   	ConnectState = (unsigned char *)(pchRecvBuf + QC_PKT_HDR_LEN + count * tag_len + 4);
		   }

		}else
//-----------
		pTagHdr = (PTAGHEADER) (pchRecvBuf + QC_PKT_HDR_LEN + count * tag_len);

	}/* while */
	return QC_SUCCESS;
}


/* handle SET commands */
int HandleSetCommand(const unsigned char *pchBuf, const int nbBufLen)
{
	int nbLen = QC_PKT_HDR_LEN;
	unsigned short nValueType = 0U;
	unsigned short nbValueLen = 0U;
	unsigned char ch = 0U;
	unsigned short i = 0U;
	char encryption_type = 0;
	char encryption_type2 = 0;
	char key_type = 0;
	char key_type2 = 0;
	char key[128] = {};
	char *transfer_key;
	unsigned char mac[6];
	char *macstr[17];
	char returnValue[256]={};

	const unsigned char* pch = pchBuf + nbLen;

	if (nbBufLen <= (QC_PKT_HDR_LEN + QC_TAG_HDR_LEN)){
		printf("%s: RecvLen Fail!",__func__);
		return QC_SET_ERR;
	}

	while ((nbLen + QC_TAG_HDR_LEN) < nbBufLen) {
		ExtractShortInt(pch, &nValueType);
		pch += sizeof(unsigned short);

		ExtractShortInt(pch, &nbValueLen);
		pch += sizeof(unsigned short);

		nbLen += QC_TAG_HDR_LEN;

		if ((nbBufLen - nbLen) < nbValueLen){
			printf("%s: Value Length Fail!",__func__);
			return QC_SET_DATA_ERR;
		}
		DEBUG_MSG("SET nValueType :%d \n", nValueType);
		switch (nValueType) {
			/* A must item: WAN Port Type */
			case TYPE_IFS_1_TYPE:
			{
				DEBUG_MSG("TYPE_IFS_1_TYPE \n");
				(void) memcpy ((char*) &ch, pch, nbValueLen);
				switch (ch) {
					case WAN_TYPE_STATIC_PPPOE:
						nvram_set("wan_proto", "pppoe");
						nvram_set("wan_pppoe_enable_00", "1");
						nvram_set("wan_pppoe_dynamic_00", "0");
						break;

					case WAN_TYPE_DYNAMIC_PPPOE:
						nvram_set("wan_proto", "pppoe");
						nvram_set("wan_pppoe_enable_00", "1");
						nvram_set("wan_pppoe_dynamic_00", "1");
						break;

					case WAN_TYPE_DHCP:
						nvram_set("wan_proto", "dhcpc");
						break;

					case WAN_TYPE_STATIC_IP:
						nvram_set("wan_proto", "static");
						break;

					case WAN_TYPE_STATIC_L2TP:
						nvram_set("wan_proto", "l2tp");
						nvram_set("wan_l2tp_dynamic", "0");
						break;

					case WAN_TYPE_DYNAMIC_L2TP:
						nvram_set("wan_proto", "l2tp");
						nvram_set("wan_l2tp_dynamic", "1");
						break;

					case WAN_TYPE_STATIC_PPTP:
						nvram_set("wan_proto", "pptp");
						nvram_set("wan_pptp_dynamic", "1");
						break;

					case WAN_TYPE_DYNAMIC_PPTP:
						nvram_set("wan_proto", "pptp");
						nvram_set("wan_pptp_dynamic", "1");
						break;

					case WAN_TYPE_BIGPOND:
						nvram_set("wan_proto", "bigpond");
						break;

					default:
						nvram_set("wan_proto", "dhcpc");
						break;

				}
				break;
			}
			/* PPPoE & PPTP & L2TP & BigPond Common Information */
			case TYPE_IFS_1_USERNAME:
			{
				DEBUG_MSG("TYPE_IFS_1_USERNAME \n");
				if (nbValueLen > POE_NAME_LEN)
					return QC_SET_USERNAME_ERR;
				if ( !nvram_match("wan_proto", "pppoe") )
					nvram_set_n("wan_pppoe_username_00", pch, nbValueLen);
				else if ( !nvram_match("wan_proto", "pptp") )
					nvram_set_n("wan_pptp_username", pch, nbValueLen);
				else if ( !nvram_match("wan_proto", "l2tp") )
					nvram_set_n("wan_l2tp_username", pch, nbValueLen);
				else if ( !nvram_match("wan_proto", "bigpond") )
					nvram_set_n("wan_bigpond_username", pch, nbValueLen);
				else
					return QC_SET_USERNAME_ERR;
				break;
			}
			case TYPE_IFS_1_PASSWORD:
			{
				DEBUG_MSG("TYPE_IFS_1_PASSWORD \n");
				if (nbValueLen > POE_PASS_LEN)
					return QC_SET_PASSWORD_ERR;
				if ( !nvram_match("wan_proto", "pppoe") )
					nvram_set_n("wan_pppoe_password_00", pch, nbValueLen);
				else if ( !nvram_match("wan_proto", "pptp") )
					nvram_set_n("wan_pptp_password", pch, nbValueLen);
				else if ( !nvram_match("wan_proto", "l2tp") )
					nvram_set_n("wan_l2tp_password", pch, nbValueLen);
				else if ( !nvram_match("wan_proto", "bigpond") )
					nvram_set_n("wan_bigpond_password", pch, nbValueLen);
				else
					return QC_SET_PASSWORD_ERR;
				break;
			}
			/* PPTP Information */
			case TYPE_PPTP_SERVER_IP:
			{
				DEBUG_MSG("TYPE_PPTP_SERVER_IP \n");
				if (nbValueLen > POE_NAME_LEN)
					return QC_SET_PPTP_SERVER_IP_ERR;
				else
					nvram_set_n("wan_pptp_server_ip", pch, nbValueLen);
				break;
			}
			case TYPE_L2TP_SERVER_IP:
			{
				DEBUG_MSG("TYPE_L2TP_SERVER_IP \n");
				if (nbValueLen > POE_NAME_LEN)
					return QC_SET_L2TP_SERVER_IP_ERR;
				nvram_set_n("wan_l2tp_server_ip", pch, nbValueLen);

				break;
			}
			/* Big pond */
			case TYPE_BIGPOND_SERVER_NAME:
			{
				DEBUG_MSG("TYPE_BIGPOND_SERVER_NAME \n");
				if (nbValueLen > POE_NAME_LEN)
					return QC_SET_BIGPOND_SRVNAME_ERR;
				if (strncmp("sm-server", (char*) pch, nbValueLen) && strncmp("dce-server", (char*) pch, nbValueLen))
					return QC_SET_BIGPOND_SRVNAME_ERR;
				nvram_set_n("wan_bigpond_auth", pch, nbValueLen);
				break;
			}
			/* WLAN Basic Settings */
			case TYPE_WLAN_SSID:
			{
				DEBUG_MSG("TYPE_WLAN_SSID \n");
				if (nbValueLen > WLA_ESSID_LEN)
					return QC_SET_SSID_ERR;
				else
					nvram_set_n("wlan0_ssid", pch, nbValueLen);
				break;
			}
			case TYPE_WLAN_CHANNEL:
			{
				DEBUG_MSG("TYPE_WLAN_CHANNEL \n");
				char channel[16] = {};
				(void) memcpy ((char*) &ch, pch, nbValueLen);
				sprintf(channel, "%d", ch);
				nvram_set("wlan0_channel", channel);
				nvram_set("wlan0_auto_channel_enable", "0");
				break;
			}
			//case TYPE_AUTOCHANSELECT_MODE:
			case TYPE_WLAN_AUTOCHANSELECT_MODE://SP 2.0
			{
				DEBUG_MSG("TYPE_AUTOCHANSELECT_MODE \n");
				char auto_ch[16] = {};
				(void) memcpy ((char*) &ch, pch, nbValueLen);
				sprintf(auto_ch, "%d", ch);
				DEBUG_MSG("auto_ch : %s\n", auto_ch);
				nvram_set("wlan0_auto_channel_enable", auto_ch);
				break;
			}
			/* WLAN Security */
			case TYPE_WLAN_ENCRYPTION_TYPE:
			{
				DEBUG_MSG("TYPE_WLAN_ENCRYPTION_TYPE \n");
				(void) memcpy ((char*) &encryption_type, pch, nbValueLen);
				key_type = encryption_type;
				switch (encryption_type) {
					case AUTHTYPE_NONE:
						//DEBUG_MSG("none\n");
						nvram_set("wlan0_security", "disable");
					break;
					case AUTHTYPE_WEP64:
						//DEBUG_MSG("wep 64\n");
						nvram_set("wlan0_security", "wep_open_64");
						nvram_set("wlan0_display", "ascii");
					break;
					case AUTHTYPE_WEP128:
						//DEBUG_MSG("wep 128\n");
					break;
					case AUTHTYPE_WPAPSK:
						//DEBUG_MSG("psk\n");
						nvram_set("wlan0_security", "wpa2auto_psk");
						nvram_set("wps_enable", "0");
						nvram_set("wlan0_psk_cipher_type", "both");
					break;
					default:
						DEBUG_MSG("no option matched\n");
					break;
				}
				break;
			}
			case TYPE_WLAN_KEY:
			{
				DEBUG_MSG("TYPE_WLAN_KEY\n");
				switch(key_type){
					case AUTHTYPE_NONE:
					break;
					case AUTHTYPE_WEP64:
						memset(returnValue, 0, 256);
						strncpy(key, pch, nbValueLen);
						transfer_key = ascii2hex(key, returnValue);
						DEBUG_MSG("pch = %s len = %d\n", pch, nbValueLen);
						DEBUG_MSG("transfer_key is %s\n", transfer_key);
						nvram_set("wlan0_wep64_key_1", transfer_key);
						nvram_set("asp_temp_37",transfer_key); //for add wireless device with wps GUI
					break;
					case AUTHTYPE_WPAPSK:
						nvram_set_n("wlan0_psk_pass_phrase", pch, nbValueLen);
					break;
				}
				break;
			}
			/* Other Information */
			case TYPE_IFS_1_IP:
			{
				DEBUG_MSG("TYPE_IFS_1_IP \n");
				if (nbValueLen > 16)
					return QC_SET_IP_ERR;

				if ( !nvram_match("wan_proto", "pppoe") )
					nvram_set_n("wan_pppoe_ipaddr_00", pch,nbValueLen);
				else if ( !nvram_match("wan_proto", "pptp") )
					nvram_set_n("wan_pptp_ipaddr", pch, nbValueLen);
				else if ( !nvram_match("wan_proto", "l2tp") )
					nvram_set_n("wan_l2tp_ipaddr", pch, nbValueLen);
				else if ( !nvram_match("wan_proto", "static") )
					nvram_set_n("wan_static_ipaddr", pch, nbValueLen);

				break;
			}
			case TYPE_IFS_1_MASK:
			{
				DEBUG_MSG("TYPE_IFS_1_MASK \n");
				if (nbValueLen < sizeof(unsigned long))
					return QC_SET_MASK_ERR;

				if ( !nvram_match("wan_proto", "pppoe") )
					nvram_set_n("wan_pppoe_netmask_00", pch, nbValueLen);
				else if ( !nvram_match("wan_proto", "pptp") )
					nvram_set_n("wan_pptp_netmask", pch, nbValueLen);
				else if ( !nvram_match("wan_proto", "l2tp") )
					nvram_set_n("wan_l2tp_netmask", pch, nbValueLen);
				else if ( !nvram_match("wan_proto", "static") )
					nvram_set_n("wan_static_netmask", pch, nbValueLen);

				break;
			}
			case TYPE_IFS_1_GATEWAY:
			{
				DEBUG_MSG("TYPE_IFS_1_GATEWAY \n");
				if (nbValueLen < sizeof(unsigned long))
					return QC_SET_GATEWAY_ERR;

				if ( !nvram_match("wan_proto", "pppoe") )
					nvram_set_n("wan_pppoe_gateway_00", pch, nbValueLen);
				else if ( !nvram_match("wan_proto", "pptp") )
					nvram_set_n("wan_pptp_gateway", pch, nbValueLen);
				else if ( !nvram_match("wan_proto", "l2tp") )
					nvram_set_n("wan_l2tp_gateway", pch, nbValueLen);
				else if ( !nvram_match("wan_proto", "static") )
					nvram_set_n("wan_static_gateway", pch, nbValueLen);

				break;
			}
			case TYPE_IFS_1_DNS_1:
			{
				DEBUG_MSG("TYPE_IFS_1_DNS_1 \n");
				if (nbValueLen < sizeof(unsigned long))
					return QC_SET_DNS_1_ERR;
				else
				{
				    if( strncmp(pch, "0.0.0.0", 7)==0 )
				    	nvram_set("wan_specify_dns", "0");
				    else
				    nvram_set("wan_specify_dns", "1");

				    nvram_set_n("wan_primary_dns", pch, nbValueLen);
				}
				break;
			}
			case TYPE_IFS_1_DNS_2:
			{
				DEBUG_MSG("TYPE_IFS_1_DNS_2 \n");
				if (nbValueLen < sizeof(unsigned long))
					return QC_SET_DNS_2_ERR;
				else
					nvram_set_n("wan_secondary_dns", pch, nbValueLen);

				break;
			}
			case TYPE_IFS_1_MTU:
			{
				DEBUG_MSG("TYPE_IFS_1_MTU \n");
				ExtractShortInt(pch, &i);
				if ( nvram_set_n("wan_mtu", pch, nbValueLen) !=0)
					return QC_SET_MTU_ERR;
				break;
			}
			case TYPE_IFS_1_SERVICES_NAME:
			{
				DEBUG_MSG("TYPE_IFS_1_SERVICES_NAME \n");
				break;
			}
			case TYPE_IFS_1_IDLE_TIME:
			{
				DEBUG_MSG("TYPE_IFS_1_IDLE_TIME \n");
				break;
			}
			case TYPE_IFS_1_ON_DEMAND:
			{
				DEBUG_MSG("TYPE_IFS_1_ON_DEMAND \n");
				break;
			}
			case TYPE_IFS_1_AUTO_RECONNECT:
			{
				DEBUG_MSG("TYPE_IFS_1_AUTO_RECONNECT \n");
				break;
			}
			case TYPE_CLONE_WAN_MAC:
			{
				memcpy(mac, pch, nbValueLen);
                		sprintf(macstr, "%02x:%02x:%02x:%02x:%02x:%02x",
                			mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
				DEBUG_MSG("TYPE_CLONE_WAN_MAC %s\n", macstr);
#ifdef SET_GET_FROM_ARTBLOCK
//Albert modify : i could not allow user can modify ART block
//				artblock_set("wan_mac", macstr);
				nvram_set("wan_mac", macstr);
#else
				nvram_set("wan_mac", macstr);
#endif
				nvram_set("mac_clone_addr", macstr);
				break;
			}
//SP 2.0
			case TYPE_WLAN_MODE:
			{
				/* 0: b/g/n - 1: b only - 2. g only - 3. n only (2.4GHz) -
				4: b/g mixed - 5: g/n mixed - 6: a/n - 7. n only (5GHz)- 8. a only */
				DEBUG_MSG("TYPE_WLAN_MODE \n");
				(void) memcpy ((char*) &ch, pch, nbValueLen);
				switch (ch) {
					case 0:
						nvram_set("wlan0_dot11_mode","11bgn");
						break;
					case 1:
						nvram_set("wlan0_dot11_mode","11b");
						break;
					case 2:
						nvram_set("wlan0_dot11_mode","11g");
						break;
					case 3:
					case 7:
						nvram_set("wlan0_dot11_mode","11n");
						break;
					case 4:
						nvram_set("wlan0_dot11_mode","11bg");
						break;
					case 5:
						nvram_set("wlan0_dot11_mode","11gn");
						break;
					case 6:
						nvram_set("wlan0_dot11_mode","11na");
					case 8:
						nvram_set("wlan0_dot11_mode","11a");
						break;
					default:
						DEBUG_MSG("TYPE_WLAN_MODE unknow mode\n");
						break;
				}
				break;
			}
//5G interface 2
			case TYPE_WLAN2_SSID:
			{
				DEBUG_MSG("TYPE_WLAN2_SSID \n");
				if (nbValueLen > WLA_ESSID_LEN)
					return QC_SET_SSID_ERR;
				else
					nvram_set_n("wlan1_ssid", pch, nbValueLen);
				break;
			}
			case TYPE_WLAN2_CHANNEL:
			{
				DEBUG_MSG("TYPE_WLAN2_CHANNEL \n");
				char channel[16] = {};
				(void) memcpy ((char*) &ch, pch, nbValueLen);
				sprintf(channel, "%d", ch);
				nvram_set("wlan1_channel", channel);
				nvram_set("wlan1_auto_channel_enable", "0");
				break;
			}
			case TYPE_WLAN2_ENCRYPTION_TYPE:
			{
				DEBUG_MSG("TYPE_WLAN2_ENCRYPTION_TYPE \n");
				(void) memcpy ((char*) &encryption_type2, pch, nbValueLen);
				key_type2 = encryption_type2;
				switch (encryption_type2) {
					case AUTHTYPE_NONE:
						//DEBUG_MSG("none\n");
						nvram_set("wlan1_security", "disable");
					break;
					case AUTHTYPE_WEP64:
						//DEBUG_MSG("wep 64\n");
						nvram_set("wlan1_security", "wep_open_64");
						nvram_set("wlan1_display", "ascii");
					break;
					case AUTHTYPE_WEP128:
						//DEBUG_MSG("wep 128\n");
					break;
					case AUTHTYPE_WPAPSK:
						//DEBUG_MSG("psk\n");
						nvram_set("wlan1_security", "wpa2auto_psk");
						nvram_set("wps_enable", "0");
						nvram_set("wlan1_psk_cipher_type", "both");
					break;
					default:
						DEBUG_MSG("no option matched\n");
					break;
				}
				break;
			}
			case TYPE_WLAN2_KEY:
			{
				DEBUG_MSG("TYPE_WLAN2_KEY\n");
				switch(key_type2){
					case AUTHTYPE_NONE:
					break;
					case AUTHTYPE_WEP64:
						memset(returnValue, 0, 256);
						strncpy(key, pch, nbValueLen);
						transfer_key = ascii2hex(key, returnValue);
						DEBUG_MSG("key = %s pch = %s len = %d\n", key, pch, nbValueLen);
						DEBUG_MSG("transfer_key is %s\n", transfer_key);
						nvram_set("wlan1_wep64_key_1", transfer_key);
						nvram_set("asp_temp_53",transfer_key); //for add wireless device with wps GUI
					break;
					case AUTHTYPE_WPAPSK:
						nvram_set_n("wlan1_psk_pass_phrase", pch, nbValueLen);
					break;
				}
				break;
			}
			case TYPE_WLAN2_AUTOCHANSELECT_MODE://SP 2.0
			{
				DEBUG_MSG("TYPE_WLAN2_AUTOCHANSELECT_MODE \n");
				char auto_ch[16] = {};
				(void) memcpy ((char*) &ch, pch, nbValueLen);
				sprintf(auto_ch, "%d", ch);
				DEBUG_MSG("auto_ch : %s\n", auto_ch);
				nvram_set("wlan1_auto_channel_enable", auto_ch);
				break;
			}
			case TYPE_WLAN2_MODE:
			{
				/* 0: b/g/n - 1: b only - 2. g only - 3. n only (2.4GHz) -
				4: b/g mixed - 5: g/n mixed - 6: a/n - 7. n only (5GHz)- 8. a only */
				DEBUG_MSG("TYPE_WLAN2_MODE \n");
				(void) memcpy ((char*) &ch, pch, nbValueLen);
				switch (ch) {
					case 0:
						nvram_set("wlan1_dot11_mode","11bgn");
						break;
					case 1:
						nvram_set("wlan1_dot11_mode","11b");
						break;
					case 2:
						nvram_set("wlan1_dot11_mode","11g");
						break;
					case 3:
					case 7:
						nvram_set("wlan1_dot11_mode","11n");
						break;
					case 4:
						nvram_set("wlan1_dot11_mode","11bg");
						break;
					case 5:
						nvram_set("wlan1_dot11_mode","11gn");
						break;
					case 6:
						nvram_set("wlan1_dot11_mode","11na");
					case 8:
						nvram_set("wlan1_dot11_mode","11a");
						break;
					default:
						DEBUG_MSG("TYPE_WLAN_MODE unknow mode\n");
						break;
				}
				break;
			}
//5G interface 2
			default:
				DEBUG_MSG("default:\n");
				return QC_TYPE_ERR;
				break;

		}
		nbLen += nbValueLen;
		pch += nbValueLen;

	}/* while */
	return QC_SUCCESS;
}

#ifdef AUTH_RUN_TIMER

void clear_allauth()
{
	memset(sp_login, 0, S_LOGIN_MAX*sizeof(struct support_login));
}

int get_splogin(char *client_mac, unsigned long splogin_ip)
{
	int i,j=0,find_it=0;
	unsigned long sticks_tmp=0;
	
	if(!client_mac) return -1;
	for(i=0;i<S_LOGIN_MAX;i++)
	{
		if(!memcmp(client_mac, sp_login[i].clientMAC, 17))
		{
			find_it=1;
			sp_login[i].sticks=S_LOGIN_MAINTAIN;
			if(sp_login[i].log_ip != splogin_ip) sp_login[i].auth_if=0;
			sp_login_p=&sp_login[i];
			break;
		}
	}
	if(find_it == 0)
	{
        	for(i=0;i<S_LOGIN_MAX;i++)
        	{
                	if(sp_login[i].log_ip == 0)
                	{
                        	find_it=1;
				sp_login[i].log_ip=splogin_ip;
				sp_login[i].auth_if=0;
				sp_login[i].sticks=S_LOGIN_MAINTAIN;
				memcpy(sp_login[i].clientMAC, client_mac, 17);
                        	sp_login_p=&sp_login[i];
                        	break;
                	}
			if(sp_login[i].sticks <= sticks_tmp)
			{
				if(i == 0)
				{
					j=i;
					sticks_tmp=sp_login[i].sticks;
				}
				else if(sp_login[i].sticks < sticks_tmp)
				{
                                        j=i;
                                        sticks_tmp=sp_login[i].sticks;
				}
			}
        	}
		if(find_it == 0)
		{
			if(!((j==0)&&(sp_login[0].sticks!=0)))
			{
                        	find_it=1;
                        	sp_login[j].log_ip=splogin_ip;
                        	sp_login[j].auth_if=0;
                        	sp_login[j].sticks=S_LOGIN_MAINTAIN;
                        	memcpy(sp_login[i].clientMAC, client_mac, 17);
                		sp_login_p=&sp_login[j];
			}
		}
	}
	return 0;
}

void auth_clear_buf()
{
	int i;

        for(i=0;i<S_LOGIN_MAX;i++)
        {
                if(sp_login[i].log_ip != 0)
                {
		    	if(sp_login[i].auth_if == 1)
			{
                        	if(sp_login[i].sticks != 0) sp_login[i].sticks--;
				else sp_login[i].auth_if=0;
			}
                }
        }
}

static void auth_check_timer(int data)
{
	if(auth_check_if!=0)
	{
		auth_clear_buf();
	}
}

static int AUTH_check_time()
{
	struct itimerval val;
	struct sigaction action;

	action.sa_handler = (void (*)())auth_check_timer;
	action.sa_flags = SA_RESTART;

	sigaction(SIGALRM, &action, NULL);
	
	val.it_interval.tv_sec = 20;
	val.it_interval.tv_usec = 0;
	val.it_value.tv_sec = 20;
	val.it_value.tv_usec = 0;

	setitimer(ITIMER_REAL, &val, 0);
	auth_check_if=1;
}
#endif

int HandleQuickConfigCommand(const int hSocket,
		const unsigned char *pchRecvBuf,
		const int nbRecvLen,
		const struct sockaddr_in *pSockAddrFrom)
{


	unsigned char *pchSendBuf = NULL;
	PPACKETHEADER pPktHdr = (PPACKETHEADER) pchRecvBuf;
	int nbSendLen = 0;
	int nRetry;
	short nRet = QC_CMD_ERR;
	unsigned short cClientID = bswap(pPktHdr->client_id);
	char fReboot = 0;
	char mac[6];
	char *lanmac;
#ifdef AUTH_RUN_TIMER
	char client_ip[20],buff[1024],*mac_ptr,client_mac_tmp[20];
	FILE *fp;
	int update_mac=0;
#endif

	pchSendBuf = (unsigned char*) malloc(MAX_BUF_SIZE + 1);
	if (pchSendBuf == NULL) {
		return QC_ERROR;
	}
	(void) memset((void*) pchSendBuf, 0, MAX_BUF_SIZE);

	nbSendLen += QC_PKT_HDR_LEN;

	DEBUG_MSG("HandleQuickConfigCommand\n");

#ifdef AUTH_RUN_TIMER
	auth_check_if=0;
	sprintf(client_ip, "%s", inet_ntoa(*((struct in_addr *)&pSockAddrFrom->sin_addr)));
	DEBUG_MSG("DCCD: client ip is %s\n", client_ip);

	if((fp = fopen("/proc/net/arp", "r")) != NULL)
	{
		memset(buff, 0, 1024);
		while( fgets(buff, 1024, fp))
		{
			if (mac_ptr = strstr(buff, client_ip) ) {
				if(mac_ptr=strstr(buff, ":"))
				{
					memcpy(client_mac_tmp, mac_ptr-2, 17);
					update_mac = 1;
					break;
				}
			}
			memset(buff, 0, 1024);
		}
		fclose(fp);
	}

	if( ! update_mac )
	{
		unsigned char *ptr =  &pSockAddrFrom->sin_addr;
		sprintf(client_mac_tmp, "00:00:%02x:%02x:%02x:%02x"
			, *ptr, *(ptr+1), *(ptr+2), *(ptr+3));
	}
	sp_login_p = NULL;
	if(get_splogin(client_mac_tmp, *((unsigned long *)&pSockAddrFrom->sin_addr)) == 0)
	{
		if(sp_login_p != NULL) printf("DCCD: client_mac-%s client_ip-%x is_auth-%d\n",client_mac_tmp,*((unsigned long *)&pSockAddrFrom->sin_addr),sp_login_p->auth_if);
#endif

	switch ( bswap(pPktHdr->cmd) ) {


		case CMD_DISCOVERY:

			sprintf(szDebug, "handling discovery commands...");
			//PutLogMsg(MLOG_DEBUG, szDebug, "QC");
			DEBUG_MSG("%s\n", szDebug);

			nRet = HandleDiscoveryCommand(pchRecvBuf, nbRecvLen);
			if (nRet == QC_SUCCESS) {
				{
					//sprintf(szDebug, "model name: %s", cfg.device.host_name);
					//PutLogMsg(MLOG_DEBUG, szDebug, "QC");
					//printf("%s\n", szDebug);
				}
				if (AppendTag(pchSendBuf, &nbSendLen, TYPE_MODELNAME, nvram_safe_get("model_number"), strlen(nvram_safe_get("model_number"))) == FALSE) {
					nRet = QC_ERROR;
				}

#ifdef SET_GET_FROM_ARTBLOCK
				if(!(lanmac = artblock_get("lan_mac")))
					lanmac = nvram_get("lan_mac");
#else
				lanmac = nvram_get("lan_mac");
#endif
				parse_mac(mac, lanmac);
				DEBUG_MSG("mac = 0x%02x-0x%02x-0x%02x-0x%02x-0x%02x-0x%02x\n", mac[0], mac[1], mac[2],mac[3],mac[4],mac[5]);

				if (AppendTag(pchSendBuf, &nbSendLen, TYPE_MAC, mac, sizeof(mac)) == FALSE) {
					nRet = QC_ERROR;
				}

#ifdef AUTH_RUN_TIMER
				if(sp_login_p != NULL) sp_login_p->auth_if = 1;
#endif				
			}

			//recvfrom(hSocket, (char*) Junk, 64, 0, (struct sockaddr*) pSockAddrFrom, &addrLen); //SandyCho add
			break;

		case CMD_REGISTER:
			sprintf(szDebug, "handling register commands...");
			//PutLogMsg(MLOG_DEBUG, szDebug, "QC");
			DEBUG_MSG("%s\n", szDebug);

#ifdef AUTH_RUN_TIMER
			if((sp_login_p != NULL)&&(sp_login_p->auth_if == 1))
			{
#endif

			nRet = HandleRegisterCommand(pchRecvBuf, nbRecvLen);
			if (nRet == QC_SUCCESS) {
				if (AppendTag(pchSendBuf, &nbSendLen, TYPE_IFS_1_TYPE, &g_nWANType, sizeof(char)) == FALSE) {
					nRet = QC_ERROR;
				}
			}
#ifdef AUTH_RUN_TIMER
			}
			else
				nRet = QC_ERROR;
#endif
			break;

		case CMD_GET_CFG:
			sprintf(szDebug, "handling GET commands...");
			//PutLogMsg(MLOG_DEBUG, szDebug, "QC");
			DEBUG_MSG("%s\n", szDebug);
#ifdef AUTH_RUN_TIMER
			if((sp_login_p != NULL)&&(sp_login_p->auth_if == 1))
			{
#endif
			nRet = HandleGetCommand(pchRecvBuf, nbRecvLen, pchSendBuf, &nbSendLen);
#ifdef AUTH_RUN_TIMER
			}
			else
				nRet = QC_ERROR;
#endif
			break;

		case CMD_SET_CFG:
			sprintf(szDebug, "handling SET commands...");
			//PutLogMsg(MLOG_DEBUG, szDebug, "QC");
			DEBUG_MSG("%s\n", szDebug);
#ifdef AUTH_RUN_TIMER
			if((sp_login_p != NULL)&&(sp_login_p->auth_if == 1))
			{
#endif
			nRet = HandleSetCommand(pchRecvBuf, nbRecvLen);
			if (nRet == QC_SUCCESS) {
				DEBUG_MSG("nvram commit\n");
				nvram_flag_set();
				nvram_commit();
			}
#ifdef AUTH_RUN_TIMER
			}
			else
				nRet = QC_ERROR;
#endif
			break;

		case CMD_REBOOT:
#ifdef AUTH_RUN_TIMER
			if((sp_login_p != NULL)&&(sp_login_p->auth_if == 1))
			{
#endif
			fReboot = 1;
			nRet = QC_SUCCESS;
#ifdef AUTH_RUN_TIMER
			}
			else
				nRet = QC_ERROR;
#endif
			break;

		default:
			cClientID = 0;
			nRet = QC_CMD_ERR;
			break;
	}
#ifdef AUTH_RUN_TIMER
	}
	else
	{
		cClientID = 0;
		nRet = QC_CMD_ERR;
	}
	auth_check_if=1;
#endif

	if (nRet != QC_SUCCESS) {
		(void) memset(pchSendBuf, 0, MAX_BUF_SIZE);
		nbSendLen = QC_PKT_HDR_LEN;
	}


	CreateHeader(pchSendBuf, bswap(pPktHdr->cmd), nRet, cClientID, bswap(pPktHdr->seq_no));


	/* try to send 3 times */
	for (nRetry = 0; nRetry < 3; ++nRetry) {
		if (sendto(hSocket, (void*) pchSendBuf, nbSendLen, 0, (struct sockaddr*) pSockAddrFrom, sizeof(struct sockaddr_in)) > 0) {
			break;
		}
	}


	if (fReboot == 1) {
		reboot();
	}

	if (nRetry > 3)
		return QC_ERROR;
	else
		return QC_SUCCESS;
}



/*
 *  * QuickConfig Main Function
 *   */
int StartQuickConfigDaemon(void)
{
	static unsigned char achRecvBuf[MAX_BUF_SIZE];
	int nbRecvLen, addrLen = sizeof(struct sockaddr_in);
	int hSock, i;
	struct sockaddr_in addrMe;
	struct sockaddr_in addrFrom;
	struct ifreq interface;
	int nRet = 0;


	/* Create Socket */
	hSock = socket(AF_INET, SOCK_DGRAM, 0);
	if(hSock == -1) {
		DEBUG_MSG("Quick Config: bad socket: %d\n", hSock);
		return -1;
	}

	i = 1;
	if (setsockopt(hSock, SOL_SOCKET, SO_REUSEADDR,(char*) &i, sizeof(i) ) < 0)
	{
		perror("Quick Config: setsockopt REUSEADDR fail");
		close(hSock);
		return -1;
	}

	strncpy(interface.ifr_ifrn.ifrn_name, nvram_get("lan_bridge"), IFNAMSIZ);
	if (setsockopt(hSock, SOL_SOCKET, SO_BINDTODEVICE,(char *)&interface, sizeof(interface)) < 0) {
		close(hSock);
		return -1;
	}

	addrMe.sin_family = AF_INET;
	addrMe.sin_addr.s_addr = htonl(INADDR_ANY);
	addrMe.sin_port = htons(QC_PORT);

	nRet = bind(hSock, (struct sockaddr*) &addrMe, sizeof(addrMe));
	if(nRet !=0) {
		getsockopt(hSock, SOL_SOCKET, SO_ERROR, (char*) &i, sizeof(i));
		DEBUG_MSG("Quick Config: bind error: %d\n", i);
		close(hSock);
	}

#ifdef AUTH_RUN_TIMER
	clear_allauth();
	AUTH_check_time();
#endif

	for (;;) {
		memset(achRecvBuf, 0, MAX_BUF_SIZE);//static unsigned char achRecvBuf[MAX_BUF_SIZE];
		//TK_SLEEP(TPS*1);


		nbRecvLen = recvfrom(hSock, (char*) achRecvBuf, MAX_BUF_SIZE, 0, (struct sockaddr*) &addrFrom, &addrLen);

		if (nbRecvLen >= 0) {
			if (nbRecvLen < QC_PKT_HDR_LEN) {
				continue;
			} else {

				sprintf(szDebug, "received: %d bytes", nbRecvLen);
				DEBUG_MSG("%s\n", szDebug);
				//PutLogMsg(MLOG_DEBUG, szDebug, "QC");

				HandleQuickConfigCommand(hSock, achRecvBuf, nbRecvLen, &addrFrom);

			}
		} else if (nbRecvLen == -1) {

			//nRet = errno(hSock);

			if (nRet == EWOULDBLOCK) {
				/* Blocked */
				continue;

			} else {
				sprintf(szDebug, "returned err code: %d ", nRet);
				DEBUG_MSG("%s\n", szDebug);
				//PutLogMsg(MLOG_DEBUG, szDebug, "QC");

				return nRet;
			}

		} else {
			sprintf(szDebug, "Error... returned %d ", nbRecvLen);
			DEBUG_MSG("%s\n", szDebug);
			//PutLogMsg(MLOG_DEBUG, szDebug, "QC");

			return nRet;
		}
	}

	/* Never reach here */ ;

	return 0;
}



int main (int argc, char **argv)
{
	FILE *pid_fp;
	if (!(pid_fp = fopen(DCC_PID, "w"))) {
		perror(DCC_PID);
		return errno;
	}

	fprintf(pid_fp, "%d", getpid());
	fclose(pid_fp);

	for (;;) {
		if (StartQuickConfigDaemon() < 0)
			perror("Quick Config open fail.\n");
		sleep(1);
	}

	return 0;
}

char trans2Hex(int c)
{
    char val;
    switch(c)
    {
    	case 10:
    		val = 0xa;
    	break;
    	case 11:
    		val = 0xb;
    	break;
    	case 12:
    		val = 0xc;
    	break;
    	case 13:
    		val = 0xd;
    	break;
    	case 14:
    		val = 0xe;
    	break;
    	case 15:
    		val = 0xf;
    	break;
    }
    return val;
}
static char *parse_mac(char *mac_addr, char *parse)
{
    char *buf;
    char *p;
    int i,j;
    static char tmp[2];

    buf = parse;

    for(i=0; i< 5; i++)
    {
    	if((p = strchr(buf, ':')) == NULL)
    	  return NULL;

    	for(j = 0; j< 2; j++)
    	{
		if(48 <= *(buf+j) && *(buf+j) <= 57)
		   tmp[j] = *(buf+j) - 48;
		else if(65 <= *(buf+j) && *(buf+j) <= 70)
		   tmp[j] = trans2Hex(*(buf+j) - 55);
		else if(97 <= *(buf+j) && *(buf+j) <= 102)
		   tmp[j] = trans2Hex(*(buf+j) - 87);
    	}
    	mac_addr[i] = tmp[0]<<4 | tmp[1] ;
    	buf = p + 1;

    }
    for(j = 0; j< 2; j++)
    {
	if(48 <= *(buf+j) && *(buf+j) <= 57)
	   tmp[j] = *(buf+j) - 48;
	else if(65 <= *(buf+j) && *(buf+j) <= 70)
	   tmp[j] = trans2Hex(*(buf+j) - 55);
	else if(97 <= *(buf+j) && *(buf+j) <= 102)
	   tmp[j] = trans2Hex(*(buf+j) - 87);
    }
    mac_addr[5] = tmp[0]<<4 | tmp[1] ;

    return (char *)mac_addr;
}
