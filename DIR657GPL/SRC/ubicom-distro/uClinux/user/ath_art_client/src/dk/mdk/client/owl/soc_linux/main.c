/*
 * $Id: //depot/sw/src/dk/mdk/client/owl/soc_linux/main.c#1 $
 *
 * Copyright (c) 2000-2003 Atheros Communications, Inc., All Rights Reserved
 *
 */
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "dk_common.h"
//#include <linux/sched.h>
//#include <errno.h>

extern A_INT32 mdk_main(A_INT32 debugMode, A_UINT16 port);
void thread_fn(void) {
    mdk_main(0, SOCK_PORT_NUM);
}

main(int argc, char *argv[])
{

    int		res;
    pthread_t	thread;

	uiPrintf("Welcome to ART build\n");
    mdk_main(0, SOCK_PORT_NUM);
	//int clone(int (*fn)(void *), void *child_stack, int flags, void *arg)
	
	/*
    res = clone((void * (*)(void *))thread_fn, NULL, CLONE_PARENT, NULL);
    if (res==-1) {
	uiPrintf("ERROR:: clone for mdk_main failed: %s:%s\n",
	       strerror(errno), strerror(res));
	       return A_ERROR;
    }
	*/

}



