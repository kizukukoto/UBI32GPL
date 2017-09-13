/*
 *   *
 *    *	Written by Glen Zhang
 *     */

#ifndef _QUICK_CONFIG_TYPE_H
#define _QUICK_CONFIG_TYPE_H

#define TRUE	1
#define FALSE	0

/* pppoe structure */
#define POE_NAME_LEN        64
#define POE_PASS_LEN        32
/* wireless structure */
#define WLA_ESSID_LEN       32


/* common error code */
#define	QC_SUCCESS			0	/* Success */
#define QC_ERROR			-1  /* default error */
#define QC_CMD_ERR			-11	/* command error, invalid command, command not support */
#define QC_CLIENT_ID_ERR	-12	/* client ID error */
#define QC_SEQ_NO_ERR		-13	/* sequence number error */
#define QC_TYPE_ERR			-14	/* Tag type error */
#define QC_BUF_ERR			-15	/* buffer overflow */

/* discovery command error code */
#define QC_DIS_ERR			-100
#define QC_DIS_AUTH_ERR		-999

/* register command error code */
#define QC_REG_ERR			-101
#define QC_REG_PC_DHCP_ERR	-102

/* get command error code */
#define QC_GET_ERR			-201
#define QC_GET_TYPE_LEN_ERR -202
#define QC_GET_TYPE_ERR		-203

/* set command error code */
#define QC_SET_ERR			-301
#define QC_SET_MAC_LEN_ERR	-302
#define QC_SET_DATA_ERR		-303
#define QC_SET_WAN_TYPE_ERR -304
#define QC_SET_IP_ERR		-305
#define QC_SET_MASK_ERR		-306
#define QC_SET_GATEWAY_ERR  -307
#define QC_SET_MTU_ERR		-308
#define QC_SET_USERNAME_ERR -309
#define QC_SET_PASSWORD_ERR -310
#define QC_SET_SSID_ERR		-311
#define QC_SET_KEY_ERR		-312
#define QC_SET_AUTHTYPE_ERR	-313
#define QC_SET_AUTHKEY_ERR	-314
#define QC_SET_PPTP_SERVER_IP_ERR -315
#define QC_SET_L2TP_SERVER_IP_ERR -316
#define QC_SET_BIGPOND_SRVNAME_ERR -317
#define QC_SET_DNS_1_ERR -318
#define QC_SET_DNS_2_ERR -319


/* dial command error code */
#define QC_DIAL_ERR				-401
#define QC_DIAL_DATA_ERR		-402
#define QC_DIAL_USERNAME_ERR	-403
#define QC_DIAL_PASSWORD_ERR	-404
#define QC_DIAL_WAN_TYPE_ERR	-405
#define QC_DIAL_FAIL_ERR		-406

/* WAN port categories & types */
#define WAN_CAT_UNKNOWN			0
#define WAN_CAT_PPPOE			1
#define WAN_CAT_NOT_PPPOE		2	/* may be DHCP, PPTP, or L2TP, ... */

#define WAN_TYPE_UNKNOWN		0
#define WAN_TYPE_STATIC_PPPOE	1
#define WAN_TYPE_DYNAMIC_PPPOE	2
#define WAN_TYPE_DHCP			3
#define WAN_TYPE_STATIC_IP		4
#define WAN_TYPE_STATIC_L2TP	5
#define WAN_TYPE_DYNAMIC_L2TP	6
#define WAN_TYPE_STATIC_PPTP	7
#define WAN_TYPE_DYNAMIC_PPTP	8
#define WAN_TYPE_BIGPOND        9

/* command enumerator */
#define	CMD_DISCOVERY			1 /* Discovery */
#define	CMD_REGISTER			2 /* Register */
#define	CMD_GET_CFG				3 /* Get Configuration */
#define	CMD_SET_CFG				4 /* Set Configuration */
#define	CMD_REBOOT				5 /* Reboot device */
#define	CMD_DIAL				6 /* PPPoE Dial */


/* TLV set enumerator */
#define TYPE_HWID					101
#define	TYPE_MODELNAME				102
#define TYPE_MAC					103
#define	TYPE_PC_DHCP_OR_NOT			104	/* PC client side is DHCP or static */

/* interface type defination */
#define TYPE_IFS_1					10001
#define	TYPE_IFS_1_TYPE				10002 /* WAN port type */
#define TYPE_IFS_1_NAME				10003 /* interface name */
#define	TYPE_IFS_1_IP				10004 /* WAN port IP */
#define TYPE_IFS_1_MASK				10005 /* WAN port Netmask */
#define	TYPE_IFS_1_GATEWAY			10006 /* gateway */
#define TYPE_IFS_1_MTU				10007 /* MTU */
#define	TYPE_IFS_1_SERVICES_NAME	10008 /* Service Name */
#define TYPE_IFS_1_IDLE_TIME		10009 /* idle time */
#define	TYPE_IFS_1_DNS_1			10010 /* DNS 1 */
#define TYPE_IFS_1_DNS_2			10011 /* DNS 2 */
#define	TYPE_IFS_1_USERNAME			10012 /* PPPoE username */
#define TYPE_IFS_1_PASSWORD			10013 /* PPPoE password */
#define	TYPE_IFS_1_ON_DEMAND		10014 /* dial on demand */
#define TYPE_IFS_1_AUTO_RECONNECT	10015 /* auto reconnect */
#define	TYPE_WAN_STATUS				10016 /* (Read-only) */
#define TYPE_WLAN_SSID				10017 //get/set
#define	TYPE_WLAN_CHANNEL			10018 //get/set
#define TYPE_WLAN_ENCRYPTION_TYPE	10019 //get/set
#define	TYPE_WLAN_KEY				10020 //set
#define TYPE_ID						10021
#define	TYPE_PASSWORD				10022
#define TYPE_PPTP_SERVER_IP			10023
#define TYPE_L2TP_SERVER_IP			10024
#define TYPE_BIGPOND_SERVER_NAME	10025
#define TYPE_WLAN_DOMAIN			10026
#define TYPE_SUPERG_MODE			10027

