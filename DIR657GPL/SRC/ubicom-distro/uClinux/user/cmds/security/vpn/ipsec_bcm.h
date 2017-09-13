#include <nvramcmd.h>

#ifdef CONFIG_BCM_IPSEC
/*
 * This is for boardcom openswan 1.x style.
 * ignore that on other platforms.
 * */
static int __ipsec_bcm_setup(FILE *fd, const char *wan_ifname, const char *lan_ifname, const char *wl_ifname)
{
	char *ipsec_setup=NULL;
	char *value_ptr =NULL;
	char *ipsec_setup_ptr =NULL;
	char *param =NULL;
	int interfaces = 0;
	int plutoload=0,uniqueids=0;
	
	#define IPSEC_INTER_FIELD_DELIMS	",\n"
	ipsec_setup=nvram_safe_get("ipsec_setup");
	if (*ipsec_setup == 0x00) { /* for bcm demoboard only */
		fprintf(stderr, "********* INIT IPSEC PAREMS********\n");
		if ((wan_ifname != NULL) && (strlen(wan_ifname) > 0))
			{fprintf(fd, "\tinterfaces=\"ipsec0=%s ", wan_ifname);interfaces++;}
		if ((lan_ifname != NULL) && (strlen(lan_ifname) > 0))
			{fprintf(fd, "ipsec1=%s ", lan_ifname);interfaces++;}
		if ((wl_ifname != NULL) && (strlen(wl_ifname) > 0))
			{fprintf(fd, "ipsec2=%s ", wl_ifname);interfaces++;}
		if (interfaces!=0)
			fputs("\"\n",fd);
	}else {
		/*fprintf(fd, "config setup\n\t%s\n", ipsec_setup);*/
		if ((value_ptr=malloc(MAX_NVRAM_IPSEC_VALUE_SIZE)) == NULL) {
			printf("rc/ipsec.c: create_ipsec_conf() unable to allocate memory\n");
			fclose(fd);
			return -1;
		}

		/* add the defaults */
		strncpy(value_ptr,ipsec_setup,MAX_NVRAM_IPSEC_VALUE_SIZE-1);
		ipsec_setup_ptr=value_ptr;
		param = STRSEP(&ipsec_setup_ptr,IPSEC_INTER_FIELD_DELIMS,NULL);
		
		/* if we dont have interface as the first param add the default */
		if (strstr(param,"interfaces") ==NULL) {
			if ((wan_ifname != NULL) && (strlen(wan_ifname) > 0))
				{fprintf(fd, "\tinterfaces=\"ipsec0=%s ", wan_ifname);interfaces++;}
			if ((lan_ifname != NULL) && (strlen(lan_ifname) > 0))
				{fprintf(fd, "ipsec1=%s ", lan_ifname);interfaces++;}
			if ((wl_ifname != NULL) && (strlen(wl_ifname) > 0))
				{fprintf(fd, "ipsec2=%s ", wl_ifname);interfaces++;}
			if (interfaces!=0)
				fputs("\"\n",fd);
		}
		/* cannot let pluto search if manual */
		/* for manual extraparams
		 * 	manualstart="connection-1"
		 * 	plutoload="connection-2"
		 * 	plutostart="connection-2
		 */
		if (strstr(param,"manualstart") !=NULL) 
			plutoload=1;
		if (strstr(param,"#manualstart") !=NULL) 
			plutoload=0;
		if (strstr(param,"plutostart") !=NULL) 
			plutoload=1;
		if (strstr(param,"#plutostart") !=NULL)  {
			if (plutoload == 0)
				plutoload=0;
		}

		if (strstr(param,"uniqueids") !=NULL) 
			uniqueids=1;
		while (param != NULL) {
			fprintf(fd,"\t%s\n",param);
			param = STRSEP(&ipsec_setup_ptr,IPSEC_INTER_FIELD_DELIMS,NULL);
		}
		free(value_ptr);
	}

	/* if interfaces still not defined huh expecting only eth0*/
	if (interfaces ==0)
		{fprintf(fd, "\tinterfaces=\"ipsec0=eth0\"\n");}

	/* enable auto= in connections */
	if (plutoload==0) 
		fputs("\tplutoload=%search\n\tplutostart=%search\n", fd);
	/* enable uniqueids= in connections */
	if (uniqueids==0) 
		fputs("\tuniqueids=yes\n", fd);
	fputs("\tsyslog=daemon.info\n", fd);
	return interfaces;
}
/*
 * For Russia dual WANs. Fetch wan info from nvram.
 * OUTPUT:
 * 	@wanip: wan ip.
 * 	@netmask: wan netmask.
 * 	@wanif: wan ifname.
 *
 * 	@russia_wanip: russia dhcp or static ip.
 * 	@russia_netmask: russia dhcp or static netmask.
 * 	@russia_wanif: russia dhcp or static ifname.
 *
 * 	you may get tempty string while dhcp release or russia release
 *
 * */
