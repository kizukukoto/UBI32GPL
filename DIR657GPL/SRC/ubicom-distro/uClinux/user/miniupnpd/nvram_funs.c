#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

#include "config.h"
#include "upnpglobalvars.h"
#include "nvram_funs.h"

#ifdef MINIUPNPD_DEBUG_NVRAM_FUNS
#define DEBUG_MSG(fmt, arg...)       printf("%s [%d]: "fmt, __FILE__, __LINE__, ##arg)
#else
#define DEBUG_MSG(fmt, arg...) 
#endif

/*	Date:	2009-06-09
*	Name:	jimmy huang
*	Reason:	Avoid miniupnpd heavy loading cause not response some UPnP soap
*			We decrease the frequency for open nvram.conf using new mechanism
*			we update vs rule in nvram only when
*			1. first start, read in all nvram vs rule data
*			2. receive signal from httpd, update all nvram vs rule data
*/
#if defined(CHECK_VS_NVRAM) || defined(CHECK_PF_NVRAM)
vf_rules_in_nvram vf_in_nvram[NVRAM_VS_RULE_NO];
int global_vs_rule_num;

// function to initialize vf_rules_in_nvram structure , need to tell the array size
int init_vs_struct(void){
	int i = 0;
	
	for(i = 0 ; i < NVRAM_VS_RULE_NO ; i++){
		vf_in_nvram[i].enable = 0;
		memset(vf_in_nvram[i].desc , '\0' , sizeof(vf_in_nvram[i].desc));
		vf_in_nvram[i].proto = 0;
		vf_in_nvram[i].e_port = 0;
		vf_in_nvram[i].i_port = 0;
		memset(vf_in_nvram[i].int_ip , '\0' , sizeof(vf_in_nvram[i].int_ip));
		vf_in_nvram[i].duration = 0;
	}
	return 1;
}

// function to sync nvram virtual server value
void update_nvram_vs_rules(void){
	int cur_pos = 0;
	FILE *fp = NULL;
	char buf[256] = {0};
	char *p = NULL;
	
	char *enable_tmp = NULL;
	char *protocol_tmp = NULL, *desc_tmp = NULL;
	char *externalPort_tmp = NULL;
	char *internalPort_tmp = NULL, *internalClient_tmp = NULL;
	char *useless = NULL;
	
	DEBUG_MSG("%s (%d): begin\n",__FUNCTION__,__LINE__);
#ifdef CHECK_VS_NVRAM
	// only need Enabled vs rule
	//sprintf(cmd,"cat %s |grep vs_rule |grep -v \"=0//\" > %s",NVRAM_PATH,NVRAM_PATH_TMP);
	
	//Format: enable/name/protocol/ext_port/int_port/ip_addr/schedule_name/filter_type
	// we need both enabled / disabled vs rule, does not need useless rule (ext_port = 0, int_port = 0)
	// useless rule : vs_rule_20=0//6/0/0/0.0.0.0/Always/Allow_All
	sprintf(buf,"cat %s |grep vs_rule |grep -v \"/0/0/\" > %s",NVRAM_PATH,NVRAM_PATH_TMP);
	system(buf);

	init_vs_struct();

	DEBUG_MSG("%s (%d): after init_vs_struct\n",__FUNCTION__,__LINE__);
	if((fp = fopen(NVRAM_PATH_TMP,"r"))!=NULL){
		while(!feof(fp)){
			if(cur_pos >= NVRAM_VS_RULE_NO ){
				break;
			}
			fgets(buf,FILE_LINE_LEN,fp);
			if(!feof(fp) && (buf[0]!='\n') ){
				if(strlen(buf) < strlen("vs_rule_0x=xxx\n")){
					// useless rule
					// ex : vs_rule_23=
					continue;
				}
				p = strchr(buf,'=');
				p = p + 1;
				//Format: enable/name/protocol/ext_port/int_port/ip_addr/schedule_name/filter_type
				get_element(p,"/","%s %s %s %s %s %s %s %s"
							,&enable_tmp
							,&desc_tmp
							,&protocol_tmp
							,&externalPort_tmp, &internalPort_tmp , &internalClient_tmp
							,&useless
							,&useless
							);
				/* only when it has name, it's valid */
				if((desc_tmp == NULL) || (strlen(desc_tmp) == 0)){
					continue;
				}

				vf_in_nvram[cur_pos].enable = atoi(enable_tmp);
				vf_in_nvram[cur_pos].proto = atoi(protocol_tmp);
				if(vf_in_nvram[cur_pos].proto < 0){
					vf_in_nvram[cur_pos].proto = 0;
				}
				vf_in_nvram[cur_pos].e_port = atoi(externalPort_tmp);
				vf_in_nvram[cur_pos].i_port = atoi(internalPort_tmp);
				if(internalClient_tmp){
					strcpy(vf_in_nvram[cur_pos].int_ip,internalClient_tmp);
				}
				if(desc_tmp)
					strcpy(vf_in_nvram[cur_pos].desc,desc_tmp);

				vf_in_nvram[cur_pos].duration = 0;
				cur_pos++;
			}
		}
		fclose(fp);
	}
	unlink(NVRAM_PATH_TMP);
#endif
	global_vs_rule_num = cur_pos;

	//return cur_pos;
}

