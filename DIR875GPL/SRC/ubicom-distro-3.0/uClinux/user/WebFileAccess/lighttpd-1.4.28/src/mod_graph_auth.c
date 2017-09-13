#include "base.h"
#include "log.h"
#include "debug.h"
#include "plugin.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <utime.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <net/if.h>
#include <net/ethernet.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/sysinfo.h>
extern wa_session_timeout;
#define WA_SESSION_TIME 7200;
#define sin_addr(s) (((struct sockaddr_in *)(s))->sin_addr)
#define DEBUG_MSG(fmt, args...) \
	if (p->conf.debug) { \
		cprintf("XXX %s(%d) - "fmt, __FUNCTION__, __LINE__, ## args); \
	}
	
	
/**
 * this is a graph_auth for a lighttpd plugin
 */

/* plugin config for all request/connections */

#ifdef CONFIG_WEB_404_REDIRECT
int enable_404_redirect=0;
#if 0
int lan_request=0;
char *device_name;
char device_name_mac[24];
char lan_ip[17];
struct in_addr lan_subnet;
#endif
#endif

typedef struct {
	buffer *login_page;
	buffer *session_dir;
	unsigned int	def_session_timeout;
	unsigned short	debug;
	array *except_paths;
	buffer *wizard_page;
	array *wizard_paths;
} plugin_config;

typedef struct {
	PLUGIN_DATA;
	buffer *location;
	buffer *client_ip;
	plugin_config **config_storage;
	plugin_config conf;
} plugin_data;

void get_current_ip_addr(char *if_name,char *lan_ip)
{
	int sockfd;
	struct ifreq ifr;
	struct in_addr in_addr;

	if ((sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_RAW)) < 0)
		return 0;

	strcpy(ifr.ifr_name, if_name);
	if( ioctl(sockfd, SIOCGIFADDR, &ifr) < 0) {
		close(sockfd);
		return 0;
	}

	close(sockfd);

	in_addr.s_addr = sin_addr(&ifr.ifr_addr).s_addr;
	strcpy(lan_ip,inet_ntoa(in_addr));
}

static char *cmd_nvram_get(const char *key, char *buf)
{
	FILE *fp;
	char tbuf[2048], cmds[128];

	if (key == NULL || *key == '\0')
		goto out;

	bzero(tbuf, sizeof(tbuf));
	sprintf(cmds, "nvram get %s", key);

	if ((fp = popen(cmds, "r")) == NULL)
		goto out;

	fgets(tbuf, sizeof(tbuf), fp);
	pclose(fp);
	if (*tbuf == '\0')
		goto out;
	if (tbuf[strlen(tbuf) - 1] == '\r' || tbuf[strlen(tbuf) - 1] == '\n')
		tbuf[strlen(tbuf) - 1] = '\0';
	return strcpy(buf, tbuf);
out:
	return NULL;
}

#ifdef CONFIG_PLEASE_WAIT_PAGE
/*add PleaseWait page for ult count.*/
int checkServiceAlivebyName(char *service)
{
	FILE * fp;
	char buffer[1024];
	if((fp = popen("ps -ax", "r"))){
		while( fgets(buffer, sizeof(buffer), fp) != NULL ) {
			if(strstr(buffer, service)) {
				pclose(fp);
				return 1;
			}
		}
	}
	pclose(fp);
	return 0;
}

int cmd_nvram_set(char *key, char *buf)
{
	FILE *fp;
	char cmds[128];

	if (key == NULL || *key == '\0' || buf == NULL || *buf=='\0')
		return 0;

	sprintf(cmds, "nvram set %s=%s commit", key,buf);

	if ((fp = popen(cmds, "r")) == NULL){
		return 0;
	}
	pclose(fp);
	return 1;
}

void go_to_pleasewait(server *srv, connection *con, plugin_data *p){
	char loc[256]={0};
	char DeviceName[256]={0};
	char lan_ip[32]={};
	char lan_interface[16]={};
	char *html_return_page;
	
	html_return_page = strtok(con->uri.path->ptr,"/");	
	cmd_nvram_set("html_response_page",html_return_page);
		
	cmd_nvram_get("lan_bridge", lan_interface);
	get_current_ip_addr(lan_interface,lan_ip);
			
	if (cmd_nvram_get("lan_ipaddr", DeviceName) == NULL)
		strcpy(DeviceName,lan_ip);
			
	sprintf(loc, "http://%s/please_wait.asp",DeviceName);
	buffer_reset(p->location);
	buffer_append_string_len(p->location, loc, strlen(loc));	
	response_header_insert(srv, con, CONST_STR_LEN("Location"), CONST_BUF_LEN(p->location));
	con->http_status = 307;
	con->file_finished = 1;
						
}
#endif

