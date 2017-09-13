#include <stdio.h>
#include <stdlib.h>
#include "nvramcmd.h"
#include "cmds.h"
#include "nvram.h"
#include "shutils.h"

#define cprintf(fmt, args...) do { \
	FILE *fp = fopen("/dev/console", "w"); \
	if (fp) { \
                fprintf(fp, fmt, ## args); \
                fclose(fp); \
        } \
} while (0)

#define __NVRAM_BUF_SIZE	4352
extern int base64_encode(char *, char *, int);      /* in lib/common-tools.c */

static void fn_nvram_download(const char *k, const char *v, void *data)
{
	char  *protection[] = {
		"wan_mac",
		"lan_mac",
		"lan_eth",
		"lan_bridge",
		"wan_eth",
		"wlan0_domain",
		"asp_temp_",
		"html_",
		"admin_username",
		"admin_password",
		"user_username",
		"user_password",
		NULL
	}, **p;
	FILE *fd = data;
	int protect = 0;
	char tmp[1024];
	char base64[1024];


	if (strncmp(k, "sys_", 4) == 0) {
		protect = 1;
		goto dump;
	}
	for (p = protection; *p != NULL; p++) {
		if (strcmp(k, *p) != 0)
			continue;
		protect = 1;
		break;
	}
dump:
	if (!protect) {
		if (v == NULL)
			v = "";
		sprintf(tmp,"%s=%d-%s", k, strlen(v), v);
		base64_encode(tmp,base64,strlen(tmp));
		fprintf(fd, "%s\n", base64);
	} else
		fprintf(stderr, "ignored : [%s:%s]\n", k, v);
}
static int param_save_conf(int argc, char *argv[])
{
	 return foreach_nvram_from_mtd(fn_nvram_download, stdout);
}


/* 
 * format sample:
 *
 * wan_port_speed=4-auto
 * timer_interval=6-604800
 * ntp_server=14-ntp1.dlink.com
 * time_zone=5-GMT+8
 * log_level=1-0
 * fw_disable=1-0
 * x509_ca_2=1281------BEGIN CERTIFICATE-----
 * MIIDhjCCAu+gAwIBAgIBBjANBgkqhkiG9w0BAQQFADCBiDELMAkGA1UEBhMCVFcx
 * DzANBgNVBAgTBlRhaXdhbjEPMA0GA1UEBxMGVGFpcGVpMQ4wDAYDVQQKEwVDQU1F
 * TzEMMAoGA1UECxMDQlUxMRIwEAYDVQQDEwlTb2Z0d2FyZTQxJTAjBgkqhkiG9w0B
 * CQEWFnBldGVyX3BhbkBjYW1lby5jb20udHcwHhcNMTAwNDEzMDcxNjM3WhcNMTMw
 * MTA3MDcxNjM3WjBxMQswCQYDVQQGEwJUVzEPMA0GA1UECBMGVGFpd2FuMQ4wDAYD
 * VQQKEwVDQU1FTzEMMAoGA1UECxMDQlUxMQ8wDQYDVQQDEwZkaXI2NTUxIjAgBgkq
 * hkiG9w0BCQEWE2RpcjY1NUBjYW1lby5jb20udHcwgZ8wDQYJKoZIhvcNAQEBBQAD
 * gY0AMIGJAoGBAMqZNiO4E9qkL0lpLvoyNNFhj+zKwZsOrsZgumO+Aih3LacRWt8v
 * TGYHg2ons9DZqVGcfvigWP6vTva1fRfradvEK8GIa2mn5wXXAn2p86povk8xIYp5
 * hfrZhHt6wZJL5PkZ7Arx3aXeHb7UVfVqGKKQO0grbgx3fXD3SijjpyiPAgMBAAGj
 * ggEUMIIBEDAJBgNVHRMEAjAAMCwGCWCGSAGG+EIBDQQfFh1PcGVuU1NMIEdlbmVy
 * YXRlZCBDZXJ0aWZpY2F0ZTAdBgNVHQ4EFgQUZ/y8zTjDp7CBeAdOTsUqGR+o33Yw
 * gbUGA1UdIwSBrTCBqoAU9iBFpw9Isot/sy8LQynrklGm8vehgY6kgYswgYgxCzAJ
 * BgNVBAYTAlRXMQ8wDQYDVQQIEwZUYWl3YW4xDzANBgNVBAcTBlRhaXBlaTEOMAwG
 * A1UEChMFQ0FNRU8xDDAKBgNVBAsTA0JVMTESMBAGA1UEAxMJU29mdHdhcmU0MSUw
 * IwYJKoZIhvcNAQkBFhZwZXRlcl9wYW5AY2FtZW8uY29tLnR3ggEAMA0GCSqGSIb3
 * DQEBBAUAA4GBALsBoe9EtmHNYo9hcRvK1ZyzmpFaLt/sIgIQJZifraquOKDwGKkx
 * +Bv6R6N62ip6+MMBN3iMTg6ZaohsdYdfhQk4z1yL5xm7jj5Q6hnwd2PXRaIjJh95
 * QmazChF9V5YJxncGO+fFL0hC4udaneFlml9dZg7F1I6xVByeXT6i70n/
 * -----END CERTIFICATE-----
 *
 * wan_bigpond_username=0-
 * wlan0_security=7-disable
 *
 */
