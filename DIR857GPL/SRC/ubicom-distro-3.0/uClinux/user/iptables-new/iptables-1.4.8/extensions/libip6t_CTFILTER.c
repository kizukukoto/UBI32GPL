#include <stdio.h>
#include <netdb.h>
#include <string.h>
#include <stdlib.h>
#include <getopt.h>
#include <xtables.h>
#include <linux/netfilter_ipv6/ip6t_CTFILTER.h>
#include <iptables/internal.h>

/* Function which prints out usage message. */
static void
help(void)
{
    printf(
	"CTFILTER v%s options:\n"
	" --proto-type \n"
	"    %d -- UDP\n"
	"    %d -- TCP\n"
	"    %d -- ICMPV6\n"
	" --type \n"
	"    %d -- Endpoint-Independent Filter\n"
	"    %d -- Address-Dependent Filter\n",
	IPTABLES_VERSION, 
	IP6T_UDP, IP6T_TCP, IP6T_ICMPV6, 
	IP6T_ENDPOINT_INDEPENDENT, IP6T_ADDRESS_DEPENDENT);
}

static char *get_proto_type(u_int32_t type)
{
	switch (type)
	{
		case IP6T_UDP:
			return "UDP";
		case IP6T_TCP:
			return "TCP";
		case IP6T_ICMPV6:
			return "ICMPV6";
		default:
			return "Unknow";
	}
}

static char *get_type(u_int32_t type)
{
	switch (type)
	{
		case IP6T_ENDPOINT_INDEPENDENT:
			return "ENDPOINT_INDEPENDENT";
		case IP6T_ADDRESS_DEPENDENT:
			return "ADDRESS_DEPENDENT";
		default:
			return "Unknow";
	}
}

static struct option opts[] = {
    { "proto-type", 1, NULL, '1' },
    { "type", 1, NULL, '2' },
    { 0 }
};

/* Function which parses command options; returns true if it
   ate an option */
static int
parse(int c, char **argv, int invert, unsigned int *flags,
      const void *entry,
      struct xt_entry_target **target)
{
	struct ip6t_ctfilter_info *info = (struct ip6t_ctfilter_info *)(*target)->data;

	switch (c) {
		case '1':
			if (atoi(optarg) == IP6T_UDP)
				info->proto_type = IP6T_UDP;
			else if (atoi(optarg) == IP6T_TCP)
				info->proto_type = IP6T_TCP;
			else if (atoi(optarg) == IP6T_ICMPV6)
				info->proto_type = IP6T_ICMPV6;
			else
				return 0;
			printf("iptables: ctfilter proto type: %d\n", info->proto_type);
			return 1;
		case '2':
			if (atoi(optarg) == IP6T_ENDPOINT_INDEPENDENT)
				info->type = IP6T_ENDPOINT_INDEPENDENT;
			else if (atoi(optarg) == IP6T_ADDRESS_DEPENDENT)
				info->type = IP6T_ADDRESS_DEPENDENT;
			else
				return 0;
			printf("iptables: ctfilter type: %d\n", info->type);
			return 1;
		default:
			return 0;
	}
}

/* Final check; don't care. */
static void final_check(unsigned int flags)
{
}

/* Prints out the targinfo. */
static void
print(const void *ip,
      const struct xt_entry_target *target,
      int numeric)
{
	struct ip6t_ctfilter_info *info = (struct ip6t_ctfilter_info *)(target)->data;
	printf("--proto-type: %s ", get_proto_type(info->proto_type));
	printf("--type: %s", get_type(info->type));
}

/* Saves the union ipt_targinfo in parsable form to stdout. */
static void
save(const void *ip, 
     const struct xt_entry_target *target)
{
	struct ip6t_ctfilter_info *info = (struct ip6t_ctfilter_info *)(target)->data;
	printf("--proto-type: %d ", info->proto_type);
	printf("--type: %d", info->type);
}

static struct xtables_target ctfilter = { 
	.name          = "CTFILTER",
	.version       = XTABLES_VERSION,
	.family        = NFPROTO_IPV6,
	.size          = XT_ALIGN(sizeof(struct ip6t_ctfilter_info)),
	.userspacesize = XT_ALIGN(sizeof(struct ip6t_ctfilter_info)),
	.help          = &help,
	.parse         = &parse,
	.final_check   = &final_check,
	.print         = &print,
	.save          = &save,
	.extra_opts    = opts
};

void _init(void)
{
    xtables_register_target(&ctfilter);
}
