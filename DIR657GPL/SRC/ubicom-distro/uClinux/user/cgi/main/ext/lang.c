#include <stdio.h>
#include <stdio_ext.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <unistd.h>
#include <signal.h>
#include <regex.h>

#include "ssi.h"
#include "log.h"
#include "mime.h"
#include "lang.h"
#include "partition.h"

/*The function extern from mtd.c */
//extern int get_mtd_char_device(const char *name, char *buf);
//extern int get_mtd_size(const char *name);


static int lang_pack_upload(const char *fdesc,const char *mtd)
{
	char cmds[128];

	//sprintf(cmds, "cat %s > %s", fdesc, LP_MTD_BLOCK);
	sprintf(cmds, "cat %s > %s", fdesc, mtd);
	return system(cmds);
}

static int do_upload_lang(struct mime_desc *dc)
{
	int mtd_len,mtd_num,fr, ret = -1;
	char buf[1024], upload_temp[] = "/tmp/uploadXXXXXX",mtd[128];
	FILE *fp_temp = NULL;

	/* check id for file , ex: <input type=file id="lp_file" name="lp_file"> */
	if (strcmp("lp_file", dc->name) != 0)
		goto leave;
	if (!mktemp(upload_temp))
		goto out;
	if ((fp_temp = fopen(upload_temp, "wb")) == NULL)
		goto out;

	while ((fr = fread(buf, 1, sizeof(buf), dc->fd)) == sizeof(buf))
		fwrite(buf, 1, fr, fp_temp);
	if (fr > 0)
		fwrite(buf, 1, fr, fp_temp);
	fclose(fp_temp);


	/* check mtd size and mtd number */
	if ((mtd_num=get_mtd_char_device("language_pack", mtd)) == -1)
		goto out;
	if ((mtd_len = get_mtd_size("language_pack")) == -1)
		goto out;
        if (dc->size > mtd_len)
                goto out;
	sprintf(mtd,"/dev/mtdblock%d",mtd_num);

	ret = 0;
out:
	if (ret < 0) {
		setenv("html_response_return_page", "tools_system.asp", 1);
		setenv("html_response_error_message", "The language packet reload is failure.", 1);
		setenv("html_response_page", "error.asp", 1);
	} else if (lang_pack_verify(upload_temp) == -1) {
		unlink(upload_temp);
		setenv("html_response_return_page", "tools_system.asp", 1);
		setenv("html_response_error_message", "The language packet verify is failure.", 1);
		setenv("html_response_page", "error.asp", 1);
	} else {
		pid_t pid;

		lang_pack_upload(upload_temp,mtd);
		lang_pack_update_info(upload_temp);
		unlink(upload_temp);
		system("lang init");
#if NOMMU
		if ((pid = vfork()) < 0) {
#else
		if ((pid = fork()) < 0) {
#endif

			ret = pid;
			goto out;
		}

		if (pid == 0) {
			sleep(2);
			exit(0);
		} else {
			setenv("html_response_return_page", "tools_firmw.asp", 1);
			setenv("html_response_message", "The language pack had reload.", 1);
			setenv("result_timer", "10", 1);
			setenv("html_response_page", "count_down.asp", 1);
		}
	}

leave:
	return ret;
}

static int do_clear_lang()
{
	lang_pack_erase();
	lang_pack_umount();
	unlink(LP_MTD_MOUNT);
	system("touch /www/lang.js");
	setenv("html_response_return_page", "tools_firmw.asp", 1);
	setenv("html_response_message", "The language pack had reload.", 1);
	setenv("result_timer", "10", 1);
	setenv("html_response_page", "count_down.asp", 1);
}

char *upload_lang()
{
	register_mime_handler(do_upload_lang);
	http_post_upload(stdin);

	return getenv("html_response_page");
}

/* Matt add - 2010/11/10 */
char *auto_upload_lang(char *file)
{
	char *t = "error.asp";
	FILE *fp;
	struct mime_desc mdec, *dc;
	struct stat buf;
	char cmd[128];
	int fd;
	
	cprintf("file = %s\n",file);
	if((fp = fopen(file, "r")) == NULL) {
		setenv("html_response_error_message", "The upload file open fail.", 1);
		return t;
	}

	bzero(&mdec, sizeof(mdec));
	dc = &mdec;
	dc->fd = fp;
	strcpy(dc->name, "lp_file");
	strcpy(dc->tpath, file);

	fd = open(file, O_RDONLY);
	fstat(fd,&buf);
	dc->size = buf.st_size;
	close(fd);

	do_upload_lang(dc);

	memset(cmd, '\0', sizeof(cmd));
	sprintf(cmd, "/bin/rm -f %s", file);
	system(cmd);

	//register_mime_handler(do_upload_lang);
	//http_post_upload(stdin);
	//t = getenv("html_response_page");

	return getenv("html_response_page");
}
/*--------------------------*/

char *clear_lang()
{
	register_mime_handler(do_clear_lang);
        http_post_upload(stdin);

        return getenv("html_response_page");
}
