/*=======================================================================
  
       Copyright(c) 2009, Works Systems, Inc. All rights reserved.
  
       This software is supplied under the terms of a license agreement 
       with Works Systems, Inc, and may not be copied nor disclosed except 
       in accordance with the terms of that agreement.
  
  =======================================================================*/
/*=======================================================================
  
  
       This software is supplied under the terms of a license agreement 
       with Works Systems, Inc, and may not be copied nor disclosed except 
       in accordance with the terms of that agreement.
  
 =======================================================================*/

/*!
 * \file tr.c
 * 
 * \mainpage TRAgent - TR-069 Client Implementation
 *   
 * \par Developer Documentation for TRAgent
 * This is the developer documentation for TRAgent.
 *
 * \image html tr069.png
 *
 * \section license License
 * \verbinclude doc/LICENSE
 *
 * \page architecture Architecture
 * \section revision Revision History
 * <table style="text-align:center">
 *	 <tr style="background-color: rgb(204, 204, 204)">
 *           <td>Date</td>
 *           <td>Version</td>
 *           <td>Author</td>
 *           <td>Description</td>
 *       </tr>
 *       <tr>
 *           <td>2008.09.20</td>
 *           <td>1.0</td>
 *           <td>Draft</td>
 *       </tr>
 * </table>
 *
 * \section introduction Introduction
 * This version agent is designed to make it can be ported to any OS with little R&D effort.
 * As you know, the TR069 protocol is a powerful but complex device management protocol. It
 * consists of many logical modules: session, TCP notification, file transfer, diagnostics,
 * STUN client and some local integration modules. Those former versions have implemented
 * these modules in a multi-thread appilication. Because the different OS has different
 * thread library approach, so I decided to make it in a single-therad application which
 * deived by a scheduler like libevent.
 *
 * \image html architecture.png
 */



#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>

#include "war_time.h"
#include "war_type.h"
#include "tr.h"
#include "periodic.h"
#include "trconfig.h"
#include "log.h"
#include "tr_strings.h"
#include "atomic.h"
#include "event.h"
#include "tr_lib.h"
#include "task.h"
#include "sched.h"
#include "tcp.h"
#include "udp.h"
#include "cli.h"
#include "inform.h"
#include "request.h"
#include "ssl.h"
#include "echo.h"
#include "wib.h"
#include "war_string.h"
#include "war_file.h"
#include "war_math.h"
#include "war_socket.h"
#include "war_time.h"

#ifdef TR196
#include "pm.h"
#endif

static char work_directory[256] = WORK_DIRECTORY; 

static char *init_arg = NULL;

int set_init_arg(const char *name, const char *value)
{
	if(init_arg)
		free(init_arg);

	init_arg = war_strdup(value);
	if(init_arg == NULL) {
		tr_log(LOG_ERROR, "Out of memory!");
		return -1;
	}

	return 0;
}

long int tr_random()
{
    war_srandom(war_time(NULL));
    return war_random();
}

short int tr_atos(const char *str)
{
    short int res = 0;
    int minus = 0;
    
    str = skip_blanks(str);
    if((*str) == '-') {
        str++;
        minus = 1;
    }

    while(*str && *str >= '0' && *str <= '9') {
        res = res * 10 + (*str) - '0';
        str++;
    }

    if(minus)
        res = -res;

    return res;
}

long int tr_file_len(FILE *fp)
{
	long int len, cur;

	cur = ftell(fp); //Record the "current position"
	fseek(fp, 0, SEEK_END);
	len = ftell(fp);
	fseek(fp, cur, SEEK_SET); //Restore the file pointer to the "current position"

	return len;
}

/*!
 * \brief Calculate a file's full name
 * If the file name is a absolute path, the full name is the same as the file name, or
 * it will be a file under the current work directory
 *
 * \param fn The file name
 * \param full_fn The buffer to hold the full file name
 * \param full_len The full file name buffer size
 *
 * \return N/A
 */
void tr_full_name(const char *fn, char *full_fn, int full_len)
{
	if(war_judge_absolutepath(fn)) {
		war_snprintf(full_fn, full_len, "%s", fn);
	} else {
		war_snprintf(full_fn, full_len, "%s%s", work_directory, fn);
	}
}


FILE *tr_fopen(const char *name, const char *mode)
{
	char fn[FILE_PATH_LEN];
	tr_full_name(name, fn, sizeof(fn));
	return fopen(fn, mode);
}


int tr_exist(const char *name)
{
	FILE *fp;

	fp = tr_fopen(name, "rb");
	if (fp) {
		fclose(fp);
		return 1;
	}

	return 0;	
}

