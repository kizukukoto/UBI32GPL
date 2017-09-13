#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <assert.h>
#include <termios.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <sys/ipc.h>
#include <pwd.h>
#include <time.h>
#include <linux/sockios.h>
//#include <time.h>
#include <sys/time.h>

#include "orayddns.h"
#include "oray_md5.h"
#include "base64.h"
#include "blowfish.h"
#include "option.h"

/* jimmy added 20081117 */
#define ERROR_MSG_PROGRAM "orayddns"
/* ---------------------- */

#ifdef __DIR730__
#define __LITTLE_ENDIAN
#undef __BIG_ENDIAN
#endif

//#include "sutil.h"
char *query_xml = 
"<?xml version=\"1.0\" encoding=\"utf-8\"?>"
"<soap:Envelope xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\" xmlns:soap=\"http://schemas.xmlsoap.org/soap/envelope/\">"
"<soap:Header>"
"<AuthHeader xmlns=\"http://tempuri.org/\">"
"<Account>%s</Account>"
"<Password>%s</Password>"
"</AuthHeader>"
"</soap:Header>"
"<soap:Body>"
"<GetMiscInfo xmlns=\"http://tempuri.org/\">"
"<clientversion>9.1.9.0</clientversion>"
"</GetMiscInfo>"
"</soap:Body>"
"</soap:Envelope>";

struct in_addr oray_server_ipadd;
unsigned char buffer[2048] = {0};
unsigned char server_name[40];
unsigned char szChallenge[256];
int nChallengeLen;
int nChatID;
int nStartID;
int nLastResponseID;
struct sockaddr_in udp_addr;
struct DATA_KEEPALIVE data;
char register_name[32]={0};
int querytime = UDP_QUERY_TIMER_30;
int udp_sockfd = 0;
int udp_connect_finished = 0;
extern struct option * ary_options;

int oray_tcp_connect(char *server_name)
{
 	int sockfd;
 	struct sockaddr_in socka, addr;
 	struct hostent *hptr; 	
 	struct in_addr peer;
 	struct sockaddr_in peer_addr;
 	char *ptr;
 	
 	ptr = server_name;//"hphwebservice.oray.net";//argv[1];
 	DBGPRINT("name = %s :)\n", ptr);
 	if( (hptr = gethostbyname(ptr) ) == NULL )
 	{
  		printf("gethostbyname error for host:%s\n", ptr);
  		return 0; 
 	}

	memcpy(&peer.s_addr, hptr->h_addr_list[0], 4);


	DBGPRINT("peer ip = %s\n", inet_ntoa(peer));
	if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
		perror("socket");
		DBGPRINT("socket fail\n");
		return -1;
	} 
	
	bzero(&socka, sizeof(socka));

	bzero(&addr, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(ORAY_TCP_PORT);
	addr.sin_addr.s_addr = peer.s_addr;
	DBGPRINT("remote oray server IP = %s\n",inet_ntoa(addr.sin_addr));
	if(connect(sockfd, &addr, sizeof(addr)) < 0) {
		perror("connect");
		DBGPRINT("Oray connect fail oray_tcp_connect \n");
		return -1;
	}	
	oray_server_ipadd = peer;
	DBGPRINT("oray_tcp_connect finished\n");
	return sockfd;
}

int oray_tcp_recv(int sockfd,char *buffer, int maxlen)
{
	int addr_len = sizeof(struct sockaddr_in);
	fd_set afdset;
	int ret, len, idx;
	char *line;
	char *resultcmd[3];
	struct timeval timeval;		
	
	bzero(buffer, sizeof(buffer));
		
	/* initial timeout */		
	memset(&timeval, 0, sizeof(timeval));
	timeval.tv_sec = RECV_TIMEOUT;
	timeval.tv_usec = 0; 	

	FD_ZERO( &afdset );
	FD_SET( sockfd, &afdset );
		
	ret = select(sockfd + 1, &afdset, NULL, NULL, &timeval);
	
	if (ret <= 0)
	{
		printf("DDNS timeout, no receive any data \n" );
		return 0;
	}

	
	len = recv(sockfd, buffer, maxlen, 0);
	if (len <= 0) {
		perror("recv");
		return 0;
	}
	DBGPRINT("tcp_recv return\n");
	return len;

}

