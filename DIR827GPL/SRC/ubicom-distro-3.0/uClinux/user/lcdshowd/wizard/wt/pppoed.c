#include <unistd.h>
#include <linux/if_packet.h>
#include <sys/socket.h>
#include <net/if.h>
#include <net/ethernet.h>
#include <asm/types.h>
#include <sys/ioctl.h>
#include <syslog.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/time.h>
#include <stdio.h>
#include <signal.h>

#define         MAXDLBUF        8192
typedef unsigned short UINT16_t;
#define PADI_TIMEOUT 2
#define MAX_PADI_ATTEMPTS 2

#define STATE_SENT_PADI     0
#define STATE_RECEIVED_PADO 1
#define STATE_SENT_PADR     2
#define STATE_SESSION       3
#define STATE_TERMINATED    4

/* Header size of a PPPoE packet */
#define PPPOE_OVERHEAD 6  /* type, code, session, length */
#define HDR_SIZE (sizeof(struct ethhdr) + PPPOE_OVERHEAD)
#define MAX_PPPOE_PAYLOAD (ETH_DATA_LEN - PPPOE_OVERHEAD)
/* Structure used to determine acceptable PADO or PADS packet */
struct PacketCriteria {
    int acNameOK;
    int serviceNameOK;
};

/* PPPoE Tag */

struct PPPoETag {
    unsigned int type:16;	/* tag type */
    unsigned int length:16;	/* Length of payload */
    unsigned char payload[ETH_DATA_LEN]; /* A LOT of room to spare */
};
/* PPPoE Tags */
#define TAG_END_OF_LIST        0x0000
#define TAG_SERVICE_NAME       0x0101
#define TAG_AC_NAME            0x0102
#define TAG_HOST_UNIQ          0x0103
#define TAG_AC_COOKIE          0x0104
#define TAG_VENDOR_SPECIFIC    0x0105
#define TAG_RELAY_SESSION_ID   0x0110
#define TAG_SERVICE_NAME_ERROR 0x0201
#define TAG_AC_SYSTEM_ERROR    0x0202
#define TAG_GENERIC_ERROR      0x0203
/* PPPoE codes */
#define CODE_PADI           0x09
#define CODE_PADO           0x07
#define CODE_PADR           0x19
#define CODE_PADS           0x65
#define CODE_PADT           0xA7
#define CODE_SESS           0x00
#define TAG_HDR_SIZE 4
struct PPPoEPacket {
    struct ethhdr ethHdr;	/* Ethernet header */
    unsigned int ver:4;		/* PPPoE Version (must be 1) */
    unsigned int type:4;	/* PPPoE Type (must be 1) */
    unsigned int code:8;	/* PPPoE code */
    unsigned int session:16;	/* PPPoE session */
    unsigned int length:16;	/* Payload length */
    unsigned char payload[ETH_DATA_LEN]; /* A bit of room to spare */
};

typedef void ParseFunc(UINT16_t type,
		       UINT16_t len,
		       unsigned char *data,
		       void *extra);

void fatalSys(const char *s)
{
	fprintf(stderr, "%s\n", s);
	perror("sys");
}

void fatal(const char *s)
{
	fprintf(stderr, "%s\n", s);
}

#define CHECK_ROOM(cursor, start, len) \
do {\
    if (((cursor)-(start))+(len) > MAX_PPPOE_PAYLOAD) { \
        syslog(LOG_ERR, "Would create too-long packet"); \
        return; \
    } \
} while(0)

