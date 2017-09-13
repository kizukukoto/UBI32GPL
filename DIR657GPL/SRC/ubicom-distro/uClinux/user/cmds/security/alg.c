#include <stdio.h>
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>

#include <nvramcmd.h>
#define DBG_APP printf

#define ALG_BUFSIZE	64
#define NV_LAN_IFNAME	"if_dev0"
#define NV_WAN_IFNAME	"if_dev2"

static int fetch_sched(int idx, char *out)
{
	char s[24], *p;

	sprintf(s, "schedule_rule%d", idx);
	if (NVRAM_MATCH(s, ""))
		goto err;
	p = nvram_get(s);
	strcpy(out, p);
	return 0;
err:
	return -1;
}

int
alg_append_sched(char **prefix_cmd, int shd, char time_cmd[][ALG_BUFSIZE])
{
	// for v1.3X: "Sun", "Mon", "Tue", "Wed", "Thr", "Fri", "Sat", NULL
	char *w[] = {
		"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat", NULL
	};
	
	int i, sec1, sec2;
	char *d, *t1, *t2, *tp, timestr[48] = "shed1/1010101/60/120";

	fetch_sched(shd, timestr);
	printf("schedule:%d = [%s]\n", shd, timestr);
	tp = timestr;
	strsep(&tp, "/");
	d = strsep(&tp, "/");
	t1 = strsep(&tp, "/");
	t2 = strsep(&tp, "/");
	sec1 = atoi(t1);
	sec2 = atoi(t2);
	/* for v1.3X
	sprintf(time_cmd[3], "%d:%d:00", sec1 / 60, sec1 % 60);
	sprintf(time_cmd[5], "%d:%d:00", sec2 / 60, sec2 % 60);
	*/
	sprintf(time_cmd[3], "%d:%d", sec1 / 60, sec1 % 60);
	sprintf(time_cmd[5], "%d:%d", sec2 / 60, sec2 % 60);
	/* Parser week of days */
	for (i = 0; i < 7; i++) {
		if (d[i] == '1') {
			strcat(time_cmd[7], w[i]);
			strcat(time_cmd[7], ",");
		}
	}
	if ((d = strrchr(time_cmd[7], ',')) != NULL)
		*d = '\0';
	
	/* jump to tail of pointers */
	for (;*prefix_cmd != NULL; *prefix_cmd++);
	
	
	for (i = 0; time_cmd[i][0] != '\0'; i++) {
		*prefix_cmd = time_cmd[i];
		*prefix_cmd++;
		fprintf(stderr , "%s ", time_cmd[i]);
	}
	fputc('\n', stderr);
	*prefix_cmd = NULL;

	return 0;
	
}

