
/* modified from tod6d.c */

/*
 * System header files. 
 */
#include <errno.h>          /* errno declaration & error codes.            */
#include <netdb.h>          /* getaddrinfo(3) et al.                       */
/* netdb.h already include netinet/in.h */
//#include <netinet/in.h>     /* sockaddr_in & sockaddr_in6 definition.      */
#include <stdio.h>          /* printf(3) et al.                            */
#include <stdlib.h>         /* exit(2).                                    */
#include <string.h>         /* String manipulation & memory functions.     */
#include <sys/poll.h>       /* poll(2) and related definitions.            */
#include <sys/socket.h>     /* Socket functions (socket(2), bind(2), etc). */
#include <unistd.h>         /* getopt(3), read(2), etc.                    */
#include <sys/types.h>  
#include <arpa/inet.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <net/if_arp.h>

#include "llmnr.h"

static struct sockaddr_in addr;
static struct sockaddr_in6 addr6;
/*
  Jery Lin added 2010/2/1
  bind_if6_idx store which interface should be listening
  default is zero for automatic, but zero usually select fault interface
*/
unsigned int bind_if6_idx=0;


/*
 * Constants.
 */
#define DFLT_SERVICE LLMNR_PORT /* Default service name.                    */
#define INVALID_DESC -1         /* Invalid file descriptor.                 */
#define MAXCONNQLEN  3          /* Max nbr of connection requests to queue. */
#define MAXUDPSCKTS  2          /* One UDP socket for IPv4 & one for IPv6.  */
#define VALIDOPTS    "v"        /* Valid command options.                   */

/*
 * Simple boolean type definition.
 */
typedef enum { false = 0, true } boolean;

/*
 * Prototypes for internal helper functions.
 */
static int openSckt( const char *service,
                     const char *protocol,
                     int         desc[ ],
                     size_t     *descSize );
static void process_llmnr( int    uSckt[ ],
                 size_t uScktSize );

/*
 * Global (within this file only) data objects.
 */
static char        hostBfr[ NI_MAXHOST ];   /* For use w/getnameinfo(3).    */
static const char *pgmName;                 /* Program name w/o dir prefix. */
static char        servBfr[ NI_MAXSERV ];   /* For use w/getnameinfo(3).    */
static boolean     verbose = false;         /* Verbose mode indication.     */
/*
 * Usage macro for command syntax violations.
 */
#define USAGE                                           \
        {                                               \
            fprintf( stderr,                            \
                    "Usage: %s [-v] [interface] [hostname] [MAC Address]\n",       \
                    pgmName );                          \
            exit( 127 );                                \
        }  /* End USAGE macro. */

/*
 * Macro to terminate the program if a system call error occurs.  The system
 * call must be one of the usual type that returns -1 on error.  This macro is
 * a modified version of a macro authored by Dr. V. Vinge, SDSU Dept. of
 * Computer Science (retired)... best professor I ever had.  I hear he writes
 * great science fiction in addition to robust code, too.
 */
#define CHK(expr)                                                       \
        do                                                              \
        {                                                               \
            if ( (expr) == -1 )                                         \
            {                                                           \
                fprintf( stderr,                                        \
                        "%s (line %d): System call ERROR - %s.\n",      \
                        pgmName,                                        \
                        __LINE__,                                       \
                        strerror( errno ) );                            \
                exit( 1 );                                              \
            }   /* End IF system call failed. */                        \
        } while ( false )

char host_name[64];
char if_name[8];
unsigned char host_mac_name[64];//NickChou for http://dlinkrouterWXYZ


/******************************************************************************
 * Function: main
 *
 * Description:
 *    Set up a llmnr server and handle network requests.  This server
 *    handles UDP mcast requests.
 *
 * Parameters:
 *    The usual argc and argv parameters to a main() function.
 *
 * Return Value:
 *    This is a daemon program and never returns.  However, in the degenerate
 *    case where no sockets are created, the function returns zero.
 *******************************************************************************/