int
openInterface(char const *ifname, UINT16_t type, unsigned char *hwaddr)
{
    int optval=1;
    int fd;
    struct ifreq ifr;
    int domain, stype;

#define HAVE_STRUCT_SOCKADDR_LL 1
#ifdef HAVE_STRUCT_SOCKADDR_LL
    struct sockaddr_ll sa;
#else
    struct sockaddr sa;
#endif

    memset(&sa, 0, sizeof(sa));

#ifdef HAVE_STRUCT_SOCKADDR_LL
    domain = PF_PACKET;
    stype = SOCK_RAW;
#else
    domain = PF_INET;
    stype = SOCK_PACKET;
#endif

    if ((fd = socket(domain, stype, htons(type))) < 0) {
	/* Give a more helpful message for the common error case */
	if (errno == EPERM) {
	    fatal("Cannot create raw socket -- pppoe must be run as root.");
	}
	fatalSys("socket");
    }

    if (setsockopt(fd, SOL_SOCKET, SO_BROADCAST, &optval, sizeof(optval)) < 0) {
	fatalSys("setsockopt");
    }

    /* Fill in hardware address */
    if (hwaddr) {
	strncpy(ifr.ifr_name, ifname, sizeof(ifr.ifr_name));
	if (ioctl(fd, SIOCGIFHWADDR, &ifr) < 0) {
	    fatalSys("ioctl(SIOCGIFHWADDR)");
	}
	memcpy(hwaddr, ifr.ifr_hwaddr.sa_data, ETH_ALEN);
    }

    /* Sanity check on MTU */
    strncpy(ifr.ifr_name, ifname, sizeof(ifr.ifr_name));
    if (ioctl(fd, SIOCGIFMTU, &ifr) < 0) {
	fatalSys("ioctl(SIOCGIFMTU)");
    }
    if (ifr.ifr_mtu < ETH_DATA_LEN) {
	char buffer[256];
	sprintf(buffer, "Interface %.16s has MTU of %d -- should be %d.  You may have serious connection problems.",
		ifname, ifr.ifr_mtu, ETH_DATA_LEN);
	//printErr(buffer);
    }
#ifdef HAVE_STRUCT_SOCKADDR_LL
    /* Get interface index */
    sa.sll_family = AF_PACKET;
    sa.sll_protocol = htons(type);

    strncpy(ifr.ifr_name, ifname, sizeof(ifr.ifr_name));
    if (ioctl(fd, SIOCGIFINDEX, &ifr) < 0) {
	fatalSys("ioctl(SIOCFIGINDEX): Could not get interface index");
    }
    sa.sll_ifindex = ifr.ifr_ifindex;

#else
    strcpy(sa.sa_data, ifname);
#endif

    /* We're only interested in packets on specified interface */
    if (bind(fd, (struct sockaddr *) &sa, sizeof(sa)) < 0) {
	fatalSys("bind");
    }

    return fd;
}
static int DiscoverySocket;
//static int SessionSocket;
const char *IfName = "eth0";
#define ETH_PPPOE_DISCOVERY 0x8863
unsigned char MyEthAddr[ETH_ALEN];   /* Source hardware address */
unsigned char PeerEthAddr[ETH_ALEN];	/* Destination hardware address */
UINT16_t Eth_PPPOE_Discovery = ETH_PPPOE_DISCOVERY;
static int DiscoveryState;
static int optPrintACNames = 0;	/* Only print access concentrator names */
static int NumPADOPacketsReceived = 0;	/* Number of PADO packets received */
char *ServiceName = NULL;       /* Desired service name */
void
receivePacket(int sock, struct PPPoEPacket *pkt, int *size)
{
    if ((*size = recv(sock, pkt, sizeof(struct PPPoEPacket), 0)) < 0) {
	fatalSys("recv (receivePacket)");
    }

}

