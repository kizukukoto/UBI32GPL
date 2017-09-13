/* Shared library add-on to iptables to add masquerade support. */
#include <stdio.h>
#include <netdb.h>
#include <string.h>
#include <stdlib.h>
#include <getopt.h>
#include <iptables.h>
#include <linux/netfilter_ipv4/ip_tables.h>
#include <linux/netfilter/xt_PORTTRIGGER.h>

/* Function which prints out usage message. */
static void
help(void)
{
	printf(
"PORTTRIGGER v%s options:\n"
" --mode [dnat (PREROUTING)|forward_in|forward_out (FORWARD)]\n"
" --trigger-proto [tcp|udp|all(any)]\n"
" --forward-proto [tcp|udp|all(any)]\n"
" --trigger-ports <port>[-<port>,<port>]\n"
" --forward-ports <port>[-<port>,<port>]\n"
" --timer [sec]\n",
IPTABLES_VERSION);
}

static struct option opts[] = {
	{"mode", 1, 0, '1'},
	{ "trigger-proto", 1, 0, '2' },
	{ "forward-proto", 1, 0, '3' },
	{ "trigger-ports", 1, 0, '4' },
	{ "forward-ports", 1, 0, '5' },
	{ "timer", 1, 0, '6' },
	{ 0 }
};

/* Initialize the target. */
static void
init(struct xt_entry_target *t)
{
}

static void
parse_multi_ports(const char *portstring, 
	struct xt_mport *minfo)
{
	char *buffer, *cp, *next, *range;
	unsigned int i;
        u_int16_t m;

	buffer = strdup(portstring);
	if (!buffer) xtables_error(OTHER_PROBLEM, "strdup failed");

        minfo->pflags = 0;

	for (cp=buffer, i=0, m=1; cp && i<IPT_MULTI_PORTS; cp=next,i++,m<<=1)
	{
		next=strchr(cp, ',');
		if (next) *next++='\0';
                range = strchr(cp, '-');
                if (range) {
                        if (i == IPT_MULTI_PORTS-1)
                                xtables_error(PARAMETER_PROBLEM,
                                           "too many ports specified");
                        *range++ = '\0';
                }
		minfo->ports[i] = atoi(cp);
                if (range) {
                        minfo->pflags |= m;
                        minfo->ports[++i] = atoi(range);
                        if (minfo->ports[i-1] >= minfo->ports[i])
                                xtables_error(PARAMETER_PROBLEM,
                                           "invalid portrange specified");
                        m <<= 1;
                }
	}

	if (cp) xtables_error(PARAMETER_PROBLEM, "too many ports specified");
	if (i == IPT_MULTI_PORTS-1)
		minfo->ports[i] = minfo->ports[i-1];
	else if (i < IPT_MULTI_PORTS-1) {
		minfo->ports[i] = ~0;
		minfo->pflags |= 1<<i;
	}
	free(buffer);
}


/* Function which parses command options; returns true if it
   ate an option */
static int
parse(int c, char **argv, int invert, unsigned int *flags,
      const void *entry,
      struct xt_entry_target **target)
{
	struct xt_porttrigger_info *info = (struct xt_porttrigger_info *)(*target)->data;

	switch (c) {
	
		case '1':
			if (xtables_check_inverse(optarg, &invert, &optind, 0, argv))
				xtables_error(PARAMETER_PROBLEM, "Unexpected `!' ");
			if (!strcasecmp(optarg, "dnat"))
				info->mode= MODE_DNAT;
			else if (!strcasecmp(optarg, "forward_in"))
				info->mode= MODE_FORWARD_IN;
			else if (!strcasecmp(optarg, "forward_out"))
				info->mode= MODE_FORWARD_OUT;
			return 1;
			
		case '2':
			if (xtables_check_inverse(optarg, &invert, &optind, 0, argv))
				xtables_error(PARAMETER_PROBLEM, "Unexpected `!' ");
			if (!strcasecmp(optarg, "tcp"))
				info->trigger_proto= IPPROTO_TCP;
			else if (!strcasecmp(optarg, "udp"))
				info->trigger_proto = IPPROTO_UDP;
			else if (!strcasecmp(optarg, "all") || !strcasecmp(optarg, "any"))
				info->trigger_proto = 0;
			return 1;

		case '3':
			if (xtables_check_inverse(optarg, &invert, &optind, 0, argv))
				xtables_error(PARAMETER_PROBLEM, "Unexpected `!' ");
			if (!strcasecmp(optarg, "tcp"))
				info->forward_proto= IPPROTO_TCP;
			else if (!strcasecmp(optarg, "udp"))
				info->forward_proto = IPPROTO_UDP;
			else if (!strcasecmp(optarg, "all") || !strcasecmp(optarg, "any"))
				info->forward_proto = 0;
			return 1;

		case '4':
			if (xtables_check_inverse(optarg, &invert, &optind, 0, argv))
				xtables_error(PARAMETER_PROBLEM, "Unexpected `!' ");
			parse_multi_ports(optarg, &info->trigger_ports);
			return 1;

		case '5':
			if (xtables_check_inverse(optarg, &invert, &optind, 0, argv))
				xtables_error(PARAMETER_PROBLEM, "Unexpected `!' ");
			parse_multi_ports(optarg, &info->forward_ports);
			return 1;
				
		case '6':
			if (xtables_check_inverse(optarg, &invert, &optind, 0, argv))
				xtables_error(PARAMETER_PROBLEM, "Unexpected `!' ");
			info->timer = atoi(optarg);
			return 1;
				
		default:
			return 0;
	}
}