int main(int argc, char *argv[ ] )
{
    int         opt;
    const char *service   = DFLT_SERVICE;
    int         uSckt[ MAXUDPSCKTS ];     /* Array of UDP socket descriptors. */
    size_t      uScktSize = MAXUDPSCKTS;  /* Size of uSckt (# of elements).   */
   
    char mac_addr[18]={0};
    int mac[6] = {0};
   
    /*
     *  Set the program name (w/o directory prefix).
     */
    pgmName = strrchr( argv[ 0 ], '/' );
    pgmName = pgmName == NULL  ?  argv[ 0 ]  :  pgmName + 1;
    
    /*
     * Process command options.
     */
    opterr = 0;   /* Turns off "invalid option" error messages. */
    while ( ( opt = getopt( argc, argv, VALIDOPTS ) ) >= 0 )
    {
        switch ( opt )
        {
            case 'v':   /* Verbose mode. */
            {
                verbose = true;
                argc--;
                argv++;
                break;
            }
            default:
            {
                USAGE;
            }
        }  /* End SWITCH on command option. */
    }  /* End WHILE processing options. */
   
    /*
     * Process command line arguments.
     */
    if (verbose){
    	fprintf(stderr, "argc = %d\n", argc);
    	printf("if = %s, name = %s, mac = %s\n", argv[1], argv[2], argv[3]);
	}

    /*Device response to LLMNR for both http://[lan_device_name][WXYZ] 
                   and http://[lan_device_name]*/
    //if (argc != 3) USAGE;
    if (argc != 4) USAGE;

    strncpy(if_name, argv[1], 8);
    strncpy(host_name, argv[2], 64);

    strncpy(mac_addr, argv[3], strlen(argv[3]));
    if (verbose) printf("mac_addr = %s\n", mac_addr);
    sscanf(mac_addr,"%02x:%02x:%02x:%02x:%02x:%02x", &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5]);
    sprintf(host_mac_name, "%s%02x%02x", host_name, mac[4], mac[5]);
    if (verbose) printf("host_mac_name = %s\n", host_mac_name);

    /*
     * Open UDP socket, for both IPv4 & IPv6, on which to receive
     * service requests.
     */
    if ( openSckt( service, "udp", uSckt, &uScktSize ) < 0  )
    {
        exit( 1 );
    }

    /*
     * Run the llmnr server.
     */
    if ( uScktSize > 0 )
    {
        process_llmnr( uSckt,         /* process_llmnr() never returns. */
            uScktSize);
    }
   
    /*
     * Since process_llmnr() never returns, execution only gets here if no sockets were
     * created.
     */
    if ( verbose )
    {
        fprintf( stderr,
            "%s: No sockets opened... terminating.\n",
            pgmName );
    }
   
    return 0;
    
}  /* End main() */

static int get_local_v4addr(unsigned char *ifname, unsigned char *addr4)
{
    int iSocket; 
    struct if_nameindex *pIndex, *pIndex2; 
    
    if ((iSocket = socket(PF_INET, SOCK_DGRAM, 0)) < 0) 
    { 
        perror("socket"); 
        return -1; 
    } 
    
    pIndex = pIndex2 = if_nameindex(); 
    while ((pIndex != NULL) && (pIndex->if_name != NULL)) 
    { 
        struct ifreq req; 
        
        strncpy(req.ifr_name, pIndex->if_name, IFNAMSIZ); 
        
        if ( verbose )
        	 fprintf(stderr, "LLMNR: get_local_v4addr: pIndex->if_name = %s\n", pIndex->if_name);
         
        if (ioctl(iSocket, SIOCGIFADDR, &req) < 0) 
        { 
            if (errno == EADDRNOTAVAIL) 
            { 
                pIndex++; 
                continue; 
            } 
            perror("LLMNR: ioctl"); 
            close(iSocket); 
            return -1; 
        } 
        
        if (strcmp(ifname, pIndex->if_name) == 0)
        {
            sprintf(addr4, inet_ntoa(((struct sockaddr_in*)&req.ifr_addr)->sin_addr));
            
            if ( verbose )
                fprintf(stderr, "LLMNR: get_local_v4addr: %s v4 addr = %s\n", ifname, addr4);
            
            if_freenameindex(pIndex2); 
            close(iSocket); 
            return 0;
        }
        pIndex++; 
    } 
    
    if_freenameindex(pIndex2); 
    close(iSocket); 
    return -1; 
}

