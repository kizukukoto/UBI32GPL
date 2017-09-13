/*
 * Ubicom Network Player
 *
 * Copyright © 2009 Ubicom Inc. <www.ubicom.com>.  All rights reserved.
 *
 * This file contains confidential information of Ubicom, Inc. and your use of
 * this file is subject to the Ubicom Software License Agreement distributed with
 * this file. If you are uncertain whether you are an authorized user or to report
 * any unauthorized use, please contact Ubicom, Inc. at +1-408-789-2200.
 * Unauthorized reproduction or distribution of this file is subject to civil and
 * criminal penalties.
 *
 */
#ifndef DEBUG_H
#define DEBUG_H

#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

enum debug_levels {
	NONE = 0,
	ERROR ,
	WARN,
	INFO,
	TRACE,
	ASSERT,
};

typedef enum debug_levels debug_levels_t;

extern debug_levels_t global_debug_level;

void debug_msg( debug_levels_t type, const char *file, int line, const char *format, ... );

#define DEBUG_ASSERT(case, x...)    if (!(case)) debug_msg(ASSERT, __FILE__, __LINE__, x );assert(case)

#define DEBUG_ERROR(x...)     if (global_debug_level > NONE) debug_msg(ERROR, __FILE__, __LINE__, x )
#define DEBUG_WARN(x...)      if (global_debug_level > ERROR) debug_msg(WARN, __FILE__, __LINE__, x )
#define DEBUG_INFO(x...)      if (global_debug_level > WARN) debug_msg(INFO, __FILE__, __LINE__, x )
#define DEBUG_TRACE(x...)     if (global_debug_level > INFO) debug_msg(TRACE, __FILE__, __LINE__, x )
#define DEBUG_TEST(x...)     if (M_(debug_level) > INFO) debug_msg(TRACE, __FILE__, __LINE__, x )

#endif

