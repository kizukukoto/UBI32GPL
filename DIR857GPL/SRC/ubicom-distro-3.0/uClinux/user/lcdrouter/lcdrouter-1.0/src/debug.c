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
#include <debug.h>

#if defined(DEBUG_COLOR)
#define ESC 0x1b
void change_color(debug_levels_t type)
{
	int color;
	putchar(ESC);
	putchar('[');
	putchar('0');//brigthness
	putchar(';');
	/*
	 * Force to a white background
	 */
	putchar('4');
	putchar('7');
	putchar(';');
	putchar('3');

	/*
	 * Valid codes are 30 (black) to 37 (white).
	 */
	switch (type) {
		case ERROR  :color = 1;break; //RED
		case WARN   :color = 3;break; //YELLOW
		case INFO   :color = 4;break; //BLUE
		case TRACE  :color = 2;break; //GREEN
		case ASSERT :color = 1;break; //RED
		default     :color = 0;break; //BLACK
	}
	putchar( color + '0');
	putchar('m');
}
#endif

void debug_msg(debug_levels_t type, const char *file, int line, const char *format, ...)
{
	char buf[512];
	va_list ap;
	va_start(ap, format);
	vsnprintf(buf, sizeof(buf), format, ap);
	va_end(ap);

#if defined(DEBUG_COLOR)
	change_color(type);
#endif

#if defined(DEBUG_TIMESTAMP)
	struct timeval time_val;
	struct timezone time_zone;
	struct tm *tm;

	gettimeofday(&time_val, &time_zone);
	tm = localtime(&time_val.tv_sec);
	printf("[%3ld.%3.0f]",  tm->tm_sec, time_val.tv_usec/1000);
#endif	
	switch (type) {
		case ERROR  : printf("ERROR: %s[%d]:%s \r\n", file, line, buf);break;
		case WARN   : printf("WARN: %s[%d]:%s \r\n", file, line, buf);break;
		case INFO   : printf("INFO: %s[%d]:%s \r\n", file, line, buf);break;
		case TRACE  : printf("TRACE: %s[%d]:%s \r\n", file, line, buf);break;
		case ASSERT : printf("ASSERT: %s[%d]:%s \r\n", file, line, buf);break;
		default:break;
	}
#if defined(DEBUG_COLOR)
	putchar(ESC);
	putchar('[');
	putchar('0');
	putchar('m');
#endif
}