void oray_hmac_md5(unsigned char *text, int text_len, unsigned char *key, int key_len, unsigned char *digest)
{
         oray_MD5_CTX context;
         unsigned char k_ipad[65];    /* inner padding key XORd with ipad */
         unsigned char k_opad[65];    /* outer padding key XORd with opad */
         unsigned char tk[16];
         int i;

         /* if key is longer than 64 bytes reset it to key=MD5(key) */
         if (key_len > 64)
         {       	
                 oray_MD5_CTX tctx;
                 oray_MD5Init(&tctx);
                 oray_MD5Update(&tctx, key, key_len);
                 oray_MD5Final(tk, &tctx);

                 key = tk;
                 key_len = 16;
         }

         /*
          * the HMAC_MD5 transform looks like:
          *
          * MD5(K XOR opad, MD5(K XOR ipad, text))
          *
          * where K is an n byte key
          * ipad is the byte 0x36 repeated 64 times
          * opad is the byte 0x5c repeated 64 times
          * and text is the data being protected
          */

         /* start out by storing key in pads */
         memset(k_ipad,0,sizeof(k_ipad));
         memcpy(k_ipad,key,key_len);
         memset(k_opad,0,sizeof(k_opad));
         memcpy(k_opad,key,key_len);

         /* XOR key with ipad and opad values */
         for (i=0; i<64; i++)
         {
                 k_ipad[i] ^= 0x36;
                 k_opad[i] ^= 0x5c;
         }

         /*
          * perform inner MD5
          */
         oray_MD5Init(&context);                     /* init context for 1st pass */
         oray_MD5Update(&context, k_ipad, 64);       /* start with inner pad */
         oray_MD5Update(&context, text, text_len);   /* then text of datagram */
         oray_MD5Final(digest, &context);            /* finish up 1st pass */ 	 
         /*
          * perform outer MD5
          */
         oray_MD5Init(&context);                     /* init context for 2nd pass */
         oray_MD5Update(&context, k_opad, 64);       /* start with outer pad */
         oray_MD5Update(&context, digest, 16);       /* then results of 1st hash */
         oray_MD5Final(digest, &context);            /* finish up 2nd pass */
}

static void string_trim_right_crlf(unsigned char *s)
{
    long len;
    len = strlen(s);
    unsigned char *p;
    p = s;
    p += len;
    p--; //

    while( ( (*p == '\r') || (*p == '\n')) && (p>=s)) {
        *p = 0;
        p--;
    }
}


