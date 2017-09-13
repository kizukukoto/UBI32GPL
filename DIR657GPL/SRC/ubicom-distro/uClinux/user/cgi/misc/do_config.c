#include <stdio.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>

#include <config.h>
#include <nvram.h>

#include "debug.h"

#define MAX_ITEMS 2048
#define NVRAM_SIZE 1024

#ifndef NVRAM_CONFIG_TMP
#define NVRAM_CONFIG_TMP ""
#endif

int in_cksum(unsigned short *, unsigned int);

int do_check_checksum(const char *chksum_value, char *upload_str)
{
	char config_str[1024*32];           // max size : 32K
	char checksum_string[32];
	unsigned long new_checksum;

	sprintf(config_str, "%s", upload_str);

	 /* calculate the checksum according to upload configuration */
        new_checksum = in_cksum((unsigned short *)&config_str[0], strlen(&config_str[0]));
        sprintf(checksum_string, "%lu", new_checksum);
        /* validate checksum consistently */
        if (strcmp(checksum_string, chksum_value) != 0 )
                return -1;

	return 1;
}

/* return checksum value */
int in_cksum(unsigned short *buf, unsigned int sz)
{
        int nleft = sz;
        int sum = 0;
        unsigned short *w = buf;
        unsigned short ans = 0;

        while (nleft > 1) {
                sum += *w++;
                nleft -= 2;
        }

        if (nleft == 1) {
                *(unsigned char *) (&ans) = *(unsigned char *) w;
                sum += ans;
        }

        sum = (sum >> 16) + (sum & 0xFFFF);
        sum += (sum >> 16);
        ans = ~sum;
        return (ans);
}

/* Get nvram key and value */
static int get_nvram_file(nvram_table *nvram_array, int len)
{
	cprintf("len :: %d\n", len);
	FILE *fp;
        char *line, *tail, *head, *ptr;
        char nvram[len], name[40], value[256], line_tmp[256];
        int i = 0;

	memset(nvram, '\0', sizeof(nvram));
	memset(name, '\0', sizeof(name));
	memset(value, '\0', sizeof(value));
	memset(line_tmp, '\0', sizeof(line_tmp));

	nvram_config2file();

        /* Read NVRAM Configuration File */
        if((fp = fopen(NVRAM_CONFIG_TMP, "r")) == 0 ) {
                return -1;
        }

	if(fread(nvram, 1, sizeof(nvram), fp) == 0) {
                printf("do_save_configuration_cgi () : file open fail \n");
                return -1;
        }
	
	head = nvram;
        line = &line_tmp[0];
        while( (tail=strstr(head,"\n"))  ) {
                memset(line, 0, 256);
                strncpy(line, head, tail-head);
                if ( (ptr=strstr(line,"=")) ) {
                        strncpy(name, line, ptr-line);
                        strncpy(value, ptr+1, strlen(ptr));

                        if(strncmp(name,"admin_",6) && strcmp(name,"wan_mac") &&
			strcmp(name,"lan_mac") && strcmp(name,"lan_eth") &&
			strcmp(name,"lan_bridge") && strcmp(name,"wan_eth") &&
			strncmp(name,"asp_temp",7) && strncmp(name,"html_",5) &&
			strcmp(name,"wlan0_domain")) {
                                strcpy(nvram_array[i].name, name);
                                strcpy(nvram_array[i].value, value);
                                i++;
                                memset(name, 0, 40);
                                memset(value, 0, 256);
                        }
                } else
                        break;
                head = tail+1;
        }

	fclose(fp);
        remove(NVRAM_CONFIG_TMP);

	return i;
}

static int paser_item(char *item, char **name, char **value)
{
        *name = item;
        *value = strchr(item, '=');
        if (*value != NULL) {
                *(*value)++ = '\0';
                return 0;
        }
        return -1;
}

static int save_items(char **affirm)
{
        char *fname, *fvalue, *p, **s;

        for (s = affirm; *s; s++) {
                if (strlen(*s) <= 0)
                        continue;
                //cprintf("S:%s\n", *s);
                p = strdup(*s);
                //cprintf("%s:%d::affirm=[%s]\n", __func__, __LINE__, p);
                if (paser_item(p, &fname, &fvalue) == 0)
                        nvram_set(fname, fvalue);
                free(p);
        }
        nvram_commit();
        return 0;
}

/* Return: count of key which need updated. */
static int confirm_items(char **affirm)
{
        char *fname, *fvalue, **s, *p;
        int f = 0, c = 0;
	nvram_table nvram_array[NVRAM_SIZE];
        int i = 0, size = 0;

	size = get_nvram_file(nvram_array, sizeof(nvram_array));

	for (s = affirm; *s; s++) {
                //cprintf("Fetch:%s :", *s);
                if ((p = strdup(*s)) == NULL)
                        continue;
                if (paser_item(p, &fname, &fvalue) == -1)
                        continue;
                //cprintf("%s:%s\n", fname, fvalue);
                f = 0;
		for (i = 0; i < size; i++) {
			if (strcmp(fname, nvram_array[i].name) != 0)
				continue;
			c++;
			break;		
		}
		if (f == 0)
                        **s = '\0';
                //cprintf(" OK\n");
                free(p);
        }

        cprintf("%d return need to be updated\n", c);
        return c;
}

