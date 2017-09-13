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

#include "ssi.h"
#include "log.h"
#include "mtd.h"


/*
 * Define LINUX_NVRAM to select BCM old style or
 * Eagles/UBICOM style
 */
#ifdef LINUX_NVRAM

#include "mime.h"
#define MAX_STRLEN_HWID		28

/*
 * Hardware ID format:
 *	eg:
 *         DIR652A1: 00MB97-AR9287-RT-09112011-00(28 byptes)
 *         DIR665A1:     MVL-88F6281-RT-090520-00(24 bytes from project.txt)
 *         	       NAMVL-88F6281-RT-090520-00(26 bytes from tail -c 30 F/W)
 *
 * We can use hexdump to check id such as:
 * [peter@localhost image]$ tail -c 30 DIR665_A1_mv_ap95_image.squashfs.bin |hexdump -C
 * 00000000  00 00 00 00 4e 41 4d 56  4c 2d 38 38 46 36 32 38  |....NAMVL-88F628|
 * 00000010  31 2d 52 54 2d 30 39 30  35 32 30 2d 30 30        |1-RT-090520-00|
 * 0000001e
 */

void dump_hex(char *p, int len)
{
	int i, off = 0;
	char buf[60 * 3 + 1];
	
	if (len > 60)
		len = 60;
	memset(buf, 0, 60 *3 +1);
	for (i = 0; i < len; i++)
		off += sprintf(buf + off, "%.2X ", p[i]);
	cprintf("dump: [%s]\n", buf);
}

static int find_hwid(FILE *fd, int id_len, char *out)
{
	char buf[128], *p;
	int i, n;

	bzero(buf, sizeof(buf));
	/* read from tail of file */
	if (fseek(fd, 0 - id_len, SEEK_END) != 0)
		goto sys_err;
	if ((n = fread(buf, 1, id_len, fd)) != id_len)
		goto sys_err;

	rewind(fd);
	/* 
 	 * So far, I just implement for DIR665A1(MVL) and DIR652A1(IP7K).
	 * eg:
	 *  DIR652A1: 00MB97-AR9287-RT-09112011-00(28 byptes)
	 *  DIR665A1:     MVL-88F6281-RT-090520-00(24 bytes from project.txt)
	 *	        NAMVL-88F6281-RT-090520-00(26 bytes from tail -c 30 F/W)
	 *
 	 * 						2010/06/10
	 */

	dump_hex(buf, n);

	/* skip none-printable char from buffer. */
	for (p = buf, i = 0; i < n && isprint(*p) == 0; p++, i++)
		;
	if (i == n)
		return -1; /* no printable ascii, illegal format, hackered! */
	n = n - i;
	memcpy(out, p, n);
	out[n] = '\0';
	return n;
sys_err:
	perror(__FUNCTION__);
	return -1;
}

static int is_valid_hwid(FILE *fd)
{
	char *sys_fw_hwid = nvram_safe_get("sys_fw_hwid");
	char bid[MAX_STRLEN_HWID + 1];
	int n;

	if ((n = find_hwid(fd, MAX_STRLEN_HWID, bid)) == -1)
		goto out;

	cprintf("[%s]\n", bid);
	cprintf("sys_fw_hwid [%s]\n", sys_fw_hwid);
	dump_hex(bid, n);

	/* FIXME: Check Country ID now? */
	if (strcmp(bid, sys_fw_hwid) != 0)
		goto out;

	return 1;
out:
	return 0;
}
static int upgrade_allow; // FIXME: i think it meaningless

static int is_loader(struct mime_desc *dc)
{
	/* TODO: we have no magic number to check loader or firmware
	 *  juset check by size ^_^
	 * */
	return dc->size < 0x100000;
}

static int fn_mime_fwupdate(struct mime_desc *dc)
{
	char *c;
	int rev = -1;

	if (strcmp("file", dc->name) != 0)
		goto out;

	cprintf("XXX Begin firmware upgrade..., size(%d)\n", dc->size);

	if (!is_valid_hwid(dc->fd))
		goto fw_err;

	rev = stream2mtd(dc, dc->size, is_loader(dc) ? "bootloader":"upgrade");

	if (rev == -1) {
		upgrade_allow = FALSE;
		cprintf("upgrade denied %s\n", (upgrade_allow == TRUE)?"TRUE":"FALSE");
	}

	cprintf("Done, reboot now\n");

out:
	return rev;
fw_err:
	upgrade_allow = FALSE;
	cprintf("%s: invalid Hardware ID\n", __FUNCTION__);
	return -1;
}


