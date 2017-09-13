/* $Id: getifstats.c,v 1.1 2009/04/21 11:14:48 jimmy_huang Exp $ */
/* MiniUPnP project
 * http://miniupnp.free.fr/ or http://miniupnp.tuxfamily.org/
 * (c) 2006,2007 Thomas Bernard 
 * This software is subject to the conditions detailed
 * in the LICENCE file provided within the distribution */

#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include <string.h>
#include <time.h>

#include "../getifstats.h"
#include "../config.h"

int
getifstats(const char * ifname, struct ifdata * data)
{
	FILE *f;
	char line[512];
	char * p;
	int i;
	int r = -1;
#ifdef ENABLE_GETIFSTATS_CACHING
	static time_t cache_timestamp = 0;
	static struct ifdata cache_data;
	time_t current_time;
#endif
#ifdef IP8000 
	unsigned long long temp;
#endif
	if(!data)
		return -1;
	data->baudrate = 4200000;
	data->opackets = 0;
	data->ipackets = 0;
	data->obytes = 0;
	data->ibytes = 0;
	if(!ifname || ifname[0]=='\0')
		return -1;
#ifdef ENABLE_GETIFSTATS_CACHING
	current_time = time(NULL);
	if(current_time == ((time_t)-1)) {
		syslog(LOG_ERR, "getifstats() : time() error : %m");
	} else {
		if(current_time < cache_timestamp + GETIFSTATS_CACHING_DURATION) {
			memcpy(data, &cache_data, sizeof(struct ifdata));
			return 0;
		}
	}
#endif
	f = fopen("/proc/net/dev", "r");
	if(!f) {
		syslog(LOG_ERR, "getifstats() : cannot open /proc/net/dev : %m");
		return -1;
	}
	/* discard the two header lines */
	fgets(line, sizeof(line), f);
	fgets(line, sizeof(line), f);
	while(fgets(line, sizeof(line), f)) {
		p = line;
		while(*p==' ') p++;
		i = 0;
		while(ifname[i] == *p) {
			p++; i++;
		}
		/* TODO : how to handle aliases ? */
		if(ifname[i] || *p != ':')
			continue;
		p++;
		while(*p==' ') p++;
#ifdef IP8000 
		temp = strtoull(p, &p, 0);
		data->ibytes = (unsigned long) temp;
		while(*p==' ') p++;
		temp = strtoull(p, &p, 0);
		data->ipackets = (unsigned long) temp;
		/* skip 6 columns */
		for(i=6; i>0 && *p!='\0'; i--) {
			while(*p==' ') p++;
			while(*p!=' ' && *p) p++;
		}
		while(*p==' ') p++;
		temp = strtoull(p, &p, 0);
		data->obytes = (unsigned long) temp;
		while(*p==' ') p++;
		temp = strtoull(p, &p, 0);
		data->opackets = (unsigned long) temp;
#else
		data->ibytes = strtoull(p, &p, 0);
		while(*p==' ') p++;
		data->ipackets = strtoul(p, &p, 0);
		/* skip 6 columns */
		for(i=6; i>0 && *p!='\0'; i--) {
			while(*p==' ') p++;
			while(*p!=' ' && *p) p++;
		}
		while(*p==' ') p++;
		data->obytes = strtoull(p, &p, 0);
		while(*p==' ') p++;
		data->opackets = strtoul(p, &p, 0);
#endif
		r = 0;
		break;
	}
	fclose(f);
#ifdef ENABLE_GETIFSTATS_CACHING
	if(r==0 && current_time!=((time_t)-1)) {
		cache_timestamp = current_time;
		memcpy(&cache_data, data, sizeof(struct ifdata));
	}
#endif
	return r;
}

