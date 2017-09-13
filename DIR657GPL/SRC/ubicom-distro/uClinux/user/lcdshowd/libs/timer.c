#include <directfb.h>
#include <lite/window.h>
#include "timer.h"

static int timer_cnt = 0;
static int timer_ary[100];

static int timer_id_search(int id)
{
	int i = 0, ret = -1;

	for (; i < timer_cnt; i++)
		if (timer_ary[i] == id) {
			ret = i;
			break;
		}

	return ret;
}

static int timer_id_insert(int id)
{
	int ret = -1;

	if (timer_id_search(id) != -1)
		goto out;

	timer_ary[timer_cnt++] = id;
	ret = 0;
out:
	return ret;
}

static int timer_id_del(int id)
{
	int i, idx, ret = -1;

	if ((idx = timer_id_search(id)) == -1)
		goto out;

	timer_ary[idx] = 0;
	for (i = idx; i < timer_cnt - 1; i++)
		timer_ary[i] = timer_ary[i + 1];

	timer_ary[i] = 0;
	timer_cnt--;
	ret = 0;
out:
	return ret;
}

DFBResult timer_start(void *data)
{
	timer_option *opt = (timer_option *)data;

	if (opt->func)
		opt->func(opt->data);

	return lite_enqueue_window_timeout(opt->timeout, (LiteTimeoutFunc)timer_start, data, &opt->timeout_id);
}

DFBResult timer_stop(timer_option *opt)
{
	return lite_remove_window_timeout(opt->timeout_id);
}
