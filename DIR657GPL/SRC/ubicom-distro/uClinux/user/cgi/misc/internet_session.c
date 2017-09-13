#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>

#include "libdb.h"
#include "nvram.h"
#include "debug.h"

#define MAX_NUMBER_FIELD 18
#define IP_CONNTRACK_FILE "/proc/net/ip_conntrack"

extern int check_valid(char *, char *, char *, char *);
extern char *get_wan_ip(char *);
extern char *get_protocol_name(char *);

static struct st_info{
	char *st_type;
	char *st_value;
} st_info[9] = {
	{"NONE", "NO"}, 
	{"SYN_SENT", "SS"},
	{"SYN_RECV", "SR"},
	{"ESTABLISHED", "EST"},
	{"FIN_WAIT", "FW"},
	{"CLOSE_WAIT", "CW"},
	{"TIME_WAIT", "TW"},
	{"LAST_ACK", "LA"},
	{"CLOSE", "CL"}
};

struct __internet_session {
	const char *param;
	const char *desc;
	int (*fn)(int, char *[]);
};

static int misc_internet_session_help(struct __internet_session *p)
{
	cprintf("Usage:\n");

	for (; p && p->param; p++)
		cprintf("\t%s: %s\n", p->param, p->desc);

	cprintf("\n");
	return 0;
}

static int default_protocol_data(char st[][24], char *lan_ip, char *lan_mask,
	 char *wan_ip_1, int len, char *session, int ajax_flag)
{
	char *local_ip=NULL, *remote_ip=NULL, \
	     *public_port=NULL, *protocol=NULL, \
             *tmp_ip=NULL, *src_ip=NULL,*dst_ip=NULL, *p = NULL;
        int direction_out = 0, i = 0, has_src_ip = 0,has_dst_ip = 0;
	char tmp_session[512];

        struct in_addr netmask, myaddr, saddr;

	inet_aton(lan_ip, &myaddr);
	inet_aton(lan_mask, &netmask);

	has_src_ip = 0;
	has_dst_ip = 0;
	/* Get src IP and dst IP*/
        for (i = 0; i < len; i++) {
                if (strlen(st[i]) != 0 || strcmp(st[i], "") != 0) {
                        if (has_src_ip == 0 && (p=strstr(st[i], "src"))) {
                                src_ip = p+4;
                                has_src_ip=1;
                                continue;
                        }
                        if (has_dst_ip==0 && (p=strstr(st[i],"dst"))) {
                                dst_ip = p+4;
                                has_dst_ip=1;
                                continue;
                        }
                }
        }

        if (has_src_ip && has_dst_ip) {
                tmp_ip = src_ip;
                inet_aton(tmp_ip, &saddr);

                /* from LAN */
                if ((netmask.s_addr & myaddr.s_addr) == (netmask.s_addr & saddr.s_addr)) {
                        direction_out = 1;
                        local_ip = src_ip;
                        remote_ip = dst_ip;
                } else { /* from WAN */
                        direction_out = 0;
                        local_ip = dst_ip;
                        remote_ip = src_ip;
                }
                if (check_valid(local_ip, remote_ip, lan_ip, wan_ip_1))
                        return -1;
                protocol = get_protocol_name(st[1]);
		if (strcmp(protocol, "icmp") == 0 || strncmp(protocol, "UNKONWN", strlen(protocol)) == 0)
			return -1;
                sprintf(tmp_session, "%s/%s/%s/%s/%s/%s/%s/%s/%s/%s/%s", \
                        protocol, st[2], "-",
                        (direction_out == 1) ? "OUT" : "IN", \
                        local_ip, "", remote_ip, "", \
                        (public_port != NULL) ? public_port+1 : "-", st[len-5], st[len-4]);
		if (ajax_flag == 0)
			printf("%s,", tmp_session);
		else
			strcpy(session, tmp_session);
        }

	return 0;
}

