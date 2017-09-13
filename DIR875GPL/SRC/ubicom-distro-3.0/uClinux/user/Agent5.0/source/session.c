/*=======================================================================
  
       Copyright(c) 2009, Works Systems, Inc. All rights reserved.
  
       This software is supplied under the terms of a license agreement 
       with Works Systems, Inc, and may not be copied nor disclosed except 
       in accordance with the terms of that agreement.
  
  =======================================================================*/
/*!
 * \file session.c
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#include "tr.h"
#include "session.h"
#include "tr_lib.h"
#include "log.h"
#include "sched.h"
#include "event.h"
#include "cli.h"
#include "retry.h"
#include "xml.h"
#include "task.h"
#include "request.h"
#include "network.h"
#include "war_string.h"
#include "inform.h"


// 2011.04.14
int g_mode_contlen = 0;

static char url[256] = "";
static char username[256] = "";
static char password[256] = "";

static int ip_ping = 0;
static int trace_route = 0;
static int dsl_diagnostics = 0;
static int atm_diagnostics = 0;
#ifdef TR143
static int download_diagnostics = 0;
static int upload_diagnostics = 0;
#endif

static int session_timeout = 30;

static struct sched *sched = NULL;
static int session_lock = 0;

int get_session_lock(void)
{
	return session_lock;
}

void set_session_lock(int lock)
{
	session_lock = lock;
}

int set_session_config(const char *name, const char *value)
{
	if(war_strcasecmp(name, "SessionTimeout") == 0) {
		int res;

		res = atoi(value);
		if(res >= 5)
			session_timeout = res;
		else
			tr_log(LOG_WARNING, "Session timeout is too short(%d), ignore it...", res);
	}

	return 0;
}

int need_reboot_device()
{
	struct session *ss;
	if(sched && sched->pdata) {
		ss = (struct session *)(sched->pdata);
		ss->reboot = 1; //Reboot device when session terminated
	}

	if(sched)
		return 0; //Agent is in session, it will reboot the device as soon as the session terminated, 
			  //the caller can not do this
	else
		return 1; //Agent is not in session, the caller MUST reboot the device by itself
}

//TR-098 Page 62
static void ip_ping_changed(const char *new)
{
    if(war_strcasecmp(new, "Requested") == 0) {
        lib_stop_ip_ping();
        ip_ping = 1;
    }
}

//TR-098 Page 62
static void trace_route_changed(const char *new)
{
    if(war_strcasecmp(new, "Requested") == 0) {
        lib_stop_trace_route();
        trace_route = 1;
    }
}
static char dsl_diag[256];
static char atm_diag[256];

static void dsl_diagnostics_changed(const char *new)
{
    char *val = (char *)new;
    if ((val = strchr(new, '|')) != NULL) {
	*val = '\0';
	val++;
	war_snprintf(dsl_diag, sizeof(dsl_diag), "%s", new);
    }
    if(war_strcasecmp(val, "Requested") == 0) {
        lib_stop_dsl_diagnostics(dsl_diag);
        dsl_diagnostics = 1;
    }
}
static void atm_diagnostics_changed(const char *new)
{
    char *val = (char *)new;
    if ((val = strchr(new, '|')) != NULL) {
	*val = '\0';
	val++;
	war_snprintf(atm_diag, sizeof(atm_diag), "%s", new);
    }
    if(war_strcasecmp(val, "Requested") == 0) {
        lib_stop_atm_diagnostics(atm_diag);
        atm_diagnostics = 1;
    }
}
#ifdef TR143
static void download_diagnostics_changed(const char *new)
{
	
    if(war_strcasecmp(new, "Requested") == 0) {
        stop_dd();
        download_diagnostics = 1;
    }
}

static void upload_diagnostics_changed(const char *new)
{
    if(war_strcasecmp(new, "Requested") == 0) {
        stop_ud();
        upload_diagnostics = 1;
    }
}
#endif

static void url_changed(const char *new)
{
    if(war_strcasecmp(url, new) != 0) {
	war_snprintf(url, sizeof(url), "%s", new);
	add_single_event(S_EVENT_BOOTSTRAP);
        add_request("GetPRCMethods", -1, NULL, "");
    }
}

static void username_changed(const char *new)
{
	war_snprintf(username, sizeof(username), "%s", new);
}

static void password_changed(const char *new)
{
	war_snprintf(password, sizeof(password), "%s", new);
}

static int session_process_soap(struct session *ss)
{
	char *msg;
	struct xml tag;
	int detail = 0;

	ss->received_empty = 0;
	msg = ((struct buffer *)(ss->http.body))->data;

	while(xml_next_tag(&msg, &tag) == XML_OK) {
		if(tag.name && tag.name[0] == '/') {
		} else if(war_strcasecmp(tag.name, "Envelope") == 0) {
			unsigned int i;
			for(i = 0; i < tag.attr_count; i++) {
				if(war_strcasecmp(tag.attributes[i].attr_name, "cwmp") == 0) {
					if(war_strcasecmp(tag.attributes[i].attr_value, "urn:dslforum-org:cwmp-1-0") == 0) {
						ss->version = 0;
					} else if(war_strcasecmp(tag.attributes[i].attr_value, "urn:dslforum-org:cwmp-1-1") == 0) {
						ss->version = 1;
					} else {
						tr_log(LOG_ERROR, "Incorrect CWMP version: %s", tag.attributes[i].attr_value);
						return METHOD_FAILED;
					}

					break;
				}
			}
		} else if(war_strcasecmp(tag.name, "Header") == 0) {
		} else if(war_strcasecmp(tag.name, "ID") == 0) {
			if(tag.value)
				war_snprintf(ss->id, sizeof(ss->id), "%s", tag.value);
		} else if(war_strcasecmp(tag.name, "HoldRequests") == 0) {
			if(tag.value)
				ss->hold = atoi(tag.value);
			else
				ss->hold = 0;
		} else if(war_strcasecmp(tag.name, "Body") == 0) {
		} else if(ss->acs) {
			char acs_rpc_name[64];

			war_snprintf(acs_rpc_name, sizeof(acs_rpc_name), "%sResponse", ss->acs->name);
			if(war_strcasecmp(tag.name, "Detail") == 0) {
				detail = 1;
			} else if(war_strcasecmp(tag.name, "Fault") == 0) {
				if(detail) {
					if(ss->acs->fault_handler)
						return ss->acs->fault_handler(ss, &msg);
					else
						return METHOD_FAILED;
				}
			} else if(war_strcasecmp(acs_rpc_name, tag.name) == 0){ //Response
				if(ss->acs->success_handler)
					return ss->acs->success_handler(ss, &msg);
				else
					return METHOD_SUCCESSED;
			} else {
				tr_log(LOG_NOTICE, "Unknown tag: %s", tag.name);
			}
		} else {
			ss->cpe = get_cpe_method_by_name(tag.name);
			if(ss->cpe->process) {
				ss->cpe_result = ss->cpe->process(ss, &msg);
				return ss->cpe_result;
			} else {
				return METHOD_SUCCESSED;
			}
		}
	}

	if(ss->acs && war_strcasecmp(ss->acs->name, "Inform") == 0)
		return METHOD_RETRY_LATER;
	else if(ss->acs) {
		tr_log(LOG_WARNING, "No ACS RPC Method response found");
		return METHOD_FAILED;
	} else
		return METHOD_SUCCESSED;
}


static void session_readable(struct sched *sc)
{
	struct session *ss;
	int res;

	ss = (struct session *)sc->pdata;

	res = http_recv(&(ss->http), &(ss->conn));
	if(res == HTTP_COMPLETE) {
            if(ss->http.msg_type == HTTP_REQUEST) {
                    if(ss->acs && war_strcasecmp(ss->acs->name, "Inform") == 0) {
                        retry_later();
                    }

                    sc->need_destroy = 1;
            } else if(war_strcasecmp(ss->http.start_line.response.code, "204") == 0 ||
                    (war_strcasecmp(ss->http.start_line.response.code, "200") == 0 && 
		     ((struct buffer *)(ss->http.body))->data_len == 0)) {
                    tr_log(LOG_DEBUG, "Receive an empty package");
                    if(ss->acs && war_strcasecmp(ss->acs->name, "Inform") == 0) {
			tr_log(LOG_WARNING, "ACS return an empty message for Inform RPC Methods, retry session later");
												
// 2011.04.14			
						            ss->mode_contlen = 1; // session will delete, no use
						            g_mode_contlen = 1;
						            // try use
						            printf("try use Context Length\n");

                        retry_later();
                        sc->need_destroy = 1;
                    } else {
                        ss->received_empty = 1;
                        reset_retry_count();
                        if(ss->hold == 0 && ss->sent_empty) {
                            sc->need_destroy = 1;
                            return;
                        }
                        
                        if(ss->acs) {
                            //Nerver should receive a empty package for a ACS method
			    tr_log(LOG_ERROR, "ACS return an empty message for %s RPC Methods, abort session", ss->acs->name);
                            sc->need_destroy = 1;
                            return;
                        }

                        if(ss->cpe) {
                            if(ss->cpe->destroy) {
                                ss->cpe->destroy(ss);
                            } else if(ss->cpe_pdata) {
                                free(ss->cpe_pdata);
                                ss->cpe_pdata = NULL;
                            }

                            ss->cpe = NULL;
                        }

                        ss->hold = 0;
                        ss->acs = next_acs_method();
                        if(ss->acs) {
                            if(ss->acs->process)
                                ss->acs->process(ss);
			    //ss->sent_empty = 0;
                        }

                        sc->type = SCHED_WAITING_WRITABLE;
                        sc->timeout = current_time() + session_timeout;
                        ss->next_step = NEXT_STEP_HTTP_HEADER;
                    	del_http_headers(&(ss->http));
                    }
            } else if(war_strcasecmp(ss->http.start_line.response.code, "200") == 0) {
                char *ct;
                ct = http_get_header(&(ss->http), "Content-Type: ");
                if(ct == NULL || war_strncasecmp(ct, "text/xml", 8)) {
                    tr_log(LOG_DEBUG, "Content type is: %s", ct ? ct : "NULL");
                    if(ss->acs && war_strcasecmp(ss->acs->name, "Inform") == 0) {
                        retry_later();
                    }

                    sc->need_destroy = 1;
                } else {
                    tr_log(LOG_DEBUG, "SOAP message:\n%s", ((struct buffer *)(ss->http.body))->data);
                    if(ss->cpe) {
                        if(ss->cpe->destroy)
                            ss->cpe->destroy(ss);
                        else if(ss->cpe_pdata) {
                            free(ss->cpe_pdata);
                        }

                        ss->cpe_pdata = NULL;
                        ss->cpe = NULL;
                    }

                    del_http_headers(&(ss->http));

                    res = session_process_soap(ss);

                    if(ss->acs && res != METHOD_RETRANSMISSION) {
                        if(ss->acs->destroy)
                            ss->acs->destroy(ss);
                        else if(ss->acs_pdata)
                            free(ss->acs_pdata);

                        ss->acs_pdata = NULL;
                        ss->acs = NULL;
                    }

                    if(res == METHOD_RETRANSMISSION) {
			tr_log(LOG_NOTICE, "Retry request");
                        if(ss->acs && ss->acs->rewind)
                            ss->acs->rewind(ss);
                        reset_buffer((struct buffer *)(ss->http.body));
                        sc->type = SCHED_WAITING_WRITABLE;
                        sc->timeout = current_time() + session_timeout;
                        ss->next_step = NEXT_STEP_HTTP_HEADER;
                    } else if(res == METHOD_SUCCESSED || res == METHOD_FAILED) {
                        if(ss->cpe == NULL && ss->hold == 0) {
                            ss->acs = next_acs_method();
                            if(ss->acs && ss->acs->process)
                                ss->acs->process(ss);
                        }
                        sc->type = SCHED_WAITING_WRITABLE;
                        sc->timeout = current_time() + session_timeout;
                        ss->next_step = NEXT_STEP_HTTP_HEADER;
                    } else if(res == METHOD_RETRY_LATER) {
                        retry_later();
                        sc->need_destroy = 1;
                    } else { //METHOD_END_SESSION
                        sc->need_destroy = 1;
                    }
                }
            } else if(war_strcasecmp(ss->http.start_line.response.code, "301") == 0 ||
                    war_strcasecmp(ss->http.start_line.response.code, "302") == 0 ||
                    war_strcasecmp(ss->http.start_line.response.code, "307") == 0) {
                if(ss->redirect_count++ < 5) {
                    tr_log(LOG_DEBUG, "Session is directed to ....");
                    //Todo: tr_disconn();
                    //tr_conn();
                    ss->http.authorization[0] = '\0';
                    ss->challenged = 0;
                    http_destroy(&(ss->http));
                    if(ss->acs && ss->acs->rewind)
                        ss->acs->rewind(ss);
                    if(ss->cpe && ss->cpe->rewind)
                        ss->cpe->rewind(ss);

                    sc->type = SCHED_WAITING_WRITABLE;
                    sc->timeout = current_time() + session_timeout;
                    ss->next_step = NEXT_STEP_HTTP_HEADER;
                } else {
                    tr_log(LOG_DEBUG, "Session was directed more than 5 times, end the session");
                    if(ss->acs && war_strcasecmp(ss->acs->name, "Inform") == 0)
                        retry_later();
                    sc->need_destroy = 1;
                }
            } else if(war_strcasecmp(ss->http.start_line.response.code, "401") == 0 ||
                    war_strcasecmp(ss->http.start_line.response.code, "407") == 0) {
                tr_log(LOG_DEBUG, "Authentication needed");
                if(ss->challenged) {
                    tr_log(LOG_DEBUG, "Authenticate failed");
                    if(ss->acs && war_strcasecmp(ss->acs->name, "Inform") == 0)
                        retry_later();
                    sc->need_destroy = 1;
                } else if(http_auth(&(ss->http), username, password, "POST", ss->conn.path) == 0) {
                    char *header;
                    if(ss->acs && ss->acs->rewind)
                        ss->acs->rewind(ss);
                    if(ss->cpe && ss->cpe->rewind)
                        ss->cpe->rewind(ss);

                    header = http_get_header(&(ss->http), "Connection:");
                    if(header && war_strncasecmp(header, "close", 5) == 0) {
                        tr_disconn(&(ss->conn));
                        memset(&(ss->conn), 0, sizeof(ss->conn));
                        if(tr_conn(&(ss->conn), url) < 0) {
                            if(ss->acs && war_strcasecmp(ss->acs->name, "Inform") == 0)
                                retry_later();
                            sc->need_destroy = 1;
                            return;
                        } else {
			    war_snprintf(ss->http.req_host, sizeof(ss->http.req_host), "%s", ss->conn.host);
			    war_snprintf(ss->http.req_path, sizeof(ss->http.req_path), "%s", ss->conn.path);
			    ss->http.req_port = tr_atos(ss->conn.port);
                            sc->fd = ss->conn.fd;
                        }
                    }

                    del_http_headers(&(ss->http));
                    reset_buffer((struct buffer *)(ss->http.body));
                    sc->type = SCHED_WAITING_WRITABLE;
                    sc->timeout = current_time() + session_timeout;
                    ss->next_step = NEXT_STEP_HTTP_HEADER;
                    ss->challenged = 1;
                }
            } else {
                tr_log(LOG_DEBUG, "Unsupported response code(%s), end the session", ss->http.start_line.response.code);
                if(ss->acs && war_strcasecmp(ss->acs->name, "Inform") == 0)
                    retry_later();
                sc->need_destroy = 1;
            }
        } else if(res == HTTP_ERROR) {
            if(ss->acs && war_strcasecmp(ss->acs->name, "Inform") == 0)
                retry_later();
            sc->need_destroy = 1;
        }
}

static int http_header(struct session *ss)
{
    if(ss->challenged)
        http_update_authorization(&(ss->http), username, password);
     cookie_header(&(ss->http.cookie), ss->conn.secure, ss->http.req_host, ss->http.req_path, ss->http.req_port);

    if(ss->acs || ss->cpe) {
        push_buffer(&(ss->outbuf),
                "POST %s HTTP/1.1\r\n"
                "Host: %s:%d\r\n"
                "User-Agent: " TR069_CLIENT_VERSION "\r\n"
		"Content-Type: text/xml; charset=utf-8\r\n"
                "%s"
		"%s"
                "SOAPAction: %s\r\n"
                "Transfer-Encoding: chunked\r\n" 
                "\r\n",
		ss->http.req_path,
		ss->http.req_host,
		ss->http.req_port,
		ss->http.authorization,
		ss->http.cookie.cookie_header.data_len > 0 ? ss->http.cookie.cookie_header.data : "",
		ss->acs ? ss->acs->name : "");
    } else {
        push_buffer(&(ss->outbuf),
                "POST %s HTTP/1.1\r\n"
                "Host: %s:%d\r\n"
                "User-Agent: " TR069_CLIENT_VERSION "\r\n"
                "%s"
                "%s"
                //"Content-Length: 0\r\n"
                //"\r\n",
                "Transfer-Encoding: chunked\r\n" 
		"\r\n"
		"0\r\n\r\n",
		ss->http.req_path,
		ss->http.req_host,
		ss->http.req_port,
		ss->http.authorization,
		ss->http.cookie.cookie_header.data_len > 0 ? ss->http.cookie.cookie_header.data : "");
        ss->next_step = NEXT_STEP_NEXT_MSG;
    }

    return METHOD_COMPLETE;
}




static int soap_header(struct session *ss)
{
    struct buffer buf;

    init_buffer(&buf);
	push_buffer(&buf, 
		"<?xml version='1.0' encoding='UTF-8'?>\n"
		"<soap-env:Envelope xmlns:soap-env='http://schemas.xmlsoap.org/soap/envelope/' xmlns:soap-enc='http://schemas.xmlsoap.org/soap/encoding' xmlns:xsd='http://www.w3.org/2001/XMLSchema' xmlns:xsi='http://www.w3.org/2001/XMLSchema-instance' xmlns:cwmp='urn:dslforum-org:cwmp-1-%d'>\n"
		"<soap-env:Header>\n"
		"<cwmp:ID soap-env:mustUnderstand='1'>%s</cwmp:ID>\n"
		"</soap-env:Header>\n"
		"<soap-env:Body>\n", ss->version, ss->id);
    if(ss->cpe) {
	if(ss->cpe_result == METHOD_FAILED) {
	    push_buffer(&buf,
		    "<soap-env:Fault>\n"
		    "<faultcode>Client</faultcode>\n"
		    "<faultstring>CWMP fault</faultstring>\n"
		    "<detail>\n"
		    "<cwmp:Fault>\n");

	} else {
	    push_buffer(&buf, "<cwmp:%sResponse>\n", ss->cpe->name);
	}
    } else if(ss->acs) {
	push_buffer(&buf, "<cwmp:%s>\n", ss->acs->name);
    }

    push_buffer(&(ss->outbuf), "%x\r\n%s\r\n", buf.data_len, buf.data); //Save the data into session's outbuf as chunk block

// 2011.04.14        
    push_buffer(&(ss->outbuf2_data), "%s", buf.data); //Save the data into session's outbuf as chunk block
    
    destroy_buffer(&buf);


    return METHOD_COMPLETE;
}

static int soap_tail(struct session *ss)
{
    struct buffer buf;

    if(ss->hold)
    	ss->sent_empty = 0;

    init_buffer(&buf);
    if(ss->cpe) {
        if(ss->cpe_result == METHOD_FAILED) {
	    push_buffer(&buf,
		    "</cwmp:Fault>\n"
		    "</detail>\n"
		    "</soap-env:Fault>\n"
		    "</soap-env:Body>\n"
		    "</soap-env:Envelope>\n");
	} else {
	    push_buffer(&buf,
		    "</cwmp:%sResponse>\n"
		    "</soap-env:Body>\n"
		    "</soap-env:Envelope>\n", ss->cpe->name);

	}
    } else if(ss->acs) {
	push_buffer(&buf,
		"</cwmp:%s>\n"
		"</soap-env:Body>\n"
		"</soap-env:Envelope>\n", ss->acs->name);


    }
    push_buffer(&(ss->outbuf), "%x\r\n%s\r\n0\r\n\r\n", buf.data_len, buf.data); //Save the data into session's outbuf as chunk block

// 2011.04.14        
    push_buffer(&(ss->outbuf2_data), "%s", buf.data); //Save the data into session's outbuf as chunk block

    destroy_buffer(&buf);

    return METHOD_COMPLETE;
}



static int http_header2(struct session *ss)
{
    if(ss->challenged)
        http_update_authorization(&(ss->http), username, password);
     cookie_header(&(ss->http.cookie), ss->conn.secure, ss->http.req_host, ss->http.req_path, ss->http.req_port);

    if(ss->acs || ss->cpe) {
        push_buffer(&(ss->outbuf2_header),
                "POST %s HTTP/1.1\r\n"
                "Host: %s:%d\r\n"
                "User-Agent: " TR069_CLIENT_VERSION "\r\n"
		"Content-Type: text/xml; charset=utf-8\r\n"
                "%s"
		"%s"
                "SOAPAction: %s\r\n"
//                "Transfer-Encoding: chunked\r\n" 
                "Content-Length: %d\r\n"
                "\r\n",
		ss->http.req_path,
		ss->http.req_host,
		ss->http.req_port,
		ss->http.authorization,
		ss->http.cookie.cookie_header.data_len > 0 ? ss->http.cookie.cookie_header.data : "",
		ss->acs ? ss->acs->name : "",
			ss->outbuf2_data.data_len);
    } else {
        push_buffer(&(ss->outbuf2_header),
                "POST %s HTTP/1.1\r\n"
                "Host: %s:%d\r\n"
                "User-Agent: " TR069_CLIENT_VERSION "\r\n"
                "%s"
                "%s"
                "Content-Length: 0\r\n"
                //"\r\n",
                //"Transfer-Encoding: chunked\r\n" 
		"\r\n"
//		"0\r\n\r\n",
,
		ss->http.req_path,
		ss->http.req_host,
		ss->http.req_port,
		ss->http.authorization,
		ss->http.cookie.cookie_header.data_len > 0 ? ss->http.cookie.cookie_header.data : "");
        ss->next_step = NEXT_STEP_NEXT_MSG;
    }

    return METHOD_COMPLETE;
}


static void session_writable(struct sched *sc)
{
    struct session *ss;

    ss = (struct session *)(sc->pdata);
    for(;;) {
        sc->timeout = current_time() + session_timeout;

// Not send this time
// 2011.04.14        
        if( ss->mode_contlen == 0 ) 
        if(ss->offset < ss->outbuf.data_len) {
            int len;

            len = tr_conn_send(&(ss->conn), ss->outbuf.data + ss->offset, ss->outbuf.data_len - ss->offset);
            if(len > 0) {
                ss->offset += len;
            } else {
                tr_log(LOG_ERROR, "Send to ACS failed!");
                if(ss->acs && war_strcasecmp(ss->acs->name, "Inform") == 0) {
                    retry_later();
                    sc->need_destroy = 1;
                }
            }
            return;
        }

        ss->offset = 0;
        reset_buffer(&(ss->outbuf));

        //send complete
        if(ss->next_step == NEXT_STEP_NEXT_MSG) {
            destroy_buffer(&(ss->outbuf));
            ss->http.block_len = 0;
            ss->http.state = HTTP_STATE_RECV_HEADER;
            reset_buffer((struct buffer *)(ss->http.body));

            sc->type = SCHED_WAITING_READABLE;
            sc->timeout = current_time() + session_timeout;

// 2011.04.14
		        	http_header2(ss);
//						printf("[%d][%s]\n",ss->outbuf2_data.data_len,ss->outbuf2_header.data);
//						printf("[%d][%s]\n",ss->outbuf2_data.data_len,ss->outbuf2_data.data);
		        if( ss->mode_contlen ) {
		        	
	            int len;
//	            int len2;

  	          len = tr_conn_send(&(ss->conn), ss->outbuf2_header.data, ss->outbuf2_header.data_len);
//  	          len = tr_conn_send(&(ss->conn), "\r\n", 2);
							if( ss->outbuf2_data.data_len )
	  	          len = tr_conn_send(&(ss->conn), ss->outbuf2_data.data, ss->outbuf2_data.data_len);

            if(len > 0 ) {
                //ss->offset += len;
            } else {
                tr_log(LOG_ERROR, "Send to ACS failed!");
                if(ss->acs && war_strcasecmp(ss->acs->name, "Inform") == 0) {
                    retry_later();
                    sc->need_destroy = 1;
                }
            }

		        reset_buffer(&(ss->outbuf2_data));
		        reset_buffer(&(ss->outbuf2_header));


						}
						
            return;
        }

        switch(ss->next_step) {
            case NEXT_STEP_HTTP_HEADER:
                tr_log(LOG_DEBUG, "HTTP header");
                if(http_header(ss) == METHOD_COMPLETE) {//Always be TRUE
                    if(ss->acs || ss->cpe) {
                        ss->next_step = NEXT_STEP_SOAP_HEADER;
                    } else {
			    if(ss->hold == 0)
                        	ss->sent_empty = 1;
                        ss->next_step = NEXT_STEP_NEXT_MSG;
                        /*if(ss->received_empty == 1)
                            sc->need_destroy = 1;*/
                    }
                }
                break;
            case NEXT_STEP_SOAP_HEADER:
                if(soap_header(ss) == METHOD_COMPLETE) //Always be TRUE
                    ss->next_step = NEXT_STEP_SOAP_BODY;
                break;
            case NEXT_STEP_SOAP_BODY:
                if(ss->cpe) {
                    tr_log(LOG_NOTICE, "CPE Method name=%s", ss->cpe->name); //Brian+ for debug
                    if((ss->cpe->body == NULL) || (ss->cpe->body(ss) == METHOD_COMPLETE))
                        ss->next_step = NEXT_STEP_SOAP_TAIL;
                } else if(ss->acs) {
     		    if(ss->acs->body == NULL)
    			ss->next_step = NEXT_STEP_SOAP_TAIL;
		    else{
			int res = ss->acs->body(ss);			
			if(res == METHOD_COMPLETE)
			    ss->next_step = NEXT_STEP_SOAP_TAIL;
			else if(res == METHOD_FAILED){
			    retry_later();
			    sched->need_destroy = 1;
			}
		    }
                }
                break;
            case NEXT_STEP_SOAP_TAIL:
                if(soap_tail(ss) == METHOD_COMPLETE) //Always be TRUE
                    ss->next_step = NEXT_STEP_NEXT_MSG;
                break;
            default:
                //Nerver happened
                break;
        }
    }
}

