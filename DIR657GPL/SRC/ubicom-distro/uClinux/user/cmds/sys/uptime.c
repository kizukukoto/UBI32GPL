#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int uptime_main(int argc, char *argv[])
{
	//char *out;
        unsigned long int ultime = 0;
        int day = 0, hour = 0, minute = 0, second = 0;
        char s[64]="", tmp[64]="";
        int i = 0;
        FILE *file=NULL;

        file = fopen("/proc/uptime", "r");
        if(file==NULL){
                printf("open /proc/uptime fail.\n");
                return -1;
        }

        memset(tmp, 0, sizeof(tmp));
        fgets(s, sizeof(s), file);
        if( strstr(s, ".")>0 ){
                for(i=0; i<strlen(s); i++)
                {
                        if(s[i]=='.') break;
                        tmp[i] = s[i];
                }
                tmp[i]='\0';
                //strcat(out, tmp);
                //printf("The open time is [%s]\n", tmp);
        }
        fclose(file);

        ultime = strtoul(tmp, NULL, 10);
        if(ultime>0){
                day = ultime/(3600*24);
                ultime = ultime - 3600*24*day;
                hour = ultime / 3600;
                ultime = ultime - 3600 * hour;
                minute = ultime / 60;
                second = ultime - minute * 60;
                printf("%d days, %d:%d:%d\n", day, hour, minute, second);
        }
	return 0;
}

