#include <stdio.h>
#include <time.h>
#include <string.h>
#include <sys/time.h>
#include <sys/stat.h>

static char *dur_time(long es_time)
{
	char timer[128];
	int min = es_time/60;
	int sec = es_time%60;
	
	if (min > 59){
		int hr = min/60;
		min = min%60;
		sprintf(timer, "%02d:%02d:%02d", hr, min, sec);
		return timer;
	}
	
	sprintf(timer, "00:%02d:%02d", min, sec);
	return timer;
}

void calculate_time(char *fname)
{
	FILE *fp;
	char cmds[128], cmds1[128], 
	     time_str[24], time_str1[24];
	struct timeval t_start;
        struct stat buffer;
        int status;
        char *ptr, *pptr;
	ptr = strsep(&fname, "\n");
	sprintf(cmds, "%s", ptr);
	sprintf(cmds1, "/tmp/vpn_tunnel/%s", ptr);
	gettimeofday(&t_start, NULL);
	
	status = stat(cmds1, &buffer);
	if(status != 0){
		printf("faile\n");
                return;
	}
	sprintf(time_str, "%s", ctime(&buffer.st_ctime));
	pptr = time_str;
	ptr = strsep(&pptr, "\n");
	printf("%s,%s,%s#", cmds, ptr, dur_time(t_start.tv_sec-buffer.st_ctime));
}

int get_file(const char *cmds, char *tmp_file, int len)
{
	FILE *fp;
	int rev = -1;
	
	fp = popen(cmds, "r");
        if (fp == NULL)
                goto out;

        while(!feof(fp)){
                if(fgets(tmp_file, len, fp) != NULL){
			calculate_time(tmp_file);
		}
		bzero(tmp_file, sizeof(tmp_file));
        }
        pclose(fp);
        rev = 0;

out : 
	return rev;
}

int utility_vpn_main(int argc, char *argv[])
{	
	FILE *fp;
	char tmp_file[1024],cmd[128];
	chdir("/tmp");
	sprintf(cmd, "ls vpn_tunnel/");
	get_file(cmd, tmp_file, sizeof(tmp_file));
	chdir("..");
	return 0;
}