int
app_parser(int up, char *app)
{
	/*
	 * Format: proto:out_port_rangs>in_proto:in_port_rangs,enable,desc,sched_idx
	 * 	out_proto := <tcp|udp|any>
	 *	out_port_rangs := <start_port-end_port>/<start_port2-end_port2> ...
	 * 	in_proto := <tcp|udp|any>
	 *	in_port_rangs  := <start_port-end_port>/<start_port2-end_port2> ...
	 *	enable := <on|off>
	 *	desc = description strings
	 *	sched_idx := <index of schedule_rule | always>
	 *	Ex:	tcp:6000-6100>udp:3000-3100,on,app1,always
	 *		any:6000-6100/6105/7000-7010>any:6000-6100/8000-8100,on,complex_app,3
	 * */
	
	int i = 0;
	char *out_proto, *out_start, *in_start, *in_proto;
	char *p, *enable, *sched, schedbuf[512];
	char **argv, *prefix_cmd[256] = {
		"iptables",
		"-t",
		"nat",
		up?"-A":"-D",
		"PREROUTING",
		NULL
	};
	char time_cmd[][ALG_BUFSIZE] = {
		"-m",
		"time",
		"--timestart",
		"HH:MM:SS",
		"--timestop",
		"HH:MM:SS",
		"--days",
		"",
		""
	};
	char alg_cmd[][ALG_BUFSIZE] = {
		"-m",
		"alg",
		"--tri-ports",
		"XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX",
		"--dst-ports",
		"XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX",
		"--tri-proto",
		"XXXXXX",
		"--dst-proto",
		"XXXXXX",
		"--tri-dev",
		"XXXXXX",
		"--dst-dev",
		"XXXXXX",
		"-j",
		"ALG",
		""
	};

	argv = prefix_cmd;
	/*
	 * Parser ALG string, Ex:
	 * any:6000-6100/6105/7000-7010>any:6000-6100/8000-8100,on,XXXX,XXXX
	 * */
	out_proto = strsep(&app, ":");
	out_start = strsep(&app, ">");
	in_proto = strsep(&app, ":");
	in_start = strsep(&app, ",");

	//DBG_APP("out_proto:[%s], out_start:[%s], in_proto[%s], in_start[%s], tail[%s]\n",
	//	out_proto, out_start, in_proto, in_start, app);
	
	enable = strsep(&app, ",");
	if (enable == NULL || strcmp(enable, "on") != 0)
		return 0;
	
	/* Transfer port ranges format '/' to ',' AND  '-' to ':' */
	for (p = out_start; (p = strchr(p, '/')) != NULL; p++)
		*p = ',';
	for (p = out_start; (p = strchr(p, '-')) != NULL; p++)
		*p = ':';

	for (p = in_start; (p = strchr(p, '/')) != NULL; p++)
		*p = ',';
	for (p = in_start; (p = strchr(p, '-')) != NULL; p++)
		*p = ':';
	
	/* Fullout to @alg_cmd */
	strcpy(alg_cmd[3], out_start);
	strcpy(alg_cmd[5], in_start);
	strcpy(alg_cmd[7], out_proto);
	strcpy(alg_cmd[9], in_proto);
	/* FIXME mm...! */
	//strcpy(alg_cmd[11], nvram_get("lan_ifname"));
	strcpy(alg_cmd[11], nvram_safe_get(NV_LAN_IFNAME));
	
	//strcpy(alg_cmd[13], nvram_get("wan0_ifname"));
	//sprintf(alg_cmd[13], "!%s", nvram_get("lan_ifname"));
	sprintf(alg_cmd[13], "!%s", nvram_safe_get(NV_LAN_IFNAME));
	
	/*
	 * Parser schedule_rule
	 * any:6000-6100/6105/7000-7010>any:6000-6100/8000-8100,on,XXXX,shedule_rule
	 * @schedule_rule = "always" or [0-19]
	 * */
	
	sched = strrchr(app, ',') + 1;

	if (!(*sched < '0' || '9' < *sched)) {
		bzero(schedbuf, sizeof(schedbuf));
		/*
		 * @alg_append will format schedule_rule to @time_cmd
		 * then APPEND to @prefix_cmd
		 * */
		alg_append_sched(prefix_cmd, atoi(sched), time_cmd);
	}
	
	/*
	 * Append @alg_cmd to @prefix_cmd
	 * */
	for (argv = prefix_cmd; *argv != NULL; *argv++);
	for (i = 0; *alg_cmd[i] != '\0'; i++) {
		*argv = alg_cmd[i];
		//fprintf(stderr, "%s ", *argv);
		*argv++;
	}
	*argv = NULL;
	fprintf(stderr, "\n==============Parser out:===============\n");
	/*
	 * Finished format alg rule
	 * */
	for (argv = prefix_cmd; *argv != NULL; *argv++)
		fprintf(stderr, "%s ", *argv);
	printf("\n");
	_eval(prefix_cmd, ">/dev/console", 0, NULL);
	return 0;
};

/*
int main(int argc, char *argv[])
{

	 char alg[] = "any:6000-6100/6105/7000-7010>any:6000-6100/8000-8100,on,complex_app,3";
	 app_parser(alg);
}
*/
static int alg_updown(int up)
{
	int i;
	char name[] = "app_portXXXXXXXXXX", value[1000];
	int rev = 0;
	printf("start app\n");
	for (i = 0; i < 24; i++) {
		sprintf(name, "app_port%d", i);
		if (!nvram_invmatch(name, ""))
			continue;
		strncpy(value, nvram_get(name), sizeof(value));
		printf("start app%d:%s\n", i, value);
		printf("start app%d:%s\n", i, value);
		DBG_APP("name:%s = [%s]\n", name, value);

		app_parser(up, value);
	}
	return rev;

}
static int alg_start_main(int argc, char *argv[])
{
	fprintf(stderr, "app start\n");
	return alg_updown(1);
}

static int alg_stop_main(int argc, char *argv[])
{
	fprintf(stderr, "app stop\n");
	return alg_updown(0);
}

#include "cmds.h"
int alg_main(int argc, char *argv[])
{
	struct subcmd cmds[] = {
		{ "start", alg_start_main},
		{ "stop", alg_stop_main},
		{ NULL, NULL}
	};
	return lookup_subcmd_then_callout(argc, argv, cmds);
}
