/*=======================================================================
  
       Copyright(c) 2009, Works Systems, Inc. All rights reserved.
  
       This software is supplied under the terms of a license agreement 
       with Works Systems, Inc, and may not be copied nor disclosed except 
       in accordance with the terms of that agreement.
  
  =======================================================================*/
/*!
 * \file inform.c
 *
 */
#include <string.h>
#include <stdlib.h>

#include "event.h"
#include "log.h"
#include "retry.h"
#include "xml.h"
#include "tr.h"
#include "tr_lib.h"
#include "inform.h"
#include "session.h"
#include "request.h"
#include "war_string.h"

enum {
    INFORM_MUST,
    INFORM_EVENT,
    INFORM_PARAMETER_HEADER,
    INFORM_PARAMETER,
    INFORM_PARAMETER_TAIL
};

char inform_time[32]={0};

// 2011.03.14
int inform_first=1;
int inform_first_count=3;
char *inform_first_path[] = { 
		"InternetGatewayDevice.ManagementServer.ConnectionRequestURL",
		"InternetGatewayDevice.ManagementServer.ConnectionRequestUsername",
		"InternetGatewayDevice.ManagementServer.ConnectionRequestPassword"};


#define dprintf printf

static int next_step;
static struct inform_parameter *next_ip; //next inform parameter

//All of the following parameters MUST not be changed for erver
static char man[128] = "";
static char oui[128] = "";
static char pclass[128] = "";
static char serialnu[128] = "";

static int active_no_param = 0;//has Active notification parameter in inform parameter list.

struct inform_parameter {
    int commited;
    int type;
    char *path;
    struct inform_parameter *next;
}; 

static struct {
    int count;
    struct inform_parameter *header;
    struct inform_parameter *tail;
} inform_parameters_header = {0, NULL, NULL};


int has_active_no_param(void)
{
	return active_no_param;
}

void reset_active_no_param(void)
{
	active_no_param = 0;
}

int add_inform_parameter(const char *path, int type)
{
	struct inform_parameter *cur;
	int event = 0;
	int already_in = 0;
	int noc;

	if(type == 1) {
		node_t node;
		if(lib_resolve_node(path, &node) == 0) {
			char prop[PROPERTY_LENGTH];
			if(lib_get_property(node, "noc", prop) == 0) {
				noc = atoi(prop);
				if(noc == 0)
					return 0; //The ACS turn off the notification for this parameter
				else if(noc == 1)
					event = 1;
				else if(noc == 2){
					event = 1;
					active_no_param = 1;
				}
			}
		} else {
			return -1;
		}
	}

	for(cur = inform_parameters_header.header; cur; cur = cur->next) {
		if(war_strcasecmp(cur->path, path) == 0) {//The parameter already in the list
			tr_log(LOG_NOTICE, "The parameter(%s) already in the list!", path);
			cur->commited = 0; //reset commit stauts
			already_in = 1;
			break;
		}
	}

	if(already_in == 0) {
		cur = calloc(1, sizeof(*cur));
		if(cur == NULL) {
			tr_log(LOG_ERROR, "Out of memory!");
			return -1;
		} else {
			cur->type = type;
			cur->path = war_strdup(path);

			if(inform_parameters_header.tail == NULL) {
				inform_parameters_header.header = cur;
				inform_parameters_header.tail = cur;
				inform_parameters_header.count = 1;
			} else {
				inform_parameters_header.tail->next = cur;
				inform_parameters_header.tail = cur;
				inform_parameters_header.count++;
			}
		}
	}

	if(event)
		add_single_event(S_EVENT_VALUE_CHANGE);
	return 0;	
}

int add_static_inform_parameter(const char *name, const char *path)
{
	return add_inform_parameter(path, 0);
}

static struct inform_parameter *next_inform_parameter(struct inform_parameter **cur)
{
	if(*cur == NULL)
		*cur = inform_parameters_header.header;
	else
		*cur = (*cur)->next;

    return *cur;
}

