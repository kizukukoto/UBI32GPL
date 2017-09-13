
#define xmlWrite(xml_result, fmt, args...) ({sprintf(xml_result, fmt, ## args);})
#define MIN(a, b)	((a) < (b)? (a): (b))

/* call in httpd.c */
extern void pure_init(int conn_fd,char *protocol,long content_length,char *content_type,char *modified,char *action_string);
/* call in core.c */
extern void do_pure_action(char *url, FILE *stream, int len, char *boundary);
extern void do_restart_cgi(char *url, FILE *stream);
extern void send_pure_response(char *url, FILE *stream);
extern void do_pure_check(char *url, FILE *stream);
extern void pure_authorization_error(void);

extern const char VER_FIRMWARE[];

/* management user name and password */
#define DEV_HOST_NAME_LEN   62

/* wireless structure */
#define WLA_ESSID_LEN       512

/* wan settings structure */
#define POE_NAME_LEN        256
#define POE_PASS_LEN        512

/* max response lenth */
#define MAX_RES_LEN         10240
#define MAX_XML_LEN         5120
#define MAX_REC_LEN         5120

/* others  */
#define MAX_ARRAY_TMP       2048//1000

/* for pure object */
typedef struct pure_atrs_s{		
	/* init in httpd.c */
	int conn_fd;
	long content_length;	
	char *if_modified_since;
	char *content_type;			
	char *protocol;	
	char *action_string;
		
	/* pure.c (set_pure_settings) */
	int reboot;		
	int action;			
	int set_nvram_error;						
						
	/* init in pure.c (add to response)*/
	char response[MAX_RES_LEN];		
	int response_size;			
	int response_len;				
	int status;							
	long bytes;				
		
	/* init in pure.c (ConnectedDevice) */			
	char wlanTxPkts[10];
	char wlanRxPkts[10];
	char wlanTxBytes[10];
	char wlanRxBytes[10];
	char lanTxPkts[10];
	char lanRxPkts[10];
	char lanTxBytes[10];
	char lanRxBytes[10];		
	
	/* init in pure.c for get soap body (if soap header and soap body in one package)*/			
	FILE *stream;	
	
}pure_atrs_t;

/* for mac filter */
typedef struct mac_filter_list_s{
	char *mac_address;	
}mac_filter_list_t;

/* for vs rule */
typedef struct vs_rule_s{
	char *rule;	
}vs_rule_t;

/* for connected devices*/
typedef struct cnted_client_list_s{
	char *cnted_time;
	char *mac_add;
	char *dev_name;
	char *port_name;
	char *wireless;
	char *active;
}cnted_client_list_t;

/* for wan settings*/
typedef struct wan_settings_s{
	char *protocol;
	char *name;
	char *password;
	char *idle_time;
	char *service;
	char *mode;				// always_on | manual | on_demand , not support on_demand
	char *ip;	
	char *netmask;
	char *gateway;
	char *primary_dns;
	char *secondary_dns;
	char *mtu;
	char *mac;
	char *dynamic;
#ifdef OPENDNS
	char *opendns;
#endif
}wan_settings_t;

/* for wan settings struction */
typedef struct pppoe_pptp_l2tp_s{
	char *mode;
	char *name;
	char *password;
	char *idle_time;
	char *service;	  // in pptp,l2tp is server ip 
	char *ip;	
	char *netmask;
	char *gateway;
	char *dynamic;
}pppoe_pptp_l2tp_t;

/* handle long string with type string1/string2/string3/string4/.... into struct */
#define MAX_ITEM_COUNT			  10
#define MAX_ITEM_LENGTH			  20
typedef struct LONG_ITEM_T{
	int  count;
	char *item[MAX_ITEM_LENGTH];
}LONG_ITEM;
extern pure_atrs_t pure_atrs;
extern wan_settings_t wan_settings;
extern cnted_client_list_t cnted_client_table[4];    // max client number
