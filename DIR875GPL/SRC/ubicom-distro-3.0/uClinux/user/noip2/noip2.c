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
#include <syslog.h>

#include <sutil.h>
#include <shvar.h>

/*----------------------------------------------------------------------------------------*/
#define DYNDNS_TYPE			0			
#define EASYDNS_TYPE			1
#define NOIP_TYPE			2
#define CHANGEIP_TYPE                   3
#define EURODYNDNS_TYPE                 4
#define ODS_TYPE                        5
#define OVH_TYPE                        6
#define REGFISH_TYPE                    7
#define TZO_TYPE                        8
#define ZONEEDIT_TYPE                   9
#define DLINK_TYPE                      10
#define CYBER_GATE_TYPE			11
/*  Date: 2010-01-25
*   Name: Ken Chiang
*   Reason: modify for the DDNS can support other like DDNS server 
*   Notice :
*/	
#define OTHER_DDNS_TYPE			12

#define CLIENT_IP_PORT                  80
#define IPLEN					20
#define LINELEN					256
#define BIGBUFLEN				1024
#define NOIP_NAME				"dynupdate.no-ip.com"
#define USER_AGENT				"User-Agent: Linux DUC "
#define OPTCHARS				"CYU:Fc:hSMi:K:I:u:p:R:T:wk:a:n:t:"
#define ARGC						1
#define ARGF						2
#define ARGY						4
#define ARGU						8
#define ARGI						16
#define ARGD						32
#define ARGS						64
#define ARGM					128
#define ARGK						256
#define ARGi						512
#define HOST						1
#define GROUP					2
#define MAX_DEVLEN				16
#define SUCCESS					1
#define CYBERGATEDDNS				0
#define FATALERR				-1
#define BADAUTH				                 	-2
#define FAILURE					-1
#define NOHOST				                 	-3
#define SOCKETFAIL				-3
#define CONNTIMEOUT			-4
#define CONNFAIL				-5
#define READFAIL				-7
#define DDNS_TIMEOUT_SECOND	5		
#define DDNS_STATUS_FILE	              "/var/tmp/ddns_status"
//#define DDNS_DEBUG
#ifdef DDNS_DEBUG
#define DEBUG_MSG(fmt, arg...)       printf("%s [%d]: "fmt, __FILE__, __LINE__, ##arg)
#else
#define DEBUG_MSG(fmt, arg...) 
#endif
/*----------------------------------------------------------------------------------------*/
int	timed_out			=	0;
int	background			=	1;	
int	port_to_use			=	CLIENT_IP_PORT;
int	socket_fd			=	-1;
int	config_fd			=	-1;
int	nat					=	0;
int	log2syslog			=	0;
int 	offset				=	0;
int	needs_conf			=	0;
int	firewallbox			=	0;
int	forceyes			=	0;
int	update_cycle		=	0;
int 	ddns_type			=	0;  
int	show_config			=	0;
int	multiple_instances	=	0;
int	debug_toggle		=	0;
int	kill_proc			=	0;
int	reqnum				=	0;
int	count =	0;
char	*program;
char	*ourname;
char	tmp_filename[LINELEN]	=	"/tmp/NO-IPXXXXXX";
char	*config_filename;
char	*request;
char	*pid_path;
char	*execpath;
char	config_username[32] = {0};
char	config_password[32] = {0};
char	config_Rhostname[32] = {0};  
char	*ddns_user_agent=NULL;
char    wildcard[4] = "OFF";
char wan_IP[IPLEN]			=	{0};
static int tables_initialised		=	0;
char	IPaddress[IPLEN];
//char	*iface;
static char base64_to_char [64];
char	saved_args[LINELEN];
char	cyber_gate_salt[32] = {};
char    cyber_gate_time[32] = {};
char    cyber_gate_sign[64] = {};
char	cyber_gate_result[4] = {};
int	cyber_gate_ddns_flag = 0;

extern const char VER_FIRMWARE[];
static char wan_eth[10]={0};
static char ddns_dyndns_kinds[32]={0};
static char model_name[32]={0};
static char model_number[32]={0};
/*  Date: 2010-01-25
*   Name: Ken Chiang
*   Reason: modify for the DDNS can support other like DDNS server 
*   Notice :
*/	
char ddns_server_other[LINELEN]={0};

void save_to_file(void);

/* ddns_server[10](dlinkddns) redirect to ddns_server[0](dyndns) */
char *ddns_server[12] = {  
	"members.dyndns.com",
	"members.easydns.com",
	"dynupdate.no-ip.com",
    "www.changeip.com",
	"eurodyndns.org",
	"ods.org",
	"ovh.com",
	"www.regfish.com",
	"clusterlookup.tzo.com",
	"dynamic.zoneedit.com",
	"members.dyndns.com",
	"ddns.cybergate.planex.co.jp",  
};

/*----------------------------------------------------------------------------------------*/
struct	termios argin, argout;
struct	sigaction sig;
/*----------------------------------------------------------------------------------------*/
void	process_options(int argc, char *argv[]);
void	alarm_handler(int signum);
int	get_our_visible_IPaddr(char *dest, char *ret_ip);
int	Read(int sock, char *buf, size_t count_t);
int	Write(int fd, char *buf, size_t count_t);
int	Connect(int port, char *dest, int type);
int	converse_with_web_server(int fd, int d_type, char *r_host, char *send_buf);
int	converse_with_web_server_2(char *ret_ip, int _socket_fd, char *send_buf);  
void	dump_buffer(int x);
int	dynamic_update(int port, int d_type, unsigned long my_ip, char *buf);
static void init_conversion_tables (void);
int	encode_base64(const unsigned char *source, unsigned char *target, size_t source_size);
void 	save_to_file(void);
/*----------------------------------------------------------------------------------------*/
char *get_ipaddr(char *if_name)
{
	int sockfd;
	struct ifreq ifr;
	struct in_addr temp_ip;

	if( strlen(if_name) < 1 )
		return 0;
		
	if ((sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_RAW)) < 0)
		return 0;

	strcpy(ifr.ifr_name, if_name);
	if( ioctl(sockfd, SIOCGIFADDR, &ifr) < 0) {
		close(sockfd);
		return 0;
	}

	close(sockfd);

	temp_ip.s_addr = sin_addr(&ifr.ifr_addr).s_addr;
	return inet_ntoa(temp_ip);
}


