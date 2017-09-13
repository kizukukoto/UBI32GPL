#include <stdlib.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <netdb.h>
#include <fcntl.h>
#include <string.h>
#include <ctype.h>

#include "querys.h"
#include "ssc.h"
#include "ssi.h"
#include "log.h"
#include "libdb.h"

#if 1
#define DEBUG_MSG(fmt, arg...)       cprintf(fmt, ##arg)
#else
#define DEBUG_MSG(fmt, arg...) 
#endif

/*
 * Define keywords from HTTPD client.
 * */
#define HTML_RESPONSE_PAGE "html_response_page"
#define HTML_RETURN_PAGE "html_response_return_page"
#define SYS_SERVICE	"sys_service"	/* what services need restart? */
char *get_response_page()
{
	return get_env_value(HTML_RESPONSE_PAGE);
}

void *create_ca(struct ssc_s *obj)
{
	char *pwd, buf[256];
	char *act;
	/*
action:edit,x509_1,CCXXXVX,PKI;Issuer$ C=TW ST=Taiwan L=Taipei O=CAMEO OU=BU1 CN=Software4 emailAddress=peter_pan@cameo.com.tw;Subject$ C=TW ST=Taiwan L=Taipei O=CAMEO OU=BU1 CN=Software4 emailAddress=peter_pan@cameo.com.tw;Validity$ Mar 7 20:21:04 2007-Mar
	       
	*/
	cprintf("x509_action:%s\n", get_env_value("x509_action"));
	cprintf("certi_name:%s\n", get_env_value("self_ca_name"));//get_env_value("certi_name"));
	cprintf("cu_name: %s\n", get_env_value("cu_name"));
	cprintf("city_name: %s\n", get_env_value("city_name"));
	cprintf("og_name: %s\n", get_env_value("og_name"));
	cprintf("ou_name: %s\n", get_env_value("ou_name"));
	cprintf("cn_name: %s\n", get_env_value("cn_name"));
	cprintf("com_email_addre: %s\n", get_env_value("com_email_addre"));
	cprintf("keylen:%s\n", get_env_value("keylen"));
	cprintf("valid_period%s\n", get_env_value("valid_period"));

	if ((act = get_env_value("x509_action")) == NULL)
		goto err1;
	if ((act = strdup(act)) == NULL)
		goto err1;
	if (strncasecmp(act, "edit", 4) == 0) {
		char *s, *k;
		
		s = act;
		strsep(&s, ",");
		if (!s)
			goto free_out;
		k = strsep(&s, ",");
		nvram_set(k, s);
		nvram_commit();
		goto free_out;
	}
	if (strncasecmp(act, "create", 6) == 0) {
//action:edit,x509_1,CCXXXVX,PKI;Issuer$ C=TW ST=Taiwan L=Taipei O=CAMEO OU=BU1 CN=Software4 emailAddress=peter_pan@cameo.com.tw;Subject$ C=TW ST=Taiwan L=Taipei O=CAMEO OU=BU1 CN=Software4 emailAddress=peter_pan@cameo.com.tw;Validity$ Mar 7 20:21:04 2007-Mar
		cprintf("CREATE SELF CERT\n");
		setenv("X509_NAME", get_env_value("self_ca_name"), 1);//get_env_value("certi_name"), 1);
		setenv("REQ_C", get_env_value("cu_name"), 1);
		setenv("REQ_ST", get_env_value("city_name"),1);
		setenv("REQ_L", get_env_value("cu_name"), 1);
		setenv("REQ_O", get_env_value("og_name"), 1);
		setenv("REQ_OU", get_env_value("ou_name"), 1);
		setenv("REQ_CN", get_env_value("cn_name"), 1);
		setenv("REQ_EMAIL", get_env_value("com_email_addre"), 1);
		setenv("REQ_KEYLEN", get_env_value("keylen"), 1);
		setenv("REQ_DAYS", get_env_value("valid_period"), 1);
		
		setenv("OPENSSL_CONF", "/etc/ca/pem.info", 1);

		pwd = getcwd(buf, sizeof(buf));
		chdir("/tmp/ca");
		system("./create_ca.sh");
		chdir(pwd);
	}
free_out:
	free(act);
err1:
	return get_response_page();
}

