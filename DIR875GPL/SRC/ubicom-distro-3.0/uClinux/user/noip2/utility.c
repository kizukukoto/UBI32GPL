#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <errno.h>

/* Init File and clear the content */
int init_file(char *file)
{
	FILE *fp;

	if ((fp = fopen(file ,"w")) == NULL) {
		printf("Can't open %s\n", file);
		exit(1);
	}

	fclose(fp);
	return 0;
}


/* Save data into file	
 * Notice: You must call init_file() before save2file()
 */
void save2file(const char *file, const char *fmt, ...)
{
	char buf[10240];
	va_list args;
	FILE *fp;

	if ((fp = fopen(file ,"a")) == NULL) {
		printf("Can't open %s\n", file);
		exit(1);
	}

	va_start(args, fmt);
	vsnprintf(buf, sizeof(buf), fmt, args);
	va_end(args);
	va_start(args, fmt);
	fprintf(fp, "%s", buf);
	va_end(args);

	fclose(fp);
}