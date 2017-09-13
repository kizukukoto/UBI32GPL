#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include "cmds.h"
#include <syslog.h>
#include <signal.h>
#include "nvram.h"
#include "shutils.h"
#include "interface.h"
#include "proto.h"
#include "convert.h"
#include "libcameo.h"
#define USB3G_CHAT "/tmp/ppp/peers/3g-connect-chat"
#define USB3G_OPTIONS "/tmp/ppp/peers/3g.options"
void loop_until_usb3g() {
	/*
	while (1) {
		if (access("/tmp/usb3g.add", F_OK) != 0) {
			sleep(5);
			printf("wait usb3g\n");
			continue;
		}
		break;
	}
	*/
	sleep(15);
	system("pppd call 3g.options &");
}
int usb3g_start(const char *alias, const char *dev, const char *phy)
{
	FILE *opt_fd, *chat_fd;
	int rev = -1, idx, unit;
	struct proto_usb3g usb3g;
	extern int manual_start;
	
	trace("XXXX");
	if ((chat_fd = fopen(USB3G_CHAT, "w")) == NULL)
		goto out;
	trace("XXXX");
	if ((opt_fd = fopen(USB3G_OPTIONS, "w")) == NULL) {
		fclose(chat_fd);
		goto out;
	}
	trace("XXXX");
	if ((idx = nvram_find_index("usb3g_alias", alias)) == -1)
		goto out;
	
	trace("XXXX");
	if (init_proto_usb3g((struct proto_base *)&usb3g, sizeof(usb3g), idx) == -1)
		goto out;
	
	trace("ppp dev:%s", dev);
	unit = atoi(dev + 3);
	dump_proto_struct(PROTO_USB3G, (struct proto_base *)&usb3g, stdout);
	
	fprintf(chat_fd, "'' AT\n"
		"OK ATZ\n"
		"OK 'AT+CGDCONT=1,\"IP\",\"%s\"'\n"
		"OK ATD%s\n"
		"CONNECT ''\n",
		usb3g.apn_name, usb3g.dial_num
	);

	fprintf(opt_fd,
		"-detach\n"
		"ipcp-max-failure 30\n"
		"lcp-echo-failure 0\n"
		"/dev/ttyUSB0\n"
		"115200\n"
		"debug\n"
		"defaultroute\n"
		"usepeerdns\n"
		"ipcp-accept-local\n"
		"ipcp-accept-remote\n"
		"user %s\n"
		"password %s\n"
		"unit %d\n"
		"crtscts\n"
		"lock\n"
		"connect 'chat -v -t6 -f %s'\n"
		"ipparam usb3g\n",
		usb3g.user, usb3g.pass, unit, USB3G_CHAT);
	switch (*(usb3g.dial)) {
	case '0':
		break;
	case '1':
		printf("XXXXXXXXXXXXXXXXXXXX ALWAYS\n");
		fprintf(opt_fd, "persist\n");
		break;
	case '2':
		printf("XXXXXXXXXXXXXXXXXXXX AUTO\n");
		fprintf(opt_fd, "active-filter \"outbound and tcp "
				"port 80 or udp port 53 or icmp\"\n");
		fprintf(opt_fd, "idle %s\n"
			"demand\n"
			"persist\n", usb3g.idle);
		break;
	default:
		break;
	}
	
	fclose(chat_fd);
	fclose(opt_fd);
	
	if (*((struct proto_ppp *)&usb3g)->dial == '0' &&
		manual_start == 0) {
		trace("Manual mode not start!");
		return 0;
	}
	loop_until_usb3g();
	rev = 0;
out:
	rd(rev, "error");
	return rev;
}
int usb3g_start_main(int argc, char *argv[])
{
	int rev = -1;
	char *alias, *dev, *phy;
	

	if (argc < 4){
		goto out;
	}	
	args(argc - 1, argv + 1, &alias, &dev, &phy);
	if (strncmp(dev, "ppp", 3) != 0)
		goto out;

	// set clone mac and mtu
	clone_mac("usb3g", alias, argv[3]);	// argv[3]: phy
	/* only ethX in static/dhcpc need set mtu */
	//set_mtu("usb3g", alias, argv[3]);

	rev = usb3g_start(alias, dev, phy);
out:
	rd(rev, "error");
	return rev;
}

int usb3g_stop_main(int argc, char *argv[])
{
	int rev = -1, i, ppp_pid;
	char *alias = NULL, *dev = NULL;

	if (argc < 4)
		goto out;
	args(argc - 1, argv + 1, &alias, &dev);
	
	if ((i = nvram_find_index("usb3g_alias", alias)) == -1)
		goto out;
	if ((ppp_pid = __ppp_get_pid(dev)) <= 0)
		goto out;
	kill(ppp_pid, SIGINT);
	eval("iptables", "-D", "FORWARD", "-o", argv[3], "-j", "ACCEPT");
	rev = 0;
out:
	return rev;
}

int usb3g_status_main(int argc, char *argv[])
{
	if (argc < 4)
		help_proto_args(stderr, 1);
	return proto_xxx_status2(PROTO_USB3G, argv[1], argv[2], argv[3]);
}

static int usb3g_showconfig_main(int argc, char *argv[])
{
	return proto_showconfig_main(PROTO_USB3G, argc, argv);
}
int usb3g_attach_main(int argc, char *argv[])
{
	return pppxxx_attach_main(PROTO_USB3G_STR, argc, argv);
}
int usb3g_detach_main(int argc, char *argv[])
{
	return pppxxx_detach_main(PROTO_USB3G_STR, argc, argv);
}
int usb3g_main(int argc, char *argv[])
{
	struct subcmd cmds[] = {
		{ "start", usb3g_start_main}, 
		{ "stop", usb3g_stop_main},   
		{ "status", usb3g_status_main}, 
		{ "showconfig", usb3g_showconfig_main},
		{ "attach", usb3g_attach_main}, 
		{ "detach", usb3g_detach_main}, 
		{ NULL, NULL}
	};
	return lookup_subcmd_then_callout(argc, argv, cmds);
}