void
sendPacket(int sock, struct PPPoEPacket *pkt, int size)
{
    if (send(sock, pkt, size, 0) < 0) {
	fatalSys("send (sendPacket)");
    }
}
static struct PPPoETag cookie;		/* We have to send this if we get it */
struct PPPoETag relayId;		/* Ditto */
static char *DesiredACName = NULL;	/* Desired access concentrator */
void
parsePADOTags(UINT16_t type, UINT16_t len, unsigned char *data,
	      void *extra)
{
    struct PacketCriteria *pc = (struct PacketCriteria *) extra;
    int i;

    switch(type) {
    case TAG_AC_NAME:
	if (optPrintACNames) {
	    printf("Access-Concentrator: %.*s\n", (int) len, data);
	}
	if (DesiredACName && len == strlen(DesiredACName) &&
	    !strncmp((char *) data, DesiredACName, len)) {
	    pc->acNameOK = 1;
	}
	break;
    case TAG_SERVICE_NAME:
	if (optPrintACNames && len > 0) {
	    printf("       Service-Name: %.*s\n", (int) len, data);
	}
	if (ServiceName && len == strlen(ServiceName) &&
	    !strncmp((char *) data, ServiceName, len)) {
	    pc->serviceNameOK = 1;
	}
	break;
    case TAG_AC_COOKIE:
	if (optPrintACNames) {
	    printf("Got a cookie:");
	    /* Print first 20 bytes of cookie */
	    for (i=0; i<len && i < 20; i++) {
		printf(" %02x", (unsigned) data[i]);
	    }
	    if (i < len) printf("...");
	    printf("\n");
	}
	cookie.type = htons(type);
	cookie.length = htons(len);
	memcpy(cookie.payload, data, len);
	break;
    case TAG_RELAY_SESSION_ID:
	if (optPrintACNames) {
	    printf("Got a Relay-ID\n");
	}
	relayId.type = htons(type);
	relayId.length = htons(len);
	memcpy(relayId.payload, data, len);
	break;
    case TAG_SERVICE_NAME_ERROR:
	if (optPrintACNames) {
	    printf("Got a Service-Name-Error tag: %.*s\n", (int) len, data);
	} else {
	    syslog(LOG_ERR, "PADO: Service-Name-Error: %.*s", (int) len, data);
	    exit(1);
	}
	break;
    case TAG_AC_SYSTEM_ERROR:
	if (optPrintACNames) {
	    printf("Got a System-Error tag: %.*s\n", (int) len, data);
	} else {
	    syslog(LOG_ERR, "PADO: System-Error: %.*s", (int) len, data);
	    exit(1);
	}
	break;
    case TAG_GENERIC_ERROR:
	if (optPrintACNames) {
	    printf("Got a Generic-Error tag: %.*s\n", (int) len, data);
	} else {
	    syslog(LOG_ERR, "PADO: Generic-Error: %.*s", (int) len, data);
	    exit(1);
	}
	break;
    }
}
int
parsePacket(struct PPPoEPacket *packet, ParseFunc *func, void *extra)
{
    UINT16_t len = ntohs(packet->length);
    unsigned char *curTag;
    UINT16_t tagType, tagLen;

    if (packet->ver != 1) {
	syslog(LOG_ERR, "Invalid PPPoE version (%d)", (int) packet->ver);
	return -1;
    }
    if (packet->type != 1) {
	syslog(LOG_ERR, "Invalid PPPoE type (%d)", (int) packet->type);
	return -1;
    }

    /* Do some sanity checks on packet */
    if (len > ETH_DATA_LEN - 6) { /* 6-byte overhead for PPPoE header */
	syslog(LOG_ERR, "Invalid PPPoE packet length (%u)", len);
	return -1;
    }

    /* Step through the tags */
    curTag = packet->payload;
    while(curTag - packet->payload < len) {
	/* Alignment is not guaranteed, so do this by hand... */
	tagType = (((UINT16_t) curTag[0]) << 8) +
	    (UINT16_t) curTag[1];
	tagLen = (((UINT16_t) curTag[2]) << 8) +
	    (UINT16_t) curTag[3];
	if (tagType == TAG_END_OF_LIST) {
	    return 0;
	}
	if ((curTag - packet->payload) + tagLen + TAG_HDR_SIZE > len) {
	    syslog(LOG_ERR, "Invalid PPPoE tag length (%u)", tagLen);
	    return -1;
	}
	func(tagType, tagLen, curTag+TAG_HDR_SIZE, extra);
	curTag = curTag + TAG_HDR_SIZE + tagLen;
    }
    return 0;
}
#define BPF_BUFFER_IS_EMPTY 1
void
waitForPADO(int timeout)
{
    fd_set readable;
    int r;
    struct timeval tv;
    struct PPPoEPacket packet;
    int len;

    struct PacketCriteria pc;
    pc.acNameOK      = (DesiredACName)    ? 0 : 1;
    pc.serviceNameOK = (ServiceName)      ? 0 : 1;
	
    do {
	if (BPF_BUFFER_IS_EMPTY) {
	    tv.tv_sec = timeout;
	    tv.tv_usec = 0;
	
	    FD_ZERO(&readable);
	    FD_SET(DiscoverySocket, &readable);

	    while(1) {
		r = select(DiscoverySocket+1, &readable, NULL, NULL, &tv);
		if (r >= 0 || errno != EINTR) break;
	    }
	    if (r < 0) {
		fatalSys("select (waitForPADO)");
	    }
	    if (r == 0) return;        /* Timed out */
	}
	
	/* Get the packet */
	receivePacket(DiscoverySocket, &packet, &len);

	/* Check length */
	if (ntohs(packet.length) + HDR_SIZE > len) {
	    syslog(LOG_ERR, "Bogus PPPoE length field");
	    continue;
	}

#ifdef USE_BPF
	/* If it's not a Discovery packet, loop again */
	if (etherType(&packet) != Eth_PPPOE_Discovery) continue;
#endif
/*
	if (DebugFile) {
	    fprintf(DebugFile, "RCVD ");
	    dumpPacket(DebugFile, &packet);
	    fprintf(DebugFile, "\n");
	    fflush(DebugFile);
	}
*/
	/* If it's not for us, loop again */
#if 0 /* FIXME: ignore */
	if (!packetIsForMe(&packet)) continue;
#endif 
	if (packet.code == CODE_PADO) {
	    NumPADOPacketsReceived++;
	    if (optPrintACNames) {
		printf("--------------------------------------------------\n");
	    }
	    parsePacket(&packet, parsePADOTags, &pc);
	    if (pc.acNameOK && pc.serviceNameOK) {
		memcpy(PeerEthAddr, packet.ethHdr.h_source, ETH_ALEN);
		if (optPrintACNames) {
		    printf("AC-Ethernet-Address: %02x:%02x:%02x:%02x:%02x:%02x\n",
			   (unsigned) PeerEthAddr[0], 
			   (unsigned) PeerEthAddr[1],
			   (unsigned) PeerEthAddr[2],
			   (unsigned) PeerEthAddr[3],
			   (unsigned) PeerEthAddr[4],
			   (unsigned) PeerEthAddr[5]);
		    continue;
		}
		DiscoveryState = STATE_RECEIVED_PADO;
		break;
	    }
	}
    } while (DiscoveryState != STATE_RECEIVED_PADO);
}


