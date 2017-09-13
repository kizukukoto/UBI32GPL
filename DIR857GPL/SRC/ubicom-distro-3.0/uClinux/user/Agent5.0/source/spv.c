/*=======================================================================
  
       Copyright(c) 2009, Works Systems, Inc. All rights reserved.
  
       This software is supplied under the terms of a license agreement 
       with Works Systems, Inc, and may not be copied nor disclosed except 
       in accordance with the terms of that agreement.
  
  =======================================================================*/
/*!
 * \file spv.c
 *
 */
#include <stdlib.h>
#include <string.h>

#include "tr_lib.h"
#include "session.h"
#include "xml.h"
#include "log.h"
#include "b64.h"
#include "atomic.h"
#include "spv.h"
#include "pkey.h"
#include "method.h"
#include "atomic.h"
#include "cli.h"
#include "tr_strings.h"
#include "tr_strptime.h"
#include "encrypt.h"
#include "war_string.h"
#include "grm.h"

struct spv_node {
	int fault_code;
	node_t node;
	char *path;
	struct spv_node *next;
};

struct spv {
	int fault_code;
        int crypt; /*!<0:indicate no encrypt; 1:indicate have encrypt */
        char cookie[128];

	struct spv_node *nodes;
	struct spv_node *next;
};

static void spv_node_destroy(struct spv_node *nodes)
{
	struct spv_node *header, *cur;

	for(header = nodes; (cur = header); ) {
		header = cur->next;

		if(cur->path)
			free(cur->path);

		free(cur);
	}
}

static int already_in(node_t node, struct spv_node *nodes)
{
	if(nodes) { 
		nodes = nodes->next; //Skip the first node -- the node itself;
		for(;nodes;) {
			if(memcmp(&node, &(nodes->node), sizeof(node)) == 0)
				return 1;

			nodes = nodes->next;
		}
	}

	return 0;
}

#ifndef INT32_MAX
#define INT32_MAX	(2147483647)
#endif

#ifndef INT32_MIN
#define INT32_MIN	(-2147483647-1)
#endif

#ifndef UINT32_MAX
#define UINT32_MAX	(4294967295U)
#endif

#ifdef TR196
int check_parameter_value(const char *value, const char *type, char *limitation)
#else
static int check_parameter_value(const char *value, const char *type, char *limitation)
#endif
{
	char *left, *right;
	//For simplifying processing, no space allowed in limitation
	if(limitation && *limitation) {
		left = limitation;
		right = strchr(limitation, ':');
		if(right) {
			*right = '\0';
			right++;
		}
		if(left && *left == '\0')
			left = NULL;
		if(right && *right == '\0')
			right = NULL;
	} else {
		left = right = NULL;
	}

	if(war_strcasecmp(type, "string") == 0) { //For string value, the limitation can be NULL, EMPTY or "MIN_LEN:MAX_LEN"
		if(left || right) {
			int min, max;
			int value_len;

			min = left ? atoi(left) : 0;
			max = right ? atoi(right) : INT32_MAX;

			value_len = strlen(value);
			if(value_len < min || value_len > max) {
				tr_log(LOG_WARNING, "String value length does not match limitation: %d [%d:%d]", value_len, min, max);
				return 9007;
			}
		}
	} else if(war_strcasecmp(type, "int") == 0) { //For integer value, the limitation can be NULL, EMPTY or "MIN:MAX"
		if(*value && string_is_digits(value)) {
			if(left || right) {
				int min, max;
				int ivalue;
				min = left ? atoi(left) : INT32_MIN;
				max = right ? atoi(right) : INT32_MAX;

				ivalue = atoi(value);
				if(ivalue < min || ivalue > max) {
					tr_log(LOG_WARNING, "Integer value does not match limitation: %d [%d:%d]", ivalue, min, max);
					return 9007;
				}
			}
		} else {
			tr_log(LOG_WARNING, "Integer value is empty or does not only includ digits: %s", value);
			return 9007;
		}
	} else if(war_strcasecmp(type, "unsignedInt") == 0) {
		if(*value && string_is_digits(value) && *value != '-') {
			if(left || right) {
				unsigned long int min, max;
				unsigned long int ivalue;
				min = left ? atol(left) : 0;
				max = right ? atol(right) : UINT32_MAX;

				ivalue = atoi(value);
				if(ivalue < min || ivalue > max) {
					tr_log(LOG_WARNING, "Unsigned integer value does not match limitation: %d [%d:%d]", ivalue, min, max);
					return 9003;
				}
			}
		} else {
			tr_log(LOG_WARNING, "Unsigned integer value is empty, start with a minus or does not only includ digits: %s", value);
			return 9007;
		}
	} else if(war_strcasecmp(type, "boolean") == 0) {
		if(war_strcasecmp(value, "false") &&
				war_strcasecmp(value, "0") &&
				war_strcasecmp(value, "true") &&
				war_strcasecmp(value, "1")) {
			tr_log(LOG_WARNING, "Boolean value is incorrect: %s", value);
			return 9007;
		}
	} else if(war_strcasecmp(type, "dateTime") == 0) {
		int len;

		len = strlen(value);
		if(len == (strlen(UNKNOWN_TIME) - 1) || len == (strlen(UNKNOWN_TIME) + 5)) {
			if(string_time2second(value) == (time_t)-1) {
				tr_log(LOG_WARNING, "Date Time value is incorrect: %s", value);
				//return 9007;
			}
		} else if(len != strlen(UNKNOWN_TIME) || strcmp(value, UNKNOWN_TIME)) {
			tr_log(LOG_WARNING, "Date Time value is incorrect: %s", value);
			return 9007;
		}
	} else if(war_strcasecmp(type, "base64") == 0) {
		if(!string_is_b64(value)) {
			tr_log(LOG_WARNING, "Base64 value is incorrect: %s", value);
			return 9007;
		}
	} else {
		tr_log(LOG_WARNING, "Unknown value type: %s", type);
		return 9006;
	}	 

	return 0;	
}


