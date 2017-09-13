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

#include "querys.h"
#include "ssc.h"
#include "ssi.h"
#include "log.h"
#include "libdb.h"
#include "rcctrl.h"
#define SYS_SERVICE	"sys_service"	/* what services need restart? */
#if 1
#define DEBUG_MSG(fmt, arg...)       cprintf(fmt, ##arg)
#else
#define DEBUG_MSG(fmt, arg...) 
#endif

static int get_base(struct items_range *it)
{
	int _base = 0, tail;
	char buf[128], *e;
	
	if (it->_base == NULL)
		goto out;

	if ((e = get_env_value(it->_base)) == NULL)
		goto out;
	tail = atoi(e);
	for (_base = it->start;_base < tail; _base++) {
		sprintf(buf, "%s%d", it->key, _base);
		if (NVRAM_MATCH(buf, ""))	/* buf might be NULL or "" */
			break;
	}
out:
	return _base;
}

static void re_sort(struct items_range *it)
{
	int _base, i = 0;
	char *p, from[512], to[512];
	
	for (i = _base = it->start;_base <= it->end; _base++) {
		sprintf(from, "%s%d", it->key, _base);
		if (*(p = nvram_safe_get(from)) == '\0')
			continue;
		if (i != _base) {
			sprintf(to, "%s%d", it->key, i);
			nvram_set(to, p);
			nvram_set(from, "");
		}
		i++;
	}

	return;

}

static int is_readonly()
{
	int ret = 0;
	/*
 	 * XXX: for test only, always allow sava config
	 */
	return 0;
	if (strcmp(nvram_safe_get("ro_username"), nvram_safe_get("current_user")) != 0)
		goto out;

	ret = 1;
out:
	return ret;
}

void __post2nvram_range(struct ssc_s *obj)
{
	struct items_range *it;
	char *s, buf[512], *value;
	int i;
	
	DEBUG_MSG("%s: opt->action=%s\n", __FUNCTION__, obj->action);
	for (it = obj->data; it != NULL && it->key != NULL; it++) {
		int _base = 0;

		DEBUG_MSG("%s: it->key=%s\n", __FUNCTION__, it->key);
		if (it->_flags & IRFG_NV_OFFSET)
		{
			_base = get_base(it);
		}
		for (i = it->start; i <= it->end; i++) {
			/* is a ranges ? */
			if (it->start == -1)
				sprintf(buf, "%s", it->key);
			else
				sprintf(buf, "%s%d", it->key, i);
			/* get key value from browser */
			s = get_env_value(buf);
			if (s == NULL) {
				//cprintf("XXX LOSE KEY: %s = NULL\n", buf);
				if (it->_default == NULL)
					continue;
				value = it->_default;
			} else {
				value = s;
			}
			/* Need OFFSET ? */
			if (it->_flags & IRFG_NV_OFFSET)
				sprintf(buf, "%s%d", it->key, i + _base);
			
			DEBUG_MSG("%s: nvram_set %s:%s\n", __FUNCTION__, buf, value);
			nvram_set(buf, value);
		}
		if (it->_flags & IRFG_NV_OFFSET)
			re_sort(it);
	}
}

/* XXX: porting from DIR-130/330 cgi, should we rc restart anytime? */
inline void __do_apply(struct ssc_s *obj)
{
	pid_t pid;
#if NOMMU
	pid = vfork();
#else
	pid = fork();
#endif
	switch (pid) {
	case -1:
		return;
	case 0:
		close(0);
		close(1);
		close(2);
		sleep(1);
		rc_restart();	/* ext/rcctrl.c */
		exit(0);
	default:
		waitpid(pid, NULL, 0);
	}
}


void *do_apply_cgi_range(struct ssc_s *obj)
{

	if (is_readonly()) {
		make_back_msg("Read only user cannot save settings.");
		return "error.asp";
	}

	DEBUG_MSG("do_apply_cgi_range:opt->action=%s\n",obj->action);
	cprintf("mmu test %s:%d", __FUNCTION__, __LINE__);
#if !NOMMU
	if (fork())
#endif
	{
		char *sys = get_env_value(SYS_SERVICE);
		updown_services(0, sys);
		__post2nvram_range(obj);
		nvram_commit();
		/* XXX: Do we sure every time is kill rc? And Kill rc need fork? 
		 * May be need sleep 1 sec, but sleep on rc is more better
		 */
		DEBUG_MSG("do_apply_cgi_range ok\n");
		__do_apply(obj);
		updown_services(1, sys);
#if !NOMMU
		exit(0);
#endif
	}
	return get_response_page();
}

/*
 * save data from GUI to nvram.
 * XXX: but not flush to flash by "nvram save"
 * */
void __post2nvram(struct ssc_s *obj)
{
	struct items *it;
	char *s;
	char *value;

	for (it = obj->data; it != NULL && it->key != NULL; it++) {
		
		DEBUG_MSG("do apply cgi: it->key=%s:",it->key);
		s = get_env_value(it->key);
		DEBUG_MSG("%s\n", s? s: it->_default? it->_default: "NULL");
		if (s == NULL) {
			//cprintf("XXX LOSE KEY: %s = NULL\n", it->key);
			if (it->_default == NULL)
				continue;
			value = it->_default;
		} else {
			value = s;
		}

		update_record(it->key, value);
	}
}

void *do_apply_cgi(struct ssc_s *obj)
{
	if (is_readonly()) {
		make_back_msg("Read only user cannot save settings.");
		return "error.asp";
	}

	DEBUG_MSG("do apply cgi:opt->action=%s\n",obj->action);
	SYSLOG("DO_APPLY_CGI");
#if !NOMMU
	if (fork() == 0)
#endif
	{
		char *sys = get_env_value(SYS_SERVICE);
		updown_services(0, sys);
		__post2nvram(obj);
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
		__do_apply(obj);
		updown_services(1, sys);
#if !NOMMU
		exit(0);
#endif
	}
	return get_response_page();
}

