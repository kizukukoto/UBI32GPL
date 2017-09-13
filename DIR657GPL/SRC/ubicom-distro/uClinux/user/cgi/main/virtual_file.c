#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <sys/wait.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>

#include "virtual_file.h"
#include "firmware.h"
#include "querys.h"
#include "ssc.h"
#include "log.h"
#include "mime.h"
#include "rcctrl.h"
#include "debug.h"

#define HWID_LENGTH 128
#define DEFAULT_HWID "D-LINK_HWID_CONFIG"

extern int base64_encode(char *, char *, int);
extern int base64_decode(char *, char *, int , bool);
extern void __do_reboot(struct ssc_s *obj);




#if 0
u_int sum_byte(unsigned char* buf, long len)
{
	unsigned long i, sum = 0;

	for (i = 0; i < len; i++)
		sum += buf[i];

	return sum;
}
#endif

static int check_valid_hwid(FILE *fd)
{
	char sys_fw_hwid[HWID_LENGTH];
	char buf[HWID_LENGTH];
	char base64[128];
	bzero(sys_fw_hwid, HWID_LENGTH);
	bzero(buf,HWID_LENGTH);
	bzero(buf,sizeof(base64));

        sprintf(sys_fw_hwid, "%s", nvram_safe_get("sys_fw_hwid"));
        if(!strlen(sys_fw_hwid))
                sprintf(sys_fw_hwid, "%s", DEFAULT_HWID);


	fgets(buf,sizeof(buf),fd);
	base64_decode(base64,buf,strlen(buf),true);
	cprintf("XXX %s[%d] buf:%s base64:%s sys_fw_hwid:%s XX\n", __FUNCTION__, __LINE__, buf,base64,sys_fw_hwid);
	if (strncmp(base64, sys_fw_hwid, strlen(sys_fw_hwid)) != 0)
		goto out;
	
	fflush(stdin);
	return 1;
out:
	return 0;
	
}

static int wrap_config_version_save_load(FILE *stream, int save)
{
	char *m, *n;
	char buf[128];
	char b64[128];
	m = nvram_get("sys_config_version_major");
	n = nvram_get("sys_config_version_minor");

	if (m == NULL || n == NULL)
		goto pass;

	if (save) {
		
		bzero(b64, sizeof(b64));
		
		sprintf(buf, "config_ver:%s.%s", m, n);
		base64_encode(buf, b64, strlen(buf));
		fprintf(stream, "%s\n", b64);
	} else {
		char *p, *s;
		int new_major, new_minor;
		if (fgets(buf, sizeof(buf), stream) == NULL)
			return -1;
		base64_decode(b64, buf, strlen(buf), true);
		p = b64;
		cprintf("confi_ver:%s, i am :%s.%s\n", p, m, n);
		if ((s = strsep(&p, ":")) == NULL)
			goto pass;	/* pass if without config version control */
		if (strcmp("config_ver", s) != 0)
			goto err_fmt;
		if ((s = strsep(&p, ".")) == NULL)
			goto err_fmt;
		new_major = atoi(s);
		new_minor = atoi(p);
		if (new_major != atoi(m))
			goto err_major;
		/* TODO:
 		 * pass without check minor, and only update those keys where
 		 * exist in /etc/nvram.defualt.
 		 * */
	}
pass:
	return 0;
err_major:
err_fmt:
	return -1;
}
int do_upload_conf(struct mime_desc *dc)
{

#define BASE64_BUF_SIZE		2732
/* =2048*4/3, if use large key, such as CA in config */
	char tmp[BASE64_BUF_SIZE];
	FILE *fp;
	int line = 0, ret = -1;
	pid_t pid;
	char base64[BASE64_BUF_SIZE];
	int base64_enable; /* DIR-130/330 config file do not support encode with base64 */ 

	if (strcmp("file", dc->name) != 0)
		return 0;

	/* DIR130 old style use /sbin/param, use cli in newer project */
	
	if(access("/sbin/param", X_OK) == 0) {
		fp = popen("/sbin/param load", "w");
		base64_enable = 0;

	} else {
		if(!check_valid_hwid(dc->fd))
			goto out;
		/* 
 		 * TODO: not implemnet nor define if config version
 		 * dismatch. I just ignore it.
 		 * */
		wrap_config_version_save_load(dc->fd, 0);		
		fp = popen("/bin/cli sys param load", "w");
		base64_enable = 1;
	}

	if (!fp)
		goto out;

	while (fgets(tmp, sizeof(tmp), dc->fd) != NULL ) {

		if (base64_enable) {
			base64_decode(base64,tmp,strlen(tmp),true);
			sprintf(tmp,"%s\n",base64);
		}

		fputs(tmp, fp);
		bzero(base64,sizeof(base64));
		bzero(tmp,sizeof(tmp));
	}

	ret = pclose(fp);
	//cprintf("pclose:[%s]\n", strerror(errno));
	cprintf("pclosed():%d WIFEXITED(ret)=%d to %s:%d\n", ret, WIFEXITED(ret),
		WIFEXITED(ret) ? "normal":"abort", WEXITSTATUS(ret));
	/* ret always be -1 and WIFEXITED(ret) always return 0, because httpd.c don't
	 * care all forked processes return value, which use signal(SIGCHLD, SIG_IGN);
	 * before fork. So, waitpid in httpd.c always get -1, but not means child
	 * process return -1.
	 */
	//ret = (WIFEXITED(ret)) ? WEXITSTATUS(ret) : -1;
	//ret = (ret == 0) ? 0 : -1;
	//cprintf("ret:%d\n", ret);
	ret = 0;
out:
	if ( ret < 0 ){	/* Fail process  */
		setenv("html_response_return_page", 
				"tools_system.asp", 1);
		setenv("html_response_error_message", 
				"The configure file reload is failure.", 1);
		setenv("html_response_page", "error.asp", 1);
		return 0;
	} else {	/* Success process  */
		/* XXX: this is new style, remind in DIR130/330 */
#if 1
		nvram_commit();
		__do_reboot(NULL);
		setenv("html_response_return_page", 
				"tools_system.asp", 1);
		setenv("html_response_message", 
				"The configure file had reload.", 1);
		setenv("result_timer", "40", 1);
		setenv("html_response_page", "count_down.asp", 1);
		return 0;
#else
		/* TODO: remove me if unsed */

		pid = fork();

		if (pid < 0) { // fork failure
			ret = pid;
			goto out;
		}

		if (pid == 0) { //Child
			sleep(2);
			rc_term();
			exit(0);
		} else {        //parent
			setenv("html_response_return_page", 
					"tools_system.asp", 1);
			setenv("html_response_message", 
					"The configure file had reload.", 1);
			setenv("result_timer", "40", 1);
			setenv("html_response_page", "count_down.asp", 1);
			return 0;
		}
#endif
	}
}