static char* get_local_v6addr(unsigned char *ifname, unsigned char *s_addr6, char *s_scope6)
{
    FILE *fp;
    char str_addr[40];
    char *p_addr = NULL;
    unsigned int plen, scope, dad_status, if_idx;
    char devname[IFNAMSIZ];
#define PATH_PROC_NET_IF_INET6 "/proc/net/if_inet6"
// copy from {kernel}/include/net/ipv6.h
#define IPV6_ADDR_LOOPBACK      0x0010U
#define IPV6_ADDR_LINKLOCAL     0x0020U
#define IPV6_ADDR_SITELOCAL     0x0040U
// copy from {kernel}/include/linux/if_addr.h
/* ifa_flags */
#define IFA_F_SECONDARY         0x01
#define IFA_F_TEMPORARY         IFA_F_SECONDARY
#define IFA_F_NODAD             0x02
#define IFA_F_OPTIMISTIC        0x04
#define IFA_F_DADFAILED         0x08
#define IFA_F_HOMEADDRESS       0x10
#define IFA_F_DEPRECATED        0x20
#define IFA_F_TENTATIVE         0x40
#define IFA_F_PERMANENT         0x80

    if ((fp = fopen(PATH_PROC_NET_IF_INET6, "r")) == NULL)
    {
        printf("can't open %s: %s", PATH_PROC_NET_IF_INET6, strerror(errno));
        return 0;    
    }
    
    while (fscanf(fp, "%32s %x %02x %02x %02x %15s\n",
              str_addr, &if_idx, &plen, &scope, &dad_status,
              devname) != EOF)
    {
        if (strcmp(ifname, devname) == 0)
        {
            if (!bind_if6_idx)
                bind_if6_idx = if_idx;
	    
            // check addr is dad success
            if (!(dad_status & IFA_F_TENTATIVE))
            {
                if (strcmp(s_scope6, "linklocal")==0 && (scope & IPV6_ADDR_LINKLOCAL))
                {
                    p_addr = s_addr6;
                }
                else if (strcmp(s_scope6, "global")==0 && 
                         !(scope & (IPV6_ADDR_LINKLOCAL | IPV6_ADDR_LOOPBACK | IPV6_ADDR_SITELOCAL)) )
                {
                    p_addr = s_addr6;
                }
            }

	    if (p_addr)
	    {
                memcpy(p_addr   , str_addr+ 0, 4);
                *(p_addr+ 4) = ':';
                memcpy(p_addr+ 5, str_addr+ 4, 4);
                *(p_addr+ 9) = ':';
                memcpy(p_addr+10, str_addr+ 8, 4);
                *(p_addr+14) = ':';
                memcpy(p_addr+15, str_addr+12, 4);
                *(p_addr+19) = ':';
                memcpy(p_addr+20, str_addr+16, 4);
                *(p_addr+24) = ':';
                memcpy(p_addr+25, str_addr+20, 4);
                *(p_addr+29) = ':';
                memcpy(p_addr+30, str_addr+24, 4);
                *(p_addr+34) = ':';
                memcpy(p_addr+35, str_addr+28, 4);
                *(p_addr+39) = 0;
                break;
            }
        }
    }
    
    fclose(fp);
    
    
    if (p_addr)
    {
        if ( verbose )
            fprintf(stderr, "LLMNR: get_local_v6addr: %s v6 addr = %s\n", ifname, s_addr6);
    }
    return p_addr;
}

/******************************************************************************
 * Function: openSckt
 *
 * Description:
 *    Open passive (server) sockets for the indicated inet service & protocol.
 *    Notice in the last sentence that "sockets" is plural.  During the interim
 *    transition period while everyone is switching over to IPv6, the server
 *    application has to open two sockets on which to listen for connections...
 *    one for IPv4 traffic and one for IPv6 traffic.
 *
 * Parameters:
 *    service  - Pointer to a character string representing the well-known port
 *               on which to listen (can be a service name or a decimal number).
 *    protocol - Pointer to a character string representing the transport layer
 *               protocol (only "udp" are valid).
 *    desc     - Pointer to an array into which the socket descriptors are
 *               placed when opened.
 *    descSize - This is a value-result parameter.  On input, it contains the
 *               max number of descriptors that can be put into 'desc' (i.e. the
 *               number of elements in the array).  Upon return, it will contain
 *               the number of descriptors actually opened.  Any unused slots in
 *               'desc' are set to INVALID_DESC.
 *
 * Return Value:
 *    0 on success, -1 on error.
 ******************************************************************************/
