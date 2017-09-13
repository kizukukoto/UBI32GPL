#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <syslog.h>

#include "cmds.h"

/*
struct subcmd {
	char *action;
	int (*fn)(int argc, char *argv[]);
	char *help_string;
};
*/
#if 0
#define DEBUG_MSG(fmt, arg...)       cprintf(fmt, ##arg)
#else
#define DEBUG_MSG(fmt, arg...) 
#endif

/***************************************************
 * wireless, nvram, independence helper.
 * ************************************************/
#define dbg(fmt, a...)	fprintf(stderr, fmt, ##a)
typedef int (*action_t)(char *, int, void *);

int str_stream_loop_replace(FILE *in, FILE *out, action_t act, void *data)
{
	char buf[1024];
	int rev;

	while (fgets(buf, sizeof(buf), in) != NULL) {
		rev = act(buf, sizeof(buf), data);

		switch (rev) {
		case 0:	//ignored buffer.
			break;
		case 1:
			fputs(buf, out);
			break;
		default:
			return -1; // undefined
		}
	}
	return 0;
}

static char *chop(char *io)
{
	char *p;

	p = io + strlen(io) - 1;
	while ( *p == '\n' || *p == '\r') {
		*p = '\0';
		p--;
	}
	return io;
}

int match_strncmp(char *s, int size, void *data)
{
	char **k = data;

	if (strncmp(s, *k, strlen(*k)) == 0) {
		//fprintf(stderr, "string replaced %s to %s", s, *(k + 1));
		strncpy(s, *(++k), size - 1);
		strcat(s, "\n");
		**(++k) = '1';
	}
	return 1;
}
#if 0
#define SKIP_SPACE(p) while (isspace(*(p)) && *(p) != '\0'){(p)++;}
#define JUMP_SPACE(p) while (!isspace(*(p)) && *(p) != '\0'){(p)++;}
int parseconfig(char *s, int size, void *data)
{
	struct {
		const char *k;
		const char *v;
		const char *delim;
		const char *comment;
		int res;
	} *r = data;
	char *p;
	
	SKIP_SPACE(s);
	JUMP_SPACE(s);
	if (strncmp(s, *k, strlen(*k)) == 0) {
		fprintf(stderr, "string replaced %s to %s", s, *(k + 1));
		strncpy(s, *(++k), size - 1);
		strcat(s, "\n");
		*(++k) = '1';
	}
	return 1;
}

int config2file(const char *p1, const char *key, const char *delim, const char *comment, const char *value)
{
	FILE *f1 = NULL, *f2 = NULL;
	int rev = -1;
	int c;
	
	struct {
		const char *k;
		const char *v;
		const char *delim;
		const char *comment;
		int res;
	} replace = {
		key, value, delim, comment, 0
	};
	
	if ((f1 = fopen(p1, "r+")) == NULL)
		goto sys_out;
	
	if ((f2 = tmpfile()) == NULL)
		goto sys_out;
	
	if (str_stream_loop_replace(f1, f2, match_strncmp, &replace) == -1)
		goto out;
	
	if (fseek(f1, 0, SEEK_SET) == -1)
		goto sys_out;
	if (fseek(f2, 0, SEEK_SET) == -1)
		goto sys_out;
	fclose(f1);
	
	if ((f1 = fopen(p1, "w")) == NULL)
		goto sys_out;
	
	while ((c = fgetc(f2)) != EOF)
		fputc(c, f1);

		
	fclose(f1);
	fclose(f2);
out:
	return rev;
sys_out:
	perror("sys ");
	if (f1)
		fclose(f1);
	if (f2)
		fclose(f2);
	return rev;
}
#endif
int file_strncmp2replace(const char *p1, const char *s1, const char *s2)
{
	FILE *f1 = NULL, *f2 = NULL;
	int rev = -1;
	int c;
	char hit[] = "0";
	char const *replace[] = {
		s1, s2, hit
	};
	
	if ((f1 = fopen(p1, "r+")) == NULL)
		goto sys_out;
	
	if ((f2 = tmpfile()) == NULL)
		goto sys_out;
	
	if (str_stream_loop_replace(f1, f2, match_strncmp, replace) == -1)
		goto out;
	
	if (fseek(f1, 0, SEEK_SET) == -1)
		goto sys_out;
	if (fseek(f2, 0, SEEK_SET) == -1)
		goto sys_out;
	fclose(f1);
	
	if ((f1 = fopen(p1, "w")) == NULL)
		goto sys_out;
	
	while ((c = fgetc(f2)) != EOF)
		fputc(c, f1);
	
	if (*replace[2] == '0')
		fprintf(f1, "%s\n", s2);

	fclose(f1);
	fclose(f2);
out:
	return rev;
sys_out:
	perror("sys ");
	if (f1)
		fclose(f1);
	if (f2)
		fclose(f2);
	return rev;
}

