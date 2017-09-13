/* script.c
 *
 * Functions to call the DHCP client notification scripts
 *
 * Russ Dill <Russ.Dill@asu.edu> July 2001
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <ctype.h>

#include "options.h"
#include "dhcpd.h"
#include "dhcpc.h"
#include "common.h"

#ifdef UDHCPD_NETBIOS
int need_nvram_commit=0;
#endif
/* get a rough idea of how long an option will be (rounding up...) */
static const int max_option_length[] = {
	[OPTION_IP] =		sizeof("255.255.255.255 "),
	[OPTION_IP_PAIR] =	sizeof("255.255.255.255 ") * 2,
	[OPTION_STRING] =	1,
	[OPTION_BOOLEAN] =	sizeof("yes "),
	[OPTION_U8] =		sizeof("255 "),
	[OPTION_U16] =		sizeof("65535 "),
	[OPTION_S16] =		sizeof("-32768 "),
	[OPTION_U32] =		sizeof("4294967295 "),
	[OPTION_S32] =		sizeof("-2147483684 "),
};


static inline int upper_length(int length, int opt_index)
{
	return max_option_length[opt_index] *
		(length / option_lengths[opt_index]);
}


static int sprintip(char *dest, char *pre, uint8_t *ip)
{
	return sprintf(dest, "%s%d.%d.%d.%d", pre, ip[0], ip[1], ip[2], ip[3]);
}


/* really simple implementation, just count the bits */
static int mton(struct in_addr *mask)
{
	int i;
	unsigned long bits = ntohl(mask->s_addr);
	/* too bad one can't check the carry bit, etc in c bit
	 * shifting */
	for (i = 0; i < 32 && !((bits >> i) & 1); i++);
	return 32 - i;
}

/* jimmy added 20080718, for checking NetBios info changed */
//int cur_has_option_wins = 0,
#ifdef UDHCPD_NETBIOS
#ifndef NVRAM_MAX_LEN
#define NVRAM_MAX_LEN 600
#endif
#define DHCP_FILE "/var/etc/udhcpd.conf"
extern int netbios_changed ;

struct wins_server_list{
    char server[32];
    struct wins_server_list *next;
    int checked;
};

struct netbios_info{
    unsigned char node_type;
    char *scope;
	int index;
    struct wins_server_list *wins_server;
};

void free_wins_server(struct wins_server_list *ptr){
        if(ptr->next != NULL){
                free_wins_server(ptr->next);
        }
        free(ptr);
}

void free_local_netbios(struct netbios_info *local_netbios){
     if(local_netbios != NULL){
        if(local_netbios->scope != NULL){
            free(local_netbios->scope);
        }
        if(local_netbios->wins_server != NULL){
            free_wins_server(local_netbios->wins_server);
        }
        free(local_netbios);
     }
}

