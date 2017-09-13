#include <stdio.h>
#include <stdlib.h>

#ifdef RC_DEBUG
#define DEBUG_MSG(fmt, arg...)       printf("%s [%d]: "fmt, __FILE__, __LINE__, ##arg)
#else
#define DEBUG_MSG(fmt, arg...) 
#endif

static char *char_token(char *src_str, char sub_char)
{
    char *current_ptr,*tmp_ptr;
    int len=0;
    static char  *next_ptr;

    if (src_str != NULL)
    {
        current_ptr = src_str;
    }
    else
    {
    	current_ptr =next_ptr;
    }
	tmp_ptr=current_ptr;
	
    while (1)
    {
        if ((*tmp_ptr == sub_char)|| (*tmp_ptr=='\0'))
        {
            *tmp_ptr='\0';
            next_ptr=tmp_ptr+1;
            return current_ptr;
        }
        else
        {
	    tmp_ptr++;
	    len++;
        }
    }
}

static void set_root_qdisc(char *dev_name, int rate)
{
    char rule[256];

    /* create root qdisc */
    sprintf(rule, "tc qdisc add dev %s root handle 10: htb default 300", dev_name);
    _system(rule);

    /* create root rule */
    sprintf(rule, "tc class add dev %s parent 10: classid 10:1 htb rate %dkbps ceil %dkbps",
            dev_name, rate, rate);
    _system(rule);

    /* create default rule */
    sprintf(rule, "tc class add dev %s parent 10:1 classid 10:300 htb rate 10kbps ceil %dkbps prio 4",
            dev_name, rate);
    _system(rule);

    sprintf(rule, "tc qdisc add dev %s parent 10:300 handle 130: pfifo", dev_name);
    _system(rule);
}


