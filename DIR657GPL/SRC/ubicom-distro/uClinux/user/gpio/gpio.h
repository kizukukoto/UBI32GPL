/* gpio.h */
#ifndef _GPIO_APP_H
#define _GPIO_APP_H

#define GPIO_DRIVER	"/dev/gpio_ioctl"
#define GPIO_SYSTEM	"/var/run/gpio.pid"
#define GPIO_POWER	"/var/run/gpio_power.pid"

#define GPIO_LED_ON          0
#define GPIO_LED_OFF         1
#define GPIO_LED_STATUS      2
#define AR2316_GPIO_OUTPUT_BASE 	0xb1000090
#define AR2316_GPIO_INPUT_BASE 		0xb1000088

#define NO_BIT      0x00000000
#define BIT0        0x00000001
#define BIT1        0x00000002
#define BIT2        0x00000004
#define BIT3        0x00000008
#define BIT4        0x00000010
#define BIT5        0x00000020
#define BIT6        0x00000040
#define BIT7        0x00000080
#define BIT8        0x00000100
#define BIT9        0x00000200
#define BIT10       0x00000400
#define BIT11       0x00000800
#define BIT12       0x00001000
#define BIT13       0x00002000
#define BIT14       0x00004000
#define BIT15       0x00008000
#define BIT16       0x00010000
#define BIT17       0x00020000
#define BIT18       0x00040000
#define BIT19       0x00080000
#define BIT20       0x00100000
#define BIT21       0x00200000
#define BIT22       0x00400000
#define BIT23       0x00800000
#define BIT24       0x01000000
#define BIT25       0x02000000
#define BIT26       0x04000000
#define BIT27       0x08000000
#define BIT28       0x10000000
#define BIT29       0x20000000
#define BIT30       0x40000000
#define BIT31       0x80000000

#define    MV_GPP0  BIT0
#define    MV_GPP1  BIT1
#define    MV_GPP2  BIT2
#define    MV_GPP3  BIT3
#define    MV_GPP4  BIT4
#define    MV_GPP5  BIT5
#define    MV_GPP6  BIT6
#define    MV_GPP7  BIT7
#define    MV_GPP8  BIT8
#define    MV_GPP9  BIT9
#define    MV_GPP10 BIT10
#define    MV_GPP11 BIT11
#define    MV_GPP12 BIT12
#define    MV_GPP13 BIT13
#define    MV_GPP14 BIT14
#define    MV_GPP15 BIT15
#define    MV_GPP16 BIT16
#define    MV_GPP17 BIT17
#define    MV_GPP18 BIT18
#define    MV_GPP19 BIT19
#define    MV_GPP20 BIT20
#define    MV_GPP21 BIT21
#define    MV_GPP22 BIT22
#define    MV_GPP23 BIT23
#define    MV_GPP24 BIT24
#define    MV_GPP25 BIT25
#define    MV_GPP26 BIT26
#define    MV_GPP27 BIT27
#define    MV_GPP28 BIT28
#define    MV_GPP29 BIT29
#define    MV_GPP30 BIT30
#define    MV_GPP31 BIT31

/*	Date:	2009-03-31
*	Name:	jimmy huang
*	Reason:	Reduce the frquency we use strcmp()
*			Parse only once, then convert to int for later used
*	Note:	Add the function below
*			1.	parameters are the transfer from main()'s argv[],
*				argv[] has been left shift 1, which means
*				parameters[0] here are argv[1] in main()
*			2.	other parameters please refer to gpio.h
*/
#define GPIO_ERROR		0
#define GPIO_SUCCESS	1

typedef enum pin_gpio_name{
    GPIONAME_INTERNET_LED = 0,
	GPIONAME_POWER_LED,
	GPIONAME_STATUS_LED,
	GPIONAME_USB_LED,
	GPIONAME_WLAN_LED,
	GPIONAME_SYSTEM,
	GPIONAME_SWITCH_CONTROL,
	GPIONAME_TEST,
	GPIONAME_WPS,
	GPIONAME_WPS_BUTTON,
	GPIONAME_ERROR_TYPE,
#ifdef USE_SILEX
	GPIONAME_SD_CHECK,
#endif
}pin_gpio_name_s;

typedef enum pin_gpio_action{
    GPIOSTATE_PIN_LED_ON = 0,
    GPIOSTATE_PIN_LED_OFF,
    GPIOSTATE_PIN_LED_BLINK,
	GPIOSTATE_PIN_LED_SBLINK,
	GPIOSTATE_PIN_STATUS,
    GPIOSTATE_PIN_CHECK,
    GPIOSTATE_ERROR_GPIO_ACTION_TYPE,
}pin_gpio_action_s;

typedef enum pin_gpio_parameter{
    GPIOPARAMETER_AMBER = 0,
	GPIOPARAMETER_GREEN,
	GPIOPARAMETER_ERROR_PARAMETER,
}pin_gpio_parameter;

#endif