/*
 * Those interfaces is NOT useful now(Eagle, DIRx30)
 * this is just used for obsolete DIR730...
 * XXX: maybe them will be removed in the future
 * I rename staytsm_start|stop to services_start|stop
 * and only export updown_services() to wrap them.
 * */
static inline void services_stop(const char *service)
{
	pid_t pid;
	char cmds[1024] = "/etc/init.d/S40network stop >/dev/null 2>&1";
	
	if (service == NULL || strlen(service) == 0) {
		cprintf("CGI system_stop: %s\n", cmds);
		system(cmds);
	} else {
		char _service[1024], *sptr, *ptr;

		strcpy(_service, service);
		sptr = _service;
		
		while ((ptr = strsep(&sptr, ",")) != NULL) {
			sprintf(cmds, "cli services %s stop >/dev/null 2>&1", ptr);
			
			pid = fork();
		
			switch (pid) {
			case -1:
				return;
			case 0:
				sleep(3);
				cprintf("CGI system_stop: %s\n", cmds);
				system(cmds);
				//system("cli service syslogd start");
				
				exit(0);
			}
		}
	}
}
static inline void services_start(const char *service)
{
	pid_t pid;
	char cmds[1024] = "/etc/init.d/S40network start >/dev/null 2>&1";
	 
	if (service == NULL || strlen(service) == 0) {
		cprintf("CGI system_start: %s\n", cmds);
		system(cmds);
	} else {
		char *ptr,*sptr,_service[1024];
		
		strcpy(_service, service);
		sptr = _service;

		while ((ptr = strsep(&sptr, ",")) != NULL) {
			sprintf(cmds, "cli services %s start >/dev/null 2>&1", ptr);
			
			pid = fork();
		
			switch (pid) {
			case -1:
				return;
			case 0:
				sleep(3);
				cprintf("CGI system_start: %s\n", cmds);
				system(cmds);
				//system("cli service syslogd start");
				
				exit(0);
			}
		}
	}
}

void updown_services(int up, const char *service)
{
	up ? services_start(service):services_stop(service);
}

//Albert add 
extern struct items wireless_0[];
extern struct items wireless_1[];

