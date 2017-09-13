#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <stdio.h>
#include <sys/stat.h>

#include "upnpglobalvars.h"
#include "addressManip.h"

int
retrieve_timeout(const char * name, int * timeout)
{
	char fileName[64] = "/proc/sys/net/netfilter/nf_conntrack_", str[12];
	strcat(fileName, name);
	printf("\tFile Name: %s\n", fileName);
	
	FILE * fd;
	fd = fopen(fileName, "r");
	if (fd==NULL)
	{
		syslog(LOG_ERR, "could not open ip6tables rule file: %s", fileName);
		return -1;
	}
	else
	{
		fgets(str, sizeof(str), fd);
		printf("\tRetrieve timeout = %s\n", str);
		*timeout = atoi(str);
		fclose(fd);
		return 1;
	}
}

int
retrieve_packets(const char * cmd, int * line, int * packets)
{
	char numline[5];
	snprintf(numline, sizeof(numline), "%d", *line);
	printf("get number of packets for line n°%s\n", numline);
	char pkts[12], bytes[12], target[8], prot[8], opt[8], in[12], out[12], source[40], destination[40], eport[20], iport[20];

	FILE * p;
	p = popen(cmd, "r");
	if (p==NULL)
	{
		syslog(LOG_ERR, "could not perform the command: %s", cmd);
		return -1;
	}
	else
	{
		fscanf(p,"%s %s %s %s %s %s %s %s %s %s %s\n", pkts, bytes, target, prot, in, out, source, destination, opt, eport, iport);
		printf("num: %s - pkts: %s - bytes: %s - target: %s - prot: %s - opt: %s - in: %s - out: %s - source: %s - destination: %s\n", numline, pkts, bytes, target, prot, opt, in, out, source, destination);
		*packets=atoi(pkts);
		printf("\tRetrieve packets = %d\n", *packets);
		fclose(p);
		return 1;
	}
}

int
rule_file_add(const char * raddr, unsigned short rport, const char * iaddr, unsigned short iport, const char * protocol, const char * leasetime, int * uid)
{
	FILE * fd;
	char remoteHost[INET6_ADDRSTRLEN] = "", internalClient[INET6_ADDRSTRLEN] = "";

	if (rulefilename == NULL) return -3;

	fd = fopen(rulefilename, "a");
	if (fd==NULL)
	{
		syslog(LOG_ERR, "Rule_file_add: could not open ip6tables rule file: %s", rulefilename);
		return -1;
	}
	printf("\t_add_ raddr: %s\n", raddr);
	if(raddr!=NULL)
		strcpy(remoteHost, raddr);
	else
		strcpy(remoteHost, "::");
	strcpy(internalClient, iaddr);
	del_char(remoteHost);
	del_char(internalClient);
	//fprintf(fd, "id %.4d ln %d from [%s]:%hu to [%s]:%hu proto %s lt %.5s\n", *uid, line_number, raddr, rport, iaddr, iport, protocol, leasetime);
	// Store pinhole with the folowing format:
	// IIII L XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX PPPPP XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX PPPPP UUU TTTTT
	// id line_number remoteHost remotePort internalClient internalPort protocol lease_time
	fprintf(fd, "%.4d %d %s %.5hu %s %.5hu %.5d %.10d\n", *uid, line_number, remoteHost, rport, internalClient, iport, atoi(protocol), atoi(leasetime));
	line_number++;
	fclose(fd);

	return 1;
}

