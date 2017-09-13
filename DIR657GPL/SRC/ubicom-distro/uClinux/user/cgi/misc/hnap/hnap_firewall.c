#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <signal.h>
#include <fcntl.h>

#include "libdb.h"
#include "debug.h"
#include "hnap.h"
#include "hnapfirewall.h"


/* for PortMappings & PortMappings Count */
vs_rule_t vs_rule_table[] = {
	{"vs_rule_00"},{"vs_rule_01"},{"vs_rule_02"},{"vs_rule_03"},
	{"vs_rule_04"},{"vs_rule_05"},{"vs_rule_06"},{"vs_rule_07"},
	{"vs_rule_08"},{"vs_rule_09"},{"vs_rule_10"},{"vs_rule_11"},
	{"vs_rule_12"},{"vs_rule_13"},{"vs_rule_14"},{"vs_rule_15"},
	{"vs_rule_16"},{"vs_rule_17"},{"vs_rule_18"},{"vs_rule_19"}
};

/* for MacFilter2 */
mac_filter_list_t mac_filter_table[] = {
	{"mac_filter_00"},{"mac_filter_01"},{"mac_filter_02"},{"mac_filter_03"},
	{"mac_filter_04"},{"mac_filter_05"},{"mac_filter_06"},{"mac_filter_07"},
	{"mac_filter_08"},{"mac_filter_09"},{"mac_filter_10"},{"mac_filter_11"},
	{"mac_filter_12"},{"mac_filter_13"},{"mac_filter_14"},{"mac_filter_15"},
	{"mac_filter_16"},{"mac_filter_17"},{"mac_filter_18"},{"mac_filter_19"}
};

static char *find_next_element(char *source_xml, char *element_name){
        char *index1 = NULL;
        char *ch = NULL;

        index1 = strstr(source_xml, element_name);
        if (index1 != 0){
                while ((*(ch = index1 - 1) != '<') || (*(ch = index1 + strlen(element_name)) != '>'
                                                && *(ch = index1 + strlen(element_name)) != ' ' && *(ch = index1 + strlen(element_name)) != '/')){            
                        index1 += strlen(element_name);
                        index1 = strstr(index1, element_name);

                        if (index1 == NULL)
                                return index1;
        }

                index1 += strlen(element_name) + 1;
                source_xml = index1;
        }else{
                source_xml = NULL;
        }

        return source_xml;
}

int check_mac_filter(char *mac_t)
{
	int index_t = 0;
	char *mac_tmp = NULL;
	char *mac_tmp_index = NULL;
	char mac[18] = {};
	

	/* hnap : multicast address will return 0 */
	/* condition protection , may add more conditions */
	/*
	if(	strcasecmp(mac_t,"") == 0 || \
		mac_check(mac_t,0) == 0 )	{			
		cprintf("XXX %s[%d] XXX\n",__FUNCTION__,__LINE__);						
		return 0;
	}
	*/

	/* check if same mac in table */
	for(index_t=0; index_t<ARRAYSIZE(mac_filter_table); index_t++){
		mac_tmp = nvram_get(mac_filter_table[index_t].mac_address);		
		if(strcmp(mac_tmp,"name/00:00:00:00:00:00/Always") == 0)		// ignore default value 
			continue;
		/* get true mac of nvram and compare to xml value */	
		mac_tmp_index = strstr(mac_tmp,"/");
		strncat(mac,mac_tmp_index+1,17);								
		if( strcasecmp(mac,mac_t)== 0 ) {
			return 0;
		}							
		memset(mac,0,18);
	}
	return 1;
			
			
}