/*
 * return 1 : match
 * return 0 : not match
 */
static int nvram_match(char *key, char *string) {
	char comm[128]={0};
	FILE *fp = NULL;
	char get_nvram[128]={0};

	sprintf(comm, "/bin/nvram get %s", key);

	fp = popen(comm, "r");
	if (fp == NULL)
		return 0;
	fgets(get_nvram, sizeof(get_nvram) - 1, fp);
	pclose(fp);
	if (strlen(get_nvram) == 0) 
		return 0; //no such key.
	// value can be as "\n"
	get_nvram[strlen(get_nvram) - 1] = '\0';

	return (!strcmp(get_nvram, string));
}

#ifdef CONFIG_WEB_404_REDIRECT
/* Signal handling */
static void http_signal(int sig)
{
	switch (sig)
	{
#if 0
//***************************************************************************************************
		case SIGUSR2:
			nvram_flag_reset();
/*	Date:	2010-01-27
*	Name:	jimmy huang
*	Reason:	When change usb type to 3 or 4 (windows mobile 5 or iPhone)
*			We will change lan ip to 192.168.99.1 and reboot device
*			But before reboot, br0 is still 192.168.0.1, thus
*			host will not match nvram_safe_get(lan_bridge)
*			If we do redirect to 404 to 192.168.99.1/error-404.htm, the count down page
*			will not success show out
*			So... we using real br0's ip instead of using nvram's value
*/
			strncpy(lan_ip,read_ipaddr_no_mask(nvram_safe_get("lan_bridge")),sizeof(lan_ip));
			device_name = nvram_safe_get("lan_device_name");

			struct in_addr lan_ip_in_addr;
			struct in_addr lan_mask_in_addr;
	
			inet_aton(lan_ip, &lan_ip_in_addr);
			get_netmask(nvram_safe_get("lan_bridge"),&lan_mask_in_addr);
			lan_subnet.s_addr = lan_ip_in_addr.s_addr & lan_mask_in_addr.s_addr;
			break;
//***************************************************************************************************
#endif
		case SIGILL:
			//cprintf("lighttpd recv SIGILL\n");
			enable_404_redirect = 0;
			break;
		case SIGSYS:
			//cprintf("lighttpd recv SIGSYS\n");
			enable_404_redirect = 1;
			break;
	}
}
/*
	return 1 : It's my ipv6 address
	return 0 : It's not  my ipv6 address
*/
static int my_ipv6_addr(char *target)
{
	FILE *fp = NULL;
	char buf[256] = {};
	char addr[64] = {};
	char *p = NULL;

	if ((p = strchr(target, '[')))
		strncpy(addr, p + 1, sizeof(addr));

	if ((p = strchr(addr, ']')))
		*p = '\0';
	
	sprintf(buf, "ifconfig | grep \"inet6 addr: %s/64\"", addr);

	if ((fp = popen(buf, "r")) == NULL)
		return 0;

	if (fgets(buf, sizeof(buf), fp)) {
		pclose(fp);
		return 1;
	} else {
		pclose(fp);
		return 0;
	}

}

/*
 *	return 1 : not match
 *	return 0 : match
 */
