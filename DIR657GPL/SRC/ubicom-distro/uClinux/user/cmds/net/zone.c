#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include "cmds.h"
#include "nvram.h"
#include "shutils.h"
#include "interface.h"
#include "nvram_ext.h"
#include "libcameo.h"

#if 0
#define DEBUG_MSG(fmt, arg...)       cprintf(fmt, ##arg)
#else
#define DEBUG_MSG(fmt, arg...) 
#endif

#define II_START_MAIN(fmt, a...)  ({				\
         char *v[] = {fmt, ## a, NULL };			\
         ii_start_main(sizeof(v)/sizeof(char *) - 1, v);	\
	 })

#define II_STOP_MAIN(fmt, a...)  ({				\
         char *v[] = {fmt, ## a, NULL };			\
         ii_stop_main(sizeof(v)/sizeof(char *) - 1, v);	\
	 })
#if 0
#define II_STOP_MAIN(fmt, a...)					\
         char *v[] = {fmt, ## a, NULL };			\
         ii_stop_main(sizeof(v)/sizeof(char *) - 1, v); 	\
	 })
#endif

#if 0//set/get port status
extern int VctGetPortConnectState(char *ifname, int portNum, char *connect_State);
extern int VctGetPortSpeedState(char *ifname, int portNum, char *portSpeed, char *duplexMode);
extern int VctSetPortSpeed(char *ifname, char *portspeed);
#endif
int zone_start_main(int argc, char *argv[])
{
	int i, rev = -1;
	char *a, *pa, *p;

	if (argc < 2)
		goto out;
		
	
	if ((i = nvram_find_index("zone_alias", argv[1])) == -1)
		goto out;
	
	if ((pa = nvram_get_i("zone_if_list", i, g1)) == NULL)
		goto out;
	
	if ((p = strdup(pa)) == NULL)
		goto out;
	
	if ((a = strchr(p, ' ')) != NULL)
		*a = '\0';
	DEBUG_MSG("%s: p=%s\n",__FUNCTION__,p);
	rev = II_START_MAIN("start", p);
	free(p);
out:
	rd(rev, "error");
	return rev;
}

int zone_stop_main(int argc, char *argv[])
{
	int i, rev = -1;
	char *if_list, *p;

	if (argc < 2)
		goto out;
	
	if ((i = nvram_find_index("zone_alias", argv[1])) == -1)
		goto out;
	if ((if_list = nvram_get_i("zone_if_list", i, g1)) == NULL)
		goto out;
	
	if ((if_list = strdup(if_list)) == NULL)
		goto out;

	if ((p = strchr(if_list, ' ')) != NULL)
		*p = '\0';
	
	rev = II_STOP_MAIN("stop", if_list);
	free(if_list);
out:
	rd(rev, "error");
	return rev;
}
#if 0
extern int ii_bound_main(int, char **);
extern int ii_deconfig_main(int, char **);
int zone_call_ii(int argc, char *argv[])
{
	char *dev;
	int rev = -1;
	int i = 0;
	char tmp[512], *p;
	struct {
		char *argv0;
		int (*main)(int, char **);
	} m[] = {
		{ "bound", ii_bound_main},
		{ "deconfig", ii_deconfig_main},
		{ NULL, NULL}
	}, *n;
	printf("************ zone %s %s *****\n", argv[0], getenv("interface"));
	if ((dev = getenv("interface")) == NULL)
		goto out;

	foreach_nvram(i, 20, p, "zone_alias", tmp, 1) {
		char *ifalias;
		int t;
		
		if (p == NULL)
			continue;
		if ((ifalias = nvram_get(strcat_i("zone_if_alias", i, tmp))) == NULL ||
		    (ifalias = nvram_get(strcat_i("zone_if_ext_alias", i, tmp))) == NULL)
	    	{
			continue;
		}


		ifalias = nvram_get(strcat_i("zone_if_alias", i, tmp));
		if ((t = nvram_find_index("if_alias", ifalias)) == -1)
			continue;
		if (strcmp(dev, nvram_safe_get(strcat_i("if_dev", i, tmp))) != 0)
			continue;
		setenv1("if_alias", p);
		syslog(LOG_INFO, "\t**** zone %s, %s %s, %s\n", p, argv[0], ifalias, dev);
		for (n = m; n->argv0 != NULL; n++) {
			if (strcmp(argv[0], n->argv0) != 0)
				continue;
			rev = n->main(argc, argv);
		}
		break;
		
	}
	
out:
	
	return rev;
}
#endif
int zone_find_dev(const char *dev)
{
	int i = 0, rev = -1;
	char tmp[512], *p;

	foreach_nvram(i, 20, p, "zone_alias", tmp, 1) {
		char *ifalias;
		int t;
		
		if (p == NULL)
			continue;
		
		if ((ifalias = nvram_get(strcat_i("zone_if_alias", i, tmp))) == NULL ||
		    (ifalias = nvram_get(strcat_i("zone_if_ext_alias", i, tmp))) == NULL)
	    	{
			continue;
		}
		if ((t = nvram_find_index("if_alias", ifalias)) == -1)
			continue;
		if (strcmp(dev, nvram_safe_get(strcat_i("if_dev", i, tmp))) != 0)
			continue;
	
		rev = i;
		break;
	}
	return rev;
}
/*
 * Output: Return index of @zone or -1 on failure.
 * 	@out is point to offset of zone_if_listXX within nvram which
 * 	matched by @ii_alias.
 * */
