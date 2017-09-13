/*
 *
 *  $Id: qos.c,v 1.12 2009/05/06 02:38:44 NickChou Exp $
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <linux_vct.h>
#include <sutil.h>
#include <nvram.h>
#include <shvar.h>
#include <rc.h>

extern struct action_flag action_flags;

#ifdef RC_DEBUG
#define DEBUG_MSG(fmt, arg...)       printf("%s [%d]: "fmt, __FILE__, __LINE__, ##arg)
#else
#define DEBUG_MSG(fmt, arg...) 
#endif

#ifdef CONFIG_USER_TC
#define TC_FILE	"/var/tmp/tc.sh" 
#define TC_CHANGE_FILE	"/var/tmp/tcchange.sh" 
#define FW_MANGLE	"/var/tmp/fw_mangle"
#define DOWNLINK_BANDWIDTH 0


enum { DSCP0_PRIO, DSCP1_PRIO, DSCP2_PRIO, DSCP3_PRIO, DSCP4_PRIO, DSCP5_PRIO, DSCP6_PRIO, DSCP7_PRIO};
unsigned dscp0_rate, dscp1_rate, dscp2_rate, dscp3_rate, dscp4_rate, dscp5_rate, dscp6_rate, dscp7_rate;
unsigned mssflag,mss;
int init_rate(unsigned qos_rate)
{	
	if(qos_rate < 20)
		qos_rate = 20;
	dscp1_rate = qos_rate * 0.01;
	dscp2_rate = qos_rate * 0.01;
	dscp3_rate = qos_rate * 0.01;
	dscp4_rate = qos_rate * 0.01;
	dscp5_rate = qos_rate * 0.01;
	dscp6_rate = qos_rate * 0.01;
	dscp7_rate = qos_rate * 0.01;
	if(dscp1_rate < 5)
		dscp1_rate = 5;
	if(dscp2_rate < 5)
		dscp2_rate = 5;	
	if(dscp3_rate < 1)
		dscp3_rate = 1;
	if(dscp4_rate < 1)
		dscp4_rate = 1;
	if(dscp5_rate < 1)
		dscp5_rate = 1;
	if(dscp6_rate < 1)
		dscp6_rate = 1;
	if(dscp7_rate < 1)
		dscp7_rate = 1;
	dscp0_rate = qos_rate -dscp1_rate -dscp2_rate-dscp3_rate-dscp4_rate-dscp5_rate-dscp6_rate-dscp7_rate;	
	if(dscp0_rate < 5)
		dscp0_rate = 5;		
	return 0;
}

int init_mss(unsigned qos_rate)
{
	unsigned delmss;
	DEBUG_MSG("init_mss: qos_rate=%d\n",qos_rate);
	if( qos_rate < 600 ){
		mssflag = 1;
		delmss = (600-qos_rate)*5/2;
		DEBUG_MSG("init_mss: delmss=%d\n",delmss);
		mss = 1460-delmss;
		if(mss < 536)
			mss = 536;	
		DEBUG_MSG("init_mss: mss=%d\n",mss);
	}	
	else
		mssflag = 0;
		
	DEBUG_MSG("init_mss: mssflag=%d\n",mssflag);		
	return 0;
}

/* return 1 if we have wan_ip, otherwise return 0 */
int wan_ip_is_obtained(void)
{
	char *tmp_wan_ip1=NULL, *tmp_wan_ip2=NULL;
	char wan_ipaddr[2][16]={{0}};
	char status[16];
#if defined(CONFIG_USER_3G_USB_CLIENT_WM5) || defined(CONFIG_USER_3G_USB_CLIENT_IPHONE)
	int cable_disconnect = 0;
#endif
	int i;
#ifdef CONFIG_USER_3G_USB_CLIENT		
#ifdef CONFIG_USER_3G_USB_CLIENT_WM5
/*	Date:	2010-02-05
 *	Name:	Cosmo Chang
 *	Reason:	add usb_type=5 for Android Phone RNDIS feature
 */
//	if( ( nvram_match("wan_proto", "usb3g_phone") ==0 ) && (nvram_match("usb_type", "3") == 0) ){
//		cable_disconnect = mobile_phone_detect(3);
	char usb3g_phone_usb_type = atoi(nvram_safe_get("usb_type"));
	if( ( nvram_match("wan_proto", "usb3g_phone") ==0 ) && (usb3g_phone_usb_type == 3 || usb3g_phone_usb_type == 5)){
		cable_disconnect = mobile_phone_detect(usb3g_phone_usb_type);
		DEBUG_MSG("%s (%d), mobile_phone_detect %d (%s) \n",__func__,__LINE__,cable_disconnect
				,cable_disconnect == 0 ? "Not attached" : 
				(cable_disconnect == 1 ? "Attached, but driver not initialized" : 
				(cable_disconnect == 2 ? "Attached, driver initialized OK" : "Unknow error")));
		switch(cable_disconnect){
			case 0: // device not attached
				return 0;
			case 1: // device attached, driver not OK
				return 0;
			case 2: //device attached, driver OK
				tmp_wan_ip1 = get_ipaddr(nvram_safe_get("wan_eth"));
				if( tmp_wan_ip1 != NULL ){
					DEBUG_MSG("%s (%d), %s ip_addr = %s, wan_state return 1\n",__func__,__LINE__,tmp_wan_ip1);
					return 1;
				}else{
					DEBUG_MSG("%s (%d), %s ip_addr = NULL, wan_state return 0\n",__func__,__LINE__);
					return 0;
				}
			default:
				return 0;
		}
	}else
#endif // end CONFIG_USER_3G_USB_CLIENT_WM5
#ifdef CONFIG_USER_3G_USB_CLIENT_IPHONE
	if( (nvram_match("usb_type", "4") == 0) && ( nvram_match("wan_proto", "usb3g_phone") ==0 ) ){
		cable_disconnect = mobile_phone_detect(4);
		DEBUG_MSG("%s (%d), mobile_phone_detect %d (%s) \n",__func__,__LINE__,cable_disconnect
				,cable_disconnect == 0 ? "Not attached" : 
				(cable_disconnect == 1 ? "Attached, but driver not initialized" : 
				(cable_disconnect == 2 ? "Attached, driver initialized OK" : "Unknow error")));
		switch(cable_disconnect){
			case 0: // device not attached
				return 0;
			case 1: // device attached, driver not OK
				return 0;
			case 2: //device attached, driver OK
				tmp_wan_ip1 = get_ipaddr(nvram_safe_get("wan_eth"));
				if( tmp_wan_ip1 != NULL ){
					DEBUG_MSG("%s (%d), %s ip_addr = %s, return 1\n",__func__,__LINE__,tmp_wan_ip1);
					return 1;
				}else{
					DEBUG_MSG("%s (%d), %s ip_addr = NULL, return 0\n",__func__,__LINE__);
					return 0;
				}
			default:
				return 0;
		}
	}else
#endif // end CONFIG_USER_3G_USB_CLIENT_IPHONE
	if(nvram_match("wan_proto", "usb3g")) //wan_proto is not usb3g
	{
#endif	
	/* Check phy connect statue */
	for(i=0;i<20;i++)
	{
		VCTGetPortConnectState( nvram_safe_get("wan_eth"), VCTWANPORT0, status);
		if( !strncmp("disconnect", status, 10) )
		{
			if(i>=10)
			{
				DEBUG_MSG("disconnect\n");
				return 0;
			}
			sleep(1);
		}
		else
			break;
	}

#ifdef CONFIG_USER_3G_USB_CLIENT	
	}
#endif		

	if (( nvram_match("wan_proto", "dhcpc") ==0 ) || ( nvram_match("wan_proto", "static") ==0 )
#if defined(CONFIG_USER_3G_USB_CLIENT_WM5) || defined(CONFIG_USER_3G_USB_CLIENT_IPHONE)
		|| ( nvram_match("wan_proto", "usb3g_phone") ==0 )
#endif
	)
	{
		tmp_wan_ip1 = get_ipaddr( nvram_safe_get("wan_eth") );
		if(tmp_wan_ip1)
			strncpy(wan_ipaddr[0],tmp_wan_ip1,strlen(tmp_wan_ip1));	
	}
	else 
	{
		tmp_wan_ip1 = get_ipaddr( "ppp0" );
		if(tmp_wan_ip1)
			strncpy(wan_ipaddr[0],tmp_wan_ip1,strlen(tmp_wan_ip1));
			
		/* if Russia is enabled */
		if ((( nvram_match("wan_proto", "pppoe") ==0 ) && ( nvram_match("wan_pppoe_russia_enable", "1") ==0 )) ||
			(( nvram_match("wan_proto", "pptp") ==0 ) && ( nvram_match("wan_pptp_russia_enable", "1") ==0 )) ||
			(( nvram_match("wan_proto", "l2tp") ==0 ) && ( nvram_match("wan_l2tp_russia_enable", "1") ==0 )) )
		{
			tmp_wan_ip2 = get_ipaddr( nvram_safe_get("wan_eth") );
			if(tmp_wan_ip2)
				strncpy(wan_ipaddr[1],tmp_wan_ip2,strlen(tmp_wan_ip2));	
		}
	}

	if(!(strlen(wan_ipaddr[0])) && !(strlen(wan_ipaddr[1])))
	{
		printf("wan_ipaddr == NULL, QOS don't start\n");
		return 0;
	}
	else
		return 1;

}