static int check_lan_match(void *p_d, char *authority) {
	plugin_data *p = p_d;
	
	char *tmp_1, *tmp_2, *tmp_3;
	char addr[64] = {};
	char lan_ip[32] = {};
	char lan_interface[16] = {};
	int count, ret = 0;

	//get current lan ip from system instead nvram for compare correct lan ip
	cmd_nvram_get("lan_bridge", lan_interface);
	get_current_ip_addr(lan_interface,lan_ip);
	
	DEBUG_MSG("#### authority: %s\n", authority);
	tmp_1 = authority;
	for (count = 0; (tmp_2 = strchr(tmp_1, ':')) != 0; count++)
		tmp_1 = tmp_2 + 1;
	
	DEBUG_MSG(" #### count:%d\n", count);
	if (count == 0 && (strcmp(lan_ip,authority)!=0)) { // maybe is 192.168.0.1 or dlinkrouter or dlinkrouterXXXX
		cmd_nvram_get("lan_device_name", addr);
		DEBUG_MSG("#### addr:'%s', authority:'%s'\n", addr, authority);
		if (strncmp(addr, authority, strlen(addr)))
			ret = 1;
	} else if (count == 1) { // maybe is 192.168.0.1:8080
		DEBUG_MSG("##### tmp_2:%d, authority:%d, cut:%d\n", strlen(tmp_1), strlen(authority), (tmp_1 - 1) - authority);
		strncpy(addr, authority, (tmp_1 - 1) - authority);
		DEBUG_MSG("#### IPv4 addr: %s\n", addr);
		if (strcmp(lan_ip,authority)!=0)
			ret = 1;
	} else if (count > 1) { // maybe is [2001:aaaa:bbbb:cccc:240:d0ff:fea3:63ab] or [2001:aaaa:bbbb:cccc:240:d0ff:fea3:63ab]:8080
		tmp_1 = authority;
		tmp_2 = strchr(tmp_1, ']');
		if ((tmp_1 = strchr(tmp_2, ':')) != 0)
			strncpy(addr, authority, tmp_1 - authority);
		else
			strcpy(addr, authority);

		if ( (tmp_3=strchr(addr, '%')) )
			*tmp_3 = '\0';

		DEBUG_MSG("#### IPv6 addr: %s\n", addr);

		if (!my_ipv6_addr(addr))
			ret = 1;
	}

	DEBUG_MSG("#### ret:%d\n", ret);
	return ret;
}
#endif

static int auth_session_exist(void *p_d, const char *session_handle)
{
	plugin_data *p = p_d;
	if (p->conf.debug) {
		int ret = access(session_handle, F_OK);

		DEBUG_MSG("\n\tsession_handle: '%s' ret: '%d'\n", session_handle, ret);

		return ret;
	}
	return access(session_handle, F_OK);
}

static int auth_session_renew(void *p_d, const char *session_handle)
{
	plugin_data *p = p_d;
	int ret = -1;

	if (access(session_handle, F_OK) != 0)
		goto out;

	utime(session_handle, NULL);
	ret = 0;
out:
	DEBUG_MSG("\n\tsession_handle: '%s' ,ret: '%d'\n", session_handle, ret);

	return ret;
}

static int auth_get_timeout_second(void *p_d)
{
	plugin_data *p = p_d;
	char session_timeout[64];

        if(wa_session_timeout)
                return WA_SESSION_TIME;
	if (cmd_nvram_get("session_timeout", session_timeout) == NULL)
		goto default_t;
	if (strlen(session_timeout) == 0)
		goto default_t;
	return atoi(session_timeout);
default_t:
	return p->conf.def_session_timeout;	/* default value */
}

static int auth_session_timeout(void *p_d, const char *session_handle)
{
	plugin_data *p = p_d;
	int ret = -1;
	struct stat buf;
	time_t cur_time = time(NULL);

	stat(session_handle, &buf);

	if (cur_time - buf.st_atime <= auth_get_timeout_second(p_d)) {
		auth_session_renew(p_d, session_handle);
		ret = 0;
	} else
		unlink(session_handle);

	DEBUG_MSG("\n\tsession_handle: '%s' ret: '%d'\n", session_handle, ret);

	return ret;
}

/* Lighttpd Input:
 *
 * IPv6 @rmt_ip_str := [::ffff:192.168.0.100]
 * IPv4 @rmt_ip_str := 192.168.0.100
 *
 * Return: Always 0
 * */
