/*
 * File:         fdpichdr.c
 * Author:       Mike Frysinger <michael.frysinger@analog.com>
 * Description:  Tweak ELF header on the fly
 * Modified:     Copyright 2006-2008 Analog Devices Inc.
 * Bugs:         Enter bugs at http://blackfin.uclinux.org/
 * Licensed under the GPL-2, see the file COPYING in this dir
 */

#include "headers.h"
#include "helpers.h"
#include "elf.h"

static const char *rcsid = "$Id$";
const char *argv0;
int force = 0, verbose = 0, quiet = 0;

static void show_version(void)
{
	printf("fdpichdr-%s: %s compiled %s\n%s\n",
	       VERSION, __FILE__, __DATE__, rcsid);
	exit(EXIT_SUCCESS);
}

struct option_help {
	const char *desc, *opts;
};

static void show_some_usage(struct option const opts[], struct option_help const help[], const char *flags, int exit_status)
{
	unsigned long i;

	printf("Usage: fdpichdr [options] <elfs>\n\n");
	printf("Options: -[%s]\n", flags);
	for (i=0; opts[i].name; ++i)
		printf("  -%c, --%-10s %-14s * %s\n",
		       opts[i].val, opts[i].name,
		       (help[i].opts != NULL ? help[i].opts :
		          (opts[i].has_arg == no_argument ? "" : "<arg>")),
		       help[i].desc);

	exit(exit_status);
}

#define PARSE_FLAGS "s:vqhV"
#define a_argument required_argument
static struct option const long_opts[] = {
	{"stack-size",    a_argument, NULL, 's'},
	{"verbose",      no_argument, NULL, 'v'},
	{"quiet",        no_argument, NULL, 'q'},
	{"help",         no_argument, NULL, 'h'},
	{"version",      no_argument, NULL, 'V'},
	{NULL,           no_argument, NULL, 0x0}
};
static struct option_help const opts_help[] = {
	{"Set the stack size",            "<size>"},
	{"Make a lot of noise",           NULL},
	{"Only show errors",              NULL},
	{"Print this help and exit",      NULL},
	{"Print version and exit",        NULL},
	{NULL,NULL}
};
#define show_usage(status) show_some_usage(long_opts, opts_help, PARSE_FLAGS, status)

int main(int argc, char *argv[])
{
	int i;
	uint32_t stack;

	argv0 = strchr(argv[0], '/');
	argv0 = (argv0 == NULL ? argv[0] : argv0+1);
	stack = 0;

	while ((i=getopt_long(argc, argv, PARSE_FLAGS, long_opts, NULL)) != -1) {
		switch (i) {
			case 's': stack = parse_int_hex(optarg); break;
			case 'v': ++verbose; break;
			case 'q': ++quiet; break;
			case 'h': show_usage(0);
			case 'V': show_version();
			case ':': err("Option '%c' is missing parameter", optopt);
			case '?': err("Unknown option '%c' or argument missing", optopt);
			default:  err("Unhandled option '%c'; please report this", i);
		}
	}
	argc -= optind;
	argv += optind;

	if (stack)
		return set_stack(stack, argv);

	show_usage(EXIT_FAILURE);

	return 0;
}