static int transfer_vs_rule(int v_index, char *enable, char *name, char *protocol, char *exter_port, char *inter_port, char *ip, char *schedule)
{
	/*
	*   Date: 2010-05-25
	*   Name: Marine Chiu
	*   Reason: To pass HNAP TestDevice 10.2.8130.1
	*   Notice :  PortMappings case[39],no description, it's hard for sscanf, strtok or others to detect "//", ex: "0//TCP/0/0/0.0.0.0/Always/Allow_All"
	*/
	char *vs_rule = NULL,*ptr=NULL;
	char vs_rule_tmp[200] = {},tmp[200]={};
	int i = 0;
	
	/* transfer nvram value of vs_rule to pure, is a loop for PortMapping_xml in pure_xml.c */		
	vs_rule = nvram_safe_get(vs_rule_table[v_index].rule);						
	memset(vs_rule_tmp, 0, sizeof(vs_rule_tmp));
	strcpy(vs_rule_tmp, vs_rule);					
	
	/*if default value, then ignore*/
	if( strcmp(vs_rule_tmp,"0//TCP/0/0/0.0.0.0/Always/Allow_All") == 0 ||
		strcmp(vs_rule_tmp,"0//6/0/0/0.0.0.0/Always/Allow_All") == 0 || 
		strlen(vs_rule_tmp) == 0)		
			return 0;	
		
	/* hadndle for name = (test 1) .If it has a space in name ,then substitute for "#" (for TestTool) */	
	for(i=0; i<strlen(vs_rule_tmp); i++)											
		if(*(vs_rule_tmp+i) == ' ')				
			*(vs_rule_tmp+i) = '#';										

	/*
	*   Date: 2010-05-25
	*   Name: Marine Chiu
	*   Reason: To pass HNAP TestDevice 10.2.8130.1
	*   Notice :  PortMappings case[39],no description, it's hard for sscanf, strtok or others to detect "//", ex: "0//TCP/0/0/0.0.0.0/Always/Allow_All"
	*/

 	if(ptr= strstr(vs_rule_tmp,"//")) {
		if((strlen(vs_rule_tmp) - strlen(ptr)) < 2) {
			strncpy(tmp,vs_rule_tmp,(strlen(vs_rule_tmp) - strlen(ptr)));
			strcat(tmp,"/#");
			strcat(tmp,ptr);
			strcpy(vs_rule_tmp,tmp);
		}
 	}

	/* substitute "/" to " " , for sscanf function */
	for(i=0; i<strlen(vs_rule_tmp); i++)											
		if(*(vs_rule_tmp+i) == '/')				
			*(vs_rule_tmp+i) = ' ';								
								
	/* format scan in & write to xml */					
	sscanf(vs_rule_tmp,"%s %s %s %s %s %s %s",enable,name,protocol,exter_port,inter_port,ip,schedule);
	/*
	*   Date: 2010-05-25
	*   Name: Marine Chiu
	*   Reason: to pass HNAP TestDevice 10.2.8130
	*   Notice : TCP = 6, UDP = 17
	*/
        if(!strcmp(protocol,"6")) //TCP=6,UDP=17,Others=256
		strcpy(protocol,"TCP");
        else
		strcpy(protocol,"UDP");

	/* substitute "#" for " " (space) done above */
	for(i=0; i<strlen(name); i++)
		if(*(name+i) == '#')
			*(name+i) = ' ';

	return 1;
}


static int do_get_port_mappings_to_xml(char *dest_xml, char *general_xml, char *vs_rule_xml)
{
        int rule_index = 0;
        char vs_rule_tmp[5000] = {};
        char vs_rule_tmp_t[300] = {};
        char enable[2] = {};
        char name[32] = {};
        char protocol[5] = {};
        char ip[16] = {};
        char exter_port[7] = {};
        char inter_port[7] = {};
        char schedule[10] = {};

        /* transfer nvram value of vs_rule to pure, is a loop for PortMapping_xml in pure_xml.c */
        for(rule_index=0; rule_index<ARRAYSIZE(vs_rule_table); rule_index++){
                if(!transfer_vs_rule(rule_index,enable,name,protocol,exter_port,inter_port,ip,schedule))
                        continue;

                sprintf(vs_rule_tmp_t,vs_rule_xml,name,ip,protocol,exter_port,inter_port);
                sprintf(vs_rule_tmp,"%s%s",vs_rule_tmp,vs_rule_tmp_t);
                memset(vs_rule_tmp_t,0,300);
        }

        sprintf(dest_xml,general_xml,"OK",vs_rule_tmp);
        return 1;

}

