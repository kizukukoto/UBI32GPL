/*=======================================================================
  
       Copyright(c) 2009, Works Systems, Inc. All rights reserved.
  
       This software is supplied under the terms of a license agreement 
       with Works Systems, Inc, and may not be copied nor disclosed except 
       in accordance with the terms of that agreement.
  
  =======================================================================*/
/*!
 * \file cli.c
 *
 * \brief The integration interface. The device can send message to agent through HTTP.
 */
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "tr.h"
#include "atomic.h"
#include "ao.h"
#include "do.h"
#include "cli.h"
#include "log.h"
#include "event.h"
#include "inform.h"
#include "tr_strings.h"
#include "request.h"//add for request download

#include "war_string.h"
#include "war_socket.h"
#include "war_errorcode.h"
#include "war_type.h"
#include "war_time.h"
#include "task.h"


/*!
 * \struct cli
 * \brief The CLI session
 */
struct cli {
    struct http http; /*!<The cli session's HTTP session*/
    struct connection conn; /*!<The session's connection*/
    int destroy_after_write; /*!<If or not destroy the CLI session after sending the response*/
    int offset; /*!<The offset of the output buffer*/
    struct buffer buf; /*!<The output buffer*/
    int cli_nonce;
    int need_session;
};

#define MAX_CLI_CACHE 16
#define MAX_CLI_NONCE 256
static int cli_nonce = 0;
static int cli_cache[MAX_CLI_CACHE];
static int cache_lock = 0;

/*!
 * \struct vct
 * \brief The Value Change Trigger
 */
static struct vct {
    unsigned long int chksum; /*!We calculate the check sum for each trigger, but we not maintain it in hash table,
    		 *because the number of triggers is not too large*/
    const char *path; /*!<The parameter path*/
    void (*trigger)(const char *new_value); /*!<The callback function to notify the parameter's value changed*/
    struct vct *next;
} *vcts = NULL;

static char addr[128] = "127.0.0.1"; /*The CLI listening address*/
static short int port = 1234; /*The CLI listening port*/
static int timeout = 30; /*How long to wait the device to post the next message*/

static unsigned long int calculate_check_sum(const char *str)
{
	unsigned long int hash = 0;

	while(*str) {
		hash = *str + (hash << 6) + (hash << 16) - hash;
		str++;
	}

	return hash;
}

int register_vct(const char *path, void (*trigger)(const char *new_value))
{
	struct vct *prev, *cur, *n;
	unsigned long int chksum;
	int res;

	chksum = calculate_check_sum(path);

	for(prev = NULL, cur = vcts; cur; prev = cur, cur = cur->next) {
		if(chksum != cur->chksum)
			continue;

		res = war_strcasecmp(cur->path, path);
		if(res == 0) {
			cur->trigger = trigger;
			return 0;
		} else if(res > 0) { //Make sure the VCT in a sorted list by alphabet.
			break;
		}
	}

	n = malloc(sizeof(*n));
	if(n) {
		n->chksum = chksum;
		n->path = path; //The path can not be a local variable of the caller's scope
		n->trigger = trigger;
		if(prev) { 
			n->next = prev->next;
			prev->next = n;
		} else {
			n->next = vcts;
			vcts = n;
		}

		return 0;
	} else {
		tr_log(LOG_ERROR, "Out of memory!");
		return -1;
	}
}



void value_change(const char *path, const char *new)
{
    struct vct *v;
    unsigned long int chksum;
    int res, match = 0;
    char *match_val = NULL;

    if (strstr(path, DSL_DIAGNOSTICS)) {
	match = 1; 
	chksum = calculate_check_sum(DSL_DIAGNOSTICS);
    } else if(strstr(path, ATM_DIAGNOSTICS)) {
	match = 2; 
	chksum = calculate_check_sum(ATM_DIAGNOSTICS);
    } else {
	chksum = calculate_check_sum(path);
    }

    for(v = vcts; v; v = v->next) {
	if(chksum != v->chksum)
	    continue;

	if (match == 1) {
	    res = war_strcasecmp(v->path, DSL_DIAGNOSTICS);
	} else if (match == 2) {
	    res = war_strcasecmp(v->path, ATM_DIAGNOSTICS);
	} else 
	    res = war_strcasecmp(v->path, path);
	if(res == 0) {
	    if (match == 0) {
		v->trigger(new);
	    } else {
		int len = strlen(path) + strlen(new) + 2;
		match_val = calloc(1, len);
		if (match_val != NULL) {
                    war_snprintf(match_val, len, "%s|%s", path, new);
		    v->trigger(match_val);
		    free(match_val);
		}
	    }
	    return;
	} else if(res > 0) {
	    return; //Not found
	}
    }
}

int set_cli_config(const char *name, const char *value)
{
    if(war_strcasecmp(name, "CLIAddress") == 0) { /*!<To specify the listening address*/
	war_snprintf(addr, sizeof(addr), "%s", value);
    } else if(war_strcasecmp(name, "CLIPort") == 0) { /*!<To specify the listening port*/
	port = (short)atoi(value);
	} else if(war_strcasecmp(name, "CLITimeout") == 0) { /*!<To specify the waiting timeout*/
		timeout = atoi(value);
	}

	return 0;
}

/*!
 * \brief Send the HTTP response to device
 *
 * \param sc The CLI schedule
 *
 * \return N/A
 */
