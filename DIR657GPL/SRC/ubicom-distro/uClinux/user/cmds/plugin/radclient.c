#include <syslog.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include <includes.h>
#include <freeradius-client.h>
#include <pathnames.h>
#define RC_CONFIG_FILE "/etc/radiusclient/radiusclient.conf"
char *plugin_mnt_point()
{
	return "security";
}
int
radius_auth(const char *user, const char *pass)
{
	int             result;
	char		username[128];
	char            passwd[AUTH_PASS_LEN + 1];
	VALUE_PAIR 	*send, *received;
	uint32_t		service;
	char 		msg[4096], username_realm[256];
	char		*default_realm;
	rc_handle	*rh;


	rc_openlog("radclient");

	if ((rh = rc_read_config(RC_CONFIG_FILE)) == NULL)
		return ERROR_RC;

	if (rc_read_dictionary(rh, rc_conf_str(rh, "dictionary")) != 0)
		return ERROR_RC;

	default_realm = rc_conf_str(rh, "default_realm");

	//strncpy(username, rc_getstr (rh, "login: ",1), sizeof(username));
	//strncpy (passwd, rc_getstr(rh, "Password: ",0), sizeof (passwd));
	strncpy(username, user, sizeof(username));
	strncpy (passwd, pass, sizeof (passwd));

	send = NULL;

	/*
	 * Fill in User-Name
	 */

	strncpy(username_realm, username, sizeof(username_realm));

	/* Append default realm */
	if ((strchr(username_realm, '@') == NULL) && default_realm &&
	    (*default_realm != '\0'))
	{
		strncat(username_realm, "@", sizeof(username_realm)-strlen(username_realm)-1);
		strncat(username_realm, default_realm, sizeof(username_realm)-strlen(username_realm)-1);
	}

	if (rc_avpair_add(rh, &send, PW_USER_NAME, username_realm, -1, 0) == NULL)
		return ERROR_RC;

	/*
	 * Fill in User-Password
	 */

	if (rc_avpair_add(rh, &send, PW_USER_PASSWORD, passwd, -1, 0) == NULL)
		return ERROR_RC;

	/*
	 * Fill in Service-Type
	 */

	service = PW_AUTHENTICATE_ONLY;
	if (rc_avpair_add(rh, &send, PW_SERVICE_TYPE, &service, -1, 0) == NULL)
		return ERROR_RC;

	result = rc_auth(rh, 0, send, &received, msg);

	if (result == OK_RC)
	{
		fprintf(stderr, "\"%s\" RADIUS Authentication OK\n", username);
	}
	else
	{
		fprintf(stderr, "\"%s\" RADIUS Authentication failure (RC=%i)\n", username, result);
	}

	return result;
}

static int write_conf(
	const char *ip, const char *port, const char *secret,
	const char *name, const char *pass)
{
#define RAD_SERVERS		"/etc/radiusclient/servers"
#define RAD_CONFIG		"/etc/radiusclient/radiusclient.conf"
	FILE *fd;
	int rev = -1;


	if ((fd = fopen(RAD_SERVERS, "w")) == NULL)
		goto sys_err;
	fprintf(fd, "%s	%s\n", ip, secret);
	fclose(fd);
	
	if ((fd = fopen(RAD_CONFIG, "w")) == NULL)
		goto sys_err;
	
	fprintf(fd,
		"auth_order	radius\n"
		"login_tries	1\n"
		"login_timeout	1\n"
		"nologin /etc/nologin\n"
		"issue	/share/rootfs/etc/radiusclient/issue\n"
		"authserver 	%s\n"
		"acctserver 	localhost\n"
		"servers		/etc/radiusclient/servers\n"
		"dictionary 	/etc/radiusclient/dictionary\n"
		"login_radius	/sbin/login.radius\n"
		"seqfile		/var/run/radius.seq\n"
		"mapfile		/etc/radiusclient/port-id-map\n"
		"default_realm\n"
		"radius_timeout	1\n"
		"radius_retries	1\n"
		"radius_deadtime	1\n"
		"bindaddr *\n"
		"login_local	/bin/login\n",
		ip);
	fclose(fd);
	return 0;
sys_err:
	syslog(LOG_INFO, "write radius config: %s", strerror(errno));
	return rev;
}

int
plugin_init_main(int argc, char **argv)
{
	int rev = -1, i;
	char *ip, *port, *secret, buf[512];
	char *user, *passwd;
	
	if ((user = getenv("USERNAME")) == NULL) goto env_err;
	if ((passwd = getenv("PASSWORD")) == NULL) goto env_err;

	for (i = 0;i < 2; i++) {
		sprintf(buf, "RADIUS_SERVER%d", i);
		if ((ip = getenv(buf)) == NULL) goto env_err;
		sprintf(buf, "RADIUS_SECRET%d", i);
		if ((secret = getenv(buf)) == NULL) goto env_err;
		sprintf(buf, "RADIUS_PORT%d", i);
		if ((port = getenv(buf)) == NULL) goto env_err;

		if (write_conf(ip, port, secret, user, passwd) == -1)
			break;
		
		/*1: handshark failure, 2: auth failure, 254: secret failure */
		if ((rev = radius_auth(user, passwd)) == 1)
			continue;
		break;
	}
	
	return rev;
env_err:
	syslog(LOG_INFO, "%s parameters error", __FUNCTION__);
out:
	return rev;
}