int read_local_netbios(struct netbios_info **local_netbios){
    char buf[NVRAM_MAX_LEN] = {0},*ptr_start = NULL, *ptr_tmp = NULL;
    FILE *fp;
    int i = 0;
    struct wins_server_list *win_ptr_tmp = NULL;

    (*local_netbios) = malloc(sizeof(struct netbios_info));
    (*local_netbios)->scope = NULL;
    (*local_netbios)->wins_server = NULL;
	(*local_netbios)->node_type = '\0';
	(*local_netbios)->index = 0;

    if((fp = fopen(DHCP_FILE,"r")) != NULL){
        while(!feof(fp)){
            memset(buf,'\0',sizeof(buf));
            fgets(buf,sizeof(buf),fp);
            if(strstr(buf,"option wins")){
                i = 0;
                while(i < NVRAM_MAX_LEN){
                    if(isdigit(buf[i])){
                        ptr_start = &buf[i];
                        break;
                    }
                    i++;
                }
                if (ptr_start == NULL){
                	continue;
                }	
                ptr_tmp = strtok(ptr_start," ");
                if(ptr_tmp){
                    (*local_netbios)->wins_server = malloc(sizeof(struct wins_server_list));
                    memset((*local_netbios)->wins_server->server,'\0',32);
                    strcpy((*local_netbios)->wins_server->server,ptr_tmp);
                    (*local_netbios)->wins_server->checked = 0;
					(*local_netbios)->index = (*local_netbios)->index + 1;
                    (*local_netbios)->wins_server->next = NULL;
                }
                win_ptr_tmp = (*local_netbios)->wins_server;
                while((ptr_tmp = strtok(NULL," "))){
                    win_ptr_tmp->next = malloc(sizeof(struct wins_server_list));
                    win_ptr_tmp = win_ptr_tmp->next;
					win_ptr_tmp->next = NULL;
                    memset(win_ptr_tmp->server,'\0',32);
                    sscanf(ptr_tmp,"%s",win_ptr_tmp->server);
                    win_ptr_tmp->checked = 0;
					(*local_netbios)->index++;
                    win_ptr_tmp->next = NULL;
                }
            }else if(strstr(buf,"option nodetype")){
                i = 0;
                while(i < NVRAM_MAX_LEN){
                    if(isdigit(buf[i])){
                        ptr_start = &buf[i];
                        break;
                    }
                    i++;
                }
                (*local_netbios)->node_type = buf[i];
                //sscanf(ptr_start,"%s",(*local_netbios)->node_type);
            }else if((ptr_tmp = strstr(buf,"option scope"))){
                i = 0;
                ptr_tmp = ptr_tmp + strlen("option scope") + 1;
                while((ptr_tmp[0] != '\n') || (ptr_tmp[0] != '\0')){
                    if(ptr_tmp[0] != ' '){
                        ptr_start = ptr_tmp;
                        break;
                    }
                    ptr_tmp++;
                }
                (*local_netbios)->scope = malloc(strlen(ptr_start));
                sscanf(ptr_start,"%s",(*local_netbios)->scope);
            }
        }
        fclose(fp);
    }else{
		return 0;
	}
	return 1;
}

#endif
/* --------------------------------------------- */

