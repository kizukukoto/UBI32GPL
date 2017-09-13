/* osWrap_linux.c - DK 3.0 functions to hide os dependent calls */

/* Copyright (c) 2000 Atheros Communications, Inc., All Rights Reserved */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>
#include <assert.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <signal.h>
#include <arpa/inet.h>
#include <sys/ipc.h>
#include <sys/sem.h>

#include "wlantype.h"

#include "athreg.h"
#include "manlib.h"

#ifndef SOC_AP
#include "mInst.h"
#endif

#include "termios.h"
#include <fcntl.h>


#include "dk_common.h"
#include "wlantype.h"

struct termios oldtio;

#define COM1 "/dev/ttyS0"
#define COM2 "/dev/ttyS1"

#define SEND_BUF_SIZE        1024

extern A_UINT32 sent_bytes;
extern A_UINT32 received_bytes;

static A_UINT32 os_com_open(OS_SOCK_INFO *pOSSock);
static A_UINT32 os_com_close(OS_SOCK_INFO *pOSSock);
extern HANDLE open_device(A_UINT32 device_fn, A_UINT32 devIndex, char*);
static A_INT32 fd_write(A_INT32 fd, A_UINT16 port_num, A_UINT8 *buf, A_INT32 bytes);
static A_INT32 fd_read(A_INT32 fd, A_UINT16 port_num, A_UINT8 *buf, A_INT32 bytes);
static A_INT32 socketConnect
(
 	char *target_hostname, 
	int target_port_num, 
	A_UINT32 *ip_addr
);

#define SEMKEY	0x1234

A_INT32 gSem=0;

A_STATUS
osThreadCreate
	(
	void           threadFunc(void * param),
	void 			*param,
	A_CHAR 			*threadName,
	A_INT32			threadPrio,
	A_UINT32			*threadId
	)
{

    int		res;
    pthread_t	thread;
    pthread_attr_t attr;

    // osThreadCreate uses a func returning only (void) and not (void *) - ugly casting here
    // This is done as the windows threads have no exact equivalent using the (void *) return
    res = pthread_attr_init(&attr);
    if (res == 0) {
#ifdef __UBICOM32__
        res = pthread_attr_setstacksize(&attr, 32768);
        if (res == 0)
#endif
            res = pthread_create(&thread, &attr, (void * (*)(void *))threadFunc, (void *) param);
    }

    if (res) {
	uiPrintf("ERROR::osThreadCreate: pthread_create failed: %s:%s\n",
	       strerror(errno), strerror(res));
	       return A_ERROR;
    }
    else {
		uiPrintf("osThreadCreate: pthread_create sucessful. thread=0x%08lx\n",
	       (unsigned long) thread);
    }

	if (threadId != NULL) {
		*threadId = (A_UINT32)thread;
	}

    return A_OK;
}

void osThreadKill
(
	A_UINT32 threadId
)
{
	pthread_t	thread;

	thread = (pthread_t)threadId;
	pthread_kill(thread, SIGKILL);

	return;
}


A_INT32 osSockRead
	(
	OS_SOCK_INFO    *pSockInfo,
	A_UINT8         *buf,
	A_INT32         len
	)
{
	A_INT32		res, iIndex;

	q_uiPrintf("osSockRead:%d\n", len);
	   res = fd_read(pSockInfo->sockfd, pSockInfo->port_num, (A_UINT8 *) buf, len);
       
    
   received_bytes += len; 
       
       
   
    q_uiPrintf("Read %d bytes of %d bytes\n", res, len);
    for(iIndex=0; iIndex<len; iIndex++) {
            q_uiPrintf("%x ", buf[iIndex]);
            if (!(iIndex % 32)) q_uiPrintf("\n");
    }
	
    
	   if (res != len) {
		   		return 0;
	   }
	   else {
		   return len;
	   }
}


A_INT32 osSockWrite
(
	OS_SOCK_INFO           *pSockInfo,
	A_UINT8                *buf,
	A_INT32                len
	)
{
	A_UINT32		res, iIndex;

	q_uiPrintf("osSockWrite:%d\n", len);
   

	res = fd_write(pSockInfo->sockfd, pSockInfo->port_num, (A_UINT8 *) buf, len);
   sent_bytes += len; 

    q_uiPrintf("Sent %d bytes of %d bytes\n", res, len);
    for(iIndex=0; iIndex<len; iIndex++) {
            q_uiPrintf("%x ", buf[iIndex]);
            if (!(iIndex % 32)) q_uiPrintf("\n");
    }

	if (res != len) {
		return 0;
	}
	else {
		return len;
	}
}