int
rule_file_update(const char * uid, const char * leasetime)
{
	FILE* fd, *fdt;
	int t, line, ltime;
	char buf[512], raddr[40], rport[8], iaddr[40], iport[8], proto[6], lt[12];
	char str[5], id[5];
	char tmpfilename[128];
	int str_size, buf_size;


	if (rulefilename == NULL) return -3;

	strncpy(tmpfilename, rulefilename, sizeof(rulefilename) );
	strncat(tmpfilename, "XXXXXX", sizeof(tmpfilename) - strlen(rulefilename));

	fd = fopen(rulefilename, "r");
	if (fd==NULL)
	{
		syslog(LOG_ERR, "Rule_file_update: could not open ip6tables rule file: %s", rulefilename);
		return -1;
	}

	snprintf(str, sizeof(str), "%.4s", uid);
	str_size = strlen(str);

	t = mkstemp(tmpfilename);
	if (t==-1)
	{
		fclose(fd);
		syslog(LOG_ERR, "Rule_file_update: could not open temporary rule file");
		return -2;
	}

	fchmod(t, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
	fdt = fdopen(t, "a");
	
	buf[sizeof(buf)-1] = 0;
	while(fscanf(fd,"%4s %d %32s %5s %32s %5s %5s %10s\n", id, &line, raddr, rport, iaddr, iport, proto, lt) != EOF)
	{
		//printf("current id: %s\n", id);
		//printf("line: %d - raddr: %s - rport: %s - iaddr: %s - iport: %s - proto: %s - leasetime: %s\n", line, raddr, rport, iaddr, iport, proto, lt);
		if (strncmp(str, id, str_size)!=0)
		{
			sprintf(buf, "%s %d %s %s %s %s %s %s\n", id, line, raddr, rport, iaddr, iport, proto, lt);
		}
		else
		{
			ltime = atoi(lt) + atoi(leasetime);
			printf("Update lease time %s with %s -> %d\n", lt, leasetime, ltime);
			sprintf(buf, "%s %d %s %s %s %s %s %.10d\n", id, line, raddr, rport, iaddr, iport, proto, ltime);
		}
		buf_size = strlen(buf);
		fwrite(buf, buf_size, 1, fdt);
	}
	fclose(fdt);
	fclose(fd);
	remove(rulefilename);

	if (rename(tmpfilename, rulefilename) < 0)
	{
		syslog(LOG_ERR, "Rule_file_update: could not rename temporary rule file to %s", rulefilename);
		remove(tmpfilename);
	}

	return 1;
}

int
rule_file_remove(const char * uid, int linenum)
{
	FILE* fd, *fdt;
	int t, line;
	char buf[512], raddr[40], rport[8], iaddr[40], iport[8], proto[6], lt[12];
	char str[5], id[5];
	char tmpfilename[128];
	int str_size, buf_size;


	if (rulefilename == NULL) return -3;

	strncpy(tmpfilename, rulefilename, sizeof(rulefilename) );
	strncat(tmpfilename, "XXXXXX", sizeof(tmpfilename) - strlen(rulefilename));

	fd = fopen(rulefilename, "r");
	if (fd==NULL)
	{
		syslog(LOG_ERR, "Rule_file_remove: could not open ip6tables rule file: %s", rulefilename);
		return -1;
	}

	snprintf(str, sizeof(str), "%.4s", uid);
	str_size = strlen(str);

	t = mkstemp(tmpfilename);
	if (t==-1)
	{
		fclose(fd);
		syslog(LOG_ERR, "Rule_file_remove: could not open temporary rule file");
		return -1;
	}

	fchmod(t, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
	fdt = fdopen(t, "a");
	
	buf[sizeof(buf)-1] = 0;
	while(fscanf(fd,"%4s %d %32s %5s %32s %5s %5s %10s\n", id, &line, raddr, rport, iaddr, iport, proto, lt) != EOF)
	{
		printf("\tcurrent id: %s\n", id);
		printf("\tline: %d - raddr: %s - rport: %s - iaddr: %s - iport: %s - proto: %s - leasetime: %s\n", line, raddr, rport, iaddr, iport, proto, lt);
		if (strncmp(str, id, str_size)!=0)
		{
			if(line > linenum)
				line--;
			sprintf(buf, "%s %d %s %s %s %s %s %s\n", id, line, raddr, rport, iaddr, iport, proto, lt);
			buf_size = strlen(buf);
			fwrite(buf, buf_size, 1, fdt);
		}
	}
	fclose(fdt);
	fclose(fd);
	remove(rulefilename);

	if (rename(tmpfilename, rulefilename) < 0)
	{
		syslog(LOG_ERR, "Rule_file_remove: could not rename temporary rule file to %s", rulefilename);
		remove(tmpfilename);
	}
	line_number--;
	return 1;
}

int
check_rule_from_file(const char * uid, int * line)
{
	FILE * fd;
	char buf[512];
	int res = -4;

	if (rulefilename == NULL) return -1;

	fd = fopen(rulefilename, "r");
	if (fd==NULL)
	{
		syslog(LOG_ERR, "Check_rule: could not open ip6tables rule file: %s", rulefilename);
		return -1;
	}

	syslog(LOG_INFO, "Check_rule: Starting checking file %s for uid: %.4s\n", rulefilename, uid);
	buf[sizeof(buf)-1] = 0;
	while(fgets(buf, sizeof(buf)-1, fd) != NULL)
	{
		char str[8];
		int str_size;
		snprintf(str, sizeof(str), "%.4d", atoi(uid));
		str_size = strlen(str);
		if (strncmp(str, buf, str_size)==0)
		{
			if(line)
			{
				char * p;
				p = strstr(buf, str);
				p+=5;
				*line=atoi(p);
			}
			res = 1;
		}
	}
	fclose(fd);
	return res;
}

int
get_rule_from_file(const char * raddr, unsigned short rport, char * iaddr, unsigned short * iport, char * protocol, const char * uid, char * leasetime, char * tmpuid)
{
	FILE * fd;
	char id[5];
	char tmpHost[40], remHost[40], intClient[40], proto[6], remPort[8], intPort[8], lt[12];
	int res = -4, line=0;

	if (rulefilename == NULL) return -3;

	fd = fopen(rulefilename, "r");
	if (fd==NULL)
	{
		syslog(LOG_ERR, "Get_rule: could not open ip6tables rule file: %s", rulefilename);
		return -1;
	}

	syslog(LOG_INFO, "Get_rule: Starting getting info in file %s for %s\n", rulefilename, (!uid)? raddr:uid);
	while(fscanf(fd,"%4s %d %32s %5s %32s %5s %5s %10s\n", id, &line, remHost, remPort, intClient, intPort, proto, lt) != EOF)
	{
		//printf("line: %d - raddr: %s - rport: %s - iaddr: %s - iport: %s - proto: %s - leasetime: %s\n", line, remHost, remPort, intClient, intPort, proto, lt);
		add_char(remHost);
		add_char(intClient);
		if(!uid)
		{
			if(raddr!=NULL)
				strcpy(tmpHost, raddr);
			else
				strcpy(tmpHost, "::");
			printf("\tCompare addr: %s // port: %d // protocol: %s\n\t     to addr: %s // port: %d // protocol: %s\n", tmpHost, rport, protocol, remHost, atoi(remPort), proto);
			if((strcmp(remHost, tmpHost)==0) && rport==atoi(remPort) && (strcmp(proto, protocol)==0))
			{
				strcpy(iaddr, intClient);
				printf("\tget id = %s\n", id);
				strcpy(tmpuid, id);
				*iport=atoi(intPort);
				res = 1;
			}
		}
		else
		{
			char str[8];
			int str_size;
			snprintf(str, sizeof(str), "%.4d", atoi(uid));
			str_size = strlen(str);
			if (strncmp(str, id, str_size)==0)
			{
				strcpy(iaddr, intClient);
				strcpy(protocol, proto);
				*iport=atoi(intPort);
				strcpy(leasetime, lt);
				res = 1;
			}
		}
	}
	fclose(fd);
	return res;
}

int
get_rule_from_leasetime(char * uid, char * leasetime)
{
	FILE * fd;
	char id[5] = "0000";
	char remHost[40], intClient[40], proto[6], remPort[8], intPort[8], lt[12] = "0", ltTmp[12];
	int res = -4, line=0, tmp=0;

	if (rulefilename == NULL) return -3;

	fd = fopen(rulefilename, "r");
	if (fd==NULL)
	{
		syslog(LOG_ERR, "Get_rule: could not open ip6tables rule file: %s", rulefilename);
		return -1;
	}

	syslog(LOG_INFO, "Get_rule: Starting retrieving lower leaseTime in file %s\n", rulefilename);
	do
	{
		if(!tmp)
		{
			strcpy(uid,id);
			strcpy(leasetime,lt);
			strcpy(ltTmp,lt);
			tmp = 1;
			res = 1;
		}
		else if(atoi(ltTmp)==0 || atoi(ltTmp) > atoi(lt))
		{
			strcpy(uid,id);
			strcpy(leasetime,lt);
			strcpy(ltTmp,lt);
			res = 1;
		}

	}while(fscanf(fd,"%4s %d %32s %5s %32s %5s %5s %10s\n", id, &line, remHost, remPort, intClient, intPort, proto, lt) != EOF);
	fclose(fd);
	return res;
}

void
remove_rule_file()
{
	remove(rulefilename);
}