static int spv(char **msg, struct spv *s)
{
	struct xml tag;
	int res;
	char *path = NULL;
	char *value = NULL;
	char *type = NULL;
	struct spv_node *node;

	while(xml_next_tag(msg, &tag) == XML_OK) {
		if(war_strcasecmp(tag.name, "Name") == 0) {
			path = tag.value;
		} else if(war_strcasecmp(tag.name, "Value") == 0) {
			unsigned int i;
			for(i = 0, type = NULL; i < tag.attr_count; i++) {
				if(war_strcasecmp(tag.attributes[i].attr_name, "type") == 0) {
					char *colon;
					type = tag.attributes[i].attr_value;
					colon = strchr(type, ':');
					if(colon)
						type = colon + 1;
					break;
				}
			}
			value = tag.value ? tag.value : "";
		} else if(war_strcasecmp(tag.name, "/ParameterValueStruct") == 0) {
			break;
		}
	}

	res = 0;
	node = calloc(1, sizeof(*node));
	if(node == NULL) {
		tr_log(LOG_ERROR, "Out of memory!");
		goto internal_error;
	} else {
		node->next = s->nodes;
		s->nodes = node;
		node->path = war_strdup(path ? path : "");
		if(node->path == NULL) {
			tr_log(LOG_ERROR, "Out of memory!");
			goto internal_error;
		}
	}


	if(path == NULL) {
		s->fault_code = 9003;
		node->fault_code = 9005;
		goto complete;
	} else if(type == NULL) {
		s->fault_code = 9003;
		node->fault_code = 9006;
		goto complete;
	} else if(value == NULL) {
		s->fault_code = 9003;
		node->fault_code = 9003;
		goto complete;
	}

	res = lib_resolve_node(path, &(node->node));
	if(res < 0) {
		goto internal_error;
	} else if(res > 0) {
		s->fault_code = 9003;
		node->fault_code = 9005;
		res = 0;
		goto complete;
	} else if(already_in(node->node, s->nodes)) {
		s->fault_code = 9003;
		node->fault_code = 9003;
		goto complete;
	} else {
		{
			char rw[PROPERTY_LENGTH];
			if(lib_get_property(node->node, "rw", rw) != 0)
				goto internal_error;

			if(string2boolean(rw) != BOOLEAN_TRUE) {
				s->fault_code = 9003;
				node->fault_code = 9008;
				goto complete;
			}
		}
		{
			char ptype[PROPERTY_LENGTH];
			char *limitation;
			char *square_bracket;

			if(lib_get_property(node->node, "type", ptype) != 0)
				goto internal_error;

			if(war_strcasecmp(ptype, "node") == 0) {
				s->fault_code = 9003;
				node->fault_code = 9005;
				goto complete;
			}

			square_bracket = strchr(ptype, '[');
			if(square_bracket) {
				*square_bracket = '\0';
				limitation = square_bracket + 1;
				square_bracket = strchr(limitation, ']');
				if(square_bracket)
					*square_bracket = '\0';
			} else {
				limitation = NULL;
			}

			if(war_strcasecmp(type, ptype) != 0) {
				s->fault_code = 9003;
				node->fault_code = 9006;
				goto complete;
			}

			if((node->fault_code = check_parameter_value(value ? value : "", ptype, limitation)) != 0) {
				s->fault_code = 9003;
				node->fault_code = 9007;
				goto complete;
			}
		}


		//All check passed
		if(s->fault_code == 0) {
                    char *v = NULL;
                    /*add for encrypt */
                    char *val = NULL ;

                    if (s->crypt) {//here we need decrypt value
                        unsigned char cek[16];

                        memset(cek, 0, 16);
                        if (calculate_cek(s->cookie, cek) != 0) {//Content Encryption Keys
                            tr_log(LOG_ERROR, "calculate Content Encryption Key failed!");
                            goto internal_error;
                        }
                        if (value) {
                            if (content_decrypt(value, cek, 16, (unsigned char **)&val) !=0 ) {
                                goto internal_error;
                            }
                        }
                    } else {
                        val = value;
                    }
			if(lib_get_value(node->node, &v) != 0) {
                            if (s->crypt) {
				if(val) 
                                    free(val);
                            }
				if(v)
					lib_destroy_value(v);
				goto internal_error;
			} else if(strcmp(v ? v : "", val) == 0) { //No change from old version
				value_change(path, val);
                                if (s->crypt) {
                                    if(val) 
                                        free(val);
                                }
				if(v)
					lib_destroy_value(v);
				goto complete;
			} else if(spv_journal(path, v) != 0 || lib_set_value(node->node, val) != 0) {
                                if (s->crypt) {
                                    if(val) 
                                        free(val);
                                }
				if(v)
					lib_destroy_value(v);
				goto internal_error;
			} else {
				value_change(path, val);
                                if (s->crypt) {
                                    if(val) 
                                        free(val);
                                }
				if(v)
					lib_destroy_value(v);
				goto complete;
			}
		} else {
			goto complete;
		}
	}

internal_error:
	s->fault_code = 9002;
	spv_node_destroy(s->nodes);
	s->nodes = NULL;
	res = -1;

complete:
	return res;
}


