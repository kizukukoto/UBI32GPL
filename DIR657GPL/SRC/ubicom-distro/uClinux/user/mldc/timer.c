
#include "mld.h"

static struct timer_lst timers_head = {
	{LONG_MAX, LONG_MAX},
	NULL,
	NULL,
	&timers_head,
	&timers_head
};

static void alarm_handler(int sig);
int inline check_time_diff(struct timer_lst *tm, struct timeval tv);


static void schedule_timer(void) {
	struct timer_lst *tm = timers_head.next;
	struct timeval    tv;

	gettimeofday(&tv, NULL);

	if (tm != &timers_head) {
		struct itimerval next;

		memset(&next, 0, sizeof(next));
		timersub(&tm->expires, &tv, &next.it_value);
		signal(SIGALRM, alarm_handler);

		if (  (next.it_value.tv_sec  >  0) ||
		     ((next.it_value.tv_sec  == 0) &&
		      (next.it_value.tv_usec >  0)) ) {
			if (setitimer(ITIMER_REAL, &next,  NULL)) {
			}
		}else{
			kill(getpid(), SIGALRM);
		}
	}
}

void set_timer(struct timer_lst *tm, int secs) {
	struct timeval     tv, firein;
	struct timer_lst  *lst;
	       sigset_t    bmask, oldmask;

	clear_timer(tm);
	firein.tv_sec  = (long)secs;
	firein.tv_usec = (long)0;//((secs - (double)firein.tv_sec) * 1000000);

	gettimeofday(&tv, NULL);
	timeradd(&tv, &firein, &tm->expires);

	sigemptyset(&bmask);
	sigaddset(&bmask, SIGALRM);
	sigprocmask(SIG_BLOCK, &bmask, &oldmask);

	/* the timers are in the list in the order they expire, the soonest first */
	lst = &timers_head;
	do {
		lst = lst->next;
	} while ( (tm->expires.tv_sec  >  lst->expires.tv_sec) ||
	         ((tm->expires.tv_sec  == lst->expires.tv_sec) &&
	          (tm->expires.tv_usec >  lst->expires.tv_usec)));

	tm->next = lst;
	tm->prev = lst->prev;
	tm->prev->next = tm;
	lst->prev = tm;

	schedule_timer(); //SIGALRM is blocked here and not immediately fired

	sigprocmask(SIG_SETMASK, &oldmask, NULL);
}


void clear_timer(struct timer_lst *tm) {
	sigset_t bmask, oldmask;

	if (!(tm->prev) || !(tm->next))
		return;

	sigemptyset(&bmask);
	sigaddset(&bmask, SIGALRM);
	sigprocmask(SIG_BLOCK, &bmask, &oldmask);
	
	tm->prev->next = tm->next;
	tm->next->prev = tm->prev;
	tm->prev = tm->next = NULL;

	schedule_timer();

	sigprocmask(SIG_SETMASK, &oldmask, NULL);
}


static void alarm_handler(int sig) {

	struct timer_lst *tm, *back;
	struct timeval    tv;

	gettimeofday(&tv, NULL);
	tm = timers_head.next;

	/*
	 * This handler is called when the alarm goes off, so at least one of
	 * the interfaces' timers should satisfy the while condition.
	 *
	 * Sadly, this is not always the case, at least on Linux kernels:
	 * see http://lkml.org/lkml/2005/4/29/163. :-(.  It seems some
	 * versions of timers are not accurate and get called up to a couple of
	 * hundred microseconds before they expire.
	 *
	 * Therefore we allow some inaccuracy here; it's sufficient for us
	 * that a timer should go off in a millisecond.
	 */

	/* unused timers are initialized to LONG_MAX so we skip them */
	while ( (LONG_MAX!=tm->expires.tv_sec) &&
	        check_time_diff(tm, tv) ) {
		tm->prev->next = tm->next;
		tm->next->prev = tm->prev;
		back = tm;
		tm = tm->next;
		back->prev = back->next = NULL;

		if (back->handler)
			(*back->handler)(back->data);
	}

	schedule_timer();
}


void init_timer( struct timer_lst  *tm,
                        void (*handler)(void*),
                        void       *data) {
	memset(tm, 0, sizeof(struct timer_lst));
	tm->handler = handler;
	tm->data    = data;
}


int inline check_time_diff(struct timer_lst *tm, struct timeval tv) {

	struct itimerval diff;
	memset(&diff, 0, sizeof(diff));

	#define ALLOW_CLOCK_USEC 1000

	timersub(&tm->expires, &tv, &diff.it_value);

	if (diff.it_value.tv_sec <= 0) {
		/* already gone, this is the "good" case */
		if (diff.it_value.tv_sec < 0)
			return 1;
#ifdef __linux__ /* we haven't seen this on other OSes */
		/* still OK if the expiry time is not too much in the future */
		else if (diff.it_value.tv_usec > 0 &&
		            diff.it_value.tv_usec <= ALLOW_CLOCK_USEC) {
			return 2;
		}
#endif /* __linux__ */
		else /* scheduled intentionally in the future? */
			return 0;
	}
	return 0;
}

