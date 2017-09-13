# include <stdio.h>
#include <sys/types.h>

#include "libdb.h"
#include "ssc.h"
#include "public.h"
#include "querys.h"
/* jimmy added 20081027 */
#include <syslog.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <netdb.h>
/* ---------------------- */

#include "restore_default.h"

extern struct nvram_ops *nvram;
extern struct method_plugin *ssc_post_method_plugin, *ssi_get_method_plugin;
extern struct ssc_s *SSC_OBJS;

extern void *do_apply_cgi(struct ssc_s *obj);
extern void *do_apply_cgi_range(struct ssc_s *obj);
extern void *do_wireless_cgi(struct ssc_s *obj);
extern void *do_wps_action();
extern inline void __do_apply(struct ssc_s *obj);
extern char *get_response_page();
extern void *create_ca(struct ssc_s *obj);
//Albert add 
extern void *restore_default_wireless(struct ssc_s *obj);
extern void *do_restore_defaults(struct ssc_s *obj);
extern void *do_timeset(struct ssc_s *obj);

extern void *_connect(struct ssc_s *obj);
extern void *_disconnect(struct ssc_s *obj);
extern void *_rconnect(struct ssc_s *obj);
extern void *do_get_trx_cgi(struct ssc_s *obj);
extern void *do_graph_auth(struct ssc_s *obj);	/* graph.c */
extern void *do_wan_connect(struct ssc_s *obj);
extern void *do_wireless_wizard(struct ssc_s *obj);

extern void do_lan_revoke(const char *revoke_ip, const char *revoke_mac);

extern void __do_reboot(struct ssc_s *obj);
extern void do_reboot(struct ssc_s *obj);

/* wps */
extern char do_sta_enrollee_cgi(struct ssc_s *obj);
extern char do_sta_pbc_cgi(struct ssc_s *obj);

extern char do_allow_reject_page(struct ssc_s *obj);
extern void wizard_ipv6(struct ssc_s *obj);

#if 0
#define DEBUG_MSG(fmt, arg...)       cprintf(fmt, ##arg)
#else
#define DEBUG_MSG(fmt, arg...) 
#endif

int nvram_register(struct nvram_ops *v)
{
	if (nvram != NULL)
		return -1;
	nvram = v;
	return 0;
}

int ssc_action_register(struct ssc_s *ss)
{
	if (SSC_OBJS != NULL)
		return -1;
	SSC_OBJS = ss;
	return 0;
}

int ssc_post_method_register(struct method_plugin *m)
{
	if (m == NULL)
		return -1;
	ssc_post_method_plugin = m;
	return 0;
}

int ssi_get_method_register(struct method_plugin *m)
{
	if (m == NULL)
		return -1;
	ssi_get_method_plugin = m;
	return 0;

}

int start_cgi(int argc, char *argv[]){
	return -1;
}

extern char *do_log_first_page_cgi();
extern char *do_log_last_page_cgi();
extern char *do_log_next_page_cgi();
extern char *do_log_previous_page_cgi();
extern char *do_log_clear_page_cgi();
extern void *do_save_log_cgi();
extern char *do_send_log_email_cgi();

extern int *save_log();
extern char *save_conf();
extern char *upload_conf();
extern char *upload_lang();
extern char *auto_upload_lang(char *file); // Matt add - 2010/11/10
extern char *clear_lang();
extern char *update_firmware();
extern char *auto_update_firmware(char *file); // Matt add - 2010/11/09
extern char *online_firmware_check();
//extern char *update_ips();
extern char *upload_ca();
extern char *do_hnap_post();
extern char *do_hnap_get();
extern char *do_save_cert();
/*
 * A special HOOKER in POST method.
 * It is used to firmware upgrade, certicates upload, upload config
 * and something else ...
 * */
static struct method_plugin post_plugin[] = {
	{"/update_firmw.cgi", update_firmware },
//	{"/update_ips.cgi", update_ips},
	{"/upload_certificate.cgi", upload_ca },
	{"/upload_configure.cgi", upload_conf },
	{"/upload_lang.cgi", upload_lang },
	{"/lp_clear.cgi", clear_lang },
	{"/online_firmware_check.cgi", online_firmware_check },
	{"/hnap.cgi", do_hnap_post },
	
	//sys log
	{"/log_first_page.cgi", do_log_first_page_cgi },
        {"/log_last_page.cgi", do_log_last_page_cgi },
        {"/log_next_page.cgi", do_log_next_page_cgi },
        {"/log_previous_page.cgi", do_log_previous_page_cgi },
        {"/log_clear_page.cgi", do_log_clear_page_cgi },
        {"/save_log_page.cgi", do_save_log_cgi },
        {"/send_log_email.cgi", do_send_log_email_cgi },

	{"", NULL}
};

static struct method_plugin method_get_virtual_files[] = {
	{"/save_configure.cgi", save_conf},
	{"/save_log.cgi", save_log}, /* virtual_file.c */
	{"/hnap.cgi", do_hnap_get},  /* ext/hnap_get.c */
	{"/save_certificate.cgi", do_save_cert},
	{"", NULL}
};

static void *do_lan(struct ssc_s *obj)
{
	system("/bin/rm /tmp/dnsmasq.lease");
	sleep(1);
	return do_apply_cgi(obj);
}

static void *do_revoke(struct ssc_s *obj)
{
        char *revoke_ip = get_env_value("revoke_ip");
        char *revoke_mac = get_env_value("revoke_mac");

        do_lan_revoke(revoke_ip, revoke_mac);
        sleep(1);
        return do_apply_cgi(obj);
}

static void *do_dmz(struct ssc_s *obj)
{
	//system("/bin/rm /tmp/dnsmasq.lease");
	sleep(1);
	return do_apply_cgi_range(obj);
}

static int check_service(char *service)
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

