/*
 * ubicom_wish.c	Ubicom Inc. Wireless Intelligent Stream Handling - WISH
 *
 *		This program is free software; you can redistribute it and/or
 *		modify it under the terms of the GNU General Public License
 *		as published by the Free Software Foundation; either version
 *		2 of the License, or (at your option) any later version.
 *
 * Authors:	Gareth Williams, <gareth.williams@ubicom.com>
 *
 * This file supports the Cameo DIR655 RC program - it is a backup to the file that should be contained in and built as part of uClinux/user/rc program.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <ctype.h>

#include <sutil.h>
#include <nvram.h>
#include <shvar.h>
#include <rc.h>

//#define RC_DEBUG 1
#ifdef RC_DEBUG
#define DEBUG_MSG(fmt, arg...)       printf("%s [%d]: "fmt, __FILE__, __LINE__, ##arg)
#else
#define DEBUG_MSG(fmt, arg...)
#endif

#define UBICOM_WISH_RULES 24

struct wish_rule_entry {
	int enable;
	char *enable_str;
	char *name;
	char *prio_str;
	int prio;
	char *uplink;
	char *downlink;
	char *proto;
	char *src_ip_start;
	char *src_ip_end;
	char *src_port_start;
	char *src_port_end;
	char *dst_ip_start;
	char *dst_ip_end;
	char *dst_port_start;
	char *dst_port_end;
};


extern struct action_flag action_flags;

/*
 * ubicom_wish_enabled()
 */
int ubicom_wish_enabled(void)
{
	/*
	 * WISH is enabled when when its own setting is enabled in addition to wireless being enabled
	 */

	return (nvram_match("wish_engine_enabled", "1") == 0) && (nvram_match("wlan0_enable", "1") == 0);
}

/*
 * ubicom_wish_media_enabled()
 */
int ubicom_wish_media_enabled(void)
{
	return nvram_match("wish_media_enabled", "1") == 0;
}

/*
 * ubicom_wish_http_enabled()
 */
int ubicom_wish_http_enabled(void)
{
	return nvram_match("wish_http_enabled", "1") == 0;
}

/*
 * ubicom_wish_write_manual_rules()
 */
static int ubicom_wish_write_manual_rules(void)
{
	int i;
	char rule_buffer[256];
	char wish_rule_name[] = "wish_rule_XXXX";
	
	for(i = 0; i < UBICOM_WISH_RULES; i++)
	{
		struct wish_rule_entry temp_rule;
		char *rule;

		/*
		 * Read in the rule
		 */
		snprintf(wish_rule_name, sizeof(wish_rule_name), "wish_rule_%02d", i) ;
		rule = nvram_get(wish_rule_name);
		if ((rule == NULL) || (strlen(rule) == 0)) {
			DEBUG_MSG("\tError reading WISH rule %s\n", wish_rule_name);
			continue;
		}
		strcpy(rule_buffer, rule);
		if (getStrtok(rule_buffer, "/", "%s %s %s %s %s %s %s %s %s %s %s %s %s %s",
				&temp_rule.enable_str, &temp_rule.name, &temp_rule.prio_str, &temp_rule.uplink, &temp_rule.downlink, &temp_rule.proto, &temp_rule.src_ip_start, &temp_rule.src_ip_end,
				&temp_rule.src_port_start, &temp_rule.src_port_end, &temp_rule.dst_ip_start, &temp_rule.dst_ip_end, &temp_rule.dst_port_start, &temp_rule.dst_port_end) != 14) {
			DEBUG_MSG("\tParse error WISH rule %s\n", wish_rule_name);
			continue;
		}

		/*
		 * Copy priority and enable flags to our rule buffer
		 */
		temp_rule.enable = atoi(temp_rule.enable_str);
		if (!temp_rule.enable)  {
			DEBUG_MSG("\tSet WISH rule %s not enabled\n", wish_rule_name);
			continue;
		}

		DEBUG_MSG("\tSet WISH rule %s enabled: %s\n", wish_rule_name, rule);
	
		/*
		 * Add this rule to wish
		 */
		write2file("/sys/devices/system/ubicom_wish/ubicom_wish0/add_manual_rule", rule);

	}

	/*
	 * If media centre support is enabled then we create rules for that.
	 * Rules are formed as:
	 * <unused>/<unused>/<prio>/<unused>/<unused>/<proto>/<h1 ip from>/<h1 ip to>/<h1 port from>/<h1 port to>/<h2 ip from>/<h2 ip to>/<h2 port from>/<h2 port to>
	 */
	if (ubicom_wish_media_enabled()) {
		DEBUG_MSG("\tWISH create media centre rules\n");
		write2file("/sys/devices/system/ubicom_wish/ubicom_wish0/add_manual_rule", "1/0/4/0/0/17/0.0.0.0/255.255.255.255/5555/5555/0.0.0.0/255.255.255.255/0/65535");
		write2file("/sys/devices/system/ubicom_wish/ubicom_wish0/add_manual_rule", "1/0/4/0/0/17/0.0.0.0/255.255.255.255/3390/3390/0.0.0.0/255.255.255.255/0/65535");
		write2file("/sys/devices/system/ubicom_wish/ubicom_wish0/add_manual_rule", "1/0/4/0/0/17/0.0.0.0/255.255.255.255/0/65535/0.0.0.0/255.255.255.255/5555/5555");
		write2file("/sys/devices/system/ubicom_wish/ubicom_wish0/add_manual_rule", "1/0/4/0/0/17/0.0.0.0/255.255.255.255/0/65535/0.0.0.0/255.255.255.255/3390/3390");

		write2file("/sys/devices/system/ubicom_wish/ubicom_wish0/add_manual_rule", "1/0/4/0/0/6/0.0.0.0/255.255.255.255/5555/5555/0.0.0.0/255.255.255.255/0/65535");
		write2file("/sys/devices/system/ubicom_wish/ubicom_wish0/add_manual_rule", "1/0/4/0/0/6/0.0.0.0/255.255.255.255/3390/3390/0.0.0.0/255.255.255.255/0/65535");
		write2file("/sys/devices/system/ubicom_wish/ubicom_wish0/add_manual_rule", "1/0/4/0/0/6/0.0.0.0/255.255.255.255/0/65535/0.0.0.0/255.255.255.255/5555/5555");
		write2file("/sys/devices/system/ubicom_wish/ubicom_wish0/add_manual_rule", "1/0/4/0/0/6/0.0.0.0/255.255.255.255/0/65535/0.0.0.0/255.255.255.255/3390/3390");
	}

	return 0;
}

