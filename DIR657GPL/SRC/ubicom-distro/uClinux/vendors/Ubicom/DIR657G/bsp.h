#ifndef __BSP_H__
#define __BSG_H__

#define HWID                    "MB97-AR9227-RT-101115-00"
#define HWCOUNTRYID             "00"//00:us/na, 01:others country
#define MAC_ID                  0x00, 0x63, 0x07, 0x08, 0x64, 0x09, 0x10, 0x00
#define MAC_ADDR		0xbfc004d0

#define DIAGNOSTIC_LED          1
#define DLINK_ROUTER_LED_DEFINE	1


#ifndef BSP_SETUP

#define USB_LED		135  // no use for 657
#define STATUS_LED	98 // Work
#define POWER_LED	99 //green(blue) led

#define INTERNET_LED_1	134 //amber
#define INTERNET_LED_2	133 //green
//#define WIRELESS_LED	5
#define WPS_LED		99 //blue led
#define WPS_BUTTON	102

#define PUSH_VALUE	0
#define PUSH_BUTTON	103
#define SWITCH_CONTROL	133

#endif//BSP_SETUP

#endif //__BSP_H__


