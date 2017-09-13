#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
typedef int SOCKET;
/*!***************************************************
* int InetInitSockaddr(
*	const char* host,unsigned short port,int protocol,
*	struct sockaddr_in* skaddr)
*
*	Description:
*		Initial struct sockaddr's INET socket domain family.
*	Arguments:
*		host: Host Name or address.ex ("www.foo.com" or "192.168.0.1")
*		port: TCP or UDP port 
*		type: SOCK_STREAM or SOCK_DGRAM
*		skaddr: struct sockaddr_in* for output argument
*	Return:
*		out:struct sockaddr_in* skaddr
*		-1: sockaddr initial failure. gethostbyname() error.
*		0:Success
*	20041210 Robber 1.0 stable
******************************************************/
SOCKET InetInitSockaddr(
	const char *host,
	unsigned short port,
	int type,
	struct sockaddr_in *skaddr
){

//	struct sockaddr_in skaddr;
	struct hostent *ip;
	in_addr_t ipraw;

	memset(skaddr,0,sizeof(skaddr));
	skaddr->sin_family=AF_INET;
	skaddr->sin_port=htons(port);
	//cout<<"Init host:"<<host<<endl;
	if((ipraw=inet_addr(host))!=0xffffffff){	//-1
		//skaddr->sin_addr->s_addr=inet_addr(host);
		skaddr->sin_addr.s_addr=inet_addr(host);
		//skaddr->sin_addr->s_addr=(struct in_addr)ipraw;
	}else{
	//	if(!*host) strcpy(host,"localhost");
		if(!(ip = gethostbyname(host))){
			//fprintf(stderr, "gethostbyname(%s) failure\n", host);
			return -1;
		}else{
			memcpy((char *)&skaddr->sin_addr.s_addr,
			(void *)ip->h_addr,
			ip->h_length);
		}
	}
	return 0;
}

/*!********************************************************
* SOCKET InetSockCliInit(char *host,int type,unsigned short port)
*	Description:
*		Initial Socket client
*	Arguments:
*		host: Host Name or address.ex ("www.foo.com" or "192.168.0.1")
*		port: TCP ro UPD port
*		type: SOCK_STREAM or SOCK_DGRAM
*		cliaddr: struct sockaddr_in*
*	Return:
*		IN UNIX SOCKET,less than zero return if initialized failure
*		IN WINSOCKET,zero return if initialized failure 
*		out: struct sockaddr_in* cliaddr
*		-1: scoket() error.
*		-2: Initial sockaddr_in error.
*		-3: connection error.
*		Success: Return socket descriptor
*		v1.0 stable
***********************************************************/
SOCKET InetSockCliInit(
	const char *host,unsigned short port,int type,struct sockaddr_in *cliaddr
){
	SOCKET s;
	int res;

#ifdef WIN32
	WSADATA WSAData;                    // Contains details of the Winsock
                                      // implementation
	if (WSAStartup (MAKEWORD(1,1), &WSAData) != 0){
		//	cout<<"WSAStartip() error\n";
			return(-1);
	}
#endif //__WIN32__
	
	if((s=socket(AF_INET,type,0))<=0){
#ifdef WIN32
		return 0;
#else
		return -1;
#endif //WIN32
		//cout<<"socket.....Error()"<<endl;
	}
	if(InetInitSockaddr(host,port,type,cliaddr)!=0){
		//cout<<"initaddr.....Error()"<<endl;
#ifdef WIN32
		return 0;
#else
		return -2;
#endif //WIN32
	}
	if((res=connect(s,(struct sockaddr *)cliaddr,sizeof(struct sockaddr_in)))!=0){
#ifdef WIN32
		return 0;
#else
		return -3;
#endif //WIN32
	}
	return s;
}
SOCKET InetSockSrvInit(
	const char *host,unsigned short port,int type,struct sockaddr_in *srvaddr
){
	SOCKET s;

#ifdef __WIN32__
	WSADATA WSAData;                    // Contains details of the Winsock
                                      // implementation
	if (WSAStartup (MAKEWORD(1,1), &WSAData) != 0) return(-1);
#endif //__WIN32__

	if((s=socket(AF_INET,type,0))<=0){
		return -1;
	}
	if(InetInitSockaddr(host,port,type,srvaddr)!=0){
		return -2;
	}
	if(type==SOCK_STREAM){
		bind(s,(struct sockaddr*)srvaddr,sizeof(struct sockaddr_in));
	}
	return s;
}
int InetDestroySocket(SOCKET s){
	if(s<=0){
		return 0;
	}
	shutdown(s,SHUT_WR);
#ifdef __WIN32__
	WSACleanup();
#endif
	return 1;
}