int start_qos(void)
{
	FILE *fp;
	char *wan_eth=NULL,*wan_eth2=NULL, *lan_eth, *getValue, qos_value[256]={},qos_cmd[256]={},qos_cmd2[256]={};
	char qos_rule[]="qos_rule_XXXX",bandwidth[32]={0};	
	unsigned char *prio, *uplink, *downlink;
	char *enable, *name, *proto, *src_ip_start, *src_ip_end, *src_port_start, *src_port_end, *wan_proto;//, *src_mac;
	char *dst_ip_start, *dst_ip_end, *dst_port_start, *dst_port_end;
	unsigned int i, qos_rate=0;
		
	if(!( action_flags.all || action_flags.qos ))
		return 1;
		
	/*When rc restart_ALL or restart_QoS with disable measuring bandwidth, we remove bandwidth file */
	if((action_flags.all) || (nvram_match("traffic_shaping", "1")) || (nvram_match("auto_uplink", "1")))
		unlink("/var/tmp/bandwidth_result.txt");	

	DEBUG_MSG("start_qos\n");

	if (nvram_match("traffic_shaping", "0") == 0) {
		DEBUG_MSG("traffic_shaping disable\n");
	/* 
	 *     Date: 2010-02-22
	 *     Name: Murat Sezgin <msezgin@ubicom.com>
	 *     Reason: If the traffic shaping is disabled, attach the WAN interface to the 
	 *             linux's own priority scheduler.
	 */
	 	wan_proto = nvram_safe_get("wan_proto");
	 	if( strcmp(wan_proto, "pppoe") == 0 || strcmp(wan_proto, "pptp") == 0  || strcmp(wan_proto, "l2tp") == 0) {
	 		//wan_eth=nvram_safe_get("wan_eth");
			//_system("tc qdisc add dev %s handle 1:0 root prio", wan_eth);
			//_system("tc qdisc show dev %s", wan_eth);
		}
		else {
#ifdef CONFIG_MODULE_UBICOM_STREAMENGINE2_IP8K			
			// Only for wan mode is static and dhcp.
//			system("[ -z \"`lsmod | grep ubicom_na_connection_manager_ipv4`\" ] && insmod /lib/modules/2.6.36+/ubicom_na_connection_manager_ipv4.ko");
///insmod			system("[ -z \"`lsmod | grep ubicom_na_connection_manager_ipv6`\" ] && insmod /lib/modules/2.6.36+/ubicom_na_connection_manager_ipv6.ko");
		system("/etc/init.d/se_restart");
	        _system("echo \"%d\" > /sys/devices/system/streamengine_db/streamengine_db0/connections_max", MAX_IP_CONNTRACK_NUMBER);
#endif		
		}

		return 0;
	}
	DEBUG_MSG("traffic_shaping\n");
	
	/* Check if we have wan IP already*/
	if (!wan_ip_is_obtained()) {
		DEBUG_MSG("wan no IP already\n");
		return 1;
	}	
	DEBUG_MSG("wan IP already\n");

/* 
 *	Date: 2009-10-08
 *	Name: Gareth Williams <gareth.williams@ubicom.com>
 *	Reason: Streamengine support - streamengine shouldn't start until we have a WAN IP
 */
	wan_eth=nvram_safe_get("wan_eth");
	printf("iptables -t filter -I FORWARD 1 --out-interface %s -j DROP\n",wan_eth);
	_system("iptables -t filter -I FORWARD 1 --out-interface %s -j DROP",wan_eth);
#ifdef CONFIG_USER_STREAMENGINE2
	_system("/etc/init.d/se_restart");
#ifdef CONFIG_MODULE_UBICOM_STREAMENGINE2_IP8K			
	_system("echo \"%d\" > /sys/devices/system/streamengine_db/streamengine_db0/connections_max", MAX_IP_CONNTRACK_NUMBER);
#endif
#elif CONFIG_USER_STREAMENGINE
	_system("/etc/init.d/rc_streamengine_restart");
#else
	if(nvram_match("auto_uplink", "1")==0){		
		fp = fopen("/var/tmp/bandwidth_result.txt","r");
		if(!fp)
		{
			printf("THERE IS NO FILE\n");
			/* jimmy modified 20080602 */
			//start_mbandwidth();	
/* 
 	Date: 2009-1-08 
 	Name: Ken_Chiang
 	Reason: modify for remove mbandwidth daemon link sutil and nvram.
 	Notice :
*/ 	
/*
			_system("mbandwidth 10 &");
*/			
/* 
 	Date: 2009-1-08 
 	Name: Ken_Chiang
 	Reason: modify for mbandwidth ping url from manufacturer_url to google.
 	Notice :
*/ 	
/*
			_system("mbandwidth %s %s &",nvram_get("wan_eth"),nvram_get("manufacturer_url"));
*/			
#ifdef CONFIG_USER_3G_USB_CLIENT
			if(nvram_match("wan_proto", "usb3g"))	// is not 3G
				_system("mbandwidth %s %s 0 &",nvram_get("wan_eth"),"http://www.google.com");
			else
				_system("mbandwidth %s %s 1 &",nvram_get("wan_eth"),"http://www.google.com");	
#else
			_system("mbandwidth %s %s 0 &",nvram_get("wan_eth"),"http://www.google.com");
#endif			
		}
		else
		{
			printf("THERE IS FILE\n");
			fclose(fp);
		}
	}
	
	if (( nvram_match("wan_proto", "dhcpc") ==0 ) || ( nvram_match("wan_proto", "static") ==0 )
#if defined(CONFIG_USER_3G_USB_CLIENT_WM5) || defined(CONFIG_USER_3G_USB_CLIENT_IPHONE)
		|| ( nvram_match("wan_proto", "usb3g_phone") ==0 )
#endif
	)
	{
		DEBUG_MSG("dhcpc || static\n");
		wan_eth = nvram_get("wan_eth");	
	}
	else 
	{
		DEBUG_MSG("ppp\n");
		wan_eth = "ppp0";					
		/* if Russia is enabled */
		if ((( nvram_match("wan_proto", "pppoe") ==0 ) && ( nvram_match("wan_pppoe_russia_enable", "1") ==0 )) ||
			(( nvram_match("wan_proto", "pptp") ==0 ) && ( nvram_match("wan_pptp_russia_enable", "1") ==0 )) ||
			(( nvram_match("wan_proto", "l2tp") ==0 ) && ( nvram_match("wan_l2tp_russia_enable", "1") ==0 )))
		{
			DEBUG_MSG("Russia\n");
			wan_eth2 = nvram_get("wan_eth");
			DEBUG_MSG("wan_eth2=%s\n",wan_eth2);
		}
	}	
	lan_eth = nvram_get("lan_bridge");
	
	if(nvram_match("auto_uplink", "1")==0){
		DEBUG_MSG("auto_uplink\n");		
			get_measure_bandwidth(bandwidth);
			qos_rate = atoi(bandwidth);					
	}
	else{//manual
		DEBUG_MSG("maul_uplink\n");
		qos_rate = atoi( nvram_safe_get("qos_uplink"));
	}	
	
	if(!qos_rate){
		printf("qos_rate==0\n");
		/* Modify if QoS can't get BW set to 100Mbps*/
		//qos_rate = 100*1024;
		return 1;
	}	
		
	DEBUG_MSG("wan_eth=%s,lan_eth=%s,qos_rate=%d\n",wan_eth,lan_eth,qos_rate);
	if(wan_eth2)
		DEBUG_MSG("wan_eth2=%s\n",wan_eth2);
	init_file(TC_FILE);
	chmod(TC_FILE, S_IRUSR|S_IWUSR|S_IXUSR);
	init_rate(qos_rate);
	init_mss(qos_rate);

#if (DOWNLINK_BANDWIDTH)
	printf("DOWNLINK_BANDWIDTH\n");
	/* root */
	save2file(TC_FILE, "tc qdisc add dev %s root handle 10: htb default 80\n", lan_eth);
	save2file(TC_FILE, "tc class add dev %s parent 10: classid 10:1 htb rate %dkbit\n", lan_eth, qos_rate);
	
	/* DSCP0_PRIO 100 */
	save2file(TC_FILE, "tc class add dev %s parent 10:1 classid 10:10 htb rate %dkbit ceil %dkbit prio %d burst 8k\n", lan_eth, dscp0_rate, qos_rate, DSCP0_PRIO);//6k
	save2file(TC_FILE, "tc qdisc add dev %s parent 10:10 handle 100: pfifo limit 128\n", lan_eth);//100
    /* DSCP0_PRIO  Class filter */
	save2file(TC_FILE, "tc filter add dev %s parent 10:0 protocol ip prio %d handle 10 fw classid 10:10\n", lan_eth, DSCP0_PRIO*10+10);//no prio    	
	
	/* DSCP0_PRI1 200 */
	save2file(TC_FILE, "tc class add dev %s parent 10:1 classid 10:20 htb rate %dkbit ceil %dkbit prio %d burst 8k\n", lan_eth, dscp1_rate, qos_rate, DSCP1_PRIO);
	save2file(TC_FILE, "tc qdisc add dev %s parent 10:20 handle 200: pfifo limit 128\n", lan_eth);
    /* DSCP0_PRI1 Class filter */
	save2file(TC_FILE, "tc filter add dev %s parent 10:0 protocol ip prio %d handle 20 fw classid 10:20\n", lan_eth, DSCP1_PRIO*10+10);    	
	
	/* DSCP0_PRI2 300 */
	save2file(TC_FILE, "tc class add dev %s parent 10:1 classid 10:30 htb rate %dkbit ceil %dkbit prio %d burst 8k\n", lan_eth, dscp2_rate, qos_rate, DSCP2_PRIO);
	save2file(TC_FILE, "tc qdisc add dev %s parent 10:30 handle 300: pfifo limit 128\n", lan_eth);
    /* DSCP0_PRI2 Class filter */
	save2file(TC_FILE, "tc filter add dev %s parent 10:0 protocol ip prio %d handle 30 fw classid 10:30\n", lan_eth, DSCP2_PRIO*10+10);    	
	
	/* DSCP0_PRI3 400 */
	save2file(TC_FILE, "tc class add dev %s parent 10:1 classid 10:40 htb rate %dkbit ceil %dkbit prio %d burst 8k\n", lan_eth, dscp3_rate, qos_rate, DSCP3_PRIO);
	save2file(TC_FILE, "tc qdisc add dev %s parent 10:40 handle 400: pfifo limit 128\n", lan_eth);
    /* DSCP0_PRI3 Class filter */
	save2file(TC_FILE, "tc filter add dev %s parent 10:0 protocol ip prio %d handle 40 fw classid 10:40\n", lan_eth, DSCP3_PRIO*10+10);
			    	
	/* DSCP0_PRI4 500 */
	save2file(TC_FILE, "tc class add dev %s parent 10:1 classid 10:50 htb rate %dkbit ceil %dkbit prio %d burst 8k\n", lan_eth, dscp4_rate, qos_rate, DSCP4_PRIO);
	save2file(TC_FILE, "tc qdisc add dev %s parent 10:50 handle 500: pfifo limit 128\n", lan_eth);
	/* DSCP0_PRI4 Class filter */
	save2file(TC_FILE, "tc filter add dev %s parent 10:0 protocol ip prio %d handle 50 fw classid 10:50\n", lan_eth, DSCP4_PRIO*10+10);    	
	
	/* DSCP0_PRI5 600 */
	save2file(TC_FILE, "tc class add dev %s parent 10:1 classid 10:60 htb rate %dkbit ceil %dkbit prio %d burst 8k\n", lan_eth, dscp5_rate, qos_rate, DSCP5_PRIO);
	save2file(TC_FILE, "tc qdisc add dev %s parent 10:60 handle 600: pfifo limit 128\n", lan_eth);
	/* DSCP0_PRI5 Class filter */
	save2file(TC_FILE, "tc filter add dev %s parent 10:0 protocol ip prio %d handle 60 fw classid 10:60\n", lan_eth, DSCP5_PRIO*10+10);
	
	/* DSCP0_PRI6 700 */
	save2file(TC_FILE, "tc class add dev %s parent 10:1 classid 10:70 htb rate %dkbit ceil %dkbit prio %d burst 8k\n", lan_eth, dscp6_rate, qos_rate, DSCP6_PRIO);
	save2file(TC_FILE, "tc qdisc add dev %s parent 10:70 handle 700: pfifo limit 128\n", lan_eth);
	/* DSCP0_PRI6 Class filter */
	save2file(TC_FILE, "tc filter add dev %s parent 10:0 protocol ip prio %d handle 70 fw classid 10:70\n", lan_eth, DSCP6_PRIO*10+10);
	
	/* DSCP0_PRI7 800 */
	save2file(TC_FILE, "tc class add dev %s parent 10:1 classid 10:80 htb rate %dkbit ceil %dkbit prio %d burst 8k\n", lan_eth, dscp7_rate, qos_rate, DSCP7_PRIO);
	save2file(TC_FILE, "tc qdisc add dev %s parent 10:80 handle 800: pfifo limit 128\n", lan_eth);
	/* DSCP0_PRI7 Class  filter */
	save2file(TC_FILE, "tc filter add dev %s parent 10:0 protocol ip prio %d handle 80 fw classid 10:80\n", lan_eth, DSCP7_PRIO*10+10);
	
	/* DSCP0_PRIO 100 */
	//save2file(TC_FILE, "tc filter add dev %s parent 10:0 protocol ip prio %d u32 match ip tos 0xe0 0xff flowid 10:10\n", lan_eth, DSCP0_PRIO);
	/* DSCP1_PRIO 200 */
	//save2file(TC_FILE, "tc filter add dev %s parent 10:0 protocol ip prio %d u32 match ip tos 0xc0 0xff flowid 10:20\n", lan_eth, DSCP1_PRIO);
	/* DSCP2_PRIO 300 */
	//save2file(TC_FILE, "tc filter add dev %s parent 10:0 protocol ip prio %d u32 match ip tos 0xa0 0xff flowid 10:30\n", lan_eth, DSCP2_PRIO);
	/* DSCP3_PRIO 400 */
	//save2file(TC_FILE, "tc filter add dev %s parent 10:0 protocol ip prio %d u32 match ip tos 0x80 0xff flowid 10:40\n", lan_eth, DSCP3_PRIO);
	/* DSCP4_PRIO 500 */
	//save2file(TC_FILE, "tc filter add dev %s parent 10:0 protocol ip prio %d u32 match ip tos 0x60 0xff flowid 10:50\n", lan_eth, DSCP4_PRIO);
	/* DSCP5_PRIO 600 */
	//save2file(TC_FILE, "tc filter add dev %s parent 10:0 protocol ip prio %d u32 match ip tos 0x40 0xff flowid 10:60\n", lan_eth, DSCP5_PRIO);
	/* DSCP6_PRIO 700 */
	//save2file(TC_FILE, "tc filter add dev %s parent 10:0 protocol ip prio %d u32 match ip tos 0x20 0xff flowid 10:70\n", lan_eth, DSCP6_PRIO);
	
	if( nvram_match("qos_enable", "1")==0 ){
		DEBUG_MSG("qos_enable\n");
			
		/* iptables rule */
		init_file(FW_MANGLE);
		save2file(FW_MANGLE,
			"*mangle\n"
			":PREROUTING ACCEPT [0:0]\n"
			":INPUT ACCEPT [0:0]\n"
			":FORWARD ACCEPT [0:0]\n"
			":OUTPUT ACCEPT [0:0]\n"
			":POSTROUTING ACCEPT [0:0]\n");
			
		if( nvram_match("auto_classification", "1")==0 ){
			DEBUG_MSG("auto_class\n");		        				     	
			/* DSCP3_PRIO 800 */			

			/* DSCP4_PRIO 700 */			
			        	
			/* DSCP5_PRIO 600 */			
					
			/* DSCP6_PRIO 500 */		
						
			/* DSCP7_PRIO 400 */
			
			/* DSCP2_PRIO 300 */			
						
			/* DSCP1_PRIO 200 */        	
			
			/* DSCP0_PRIO 100 */			
			
		}
			
		for(i=0; i<MAX_QOS_NUMBER; i++) { 
			snprintf(qos_rule, sizeof(qos_rule), "qos_rule_%02d",i) ;
			getValue = nvram_get(qos_rule);
			if ( getValue == NULL ||  strlen(getValue) == 0 ) 
				continue;
			else 
				strncpy(qos_value, getValue, sizeof(qos_value));
			DEBUG_MSG("qos_rule=%s\n",qos_rule);
			if( getStrtok(qos_value, "/", "%s %s %s %s %s %s %s %s %s %s %s %s %s %s", &enable, &name, &prio, &uplink, &downlink, &proto, &src_ip_start, &src_ip_end, \
							&src_port_start, &src_port_end, &dst_ip_start, &dst_ip_end, &dst_port_start, &dst_port_end) !=14 )
				continue;

			if( !strcmp("1", enable) )  {
				DEBUG_MSG("qos_rule=%s enable prio=%s,proto=%s,src_port_start=%s,src_port_end=%s,dst_port_start=%s,dst_port_end=%s\n",qos_rule,prio,proto,src_port_start,src_port_end,dst_port_start,dst_port_end);
				DEBUG_MSG("src_ip_start=%s src_ip_end=%s,dst_ip_start=%s,dst_ip_end=%s\n",src_ip_start,src_ip_end,dst_ip_start,dst_ip_end);
				if(!strcmp(proto, "256")){//Any
					DEBUG_MSG("Any\n");
					sprintf(qos_cmd, "-p all");					
					if( src_ip_start || dst_ip_start) {						
						if( src_ip_end && dst_ip_end && strcmp(src_ip_start, "0.0.0.0")!=0 && strcmp(src_ip_end, "255.255.255.255")!=0 
							&& strcmp(dst_ip_start, "0.0.0.0")!=0 && strcmp(dst_ip_end, "255.255.255.255")!=0 ){
							sprintf(qos_cmd, "%s -m iprange", qos_cmd);
							sprintf(qos_cmd, "%s --src-range %s-%s", qos_cmd, src_ip_start, src_ip_end );
							sprintf(qos_cmd, "%s --dst-range %s-%s", qos_cmd, dst_ip_start, dst_ip_end );
						}
						else if( src_ip_end && dst_ip_end && strcmp(src_ip_start, "0.0.0.0")!=0 && strcmp(src_ip_end, "255.255.255.255")!=0
							&& strcmp(dst_ip_start, "0.0.0.0")==0 && strcmp(dst_ip_end, "255.255.255.255")==0 ){
							sprintf(qos_cmd, "%s --dst any/0", qos_cmd );
							sprintf(qos_cmd, "%s -m iprange", qos_cmd);
							sprintf(qos_cmd, "%s --src-range %s-%s", qos_cmd, src_ip_start, src_ip_end );			
						}
						else if( src_ip_end && dst_ip_end && strcmp(src_ip_start, "0.0.0.0")==0 && strcmp(src_ip_end, "255.255.255.255")==0
							&& strcmp(dst_ip_start, "0.0.0.0")!=0 && strcmp(dst_ip_end, "255.255.255.255")!=0 ){
							sprintf(qos_cmd, "%s --src any/0", qos_cmd );
							sprintf(qos_cmd, "%s -m iprange", qos_cmd);
							sprintf(qos_cmd, "%s --dst-range %s-%s", qos_cmd, dst_ip_start, dst_ip_end );							
						}
						else if( src_ip_end && dst_ip_end && strcmp(src_ip_start, "0.0.0.0")==0 && strcmp(src_ip_end, "255.255.255.255")==0
							&& strcmp(dst_ip_start, "0.0.0.0")==0 && strcmp(dst_ip_end, "255.255.255.255")==0 ){
							sprintf(qos_cmd, "%s --src any/0", qos_cmd );
							sprintf(qos_cmd, "%s --dst any/0", qos_cmd );
						}						
					}					
					DEBUG_MSG("qos_cmd=%s \n",qos_cmd);					
					save2file(FW_MANGLE, "-A POSTROUTING -i %s %s -j MARK --set-mark %d \n",wan_eth, qos_cmd, atoi(prio)*10 );
				}
				else if(!strcmp(proto, "257")){//Both
					DEBUG_MSG("Both\n");
					sprintf(qos_cmd, "-p tcp -m tcp");	
					sprintf(qos_cmd2, "-p udp -m udp");										
					if( src_port_start && src_port_end){
							sprintf(qos_cmd, "%s --sport %s:%s", qos_cmd, src_port_start, src_port_end);	
							sprintf(qos_cmd2, "%s --sport %s:%s", qos_cmd2, src_port_start, src_port_end);													
					}					
					if( dst_port_start && dst_port_end){
							sprintf(qos_cmd, "%s --dport %s:%s", qos_cmd, dst_port_start, dst_port_end);							
							sprintf(qos_cmd2, "%s --dport %s:%s", qos_cmd2, dst_port_start, dst_port_end);							
					}
					if( src_ip_start || dst_ip_start) {						
						if( src_ip_end && dst_ip_end && strcmp(src_ip_start, "0.0.0.0")!=0 && strcmp(src_ip_end, "255.255.255.255")!=0 
							&& strcmp(dst_ip_start, "0.0.0.0")!=0 && strcmp(dst_ip_end, "255.255.255.255")!=0 ){
							sprintf(qos_cmd, "%s -m iprange", qos_cmd);
							sprintf(qos_cmd, "%s --src-range %s-%s", qos_cmd, src_ip_start, src_ip_end );
							sprintf(qos_cmd, "%s --dst-range %s-%s", qos_cmd, dst_ip_start, dst_ip_end );
							sprintf(qos_cmd2, "%s -m iprange", qos_cmd2);
							sprintf(qos_cmd2, "%s --src-range %s-%s", qos_cmd2, src_ip_start, src_ip_end );
							sprintf(qos_cmd2, "%s --dst-range %s-%s", qos_cmd2, dst_ip_start, dst_ip_end );
						}
						else if( src_ip_end && dst_ip_end && strcmp(src_ip_start, "0.0.0.0")!=0 && strcmp(src_ip_end, "255.255.255.255")!=0
							&& strcmp(dst_ip_start, "0.0.0.0")==0 && strcmp(dst_ip_end, "255.255.255.255")==0 ){
							sprintf(qos_cmd, "%s --dst any/0", qos_cmd );
							sprintf(qos_cmd, "%s -m iprange", qos_cmd);
							sprintf(qos_cmd, "%s --src-range %s-%s", qos_cmd, src_ip_start, src_ip_end );
							sprintf(qos_cmd2, "%s --dst any/0", qos_cmd2 );
							sprintf(qos_cmd2, "%s -m iprange", qos_cmd2);
							sprintf(qos_cmd2, "%s --src-range %s-%s", qos_cmd2, src_ip_start, src_ip_end );				
						}
						else if( src_ip_end && dst_ip_end && strcmp(src_ip_start, "0.0.0.0")==0 && strcmp(src_ip_end, "255.255.255.255")==0
							&& strcmp(dst_ip_start, "0.0.0.0")!=0 && strcmp(dst_ip_end, "255.255.255.255")!=0 ){
							sprintf(qos_cmd, "%s --src any/0", qos_cmd );
							sprintf(qos_cmd, "%s -m iprange", qos_cmd);
							sprintf(qos_cmd, "%s --dst-range %s-%s", qos_cmd, dst_ip_start, dst_ip_end );							
							sprintf(qos_cmd2, "%s --src any/0", qos_cmd2 );
							sprintf(qos_cmd2, "%s -m iprange", qos_cmd2);
							sprintf(qos_cmd2, "%s --dst-range %s-%s", qos_cmd2, dst_ip_start, dst_ip_end );							
						}
						else if( src_ip_end && dst_ip_end && strcmp(src_ip_start, "0.0.0.0")==0 && strcmp(src_ip_end, "255.255.255.255")==0
							&& strcmp(dst_ip_start, "0.0.0.0")==0 && strcmp(dst_ip_end, "255.255.255.255")==0 ){
							sprintf(qos_cmd, "%s --src any/0 ", qos_cmd );
							sprintf(qos_cmd2, "%s --dst any/0 ", qos_cmd2 );
						}						
					}
					
					DEBUG_MSG("qos_cmd=%s \n",qos_cmd);	
					DEBUG_MSG("qos_cmd2=%s \n",qos_cmd2);				
					save2file(FW_MANGLE, "-A POSTROUTING -i %s %s -j MARK --set-mark %d \n", wan_eth, qos_cmd, atoi(prio)*10 );
					save2file(FW_MANGLE, "-A POSTROUTING -i %s %s -j MARK --set-mark %d \n", wan_eth, qos_cmd2, atoi(prio)*10 );										
				}
				else{//TCP,UDP,ICMP,Other		
					DEBUG_MSG("TCP,UDP,ICMP,Other\n");
					sprintf(qos_cmd, "-p %s", proto);
					if( !strcmp(proto, "6") || !strcmp(proto, "17") ){	/* ports only for UTP or TCP */
						if( src_port_start && src_port_end)
							sprintf(qos_cmd, "%s --sport %s:%s", qos_cmd, src_port_start, src_port_end);
						if( dst_port_start && dst_port_end)
							sprintf(qos_cmd, "%s --dport %s:%s", qos_cmd, dst_port_start, dst_port_end);
					}
					if( src_ip_start || dst_ip_start) {						
						if( src_ip_end && dst_ip_end && strcmp(src_ip_start, "0.0.0.0")!=0 && strcmp(src_ip_end, "255.255.255.255")!=0 
							&& strcmp(dst_ip_start, "0.0.0.0")!=0 && strcmp(dst_ip_end, "255.255.255.255")!=0 ){
							sprintf(qos_cmd, "%s -m iprange", qos_cmd);
							sprintf(qos_cmd, "%s --src-range %s-%s", qos_cmd, src_ip_start, src_ip_end );
							sprintf(qos_cmd, "%s --dst-range %s-%s", qos_cmd, dst_ip_start, dst_ip_end );
						}
						else if( src_ip_end && dst_ip_end && strcmp(src_ip_start, "0.0.0.0")!=0 && strcmp(src_ip_end, "255.255.255.255")!=0
							&& strcmp(dst_ip_start, "0.0.0.0")==0 && strcmp(dst_ip_end, "255.255.255.255")==0 ){
							sprintf(qos_cmd, "%s --dst any/0", qos_cmd );
							sprintf(qos_cmd, "%s -m iprange", qos_cmd);
							sprintf(qos_cmd, "%s --src-range %s-%s", qos_cmd, src_ip_start, src_ip_end );			
						}
						else if( src_ip_end && dst_ip_end && strcmp(src_ip_start, "0.0.0.0")==0 && strcmp(src_ip_end, "255.255.255.255")==0
							&& strcmp(dst_ip_start, "0.0.0.0")!=0 && strcmp(dst_ip_end, "255.255.255.255")!=0 ){
							sprintf(qos_cmd, "%s --src any/0", qos_cmd );
							sprintf(qos_cmd, "%s -m iprange", qos_cmd);
							sprintf(qos_cmd, "%s --dst-range %s-%s", qos_cmd, dst_ip_start, dst_ip_end );							
						}
						else if( src_ip_end && dst_ip_end && strcmp(src_ip_start, "0.0.0.0")==0 && strcmp(src_ip_end, "255.255.255.255")==0
							&& strcmp(dst_ip_start, "0.0.0.0")==0 && strcmp(dst_ip_end, "255.255.255.255")==0 ){
							sprintf(qos_cmd, "%s --src any/0", qos_cmd );
							sprintf(qos_cmd, "%s --dst any/0", qos_cmd );
						}						
					}
					
					DEBUG_MSG("qos_cmd=%s,mask=%d \n",qos_cmd, atoi(prio)*10);					
					save2file(FW_MANGLE, "-A POSTROUTING -i %s %s -j MARK --set-mark %d \n", wan_eth, qos_cmd, atoi(prio)*10 );
				}	
			}
		}
		
		save2file(FW_MANGLE, "COMMIT\n");
		_system("iptables-restore %s", FW_MANGLE);
		unlink(FW_MANGLE);		
	}	

#else//DOWNLINK_BANDWIDTH
	printf("UPLINK_BANDWIDTH\n");	
	/* root */
	save2file(TC_FILE, "tc qdisc add dev %s root handle 10: htb default 80\n", wan_eth);
	save2file(TC_FILE, "tc class add dev %s parent 10: classid 10:1 htb rate %dkbit\n", wan_eth, qos_rate);
	
	/* DSCP0_PRIO 100 */
	save2file(TC_FILE, "tc class add dev %s parent 10:1 classid 10:10 htb rate %dkbit ceil %dkbit prio %d burst 8k\n", wan_eth, dscp0_rate, qos_rate, DSCP0_PRIO);//6k
	save2file(TC_FILE, "tc qdisc add dev %s parent 10:10 handle 100: pfifo limit 128\n", wan_eth);//100
    /* DSCP0_PRIO  Class filter */
	save2file(TC_FILE, "tc filter add dev %s parent 10:0 protocol ip prio %d handle 10 fw classid 10:10\n", wan_eth, DSCP0_PRIO*10+10);//no prio    	
	
	/* DSCP0_PRI1 200 */
	save2file(TC_FILE, "tc class add dev %s parent 10:1 classid 10:20 htb rate %dkbit ceil %dkbit prio %d burst 8k\n", wan_eth, dscp1_rate, qos_rate, DSCP1_PRIO);
	save2file(TC_FILE, "tc qdisc add dev %s parent 10:20 handle 200: pfifo limit 128\n", wan_eth);
    /* DSCP0_PRI1 Class filter */
	save2file(TC_FILE, "tc filter add dev %s parent 10:0 protocol ip prio %d handle 20 fw classid 10:20\n", wan_eth, DSCP1_PRIO*10+10);    	
	
	/* DSCP0_PRI2 300 */
	save2file(TC_FILE, "tc class add dev %s parent 10:1 classid 10:30 htb rate %dkbit ceil %dkbit prio %d burst 8k\n", wan_eth, dscp2_rate, qos_rate, DSCP2_PRIO);
	save2file(TC_FILE, "tc qdisc add dev %s parent 10:30 handle 300: pfifo limit 128\n", wan_eth);
    /* DSCP0_PRI2 Class filter */
	save2file(TC_FILE, "tc filter add dev %s parent 10:0 protocol ip prio %d handle 30 fw classid 10:30\n", wan_eth, DSCP2_PRIO*10+10);    	
	
	/* DSCP0_PRI3 400 */
	save2file(TC_FILE, "tc class add dev %s parent 10:1 classid 10:40 htb rate %dkbit ceil %dkbit prio %d burst 8k\n", wan_eth, dscp3_rate, qos_rate, DSCP3_PRIO);
	save2file(TC_FILE, "tc qdisc add dev %s parent 10:40 handle 400: pfifo limit 128\n", wan_eth);
    /* DSCP0_PRI3 Class filter */
	save2file(TC_FILE, "tc filter add dev %s parent 10:0 protocol ip prio %d handle 40 fw classid 10:40\n", wan_eth, DSCP3_PRIO*10+10);
			    	
	/* DSCP0_PRI4 500 */
	save2file(TC_FILE, "tc class add dev %s parent 10:1 classid 10:50 htb rate %dkbit ceil %dkbit prio %d burst 8k\n", wan_eth, dscp4_rate, qos_rate, DSCP4_PRIO);
	save2file(TC_FILE, "tc qdisc add dev %s parent 10:50 handle 500: pfifo limit 128\n", wan_eth);
	/* DSCP0_PRI4 Class filter */
	save2file(TC_FILE, "tc filter add dev %s parent 10:0 protocol ip prio %d handle 50 fw classid 10:50\n", wan_eth, DSCP4_PRIO*10+10);    	
	
	/* DSCP0_PRI5 600 */
	save2file(TC_FILE, "tc class add dev %s parent 10:1 classid 10:60 htb rate %dkbit ceil %dkbit prio %d burst 8k\n", wan_eth, dscp5_rate, qos_rate, DSCP5_PRIO);
	save2file(TC_FILE, "tc qdisc add dev %s parent 10:60 handle 600: pfifo limit 128\n", wan_eth);
	/* DSCP0_PRI5 Class filter */
	save2file(TC_FILE, "tc filter add dev %s parent 10:0 protocol ip prio %d handle 60 fw classid 10:60\n", wan_eth, DSCP5_PRIO*10+10);
	
	/* DSCP0_PRI6 700 */
	save2file(TC_FILE, "tc class add dev %s parent 10:1 classid 10:70 htb rate %dkbit ceil %dkbit prio %d burst 8k\n", wan_eth, dscp6_rate, qos_rate, DSCP6_PRIO);
	save2file(TC_FILE, "tc qdisc add dev %s parent 10:70 handle 700: pfifo limit 128\n", wan_eth);
	/* DSCP0_PRI6 Class filter */
	save2file(TC_FILE, "tc filter add dev %s parent 10:0 protocol ip prio %d handle 70 fw classid 10:70\n", wan_eth, DSCP6_PRIO*10+10);
	
	/* DSCP0_PRI7 800 */
	save2file(TC_FILE, "tc class add dev %s parent 10:1 classid 10:80 htb rate %dkbit ceil %dkbit prio %d burst 8k\n", wan_eth, dscp7_rate, qos_rate, DSCP7_PRIO);
	save2file(TC_FILE, "tc qdisc add dev %s parent 10:80 handle 800: pfifo limit 128\n", wan_eth);
	/* DSCP0_PRI7 Class  filter */
	save2file(TC_FILE, "tc filter add dev %s parent 10:0 protocol ip prio %d handle 80 fw classid 10:80\n", wan_eth, DSCP7_PRIO*10+10);
	
	if(wan_eth2){
		DEBUG_MSG("wan_eth2=%s\n",wan_eth2);
		save2file(TC_FILE, "tc qdisc add dev %s root handle 10: htb default 80\n", wan_eth2);
		save2file(TC_FILE, "tc class add dev %s parent 10: classid 10:1 htb rate %dkbit\n", wan_eth2, qos_rate);
		
		/* DSCP0_PRIO 100 */
		save2file(TC_FILE, "tc class add dev %s parent 10:1 classid 10:10 htb rate %dkbit ceil %dkbit prio %d burst 8k\n", wan_eth2, dscp0_rate, qos_rate, DSCP0_PRIO);//6k
		save2file(TC_FILE, "tc qdisc add dev %s parent 10:10 handle 100: pfifo limit 128\n", wan_eth2);//100
    	/* DSCP0_PRIO  Class filter */
		save2file(TC_FILE, "tc filter add dev %s parent 10:0 protocol ip prio %d handle 10 fw classid 10:10\n", wan_eth2, DSCP0_PRIO*10+10);//no prio    	
		
		/* DSCP0_PRI1 200 */
		save2file(TC_FILE, "tc class add dev %s parent 10:1 classid 10:20 htb rate %dkbit ceil %dkbit prio %d burst 8k\n", wan_eth2, dscp1_rate, qos_rate, DSCP1_PRIO);
		save2file(TC_FILE, "tc qdisc add dev %s parent 10:20 handle 200: pfifo limit 128\n", wan_eth2);
    	/* DSCP0_PRI1 Class filter */
		save2file(TC_FILE, "tc filter add dev %s parent 10:0 protocol ip prio %d handle 20 fw classid 10:20\n", wan_eth2, DSCP1_PRIO*10+10);    	
		
		/* DSCP0_PRI2 300 */
		save2file(TC_FILE, "tc class add dev %s parent 10:1 classid 10:30 htb rate %dkbit ceil %dkbit prio %d burst 8k\n", wan_eth2, dscp2_rate, qos_rate, DSCP2_PRIO);
		save2file(TC_FILE, "tc qdisc add dev %s parent 10:30 handle 300: pfifo limit 128\n", wan_eth2);
    	/* DSCP0_PRI2 Class filter */
		save2file(TC_FILE, "tc filter add dev %s parent 10:0 protocol ip prio %d handle 30 fw classid 10:30\n", wan_eth2, DSCP2_PRIO*10+10);    	
		
		/* DSCP0_PRI3 400 */
		save2file(TC_FILE, "tc class add dev %s parent 10:1 classid 10:40 htb rate %dkbit ceil %dkbit prio %d burst 8k\n", wan_eth2, dscp3_rate, qos_rate, DSCP3_PRIO);
		save2file(TC_FILE, "tc qdisc add dev %s parent 10:40 handle 400: pfifo limit 128\n", wan_eth2);
    	/* DSCP0_PRI3 Class filter */
		save2file(TC_FILE, "tc filter add dev %s parent 10:0 protocol ip prio %d handle 40 fw classid 10:40\n", wan_eth2, DSCP3_PRIO*10+10);
				    	
		/* DSCP0_PRI4 500 */
		save2file(TC_FILE, "tc class add dev %s parent 10:1 classid 10:50 htb rate %dkbit ceil %dkbit prio %d burst 8k\n", wan_eth2, dscp4_rate, qos_rate, DSCP4_PRIO);
		save2file(TC_FILE, "tc qdisc add dev %s parent 10:50 handle 500: pfifo limit 128\n", wan_eth2);
		/* DSCP0_PRI4 Class filter */
		save2file(TC_FILE, "tc filter add dev %s parent 10:0 protocol ip prio %d handle 50 fw classid 10:50\n", wan_eth2, DSCP4_PRIO*10+10);    	
		
		/* DSCP0_PRI5 600 */
		save2file(TC_FILE, "tc class add dev %s parent 10:1 classid 10:60 htb rate %dkbit ceil %dkbit prio %d burst 8k\n", wan_eth2, dscp5_rate, qos_rate, DSCP5_PRIO);
		save2file(TC_FILE, "tc qdisc add dev %s parent 10:60 handle 600: pfifo limit 128\n", wan_eth2);
		/* DSCP0_PRI5 Class filter */
		save2file(TC_FILE, "tc filter add dev %s parent 10:0 protocol ip prio %d handle 60 fw classid 10:60\n", wan_eth2, DSCP5_PRIO*10+10);
		
		/* DSCP0_PRI6 700 */
		save2file(TC_FILE, "tc class add dev %s parent 10:1 classid 10:70 htb rate %dkbit ceil %dkbit prio %d burst 8k\n", wan_eth2, dscp6_rate, qos_rate, DSCP6_PRIO);
		save2file(TC_FILE, "tc qdisc add dev %s parent 10:70 handle 700: pfifo limit 128\n", wan_eth2);
		/* DSCP0_PRI6 Class filter */
		save2file(TC_FILE, "tc filter add dev %s parent 10:0 protocol ip prio %d handle 70 fw classid 10:70\n", wan_eth2, DSCP6_PRIO*10+10);
		
		/* DSCP0_PRI7 800 */
		save2file(TC_FILE, "tc class add dev %s parent 10:1 classid 10:80 htb rate %dkbit ceil %dkbit prio %d burst 8k\n", wan_eth2, dscp7_rate, qos_rate, DSCP7_PRIO);
		save2file(TC_FILE, "tc qdisc add dev %s parent 10:80 handle 800: pfifo limit 128\n", wan_eth2);
		/* DSCP0_PRI7 Class  filter */
		save2file(TC_FILE, "tc filter add dev %s parent 10:0 protocol ip prio %d handle 80 fw classid 10:80\n", wan_eth2, DSCP7_PRIO*10+10);
	}	
	/* DSCP0_PRIO 100 */
	//save2file(TC_FILE, "tc filter add dev %s parent 10:0 protocol ip prio %d u32 match ip tos 0xe0 0xff flowid 10:10\n", wan_eth, DSCP0_PRIO);
	/* DSCP1_PRIO 200 */
	//save2file(TC_FILE, "tc filter add dev %s parent 10:0 protocol ip prio %d u32 match ip tos 0xc0 0xff flowid 10:20\n", wan_eth, DSCP1_PRIO);
	/* DSCP2_PRIO 300 */
	//save2file(TC_FILE, "tc filter add dev %s parent 10:0 protocol ip prio %d u32 match ip tos 0xa0 0xff flowid 10:30\n", wan_eth, DSCP2_PRIO);
	/* DSCP3_PRIO 400 */
	//save2file(TC_FILE, "tc filter add dev %s parent 10:0 protocol ip prio %d u32 match ip tos 0x80 0xff flowid 10:40\n", wan_eth, DSCP3_PRIO);
	/* DSCP4_PRIO 500 */
	//save2file(TC_FILE, "tc filter add dev %s parent 10:0 protocol ip prio %d u32 match ip tos 0x60 0xff flowid 10:50\n", wan_eth, DSCP4_PRIO);
	/* DSCP5_PRIO 600 */
	//save2file(TC_FILE, "tc filter add dev %s parent 10:0 protocol ip prio %d u32 match ip tos 0x40 0xff flowid 10:60\n", wan_eth, DSCP5_PRIO);
	/* DSCP6_PRIO 700 */
	//save2file(TC_FILE, "tc filter add dev %s parent 10:0 protocol ip prio %d u32 match ip tos 0x20 0xff flowid 10:70\n", wan_eth, DSCP6_PRIO);
	
	if( nvram_match("qos_enable", "1")==0 ){
		DEBUG_MSG("qos_enable\n");
			
		/* iptables rule */
		init_file(FW_MANGLE);
		save2file(FW_MANGLE,
			"*mangle\n"
			":PREROUTING ACCEPT [0:0]\n"
			":INPUT ACCEPT [0:0]\n"
			":FORWARD ACCEPT [0:0]\n"
			":OUTPUT ACCEPT [0:0]\n"
			":POSTROUTING ACCEPT [0:0]\n");		
		
		
		if( nvram_match("auto_classification", "1")==0 ){
			DEBUG_MSG("auto_class\n");						     	
			/* DSCP7_PRIO 800 */
			
			/* DSCP6_PRIO 700 */
			save2file(TC_FILE, "tc filter add dev %s parent 10:0 protocol ip prio %d u32 match ip tos 0x20 0xff flowid 10:70\n", wan_eth, DSCP6_PRIO);
			//save2file(FW_MANGLE, "-A PREROUTING -i %s -m dscp --dscp 0x8 -j MARK --set-mark %d \n", lan_eth , 70 );/* DSCP pri1 */
									        	
			/* DSCP5_PRIO 600 */
			save2file(TC_FILE, "tc filter add dev %s parent 10:0 protocol ip prio %d u32 match ip tos 0x40 0xff flowid 10:60\n", wan_eth, DSCP5_PRIO);
			//save2file(FW_MANGLE, "-A PREROUTING -i %s -m dscp --dscp 0x10 -j MARK --set-mark %d \n", lan_eth , 60 );/* DSCP pri2 */
											
			/* DSCP4_PRIO 500 */
			save2file(TC_FILE, "tc filter add dev %s parent 10:0 protocol ip prio %d u32 match ip tos 0x60 0xff flowid 10:50\n", wan_eth, DSCP4_PRIO);
			//save2file(FW_MANGLE, "-A PREROUTING -i %s -m dscp --dscp 0x18 -j MARK --set-mark %d \n", lan_eth , 50 );/* DSCP pri3 */	
									
			/* DSCP3_PRIO 400 */
			save2file(TC_FILE, "tc filter add dev %s parent 10:0 protocol ip prio %d u32 match ip tos 0x80 0xff flowid 10:40\n", wan_eth, DSCP3_PRIO);
			//save2file(FW_MANGLE, "-A PREROUTING -i %s -m dscp --dscp 0x20 -j MARK --set-mark %d \n", lan_eth , 40 );/* DSCP pri4 */	
			/* TCP Well Known Ports */
			save2file(FW_MANGLE, "-A PREROUTING -i %s -p tcp --sport 0:1024 -j MARK --set-mark %d \n", lan_eth , 40 );
			save2file(FW_MANGLE, "-A PREROUTING -i %s -p tcp --dport 0:1024 -j MARK --set-mark %d \n", lan_eth , 40 );
			/* UDP Well Known Ports */
			//save2file(FW_MANGLE, "-A PREROUTING -i %s -p udp --sport 0:1024 -j MARK --set-mark %d \n", lan_eth , 40 );
			//save2file(FW_MANGLE, "-A PREROUTING -i %s -p udp --dport 0:1024 -j MARK --set-mark %d \n", lan_eth , 40 );
			/* ICMP */
			save2file(FW_MANGLE, "-A PREROUTING -i %s -p icmp -j MARK --set-mark %d \n", lan_eth , 40 );/* ICMP */
			
			/* DSCP2_PRIO 300 */
			save2file(TC_FILE, "tc filter add dev %s parent 10:0 protocol ip prio %d u32 match ip tos 0xa0 0xff flowid 10:30\n", wan_eth, DSCP2_PRIO);
			//save2file(FW_MANGLE, "-A PREROUTING -i %s -m dscp --dscp 0x28 -j MARK --set-mark %d \n", lan_eth , 30 );/* DSCP pri5 */	
			/* TCP length<128 */			
			save2file(FW_MANGLE, "-A PREROUTING -i %s -p tcp -m length --length 0:127 -j MARK --set-mark %d \n", lan_eth , 30 );
			
			/* DSCP1_PRIO 200 */
			save2file(TC_FILE, "tc filter add dev %s parent 10:0 protocol ip prio %d u32 match ip tos 0xc0 0xff flowid 10:20\n", wan_eth, DSCP1_PRIO);
			//save2file(FW_MANGLE, "-A PREROUTING -i %s -m dscp --dscp 0x30 -j MARK --set-mark %d \n", lan_eth , 20 );/* DSCP pri6 */	
			/* UDP */		
			save2file(FW_MANGLE, "-A PREROUTING -i %s -p udp -j MARK --set-mark %d \n", lan_eth , 20 );
			
			/* DSCP0_PRIO 100 */
			save2file(TC_FILE, "tc filter add dev %s parent 10:0 protocol ip prio %d u32 match ip tos 0xe0 0xff flowid 10:10\n", wan_eth, DSCP0_PRIO);
			//save2file(FW_MANGLE, "-A PREROUTING -i %s -m dscp --dscp 0x38 -j MARK --set-mark %d \n", lan_eth , 10 );/* DSCP pri7 */
			if(wan_eth2){
				DEBUG_MSG("wan_eth2=%s\n",wan_eth2);
				/* DSCP7_PRIO 800 */
			
				/* DSCP6_PRIO 700 */
				save2file(TC_FILE, "tc filter add dev %s parent 10:0 protocol ip prio %d u32 match ip tos 0x20 0xff flowid 10:70\n", wan_eth2, DSCP6_PRIO);
				//save2file(FW_MANGLE, "-A PREROUTING -i %s -m dscp --dscp 0x8 -j MARK --set-mark %d \n", lan_eth , 70 );/* DSCP pri1 */
										        	
				/* DSCP5_PRIO 600 */
				save2file(TC_FILE, "tc filter add dev %s parent 10:0 protocol ip prio %d u32 match ip tos 0x40 0xff flowid 10:60\n", wan_eth2, DSCP5_PRIO);
				//save2file(FW_MANGLE, "-A PREROUTING -i %s -m dscp --dscp 0x10 -j MARK --set-mark %d \n", lan_eth , 60 );/* DSCP pri2 */
												
				/* DSCP4_PRIO 500 */
				save2file(TC_FILE, "tc filter add dev %s parent 10:0 protocol ip prio %d u32 match ip tos 0x60 0xff flowid 10:50\n", wan_eth2, DSCP4_PRIO);
				//save2file(FW_MANGLE, "-A PREROUTING -i %s -m dscp --dscp 0x18 -j MARK --set-mark %d \n", lan_eth , 50 );/* DSCP pri3 */	
										
				/* DSCP3_PRIO 400 */
				save2file(TC_FILE, "tc filter add dev %s parent 10:0 protocol ip prio %d u32 match ip tos 0x80 0xff flowid 10:40\n", wan_eth2, DSCP3_PRIO);
				//save2file(FW_MANGLE, "-A PREROUTING -i %s -m dscp --dscp 0x20 -j MARK --set-mark %d \n", lan_eth , 40 );/* DSCP pri4 */	
				/* TCP Well Known Ports */
				//save2file(FW_MANGLE, "-A PREROUTING -i %s -p tcp --sport 0:1024 -j MARK --set-mark %d \n", lan_eth , 40 );
				//save2file(FW_MANGLE, "-A PREROUTING -i %s -p tcp --dport 0:1024 -j MARK --set-mark %d \n", lan_eth , 40 );
				/* UDP Well Known Ports */
				//save2file(FW_MANGLE, "-A PREROUTING -i %s -p udp --sport 0:1024 -j MARK --set-mark %d \n", lan_eth , 40 );
				//save2file(FW_MANGLE, "-A PREROUTING -i %s -p udp --dport 0:1024 -j MARK --set-mark %d \n", lan_eth , 40 );
				/* ICMP */
				//save2file(FW_MANGLE, "-A PREROUTING -i %s -p icmp -j MARK --set-mark %d \n", lan_eth , 40 );/* ICMP */
				
				/* DSCP2_PRIO 300 */
				save2file(TC_FILE, "tc filter add dev %s parent 10:0 protocol ip prio %d u32 match ip tos 0xa0 0xff flowid 10:30\n", wan_eth2, DSCP2_PRIO);
				//save2file(FW_MANGLE, "-A PREROUTING -i %s -m dscp --dscp 0x28 -j MARK --set-mark %d \n", lan_eth , 30 );/* DSCP pri5 */	
				/* TCP length<128 */			
				//save2file(FW_MANGLE, "-A PREROUTING -i %s -p tcp -m length --length 0:127 -j MARK --set-mark %d \n", lan_eth , 30 );
				
				/* DSCP1_PRIO 200 */
				save2file(TC_FILE, "tc filter add dev %s parent 10:0 protocol ip prio %d u32 match ip tos 0xc0 0xff flowid 10:20\n", wan_eth2, DSCP1_PRIO);
				//save2file(FW_MANGLE, "-A PREROUTING -i %s -m dscp --dscp 0x30 -j MARK --set-mark %d \n", lan_eth , 20 );/* DSCP pri6 */	
				/* UDP */		
				//save2file(FW_MANGLE, "-A PREROUTING -i %s -p udp -j MARK --set-mark %d \n", lan_eth , 20 );
				
				/* DSCP0_PRIO 100 */
				save2file(TC_FILE, "tc filter add dev %s parent 10:0 protocol ip prio %d u32 match ip tos 0xe0 0xff flowid 10:10\n", wan_eth2, DSCP0_PRIO);
				//save2file(FW_MANGLE, "-A PREROUTING -i %s -m dscp --dscp 0x38 -j MARK --set-mark %d \n", lan_eth , 10 );/* DSCP pri7 */				
			}				
			
			/* SIP and Over TLS */
			save2file(FW_MANGLE, "-A PREROUTING -i %s -p tcp -m tcp --sport 5060:5061 -j MARK --set-mark %d \n", lan_eth , 10 );
			save2file(FW_MANGLE, "-A PREROUTING -i %s -p tcp -m tcp --dport 5060:5061 -j MARK --set-mark %d \n", lan_eth , 10 );
			save2file(FW_MANGLE, "-A PREROUTING -i %s -p udp -m udp --sport 5060:5061 -j MARK --set-mark %d \n", lan_eth , 10 );
			save2file(FW_MANGLE, "-A PREROUTING -i %s -p udp -m udp --dport 5060:5061 -j MARK --set-mark %d \n", lan_eth , 10 );
			/* RP&QT RTP/RTCP */
			save2file(FW_MANGLE, "-A PREROUTING -i %s -p udp -m udp --sport 6970:7170 -j MARK --set-mark %d \n", lan_eth , 10 );
			save2file(FW_MANGLE, "-A PREROUTING -i %s -p udp -m udp --dport 6970:7170 -j MARK --set-mark %d \n", lan_eth , 10 );
			/* RP RTSP */
			save2file(FW_MANGLE, "-A PREROUTING -i %s -p tcp -m tcp --sport 7070 -j MARK --set-mark %d \n", lan_eth , 10 );
			save2file(FW_MANGLE, "-A PREROUTING -i %s -p tcp -m tcp --dport 7070 -j MARK --set-mark %d \n", lan_eth , 10 );
			//save2file(FW_MANGLE, "-A PREROUTING -i %s -p udp -m udp --sport 7070 -j MARK --set-mark %d \n", lan_eth , 10 );
			//save2file(FW_MANGLE, "-A PREROUTING -i %s -p udp -m udp --dport 7070 -j MARK --set-mark %d \n", lan_eth , 10 );
			/* MMS RTSP/RTP/RTCP */
			save2file(FW_MANGLE, "-A PREROUTING -i %s -p tcp -m tcp --sport 1755 -j MARK --set-mark %d \n", lan_eth , 10 );
			save2file(FW_MANGLE, "-A PREROUTING -i %s -p tcp -m tcp --dport 1755 -j MARK --set-mark %d \n", lan_eth , 10 );
			save2file(FW_MANGLE, "-A PREROUTING -i %s -p udp -m udp --sport 1755 -j MARK --set-mark %d \n", lan_eth , 10 );
			save2file(FW_MANGLE, "-A PREROUTING -i %s -p udp -m udp --dport 1755 -j MARK --set-mark %d \n", lan_eth , 10 );
			/* QT RTSP */
			save2file(FW_MANGLE, "-A PREROUTING -i %s -p tcp -m tcp --sport 554 -j MARK --set-mark %d \n", lan_eth , 10 );
			save2file(FW_MANGLE, "-A PREROUTING -i %s -p tcp -m tcp --dport 554 -j MARK --set-mark %d \n", lan_eth , 10 );
			save2file(FW_MANGLE, "-A PREROUTING -i %s -p udp -m udp --sport 554 -j MARK --set-mark %d \n", lan_eth , 10 );
			save2file(FW_MANGLE, "-A PREROUTING -i %s -p udp -m udp --dport 554 -j MARK --set-mark %d \n", lan_eth , 10 );	
			/* HTTP for HTTP strem (Ex.web-tv)*/
			save2file(FW_MANGLE, "-A PREROUTING -i %s -p tcp -m tcp --sport 80 -j MARK --set-mark %d \n", lan_eth , 10 );
			save2file(FW_MANGLE, "-A PREROUTING -i %s -p tcp -m tcp --dport 80 -j MARK --set-mark %d \n", lan_eth , 10 );	
		}
		
		if( nvram_match("qos_dyn_fragmentation", "1")==0 ){
			DEBUG_MSG("dyn_fragmentation\n");
			if(mssflag){
				DEBUG_MSG("mssflag\n");
				save2file(FW_MANGLE, "-A POSTROUTING -o %s -p tcp --tcp-flags SYN,RST SYN -j TCPMSS --set-mss %d\n", lan_eth, mss);
			}	
		}
		
		for(i=0; i<MAX_QOS_NUMBER; i++) { 
			snprintf(qos_rule, sizeof(qos_rule), "qos_rule_%02d",i) ;
			getValue = nvram_get(qos_rule);
			if ( getValue == NULL ||  strlen(getValue) == 0 ) 
				continue;
			else 
				strncpy(qos_value, getValue, sizeof(qos_value));
			DEBUG_MSG("qos_rule=%s\n",qos_rule);
			if( getStrtok(qos_value, "/", "%s %s %s %s %s %s %s %s %s %s %s %s %s %s", &enable, &name, &prio, &uplink, &downlink, &proto, &src_ip_start, &src_ip_end, \
							&src_port_start, &src_port_end, &dst_ip_start, &dst_ip_end, &dst_port_start, &dst_port_end) !=14 )
				continue;

			if( !strcmp("1", enable) )  {
				DEBUG_MSG("qos_rule=%s enable prio=%s,proto=%s,src_port_start=%s,src_port_end=%s,dst_port_start=%s,dst_port_end=%s\n",qos_rule,prio,proto,src_port_start,src_port_end,dst_port_start,dst_port_end);
				DEBUG_MSG("src_ip_start=%s src_ip_end=%s,dst_ip_start=%s,dst_ip_end=%s\n",src_ip_start,src_ip_end,dst_ip_start,dst_ip_end);
				if(!strcmp(proto, "256")){//Any
					DEBUG_MSG("Any\n");
					sprintf(qos_cmd, "-p all");					
					if( src_ip_start || dst_ip_start) {						
						if( src_ip_end && dst_ip_end && strcmp(src_ip_start, "0.0.0.0")!=0 && strcmp(src_ip_end, "255.255.255.255")!=0 
							&& strcmp(dst_ip_start, "0.0.0.0")!=0 && strcmp(dst_ip_end, "255.255.255.255")!=0 ){
							sprintf(qos_cmd, "%s -m iprange", qos_cmd);
							sprintf(qos_cmd, "%s --src-range %s-%s", qos_cmd, src_ip_start, src_ip_end );
							sprintf(qos_cmd, "%s --dst-range %s-%s", qos_cmd, dst_ip_start, dst_ip_end );
						}
						else if( src_ip_end && dst_ip_end && strcmp(src_ip_start, "0.0.0.0")!=0 && strcmp(src_ip_end, "255.255.255.255")!=0
							&& strcmp(dst_ip_start, "0.0.0.0")==0 && strcmp(dst_ip_end, "255.255.255.255")==0 ){
							sprintf(qos_cmd, "%s --dst any/0", qos_cmd );
							sprintf(qos_cmd, "%s -m iprange", qos_cmd);
							sprintf(qos_cmd, "%s --src-range %s-%s", qos_cmd, src_ip_start, src_ip_end );			
						}
						else if( src_ip_end && dst_ip_end && strcmp(src_ip_start, "0.0.0.0")==0 && strcmp(src_ip_end, "255.255.255.255")==0
							&& strcmp(dst_ip_start, "0.0.0.0")!=0 && strcmp(dst_ip_end, "255.255.255.255")!=0 ){
							sprintf(qos_cmd, "%s --src any/0", qos_cmd );
							sprintf(qos_cmd, "%s -m iprange", qos_cmd);
							sprintf(qos_cmd, "%s --dst-range %s-%s", qos_cmd, dst_ip_start, dst_ip_end );							
						}
						else if( src_ip_end && dst_ip_end && strcmp(src_ip_start, "0.0.0.0")==0 && strcmp(src_ip_end, "255.255.255.255")==0
							&& strcmp(dst_ip_start, "0.0.0.0")==0 && strcmp(dst_ip_end, "255.255.255.255")==0 ){
							sprintf(qos_cmd, "%s --src any/0", qos_cmd );
							sprintf(qos_cmd, "%s --dst any/0", qos_cmd );
						}						
					}					
					DEBUG_MSG("qos_cmd=%s \n",qos_cmd);					
					save2file(FW_MANGLE, "-A PREROUTING -i %s %s -j MARK --set-mark %d \n", lan_eth, qos_cmd, atoi(prio)*10 );
					//save2file(FW_MANGLE, "-A OUTPUT %s -j MARK --set-mark %d \n", qos_cmd, atoi(prio)*10 );					
				}
				else if(!strcmp(proto, "257")){//Both
					DEBUG_MSG("Both\n");
					sprintf(qos_cmd, "-p tcp -m tcp");	
					sprintf(qos_cmd2, "-p udp -m udp");										
					if( src_port_start && src_port_end){
							sprintf(qos_cmd, "%s --sport %s:%s", qos_cmd, src_port_start, src_port_end);	
							sprintf(qos_cmd2, "%s --sport %s:%s", qos_cmd2, src_port_start, src_port_end);													
					}					
					if( dst_port_start && dst_port_end){
							sprintf(qos_cmd, "%s --dport %s:%s", qos_cmd, dst_port_start, dst_port_end);							
							sprintf(qos_cmd2, "%s --dport %s:%s", qos_cmd2, dst_port_start, dst_port_end);							
					}
					if( src_ip_start || dst_ip_start) {						
						if( src_ip_end && dst_ip_end && strcmp(src_ip_start, "0.0.0.0")!=0 && strcmp(src_ip_end, "255.255.255.255")!=0 
							&& strcmp(dst_ip_start, "0.0.0.0")!=0 && strcmp(dst_ip_end, "255.255.255.255")!=0 ){
							sprintf(qos_cmd, "%s -m iprange", qos_cmd);
							sprintf(qos_cmd, "%s --src-range %s-%s", qos_cmd, src_ip_start, src_ip_end );
							sprintf(qos_cmd, "%s --dst-range %s-%s", qos_cmd, dst_ip_start, dst_ip_end );
							sprintf(qos_cmd2, "%s -m iprange", qos_cmd2);
							sprintf(qos_cmd2, "%s --src-range %s-%s", qos_cmd2, src_ip_start, src_ip_end );
							sprintf(qos_cmd2, "%s --dst-range %s-%s", qos_cmd2, dst_ip_start, dst_ip_end );
						}
						else if( src_ip_end && dst_ip_end && strcmp(src_ip_start, "0.0.0.0")!=0 && strcmp(src_ip_end, "255.255.255.255")!=0
							&& strcmp(dst_ip_start, "0.0.0.0")==0 && strcmp(dst_ip_end, "255.255.255.255")==0 ){
							sprintf(qos_cmd, "%s --dst any/0", qos_cmd );
							sprintf(qos_cmd, "%s -m iprange", qos_cmd);
							sprintf(qos_cmd, "%s --src-range %s-%s", qos_cmd, src_ip_start, src_ip_end );
							sprintf(qos_cmd2, "%s --dst any/0", qos_cmd2 );
							sprintf(qos_cmd2, "%s -m iprange", qos_cmd2);
							sprintf(qos_cmd2, "%s --src-range %s-%s", qos_cmd2, src_ip_start, src_ip_end );				
						}
						else if( src_ip_end && dst_ip_end && strcmp(src_ip_start, "0.0.0.0")==0 && strcmp(src_ip_end, "255.255.255.255")==0
							&& strcmp(dst_ip_start, "0.0.0.0")!=0 && strcmp(dst_ip_end, "255.255.255.255")!=0 ){
							sprintf(qos_cmd, "%s --src any/0", qos_cmd );
							sprintf(qos_cmd, "%s -m iprange", qos_cmd);
							sprintf(qos_cmd, "%s --dst-range %s-%s", qos_cmd, dst_ip_start, dst_ip_end );							
							sprintf(qos_cmd2, "%s --src any/0", qos_cmd2 );
							sprintf(qos_cmd2, "%s -m iprange", qos_cmd2);
							sprintf(qos_cmd2, "%s --dst-range %s-%s", qos_cmd2, dst_ip_start, dst_ip_end );							
						}
						else if( src_ip_end && dst_ip_end && strcmp(src_ip_start, "0.0.0.0")==0 && strcmp(src_ip_end, "255.255.255.255")==0
							&& strcmp(dst_ip_start, "0.0.0.0")==0 && strcmp(dst_ip_end, "255.255.255.255")==0 ){
							sprintf(qos_cmd, "%s --src any/0 ", qos_cmd );
							sprintf(qos_cmd2, "%s --dst any/0 ", qos_cmd2 );
						}						
					}
					
					DEBUG_MSG("qos_cmd=%s \n",qos_cmd);	
					DEBUG_MSG("qos_cmd2=%s \n",qos_cmd2);				
					save2file(FW_MANGLE, "-A PREROUTING -i %s %s -j MARK --set-mark %d \n", lan_eth, qos_cmd, atoi(prio)*10 );
					//save2file(FW_MANGLE, "-A OUTPUT %s -j MARK --set-mark %d \n", qos_cmd, atoi(prio)*10 );
					save2file(FW_MANGLE, "-A PREROUTING -i %s %s -j MARK --set-mark %d \n", lan_eth, qos_cmd2, atoi(prio)*10 );
					//save2file(FW_MANGLE, "-A OUTPUT %s -j MARK --set-mark %d \n", qos_cmd2, atoi(prio)*10 );									
				}
				else{//TCP,UDP,ICMP,Other		
					DEBUG_MSG("TCP,UDP,ICMP,Other\n");
					sprintf(qos_cmd, "-p %s", proto);
					if( !strcmp(proto, "6") || !strcmp(proto, "17") ){	/* ports only for UTP or TCP */
						if( src_port_start && src_port_end)
							sprintf(qos_cmd, "%s --sport %s:%s", qos_cmd, src_port_start, src_port_end);
						if( dst_port_start && dst_port_end)
							sprintf(qos_cmd, "%s --dport %s:%s", qos_cmd, dst_port_start, dst_port_end);
					}
					if( src_ip_start || dst_ip_start) {						
						if( src_ip_end && dst_ip_end && strcmp(src_ip_start, "0.0.0.0")!=0 && strcmp(src_ip_end, "255.255.255.255")!=0 
							&& strcmp(dst_ip_start, "0.0.0.0")!=0 && strcmp(dst_ip_end, "255.255.255.255")!=0 ){
							sprintf(qos_cmd, "%s -m iprange", qos_cmd);
							sprintf(qos_cmd, "%s --src-range %s-%s", qos_cmd, src_ip_start, src_ip_end );
							sprintf(qos_cmd, "%s --dst-range %s-%s", qos_cmd, dst_ip_start, dst_ip_end );
						}
						else if( src_ip_end && dst_ip_end && strcmp(src_ip_start, "0.0.0.0")!=0 && strcmp(src_ip_end, "255.255.255.255")!=0
							&& strcmp(dst_ip_start, "0.0.0.0")==0 && strcmp(dst_ip_end, "255.255.255.255")==0 ){
							sprintf(qos_cmd, "%s --dst any/0", qos_cmd );
							sprintf(qos_cmd, "%s -m iprange", qos_cmd);
							sprintf(qos_cmd, "%s --src-range %s-%s", qos_cmd, src_ip_start, src_ip_end );			
						}
						else if( src_ip_end && dst_ip_end && strcmp(src_ip_start, "0.0.0.0")==0 && strcmp(src_ip_end, "255.255.255.255")==0
							&& strcmp(dst_ip_start, "0.0.0.0")!=0 && strcmp(dst_ip_end, "255.255.255.255")!=0 ){
							sprintf(qos_cmd, "%s --src any/0", qos_cmd );
							sprintf(qos_cmd, "%s -m iprange", qos_cmd);
							sprintf(qos_cmd, "%s --dst-range %s-%s", qos_cmd, dst_ip_start, dst_ip_end );							
						}
						else if( src_ip_end && dst_ip_end && strcmp(src_ip_start, "0.0.0.0")==0 && strcmp(src_ip_end, "255.255.255.255")==0
							&& strcmp(dst_ip_start, "0.0.0.0")==0 && strcmp(dst_ip_end, "255.255.255.255")==0 ){
							sprintf(qos_cmd, "%s --src any/0", qos_cmd );
							sprintf(qos_cmd, "%s --dst any/0", qos_cmd );
						}						
					}
					
					DEBUG_MSG("qos_cmd=%s,mask=%d \n",qos_cmd, atoi(prio)*10);
					save2file(FW_MANGLE, "-A PREROUTING -i %s %s -j MARK --set-mark %d \n", lan_eth, qos_cmd, atoi(prio)*10 );
					//save2file(FW_MANGLE, "-A OUTPUT %s -j MARK --set-mark %d \n", qos_cmd, atoi(prio)*10 );
				}	
			}
		}	
				
		save2file(FW_MANGLE, "COMMIT\n");
		_system("iptables-restore %s", FW_MANGLE);
		unlink(FW_MANGLE);		
	}	
#endif//DOWNLINK_BANDWIDTH
	/* execute TC */
	//stop_qos();
	_system("sh %s", TC_FILE);
#endif //CONFIG_USER_STREAMENGINE
	return 0;
}

