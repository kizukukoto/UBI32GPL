/*
 * uClinux/user/profilerd/profilerd.c
 *   Implementation for Ubicom32 Profiler Daemon
 *
 * (C) Copyright 2009, Ubicom, Inc.
 *
 * This file is part of the Ubicom32 Linux Kernel Port.
 *
 * The Ubicom32 Linux Kernel Port is free software: you can redistribute
 * it and/or modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation, either version 2 of the
 * License, or (at your option) any later version.
 *
 * The Ubicom32 Linux Kernel Port is distributed in the hope that it
 * will be useful, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
 * the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with the Ubicom32 Linux Kernel Port.  If not,
 * see <http://www.gnu.org/licenses/>.
 */
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <sys/socket.h>
#include <string.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <linux/types.h>
#include <asm/profpkt.h>
#include <asm/profilesample.h>

#define DATA_SIZE 1440
#define PROFILE_PORT 51080
#define PROFILE_CONTROL_PORT 51081

#define TRUE 1
#define FALSE 0

/*
 * send the memory map for a pid
 */
int send_pid_map(int request_fd, unsigned int pid)
{
	char name[100];
	char buf[1000];
	if (pid > 32767) {
		printf("bad pid: %u\n", pid);
		return FALSE;
	}
	sprintf(buf, "PID:%u\n", pid);
	write(request_fd, buf, strlen(buf));
	sprintf(name, "/proc/%d/cmdline", pid);
	FILE *f = fopen(name, "r");
	if (!f) {
		sprintf(buf, "done - no %u process\n", pid);
		write(request_fd, buf, strlen(buf));
		return TRUE;
	}
	if (fgets(buf, 1999, f) == NULL) {
		fclose(f);
		write(request_fd, "done - empty command\n", 21);
		return TRUE;
	}
	fclose(f);
	/* convert null separators to spaces */
	char *p;
	for (p = buf; *p || *(p+1); ++p) {
		if (!*p)
			*p = ' ';
	}
	*p = '\n';
	write(request_fd, buf, strlen(buf));
	sprintf(name, "/proc/%d/maps", pid);
	f = fopen(name, "r");
	if (!f) {
		sprintf(buf, "done - no %u maps\n", pid);
		write(request_fd, buf, strlen(buf));
		return TRUE;
	}
	while (fgets(buf, 1999, f) != NULL) {
		int sent;
		int tries = 0;
		while (1) {
			sent = write(request_fd, buf, strlen(buf));
			if (sent == strlen(buf)) {
				break;
			}
			if (sent < 0 || tries > 5) {
				fprintf(stderr, "PID maps network write failed, error %s\n", strerror(errno));
				fclose(f);
				return FALSE;
			}
			tries++;
			usleep(50);
		}
	}
	fclose(f);
	write(request_fd, "done\n", 5);
	return TRUE;
}

#define MAX_PID 8

unsigned int sent_pid[MAX_PID];
int next_pid;

/*
 * check if this is a new PID.  if it is, send its map
 */
int send_pid_check(unsigned int pid)
{
	int i;
	for (i = 0; i < MAX_PID; ++i) {
		if (sent_pid[i] == pid) {
			return FALSE;
		}
	}
	sent_pid[next_pid] = pid;
	next_pid++;
	if (next_pid == MAX_PID) {
		next_pid = 0;
	}
	return TRUE;
}

/*
 * read sample packets from data_fd and send to send_fd until sample buffer is empty
 */