static int zone_find_index(const char *ii_alias, char **out)
{
	char *p, *s, buf[512];
	int rev = -1, i = 0;
	
	foreach_nvram(i, 3, p, "zone_if_list", g2, 1) {
		if (p == NULL)
			continue;
		if ((s = strstr(p, ii_alias)) == NULL)
			continue;
		*out = s;	/* @s come from nvram, don't worry about stack */
		rev = i;
		break;
	}
	rd((rev == -1), "error");	
	return rev;
}

static int zone_detach(struct info_if *ii)
{
	char *p, *s, *if_list, buf[512];
	int rev = -1, i = 0;
	
	if ((i = zone_find_index(ii->alias, &s)) == -1)
		goto err;
	if ((if_list = nvram_get_i("zone_if_attached", i , g1)) == NULL)
		goto err;
	if ((if_list = strdup(if_list)) == NULL)
		goto err;
	
	debug( "delete:[%s] from [%s]", ii->alias, if_list);
	if (strlist_del(if_list, ii->alias) == -1) {
		debug( "[%s] not attached yet!", ii->alias);
		goto err;
	}
	debug( "deleted:%s as [%s]", ii->alias, if_list);
	nvram_set_i("zone_if_attached", i, g1, if_list);
	if (p = strchr(if_list, ' ') != NULL) {
		*p = '\0';
		if (strlen(if_list) > 0) {
			debug( "==========STOP NEXT INTERFACE ALIAS:[%s]", if_list);
			rev = II_STOP_MAIN("stop", if_list);
		}
	}
	free(if_list);
	rev = 0;
err:
	rd(rev, "error");
	return rev;	
}

static int zone_attach(struct info_if *ii)
{
	char *p, *s, *ath, buf[512];
	int rev = -1, i = 0;
	
	if ((i = zone_find_index(ii->alias, &s)) == -1)
		goto err;
	if ((ath = nvram_get_i("zone_if_attached", i , g1)) == NULL)
		goto err;
	
	if (strlen(ath) == 0)
		strcpy(buf, ii->alias);
	else
		sprintf(buf, "%s %s", ath, ii->alias);
	debug( "nvram set zone_if_attached%d=%s", i, buf);
	nvram_set_i("zone_if_attached", i, g1, buf);
	
	s += strlen(ii->alias);
	if (strlen(s) <= 0)
		goto ready;
	SKIP_SPACE(s);
	debug( "==========START NEXT INTERFACE:[%s]", s);
	rev = II_START_MAIN("start", s);
ready:
	rev = 0;
err:
	rd(rev, "error :%s", ii->alias);
	return rev;	
	
}
int zone_attach_detach(int attach, struct info_if *ii)
{
	return (attach) ? zone_attach(ii) : zone_detach(ii);
}
static void dump_zone(int i)
{
	char *z, *ia, *ath;
	
	z = nvram_get_i("zone_alias", i, g1);
	ia = nvram_get_i("zone_if_list", i, g1);
	ath = nvram_get_i("zone_if_attached", i, g1);
	printf("%s: if: %s, ready: %s\n", z, ia, ath);
	return;
}

static int zone_showconfig_main(int argc, char *argv[])
{
	int i = 0;
	char *p;
	
	foreach_nvram(i, 20, p, "zone_alias", g1, 1) {
		if (p == NULL)
			continue;
		dump_zone(i);
	}
}

static int zone_status_main(int argc , char *argv[])
{
	int rev = -1;
	int i = 0;
	char *p, *h, *t;

	
	if (argc < 2)
		goto out;
	if ((i = nvram_find_index("zone_alias", argv[1])) == -1)
		goto out;
	if ((h = strdup(nvram_get_i("zone_if_list", i, g1))) == NULL)
		goto out;
	t = h;

	while (t != NULL && (p = strsep(&t, " ")) != NULL) {
		printf("[%s]\n", p);
		
		ii_status_main_2(2, (char *[]){"status", p});
	}
	/*
	foreach_nvram(i, 20, p, "zone_alias", g1, 1) {
		if (p == NULL)
			continue;
		dump_zone(i);
	}
	*/
out:
	return rev;
		
}
int zone_main(int argc, char *argv[])
{
	struct subcmd cmds[] = {
		{ "start", zone_start_main},
		{ "stop", zone_stop_main},
		{ "restart", NULL},
		{ "status", zone_status_main},
		{ "showconfig", zone_showconfig_main},
		{ NULL, NULL}
	};
	
	
	return lookup_subcmd_then_callout(argc, argv, cmds);
}