static int openSckt( const char *service,
                     const char *protocol,
                     int         desc[ ],
                     size_t     *descSize )
{
    struct addrinfo *ai;
    int              aiErr;
    struct addrinfo *aiHead;
    struct addrinfo  hints    = { .ai_flags  = AI_PASSIVE,    /* Server mode.  */
                                  .ai_family = PF_UNSPEC };   /* IPv4 or IPv6. */
    size_t           maxDescs = *descSize;
    struct addrinfo *addr_info;
   
    /*
     * Initialize output parameters.  When the loop completes, *descSize is 0.
     */
    while ( *descSize > 0 )
    {
        desc[ --( *descSize ) ] = INVALID_DESC;
    }
    
    /*
     * Check which protocol is selected (only UDP are valid).
     */
    if ( strcmp( protocol, "udp" ) == 0 )   /* UDP protocol.     */
    {
        hints.ai_socktype = SOCK_DGRAM;
        hints.ai_protocol = IPPROTO_UDP;
    }
    else                                         /* Invalid protocol. */
    {
        fprintf( stderr,
                "%s (line %d): ERROR - Unsupported  transport "
                "layer protocol \"%s\".\n",
                pgmName,
                __LINE__,
                protocol );
        return -1;
    }
    
    /*
     * Look up the service's well-known port number.  Notice that NULL is being
     * passed for the 'node' parameter, and that the AI_PASSIVE flag is set in
     * 'hints'.  Thus, the program is requesting passive address information.
     * The network address is initialized to :: (all zeros) for IPv6 records, or
     * 0.0.0.0 for IPv4 records.
     */
    if ( ( aiErr = getaddrinfo( NULL,
                               service,
                               &hints,
                               &aiHead ) ) != 0 )
    {
        fprintf( stderr,
                "%s (line %d): ERROR - %s.\n",
                pgmName,
                __LINE__,
                gai_strerror( aiErr ) );
        return -1;
    }
   
    /*
     * For each of the address records returned, attempt to set up a passive
     * socket.
     */
    for ( ai = aiHead;
        ( ai != NULL ) && ( *descSize < maxDescs );
        ai = ai->ai_next )
    {
        if ( verbose )
        {
            /*
             * Display the current address info.   Start with the protocol-
             * independent fields first.
             */
            fprintf( stderr,
                "Setting up a passive socket based on the "
                "following address info:\n"
                "   ai_flags     = 0x%02X\n"
                "   ai_family    = %d (PF_INET = %d, PF_INET6 = %d)\n"
                "   ai_socktype  = %d (SOCK_STREAM = %d, SOCK_DGRAM = %d)\n"
                "   ai_protocol  = %d (IPPROTO_TCP = %d, IPPROTO_UDP = %d)\n"
                "   ai_addrlen   = %d (sockaddr_in = %d, "
                "sockaddr_in6 = %d)\n",
                ai->ai_flags,
                ai->ai_family,
                PF_INET,
                PF_INET6,
                ai->ai_socktype,
                SOCK_STREAM,
                SOCK_DGRAM,
                ai->ai_protocol,
                IPPROTO_TCP,
                IPPROTO_UDP,
                ai->ai_addrlen,
                sizeof( struct sockaddr_in ),
                sizeof( struct sockaddr_in6 ) );
            
            /*
             * Now display the protocol-specific formatted socket address.  Note
             * that the program is requesting that getnameinfo(3) convert the
             * host & service into numeric strings.
             */
            getnameinfo( ai->ai_addr,
                      ai->ai_addrlen,
                      hostBfr,
                      sizeof( hostBfr ),
                      servBfr,
                      sizeof( servBfr ),
                      NI_NUMERICHOST | NI_NUMERICSERV );
         
            switch ( ai->ai_family )
            {
                case PF_INET:   /* IPv4 address record. */
                {
                    struct sockaddr_in *p = (struct sockaddr_in*) ai->ai_addr;
                    fprintf( stderr,
                        "   ai_addr      = sin_family:   %d (AF_INET = %d, "
                        "AF_INET6 = %d)\n"
                        "                  sin_addr:     %s\n"
                        "                  sin_port:     %s\n",
                        p->sin_family,
                        AF_INET,
                        AF_INET6,
                        hostBfr,
                        servBfr );
                    break;
                }  /* End CASE of IPv4. */
            
                case PF_INET6:   /* IPv6 address record. */
                {
                    struct sockaddr_in6 *p = (struct sockaddr_in6*) ai->ai_addr;
                    fprintf( stderr,
                        "   ai_addr      = sin6_family:   %d (AF_INET = %d, "
                        "AF_INET6 = %d)\n"
                        "                  sin6_addr:     %s\n"
                        "                  sin6_port:     %s\n"
                        "                  sin6_flowinfo: %d\n"
                        "                  sin6_scope_id: %d\n",
                        p->sin6_family,
                        AF_INET,
                        AF_INET6,
                        hostBfr,
                        servBfr,
                        p->sin6_flowinfo,
                        p->sin6_scope_id );
                    break;
                }  /* End CASE of IPv6. */
            
                default:   /* Can never get here, but just for completeness. */
                {
                    fprintf( stderr,
                        "%s (line %d): ERROR - Unknown protocol family (%d).\n",
                        pgmName,
                        __LINE__,
                        ai->ai_family );
                    freeaddrinfo( aiHead );
                    return -1;
                }  /* End DEFAULT case (unknown protocol family). */
            }  /* End SWITCH on protocol family. */
        }  /* End IF verbose mode. */
      
        /*
         * Create a socket using the info in the addrinfo structure.
         */
        CHK( desc[ *descSize ] = socket( ai->ai_family,
                                       ai->ai_socktype,
                                       ai->ai_protocol ) );
        /*
         * Here is the code that prevents "IPv4 mapped addresses", as discussed
         * in Section 22.1.3.1.  If an IPv6 socket was just created, then set the
         * IPV6_V6ONLY socket option.
         */
        if ( ai->ai_family == PF_INET6 )
        {
#if defined( IPV6_V6ONLY )
            /*
             * Disable IPv4 mapped addresses.
             */
            int v6Only = 1;
            CHK( setsockopt( desc[ *descSize ],
                          IPPROTO_IPV6,
                          IPV6_V6ONLY,
                          &v6Only,
                          sizeof( v6Only ) ) );
#else
            /*
             * IPV6_V6ONLY is not defined, so the socket option can't be set and
             * thus IPv4 mapped addresses can't be disabled.  Print a warning
             * message and close the socket.  Design note: If the
             * #if...#else...#endif construct were removed, then this program
             * would not compile (because IPV6_V6ONLY isn't defined).  That's an
             * acceptable approach; IPv4 mapped addresses are certainly disabled
             * if the program can't build!  However, since this program is also
             * designed to work for IPv4 sockets as well as IPv6, I decided to
             * allow the program to compile when IPV6_V6ONLY is not defined, and
             * turn it into a run-time warning rather than a compile-time error.
             * IPv4 mapped addresses are still disabled because _all_ IPv6 traffic
             * is disabled (all IPv6 sockets are closed here), but at least this
             * way the server can still service IPv4 network traffic.
             */
            fprintf( stderr,
                "%s (line %d): WARNING - Cannot set IPV6_V6ONLY socket "
                "option.  Closing IPv6 %s socket.\n",
                pgmName,
                __LINE__,
                ai->ai_protocol == IPPROTO_TCP  ?  "TCP"  :  "UDP" );
            CHK( close( desc[ *descSize ] ) );
            
            continue;   /* Go to top of FOR loop w/o updating *descSize! */
#endif /* IPV6_V6ONLY */
        }  /* End IF this is an IPv6 socket. */
      
        /*
         * Bind the socket.  Again, the info from the addrinfo structure is used.
         */
        CHK( bind( desc[ *descSize ],
            ai->ai_addr,
            ai->ai_addrlen ) );
        

        if (ai->ai_family == PF_INET) /* IPv4 */
        {
            struct ip_mreq imr; /* Ip multicast membership */
            char v4_addr[20];
    
            get_local_v4addr (if_name, v4_addr);
    
             if ( verbose )
             	printf("LLMNR: openSckt: %s v4_addr=%s\n", if_name, v4_addr);
    
            /* setting up imr structure */
            imr.imr_multiaddr.s_addr = inet_addr(LLMNR_MCAST_V4GROUP);/* multicast group to join */
            
            /* interface to join on */ 
            /* set imr_interface to INADDR_ANY for default interface */ 
            imr.imr_interface.s_addr = inet_addr(v4_addr);//NickChou htonl(INADDR_ANY);

            /* If a sender is also a member of the multicast group, 
            it can control if an outgoing packet should loop back to itself
			u_char loop;  0/1: disable/enable loop back 
			setsockopt(sock, IPPROTO_IP, IP_MULTICAST_LOOP, &loop, sizeof(loop));
			*/ 
			
			/* Join a multicast group */
            if (setsockopt(desc[ *descSize ], IPPROTO_IP, IP_ADD_MEMBERSHIP, (void *)&imr, sizeof(struct ip_mreq)) < 0)
            {
                    perror("setsockopt(udp, IP_ADD_MEMBERSHIP)");
                    return -1;
            }
        }

        if (ai->ai_family == PF_INET6) /* IPv6 */
        {
            struct ipv6_mreq mreq6;
            char v6_addr[50];
            int count=0;
    
            getaddrinfo( LLMNR_MCAST_V6GROUP, service, &hints, &addr_info);

            for (count=0; count<20; count++) {
                if (!get_local_v6addr(if_name, v6_addr, "linklocal")) {
                    printf("llmnr: have no available linklocal address. wait count=%d\n", count);
                    sleep(1);
                }
                else {
                	break;
                }
            }
            
            if (count>=20) {
                printf("llmnr: have no available linklocal address. exit.\n");
                exit(1);
            }
    
            memcpy(&mreq6.ipv6mr_multiaddr, 
                &((struct sockaddr_in6 *)(addr_info->ai_addr))->sin6_addr, 
                sizeof(mreq6.ipv6mr_multiaddr));
    
            mreq6.ipv6mr_interface = bind_if6_idx;	//Jery Lin modify 2010/02/01

            if (setsockopt (desc[ *descSize ], IPPROTO_IPV6, IPV6_ADD_MEMBERSHIP, &mreq6, sizeof(mreq6)) < 0) 
            {
                perror("setsockopt(udp, IPV6_ADD_MEMBERSHIP)");
                if (aiHead) freeaddrinfo( aiHead );
                if (addr_info) freeaddrinfo( addr_info );
                return -1;
            }
        }

        /*
         * Socket set up okay.  Bump index to next descriptor array element.
         */
        *descSize += 1;

    }  /* End FOR each address info structure returned. */
   
    /*
     * Dummy check for unused address records.
     */
    if ( verbose && ( ai != NULL ) )
    {
        fprintf( stderr,
            "%s (line %d): WARNING - Some address records were "
            "not processed due to insufficient array space.\n",
            pgmName,
            __LINE__ );
    }  /* End IF verbose and some address records remain unprocessed. */
    
    /*
     * Clean up.
     */
    if (aiHead) freeaddrinfo( aiHead );
    if (addr_info) freeaddrinfo( addr_info );
   
    return 0;
}  /* End openSckt() */