int send_packets(int data_fd, int send_fd, int request_fd, struct in_addr *ip_addr)
{
	char packet[DATA_SIZE];
	int size, err;
	struct sockaddr_in si;
	struct profile_header *ph;
	struct profile_sample *ps;
	int i;
	si.sin_family = AF_INET;
	si.sin_port = htons(PROFILE_PORT);
	si.sin_addr.s_addr = ip_addr->s_addr;

	while (1) {
		size = read(data_fd, packet, DATA_SIZE);
		if (size == -1) {
			fprintf(stderr, "Profiler data read failed.  Error %s\n", strerror(errno));
			return 0;
		}
		if (size == 0) {
			return 1;
		}
		if (size > DATA_SIZE) {
			fprintf(stderr, "Read returned too much data (%d bytes)\n", size);
			return 1;
		}
		ph = (struct profile_header *)packet;
		if ((ph->magic & 0xffe0) == PROF_MAGIC) {
			ps = (struct profile_sample *)(packet + ph->header_size);
			for (i = 0; i < ph->sample_count; ++i) {
				if (send_pid_check(ps->pid)) {
					if (!send_pid_map(request_fd, ps->pid)) {
						return 0;
					}
				}
				ps++;
			}
		}
		err = sendto(send_fd, packet, size, 0, (struct sockaddr *)&si, sizeof(struct sockaddr));
		if (err < 0) {
			fprintf(stderr, "UDP packet send error %s\n", strerror(errno));
			return 0;
		}
	}
}

/*
 * read sample packets from data_fd and send to send_fd until sample buffer is empty
 */
int send_vma_packets(int data_fd, int send_fd, struct in_addr *ip_addr)
{
	char packet[DATA_SIZE];
	int size, err;
	struct sockaddr_in si;
	si.sin_family = AF_INET;
	si.sin_port = htons(PROFILE_PORT);
	si.sin_addr.s_addr = ip_addr->s_addr;

	while (1) {
		size = read(data_fd, packet, DATA_SIZE);
		if (size == -1) {
			fprintf(stderr, "Profiler data read failed.  Error %s\n", strerror(errno));
			return 0;
		}
		if (size == 0) {
			return 1;
		}
		if (size > DATA_SIZE) {
			fprintf(stderr, "Read returned too much data (%d bytes)\n", size);
			return 1;
		}
		err = sendto(send_fd, packet, size, 0, (struct sockaddr *)&si, sizeof(struct sockaddr));
		if (err < 0) {
			fprintf(stderr, "UDP packet send error %s\n", strerror(errno));
			return 0;
		}
	}
}

/*
 * socket_listen
 *	create a socket to listen for a connction request on the PROFILE_CONTROL_PORT
 */

int socket_listen(void)
{  
	int sfd = socket(PF_INET, SOCK_STREAM, 0); 
	if (sfd == -1) { 
		fprintf(stderr, "Can't create tcp socket, error %s\n", strerror(errno));
		exit(4); 
	} 
  
	struct sockaddr_in isa;
	memset(&isa, 0, sizeof(struct sockaddr_in));
	isa.sin_family = AF_INET;
	isa.sin_port = htons(PROFILE_CONTROL_PORT);

	if (bind(sfd, (struct sockaddr*)&isa, sizeof isa) == -1) { 
		close(sfd);
		fprintf(stderr, "Can't bind tcp socket to listen for connection requests. Error %s\n", strerror(errno)); 
		exit(4); 
	} 
  
	return sfd;
}

#define RBUF_SIZE 1500
#define IN_SIZE 5000

static char inbuf[IN_SIZE + 1];

/*
 * get_command
 *	parse one command (up to /n) from the input tcp stream, into request_buf
 */
int get_command(int request_fd, char *request_buf)
{
	char tbuf[RBUF_SIZE + 1];
	char *loc;
	int size;

	while (1) {
		/* is there already a command to process? */
		loc = strchr(inbuf, '\n');
		if (loc) {
			size = loc - inbuf + 1;
			strncpy(request_buf, inbuf, size);
			request_buf[size] = 0;
			memcpy(inbuf, loc + 1, strlen(loc) + 1); 
			return 1;
		}

		/* get more input if it is available */
		size = recv(request_fd,  tbuf, RBUF_SIZE, 0);
		if (size <= 0)
			return size;
		tbuf[size] = 0;
		strcat(inbuf, tbuf);
	}
}

/*
 * process_requests
 *	process all requests from the input tcp stream
 */

