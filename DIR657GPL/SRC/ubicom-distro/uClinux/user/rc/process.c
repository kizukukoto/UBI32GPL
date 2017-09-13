/*
 * $Id: process.c,v 1.2 2007/09/04 09:29:13 roy Exp $
 */
#include <stdio.h>
#include <string.h>
 
#include <sutil.h>
#include <nvram.h>
#include <shvar.h>
#include <rc.h>

#ifdef RC_DEBUG
#define DEBUG_MSG(fmt, arg...)       printf("%s [%d]: "fmt, __FILE__, __LINE__, ##arg)
#else
#define DEBUG_MSG(fmt, arg...) 
#endif

int checkServiceAlivebyName(char *service)
{
	FILE * fp;
	char buffer[1024];
	if((fp = popen("ps -ax", "r"))){
		while( fgets(buffer, sizeof(buffer), fp) != NULL ) {
			if(strstr(buffer, service)) {
				pclose(fp);
				return 1;
			}
		}
	}
	pclose(fp);
	return 0;
}