void start_qos(void)
{
    char rule[256], rule_nvram[256];
    int  i, n, p, len;
    char *lan_eth, *wlan_eth, *wan_eth, *interface;
    int  qos_uplink, qos_downlink, priority, uplink, downlink;
    char *enable, *name, *protocol;
    char *local_start_ip, *local_end_ip, *local_start_port, *local_end_port;
    char *remote_start_ip, *remote_end_ip, *remote_start_port, *remote_end_port;
    char *next=NULL;
    char sub_char='/';

    /* delete all QoS rules */
    wlan_eth = "wifi0";
    sprintf(rule, "tc qdisc del dev %s root", wlan_eth);
    _system(rule);

    lan_eth = nvram_get("lan_eth");
    sprintf(rule, "tc qdisc del dev %s root", lan_eth);
    _system(rule);

#ifdef CONFIG_VLAN_ROUTER
    wan_eth = nvram_get("wan_eth");
    sprintf(rule, "tc qdisc del dev %s root", wan_eth);
    _system(rule);
#endif

    /* delete all iptables rules */
    _system("iptables -F -t mangle");

    if (strcmp(nvram_get("qos_enable"), "0") == 0)   return;

    qos_uplink = atoi(nvram_get("qos_uplink")) / 8;       /* convert bit to byte */
    qos_downlink = atoi(nvram_get("qos_downlink")) / 8;   /* convert bit to byte */

    /* wlan */
    set_root_qdisc(wlan_eth, qos_downlink);

#ifdef CONFIG_VLAN_ROUTER
    /* lan */
    set_root_qdisc(lan_eth, qos_downlink);

    /* wan */
    set_root_qdisc(wan_eth, qos_uplink);
#else
    /* lan */
    set_root_qdisc(lan_eth, qos_uplink);
#endif

    for (i=1; i<=20; i++)
    {
		sprintf(rule, "qos_rule_%02d", i-1);
		strcpy(rule_nvram, nvram_get(rule));

		if (strlen(rule) == 0)
			continue;

		enable = char_token(rule_nvram, sub_char);
		name = char_token(NULL, sub_char);
		priority = atoi(char_token(NULL, sub_char));
		uplink = atoi(char_token(NULL, sub_char)) / 8;      /* convert bit to byte */
		downlink = atoi(char_token(NULL, sub_char)) / 8;    /* convert bit to byte */
		protocol = char_token(NULL, sub_char);
		local_start_ip = char_token(NULL, sub_char);
		local_end_ip = char_token(NULL, sub_char);
		local_start_port = char_token(NULL, sub_char);
		local_end_port = char_token(NULL, sub_char);
		remote_start_ip = char_token(NULL, sub_char);
		remote_end_ip = char_token(NULL, sub_char);
		remote_start_port = char_token(NULL, sub_char);
		remote_end_port = char_token(NULL, sub_char);

		if (strcmp(enable, "0") == 0)   continue;

        /* downlink */
#ifdef CONFIG_VLAN_ROUTER
        /* router must set wlan and lan interface */
        for (n=1 ;n <= 2; n++)
#else
        /* AP only set wlan interface */
        n = 1;
#endif
        {
            if (n == 1)
                interface = wlan_eth;
            else
                interface = lan_eth;

            sprintf(rule, "tc class add dev %s parent 10:1 classid 10:%d htb rate 10kbps ceil %dkbps prio %d",
                    interface, i*10, downlink, priority);
            _system(rule);

            sprintf(rule, "tc qdisc add dev %s parent 10:%d handle %d: pfifo",
                    interface, i*10, 100+i);
            _system(rule);

            sprintf(rule, "tc filter add dev %s parent 10: protocol ip prio 100 handle %d fw classid 10:%d",
                    interface, i*100, i*10);
            _system(rule);

            for (p=0; p<= 1; p++)
            {
                len = sprintf(rule, "iptables -t mangle -A POSTROUTING ");
                if (strcmp(protocol, "256") == 0)
                {
                    if (p == 1)
                        len += sprintf(rule + len, "-p 6 ");
                    else
                        len += sprintf(rule + len, "-p 17 ");
                }
                else
                {
                    if (p == 1)    break;

                    len += sprintf(rule + len, "-p %s ", protocol);
                }

                if (strlen(local_start_ip) > 0 || strlen(local_end_ip) > 0 ||
                    strlen(remote_start_ip) > 0 || strlen(remote_end_ip) > 0)
                {
                    len += sprintf(rule + len, "-m iprange ");

                    /* no assign local_end_ip */
                    if (strlen(local_end_ip) == 0)
                        local_end_ip = local_start_ip;

                    if (strlen(local_start_ip) > 0)
                        len += sprintf(rule + len, "--dst-range %s-%s ", local_start_ip, local_end_ip);

                    if (strcmp(protocol, "6") == 0 || strcmp(protocol, "17") == 0|| strcmp(protocol, "256") == 0)
                        len += sprintf(rule + len, "--dport %s:%s ", local_start_port, local_end_port);

                    /* no assign remote_end_ip */
                    if (strlen(remote_end_ip) == 0)
                        remote_end_ip = remote_start_ip;

                    if (strlen(remote_start_ip) > 0)
                        len += sprintf(rule + len, "--src-range %s-%s ", remote_start_ip, remote_end_ip);

                    if (strcmp(protocol, "6") == 0 || strcmp(protocol, "17") == 0|| strcmp(protocol, "256") == 0)
                        len += sprintf(rule + len, "--sport %s:%s ", remote_start_port, remote_end_port);
                }

                len += sprintf(rule + len, "-j MARK --set-mark %d", i*100);

                _system(rule);
            }
        }


        /* uplink */
#ifdef CONFIG_VLAN_ROUTER
        interface = wan_eth;
#else
        interface = lan_eth;
#endif

        sprintf(rule, "tc class add dev %s parent 10:1 classid 10:%d htb rate 10kbps ceil %dkbps prio %d",
                interface, i*10, uplink, priority);
        _system(rule);

        sprintf(rule, "tc qdisc add dev %s parent 10:%d handle %d: pfifo",
                interface, i*10, 100+i);
        _system(rule);

        sprintf(rule, "tc filter add dev %s parent 10: protocol ip prio 100 handle %d fw classid 10:%d",
                interface, i*100, i*10);
        _system(rule);

        for (p=0; p <= 1; p++)
        {
            len = sprintf(rule, "iptables -t mangle -A POSTROUTING ");
            if (strcmp(protocol, "256") == 0)
            {
                if (p == 1)
                    len += sprintf(rule + len, "-p 6 ");
                else
                    len += sprintf(rule + len, "-p 17 ");
            }
            else
            {
                if (p == 1)    break;

                len += sprintf(rule + len, "-p %s ", protocol);
            }

            if (strlen(local_start_ip) > 0 || strlen(local_end_ip) > 0 ||
                strlen(remote_start_ip) > 0 || strlen(remote_end_ip) > 0)
            {
                len += sprintf(rule + len, "-m iprange ");

                /* no assign local_end_ip */
                if (strlen(local_end_ip) == 0)
                    local_end_ip = local_start_ip;

                if (strlen(local_start_ip) > 0)
                    len += sprintf(rule + len, "--src-range %s-%s ", local_start_ip, local_end_ip);

                if (strcmp(protocol, "6") == 0 || strcmp(protocol, "17") == 0|| strcmp(protocol, "256") == 0)
            len += sprintf(rule + len, "--sport %s:%s ", local_start_port, local_end_port);

                /* no assign remote_end_ip */
                if (strlen(remote_end_ip) == 0)
                    remote_end_ip = remote_start_ip;

                if (strlen(remote_start_ip) > 0)
            len += sprintf(rule + len, "--dst-range %s-%s ", remote_start_ip, remote_end_ip);

                if (strcmp(protocol, "6") == 0 || strcmp(protocol, "17") == 0|| strcmp(protocol, "256") == 0)
            len += sprintf(rule + len, "--dport %s:%s ", remote_start_port, remote_end_port);
           }

            len += sprintf(rule + len, "-j MARK --set-mark %d", i*100);

            _system(rule);
        }
    }
}


void stop_qos(void)
{
    char rule[64];

    sprintf(rule, "tc qdisc del dev wifi0 root");
    _system(rule);
    sprintf(rule, "tc qdisc del dev %s root", nvram_get("lan_eth"));
    _system(rule);
#ifdef CONFIG_VLAN_ROUTER
    sprintf(rule, "tc qdisc del dev %s root", nvram_get("wan_eth"));
    _system(rule);
#endif

    _system("iptables -F -t mangle");
}