static int param_load_conf(int dump, int exist_only)
{
	FILE *fd = stdin;
	char buf[__NVRAM_BUF_SIZE], *k, *v, *n;
	int i;

	bzero(buf, sizeof(buf));
	while (fgets(buf, sizeof(buf), fd) != NULL) {
		v = buf;
		if ((k = strsep(&v, "=")) == NULL) goto fmt_err;
		if ((n = strsep(&v, "-")) == NULL) goto fmt_err;
		i = atoi(n);
		i -=  strlen(v) - 1;
		/* it is impossible to save a big value */
		if (i < 0 ||  65565 < i) goto fmt_err;

		if (i > 0 && fread(v + strlen(v), 1, i, fd) != i) {
			cprintf("%s,%d### a error on read more nvram value\n",
				 __FUNCTION__, __LINE__);
			goto fmt_err;
			
		}
		v[strlen(v) - 1] = '\0';	//strip
		if (dump)
			printf("%s=%d-%s\n", k, strlen(v), v);
		else {
			//cprintf(".");
			//cprintf("XXX %s[%d] %s=%d-%s\n", __FUNCTION__, __LINE__,k, strlen(v), v);
			if (exist_only) {
				if (nvram_get(k) != NULL)
			nvram_set(k, v);
			} else
				nvram_set(k, v);
	
		}
		bzero(buf, sizeof(buf));
	}
	return 0;
fmt_err:
	fprintf(stderr, "data format error, key: %s, value=%s\n", k, v);
	return -1;
}

static int param_load_main(int argc, char *argv[])
{
	int dump = 0;
	if (argc > 1 && (strcmp(argv[1], "--help") == 0 ||
			 strcmp(argv[1], "-h") == 0))
	{
		fprintf(stderr, "use \"load -\" to dump key and value from stdin to stdout\n"
				"\twe can use this to debug/test file format.\n"
			"XXX: if nvram key \"load_key_force\" were set "
				"and value is \"1\", I will load all of keys\n "
				"\tfrom input(debug only)\n");
		return 0;
	}
	if (argc > 1 && strcmp(argv[1], "-") == 0)
		dump = 1;
	return param_load_conf(dump, NVRAM_MATCH("load_key_force", "1"));
}

int param_main(int argc, char *argv[])
{
	struct subcmd cmds[] = {
		{ "save", param_save_conf, "dump configs to stdout, system keys will be filterd"},
		{ "load", param_load_main, "load configs from stdin, then set to nvram but _NOT_ commit to, --help for more info"},
		{ NULL, NULL}
	};
	return lookup_subcmd_then_callout(argc, argv, cmds);
}