OS_SOCK_INFO *osSockConnect
(
	char *mach_name
)
{
	OS_SOCK_INFO *pOSSock;
	int res;
	char *cp;
    A_UINT32 err;
   HANDLE       hDevice;

        //uiPrintf("SNOOP:: osSockConnect called\n");
    pOSSock = (OS_SOCK_INFO *) A_MALLOC(sizeof(*pOSSock));
    if (!pOSSock) {
       uiPrintf("ERROR::osSockConnect: malloc failed for pOSSock \n");
       return NULL;
    }


	while (*mach_name == '\\') {
		mach_name++;
	}
	
	for (cp = mach_name; (*cp != '\0') && (*cp != '\\'); cp++) {
	}
	*cp = '\0';

	q_uiPrintf("osSockConnect: starting mach_name = '%s'\n", mach_name);

	if (!strcmp(mach_name, ".")) {
			mach_name = "localhost";
     }

	  strcpy(pOSSock->hostname, mach_name);
	  pOSSock->port_num = SOCK_PORT_NUM;

    if (!strcasecmp(mach_name, "COM1")) {
	    q_uiPrintf("osSockConnect: Using serial communication port 1\n");
	    strcpy(pOSSock->hostname, COM1);
		pOSSock->port_num = COM1_PORT_NUM;
    }
    if (!strcasecmp(mach_name, "COM2")) {
	    q_uiPrintf("osSockConnect: Using serial communication port 2\n");
	    strcpy(pOSSock->hostname, COM2);
		pOSSock->port_num = COM2_PORT_NUM;
    }
	if (!strcasecmp(pOSSock->hostname, "SDIO")) {
       pOSSock->port_num = MBOX_PORT_NUM;
	}
	pOSSock->sockDisconnect = 0;
	pOSSock->sockClose = 0;

    switch(pOSSock->port_num) {
       case MBOX_PORT_NUM: 
            hDevice = open_device(SDIO_FUNCTION, 0, NULL);
		    pOSSock->sockfd = hDevice;
			break;
       case COM1_PORT_NUM:
       case COM2_PORT_NUM:
        if ((err=os_com_open(pOSSock)) != 0) {
            uiPrintf("ERROR::osSockConnect::Com port open failed with error = %x\n", err);
            exit(0);
        }
		break;
       case SOCK_PORT_NUM: {
			q_uiPrintf("osSockConnect: revised mach_name = '%s':%d\n", pOSSock->hostname, pOSSock->port_num);
	  		res = socketConnect(pOSSock->hostname, pOSSock->port_num, &pOSSock->ip_addr);
	  		if (res < 0) {
	          uiPrintf("ERROR::osSockConnect: pipe connect failed\n"); A_FREE(pOSSock);
	          return NULL;
	  		}
	  		q_uiPrintf("osSockConnect: Connected to pipe\n");

	  		q_uiPrintf("ip_addr = %d.%d.%d.%d\n",(pOSSock->ip_addr >> 24) & 0xff,
											(pOSSock->ip_addr >> 16) & 0xff,
											(pOSSock->ip_addr >> 8) & 0xff,
											(pOSSock->ip_addr >> 0) & 0xff);
	  		pOSSock->sockfd = res;

		} // end of else
	}

	return pOSSock;
}

/**************************************************************************
* osSockClose - close socket
*
* Close the handle to the socket
*
*/
void osSockClose
(
	OS_SOCK_INFO *pOSSock
)
{
        A_UINT32 err;
        //uiPrintf("SNOOP:: osSockClose called for port_num=%d\n", pOSSock->port_num);
	if (pOSSock->port_num == COM1_PORT_NUM || pOSSock->port_num== COM2_PORT_NUM) {
      if ((err = os_com_close(pOSSock)) != 0) {
            uiPrintf("osSockClose::Com port close failure with errror = %x\n", err);
      }
	} else {
	   if (err=close(pOSSock->sockfd)) {
            uiPrintf("osSockClose::close failure with errror = %x\n", err);
       }
	}
	A_FREE(pOSSock);
	return;
}