static void del_inform_parameter(struct inform_parameter *ip)
{
    if(ip->type == 1 && ip->commited == 1) {
        struct inform_parameter *prev, *cur;
        for(prev = NULL, cur = inform_parameters_header.header; cur != ip && cur; prev = cur, cur = cur->next);
        if(cur) {
            if(prev)
                prev->next = ip->next;
            else
                inform_parameters_header.header = ip->next;

            if(ip->next == NULL)
                inform_parameters_header.tail = prev;

            free(cur->path);
            free(cur);
            inform_parameters_header.count--;
        }
    }
}

int inform_process(struct session *ss)
{
    static int first = 1;
    int res = 0;
    
    next_step = INFORM_MUST;
    next_ip = NULL;
    
	//2011/01/05 Brian+: fix VT bug that passive notification is not working on the wan up time 
   
    add_inform_parameter("InternetGatewayDevice.DeviceInfo.UpTime", 1);
    add_inform_parameter("InternetGatewayDevice.WANDevice.1.WANConnectionDevice.1.WANIPConnection.1.Uptime", 1);                        

    while(next_inform_parameter(&next_ip)) {
	    if(next_ip->type == 1) {
		    add_single_event(S_EVENT_VALUE_CHANGE);
		    break;
	    }
    }
    next_ip = NULL;

    if(first == 1) {
        GET_NODE_VALUE(DEVICE_ID_MANUFACTURER, man);
        GET_NODE_VALUE(DEVICE_ID_OUI, oui);
        GET_NODE_VALUE(DEVICE_ID_PRODUCT_CLASS, pclass);
        GET_NODE_VALUE(DEVICE_ID_SERIAL_NUMBER, serialnu);

        if(res == 0)
            first = 0;
    }

    return res == 0 ? METHOD_SUCCESSED : METHOD_RETRY_LATER;
}


int inform_body(struct session *ss)
{
        struct buffer buf;
			int count,i;

	init_buffer(&buf);
	switch(next_step) {
		case INFORM_MUST:
                    push_buffer(&buf,
                            "<DeviceId>\n"
                                "<Manufacturer>%s</Manufacturer>\n"
                                "<OUI>%s</OUI>\n"
                                "<ProductClass>%s</ProductClass>\n"
                                "<SerialNumber>%s</SerialNumber>\n"
                            "</DeviceId>\n",
			    man, oui, pclass, serialnu);

			next_step++;
			break;

		case INFORM_EVENT:
                        {
                            struct event *e = NULL;
                            int i;
                            i = get_event_count();
	                       push_buffer(&buf,
                               "<Event xsi:type='soap-enc:Array' soap-enc:arrayType='cwmp:EventStruct[%d]'>\n", i);

                            while(next_event(&e)) {
                                e->commited = 1;
                                push_buffer(&buf,
                                      "<EventStruct>\n"
                                          "<EventCode>%s</EventCode>\n"
                                          "<CommandKey>%s</CommandKey>\n"
                                      "</EventStruct>\n", code2string(e->event_code), e->command_key);
                            }

							push_buffer(&buf, "</Event>\n");
                        }
			next_step++;
			break;
		case INFORM_PARAMETER_HEADER:

			
			count = inform_parameters_header.count;
			if( inform_first ) 
				count += inform_first_count;

			push_buffer(&buf,
				"<MaxEnvelopes>1</MaxEnvelopes>\n"
				"<CurrentTime>%s</CurrentTime>\n"
				"<RetryCount>%d</RetryCount>\n" 
                                "<ParameterList xsi:type='soap-enc:Array' soap-enc:arrayType='cwmp:ParameterValueStruct[%d]'>\n", 
				    lib_current_time(), get_retry_count(),count);

// 2011.03.14 Google Case
				char *path;
				node_t node;
				
				if(inform_first)
				for(i=0;i<inform_first_count;i++) {				
					path = inform_first_path[i];
					if(lib_resolve_node(path, &node) == 0) {
						char type[PROPERTY_LENGTH];
						char *v = NULL;
						if(lib_get_property(node, "type", type) == 0 && lib_get_value(node, &v) == 0) {
							
							push_buffer(&buf,
									"<ParameterValueStruct>\n"
									"<Name>%s</Name>\n"
									"<Value xsi:type='xsd:%s'>%s</Value>\n"
									"</ParameterValueStruct>\n", path, type, v);
						}
					}
				}
								
			next_step++;
			break;
		case INFORM_PARAMETER:
			{
				struct event *e;
				node_t node;


				e = get_event(S_EVENT_VALUE_CHANGE, NULL);
				while(next_inform_parameter(&next_ip)) {
					//if(next_ip->type == 1 && (e == NULL || e->commited == 0))
					if(next_ip->type == 1 && e == NULL)
						continue;
					if(e && e->commited)
						next_ip->commited = 1;
					if(lib_resolve_node(next_ip->path, &node) == 0) {
						char type[PROPERTY_LENGTH];
						char *v = NULL;
						if(lib_get_property(node, "type", type) == 0 && lib_get_value(node, &v) == 0) {
							char *square_bracket;
							square_bracket = strchr(type, '[');
							if(square_bracket)
								*square_bracket = '\0';

							push_buffer(&buf,
									"<ParameterValueStruct>\n"
									"<Name>%s</Name>\n"
									"<Value xsi:type='xsd:%s'>%s</Value>\n"
									"</ParameterValueStruct>\n", next_ip->path, type, v);
						} else {
							tr_log(LOG_ERROR, "get parameter %s value error", next_ip->path);
							return METHOD_FAILED;
						}
						if(v)
							lib_destroy_value(v);
					} else {
						tr_log(LOG_ERROR, "parameter %s does not exist", next_ip->path);
						return METHOD_FAILED;
					}

					break;
				}
			}

			if(next_ip == NULL)
				next_step++;
			break;
		case INFORM_PARAMETER_TAIL:
			push_buffer(&buf, "</ParameterList>\n");
			next_step++;
			break;
		default:
			return METHOD_COMPLETE;
	}

	if(buf.data_len > 0)
            push_buffer(&(ss->outbuf), "%x\r\n%s\r\n", buf.data_len, buf.data);

// 2011.04.14
	if(buf.data_len > 0)
            push_buffer(&(ss->outbuf2_data), "%s", buf.data);

        destroy_buffer(&buf);

	return next_step > INFORM_PARAMETER_TAIL ? METHOD_COMPLETE : METHOD_MORE_DATA;
}