int main(int argc, char *argv[]){
	char *p_instruction;
	char  send_buf[1024];  
	unsigned  curr_wan_ip=0;
	int result,retry_count = 0;
	time_t	first_time;
	char buf[256]={};
	time_t clock_t;
	struct tm *tm;
	timed_out = 0;
	sig.sa_flags = 0; 
	sigemptyset(&sig.sa_mask);
	sig.sa_handler = SIG_IGN;
	sigaction(SIGHUP,&sig,NULL);
	sigaction(SIGPIPE,&sig,NULL);
	sigaction(SIGUSR1,&sig,NULL);
	sigaction(SIGUSR2,&sig,NULL);
	sig.sa_handler = alarm_handler;
	sigaction(SIGALRM,&sig,NULL);
	DEBUG_MSG("start\n");
	p_instruction = strrchr(argv[0], '/');

	if (p_instruction)
		program = ++p_instruction;
	else
		program = argv[0];    
	DEBUG_MSG("instruction is %s\n", program);

	sprintf(saved_args, "%s", program);
	
	//*iface = 0;
	DEBUG_MSG("before process_option\n");		
	process_options(argc, argv);
	DEBUG_MSG("finish process_option\n");

	nat = 1;
	background = 0;
	DEBUG_MSG("ddns_type = %d\n",ddns_type);
	DEBUG_MSG("Rhostname = %s\n",config_Rhostname);
	DEBUG_MSG("username = %s\n",config_username);
	DEBUG_MSG("password = %s\n",config_password);


	if (ddns_type != NOIP_TYPE)
		port_to_use = 80;	
	if (ddns_type != ODS_TYPE)
		port_to_use = 7070;
	if (ddns_type == CYBER_GATE_TYPE)
		port_to_use = 80;

	if (ddns_type == NOIP_TYPE)
		port_to_use = 80;

	DEBUG_MSG("In noip2, nat=%u, bk=%u, wan_IP=%s port=%d\n", nat, background,wan_IP,port_to_use);

	first_time = time((time_t *)NULL);

	if (first_time == -1) {
		DEBUG_MSG("time() fail \n");
		exit (1);
	}

	DEBUG_MSG("First_Time=%d \n",first_time);

	curr_wan_ip = inet_addr(wan_IP);
	if (curr_wan_ip == INADDR_NONE) {
		printf("noip2: Invalid IP address = %s\n", wan_IP);
		exit (1);
	}
	DEBUG_MSG("curr_wan_ip=%lx \n",curr_wan_ip);

	init_file(DDNS_STATUS_FILE);
	save2file(DDNS_STATUS_FILE,"NOIP2 Updating ...");
	
	for( ; ; )
	{
retry:
		result = dynamic_update(port_to_use, ddns_type, curr_wan_ip, send_buf);
		time(&clock_t);
		tm = localtime(&clock_t) ;
		strftime(buf,sizeof(buf),"%A, %B %d, %Y %X",tm);

		if ((result != SUCCESS) && (result != CYBERGATEDDNS)) 
		{
			if(result == BADAUTH)
			{
				init_file(DDNS_STATUS_FILE);
				save2file(DDNS_STATUS_FILE,"Authentication failed. User Name/Password is not correct");
			}
			else if(result == NOHOST)
			{       
				init_file(DDNS_STATUS_FILE);
				save2file(DDNS_STATUS_FILE,"Update failed. Host Name is not correct.");
			}

			count++;
			DEBUG_MSG("NOIP2: dynamic_update: try %d times for 5 sec\n", count);	
			if(count > 4)
			{
/* Chun modify:
 * Since there is schedule feature in UI, we don't need to retry forever.
 * we only allow noip to retry 5 times. If it still fails, noip will be shutdown.  
 */				
				printf("NOIP2, dynamic_update() fail !!!\n");
					break;
			}	

		}
		else if(result == CYBERGATEDDNS)
		{	//send secondary query
			result = dynamic_update(port_to_use, ddns_type, curr_wan_ip, send_buf);
			if(result == SUCCESS)
			{
				retry_count = 0;
				break;
			}
			else
			{
				if(retry_count == 2)
				{
					retry_count = 0;
					break;
				}
				else
				{
					retry_count++;
					sleep(5);
					goto retry;
				}
			}
		}
		else
		{
			syslog(LOG_INFO, "NOIP2: Update DDNS Success\n");				
			save_to_file();
			count = 0;
			break;
		}	
	}

/* jimmy added , used only for DIR-730 , make noip2 as daemon */
#ifdef sl3516
	if(update_cycle > 0){
		DEBUG_MSG("%s, update_cycle (-U) has been specified to %d, loop again\n",__FUNCTION__,update_cycle);
		retry_count = 0;
		sleep(update_cycle);
		goto retry;
	}
#endif
/* --------------------------------------------------------- */
return 0;
}

/************************************************************************
 *Function name:process_options(...)											*
 *Description:Thie function get input instruction from user. This function can work by	* 
 *		     inputing -T -R -i -p -u only.										*
 *Input: int argc 															*
 *	    char *argv															*
 *Return:none																*
 *************************************************************************/ 