static int udp_protocol_data(char st[][24], char *lan_ip, char *lan_mask, 
	char *wan_ip_1, int len, char *session, int ajax_flag)
{
	char *local_ip = NULL, *remote_ip = NULL, *local_port = NULL, *remote_port = NULL, \
	     *public_port = NULL, *protocol = NULL, *tmp_ip = NULL;
        int direction_out = 0;
        struct in_addr netmask, myaddr, saddr;
	char tmp_session[512];

	inet_aton(lan_ip, &myaddr);
	inet_aton(lan_mask, &netmask);
	tmp_ip = strchr(st[3], '=');
	tmp_ip++;
	inet_aton(tmp_ip, &saddr);
	/* from LAN */
	if ((netmask.s_addr & myaddr.s_addr) == \
	   (netmask.s_addr & saddr.s_addr)) {
		direction_out = 1;
		local_ip = strchr(st[3], '=');
		remote_ip = strchr(st[4], '=');
		local_port = strchr(st[5], '=');
		remote_port = strchr(st[6], '=');
		public_port = strchr(st[12], '=');

	} else { /* from WAN */
		local_ip = strchr(st[10], '=');
		local_port = strchr(st[12], '=');
		remote_ip = strchr(st[3], '=');
		remote_port = strchr(st[5], '=');
		public_port = strchr(st[6], '=');
	}

	if (local_ip == NULL || remote_ip == NULL ) {
		return -1;
	}
	if (check_valid(local_ip+1, remote_ip+1, lan_ip, wan_ip_1)) {
		return -1;
	}
	protocol = get_protocol_name(st[1]);

	sprintf(tmp_session, "%s/%s/%s/%s/%s/%s/%s/%s/%s/%s/%s", \
		protocol, st[2], "-", (direction_out == 1) ? "OUT" : "IN", \
		local_ip+1, local_port+1, remote_ip+1, remote_port+1, \
		(public_port+1) != NULL ? public_port+1 : "-", st[len-5], st[len-4]);
	if (ajax_flag == 0)
		printf("%s,", tmp_session);
	else
		strcpy(session, tmp_session);
	return 0;
}

static int tcp_protocol_data(char st[][24], char *lan_ip, char *lan_mask, 
	char *wan_ip_1, int len, char *session, int ajax_flag)
{	
	char state[4];
	char *local_ip=NULL, *remote_ip=NULL, *local_port=NULL, *remote_port=NULL,\
	     *public_port=NULL, *protocol=NULL, *tmp_ip=NULL;
        int direction_out=0, i=0;
	char tmp_session[512];

        struct in_addr netmask, myaddr, saddr;

	inet_aton(lan_ip, &myaddr);
	inet_aton(lan_mask, &netmask);

        if(!strcmp(st[10], "[UNREPLIED]")) {
		return -1;
	}

	for (i = 0; i < 8; i++) {
		if (strcmp(st_info[i].st_type, st[3]) == 0) {
			strcpy(state, st_info[i].st_value);
			break;
		}
	}

	tmp_ip = strchr(st[4], '=');
	tmp_ip++;
	inet_aton(tmp_ip, &saddr);
	/* from LAN */
	if ((netmask.s_addr & myaddr.s_addr) == (netmask.s_addr & saddr.s_addr)) {
		direction_out = 1;
		local_ip = strchr(st[4], '=');
		remote_ip = strchr(st[5], '=');
		local_port = strchr(st[6], '=');
		remote_port = strchr(st[7], '=');
		public_port = strchr(st[13], '=');
	} else { /* from WAN */
		direction_out = 0;
		local_ip = strchr(st[10], '=');
		local_port = strchr(st[12], '=');
		remote_ip = strchr(st[4], '=');
		remote_port = strchr(st[6], '=');
		public_port = strchr(st[7], '=');
	}
	
	if (local_ip == NULL || remote_ip == NULL) {
		return -1;
	}
	if (check_valid(local_ip+1, remote_ip+1, lan_ip, wan_ip_1)) {
		return -1;
	}
	protocol = get_protocol_name(st[1]);
	sprintf(tmp_session, "%s/%s/%s/%s/%s/%s/%s/%s/%s/%s/%s", \
		protocol, st[2], state, (direction_out == 1) ? "OUT" : "IN", \
		local_ip+1, local_port+1, remote_ip+1, remote_port+1, \
		(public_port+1) != NULL ? public_port+1 : "-", st[len-5], st[len-4]);
	if (ajax_flag == 0)
		printf("%s,",tmp_session);
	else
		strcpy(session, tmp_session);
        return 0;
}