void process_requests(int request_fd)
{
	int send_fd = -1;
	int data_fd = -1;
	int map_fd = -1;
	struct in_addr ip_addr;
	int err, i;
	char request_buf[RBUF_SIZE];
	struct sockaddr_in si;
	char buf[2000];
	char version [2000];
	int flags = fcntl(request_fd, F_GETFL);
	if (flags < 0) {
		fprintf(stderr, "Can't get tcp link flags.  Error %s\n", strerror(errno));
		return;
	}
	flags |= O_NDELAY;
	fcntl(request_fd, F_SETFL, flags);

	next_pid = 0;
	for (i = 0; i < MAX_PID; ++i) {
		sent_pid[i] = 0;
	}

	/* process commands, then send all data */
	while (1) {
		while (1) {
			flags |= O_NDELAY;
			fcntl(request_fd, F_SETFL, flags);
			err = get_command(request_fd, request_buf);
			if (err == 0) {
				goto finished;	/* no more connection */
			}
			if (err < 0) {
				if (errno == EWOULDBLOCK || errno == EAGAIN) {	/* no command now try again later */
					break;
				} else {
					fprintf(stderr, "Lost connection to profiler tool.  Error %s\n", strerror(errno));
					goto finished;	/* socket closed from other end most likely */
				}
			}
			flags &= ~O_NDELAY;
			fcntl(request_fd, F_SETFL, flags);
			if (strstr(request_buf, "map") == request_buf) {
				/* 
				 * send proc/maps (IP7K only) and proc/modules to get segment names and load addresses
				 */
				next_pid = 0;
				for (i = 0; i < MAX_PID; ++i) {
					sent_pid[i] = 0;
				}
				strcpy(version, "PID:0 ");
				FILE *f = fopen("/proc/version\n", "r");
				if (f) {
					fgets(buf, 1900, f);
					strcat(version, buf);
					fclose(f);
				} else {
					strcat(version, "Unknown version\n");
				}
				fprintf(stderr, "Sending memory map data\n");
				if (write(request_fd, version, strlen(version)) <= 0) {
					fprintf(stderr, "modules PID network write failed, error %s\n", strerror(errno));
					goto finished;
				}

				f = fopen("/proc/maps", "r");
				if (f) {
					while (fgets(buf, 1999, f) != NULL) {
						if (write(request_fd, buf, strlen(buf)) <= 0) {
							fprintf(stderr, "maps network write failed, error %s\n", strerror(errno));
							fclose(f);
							goto finished;
						}
					}
					fclose(f);
					write(request_fd, "done\n", 5);
				}
				f = fopen("/proc/modules", "r");
				if (!f) {
					fprintf(stderr, "Can't open /proc/modules\n");
					write(request_fd, "done - no modules\n", 18);
					continue;
				}
				while (fgets(buf, 1999, f) != NULL) {
					if (write(request_fd, buf, strlen(buf)) <= 0) {
						fprintf(stderr, "modules network write failed, error %s\n", strerror(errno));
						fclose(f);
						goto finished;
					}
				}
				fclose(f);
				write(request_fd, "done\n", 5);
			} else if (strstr(request_buf, "pid") == request_buf) {
				/* send map and command line for a PID */
				int pid = atoi(&request_buf[4]);
				if (!send_pid_map(request_fd, pid)) {
					goto finished;
				}
			} else if (strstr(request_buf, "vma") == request_buf && send_fd != -1) {
				fprintf(stderr, "Sending memory usage data\n");
				map_fd = open("/proc/profile/maps", O_RDONLY | O_NDELAY);
				if (map_fd == -1) {
					fprintf(stderr, "Can't open /proc/profile/maps\n");
					goto finished;
				}
				send_vma_packets(map_fd, send_fd, &ip_addr);
				close(map_fd);
			} else if (strstr(request_buf, "ip ") == request_buf) {
				/* create UDP socket */
				if (send_fd != -1) {
					close(send_fd);
				}
				send_fd = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
				if (send_fd == -1) {
					fprintf(stderr, "Can't create send udp socket, error %s\n", strerror(errno));
					goto finished;
				}
				/* parse IP address */
				if (inet_aton(&request_buf[3], &ip_addr) == 0) {
					fprintf(stderr, "Can't parse IP address %s\n", &request_buf[3]);
					goto finished;
				}
				memset(&si, 0, sizeof(struct sockaddr_in));
				si.sin_family = AF_INET;
				si.sin_addr.s_addr = htonl(INADDR_ANY);
				si.sin_port = htons(0);
				err = bind(send_fd, (struct sockaddr *)&si, sizeof(si));
				if (err < 0) {
					fprintf(stderr, "Can't bind UDP send socket.  Error %s\n", strerror(errno));
					goto finished;
				}
				fprintf(stderr, "connected to profiler tool at %s\n", &request_buf[3]);
			} else if (strstr(request_buf, "start") == request_buf && data_fd == -1) {
				fprintf(stderr, "Start sending samples\n");
				data_fd = open("/proc/profile/data", O_RDONLY | O_NDELAY);
				if (data_fd == -1) {
					fprintf(stderr, "Can't open /proc/profile/data. Error %s.\n", strerror(errno));
					goto finished;
				}
			} else if (strstr(request_buf, "stop") == request_buf && data_fd != -1) {
				fprintf(stderr, "Stop sending samples\n");
				close(data_fd);
				data_fd = -1;
			} else if(strstr(request_buf, "close") == request_buf) {
				goto finished;
			}
		}

		if (data_fd != -1 && send_fd != -1) {
			if (!send_packets(data_fd, send_fd, request_fd, &ip_addr)) {
				break;
			}
		}
		sleep(1);
	}

finished:
	if (data_fd != -1) {
		close(data_fd);
	}
	if (send_fd != -1) {
		close(send_fd);
	}
}