//#define TYPE_AUTOCHANSELECT_MODE	10028
//SP 2.0
#define TYPE_WLAN_AUTOCHANSELECT_MODE 10028 //get/set

#define TYPE_DETECT_ETHERNET_CABLE_CONNECTED			10030
#define TYPE_CLONE_WAN_MAC			10036
//SP 2.0
#define TYPE_WLAN_MODE_CAPABILITY		10050 //get
#define TYPE_WLAN_MODE					10051 //get/set
#define TYPE_WLAN_CHANNEL_LIST			10052 //get
#define TYPE_WLAN_NUMBER				10060 //get
//5G interface 2
#define TYPE_WLAN2_SSID					10070 //get/set
#define	TYPE_WLAN2_CHANNEL				10071 //get/set
#define	TYPE_WLAN2_ENCRYPTION_TYPE		10072 //get/set
#define	TYPE_WLAN2_KEY					10073 //set
#define	TYPE_WLAN2_AUTOCHANSELECT_MODE	10074 //get/set
#define	TYPE_WLAN2_MODE_CAPABILITY		10075 //get
#define	TYPE_WLAN2_MODE					10076 //get/set
#define TYPE_WLAN2_CHANNEL_LIST			10077 //get
//5G interface 2

/* auth type */
#define AUTHTYPE_NONE				0
#define AUTHTYPE_WEP64				1
#define AUTHTYPE_WPAPSK				2
#define AUTHTYPE_WEP64HEX			3
#define	AUTHTYPE_WEP128				4
#define	AUTHTYPE_WEP128HEX			5
#define	AUTHTYPE_UNKNOWN			9

/*
 *  * QuickConfig protocol packet header
 *   */
typedef struct tagPACKETHEADER {
	unsigned short	cmd;
	short			ret_code;
	unsigned short	client_id;
	unsigned short	seq_no;
} PACKETHEADER;

typedef PACKETHEADER* PPACKETHEADER;
typedef const PACKETHEADER* PKPACKETHEADER;

#define QC_PKT_HDR_LEN sizeof(PACKETHEADER)


/*
 *  * QuickConfig protocol TAG (TLV set) header
 *   */
typedef struct tagTAGHEADER {
	unsigned short	type;
	unsigned short	len;
} TAGHEADER;

typedef TAGHEADER* PTAGHEADER;

typedef struct EthtagTAGHEADER {
	PTAGHEADER tag;
	char	  val;
} TAGHEADER_ETH;__attribute__ ((packed));

typedef const TAGHEADER* PKTAGHEADER;


#define QC_TAG_HDR_LEN sizeof(TAGHEADER)



#define QC_PORT		2003
#define MAX_BUF_SIZE 512

#define UC_LEN sizeof(unsigned char)
#define US_LEN sizeof(unsigned short)
#define UL_LEN sizeof(unsigned long)

#define MAC_LEN 6


/*
* Date: 2008-01-16
* Name: NickChou
* Detail: for wireless domanin is not 0x10 or 0x30.
          So I can use IOCTL to get wireless channel from Atheros wireless driver.
          Modify from "wlanconfig ath0 list chan" command.
*/
// copy from 802_11\common\umac\include\_ieee80211.h
struct ieee80211_channel {
    u_int16_t       ic_freq;        /* setting in Mhz */
    u_int32_t       ic_flags;       /* see below */
    u_int8_t        ic_flagext;     /* see below */
    u_int8_t        ic_ieee;        /* IEEE channel number */
    int8_t          ic_maxregpower; /* maximum regulatory tx power in dBm */
    int8_t          ic_maxpower;    /* maximum tx power in dBm */
    int8_t          ic_minpower;    /* minimum tx power in dBm */
};

//copy from 802_11\linux\net80211\ieee80211_ioctl.h
struct ieee80211req_chaninfo {
	u_int	ic_nchans;
	struct ieee80211_channel ic_chans[255]; //IEEE80211_CHAN_MAX=255
};

/*copy from wireless.h, for using ioctl getting wlan channel*/
struct	iw_freq
{
	int	m;		/* Mantissa */
	int	e;		/* Exponent */
	int	i;		/* List index (when in range struct) */
	int	flags;	/* Flags (fixed/auto) */
};

struct	iw_point
{
	void	*pointer;	/* Pointer to the data  (in user space) */
	int		length;		/* number of fields or size in bytes */
	int		flags;		/* Optional params */
};

union	iwreq_data
{
	/* Config - generic */
	char		name[IFNAMSIZ];
	/* Name : used to verify the presence of  wireless extensions.
	 * Name of the protocol/provider... */
	struct iw_freq	freq;		/* frequency or channel :
					 * 0-1000 = channel
					 * > 1000 = frequency in Hz */
	struct iw_point	data;		/* Other large parameters */
};

struct	iwreq
{
	union
	{
		char	ifrn_name[16];	/* if name, e.g. "eth0" IFNAMSIZ=16*/
	} ifr_ifrn;

	/* Data part (defined just above) */
	union	iwreq_data	u;
};

typedef struct	iw_freq iwfreq;

#endif