static A_INT32 socketConnect
(
 	char *target_hostname, 
	int target_port_num, 
	A_UINT32 *ip_addr
)
{
	A_INT32		sockfd;
	A_INT32		res;
	struct sockaddr_in	sin;
	struct hostent *hostent;
	A_INT32		i;
	A_INT32		j;

	sockfd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sockfd == -1) {
		q_uiPrintf("ERROR::socket failed: %s\n", strerror(errno));
		return -1;
   	}

	/* Allow immediate reuse of port */
    q_uiPrintf("setsockopt SO_REUSEADDR start\n");
    i = 1;
    res = setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (char *) &i, sizeof(i));
    if (res != 0) {
        uiPrintf("ERROR::socketConnect: setsockopt SO_REUSEADDR failed: %d\n",strerror(errno));
		close(sockfd);
	     return -1;
 	}
    q_uiPrintf("setsockopt SO_REUSEADDR end\n");


	/* Set TCP Nodelay */
    q_uiPrintf("setsockopt TCP_NODELAY start\n");
	i = 1;
    j = sizeof(i);
    res = setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, (A_INT8 *)&i, j);
   	if (res == -1) {
		uiPrintf("ERROR::setsockopt failed: %s\n", strerror(errno));
		close(sockfd);
		return -1;
   	}	
	q_uiPrintf("setsockopt TCP_NODELAY end\n");


	q_uiPrintf("gethostbyname start\n");
    q_uiPrintf("socket_connect: target_hostname = '%s'\n", target_hostname);
    hostent = gethostbyname(target_hostname);
    q_uiPrintf("gethostbyname end\n");
    if (!hostent) {
        uiPrintf("ERROR::socketConnect: gethostbyname failed: %d\n",strerror(errno));
		close(sockfd);
	    return -1;
    }

	memcpy(ip_addr, hostent->h_addr_list[0], hostent->h_length);
	*ip_addr = ntohl(*ip_addr);
					
   	sin.sin_family = AF_INET;
	memcpy(&sin.sin_addr.s_addr, hostent->h_addr_list[0], hostent->h_length);
	sin.sin_port = htons((short)target_port_num);


	for (i = 0; i < 20; i++) {
		q_uiPrintf("connect start %d\n", i);
	   	res = connect(sockfd, (struct sockaddr *) &sin, sizeof(sin));
		q_uiPrintf("connect end %d\n", i);
        if (res == 0) {
            break;
        }
        milliSleep(1000);
    }
	
    if (i == 20) {
        uiPrintf("ERROR::connect failed completely\n");
		close(sockfd);
        return -1;
    }
		
   	return sockfd;
}

/* Returns number of bytes read, -1 if error */
static A_INT32 fd_read
(
	A_INT32 fd,
    A_UINT16 port_num,
 	A_UINT8 *buf,
 	A_INT32 bytes
)
{
    	A_INT32		cnt;
    	A_UINT8*		bufpos;

    	bufpos = buf;

    	while (bytes) {
    		switch(port_num) {
				case MBOX_PORT_NUM:
					cnt = read(fd, (A_INT8 *)bufpos, bytes);	
				break;
				default:
					cnt = read(fd, (A_INT8 *)bufpos, bytes);	
				break;
			}

		if (!cnt) break;

		if (cnt == -1) {
	    		if (errno == EINTR || errno == EAGAIN) {
				continue;
	     		}
	     		else {
				return -1;
	     		}
   		}
		bytes -= cnt;
		bufpos += cnt;
    	}

    	return (bufpos - buf);
}

/* Returns number of bytes written, -1 if error */
static A_INT32 fd_write
	(
	A_INT32 fd,
    A_UINT16 port_num,
	A_UINT8 *buf,
	A_INT32 bytes
	)
{
    	A_INT32	cnt;
    	A_UINT8*	bufpos;

    	bufpos = buf;

    	while (bytes) {
    		switch(port_num) {
				case MBOX_PORT_NUM:
				  cnt = write(fd, bufpos, bytes);
				break;
				default:
				  cnt = write(fd, bufpos, bytes);
				break;
			}

		if (!cnt) {
			break;
		}
		if (cnt == -1) {
	    		if (errno == EINTR) {
				continue;
	    		}
	    		else {
				return -1;
	    		}
		}

		bytes -= cnt;
		bufpos += cnt;
    	}

    	return (bufpos - buf);
}