char *update_firmware()
{
	char *t = "error.asp";
	struct mime_opt mopt = { .name = "file" };
	pid_t pid;

	upgrade_allow = TRUE;  // global variable
	register_mime_handler(fn_mime_fwupdate);
	http_post_upload_ext(stdin, &mopt);
	//http_post_upload(stdin);

	cprintf("upgrade_allow: %s\n", (upgrade_allow == TRUE)?"TRUE":"FALSE");

	if (upgrade_allow == FALSE) {
		t = "error.asp";
		cprintf("deny upgrade, return %s page.\n", t);
		setenv("html_response_page", "tools_firmw.asp", 1);
		setenv("html_response_error_message", "The hardware ID checking fail ", 1);
		setenv("html_response_return_page", "tools_firmw.asp", 1);
		setenv("result_timer", "5", 1);

		return t;
	}
	t = "count_down.asp";
	cprintf("allow upgrade, return %s page.\n", t);
	setenv("html_response_return_page", "tools_firmw.asp", 1);
	setenv("html_response_message", "The firmware upgrading ", 1);
	setenv("result_timer", "180", 1);
	cprintf("XXXXXX %s %d: return %s\n", __FUNCTION__, __LINE__, t);
	return t;
}


/* Matt add - 2010/11/09 */
char *auto_update_firmware(char *file)
{
	char *t = "error.asp";
	FILE *fp;
	struct mime_opt mopt = { .name = "file" };
	struct mime_desc mdec, *dc;
	struct stat buf;
	char cmd[128];
	int fd;
	
	cprintf("file = %s\n",file);
	if((fp = fopen(file, "r")) == NULL) {
		setenv("html_response_error_message", "The update file open fail.", 1);
		goto out;
	}

	upgrade_allow = TRUE;  // global variable

	bzero(&mdec, sizeof(mdec));
	dc = &mdec;
	dc->fd = fp;
	strcpy(dc->name, "file");
	strcpy(dc->tpath, file);

	fd = open(file, O_RDONLY);
	fstat(fd,&buf);
	dc->size = buf.st_size;
	close(fd);

	dc->opt = &mopt;
	
	fn_mime_fwupdate(dc);
	//register_mime_handler(fn_mime_fwupdate);
	//http_post_upload_ext(fp, &mopt);
	
	cprintf("upgrade_allow: %s\n", (upgrade_allow == TRUE)?"TRUE":"FALSE");

	fclose(fp);
	if (upgrade_allow == FALSE) {
		t = "error.asp";
		memset(cmd, '\0', sizeof(cmd));
		sprintf(cmd, "/bin/rm -f %s", file);
		system(cmd);
		cprintf("deny upgrade, return %s page.\n", t);
		setenv("html_response_page", "tools_firmw.asp", 1);
		setenv("html_response_error_message", "The hardware ID checking fail.", 1);
		setenv("html_response_return_page", "tools_firmw.asp", 1);
		setenv("result_timer", "5", 1);

		return t;
	}

	t = "count_down.asp";
	cprintf("allow upgrade, return %s page.\n", t);
	setenv("html_response_return_page", "tools_firmw.asp", 1);
	setenv("html_response_message", "The firmware upgrading ", 1);
	setenv("result_timer", "180", 1);
	cprintf("XXXXXX %s %d: return %s\n", __FUNCTION__, __LINE__, t);
out:
	return t;

	
}
/*--------------------------*/

#else

#define MIN(a,b) (((a)<(b))?(a):(b))

/*
 * fwrite() with automatic retry on syscall interrupt
 * @param	ptr	location to read from
 * @param	size	size of each element of data
 * @param	nmemb	number of elements
 * @param	stream	file stream
 * @return	number of items successfully written
 */