/*
 * All session is based on /proc/net/ip_conntrack. All packets sending to the router will be recorded in the file.
 * INPUT PARAM: 
 * proto_flag - Which kind of protocol we need to gather information about?
 *   1, we need to gather information for ONLY TCP and UDP.
 *   2, we need to gather information for ALL.
 *
 * packet_type - What status of packets we need to gather information about?
 *  ALL: all kinds of session excepts for [UNREPLIED] session
 *  ESTABLISHED: only ESTABLISHED session for TCP and all session but [UNREPLIED] for UDP and ICMP
 *
 * ============================================================================
 * linux 2.4  
 * ============================================================================
 * udp format
 * [name |proto |time |src_ip |dst_ip |sport |dport |reply |src_ip2 |dst_ip2 |sport2  |dport2 ] 
 * [udp  |17    |40   |src    |dst    |sport |dport |[UNREPLIED]|src|dst     |sport   |dport  ]
 *
 * tcp format
 * [name |proto |time |state  |src_ip |dst_ip|sport |dport |reply   |src_ip2 |dst_ip2 |sport2 |dport2 ]
 * [tcp  |6     |7500 |ESTABLISHED|src|dst   |sport |dport |[UNREPLIED]|src  |dst     |sport  |dport  ]
 *
 * ===========================================================================
 * linux 2.6  
 * ===========================================================================
 * udp format
 * [name |proto |time  |src_ip |dst_ip |sport |dport |packets |bytes |reply |src_ip2 |dst_ip2 |sport2 |dport2 ]
 * [udp  |17    |40    |src    |dst    |sport |dport |packets |bytes |[UNREPLIED]|src|dst     |sport  |dport  ]
 *
 * tcp format
 * [name |proto |time |state   |src_ip |dst_ip|sport |dport |packets |bytes |reply |src_ip2 |dst_ip2 |sport2 
 * |dport2 ]
 * [tcp  |6     |7500 |ESTABLISHED|src |dst   |sport |dport |packets |bytes |[UNREPLIED]|src|dst     |sport  
 * |dport  ]
 *
 */

