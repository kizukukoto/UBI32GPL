#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

//append:Add DNS to the last
//insert:Add DNS to the first
//del   :Del DNS
//return:
//	-1: open file fail
//	0 : success
//	1 : error in parameter
//	2 : Add DNS fail because the DNS is repeat
//	3 : Del DNE fail because the DNS can't find
int do_dns(int argc, char *argv[])
{
	FILE *file = NULL, *fileTmp = NULL;
	int nIP = 0;
	long  current_pos=0;
	char strIP[30], s[64];
	int flag = 0;
	char *action="", *strIN="";

	action = argv[0];
	strIN = argv[1];

	if(strlen(action)==0 || strlen(strIN)==0 )
		return 1;

	nIP = inet_addr(strIN);
	if(nIP<=0){
		//printf("The IP [%s]=[%d] is invalid.\n", strIN, nIP);
		return 1;
	}
	memset(strIP, 0, sizeof(strIP));
	strcat(strIP, "nameserver ");
	strcat(strIP, strIN);
	strIP[strlen(strIP)]='\n';
	//printf("DNS str=[%s], nIP=[%d]\n", strIP, nIP);

	fileTmp = fopen("/tmp/dns.tmp", "w+");
	if( fileTmp==NULL){
		//printf("open /tmp/dns.tmp fail\n");
		return -1;
	}

	file = fopen("/etc/resolv.conf", "r");
	if(file==NULL){
		//printf("open /etc/resolv.conf fail.\n");
		return -1;
	}

	while( !feof(file) ){
		if(flag==0)
			current_pos = ftell(file);
		memset(s, 0, sizeof(s));
		fgets(s, sizeof(s), file);
		fputs(s, fileTmp);
		if( strcmp(s, strIP)==0 ){
			flag = flag + 1;
			//printf("current_pos=%d\n", current_pos);
		}
	}
	fclose(file);

	fseek(fileTmp, 0, SEEK_SET);

	if( strcmp(action, "append")==0 ){
		if(flag>0)	return 2;

		file = fopen("/etc/resolv.conf", "a+");
		if(file==NULL){
			//printf("open /etc/resolv.conf fail.\n");
			return -1;
		}
		fputs(strIP, file);
		//printf("add dns\n");
		fclose(file);
	} else if( strcmp(action, "del")==0 ){
		if(flag==0) return 3;
		file = fopen("/etc/resolv.conf", "w+");
		if(file==NULL){
			//printf("open /etc/resolv.conf fail.\n");
			return -1;
		}

		while( !feof(fileTmp) ){
			memset(s, 0, sizeof(s));
			fgets(s, sizeof(s), fileTmp);
			if( strcmp(s, strIP)!=0 ){
				fputs(s, file);
			}
			//printf("ss=[%s]\n", s);
		}

		fclose(file);
		fclose(fileTmp);
		//printf("del dns\n");
	} else if( strcmp(action, "insert")==0 ){
		if(flag>0) return 2;

		file = fopen("/etc/resolv.conf", "w+");
		if(file==NULL){
			//printf("open /etc/resolv.conf fail.\n");
			return -1;
		}
		
		fputs(strIP, file);
		while( !feof(fileTmp) ){
			memset(s, 0, sizeof(s));
			fgets(s, sizeof(s), fileTmp);
			fputs(s, file);
			//printf("ss=[%s]\n", s);
		}

		fclose(file);
		fclose(fileTmp);
	
		//printf("insert dns\n");
	}
	return 0;
}

int get_dns(int argc, char *argv[])
{
	FILE *file=NULL;
	char out[1024]="", tmp[32]="", *key="nameserver ";
	int index = 0, n = 0, i = 0;

	//printf("argv[0]=[%s], argv[1]=[%s]\n", argv[0], argv[1]);
	if( strcmp(argv[0], "get") != 0 ){
		return 1;
	}

	file = fopen("/etc/resolv.conf", "r");
	if(file==NULL) {
		printf("open /etc/resolv.conf fail. \n");
		return -1;
	}

	memset(out, 0, sizeof(out));
	while( !feof(file) ) {
		memset(tmp, 0, sizeof(tmp));
		fgets(tmp, sizeof(tmp), file);
		if( strstr(tmp, key) > 0 ){
			n = strlen(key);
			for(i=0; i<strlen(tmp)-strlen(key)-1; i++){
				out[index]=tmp[n+i];
				index++;
			}
			out[index]= ' ';
			index++;
			//printf("i=[%d], n=[%d], index=[%d], out=[%s], tmp=[%s]\n", i, n, index, out, tmp);
		}
	}
	printf(out);

	fclose(file);
	return 0;
}

int my_dns_main(int argc, char *argv[])

{
	struct subcmd cmds[] = {
		{ "append", do_dns},
		{ "insert", do_dns},
		{ "del", do_dns},
		{ "get", get_dns},
		{ NULL, NULL}
	};
	return lookup_subcmd_then_callout(argc, argv, cmds);
}
