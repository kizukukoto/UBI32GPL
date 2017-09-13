#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <stdio.h>
#include <sys/stat.h>

int
retrieve_timeout(const char * name, int * timeout);

int
retrieve_packets(const char * cmd, int * line, int * packets);

int
rule_file_add(const char * raddr, unsigned short rport, const char * iaddr, unsigned short iport, const char * protocol, const char * leasetime, int * uid);

int
rule_file_update(const char * uid, const char * leasetime);

int
rule_file_remove(const char * uid, int line_number);

int
check_rule_from_file(const char * uid, int * line);

int
get_rule_from_file(const char * raddr, unsigned short rport, char * iaddr, unsigned short * iport, char * protocol, const char * uid, char * leasetime, char * tmpuid);

int
get_rule_from_leasetime(char * uid, char * leasetime);

void
remove_rule_file();