extern int do_putquerystring_env(struct mime_desc *dc);

char *upload_conf() {
	register_mime_handler(do_upload_conf);
	//register_mime_handler(do_putquerystring_env);
	http_post_upload(stdin);
	cprintf("Upoad conf return:%s\n", getenv("html_response_page"));
	return getenv("html_response_page");
}


static int build_config_header(FILE *stream)
{
	char sys_fw_hwid[HWID_LENGTH];
	char base64[128];
	
	sprintf(sys_fw_hwid, "%s", nvram_safe_get("sys_fw_hwid"));
	if (!strlen(sys_fw_hwid))
		sprintf(sys_fw_hwid, "%s", DEFAULT_HWID);
	bzero(base64, sizeof(base64));

	//sprintf(tmp,"%s\n", sys_fw_hwid);
	base64_encode(sys_fw_hwid, base64, strlen(sys_fw_hwid));
	fprintf(stream,"%s\n", base64);
	wrap_config_version_save_load(stream, 1);
	fflush(stream);
	return 0;
}

char *save_conf()
{
	char *ret = "error.asp";
	char tmp[128], value[256];
	

	bzero(tmp, sizeof(tmp));

	query_vars("model_name", value, sizeof(value));
	fputs("Cache-Control: no-cache\r\n", stdout);
	fputs("Pragma: no-cache\r\n", stdout);
	fputs("Expires: 0\r\n", stdout);
	fputs("Content-Type: application/download\r\n", stdout);
	sprintf(tmp, "Content-Disposition: attachment ; filename=%s_config.bin\n\n", value);
	fputs(tmp, stdout);
#if 0	/* for DIR-730 */
	if ((fd = popen("/bin/cli sys param save", "r")) == NULL)
		goto out;

	while(!feof(fd)) {
		bzero(key, sizeof(key));
		if (fgets(key, sizeof(key), fd) != NULL)
			fputs(key, stdout);
	}

	cprintf("pclose(fd) = %d\n", pclose(fd));
out:
	fflush(stdout);
#else
	fflush(stdout);
	/* DIR130 old style use /sbin/param, use cli in newer project */
	if (access("/sbin/param", X_OK) == 0)
		system("/sbin/param save");
	else {
		build_config_header(stdout);
		system("cli sys param save");
	}
#endif
out:
	return ret;
}

int *save_log()
{
	system("/bin/log_page save");
	return 0;
}

char *do_save_cert()
{
	int idx = 0;
	char type[12], *ptr = NULL, *pub = NULL, *pri = NULL;
	char tmp[2048], key[64], value[4096];

	memset(type, '\0', sizeof(type));
	memset(tmp, '\0', sizeof(tmp));
	memset(key, '\0', sizeof(key));
	memset(value, '\0', sizeof(value));

	if (getenv("ca_idx") == NULL) {
		cprintf("Can not get environment ca_idx key\n");
		goto out;
	} else
		idx = atoi(getenv("ca_idx"));
	
	if (getenv("ca_type") == NULL) {
		cprintf("Can not get environment type key\n");
		goto out;
	} else
		sprintf(type, "%s", getenv("ca_type"));
	
	sprintf(key, "x509_ca_%d", idx);
	sprintf(value, "%s", nvram_safe_get(key));
	cprintf("=========================================\n");
	cprintf("idx :: %d\n", idx);
	cprintf("type :: %s\n", type);
	cprintf("key :: %s\n", key);
	cprintf("value :: %s\n", value);
	cprintf("=========================================\n");

	ptr = value;
	pri = strsep(&ptr, ",");
	pub = ptr;

	fputs("Cache-Control: no-cache\r\n", stdout);
	fputs("Pragma: no-cache\r\n", stdout);
	fputs("Expires: 0\r\n", stdout);
	fputs("Content-Type: application/download\r\n", stdout);
	sprintf(tmp, "Content-Disposition: attachment ; filename=%s%d.pem\n\n", type, idx);
	fputs(tmp, stdout);
	fflush(stdout);

	if (strcmp(type, "public") == 0)
		sprintf(tmp, "%s", pub);
	else
		sprintf(tmp, "%s", pri);
	fputs(tmp, stdout);
	fflush(stdout);
out:
	return NULL;
}