static unsigned char BroadcastAddr[ETH_ALEN] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
void
sendPADI(void)
{
    struct PPPoEPacket packet;
    unsigned char *cursor = packet.payload;
    struct PPPoETag *svc = (struct PPPoETag *) (&packet.payload);
    UINT16_t namelen = 0;
    UINT16_t plen;

    if (ServiceName) {
	namelen = (UINT16_t) strlen(ServiceName);
    }
    plen = TAG_HDR_SIZE + namelen;
    CHECK_ROOM(cursor, packet.payload, plen);

    memcpy(packet.ethHdr.h_dest, BroadcastAddr, ETH_ALEN);
    memcpy(packet.ethHdr.h_source, MyEthAddr, ETH_ALEN);

    packet.ethHdr.h_proto = htons(Eth_PPPOE_Discovery);
    packet.ver = 1;
    packet.type = 1;
    packet.code = CODE_PADI;
    packet.session = 0;

    svc->type = TAG_SERVICE_NAME;
    svc->length = htons(namelen);
    CHECK_ROOM(cursor, packet.payload, namelen+TAG_HDR_SIZE);

    if (ServiceName) {
	memcpy(svc->payload, ServiceName, strlen(ServiceName));
    }
    cursor += namelen + TAG_HDR_SIZE;

	/* FIXME: ignored */
#if 0
    /* If we're using Host-Uniq, copy it over */
    if (optUseHostUnique) {
	struct PPPoETag hostUniq;
	pid_t pid = getpid();
	hostUniq.type = htons(TAG_HOST_UNIQ);
	hostUniq.length = htons(sizeof(pid));
	memcpy(hostUniq.payload, &pid, sizeof(pid));
	CHECK_ROOM(cursor, packet.payload, sizeof(pid) + TAG_HDR_SIZE);
	memcpy(cursor, &hostUniq, sizeof(pid) + TAG_HDR_SIZE);
	cursor += sizeof(pid) + TAG_HDR_SIZE;
	plen += sizeof(pid) + TAG_HDR_SIZE;
    }
#endif

    packet.length = htons(plen);

    sendPacket(DiscoverySocket, &packet, (int) (plen + HDR_SIZE));
/*
    if (DebugFile) {
	fprintf(DebugFile, "SENT ");
	dumpPacket(DebugFile, &packet);
	fprintf(DebugFile, "\n");
	fflush(DebugFile);
    }
*/
}