static void cli_writable(struct sched *sc)
{
    int res;
    struct cli *cli;

    cli = (struct cli *)sc->pdata;

    while(cli->offset != cli->buf.data_len) {
        res = tr_conn_send(&cli->conn, cli->buf.data + cli->offset, cli->buf.data_len - cli->offset);
        if(res > 0) {
            cli->offset += res;
        } else if(war_geterror() == WAR_EAGAIN) {
            return;
        } else {
            break;
        }
    }

    if(cli->destroy_after_write) {
        sc->need_destroy = 1; //Destroy the schedule after send the response
    } else { //Send the current response completed, prepare to receive the next request
            reset_buffer(&(cli->buf));
            cli->http.block_len = 0;
            cli->http.state = HTTP_STATE_RECV_HEADER;
            reset_buffer((struct buffer *)(cli->http.body));
	    del_http_headers(&(cli->http));
	    cli->http.inlen = 0;
	    cli->http.inbuf[0] = '\0';

            sc->type = SCHED_WAITING_READABLE;
            sc->timeout = current_time() + timeout;
    }

    return;
}

#ifdef CODE_DEBUG
/*!
 * \brief Return a html page, this just used for debug mode. The user can test it through browser.
 *
 * \param cli The CLI
 * \param result A string to show on the html page
 * \param name The parameter path
 * \param value The parameter value
 *
 * \return N/A
 */

static void __cli_get_value_change(struct cli *cli, const char *result, const char *name, const char *value)
{
	int len;
	char body[512];

	len = war_snprintf(body, sizeof(body),
			"<html>"
				"<head><title>Value Change</title></head>"
				"<body>"
					"<h2>%s</h2>"
					"<form action='.' method='POST' id='vc'>"
						"Parameter Name :<input type='text' name='name' value='%s'/><br/>"
						"Parameter Value:<input type='text' name='value' value='%s'/><br/>"
					"</form>"
					"<button type='button' onclick=\"javascript: document.getElementById(\'vc\').submit();\">OK</button>"
				"</body>"
			"</html>", result ? result : "Notify Agent Some Parameters'value Changed", name, value);
	push_buffer(&(cli->buf),
			"HTTP/1.1 200 OK\r\n"
			"Content-Length: %d\r\n"
			"Server: TR069 client CLI Server\r\n"
			"Keep-Alive: %d\r\n"
			"Connection: keep-alive\r\n"
			"\r\n"
			"%s", len, timeout, body);
}


/*!
 * \brief A wrapper of __cli_get_value_change()
 *
 * \param cli The CLI session
 *
 * \return N/A
 */
static void cli_get_value_change(struct cli *cli)
{
	__cli_get_value_change(cli, NULL, "", "");
	cli->destroy_after_write = 1;
}


static void __cli_get_add_event(struct cli *cli, const char *result, const char *name, const char *value)
{
	int len;
	char body[512];

	len = war_snprintf(body, sizeof(body),
			"<html>"
				"<head><title>Add Event</title></head>"
				"<body>"
					"<h2>%s</h2>"
					"<form action='.' method='POST' id='vc'>"
						"Event Code:<input type='text' name='code' value='%s'/><br/>"
						"Command Key:<input type='text' name='cmdkey' value='%s'/><br/>"
					"</form>"
					"<button type='button' onclick=\"javascript: document.getElementById(\'vc\').submit();\">OK</button>"
				"</body>"
			"</html>", result ? result : "Send a event to ACS", name, value);
	push_buffer(&(cli->buf),
			"HTTP/1.1 200 OK\r\n"
			"Content-Length: %d\r\n"
			"Server: TR069 client CLI Server\r\n"
			"Keep-Alive: %d\r\n"
			"Connection: keep-alive\r\n"
			"\r\n"
			"%s", len, timeout, body);
}


static void cli_get_add_event(struct cli *cli)
{
	__cli_get_add_event(cli, NULL, "", "");
	cli->destroy_after_write = 1;
}


static void __cli_get_add_object(struct cli *cli, const char *result, const char *name)
{
	int len;
	char body[512];

	len = war_snprintf(body, sizeof(body),
			"<html>"
				"<head><title>Add Object</title></head>"
				"<body>"
					"<h2>%s</h2>"
					"<form action='.' method='POST' id='vc'>"
						"Parent Name:<input type='text' name='name' value='%s'/><br/>"
					"</form>"
					"<button type='button' onclick=\"javascript: document.getElementById(\'vc\').submit();\">OK</button>"
				"</body>"
			"</html>", result ? result : "Notify Agent Some Object Added", name);
	push_buffer(&(cli->buf),
			"HTTP/1.1 200 OK\r\n"
			"Content-Length: %d\r\n"
			"Server: TR069 client CLI Server\r\n"
			"Keep-Alive: %d\r\n"
			"Connection: keep-alive\r\n"
			"\r\n"
			"%s", len, timeout, body);
}


static void cli_get_add_object(struct cli *cli)
{
	__cli_get_add_object(cli, NULL, "");
	cli->destroy_after_write = 1;
}