int tr_create(const char *name)
{
	FILE *fp;

	fp = tr_fopen(name, "wb");
	if (fp) {
		fclose(fp);
		return 1;
	}

	return 0;	
}

int tr_rename(const char *old_name, const char *new_name)
{
	if(tr_exist(old_name)) {
		char old[FILE_PATH_LEN];
		char new[FILE_PATH_LEN];

		tr_full_name(old_name, old, sizeof(old));
		tr_full_name(new_name, new, sizeof(new));

		war_unlink(new);

		return war_rename(old, new);
	} else {
		return -1;
	}
}

int tr_remove(const char *file_name)
{
	char fn[FILE_PATH_LEN];

	tr_full_name(file_name, fn, sizeof(fn));

	return war_unlink(fn);
}

int tr_backup(const char *name)
{
	char backup[FILE_PATH_LEN];

	war_snprintf(backup, sizeof(backup), "%s.bak", name);

	return tr_rename(name, backup);
}

int tr_remove_backup(const char *name)
{
	char backup[FILE_PATH_LEN];

	war_snprintf(backup, sizeof(backup), "%s.bak", name);

	return tr_remove(backup);
}


int tr_restore(const char *name)
{
	char backup[FILE_PATH_LEN];

	war_snprintf(backup, sizeof(backup), "%s.bak", name);

	return tr_rename(backup, name);
}


//int cpe_agent(int argc, char *argv[])
int main(int argc, char *argv[])
{
    	if(init_argument(argc, argv, work_directory, sizeof(work_directory)) != 0)
		exit(1);

	if(read_config_file() != 0) {
		tr_log(LOG_ERROR, "Read config file failed!");
		exit(1);
	}
	start_logger();
	if(lib_init(init_arg) != 0) {
		tr_log(LOG_ERROR, "Init tree failed!");
		exit(1);
	}
	clear_journals();

        load_event();

	if (tr_exist(FLAG_NEED_FACTORY_RESET)) {
		tr_log(LOG_ERROR, "tr_exist(FLAG_NEED_FACTORY_RESET)");
		add_single_event(S_EVENT_BOOTSTRAP);
		complete_add_event(0);
		tr_create(FLAG_BOOTSTRAP); /*If the FLAG_BOOTSTRAP exists, it means that it is not the FIRST INSTALL*/
		lib_factory_reset();
		/*If power fault here, it doesn't matter*/
		tr_remove(FLAG_NEED_FACTORY_RESET);
		lib_reboot();
		exit(0);
	}

 	if (!tr_exist(FLAG_BOOTSTRAP)) { 
		add_single_event(S_EVENT_BOOTSTRAP);
	#ifdef TR069_WIB
		launch_wib_sched();
	#else /*If the FLAG_BOOTSTRAP exists, it means it is the FIRST INSTALL*/
		tr_create(FLAG_BOOTSTRAP);
	#endif
	}

	add_single_event(S_EVENT_BOOT);

	load_request();
	load_task();

	war_loadsock();
#if 0
	launch_tcp_listener();
	launch_cli_listener();
#endif
/*
Note: complete_add_event()  launch_udp_listener() launch_periodic_sched() will send INFORM
When device first online(without FLAG_BOOTSTRAP), forbidden them before WIB
*/
#ifdef TR069_WIB /*if defined TR069_WIB, launch_periodic_sched is called after bootstrap, if no need of bootstrap, launch_periodic_sched is called*/
	if (tr_exist(FLAG_BOOTSTRAP)) { 
		tr_log(LOG_DEBUG, "ifdef TR069_WIB and tr_exist(FLAG_BOOTSTRAP) and call launch_periodic_sched");
	        launch_tcp_listener();
 	        launch_cli_listener();
		launch_periodic_sched();
		complete_add_event(0);
		#ifdef TR111
			#ifndef __DEVICE_IPV6
				launch_udp_listener();
			#endif
		#endif
		#ifdef TR196
				launch_periodic_upload_sched();
		#endif
	}
#else
	launch_tcp_listener();
	launch_cli_listener();
	launch_periodic_sched();
	complete_add_event(0);
	#ifdef TR111
		#ifndef __DEVICE_IPV6 
			launch_udp_listener();
		#endif
	#endif
	#ifdef TR196
			launch_periodic_upload_sched();
	#endif
#endif

#ifdef TR143
	launch_echo_server();	
#endif
	start_sched();
	tr_log(LOG_DEBUG, "Exit OK");
	return 0;
}
