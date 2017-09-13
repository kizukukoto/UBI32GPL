#ifndef _INETSOCKET_H
#define _INETSOCKET_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
typedef int SOCKET;
extern int InetDestroySocket(SOCKET);
extern  SOCKET InetSockCliInit(const char*,unsigned short,int,struct sockaddr_in*);
extern  SOCKET InetSockSrvInit(const char*,unsigned short,int,struct sockaddr_in*);
extern int chkhostname(const char *p);
#endif	//_INETSOCKET_H