int oray_update(char *username, char *passwd, char *request_domain)
{
	int sockfd_t;
	int len;
	unsigned char buf[128];	
	unsigned char out_buffer[256];
	unsigned char sendbuffer[256];
	unsigned char sendbuffer_pre[256];
	char regicommand[255];
	long *tmp;
	char szdata[40];
	unsigned char key[128] = "";
	unsigned char out_key[256];
	long a1,a2,a3;
	long int clientinfo = CLIENTINFO;
	long int new_challengekey = CHALLENGEKEY;
	int nMoveBits;
	long challengetime_new = 0;
	long challengetime = 0;
	int i,j,totallen,pre_len;
	char *chatid;
	char *startid;	
	
	DBGPRINT("oray update request_domain =%s \n", request_domain);
	sockfd_t = oray_tcp_connect(server_name);		// C: connect to port 6060
	if (sockfd_t < 0)
	{
		close(sockfd_t);
		return -1;
	}	
	len = oray_tcp_recv(sockfd_t, buf, sizeof(buffer));
	if (len <=0 )
	{
		DBGPRINT("oray no return value \n");
		close(sockfd_t);
		return -1;
	}
	//C: AUTH ROUTER
	send(sockfd_t, COMMAND_AUTH, sizeof(COMMAND_AUTH), 0);
	memset(buf, 0, sizeof(buf));	
	//S: 334 XXXXXXXX (base64)

	len = oray_tcp_recv(sockfd_t, buf,sizeof(buf));
	if (len <=0 )
	{
		close(sockfd_t);
		return -1;
	}	
	
	

	//check "334"
	if (buf[0] != '3' && buf[1] != '3') {			
		close(sockfd_t);
		return -1;
	}
	//Generate encoded auth string
	string_trim_right_crlf(buf);
	
	DBGPRINT("SERVER SIDE KEY \"%s\" \n",buf);
	
	len = b64_decode(buf+4, out_buffer, strlen(buf)-4);
	if (len < 0 || len > 256) {

		DBGPRINT("error - base64_decode()\n");
		close(sockfd_t);
		return -1;

	}
	DBGPRINT("out_buffer = \"%s\" \n", out_buffer);
	nChallengeLen = len;
	//save challenge string "+4" skips "334 "

	memcpy(szChallenge,out_buffer,len);
		
#ifdef __LITTLE_ENDIAN
	memcpy(&challengetime, szChallenge+6, 4);
#else	
	memset(szdata, 0, sizeof(szdata));
	//endian problem
	//little->big
	szdata[3] = szChallenge[6];
	szdata[2] = szChallenge[7];
	szdata[1] = szChallenge[8];
	szdata[0] = szChallenge[9];
	tmp = szdata;
	challengetime = *tmp;
#endif
	
	challengetime |= ~new_challengekey;
	nMoveBits = (int) (challengetime % 30);

	// shift fixup, for ARM
	if (nMoveBits < 0) {
		a1 = challengetime << (0 - nMoveBits);
		a3 = ~(0xffffffff << (0 - nMoveBits));
		a2 = challengetime >> (32 + nMoveBits);
	}		
	else {
		a1 = challengetime << (32 - nMoveBits);
		a2 = challengetime >> nMoveBits;
		a3 = ~(0xffffffff << (32 - nMoveBits));
	}	


	challengetime_new = a1 | (a2 & a3);

	strcpy(key, passwd);

	memset(out_key, 0, 16);
	
	oray_hmac_md5((unsigned char*)key, strlen(key), (unsigned char *)out_buffer,16,(unsigned char *)out_key);

	memset(sendbuffer_pre, 0, sizeof(sendbuffer_pre));
	strcpy(sendbuffer_pre, username);
        strcat(sendbuffer_pre," ");

	pre_len = strlen(sendbuffer_pre);
	totallen = pre_len + 16 + 4 + 4;


#ifdef __LITTLE_ENDIAN
	memcpy(sendbuffer_pre+pre_len,&challengetime_new,4);
	memcpy(sendbuffer_pre+pre_len+4,&clientinfo,4);
#else	
	memcpy(szdata,&challengetime_new,4);
	sendbuffer_pre[pre_len] = szdata[3];
	sendbuffer_pre[pre_len+1] = szdata[2];
	sendbuffer_pre[pre_len+2] = szdata[1];
	sendbuffer_pre[pre_len+3] = szdata[0];
	memcpy(szdata,&clientinfo,4);
	sendbuffer_pre[pre_len+4] = szdata[3];
	sendbuffer_pre[pre_len+4+1] = szdata[2];
	sendbuffer_pre[pre_len+4+2] = szdata[1];
	sendbuffer_pre[pre_len+4+3] = szdata[0];
#endif

	for (i=pre_len+8,j=0;i<pre_len+16+8;i++,j++)
	{
		sendbuffer_pre[i] = out_key[j];
	}

	DBGPRINT("sendbuffer:\n");

	memset(sendbuffer, 0, 256);
	len = b64_encode(sendbuffer_pre, totallen, sendbuffer);
	if (len < 0) {
		DBGPRINT(" error - base64_encode()", "Oray_update()");	//Temp Debug
		close(sockfd_t);
		return -1;

	}

    	strcat(sendbuffer,"\r\n");
    	//Generate ok.

	//////////////////////////////////////////////////////////////////////////
	//send auth data
	DBGPRINT("send auth data : %s\n", sendbuffer);

	send(sockfd_t, sendbuffer,strlen(sendbuffer),0);

	bzero(buffer, sizeof(buffer));

	len = oray_tcp_recv(sockfd_t,buffer,sizeof(buffer));

	buffer[3] = 0;


	if (len <=0 )
	{
		close(sockfd_t);
		return -2;
	}
	if (strcmp(buffer,"250")!=0)
	{
		close(sockfd_t);
		return -3;

	}

	//send domain regi commands list

	sprintf(regicommand,"%s %s\r\n",
		COMMAND_REGI, request_domain);
		
        DBGPRINT("%s",regicommand);

        //C: REGI A sandyc.xicp.net   

        send(sockfd_t, regicommand,strlen(regicommand),0);


	bzero(buf, 128);
	//S: 250 reguster successfully

	len = oray_tcp_recv(sockfd_t,buf,sizeof(buf));
	if (len <= 0)
	{
		close(sockfd_t);
		printf("regi error?\n");
		return -4;
	}
	//check "250"
	if (buf[0] != '2' && buf[1] != '5') {			
		close(sockfd_t);
		printf("regi error? %s\n", buf);
		return -1;
	}
	//C: CNFM

    	send(sockfd_t, COMMAND_CNFM,strlen(COMMAND_CNFM),0);

	bzero(buf, 128);

	//S: 250 XXXXX XXXXX

	len = recv(sockfd_t, buf,sizeof(buf),0);
	if (len <= 0)
	{
		close(sockfd_t);
		return -5;
	}

	//////////////////////////////////////////////////////////////////////////
	//find chatid & startid
	chatid = buf + 4;
	startid = NULL;

	for (i=4;i<strlen(buf);i++)
	{
		if (buf[i] == ' ')
		{
			buf[i] = 0;
			startid = buf + i + 1;
			break;
		}
	}
	nChatID = atoi(chatid);
	if (startid) nStartID = atoi(startid);


	//good bye!

	//C: QUIT
	send(sockfd_t, COMMAND_QUIT,sizeof(COMMAND_QUIT),0);


	len = oray_tcp_recv(sockfd_t,buf,sizeof(buf));
	//disconnect

	close(sockfd_t);

	return 0;			
	
}
int encapsulate_query_packet(char *user, char *passwd)
{
	unsigned char xml_buf[512];

	DBGPRINT("encapsulate_query_packet user =%s password = %s\n", user, passwd);

	sprintf(xml_buf, query_xml, user, passwd);
	sprintf(buffer, "POST /userinfo.asmx HTTP/1.1\r\n");

	sprintf(buffer+strlen(buffer), "Host: phservice2.oray.net\r\n");

	sprintf(buffer+strlen(buffer), "Content-Type: text/xml; charset=utf-8\r\n");

	sprintf(buffer+strlen(buffer), "Content-length: %d\r\n", strlen(xml_buf));

	sprintf(buffer+strlen(buffer), "SOAPAction: \"http://tempuri.org/GetMiscInfo\"\r\n");

	sprintf(buffer+strlen(buffer), "\r\n");

	sprintf(buffer+strlen(buffer), xml_buf);

	
	return 0;	
}