static int do_add_port_mappings_to_xml(char *source_xml)
{	
	char name[32] = {};
	char ip[16] = {};
	char protocol[5] = {};
	char exter_port[7] = {};
	char inter_port[7] = {};
	char vs_rule[100] = {};
	int rule_index = 0;
	
	/* value in nvram */		
	char enable_n[2] = {};
	char name_n[32] = {};
	char protocol_n[5] = {};
	char ip_n[16] = {};
	char exter_port_n[7] = {};
	char inter_port_n[7] = {};
	char schedule_n[10] = {};	
	
	int full = 1;
	char *vs_rule_tt = NULL;
	
	while(1) {			
			do_get_element_value(source_xml, name, "PortMappingDescription");
			do_get_element_value(source_xml, ip, "InternalClient");
			do_get_element_value(source_xml, protocol, "PortMappingProtocol");
			do_get_element_value(source_xml, exter_port, "ExternalPort");
			do_get_element_value(source_xml, inter_port, "InternalPort");						
						
			for(rule_index=0; rule_index<ARRAYSIZE(vs_rule_table); rule_index++) {
				vs_rule_tt = nvram_safe_get(vs_rule_table[rule_index].rule);				
				if( strcmp(vs_rule_tt, "0//TCP/0/0/0.0.0.0/Always/Allow_All") == 0 ||
					strcmp(vs_rule_tt, "0//6/0/0/0.0.0.0/Always/Allow_All" ) == 0 	||
					strlen(vs_rule_tt) == 0)
					full = 0;
			}
			
			if(full == 1) 
				return 0;	

			
			/* test if xml value is already in nvram */	
			for(rule_index=0; rule_index<ARRAYSIZE(vs_rule_table); rule_index++) {
				if(!transfer_vs_rule(rule_index,enable_n,name_n,protocol_n,exter_port_n,inter_port_n,ip_n,schedule_n))
					continue;
				/* compare xml value and nvram value */												
				if(strcmp(exter_port,exter_port_n) == 0 && strcmp(protocol,protocol_n) == 0) {
					return 1;
				}
			}

			/*
			*   Date: 2010-05-25
			*   Name: Marine Chiu
			*   Reason: to pass HNAP TestDevice 10.2.8130
			*   Notice : TCP = 6, UDP = 17
			*/
			if(!strcasecmp(protocol,"TCP"))
                            strcpy(protocol,"6");
                        else
                            strcpy(protocol,"17");				
						
			/* transfer xml value to vs_rule of nvram */
			sprintf(vs_rule,"%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s",\
				"1","/",name,"/",protocol,"/",exter_port,"/",inter_port,"/",ip,"/","Always","/","Allow_All");
				
			/* if vs_rule = 0//TCP/0/0/0.0.0.0/Always/Allow_All (default value) , then add to nvram */	
			for(rule_index=0; rule_index<ARRAYSIZE(vs_rule_table); rule_index++){
				if( strcmp(nvram_safe_get(vs_rule_table[rule_index].rule),"0//TCP/0/0/0.0.0.0/Always/Allow_All") == 0 ||
					strcmp(nvram_safe_get(vs_rule_table[rule_index].rule),"0//6/0/0/0.0.0.0/Always/Allow_All") == 0 	||
					strlen(nvram_safe_get(vs_rule_table[rule_index].rule)) == 0) {																
					nvram_set(vs_rule_table[rule_index].rule,vs_rule);
					break;
				}
			}				
	}
	
	return 1;       
}