/*
 * show_rate
 *	display the profiler rate and check if the profiler driver is loaded
 */
int show_rate(void)
{
	char rate_buf[200];
	FILE *rate = fopen("/proc/profile/rate", "r");
	if (rate == NULL) {
		return 0;
	}
	fgets(rate_buf, 199, rate);
	fprintf(stderr, "%s\n", rate_buf);
	fclose(rate);
	return 1;
}

void bye(void) {
        fprintf(stderr, "Profilerd exiting.\n");
}

void sig_handler(int signum)
{
	fprintf(stderr, "got signal %d\n", signum);
}

int main(int argc, char *argv[])
{
	int listen_fd;		// listen for TCP connections from the profiler tool
	int request_fd;		// get requests from the profiler tool (ip address, start, stop)
	int fail_count = 0;
	struct sigaction act;

	if (atexit(bye) != 0) {
		fprintf(stderr, "Can't set atexit function\n");
	}

	act.sa_handler = SIG_IGN;
	sigemptyset (&act.sa_mask);
	act.sa_flags = 0;
	sigaction(SIGPIPE, &act, NULL);	// ignore SIG_PIPE, raised when profiler tool exits, so next write fails

	if (!show_rate()) {
		fprintf(stderr, "Profiler is not installed.  Can't open /proc/profile/rate.\n");
		exit(1);
	}

	listen_fd = socket_listen();

	while (1) {
		fprintf(stderr, "Listening for a profiler tool connection\n");
		if (listen(listen_fd, 1) == -1) { 
			close(listen_fd);
			fprintf(stderr, "Can't listen, error %s\n", strerror(errno));
			exit(4); 
		} 
		request_fd = accept(listen_fd, NULL, NULL);
		if (request_fd == -1) {
			fprintf(stderr, "Accept failed, error %s\n", strerror(errno));
			fail_count++;
			if (fail_count > 20) {
				fprintf(stderr, "Too many accept failures\n");
				exit(1);
			}
			continue;
		}
		process_requests(request_fd);
		close(request_fd);
	}
}
