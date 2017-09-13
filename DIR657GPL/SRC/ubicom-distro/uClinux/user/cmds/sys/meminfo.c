#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int GetMemInfo(char *strKey, char *strOut)
{
	int j = 0, n = 0, nRes=-1;
        char s[80]="", out[80]="", key[80]="";
        FILE *file = NULL;

        if(strlen(strKey)==0) return 0;
        strcpy(key, strKey);

        file = fopen("/proc/meminfo", "r");
        if(file==NULL)
        {
                perror("fopen fail\n");
                return nRes;
        }
	while( !feof(file) ){
                memset(s, 0, sizeof(s));
                fgets(s, sizeof(s), file);
                if( strstr(s, key)>0 ){
                        nRes = 1;
                        for( n=strlen(key)+1; n<strlen(s)-4; n++ )
                        {
                                if(s[n]!=' '){
                                        out[j]=s[n];
                                        j++;
                                }
                        }
			out[j]='\0';
			//strOut = out;
			strcat(strOut, out);
			//printf("Key=[%s], out=[%s], strOut=[%s]\n", key, out, strOut);
			//printf("%s\n", out);
		}
	}
	fclose(file);
	return nRes;
}

int get_meminfo(int argc, char *argv[])
{
	char strTotal[32]="", strFree[32]="", strUsed[32]="";
	char *strKey="";
	long int used = 0;
	
	memset( strTotal, 0, sizeof(strTotal) );
	memset( strTotal, 0, sizeof(strFree) );
	memset( strTotal, 0, sizeof(strUsed) );

	if( strlen(argv[0])==0 ) return 1;

	if( strcmp(argv[0], "MemTotal") == 0 ){
		strKey = "MemTotal:";
		GetMemInfo(strKey, strTotal);
		printf("%s\n", strTotal);
	}else if( strcmp(argv[0], "MemFree") == 0 ){
		strKey = "MemFree:";
		GetMemInfo(strKey, strFree);
		printf("%s\n", strFree);
	}else if( strcmp(argv[0], "MemUsed") == 0 ){
		strKey = "MemTotal:";
		GetMemInfo(strKey, strTotal);
		strKey = "MemFree:";
		GetMemInfo(strKey, strFree);
		used = strtol(strTotal, NULL, 10) - strtol(strFree, NULL, 10);
		sprintf(strUsed, "%ld\n", used);
		printf("%s", strUsed);
	}
	return 0;
}

int meminfo_main(int argc, char *argv[])
{
	struct subcmd cmds[] = {
		{ "MemTotal", get_meminfo},
		{ "MemFree", get_meminfo},
		{ "MemUsed", get_meminfo},
		{ NULL, NULL}
	};
	return lookup_subcmd_then_callout(argc, argv, cmds);
}