static int do_delete_port_mapping_to_xml(char *source_xml)
{	
	char protocol[5] = {};
	char exter_port[7] = {};			
	int rule_index = 0;	
	
	/* value of nvram */
	char enable_n[2] = {};
	char name_n[32] = {};
	char protocol_n[5] = {};
	char ip_n[16] = {};
	char exter_port_n[7] = {};
	char inter_port_n[7] = {};
	char schedule_n[10] = {};		
	
	do_get_element_value(source_xml, protocol, "PortMappingProtocol");
	do_get_element_value(source_xml, exter_port, "ExternalPort");
	
	/* search vs_rule from nvram */
	for(rule_index=0; rule_index<ARRAYSIZE(vs_rule_table); rule_index++){	
		if(!transfer_vs_rule(rule_index,enable_n,name_n,protocol_n,exter_port_n,inter_port_n,ip_n,schedule_n))
			continue;		

		/* delete vs_rule from nvram if match xml element */		
		if(strcmp(protocol, protocol_n) == 0 && strcmp(exter_port,exter_port_n) == 0)
		{							
			//nvram_set(vs_rule_table[rule_index].rule, "0//TCP/0/0/0.0.0.0/Always/Allow_All");
			nvram_set(vs_rule_table[rule_index].rule, "0//6/0/0/0.0.0.0/Always/Allow_All");			
			break;
		}	
	}		
	return 1;
}

static int do_get_macfilters2_to_xml(char *dest_xml, char *general_xml, char *mac_info_xml)
{

	char *enable = NULL;
	char *is_allow_list=NULL;
	char *mac_tmp = NULL;	
	char *mac_tmp_index = NULL;	
	char name[32] = {};
	char mac[18] = {};	
	char *mac_filter_type = NULL;
	int mac_index = 0;
	char mac_info_tmp[4000] = {};
	char mac_info_tmp_t[100] = {};	
	char *mac_sub_index = NULL;	
	
	/* transfer nvram value of pure */	
	mac_filter_type = nvram_safe_get("mac_filter_type");	
	if(!strcmp(mac_filter_type,"disable")) {
		enable = "false";
	}else if(!strcmp(mac_filter_type,"list_allow")) {
		enable = "true";
	}else if(!strcmp(mac_filter_type,"list_deny")) {
		enable = "true";
	}

	is_allow_list =nvram_safe_get("IsAllowList");	
	nvram_unset("AllowList");
	/* transfer nvram value of mac to pure, is a loop for MACInfo_xml in pure_xml.c */
	for(mac_index=0; mac_index<ARRAYSIZE(mac_filter_table); mac_index++){

		mac_tmp = nvram_safe_get(mac_filter_table[mac_index].mac_address);  /* 0/name/00:01:02:03:04:00/Always */

		if(strlen(mac_tmp) == 0)
		    continue;

		mac_sub_index = strstr(mac_tmp, "/");
		mac_tmp_index = strstr(mac_sub_index+1, "/");
		if(strcmp(mac_tmp,"0/name/00:00:00:00:00:00/Always")){		       // if not default value 

			strncat(name, mac_sub_index+1, mac_tmp_index - mac_sub_index - 1);
	
			if(strcmp(name, "0") == 0)
				strcpy(name, "");
			
			strncat(mac,mac_tmp_index+1,17);	
			sprintf(mac_info_tmp_t,mac_info_xml,mac,name);		
			sprintf(mac_info_tmp,"%s%s",mac_info_tmp,mac_info_tmp_t);				
		}
		/* initial mac tmp */
		memset(mac_info_tmp_t,0,100);                              
		memset(mac,0,18);
		memset(name,0,32);		
	}

	sprintf(dest_xml,general_xml,"OK",enable,is_allow_list,mac_info_tmp);
	return 1;	
}