/*
 * Parsing keywords and value from file @cfg
 * 	The file can be any text file format as following format sush as:
 * 
 *  int rev = read_config("file.conf", "my_key", "=:", buf);
 *  
 *  If context in file.conf:
 * # 
 * #my_key=a value	<=== Fail: this is marked, so will not be hit.
 * my_key = a vlaue	
 * 	 ^		<=== Fail: this is " " <space> between @key and @value.
 * my_key0=hey		<=== Fail: @key is "my_key" not "my_key0" 
 * my_key=12345		<=== Match: "12345" will be fulled into @value back.
 * my_key:56489		<=== Natch: "56789" will be fulled into @value back.
 * 
 * XXX: The newline character of @value from file will be striped
 * */
int read_config(const char *cfg, const char *key, const char *delim, char *value)
{
	FILE *fd;
	char buf[4096], *p;
	int rev = -1;
	
	if ((fd = fopen(cfg, "r")) == NULL)
		goto sys_err;
	
	while (fgets(buf, sizeof(buf), fd) != NULL) {
		if (strncmp(buf, key, strlen(key)) != 0)
			continue;
		if ((p = strchr(delim, buf[strlen(key)])) == NULL)
			continue;
		strcpy(value, buf + strlen(key) + 1);
		chop(value);
		rev = 0;
		break;
	}
	fclose(fd);
	return rev;
sys_err:
	perror("read config ");
	return -1;
}
/*************************************************************
 * subcmds helpers
 * **********************************************************/
static void main_using(const char *cmd, struct subcmd *cmds)
{
	fprintf(stderr, "Unknown command: %s, use subcommands below\n", cmd);

	for (;cmds->action != NULL; cmds++) {
		fprintf(stderr, "  %-15s: %s\n", cmds->action,
			cmds->help_string == NULL?"":cmds->help_string);
	}
	fprintf(stderr, "build %s %s\n", __DATE__ , __TIME__);
}

/*
static struct subcmd *lookup_subcmd(int argc, char *argv[], struct subcmd *cmds)
{
	
	struct subcmd *p = NULL;

	if (argc < 2 ||
		(strcmp(argv[1], "-h") == 0 ||
		strcmp(argv[1], "--help") == 0))
	{
		goto out;
	}
	
	for (p = cmds; p->action != NULL; p++) {
		if (strncmp(argv[1], p->action, strlen(argv[1])) == 0) {
			DEBUG_MSG("lookup_subcmd: p->action=%s\n",p->action);
			return p;
		}
	}
out:
	return NULL;
}

int lookup_subcmd_then_callout(int argc, char *argv[], struct subcmd *cmds)
{
	struct subcmd *p = NULL;
	int rev = -1;
	char *upcmd;
	upcmd = argv[0];
	
	if ((p = lookup_subcmd(argc, argv, cmds)) == NULL) {
		main_using(argv[0], cmds);
		goto out;
	}
	if (p->fn == NULL) {
		fprintf(stderr, "%s not implement!\n", p->action);
		goto out;
	}
	rev = p->fn(--argc, ++argv);
out:
	return rev;
}
*/
/*************************************************
 * WPS LED toggle stuff
 * **********************************************/
static void toggle_wps_led(int on)
{
	if (on)
		system("gpio WPS on");
	else
		system("gpio WPS off");
}

static void toggle_wps_led_blank(int on, int off, int timeout)
{
	if (on < 100 || off < 100)
		return;
	
	on *= 1000;
	off *= 1000;
	timeout *= 1000;
	while (timeout >= 0) {
		toggle_wps_led(1);
		usleep(on);
		toggle_wps_led(0);
		usleep(off);
		timeout -= on + off;
	}
}
/**************************************************
 * Follow Wi-Fi Alliance WPS 1.0 Sep, 2006 Page.81
 * ************************************************/
static void wps_led_inprocess(int timeout)
{
	toggle_wps_led_blank(200, 100, timeout);
}

static void wps_led_error(int timeout)
{
	toggle_wps_led_blank(100, 100, timeout);
}