//	this fucntion gets virtual server rules for specific proto + port 
//	within the structure read from nvram 
int nvram_GetSpecificPortMappingEntry(unsigned short externalPort_in
			, int protocol_in
			, unsigned short *internalPort_out
			, char *internalClient_out
			, int *enable_out
			, char *desc_out
			, long int *duration_out){

	int result = 0;
	int cur_pos = 0;

	DEBUG_MSG("\n%s (%d): begin\n",__FUNCTION__,__LINE__);
	if((externalPort_in == 0) || (protocol_in == 0)) {
		return 0;
	}
#ifdef CHECK_VS_NVRAM
	for(cur_pos = 0 ; cur_pos < NVRAM_VS_RULE_NO ; cur_pos++){
		if(externalPort_in == vf_in_nvram[cur_pos].e_port){
			if((vf_in_nvram[cur_pos].proto == 0) || (vf_in_nvram[cur_pos].e_port == 0)){
				// end of the useful structure list
				break;
			}
			if((vf_in_nvram[cur_pos].proto != 0) && (protocol_in == vf_in_nvram[cur_pos].proto)){
				*internalPort_out = vf_in_nvram[cur_pos].i_port;
				if(vf_in_nvram[cur_pos].int_ip){
					strcpy(internalClient_out,vf_in_nvram[cur_pos].int_ip);
				}
				*enable_out = vf_in_nvram[cur_pos].enable;
				if(desc_out)
					strcpy(desc_out,vf_in_nvram[cur_pos].desc);
				*duration_out = 0;
				result = 1;
				break;
			}
		}
	}
#endif
	return result;
}

// this fucntion gets virtual server rules from nvram for specific number
int nvram_GetGenericPortMappingEntry(int index_local
			, unsigned short * externalPort_out
			, int *protocol_out
			, unsigned short * internalPort_out
			, char *internalClient_out
			, int *enable_out
			, char *desc_out
			, long int *duration_out){

	int result = 0;
	int cur_pos = 0;
	DEBUG_MSG("\n%s (%d): begin\n",__FUNCTION__,__LINE__);

	if(index_local < 0){
		return 0;
	}

#ifdef CHECK_VS_NVRAM
	//Format: enable/name/protocol/ext_port/int_port/ip_addr/schedule_name/filter_type
	// we need both enabled / disabled vs rule, does not need useless rule (ext_port = 0, int_port = 0)
	// useless rule : vs_rule_20=0//6/0/0/0.0.0.0/Always/Allow_All

	for(cur_pos = 0 ; cur_pos < NVRAM_VS_RULE_NO ; cur_pos++){
		/* only when it has name, it's valid */
		if( (strlen(vf_in_nvram[cur_pos].desc) == 0) || (vf_in_nvram[cur_pos].e_port == 0) ){
			// end of the useful structure list
			break;
		}
		if(cur_pos == index_local){
			*externalPort_out = vf_in_nvram[cur_pos].e_port;
			*internalPort_out = vf_in_nvram[cur_pos].i_port;
			*protocol_out = vf_in_nvram[cur_pos].proto;
			/*
			* Note: when get_upnp_rules_state_list()
			*       it will call get_redirect_rule_by_index() with iaddr = 0, desc = 0
			*       which means the 2 variable does not has memory location to save
			*       new values
			*/
			if(desc_out){
				strcpy(desc_out,vf_in_nvram[cur_pos].desc);
			}
			if(internalClient_out){
				strcpy(internalClient_out,vf_in_nvram[cur_pos].int_ip);
			}
			*duration_out = 0;
			result = 1;
			break;
		}
	}
#endif
#ifdef CHECK_PF_NVRAM
	// Not implement
#endif

	DEBUG_MSG("%s (%d): end with result = %d\n",__FUNCTION__,__LINE__,result);
	return result; 
}

int nvram_GetVsRules_Total_Number(void)
{
	return global_vs_rule_num;
}
#endif
//--------------------------------------



#if 0
#if defined(CHECK_VS_NVRAM) || defined(CHECK_PF_NVRAM)