static int do_set_macfilters2_to_xml(char *source_xml)
{
	char enable[6] = {};
	char is_allow_list[6] = {};
	char tmp[62] = {};
	char name[32] = {};
	char mac[18] = {};		
	int mac_index = 0;
	
	memset(is_allow_list, 0, sizeof(is_allow_list));
	do_get_element_value(source_xml, enable, "Enabled");
	do_get_element_value(source_xml, is_allow_list, "IsAllowList");
	nvram_set("IsAllowList",is_allow_list);
	
	
	/* transfer type of pure value to nvram*/
	if(strcasecmp( enable, "false" ) == 0)								// [false , false/true ]
		nvram_set("mac_filter_type","disable");

	else if(strcasecmp( enable, "true" ) == 0) {
		if(strcasecmp( is_allow_list , "true") == 0)        // [true , true ]
			nvram_set("mac_filter_type","list_allow");
			//nvram_set("mac_filter_type","true");
		else if(strcasecmp( is_allow_list , "false") == 0)	// [ true , false ]
			nvram_set("mac_filter_type","list_deny");
			//nvram_set("mac_filter_type","false");
		else 
			return 0;
	} else		
			
			return 0;	
	
	/* initial mac filter */			
	for(mac_index=0; mac_index<ARRAYSIZE(mac_filter_table); mac_index++) {
		nvram_set(mac_filter_table[mac_index].mac_address,"0/name/00:00:00:00:00:00/Always");
		usleep(10000);
	}
			
	/* set mac value of pure to nvram */		
	for(mac_index=0; mac_index<ARRAYSIZE(mac_filter_table); mac_index++){
		if ((source_xml = find_next_element(source_xml, "MACInfo")) != NULL) {												
			do_get_element_value(source_xml, mac, "MacAddress");
			do_get_element_value(source_xml, name, "DeviceName");	
					
			/* check if invalid mac */
			if(!check_mac_filter(mac)) 		
				return 0;			
			
			if(strcmp(name, "") == 0 || name == NULL) 
				strcpy(name, "0");
			
			sprintf(tmp,"1/%s%s%s%s%s",name,"/",mac,"/","Always");
			// make format , like 1/name/00:01:02:03:04:00/Always					
			nvram_set(mac_filter_table[mac_index].mac_address,tmp);
            		usleep(10000);
			
			/* initial mac tmp */
			memset(tmp, 0, 62);
			memset(name, 0, 32);
			memset(mac, 0, 18);
		} else
			break;					
	}		

	return 1;
}

int do_get_port_mappings(const char *key, char *raw)
{
	int ret;
	char xml_resraw[8192];
	char xml_resraw2[6144];

        bzero(xml_resraw, sizeof(xml_resraw));
        do_xml_create_mime(xml_resraw);
        
	ret=do_get_port_mappings_to_xml(xml_resraw2, get_port_mappings_result, port_mapping_result);
	strcat(xml_resraw,xml_resraw2);
		
        return do_xml_response(xml_resraw);

}

int do_add_port_mappings(const char *key, char *raw)
{
        char xml_resraw[8192];
        char xml_resraw2[6144];

        bzero(xml_resraw, sizeof(xml_resraw));
        do_xml_create_mime(xml_resraw);

	sprintf(xml_resraw2,add_port_mapping_result,
		do_add_port_mappings_to_xml(raw)? "OK":"ERROR");
        strcat(xml_resraw,xml_resraw2);

        return do_xml_response(xml_resraw);
}

int do_delete_port_mapping(const char *key, char *raw)
{
	char xml_resraw[8192];
	char xml_resraw2[6144];

	bzero(xml_resraw, sizeof(xml_resraw));
	do_xml_create_mime(xml_resraw);

	sprintf(xml_resraw2,delete_port_mapping_result,
                do_delete_port_mapping_to_xml(raw)? "OK":"ERROR");
        strcat(xml_resraw,xml_resraw2);

        return do_xml_response(xml_resraw);
	
}


int do_get_macfilters2(const char *key, char *raw)
{
	int ret;
	char xml_resraw[6144];
	char xml_resraw2[2048];

	bzero(xml_resraw, sizeof(xml_resraw));
	do_xml_create_mime(xml_resraw);


	ret=do_get_macfilters2_to_xml(xml_resraw2, get_macfilters2_result, mac_info);
	strcat(xml_resraw, xml_resraw2);

        return do_xml_response(xml_resraw);
}

int do_set_macfilters2(const char *key, char *raw)
{
        char xml_resraw[2048];
        char xml_resraw2[1024];

        bzero(xml_resraw, sizeof(xml_resraw));
        do_xml_create_mime(xml_resraw);

	sprintf(xml_resraw2, set_macfilters2_result,
			do_set_macfilters2_to_xml(raw) ? "REBOOT":"ERROR");


        strcat(xml_resraw, xml_resraw2);

        return do_xml_response(xml_resraw);

}