void *do_wan_connect(struct ssc_s *obj)
{
	char tmp[128], value[64], countdown[4];
	int wan_type = 0;
	char *status = get_env_value("status");
	char wan_ifname[8], russia[8];
	char dns[128], *ip, *pt;
	FILE *fp;

	bzero(tmp, sizeof(tmp));
	bzero(value, sizeof(value));
	bzero(countdown, sizeof(countdown));
	query_vars("wan0_proto", value, sizeof(value));
	query_vars("dhcp_renew_countdown", countdown, sizeof(countdown));

	nvram_set("dhcp_renew_countdown", countdown);

	if (strcmp(value, "dhcpc") == 0) {
		wan_type = 0;
	} else if(strcmp(value, "pppoe") == 0) {
		wan_type = 2;
	} else if(strcmp(value, "pptp") == 0) {
		wan_type = 3;
	} else if(strcmp(value, "l2tp") == 0) {
		wan_type = 5;
	} else if(strcmp(value, "rupppoe") == 0) {
		wan_type = 7;
	} else if(strcmp(value, "rupptp") == 0) {
		wan_type = 8;
	}

	/* for russia pppoe and russia pptp */
	query_vars("russia", russia, sizeof(russia));
	if(strcmp(russia, "russia") == 0)
		wan_type = 0;

	wan_status(tmp, wan_type);

	if (wan_type == 0) {
		if (strcmp(status, "DHCP Renew") == 0) {
			if (strcmp(tmp, "Disconnected") == 0 || strcmp(tmp, "Establishing") == 0) {
				if((strcmp(russia, "russia") == 0) &&
					     (strcmp(value, "rupppoe") == 0)) {
					system("/sbin/do_dhcp");
					sleep(5);
					return get_response_page();
				}
				kill(1, SIGHUP);
			}
		} else if (strcmp(status, "DHCP Release") == 0) {
			if (strcmp(tmp, "Disconnected") != 0) {
				system("killall udhcpc");
				system("rm -f /tmp/var/run/udhcpc0.txt");
				query_vars("wan_ifname", wan_ifname, sizeof(wan_ifname));
				sprintf(value, "/sbin/ifconfig %s 0.0.0.0", wan_ifname);
				system(value);
				query_vars("wan0_proto", value, sizeof(value));
				if((strcmp(russia, "russia") == 0) &&
					      (strcmp(value, "rupppoe") == 0)) {
					tmp[0] = '\0';
					value[0] = '\0';
					query_vars("wan0_dns", dns, sizeof(dns));
					pt = dns;
					system("echo \"before while\" >> /tmp/kgp");
					while(strlen(pt) != 0) {
						system("echo \"in while\" >> /tmp/kgp");
						ip = strsep(&pt, " ");
						sprintf(value, "echo \"ip = %s\" >> /tmp/kgp", ip);
						system(value);
						sprintf(value, "nameserver %s\n", ip);
						strcat(tmp, value);
					}
					if(fp = fopen("/tmp/resolv.conf", "w")) {
						system("echo \"write /tmp/resolv.conf\" >> /tmp/kgp");
						fprintf(fp, "%s", tmp);
					}
					fclose(fp);
				} else {
					system("/usr/sbin/nvram set wan0_ipaddr=0.0.0.0");
					system("/usr/sbin/nvram set wan0_gateway=0.0.0.0");
					system("/usr/sbin/nvram set wan0_netmask=0.0.0.0");
					system("echo \"\" > /tmp/resolv.conf");
				}
				/* add for russia physical wan */
				system("/usr/sbin/nvram set wan0_dhcp_ipaddr=0.0.0.0");
				system("/usr/sbin/nvram set wan0_dhcp_gateway=0.0.0.0");
				system("/usr/sbin/nvram set wan0_dhcp_netmask=0.0.0.0");
				system("/usr/sbin/nvram set wan0_dhcp_dns=");
			}
		}
	} else if (wan_type >= 2) {
		if (strcmp(status, "Connect") == 0) {
			if (strcmp(tmp, "Disconnected") == 0) {
				/* for russia */
				system("echo 0 > /proc/sys/net/ipv4/ip_forward");
				system("/bin/pppoe_connect > /dev/null 2>&1");
			} else if (strcmp(tmp, "Connected") == 0) {
				strcpy(value, "");
				if (wan_type == 2) {
					query_vars("wan0_pppoe_demand", value, sizeof(value));
				} else if (wan_type == 3) {
					query_vars("wan0_pptp_demand", value, sizeof(value));
				} else if (wan_type == 4) {
				} else if (wan_type == 5) {
					query_vars("wan0_l2tp_demand", value, sizeof(value));
				} else if (wan_type == 7) {
					query_vars("wan0_rupppoe_demand", value, sizeof(value));
				} else if (wan_type == 8) {
					query_vars("wan0_rupptp_demand", value, sizeof(value));
				}
				if (atoi(value) == 1) {
					kill(1, SIGHUP);
				}
			}
		}
		if (strcmp(status, "Disconnect") == 0) {
			if (strcmp(tmp, "Disconnected") != 0) {
				system("/usr/sbin/nvram set"
						" wan0_ipaddr=0.0.0.0");
				system("/usr/sbin/nvram set"
						" wan0_netmask=0.0.0.0");
				system("/usr/sbin/nvram set"
						" wan0_gateway=0.0.0.0");
				/* system("/usr/sbin/nvram set wan0_dns="); */
				kill(1, SIGHUP);
			}
		}
	}
	return get_response_page();
}
