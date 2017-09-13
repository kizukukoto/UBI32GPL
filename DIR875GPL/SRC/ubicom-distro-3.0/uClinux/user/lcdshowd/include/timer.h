#ifndef __TIMER_H__
#define __TIMER_H__

typedef void (*TimerFunc)();

typedef struct timer_option {
	TimerFunc func;
	int timeout;
	int timeout_id;
	void *data;
} timer_option;

extern DFBResult timer_start(void *data);
extern DFBResult timer_stop(timer_option *opt);

#endif