/*
udp 17 49 src=172.21.45.63  dst=172.21.47.255 sport=138 dport=138 packets=1 bytes=241 [UNREPLIED] 
          src=172.21.47.255 dst=172.21.45.63  sport=138 dport=138 packets=0 bytes=0   mark=0 use=1
*/
static void get_napt_session(int proto_flag, char *packet_type, int ajax_flag)
{
	FILE *fp = NULL;
	char buf[1024], lan_ip[16], lan_mask[16], wan_ip_1[16];
        char *tmp_ip = NULL, *tmp_mask = NULL, *tmp = NULL, *tmp1 = NULL;
	char st[20][24], session[512];
	int i = 0, cnt = 0, session_idx = 0, res = 0;

	memset(buf, '\0', sizeof(buf));
	memset(lan_ip, '\0', sizeof(lan_ip));
	memset(lan_mask, '\0', sizeof(lan_mask));
	memset(wan_ip_1, '\0', sizeof(wan_ip_1));

	get_wan_ip(wan_ip_1);
	/* wan_ipaddr == NULL, ACTIVE session don't start */
        if (!(strlen(wan_ip_1)))
                return;
        /* lan_ip=NULL implies that there is no active session */
        if ((tmp_ip = nvram_safe_get("lan_ipaddr")) == NULL)
		return;
	if ((tmp_mask = nvram_safe_get("lan_netmask")) == NULL)
		return;

	strcpy(lan_ip, tmp_ip);
	strcpy(lan_mask, tmp_mask);

	/* open file */
	if ((fp = fopen(IP_CONNTRACK_FILE, "r")) == NULL){
		cprintf("Open %s record file FAIL\n", IP_CONNTRACK_FILE);
		return;
	}

	memset(buf, '\0', sizeof(buf));

	if (ajax_flag == 1) {
		printf("{");
	}

	if (ajax_flag == 2) {
		printf("<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\" ?>" \
                "<env:Envelope " \
                "xmlns:env=\"httpd://www.w3.org/org/2003/05/spap-envlop\" " \
                "env:encodingStyle=\"httpd://www.w3.org/org/2003/05/spap-envlop\">" \
                "<env:Body>\n");
	}

	while(fgets(buf, sizeof(buf), fp) != NULL) {
		memset(st, '\0', sizeof(st));
		memset(session, '\0', sizeof(session));
		buf[strlen(buf) -1] = '\0';
		tmp = buf;
		cnt = 0; i = 0;
		while ((tmp1 = strsep(&tmp, " ")) != NULL) {
			if (cnt >= 1 && cnt <= 5){
				cnt++;
				continue;
			}
			sprintf(st[i], "%s", tmp1);
			cnt++;
			i++;
		}
		
		switch (atoi(st[1])) {
		  case 6: /* TCP ptotocol */
			if ((!strcmp(packet_type, "ESTABLISHED") \
				&&  !strcmp(st[3], "ESTABLISHED")) \
				|| !strcmp(packet_type, "ALL")) {
				if ((res = tcp_protocol_data(st, lan_ip, lan_mask, 
				wan_ip_1, i, session, ajax_flag)) != 0)
					continue;
			}
			break;

		  case 17: /* UDP ptotocol */
			if (strcmp(st[9], "[UNREPLIED]")) {
				if ((res =udp_protocol_data(st, lan_ip, lan_mask,	
				wan_ip_1, i, session, ajax_flag)) != 0)
					continue;
			}
			break;

		  default:
			if(proto_flag == 2) {
				if ((res = default_protocol_data(st, lan_ip, lan_mask,
				wan_ip_1, i, session, ajax_flag)) != 0)
					continue;
			}
			break;
		}
		
		if (ajax_flag == 1 && strcmp(session, "") != 0) {
			sprintf(buf, "session%d : \"%s\",", session_idx, session);
			session_idx++;
			printf("%s\n", buf);
		}
		if (ajax_flag == 2 && strcmp(session, "") != 0) {
			sprintf(buf, "<session%d>%s</session%d>",
			session_idx, session, session_idx);
			session_idx++;
			printf("%s\n", buf);
		}

		memset(session, '\0', sizeof(session));
		memset(buf, '\0', sizeof(buf));
	}
	/* close file */
	fclose(fp);

	if (ajax_flag == 1) {
		printf("len : \"%d\"}", session_idx);
	}
	if (ajax_flag == 2) {
		printf("<len>%d</len></env:Body></env:Envelope>", session_idx);
	}
}

static int internet_session_table(int argc, char *argv[])
{
	int ret = -1;
	if (argc < 2)
		goto out;

	if (strcmp(argv[1], "2") == 0) {
		get_napt_session(2, "ALL", 0);
		ret = 0;
	}
out:
	return ret;
}

static int create_json_file(int argc, char *argv[])
{
	get_napt_session(2, "ALL", 1);
}

static int create_xml_file(int argc, char *argv[])
{
	get_napt_session(2, "ALL", 2);
}


int internet_session_main(int argc, char *argv[])
{
	int ret = -1;
	struct __internet_session *p, list[] = {
		{ "table", "get internet session table", internet_session_table},
		{ "json", "Create Json file", create_json_file},
		{ "xml", "Create Json file", create_xml_file},
		{ NULL, NULL, NULL }
	};

	if (argc == 1 || strcmp(argv[1], "help") == 0)
		goto help;

	for (p = list; p && p->param; p++) {
		if (strcmp(argv[1], p->param) == 0) {
			ret = p->fn(argc - 1, argv + 1);
			goto out;
		}
	}
help:
	ret = misc_internet_session_help(list);
out:
	return ret;
}