static int auth_create_session_handle(void *p_d, char *buf)
{
	char *pe, ip[128];
	plugin_data *p = p_d;

	bzero(ip, sizeof(ip));

#if 1
	UNUSED(pe);
	strcpy(ip, p->client_ip->ptr);
#else
	if ((pe = strrchr(p->client_ip->ptr, ':')) == NULL)	/* port not found */
		strcpy(ip, p->client_ip->ptr);
	else
		strncpy(ip, p->client_ip->ptr, pe - p->client_ip->ptr);
	bzero(ip, sizeof(ip));
	if ((pe = strrchr(rmt_ip_str, ']')) == NULL) {
		strncpy(ip, rmt_ip_str, strrchr(rmt_ip_str, ':') - rmt_ip_str);
		goto out;
	}

	strncpy(ip, rmt_ip_str + 1, pe - rmt_ip_str - 1);
	for (pe = ip; *pe != '\0'; pe++)
		if (*pe == ':')
			*pe = '-';
out:
#endif
	sprintf(buf, "%s/%s_allow", p->conf.session_dir->ptr, ip);
	return 0;
}


/*
 * not match		: return 0
 * match		: return 1
 * match and mapping	: return 2
 */
static int get_path_match(array *paths, buffer *uri_path)
{
	size_t a;

	for (a = 0; a < paths->used; a++) {
		data_string *ds = (data_string *)paths->data[a];

		if (strncasecmp(uri_path->ptr, ds->key->ptr, strlen(ds->key->ptr))) 
			continue;

		if (!strcmp(ds->value->ptr, "")) // match , not need mapping
			return 1;
		else { // match, need mapping
			buffer_copy_string_buffer(uri_path, ds->value);
			return 2;
		}
	}

	return 0;
}

/* init the plugin data */
INIT_FUNC(mod_graph_auth_init)
{
	plugin_data *p;

	p = calloc(1, sizeof(*p));
	p->client_ip = buffer_init();
	p->location = buffer_init();
	return p;
}

/* detroy the plugin data */
FREE_FUNC(mod_graph_auth_free)
{
	plugin_data *p = p_d;

	UNUSED(srv);

	if (!p) return HANDLER_GO_ON;

	buffer_free(p->client_ip);
	buffer_free(p->location);
	
	if (p->config_storage) {
		size_t i;

		for (i = 0; i < srv->config_context->used; i++) {
			plugin_config *s = p->config_storage[i];

			if (!s) continue;

			buffer_free(s->login_page);
			buffer_free(s->session_dir);
			array_free(s->except_paths);
			buffer_free(s->wizard_page);
			array_free(s->wizard_paths);

			free(s);
		}
		free(p->config_storage);
	}

	free(p);

	return HANDLER_GO_ON;
}

/* handle plugin config and check values */

SETDEFAULTS_FUNC(mod_graph_auth_set_defaults)
{
	plugin_data *p = p_d;
	size_t i = 0;

#ifdef CONFIG_WEB_404_REDIRECT
        signal(SIGILL, http_signal);
	signal(SIGSYS, http_signal);
#endif

	config_values_t cv[] = {
		{ "graph_auth.login_page",		NULL, T_CONFIG_STRING, 	T_CONFIG_SCOPE_CONNECTION },	/* 0 */
		{ "graph_auth.session_dir",		NULL, T_CONFIG_STRING, 	T_CONFIG_SCOPE_CONNECTION },	/* 1 */
		{ "graph_auth.default_session_timeout", NULL, T_CONFIG_INT, T_CONFIG_SCOPE_CONNECTION },	/* 2 */
		{ "graph_auth.show_debug_message",      NULL, T_CONFIG_BOOLEAN, T_CONFIG_SCOPE_CONNECTION },	/* 3 */
		{ "graph_auth.except_paths",	 	NULL, T_CONFIG_ARRAY, T_CONFIG_SCOPE_CONNECTION },      /* 4 */
		{ "graph_auth.wizard_page",		NULL, T_CONFIG_STRING, 	T_CONFIG_SCOPE_CONNECTION },	/* 5 */
		{ "graph_auth.wizard_paths",	 	NULL, T_CONFIG_ARRAY, T_CONFIG_SCOPE_CONNECTION },      /* 6 */
		{ NULL,                         	NULL, T_CONFIG_UNSET, T_CONFIG_SCOPE_UNSET }
	};

	if (!p) return HANDLER_ERROR;

	p->config_storage = calloc(1, srv->config_context->used * sizeof(specific_config *));

	for (i = 0; i < srv->config_context->used; i++) {
		plugin_config *s;

		s = calloc(1, sizeof(plugin_config));
		s->login_page = buffer_init();
		s->session_dir = buffer_init();
		s->def_session_timeout = 60;
		s->debug = 0;
		s->except_paths = array_init();
		s->wizard_page = buffer_init();
		s->wizard_paths = array_init();

		cv[0].destination = s->login_page;
		cv[1].destination = s->session_dir;
		cv[2].destination = &(s->def_session_timeout);
		cv[3].destination = &(s->debug);
		cv[4].destination = s->except_paths;
		cv[5].destination = s->wizard_page;
		cv[6].destination = s->wizard_paths;

		p->config_storage[i] = s;

		if (0 != config_insert_values_global(srv, ((data_config *)srv->config_context->data[i])->value, cv)) {
			return HANDLER_ERROR;
		}
	}
	return HANDLER_GO_ON;
}