/*
 * ubicom_wish_start()
 * NOTE: This function does not create iptables rules for WISH - those are managed by firewall.c
 * This is not ideal but firewall.c makes the assumption that it controls all iptables rules and flushes the lot before establishing its own.
 * This ends up destroying wish rules!!
 */
void ubicom_wish_start(void)
{
	if(!(action_flags.all || action_flags.wlan)) {
		return;
	}

	DEBUG_MSG("WISH start\n");

	if (!ubicom_wish_enabled()) {
		DEBUG_MSG("WISH not enabled\n");
		_system("echo 0 > /proc/br_nf_wish");
		return;
	}

	_system("echo 0 > /proc/br_nf_wish");
	_system("insmod /lib/modules/nf_ubicom_wish.ko");
	if (!wait4file("/sys/devices/system/ubicom_wish/ubicom_wish0/wish_terminate", 1, 2)) {
		DEBUG_MSG("WISH failed to start\n");
		return;
	}

	/*
	 * Configure wish functions
	 */
	if (ubicom_wish_http_enabled()) {
		write2file("/sys/devices/system/ubicom_wish/ubicom_wish0/http_classifier", "1\n");
	} else {
		write2file("/sys/devices/system/ubicom_wish/ubicom_wish0/http_classifier", "0\n");
	}

	/*
	 * Configure eth name - WISH will only operate on packets from/to this actual bridge port
	 */
#ifdef IP8000
	write2file("/sys/devices/system/ubicom_wish/ubicom_wish0/if_name", "ath");
#else
	write2file("/sys/devices/system/ubicom_wish/ubicom_wish0/if_name", nvram_safe_get("wlan0_eth"));
#endif
	/*
	 * Set manual rules
	 */
	ubicom_wish_write_manual_rules();

	/*
	 * Mount state file
	 */
	_system("/etc/init.d/wish_mount_state");
}

/*
 * ubicom_wish_stop()
 */
void ubicom_wish_stop(void)
{
	if (!(action_flags.all || action_flags.wlan)) {
		return;
	}

	DEBUG_MSG("WISH stop\n");

	/*
	 * Unmount wish state
	 */
	_system("/etc/init.d/wish_unmount_state");

	_system("echo 0 > /proc/br_nf_wish");
	/*
	 * If not running then nothing to do
	 */
	if (!wait4file("/sys/devices/system/ubicom_wish/ubicom_wish0/wish_terminate", 1, 1)) {
		DEBUG_MSG("WISH not running\n");
		return;
	}
	
        //system("/etc/init.d/stop_wish &");
        //sleep(2);

	/*
	 * Tell wish to stop
	 */
	write2file("/sys/devices/system/ubicom_wish/ubicom_wish0/wish_terminate", "1\n");

	/*
	 * Wait for termination
	 */
	wait4file("/sys/devices/system/ubicom_wish/ubicom_wish0/wish_terminate", 0, 2);

	/*
	 * Unload the kernel module
	 */
	_system("rmmod nf_ubicom_wish");
}