static A_INT32 socketListen
(
	OS_SOCK_INFO *pOSSock
)
{
	A_INT32     sockfd;
	A_INT32     res;
	struct sockaddr_in  sin;
	A_INT32     i;
	A_INT32     j;

	sockfd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sockfd == -1) {
          uiPrintf( "ERROR::socket failed: %s\n", strerror(errno));
          return -1;
	}

	// Allow immediate reuse of port
	i = 1;
	j = sizeof(i);
	res = setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (A_INT8 *)&i, j);
	if (res == -1) {
		uiPrintf( "ERROR::setsockopt failed: %s\n", strerror(errno));
		return -1;
	}

	i = 1;
	j = sizeof(i);
	res = setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, (A_INT8 *)&i, j);
	if (res == -1) {
		uiPrintf( "ERROR::setsockopt failed: %s\n", strerror(errno));
		return -1;
	}

	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr =  INADDR_ANY;
	sin.sin_port = htons(pOSSock->port_num);
	res = bind(sockfd, (struct sockaddr *) &sin, sizeof(sin)); 
	if (res == -1) { 
		uiPrintf( "ERROR::bind failed: %s\n", strerror(errno)); 
		return -1; 
	}
	res = listen(sockfd, 4);
	if (res == -1) { 
		uiPrintf( "ERROR::listen failed: %s\n", strerror(errno)); 
		return -1; 
	} 
						
	return sockfd;
}

/**************************************************************************
* osSockAccept - Wait for a connection
*
*/
OS_SOCK_INFO *osSockAccept
	(
	OS_SOCK_INFO *pOSSock
	)
{
	OS_SOCK_INFO *pOSNewSock;
	A_INT32		i;
	A_INT32		sfd;
	struct sockaddr_in	sin;

    //    uiPrintf("SNOOP:: osSockAccept called\n");
	pOSNewSock = (OS_SOCK_INFO *) A_MALLOC(sizeof(*pOSNewSock));
	if (!pOSNewSock) {
		uiPrintf("ERROR::osSockAccept: malloc failed for pOSNewSock \n");
		return NULL;
	}
  
	
   	i = sizeof(sin);
   	sfd = accept(pOSSock->sockfd, (struct sockaddr *) &sin, (socklen_t *)&i);
   	if (sfd == -1) {
		A_FREE(pOSNewSock);
		uiPrintf( "ERROR::accept failed: %s\n", strerror(errno));
		return NULL;
	}

  	strcpy(pOSNewSock->hostname, inet_ntoa(sin.sin_addr));
	pOSNewSock->port_num = sin.sin_port;

	pOSNewSock->sockDisconnect = 0;
	pOSNewSock->sockClose = 0;
	pOSNewSock->sockfd = sfd;

	return pOSNewSock;
}


OS_SOCK_INFO *osSockListen
(
	A_UINT32 acceptFlag, A_UINT16 port
)
{
	OS_SOCK_INFO *pOSSock;
	OS_SOCK_INFO *pOSNewSock;
    A_UINT32 err;

    pOSSock = (OS_SOCK_INFO *) A_MALLOC(sizeof(*pOSSock));
    if (!pOSSock) {
	        uiPrintf("ERROR::osSockListen: malloc failed for pOSSock \n");
	        return NULL;
    }

    pOSSock->port_num = port;
    switch(pOSSock->port_num) {
	case COM1_PORT_NUM:
		strcpy(pOSSock->hostname, COM1);
		break;
	case COM2_PORT_NUM:
		strcpy(pOSSock->hostname, COM2);
		break;
	case SOCK_PORT_NUM:
		strcpy(pOSSock->hostname, "localhost");
		break;
	}

	pOSSock->sockDisconnect = 0;
	pOSSock->sockClose = 0;

	if (port == SOCK_PORT_NUM) {
       pOSSock->sockfd = socketListen(pOSSock);
       if (pOSSock->sockfd == -1) {
	        uiPrintf("ERROR::Socket create failed \n");
			A_FREE(pOSSock);
	        return NULL;
       }

	   if (acceptFlag) {
		   pOSNewSock = osSockAccept(pOSSock);
		   if (!pOSNewSock) {
			   uiPrintf("ERROR::osSockListen: malloc failed for pOSSock \n");
			   osSockClose(pOSSock);
			   return NULL;
		   }
		   osSockClose(pOSSock);
		   pOSSock = pOSNewSock;
	   }
	}
	else {
		if ((err=os_com_open(pOSSock)) != 0) {
			uiPrintf("ERROR::osSockListen::Com port open failed\n");
		}
	}

	return pOSSock;
}

