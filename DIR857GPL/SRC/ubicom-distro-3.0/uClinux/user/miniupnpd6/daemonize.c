/* $Id: daemonize.c,v 1.11 2008/01/29 13:04:45 nanard Exp $ */
/* MiniUPnP project
 * http://miniupnp.free.fr/ or http://miniupnp.tuxfamily.org/
 * (c) 2006 Thomas Bernard
 * This software is subject to the conditions detailed
 * in the LICENCE file provided within the distribution */

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include <string.h>
#include <signal.h>

#include "daemonize.h"
#include "config.h"

#ifdef MINIUPNPD_DEBUG
#define DEBUG_MSG(fmt, arg...)       printf("%s [%d]: "fmt, __FILE__, __LINE__, ##arg)
#else
#define DEBUG_MSG(fmt, arg...) 
#endif

#ifndef USE_DAEMON

int
daemonize(void)
{
	int pid;
	//int i;

	DEBUG_MSG("%s (%d): begin\n",__FUNCTION__,__LINE__);

#ifdef  __uClinux__
	switch(vfork())
#else
	switch(fork())
#endif
	{
	/* fork error */
	case -1:
#ifdef  __uClinux__
		perror("vfork()");
#else
		perror("fork()");
#endif
		DEBUG_MSG("%s (%d): failed\n",__FUNCTION__,__LINE__);
		exit(1);

	/* child process */
	case 0:
		/* obtain a new process group */
		if( (pid = setsid()) < 0)
		{
			perror("setsid()");
			DEBUG_MSG("%s (%d): failed\n",__FUNCTION__,__LINE__);
			exit(1);
		}

		/* close all descriptors */
		/*	Date:	2009-04-20
		*	Name:	jimmy huang
		*	Reason:	here will close all descriptors...
		*			So mark these codes
		*/
		/*
		for (i=getdtablesize();i>=0;--i) close(i);

		i = open("/dev/null",O_RDWR); // open stdin
		dup(i); // stdout
		dup(i); // stderr

		umask(027);
		chdir("/"); // chdir to /tmp ?
		*/

		return pid;

	/* parent process */
	default:
#ifdef  __uClinux__
		_exit(0);
#else
		exit(0);
#endif
	}
}
#endif

int
writepidfile(const char * fname, int pid)
{
	char pidstring[16];
	int pidstringlen;
	int pidfile;

	if(!fname || (strlen(fname) == 0))
		return -1;

	if( (pidfile = open(fname, O_WRONLY|O_CREAT|O_EXCL, 0666)) < 0)
	{
		syslog(LOG_ERR, "Unable to open pidfile for writing %s: %m", fname);
		return -1;
	}

	pidstringlen = snprintf(pidstring, sizeof(pidstring), "%d\n", pid);
	if(pidstringlen <= 0)
	{
		syslog(LOG_ERR,
			"Unable to write to pidfile %s: snprintf(): FAILED", fname);
		close(pidfile);
		return -1;
	}
	else
	{
		if(write(pidfile, pidstring, pidstringlen) < 0)
			syslog(LOG_ERR, "Unable to write to pidfile %s: %m", fname);
	}

	close(pidfile);

	return 0;
}

int
checkforrunning(const char * fname)
{
	char buffer[64];
	int pidfile;
	pid_t pid;

	if(!fname || (strlen(fname) == 0))
		return -1;

	if( (pidfile = open(fname, O_RDONLY)) < 0)
		return 0;

	memset(buffer, 0, 64);

	if(read(pidfile, buffer, 63))
	{
		if( (pid = atol(buffer)) > 0)
		{
			if(!kill(pid, 0))
			{
				close(pidfile);
				return -2;
			}
		}
	}

	close(pidfile);

	return 0;
}

