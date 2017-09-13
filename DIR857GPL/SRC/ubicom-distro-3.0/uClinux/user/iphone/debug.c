

#include <stdint.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <errno.h>

#include "debug.h"

// debug use

static void debug_printf(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	vprintf(fmt, ap);
	va_end(ap);
}


void debug_vprint_level(int level, const char *file, unsigned int line, const char *fmt, ...)
{
	static char *lvl_msg[] = {
		NULL,
		"Error",
		"Warn",
		"Info",
		"Trace",
	};

//	if (level == DEBUG_LEVEL_DISABLED) {
//		return;
//	}
//
//	if (level > DEBUG_LEVEL_TRACE) {
//		debug_assert_and_abort(__this_file, __LINE__, "level: %d is out of range", level);
//	}

//	if (debug_is_start_of_line()) {
//		debug_printf("%s: %s[%d]: ^", lvl_msg[level], file, line);
		debug_printf("%s[%d]: ",  file, line);
//	}

	va_list ap;

	va_start(ap, fmt);
	vprintf(fmt, ap);
	va_end(ap);
}


#define DEBUG_INFO(fmt, ...) debug_vprint_level(3, "", __LINE__, fmt "\n", ##__VA_ARGS__)

typedef unsigned short int bool_t;
typedef signed char s8_t;
typedef unsigned char u8_t;
typedef signed short int s16_t;
typedef unsigned short int u16_t;
typedef signed long int s32_t;
typedef unsigned long int u32_t;
typedef signed long long int s64_t;
typedef unsigned long long int u64_t;
typedef unsigned int fast_u16_t;


/*
 * debug_printf_nibble()
 */
static void debug_printf_nibble(u8_t n)
{
	n += '0';
	if (n > '9') {
		n += ('A' - '0' - 10);
	}
	putchar(n);
}

/*
 * debug_printf_hex_array()
 */
static void debug_printf_hex_array(u8_t *v, fast_u16_t len)
{
	while (len--) {
		debug_printf_nibble(*v >> 4);
		debug_printf_nibble(*v & 0xf);
		v++;
	}
}

#define debug_send_byte putchar

void debug_print_hex_array(const void *buf, u16_t len)
{
	u8_t *p = (u8_t *) buf;
	u16_t i = 0;
	while (len--) {
		debug_printf("%02x ", *p++);
		i++;
		if (!(i % 16)) {
			debug_send_byte('\n');
		}
	}
	debug_send_byte('\n');
}