static A_INT32 socket_create_and_accept
(
	A_INT32 port_num
)
{
	A_INT32     sockfd;
	A_INT32     res;
	struct sockaddr_in  sin;
	A_INT32     i;
	A_INT32     j;
	A_INT32     sfd;

	sockfd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sockfd == -1) {
          fprintf(stderr, "socket failed: %s\n", strerror(errno));
          return -1;
	}

	// Allow immediate reuse of port
	i = 1;
	j = sizeof(i);
	res = setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (A_INT8 *)&i, j);
	if (res == -1) {
		fprintf(stderr, "setsockopt failed: %s\n", strerror(errno));
		return -1;
	}

	i = 1;
	j = sizeof(i);
	res = setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, (A_INT8 *)&i, j);
	if (res == -1) {
		fprintf(stderr, "setsockopt failed: %s\n", strerror(errno));
		return -1;
	}

	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr =  INADDR_ANY;
	sin.sin_port = htons(port_num);
	res = bind(sockfd, (struct sockaddr *) &sin, sizeof(sin)); 
	if (res == -1) { 
		fprintf(stderr, "bind failed: %s\n", strerror(errno)); 
		return -1; 
	}
	res = listen(sockfd, 4);
	if (res == -1) { 
		fprintf(stderr, "listen failed: %s\n", strerror(errno)); 
		return -1; 
	} 
						
	i = sizeof(sin);
	sfd = accept(sockfd, (struct sockaddr *) &sin, (socklen_t *)&i);
	if (sfd == -1) { 
		fprintf(stderr, "accept failed: %s\n", strerror(errno)); 
		return -1; 
	} 
	
	res = close(sockfd); 
	if (res == -1) { 
		fprintf(stderr, "sockfd close failed: %s\n", strerror(errno)); 
		return 1; 
	}
								
	return sfd;
}

ART_SOCK_INFO *osSockCreate
(
	char *pname
)
{
	ART_SOCK_INFO *pOSSock;

    pOSSock = (ART_SOCK_INFO *) A_MALLOC(sizeof(*pOSSock));
    if (!pOSSock) {
	        uiPrintf("osSockCreate: malloc failed for pOSSock \n");
	        return NULL;
    }

    strcpy(pOSSock->hostname, "localhost");
    pOSSock->port_num = SOCK_PORT_NUM;

    pOSSock->sockfd = socket_create_and_accept(pOSSock->port_num);

    if (pOSSock->sockfd == -1) {
	        uiPrintf("Socket create failed \n");
	        return NULL;
    }

    return pOSSock;
}

A_UINT32 semInit
(
	void
) 
{
#ifndef SOC_LINUX
	A_INT32 semId;
	A_INT32 initVal;

	semId = semget(SEMKEY, 1, 0777 | IPC_CREAT);
	if (semId == -1) {
		uiPrintf("ERROR:: Semaphore creation failed \n");
		return 0;
	}
	initVal = 1;
	if (semctl(semId, 0, SETVAL, initVal) == -1) {
		uiPrintf("ERROR:: Semaphore initalization failed \n");
		return 0;
	}
	
	//remember the semid which can be deleted on a CNTL-c close
	gSem = semId;
#endif

	return 1;
}

A_INT32 semLock
(
	A_UINT32 semId
)
{
#ifndef SOC_LINUX
	struct sembuf sem;

		sem.sem_num = 0;
		sem.sem_op = -1;
		sem.sem_flg = SEM_UNDO;

		if (semop(semId, &sem, 1) == -1) {
			return -1;
		}
#endif
	return 0;
}