static void wps_led_success(int timeout) /* led on 300 sec by SPEC */
{
	if (timeout == 0)
		timeout = 300 * 1000;
	toggle_wps_led_blank(timeout, 0, timeout);
}
static void wps_led_overlap(int timeout)
{
	while (timeout > 0) {
		toggle_wps_led_blank(100, 100, 1000);
		toggle_wps_led(0);
		usleep(500 * 1000);
		timeout -= 1500;
	}
}

/***************************************************
 * main functions. Wireless, WPS...
 * ************************************************/
extern void __start_wps(void);
extern int __stop_wps(void);
extern void init_wps(void);

static int wps_status_monitor();
static int wl_debug;
#define wlmsg(fmt, a...) { wl_debug ? fprintf(stderr, fmt, ##a) : 0;}
static int pin_main(int argc, char *argv[])
{
	char cmds[80];
	
	if (argc < 2) {
		printf("args: <pin>: set up PIN code\n");
		return -1;
	}
	snprintf(cmds, sizeof(cmds) - 1, "echo \"%s\" > "
		"/etc/wsc/wdev0ap0/wsc_enrollee_pin.txt", argv[1]);
	DEBUG_MSG("%s:%d\n", __FUNCTION__, __LINE__);
	printf("wps pin cmd:[%s]\n", cmds);
	system(cmds);
	system("wsc_command wdev0ap0 3");
	//wps_status_monitor();
	return 0;
}


static void sig(int n)
{
	toggle_wps_led(0);
	printf("exit\n");
	exit(0);
}
#include "nvram.h"
//#define nvram_set(k, v) fprintf(stderr, "nvram_set(%s,%s)\n", k, v)
static int wps_success_handler()
{
	char *fname = "/etc/wsc/wdev0ap0/hostapd.conf";
	char value[4096];

	/* XXX: for debug flags ... */
	if (nvram_match("wps_commit", "0") == 0) {
		printf("no commit wps success\n");
		return 0;
	}
	printf("commit wps success\n");
	
	/* in MVL, WPS not support 11n mode... */
	if (nvram_match("wlan0_radio_mode", "0") == 0)
		nvram_set("wlan0_dot11_mode", "11bg");
	else
		nvram_set("wlan1_dot11_mode", "11a");
	
	if (nvram_match("wps_configured_mode", "5") == 0) {
		printf("wps already in configured mode\n");
		return 0;
	}
		
	/* set WPS as configured mode */
	nvram_set("wps_configured_mode", "5");
	
	/* setup SSID */
	if (read_config(fname, "ssid", "=", value) != 0)
		goto cfg_err;
	wlanxxx_nvram_set("ssid", value);
	
	if (read_config(fname, "wpa", "=", value) != 0)
		goto cfg_err;
	if (*value == '0') {
		wlanxxx_nvram_set("security", "disable");
		return 0;	/* no security WEP ? open ? shared ?*/
	}
	
	if (read_config(fname, "wpa_key_mgmt", "=", value) != 0)
		goto cfg_err;
	{
	/*
	 * hostapd.conf(wpa_key_mgmt)  nvram(wlan0_security)
	 * ?				wpa_eap
	 * WPA-PSK WPA-EAP		?
	 * WPA-PSK			wpa_psk
	 * ?				wpa2_psk
	 * ?				wpa2_eap
	 * ?				wpa2auto_psk
	 * ?				wpa2auto_eap
	 * ?				wpa2_eap
	 *
	 * FIXME:I suppose only wpa_psk...
	 * */
	wlanxxx_nvram_set("security", "wpa_psk");
	}
	
	if (read_config(fname, "wpa_passphrase", "=", value) != 0)
		goto cfg_err;
	wlanxxx_nvram_set("psk_pass_phrase", value);
	
	if (read_config(fname, "wpa_pairwise", "=", value) != 0)
		goto cfg_err;
	{
	/*
	 *  hostapd.conf	nvram(wlan0_psk_cipher_type)
	 *  CCMP		aes
	 *  TKIP		tkip
	 *  TKIP CCMP		both
	 *  CCMP TKIP		both.
	 * */
	struct {
		char *hostapd;
		char *nvram;
	} map[] = {
		{"CCMP TKIP", "both"},
		{"TKIP CCMP", "both"},
		{"CCMP", "aes"},
		{"TKIP", "tkip"},
		{ NULL, NULL}
	}, *m;
	for (m = map; m->hostapd != NULL; m++) {
		if (strncmp(m->hostapd, value, strlen(m->hostapd)) != 0)
			continue;
		wlanxxx_nvram_set("psk_cipher_type", m->nvram);	/* 2.4, 5G? */
		break;
	}
	}
	nvram_commit();
	return 0;
cfg_err:
	printf("WPS configured mode change error\n");
	return -1;
}

