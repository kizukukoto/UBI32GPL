/***********************************************************************
 *
 * main.c
 *
 * Main program of l2tp
 *
 * Copyright (C) 2002 by Roaring Penguin Software Inc.
 *
 * This software may be distributed under the terms of the GNU General
 * Public License, Version 2, or (at your option) any later version.
 *
 * LIC: GPL
 *
 ***********************************************************************/

static char const RCSID[] =
"$Id: main.c,v 1.3 2008/10/07 06:45:35 ken_chiang Exp $";

#include "l2tp.h"
#include <stdio.h>
#include <string.h>
#include <getopt.h>
#include <signal.h>
#include <fcntl.h>
#include <stdlib.h>
#include <syslog.h>

#define PID_FILE "/var/run/l2tp.pid"

static void create_pidfile(int pid)
{
	FILE *pidfile;

	if ((pidfile = fopen(PID_FILE, "w")) != NULL) {
		fprintf(pidfile, "%d\n", pid);
		(void) fclose(pidfile);
	} else {
		fprintf(stderr, "Failed to create pid file %s", PID_FILE);
	}
}

void remove_pidfiles(void)
{
	if (unlink(PID_FILE) < 0 )
		fprintf(stderr, "unable to delete pid file %s", PID_FILE);
	printf("remove file\n");
}

static void l2tp_close(int sig)
{
	remove_pidfiles();
	l2tp_die();
}

static void
usage(int argc, char *argv[], int exitcode)
{
	fprintf(stderr, "\nl2tpd Version %s Copyright 2002 Roaring Penguin Software Inc.\n", VERSION);
	fprintf(stderr, "http://www.roaringpenguin.com/\n\n");
	fprintf(stderr, "Usage: %s [options]\n", argv[0]);
	fprintf(stderr, "Options:\n");
	fprintf(stderr, "-d level               -- Set debugging to 'level'\n");
	fprintf(stderr, "-f                     -- Do not fork\n");
	fprintf(stderr, "-h                     -- Print usage\n");
	fprintf(stderr, "\nThis program is licensed under the terms of\nthe GNU General Public License, Version 2.\n");
	exit(exitcode);
}

int
main(int argc, char *argv[])
{
	EventSelector *es = Event_CreateSelector();
	int i;
	int opt;
	int do_fork = 1;
	int debugmask = 0;

	/* syslog(LOG_NOTICE, "l2tp:\n"); */
	while((opt = getopt(argc, argv, "d:fh")) != -1) {
		switch(opt) {
			case 'h':
				usage(argc, argv, EXIT_SUCCESS);
				break;
			case 'f':
				do_fork = 0;
				break;
			case 'd':
				sscanf(optarg, "%d", &debugmask);
				break;
			default:
				usage(argc, argv, EXIT_FAILURE);
		}
	}
	
	/* syslog(LOG_NOTICE, "l2tp: init\n"); */
	l2tp_random_init();
	l2tp_tunnel_init(es);
	l2tp_peer_init();
	l2tp_debug_set_bitmask(debugmask);

	if (l2tp_parse_config_file(es, "/var/etc/l2tp.conf") < 0) {
		/* syslog(LOG_NOTICE, "l2tp: l2tp_parse_config_file fail!\n"); */
		l2tp_die();
	}

	if (!l2tp_network_init(es)) {
		/* syslog(LOG_NOTICE, "l2tp: l2tp_network_init fail!\n"); */
		l2tp_die();
	}

	/* Daemonize */
	if (do_fork) {
#ifndef __uClinux__
		i = fork();
#else
		i = vfork();
#endif
		if (i < 0) {
			perror("fork");
			_exit(EXIT_FAILURE);
		} else if (i != 0) {			/* Parent */
			
			_exit(EXIT_SUCCESS);
		}
		setsid();
		/* syslog(LOG_NOTICE, "l2tp: signal\n"); */
		signal(SIGHUP, SIG_IGN);
		signal(SIGTERM, l2tp_close);
#ifndef __uClinux__
		i = fork();
#else
		i = vfork();
#endif
		if (i < 0) {
			perror("fork");
			_exit(EXIT_FAILURE);
		} else if (i != 0) {
			_exit(EXIT_SUCCESS);
		}
		chdir("/");

		/* Point stdin/stdout/stderr to /dev/null */
		for (i=0; i<3; i++) {
			close(i);
		}
		i = open("/dev/null", O_RDWR);
		if (i >= 0) {
			dup2(i, 0);
			dup2(i, 1);
			dup2(i, 2);
			if (i > 2) close(i);
		}
	}
	/* syslog(LOG_NOTICE, "l2tp: create_pidfile\n"); */
	create_pidfile( getpid() );
	while(1) {
		i = Event_HandleEvent(es);
		if (i < 0) {
			fprintf(stderr, "Event_HandleEvent returned %d\n", i);
			/* syslog(LOG_NOTICE, "l2tp: Event_HandleEvent returned %d\n",i); */
			l2tp_cleanup();
			_exit(EXIT_FAILURE);
		}
	}
	return 0;
}