/* Parser from stdin only, syntax is "@key=@len-@value */
static int init_item(char *buf, char **key, char **value)
{
        char *k, *v;

        if (paser_item(buf, &k, &v) == 0) {
                int ret;
                *key = k;
                *value = v;
                if (strchr(*value, '-') == NULL)
                        goto err;
                ret = atoi(strsep(&(*value), "-"));
                return ret >= 4096 ? -1 : ret;
        }
err:
        return -1;
}

/*
 * Initail @affirm from stdin, but did not save them to nvram at the moment.
 * */
static int load_data(int num, char **affirm)
{
        char buf[4352], chksum[32]; /* for IPSec CA must 4096 + org 256 bytes*/
        int i = 0, ret = -1;
        char *k, *v, *vtmp = NULL, *checksum;
        int len = 0, tlen;
	char upload_config_str[1024*32];           // max size : 32K

	memset(upload_config_str, '\0', sizeof(upload_config_str));

        do {
                if (fgets(buf, sizeof(buf), stdin) == NULL) {
                        //cprintf("total line=%d\n", line);
                        ret = 0;        /* Eof from stdin*/
                        break;
                }

		if (strstr(buf, "config_checksum") != NULL) {
			checksum = buf;
			k = strsep(&checksum, "=");
			sprintf(chksum, "%s", checksum);
			ret = 0;
			break;
		} else {
			sprintf(upload_config_str, "%s%s", upload_config_str, buf);
		}

                if (len == 0 && (buf[0] == '\r' || buf[0] == '\n'))
                        continue;

                if (len == 0) {
                        vtmp = NULL;
                        if ((len = init_item(buf, &k, &v)) <= 0)
                                continue;
                        tlen = len;
                        //cprintf("LEN:%d, %s:%s", len, k, v);
                        /* 2 for : '=' + '\0' */
                        if ((vtmp = malloc(strlen(k) + len + 2)) == NULL) {
                                cprintf("memory located failure!");
                                break;
                        }
                        vtmp[strlen(k) + len + 1] = '\0';

                        strcpy(vtmp, k);
                        strcat(vtmp, "=");
                        strncat(vtmp, v, len);
                        len -= len > strlen(v) ? strlen(v) : len;
                } else {
                        /* stream is continue from the value of last key.*/
                        if (vtmp == NULL)
                                continue;
                        strncat(vtmp, buf, len);
                        len -= strlen(buf);
                }

                if (len > 0 || vtmp == NULL)
                        continue;
                //cprintf("item: %d|%s\n\n", tlen, vtmp);
                /*
                 * init to @affirm array if syntax ok
                 * fmt of @vtmp is "key=value"
                 */
                affirm[i] = vtmp;
                i++;
                num--;
                len = 0;

        } while(num);

	if(!chksum)
                ret = -1;
        else
		ret = do_check_checksum(chksum, upload_config_str);
	
        cprintf("%d item pre-parsed! %d Returned\n", i - 1, ret);
        return i <= 0 ? -1: ret;
}


static int load_configuration()
{
	char *affirm_items[MAX_ITEMS];
        int ret = -1, i;
	
	/* initial affirm_items point */
        for (i = 0; i <= MAX_ITEMS; i++) 
                affirm_items[i] = (char *)NULL;

	/* load_data that means it reads the data of upload from stdin */
        if ((ret = load_data(sizeof(affirm_items)/sizeof(char *), 
		   affirm_items)) == -1)
                goto out;

        ret = confirm_items(affirm_items);

        if (ret <= 0) {
                ret = -1;
                goto out;
        }

        cprintf("save to flash\n");
        save_items(affirm_items);
        cprintf("%d::Finish save_items\n", __LINE__);

        cprintf(".done\n");
        ret = 0;
out:
        for (i = 0; affirm_items[i] != NULL; i++) {
                free(affirm_items[i]);
        }
        cprintf("load():%d\n", ret);
        return ret;
}

static int save_configuration()
{
        int i = 0, size = 0;
	unsigned long checksum;
	char nvram_config_str[1024*32];           // max size : 32K
        nvram_table nvram_array[NVRAM_SIZE];

	memset(nvram_config_str, '\0', sizeof(nvram_config_str));

	size = get_nvram_file(nvram_array, sizeof(nvram_array));

        for(i = 0; i < size; i++) {
		printf("%s=%d-%s\n", nvram_array[i].name, \
			strlen(nvram_array[i].value), \
			nvram_array[i].value);

		sprintf(nvram_config_str, "%s%s=%d-%s\n", nvram_config_str, nvram_array[i].name, \
			strlen(nvram_array[i].value), \
			nvram_array[i].value);
        }

	checksum = in_cksum((unsigned short *)&nvram_config_str[0], strlen(&nvram_config_str[0]));

	return 0;
}

int do_config_main(int argc, char *argv[])
{
	int ret = -1;

	if (strcmp(argv[1], "save") == 0)
		ret = save_configuration();
	
	if (strcmp(argv[1], "load") == 0)
		ret = load_configuration();

	//if (ret < 0)
	//	system("echo '-1' > /tmp/tmp/do_config.log");

	cprintf("Command miss out[%s].\n", argv[1]);
	return ret;
}