static void __cli_get_del_object(struct cli *cli, const char *result, const char *name)
{
	int len;
	char body[512];

	len = war_snprintf(body, sizeof(body),
			"<html>"
				"<head><title>Delete Object</title></head>"
				"<body>"
					"<h2>%s</h2>"
					"<form action='.' method='POST' id='vc'>"
						"Object Name:<input type='text' name='name' value='%s'/><br/>"
					"</form>"
					"<button type='button' onclick=\"javascript: document.getElementById(\'vc\').submit();\">OK</button>"
				"</body>"
			"</html>", result ? result : "Notify Agent Some Object Deleted", name);
	push_buffer(&(cli->buf),
			"HTTP/1.1 200 OK\r\n"
			"Content-Length: %d\r\n"
			"Server: TR069 client CLI Server\r\n"
			"Keep-Alive: %d\r\n"
			"Connection: keep-alive\r\n"
			"\r\n"
			"%s", len, timeout, body);
}


static void cli_get_del_object(struct cli *cli)
{
	__cli_get_del_object(cli, NULL, "");
	cli->destroy_after_write = 1;
}

static void __cli_get_request_download(struct cli *cli, const char *result, const char *name)
{
        int len;
        char body[512];

        len = war_snprintf(body, sizeof(body),
                        "<html>"
                                "<head><title>Request Download</title></head>"
                                "<body>"
                                        "<h2>%s</h2>"
                                        "<form action='.' method='POST' id='vc'>"
                                                "Request Download message:<input type='text' name='data' value='%s'/><br/>"
                                        "</form>"
                                        "<button type='button' onclick=\"javascript: document.getElementById(\'vc\').submit();\">OK</button>"
                                "</body>"
                        "</html>", result ? result : "Notify Agent Request Download", name);
        push_buffer(&(cli->buf),
                        "HTTP/1.1 200 OK\r\n"
                        "Content-Length: %d\r\n"
                        "Server: TR069 client CLI Server\r\n"
                        "Keep-Alive: %d\r\n"
                        "Connection: keep-alive\r\n"
                        "\r\n"
                        "%s", len, timeout, body);
}


static void cli_get_request_download(struct cli *cli)
{
        __cli_get_request_download(cli, NULL, "");
        cli->destroy_after_write = 1;
}

#endif

#ifndef CODE_DEBUG
/*!
 * \brief Append the absolute path of the new object
 *
 * \param pname The absolute path of the new object's parent
 * \param in The instance number for the new object, also is the new object's name in database or xml
 *
 * \return N/A
 */

static void cli_response_append_in(struct cli *cli, char *pname, int in)
{
	push_buffer(&(cli->buf), "name=%s%d.\r\n", pname, in);
}
#endif

/*!
 * \brief Generate an error response to device
 *
 * \param cli The CLI session
 * \param code The HTTP response code
 *
 * \return N/A
 */

static void cli_error_response(struct cli *cli, int code)
{
	const char *str;
	char ka[80]; //change 64 to 80
	switch(code) {
		case 200:
			str = "OK";
			break;
		case 400:
			str = "Bad Request";
			break;
		case 403:
			str = "Forbidden";
			break;
		case 404:
			str = "Not Found";
			break;
		default:
			str = "Server Internal Error";
			break;
	}

	if(code == 200) {
		war_snprintf(ka, sizeof(ka), "Keep-Alive: %d\r\nConnection: keep-alive", timeout);

	} else {
		war_snprintf(ka, sizeof(ka), "Connection: close");
	}
	push_buffer(&(cli->buf),
			"HTTP/1.1 %d %s\r\n"
			"Content-Length: 0\r\n"
			"Server: TR069 client CLI Server\r\n"
			"%s\r\n"
			"\r\n", code, str, ka);

	cli->destroy_after_write = (code != 200);
}




/*!
 * \brief Decode the post data. The CLI mode require the request to be encode as the URL mode.
 *
 * \param src The URL string to be decoded
 *
 * \return The decoded URL string
 *
 * URL Encode Rules:
 * 1:space covert to '+' or %2D
 * 2.0-9, a-z, A-Z need not convert.
 * 3.others convert to HEX value,and add '%' before HEX.
 */
static char *url_decode(char *src)
{
	char *dst = NULL;
	char *backup = NULL;

	backup = dst = src;
	while(*src) {
		if(*src == '%'&& (isxdigit(*(src + 1)) && (isxdigit(*(src+ 2))))) {
			char tmp[3];

			memcpy(tmp, src + 1, 2);
			tmp[2] = '\0';

			*dst = strtol(tmp, NULL, 16);
			if(*dst == 0X2D)             /*may space convert to %2D*/
			   *dst = ' ';
			dst++;
			src += 3;
		} else if(*src == '+') {
			*dst = ' ';
			dst++;
			src++;
		} else {
			*dst = *src;
			dst++;
			src++;
		}
	}

	*dst = '\0';

	return backup;
}

/*!
 * \brief Process the value change request
 *
 * \param cli The CLI session
 * \param data The posted data from device
 *
 * \return N/A
 */

