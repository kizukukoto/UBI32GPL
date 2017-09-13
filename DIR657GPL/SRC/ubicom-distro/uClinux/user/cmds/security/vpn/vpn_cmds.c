#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cmds.h"
#include "shutils.h"


extern int ipsec_main(int argc, char *argv[]);
extern int xl2tpd_main(int argc, char *argv[]);
extern int pptpd_main(int argc, char *argv[]);
extern int sslvpn_main(int argc, char *argv[]);

int vpn_subcmd(int argc, char *argv[])
{
	struct subcmd cmds[] = {
		{ "l2tpd", xl2tpd_main},
		{ "pptpd", pptpd_main},
		{ "ipsec", ipsec_main},
		{ "sslvpn", sslvpn_main},
		{ NULL, NULL}
	};
	return lookup_subcmd_then_callout(argc, argv, cmds);
}