/* Fill dest with the text of option 'option'. */
static void fill_options(char *dest, uint8_t *option, struct dhcp_option *type_p)
{
	int type, optlen;
	uint16_t val_u16;
	int16_t val_s16;
	uint32_t val_u32;
	int32_t val_s32;
	int len = option[OPT_LEN - 2];
#ifdef UDHCPD_NETBIOS
	struct in_addr wins_server_ip[5]={{0}};
	int i=0, length=0, index_local=0;
	char wins_servers[80]={0};
	char cmd[110]={0};
/* jimmy added 20080718, for checking NetBios info changed */
	struct netbios_info *local_netbios = NULL;
	struct wins_server_list *win_ptr_tmp = NULL;
	int already_has = 0;
	read_local_netbios(&local_netbios);
/* ------------------------------------------------------- */
#endif
	dest += sprintf(dest, "%s=", type_p->name);

	type = type_p->flags & TYPE_MASK;
	optlen = option_lengths[type];
	for(;;) {
		switch (type) {
			case OPTION_IP_PAIR:
				dest += sprintip(dest, "", option);
				*(dest++) = '/';
				option += 4;
				optlen = 4;
			case OPTION_IP:	/* Works regardless of host byte order. */
#ifdef UDHCPD_NETBIOS
				/* Add wins server info into nvram */
				if((type_p->code) == DHCP_WINS_SERVER)
				{
					wins_server_ip[index_local].s_addr=*(uint32_t *)option;
					index_local++;
					need_nvram_commit=1;
				}
#endif
				dest += sprintip(dest, "", option);
				break;
			case OPTION_BOOLEAN:
				dest += sprintf(dest, *option ? "yes" : "no");
				break;
			case OPTION_U8:
#ifdef UDHCPD_NETBIOS
				/* Add node type info into nvram */
				if((type_p->code) == DHCP_NODE_TYPE && client_config.netbios_enable == 1)
				{
					sprintf(cmd,"nvram set dhcpd_dynamic_node_type=%u",*option);
					system(cmd);
					need_nvram_commit=1;
			/* jimmy added 20080718, for checking NetBios info changed */
					if(!netbios_changed){
						if( (local_netbios->node_type == '\0') || (local_netbios->node_type != *option) ){
							netbios_changed = 1;
						}
					}
			/* ------------------------------------------------------- */
				}
#endif
				dest += sprintf(dest, "%u", *option);
				break;
			case OPTION_U16:
				memcpy(&val_u16, option, 2);
				dest += sprintf(dest, "%u", ntohs(val_u16));
				break;
			case OPTION_S16:
				memcpy(&val_s16, option, 2);
				dest += sprintf(dest, "%d", ntohs(val_s16));
				break;
			case OPTION_U32:
				memcpy(&val_u32, option, 4);
				dest += sprintf(dest, "%lu", (unsigned long) ntohl(val_u32));
				break;
			case OPTION_S32:
				memcpy(&val_s32, option, 4);
				dest += sprintf(dest, "%ld", (long) ntohl(val_s32));
				break;
			case OPTION_STRING:
#ifdef UDHCPD_NETBIOS
				/* Add scope info into nvram */
				if((type_p->code) == DHCP_SCOPE && client_config.netbios_enable == 1)
				{
					sprintf(cmd,"nvram set dhcpd_dynamic_scope=%s",option);
					system(cmd);
					need_nvram_commit=1;
			/* jimmy added 20080718, for checking NetBios info changed */
					if(!netbios_changed ){
						if((local_netbios->scope == NULL) || (strcmp(local_netbios->scope, option) !=0) ){
							netbios_changed = 1;
						}
					}
			/* ------------------------------------------------------- */
				}
#endif
				memcpy(dest, option, len);
				dest[len] = '\0';
				return;	 /* Short circuit this case */
		}
		option += optlen;
		len -= optlen;
		if (len <= 0) break;
		dest += sprintf(dest, " ");
	}
#ifdef UDHCPD_NETBIOS
	/* Organize all ip of wins-server to one line */
	if(index_local && client_config.netbios_enable == 1)
	{
		for(i=0 ; i<index_local ; i++)
		{
			sprintf(wins_servers+length,"%s/",inet_ntoa(wins_server_ip[i]));
			length = strlen(wins_servers);
		}
		wins_servers[length-1]='\0';
		sprintf(cmd,"nvram set dhcpd_dynamic_wins_server=%s",wins_servers);
		system(cmd);
	/* jimmy added 20080718, for checking NetBios info changed */
		if( !netbios_changed ){
			if(index_local != local_netbios->index){
				netbios_changed = 1;
			}else{
				//while(!netbios_changed && (win_ptr_tmp != NULL)){
				for(i=0 ; i<index_local && netbios_changed !=1 ; i++){
					already_has = 0;
					win_ptr_tmp = local_netbios->wins_server;
					while(win_ptr_tmp != NULL){
						if(strcmp(win_ptr_tmp->server,inet_ntoa(wins_server_ip[i])) == 0){
							already_has = 1;
							break;
						}
						win_ptr_tmp = win_ptr_tmp->next;
					}
					if(!already_has){
						netbios_changed = 1;
						break;
					}
				}
			}
		}
	/* ------------------------------------------------------- */
	}
/* jimmy added 20080718, for checking NetBios info changed */
	free_local_netbios(local_netbios);
/* ------------------------------------------------------- */
#endif
}