void *restore_default_wireless(struct ssc_s *obj)
{
	struct items *it;
	char *s;
	char *value;

	DEBUG_MSG("do apply cgi:opt->action=%s\n",obj->action);
	cprintf("do restore_default_wireless cgi:opt->action=%s\n",obj->action);
	SYSLOG("DO_APPLY_CGI");
#if NOMMU
	if (vfork() == 0) {
#else
	if (fork() == 0) {
#endif
		//updown_services(0, NULL);
		for (it = &wireless_0[0]; it != NULL && it->key != NULL; it++) {
			int ret = -1;
			DEBUG_MSG("do apply cgi: it->key=%s\n",it->key);
			cprintf("do apply cgi: it->key=%s\n",it->key);
			s = get_env_value(it->key);
			
			value = it->_default;

			ret = update_record(it->key, value);
			DEBUG_MSG("do apply cgi: Update %s: %s, %d\n", it->key, value, ret);
			cprintf("do apply cgi: Update %s: %s, %d\n", it->key, value, ret);
			SYSLOG("Update %s: %s, %d\n", it->key, value, ret);
		}
		for (it = &wireless_1[0]; it != NULL && it->key != NULL; it++) {
			int ret = -1;
			DEBUG_MSG("do apply cgi: it->key=%s\n",it->key);
			cprintf("do apply cgi: it->key=%s\n",it->key);
			s = get_env_value(it->key);
			
			value = it->_default;

			ret = update_record(it->key, value);
			DEBUG_MSG("do apply cgi: Update %s: %s, %d\n", it->key, value, ret);
			cprintf("do apply cgi: Update %s: %s, %d\n", it->key, value, ret);
			SYSLOG("Update %s: %s, %d\n", it->key, value, ret);
		}
		/*
		if (obj->shell) {
			//fire_event(obj->shell);
		}
		*/
		nvram_commit();
		/* XXX: Do we sure every time is kill rc? And Kill rc need fork? 
		 * May be need sleep 1 sec, but sleep on rc is more better
		 */
		DEBUG_MSG("debug message\n");
		//__do_apply(obj);
		//updown_services(1, NULL);
		exit(0);
	}
	do_apply_cgi(obj);
}

void *_connect(struct ssc_s *obj) 
{
  	char cmd[128];

	sprintf(cmd, "cli net ii start %s manual > /dev/null 2>&1", get_env_value("wan_type"));
	system(cmd);
	return get_response_page();
}

void *_disconnect(struct ssc_s *obj)
{
	char cmd[128];
	
	sprintf(cmd, "cli net ii stop %s > /dev/null 2>&1", get_env_value("wan_type"));
	system(cmd);
	return get_response_page();
}

void *_rconnect(struct ssc_s *obj) 
{
	char cmd[128];

	sprintf(cmd, "cli net ii stop %s > /dev/null 2>&1", get_env_value("wan_type"));
	system(cmd);
	sleep(1);

	bzero(cmd, sizeof(cmd));
	sprintf(cmd, "cli net ii start %s manual > /dev/null 2>&1", get_env_value("wan_type"));
	system(cmd);
	return get_response_page();
}

void *do_get_trx_cgi(struct ssc_s *obj)
{
	system("/bin/get_trx lan set_trx");
	system("/bin/get_trx wan set_trx");
	system("/bin/get_trx dmz set_trx");
	system("/bin/get_trx ra0 set_trx");

	return get_response_page();
}

static void auth_graph_toupper(const char *gcode, char *buf)
{
	int i = 0;

	if (gcode == NULL || *gcode == '\0')
		return;

	for (; i < strlen(gcode); i++) {
		if (islower(gcode[i]))
			buf[i] = toupper(gcode[i]);
		else
			buf[i] = gcode[i];
	}
	buf[i] = '\0';
}

#define GRAPH_AUTH_TEMPDIR      "/tmp/graph"

static int user_exist_type(
	const char *cfg,
	const char *user,
	const char *pass)
{
	FILE *fp;
	int ret = 0;
	char t[128], dir[64], uid[64], pwd[64];

	if ((fp = fopen(cfg, "r")) == NULL)
		goto fail;

	bzero(t, sizeof(t));
	DEBUG_MSG("XXX user_exist_type %s user [%s] pass [%s]\n", cfg, user, pass);
	while (fscanf(fp, "%s", t) != EOF) {
		/* ignore the first line 'A:192.168.0.1/24' */
		if (*t == 'A') continue;

		bzero(dir, sizeof(dir));
		bzero(uid, sizeof(uid));
		bzero(pwd, sizeof(pwd));

		sscanf(t, "%[^:]:%[^:]:%s", dir, uid, pwd);
		DEBUG_MSG("XXX uid [%s] <==> user [%s], pwd [%s] <==> pass [%s]\n",
			   uid, user, pwd, pass);
		if (strcmp(user, uid) == 0 && strcmp(pass, pwd) == 0)
			goto out;
		bzero(t, sizeof(t));
	}
fail:
	ret = -1;
out:
	if (fp) fclose(fp);
	return ret;
}

static int http_user_exist(const char *user, const char *pass)
{
	return user_exist_type("/tmp/password", user, pass);
}

static int sslvpn_user_exist(const char *user, const char *pass)
{
	int in_https = user_exist_type("/tmp/https.conf", user, pass);
	int in_ssl = user_exist_type("/tmp/ssl.users", user, pass);

	return (in_https == 0 && in_ssl == 0)?0:-1;
}

static int https_user_exist(const char *user, const char *pass)
{
	return user_exist_type("/tmp/https.conf", user, pass);
}
