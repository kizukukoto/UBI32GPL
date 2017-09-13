#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <time.h>
#include <ctype.h>
#include <errno.h>

#ifdef X86
#include "../../include/hnap.h"
#else
#include "hnap.h"
#include "debug.h"
#endif

extern int do_xml_main(char *);

static int do_xml_tag_verify(
	const char *xraw,
	const char *tag,
	char **tstart)
{
	/* ret is contents size */
	int ret = -1;
	/* XXX: assuming tag size 128 bytes */
	char *pb, *pbe, *pe, bxt[128], bxt2[128], ext[128];

	sprintf(bxt, "<%s>", tag);
	sprintf(bxt2, "<%s ", tag);
	sprintf(ext, "</%s>", tag);

	if ((pb = strstr(xraw, bxt)) == NULL && (pb = strstr(xraw, bxt2)) == NULL)
		goto out;
	if ((pbe = strstr(pb, "/>")) == NULL && (pbe = strstr(pb, ">")) == NULL)
		goto out;
	if (*pbe == '>' && (pe = strstr(pbe + 1, ext)) == NULL)
		goto out;
	if (*pbe == '/')	/* End of tag is <..../> */
		ret = 0;
	else {
		*tstart = pbe + 1;
		ret = pe - *tstart;
	}
out:
	return ret;
}

void do_get_element_value(const char *xraw, char *value, char *tag)
{

        int size;
        char *ps = NULL,buf[128]={};

        if (tag == NULL )
                cprintf("XXX not found tag XXX XXX\n");

        else if ((size = do_xml_tag_verify(xraw, tag, &ps)) >= 0) {
                memcpy(buf, ps, size);
                memcpy(value, buf, strlen(buf));
                value[size] = '\0';
        }
}

int do_xml_mapping(const char *xraw, hnap_entry_s *p)
{
	int size, ret = -1;
	char *ps = NULL, buf[2048];		/* XXX: assuming tag contents 2KB */

	for (; p && p->fn; p++) {
		bzero(buf, sizeof(buf));

		if (p->tag == NULL && ret == -1) {	/* unknown call */
			ret = p->fn(p->key, buf);
			break;
		} else if ((size = do_xml_tag_verify(xraw, p->tag, &ps)) >= 0) {
			strncpy(buf, ps, size);
			ret = p->fn(p->key, buf);
		}
	}

	return ret;
}

int do_xml_response(const char *res)
{
#if 0
	int fd;

	mkfifo(HNAP_CGI_FIFO_FROM_MISC, 0777);
	fd = open(HNAP_CGI_FIFO_FROM_MISC, O_WRONLY);
	write(fd, res, strlen(res));
	close(fd);
#endif
	fputs(res, stdout);
	return 0;
}

#define MIN(a,b) (((a)<(b))?(a):(b))
int hnap_main(int argc, char *argv[])
{
#if 0
	int fd;
	char tmp[4096];

	mkfifo(HNAP_CGI_FIFO_TO_MISC, 0777);
	fd = open(HNAP_CGI_FIFO_TO_MISC, O_RDONLY );
	read(fd, tmp, sizeof(tmp) - 1);
	close(fd);
#endif
	int len, ret = -1;
	long flags = -1;
	char *s = getenv("CONTENT_LENGTH");
	char tmp[128], xraw[4096];

	if (s == NULL) {
		cprintf("XXX CONTENT_LENGTH is too big\n");
		goto out;
	}

	len = atoi(s);
	/*
	 * XXX: I use block IO to stdin, so EAGAIN should not occur any more.
	 * */

	if ((flags = fcntl(fileno(stdin), F_GETFL)) < 0 ||
		fcntl(fileno(stdin), F_SETFL, flags | O_NONBLOCK) < 0) {
		cprintf("XXX stdin is blocked.\n");
		goto out;
	}

	bzero(xraw, sizeof(xraw));
	/* Look for our part */
	while (len > 0) {
		if (fgets(tmp, MIN(len + 1, sizeof(tmp) - 1), stdin) == NULL) {
			if (errno == EAGAIN)
				continue;
			goto out;
		}
		strcat(xraw, tmp);
		len -= strlen(tmp);
	}

	ret = do_xml_main(xraw);
out:
	return ret;
}

