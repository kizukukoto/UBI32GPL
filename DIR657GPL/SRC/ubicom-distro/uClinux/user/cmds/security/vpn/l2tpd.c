#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <shutils.h>
#include <unistd.h>
/*
#include <nvpptp.h>
#include <nvl2tp.h>
#include <bcmnvram.h>
#include "rc.h"
*/
#include "nvpptp.h"
#include "nvl2tp.h"
#include <nvramcmd.h>
#define L2TPD_OPTION_FILE   "/tmp/ppp/options.l2tpd"
#define L2TPCLIENT_OPTION_FILE   "/tmp/ppp/options.l2tpclient"
#define L2TPD_CONF_FILE     "/tmp/ppp/l2tpd.conf"
#define L2TPD_AUTHMETHOD    nvram_safe_get("l2tpd_authmethod")
#define L2TPD_PLUGIN        (!strcmp(L2TPD_AUTHMETHOD, "DB"))?"plugin ppp-login.so\n":""

int write_l2tpd_lns(char *);
int write_l2tpd_options( void );

#define CHAP_SECRETS_FILE	"/var/etc/chap-secrets"
#define PAP_SECRETS_FILE	"/var/etc/pap-secrets"
int write_configfile(char *file)
{
	FILE *fd;
	char group[16], user[64], buf[2048];
	char *pt, *passwd, *tmp, *groupname;  
	int i;
	/* Write to options  file based on current information */
	if ( !(fd = fopen(file, "w")) ) {
		perror(file);
		return -1;
	}
	
	fprintf(fd,
	     "# secrets for pppd\n"
	     "# client    server    secret    addrs\n");

	if(strcmp(nvram_safe_get("auth_pptpd_l2tpd"), "")) {
		sprintf(group, "auth_group%d",
			       	atoi(nvram_safe_get("auth_pptpd_l2tpd")));
		sprintf(buf, "%s", nvram_safe_get(group));
		tmp = buf;
		groupname = strtok(tmp, "/");
		while((pt = strtok(NULL, "/"))) {
			if ((passwd = strchr(pt, ',')) == NULL)
				continue; // format error!
			strcpy(user, pt);
			user[strlen(user) - strlen(passwd)] = '\0';
			passwd++;
			fprintf(fd, "\"%s\"  *  \"%s\"  *\n", user, passwd);
		}
	} else { /* XXX:It might be obsoleted from v1.10 */
		for(i = 1; i <= 3; i++) {
			sprintf(group, "username%d", i);
			sprintf(buf, "passwd%d", i);
			if(strcmp(nvram_safe_get(group), "")) {
				fprintf(fd, "\"%s\"  *  \"%s\"  *\n",
						nvram_safe_get(group),
						nvram_safe_get(buf));
			}
		}
	}
	fclose( fd );
	return 0;
}

int write_secrets( void )
{
    if (nvram_invmatch("l2tpd_enable", "0") &&
		    NVRAM_MATCH("l2tpd_auth_proto", "pap") ||
		    nvram_invmatch("pptpd_enable", "0") &&
		    NVRAM_MATCH("pptpd_auth_proto", "pap")) {
	    write_configfile(PAP_SECRETS_FILE);
    }
    if (nvram_invmatch("l2tpd_enable", "0") &&
		    nvram_invmatch("l2tpd_auth_proto", "pap") ||
		    nvram_invmatch("pptpd_enable", "0") &&
		    nvram_invmatch("pptpd_auth_proto", "pap")) {
	    write_configfile(CHAP_SECRETS_FILE);
    }

    return 0;
}
/*
* create/recreate the options file in /tmp/ppp
*/
int
write_l2tpd_lns(char *prefix)
{
    FILE *fd;
    l2tpd_t l2tpd_data;
    char p[32];

    //mkdir( "/tmp/ppp", 0777 );

    memset( &l2tpd_data, 0x00, sizeof(l2tpd_t) );
    if ( get_nvl2tpd( &l2tpd_data ) == -1) 
        return -1;  /* no parameters */

    if ( !l2tpd_data.enable )
        return -1;          // l2tpd disabled


    /* Write to options  file based on current information */
    if ( !(fd = fopen(L2TPD_CONF_FILE, "w")) ) {
	perror( L2TPD_CONF_FILE );
	return -1;
    }

    p[0] = '\0';
    if(NVRAM_MATCH("l2tpd_auth_proto", "pap")) {
	    sprintf(p, "refuse chap = yes\nrequire pap = yes\n");
    } else {
	    sprintf(p, "refuse pap = yes\nrequire chap = yes\n");
    }


    fprintf( fd,
	     "[global]\n\n"
	     "[lns default]\n"
	     "ip range = %s\n"
	     "local ip = %s\n"
	     "%s"
	     "require authentication = yes\n"
	     "name = %s\n"
	     "ppp debug = yes\n"
	     "pppoptfile = %s\n"
	     "length bit = yes\n\n"
	     , l2tpd_data.remoteips,
	     l2tpd_data.localip,
	     p,
	     l2tpd_data.servername,
	     L2TPD_OPTION_FILE
		     );
    fclose( fd );

    return 0;
}