void process_options(int argc, char *argv[]){
	//char    *optarg;
	int     returnopt, have_args = 0;
	int tmp;

	DEBUG_MSG("=============Entry process_options() !!=============\n");
	while ((returnopt = getopt(argc, argv, OPTCHARS)) != EOF)	{
		DEBUG_MSG("in while loop\n");
		switch (returnopt) {
			case 'C':
				needs_conf++;
				log2syslog = -1;
				have_args |= ARGC;
				break;
			case 'F':
				firewallbox++;
				have_args |= ARGF;
				break;
			case 'Y':
				forceyes++;
				have_args |= ARGY;
				break;
			case 'U':
				update_cycle = atoi(optarg);
				have_args |= ARGU;
				break;
			case 'c':
				config_filename = optarg;
				strcat(saved_args, " -c ");
				strcat(saved_args, optarg);
				break;		
			case 'S':
				show_config++;
				log2syslog = -1;
				have_args |= ARGS;
				break;
			case 'M':
				multiple_instances++;
				have_args |= ARGM;
				break;
			case 'K':
				kill_proc = atoi(optarg);
				have_args |= ARGK;
				break;
			case 'i':/*Wan_IP*/
				strcpy(wan_IP, optarg);
				have_args |= ARGi;
				break;
			case 'I':
				strcpy( wan_eth, optarg);
				//strcpy(iface, optarg);
				strcat(saved_args, " -I ");
				strcat(saved_args, optarg);
				have_args |= ARGI;
				strcpy( wan_IP, get_ipaddr(optarg));
				have_args |= ARGi;
				break;
			case 'u': /* user name */			
				strcpy(config_username , optarg);
				break;
			case 'p': /* password name */
				strcpy(config_password , optarg);
				break;			
			case 'R': /* remote host name */
				strcpy(config_Rhostname , optarg);
				break;
			case 'T': /*DDNS Type*/				
				ddns_type = atoi(optarg);
				have_args |= ARGU;
				break;
			case 'w': /* wildcard enable */
				strcpy(wildcard , "ON");
				break;	
			case 'k': /* ddns_dyndns_kinds */
				strcpy( ddns_dyndns_kinds, optarg);
				break;
			case 'a': /* model_name */
				strcpy( model_name, optarg);
				break;
			case 'n': /* model_number */
				strcpy( model_number, optarg);
				break;	
/*  Date: 2010-01-25
*   Name: Ken Chiang
*   Reason: modify for the DDNS can support other like DDNS server 
*   Notice :
*/	
			case 't': /* other ddns_server name */
				strcpy(ddns_server_other , optarg);
				DEBUG_MSG("ddns_server_other=%s\n",ddns_server_other);
				break;	
			default:
				exit(0);
		}
	}
}
/************************************************************************
 *Function name:alarm_handler(...)											*
 *Description:This function enable time_out (true)								* 
 *Input: int -> signal number													*
 *Return:none																*
 *************************************************************************/ 
void alarm_handler(int signum)	
{
	timed_out = 1;
}

/************************************************************************
 *Function name:Read(...)													*
 *Description:This function read the rec_buf from remote DDNS server				* 
 *Input: int -> socket ID														*
 *	  : char * -> point to the rec_buf											*
 *	  : size_t -> the size of rec_buf 											*
 *Return: success -> size_t (bytes)											*
 *	    : failure -> READFAIL (-7)											*
 *************************************************************************/ 
int Read(int sock, char *buf, size_t count_t){
	size_t bytes_read = 0;
	bytes_read = read(sock, buf, count_t);
	DEBUG_MSG("rec_buf = %s\n",buf);		
	if (bytes_read <= 0)  
		return READFAIL;
	else 
		return bytes_read;
}

/************************************************************************
 *Function name:Write(...)													*
 *Description:This function write the send_buf to remote DDNS server				* 
 *Input: int -> file ID														*
 *	  : char * -> point to the send_buf										*
 *	  : size_t -> the size of send_buf 											*
 *Return: success -> SUCCESS (1)											*
 *	    : failure -> FAILURE (-1)												*
 *************************************************************************/ 
int Write(int fd, char *buf, size_t count_t){
	if (write(fd, buf, count_t)==-1) {
		DEBUG_MSG("Write Failure !!!!\n");
		return FAILURE;
	}
	return SUCCESS;
}

/************************************************************************
 *Function name:Connect(...)													*
 *Description:This function will go to get remote DDNS server IP address.				*
 *		     But the subrountine:gethostbyname() can not work by this complier		* 
 *		     version.														*
 *		     So i first write the static IP for different DDNS server, this bug will be		*
 *		     solved after changing another complier version.						* 
 *Input: int -> port number													*
 *	  : char * -> point to the DDNS server host name							*
 *	  : int -> the type of DDNS server										*
 *Return: success -> socket ID												*
 *	    : failure -> an interget smaller than 0									*
 *************************************************************************/ 