/* XXX
Joe Huang : For avoid cgi would got stuck because ping can not finish.
	    Check the ping procss is alive or not, 
	    and send signal to finish ping after 2 sec.
	    Both ping and ping6 are use this procedures
*/
static void *do_ping_test(struct ssc_s *obj)
{
	FILE *fp;
	char tmp[256];
	char *addr = get_env_value("ping_ipaddr");
	int i = 2;
        char *relay_ptr;
        struct in_addr v4addr_in;
        char ip[200] = {};

	if (inet_aton(addr, &v4addr_in))//Joe Huang : Check the address is domain name or not
	        strcpy(ip, addr);
	else{
        	struct hostent *ip_h = NULL;
	        if (ip_h = gethostbyname(addr))
        	        strcpy(ip, inet_ntoa(*(struct in_addr *)(ip_h->h_addr)));
		else{
			sprintf(tmp, "ping: %s Unable to resolve, "
				"check that the name is correct", addr);
			setenv("ping_result", tmp, 1);
			goto err;
		}
	}

	sprintf(tmp, "ping %s -c 1 -w 1 2>&1", ip);

	if ((fp = popen(tmp, "r")) == NULL) {
		perror("popen ping");
		setenv("ping_result", "internal ping error",1);
		goto err;
	}

	while ((!feof(fp)) && i--) {
		sleep(1);
		if(check_service("ping") == 0)
			fgets(tmp, sizeof(tmp), fp);
		else if(i == 0)
			system("killall -SIGINT ping");
	}

	if (strstr(tmp, "from")) {
		sprintf(tmp, "ping: %s is alive", addr);
	} else if (strstr(tmp, "bad address")) {
		sprintf(tmp, "ping: %s Unable to resolve, "
			"check that the name is correct", addr);
	} else {
		sprintf(tmp, "ping: %s No response from host", addr);
	}
	setenv("ping_result", tmp, 1);
	pclose(fp);
err:
	return get_response_page();
}

static void *do_ping6_test(struct ssc_s *obj)
{
        FILE *fp;
        char tmp[256];
        char *addr = get_env_value("ping6_ipaddr");
        int i = 2;
        char *relay_ptr;
        struct in6_addr v6addr_in;
        char ip[200] = {};

	if (inet_pton(AF_INET6, addr, &v6addr_in)) //Joe Huang : Check the address is domain name or not
		strcpy(ip, addr);
	else{
		struct hostent *ip_h = NULL;
		if (ip_h = gethostbyname2(addr, AF_INET6))
			inet_ntop(AF_INET6, *(struct in6_addr *)(ip_h->h_addr), ip, sizeof(ip));
		else{
			sprintf(tmp, "ping6: %s Unable to resolve, "
				"check that the name is correct", addr);
			setenv("ping6_result", tmp, 1);
			goto err;
		}
	}

        sprintf(tmp, "ping6 %s -c 1 2>&1", ip);
        if ((fp = popen(tmp, "r")) == NULL) {
                perror("popen ping6");
                setenv("ping6_result", "internal ping6 error",1);
                goto err;
        }

        while ((!feof(fp)) && i--) {
		sleep(1);
		if(check_service("ping6") == 0)
			fgets(tmp, sizeof(tmp), fp);
		else if(i == 0)
			system("killall -SIGINT ping6");
        }

        if (strstr(tmp, "from")) {
                sprintf(tmp, "ping6: %s is alive", addr);
        } else if (strstr(tmp, "bad address")) {
                sprintf(tmp, "ping6: %s Unable to resolve, "
                        "check that the name is correct", addr);
        } else {
                sprintf(tmp, "ping6: %s No response from host", addr);
        }
        setenv("ping6_result", tmp, 1);
        pclose(fp);
err:
        return get_response_page();
}

static void *do_opendns(struct ssc_s *obj)
{
	if (!NVRAM_MATCH("en_opendns", "0")) {
		nvram_set("wan_primary_dns", "0.0.0.0");
		nvram_set("wan_secondary_dns", "0.0.0.0");
	} else {
		nvram_set("wan_primary_dns", "208.67.222.222");
		nvram_set("wan_secondary_dns", "208.67.220.220");
	}
	nvram_commit();
	return do_apply_cgi(obj);
}

static int *do_clean_statistics(struct ssc_s *obj)
{
	system("/bin/statistics clean");
	return get_response_page();
}

/* jimmy added 20081027 , send mail */
#define SMTP_CONF           "/var/etc/smtp.conf"

/* create smtp config */
#if 0
int init_smtp_conf(void){
	FILE *fp;
	int ret = 0;
	if((fp = fopen (SMTP_CONF,"w"))!=NULL){
		fprintf(fp,"log_email_auth=%s\n",nvram_safe_get("auth_active"));
		fprintf(fp,"log_email_recipient=%s\n",nvram_safe_get("log_email_recipient"));  // <== ??
		fprintf(fp,"log_email_username=%s\n",nvram_safe_get("auth_acname"));
		fprintf(fp,"log_email_password=%s\n", nvram_safe_get("auth_passwd"));
		fprintf(fp,"log_email_server=%s\n",nvram_safe_get("log_email_server"));
		fprintf(fp,"log_email_sender=%s\n", nvram_safe_get("log_email_sender"));
		fprintf(fp,"log_email_server_port=%s\n",strlen(nvram_safe_get("log_email_server_port")) > 0 ? nvram_safe_get("log_email_server_port") : "25");
		fclose(fp);
		ret = 1;
	}else{
		DEBUG_MSG("%s, can not write msmtp config %s !\n",__FUNCTION__,SMTP_CONF);
		syslog(LOG_ERR,"can not write msmtp config %s !\n",__FUNCTION__,SMTP_CONF);
	}
	return ret;
}
#endif
/* --------------------- */