static void session_destroy(struct sched *sc)
{
    struct session *ss;
    int reboot;

    ss = (struct session *)(sc->pdata);
    if(ss) {
        tr_disconn(&(ss->conn));
        http_destroy(&(ss->http));
        destroy_buffer(&(ss->outbuf));

// 2011.04.14
        destroy_buffer(&(ss->outbuf2_header));
        destroy_buffer(&(ss->outbuf2_data));


        if(ss->cpe) {
            if(ss->cpe->destroy)
                ss->cpe->destroy(ss);
            else if(ss->cpe_pdata)
                free(ss->cpe_pdata);

            ss->cpe = NULL;
            ss->cpe_pdata = NULL;
        }

        if(ss->acs) {
            if(ss->acs->destroy)
                ss->acs->destroy(ss);
            else if(ss->acs_pdata)
                free(ss->acs_pdata);

            ss->acs = NULL;
            ss->acs_pdata = NULL;
        }

        reboot = ss->reboot;

        free(ss);
        sc->pdata = NULL;
        sched = NULL;
        lib_end_session();

        if(ip_ping) {
            lib_start_ip_ping();
            ip_ping = 0;
        }
        
        if(trace_route) {
            lib_start_trace_route();
            trace_route = 0;
        }
        if(dsl_diagnostics) {
            lib_start_dsl_diagnostics(dsl_diag);
	    dsl_diagnostics = 0;
        }
        if(atm_diagnostics) {
            lib_start_atm_diagnostics(atm_diag);
	    atm_diagnostics = 0;
        }
#ifdef TR143
	if(download_diagnostics) {
		start_dd();
		download_diagnostics = 0;
	}
        
	if(upload_diagnostics) {
		start_ud();
		upload_diagnostics = 0;
	}
#endif
        
        if(reboot) {
            tr_log(LOG_WARNING, "Program exit because require reboot for some RPC method(s) or something which need to reboot the device while agent is in session happened");
            lib_reboot();
            exit(0);
        }


// 2011.03.25
extern TR_LIB_API int lib_upgradefirmware(void);
extern TR_LIB_API int lib_stop_tr069_for_upgradefirmware(void);

				if( lib_upgradefirmware() ) {
						lib_stop_tr069_for_upgradefirmware();
            exit(0);
				
				}


    }

    sc->need_destroy = 1;

    session_lock = 0;
//    if(get_event_count() > 0 && get_retry_count() == 0)
//	    create_session();
    //There are more event not commited and not retry at the moment
    if(get_event_count() > 0 && get_retry_count() == 0){
	if(get_event_count() == 1){
		struct event *e = NULL;
		next_event(&e);		
		if(e->event_code != S_EVENT_VALUE_CHANGE || has_active_no_param() == 1)
			create_session();
	}else
	        create_session();
    }

}


