#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "common.h"
#include "options.h"

#define MAX_SSID_COUNT	4
#if 0
#define DEBUG_MSG(fmt, args...) do { \
        FILE *dp = fopen("/dev/console", "w"); \
        if (dp) { \
                fprintf(dp, "XXX %s(%d)" fmt, __FUNCTION__, __LINE__, ## args); \
                fclose(dp); \
        } \
} while (0)
#else
#define DEBUG_MSG(fmt, args...)
#endif


static void bb_nvram_safe_get(const char *k, char *buf)
{
	FILE *fp = NULL;
	char *p, cmd[128], txt[256];

	DEBUG_MSG("k[%s]\n", k);

	buf[0] = '\0';
	sprintf(cmd, "nvram get %s", k);

	if ((fp = popen(cmd, "r")) == NULL)
		return;
	fscanf(fp, "%[^\n]", txt);
	pclose(fp);

	DEBUG_MSG("txt[%s]\n", txt);
	if ((p = strrchr(txt, ' ')) == NULL)
		return;
	strcpy(buf, p + 1);

	DEBUG_MSG("buf[%s]\n", buf);
}

static void covert_mac_to_str(uint8_t *chaddr, char *mac)
{
	sprintf(mac, "%02X:%02X:%02X:%02X:%02X:%02X",
		chaddr[0],
		chaddr[1],
		chaddr[2],
		chaddr[3],
		chaddr[4],
		chaddr[5]);
}

/*
 * # wlanconfig ath1 list sta
 * ADDR               AID CHAN RATE RSSI IDLE  TXSEQ  RXSEQ CAPS ACAPS ERP    STATE HTCAPS   A_TIME NEGO_RATES
 * 00:16:6f:48:fc:d7    1   13  54M   52  135   6007  19360 ESs          0       27 Q      00:02:53 12         WME
 * #
 * */
static int __which_ssid_used(uint8_t *chaddr, FILE *fp)
{
	int res = -1;
	char ln[256], mac[18], cli_mac[18];

	fscanf(fp, "%[^\n]\n", ln);	/* skip first line */
	DEBUG_MSG("skip first line [%s]\n", ln);
	while (fscanf(fp, "%[^\n]\n", ln) != EOF) {
		DEBUG_MSG("sta list[%s]\n", ln);
		sscanf(ln, "%s", mac);
		covert_mac_to_str(chaddr, cli_mac);
		DEBUG_MSG("mac[%s] cli_mac[%s]\n", mac, cli_mac);
		if (strcasecmp(mac, cli_mac) == 0) {
			res = 0;
			break;
		}
	}

	DEBUG_MSG("res[%d]\n", res);
	return res;
}

/* RETURN: which ssid has the @chaddr client
 * */
static int which_ssid_used(uint8_t *chaddr)
{
	FILE *fp = NULL;
	int ssid_id = -1, i = 0;

	for (; i < MAX_SSID_COUNT; i++) {
		char cmds[128];

		sprintf(cmds, "wlanconfig ath%d list sta", i);
		DEBUG_MSG("cmds[%s]\n", cmds);
		if ((fp = popen(cmds, "r")) == NULL)
			break;
		if (__which_ssid_used(chaddr, fp) != -1) {
			ssid_id = i;
			pclose(fp);
			break;
		}
		pclose(fp);
	}

	DEBUG_MSG("ssid_id[%d]\n", ssid_id);
	return ssid_id;
}

/* @t := "br0:0,192.168.1.1,255.255.255.0" */
static void getip_from_multissid(const char *t, char *ip)
{
	char *p, *_ip, _t[128];

	p = _t;
	strcpy(_t, t);
	if (strsep(&p, ",") == NULL)	/* skip interface field */
		return;
	if ((_ip = strsep(&p, ",")) == NULL)
		*ip = '\0';
	else
		strcpy(ip, _ip);
}