void dual_wans_info(char *wanip, char *netmask, char *wanif, char *russia_wanip, char *russia_netmask, char *russia_wanif)
{
	if (wanip) wanip[0] = '\0';
	if (netmask) netmask[0] = '\0';
	if (wanif) wanif[0] = '\0';
	if (russia_wanip) russia_wanip[0] = '\0';
	if (russia_netmask) russia_netmask[0] = '\0';
	if (russia_wanif) russia_wanif[0] = '\0';
	
	// get wanip netmask wanif
	if (!NVRAM_MATCH("wan0_ipaddr", "0.0.0.0")){
		strcpy(wanip, nvram_safe_get("wan0_ipaddr"));
		strcpy(netmask, nvram_safe_get("wan0_netmask"));
		strcpy(wanif, nvram_safe_get("real_wan_ifname"));
	}

	if(nvram_invmatch("wan0_proto", "rupppoe") &&
		       	nvram_invmatch("wan0_proto", "rupptp"))
		goto out;
	// get russia_wanip  russia_netmask russia_wanif
	// russia pppoe
	if (NVRAM_MATCH("wan0_proto", "rupppoe")) {
		strcpy(russia_wanip,
		       	NVRAM_MATCH("wan0_russiapppoe_dynamic", "1") ?
			nvram_safe_get("wan0_dhcp_ipaddr") :
			nvram_safe_get("wan0_static_ipaddr"));
			
		strcpy(russia_netmask,
		       	NVRAM_MATCH("wan0_russiapppoe_dynamic", "1") ?
			nvram_safe_get("wan0_dhcp_netmask") :
			nvram_safe_get("wan0_static_netmask"));
			
	// russia pptp
	} else if (NVRAM_MATCH("wan0_proto", "rupptp")){
		strcpy(russia_wanip, NVRAM_MATCH("wan0_rupptp_dynamic", "1") ?
			nvram_safe_get("wan0_dhcp_ipaddr") :
			nvram_safe_get("wan0_static_ipaddr"));
		
		strcpy(russia_netmask, NVRAM_MATCH("wan0_rupptp_dynamic", "1") ?
			nvram_safe_get("wan0_dhcp_netmask") :
			nvram_safe_get("wan0_static_netmask"));
		
	}
	strcpy(russia_wanif, nvram_safe_get("wan0_ifname"));
	if (strcmp(russia_wanif, wanif) == 0)
		russia_wanif[0] = '\0';
out:
	fprintf(stderr, "=========WAN:%s:%s:%s, WAN:%s:%s:%s=======\n", wanip,
		netmask, wanif, russia_wanip, russia_netmask, russia_wanif);
	/*
	 * captured from console...
	 * DHCPC:
	 * =========WAN:::, WAN:172.21.33.131:255.255.240.0:eth0=======
	 * RUPPPOE:
	 * =========WAN:172.21.33.131:255.255.240.0:eth0, WAN:::=======
	 */
	return;
}
int start_ipsec(void)
{
	int noofcons=0;
	static int firsttime=0;
	FILE *fd;
	
	#define IPSEC_START_SCRIPT "/etc/init.d/ipsec"

	if (!(fd = fopen(IPSEC_START_SCRIPT, "r"))) {
		printf("IPSec script [/etc/init.d/ipsec] not found.");
		return 0;
	}
	remove("/tmp/ns.res");
	noofcons =create_ipsec_conf();
	if (noofcons <= 0) 
		return 0;	/* no ipsec profile */
	
	printf("Starting IPSec");
	eval("/etc/init.d/ipsec","stop");
	eval("/etc/init.d/ipsec","start");
	//eval("/usr/local/lib/ipsec/iptables_rules","start");
	if (firsttime != 0)
		goto finish;       
	/* Fix me */
	sleep(10);
	cprintf("Initial H/W engine for IPSec:[%s]\n",
			nvram_safe_get("firmware_upgrading"));
	while (NVRAM_MATCH("firmware_upgrading", "1")) {
		static i = 0;
		cprintf("firmware upgrading, block IPSec\n");
		sleep(3);
		if (i++ > 5 * 20){
			nvram_set("firmware_upgrading", "0");
			cprintf("firmware upgrade is too long!\n");
			i = 0;
			break;
		}
	}
	eval("insmod","bcm582x");
	/*eval("iptables","-F");*/
	eval("/etc/init.d/ipsec","stop");
	eval("/etc/init.d/ipsec","start");
	firsttime=1;
finish:
	if (access("/tmp/ns.res", F_OK) == 0)
		system("ipsec_dns /tmp/ns.res &");
	//eval_ipsec_script();
	return 0;
}

/*
 * Stop IPSec.
 */
int
stop_ipsec(void)
{
	printf("Try to stop IPSec(if running).");
	eval("/etc/init.d/ipsec","stop");
	eval("/usr/local/lib/ipsec/iptables_rules","stop");
	eval("killall", "-9", "ipsec_dns");
	remove("/tmp/ns.res");
	return 0;
}
#endif //CONFIG_BCM_IPSEC