static void cli_post_value_change(struct cli *cli, char *data)
{
	char *value = NULL, *name = NULL;
	char *ct;
       
	ct = http_get_header(&(cli->http), "Content-Type: ");
	if(ct == NULL || war_strcasecmp(ct, "application/x-www-form-urlencoded")) {
#ifdef CODE_DEBUG
		tr_log(LOG_WARNING, "Invalid Content-Type: %s", ct ? ct : "");
		 __cli_get_value_change(cli, "Invalid Content-Type", "", "");
		 return;
#else
		 cli_error_response(cli, 400);
		 return;
#endif
	}
	data = skip_blanks(data);

	for(ct = data; ct && *ct;) {
		if(war_strncasecmp(ct, "name=", 5) == 0) {
			name = ct + 5;
		} else if(war_strncasecmp(ct, "value=", 6) == 0) {
			value = ct + 6;
		}
		ct = strchr(ct, '&');
		if(ct) {
			*ct = '\0';
			ct++;
		}
	}

	if(name == NULL || value == NULL) {
#ifdef CODE_DEBUG
		 __cli_get_value_change(cli, "Invalid Post data", "", "");
		 return;
#else
		 cli_error_response(cli, 400);
		 return;
#endif
	}
	if(cli->cli_nonce == 0)
	{
        tr_log(LOG_NOTICE, "cli->cli_nonce=0"); //brian+ for debug    
        if(lib_start_session() > 0) 
        {
			name = url_decode(name);
			value = url_decode(value);
            tr_log(LOG_NOTICE, "name=%s, value=%s", name, value); //brian+ for debug
            //value_change(name, value);
            //Brian+: add parse vc.txt code here
            FILE *fp1;
            fp1 = fopen("/var/tmp/vc.txt", "r");
            if (fp1)
            {
                char tmp[256]={0};
                while(fgets(tmp,sizeof(tmp),fp1))
                {
                    tmp[strlen(tmp)-1]='\0';
                    printf("get vc parameter: %s\n", tmp);    
                    //if(tmp[0] != '\n' && tmp[0] != '\0')    
                    add_inform_parameter(tmp, 1);        
                    memset(tmp,'\0',sizeof(tmp));
                }
                fclose(fp1);
            }
            system("rm -f /var/tmp/vc.txt"); //Brian+ remove vc.txt  
			if(has_active_no_param())
			{
                
                cli->need_session = 1;
                tr_log(LOG_NOTICE, "Need create session, cli->need_session=%d", cli->need_session);
			}
            lib_end_session();
#ifdef CODE_DEBUG
			__cli_get_value_change(cli, "Value Change OK", "", "");
			return;
#else
			cli_error_response(cli, 200);
			return;
#endif
		} else {
#ifdef CODE_DEBUG
			__cli_get_value_change(cli, "Start Session failed", "", "");
			return;
#else
			cli_error_response(cli, 500);
			return;
#endif
		}
	}else{
		char vc_file[FILE_PATH_LEN];
		FILE *fp;
        tr_log(LOG_NOTICE, "cli->cli_nonce=%d", cli->cli_nonce); //brian+ for debug
        war_snprintf(vc_file,sizeof(vc_file),"vc_%d",cli->cli_nonce);
		fp = tr_fopen(vc_file,"ab+");
		if(fp == NULL){
#ifdef CODE_DEBUG
			__cli_get_value_change(cli, "Open cache file failed", "", "");
			return;
#else
			cli_error_response(cli, 500);
			return;
#endif	
		}else{
			int len;
			name = url_decode(name);
			value = url_decode(value);
			len = strlen(name);
			fwrite(&len, sizeof(len), 1, fp);
			fwrite(name, sizeof(char), len, fp);
			len = strlen(value);
			fwrite(&len, sizeof(len), 1, fp);
			fwrite(value, sizeof(char), len, fp);
			fclose(fp);
            tr_log(LOG_NOTICE, "write to vc_%d file successfully", cli->cli_nonce); //brian+ for debug
#ifdef CODE_DEBUG
			__cli_get_value_change(cli, "Value Change OK", "", "");
			return;
#else
			cli_error_response(cli, 200);
			return;
#endif
		}			
	}
}

//Brian+: mark original version
#if 0
static void cli_post_value_change(struct cli *cli, char *data)
{
	char *value = NULL, *name = NULL;
	char *ct;

	ct = http_get_header(&(cli->http), "Content-Type: ");
	if(ct == NULL || war_strcasecmp(ct, "application/x-www-form-urlencoded")) {
#ifdef CODE_DEBUG
		tr_log(LOG_WARNING, "Invalid Content-Type: %s", ct ? ct : "");
		 __cli_get_value_change(cli, "Invalid Content-Type", "", "");
		 return;
#else
		 cli_error_response(cli, 400);
		 return;
#endif
	}
	data = skip_blanks(data);

	for(ct = data; ct && *ct;) {
		if(war_strncasecmp(ct, "name=", 5) == 0) {
			name = ct + 5;
		} else if(war_strncasecmp(ct, "value=", 6) == 0) {
			value = ct + 6;
		}
		ct = strchr(ct, '&');
		if(ct) {
			*ct = '\0';
			ct++;
		}
	}

	if(name == NULL || value == NULL) {
#ifdef CODE_DEBUG
		 __cli_get_value_change(cli, "Invalid Post data", "", "");
		 return;
#else
		 cli_error_response(cli, 400);
		 return;
#endif
	}
	if(cli->cli_nonce == 0)
	{
		if(lib_start_session() > 0) {
			name = url_decode(name);
			value = url_decode(value);
			value_change(name, value);
			add_inform_parameter(name, 1);
			if(has_active_no_param())
				cli->need_session = 1;
			lib_end_session();
#ifdef CODE_DEBUG
			__cli_get_value_change(cli, "Value Change OK", "", "");
			return;
#else
			cli_error_response(cli, 200);
			return;
#endif
		} else {
#ifdef CODE_DEBUG
			__cli_get_value_change(cli, "Start Session failed", "", "");
			return;
#else
			cli_error_response(cli, 500);
			return;
#endif
		}
	}else{
		char vc_file[FILE_PATH_LEN];
		FILE *fp;
		war_snprintf(vc_file,sizeof(vc_file),"vc_%d",cli->cli_nonce);
		fp = tr_fopen(vc_file,"ab+");
		if(fp == NULL){
#ifdef CODE_DEBUG
			__cli_get_value_change(cli, "Open cache file failed", "", "");
			return;
#else
			cli_error_response(cli, 500);
			return;
#endif	
		}else{
			int len;
			name = url_decode(name);
			value = url_decode(value);
			len = strlen(name);
			fwrite(&len, sizeof(len), 1, fp);
			fwrite(name, sizeof(char), len, fp);
			len = strlen(value);
			fwrite(&len, sizeof(len), 1, fp);
			fwrite(value, sizeof(char), len, fp);
			fclose(fp);
#ifdef CODE_DEBUG
			__cli_get_value_change(cli, "Value Change OK", "", "");
			return;
#else
			cli_error_response(cli, 200);
			return;
#endif
		}			
	}
}
#endif

