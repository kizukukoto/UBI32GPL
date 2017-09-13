#ifndef __ORAYDDNS_H__
#define __ORAYDDNS_H__
/*
 *	orayddns.h
 *
 *	Written by Sandy Cho
 */

#define COMMAND_AUTH	"auth router\r\n"
#define COMMAND_REGI    "regi a"
#define COMMAND_CNFM    "cnfm\r\n"
#define COMMAND_QUIT    "quit\r\n"

#define UDP_OPCODE_UPDATE			10
#define UDP_OPCODE_UPDATE_OK			50
#define UDP_OPCODE_UPDATE_ERROR		1000

#define UDP_OPCODE_LOGOUT			11
#define UDP_OPCODE_LOGOUT_RESPONSE	51
#define UDP_OPCODE_LOGOUT_ERROR		1001

#define KEEPALIVE_PACKET_LEN	20
#define	RECV_TIMEOUT		30  //30:30sec udp update 250:5min reconnect

#define	ORAY_TCP_PORT		6060
#define	ORAY_UDP_PORT		6060

///#define SERVER_IP 	"61.152.96.115"    //hphwebservice.oray.net
//#define SERVER_IP 	"172.21.34.135"    //hphwebservice.oray.net

#define	CLIENTINFO	0x20063150
#define	CHALLENGEKEY	0x10101010

#define MD5_OUTPUT_NUM_U8 16
struct DATA_KEEPALIVE 
{
	long lChatID;
	long lOpCode;
	long lID;
	long lSum;
	long lReserved;
};


struct DATA_KEEPALIVE_EXT
{
	struct DATA_KEEPALIVE keepalive;
	long ip;
};
#define UDP_QUERY_TIMER_30 30
#define UDP_QUERY_TIMER_60 60

#ifdef __BIG_ENDIAN
#ifdef __LITTLE_ENDIAN
#undef __LITTLE_ENDIAN
#endif
#endif

#ifdef __DIR730__
#define ORAY_CONFIG	"/tmp/oray.conf"
#else
#define ORAY_CONFIG  "/var/etc/oray.conf"
#endif
#define DBGPRINT //printf

#endif //__ORAYDDNS_H__