/* Final check; don't care. */
static void final_check(unsigned int flags)
{
}

static void print_proto(int16_t proto)
{
	if(proto == IPPROTO_TCP)
		printf("TCP ");
	else if(proto == IPPROTO_UDP)
		printf("UDP ");
	else if(proto == 0)
		printf("ALL ");
}


static void print_multi_ports(struct xt_mport minfo)
{
	int i;
	u_int16_t pflags = minfo.pflags;

	for (i=0; i < IPT_MULTI_PORTS; i++) {
		if ( pflags & (1<<i) && minfo.ports[i] == 65535)
			break;
		if (i == IPT_MULTI_PORTS-1 && minfo.ports[i-1] == minfo.ports[i] )
			break;
		printf("%s", i ? "," : "");
		printf("%d", minfo.ports[i]);
		if (pflags & (1<<i)) {
			printf("-");
			printf("%d", minfo.ports[++i]);
		}
	}
	printf(" ");
}


static void print_mode(u_int16_t mode)
{
	printf("--mode:");
	if( mode == MODE_DNAT)
		printf("dnat ");
	else if( mode == MODE_FORWARD_IN)
		printf("forward_in ");
	else if( mode == MODE_FORWARD_OUT)
		printf("forward_out ");
}

/* Prints out the targinfo. */
static void
print(const void *ip,
      const struct xt_entry_target *target,
      int numeric)
{
	struct xt_porttrigger_info *info = (struct xt_porttrigger_info *)(target)->data;

	print_mode(info->mode);

	/* 
	  Modification: for protocol type==all(any) can't work
      Modified by: ken_chiang 
      Date:2007/8/21
    */
	//if( info->trigger_proto ) {
		printf("--trigger-proto:");
		print_proto(info->trigger_proto);
	//}

	//if( info->forward_proto ) {
		printf("--forward-proto:");
		print_proto(info->forward_proto);
	//}

	if( info->trigger_ports.ports[0]) {
		printf("--trigger-ports: ");
		print_multi_ports(info->trigger_ports);
	}

	if( info->forward_ports.ports[0] ) {
		printf("  --forward-ports: ");
		print_multi_ports(info->forward_ports);
	}
	
	if( info->timer >0) {
		printf("  --timer: %d", info->timer);
	}
	
}

/* Saves the union ipt_targinfo in parsable form to stdout. */
static void
save(const void *ip, const struct xt_entry_target *target)
{
	struct xt_porttrigger_info *info = (struct xt_porttrigger_info *)(target)->data;

	print_mode(info->mode);
	/* 
	  Modification: for protocol type==all(any) can't work
      Modified by: ken_chiang 
      Date:2007/8/21
    */
	//if( info->trigger_proto ) {
		printf("--trigger-proto:");
		print_proto(info->trigger_proto);
	//}

	//if( info->forward_proto ) {
		printf("--forward-proto:");
		print_proto(info->forward_proto);
	//}

	if( info->trigger_ports.ports[0]) {
		printf("--trigger-ports: ");
		print_multi_ports(info->trigger_ports);
	}

	if( info->forward_ports.ports[0] ) {
		printf("  --forward-ports: ");
		print_multi_ports(info->forward_ports);
	}

	if( info->timer >0) {
		printf("  --timer: %d", info->timer);
	}
}

static struct xtables_target porttrigger = { 
	.next		= NULL,
	.name		= "PORTTRIGGER",
	.version	= XTABLES_VERSION,
	.size		= XT_ALIGN(sizeof(struct xt_porttrigger_info)),
	.userspacesize	= XT_ALIGN(sizeof(struct xt_porttrigger_info)),
	.help		= &help,
	.init		= &init,
	.parse		= &parse,
	.final_check	= &final_check,
	.print		= &print,
	.save		= &save,
	.extra_opts	= opts
};

void _init(void)
{
	xtables_register_target(&porttrigger);
}