// this fucntion gets virtual server rules from nvram for specific proto + port
int nvram_GetSpecificPortMappingEntry(unsigned short externalPort_in
			, int protocol_in
			, unsigned short *internalPort_out
			, char *internalClient_out
			, int *enable_out
			, char *desc_out
			, long int *duration_out){
	char cmd[256] = {0};
	char buf[FILE_LINE_LEN] = {0};
	char *useless = NULL;
	char *p = NULL;
	FILE *fp;
	int result = 0;

	char *enable_tmp = NULL;
	char *protocol_tmp = NULL, *desc_tmp = NULL;
	char *externalPort_tmp = NULL;
	char *internalPort_tmp = NULL, *internalClient_tmp = NULL;
	char char_externalPort_in[7]={0};
	
	DEBUG_MSG("\n%s (%d): begin\n",__FUNCTION__,__LINE__);
	
	if((externalPort_in == 0) || (protocol_in == 0)) {
		return 0;
	}
	sprintf(char_externalPort_in,"%hu",externalPort_in);
	//Format: enable/name/protocol/ext_port/int_port/ip_addr/schedule_name/filter_type

#ifdef CHECK_VS_NVRAM
	// need both enabled / disabled vs rule, does not need useless rule (ext_port = 0, int_port = 0)
	// useless rule : vs_rule_20=0//6/0/0/0.0.0.0/Always/Allow_All
	sprintf(cmd,"cat %s |grep vs_rule |grep -v \"/0/0/\" > %s",NVRAM_PATH,NVRAM_PATH_TMP);
	system(cmd);

	if((fp = fopen(NVRAM_PATH_TMP,"r"))!=NULL){
		while(!feof(fp)){
			fgets(buf,FILE_LINE_LEN,fp);
			if(!feof(fp) && (buf[0]!='\n') ){
				if(strlen(buf) < strlen("vs_rule_0x=xxx\n")){
					// useless rule
					// ex : vs_rule_23=
					continue;
				}

				p = strchr(buf,'=');
				p = p + 1;

				//Format: enable/name/protocol/ext_port/int_port/ip_addr/schedule_name/filter_type
				get_element(p,"/","%s %s %s %s %s %s %s"
							,&enable_tmp
							,&desc_tmp
							,&protocol_tmp
							,&externalPort_tmp, &internalPort_tmp , &internalClient_tmp
							,&useless
							,&useless
							);
				/* only when it has name, it's valid */
				if((desc_tmp == NULL) || (strlen(desc_tmp) == 0)){
					continue;
				}

				if(strcmp(char_externalPort_in,externalPort_tmp) == 0){
					if((protocol_tmp != 0) && protocol_in == atoi(protocol_tmp)){
						//strcpy(internalPort_out,internalPort_tmp);
						sscanf(internalPort_tmp,"%hu,",internalPort_out);
						if(internalClient_out)
							strcpy(internalClient_out,internalClient_tmp);
						*enable_out = atoi(enable_tmp);
						if(desc_out)
							strcpy(desc_out,desc_tmp);
						*duration_out = 0;
						result = 1;
						break;
					}
				}
			}
		}

		fclose(fp);
	}else{
		//no rules in nvram
		result = 0;
	}
	unlink(NVRAM_PATH_TMP);
#endif
	DEBUG_MSG("%s (%d): End with result = %d\n",__FUNCTION__,__LINE__,result);
	return result;
}



