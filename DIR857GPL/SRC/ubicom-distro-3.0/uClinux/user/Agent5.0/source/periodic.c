/*=======================================================================
  
       Copyright(c) 2009, Works Systems, Inc. All rights reserved.
  
       This software is supplied under the terms of a license agreement 
       with Works Systems, Inc, and may not be copied nor disclosed except 
       in accordance with the terms of that agreement.
  
  =======================================================================*/
/*!
 * \file periodic.c
 *
 */
#include <stdlib.h>
#include <string.h>

#include "cli.h"
#include "log.h"
#include "tr_lib.h"
#include "sched.h"
#include "event.h"
#include "tr_strings.h"
#include "tr_strptime.h"
#include "periodic.h"
#include "war_string.h"

static struct sched sc; //The periodic scheduler
static int interval = 3600;
static int enabled = 0;
static time_t pit = 0; //Periodic Inform Time

/*!
 * \brief Calculate the next periodic inform time.
 *
 * \return The time the next periodic will expired
 */
static time_t periodic_next_timeout()
{
	time_t cur;
	time_t timeout;
	cur = current_time();

	if(enabled) {
		timeout = (pit - cur) % interval;
		if(timeout > 0)
			timeout += cur;
		else
			timeout += interval + cur;
	} else {
		timeout = (time_t)(-1); //nerver timeout
	}

 
		tr_log(LOG_NOTICE, "next timeout = %s", 
                ctime(&timeout));
        //tr_log(LOG_DEBUG, "pit:%d, cur:%d, interval:%d, timeout:%d, diff:%d\n", (int)pit, (int)cur, (int)interval, (int)timeout, (int)timeout - (int)cur);
	return timeout;
}

/*!
 * \brief Value change trigger callback function for parameter PeriodicInterval
 *
 * \param new The new value for the PeriodicInterval
 */
static void periodic_interval_changed(const char *new)
{
	int i;
	i = atoi(new);

	sc.timeout = sc.timeout - interval + i;
	interval = i;
}

/*!
 * \brief Value change trigger callback function for parameter PeriodicEnable
 *
 * \param new The new value for the PeriodicEnable
 */
static void periodic_enable_changed(const char *new)
{
	int old = enabled;
	enabled = string2boolean(new);
	if(enabled == BOOLEAN_ERROR)
		enabled = old;

	if(enabled != old) {
		sc.timeout = periodic_next_timeout();
	}
}

/*!
 * \brief Value change trigger callback function for parameter PeriodicInformTime
 *
 * \param new The new value for the PeriodicInformTime
 */
static void periodic_inform_time_changed(const char *new)
{
	if(war_strcasecmp(new, UNKNOWN_TIME) != 0) {
		pit = string_time2second(new);
                //if(pit != (time_t)(-1)) {  //fix 20101122
                if(pit == (time_t)(-1)) {
			pit = 0;
		}
	} else {
		pit = 0;
	}

	sc.timeout = periodic_next_timeout();
}

/*!
 * \brief The on_timeout callback function for periodic scheduler
 *
 * \param sched The periodic scheduler
 */
static void periodic_timeout(struct sched *sched)
{
	if(enabled) {
		tr_log(LOG_DEBUG, "Periodic inform timeout");
		add_single_event(S_EVENT_PERIODIC);
		complete_add_event(0);
	}
	sc.timeout = periodic_next_timeout();
}

int launch_periodic_sched()
{
	int res = 0;
	char buf[32] = UNKNOWN_TIME;


	memset(&sc, 0, sizeof(sc));
	register_vct(PERIODIC_INTERVAL, periodic_interval_changed);
	register_vct(PERIODIC_ENABLE, periodic_enable_changed);
	register_vct(PERIODIC_INFORM_TIME, periodic_inform_time_changed);

	if(lib_start_session() > 0) {
		GET_NODE_VALUE(PERIODIC_INTERVAL, buf);
		if(res == 0) {
			res = atoi(buf);
			if(res > 0)
				interval = res;
		}
		res = 0;
		GET_NODE_VALUE(PERIODIC_ENABLE, buf);
		if(res == 0) {
			enabled = string2boolean(buf);
			if(enabled == BOOLEAN_ERROR)
				enabled = BOOLEAN_TRUE;
		}
		res = 0;
		GET_NODE_VALUE(PERIODIC_INFORM_TIME, buf);
		if(res != 0) {
			war_snprintf(buf, sizeof(buf), "%s", UNKNOWN_TIME);
		} 

		lib_end_session();
	}


	sc.type = SCHED_WAITING_TIMEOUT;
	sc.on_timeout = periodic_timeout;
	periodic_inform_time_changed(buf);

	add_sched(&sc);

	tr_log(LOG_DEBUG, "Launch periodic sched");
	return 0;
}
