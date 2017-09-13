/*
 * $Id: //depot/sw/src/dk/mdk/client/owl/soc_linux/main.c#3 $
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

    int	i, debug_mode = 0;
    pthread_t	thread;
    
    if(argc > 1) {
    	sscanf(argv[1], "%d", &debug_mode);
    	printf("Debug mode = %d\n", debug_mode);
    }

	uiPrintf("Welcome to ART build\n");

	if(argc>1) {
		if(strcmp(argv[1], "debug") == 0)
			debug_mode = 1;
			
	}
    mdk_main(debug_mode, SOCK_PORT_NUM);
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



