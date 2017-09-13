/* vi: set sw=4 ts=4: */
/*
 * Licensed under the GPL v2 or later, see the file LICENSE in this tarball.
 */
#include <getopt.h>

#include "common.h"
#include "dhcpd.h"

int dumpleases_main(int argc, char **argv);
int dumpleases_main(int argc, char **argv)
{
	int fd;
	int i;
	unsigned opt, total_time;
	time_t expires, now_time;
	const char *file = LEASES_FILE;
	struct dhcpOfferedAddr lease;
	struct in_addr addr;
	char mac[32], ip[16];

	enum {
		OPT_a	= 0x1,	// -a
		OPT_c	= 0x2,	// -c
		OPT_r	= 0x3,	// -r
		OPT_d	= 0x4,	// -d debug mode
		OPT_f	= 0x8,	// -f
	};
#if ENABLE_GETOPT_LONG
	static const struct option options[] = {
		{ "absolute", no_argument, 0, 'a' },
		{ "CAMEO type", no_argument, 0, 'c' },
		{ "remaining", no_argument, 0, 'r' },
		{ "file", required_argument, 0, 'f' },
		{ NULL, 0, 0, 0 }
	};

	applet_long_options = options;
#endif
	opt_complementary = "=0:?:a--rc:c--ar:r--ac";
	opt = getopt32(argc, argv, "acdrf:", &file);

	fd = xopen(file, O_RDONLY);

	if (!(opt & OPT_c)) { /* no -c */
		printf("Mac Address       IP-Address      Expires %s\n", (opt & OPT_a) ? "at" : "in");
		/*     "00:00:00:00:00:00 255.255.255.255 Wed Jun 30 21:49:08 1993" */
	}
	while (full_read(fd, &lease, sizeof(lease)) == sizeof(lease)) {

//		if ((opt && OPT_d)) {
//			printf("* %s *\n", lease.host);
//		}

		if ((opt & OPT_c)) { /* -c */
			sprintf(mac, "%02x:%02x:%02x:%02x:%02x:%02x",
								lease.chaddr[0], lease.chaddr[1], lease.chaddr[2],
								lease.chaddr[3], lease.chaddr[4], lease.chaddr[5]);
			addr.s_addr = lease.yiaddr;
			sprintf(ip,"%s", inet_ntoa(addr));
			time(&now_time);
			expires = ntohl(lease.expires);
			total_time = (unsigned)now_time + (unsigned)expires;
			printf("%d %s %s %s *\n", total_time, mac, ip,
				(lease.host == NULL || *(lease.host) == '\0')?ip:lease.host);
		} else {
			printf(":%02x"+1, lease.chaddr[0]);
			for (i = 1; i < 6; i++) {
				printf(":%02x", lease.chaddr[i]);
			}
			addr.s_addr = lease.yiaddr;
			printf(" %-15s ", inet_ntoa(addr));
			expires = ntohl(lease.expires);
			if (!(opt & OPT_a)) { /* no -a */
				if (!expires)
					puts("expired");
				else {
					unsigned d, h, m;
					d = expires / (24*60*60); expires %= (24*60*60);
					h = expires / (60*60); expires %= (60*60);
					m = expires / 60; expires %= 60;
					if (d) printf("%u days ", d);
					printf("%02u:%02u:%02u\n", h, m, (unsigned)expires);
				}
			} else /* -a */
				fputs(ctime(&expires), stdout);
		}
	}
	/* close(fd); */

	return 0;
}
