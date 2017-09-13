#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include "iwinf.h"

char *
get_iw_ssid(const char *iwname, char *ssid)
{
	int skfd;
	struct iwreq wrq;
	
	skfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (skfd < 0 )
		return NULL;
		
	bzero(&wrq, sizeof(struct iwreq));
	wrq.u.essid.pointer = (caddr_t)ssid;
	wrq.u.essid.length = IW_ESSID_MAX_SIZE + 1;
	wrq.u.essid.flags = 0;
	
	strcpy(wrq.ifr_name, iwname);
	ioctl(skfd, SIOCGIWESSID, &wrq);
	close(skfd);
	return ssid;
}

char *
get_iw_haddr(const char *iwname, char *haddr)
{
	int skfd;
	struct iwreq wrq;
	char *mac;
	
	skfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (skfd < 0 )
		return NULL;
		
	bzero(&wrq, sizeof(struct iwreq));
	strcpy(wrq.ifr_name, iwname);
	if (ioctl(skfd, SIOCGIWAP, &wrq) == -1) {
		perror("Get haddr");
		close(skfd);
		return NULL;
	}
	mac = wrq.u.ap_addr.sa_data;
	sprintf(haddr, "%02x:%02x:%02x:%02x:%02x:%02x",
		mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
	close(skfd);
	return haddr;
}

double
get_iw_freq(const char *iwname, char *freq)
{
	int skfd;
	struct iwreq wrq;
	int channel;
	double res = 0;

	skfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (skfd < 0 )
		return res;
		
	bzero(&wrq, sizeof(struct iwreq));

	strcpy(wrq.ifr_name, iwname);
	if (ioctl(skfd, SIOCGIWFREQ, &wrq) >= 0) {
		int i;
		res = (double) wrq.u.freq.m;
		res /= 1000000000;
		for (i = 0; i < wrq.u.freq.e; i++)
			res *= 10;
		
		/*
		printf("Get Freq OK:\n");
		printf("in->m:%d<br>\n",wrq.u.freq.m);
		printf("in->e:%d<br>\n",wrq.u.freq.e);
		printf("in->i:%d<br>\n",wrq.u.freq.i);
		printf("in->flags:%d<br>\n",wrq.u.freq.flags);
		*/
		
		channel = (((res*1000)-2412)/5)+1;

		sprintf(freq, " %d", channel);
	}
	close(skfd);
	return res;
}

/*
int
main(int argc, char *argv[])
{
	char buf[32];
	
	bzero(buf, sizeof(buf));
	get_iw_ssid(argv[1], buf);
	printf("%s:ssid:%s\n", argv[1], buf);
	
	bzero(buf, sizeof(buf));
	get_iw_haddr(argv[1], buf);
	printf("%s:Mac:%s\n", argv[1], buf);
}
*/