int inform_fault_handler(struct session *ss, char **msg)
{
	struct xml tag;

        inform_rewind(ss);

	while(xml_next_tag(msg, &tag) == XML_OK) {
		if(war_strcasecmp(tag.name, "FaultCode") == 0) {
			if(tag.value && war_strcasecmp(tag.value, "8005") == 0) {
                            tr_log(LOG_NOTICE, "Re-transmission inform message!");
				return METHOD_RETRANSMISSION;
                        } else {
                            tr_log(LOG_WARNING, "Session ended because inform fault code: %s", tag.value);
				return METHOD_RETRY_LATER;
                        }
		}
	}

	return METHOD_RETRY_LATER;
}

int inform_success_handler(struct session *ss, char **msg)
{
    struct inform_parameter *i = NULL;
    struct event *e = NULL;
    int found = 0;

    //Delete all commited inform parameters whose type is non-zero
    while(next_inform_parameter(&i)) {
        if(i->type == 1) {
            if(i->commited == 0) {
                found = 1;
            } else {
                del_inform_parameter(i);
                i = NULL;
            }
        }
    }


	//Delete all commited event, if there are any inform parameter whose type is non-zero, do not delete the "4 VALUE CHANGE" single event, do not delete the "M download" and "M upload" multiple event
    while(next_event(&e)) {
        if(e->commited) {
            if((e->event_code == S_EVENT_VALUE_CHANGE && found) || ((e->event_code == M_EVENT_DOWNLOAD || e->event_code == M_EVENT_UPLOAD) && get_request("TransferComplete", e->event_code, e->command_key)))
                continue;

            del_event(e);
            e = NULL;
        }
    }

    complete_delete_event();

	//dprintf("inform_success_handler\n");
	inform_first = 0;

    return METHOD_SUCCESSED;
}

void inform_rewind(struct session *ss)
{
    struct event *e = NULL;
    struct inform_parameter *i = NULL;

    while(next_event(&e))
        e->commited = 0;

    while(next_inform_parameter(&i))
        i->commited = 0;

    next_step = INFORM_MUST;
    next_ip = NULL;
}