#if 0	/* for DIR-730 */
void *do_send_mail(struct ssc_s *obj){
	struct items *it;
	char *s;
	char *value;
	char buffer[328];
	
	DEBUG_MSG("%s:opt->action=%s\n",__FUNCTION__,obj->action);
	if (fork() == 0) {
		for (it = obj->data; it != NULL && it->key != NULL; it++) {
			int ret = -1;
			DEBUG_MSG("do_send_mail: it->key=%s\n",it->key);
			reset_record();
			s = get_env_value(it->key);
			if (s == NULL) {
				if (it->_default == NULL)
					continue;
				value = it->_default;
			} else {
				value = s;
			}

			ret = update_record(it->key, value);
			DEBUG_MSG("%s: Update %s: %s, %d\n",__FUNCTION__, it->key, value, ret);
			syslog(LOG_INFO,"Update %s: %s, %d\n", it->key, value, ret);
		}
		/*
		if (obj->shell) {
			//fire_event(obj->shell);
		}
		*/
		save_db();
		/* XXX: Do we sure every time is kill rc? And Kill rc need fork? 
		 * May be need sleep 1 sec, but sleep on rc is more better
		 */
		DEBUG_MSG("debug message\n");
		//__do_apply(obj);

		memset (buffer,'\0',sizeof(buffer));

		if (nvram_match("auth_active", "1"))
		{
			DEBUG_MSG("%s, auth is enable\n",__FUNCTION__);
			sprintf(buffer,"msmtp %s --auth=  --user=%s  --passwd %s --host=%s --from=%s --port=%s > /dev/null 2>&1 &",
					nvram_safe_get("log_email_recipient"),
					nvram_safe_get("auth_acname"),
					nvram_safe_get("auth_passwd"),
					nvram_safe_get("log_email_server"),
					nvram_safe_get("log_email_sender"),
					nvram_safe_get("log_email_port"));

			DEBUG_MSG("%s\n",buffer);
			system(buffer);
			//system("msmtp %s --auth=  --user=%s  --passwd %s --host=%s --from=%s --port=%s &", 
			//		nvram_safe_get("log_email_recipient"), nvram_safe_get("auth_acname")
			//		, nvram_safe_get("auth_passwd"), nvram_safe_get("log_email_server")
			//		, nvram_safe_get("log_email_sender")
			//		, strlen(nvram_safe_get("log_email_server_port")) > 0 ? nvram_safe_get("log_email_server_port") : "25"
			//		);
		}			
		else
		{
			DEBUG_MSG("%s, auth is disable\n",__FUNCTION__);
			sprintf(buffer,"msmtp %s --host=%s --from=%s --port=%s > /dev/null 2>&1 &"
				,nvram_safe_get("log_email_recipient")
				,nvram_safe_get("log_email_server")
				,nvram_safe_get("log_email_sender")
				,strlen(nvram_safe_get("log_email_server_port")) > 0 ? nvram_safe_get("log_email_server_port") : "25"
				);
			DEBUG_MSG("%s\n",buffer);
			system(buffer);
			//system("msmtp %s --host=%s --from=%s --port=%s &", 
			//		log_email_recipien, log_email_server, log_email_sender, log_email_server_port);			
		}
		//init_smtp_conf();
		exit(0);
	}
	return get_response_page();
}
#else
static void *do_send_mail(struct ssc_s *obj)
{
	char server[32], sender[128], model_NO[16];
	char string_buf[256];
	char acname[128], passwd[128];
	char from[128];
	char smtp_port[128];
	char auth_active;

	query_vars("auth_active", &auth_active, sizeof(auth_active));
	query_vars("log_email_from", from, sizeof(from));
	query_vars("auth_acname", acname, sizeof(acname));
	query_vars("auth_passwd", passwd, sizeof(passwd));
	query_vars("log_email_server", server, sizeof(server));
	query_vars("log_email_port", smtp_port, sizeof(smtp_port));
	query_vars("log_email_sender", sender, sizeof(sender));
	query_vars("model_name", model_NO, sizeof(model_NO));
	system("/bin/log_page file");

	if (*smtp_port == '\0')
		strcpy(smtp_port, "25");

	sprintf(string_buf, "/bin/mailto -s \'Log Manual (from %s)\' -S %s -e %s -f /tmp/logfile -p %s",
		model_NO, server, sender, smtp_port);

	if(auth_active == '1')
		sprintf(string_buf, "%s -F %s -A %s -P %s", string_buf, from, acname, passwd);

	system(string_buf);

	return get_response_page();
}
#endif