static void cli_post_add_event(struct cli *cli, char *data)
{
	char *code = NULL, *cmdkey = NULL;
	char *ct;
	int multi_flag = 0;

	ct = http_get_header(&(cli->http), "Content-Type: ");
	if(ct == NULL || war_strcasecmp(ct, "application/x-www-form-urlencoded")) {
#ifdef CODE_DEBUG
		tr_log(LOG_WARNING, "Invalid Content-Type: %s", ct ? ct : "");
		 __cli_get_value_change(cli, "Invalid Content-Type", "", "");

		 return;
#else
		 cli_error_response(cli, 400);

		 return;
#endif
	}
	data = skip_blanks(data);
	for(ct = data; ct && *ct;) {
		if(war_strncasecmp(ct, "code=", 5) == 0) {
			code = ct + 5;
		} else if(war_strncasecmp(ct, "cmdkey=", 7) == 0) {
			cmdkey = ct + 7;
			multi_flag = 1;
		}
		ct = strchr(ct, '&');
		if(ct) {
			*ct = '\0';
			ct++;
		}
	}

	if(code == NULL || (cmdkey == NULL && multi_flag == 1)) {
		tr_log(LOG_DEBUG, "No event code or command key found!");
#ifdef CODE_DEBUG
		 __cli_get_add_event(cli, "Invalid Post Data", "", "");

		 return;
#else
		 cli_error_response(cli, 400);

		return;
#endif
	}

	code = url_decode(code);
	if(multi_flag) {
		cmdkey = url_decode(cmdkey);
	}
	
	{
		event_code_t ec;

		ec = string2code(code);
		if(ec == (event_code_t)-1) {
#ifdef CODE_DEBUG
		 __cli_get_add_event(cli, "Invalid Event Code", "", "");

		 return;
#else
		 cli_error_response(cli, 400);

		 return;
#endif
		}
		if(cli->cli_nonce == 0)
		{
			if(multi_flag)
				add_multi_event(ec, cmdkey);
			else
				add_single_event(ec);
			cli->need_session = 1;
		}else{
			char ec_file[FILE_PATH_LEN];
			FILE *fp;
			war_snprintf(ec_file,sizeof(ec_file),"ec_%d",cli->cli_nonce);
			fp = tr_fopen(ec_file,"ab+");
			if(fp == NULL){
#ifdef CODE_DEBUG
				__cli_get_add_event(cli, "Open event cache file failed", "", "");
				 return;
#else
				 cli_error_response(cli, 500);
				 return;
#endif
			}else{
				int len;
				len = strlen(code);
				fwrite(&len, sizeof(len), 1, fp);
				fwrite(code, sizeof(char), len, fp);
				if(multi_flag){
					len = strlen(cmdkey);
					fwrite(&len, sizeof(len), 1, fp);
					fwrite(cmdkey, sizeof(char), len, fp);
				}
				else{
					len = 0;
					fwrite(&len, sizeof(len), 1, fp);
				}					
				fclose(fp);
			}
		}
	}

#ifdef CODE_DEBUG
	 __cli_get_add_event(cli, "Add Event OK", "", "");

	 return;
#else
	 cli_error_response(cli, 200);

	 return;
#endif
}


static void cli_post_add_object(struct cli *cli, char *data)
{
	char *name = NULL;
	char *ct;
	int in;
#ifdef CODE_DEBUG
	char result[256]; 
#endif
	ct = http_get_header(&(cli->http), "Content-Type: ");
	if(ct == NULL || war_strcasecmp(ct, "application/x-www-form-urlencoded")) {
#ifdef CODE_DEBUG
		tr_log(LOG_WARNING, "Invalid Content-Type: %s", ct ? ct : "");
		 __cli_get_add_object(cli, "Invalid Content-Type", "");
		 return;
#else
		 cli_error_response(cli, 400);
		 return;
#endif
	}
	data = skip_blanks(data);

	for(ct = data; ct && *ct;) {
		if(war_strncasecmp(ct, "name=", 5) == 0) {
			name = ct + 5;
			break;
		} 
	}

	if(name[0] == '\0') { 
		tr_log(LOG_DEBUG, "No object name found!");
#ifdef CODE_DEBUG
		 __cli_get_add_object(cli, "Invalid Post data", "");
		 return;
#else
		 cli_error_response(cli, 400);
		 return;
#endif
	}
	
	if(lib_start_session() > 0) {
		name = url_decode(name);
		if((in = add_object(name, strlen(name))) >= 9000) {
#ifdef CODE_DEBUG
			 __cli_get_add_object(cli, "Add Object Error", "");
			 return;
#else
		 cli_error_response(cli, 400);
		 return;
#endif
		}
		lib_end_session();
#ifdef CODE_DEBUG
		war_snprintf(result, sizeof(result), "Add Object OK. object_name=%s instance_number=%d", name,in); 
		//return __cli_get_add_object(cli, "Add Object OK", "");
		 __cli_get_add_object(cli, result, "");
		 return;
#else
		cli_error_response(cli, 200);
		 cli_response_append_in(cli, name, in);
		 return;
#endif
	} else {
#ifdef CODE_DEBUG
		 __cli_get_add_object(cli, "Start Session Failed", "");
		 return;
#else
		 cli_error_response(cli, 500);
		 return;
#endif
	}

}