#if 0
int get_num_from_attrs(struct xml *tag)
{
    int i;
    int num = 0;
    for(i=0; i<tag->attr_count; i++) {
        if (war_strcasecmp(tag->attributes[i].attr_name, "arrayType")==0) {
            char *t_flag = strchr(tag->attributes[i].attr_value, '[');
	    if (t_flag != NULL) {
		t_flag++;
		num = atoi(t_flag);
	    }
	    break;
	}
    }
    return num;
}
#endif
int spv_process(struct session *ss, char **msg)
{
	struct xml tag;
	char pkey[33] = "";
	struct spv *s;
	int len = -1;
	int exp_num = 0, act_num = 0;

	s = calloc(1, sizeof(*s));
	if(s == NULL) {
		tr_log(LOG_ERROR, "Out of memory!");
		return METHOD_FAILED;
	} else {
		ss->cpe_pdata = s;
	}
        /*whether need decrypt or not*/
	if(strcmp(ss->cpe->name, "X_WKS_SetParameterValuesCrypt") == 0) {
            int len;
            char *tmp = NULL, *id = NULL, *ter = NULL;
            
            len = strlen(ss->http.cookie.cookie_header.data);
            tmp = calloc(1, len + 1);
            war_snprintf(tmp, len+1, "%s", ss->http.cookie.cookie_header.data);
            id = strstr(tmp, "TR069SessionID=");
            id += 16; 
            ter = strchr(id, '"');
            *ter = '\0';
            war_snprintf(s->cookie, sizeof(s->cookie), "%s", id);
            *ter = '"';
            free(tmp);

            s->crypt = 1;
        } else {
            s->crypt = 0;
        }

	if(start_transaction() != 0) {
		tr_log(LOG_ERROR, "Start transaction failed!");
		s->fault_code = 9002;
		return METHOD_FAILED;
	}

	while(xml_next_tag(msg, &tag) == XML_OK) {
		if(war_strcasecmp(tag.name, "ParameterList") == 0) {
		    exp_num = get_num_from_attrs(&tag);
		} else if(war_strcasecmp(tag.name, "ParameterValueStruct") == 0) {
		        act_num++;
			if(spv(msg, s) != 0) {
				break;
			}
		} else if(war_strcasecmp(tag.name, "ParameterKey") == 0) {
			if(tag.value)
				len = war_snprintf(pkey, sizeof(pkey), "%s", tag.value ? tag.value : "");
		} else if(war_strcasecmp(tag.name, "/SetParameterValues") == 0) {
			break;
		}
	}
	if(s->fault_code == 0) {
	        if (exp_num != act_num)
			s->fault_code = 9003;
		if(len >= 0) {
			if(set_parameter_key(pkey) != 0) {
				s->fault_code = 9002;
				spv_node_destroy(s->nodes);
				s->nodes = NULL;
			}
		} else {
			s->fault_code = 9003;
		}
	}

	s->next = s->nodes;

	if(s->fault_code != 0) {
		rollback_transaction();
		return METHOD_FAILED;
	} else {
		commit_transaction();
		spv_node_destroy(s->nodes);
		s->nodes = NULL;
		s->next = NULL;
		return METHOD_SUCCESSED;
	}

}


