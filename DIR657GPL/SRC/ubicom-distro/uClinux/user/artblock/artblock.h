#ifndef __ARTBLOCK_H
#define __ARTBLOCK_H
#if 1
void calValue2ram(void);
int artblock_set(char *name, char *value);
char *artblock_get(char *name);
char *artblock_safe_get(char *name);
char *artblock_fast_get(char *name); //for widget
#endif

char *SettingInFlashName[4] =
{
	"lan_mac",
	"wan_mac",
	"hw_version",
	"wlan0_domain"		
};

struct hw_lan_wan_t {
	char hwversion[4];
	char lanmac[20];
	char wanmac[20];
//Albert added	
	char domain[4];
};

#define CONF_BUF 30*1024
#endif
