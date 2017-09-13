#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "cmds.h"
#include "nvram.h"
#include "shutils.h"

/*content:
 *1   TCP          192.168.0.0-  192.168.0.254           172.21.0.0-   172.21.0.254           1-65535              1-65535
 *
 * */

static void _del_qos_rule(char *content, int rule){
	int index = 0, i;
	char *ptr, *delim = "- ";
	char _rule[10][24], exec_cmds[1024];
	
	//cprintf("content: %s", content);
	ptr = strtok(content,delim);
	memccpy(_rule[index], ptr, 
	strlen(ptr)+1, strlen(ptr)+1);
	while((ptr = strtok(NULL, delim))){
		index++;
		memccpy(_rule[index], ptr, 
		strlen(ptr)+1, strlen(ptr)+1);
	}
	
	for( i = 0; i< 3; i++){
		_rule[1][i] = tolower(_rule[1][i]);
	}
	sprintf(exec_cmds, 
	"nat_cfg del rule %d %s %s %s %s %s %s %s %s %s ",
	rule, _rule[1], _rule[2], _rule[3], _rule[4],
	_rule[5], _rule[6], _rule[7], _rule[8], _rule[9]
	);
	//cprintf("cmds : %s\n", exec_cmds);
	system(exec_cmds);
}
static void _stop_qos_rule(const char *cmd, int rule){
	char cmds[128], content[1024];
	FILE *fp;
	int line = 0;
	
	sprintf(cmds, "%s %d 2>&1", cmd, rule);
	fp = popen(cmds, "r");
	if(fp == NULL)
		return;
	while(!feof(fp)){
		if(fgets(content, sizeof(content), fp) != NULL){
			if(line >1){
				_del_qos_rule(content, rule);
			}
			bzero(content, sizeof(content));
			line++;
		}
        }
	pclose(fp);
}
/*
 * rule = <enable: 0|1>,<name>,<Qid: 1-4>,<protocol: tcp|udp>,
 * 	<sip start>,<sip end>,<sport start>,<sport end>,
 * 	<dip start>,<dip end>,<dport start>,<dport end>
 *
 * EX: "0,MyQos,2,tcp,192.168.10.1,192.168.10.10,22,65500,172.21.32.10,172.21.46.10,1,1024"
 *
 * */
void process_qos_rule(int rule, const char *action)
{
	char cmd[256];
	char enable[3];
	
	char exec_cmds[256];
	sprintf(cmd, "%s", nvram_safe_get_i("qos_rule", rule, g1));
	char name[256], num[5], protocol[24],
	      src_ip_s[24], src_ip_e[24], 
	      src_port_s[24], src_port_e[24],
	      dest_ip_s[24], dest_ip_e[24],
	      dest_port_s[24], dest_port_e[24];
		
	sscanf(cmd, 
	"%[^,] ,%[^,] , %[^,], %[^,], %[^,], %[^,], %[^,], %[^,], %[^,], %[^,], %[^,], %[^,]",
		enable, name, num, protocol,
		src_ip_s, src_ip_e, src_port_s, src_port_e,
		dest_ip_s, dest_ip_e, dest_port_s, dest_port_e
	      );
        if (strcmp(enable, "1") != 0) {
                cprintf("This rule %d is not enable.\n", rule);
        }else {
		sprintf(exec_cmds,"nat_cfg %s rule %s %s %s %s %s %s %s %s %s %s",
			action, num, protocol,
			src_ip_s, src_ip_e, dest_ip_s, dest_ip_e,
			src_port_s, src_port_e, dest_port_s, dest_port_e
			);
//		cprintf("%s\n",exec_cmds);
		system(exec_cmds);
	}
}

/* ip_msg: 0.0.0.0/255.255.255.255 */
static void setup_eth(char *ip_msg)
{ 
	char *rip, *rmask;
	char cmd[128];
	
	rip = strsep(&ip_msg, "/");
	rmask = ip_msg;
	
	/* Qos add eth1 ip */
	sprintf( cmd, "nat_cfg add ip eth1 %s %s", rip, rmask);
	system(cmd);
	bzero(cmd, sizeof(cmd));
	/* Qos add eth0 ip */
	sprintf( cmd, "nat_cfg add ip eth0 %s %s",
		nvram_safe_get("static_addr0"), 
		nvram_safe_get("static_netmask0"));
	system(cmd);
}

/* cmds:cli net ii status WAN_primary */
static char *__get_by_popvalue(const char *cmds, char *out, int len) 
{
	int _size;
	FILE *fp;
       
	if ((fp = popen(cmds,"r")) == NULL)
		return NULL;
	
	bzero(out, len);
	if (fread(out, sizeof(char), len, fp) <= 0)
		return NULL;

	pclose(fp);
	return out;
}

