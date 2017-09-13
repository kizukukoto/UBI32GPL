
#ifndef __DEBUG__H__
#define __DEBUG__H__


static void debug_printf(const char *fmt, ...);
void debug_vprint_level(int level, const char *file, unsigned int line, const char *fmt, ...);

#define DEBUG_INFO(fmt, ...) debug_vprint_level(3, THIS_FILE, __LINE__, fmt "\n", ##__VA_ARGS__)

#define DEBUG_PRINT_HEX_ARRAY(buf, len) debug_print_hex_array(buf, len)


#endif
