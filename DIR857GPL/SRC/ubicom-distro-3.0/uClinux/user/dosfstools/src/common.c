/* common.c - Common functions

   Copyright (C) 1993 Werner Almesberger <werner.almesberger@lrc.di.epfl.ch>
   Copyright (C) 1998 Roman Hodek <Roman.Hodek@informatik.uni-erlangen.de>

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program. If not, see <http://www.gnu.org/licenses/>.

   On Debian systems, the complete text of the GNU General Public License
   can be found in /usr/share/common-licenses/GPL-3 file.
*/

/* FAT32, VFAT, Atari format support, and various fixes additions May 1998
 * by Roman Hodek <Roman.Hodek@informatik.uni-erlangen.de> */


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>
#ifdef USE_SILEX
#include <sys/ipc.h>
#include <sys/msg.h>
#endif

#include "common.h"

#ifdef USE_SILEX
#define MSG_ID 1115 //talk to dosfsck
#define TEXT_SIZE 16
struct msgbuffer
{
	long mtype;
	char mtext[TEXT_SIZE];
}msgp_c;
#endif

typedef struct _link {
    void *data;
    struct _link *next;
} LINK;

#ifdef USE_SILEX
void msg_rdonly()
{
	int msqid;
	
	msqid = msgget(MSG_ID, IPC_CREAT | 0600);
	msgp_c.mtype = 1;
	strcpy(msgp_c.mtext, "read_only");
	msgsnd(msqid, &msgp_c, TEXT_SIZE, 0);
	
	exit(1);
}
#endif

void die(char *msg,...)
{
    va_list args;
#ifdef USE_SILEX
	int msqid;
#endif

    va_start(args,msg);
    vfprintf(stderr,msg,args);
    va_end(args);
    fprintf(stderr,"\n");
#ifdef USE_SILEX
    msqid = msgget(MSG_ID, IPC_CREAT | 0600);
		msgp_c.mtype = 1;
    strcpy(msgp_c.mtext, "die");
		msgsnd(msqid, &msgp_c, TEXT_SIZE, 0);
#endif
    exit(1);
}


void pdie(char *msg,...)
{
    va_list args;
#ifdef USE_SILEX
	int msqid;
#endif
    va_start(args,msg);
    vfprintf(stderr,msg,args);
    va_end(args);
    fprintf(stderr,":%s\n",strerror(errno));
#ifdef USE_SILEX    
    msqid = msgget(MSG_ID, IPC_CREAT | 0600);
		msgp_c.mtype = 1;
    strcpy(msgp_c.mtext, "pdie");
		msgsnd(msqid, &msgp_c, TEXT_SIZE, 0);
#endif    
		exit(1);
}


void *alloc(int size)
{
    void *this;

    if ((this = malloc(size))) return this;
    pdie("malloc");
    return NULL; /* for GCC */
}


void *qalloc(void **root,int size)
{
    LINK *link;

    link = alloc(sizeof(LINK));
    link->next = *root;
    *root = link;
    return link->data = alloc(size);
}


void qfree(void **root)
{
    LINK *this;

    while (*root) {
	this = (LINK *) *root;
	*root = this->next;
	free(this->data);
	free(this);
    }
}


int min(int a,int b)
{
    return a < b ? a : b;
}


char get_key(char *valid,char *prompt)
{
    int ch,okay;

    while (1) {
	if (prompt) printf("%s ",prompt);
	fflush(stdout);
	while (ch = getchar(), ch == ' ' || ch == '\t');
	if (ch == EOF) exit(1);
	if (!strchr(valid,okay = ch)) okay = 0;
	while (ch = getchar(), ch != '\n' && ch != EOF);
	if (ch == EOF) exit(1);
	if (okay) return okay;
	printf("Invalid input.\n");
    }
}