static int
safe_fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream)
{
	size_t ret = 0;

	do {
		clearerr(stream);
		ret += fwrite((char *)ptr + (ret * size), size, nmemb - ret, stream);
	} while (ret < nmemb && ferror(stream) && errno == EINTR);

	return ret;
}
char *update_firmware()
{
	int i, len, ret = -1, sig, sz;
	char *s, *t = "error.asp";
	char tmp[1024];
	long flags = -1;
	size_t rcnt, wcnt;
	char upload_fifo[] = "/tmp/uploadXXXXXX";
	FILE *fifo = NULL;
	char *write_argv[] = { "write", upload_fifo, "linux", NULL };
	pid_t pid;

	/* FIXME bigger then setting size firmware upgrade bug (Note by Tony at 2006/1017) */
	/* XXX Now firmware Size setting is 0x7d000,please reference src/include/trxhdr.h  */
	SYSLOG("update firmware");
	system("/usr/sbin/nvram set firmware_upgrading=\"1\"");
	cprintf("Begin firmware upgrade /usr/sbin/nvram set firmware_upgrading=\"1\"\n");
	if (!mktemp(upload_fifo))
		goto out;
			
	if (mkfifo(upload_fifo, S_IRWXU) < 0)
		goto out;

	pid = fork();
	
	if (pid < 0) { // fork failure
		ret = pid;
		goto out;
	}
	
	if (pid == 0) { //Child
                /* Reset signal handlers set for parent process */
                for (sig = 0; sig < (_NSIG-1); sig++)
	                        signal(sig, SIG_DFL);

		i = open("/dev/null", O_RDWR);
		if (i < 0)
			goto child_fail;

		dup2(i, 0);
		dup2(i, 1);
		dup2(i, 2);
		close(i);
		
		setenv("PATH", "/sbin:/bin:/usr/sbin:/usr/bin", 1);
		execvp(write_argv[0], write_argv);
child_fail:
		exit(-1);
	}

	if ((fifo = fopen(upload_fifo, "w")) == NULL) {
		cprintf("FIFO open error\n");
		goto out;
	}
	
	s = getenv("CONTENT_LENGTH");
	if (s == NULL) {
		cprintf("CONTENT_LENGTH is too big\n");
		goto out;
	}

	len = atoi(s);

	/*
	 * XXX: I use block IO to stdin, so EAGAIN should not occur any more.
	 * */
 	
	if ((flags = fcntl(fileno(stdin), F_GETFL)) < 0 ||
			fcntl(fileno(stdin), F_SETFL, flags | O_NONBLOCK) < 0) {
		cprintf("stdin is blocked.\n");
		goto out;
	}
	
	cprintf("Parser Content-Disposition[%d] bytes\n", len);
	/* Look for our part */
	while (len > 0) {
		if (fgets(tmp, MIN(len + 1, sizeof(tmp)), stdin) == NULL) {
			cprintf("fgets = NULL(len = %d,%s):%s\n",
				len, feof(stdin)? "EOF":"Not empty",
				strerror(errno));
			if (errno == EAGAIN)
				continue;
			goto out;
		}
		len -= strlen(tmp);
		if (!strncasecmp(tmp, "Content-Disposition:", 20) && 
				strstr(tmp, "name=\"file\""))
			break;
	}

	cprintf("JUMP to BODY\n");
	/* Skip boundary and headers */
	while (len > 0) {
		if (fgets(tmp, MIN(len + 1, sizeof(tmp)), stdin) == NULL) {
			if (errno == EAGAIN)
				continue;
			goto out;
		}
		len -= strlen(tmp);
		if (!strcmp(tmp, "\n") || !strcmp(tmp, "\r\n"))
			break;
	}

	cprintf("linux.trx length[%d]\n", len);
	sz = len;
	while (len > 0){
		if ((rcnt = fread(tmp, 1, sizeof(tmp), stdin)) <= 0) {
			if (errno == EAGAIN)
				continue;
			cprintf("fread: %s\n", strerror(errno));
		}
		cprintf("\r:%%%d,C%d", ((sz - len)*100)/sz, rcnt);
		if (rcnt > 0) {
			if ((wcnt = safe_fwrite(tmp, 1, rcnt, fifo)) != rcnt) {
				cprintf("fwrite: %s\n", strerror(errno));
				goto out;
			}
		}
		cprintf("w:%d|", wcnt);
		len -= rcnt;
	}
	fclose(fifo);
	fifo = NULL;
	cprintf("\nWaitting for write into flash\n");
	waitpid(pid, &ret, 0);
	ret = WIFEXITED(ret) ? WEXITSTATUS(ret):-1;
	cprintf("done(%d).\n", ret);
	
out:
	cprintf("End firmware upgrade(%d) /usr/sbin/nvram set firmware_upgrading=\"0\"\n", ret);
	system("/usr/sbin/nvram set firmware_upgrading=\"0\"");

	if (ret){  	/* Fail process  */
		SYSLOG("Firmware Upgrade Fail");
		setenv("html_response_return_page", 
				"tools_firmw.asp", 1);
		setenv("html_response_message", 
				"The firmware upgrad FAIL!", 1);
		if (fifo)
			fclose(fifo);
		unlink(upload_fifo);
		return t;
	} else {	/* Success process  */
		pid = fork();
		
		if (pid < 0) { // fork failure
			ret = pid;
			goto out;
		}
		
		if (pid == 0) { //Child
			sleep(2);
			//update_record("restore_defaults", "1"); disable from 1.20B21
			nvram_commit();
			kill(1, SIGTERM);
			exit(0);
		} else {  	//parent
			SYSLOG("Firmware Upgrade success");
			t = "count_down.asp";
			setenv("html_response_return_page", 
					"tools_firmw.asp", 1);
			setenv("html_response_message", 
					"The firmware upgrading ", 1);
			setenv("result_timer", "30", 1);
			return t;
		}
	}
}

#endif //LINUX_NVRAM