int spv_body(struct session *ss)
{
	struct spv *s;
	struct buffer buf;
	int res = METHOD_COMPLETE;

	init_buffer(&buf);
	s = (struct spv *)(ss->cpe_pdata);
	if(s == NULL || s->fault_code == 9002) {
		push_buffer(&buf,
				"<FaultCode>9002</FaultCode>\n"
				"<FaultString>%s</FaultString>\n", fault_code2string(9002));
	} else if(s->fault_code != 0) {
		if(s->next == s->nodes)
			push_buffer(&buf,
					"<FaultCode>%d</FaultCode>\n"
					"<FaultString>%s</FaultString>\n", s->fault_code, fault_code2string(s->fault_code));

		while(s->next && s->next->fault_code == 0)
			s->next = s->next->next;

		if(s->next) {
			push_buffer(&buf,
					"<SetParameterValuesFault>\n"
					"<ParameterName>%s</ParameterName>\n"
					"<FaultCode>%d</FaultCode>\n"
					"<FaultString>%s</FaultString>\n"
					"</SetParameterValuesFault>\n",
					s->next->path, s->next->fault_code, fault_code2string(s->next->fault_code));

			s->next = s->next->next;
		}

		if(s->next)
			res = METHOD_MORE_DATA;
	} else {
		push_buffer(&buf, "<Status>1</Status>\n");
	}

	if(buf.data_len)
		push_buffer(&(ss->outbuf), "%x\r\n%s\r\n", buf.data_len, buf.data);

// 2011.04.14
	if(buf.data_len)
		push_buffer(&(ss->outbuf2_data), "%s", buf.data);


	destroy_buffer(&buf);

	return res;
}


void spv_rewind(struct session *ss)
{
	struct spv *s;
	s = (struct spv *)(ss->cpe_pdata);

	s->next = s->nodes;
}


void spv_destroy(struct session *ss)
{
	struct spv *s;
	s = (struct spv *)(ss->cpe_pdata);

	spv_node_destroy(s->nodes);
	free(s);

	ss->cpe_pdata = NULL;
}