static void xmit_httpd_wps_result(int success)
{
	if (success)
		system("killall -USR1 httpd");
	else
		system("killall -QUIT httpd");

}

static int wps_status_monitor()
{
	int fd;
	int rev = -1, timeout = 120;
	char status;
	char *fname = "/etc/wsc/wdev0ap0/wsc_op_result.txt";

	signal(SIGINT, sig);
	signal(SIGTERM, sig);
	signal(SIGQUIT, sig);
	
	//toggle_wps_led(s1);
	//sleep(3);
	/* polling @fname to monitor the status of WPS */
	if ((fd = open(fname, O_RDONLY)) == -1)
		goto out;
	
	while (1) {
		lseek(fd, SEEK_SET, 0);
		if (read(fd, &status, 1) != 1)  /* first byte */
			break;
		switch (status) {
		case '0':	/* Fail */
			/* reset status */
			system("echo \"3\" > /etc/wsc/wdev0ap0/wsc_op_result.txt");
			printf("WPS LED Fail!\n");
			xmit_httpd_wps_result(0);
			wps_led_error(10 * 1000);
			break;
		case '1':	/* success */
			/* reset status */
			system("echo \"3\" > /etc/wsc/wdev0ap0/wsc_op_result.txt");
			printf("WPS LED success!\n");
			xmit_httpd_wps_result(1);
			/* 
			 * FIXME: there maybe have race condition here!
			 * @wsc_cmd set/get nvram?
			 * wps_success_handler() set nvram simultaneously.
			 * */
			wps_led_success(5 * 1000);
			if (wps_success_handler() != 0)
				printf("Error on WPS changes to configured mode\n");
			break;
		case '2':	/* PIN mode Peding */
			wlmsg("WPS PIN mode LED Peding!\n");
			wps_led_inprocess(2 * 1000);
			break;
		case '3':	/* WPS not started */
			wlmsg("WPS not started!\n");
			sleep(2);
			break;
		case '4':	/* PBC mode Active */
			wlmsg("XXXXWPS PBC active!\n");
			wps_led_inprocess(1 * 1000);
			break;
		case '5':	/* PIN checksum Failed */
			wlmsg("WPS PIN checksum Fail!\n");
			wps_led_error(10 * 1000);
			close(fd);
			goto out;
		default:
			close(fd);
			goto out;
		}
		wlmsg("XXXWPS moinitoring\n");
		timeout -= 2;
	}
	close(fd);
	rev = 0;
out:
	toggle_wps_led(0);
	return rev;
}

static int pbc_main(int argc, char *argv[])
{
	/* gpio WPS on|off */
	if (nvram_match("wps_enable", "1") != 0)
		return 0;
	system("wsc_command wdev0ap0 4");
	//wps_status_monitor();
	return 0;
	
}

static int start_wps_main(int argc, char *argv[])
{
	__start_wps();
	return 0;
}

static int stop_wps_main(int argc, char *argv[])
{
	__stop_wps();
	return 0;
}

static int wps_led_main(int argc, char *argv[])
{
	int t = atoi(argv[2]);

	if (argc < 3) {
		printf("%s [inproc | error | overlap | success] [timeout(sec)]\n", argv[0]);
		return 0;
	}
	t *= 1000;
	
	signal(SIGINT, sig);
	signal(SIGTERM, sig);
	signal(SIGQUIT, sig);
	
	if (strcmp(argv[1], "inproc") == 0)
		wps_led_inprocess(t);
	else if (strcmp(argv[1], "error") == 0)
		wps_led_error(t);
	else if (strcmp(argv[1], "overlap") == 0)
		wps_led_overlap(t);
	else if (strcmp(argv[1], "success") == 0)
		wps_led_success(t);
	else
		printf("%s [inproc | error | overlap | success] [timeout(sec)]\n", argv[0]);
	return 0;
}

int wps_ok_main(int argc, char *argv[])
{
	return wps_success_handler();
}
int wps_monitor_main(int argc , char *argv[])
{
	return wps_status_monitor();
}

int init_wps_main(int argc, char *argv[])
{
	init_wps();
	return 0;
}

