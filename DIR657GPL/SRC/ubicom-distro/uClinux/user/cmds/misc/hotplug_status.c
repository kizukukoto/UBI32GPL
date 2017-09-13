#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <asm/types.h>
#include "cmds.h"

#define USB_PORTSC1	0x68000030
#define USB_PORTSC2	0x69000030

#define REG_GMAC_BASE		0x60000000
#define REG_GMAC0_STATUS	(REG_GMAC_BASE + 0xA02C)
#define REG_GMAC1_STATUS	(REG_GMAC_BASE + 0xE02C)
#define GMAC_LINK_BIT_MASK	(1 << 0)

int net_phy_present(const char *ifname)
{
	__u32 regs = 0;
	int rev = -1;
	
	struct {
		char *ifname;
		__u32 reg;
	} *p, phy[] ={
		{ "eth0", REG_GMAC0_STATUS },
		{ "eth1", REG_GMAC1_STATUS },
		{ "eth2", REG_GMAC0_STATUS },
		/* XXX: virtual devices only map eth1 now for DIR-730... */
		{ "ppp0", REG_GMAC1_STATUS },
		{ "ppp1", REG_GMAC1_STATUS },
		{ "ppp2", REG_GMAC1_STATUS },
		{ "ppp3", REG_GMAC1_STATUS },
		{ NULL, 0x0}
	};
	for (p = phy; p->ifname != NULL; p++) {
		
		if (strcmp(p->ifname, ifname) != 0)
			continue;
		if (reg_read(p->reg, 0x4, &regs) == -1)
			break;
		return (regs & GMAC_LINK_BIT_MASK) ? 0 : 1;
	}
	err_msg("(%s): No such device!\n", ifname);
out:
	return rev;
}

static int net_main(int argc, char *argv[])
{
	int rev;
	char *str;
	if (argc < 2) {
		printf("usage: %s [ifname]\n", argv[0]);
		return -1;
	}
	if ((rev = net_phy_present(argv[1])) == 0)
		str = "present";
	else if (rev == 1)
		str = "not present";
	else
		str = "no such devices or error";
	printf("%s: %s\n", argv[1], str);
	
	return rev;
}

int hotplug_status_main(int argc, char *argv[])
{
	struct subcmd cmds[] = {
		{ "net", net_main, "link status of net(port1 only), \n"
			"return 0 as present, 1 as nopresent, -1 on error\n"},
		{ NULL, NULL}
	};
	return lookup_subcmd_then_callout(argc, argv, cmds);
}