static uint32_t return_nip_by_ciaddr(uint8_t *chaddr)
{
	char nvkey[128];
	char lan_ipaddr[16], multi_ssid_lan[128];
	uint32_t nip = -1;
	uint8_t ssid_id = which_ssid_used(chaddr);

	if (ssid_id == -1)
		goto out;
/*
	if (ssid_id == 0) {
		bb_nvram_safe_get("lan_ipaddr", lan_ipaddr);
		nip = inet_addr(lan_ipaddr);
	} else {
*/
		char ssid_ip[16];
		/* multi_ssid_lan0 := "192.168.2.1/255.255.255.0" */
		sprintf(nvkey, "multi_ssid_lan%d", ssid_id);
		bb_nvram_safe_get(nvkey, multi_ssid_lan);
#if 0
		if ((p = strstr(nvtxt, ",")) == NULL)
			goto out;
		bzero(ssid_ip, sizeof(ssid_ip));
		strncpy(ssid_ip, nvtxt, p - nvtxt);
#endif
		getip_from_multissid(multi_ssid_lan, ssid_ip);
		DEBUG_MSG("ssid_ip[%s]\n", ssid_ip);
		nip = inet_addr(ssid_ip);
//	}
#if 0
	for (; pcli->climac && pcli->ssid; pcli++) {
		if (mac_compare_uint8_char(chaddr, pcli->climac) == 0) {
			for (; pw->ssid && pw->wlip; pw++)
				if (strcmp(pcli->ssid, pw->ssid) == 0) {
					nip = inet_addr(pw->wlip);
					break;
				}
		}
	}
#endif
out:
	return nip;
}

size_t FAST_FUNC udhcp_relay_hook_ssid(
        struct dhcpMessage *packet,
        size_t packlen)
{
	uint32_t nip = return_nip_by_ciaddr(packet->chaddr);

	if (nip != -1)
		packet->giaddr = nip;

	return packlen;
}

extern int get_dhcp_packet_type(struct dhcpMessage *);
size_t FAST_FUNC udhcp_relay_hook_putenv_ip_mac(
	struct dhcpMessage *p,
	size_t ret)
{
	char mac_addr[18], env_tmp[25], env_tmp1[25];
	struct in_addr inaddr;

	if (get_dhcp_packet_type(p) != DHCPACK)
		goto out;

	// get MAC
	memset(mac_addr,'\0',sizeof(mac_addr));
	sprintf(mac_addr, "%02X:%02X:%02X:%02X:%02X:%02X",
			p->chaddr[0],
			p->chaddr[1],
			p->chaddr[2],
			p->chaddr[3],
			p->chaddr[4],
			p->chaddr[5]); // ex: 00:04:23:6f:7b:0e
	//printf("XX %s(%d) , mac :: %s\n", __func__, __LINE__, mac_addr);

	memset(env_tmp,'\0',sizeof(env_tmp));
	sprintf(env_tmp,"dhmac=%s",mac_addr);
	putenv(env_tmp);

	/* Matt 10/27 add, get user info from DHCPREQUEST packet
	 * Fred 10/28 adjust, get user info from DHCPACK packet */
#if 0
	// get IP
	op_ip_tmp = get_option(p, DHCP_REQUESTED_IP);
	if(op_ip_tmp != NULL) {
		sprintf(op_ip, "%d.%d.%d.%d",
				op_ip_tmp[0],
				op_ip_tmp[1],
				op_ip_tmp[2],
				op_ip_tmp[3]); // ex: 192.168.3.152
		//printf("XX %s(%d) , ip :: %s\n", __func__, __LINE__, op_ip);

		memset(env_tmp1,'\0',sizeof(env_tmp1));
		sprintf(env_tmp1,"dhip=%s",op_ip);
		putenv(env_tmp1);

		system("cli sys dhcprelay info");
	}
#endif
	inaddr.s_addr = p->yiaddr;
	sprintf(env_tmp1, "dhip=%s", inet_ntoa(inaddr));
	putenv(env_tmp1);
	system("cli sys dhcprelay info");
out:
	return ret;
}