static void cli_post_del_object(struct cli *cli, char *data)
{
	char *name = NULL;
        //char *defaults = NULL;
	char *ct;


	ct = http_get_header(&(cli->http), "Content-Type: ");
	if(ct == NULL || war_strcasecmp(ct, "application/x-www-form-urlencoded")) {
#ifdef CODE_DEBUG
		tr_log(LOG_WARNING, "Invalid Content-Type: %s", ct ? ct : "");
		 __cli_get_add_object(cli, "Invalid Content-Type", "");
		 return;
#else
		 cli_error_response(cli, 400);
		 return;
#endif
	}
	data = skip_blanks(data);

	for(ct = data; ct && *ct;) {
		if(war_strncasecmp(ct, "name=", 5) == 0) {
			name = ct + 5;
			break;
		} 
		#if 0
		else if(war_strncasecmp(ct, "default=", 8) == 0) {
			defaults = ct + 8;
		}
		ct = strchr(ct, '&');
		if(ct) {
			*ct = '\0';
			ct++;
		}
		#endif
	}

	if(name == NULL) { // || defaults == NULL) {
		tr_log(LOG_DEBUG, "No object name found!");
#ifdef CODE_DEBUG
		 __cli_get_del_object(cli, "Invalid Post Data", "");
		 return;
#else
		 cli_error_response(cli, 400);
		 return;
#endif
	}

	if(lib_start_session() > 0) {
		name = url_decode(name);
		//defaults = url_decode(defaults);	
		if(delete_object(name, strlen(name)) != 0) {
#ifdef CODE_DEBUG
			 __cli_get_del_object(cli, "Delete Object Error", "");
			 return;
#else
		 cli_error_response(cli, 400);
		 return;
#endif
		}
		lib_end_session();
#ifdef CODE_DEBUG
		 __cli_get_del_object(cli, "Delete Object OK", "");
		 return;
#else
		 cli_error_response(cli, 200);
		 return;
#endif
	} else {
#ifdef CODE_DEBUG
		 __cli_get_del_object(cli, "Start Session Failed", "");
		 return;
#else
		 cli_error_response(cli, 500);
		 return;
#endif
	}

}


static void cli_post_request_download(struct cli *cli, char *data)
{
	char *ct;

	ct = http_get_header(&(cli->http), "Content-Type: ");
	if(ct == NULL || war_strcasecmp(ct, "application/x-www-form-urlencoded")) {
#ifdef CODE_DEBUG
		tr_log(LOG_WARNING, "Invalid Content-Type: %s", ct ? ct : "");
		 __cli_get_request_download(cli, "Invalid Content-Type", "");
		 return;
#else
		 cli_error_response(cli, 400);
		 return;
#endif
	}
	if(data == NULL) {
#ifdef CODE_DEBUG
		 __cli_get_request_download(cli, "Invalid Post data", "");
		 return;
#else
		 cli_error_response(cli, 400);
		 return;
#endif
	}

	if(lib_start_session() > 0) {
	#if 0
	    char data[200];
	    war_snprintf(data, sizeof(data),
                                "<FileType>%s</FileType>\n"
				"<FileTypeArg xsi:type='soap-enc:Array' soap-enc:arrayType='cwmp:ArgStruct[0]>"
					"<ArgStruct>"
						//"<Name>%s</Name>"
						//"<Value>%s</Value>"	
					"</ArgStruct>"
				"</FileTypeArg>\n"
				, get_req_type(req_type));
	#endif
		data = url_decode(data);
		add_request("RequestDownload", -1, NULL, data);
		add_single_event(S_EVENT_REQUEST_DOWNLOAD);
		cli->need_session = 1;
		lib_end_session();
#ifdef CODE_DEBUG
		 __cli_get_request_download(cli, "Request Download OK", "");
		 return;
#else
		 cli_error_response(cli, 200);
		 return;
#endif
	} else {
#ifdef CODE_DEBUG
		 __cli_get_request_download(cli, "Start Session Failed", "");
		 return;
#else
		 cli_error_response(cli, 500);
		 return;
#endif
	}

}

