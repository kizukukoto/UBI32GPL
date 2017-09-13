/*
 *  tslib/src/ts_reset.c
 *
 *  Copyright (C) 2009 Ubicom, Inc.
 *
 * This file is placed under the LGPL.  Please see the file
 * COPYING for more details.
 *
 * Reset subsystems
 */
#include "config.h"

#include "tslib-private.h"

#ifdef DEBUG
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#endif

int ts_reset(struct tsdev *ts, int param)
{
	int result = 0;
	struct tslib_module_info *inf = ts->list;
	while (inf) {
		if (inf->ops->reset) {
			result |= inf->ops->reset(ts->list, param);
		}
		inf = inf->next;
	}
	return result;
}