/* put all the parameters into an environment */
static char **fill_envp(struct dhcpMessage *packet)
{
	int num_options = 0;
	int i, j;
	char **envp;
	uint8_t *temp;
	struct in_addr subnet;
	char over = 0;


	if (packet == NULL)
		num_options = 0;
	else {
		for (i = 0; dhcp_options[i].code; i++)
			if (get_option(packet, dhcp_options[i].code)) {
				num_options++;
				if (dhcp_options[i].code == DHCP_SUBNET)
					num_options++; /* for mton */
			}
		if (packet->siaddr) num_options++;
		if ((temp = get_option(packet, DHCP_OPTION_OVER)))
			over = *temp;
		if (!(over & FILE_FIELD) && packet->file[0]) num_options++;
		if (!(over & SNAME_FIELD) && packet->sname[0]) num_options++;
	}

	envp = xcalloc(sizeof(char *), num_options + 5);
	j = 0;
	asprintf(&envp[j++], "interface=%s", client_config.interface);
	asprintf(&envp[j++], "%s=%s", "PATH",
			getenv("PATH") ? : "/bin:/usr/bin:/sbin:/usr/sbin");
	asprintf(&envp[j++], "%s=%s", "HOME", getenv("HOME") ? : "/");

	if (packet == NULL) return envp;

	envp[j] = xmalloc(sizeof("ip=255.255.255.255"));
	sprintip(envp[j++], "ip=", (uint8_t *) &packet->yiaddr);


	for (i = 0; dhcp_options[i].code; i++) {
		if (!(temp = get_option(packet, dhcp_options[i].code)))
			continue;

		/*	jimmy added 20080523, we implement option 33, 121, 249 ourself instead of 
			using original way to pass	those variables to shell script. 
			If we using original way here, when free memory later in run_script(),
			udhcpc will crash, "I think" the reason may be due to upper_length(), =__=...
		*/
		if(strncmp(dhcp_options[i].name,"fixroute",strlen("fixroute")) == 0){
			continue;
		}
		/* -------------------------------------------------------------------------------- */
		envp[j] = xmalloc(upper_length(temp[OPT_LEN - 2],
					dhcp_options[i].flags & TYPE_MASK) + strlen(dhcp_options[i].name) + 2);
		fill_options(envp[j++], temp, &dhcp_options[i]);

		/* Fill in a subnet bits option for things like /24 */
		if (dhcp_options[i].code == DHCP_SUBNET) {
			memcpy(&subnet, temp, 4);
			asprintf(&envp[j++], "mask=%d", mton(&subnet));
		}
	}
#ifdef UDHCPD_NETBIOS	
	/* If we get wins server option from wan dhcp server and save the info into nvram,
	 * we need perform "nvram commit".
	 */
	if(need_nvram_commit)
	{
		system("nvram commit");	
		need_nvram_commit=0;		
	}
#endif
	if (packet->siaddr) {
		envp[j] = xmalloc(sizeof("siaddr=255.255.255.255"));
		sprintip(envp[j++], "siaddr=", (uint8_t *) &packet->siaddr);
	}
	if (!(over & FILE_FIELD) && packet->file[0]) {
		/* watch out for invalid packets */
		packet->file[sizeof(packet->file) - 1] = '\0';
		asprintf(&envp[j++], "boot_file=%s", packet->file);
	}
	if (!(over & SNAME_FIELD) && packet->sname[0]) {
		/* watch out for invalid packets */
		packet->sname[sizeof(packet->sname) - 1] = '\0';
		asprintf(&envp[j++], "sname=%s", packet->sname);
	}

	return envp;
}