static void cli_post_file_transfer(struct cli *cli, char *data)
{
	char *ct;
	char announce_url[256];
	int status;

	ct = http_get_header(&(cli->http), "Content-Type: ");
	if(ct == NULL || war_strcasecmp(ct, "application/x-www-form-urlencoded")) {
		tr_log(LOG_WARNING, "Invalid Content-Type: %s", ct ? ct : "");
		cli_error_response(cli, 400);
		return;
	}

	data = skip_blanks(data);
	war_snprintf(announce_url, sizeof(announce_url),
			"http://%s:%d/file/transfer/", addr, port);

	data = url_decode(data);
	status = gen_transfer_task(announce_url, data);	
	if(status == 0)
		cli_error_response(cli, 200);
	else
		cli_error_response(cli, 400);
	return;
}

/*!
 * \struct cli_url
 * The accessable URLs which used by device to post request
 */
static struct cli_url {
	const char *url;
	void (*process_post)(struct cli *cli, char *post_data);
#ifdef CODE_DEBUG
	void (*process_get)(struct cli *cli);
#endif
} cli_urls[] = {
	{
		"/value/change/",
		cli_post_value_change
#ifdef CODE_DEBUG
		,cli_get_value_change
#endif
	},
	{
		"/add/event/",
		cli_post_add_event
#ifdef CODE_DEBUG
		,cli_get_add_event
#endif
	},
	{
		"/add/object/",
		cli_post_add_object
#ifdef CODE_DEBUG
		,cli_get_add_object
#endif
	},
	{
		"/del/object/",
		cli_post_del_object
#ifdef CODE_DEBUG
		,cli_get_del_object
#endif
	},
	{
		"/request/download/",
		cli_post_request_download
#ifdef CODE_DEBUG
		,cli_get_request_download
#endif
	},
	{
		"/file/transfer/",
		cli_post_file_transfer
	}
};

/*!
 * \brief Process the request
 *
 * \param cli The CLI session
 *
 * \return N/A
 */
static void cli_process_request(struct cli *cli)
{
	int i;
	for(i = 0; i < sizeof(cli_urls) / sizeof(cli_urls[0]); i++) {
		if(war_strcasecmp(cli->http.start_line.request.uri, cli_urls[i].url) == 0) {
			if(war_strcasecmp(cli->http.start_line.request.method, "POST") == 0) {
				 cli_urls[i].process_post(cli, ((struct buffer *)(cli->http.body))->data);
				 return;
#ifdef CODE_DEBUG
			} else if(war_strcasecmp(cli->http.start_line.request.method, "GET") == 0) {
				
				 cli_urls[i].process_get(cli);
				 return;
#endif
			} else {
				cli->destroy_after_write = 1;
				 cli_error_response(cli, 403);
				 return;
			}
		}
	}

	cli->destroy_after_write = 1;
	 cli_error_response(cli, 404);
	 return;
}


/*!
 * \brief To receive a request from device
 *
 * \param sc The CLI session scheduler
 *
 * \return N/A
 */
static void cli_readable(struct sched *sc)
{
	int rc;
	struct cli *cli;


	if(sc->pdata == NULL) {
		cli = calloc(1, sizeof(*cli));
		if(cli == NULL) {
			tr_log(LOG_ERROR, "Out of memory!");
			sc->need_destroy = 1;
			return;
		} else {
			cli->http.body_type = HTTP_BODY_BUFFER;
			cli->conn.fd = sc->fd;
			cli->http.state = HTTP_STATE_RECV_HEADER;
			sc->pdata = cli;
			cli->http.body = calloc(1, sizeof(struct buffer));
			if(get_session_lock() == 0)
				set_session_lock(1);
			else{
				cli->cli_nonce = cli_nonce ++;
				if(cli_nonce >= MAX_CLI_NONCE)
					cli_nonce = 0;
			}
			if(cli->http.body == NULL) {
				tr_log(LOG_ERROR, "Out of memory!");
				sc->need_destroy = 1;
				return;
			}
		}
	} else {
		cli = (struct cli *)(sc->pdata);
	}

	cli->destroy_after_write = 0;


	rc = http_recv(&cli->http, &cli->conn);
	if(rc  == HTTP_COMPLETE && cli->http.msg_type == HTTP_REQUEST) {
		cli_process_request(cli);
	} else if(rc == HTTP_ERROR) {
		sc->need_destroy = 1;
		return;
	} else {
		return; //Waiting
	}

	cli->offset = 0;
	sc->type = SCHED_WAITING_WRITABLE;
	sc->timeout = current_time() + timeout;

	return;
}

static int load_cache(void)
{
	int i;
	int need_session = 0;
	for(i = 0; i < MAX_CLI_CACHE; i ++){
		if(cli_cache[i] != 0){
			char fn[FILE_PATH_LEN];
			FILE *fp;
			war_snprintf(fn, sizeof(fn), "vc_%d", cli_cache[i]);
			if(lib_start_session() > 0) {
				fp = tr_fopen(fn,"rb");
				if(fp != NULL){
					int len;
					while(fread(&len, sizeof(len), 1, fp) > 0){
						char *name, *value;
						name = calloc(len + 1, sizeof(char));
						fread(name, sizeof(char), len, fp);
						fread(&len, sizeof(len), 1, fp);
						value = calloc(len + 1, sizeof(char));
						fread(value, sizeof(char), len, fp);
						value_change(name, value);
						add_inform_parameter(name, 1);
						if(has_active_no_param())
							need_session = 1;
						free(name);
						free(value);
					}
					fclose(fp);
					tr_remove(fn);
					lib_end_session();
				}else{
					lib_end_session();
					continue;
				}
			}else
				continue;
			war_snprintf(fn, sizeof(fn), "ec_%d", cli_cache[i]);
			fp = tr_fopen(fn, "rb");			
			if(fp != NULL){
				int len;
				while(fread(&len, sizeof(len), 1, fp) > 0){
					char *code, *cmd_key;
					event_code_t ec;
					code = calloc(len + 1, sizeof(char));
					fread(code, sizeof(char), len, fp);
					ec = string2code(code);
					free(code);
					fread(&len, sizeof(len), 1, fp);
					if(len > 0){
						cmd_key = calloc(len + 1, sizeof(char));
						fread(cmd_key, sizeof(char), len, fp);
						add_multi_event(ec, cmd_key);
						need_session = 1;
						free(cmd_key);
					}else{
						add_single_event(ec);	
						need_session = 1;
					}
				}
			}
			cli_cache[i] = 0;
		}
	}
	return need_session;
}

