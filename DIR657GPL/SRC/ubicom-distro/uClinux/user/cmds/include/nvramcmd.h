#ifndef __NVRAMCMD_H__
#define __NVRAMCMD_H__
#include <nvram.h>

#ifdef CONFIG_BCM_IPSEC
/*
 * in DIR130/330 nvram_match return trun/false in match.
 * but others (eg: ath/ip7k style) 0 as match, 1 as dismatch...
 * see nvram/nvram.h
 * */
#define NVRAM_MATCH(name, match)	nvram_match(name, match)
#else
#define NVRAM_MATCH(name, match)	(!nvram_match(name, match))
#endif //CONFIG_BCM_IPSEC
#endif //__NVRAMCMD_H__
