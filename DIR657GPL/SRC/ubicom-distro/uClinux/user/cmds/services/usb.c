#include <stdio.h>
#include <unistd.h>

#include "cmds.h"
#include "nvram.h"
#include "shutils.h"

/*
 * The usb spec is not very clear. Maybe have some bugs.
 */

static void usb_sp_start(const char *port1, const char *port2)
{
	char cmds[256];
	sprintf(cmds, 
               "insmod /lib/NetUSB.ko usbport1=%s usbport2=%s", 
               port1, port2);
	system("iptables -I INPUT -p tcp --dport 20005 -j ACCEPT");
	system("iptables -I INPUT -p udp --dport 9303 -j ACCEPT");
	system(cmds);
}

static void usb_sp_stop()
{
	system("rmmod /lib/NetUSB.ko");
	system("iptables -D INPUT -p tcp --dport 20005 -j ACCEPT");
	system("iptables -D INPUT -p udp --dport 9303 -j ACCEPT");
}

static void usb_wcn_start()
{
	system("insmod /lib/modules/2.6.15/kernel/drivers/usb/serial/usbserial.ko");
	system("insmod /lib/modules/2.6.15/kernel/drivers/usb/serial/option.ko");
	system("insmod /lib/modules/2.6.15/kernel/drivers/usb/storage/usb-storage.ko");
}

static void usb_wcn_stop()
{
	system("rmmod /lib/modules/2.6.15/kernel/drivers/usb/serial/usbserial.ko");
	system("rmmod /lib/modules/2.6.15/kernel/drivers/usb/serial/option.ko");
	system("rmmod /lib/modules/2.6.15/kernel/drivers/usb/storage/usb-storage.ko");	
}

static int usb_start(int argc, char *argv[])
{
	char usb1[20], usb2[20];
	int port1, port2;
	
	cprintf("++++ USB start ++++\n");
	
	sprintf(usb1, "%s", nvram_safe_get("usb_type_1"));	
	sprintf(usb2, "%s", nvram_safe_get("usb_type_2"));
	
	if((strcmp(usb1, "0") == 0) && (strcmp(usb2, "0") == 0) ){
		usb_sp_start("1", "1");
	}else if((strcmp(usb1, "0") == 0) && (strcmp(usb2, "0") != 0)){
		usb_sp_start("1", "0");
		usb_wcn_start();
	}else if((strcmp(usb1, "0") != 0) && (strcmp(usb2, "0") == 0)){
		usb_sp_start("0", "1");
		usb_wcn_start();
	}else{
		usb_wcn_start();
	}
 	
	cprintf("++++ USB Exit ++++\n");
	return 0;
}

static int usb_stop(int argc, char *argv[])
{
	cprintf("++++ USB Stop ++++\n");
	usb_wcn_stop();
	usb_sp_stop();
	/*if (strcmp(nvram_safe_get("usb_type_1"), "0") == 0) {
		cprintf("Stop KCODE\n");
		usb_sp_stop();
	}else{
		cprintf("Stop Stroage\n");
		usb_wcn_stop();
	}*/
	cprintf("++++ USB Exit ++++\n");
	return 0;
}

int usbd_main(int argc, char *argv[])
{
	struct subcmd cmds[] = {
		{ "start", usb_start},
		{ "stop", usb_stop},
		{ NULL, NULL}
	};

	return lookup_subcmd_then_callout(argc, argv, cmds);
}
