#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "libdb.h"

/* libnet.c */
extern int get_ip(const char *, char *);
extern int get_mac(const char *, char *);
extern int get_netmask(const char *, char *);
extern void get_def_gw(char *);
extern void get_dns_srv(char *);

/* libdb.c */
extern int query_vars(char *, char *, int);

int get_wan_name(char *tmp)
{
	char buf[8], wan_ifname[8], ret = -1;

	bzero(buf, sizeof(buf));
	bzero(wan_ifname, sizeof(wan_ifname));

	if (query_vars("wan0_proto", buf, sizeof(buf)) < 0)
		goto out;
	if (query_vars("wan_ifname", wan_ifname, sizeof(wan_ifname)) < 0)
		goto out;

	if (strcmp(buf, "static") == 0)
		strcat(tmp, wan_ifname);
	else if (strcmp(buf, "dhcpc") == 0)
		strcat(tmp, wan_ifname);
	else if (strcmp(buf, "pppoe") == 0)
		strcat(tmp, "ppp0");
	else if (strcmp(buf, "pptp") == 0)
		strcat(tmp, "ppp0");
	else if (strcmp(buf, "l2tp") == 0)
		strcat(tmp, "ppp0");
	else if (strcmp(buf, "rupppoe") == 0)
		strcat(tmp, wan_ifname);
	else if (strcmp(buf, "rupptp") == 0)
		strcat(tmp, wan_ifname);

        ret = 0;
out:
        return ret;
} 

static int progam_execing(int wan_type)
{
	FILE *fp;
	char cmd[64], key1[16], key2[16];

	if(wan_type == 7){
		sprintf(key1, "pppd");
		sprintf(key2, "plugin");
	} else {
		sprintf(key1, "pptp");
		sprintf(key2, "require-mppe");
	}
	sprintf(cmd, "ps");
	if((fp = popen(cmd, "r")) == NULL)
		perror("popen ps");

	cmd[0] = '\0';
	while(fgets(cmd, sizeof(cmd), fp)) {
		if(strstr(cmd, key1) && strstr(cmd, key2)) {
			pclose(fp);
			return 1;
		}
	}
	pclose(fp);
	return 0;
}

static void ppp_check(int wan_type, char *buf)
{
	FILE *fp;
	char value[32];

	{
		if(wan_type == 7 || wan_type == 8) {
			query_vars("wan0_ipaddr", value, sizeof(value));
			if(strcmp(value, "0.0.0.0") != 0) {
				strcat(buf,"Connected"); 
				return;
			}

			if(progam_execing(wan_type)) {
				strcat(buf,"Connecting");
				return;
			}

			strcat(buf,"Disconnected");
			return;
		}
	}
	fp = fopen("/tmp/var/run/ppp0.pid", "r");
	if (fp == (FILE *) 0) { 
		strcat(buf,"Disconnected"); 
		return;
	}
	fclose(fp);
	fp = fopen("/tmp/ppp/link.ppp0", "r");
	if (fp != (FILE *) 0) { 
		strcat(buf,"Connected"); 
		fclose(fp);
		return;
	}
	query_vars("wan0_pppoe_demand", value, sizeof(value));
	if (atoi(value) == 2) {
		strcat(buf,"Disconnected"); 
		return;
	}


	strcat(buf,"Connecting");
	return;
}

void wan_status(char *buf, int wan_type)
{
	FILE *fp;
	char *p = NULL, wan[8], tmp[128];

	bzero(tmp, sizeof(tmp));
	bzero(wan, sizeof(wan));
	get_wan_name(wan);
	get_ip(wan, tmp);
	p = tmp;

	if (wan_type == 0) {
		fp = fopen("/tmp/var/run/udhcpc0.pid", "r");
		if (fp == (FILE *) 0) {
			strcat(buf, "Disconnected");
			return;
		}
		fclose(fp);
		if (*p) {
			strcat(buf, "Connected");
			return;
		}
		strcat(buf, "Connecting");
		return;
	}
	if (wan_type == 6) {
		strcat(buf, "Client");
		return;
	}

	if (wan_type > 1)
		ppp_check(wan_type, buf);
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

static char GetBase64Value(char ch)
{
        if ((ch >= 'A') && (ch <= 'Z'))
                return ch - 'A';

        if ((ch >= 'a') && (ch <= 'z'))
                return ch - 'a' + 26;

        if ((ch >= '0') && (ch <= '9'))
                return ch - '0' + 52;

        switch (ch) {
        case '+':
                return 62;
        case '/':
                return 63;
        case '=':
                return 0;
        default:
                return 0;
        }
}


int base64_decode( char *OrgString, char *Base64String, int Base64StringLen, bool bForceDecode )
{
        if( Base64StringLen % 4 && !bForceDecode ) {

                OrgString[0] = '\0';
                return -1;

        }

        unsigned char Base64Encode[4];
        int OrgStringLen=0;

        while( Base64StringLen > 2 ) {

                Base64Encode[0] = GetBase64Value(Base64String[0]);
                Base64Encode[1] = GetBase64Value(Base64String[1]);
                Base64Encode[2] = GetBase64Value(Base64String[2]);
                Base64Encode[3] = GetBase64Value(Base64String[3]);

                *OrgString ++ = (Base64Encode[0] << 2) | (Base64Encode[1] >> 4);
                *OrgString ++ = (Base64Encode[1] << 4) | (Base64Encode[2] >> 2);
                *OrgString ++ = (Base64Encode[2] << 6) | (Base64Encode[3]);

                Base64String += 4;
                Base64StringLen -= 4;
                OrgStringLen += 3;
        }

        return OrgStringLen;
}