void *do_tools_email(struct ssc_s *obj){
	struct items *it;
	char *s;
	char *value;

		
	DEBUG_MSG("%s:opt->action=%s\n",__FUNCTION__,obj->action);
	cprintf("%s:opt->action=%s\n",__FUNCTION__,obj->action);
#if NOMMU
	if (vfork() == 0) {
#else
	if (fork() == 0) {
#endif
		for (it = obj->data; it != NULL && it->key != NULL; it++) {
			int ret = -1;
			DEBUG_MSG("%s: it->key=%s\n",__FUNCTION__,it->key);
			s = get_env_value(it->key);
			if (s == NULL) {
				if (it->_default == NULL)
					continue;
				value = it->_default;
			} else {
				value = s;
			}

			ret = update_record(it->key, value);
			DEBUG_MSG("%s: Update %s: %s, %d\n",__FUNCTION__, it->key, value, ret);
			cprintf("%s: Update %s: %s, %d\n",__FUNCTION__, it->key, value, ret);
			syslog(LOG_INFO,"Update %s: %s, %d\n", it->key, value, ret);
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
		__do_apply(obj);
		//init_smtp_conf();
		exit(0);
	}
	return get_response_page();
}

static void set_ipsec_wizard()
{
	int i = 0;
	char strKey[32], *strTmp;
	char fwKey[32], fwstr[512];

	cprintf("do_wizard_vpn:IPSec\n");
	for(i=1; i<26; i++){
		sprintf(strKey, "vpn_conn%d", i);
		sprintf(fwKey, "fw_vpn_conn%d", i);

		strTmp = nvram_safe_get(strKey);

		if(strlen(strTmp)==0){
			strTmp = get_env_value("vpn_ipsec");
			nvram_set(strKey, strTmp);

			sprintf(strKey, "vpn_extra%d", i);
			strTmp = "pfsgroup=modp1024,ikelifetime=28800s,keylife=3600s,#default,#leftid=,#default,#rightid=,dpdaction=clear,dpdtimeout=120s";
			nvram_set(strKey, strTmp);
			nvram_set("vpn_nat_traversal", "1");
			/* set fw rule
			 * rule example :
			 * fw_vpn_conn1=1;ok_vpn;192.168.3.0/24:IPSec>192.168.0.0/24:ANY;all;0-65535;1;Always
			 */
			sprintf(fwstr, "1;%s;%s:IPSec>%s:ANY;all;0-65535;1;Always",
				get_env_value("vpn_profile_name"),
				get_env_value("vpn_ipsec_remotenet"),
				get_env_value("vpn_ipsec_net")
			);
			nvram_set(fwKey, fwstr);
			nvram_commit();
			break;
		}
	}
}

static void set_pptp_wizard()
{
	int i = 0;
	char strKey[32], *strTmp;
	char tmp_g[12], group[12];

	cprintf("do_wizard_vpn:PPTP\n");
	for(i=1; i<26; i++){
		sprintf(strKey, "pptpd_conn_%d", i);
		strTmp = nvram_get(strKey);
		if(strlen(strTmp)==0){
			strTmp = get_env_value("vpn_pptp");
			nvram_set(strKey, strTmp);

			sprintf(strTmp, "%d", i);
			nvram_set("pptpd_enable", strTmp);

			nvram_set("pptpd_servername", get_env_value("vpn_profile_name"));

			nvram_set("pptpd_local_ip", get_env_value("vpn_pptp_serverip"));

			strcpy(strKey, "pptpd_remote_ip");
			sprintf(strTmp, "%s-%s", get_env_value("vpn_pptp_iprange"), get_env_value("vpn_pptp_ipend"));
			nvram_set(strKey, strTmp);

			nvram_set("auth_pptpd_l2tpd", get_env_value("vpn_pptp_group"));
/* set Group */
			sprintf(tmp_g, "echo_group%s", get_env_value("vpn_pptp_group"));
			sprintf(group, "auth_group%s", get_env_value("vpn_pptp_group"));
			nvram_set(group, get_env_value(tmp_g));

			nvram_set("pptpd_auth_proto", "mschap-v2");
			nvram_set("pptpd_encryption", "mppe-128");
			nvram_set("pptpd_auth_source", "local");

			break;
		}
	}
}

static void set_l2tp_wizard()
{
	int i = 0;
	char strKey[32], *strTmp;
	char tmp_g[12], group[12];

	cprintf("do_wizard_vpn:L2TP\n");
	for(i=1; i<26; i++){
		sprintf(strKey, "l2tpd_conn_%d", i);
		strTmp = nvram_safe_get(strKey);
		if(strlen(strTmp)==0){
			strTmp = get_env_value("vpn_l2tp");
			nvram_set(strKey, strTmp);

			sprintf(strTmp, "%d", i);
			nvram_set("l2tpd_enable", strTmp);

			nvram_set("l2tpd_servername", get_env_value("vpn_profile_name"));
			nvram_set("l2tpd_local_ip", get_env_value("vpn_l2tp_serverip"));

			strcpy(strKey, "l2tpd_remote_ip");
			sprintf(strTmp, "%s-%s", get_env_value("vpn_l2tp_iprange"), get_env_value("vpn_l2tp_ipend"));
			nvram_set(strKey, strTmp);
/* set Group */
			sprintf(tmp_g, "echo_group%s", get_env_value("vpn_l2tp_group"));
			sprintf(group, "auth_group%s", get_env_value("vpn_l2tp_group"));
			nvram_set(group, get_env_value(tmp_g));

			nvram_set("auth_pptpd_l2tpd", get_env_value("vpn_l2tp_group"));
			nvram_set("l2tpd_auth_proto", "mschap-v2");
			nvram_set("l2tpd_auth_source", "local");

			break;
		}
	}
}

static void set_sslvpn_wizard()
{
	int i = 0;
	char strKey[32], *strTmp;
	char tmp_g[12], group[12];

	cprintf("do_wizard_vpn:SSL VPN\n");
	for(i=1; i<7; i++){
		memset(strKey, '\0', sizeof(strKey));
		memset(tmp_g, '\0', sizeof(tmp_g));
		sprintf(strKey, "sslvpn%d", i);
		strTmp = nvram_safe_get(strKey);
		if(strlen(strTmp)==0){
			strTmp = get_env_value("vpn_ssl");
			nvram_set(strKey, strTmp);

			strTmp = get_env_value("vpn_ssl_ca");
			sprintf(strKey, "x509_%s", strTmp);
			nvram_set("https_pem", strKey);

			sprintf(tmp_g, "echo_group%s", get_env_value("vpn_ssl_group"));
			sprintf(group, "auth_group%s", get_env_value("vpn_ssl_group"));
			nvram_set(group, get_env_value(tmp_g));

			break;
		}
	}
}

static void *do_wizard_vpn(struct ssc_s *obj)
{
	cprintf("\n***do_wizard_vpn:%s***\n", get_env_value("vpn_type"));
	char *vpn_type = get_env_value("vpn_type");

	if(strcmp(vpn_type, "ipsec")==0){
		set_ipsec_wizard();
	} else if(strcmp(vpn_type, "pptp")==0){
		set_pptp_wizard();
	} else if(strcmp(vpn_type, "l2tp")==0){
		set_l2tp_wizard();
	} else if(strcmp(vpn_type, "sslvpn")==0){
		set_sslvpn_wizard();
	}

	return do_apply_cgi(obj);
}

static char *do_vpn_ipsec(struct ssc_s *obj)
{
	int i;
	char buf[256], buf1[256], tmp[128], key[16];
	char *action, *name, *conn, *local, *remote;
#define VPN_CONN_MAX    25
	for(i = 1; i <= VPN_CONN_MAX; i++) {
		sprintf(tmp, "vpn_conn%d", i);
		query_vars(tmp, buf, sizeof(buf));
		query_record(tmp, buf1, sizeof(buf1));
		if(strcmp(buf, "") == 0 || strcmp(buf, buf1) == 0) {
			if(strcmp(buf, "") == 0) {
				sprintf(key, "fw_vpn_conn%d", i - 1);
				update_record(key, "");
			}
			continue;
		}
		tmp[0] = '\0';
		action = strtok(buf, ";");
		if(strcmp(action, "ignored") == 0)
			strcat(tmp, "0;");
		else
			strcat(tmp, "1;");
		name = strtok(NULL, ";");
		strcat(tmp, name);
		strcat(tmp, ";");
		conn = strtok(NULL, ";");
		sprintf(buf, "%s", conn);
		conn = strchr(buf, ',');
		conn++;
		sprintf(buf, "%s", conn);
		conn = strchr(buf, ',');
		conn++;
		sprintf(buf, "%s", conn);
		local = strtok(buf, "-");
		remote = strtok(NULL, "-");
		if(strcmp(remote, "255.255.255.255/32") == 0) {
			strcat(tmp, "0.0.0.0");
		} else
			strcat(tmp, remote);

		strcat(tmp, ":IPSec>");
		strcat(tmp, local);
		strcat(tmp, ":ANY;all;0-65535;1;Always");
		sprintf(key, "fw_vpn_conn%d", i - 1);
		update_record(key, tmp);
	}
	return do_apply_cgi(obj);
}

/* ------------------------------------- */

#include "asp/wan.h"
#include "asp/lan_dmz.h"
#include "asp/advances.h"
#include "asp/wireless.h"
#include "asp/usb.h"
#include "asp/tools.h"
#include "asp/vpn.h"

static void set_wizard_wan_dhcp()
{
	nvram_set("dhcpc_mac2", get_env_value("dhcpc_mac2"));
	nvram_set("dhcpc_hostname2", get_env_value("dhcpc_hostname2"));
	
	nvram_set("dhcpc_mtu2", "1500");
	nvram_set("zone_if_list2", "WAN_primary");
	nvram_set("zone_if_proto2", "dhcpc");
	nvram_set("if_proto2", "dhcpc");
	nvram_set("if_dev2", "eth1");
	nvram_set("if_proto_alias2", "WAN_dhcpc");
	nvram_set("if_services2", PROTO_SINGLE_SERVICES);
}

static void set_wizard_wan_static()
{
	nvram_set("static_addr2", get_env_value("static_addr2"));
	nvram_set("static_netmask2", get_env_value("static_netmask2"));
	nvram_set("static_gateway2", get_env_value("static_gateway2"));
	nvram_set("static_primary_dns2", get_env_value("static_primary_dns2"));
	nvram_set("static_secondary_dns2", get_env_value("static_secondary_dns2"));
	
	nvram_set("static_mtu2", "1500");
	nvram_set("zone_if_proto2", "static");
	nvram_set("zone_if_list2", "WAN_primary");
	nvram_set("if_dev2", "eth1");
	nvram_set("if_proto2", "static");
	nvram_set("if_proto_alias2", "WAN_static");
	nvram_set("if_mode2", "nat");
	nvram_set("if_services2", PROTO_SINGLE_SERVICES);
}

static void set_wizard_wan_pppoe()
{
	nvram_set("pppoe_mode2", get_env_value("pppoe_mode2"));
	nvram_set("pppoe_user2", get_env_value("pppoe_user2"));
	nvram_set("pppoe_pass2", get_env_value("pppoe_pass2"));
	nvram_set("pppoe_servicename2", get_env_value("pppoe_servicename2"));
	nvram_set("pppoe_addr2", get_env_value("pppoe_addr2"));
	
	nvram_set("pppoe_idle2", "300");
	nvram_set("pppoe_mtu2", "1492");
	nvram_set("pppoe_dial2", "1");
	nvram_set("zone_if_proto2", "pppoe");
	nvram_set("zone_if_list2", "WAN_primary");
	nvram_set("if_proto2", "pppoe");
	nvram_set("if_proto_alias2", "WAN_pppoe");
	nvram_set("if_dev2", "ppp0");
	nvram_set("if_services2", PROTO_SINGLE_SERVICES);
}

static void set_wizard_wan_pptp()
{
	nvram_set("pptp_mode2", get_env_value("pptp_mode2"));
	nvram_set("pptp_addr2", get_env_value("pptp_addr2"));
	nvram_set("pptp_netmask2", get_env_value("pptp_netmask2"));
	nvram_set("pptp_gateway2", get_env_value("pptp_gateway2"));
	nvram_set("pptp_serverip2", get_env_value("pptp_serverip2"));
	nvram_set("pptp_user2", get_env_value("pptp_user2"));
	nvram_set("pptp_pass2", get_env_value("pptp_pass2"));

	nvram_set("zone_if_proto2", "pptp");
	nvram_set("zone_if_list2", "WAN_primary WAN_slave");
	nvram_set("if_dev2", "eth1");
	nvram_set("if_dev3", "ppp3");
	nvram_set("if_proto2", "dhcpc");
	nvram_set("if_proto3", "pptp");
	nvram_set("if_proto_alias2", "WAN_dhcpc");
	nvram_set("if_proto_alias3", "WAN_pptp");
	nvram_set("if_services2", PROTO_DUAL0_SERVICES);
	nvram_set("if_services3", PROTO_DUAL1_SERVICES);
	nvram_set("pptp_mppe2", "nomppe");
	nvram_set("pptp_idle2", "300");
	nvram_set("pptp_mtu2", "1450");
	nvram_set("pptp_dial2", "1");
}

static void set_wizard_wan_l2tp()
{
	nvram_set("l2tp_mode2", get_env_value("l2tp_mode2"));
	nvram_set("l2tp_addr2", get_env_value("l2tp_addr2"));
	nvram_set("l2tp_netmask2", get_env_value("l2tp_netmask2"));
	nvram_set("l2tp_gateway2", get_env_value("l2tp_gateway2"));
	nvram_set("l2tp_serverip2", get_env_value("l2tp_serverip2"));
	nvram_set("l2tp_user2", get_env_value("l2tp_user2"));
	nvram_set("l2tp_pass2", get_env_value("l2tp_pass2"));
	
	nvram_set("zone_if_proto2", "l2tp");
	nvram_set("zone_if_list2", "WAN_primary WAN_slave");
	nvram_set("if_dev2", "eth1");
	nvram_set("if_dev3", "ppp3");
	nvram_set("if_proto2", "dhcpc");
	nvram_set("if_proto3", "l2tp");
	nvram_set("if_proto_alias2", "WAN_dhcpc");
	nvram_set("if_proto_alias3", "WAN_l2tp");
	nvram_set("if_services2", PROTO_DUAL0_SERVICES);
	nvram_set("if_services3", PROTO_DUAL1_SERVICES);
	nvram_set("l2tp_mppe2", "nomppe");
	nvram_set("l2tp_idle2", "300");
	nvram_set("l2tp_mtu2", "1450");
	nvram_set("l2tp_dial2", "1");
}

static void *do_wizard_wan(struct ssc_s *obj)
{	
	char *wan_type;
	wan_type = get_env_value("wan_type");
	if (strcmp(wan_type, "dhcpc") == 0){
		set_wizard_wan_dhcp();	
	} else if (strcmp(wan_type, "static") == 0){
		set_wizard_wan_static();
	} else if (strcmp(wan_type, "pppoe") == 0){
		set_wizard_wan_pppoe();
	} else if (strcmp(wan_type, "pptp") == 0){
		set_wizard_wan_pptp();
	}else if (strcmp(wan_type, "l2tp") == 0){
		set_wizard_wan_l2tp();
	}

	return do_apply_cgi(obj);
}

/* XXX: do_timeset maybe model independent */
/*
static void *do_timeset(struct ssc_s *obj)
{
	__post2nvram(obj);
	nvram_commit();
	system("/sbin/settime");
	system("/sbin/setdaylight");
	sleep(3);

	__do_reboot(obj);
	return get_response_page();
#if 0
	char *ntp;

	if ((ntp = get_env_value("ntp_client_enable")) == NULL)
		goto out;	// FIXME: error

	if (*ntp == '1') {
		__post2nvram(obj);
		save_db();
		do_reboot(obj);
	} else {
		__post2nvram(obj);
*/		/* TODO: mplement evel API... */
/*		system("cli sys set_time");
		do_apply_cgi(obj);

	}
out:
	return get_response_page();
#endif

}
*/

/* Check index of schedule rule(@sched_idx) was been used on @key or not?
 * Return: 0 -> schedule rule unused
 *         1 -> schedule rule inused
 * XXX: porting from 130/330 */
static int schedule_inused(const char *key, int sched_idx)
{
	char buf[128], *p=NULL;
	int i;

	if (query_record(key, buf, sizeof(buf)) != 0)
		return -1;
	if ((p = strrchr(buf, ',')) == NULL)
		i = atoi(buf);
	else
		i = atoi(p + 1);
	
	if (i == sched_idx &&  sched_idx != 0)
		return 1;       /* in used, reject update schedule */
	
	return 0;               /* unused */
}

/* Check deleted the index of schedule rule is legal or not?
 * Return: 1 -> Ok
 *         0 -> No
 * XXX: porting from 130/330 */
static int ok_deleted_schedule(int sched_idx)
{
	static struct {
		char *name;
		char *prefix;
		int min;
		int max;
	} *p, schd[] = {
		{ "Virtual server", "forward_portXX", 0, 23},
		{ "Port forwarding", "forward_portXX", 0, 24},
		{ "Application", "app_portXX", 0, 24},
		{ "URL filter", "url_domain_filter_XX", 0, 49},
		{ "DMZ", "dmz_schedule", -1 , -1},
		{ "Remote Admin", "remote_schedule", -1, -1},
		{ NULL, NULL}
	};
	char buf[128];
	int i;

	for (p = schd; p->name != NULL; p++) {
		if (p->min == -1 && p->max == -1) {
			if (schedule_inused(p->prefix, sched_idx) == 1)
				return 0;
		} else {
			for (i = p->min; i <= p->max; i++) {
				strcpy(buf, p->prefix);
				sprintf(buf + strlen(buf) - 2, "%d", i);

				if (schedule_inused(buf, sched_idx) == 1)
					return 0;
			}
		}
	}

	return 1;
}


/* XXX: porting from 130/330 */
static void *do_apply_sched(struct ssc_s *obj)
{
	int sched_idx;
	char *errpg = "error.asp";
	char *s;

	if ((s = get_env_value("del_row")) == NULL)
		return errpg;

	sched_idx = atoi(s);

	if (0 <= sched_idx && ok_deleted_schedule(sched_idx) != 1)
		return errpg;

	__post2nvram_range(obj);
	nvram_commit();
	__do_apply(obj);

	return get_response_page();
}

#ifdef DIRX30
static void *do_adv_network(struct ssc_s *obj)
{
	char *speed_old = nvram_get("wan_port_speed");

	if (speed_old)
		nvram_set("wan_port_speed_prev", speed_old);

	return do_apply_cgi(obj);
}
#endif
#ifdef LINUX_NVRAM
/* Matt add - 2010/11/05 */
static void *do_download_fw_lp(struct ssc_s *obj)
{
	char *errpg = "error.asp";
	char *file_link, *file_name, *type;
	pid_t pid;

	// get file link
	if(getenv("file_link") == NULL)
		goto error;
	file_link = getenv("file_link");
	// get file name
	if(getenv("file_name") == NULL)
		goto error;
	file_name = getenv("file_name");
	// get update type : fw or lp
	if(getenv("update_type") == NULL)
		goto error;
	type = getenv("update_type");

	system("echo \"\{status : \\\"-1\\\"\}\" > /www/auto_update_st.js"); //status = -1 : prepareing...

	pid = vfork();
	if(pid == 0)
		execlp("/bin/download_fw_lp", "/bin/download_fw_lp", file_link, file_name, type, NULL);
	else if (pid >0)
		goto respg;
	else
		cprintf("XXX %s[%d] vfotk error XXX",__func__,__LINE__);
	
respg:
	return get_response_page();

error:
	setenv("html_response_error_message", "Update fail.", 1);
	return errpg;
}

static void *do_update_fw(struct ssc_s *obj)
{
	char *errpg = "error.asp";
	char file[128];
	int ret;

	// get file name
	if(getenv("update_file_name") == NULL)
		goto error;
	memset(file, '\0', sizeof(file));
	sprintf(file, "/tmp/%s", getenv("update_file_name"));

	return auto_update_firmware(file);

error:
	setenv("html_response_error_message", "Update fail.", 1);
	return errpg;
}

static void *do_update_lp(struct ssc_s *obj)
{
	char *errpg = "error.asp";
	char file[128];
	int ret;

	// get file name
	if(getenv("update_file_name") == NULL)
		goto error;
	memset(file, '\0', sizeof(file));
	sprintf(file, "/tmp/%s", getenv("update_file_name"));

	return auto_upload_lang(file);

error:
	setenv("html_response_error_message", "Update fail.", 1);
	return errpg;
}
#endif
/*--------------------------*/
static struct ssc_s ssc_objs[] = {
	{
		action: "tools_admin",
		shell: NULL,
		fn: do_apply_cgi,
		data: (void *) tools_admin
	},
	{
		action: "wan_multipoe",
		shell: NULL,
		back_html: NULL,
		fn: do_apply_cgi_range,
		data: (void *) wan_multipoe
	},
	{
		action: "wan_rupoe",
		shell: NULL,
		back_html: NULL,
		fn: do_apply_cgi,
		data: (void *) wan_rupoe
	},
	{
		action: "wan_rupptp",
		shell: NULL,
		back_html: NULL,
		fn: do_apply_cgi,
		data: (void *) wan_rupptp
	},
	{
		action: "wan_poe",
		shell: NULL,
		back_html: NULL,
		fn: do_apply_cgi,
		data: (void *) wan_poe
	},
	{
		action: "wan_pptp",
		shell: NULL,
		back_html: NULL,
		fn: do_apply_cgi,
		data: (void *) wan_pptp
	},
	{
		action: "wan_l2tp",
		shell: NULL,
		back_html: NULL,
		fn: do_apply_cgi,
		data: (void *) wan_l2tp
	},
	{
		action: "wan_static",
		shell: NULL,
		back_html: NULL,
		fn: do_apply_cgi,
		data: (void *) wan_static
	},
	{
		action: "wan_dhcp",
		shell: NULL,
		back_html: NULL,
		fn: do_apply_cgi,
		data: (void *) wan_dhcp
	},
	{
		action: "lan",
		shell: NULL,
		back_html: NULL,
		fn: do_lan,
		data: (void *) lan
	},
	{
		action: "lan_revoke",
		shell: NULL,
		back_html: NULL,
		fn: do_revoke,
		data: NULL
	},
	{
		action: "dmz",
		shell: NULL,
		back_html: NULL,
		fn: do_dmz,
		data: (void *) dmz
	},
	{
		action: "tools_ddns",
		shell: NULL,
		back_html: NULL,
		fn: do_apply_cgi,
		data: (void *) tools_ddns
	},
	{
		action: "adv_opendns",
		shell: NULL,
		back_html: NULL,
		fn: do_opendns,
		data: (void *)adv_opendns
	},
	{
		action: "restore_defaults",
		shell: NULL,
		back_html: NULL,
		fn: do_restore_defaults,
		data: NULL
	},
	{
		action: "reboot",
		shell: NULL,
		back_html: NULL,
		fn: do_reboot,
		data: NULL
	},
	{
		action: "ping_test",
		shell: NULL,
		back_html: NULL,
		fn: do_ping_test,
		data: NULL 
	},
	{
		action: "wireless",
		shell: NULL,
		back_html: NULL,
		fn: do_wireless_cgi,
		data: (void *) wireless
	},
	{
		action: "wizard_wlan",
		shell: NULL,
		back_html: NULL,
		//fn: do_apply_cgi,
		fn: *do_wireless_wizard,
		data: (void *) wizard_wlan
	},
	{
		action: "wireless_2g",
		shell: NULL,
		back_html: NULL,
		fn: do_apply_cgi,
		data: (void *) wireless_0 
	},
	{
		action: "guest_zone",
		shell: NULL,
		back_html: NULL,
		fn: do_apply_cgi,
		data: (void *) guest_zone

	},
	{
                action: "multi_ssid",
                shell: NULL,
                back_html: NULL,
                fn: do_apply_cgi,
                data: (void *) adv_mbssid

        },
	{
		action: "adv_portforward",
		shell: NULL,
		back_html: NULL,
		fn: do_apply_cgi_range,
		data: (void *) adv_portforward
	},
	{
		action: "adv_appl",
		shell: NULL,
		back_html: NULL,
		fn: do_apply_cgi_range,
		data: (void *) adv_appl
	},
	{
		action: "adv_filters_mac",
		shell: NULL,
		back_html: NULL,
		// fn: do_apply_cgi_range,
		fn: do_apply_cgi,
		data: (void *) adv_filters_mac
	},
	{
		action: "adv_filters_url",
		shell: NULL,
		back_html: NULL,
		fn: do_apply_cgi_range,
		data: (void *) adv_filters_url
	},
	{
		action: "adv_bandwidth",
		shell: NULL,
		back_html: NULL,
		fn: do_apply_cgi_range,
		data: (void *) adv_bandwidth,
	},
	{
		action: "adv_wlan_perform",
		shell: NULL,
		back_html: NULL,
		fn: do_apply_cgi,
		data: (void *) adv_wlan_perform
	},
	{
		action: "pppoe_connect",
		shell: NULL,
		back_html: NULL,
		fn: do_wan_connect,
		data: NULL
	},
	{
		action: "ru_pppoe_connect",
		shell: "/sbin/rc",
		back_html: NULL,
		fn: do_wan_connect,
		data: NULL
	},
	{
		action: "dhcp_connect",
		shell: "/sbin/rc",
		back_html: NULL,
		fn: do_wan_connect,
		data: NULL
	},
	{
		action: "adv_dmz",
		shell: NULL,
		back_html: NULL,
		fn: do_apply_cgi_range,
		data: (void *) adv_firewall
	},
	{
		action: "adv_access_control",
		shell: NULL,
		back_html: NULL,
		fn: do_apply_cgi_range,
		data: (void *) adv_access_control

	},
	{
		action: "tools_schedules",
		shell: NULL,
		back_html: NULL,
		fn: do_apply_sched,
		data: (void *) tools_schedules
	},
	{
		action: "vpn_ipsec",
		shell: NULL,
		back_html: NULL,
		//fn: do_apply_cgi_range,
		fn: do_vpn_ipsec,
		data: (void *) vpn_ipsec
	},
	{
		action: "vpn_pptp",
		shell: NULL,
		//fn: do_apply_cgi_range,
		fn: do_apply_cgi,
		data: (void *) vpn_pptp
	},
	{
		action: "user_group_edit",
		shell: NULL,
		back_html: NULL,
		fn: do_apply_cgi_range,
		data: user_group_edit
	},
	{
		action: "adv_qos",
		shell: NULL,
		back_html: NULL,
		fn: do_apply_cgi_range,
		data: (void *) qos_rule
	},
	{
		action: "response_page",
		shell: NULL,
		back_html: NULL,
		fn: get_response_page,
		data: NULL
	},
	{
		action: "certificate_create",
		shell: NULL,
		back_html: NULL,
		fn: create_ca,
		data: NULL

	},
	{
		action: "static_route_edit",
		shell: NULL,
		back_html: NULL,
		fn: do_apply_cgi,
		data: (void *) static_route 
	},
	{
		action: "wps_action",
		shell: NULL,
		back_html: NULL,
		fn: do_wps_action,
		data: (void *) wireless
	},
	{
		action: "dynamic_route",
		shell: NULL,
		back_html: NULL,
		fn: do_apply_cgi,
		data: (void *) dynamic_route 
	},	
	{
		action: "policy_route_edit",
		shell: NULL,
		back_html: NULL,
		fn: do_apply_cgi_range,
		data: (void *) policy_route 
	},
	{
		action: "url_domain_filter",
		shell: NULL,
		fn: do_apply_cgi_range,
		data: (void*)url_domain_filter
	},
	{
		action: "vpn_ssl",
		shell: NULL,
		fn: do_apply_cgi,
		data: (void *) vpn_ssl
	},
	{
		action: "timeset",
		shell: NULL,
		fn: do_timeset,
		data: (void *) timeset
	},
	{
		action: "adv_virtual",
		shell: NULL,
		fn: do_apply_cgi_range,
		data: (void *) adv_virtual
	},
	{
		action: "adv_network",
		shell: NULL,
#ifdef DIRX30	/* for DIR-130/330 */
		fn: do_adv_network,
#else
		fn: do_apply_cgi,
#endif
		data: (void *) adv_network
	},
	{
		action: "tools_syslog",
		shell: NULL,
		fn: do_apply_cgi,
		data: (void *) tools_syslog
	},
	{
		action: "st_log_setting",
		shell: NULL,
		fn: do_tools_email,
		data: (void *) tools_email
	},
	{
		action: "send_log_email",
		shell: NULL,
		fn: do_send_mail,
		data: (void *) tools_email
	},
//Albert add
//---
	{
		action: "adv_wps",
		shell: NULL,
		back_html: NULL,
		fn: do_apply_cgi,
		data: (void *) adv_wps
	},
	{
		action: "restore_default_wireless",
		shell: NULL,
		back_html: NULL,
		fn: restore_default_wireless,
		data: (void *) NULL
	},
	{
		action: "set_sta_enrollee_pin",
		shell: NULL,
		back_html: NULL,
		fn: do_sta_enrollee_cgi,
		data: (void *) NULL
	},
	{
		action: "virtual_push_button",
		shell: NULL,
		back_html: NULL,
		fn: do_sta_pbc_cgi,
		data: (void *) NULL
	},	
	{
		action: "wireless_5g",
		shell: NULL,
		back_html: NULL,
		fn: do_apply_cgi,
		data: (void *) wireless_1 
	},	
//---	
	{
		action: "st_dev_connect",
		shell: NULL,
		back_html: NULL,
		fn: _connect,
		data: NULL
	},
	{
		action: "st_dev_disconnect",
		shell: NULL,
		back_html: NULL,
		fn: _disconnect,
		data: NULL
	},
	{
		action: "st_dev_rconnect",
		shell: NULL,
		back_html: NULL,
		fn: _rconnect,
		data: NULL
	},
	{
		action: "reset_ifconfig_packet_counter",
		shell: NULL,
		back_html: NULL,
		fn: do_clean_statistics,
		data: NULL
	},
	{
		action: "usb_setting",
		shell: NULL,
		back_html: NULL,
		fn: do_apply_cgi,
		data: (void *) usb_setting
	},
	{
		action: "do_wizard_vpn",
		shell: NULL,
		back_html: NULL,
		fn: do_wizard_vpn,
		data: (void *) wizard_vpn
	},
	{
		action: "wizard_wan",
		shell: NULL,
		back_html: NULL,
		//fn: do_wizard_wan,
		fn: do_apply_cgi,
		data: (void *) wizard_wan
	},
	{
		action: "get_trx",
		shell: NULL,
		back_html: NULL,
		fn: do_get_trx_cgi,
		data: NULL
	},
	{
		action: "do_graph_auth",
		shell: NULL,
		back_html: NULL,
		fn: do_graph_auth,
		data: NULL
	},
/*
	{
		action: "wan_3g",
		shell: NULL,
		back_html: NULL,
		fn: do_apply_cgi,
		data: (void *) wan_3g
	},
*/
	{
		action: "wan_usb3gphone",
		shell: NULL,
		back_html: NULL,
		fn: do_apply_cgi,
		data: (void *) wan_usb3gphone
	},
	{
		action: "wan_usb3gadapter",
		shell: NULL,
		back_html: NULL,
		fn: do_apply_cgi,
		data: (void *) wan_usb3gadapter
	},
	{
		action: "adv_inbound_filter",
		shell: NULL,
		back_html: NULL,
		fn: do_apply_cgi_range,
		data: (void *) inbound_filter
	},
	{
		action: "adv_wish",
		shell: NULL,
		back_html: NULL,
		fn: do_apply_cgi_range,
		data: (void *) adv_wish

	},
	{
		action: "adv_ipv6_static",
		shell: NULL,
		back_html: NULL,
		fn: do_apply_cgi,
		data: (void *) adv_ipv6_static
	},
	{
		action: "adv_ipv6_poe",
		shell: NULL,
		back_html: NULL,
		fn: do_apply_cgi,
		data: (void *) adv_ipv6_poe
	},
	{
		action: "adv_ipv6_6in4",
		shell: NULL,
		back_html: NULL,
		fn: do_apply_cgi,
		data: (void *) adv_ipv6_6in4
	},
	{
		action: "adv_ipv6_6to4",
		shell: NULL,
		back_html: NULL,
		fn: do_apply_cgi,
		data: (void *) adv_ipv6_6to4
	},
	{
		action: "adv_ipv6_autodetect",
		shell: NULL,
		back_html: NULL,
		fn: do_apply_cgi,
		data: (void *) adv_ipv6_autodetect
	},
	{
		action: "adv_ipv6_autoconfig",
		shell: NULL,
		back_html: NULL,
		fn: do_apply_cgi,
		data: (void *) adv_ipv6_autoconfig
	},
	{
		action: "adv_ipv6_link_local",
		shell: NULL,
		back_html: NULL,
		fn: do_apply_cgi,
		data: (void *) adv_link_local
	},
        {
                action: "adv_ipv6_6rd",
                shell: NULL,
                back_html: NULL,
                fn: do_apply_cgi,
                data: (void *) adv_ipv6_6rd
        },
#ifndef DIRX30
        {
                action: "wizard_ipv6",
                shell: NULL,
                back_html: NULL,
                fn: wizard_ipv6,
                data: NULL
        },
#endif
        {
                action: "adv_ipv6_firewall",
                shell: NULL,
                back_html: NULL,
                fn: do_apply_cgi,
                data: (void *) adv_ipv6_firewall
        },
        {
                action: "adv_ipv6_routing",
                shell: NULL,
                back_html: NULL,
                fn: do_apply_cgi,
                data: (void *) adv_ipv6_routing
        },
        {
                action: "ping6_test",
                shell: NULL,
                back_html: NULL,
                fn: do_ping6_test,
                data: NULL
        },
        {
                action: "reject",
                shell: NULL,
                back_html: NULL,
                fn: do_allow_reject_page,
                data: NULL
        },
	/* Matt add - 2010/11/05 */
#ifdef LINUX_NVRAM
	{
		action: "download_fw_lp",
		shell: NULL,
		fn: do_download_fw_lp,
		data: NULL, 
	},
	{
		action: "auto_up_fw",
		shell: NULL,
		fn: do_update_fw,
		data: NULL, 
	},
	{
		action: "auto_up_lp",
		shell: NULL,
		fn: do_update_lp,
		data: NULL, 
	},
#endif
	/*-------------------------*/
	{
		action: NULL,
		shell: NULL,
		back_html: NULL,
		fn: NULL,
		data: NULL
	}
};

int init_plugin()
{
	int rev = -1;
	if (ssc_post_method_register(post_plugin) == -1)
		goto out;
	if (ssi_get_method_register(method_get_virtual_files) == -1)
		goto out;
	if (ssc_action_register(ssc_objs) == -1)
		goto out;
	rev = 0;
out:
	return rev;
}
