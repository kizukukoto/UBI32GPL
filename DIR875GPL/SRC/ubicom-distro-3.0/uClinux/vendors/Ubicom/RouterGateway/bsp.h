#ifndef __BSP_H__
#define __BSG_H__

#define HWID                    "AR94-AR9223-RT-090811-00"
#define HWCOUNTRYID             "00"//00:us/na, 01:others country
#define MAC_ID                  0x00, 0x63, 0x07, 0x08, 0x64, 0x09, 0x10, 0x00
#define MAC_ADDR		0xbfc004d0

#define DIAGNOSTIC_LED          1
#define DLINK_ROUTER_LED_DEFINE	1

#ifndef BSP_SETUP

#ifdef ATHEROS_11N_DRIVER

#define USB_LED		132
#define STATUS_LED	98 // Work
#define POWER_LED	99 //green(blue) led

#define INTERNET_LED_1	134 //amber
#define INTERNET_LED_2	133 //green
#define WIRELESS_LED	5
#define WPS_LED		100 //blue led
#define WPS_BUTTON	102

#define PUSH_VALUE	0
#define PUSH_BUTTON	103
#define SWITCH_CONTROL	133

#else //ATHEROS_11N_DRIVER
#define USB_LED			BIT16
#define STATUS_LED		BIT7
#define WIRELESS_LED	BIT17
#define PUSH_BUTTON		BIT6
#define PUSH_VALUE		64

#endif //ATHEROS_11N_DRIVER

#endif//BSP_SETUP

#endif //__BSP_H__