/*	Date: 2009-01-07
*	Name: jimmy huang
*	Input:
*	Output: 1 (is dhcpc), 0 (not dhcpc), -1 (can't open nvram file or not has this key)
*	Reason: we don't want use nvram library
*	Note: Marked by jimmy, 2009-01-19, because
*			1. we already can recognize wan proto via input parameter
*/
/*
int is_wan_proto_dhcpc(void){
	char buff[256];
	char *ptr = NULL;
	int status = -1;
	FILE *fp;
	char mvram_file[]="/var/etc/nvram.conf";
	char keyword[]="wan_proto";
	char tmp_file[]="/tmp/useless.txt";

	sprintf(buff,"cat %s |grep \"%s\" > %s",mvram_file,keyword,tmp_file);
	system(buff);

	if((fp = fopen(tmp_file,"r"))!=NULL){
		while(!feof(fp)){
			memset(buff,'\0',sizeof(buff));
			fgets(buff,sizeof(buff),fp);
			if(buff[0] != '\n' && buff[0] != '\0' && buff[0] != '#'){   //ignore #!/bin.sh
				if( (strncmp(buff,keyword,strlen(keyword)) == 0) && (buff[strlen(keyword)] == '=')){
					ptr = strchr(buff,'=');
					ptr++;
					if(strncmp(ptr,"dhcpc",strlen("dhcpc")) == 0){
						status = 1;
					}else{
						status = 0;
					}
					DEBUG(LOG_ERR, "%s, nvram value = %d\n",__FUNCTION__,status);
					break;
				}
			}
		}
		fclose(fp);
	}
	unlink(tmp_file);
	return status;
}
*/

/* Call a script with a par file and env vars */
void run_script(struct dhcpMessage *packet, const char *name)
{
	int pid;
	char **envp, **curr;

	if (client_config.script == NULL)
		return;

	DEBUG(LOG_INFO, "vforking and execle'ing %s", client_config.script);

	envp = fill_envp(packet);
	
	/* call script */
#if defined(__uClinux__) && !defined(IP8000)
	pid = vfork();
#else    	
	pid = fork();
#endif

	if (pid) 
	{
		waitpid(pid, NULL, 0);
		for (curr = envp; *curr; curr++) free(*curr);
		free(envp);
		return;
	}
	else if (pid == 0) 
	{
		/* close fd's? */

		/* NickChou, 2008.3.3, in RUSSIA, we do not add default gateway here.
		   we add default gateway in route table at ppp dial up process. 
		*/
/* jimmy marked, 20080526, cause TSD will test non-Russia mode with Russia environment */
//#ifdef RPPPOE		
//		if( client_config.russia_enable )
//		{
/* ------------------------------------------------------------------------------------ */
	/*jimmy added ,20080721, for wan proto is dhcpc, do not "append" physical dns */
//		if( strcmp(nvram_safe_get("wan_proto"),"dhcpc") )
		/*	Date: 2009-01-07
		*	Name: jimmy huang
		*	Reason:	we don't want to use nvram library
		*	Note: Modified the codes above to new codes below
		*		  Modified by jimmy huang 2009-01-19, because
		*			1. should be is_wan_proto_dhcpc()==0
		*			2. we can recognize wan proto via input parameter
		*/
		//if( is_wan_proto_dhcpc()==1 )
		if(strcmp(client_config.wan_proto,"dhcpc") != 0)
		{
	/* ------------------------------------------------------------- */
			if( strcmp(client_config.script, "/usr/share/udhcpc/default.bound-dns") == 0 )
				client_config.script = RUSSIA_DHCPC_DNS_SCRIPT;
			else if( strcmp(client_config.script, "/usr/share/udhcpc/default.bound-nodns") == 0 )
				client_config.script = RUSSIA_DHCPC_NODNS_SCRIPT;
	/*jimmy added ,20080703, for wan proto is dhcpc, do not "append" */
		}
	/* ------------------------------------------------------------- */
				
/* jimmy marked, 20080526, cause TSD will test non-Russia mode with Russia environment */
//		}
//#endif
/* -------------------------------------------------------------------------------------- */
		/* exec script */
		execle(client_config.script, client_config.script,
				name, NULL, envp);
		LOG(LOG_ERR, "script %s failed: %m", client_config.script);
#if defined(__uClinux__) && !defined(IP8000)
        _exit(1);
#else    	
        exit(1);
#endif
	}
}
