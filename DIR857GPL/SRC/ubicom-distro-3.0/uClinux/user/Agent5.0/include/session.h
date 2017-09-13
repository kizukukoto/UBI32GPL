/*=======================================================================
  
       Copyright(c) 2009, Works Systems, Inc. All rights reserved.
  
       This software is supplied under the terms of a license agreement 
       with Works Systems, Inc, and may not be copied nor disclosed except 
       in accordance with the terms of that agreement.
  
  =======================================================================*/
/*!
 * \file session.h
 *
 */
#ifndef __HEADER_SESSION__
#define __HEADER_SESSION__

#include "sched.h"
#include "buffer.h"
#include "http.h"
#include "method.h"
#include "connection.h"
#include "network.h"
#include "war_string.h"

enum {
    CHALLENGE_NONE,
    CHALLENGE_BASIC,
    CHALLENGE_DIGEST
};

enum {
        NEXT_STEP_HTTP_HEADER,
	NEXT_STEP_SOAP_HEADER,
	NEXT_STEP_SOAP_BODY,
	NEXT_STEP_SOAP_TAIL,
	NEXT_STEP_NEXT_MSG
};

struct session {
	struct http http;
	struct connection conn;

	char id[128];
	void *acs_pdata;
	const struct acs_method *acs;
	void *cpe_pdata;
	const struct cpe_method *cpe;

	unsigned int reboot: 1;
	unsigned int diagnostic: 1;

	unsigned int hold:1;
	unsigned int cpe_result:2;
	unsigned int version:3;
        unsigned int received_empty:1;
        unsigned int sent_empty:1;
	unsigned int next_step:3;
	unsigned int redirect_count:3;
	//Used for HTTP authentication
        unsigned int challenged: 2;
	char url[256];

        int offset;
	struct buffer outbuf;
	
	// 2011.04.14
	struct buffer outbuf2_header;
	struct buffer outbuf2_data;

	long mode_contlen;	
	
};

/*!
 * \brief Notify the session that something happened and need to reboot the device
 *
 * \return 0 if the agent is in session at the moment, -1 if not.
 */
int need_reboot_device(void);

int create_session(void);

/*!
 * \brief The config callback function
 */
int set_session_config(const char *name, const char *value);


int get_session_lock(void);

void set_session_lock(int lock);


#ifdef GET_NODE_VALUE
#error The GET_NODE_VALUE redefined
#else
#define GET_NODE_VALUE(path, buf) do { \
	char *v = NULL; \
        node_t node; \
        res |= lib_resolve_node(path, &node); \
        if(res == 0) { \
            res = lib_get_value(node, &v); \
            if(res == 0) { \
                if(v) { \
                    war_snprintf(buf, sizeof(buf), "%s", v); \
		} \
            } else { \
 	        tr_log(LOG_ERROR, "Lib get %s value fail", path); \
	    } \
        } else { \
 	    tr_log(LOG_ERROR, "Lib resolve %s fail", path); \
	} \
	if(v) \
		lib_destroy_value(v); \
} while(0)
#endif

#define UNKNOWN_TIME	"0001-01-01T00:00:00Z"
#endif
