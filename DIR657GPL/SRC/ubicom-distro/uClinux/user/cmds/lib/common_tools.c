#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <netinet/in.h>
#include <arpa/inet.h>

struct routing_rule {
	char iface[8];
	char dst[16];
	char gw[16];
	char flags[8];
	char refcnt[8];
	char use[8];
	char metric[8];
	char mask[16];
	char mtu[16];
	char window[16];
	char irtt[16];
};

static int _pow(int base, int cnt)
{
	int ans = 1;
	for(; cnt > 0; cnt--)
		ans *= base;

	return ans;
}

static int hextodec(const char *hex, int len)
{
	int ans = 0;
	int i = 0;

	for (; i < len; i++) {
		if (hex[len - i - 1] >= 'A')
			ans += (15 - ('F' - hex[len - i - 1])) * _pow(16, i);
		else
			ans += (hex[len - i - 1] - '0') * _pow(16, i);
	}

	return ans;
}

int getGatewayInterfaceByIP(const char *ip, char *dev, char *gw)
{
	char ip1[4], ip2[4], ip3[4], ip4[4];
	struct routing_rule rr;
	FILE *fp = fopen("/proc/net/route", "r");

	if (fp == NULL) {
		printf("err: can not open /proc/net/route\n");
		return -1;
	}

	sscanf(ip, "%[^.].%[^.].%[^.].%s", ip1, ip2, ip3, ip4);
	bzero(&rr, sizeof(struct routing_rule));
	while (fscanf(fp, "%s %s %s %s %s %s %s %s %s %s %s",
		rr.iface, rr.dst, rr.gw, rr.flags, rr.refcnt,
		rr.use, rr.metric, rr.mask, rr.mtu, rr.window, rr.irtt) != EOF) {
		if (strcmp(rr.iface, "Iface") == 0) continue;

		char ip_mask[16], dst_mask[16];

		sprintf(ip_mask, "%d.%d.%d.%d", atoi(ip1) & hextodec(rr.mask + 6, 2),
					atoi(ip2) & hextodec(rr.mask + 4, 2),
					atoi(ip3) & hextodec(rr.mask + 2, 2),
					atoi(ip4) & hextodec(rr.mask, 2));

		sprintf(dst_mask, "%d.%d.%d.%d", hextodec(rr.dst + 6, 2) & hextodec(rr.mask + 6, 2),
					hextodec(rr.dst + 4, 2) & hextodec(rr.mask + 4, 2),
					hextodec(rr.dst + 2, 2) & hextodec(rr.mask + 2, 2),
					hextodec(rr.dst, 2) & hextodec(rr.mask, 2));

//		printf("%s %s %s\n", rr.iface, ip_mask, dst_mask);
		if (strcmp(ip_mask, dst_mask) == 0) {
			strcpy(dev, rr.iface);

#ifdef CONFIG_UBICOM_ARCH
			sprintf(gw, "%d.%d.%d.%d", hextodec(rr.gw, 2),
							hextodec(rr.gw + 2, 2),
							hextodec(rr.gw + 4, 2),
							hextodec(rr.gw + 6, 2));
#else
			sprintf(gw, "%d.%d.%d.%d", hextodec(rr.gw + 6, 2),
							hextodec(rr.gw + 4, 2),
							hextodec(rr.gw + 2, 2),
							hextodec(rr.gw, 2));
#endif
//			printf("%s\n", rr.iface);
			break;
		}
	}

	fclose(fp);
	return -1;
}

void getIPbyDevice(const char *dev, char *ip)
{
	int inet_sock = socket(AF_INET, SOCK_DGRAM, 0);
	struct ifreq ifr;

	strcpy(ifr.ifr_name, dev);
	if (ioctl(inet_sock, SIOCGIFADDR, &ifr) < 0) {
		perror("ioctl");
		return;
	}

	strcpy(ip, inet_ntoa(((struct sockaddr_in*)&(ifr.ifr_addr))->sin_addr));
}

int getStrtok(char *input, char *token, char *fmt, ...)
{
        va_list  ap;
        int arg, count = 0;
        char *c, *tmp;

        if (!input)
		return 0;
        tmp = input;
        for(count=0; tmp=strpbrk(tmp, token); tmp+=1, count++);
        count +=1;

        va_start(ap, fmt);
        for (arg = 0, c = fmt; c && *c && arg < count;) {
		if (*c++ != '%')
			continue;
	switch (*c) {
	        case 'd':
			if(!arg)
				*(va_arg(ap, int *)) = atoi(strtok(input, token));
			else
				*(va_arg(ap, int *)) = atoi(strtok(NULL, token));
			break;
	        case 's':
			if(!arg) {
			        *(va_arg(ap, char **)) = strtok(input, token);
			} else
			        *(va_arg(ap, char **)) = strtok(NULL, token);
			break;
		}
		arg++;
        }
        va_end(ap);

        return arg;
}

/* base64_encode code from http://tech.cuit.edu.cn/forum/thread-188-1-1.html */
int base64_encode(char *OrgString, char *Base64String, int OrgStringLen)
{
	static char Base64Encode[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/=";
	int Base64StringLen = 0;

	while( OrgStringLen > 0 ) {
		*Base64String ++ = Base64Encode[(OrgString[0] >> 2 ) & 0x3f];
		if( OrgStringLen > 2 ) {
			*Base64String ++ = Base64Encode[((OrgString[0] & 3) << 4) | (OrgString[1] >> 4)];
			*Base64String ++ = Base64Encode[((OrgString[1] & 0xF) << 2) | (OrgString[2] >> 6)];
			*Base64String ++ = Base64Encode[OrgString[2] & 0x3F];
		} else {
			switch( OrgStringLen ) {
			case 1:
				*Base64String ++ = Base64Encode[(OrgString[0] & 3) << 4 ];
				*Base64String ++ = '=';
				*Base64String ++ = '=';
				break;
			case 2:
				*Base64String ++ = Base64Encode[((OrgString[0] & 3) << 4) | (OrgString[1] >> 4)];
				*Base64String ++ = Base64Encode[((OrgString[1] & 0x0F) << 2) | (OrgString[2] >> 6)];
				*Base64String ++ = '=';
				break;
			}
		}

		OrgString +=3;
		OrgStringLen -=3;
		Base64StringLen +=4;
	}

	*Base64String = 0;
	return Base64StringLen;
}

