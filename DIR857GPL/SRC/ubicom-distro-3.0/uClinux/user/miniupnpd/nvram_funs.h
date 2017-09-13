#ifndef __NVRAM_FUNS_H__
#define __NVRAM_FUNS_H__

#if defined(CHECK_VS_NVRAM) || defined(CHECK_PF_NVRAM)
#define NVRAM_PATH "/var/etc/nvram.conf"
#define NVRAM_PATH_TMP "/tmp/nvram.conf.tmp"
#define NVRAM_PATH_PF_TMP "/tmp/nvram.conf.pf.tmp"
#define PRO_LEN 5
#define HOST_LEN 16
#define PORT_LEN 6
#define DESC_LEN 64
#define DURATION_LEN 12
#define SCHEDULE_NAME_SIZE 32
#define FILE_LINE_LEN (PRO_LEN + 2*HOST_LEN + 2*PORT_LEN + DESC_LEN + 2*DURATION_LEN + SCHEDULE_NAME_SIZE)
#define NVRAM_VS_RULE_NO 25

/*	Date:	2009-06-09
*	Name:	jimmy huang
*	Reason:	Avoid miniupnpd heavy loading cause not response some UPnP soap
*			We decrease the frequency for open nvram.conf using new mechanism
*			we update vs rule in nvram only when
*			1. first start, read in all nvram vs rule data
*			2. receive signal from httpd, update all nvram vs rule data
*/
typedef struct vf_rules_in_nvram_t{
	int enable;
	char desc[80];
	int proto; //6 TCP, 17 UDP, 256 in nvram means BOTH (TCP + UDP)
	unsigned short e_port;
	unsigned short i_port;
	char int_ip[INET_ADDRSTRLEN];
	long int duration;
}vf_rules_in_nvram;


extern vf_rules_in_nvram vf_in_nvram[NVRAM_VS_RULE_NO];
extern int global_vs_rule_num;

void update_nvram_vs_rules(void);
int init_vs_struct(void);

int nvram_GetVsRules_Total_Number(void);

int nvram_GetSpecificPortMappingEntry(unsigned short externalPort_in
			, int protocol_in
			, unsigned short *internalPort_out
			, char *internalClient_out
			, int *enable_out
			, char *desc_out
			, long int *duration_out);

int nvram_GetGenericPortMappingEntry(int index_local
			, unsigned short * externalPort_out
			, int *protocol_out
			, unsigned short * internalPort_out
			, char *internalClient_out
			, int *enable_out
			, char *desc_out
			, long int *duration_out);

#endif

#endif