static void string_trim_right_space(unsigned char *s)
{
    long len;
    len = strlen(s);
    unsigned char *p;
    p = s;
    p += len;
    p--; //

    while( (*p == ' ') && (p>=s)) {
        *p = 0;
        p--;
    }
}

int parser_return_stream(char *sName)
{
	unsigned char *server_ip_s = NULL, *server_ip_e = NULL;
	unsigned char *usertype_s = NULL, *usertype_e = NULL;
	char *local_user_type[2] = {0};
	int ret = 0;
	int type = 0;
	
	DBGPRINT("recv_buf = \n%s\n", buffer);	
	
	if(strstr(buffer, "HTTP/1.1 200 OK"))
	{	
		server_ip_s = strstr(buffer, "<PHServer>");
		server_ip_e = strstr(buffer, "</PHServer>");
		usertype_s = strstr(buffer, "<UserType>");
		usertype_e = strstr(buffer, "</UserType>");
		if( server_ip_s==NULL || server_ip_e==NULL )
		{	
			printf("%s strstr error: server_ip\n",ERROR_MSG_PROGRAM);
			ret = -1;
			
		}else if( usertype_s==NULL || usertype_e==NULL )
		{	
			printf("%s strstr error: usertype\n",ERROR_MSG_PROGRAM);
			ret = -1;
		}
		else
		{
			strncpy( sName, server_ip_s+10, server_ip_e - (server_ip_s+10) );
			strncpy( local_user_type, usertype_s+10, usertype_e - (usertype_s+10) );
			string_trim_right_space(sName);			
			type = atoi(local_user_type);
			DBGPRINT("server_ip = %s user_type = %d\n", sName, type);
			if (type)
				querytime = UDP_QUERY_TIMER_30;
			else
				querytime = UDP_QUERY_TIMER_60;
		}		
	}else{
		printf("%s return value failed \n",ERROR_MSG_PROGRAM);
		ret = -1;	
	}
	return ret;		
}