static int wps_main(int argc, char *argv[])
{
	struct subcmd cmds[] = {
		{"init", init_wps_main, "init once configures for wirless"},
		{"pin", pin_main,"pin utility"},
		{"pbc", pbc_main,"pbc utility"},
		{"start", start_wps_main,"start wps daemon"},
		{"stop", stop_wps_main,"stop wps daemon"},
		{"led", wps_led_main, "toggle wps led"},
		{"monitor", wps_monitor_main, "A daemon which monitor wps status"},
		{"wps_ok", wps_ok_main, "wps_ok...(for test)"},
		{NULL, NULL}
	};
	return lookup_subcmd_then_callout(argc, argv, cmds);
}


enum {
	WLAN_G_BAND = 0,
	WLAN_A_BAND = 1,
};

/*************************************************
 * WLAN LED toggle stuff
 * **********************************************/
static void toggle_wlan_led(int on, int band)
{
	char str[64];

	sprintf(str, "gpio %s %s", (band == WLAN_G_BAND)?"WLAN_LED":"WLAN_LED_2", on?"on":"off");
	system(str);
}

static void toggle_wlan_led_blink(int on, int off, int band)
{
	if (on < 100 || off < 100)
		return;

	on *= 1000;
	off *= 1000;
	while (band != -1) {
		toggle_wlan_led(1, band);
		usleep(on);
		toggle_wlan_led(0, band);
		usleep(off);
	}
}

/*************************************************************
 * Follow Dlink Led spec. high speed: flash every 2/3 seconds.
 *************************************************************/
static void wlan_led_blink(int band)
{
	toggle_wlan_led_blink(333, 333, band); /* about 1/3 second on, 1/3 second off */
}

static void _sig_(int n)
{
	toggle_wlan_led(0, WLAN_G_BAND);
	toggle_wlan_led(0, WLAN_A_BAND);
	printf("exit\n");
	exit(0);
}

#define WLUTILS_WLANLED_PID "/var/run/wlanled.pid"
static void create_pidfile(char *fpid)
{
	FILE *pidfile;

	if ((pidfile = fopen(fpid, "w")) != NULL) {
		fprintf(pidfile, "%d\n", getpid());
		(void) fclose(pidfile);
	} else {
		error("Failed to create pid file %s: %m", WLUTILS_WLANLED_PID);
	}
}

void kill_wlanled_pid(void)
{
	FILE *pfilein;
	char pidnumber[32];
	char cmd[64];
	
	pfilein = fopen(WLUTILS_WLANLED_PID, "r");
	if (pfilein) 
	{
		if (fscanf(pfilein, "%s", pidnumber) == 1) 
		{
			fclose(pfilein);
			if (atoi(pidnumber) != getpid())
			{
				sprintf(cmd,"kill -9 %s",pidnumber);
				system(cmd);
			}
		} 
		else
			fclose(pfilein);
	}
}

static int wlan_led_main(int argc, char *argv[])
{

	int wlan_band = -1;

	if (argc < 3) {
		printf("%s [blink] [2.4G | 5G]\n", argv[0]);
		return 0;
	}
	
	signal(SIGINT, _sig_);
	signal(SIGTERM, _sig_);
	signal(SIGQUIT, _sig_);
	
	create_pidfile(WLUTILS_WLANLED_PID);

	if (strcmp(argv[1], "blink") == 0) {
		if (strcmp(argv[2], "2.4G") == 0) 
			wlan_band = WLAN_G_BAND;
		else if (strcmp(argv[2], "5G") == 0)
			wlan_band = WLAN_A_BAND;
		else
			wlan_band = -1; /* -1 as wirless disabled */
		
		wlan_led_blink(wlan_band);
	}
	else
		printf("%s [blink] [2.4G | 5G]\n", argv[0]);

	return 0;
}



static int wlan_main(int argc, char *argv[])
{
	struct subcmd cmds[] = {
		{"led", wlan_led_main, "toggle wlan led"},
		{NULL, NULL}
	};
	return lookup_subcmd_then_callout(argc, argv, cmds);
}

int wlan_subcmd(int argc, char *argv[])
{
	struct subcmd cmds[] = {
		{"wps", wps_main, "wps utility"},
		{"wlan", wlan_main, "wlan utility"},
		{NULL, NULL}
	};
	wl_debug = getenv("WL_DEBUG") == NULL ? 0 : !strcmp("1", getenv("WL_DEBUG"));
	return lookup_subcmd_then_callout(argc, argv, cmds);
}
