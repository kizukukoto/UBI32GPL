#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h> 
#include <ctype.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>


#include "debug.h"
#include "shvar.h"
#include "nvram.h"

#define RTF_UP          0x0001          /* route usable                 */
#define RTF_GATEWAY     0x0002          /* destination is a gateway     */
#define PATH_PROCNET_ROUTE      "/proc/net/route"


struct __routetable_struct {
        const char *param;
        const char *desc;
        int (*fn)(int, char *[]);
};

typedef struct routing_table_s{
        char dest_addr[16];
        char dest_mask[16];
        char gateway[16];
        char interface[10];
        int metric;
        int type;
        int creator;
} routing_table_t;

routing_table_t routing_table[];

char *proc_gen_fmt(char *name, int more, FILE * fh,...)
{
        char buf[512], format[512] = "";
        char *title, *head, *hdr;
        va_list ap;

        if (!fgets(buf, (sizeof buf) - 1, fh))
                return NULL;
        strcat(buf, " ");

        va_start(ap, fh);
        title = va_arg(ap, char *);
        for (hdr = buf; hdr;) {
                while (isspace(*hdr) || *hdr == '|')
                        hdr++;
                head = hdr;
                hdr = strpbrk(hdr, "| \t\n");
                if (hdr)
                        *hdr++ = 0;

                if (!strcmp(title, head)) {
                        strcat(format, va_arg(ap, char *));
                        title = va_arg(ap, char *);
                        if (!title || !head)
                                break;
                } else {
                        strcat(format, "%*s");  /* XXX */
                }
                strcat(format, " ");
        }
        va_end(ap);
	if (!more && title) {
                cprintf("warning: %s does not contain required field %s\n",name, title);
                return NULL;
        }
        return strdup(format);
}


char *read_gatewayaddr(char *if_name)
{
        FILE *fp;
        char buff[1024], iface[16];
        char gate_addr[128], net_addr[128];
        char mask_addr[128], *fmt;
        int num, iflags, metric, refcnt, use, mss, window, irtt;
        struct in_addr gateway, dest, route, mask;
        int success=0, count =0;

        int i = 0;
        char lan_bridge[5];

	
	memset(lan_bridge,'\0',sizeof(lan_bridge));

	fp = fopen (PATH_PROCNET_ROUTE, "r");
        if(!fp) {
                perror(PATH_PROCNET_ROUTE);
                return 0;
        }

        fmt = proc_gen_fmt(PATH_PROCNET_ROUTE, 0, fp,
                        "Iface", "%16s",
                        "Destination", "%128s",
                        "Gateway", "%128s",
                        "Flags", "%X",
                        "RefCnt", "%d",
                        "Use", "%d",
                        "Metric", "%d",
                        "Mask", "%128s",
                        "MTU", "%d",
                        "Window", "%d",
                        "IRTT", "%d",
                         NULL);
	if (!fmt)
		return 0;
	
	for(count =0;  strlen(routing_table[count].dest_addr) >0; count++ ) {
                strcpy(routing_table[count].dest_addr, "") ;
        }
        count =0;

        while( fgets(buff, 1024, fp)) {
                num = sscanf(buff, fmt, iface, net_addr, gate_addr, &iflags, &refcnt, &use, &metric, mask_addr, &mss, &window, &irtt);

                if(iflags  & RTF_UP ) {
                        dest.s_addr = strtoul(net_addr, NULL, 16);
                        route.s_addr = strtoul(gate_addr, NULL, 16);
                        mask.s_addr = strtoul(mask_addr, NULL, 16);

                        /* for route_table */
                        strcpy(routing_table[count].dest_addr, inet_ntoa(dest));
                        strcpy(routing_table[count].dest_mask, inet_ntoa(mask));
                        strcpy(routing_table[count].gateway, inet_ntoa(route));
                        strcpy(routing_table[count].interface, iface);
                        routing_table[count].metric = metric;
			 /*      Date: 2008-12-22
 			  *	 Name: jimmy huang
 			  *	 Reason: for the feature routing information can show "Type" and "Creator"
 			  *	   so we add 2 more fileds in routing_table_s structure
 			  *	   Note:
 			  *	   	Type:
 			  *			 0:INTERNET:     The table is learned from Internet
 			  *			 1:DHCP Option:  The table is learned from DHCP server in Internet
 			  *			 2:STATIC:       The table is learned from "Static Route" for Internet
 			  *			 		 Port of DIR-Series
 			  *			 3:INTRANET      :       The table is used for Intranet
 			  *			 4:LOCAL         :       Local loop back
 			  *		 Creator:
 			  *		  	 0:System:               Learned automatically
 			  *		  	 1:ADMIN:                Learned via user's operation in Web GUI 
 			  */

			if(strncmp(routing_table[count].interface,lan_bridge,strlen(lan_bridge)) == 0)//br0
                                routing_table[count].type = 3;
                        else if(strcmp(routing_table[count].interface,"lo") == 0)
                                routing_table[count].type = 4;
                        else
                             	routing_table[count].type = 0;
                        
			routing_table[count].creator = 0;
/*			cprintf("%s, reading in dest_addr = %s, dest_mask = %s, gw = %s, interface = %s, type = %d(%s), creator = %d(%s), metric = %d\n"
                                ,__FUNCTION__
                                ,routing_table[count].dest_addr
                                ,routing_table[count].dest_mask
                                ,routing_table[count].gateway
                                ,routing_table[count].interface
                                ,routing_table[count].type
                                ,(routing_table[count].type == 0) ? "Internet" : ((routing_table[count].type == 1) ? "DHCP Option" : "STATIC")
                                ,routing_table[count].creator
                                ,routing_table[count].creator ? "ADMIN" : "System"
                                ,routing_table[count].metric
                                );
*/	
	 		count +=1;
         		/* end */
			if ((iflags & RTF_GATEWAY) && !strcmp("0.0.0.0", inet_ntoa(dest)) && !strcmp(iface, if_name) ) {
				gateway.s_addr = strtoul(gate_addr,NULL,16);
				success =1;
                	}
		}
	}
	free(fmt);
	fclose(fp);
	
	if(success)
		return inet_ntoa(gateway);
	else
		return "0.0.0.0";

}

routing_table_t *read_route_table(void)
{
	read_gatewayaddr(nvram_safe_get("wan_eth"));
	return &routing_table;
}


int routing_table_info(int argc, char *argv[])
{
	int i = 0;
        routing_table_t *routing_table_list;
        routing_table_list = read_route_table();
	 for(i=0; strlen(routing_table_list[i].dest_addr) >0; i++ ){
                printf("%s/%s/%s/%s/%d/%d/%d," \
                        ,routing_table_list[i].dest_addr \
                        ,routing_table_list[i].dest_mask \
                        ,routing_table_list[i].gateway \
                        ,routing_table_list[i].interface \
                        ,routing_table_list[i].metric \
                        ,routing_table_list[i].creator \
                        ,routing_table_list[i].type \
                        );
        }
	return 0;
}


static int misc_routetable_help(struct __routetable_struct *p)
{
        cprintf("Usage:\n");

        for (; p && p->param; p++)
                cprintf("\t%s: %s\n", p->param, p->desc);

        cprintf("\n");
        return 0;
}


int get_route_table_main(int argc, char *argv[])
{
	int ret = -1;
        struct __routetable_struct *p, list[] = {
                { "routing_table", "Show route table", routing_table_info},
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
        ret = misc_routetable_help(list);
out:
        return ret;

}