void
discovery(void)
{
    int padiAttempts = 0;
    int padrAttempts = 0;
    int timeout = PADI_TIMEOUT;
    DiscoverySocket = openInterface(IfName, Eth_PPPOE_Discovery, MyEthAddr);


    do {
	padiAttempts++;
	if (padiAttempts > MAX_PADI_ATTEMPTS) {
	    fatal("Timeout waiting for PADO packets");
	    break;
	}
	sendPADI();
	DiscoveryState = STATE_SENT_PADI;
	printf("timeout:%d\n", timeout);
	waitForPADO(timeout);

	/* If we're just probing for access concentrators, don't do
	   exponential backoff.  This reduces the time for an unsuccessful
	   probe to 15 seconds. */
	if (!optPrintACNames) {
	    timeout *= 2;
	}
	if (optPrintACNames && NumPADOPacketsReceived) {
	    break;
	}
    } while (DiscoveryState == STATE_SENT_PADI);

    /* If we're only printing access concentrator names, we're done */
    if (optPrintACNames) {
	printf("--------------------------------------------------\n");
	exit(0);
    }

#if 0 /* FIXME: */
    timeout = PADI_TIMEOUT;
    do {
	padrAttempts++;
	if (padrAttempts > MAX_PADI_ATTEMPTS) {
	    fatal("Timeout waiting for PADS packets");
	}
#if 0 /* FIXME: */
	sendPADR();
#endif
	DiscoveryState = STATE_SENT_PADR;
	waitForPADS(timeout);
	timeout *= 2;
    } while (DiscoveryState == STATE_SENT_PADR);

#endif
    /* We're done. */
    //DiscoveryState = STATE_SESSION;
    return;
}

#if 0
void
session(void)
{
    fd_set readable;
    struct PPPoEPacket packet;
    struct timeval tv;
    struct timeval *tvp = NULL;
    int maxFD = 0;
    int r;

    /* Open a session socket */
    SessionSocket = openInterface(IfName, Eth_PPPOE_Session, NULL);

    /* Prepare for select() */
    if (SessionSocket > maxFD) maxFD = SessionSocket;
    if (DiscoverySocket > maxFD) maxFD = DiscoverySocket;
    maxFD++;

    /* Fill in the constant fields of the packet to save time */
    memcpy(packet.ethHdr.h_dest, PeerEthAddr, ETH_ALEN);
    memcpy(packet.ethHdr.h_source, MyEthAddr, ETH_ALEN);
    packet.ethHdr.h_proto = htons(Eth_PPPOE_Session);
    packet.ver = 1;
    packet.type = 1;
    packet.code = CODE_SESS;
    packet.session = Session;

    //initPPP();

#ifdef USE_BPF
    /* check for buffered session data */
    while (BPF_BUFFER_HAS_DATA) {
	if (Synchronous) {
	    syncReadFromEth(SessionSocket, optClampMSS);
	} else {
	    asyncReadFromEth(SessionSocket, optClampMSS);
	}
    }
#endif
#if 0
    for (;;) {
	if (optInactivityTimeout > 0) {
	    tv.tv_sec = optInactivityTimeout;
	    tv.tv_usec = 0;
	    tvp = &tv;
	}
	FD_ZERO(&readable);
	FD_SET(0, &readable);     /* ppp packets come from stdin */
	FD_SET(DiscoverySocket, &readable);
	FD_SET(SessionSocket, &readable);
	while(1) {
	    r = select(maxFD, &readable, NULL, NULL, tvp);
	    if (r >= 0 || errno != EINTR) break;
	}
	if (r < 0) {
	    fatalSys("select (session)");
	}
	if (r == 0) { /* Inactivity timeout */
	    syslog(LOG_ERR, "Inactivity timeout... something wicked happened");
	    sendPADT();
	    exit(1);
	}

	/* Handle ready sockets */
	if (FD_ISSET(0, &readable)) {
	    if (Synchronous) {
		syncReadFromPPP(&packet);
	    } else {
		asyncReadFromPPP(&packet);
	    }
	}

	if (FD_ISSET(SessionSocket, &readable)) {
	    do {
		if (Synchronous) {
		    syncReadFromEth(SessionSocket, optClampMSS);
		} else {
		    asyncReadFromEth(SessionSocket, optClampMSS);
		}
	    } while (BPF_BUFFER_HAS_DATA);
	}

#ifndef USE_BPF	
	/* BSD uses a single socket, see *syncReadFromEth() */
	/* for calls to sessionDiscoveryPacket() */
	if (FD_ISSET(DiscoverySocket, &readable)) {
	    sessionDiscoveryPacket();
	}
#endif

    }
#endif 
}
#endif

int pppoed(const char *eth)
{
	IfName = eth;
	discovery();
	if (DiscoveryState == STATE_RECEIVED_PADO)
		printf("recv PADO\n");
	else if (DiscoveryState == STATE_SENT_PADR)
		printf("not recv PADO\n");
	else
		printf("others stat:%d\n", DiscoveryState);
	return DiscoveryState == STATE_RECEIVED_PADO;
}
/*
int main(int argc, char *argv[])
{
	discovery();
	return !(DiscoveryState == STATE_RECEIVED_PADO);
}
*/