int oray_init_qurey(char *user, char *passwd)
{
 	int sockfd;
 	struct sockaddr_in socka, addr;
 	struct hostent *hptr; 	
 	struct in_addr peer, server_ip;
 	struct sockaddr_in peer_addr;
 	char *ptr;
 	struct timeval tv;
 	fd_set afdset;
 	int ret = 0;
 	
 	ptr = "hphwebservice.oray.net";//argv[1];
 	if( (hptr = gethostbyname(ptr) ) == NULL )
 	{
  		printf("%s gethostbyname error for host:%s\n",ERROR_MSG_PROGRAM, ptr);
  		return -1;
 	}

	memcpy(&peer.s_addr, hptr->h_addr_list[0], 4);

	DBGPRINT("peer ip = %s\n", inet_ntoa(peer));
	if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
		perror("socket");
		DBGPRINT("socket fail\n");
		return -1;
	} 

	bzero(&addr, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(80);
	addr.sin_addr.s_addr = peer.s_addr;
	DBGPRINT("remote original oray server IP = %s\n",inet_ntoa(addr.sin_addr));
	if(connect(sockfd, &addr, sizeof(addr)) < 0) {
		perror("connect");
		DBGPRINT("Oray connect fail oray_tcp_connect \n");
		return -1;
	}
	encapsulate_query_packet(user, passwd);	
//	DBGPRINT("send_buf len= %d send_buf=\n%s\n", strlen(buffer), buffer);
		
	if( send(sockfd, buffer, strlen(buffer), 0 ) < 0) 
	{
		perror("send()");
		close(sockfd);
		return -1;
	}	
	memset(buffer, 0 ,sizeof(buffer));
	
	FD_ZERO(&afdset);
	FD_SET(sockfd, &afdset);
	
	tv.tv_sec = 3;
	tv.tv_usec = 0;		

	/* timeout error */
	if(select(sockfd + 1, &afdset, NULL, NULL, &tv) < 0)
	{			
		perror("select()");
		close(sockfd);
		return -1;
	}
		
	/* receive fail */
	if(FD_ISSET(sockfd, &afdset))
		read(sockfd, buffer, sizeof(buffer));
	else{
		printf("%s: recv_buf = NULL\n",ERROR_MSG_PROGRAM);
		return -1;
	}		
			
	ret = parser_return_stream(server_name);
	
	close(sockfd);
	return ret;		
	
}

static void buf2hexlower(char *hexs, unsigned char *buf, unsigned short length)
{
    unsigned short i;
    
    for (i = 0; i < length; i++) {
        unsigned char lsn = (buf[i] >> 4) & 0x0f;
        if (lsn >= 0xa) {
            *hexs++ = lsn + 0x57;
        } else {
            *hexs++ = lsn + '0';
        }

        unsigned char msn = buf[i] & 0x0f;
        if (msn >= 0xa) {
            *hexs++ = msn + 0x57;
        } else {
            *hexs++ = msn + '0';
        }
    }

    *hexs = 0;  // terminate string with 0
}

int password_init(char *passwd, char *md5_passwd)
{


    char hash[MD5_OUTPUT_NUM_U8]; // 16 bytes
    oray_MD5_CTX tctx;

    oray_MD5Init(&tctx);
    oray_MD5Update(&tctx, passwd, strlen(passwd));
    oray_MD5Final(hash, &tctx);   

    //
    buf2hexlower(md5_passwd,hash,16);
    md5_passwd[32] = 0; // string to null	
    DBGPRINT("oray: password_init = %s\n", md5_passwd);
    return 0;	
}

