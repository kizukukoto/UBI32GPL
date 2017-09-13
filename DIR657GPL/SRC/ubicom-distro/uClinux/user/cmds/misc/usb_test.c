#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <asm/types.h>

#define USB_PORTSC1	0x68000030
#define USB_PORTSC2	0x69000030

static int usb_test_port(__u32 sc)
{
	int rev;
	__u32 regs;

	if (reg_read(sc, 0x4, &regs) == -1)
		goto out;

	rev = (regs & 0x1) ? 1: 0;
out:
	return rev;
}

int usb_test_main(int arg, char *argv[])
{
	__u32 regs;
	int port, rev = -1;
	__u32 usb_portsc[] = {0x68000030, 0x69000030};  /* USB1 and USB2 */
#if 0
	if (reg_read(USB_PORTSC1, 0x4, &regs) == -1)
		goto out;
	printf("USB1: %s\n", regs & 0x1 ? "present" : "nopresent");
	if (reg_read(USB_PORTSC2, 0x4, &regs) == -1)
		goto out;
	printf("USB2: %s\n", regs & 0x1 ? "present" : "nopresent");
#endif
	if (arg != 2)
		goto out;

	if ((port = atoi(argv[1])) > 2 || port < 1)
		goto out;

	printf("%d", usb_test_port(usb_portsc[port - 1]));
out:
	return rev;
}