int create_session(void)
{
    int res = 0;
    static int first = 1;
    struct session *ss;
   /* 
    if(session_lock != 0){
	if(sched == NULL)
		retry_later();
	return -1;
    }
*/
    session_lock = 1;


    reset_active_no_param();

    if(first == 1) {
        lib_start_session();
        GET_NODE_VALUE(ACS_URL, url);
        GET_NODE_VALUE(SESSION_AUTH_USERNAME, username);
        GET_NODE_VALUE(SESSION_AUTH_PASSWORD, password);

        if(res != 0) {
            lib_end_session();
            return -1;
        }

        first = 0;

        register_vct(ACS_URL, url_changed);
        register_vct(SESSION_AUTH_USERNAME, username_changed);
        register_vct(SESSION_AUTH_PASSWORD, password_changed);
        register_vct(IP_PING, ip_ping_changed);
        register_vct(TRACE_ROUTE, trace_route_changed);
        register_vct(DSL_DIAGNOSTICS, dsl_diagnostics_changed);
        register_vct(ATM_DIAGNOSTICS, atm_diagnostics_changed);
#ifdef TR143
        register_vct(DD_STATE, download_diagnostics_changed);
        register_vct(UD_STATE, upload_diagnostics_changed);
#endif
        lib_end_session();
    }



    if(sched == NULL && get_event_count() > 0) { //Not in session at present
        ss = calloc(1, sizeof(*ss));
        sched = calloc(1, sizeof(*sched));
        if(ss == NULL || sched == NULL) {
            tr_log(LOG_ERROR, "Out of memory!");
            if(ss)
                free(ss);
            if(sched)
                free(sched);
            sched = NULL;
            retry_later();
            return -1;
        }

        ss->http.body_type = HTTP_BODY_BUFFER;
        ss->http.body = calloc(1, sizeof(struct buffer));
        if(ss->http.body == NULL) {
            tr_log(LOG_ERROR, "Out of memory!");
            free(ss);
            free(sched);
            sched = NULL;
            retry_later();
            return -1;
        }

        sched->pdata = ss;
        war_snprintf(ss->url, sizeof(ss->url), "%s", url);

        res = tr_conn(&(ss->conn), ss->url);
        if(res < 0) {
	    free(ss->http.body);
            free(ss);
            free(sched);
            sched = NULL;
            retry_later();
	} else {
	    war_snprintf(ss->http.req_host, sizeof(ss->http.req_host), "%s", ss->conn.host);
	    war_snprintf(ss->http.req_path, sizeof(ss->http.req_path), "%s", ss->conn.path);
	    ss->http.req_port = tr_atos(ss->conn.port);
            sched->fd = ss->conn.fd;
            sched->type = SCHED_WAITING_WRITABLE;
            sched->timeout = current_time() + session_timeout;
            sched->on_writable = session_writable;
            sched->on_readable = session_readable;
            sched->on_destroy = session_destroy;
            sched->on_timeout = session_destroy; //The same as destroy
            ss->acs = get_acs_method_by_name("Inform");
	    ss->acs_pdata = NULL;
            ss->version = 0;

// 2011.04.14            
        reset_buffer(&(ss->outbuf2_header));
        reset_buffer(&(ss->outbuf2_data));
        		if( g_mode_contlen )
	            ss->mode_contlen = 1;
            
            war_snprintf(ss->id, sizeof(ss->id), "%ld", tr_random());
            add_sched(sched);
            lib_start_session();
            if(ss->acs->process) {
                ss->acs->process(ss); //The inform ACS method always reture METHOD_SUCCESSED
            }
            tr_log(LOG_DEBUG, "Connect to server(%s) successed!", url);
        }
    }

    return res;
}