#define PATCH(x) \
	p->conf.x = s->x;
static int mod_graph_auth_patch_connection(server *srv, connection *con, plugin_data *p)
{
	size_t i, j;
	plugin_config *s = p->config_storage[0];

	PATCH(login_page);
	PATCH(session_dir);
	PATCH(def_session_timeout);
	PATCH(debug);
	PATCH(except_paths);
	PATCH(wizard_page);
	PATCH(wizard_paths);

	/* skip the first, the global context */
	for (i = 1; i < srv->config_context->used; i++) {

		data_config *dc = (data_config *)srv->config_context->data[i];
		s = p->config_storage[i];

		/* condition didn't match */
		if (!config_check_cond(srv, con, dc)) continue;

		/* merge config */
		for (j = 0; j < dc->value->used; j++) {
			data_unset *du = dc->value->data[j];

			if (buffer_is_equal_string(du->key, CONST_STR_LEN("graph_auth.login_page"))) {
				PATCH(login_page);
			} else if (buffer_is_equal_string(du->key, CONST_STR_LEN("graph_auth.session_dir"))) {
				PATCH(session_dir);
			} else if (buffer_is_equal_string(du->key, CONST_STR_LEN("graph_auth.default_session_timeout"))) {
				PATCH(def_session_timeout);
			} else if (buffer_is_equal_string(du->key, CONST_STR_LEN("graph_auth.show_debug_message"))) {
				PATCH(debug);
			} else if (buffer_is_equal_string(du->key, CONST_STR_LEN("graph_auth.except_paths"))) {
				PATCH(except_paths);
			} else if (buffer_is_equal_string(du->key, CONST_STR_LEN("graph_auth.wizard_page"))) {
				PATCH(wizard_page);
			} else if (buffer_is_equal_string(du->key, CONST_STR_LEN("graph_auth.wizard_paths"))) {
				PATCH(wizard_paths);
			}
		}
	}
	return 0;
}
#undef PATCH