A_INT32 semUnLock
(
	A_UINT32 semId
)
{
#ifndef SOC_LINUX
	struct sembuf sem;
		
		sem.sem_num = 0;
		sem.sem_op = 1;
		sem.sem_flg = SEM_UNDO;

		if (semop(semId, &sem, 1) == -1) {
			return -1;
		}
#endif
	return 0;
}

A_INT32 semClose
(
	A_UINT32 semId
)
{
#ifndef SOC_LINUX
	if (semctl(semId, 0, IPC_RMID, 0) == -1) {
		uiPrintf("ERROR:: Semaphore deletion failed \n");
		return -1;
	}
#endif	
	return 0;
}

void osCleanup
(
	void
)
{
	if (gSem) {
		semClose(gSem);
	}
}

A_UINT map_file(A_STATUS *status, A_UCHAR **memPtr, A_UCHAR *filename) {

  FILE *fp;
  int fd;
  A_UINT length;
  fp = (FILE *) fopen((const char*)filename, "r");
  if (fp == NULL) {
    return 0;
  }
  uiPrintf("map_file:uiPrintf\n");
  fseek(fp, 0, SEEK_END);
  uiPrintf("map_file:uiPrintf\n");
  length = ftell(fp);
  fseek(fp, 0, SEEK_SET);
  fclose(fp);


  fd = open( (const char*) filename, O_RDONLY);
  *memPtr = (A_UCHAR *)mmap(0, length, PROT_READ, MAP_SHARED, fd, 0);
  if ( -1 == (A_CHAR)**memPtr) {
     uiPrintf("ERROR::map_file:mmap returned with error %d\n", **memPtr);
     return 0;
  }
  close(fd);
  return length;

}

A_UINT32 os_com_open(OS_SOCK_INFO *pOSSock)
{
    A_INT32 fd; 
    A_UINT32 nComErr=0;
    struct termios my_termios;

    fd = -1;
    fd = open(pOSSock->hostname, O_RDWR | O_NOCTTY  | O_SYNC);

    if (fd < 0) { 
        nComErr = nComErr | COM_ERROR_GETHANDLE;
        return nComErr;
    }
	

	tcgetattr(fd, &oldtio);
	// NOTE: you may want to save the port attributes
	// here so that you can restore them later
	q_uiPrintf("%s Terminal attributes\n", pOSSock->hostname);
    q_uiPrintf("old cflag=%08x\n", my_termios.c_cflag);
	q_uiPrintf("old oflag=%08x\n", my_termios.c_oflag);
	q_uiPrintf("old iflag=%08x\n", my_termios.c_iflag);
	q_uiPrintf("old lflag=%08x\n", my_termios.c_lflag);
	q_uiPrintf("old line=%02x\n", my_termios.c_line);
	tcflush(fd, TCIFLUSH);
	memset(&my_termios,0,sizeof(my_termios));
	my_termios.c_cflag = B19200 | CS8 |CREAD | CLOCAL ;
	my_termios.c_iflag = IGNPAR;
	my_termios.c_oflag = 0;
	my_termios.c_lflag = 0; //FLUSHO; //|ICANON;
	my_termios.c_cc[VMIN] = 1;
	my_termios.c_cc[VTIME] = 0;
	tcsetattr(fd, TCSANOW, &my_termios);
	q_uiPrintf("new cflag=%08x\n", my_termios.c_cflag);
	q_uiPrintf("new oflag=%08x\n", my_termios.c_oflag);
	q_uiPrintf("new iflag=%08x\n", my_termios.c_iflag);
	q_uiPrintf("new lflag=%08x\n", my_termios.c_lflag);
	q_uiPrintf("new line=%02x\n", my_termios.c_line);
	

    pOSSock->sockfd = fd;
    tcflush(pOSSock->sockfd, TCIOFLUSH);
    return 0;
}

A_UINT32 os_com_close(OS_SOCK_INFO *pOSSock)
{   
    A_UINT32 nComErr;
    // reset error byte
    nComErr = 0;
    if (pOSSock->sockfd < 0) {
            nComErr = nComErr | COM_ERROR_INVALID_HANDLE;
            return nComErr;
    }

    tcsetattr(pOSSock->sockfd,TCSANOW,&oldtio);

    close(pOSSock->sockfd);
    pOSSock->sockfd = 0;

    return 0;
}

