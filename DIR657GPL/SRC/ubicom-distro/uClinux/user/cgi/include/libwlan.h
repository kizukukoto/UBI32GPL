#ifndef __WLAN_IW_FREQ__
#define __WLAN_IW_FREQ__

#ifdef UBICOM_JFFS2
#define WLAN0_ETH "ath0"
#else
#define WLAN0_ETH nvram_safe_get("wlan0_eth")
#endif

struct  iw_freq
{
	int m;              /* Mantissa */
	int e;              /* Exponent */
	int i;              /* List index (when in range struct) */
	int flags;  /* Flags (fixed/auto) */
};

struct  iw_param
{
	int value;          /* The value of the parameter itself */
	int fixed;          /* Hardware should not use auto select */
	int disabled;       /* Disable the feature */
	int flags;          /* Various specifc flags (if any) */
};

union   iwreq_data
{
        struct iw_freq  freq;   /* frequency or channel*/
        struct sockaddr ap_addr;
        struct iw_param param;          /* Other small parameters */

};

struct  iwreq
{
	union {
		char    ifrn_name[16];  /* if name, e.g. "eth0" */
	} ifr_ifrn;

        union iwreq_data u;
};

//typedef struct  iw_freq iwfreq;
#endif

