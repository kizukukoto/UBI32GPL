/*=======================================================================
  
       Copyright(c) 2009, Works Systems, Inc. All rights reserved.
  
       This software is supplied under the terms of a license agreement 
       with Works Systems, Inc, and may not be copied nor disclosed except 
       in accordance with the terms of that agreement.
  
  =======================================================================*/
/*!
 * \file retry.c
 *
 * \brief The session retry mechanism implementation of the TR069 protocol.
 */

#include <stdlib.h>


#include "log.h"
#include "retry.h"
#include "event.h"
#include "sched.h"
#include "session.h"
#include "war_math.h"
#include "war_time.h"


static int retry_count = 0; 
static int boot_retry_count = 0; //The counter for deliver the 1 BOOT event.
static int retrying = 0;

static void retry_timeout(struct sched *sc);


/*!
 *\fn inc_boot_retry_count
 *\brief Increase the boot retry counter by 1 if it is not greater than 10
 *\return N/A
 */
static void inc_boot_retry_count(void)
{
	//RT-069 Amendent 2 Page 18, the boot retry count can not greater than 10
	if (boot_retry_count < 10)
		boot_retry_count++;
}

/*!
 *\fn inc_retry_count
 *\brief Increase the retry counter
 *\return N/A
 */
static void inc_retry_count(void)
{
	struct event *e;

	e = get_event(S_EVENT_BOOT, NULL);
	if(e)
		inc_boot_retry_count();

	retry_count++;
}


/*!
 *\fn retry_timeout
 *\brief The callback function for retry scheduler timeout.
 *\param sc The retry scheduler.
 *\return N/A
 */
static void retry_timeout(struct sched *sc)
{
	sc->need_destroy = 1;
	retrying = 0;
	create_session();
}

/*!
 *\fn retry_interval
 *\brief Calculate the time(how many seconds to wait before retry the session) to wait
 *\return N/A
 */
static int retry_interval(void)
{
	int i;
	int res;

	war_srandom(war_time(NULL));

        //TR069 Amendent 2 Page 18:
/*
 * Post reboot session retry count         Wait interval range (min-max seconds)
 * #1                                      5-10
 * #2                                      10-20
 * #3                                      20-40
 * #4                                      40-80
 * #5                                      80-160
 * #6                                      160-320
 * #7                                      320-640
 * #8                                      640-1280
 * #9                                      1280-2560
 * #10 and subsequent                      2560-5120
 */

	i = 1 << (boot_retry_count > 0 ? boot_retry_count - 1 : 0);
	res = i * 5;


	res = res + (war_random() % res);

	return res;
}


/*!
 *\fn reset_retry_count
 *\brief Reset the retry counter after each successful session
 *\return N/A
 */
void reset_retry_count(void)
{
	retry_count = 0;
}

/*!
 *\fn get_retry_count
 *\brief Get the retry count to construct the <b>inform</b> method.
 *\return retry count
 */
int get_retry_count(void)
{
	return retry_count;
}

/*!
 *\fn retry_later
 *\brief Retry the session. If the current session failed, call this API to retry it(after retry_interval).
 *\return N/A
 */
void retry_later(void)
{
	if (retrying == 0) {
		static struct sched *retry;

		inc_retry_count();
		retry = calloc(1, sizeof(*retry));
		if (retry == NULL) {
			tr_log(LOG_ERROR, "Out of memory!");
		} else {
			int seconds;
			//Retry scheduler just need wait for timeout then create a new session
			retry->type = SCHED_WAITING_TIMEOUT;
			retry->on_timeout = retry_timeout; //timeout callback function
			seconds = retry_interval();
			retry->timeout = current_time() + seconds;
			add_sched(retry);
			tr_log(LOG_DEBUG, "Retry session after %d second%s", seconds, seconds > 1 ? "s" : "");

			retrying = 1;
		}
	}
}