int write_l2tpd_lac(char *prefix)
{
	char tmp[128], p[16];
	FILE *fd;

	/* reflash config file to get new value */
	if (NVRAM_MATCH("l2tpd_enable", "0"))
		sprintf(p, "w");
	else
		sprintf(p, "a");

	/* Write to options  file based on current information */
	if ( !(fd = fopen(L2TPD_CONF_FILE, p)) ) {
		perror( L2TPD_CONF_FILE );
		return -1;
	}

    	p[0] = '\0';
	if(NVRAM_MATCH("l2tpd_enable", "0"))
		sprintf(p, "[global]\n\n");

	fprintf( fd,
	     "%s"
	     "[lac]\n"
	     "lns = %s\n"
	     "autodial = yes\n"
	     "refuse authentication = yes\n"
	     "ppp debug = yes\n"
	     "pppoptfile = %s\n",
	     p,
	     nvram_safe_get(strcat_r(prefix, "l2tp_server_ipaddr", tmp)),
	     L2TPCLIENT_OPTION_FILE);

	fclose( fd );

	return 0;
}

int
write_l2tpd_options( void )
{
    FILE *fd;
    char tmp[128], buf[128], p[32];
    char *auth[] = {"pap", "chap", "mschap", "mschap-v2", NULL};
    int i = 0;

    /* Write to options  file based on current information */
    if (!(fd = fopen(L2TPD_OPTION_FILE, "w"))) {
        perror(L2TPD_OPTION_FILE);
        return -1;
    }

    tmp[0] = '\0';
    while(auth[i]) {
	    if(nvram_invmatch("l2tpd_auth_proto", auth[i])) {
		    sprintf(p, "refuse-%s\n", auth[i]);
		    strcat(tmp, p);
	    }
	    i++;
    }
    sprintf(p, "require-%s\n", nvram_safe_get("l2tpd_auth_proto"));
    strcat(tmp, p);

    buf[0] = '\0';
    fprintf( fd,
             //"ipcp-accept-local\n"
             //"ipcp-accept-remote\n"
             "%s"
	     "ipparam xl2tpd\n"
	     "novj\n"
	     "nobsdcomp\n"
	     "novjccomp\n"
	     "nologfd\n"
             "idle 1800\n"
             "mtu 1410\n"
             "mru 1410\n"
             "debug\n"
	     "dump\n"
             "lock\n"
	     "unit 10\n"
             "proxyarp\n"
	     "%s"
	     "%s",
             //"auth\n"
	     //"noccp\n"
             //"crtscts\n"
             //"nodefaultroute\n"
             //"connect-delay 5000\n",
	     tmp, L2TPD_PLUGIN, vpn_dns(buf)
           );
    fclose( fd );

    return 0;
}


/*
 * Start L2TPD
 */
int
start_l2tpd( void )
{
	int rc=0;
	char prefix[8] = "static_", tmp[32];

	// create l2tpd lns conf file
	if (access("/var/run/xl2tpd", X_OK))
		mkdir("/var/run/xl2tpd", 0755);
	write_secrets();
	rc = write_l2tpd_lns(prefix);
	if ( rc < 0 )
		return 0;

	// create options_l2tpd file
	rc = write_l2tpd_options();
	if ( rc < 0 )
		return 0;

	printf( "starting l2tpd\n" );

	// If l2tpd enabled but not over ipsec,
	// accept all packet incoming udp 1701 port
	eval("iptables", "-A", "INPUT", "-i", "ppp+", "-j", "ACCEPT");
	eval("iptables", "-A", "FORWARD", "-i", "ppp+", "-j", "ACCEPT");
	if(NVRAM_MATCH("l2tpd_overipsec_enable", "0")) {
		eval("iptables", "-A", "INPUT", "-p", "udp",
			"--dport", "1701", "-j", "ACCEPT");
		goto out;
	}
	// If l2tpd over ipsec enabled,
	// only accept packet incoming udpp 1701 port in ipsec0 interface.
	// And, deny all not ipsec0 interface packet.
	/*
	eval("/usr/sbin/iptables", "-I", "INPUT", "-i", "ipsec0", "-p", "udp",
		"--dport", "1701", "-j", "ACCEPT");
	eval("/usr/sbin/iptables", "-I", "INPUT", "-i", "!", "ipsec0", "-p", "udp",
		"--dport", "1701", "-j", "DROP");
	*/
	//eval("xl2tpd", "-c", L2TPD_CONF_FILE );

/*
	if(!nvram_match("wan0_proto", "static") && !nvram_match("wan0_proto", "dhcpc")) {
		eval("/usr/sbin/iptables", "-I", "INPUT", "-i", "ppp+",
			"-p", "udp", "--dport", "1701", "-j", "DROP");

	eval("/usr/sbin/iptables", "-I", "INPUT", "-i", "eth0",
		"-p", "udp", "--dport", "1701", "-j", "DROP");
	//eval("l2tpd");
*/
out:
	system("xl2tpd -c /tmp/ppp/l2tpd.conf &");

	return 0;
}

/*
 * Stop L2TPD
 */
int
stop_l2tpd( void )
{
	printf( "stoping l2tpd\n" );

	eval("iptables", "-D", "INPUT", "--proto",
		    "udp", "--dport", "1701", "--jump", "ACCEPT");
	eval("iptables", "-D", "INPUT", "-i", "ppp+", "-j", "ACCEPT");
	eval("iptables", "-D", "FORWARD", "-i", "ppp+", "-j", "ACCEPT");
	eval("killall", "xl2tpd");
	return 0;
}
/* start and stop main */
#include "cmds.h"
#include "shutils.h"
static int l2tpd_start_main(int argc, char *argv[])
{
	start_l2tpd();
}
static int l2tpd_stop_main(int argc, char *argv[])
{
	stop_l2tpd();
}
int xl2tpd_main(int argc, char *argv[])
{
	struct subcmd cmds[] = {
		{ "start", l2tpd_start_main,"start l2tpd: rev0.00"},
		{ "stop", l2tpd_stop_main,"stop l2tpd: rev0.00"},
		{ NULL, NULL}
	};
	return lookup_subcmd_then_callout(argc, argv, cmds);
}