int stop_qos(void)
{
/* 
 *	Date: 2009-9-25
 *	Name: Gareth Williams <gareth.williams@ubicom.com>
 *	Reason: Streamengine support
 */ 	
#ifdef CONFIG_USER_STREAMENGINE
#ifdef CONFIG_USER_STREAMENGINE2
	_system("/etc/init.d/se_stop");
#else
	_system("/etc/init.d/rc_streamengine_stop");
#endif
#else
	char *wan_proto=NULL;
	char *wan_eth=NULL;

#ifdef	CONFIG_BRIDGE_MODE
	if(nvram_match("wlan0_mode", "ap") == 0)
	{
		system("iptables -F -t mangle");
		system("iptables -X -t mangle");
		system("iptables -Z -t mangle");
		return 1;
	}
	else
#endif
	{
		if(!( action_flags.all || action_flags.qos ))
			return 1;
	}	

#ifndef AR7161 //NickChou add, because I do not know below code.
	/* jimmy modified */
	if( nvram_match("qos_enable", "1") == 0 ){
		if( nvram_match("traffic_shaping", "0")==0 )
			return 1;
	}
	/* ----------------------- */
#endif	//#ifndef AR7161
	
	DEBUG_MSG("Clear QOS\n");
	system("iptables -F -t mangle");
	system("iptables -X -t mangle");
	system("iptables -Z -t mangle");
	
	wan_proto=nvram_safe_get("wan_proto");
	wan_eth=nvram_safe_get("wan_eth");
	
	if(( strcmp(wan_proto, "dhcpc") ==0 ) || ( strcmp(wan_proto, "static") ==0 )
#if defined(CONFIG_USER_3G_USB_CLIENT_WM5) || defined(CONFIG_USER_3G_USB_CLIENT_IPHONE)
		|| ( nvram_match("wan_proto", "usb3g_phone") ==0 )
#endif
	)
	{
		_system("tc qdisc del dev %s root", wan_eth);
	}
	else 
	{
		system("tc qdisc del dev ppp0 root");
							
		/* if Russia is enabled */
		if ((( strcmp(wan_proto, "pppoe") ==0 ) && ( nvram_match("wan_pppoe_russia_enable", "1") ==0 )) ||
			(( strcmp(wan_proto, "pptp") ==0 ) && ( nvram_match("wan_pptp_russia_enable", "1") ==0 )) ||
			(( strcmp(wan_proto, "l2tp") ==0 ) && ( nvram_match("wan_l2tp_russia_enable", "1") ==0 )) )
		{
			DEBUG_MSG("wan_eth2=%s\n",wan_eth);
			_system("tc qdisc del dev %s root", wan_eth);
		}
	}
	
#if (DOWNLINK_BANDWIDTH)				
	_system("tc qdisc del dev %s root", nvram_safe_get("lan_bridge"));
#endif	
	//if(nvram_match("auto_uplink", "1")==0){
	/* jimmy modified 20080602 */
		//stop_mbandwidth();
		KILL_APP_SYNCH("-SIGUSR1 mbandwidth");
	/* ------------------------ */
	//}
#endif // CONFIG_USER_STREAMENGINE
	return 0;
}