int oray_udp_connect()
{
	int sockfd, len;
	char buffer[256];

	
	if((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0){		
		return -1;
	}
	bzero(&udp_addr, sizeof(udp_addr));
	udp_addr.sin_family = AF_INET;
	udp_addr.sin_port = htons(ORAY_UDP_PORT);
	udp_addr.sin_addr.s_addr = oray_server_ipadd.s_addr;
	DBGPRINT("udp remote oray server IP = %s\n",inet_ntoa(udp_addr.sin_addr));

	return sockfd;
}

int oray_udp_recv(int sockfd,char *buffer, int maxlen)
{
	int addr_len = sizeof(struct sockaddr_in);
	fd_set afdset;
	int ret, len, idx;
	char *line;
	char *resultcmd[3];
	struct timeval timeval;		
		
	/* initial timeout */		
	memset(&timeval, 0, sizeof(timeval));
	timeval.tv_sec = RECV_TIMEOUT;
	timeval.tv_usec = 0; 	
	
	FD_ZERO( &afdset );
	FD_SET( sockfd, &afdset );

	ret = select(sockfd +1, &afdset, (fd_set*) 0, (fd_set*) 0, &timeval);
	if (ret <= 0)
	{
		DBGPRINT("DDNS timeout, no receive any data \n" );
		return 0;
	}

	
	len = recvfrom(sockfd, buffer,  maxlen, 0, &udp_addr, &addr_len);
	
	if (len <= 0) {
		perror("recv");
		return 0;
	}
	DBGPRINT("udp_recv return\n");
	return len;

}

long BTOL(long a)
{
	char tmp[10];
	long *b;
	char s;

	bzero(tmp,sizeof(tmp));
	memcpy(tmp, &a, 4);

	/*1234 -> 4321 */
	s = tmp[3];
	tmp[3] = tmp[0];
	tmp[0] = s;
	s = tmp[2];
	tmp[2] = tmp[1];
	tmp[1] = s;

	b = tmp;
	return	*b;
}

int KeepAlive_Fail_cnt = 0;
SendKeepAlive(int sockfd)
{

	unsigned char p1[KEEPALIVE_PACKET_LEN], p2[KEEPALIVE_PACKET_LEN];
	long a, *tmp;
	int i;
	int opCode;

	DBGPRINT("SendKeepAlive socket id = %d \n", sockfd);  
	
    	opCode = UDP_OPCODE_UPDATE;
    
	bzero(&data,sizeof(data));
			
#ifdef __LITTLE_ENDIAN
	data.lChatID = nChatID;
#else	
	data.lChatID = BTOL(nChatID);	
#endif
	data.lID = nStartID;
	data.lOpCode = opCode;
	data.lSum = 0 - (data.lID + data.lOpCode);

	data.lReserved = 0;

	//dumphex(&data, KEEPALIVE_PACKET_LEN);

	bzero(p1, KEEPALIVE_PACKET_LEN);
	bzero(p2, KEEPALIVE_PACKET_LEN);
	memcpy(p1,&data,KEEPALIVE_PACKET_LEN);
	memcpy(p2,&data,KEEPALIVE_PACKET_LEN);

	
	CBlowfish(szChallenge, nChallengeLen);		
	CBlowfish_EnCode(p1+4, p2+4, KEEPALIVE_PACKET_LEN - 4);

#ifndef __LITTLE_ENDIAN
	tmp = p2+4;
	for(i = 0; i < 4; i++) {
		a = BTOL(*tmp);
		//a = htonl(*tmp);
		*tmp = a;
		tmp++;
	}
#endif
	DBGPRINT("after blowfish:\n");

	DBGPRINT("Send UDP data ...\n");
	
	int addr_len = sizeof(struct sockaddr_in);

	if (sendto(sockfd, p2, KEEPALIVE_PACKET_LEN, 0, &udp_addr, addr_len) != KEEPALIVE_PACKET_LEN) {
		
		printf("%s Send UDP data error...\n",ERROR_MSG_PROGRAM);
		return -1;
	}

	if ( RecvKeepaliveResponse(sockfd) != UDP_OPCODE_UPDATE_OK ) {
		//fail
		int	j;
		
		if (KeepAlive_Fail_cnt>=5) {
//			for(j=IFACE_ETH1;j<ETHER_NUM;j++)
//				dyn_old_ip[j] = 0;
				
			KeepAlive_Fail_cnt = 0;
		}
		else 
			KeepAlive_Fail_cnt++;
	}
	else {
		// successful
		KeepAlive_Fail_cnt = 0;
	}
	
	return 1;
}

//========================
//= return UDP_OPCODE_UPDATE_OK --> Successful
//= return UDP_OPCODE_UPDATE_ERROR --> Fail
//========================
int RecvKeepaliveResponse(int udp_sockfd)
{

	struct DATA_KEEPALIVE_EXT rdata;
	char p1[KEEPALIVE_PACKET_LEN],p2[KEEPALIVE_PACKET_LEN];
	long a, *tmp;
	int i, ret;	

	if (oray_udp_recv(udp_sockfd,&rdata,sizeof(struct DATA_KEEPALIVE_EXT)) <= 0)	{
		DBGPRINT("RecvKeepaliveResponse()", "oray_udp_recv() fail");	//Temp Debug
		return 0;
	}
#ifndef __LITTLE_ENDIAN
	tmp = &rdata;
	for(i = 0; i < 5; i++) {
		a = BTOL(*tmp);
		*tmp = a;
		tmp++;
	}
#endif

	CBlowfish(szChallenge, nChallengeLen);

	bzero(p1, KEEPALIVE_PACKET_LEN);
	bzero(p2, KEEPALIVE_PACKET_LEN);
	memcpy(p1,&rdata,KEEPALIVE_PACKET_LEN);

	memcpy(p2,&rdata,KEEPALIVE_PACKET_LEN);

	CBlowfish_DeCode(p1+4, p2+4, KEEPALIVE_PACKET_LEN - 4);

	memcpy(&rdata,p2,KEEPALIVE_PACKET_LEN);

	nStartID = rdata.keepalive.lID + 1;

	if (data.lID - nLastResponseID > 3 && nLastResponseID != -1)
	{

	}

	DBGPRINT("chatid = 0x04x\n",rdata.keepalive.lChatID);
	DBGPRINT("opcode = %d\n",rdata.keepalive.lOpCode);

	if (rdata.keepalive.lOpCode == UDP_OPCODE_UPDATE_OK){		
		DBGPRINT("Oray KeepAlive Update OK!","RecvKeepaliveResponse()");	//Temp Debug
		ret = UDP_OPCODE_UPDATE_OK;
	} else if (rdata.keepalive.lOpCode == UDP_OPCODE_UPDATE_ERROR) {
		DBGPRINT("Oray KeepAlive Update Fail!","RecvKeepaliveResponse()");	//Temp Debug
		ret = UDP_OPCODE_UPDATE_ERROR;
	}	

	nLastResponseID = rdata.keepalive.lID;
	return ret;
}

void udp_heart_beat_check()
{
	struct timeval tv;
    struct timezone tz;	
    int ret = 0;
    time_t timep;
    struct tm *p;  
    
	gettimeofday(&tv,&tz);
	time(&timep);
	p = gmtime(&timep);
	SendKeepAlive(udp_sockfd);						
}

int main(int argc, char **argv)
{
	const char * optionsfile = ORAY_CONFIG;
     	static char username[32]={0};
     	static char password[32]={0};
     	char md5_passwd[32]={0};
     	int ret = 0;
	struct itimerval value;
	struct sigaction act;
	int i = 0;

	//perform check() after one second continuously
	
	if (readoptionsfile(optionsfile) < 0)
	{
		printf("%s open file error\n",ERROR_MSG_PROGRAM);
	}
	for(i=0; i<num_options; i++)
	{
		switch(ary_options[i].id)
		{
		case ORAY_USERNAME:
			strcpy(username, ary_options[i].value);			
			break;
		case ORAY_PASSWD:
			strcpy(password, ary_options[i].value);	
			break;
		case ORAY_REGISTERNAME:
			strcpy(register_name, ary_options[i].value);	
			break;	
		}
	}			
	DBGPRINT("username = %s\n",username);
	DBGPRINT("password = %s\n",password);
	DBGPRINT("register name = %s\n", register_name);
	
	//init timer
	act.sa_handler=udp_heart_beat_check; 
	act.sa_flags=SA_RESTART; 
	sigaction(SIGALRM, &act, NULL);
	
 	DBGPRINT("oray service start\n");  
 	   	
	password_init(password, md5_passwd);
	while(!udp_connect_finished){	
 		if (oray_init_qurey(username, md5_passwd) < 0){
 			DBGPRINT("oray init fail\n"); 
 			goto err;		
 		}	
 		if (oray_update(username, password, register_name) < 0)
 		{
 			DBGPRINT("oray update fail\n"); 
 			goto err;
 		}	
 		udp_sockfd = oray_udp_connect();
 			udp_connect_finished = 1;
 		DBGPRINT("oray update finished udp_sockfd = %d \n", udp_sockfd); 
err:
		sleep(15); 		
	}
	value.it_value.tv_sec = querytime;
	value.it_value.tv_usec = 0;
	value.it_interval.tv_sec = querytime;
	value.it_interval.tv_usec = 0;
	setitimer(ITIMER_REAL, &value, NULL); 
	DBGPRINT("oray udp heart beat timer = %d \n", querytime);    
	for(;;){
		
	}	
	
 	return 0;
}
