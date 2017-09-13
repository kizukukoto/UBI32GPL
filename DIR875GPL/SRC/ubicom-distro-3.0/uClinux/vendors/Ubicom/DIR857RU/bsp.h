#ifndef __BSP_H__
#define __BSG_H__

#define HWID                    "MB97-AR9380-RT-110720-00"
#define HWCOUNTRYID             "00"//00:us/na, 01:others country
#define MAC_ID                  0x00, 0x63, 0x07, 0x08, 0x64, 0x09, 0x10, 0x00
#define MAC_ADDR                0xbfc004d0

#define DIAGNOSTIC_LED          1
#define DLINK_ROUTER_LED_DEFINE 1


#ifndef BSP_SETUP

// GPIO define, look at linux-2.6.x/arch/ubicom32/include/asm/gpio.h
#define USB_LED         135  // no use
#define STATUS_LED      149
#define POWER_LED       148

#define INTERNET_LED_1  152 //amber
#define INTERNET_LED_2  150 //green
//#define WIRELESS_LED    5
#define WPS_LED         148 //blue led
#define WPS_BUTTON      102

#define PUSH_VALUE      0
#define PUSH_BUTTON     157
#define SWITCH_CONTROL  133

#define SD_CARD_DETECT  155

#endif//BSP_SETUP

#endif //__BSP_H__


