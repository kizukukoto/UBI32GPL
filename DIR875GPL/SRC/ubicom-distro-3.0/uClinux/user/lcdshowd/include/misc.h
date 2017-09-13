#ifndef __MISC_H__
#define __MISC_H__
#include <directfb.h>
#include <lite/lite.h>


#define NVRAM_FILE "/var/etc/nvram.conf"

#ifdef PC
#define BRIGHTNESS_FILE "/tmp/brightness_file"
#else
#define BRIGHTNESS_FILE "/sys/class/backlight/ubicom32bl/brightness"
#endif

#define BRIGHTNESS_MAX 255
#define BRIGHTNESS_MIN 0
#define LUMINANCE_BAR_X 10
#define LUMINANCE_BAR_Y 115
#define LUMINANCE_BAR_WIDTH 300
#define LUMINANCE_BAR_HEIGHT 20
#define LUMINANCE_BAR_DRIFT_X 5 //to enlarge the sensitivity area
#define LUMINANCE_BAR_DRIFT_Y 5 //to enlarge the sensitivity area
#define LCDPOWER_FILE "/sys/class/backlight/ubicom32bl/bl_power"
#define LCDPOWER_ON 0
#define LCDPOWER_OFF 1
#define LCDPOWER_SAVING_TIME 300 // secs
#define LCDPOWER_CHECK_INTERVAL 5 // secs
#define STR_2_NVRAM_WAN_PROTO_DHCPC "dhcpc"
#define STR_2_NVRAM_WAN_PROTO_PPPOE "pppoe"
#define STR_2_GUI_WAN_PROTO_DHCPC "dhcpc"
#define STR_2_GUI_WAN_PROTO_PPPOE "pppoe"


#define LITE_INIT_CHECK(argc, argv)		\
	do {					\
		if (lite_open(&argc, &argv))	\
		return -1;			\
	} while (0)

#define LITE_CLOSE	lite_close
#define MAX_STATIC_ROUTING_NUMBER 20
#define PATH_PROCNET_ROUTE	"/proc/net/route"
#define RTF_UP			0x0001          /* route usable                 */
#define RTF_GATEWAY	0x0002          /* destination is a gateway     */

extern void window_enable(LiteWindow *win, int enabled);
extern void cursor_enable(int enabled);
extern DFBResult screen_init(int *layer_width, int *layer_height);
extern void get_sys_uptime(int *day, int *hour, int *min, int *sec);
extern unsigned long uptime(void);

extern int get_luminance(void);
extern int adjust_luminance(int adjust_value);
extern int get_lcd_power_status(void);
extern int switch_lcd_power(int action);
extern int init_power_save_timer(void);
extern int adjust_power_save_time(int new_value);

extern time_t power_saving_time;
extern time_t last_action_time;
extern struct itimerval power_saving_value ;

#endif