int change_qos_rate(int ratetype)
{	
	char *wan_eth=NULL,*wan_eth2=NULL, *lan_eth, bandwidth[32]={0};	
	unsigned int qos_rate=0;	

	printf("change_qos_rate\n");
	
	if( nvram_match("traffic_shaping", "1")!=0 ){
		printf("traffic_shaping disable\n");
		return 1;			
	}	
	printf("traffic_shaping\n");
	
	/* Check if we have wan IP already*/
	if(!wan_ip_is_obtained()){
		printf("wan no IP already\n");
		return 1;		
	}	
	printf("wan IP already\n");

/* 
 *	Date: 2009-9-25
 *	Name: Gareth Williams <gareth.williams@ubicom.com>
 *	Reason: Streamengine support
 */ 	
#ifdef CONFIG_USER_STREAMENGINE
#ifdef CONFIG_USER_STREAMENGINE2
	_system("/etc/init.d/se_restart");
#else
	_system("/etc/init.d/rc_streamengine_restart");
#endif
#else
	if (( nvram_match("wan_proto", "dhcpc") ==0 ) || ( nvram_match("wan_proto", "static") ==0 )
#if defined(CONFIG_USER_3G_USB_CLIENT_WM5) || defined(CONFIG_USER_3G_USB_CLIENT_IPHONE)
		|| ( nvram_match("wan_proto", "usb3g_phone") ==0 )
#endif
	)
	{
		wan_eth = nvram_get("wan_eth");	
	}
	else 
	{
		wan_eth = "ppp0";
					
		/* if Russia is enabled */
		if ((( nvram_match("wan_proto", "pppoe") ==0 ) && ( nvram_match("wan_pppoe_russia_enable", "1") ==0 )) ||
			(( nvram_match("wan_proto", "pptp") ==0 ) && ( nvram_match("wan_pptp_russia_enable", "1") ==0 )) || 
			(( nvram_match("wan_proto", "l2tp") ==0 ) && ( nvram_match("wan_l2tp_russia_enable", "1") ==0 ))
		)
		{
			wan_eth2 = nvram_get("wan_eth");
		}
	}		
	
	lan_eth = nvram_get("lan_bridge");
	
	if(nvram_match("auto_uplink", "1")==0){
		printf("auto_uplink\n");		
			get_measure_bandwidth(bandwidth);
			qos_rate = atoi(bandwidth);					
	}
	else{//manual
		printf("maul_uplink\n");
		qos_rate = atoi( nvram_safe_get("qos_uplink"));
	}	
	
	if(!qos_rate){
		printf("qos_rate==0\n");
		return 1;
	}	
		
	printf("wan_eth=%s,lan_eth=%s,qos_rate=%d\n",wan_eth,lan_eth,qos_rate);
	
	if(qos_rate < 20)
		qos_rate = 20;
	switch (ratetype) {
			case 1:
				printf("ratetype 1\n");
				dscp1_rate = qos_rate * 0.02;
				dscp2_rate = qos_rate * 0.02;
				dscp3_rate = qos_rate * 0.01;
				dscp4_rate = qos_rate * 0.01;
				dscp5_rate = qos_rate * 0.01;
				dscp6_rate = qos_rate * 0.01;
				dscp7_rate = qos_rate * 0.01;
				break;
			case 2:
				printf("ratetype 2\n");
				dscp1_rate = qos_rate * 0.03;
				dscp2_rate = qos_rate * 0.03;
				dscp3_rate = qos_rate * 0.01;
				dscp4_rate = qos_rate * 0.01;
				dscp5_rate = qos_rate * 0.01;
				dscp6_rate = qos_rate * 0.01;
				dscp7_rate = qos_rate * 0.01;
				break;
			case 3:
				printf("ratetype 3\n");
				dscp1_rate = qos_rate * 0.04;
				dscp2_rate = qos_rate * 0.04;
				dscp3_rate = qos_rate * 0.01;
				dscp4_rate = qos_rate * 0.01;
				dscp5_rate = qos_rate * 0.01;
				dscp6_rate = qos_rate * 0.01;
				dscp7_rate = qos_rate * 0.01;
				break;
			default:
				printf("ratetype default\n");
			return 1;
				
	}			
	
	if(dscp1_rate < 5)
		dscp1_rate = 5;
	if(dscp2_rate < 5)
		dscp2_rate = 5;	
	if(dscp3_rate < 1)
		dscp3_rate = 1;
	if(dscp4_rate < 1)
		dscp4_rate = 1;
	if(dscp5_rate < 1)
		dscp5_rate = 1;
	if(dscp6_rate < 1)
		dscp6_rate = 1;
	if(dscp7_rate < 1)
		dscp7_rate = 1;
	dscp0_rate = qos_rate -dscp1_rate -dscp2_rate-dscp3_rate-dscp4_rate-dscp5_rate-dscp6_rate-dscp7_rate;	
	if(dscp0_rate < 5)
		dscp0_rate = 5;		
	
	init_file(TC_CHANGE_FILE);
	chmod(TC_CHANGE_FILE, S_IRUSR|S_IWUSR|S_IXUSR);
	//_system("tc qdisc del dev %s root", nvram_get("wan_eth"));
	//_system("tc qdisc del dev %s root", nvram_get("lan_bridge"));
	save2file(TC_CHANGE_FILE, "tc qdisc del dev %s root\n", nvram_get("wan_eth"));
	save2file(TC_CHANGE_FILE, "tc qdisc del dev %s root\n", nvram_get("lan_bridge"));
#if (DOWNLINK_BANDWIDTH)
	printf("DOWNLINK_BANDWIDTH\n");
	/* root */
	save2file(TC_CHANGE_FILE, "tc qdisc add dev %s root handle 10: htb default 80\n", lan_eth);
	save2file(TC_CHANGE_FILE, "tc class add dev %s parent 10: classid 10:1 htb rate %dkbit\n", lan_eth, qos_rate);
	
	/* DSCP0_PRIO 100 */
	save2file(TC_CHANGE_FILE, "tc class add dev %s parent 10:1 classid 10:10 htb rate %dkbit ceil %dkbit prio %d burst 8k\n", lan_eth, dscp0_rate, qos_rate, DSCP0_PRIO);//6k
	save2file(TC_CHANGE_FILE, "tc qdisc add dev %s parent 10:10 handle 100: pfifo limit 128\n", lan_eth);//100
    /* DSCP0_PRIO  Class filter */
	save2file(TC_CHANGE_FILE, "tc filter add dev %s parent 10:0 protocol ip prio %d handle 10 fw classid 10:10\n", lan_eth, DSCP0_PRIO*10+10);//no prio    	
	
	/* DSCP0_PRI1 200 */
	save2file(TC_CHANGE_FILE, "tc class add dev %s parent 10:1 classid 10:20 htb rate %dkbit ceil %dkbit prio %d burst 8k\n", lan_eth, dscp1_rate, qos_rate, DSCP1_PRIO);
	save2file(TC_CHANGE_FILE, "tc qdisc add dev %s parent 10:20 handle 200: pfifo limit 128\n", lan_eth);
    /* DSCP0_PRI1 Class filter */
	save2file(TC_CHANGE_FILE, "tc filter add dev %s parent 10:0 protocol ip prio %d handle 20 fw classid 10:20\n", lan_eth, DSCP1_PRIO*10+10);    	
	
	/* DSCP0_PRI2 300 */
	save2file(TC_CHANGE_FILE, "tc class add dev %s parent 10:1 classid 10:30 htb rate %dkbit ceil %dkbit prio %d burst 8k\n", lan_eth, dscp2_rate, qos_rate, DSCP2_PRIO);
	save2file(TC_CHANGE_FILE, "tc qdisc add dev %s parent 10:30 handle 300: pfifo limit 128\n", lan_eth);
    /* DSCP0_PRI2 Class filter */
	save2file(TC_CHANGE_FILE, "tc filter add dev %s parent 10:0 protocol ip prio %d handle 30 fw classid 10:30\n", lan_eth, DSCP2_PRIO*10+10);    	
	
	/* DSCP0_PRI3 400 */
	save2file(TC_CHANGE_FILE, "tc class add dev %s parent 10:1 classid 10:40 htb rate %dkbit ceil %dkbit prio %d burst 8k\n", lan_eth, dscp3_rate, qos_rate, DSCP3_PRIO);
	save2file(TC_CHANGE_FILE, "tc qdisc add dev %s parent 10:40 handle 400: pfifo limit 128\n", lan_eth);
    /* DSCP0_PRI3 Class filter */
	save2file(TC_CHANGE_FILE, "tc filter add dev %s parent 10:0 protocol ip prio %d handle 40 fw classid 10:40\n", lan_eth, DSCP3_PRIO*10+10);
			    	
	/* DSCP0_PRI4 500 */
	save2file(TC_CHANGE_FILE, "tc class add dev %s parent 10:1 classid 10:50 htb rate %dkbit ceil %dkbit prio %d burst 8k\n", lan_eth, dscp4_rate, qos_rate, DSCP4_PRIO);
	save2file(TC_CHANGE_FILE, "tc qdisc add dev %s parent 10:50 handle 500: pfifo limit 128\n", lan_eth);
	/* DSCP0_PRI4 Class filter */
	save2file(TC_CHANGE_FILE, "tc filter add dev %s parent 10:0 protocol ip prio %d handle 50 fw classid 10:50\n", lan_eth, DSCP4_PRIO*10+10);    	
	
	/* DSCP0_PRI5 600 */
	save2file(TC_CHANGE_FILE, "tc class add dev %s parent 10:1 classid 10:60 htb rate %dkbit ceil %dkbit prio %d burst 8k\n", lan_eth, dscp5_rate, qos_rate, DSCP5_PRIO);
	save2file(TC_CHANGE_FILE, "tc qdisc add dev %s parent 10:60 handle 600: pfifo limit 128\n", lan_eth);
	/* DSCP0_PRI5 Class filter */
	save2file(TC_CHANGE_FILE, "tc filter add dev %s parent 10:0 protocol ip prio %d handle 60 fw classid 10:60\n", lan_eth, DSCP5_PRIO*10+10);
	
	/* DSCP0_PRI6 700 */
	save2file(TC_CHANGE_FILE, "tc class add dev %s parent 10:1 classid 10:70 htb rate %dkbit ceil %dkbit prio %d burst 8k\n", lan_eth, dscp6_rate, qos_rate, DSCP6_PRIO);
	save2file(TC_CHANGE_FILE, "tc qdisc add dev %s parent 10:70 handle 700: pfifo limit 128\n", lan_eth);
	/* DSCP0_PRI6 Class filter */
	save2file(TC_CHANGE_FILE, "tc filter add dev %s parent 10:0 protocol ip prio %d handle 70 fw classid 10:70\n", lan_eth, DSCP6_PRIO*10+10);
	
	/* DSCP0_PRI7 800 */
	save2file(TC_CHANGE_FILE, "tc class add dev %s parent 10:1 classid 10:80 htb rate %dkbit ceil %dkbit prio %d burst 8k\n", lan_eth, dscp7_rate, qos_rate, DSCP7_PRIO);
	save2file(TC_CHANGE_FILE, "tc qdisc add dev %s parent 10:80 handle 800: pfifo limit 128\n", lan_eth);
	/* DSCP0_PRI7 Class  filter */
	save2file(TC_CHANGE_FILE, "tc filter add dev %s parent 10:0 protocol ip prio %d handle 80 fw classid 10:80\n", lan_eth, DSCP7_PRIO*10+10);
	
	/* DSCP0_PRIO 100 */
	//save2file(TC_CHANGE_FILE, "tc filter add dev %s parent 10:0 protocol ip prio %d u32 match ip tos 0xe0 0xff flowid 10:10\n", lan_eth, DSCP0_PRIO);
	/* DSCP1_PRIO 200 */
	//save2file(TC_CHANGE_FILE, "tc filter add dev %s parent 10:0 protocol ip prio %d u32 match ip tos 0xc0 0xff flowid 10:20\n", lan_eth, DSCP1_PRIO);
	/* DSCP2_PRIO 300 */
	//save2file(TC_CHANGE_FILE, "tc filter add dev %s parent 10:0 protocol ip prio %d u32 match ip tos 0xa0 0xff flowid 10:30\n", lan_eth, DSCP2_PRIO);
	/* DSCP3_PRIO 400 */
	//save2file(TC_CHANGE_FILE, "tc filter add dev %s parent 10:0 protocol ip prio %d u32 match ip tos 0x80 0xff flowid 10:40\n", lan_eth, DSCP3_PRIO);
	/* DSCP4_PRIO 500 */
	//save2file(TC_CHANGE_FILE, "tc filter add dev %s parent 10:0 protocol ip prio %d u32 match ip tos 0x60 0xff flowid 10:50\n", lan_eth, DSCP4_PRIO);
	/* DSCP5_PRIO 600 */
	//save2file(TC_CHANGE_FILE, "tc filter add dev %s parent 10:0 protocol ip prio %d u32 match ip tos 0x40 0xff flowid 10:60\n", lan_eth, DSCP5_PRIO);
	/* DSCP6_PRIO 700 */
	//save2file(TC_CHANGE_FILE, "tc filter add dev %s parent 10:0 protocol ip prio %d u32 match ip tos 0x20 0xff flowid 10:70\n", lan_eth, DSCP6_PRIO);
	if( nvram_match("qos_enable", "1")==0 ){
		printf("qos_enable\n");
		if( nvram_match("auto_classification", "1")==0 ){
			printf("auto_class\n");						     	
			/* DSCP7_PRIO 800 */
			
			/* DSCP6_PRIO 700 */
			save2file(TC_CHANGE_FILE, "tc filter add dev %s parent 10:0 protocol ip prio %d u32 match ip tos 0x20 0xff flowid 10:70\n", wan_eth, DSCP6_PRIO);
			//save2file(FW_MANGLE, "-A PREROUTING -i %s -m dscp --dscp 0x8 -j MARK --set-mark %d \n", lan_eth , 70 );/* DSCP pri1 */
									        	
			/* DSCP5_PRIO 600 */
			save2file(TC_CHANGE_FILE, "tc filter add dev %s parent 10:0 protocol ip prio %d u32 match ip tos 0x40 0xff flowid 10:60\n", wan_eth, DSCP5_PRIO);
			//save2file(FW_MANGLE, "-A PREROUTING -i %s -m dscp --dscp 0x10 -j MARK --set-mark %d \n", lan_eth , 60 );/* DSCP pri2 */
											
			/* DSCP4_PRIO 500 */
			save2file(TC_CHANGE_FILE, "tc filter add dev %s parent 10:0 protocol ip prio %d u32 match ip tos 0x60 0xff flowid 10:50\n", wan_eth, DSCP4_PRIO);
			//save2file(FW_MANGLE, "-A PREROUTING -i %s -m dscp --dscp 0x18 -j MARK --set-mark %d \n", lan_eth , 50 );/* DSCP pri3 */	
									
			/* DSCP3_PRIO 400 */
			save2file(TC_CHANGE_FILE, "tc filter add dev %s parent 10:0 protocol ip prio %d u32 match ip tos 0x80 0xff flowid 10:40\n", wan_eth, DSCP3_PRIO);
			//save2file(FW_MANGLE, "-A PREROUTING -i %s -m dscp --dscp 0x20 -j MARK --set-mark %d \n", lan_eth , 40 );/* DSCP pri4 */	
						
			/* DSCP2_PRIO 300 */
			save2file(TC_CHANGE_FILE, "tc filter add dev %s parent 10:0 protocol ip prio %d u32 match ip tos 0xa0 0xff flowid 10:30\n", wan_eth, DSCP2_PRIO);
			//save2file(FW_MANGLE, "-A PREROUTING -i %s -m dscp --dscp 0x28 -j MARK --set-mark %d \n", lan_eth , 30 );/* DSCP pri5 */	
						
			/* DSCP1_PRIO 200 */
			save2file(TC_CHANGE_FILE, "tc filter add dev %s parent 10:0 protocol ip prio %d u32 match ip tos 0xc0 0xff flowid 10:20\n", wan_eth, DSCP1_PRIO);
			//save2file(FW_MANGLE, "-A PREROUTING -i %s -m dscp --dscp 0x30 -j MARK --set-mark %d \n", lan_eth , 20 );/* DSCP pri6 */	
						
			/* DSCP0_PRIO 100 */
			save2file(TC_CHANGE_FILE, "tc filter add dev %s parent 10:0 protocol ip prio %d u32 match ip tos 0xe0 0xff flowid 10:10\n", wan_eth, DSCP0_PRIO);
			//save2file(FW_MANGLE, "-A PREROUTING -i %s -m dscp --dscp 0x38 -j MARK --set-mark %d \n", lan_eth , 10 );/* DSCP pri7 */				
		}
	}

#else//DOWNLINK_BANDWIDTH
	printf("UPLINK_BANDWIDTH\n");	
	/* root */
	save2file(TC_CHANGE_FILE, "tc qdisc add dev %s root handle 10: htb default 80\n", wan_eth);
	save2file(TC_CHANGE_FILE, "tc class add dev %s parent 10: classid 10:1 htb rate %dkbit\n", wan_eth, qos_rate);
	
	/* DSCP0_PRIO 100 */
	save2file(TC_CHANGE_FILE, "tc class add dev %s parent 10:1 classid 10:10 htb rate %dkbit ceil %dkbit prio %d burst 8k\n", wan_eth, dscp0_rate, qos_rate, DSCP0_PRIO);//6k
	save2file(TC_CHANGE_FILE, "tc qdisc add dev %s parent 10:10 handle 100: pfifo limit 128\n", wan_eth);//100
    /* DSCP0_PRIO  Class filter */
	save2file(TC_CHANGE_FILE, "tc filter add dev %s parent 10:0 protocol ip prio %d handle 10 fw classid 10:10\n", wan_eth, DSCP0_PRIO*10+10);//no prio    	
	
	/* DSCP0_PRI1 200 */
	save2file(TC_CHANGE_FILE, "tc class add dev %s parent 10:1 classid 10:20 htb rate %dkbit ceil %dkbit prio %d burst 8k\n", wan_eth, dscp1_rate, qos_rate, DSCP1_PRIO);
	save2file(TC_CHANGE_FILE, "tc qdisc add dev %s parent 10:20 handle 200: pfifo limit 128\n", wan_eth);
    /* DSCP0_PRI1 Class filter */
	save2file(TC_CHANGE_FILE, "tc filter add dev %s parent 10:0 protocol ip prio %d handle 20 fw classid 10:20\n", wan_eth, DSCP1_PRIO*10+10);    	
	
	/* DSCP0_PRI2 300 */
	save2file(TC_CHANGE_FILE, "tc class add dev %s parent 10:1 classid 10:30 htb rate %dkbit ceil %dkbit prio %d burst 8k\n", wan_eth, dscp2_rate, qos_rate, DSCP2_PRIO);
	save2file(TC_CHANGE_FILE, "tc qdisc add dev %s parent 10:30 handle 300: pfifo limit 128\n", wan_eth);
    /* DSCP0_PRI2 Class filter */
	save2file(TC_CHANGE_FILE, "tc filter add dev %s parent 10:0 protocol ip prio %d handle 30 fw classid 10:30\n", wan_eth, DSCP2_PRIO*10+10);    	
	
	/* DSCP0_PRI3 400 */
	save2file(TC_CHANGE_FILE, "tc class add dev %s parent 10:1 classid 10:40 htb rate %dkbit ceil %dkbit prio %d burst 8k\n", wan_eth, dscp3_rate, qos_rate, DSCP3_PRIO);
	save2file(TC_CHANGE_FILE, "tc qdisc add dev %s parent 10:40 handle 400: pfifo limit 128\n", wan_eth);
    /* DSCP0_PRI3 Class filter */
	save2file(TC_CHANGE_FILE, "tc filter add dev %s parent 10:0 protocol ip prio %d handle 40 fw classid 10:40\n", wan_eth, DSCP3_PRIO*10+10);
			    	
	/* DSCP0_PRI4 500 */
	save2file(TC_CHANGE_FILE, "tc class add dev %s parent 10:1 classid 10:50 htb rate %dkbit ceil %dkbit prio %d burst 8k\n", wan_eth, dscp4_rate, qos_rate, DSCP4_PRIO);
	save2file(TC_CHANGE_FILE, "tc qdisc add dev %s parent 10:50 handle 500: pfifo limit 128\n", wan_eth);
	/* DSCP0_PRI4 Class filter */
	save2file(TC_CHANGE_FILE, "tc filter add dev %s parent 10:0 protocol ip prio %d handle 50 fw classid 10:50\n", wan_eth, DSCP4_PRIO*10+10);    	
	
	/* DSCP0_PRI5 600 */
	save2file(TC_CHANGE_FILE, "tc class add dev %s parent 10:1 classid 10:60 htb rate %dkbit ceil %dkbit prio %d burst 8k\n", wan_eth, dscp5_rate, qos_rate, DSCP5_PRIO);
	save2file(TC_CHANGE_FILE, "tc qdisc add dev %s parent 10:60 handle 600: pfifo limit 128\n", wan_eth);
	/* DSCP0_PRI5 Class filter */
	save2file(TC_CHANGE_FILE, "tc filter add dev %s parent 10:0 protocol ip prio %d handle 60 fw classid 10:60\n", wan_eth, DSCP5_PRIO*10+10);
	
	/* DSCP0_PRI6 700 */
	save2file(TC_CHANGE_FILE, "tc class add dev %s parent 10:1 classid 10:70 htb rate %dkbit ceil %dkbit prio %d burst 8k\n", wan_eth, dscp6_rate, qos_rate, DSCP6_PRIO);
	save2file(TC_CHANGE_FILE, "tc qdisc add dev %s parent 10:70 handle 700: pfifo limit 128\n", wan_eth);
	/* DSCP0_PRI6 Class filter */
	save2file(TC_CHANGE_FILE, "tc filter add dev %s parent 10:0 protocol ip prio %d handle 70 fw classid 10:70\n", wan_eth, DSCP6_PRIO*10+10);
	
	/* DSCP0_PRI7 800 */
	save2file(TC_CHANGE_FILE, "tc class add dev %s parent 10:1 classid 10:80 htb rate %dkbit ceil %dkbit prio %d burst 8k\n", wan_eth, dscp7_rate, qos_rate, DSCP7_PRIO);
	save2file(TC_CHANGE_FILE, "tc qdisc add dev %s parent 10:80 handle 800: pfifo limit 128\n", wan_eth);
	/* DSCP0_PRI7 Class  filter */
	save2file(TC_CHANGE_FILE, "tc filter add dev %s parent 10:0 protocol ip prio %d handle 80 fw classid 10:80\n", wan_eth, DSCP7_PRIO*10+10);
	
	/* DSCP0_PRIO 100 */
	//save2file(TC_CHANGE_FILE, "tc filter add dev %s parent 10:0 protocol ip prio %d u32 match ip tos 0xe0 0xff flowid 10:10\n", wan_eth, DSCP0_PRIO);
	/* DSCP1_PRIO 200 */
	//save2file(TC_CHANGE_FILE, "tc filter add dev %s parent 10:0 protocol ip prio %d u32 match ip tos 0xc0 0xff flowid 10:20\n", wan_eth, DSCP1_PRIO);
	/* DSCP2_PRIO 300 */
	//save2file(TC_CHANGE_FILE, "tc filter add dev %s parent 10:0 protocol ip prio %d u32 match ip tos 0xa0 0xff flowid 10:30\n", wan_eth, DSCP2_PRIO);
	/* DSCP3_PRIO 400 */
	//save2file(TC_CHANGE_FILE, "tc filter add dev %s parent 10:0 protocol ip prio %d u32 match ip tos 0x80 0xff flowid 10:40\n", wan_eth, DSCP3_PRIO);
	/* DSCP4_PRIO 500 */
	//save2file(TC_CHANGE_FILE, "tc filter add dev %s parent 10:0 protocol ip prio %d u32 match ip tos 0x60 0xff flowid 10:50\n", wan_eth, DSCP4_PRIO);
	/* DSCP5_PRIO 600 */
	//save2file(TC_CHANGE_FILE, "tc filter add dev %s parent 10:0 protocol ip prio %d u32 match ip tos 0x40 0xff flowid 10:60\n", wan_eth, DSCP5_PRIO);
	/* DSCP6_PRIO 700 */
	//save2file(TC_CHANGE_FILE, "tc filter add dev %s parent 10:0 protocol ip prio %d u32 match ip tos 0x20 0xff flowid 10:70\n", wan_eth, DSCP6_PRIO);
	
	if( nvram_match("qos_enable", "1")==0 ){
		printf("qos_enable\n");
		if( nvram_match("auto_classification", "1")==0 ){
			printf("auto_class\n");						     	
			/* DSCP7_PRIO 800 */
			
			/* DSCP6_PRIO 700 */
			save2file(TC_CHANGE_FILE, "tc filter add dev %s parent 10:0 protocol ip prio %d u32 match ip tos 0x20 0xff flowid 10:70\n", wan_eth, DSCP6_PRIO);
			//save2file(FW_MANGLE, "-A PREROUTING -i %s -m dscp --dscp 0x8 -j MARK --set-mark %d \n", lan_eth , 70 );/* DSCP pri1 */
									        	
			/* DSCP5_PRIO 600 */
			save2file(TC_CHANGE_FILE, "tc filter add dev %s parent 10:0 protocol ip prio %d u32 match ip tos 0x40 0xff flowid 10:60\n", wan_eth, DSCP5_PRIO);
			//save2file(FW_MANGLE, "-A PREROUTING -i %s -m dscp --dscp 0x10 -j MARK --set-mark %d \n", lan_eth , 60 );/* DSCP pri2 */
											
			/* DSCP4_PRIO 500 */
			save2file(TC_CHANGE_FILE, "tc filter add dev %s parent 10:0 protocol ip prio %d u32 match ip tos 0x60 0xff flowid 10:50\n", wan_eth, DSCP4_PRIO);
			//save2file(FW_MANGLE, "-A PREROUTING -i %s -m dscp --dscp 0x18 -j MARK --set-mark %d \n", lan_eth , 50 );/* DSCP pri3 */	
									
			/* DSCP3_PRIO 400 */
			save2file(TC_CHANGE_FILE, "tc filter add dev %s parent 10:0 protocol ip prio %d u32 match ip tos 0x80 0xff flowid 10:40\n", wan_eth, DSCP3_PRIO);
			//save2file(FW_MANGLE, "-A PREROUTING -i %s -m dscp --dscp 0x20 -j MARK --set-mark %d \n", lan_eth , 40 );/* DSCP pri4 */	
						
			/* DSCP2_PRIO 300 */
			save2file(TC_CHANGE_FILE, "tc filter add dev %s parent 10:0 protocol ip prio %d u32 match ip tos 0xa0 0xff flowid 10:30\n", wan_eth, DSCP2_PRIO);
			//save2file(FW_MANGLE, "-A PREROUTING -i %s -m dscp --dscp 0x28 -j MARK --set-mark %d \n", lan_eth , 30 );/* DSCP pri5 */	
						
			/* DSCP1_PRIO 200 */
			save2file(TC_CHANGE_FILE, "tc filter add dev %s parent 10:0 protocol ip prio %d u32 match ip tos 0xc0 0xff flowid 10:20\n", wan_eth, DSCP1_PRIO);
			//save2file(FW_MANGLE, "-A PREROUTING -i %s -m dscp --dscp 0x30 -j MARK --set-mark %d \n", lan_eth , 20 );/* DSCP pri6 */	
						
			/* DSCP0_PRIO 100 */
			save2file(TC_CHANGE_FILE, "tc filter add dev %s parent 10:0 protocol ip prio %d u32 match ip tos 0xe0 0xff flowid 10:10\n", wan_eth, DSCP0_PRIO);
			//save2file(FW_MANGLE, "-A PREROUTING -i %s -m dscp --dscp 0x38 -j MARK --set-mark %d \n", lan_eth , 10 );/* DSCP pri7 */				
		}
	}
#endif//DOWNLINK_BANDWIDTH
	
	_system("sh %s", TC_CHANGE_FILE);	
	return 0;
#endif // CONFIG_USER_STREAMENGINE
}
#endif