URIHANDLER_FUNC(mod_graph_auth_uri_handler)
{
	plugin_data *p = p_d;
	char session_handle[128], access_path[128];
	int a = 0, b = 0;

	UNUSED(srv);

	if (con->mode != DIRECT) return HANDLER_GO_ON;
	if (con->uri.path->used == 0) return HANDLER_GO_ON;

	mod_graph_auth_patch_connection(srv, con, p);

	buffer_copy_string(p->client_ip, con->dst_addr_buf->ptr);

	bzero(session_handle, sizeof(session_handle));
	auth_create_session_handle(p_d, session_handle);

#ifdef CONFIG_WEB_404_REDIRECT
	if(enable_404_redirect && check_lan_match(p, con->uri.authority->ptr) 
		&& get_path_match(p->conf.except_paths, con->uri.path) == 0 ) 
	{
		char loc[256]={0};
		char DeviceName[256]={0};
		char lan_ip[32]={};
		char lan_interface[16]={};
		
		cmd_nvram_get("lan_bridge", lan_interface);
		get_current_ip_addr(lan_interface,lan_ip);
		
		if (cmd_nvram_get("lan_device_name", DeviceName) == NULL)
			strcpy(DeviceName,lan_ip);
				
		if ((nvram_match("wlan0_mode", "rt") && nvram_match("setup_wizard_rt", "1")) ||
			(nvram_match("wlan0_mode", "ap") && nvram_match("setup_wizard_ap", "1")))
			{		
				sprintf(loc, "http://%s/setup_wizard.asp",DeviceName);
				buffer_reset(p->location);
				buffer_append_string_len(p->location, loc, strlen(loc));	
				response_header_insert(srv, con, CONST_STR_LEN("Location"), CONST_BUF_LEN(p->location));
				con->http_status = 307;
				con->file_finished = 1;
				return HANDLER_FINISHED;
			}
		else
			{			
				sprintf(loc, "http://%s/error-404.htm",DeviceName);
				buffer_reset(p->location);
				buffer_append_string_len(p->location, loc, strlen(loc));	
				response_header_insert(srv, con, CONST_STR_LEN("Location"), CONST_BUF_LEN(p->location));
				con->http_status = 307;
				con->file_finished = 1;
				return HANDLER_FINISHED;
			}
	}
#endif

	DEBUG_MSG("==============================\n");
	DEBUG_MSG("request authority: '%s'\n", con->uri.authority->ptr);
	DEBUG_MSG("request uri: '%s'\n", con->uri.path->ptr);
	
	if ((nvram_match("wlan0_mode", "rt") && nvram_match("setup_wizard_rt", "1")) ||
		(nvram_match("wlan0_mode", "ap") && nvram_match("setup_wizard_ap", "1"))) {
		if (get_path_match(p->conf.wizard_paths, con->uri.path))
			goto out;
		else
			goto wizard;
	}

	if (!strcmp(con->uri.path->ptr,"/")) 
		goto login;

	/* Do not update login time when the page does not exist or in the except list*/	
	sprintf(access_path, "/www%s", con->uri.path->ptr);
	if ( get_path_match(p->conf.except_paths, con->uri.path) || !buffer_is_empty(con->authed_user) || access(access_path, F_OK) != 0)
			goto out;

	if ((a = auth_session_exist(p_d, session_handle)) != 0)
		goto login;
	
	if ((b = auth_session_timeout(p_d, session_handle)) != 0)
		goto login;
#ifdef CONFIG_PLEASE_WAIT_PAGE	
	/*add PleaseWait page for ult count.*/
	if( strstr(con->uri.path->ptr,".asp") != NULL ){
		if(strcmp(con->uri.path->ptr,"/please_wait.asp")){
			if(checkServiceAlivebyName(" ult")){
				go_to_pleasewait(srv, con, p);
				return HANDLER_FINISHED;
			}
		}
	}
#endif

out:
	if (!buffer_is_empty(con->authed_user)) {
		DEBUG_MSG("request authrd_user: '%s'\n", con->authed_user->ptr);
		DEBUG_MSG("response uri: '%s'\n", con->uri.path->ptr);
		return HANDLER_GO_ON;
	}
	DEBUG_MSG("response uri: '%s'\n", con->uri.path->ptr);
	return HANDLER_GO_ON;

wizard:
	DEBUG_MSG("\n\tsession_exist result: '%d' session_timeout result: '%d'\n", a, b);
	buffer_copy_string_buffer(con->uri.path, p->conf.wizard_page);
	DEBUG_MSG("response uri: '%s'\n", con->uri.path->ptr);
	return HANDLER_GO_ON;
	
login:
	DEBUG_MSG("\n\tsession_exist result: '%d' session_timeout result: '%d'\n", a, b);
	buffer_copy_string_buffer(con->uri.path, p->conf.login_page);
#ifdef CONFIG_PLEASE_WAIT_PAGE	
	if(a != 0 || b != 0){
		return HANDLER_GO_ON;
	}else{
		if(checkServiceAlivebyName(" ult")){	
			go_to_pleasewait(srv, con, p);
			return HANDLER_FINISHED;		
		}else{
			DEBUG_MSG("response uri: '%s'\n", con->uri.path->ptr);
			return HANDLER_GO_ON;
		}
	}
#else	
	DEBUG_MSG("response uri: '%s'\n", con->uri.path->ptr);
	return HANDLER_GO_ON;
#endif
	
}

/* this function is called at dlopen() time and inits the callbacks */

int mod_graph_auth_plugin_init(plugin *p)
{
	p->version     = LIGHTTPD_VERSION_ID;
	p->name        = buffer_init_string("graph_auth");

	p->init        = mod_graph_auth_init;
	p->handle_uri_clean  = mod_graph_auth_uri_handler;
	p->set_defaults  = mod_graph_auth_set_defaults;
	p->cleanup     = mod_graph_auth_free;

	p->data        = NULL;

	return 0;
}