static void string_tolower(unsigned char *str)
{
    if (str == NULL) {
        perror("string_tolower");
        return;
    }
        
    while(*str)
    {
        *str = tolower(*str);
        str++;
    }
    
    return;
}

/******************************************************************************
 * Function: process_llmnr
 *
 * Description:
 *    Listen on a set of sockets and send the process_llmnr response to any
 *    clients.  This function never returns.
 *
 * Parameters:
 *    uSckt     - Array of UDP socket descriptors on which to listen.
 *    uScktSize - Size of the uSckt array (nbr of elements).
 *
 * Return Value: None.
 ******************************************************************************/
static void process_llmnr( int    uSckt[ ],
                 size_t uScktSize )
{
    char                     bfr[ MAX_BUFLEN ];
    ssize_t                  count;
    struct pollfd           *desc;

    size_t                   descSize = uScktSize;

    int                      idx;
    struct sockaddr         *sadr;
    socklen_t                sadrLen;
    struct sockaddr_storage  sockStor;
    int                      status;
    ssize_t                  wBytes;
   
    /*
     * Allocate memory for the poll(2) array.
     */
    desc = malloc( descSize * sizeof( struct pollfd ) );
    if ( desc == NULL )
    {
        fprintf( stderr,
            "%s (line %d): ERROR - %s.\n",
            pgmName,
            __LINE__,
            strerror( ENOMEM ) );
        exit( 1 );
    }
   
    /*
     * Initialize the poll(2) array.
     */

    for ( idx = 0;     idx < descSize;     idx++ )
    {
        desc[ idx ].fd      = uSckt[ idx ];
        desc[ idx ].events  = POLLIN;
        desc[ idx ].revents = 0;
    }
   
    /*
     * Main llmnr server loop.  Handles UDP requests.  This is
     * an interative server, and all requests are handled directly within the
     * main loop.
     */
    while ( true )   /* Do forever. */
    {
        /*
         * Wait for activity on one of the sockets.  The DO..WHILE construct is
         * used to restart the system call in the event the process is
         * interrupted by a signal.
         */
      
        do
        {
            status = poll( desc,
                        descSize,
                        -1 /* Wait indefinitely for input. */ );
        } while ( ( status < 0 ) && ( errno == EINTR ) );
      
        CHK( status );   /* Check for a bona fide system call error. */
      
        /*
         * Indicate that there is new network activity.
         */
        if ( verbose )
        {
            fprintf( stderr,
                "%s: New network activity ...\n",
                pgmName);
        }  /* End IF verbose. */
      
        /*
         * Process sockets with input available.
         */
        for ( idx = 0;     idx < descSize;     idx++ )
        {
            switch ( desc[ idx ].revents )
            {
                case 0:        /* No activity on this socket; try the next. */
                    continue;
                case POLLIN:   /* Network activity.  Go process it.         */
                    break;
                default:       /* Invalid poll events.                      */
                {
                    fprintf( stderr,
                        "%s (line %d): ERROR - Invalid poll idx=%d, event (0x%02X).\n",
                        pgmName,
                        __LINE__,
                        idx,
                        desc[ idx ].revents );
                    exit( 1 );
                }
            }  /* End SWITCH on returned poll events. */
         
            {

                sadrLen = sizeof( sockStor );
                sadr    = (struct sockaddr*) &sockStor;
                CHK( count = recvfrom( desc[ idx ].fd,
                                   bfr,
                                   sizeof( bfr ),
                                   0,
                                   sadr,
                                   &sadrLen ) );
                /*
                 * Display whatever was received on stdout.
                 */
                if ( verbose )
                {
                    ssize_t rBytes = count;
                    fprintf( stderr,
                        "%s: UDP datagram received (%d bytes).\n",
                        pgmName,
                        count );
               
                    while ( count > 0 )
                    {
                        int cnt = 0;
                        for (cnt = 0; cnt<16; cnt++)
                            fprintf(stderr, "%02x ", (unsigned char) bfr[ rBytes - count-- ]);
                        printf("\n");
                    }

                    count = rBytes;

                    if ( bfr[ rBytes-1 ] != '\n' )
                        fputc( '\n', stdout );   /* Newline also flushes stdout. */
               
                    /*
                     * Display the socket address of the remote client.  Address-
                     * independent fields first.
                     */
                    fprintf( stderr,
                        "Remote client's sockaddr info:\n"
                        "   sa_family = %d (AF_INET = %d, AF_INET6 = %d)\n"
                        "   addr len  = %d (sockaddr_in = %d, "
                        "sockaddr_in6 = %d)\n",
                        sadr->sa_family,
                        AF_INET,
                        AF_INET6,
                        sadrLen,
                        sizeof( struct sockaddr_in ),
                        sizeof( struct sockaddr_in6 ) );
                        
                    /*
                     * Display the address-specific information.
                     */
                    getnameinfo( sadr,
                        sadrLen,
                        hostBfr,
                        sizeof( hostBfr ),
                        servBfr,
                        sizeof( servBfr ),
                        NI_NUMERICHOST | NI_NUMERICSERV );
               
                    switch ( sadr->sa_family )
                    {
                        case AF_INET:   /* IPv4 address. */
                        {
                            struct sockaddr_in *p = (struct sockaddr_in*) sadr;
                            fprintf( stderr,
                                "   sin_addr  = sin_family: %d\n"
                                "               sin_addr:   %s\n"
                                "               sin_port:   %s\n",
                                p->sin_family,
                                hostBfr,
                                servBfr );
                            break;
                        }  /* End CASE of IPv4 address. */
                  
                        case AF_INET6:   /* IPv6 address. */
                        {
                            struct sockaddr_in6 *p;
                            p = (struct sockaddr_in6*) sadr;
                            fprintf( stderr,
                                "   sin6_addr = sin6_family:   %d\n"
                                "               sin6_addr:     %s\n"
                                "               sin6_port:     %s\n"
                                "               sin6_flowinfo: %d\n"
                                "               sin6_scope_id: %d\n",
                                p->sin6_family,
                                hostBfr,
                                servBfr,
                                p->sin6_flowinfo,
                                p->sin6_scope_id );
                            break;
                        }  /* End CASE of IPv6 address. */
                  
                        default:   /* Can never get here, but for completeness. */
                        {
                            fprintf( stderr,
                                "%s (line %d): ERROR - Unknown address "
                                "family (%d).\n",
                                pgmName,
                                __LINE__,
                                sadr->sa_family );
                            break;
                        }  /* End DEFAULT case (unknown address family). */
                    }  /* End SWITCH on address family. */
                }  /* End IF verbose mode. */

                {
                    struct llmnr_hdr_s *llmnr_hdr;
                    unsigned char response_buf[1024];
                    char *pbuf, *p_response_buf;
                    llmnr_hdr = (struct llmnr_hdr_s *) bfr;
                    int i;
                    unsigned char *p_addr;
                    unsigned char qd_name[128];  
                    unsigned long len;
                    unsigned char qd_type;
                    unsigned char qd_class;
                    int numbytes = count;
                    char v4_addr[20];
                    char v6_addr[50];
                    
                    get_local_v4addr(if_name, v4_addr);
                    inet_aton(v4_addr, &addr.sin_addr);

                    // if llmnr query source address is global, try read global first
                    v6_addr[0] = '\0';
                    if ( sadr->sa_family == AF_INET6 && ( ((struct sockaddr_in6*)sadr)->sin6_addr.s6_addr32[0] & htonl(0xE0000000)) == htonl(0x20000000))
                    {
                        get_local_v6addr(if_name, v6_addr, "global");
                    }

                    if (v6_addr[0]=='\0' && !get_local_v6addr(if_name, v6_addr, "linklocal"))
                    {
                        perror("have no available ipv6 address");
                        exit(1);
                    }
                    inet_pton(AF_INET6, v6_addr, &(addr6.sin6_addr));

                    if ( verbose ) 
                    {
                        fprintf(stderr, "llmnr id        = 0x%04x\n", ntohs(llmnr_hdr->id));
                        fprintf(stderr, "llmnr flags     = 0x%04x\n", ntohs(llmnr_hdr->flags));
                        fprintf(stderr, "llmnr qdcount   = %d\n", ntohs(llmnr_hdr->qdcount));
                        fprintf(stderr, "llmnr ancount   = %d\n", ntohs(llmnr_hdr->ancount));
                        fprintf(stderr, "llmnr nscount   = %d\n", ntohs(llmnr_hdr->nscount));
                        fprintf(stderr, "llmnr arcount   = %d\n", ntohs(llmnr_hdr->arcount));
                    }
                    
                    pbuf = (unsigned char *) &llmnr_hdr->flags;
                    *pbuf       = 0x80; /* enable response bit */
                    *(pbuf+1)   = 0;

                    memcpy(response_buf, bfr, sizeof(struct llmnr_hdr_s));
                    p_response_buf = response_buf + sizeof(struct llmnr_hdr_s);

                    pbuf = &bfr[sizeof(struct llmnr_hdr_s)];

                    llmnr_hdr = (struct llmnr_hdr_s *) response_buf;

                    /* now support qdcount == 1 only */
                    if (htons(llmnr_hdr->qdcount) > 1)
                        continue;

                    /* now, implement qdcount/ancount only */
                    for (i=0; i<htons(llmnr_hdr->qdcount); i++)
                    {
                        strncpy(qd_name, pbuf, numbytes);
                        pbuf += strlen(qd_name);
                        pbuf += 2;;
                        qd_type = *pbuf;
                        pbuf += 2;;
                        qd_class = *pbuf;
                        pbuf++;

                        if ( verbose )
                        {
                            fprintf(stderr, "qd_name  = %s\n", qd_name + 1);
                            fprintf(stderr, "qd_type  = 0x%02x\n", qd_type);
                            fprintf(stderr, "qd_class = 0x%02x\n", qd_class);
                        }
                        
                        string_tolower(qd_name+1);
                        string_tolower(host_name);
                        
                        if ( verbose )
                        {
                            fprintf(stderr, "host_name = %s\n", host_name);
                            fprintf(stderr, "host_mac_name = %s\n", host_mac_name);
                        }
                        
                        if(strcmp(host_name, qd_name + 1) == 0)
                        {                
                            llmnr_hdr->ancount++;
                            if ( verbose ) fprintf(stderr, "matched normal hostname, counter = %d\n", llmnr_hdr->ancount);
                        }
                        else if(strcmp(host_mac_name, qd_name + 1) == 0)
                        {
                        	llmnr_hdr->ancount++;
                            if ( verbose ) fprintf(stderr, "matched special hostname\n");
                        }
                        else
                        {
                            if ( verbose ) fprintf(stderr, "not a matched hostname\n");
                            continue;
                        }
        
                        /* query */
                        *p_response_buf++ = qd_name[0];
                        memcpy(p_response_buf, &qd_name[1], qd_name[0]);
    
                        p_response_buf += qd_name[0];
                        *p_response_buf++ = 0;
                        *p_response_buf++ = 0;
                        *p_response_buf++ = qd_type;
                        *p_response_buf++ = 0;
                        *p_response_buf++ = qd_class;
            
                        /* answer */
                        *p_response_buf++ = qd_name[0];
                        memcpy(p_response_buf, &qd_name[1], qd_name[0]);
                        p_response_buf += qd_name[0];

                        *p_response_buf++ = 0;
                        *p_response_buf++ = 0;
                        *p_response_buf++ = qd_type;
                        *p_response_buf++ = 0;
                        *p_response_buf++ = qd_class;
                        *(unsigned long *) p_response_buf = htonl(LLMNR_TTL);
                        p_response_buf += 4;
            
                        switch (qd_type)
                        {
                            default:
                            case 0x01:
                                if ( verbose ) fprintf(stderr, "v4 request\n");
                                *(unsigned short *) p_response_buf = htons(4);
                                p_response_buf += 2;
                                p_addr = (unsigned char *) &(addr.sin_addr.s_addr);
                                memcpy(p_response_buf, p_addr, 4);
                                p_response_buf += 4;
                                break;
                            case 0x1c:  /* ipv6 */
                                if ( verbose ) fprintf(stderr, "v6 request\n");
                                *(unsigned short *) p_response_buf = htons(16);
                                p_response_buf += 2;
                                p_addr = (unsigned char *) &(addr6.sin6_addr.s6_addr);
                                memcpy(p_response_buf, p_addr, 16);
                                p_response_buf += 16;                
                                break;
                        }
                    }
    
                    llmnr_hdr->ancount = htons(llmnr_hdr->ancount);

                    if (llmnr_hdr->ancount) {
                        len = (unsigned long)p_response_buf - (unsigned long)response_buf;
                        if (verbose) fprintf(stderr, "send response, len = %d\n", len);

                        /*
                         * Send the llmnr to the client.
                         */
                        wBytes = len;
                        while ( wBytes > 0 )
                        {
                            do
                            {
                                count = sendto( desc[ idx ].fd,
                                    response_buf,
                                    wBytes,
                                    0,
                                    sadr,        /* Address & address length   */
                                    sadrLen );   /*    received in recvfrom(). */
                                } while ( ( count < 0 ) && ( errno == EINTR ) );
                        
                                //CHK( count );   /* Check for a bona fide error. */
                                
                                /*NickChou add 
                                When count < 0, but errno != EINTR
                                wBytes is always > 0 => infinity loop
                                */
                                if (count<0) 
                                	break;
                                
                                wBytes -= count;
                        }  /* End WHILE there is data to send. */
                    }
                }

            }  /* End ELSE a UDP datagram is available. */
            
            desc[ idx ].revents = 0;   /* Clear the returned poll events. */
            
        }  /* End FOR each socket descriptor. */
    }  /* End WHILE forever. */
}  /* End process_llmnr() */
