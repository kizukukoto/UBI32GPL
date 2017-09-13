
#include <nvram.h>
#include <sutil.h>
#include <shvar.h>
#include "project.h"

/*
 * NVRAM Debug Messgae Define
 */
 
#define USB_MOUNT_DIR 	"/var/usb"

/*	Date:	2009-03-05
*	Name:	jimmy huang
*	Reason: DIR-825 now attached to sda1 so we don't
*			use sdb1 anymore
*	Note:	modified the codes below
*/
/*
#ifdef AR7161
#define USB_DEVICE_ID 	"/dev/sdb1"
#else
#define USB_DEVICE_ID 	"/dev/sda1"
#endif
*/
#define USB_DEVICE_ID 	"/dev/sda1"

#define XML_LENGTH 1024
#if 0
#define WFC_SETTING_FILE "/var/usb/smrtntky/wsetting.wfc"
#define WFC_FILE_PATH "/var/usb/smrtntky/device/"
#else
#define STORAGE_DEFAULT_FOLDER USBMOUNT_FOLDER
#endif
#define MYWFC_TEMP_FILE "/tmp/DevConfigWCN_FileName.txt"
#if 1//configId equal do wcn
#define WCN_SET_FILE "/tmp/wcnset"
#endif

extern const char VER_FIRMWARE[];
//main
void get_configId(char *);
void get_filename(char *);
void get_mywcn_filename(char*);
void mount_usb_path(void);
void unmount_usb_path(void);
int check_mywcn_file_exist(void);
int check_set_wcn(void);
void write_pid_to_var(void);
//wcn
void do_wcn(int, char *);
void wcn_led_succesfully_show(void);
void wcn_led_unsuccesfully_show(void);

struct Wsetting_elements {
	char configId[40];
	char configAuthorId[40];
	char configAuthor[40];
	char ssid[32];
	char connectionType[8];
	char authenticateType[8];
	char encryption[8];
	char networkKey[64];
	char keyProvidedAutomatically[8];
	char ieee802Dot1xEnable[8];
	char eapMethod[24];
	char channel2Dot4[8];
	char channel5Dot0[8];
	char deviceMode[16];
	unsigned int timeToLive;
	unsigned int configHash;
	char keyIndex[8];
};

struct wcn_to_nvram {
	char *wlan0_ssid;
	char *wlan0_channel;
	char *wlan0_auto_channel_enable;
	char *wlan0_mode;
	char *wlan0_security;
	char *wlan0_wep_display;
	char *wlan0_wep_default_key;
	char *wlan0_psk_cipher_type;
	char *wlan0_psk_pass_phrase;
	char *wlan0_eap_proto;
	char *wlan0_peap_inner_proto;
};
