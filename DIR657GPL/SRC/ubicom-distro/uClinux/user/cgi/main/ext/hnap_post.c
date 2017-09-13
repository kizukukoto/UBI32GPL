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

#include "mime.h"
#include "hnap.h"
#if 0
#define MIN(a,b) (((a)<(b))?(a):(b))

static int do_hnap_pipe_io(char *raw)
{
	FILE *fp = fopen(HNAP_RESPONSE_PAGE, "w");
	char tmp[4096];
	int fd, ret = -1;

	unlink(HNAP_CGI_FIFO_FROM_MISC);
	unlink(HNAP_CGI_FIFO_TO_MISC);
	mkfifo(HNAP_CGI_FIFO_TO_MISC, 0777);

	system(HNAP_CGI_MISC);
	fd = open(HNAP_CGI_FIFO_TO_MISC, O_WRONLY);
	write(fd, raw, strlen(raw));
	close(fd);

	mkfifo(HNAP_CGI_FIFO_FROM_MISC, 0777);
	fd = open(HNAP_CGI_FIFO_FROM_MISC, O_RDONLY);
	read(fd, tmp, sizeof(tmp) - 1);
	close(fd);

	if (fp == NULL)
		goto out;

	fputs(tmp, fp);
	fclose(fp);

	ret = 0;
out:
	return ret;
}
#endif
char *do_hnap_post()
{
	char *cmd = "/bin/hnap_main";

	execv(cmd, (char *[]){cmd, NULL});
	return NULL;
#if 0
	int i, sz, sig, len, rcnt, ret = -1;
	long flags = -1;
	char *s, tmp[128], total_content[4096];
	pid_t pid;

#if NOMMU
	pid = vfork();
#else
	pid = fork();
#endif

	if (pid < 0) { /* fork failure */
		ret = pid;
		goto out;
	}

	if (pid == 0) { /* Child */
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
child_fail:
		exit(-1);
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

	bzero(total_content, sizeof(total_content));
	cprintf("Parser Content-Disposition[%d] bytes\n", len);
	/* Look for our part */
	while (len > 0) {
		if (fgets(tmp, MIN(len + 1, sizeof(tmp) - 1), stdin) == NULL) {
			if (errno == EAGAIN)
				continue;
			goto out;
		}
		strcat(total_content, tmp);
		len -= strlen(tmp);
	}

	do_hnap_pipe_io(total_content);
out:
	return HNAP_RESPONSE_FILE;
#endif
}
