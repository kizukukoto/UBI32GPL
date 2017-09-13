#include <stdio.h>
#include "cmds.h"
#include "nvram.h"
#include "shutils.h"
#include "libcameo.h"

#if 1
#define DEBUG_MSG(fmt, arg...)       cprintf(fmt, ##arg)
#else
#define DEBUG_MSG(fmt, arg...) 
#endif

#define SNORT_LOG	"/var/log/snort"
#define SNORT_CONF	"/tmp/snort.conf"
#define SNORT_RULEPATH	"/tmp/rules"
#define SNORT_RULEFILE	"local.rules"
#define SNORT_MTD	"/dev/mtd6"

static int ips_create_rule()
{
	char *buf = NULL, rulestr[128];
	int ips_size, ret = -1;
	FILE *mtdfp, *rulefp;

	ips_size = atoi(nvram_safe_get("ips_size"));
	buf = (char *)malloc(ips_size);

	sprintf(rulestr, "%s/%s", SNORT_RULEPATH, SNORT_RULEFILE);

	mtdfp = fopen(SNORT_MTD, "r");
	eval("mkdir", SNORT_RULEPATH);
	rulefp = fopen(rulestr, "w");

	if (mtdfp == NULL || rulefp == NULL)
		goto out;

	fread(buf, 1, ips_size, mtdfp);
	fwrite(buf, 1, ips_size, rulefp);

	ret = 0;
out:
	if (buf)
		free(buf);

	fcloseall();
	return ret;
}

static int ips_config_main()
{
	int ret = -1;
	FILE *fp = fopen(SNORT_CONF, "w");

	if (fp == NULL)
		goto out;

	fprintf(fp, "var HOME_NET any\n");
	fprintf(fp, "var EXTERNAL_NET any\n");
	fprintf(fp, "var DNS_SERVERS $HOME_NET\n");
	fprintf(fp, "var SMTP_SERVERS $HOME_NET\n");
	fprintf(fp, "var HTTP_SERVERS $HOME_NET\n");
	fprintf(fp, "var SQL_SERVERS $HOME_NET\n");
	fprintf(fp, "var TELNET_SERVERS $HOME_NET\n");
	fprintf(fp, "var SNMP_SERVERS $HOME_NET\n");
	fprintf(fp, "var HTTP_PORTS 80\n");
	fprintf(fp, "var SHELLCODE_PORTS !80\n");
	fprintf(fp, "var ORACLE_PORTS 1521\n");

	fprintf(fp, "dynamicpreprocessor directory "
			"/usr/lib/snort_dynamicpreprocessor/\n");
	fprintf(fp, "dynamicengine "
			"/usr/lib/snort_dynamicengine/libsf_engine.so\n");

	fprintf(fp, "preprocessor frag3_global: "
			"max_frags 65536\n");
	fprintf(fp, "preprocessor frag3_engine: "
			"policy first detect_anomalies\n");
	fprintf(fp, "preprocessor stream5_global: "
			"max_tcp 8192, track_tcp yes, track_udp no\n");
	fprintf(fp, "preprocessor stream5_tcp: "
			"policy first, use_static_footprint_sizes\n");
	fprintf(fp, "preprocessor http_inspect: "
			"global iis_unicode_map unicode.map 1252\n");
	fprintf(fp, "preprocessor http_inspect_server: "
			"server default profile all ports "
			"{ 80 8080 8180 } oversize_dir_length 500\n");
	fprintf(fp, "preprocessor rpc_decode: 111 32771\n");
	fprintf(fp, "preprocessor bo\n");

	fprintf(fp, "preprocessor ftp_telnet: global \\\n"
			"   encrypted_traffic yes \\\n"
			"   inspection_type stateful\n\n");

	fprintf(fp, "preprocessor ftp_telnet_protocol: telnet \\\n"
			"   normalize \\\n"
			"   ayt_attack_thresh 200\n\n");

	fprintf(fp, "preprocessor ftp_telnet_protocol: ftp server default \\\n"
			"   def_max_param_len 100 \\\n"
			"   alt_max_param_len 200 { CWD } \\\n"
			"   cmd_validity MODE < char ASBCZ > \\\n"
			"   cmd_validity MDTM < [ date nnnnnnnnnnnnnn[.n[n[n]]] ] string > \\\n"
			"   chk_str_fmt { USER PASS RNFR RNTO SITE MKD } \\\n"
			"   telnet_cmds yes \\\n"
			"   data_chan\n\n");

	fprintf(fp, "preprocessor ftp_telnet_protocol: ftp client default \\\n"
			"   max_resp_len 256 \\\n"
			"   bounce yes \\\n"
			"   telnet_cmds yes\n\n");

	fprintf(fp, "preprocessor smtp: \\\n"
			"  ports { 25 } \\\n"
			"  inspection_type stateful \\\n"
			"  normalize cmds \\\n"
			"  normalize_cmds { EXPN VRFY RCPT } \\\n"
			"  alt_max_command_line_len 260 { MAIL } \\\n"
			"  alt_max_command_line_len 300 { RCPT } \\\n"
			"  alt_max_command_line_len 500 { HELP HELO ETRN } \\\n"
			"  alt_max_command_line_len 255 { EXPN VRFY }\n\n");

	fprintf(fp, "preprocessor sfportscan: proto  { all } \\\n"
			"  memcap { 10000000 } \\\n"
			"  sense_level { low }\n\n");

	fprintf(fp, "preprocessor dcerpc: \\\n"
			"  autodetect \\\n"
			"  max_frag_size 3000 \\\n"
			"  memcap 100000\n\n");

	fprintf(fp, "preprocessor dns: \\\n"
			"  ports { 53 } \\\n"
			"  enable_rdata_overflow\n\n");

	fprintf(fp, "output log_tcpdump: tcpdump.log\n\n");
	fprintf(fp, "include %s/%s\n", SNORT_RULEPATH, SNORT_RULEFILE);

	fclose(fp);

	eval("cp", "/etc/unicode.map", "/tmp");

	ret = 0;
out:
	return ret;
}

static int ips_start_main(int argc, char *argv[])
{
	int ret = -1;
	char *ips_enable = nvram_safe_get("ips_enable");
	char *ips_size = nvram_safe_get("ips_size");
	char cmds[256];

	/* @argv[1] = WAN interface */
	if (argc != 2 || ips_enable == NULL ||
		strncmp(ips_enable, "1", 1) != 0 ||
		strncmp(ips_size, "0", 1) == 0)
		goto out;

	ips_config_main();
	ips_create_rule();

	eval("mkdir", SNORT_LOG);
	sprintf(cmds, "snort -i %s -d -l %s -c %s -s &", argv[1], SNORT_LOG, SNORT_CONF);
	ret = system(cmds);
out:
	return ret;
}

static int ips_stop_main(int argc, char *argv[])
{
	int ret = -1;

	eval("killall", "-9", "snort");

	ret = 0;
out:
	return ret;
}

int ips_main(int argc, char *argv[])
{
	struct subcmd cmds[] = {
		{ "start", ips_start_main},
		{ "stop", ips_stop_main},
		{ "config", ips_create_rule},
		{ NULL, NULL}
	};

	return lookup_subcmd_then_callout(argc, argv, cmds);
}