/* this fucntion gets virtual server rules from nvram for specific number */
int nvram_GetGenericPortMappingEntry(int index_local
			, unsigned short * externalPort_out
			, int *protocol_out
			, unsigned short * internalPort_out
			, char *internalClient_out
			, int *enable_out
			, char *desc_out
			, long int *duration_out){
	int index_file = 0;
	char cmd[256] = {0};
	char buf[FILE_LINE_LEN] = {0};
	char *useless = NULL;
	char *p = NULL;
	FILE *fp;
	int result = 0;

	char *enable_tmp = NULL;
	char *protocol_tmp = NULL, *desc_tmp = NULL;
	char *externalPort_tmp = NULL;
	char *internalPort_tmp = NULL, *internalClient_tmp = NULL;

	DEBUG_MSG("\n%s (%d): begin\n",__FUNCTION__,__LINE__);
	
	if(index_local < 0){
		return 0;
	}

	/*
	 * Note: when get_upnp_rules_state_list()
	 *       it will call get_redirect_rule_by_index() with iaddr = 0, desc = 0
	 *       which means the 2 variable does not has memory location to save
	 *       new values
	 */

#ifdef CHECK_VS_NVRAM
	// only need Enabled vs rule
	//sprintf(cmd,"cat %s |grep vs_rule |grep -v \"=0//\" > %s",NVRAM_PATH,NVRAM_PATH_TMP);
	
	//Format: enable/name/protocol/ext_port/int_port/ip_addr/schedule_name/filter_type
	
	// need both enabled / disabled vs rule, does not need useless rule (ext_port = 0, int_port = 0)
	// useless rule : vs_rule_20=0//6/0/0/0.0.0.0/Always/Allow_All
	sprintf(cmd,"cat %s |grep vs_rule |grep -v \"/0/0/\" > %s",NVRAM_PATH,NVRAM_PATH_TMP);
	system(cmd);

	if((fp = fopen(NVRAM_PATH_TMP,"r"))!=NULL){
		while(!feof(fp)){
			fgets(buf,FILE_LINE_LEN,fp);
			if(!feof(fp) && (buf[0]!='\n') ){
				if(strlen(buf) < strlen("vs_rule_0x=xxx\n")){
					// useless rule
					// ex : vs_rule_23=
					continue;
				}
				p = strchr(buf,'=');
				p = p + 1;
				//Format: enable/name/protocol/ext_port/int_port/ip_addr/schedule_name/filter_type
				get_element(p,"/","%s %s %s %s %s %s %s %s"
							,&enable_tmp
							,&desc_tmp
							,&protocol_tmp
							,&externalPort_tmp, &internalPort_tmp , &internalClient_tmp
							,&useless
							,&useless
							);
				/* only when it has name, it's valid */
				if((desc_tmp == NULL) || (strlen(desc_tmp) == 0)){
					continue;
				}
				if(index_file != index_local){
					index_file++;
					continue;
				}
				sscanf(externalPort_tmp,"%hu,",externalPort_out);
				//6 TCP, 17 UDP, 256 in nvram means BOTH (TCP + UDP)
				(*protocol_out) = atoi(protocol_tmp);
				if(protocol_out <0 ){
					protocol_out = 0;
				}
				sscanf(internalPort_tmp,"%hu,",internalPort_out);
				*enable_out = atoi(enable_tmp);
				if(internalClient_out){
					strcpy(internalClient_out,internalClient_tmp);
				}
				if(desc_out)
					strcpy(desc_out,desc_tmp);

				*duration_out = 0;
				result = 1;
				break;
			}
		}
		fclose(fp);
	}else{
		//no rules in nvram
		result = 0;
	}
	unlink(NVRAM_PATH_TMP);
#endif
	DEBUG_MSG("%s (%d): end with result = %d\n",__FUNCTION__,__LINE__,result);
	return result;
}

/* this fucntion gets virtual server rules from nvram for specific number */
int nvram_GetVsRules_Total_Number(void)
{
	char cmd[256] = {0};
	char buf[FILE_LINE_LEN] = {0};
//	char *p = NULL;
	int num = 0;
	FILE *fp;
	DEBUG_MSG("\n%s (%d): begin\n",__FUNCTION__,__LINE__);
#ifdef CHECK_VS_NVRAM
	/*
		Format for nvram virtual server
		enable/name/protocol/ext_port/int_port/ip_addr/schedule_name
	*/
	// ignore disabled rules
	//sprintf(cmd,"cat %s |grep vs_rule |grep -v \"=0//\" > %s",NVRAM_PATH,NVRAM_PATH_TMP);

	// we need all rules, except export and int_port are 0
	sprintf(cmd,"cat %s |grep vs_rule |grep -v \"/0/0/\" > %s",NVRAM_PATH,NVRAM_PATH_TMP);
	system(cmd);
	if((fp = fopen(NVRAM_PATH_TMP,"r"))!=NULL){
		while(!feof(fp)){
			fgets(buf,FILE_LINE_LEN,fp);
			if(!feof(fp) && (buf[0]!='\n') && (strlen(buf) > strlen("vs_rule_0x=xx"))){
				num ++;
			}
		}
		fclose(fp);
	}
#endif
#ifdef CHECK_PF_NVRAM
	/*
		Format for nvram port forward rule
		enable/name/int_ip_addr/tcp_ex_port/udp_ex_port/schedule_name/inbound_filter
	*/
	// ignore disabled rules
	//sprintf(cmd,"cat %s |grep vs_rule |grep -v \"=0//\" > %s",NVRAM_PATH,NVRAM_PATH_TMP);

	// we need all rules, except export and int_port are 0
	sprintf(cmd,"cat %s |grep vs_rule |grep -v \"/0/0/\" > %s",NVRAM_PATH,NVRAM_PATH_TMP);
	system(cmd);
	if((fp = fopen(NVRAM_PATH_TMP,"r"))!=NULL){
		while(!feof(fp)){
			fgets(buf,FILE_LINE_LEN,fp);
			if(!feof(fp) && (buf[0]!='\n') && (strlen(buf) > strlen("vs_rule_0x=xx"))){
				num ++;
			}
		}
		fclose(fp);
	}
#endif
	DEBUG_MSG("%s (%d): End with num = %d\n",__FUNCTION__,__LINE__,num);
	return num;
}

#endif
#endif