static void start_hwnat()
{
	char ip[256], *_p;
	char *wan_type, *type, *conn, *ipmask, *mac;
	char buffer[256];
	
	/* Clear default value */
	system("nat_cfg del eth0");
	system("nat_cfg del eth1");
	/* Set eth0 = lan, eth1 = wan */
	system("nat_cfg set eth0 lan");
	system("nat_cfg set eth1 wan");
	/* Set eth0 and eth1 could enable */
	system("nat_cfg set enable");

	if (__get_by_popvalue("cli net ii status WAN_primary",
		buffer, sizeof(buffer)) == NULL) {
		cprintf("Cannot get Wan_primary infomessage.\n");
		return;
	}
	/* ex: WAN_dhcpc dhcpc connected 172.21.32.255/255.255.240.0 00:11:22:33:44:55 */
	_p = buffer;
	/*
	 * process Wan_primary string
	 * WAN_primary:wan_dhcpd dhcpd connecting 0.0.0.0/255.255.255.255 00:11:22:33:44:55
	 * We only need to get ip and mask, others ingore.
	 */
	wan_type = strsep(&_p, " ");
	type = strsep(&_p, " ");
	conn = strsep(&_p, " ");
	ipmask = strsep(&_p, " ");
	mac = _p;
	
	strcpy(ip, ipmask);
	setup_eth(ip);
}

static void process_weight(char *rule_weight, const char *_delima)
{
	char *wei_rule1, *wei_rule2, 
	     *wei_rule3, *wei_rule4;
	char cmd[256];

	wei_rule1 = strsep(&rule_weight, _delima);
	wei_rule2 = strsep(&rule_weight, _delima);
	wei_rule3 = strsep(&rule_weight, _delima);
	wei_rule4 = rule_weight;
	
	sprintf(cmd, "nat_cfg set weight %s %s %s %s",
	        wei_rule1, wei_rule2, wei_rule3, wei_rule4);
	system(cmd);
}

static void qos_engine_start(){
	char cmd[128];
	int rule_max = 10, i = 0, qos_rule= 1;
	char *qid_weight;
	
	/* set Qid Weight */
	sprintf(cmd,"%s", nvram_safe_get("qos_weight_rule"));
	if (strcmp(cmd, "") != 0) {
		process_weight(cmd,",");
	}
	
	for (i = 1 ; i <= 4 ; i++) {
		_stop_qos_rule("nat_cfg show rule", i);
	}

	for (i = 0 ; i < rule_max ; i++ , qos_rule++) {
		/* Qos rule  0 start , 1 stop */
		if (strcmp(nvram_safe_get_i("qos_rule", qos_rule , g1), "0") != 0) {
			cprintf("########## Add Qos Rule %d ##########\n", qos_rule);
			process_qos_rule(qos_rule, "add");
			cprintf("This rule %d is set up.\n", qos_rule);
		}
	}
}

static int qos_start(int argc, char *argv[])
{
	int qos_rule;
	cprintf("##### HWNAT Start #####\n");
	start_hwnat();
	
	cprintf("##### Qos Start #####\n");
	if (strcmp(nvram_safe_get("is_enable_qos"),"1") != 0) {
		cprintf("---- Qos Engine not Enable -----\n");
		for(qos_rule = 1; qos_rule < 5 ; qos_rule ++){
			_stop_qos_rule("nat_cfg show rule", qos_rule);
		}
	} else {
		cprintf("----- Qos Engine Enable -----\n");
		qos_engine_start();
	}

	return 0;
}

static int qos_stop(int argc, char *argv[])
{
	int qos_rule= 1;

	cprintf("##### Qos Stop ######\n");
	//if (strcmp(nvram_safe_get("is_enable_qos"), "0") == 0) {
	if (strcmp(nvram_safe_get("is_enable_qos"), "0") != 0) {
		system("nat_cfg set weighting 1 1 1 1");
		for(qos_rule = 1; qos_rule < 5 ; qos_rule ++){
			_stop_qos_rule("nat_cfg show rule", qos_rule);
		}
	}
	
	return 0;
}
/*
 * operation policy:
 * start: qos
 * */
int qosd_main(int argc, char *argv[])
{
	struct subcmd cmds[] = {
		{ "start", qos_start},
		{ "stop", qos_stop},
		{ NULL, NULL}
	};
	return lookup_subcmd_then_callout(argc, argv, cmds);
}
