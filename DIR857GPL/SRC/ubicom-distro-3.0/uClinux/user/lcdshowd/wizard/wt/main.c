#include <stdio.h>


static int verbose = 1;
#define vmsg(fmt, a...)	do {if(verbose)fprintf(stderr, fmt, ##a);}while(0)

int main(int argc, char *argv[])
{
	int rev;
	char *eth = "eth0";
	
	if (argc >= 2)
		eth = argv[1];

	if (is_valid_address(eth))
		return 1;	/* have ip, guess get by dhcp */
	vmsg("detect dhcp\n");
	if (dhcpcd_detect(eth) == 1)
		return 2;	/* dhcpd */
	vmsg("detect pppoe:%d\n", argc);
	if (pppoed(eth) == 1)
		return 3;	/* pppoe */

	return 0;		/* deteted nothing */
}
