#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "log.h"
#include "nvram.h"
#include "shvar.h"
//jimmy added
#include "sutil.h"
//-----------

log_dynamic_table_t log_dynamic_table[MAX_LOG_LENGTH];

// aaron 2009.10.21
//char *hostname = NULL;
char hostname[64+1]; // check define 65 bytes

void fill_log_into_structure(char *time, char *type, char *messages, int index_t)
{
	int i;
	for(i = 0; i < strlen(messages);i++)
	{
		if(messages[i] == '"')
			messages[i] = ' ';
		else if(messages[i] == '<')
			messages[i] = '\'';
		else if(messages[i] == '>')
			messages[i] = '\'';
	}
	strcpy(log_dynamic_table[index_t].time , time);
	log_dynamic_table[index_t].type = type;
	strcpy(log_dynamic_table[index_t].message , messages);
}

// aaron 2009.10.21
// ref buzybox, use uname -n
#include <sys/utsname.h>

/*
 * On success return the current malloced and NUL terminated hostname.
 * On error return malloced and NUL terminated string "?".
 * This is an illegal first character for a hostname.
 * The returned malloced string must be freed by the caller.
 */
void safe_gethostname_uname(char *hostname)
{
	struct utsname uts;

	/* The length of the arrays in a struct utsname is unspecified;
	 * the fields are terminated by a null byte.
	 * Note that there is no standard that says that the hostname
	 * set by sethostname(2) is the same string as the nodename field of the
	 * struct returned by uname (indeed, some systems allow a 256-byte host-
	 * name and an 8-byte nodename), but this is true on Linux. The same holds
	 * for setdomainname(2) and the domainname field.
	 */

	/* Uname can fail only if you pass a bad pointer to it. */
	uname(&uts);

	//return xstrndup(!uts.nodename[0] ? "?" : uts.nodename, sizeof(uts.nodename));

	if( !uts.nodename[0] ) {
		// no hostname
	}
	else {
		memcpy(hostname,uts.nodename, sizeof(uts.nodename));
		

	}
}


int check_log_type(char *input, char *type, int index_t)
{
	char *time = NULL;
	char *messages=NULL;
	char *type_index=NULL;
	int len_time, len_message, len_type;
	char *tmpMessages=NULL;
	
/* jimmy added 20080819, for avoid httpd crash */
//	hostname= nvram_get("hostname");
// aaron 2009.10.21
//	hostname= nvram_safe_get("hostname");
/* ----------------------------------------- */

	if(strstr(input,type) != NULL)
	{
		tmpMessages = strstr(input, type) + strlen(type) +1;
		if(strchr(tmpMessages,':') == 0)
			return 0;
/*
* Albert Chen 2009-06-26
* Detail take off not necessary define
*/
#if 0 // AR9100
		if (/*strstr(tmpMessages,"kernel") != 0 || */ strstr(tmpMessages,"init") != 0)
			return 0;
#endif			
		messages    = strchr(tmpMessages,':') + 1;
		len_message = strlen(messages);
		messages[len_message-1] = '\0';
		if(strstr(input, hostname) == 0)
			return 0;
		type_index = strstr(input, hostname) + strlen(hostname) +1 ;
		len_type = strchr(type_index, ' ') - type_index;
		type_index[len_type]= '\0';
		time = input;
		len_time = strstr(input, hostname ) - time ;
		time[len_time] = '\0';

		fill_log_into_structure(time,type,messages,index_t);
		return 1;
	}
	return 0;
}

int update_log_table(int log_system_activity, int log_debug_information, int log_attacks, int log_dropped_packets, int log_notice)
{
	FILE *fp;
	char buffer[log_length] = "";
	int index_t = 0;

//	hostname= nvram_get("hostname");
// aaron 2009.10.21
	hostname[0] = 0;
	safe_gethostname_uname(hostname);


/* jimmy modified for IPC syslog */
/*
 Reason: let ui show log
 Modified: John Huang
 Date: 2009.08.04
*/
//	_system("logread -r > %s ",LOG_FILE_HTTP);
//	_system("logread | tac > %s ",LOG_FILE_HTTP);
        _system("cat %s | tac > %s ",LOG_FILE_BAK ,LOG_FILE_HTTP);
//------------
	if((fp = fopen(LOG_FILE_HTTP,"r+")) == NULL)
		return 0;
	else	
	{
		while(fgets(buffer,log_length,fp))
		{
			if(log_notice == 1 )
			{
				index_t += check_log_type(buffer,"notice",index_t);
			}

			if(log_debug_information == 1)
			{
				index_t += check_log_type(buffer,"debug",index_t);
			}

			if(log_system_activity == 1)
			{
				index_t += check_log_type(buffer,"info",index_t);
			}

			if(log_attacks == 1)
			{
				index_t += check_log_type(buffer,"attack",index_t);
			}

			if(log_dropped_packets == 1)
			{
				index_t += check_log_type(buffer,"dropped",index_t);
			}

			if(index_t > MAX_LOG_LENGTH) 
				index_t = MAX_LOG_LENGTH;
		}//end while
	}//end if

	fclose(fp);

/*
 Reason: let ui show log
 Modified: John Huang
 Date: 2009.08.04
*/
#if 0 // AR9100

	if((fp = fopen(LOG_FILE,"r+")) == NULL)
		return 0;
	else	
	{
		while(fgets(buffer,log_length,fp))
		{
			if(log_attacks == 1)
			{
				index_t += check_log_type(buffer,"attack", index_t);
			}
			if(index_t > MAX_LOG_LENGTH) 
				index_t = MAX_LOG_LENGTH;
		}//end while
	}//end if

	fclose(fp);
#endif

	return index_t;
}