int Connect(int port, char *dest, int type){
	int     sid, connectret;
	struct  in_addr saddr;
	struct  sockaddr_in addr;
	struct  hostent *host;
	host = (struct hostent *) malloc (128);

	DEBUG_MSG("before gethostbyname() function\n");
	DEBUG_MSG("DDNS server host name = %s\n",dest);
	host = gethostbyname(dest);
	DEBUG_MSG("finished gethostbyname() function\n");

	if (!host || host == NULL)
	{
		DEBUG_MSG("In Connect(), gethostbyname() fail !!!!\n");
		return -2;
	}
	memcpy(&saddr.s_addr, host->h_addr_list[0], 4);
	memset((char *) &addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;

	if (ddns_type == NOIP_TYPE)
	{
		DEBUG_MSG("----port %d\n",port);
		addr.sin_port = htons(80);
	}
	else
	{
		port = 80;
		DEBUG_MSG("----port %d\n",port);
		addr.sin_port = htons(port);
	}

	DEBUG_MSG("port %d\n",addr.sin_port);
	addr.sin_addr.s_addr = saddr.s_addr;
	DEBUG_MSG("remote DDNS server IP = %s\n",inet_ntoa(addr.sin_addr));
	sid = socket(AF_INET, SOCK_STREAM, 0);

	if (sid < 0) {
		DEBUG_MSG("Socket : connect, socket() fail !!!\n");
		return SOCKETFAIL;
	}

	connectret = connect(sid, (struct sockaddr *)&addr, sizeof(addr));

	if (connectret < 0)  {
		DEBUG_MSG("Connect : In Connect(), connect() fail !!!\n");

		if (timed_out) 
			connectret = CONNTIMEOUT;			
		else
			connectret = CONNFAIL;

		close(sid);		
		return CONNTIMEOUT;
	}
	DEBUG_MSG("finish connect();\n");
	return sid;

}

void save_to_file(void)
{
	char buf[64]={};
	time_t clock_t;
	//char temp[10] = {};
	struct tm *tm;

	//strcpy(temp,wan_eth);
	time(&clock_t);
	tm = localtime(&clock_t);
	strftime(buf,sizeof(buf),"%I:%M %p|%m/%d/%Y",tm);
	
	init_file(DDNS_STATUS_FILE);
	save2file(DDNS_STATUS_FILE, "SUCCESS|%s|%s|%s|%s",
		config_Rhostname, wan_IP, ddns_type==DYNDNS_TYPE? "DynDns.org" : "dlinkddns.com", buf);
}


/************************************************************************
 *Function name:converse_with_web_server(...)									*
 *Description:This function will send buffer to DDNS server and receive response buffer.	*
 *Input: int -> socket ID														*
 *	  : int -> DDNS server type												*
 *	  : char * -> remote host name (this host name need to be registed )			*
 *	  : char * -> point to the send_buf										*
 *Return: success -> SUCCESS (1)											*
 *	    : failure -> an interget smaller than 0									*
 *************************************************************************/ 
int converse_with_web_server(int fd, int d_type, char *r_host, char *send_buf){	
	int  rwret, ret;  
	char *cptr;
	char recv_buf[1024];
	char *start,*end;

	DEBUG_MSG("Enter converse_with_web_server() \n"); 
	DEBUG_MSG("buffer=%s\n",send_buf);

	if ((rwret = Write(fd, send_buf, strlen(send_buf))) != SUCCESS) {
		perror("noip2: converse_with_web_server: Write()");
		close(fd);
		return FAILURE;
	}

	memset(recv_buf,0,sizeof(recv_buf));

	if ((rwret = Read(fd, recv_buf, sizeof(recv_buf) - 2)) <= 0) { //Read return bytes to read
		perror("noip2: converse_with_web_server: Read()");
		close(fd);
		return FAILURE;
	}

	DEBUG_MSG("In converse_with_web_server(), receive bytes=%d \n",rwret);
	DEBUG_MSG("NOIP2: converse_with_web_server : recv_buf = \n%s\n", recv_buf);	
		
	if ((d_type != NOIP_TYPE) && (d_type != CYBER_GATE_TYPE)) 
        {// not noip & cyber gate
		cptr = strstr(recv_buf, "good");
		if (cptr == NULL) 
		{
			if ( strstr(recv_buf, "nochg")== NULL) 
			{
				if( strstr(recv_buf, "badauth") != NULL)
					return BADAUTH;	
				else 
				{
					if( strstr(recv_buf, "nohost")!= NULL)
						return NOHOST;
					else
				return FATALERR;
			}
			}
			else
			{
				DEBUG_MSG("NOIP2: converse_with_web_server 1: SUCCESS\n");			
				return SUCCESS;		
		}
		}
		else
		{	
			DEBUG_MSG("NOIP2: converse_with_web_server 2: SUCCESS\n");	
			return SUCCESS;
	}
	}
	else if(d_type == CYBER_GATE_TYPE)
	{//cyber gate ddns
		if(cyber_gate_ddns_flag == 0)//first response form server
		{
			start = strstr(recv_buf,"salt");
			start = start + 15;
			end = strstr(start,"\"");
			strncpy(cyber_gate_salt,start,end - start);
			start = strstr(recv_buf,"time");
			start = start + 15;
			end = strstr(start,"\"");
			strncpy(cyber_gate_time,start,end - start);
			start = strstr(recv_buf,"sign");
                	start = start + 15;
                	end = strstr(start,"\"");
                	strncpy(cyber_gate_sign,start,end - start);
			cyber_gate_ddns_flag = 1;
			DEBUG_MSG("SALT = %s\n",cyber_gate_salt);
			DEBUG_MSG("TIME = %s\n",cyber_gate_time);
			DEBUG_MSG("SIGN = %s\n",cyber_gate_sign);
			return CYBERGATEDDNS;
		}
		
		if(cyber_gate_ddns_flag == 1)//first response form server
                {
			cyber_gate_ddns_flag = 0;
			start = strstr(recv_buf,"retc");
			start = start + 15;
                        end = strstr(start,"\"");
			strncpy(cyber_gate_result,start,end - start);
			if(atoi(cyber_gate_result) == 0)
			{
				DEBUG_MSG("YES UPDATE SUCCESS\n");
				return SUCCESS;
			}
			else
				return FAILURE;
		}
	}
	else
	{ //noip ddns
		DEBUG_MSG("rec_buf = %s",recv_buf);
		cptr = strstr(recv_buf, r_host);

		if (cptr==NULL) {
			DEBUG_MSG("Cannot find %s in recv_buf \n", r_host);
			return FATALERR;
		}

		DEBUG_MSG("cptr=%s \n",cptr);
		cptr = cptr+strlen(r_host);

		if (*cptr !=':')
			return FAILURE;

		cptr++;

		ret = atoi(cptr);
		switch (ret) {
			case 0:
				DEBUG_MSG("DNS is Current, no update done \n");
				return SUCCESS;
				break;
			case 1:
				DEBUG_MSG("DNS update successful \n");
				return SUCCESS;
				break;
			default:
				DEBUG_MSG("DDNS update Failure, ret=%d !!!\n",ret);
				return FAILURE;
				break;
		}		
	}
	
	return SUCCESS;
}

/************************************************************************
 *Function name:converse_with_web_server2(...)								*
 *Description:This function will send buffer to DDNS server and receive response buffer.	*
 *Input: char *-> point to the wan ip address									*
 *	  : int -> socket ID														*
 *	  : char * -> point to the send_buf										*
 *Return: success -> SUCCESS (1)											*
 *	    : failure -> FATALERR (-1)											*
 *************************************************************************/ 
int converse_with_web_server_2(char *ret_ip, int fd, char *send_buf){
	int	retrw;
	char *p;
	char rcv_buf[1024];
	DEBUG_MSG("Entry converse_with_web_server_2() \n");
	DEBUG_MSG("send_buf=%s\n", send_buf);

	if ((retrw = Write(fd, send_buf, strlen(send_buf))) != SUCCESS) {
		DEBUG_MSG("In converse_with_web_server_2(), write fail !!\n");
		return retrw;
	}

	if ((retrw = Read(fd, rcv_buf, sizeof(rcv_buf))) < 0) {
		DEBUG_MSG("In converse_with_web_server_2(), read fail !!\n");
		return retrw;
	}

	rcv_buf[retrw++] = 0;		

	if ((p = strrchr(rcv_buf,'\n')))
		strcpy(ret_ip, ++p);
	else 
		return FATALERR;
	return SUCCESS;	

}

/************************************************************************
 *Function name:get_our_visible_IPaddr(...)										*
 *Description:This function will send buffer to DDNS server and receive response buffer.	*
 *Input: char *-> point to the remote registed host name							*
 *	  : char * -> point to the wan ip address									*
 *Return: success -> SUCCESS (1)											*
 *	    : failure -> FAILURE (-1)												*
 *************************************************************************/ 
int get_our_visible_IPaddr(char *dest, char *ret_ip){
	int	web_converse2_ret , fd;
	char send_buf[512];

	if ((fd = Connect(CLIENT_IP_PORT, ddns_server[ddns_type],ddns_type)) < 0)  {
		DEBUG_MSG("Connect(%s) fail \n",ddns_server[ddns_type]);
		return FAILURE;
	} else {
		sprintf(send_buf, 
				"GET http://%s/ip.php HTTP/1.1\n%s\n\n", NOIP_NAME,USER_AGENT);

		if ((web_converse2_ret = converse_with_web_server_2(ret_ip, fd, send_buf)) != SUCCESS) {  
			DEBUG_MSG("converse_with_web_server_2() fail, x=%x \n",web_converse2_ret);
			close(socket_fd);
			return FAILURE;
		} else {
			DEBUG_MSG("converse_with_web_server_2() successful, ret_ip=%s \n",ret_ip);
			close(socket_fd);
			return SUCCESS;				
		} 
	}	
}

/************************************************************************
 *Function name:dynamic_update(...)											*
 *Description:this function send wan IP address to the remote DDNS server to			*
 *		     change the IP address.											*
 *Input: int -> port number													*
 *	  : int -> DDNS server type												*
 *	  : unsigned long -> current device wan IP address							*
 *	  : char * -> point to the send_buf										*
 *Return: success -> SUCCESS (1)											*
 *	    : failure -> an interget smaller than 0									*
 *************************************************************************/ 
int dynamic_update(int port, int d_type, unsigned long my_ip, char *send_buf){
	int	fd, web_converse_ret;

	struct in_addr inetaddr;
	char tmp_buf[101], tmp_buf1[101];
	DEBUG_MSG("Entry dynamic_update() \n");  
	inetaddr.s_addr = my_ip;
	
/*  Date: 2010-01-25
*   Name: Ken Chiang
*   Reason: modify for the DDNS can support other like DDNS server 
*   Notice :
*/	
	if(ddns_type==OTHER_DDNS_TYPE){
		DEBUG_MSG("%s: ddns_type==OTHER_DDNS_TYPE\n",__func__);
		if ((fd = Connect(port, ddns_server_other,ddns_type)) <0) {
			DEBUG_MSG("Connect(%s) fail, fd=[%u] \n",ddns_server_other,fd);
			return FAILURE;
		}		
	}
	else{	
		if ((fd = Connect(port, ddns_server[d_type],ddns_type)) <0) {
			DEBUG_MSG("Connect(%s) fail, fd=[%u] \n",ddns_server[d_type],fd);
			return FAILURE;
		}		
	}

	switch (d_type) {
		/* dlinkddns is same as dyndns */
		case DYNDNS_TYPE: 
		case DLINK_TYPE:						
/*  Date: 2010-01-25
*   Name: Ken Chiang
*   Reason: modify for the DDNS can support other like DDNS server 
*   Notice :
*/	
		case OTHER_DDNS_TYPE:							
			if(strcmp(ddns_dyndns_kinds,"Static") == 0)
				sprintf(send_buf, "GET /nic/update?system=statdns&hostname=%s",config_Rhostname);
			else if(strcmp(ddns_dyndns_kinds,"Custom") == 0)
				sprintf(send_buf, "GET /nic/update?system=custom&hostname=%s",config_Rhostname);
			else
			sprintf(send_buf, "GET /nic/update?system=dyndns&hostname=%s",config_Rhostname);

			sprintf(tmp_buf, "&myip=%s", inet_ntoa(inetaddr));
			strcat(send_buf, tmp_buf);
			sprintf(tmp_buf, "&wildcard=%s",wildcard);
			strcat(send_buf, tmp_buf);
			sprintf(tmp_buf, "&mx=");
			strcat(send_buf, tmp_buf);
			sprintf(tmp_buf, "&backmx=NO");
			strcat(send_buf, tmp_buf);
			sprintf(tmp_buf, "&offline=NO ");
			strcat(send_buf, tmp_buf);
			strcpy(tmp_buf, "HTTP/1.1\r\n");
			strcat(send_buf, tmp_buf);      
/*  Date: 2010-01-25
*   Name: Ken Chiang
*   Reason: modify for the DDNS can support other like DDNS server 
*   Notice :
*/	
			if(ddns_type==OTHER_DDNS_TYPE){
				sprintf(tmp_buf, "Host: %s\r\n", ddns_server_other);
			}
			else{	
				sprintf(tmp_buf, "Host: %s\r\n", ddns_server[d_type]);
			}
			strcat(send_buf, tmp_buf);
			strcpy(tmp_buf, "Authorization: Basic ");
			strcat(send_buf, tmp_buf);
			sprintf(tmp_buf1,"%s:%s",config_username, config_password);
			encode_base64(tmp_buf1, tmp_buf, strlen(tmp_buf1));
			strcat(send_buf, tmp_buf);
			strcpy(tmp_buf, "\r\n");
			strcat(send_buf, tmp_buf);
			sprintf(tmp_buf, "User-Agent: %s %s %s\r\n\r\n", model_name, model_number, VER_FIRMWARE);
			strcat(send_buf, tmp_buf);
			DEBUG_MSG("In dynamic_update(), DYNDNS TYPE combine http finish \n");
			break;
		case EASYDNS_TYPE:    
			sprintf(send_buf, "GET /dyn/dyndns.php?&hostname=%s", config_Rhostname);
			sprintf(tmp_buf, "&myip=%s",inet_ntoa(inetaddr));
			strcat(send_buf, tmp_buf);
			sprintf(tmp_buf, "&wildcard=OFF");
			strcat(send_buf, tmp_buf);
			sprintf(tmp_buf, "&mx=");
			strcat(send_buf, tmp_buf);
			sprintf(tmp_buf, "&backmx=NO ");
			strcat(send_buf, tmp_buf);
			strcpy(tmp_buf, "HTTP/1.1\r\n");
			strcat(send_buf, tmp_buf); 
			sprintf(tmp_buf, "User-Agent: %s %s %s\r\n\r\n", model_name, model_number, VER_FIRMWARE);     
			strcat(send_buf, tmp_buf);      
			sprintf(tmp_buf, "Host: %s\r\nConnection: close\r\n", ddns_server[d_type]);
			strcat(send_buf, tmp_buf);
			strcpy(tmp_buf, "Authorization: Basic ");
			strcat(send_buf, tmp_buf);
			sprintf(tmp_buf1,"%s:%s",config_username, config_password);
			encode_base64(tmp_buf1, tmp_buf, strlen(tmp_buf1));
			strcat(send_buf, tmp_buf);
			strcpy(tmp_buf, "\r\n\r\n");
			strcat(send_buf, tmp_buf); 
			break;
		case NOIP_TYPE:
			printf("NOIP_TYPE\n");
			strcpy(send_buf, "GET /ducupdate.php?");
			sprintf(tmp_buf, "username=%s", config_username);
			strcat(send_buf, tmp_buf);
			sprintf(tmp_buf, "&pass=%s", config_password); 
			strcat(send_buf, tmp_buf);

			sprintf(tmp_buf, "&h[]=%s", config_Rhostname); 
			strcat(send_buf, tmp_buf);
			sprintf(tmp_buf, "&ip=%s", inet_ntoa(inetaddr));
                        strcat(send_buf, tmp_buf);
			strcpy(tmp_buf, " HTTP/1.0\r\n");
			strcat(send_buf, tmp_buf);
			sprintf(tmp_buf, "Host: %s\r\n", ddns_server[d_type]);
			strcat(send_buf, tmp_buf);    
/*			sprintf(tmp_buf, "Authorization: Basic %s:%s\r\n", config_username, config_password);
			strcat(send_buf, tmp_buf);  */
			sprintf(tmp_buf, "User-Agent: %s %s %s\r\n\r\n", model_name, model_number, VER_FIRMWARE);
			strcat(send_buf, tmp_buf);
			break;
		case CHANGEIP_TYPE:
			sprintf(send_buf, "GET /nic/update?hostname=%s",config_Rhostname);	
			sprintf(tmp_buf, "&myip=%s", inet_ntoa(inetaddr));
			strcat(send_buf, tmp_buf);
			sprintf(tmp_buf, "&wildcard=%s",wildcard);
			strcat(send_buf, tmp_buf);
			sprintf(tmp_buf, "&mx=");
			strcat(send_buf, tmp_buf);
			sprintf(tmp_buf, "&backmx=NO");
			strcat(send_buf, tmp_buf);
			sprintf(tmp_buf, "&offline=NO ");
			strcat(send_buf, tmp_buf);
			strcpy(tmp_buf, "HTTP/1.1\r\n");
			strcat(send_buf, tmp_buf);
			sprintf(tmp_buf, "User-Agent: %s %s %s\r\n\r\n", model_name, model_number, VER_FIRMWARE);     
			strcat(send_buf, tmp_buf);
			sprintf(tmp_buf, "Host: %s\r\nConnection: close\r\n", ddns_server[d_type]);
			strcat(send_buf, tmp_buf);      
			strcpy(tmp_buf, "Authorization: Basic ");
			strcat(send_buf, tmp_buf);
			sprintf(tmp_buf1,"%s:%s",config_username, config_password);
			encode_base64(tmp_buf1, tmp_buf, strlen(tmp_buf1));
			strcat(send_buf, tmp_buf);
			strcpy(tmp_buf, "\r\n\r\n");
			strcat(send_buf, tmp_buf);
			DEBUG_MSG("In dynamic_update(), DYNDNS TYPE combine http finish \n");
			break;
		case EURODYNDNS_TYPE:
			sprintf(send_buf, "GET /nic/update?hostname=%s",config_Rhostname);
			sprintf(tmp_buf, "&myip=%s", inet_ntoa(inetaddr));
			strcat(send_buf, tmp_buf);
			sprintf(tmp_buf, "&wildcard=%s",wildcard);
			strcat(send_buf, tmp_buf);
			sprintf(tmp_buf, "&mx=");
			strcat(send_buf, tmp_buf);
			sprintf(tmp_buf, "&backmx=NO");
			strcat(send_buf, tmp_buf);
			sprintf(tmp_buf, "&offline=NO ");
			strcat(send_buf, tmp_buf);
			strcpy(tmp_buf, "HTTP/1.1\r\n");
			strcat(send_buf, tmp_buf);
			sprintf(tmp_buf, "User-Agent: %s %s %s\r\n\r\n", model_name, model_number, VER_FIRMWARE);     
			strcat(send_buf, tmp_buf); 
			sprintf(tmp_buf, "Host: %s\r\nConnection: close\r\n", ddns_server[d_type]);
			strcat(send_buf, tmp_buf);
			strcpy(tmp_buf, "Authorization: Basic ");
			strcat(send_buf, tmp_buf);
			sprintf(tmp_buf1,"%s:%s",config_username, config_password);
			encode_base64(tmp_buf1, tmp_buf, strlen(tmp_buf1));
			strcat(send_buf, tmp_buf);
			strcpy(tmp_buf, "\r\n\r\n");
			strcat(send_buf, tmp_buf);
			DEBUG_MSG("In dynamic_update(), DYNDNS TYPE combine http finish \n");
			break;
		case ODS_TYPE: case OVH_TYPE: case TZO_TYPE:
			sprintf(send_buf, "GET /nic/update?hostname=%s",config_Rhostname);
			sprintf(tmp_buf, "&myip=%s", inet_ntoa(inetaddr));
			strcat(send_buf, tmp_buf);
			sprintf(tmp_buf, "&wildcard=%s",wildcard);
			strcat(send_buf, tmp_buf);
			sprintf(tmp_buf, "&mx=");
			strcat(send_buf, tmp_buf);
			sprintf(tmp_buf, "&backmx=NO");
			strcat(send_buf, tmp_buf);
			sprintf(tmp_buf, "&offline=NO ");
			strcat(send_buf, tmp_buf);
			strcpy(tmp_buf, "HTTP/1.1\r\n");
			strcat(send_buf, tmp_buf);
			sprintf(tmp_buf, "User-Agent: %s %s %s\r\n\r\n", model_name, model_number, VER_FIRMWARE);     
			strcat(send_buf, tmp_buf); 
			sprintf(tmp_buf, "Host: %s\r\nConnection: close\r\n", ddns_server[d_type]);
			strcat(send_buf, tmp_buf);
			strcpy(tmp_buf, "Authorization: Basic ");
			strcat(send_buf, tmp_buf);
			sprintf(tmp_buf1,"%s:%s",config_username, config_password);
			encode_base64(tmp_buf1, tmp_buf, strlen(tmp_buf1));
			strcat(send_buf, tmp_buf);
			strcpy(tmp_buf, "\r\n\r\n");
			strcat(send_buf, tmp_buf);
			DEBUG_MSG("In dynamic_update(), DYNDNS TYPE combine http finish \n");
			break;
		case REGFISH_TYPE:
			sprintf(send_buf, "GET /dyndns/2/?ipv4=%s",inet_ntoa(inetaddr));
			sprintf(tmp_buf, "&fqdn=%s&authtype=secure",config_Rhostname);
			strcat(send_buf, tmp_buf);
			sprintf(tmp_buf, "&token=%s&forcehost=0 ",config_username);
			strcat(send_buf, tmp_buf);
			strcpy(tmp_buf, "HTTP/1.1\r\n");
			strcat(send_buf, tmp_buf);
			sprintf(tmp_buf, "User-Agent: %s %s %s\r\n\r\n", model_name, model_number, VER_FIRMWARE);     
			strcat(send_buf, tmp_buf); 
			sprintf(tmp_buf, "Host: %s\r\nConnection: close\r\n", ddns_server[d_type]);
			strcat(send_buf, tmp_buf);
			strcpy(tmp_buf, "Authorization: Basic ");
			strcat(send_buf, tmp_buf);
			sprintf(tmp_buf1,"%s:%s",config_username, config_password);
			encode_base64(tmp_buf1, tmp_buf, strlen(tmp_buf1));
			strcat(send_buf, tmp_buf);
			strcpy(tmp_buf, "\r\n\r\n");
			strcat(send_buf, tmp_buf);
			DEBUG_MSG("In dynamic_update(), DYNDNS TYPE combine http finish \n");
			break;
		case ZONEEDIT_TYPE:
			sprintf(send_buf, "GET /auth/dynamic.html?host=%s",config_Rhostname);
			sprintf(tmp_buf, "&myip=%s",inet_ntoa(inetaddr));
			strcat(send_buf, tmp_buf);
			sprintf(tmp_buf, "&wildcard=%s",wildcard);
			strcat(send_buf, tmp_buf);
			sprintf(tmp_buf, "&mx=");
			strcat(send_buf, tmp_buf);
			sprintf(tmp_buf, "&backmx=NO");
			strcat(send_buf, tmp_buf);
			sprintf(tmp_buf, "&offline=NO ");
			strcat(send_buf, tmp_buf);
			strcpy(tmp_buf, "HTTP/1.1\r\nUser-Agent: client/1.1\r\n");
			strcat(send_buf, tmp_buf);
			sprintf(tmp_buf, "Host: %s\r\nConnection: close\r\n", ddns_server[d_type]);
			strcat(send_buf, tmp_buf);
			strcpy(tmp_buf, "Authorization: Basic ");
			strcat(send_buf, tmp_buf);
			sprintf(tmp_buf1,"%s:%s",config_username, config_password);
			encode_base64(tmp_buf1, tmp_buf, strlen(tmp_buf1));
			strcat(send_buf, tmp_buf);
			strcpy(tmp_buf, "\r\n\r\n");
			strcat(send_buf, tmp_buf);
			DEBUG_MSG("In dynamic_update(), ZONEEDIT TYPE combine http finish \n");
			break;
		case CYBER_GATE_TYPE:
			if(cyber_gate_ddns_flag == 0)//first query
			{
				sprintf(send_buf,"GET /gnudip/cgi-bin/gdipupdt.cgi HTTP/1.0\r\nUser-Agent: GnuDIP/2.3.5\r\nPragma: no-cache\r\nHost: %s:80\r\n\r\n",ddns_server[d_type]);
			}
			if(cyber_gate_ddns_flag == 1)//secondary query
                        {
				DEBUG_MSG(" ENTER secondary query\n");

				char *domain_name;
				unsigned char md5_result[128] = {};
				unsigned char final[128] = {}; 
				domain_name = (strstr(config_Rhostname,"."));
				domain_name = domain_name + 1;
/*	
                		MDbegin(&MD);
                		MDupdate(&MD,config_password,strlen(config_password));
                		strcpy(md5_result,MDprint(&MD));
				strcat(md5_result,".");
				strcat(md5_result,cyber_gate_salt);
				MDbegin(&MD);
				MDupdate(&MD,md5_result,strlen(md5_result));
				memset(md5_result,0,sizeof(md5_result));
				strcpy(md5_result,MDprint(&MD));
*/
				strcpy(md5_result,return_md5_result(config_password));
				DEBUG_MSG("First md5_result = %s\n",md5_result);
				strcat(md5_result,".");
				strcat(md5_result,cyber_gate_salt);
				strcpy(final,return_md5_result(md5_result));
				DEBUG_MSG("After marge md5_result = %s\n",final);

				sprintf(send_buf,"GET /gnudip/cgi-bin/gdipupdt.cgi?salt=%s&time=%s&sign=%s&user=%s&pass=%s&domn=%s&reqc=0&addr=%s HTTP/1.0\r\nUser-Agent: GnuDIP/2.3.5\r\nPragma: no-cache\r\nHost: %s:80\r\n\r\n",cyber_gate_salt,cyber_gate_time,cyber_gate_sign,config_username,final,domain_name,inet_ntoa(inetaddr),ddns_server[d_type]);
			}
			DEBUG_MSG("In dynamic_update(), CYBER GATE TYPE combine http finish \n");
			break;
		default:
			DEBUG_MSG("DDNS Type:%d not support !! \n",ddns_type);
			printf("DDNS Type:%d not support !! \n",ddns_type);
			return FAILURE;
	}

	printf("send_buf = %s\n", send_buf);
	web_converse_ret = converse_with_web_server(fd, d_type, config_Rhostname, send_buf);

	if ((web_converse_ret !=SUCCESS) && (web_converse_ret != CYBERGATEDDNS)){	
		printf("converse_with_web_server() fail !!!\n"); 
		return web_converse_ret;
	}
	else if(web_converse_ret == CYBERGATEDDNS)
	{
		printf("Ready to send sendary packet to server !!!\n");
		return CYBERGATEDDNS;
	}
	else
		printf("Dynamic update IP Successful !!!!\n");
		
	close(fd);	
	
	return web_converse_ret;
}


static void init_conversion_tables (void){
	unsigned char value; 
	unsigned char _offset;
	unsigned char _index;
	memset (base64_to_char, 0, sizeof (base64_to_char));
	value = 'A';
	_offset = 0;

	for (_index = 0; _index < 62; _index++)
	{
		if (_index == 26)
		{
			value = 'a';
			_offset = 26;
			_offset = 26;
		}
		else
			if (_index == 52)
			{
				value = '0';
				_offset = 52;
			}
		base64_to_char [_index] = value + _index - _offset;
	}
	base64_to_char [62] = '+';
	base64_to_char [63] = '/';
	tables_initialised = 1;
}


int encode_base64(const unsigned char *source, unsigned char *target, size_t source_size)
{
	int target_size = 0;                /*  Length of target buffer          */
	int nb_block;                       /*  Total number of blocks           */
	unsigned char       *p_source;      /*  Pointer to source buffer         */
	unsigned char       *p_target;      /*  Pointer to target buffer         */
	unsigned char       value;          /*  Value of Base64 byte             */

	if (source_size == 0)
		return (0);

	if (!tables_initialised)
		init_conversion_tables();

	nb_block = (int) (source_size / 3);
	/*  Check if we have a partially-filled block                            */
	if (nb_block * 3 != (int) source_size)
		nb_block++;

	target_size = (size_t) nb_block * 4;
	target [target_size] = '\0';
	p_source = (unsigned char *) source;         /*  Point to start of buffers        */
	p_target = target;

	while (nb_block--)
	{
		/*  Byte 1                                                           */
		value = *p_source >> 2;
		*p_target++ = base64_to_char [value];

		/*  Byte 2                                                           */
		value = (*p_source++ & 0x03) << 4;
		if ((size_t) (p_source - source) < source_size)
			value |= (*p_source & 0xF0) >> 4;
		*p_target++ = base64_to_char [value];

		/*  Byte 3 - pad the buffer with '=' if block not completed          */
		if ((size_t) (p_source - source) < source_size)
		{
			value = (*p_source++ & 0x0F) << 2;
			if ((size_t) (p_source - source) < source_size)
				value |= (*p_source & 0xC0) >> 6;
			*p_target++ = base64_to_char [value];
		}
		else
			*p_target++ = '=';

		/*  Byte 4 - pad the buffer with '=' if block not completed          */
		if ((size_t) (p_source - source) < source_size)
		{
			value       = *p_source++ & 0x3F;
			*p_target++ = base64_to_char [value];
		}
		else
			*p_target++ = '=';
	}
	return (target_size);
}