static void clear_cache(void)
{
	int i;
	for(i = 0; i < MAX_CLI_NONCE; i ++){
		char fn[FILE_PATH_LEN];
		war_snprintf(fn, sizeof(fn), "vc_%d", i);
		if(tr_exist(fn))
			tr_remove(fn);
		war_snprintf(fn, sizeof(fn), "ec_%d", i);
		if(tr_exist(fn))
			tr_remove(fn);
	}
}

static void cache_timeout(struct sched *sc)
{
	if(get_session_lock() == 0){
		sc->need_destroy = 1;	
		if(load_cache() == 1)
			create_session();
	}else{
		sc->timeout = war_time(NULL) + 1;
	}
}

/*!
 * \brief Destroy a CLI session scheduler
 *
 * \param sc The CLI session scheduler
 * \return N/A
 */
static void cli_destroy(struct sched *sc)
{
	int need_session = 0;
	sc->need_destroy = 1;
	if(sc->pdata) {
		struct cli *cli;

		cli = (struct cli *)(sc->pdata);
		http_destroy(&cli->http);
		tr_disconn(&cli->conn);
		destroy_buffer(&cli->buf);
		if(cli->cli_nonce == 0){
			set_session_lock(0);
			need_session = cli->need_session;
		}else{
			int i;
			for(i = 0; i < MAX_CLI_CACHE; i ++){				
				if(cli_cache[i] == 0)
					cli_cache[i] = cli->cli_nonce;
			}
			if(i == MAX_CLI_CACHE){
				char fn[FILE_PATH_LEN];
				tr_log(LOG_ERROR,"CLI cache full, drop changed parameters and events.");
				war_snprintf(fn, sizeof(fn), "vc_%d", cli->cli_nonce);
				if(tr_exist(fn))
					tr_remove(fn);
				war_snprintf(fn, sizeof(fn), "ec_%d", cli->cli_nonce);
				if(tr_exist(fn))
					tr_remove(fn);
			}
			if(cache_lock == 0)
			{
				struct sched *cli_sc;
				cli_sc = calloc(1, sizeof(*cli_sc));
				sc->type = SCHED_WAITING_TIMEOUT;
				sc->timeout = war_time(NULL) + 1; 
				sc->on_timeout = cache_timeout;
				add_sched(cli_sc);				
				cache_lock = 1;
			}
		}
		free(sc->pdata);
		sc->fd = -1;
		sc->pdata = NULL;
	}

	if(sc->fd >= 0)
		war_sockclose(sc->fd);

	if(need_session == 1) {
		tr_log(LOG_DEBUG, "Create session after CLI session");
		create_session();
	}
}

/*!
 * \brief To accept a new incoming CLI connection
 *
 * \param sc The CLI server scheduler
 *
 * \return N/A
 */
static void cli_acceptable(struct sched *sc)
{
	int client;
#if defined(__DEVICE_IPV4__)
	struct sockaddr_in cli_addr;
#else
	struct sockaddr_storage cli_addr;
#endif
	socklen_t cli_len = sizeof(cli_addr);

	client = war_accept(sc->fd, (struct sockaddr *)&cli_addr, &cli_len);
	if(client >= 0) {
		/*Create a CLI session*/
		nonblock_socket(client);
		sc = calloc(1, sizeof(*sc));
		if(sc == NULL) {
			tr_log(LOG_ERROR, "Out of memory!");
			war_sockclose(client);
		} else {
			sc->type = SCHED_WAITING_READABLE;
			sc->fd = client;
			//sc->timeout = current_time() + timeout;
			sc->timeout = war_time(NULL) + timeout; /*current_time is schdule for first call time. in cli we must use system current time*/
			sc->on_readable = cli_readable;
			sc->on_writable = cli_writable;
			sc->on_timeout = cli_destroy;
			sc->on_destroy = cli_destroy;
			add_sched(sc);
		}
	}

	return;
}

int launch_cli_listener()
{
	int cli;
	cli = tr_listen(addr, port, SOCK_STREAM, 1);
	if(cli >= 0) {
		static struct sched sched;
		memset(&sched, 0, sizeof(sched));
		sched.type = SCHED_WAITING_READABLE;
		sched.timeout = -1; //Nerver timeout
		sched.fd = cli;
		sched.on_readable = cli_acceptable;
		sched.on_writable = NULL;
		sched.on_destroy = NULL;
		add_sched(&sched);
	}
	clear_cache();
	memset(cli_cache, 0, sizeof(cli_cache));
	return 0;
}
