#ifndef __WIZARD_H__
#define __WIZARD_H__
#include <leck/textline.h>

#define LOAD_INTO_DDR

#define PAGE_UNDEFINE		0x0000
#define PAGE_CANCLE		0x0001
#define	PAGE_NEXT		0x0002
#define PAGE_PREV		0x0004
#define PAGE_BACK		0x0008
#define PAGE_BEGIN		0x0010
#define PAGE_NOTHANKS		0x0020
#define PAGE_YES		0x0040
#define PAGE_NO			0x0080
#define PAGE_SAVESETTING	0x0100
#define PAGE_STARTOVER		0x0200

#define OPT_UNDEFINE	0x00
#define OPT_CANCLE	0x01
#define OPT_NEXT	0x02
#define OPT_PREV	0x04
#define	OPT_BACK	OPT_PREV
#define OPT_BEGIN	OPT_NEXT
#define OPT_NOTHANKS	OPT_PREV

#define TRUE	1
#define FALSE	0

typedef struct __wizard_page {
#ifdef LOAD_INTO_DDR
	LiteWindow *win;
#endif
	int (*ui_func)(LiteWindow *, struct __wizard_page **);
	int (*evt_func)(LiteWindow *, struct __wizard_page **);
	void *param;
#ifdef LOAD_INTO_DDR
	int offset;
#endif
} wizard_page;

typedef struct __pppoe_account {
	LiteTextLine *text_username;
	LiteTextLine *text_password;
} pppoe_account;

typedef struct __tz_option {
	LiteTextLine *text_tz;
	int opt;
} tz_option;

/* button_option.c */
extern int option_return_click(LiteWindow *, int);

/* welcome.c */
extern int wz_welcome_ui_1(LiteWindow *, wizard_page **);
extern int wz_welcome_ui_2(LiteWindow *, wizard_page **);
extern int wz_welcome_evt_1(LiteWindow *, wizard_page **);
extern int wz_welcome_evt_2(LiteWindow *, wizard_page **);
/* install.c */
extern int wz_install_ui_1(LiteWindow *, wizard_page **);
extern int wz_install_ui_2(LiteWindow *, wizard_page **);
extern int wz_install_ui_3(LiteWindow *, wizard_page **);
extern int wz_install_ui_4(LiteWindow *, wizard_page **);
extern int wz_install_ui_5(LiteWindow *, wizard_page **);
extern int wz_install_evt_1(LiteWindow *, wizard_page **);
extern int wz_install_evt_2(LiteWindow *, wizard_page **);
extern int wz_install_evt_3(LiteWindow *, wizard_page **);
extern int wz_install_evt_4(LiteWindow *, wizard_page **);
extern int wz_install_evt_5(LiteWindow *, wizard_page **);

/* network.c */
extern int wz_network_ui_1(LiteWindow *, wizard_page **);
extern int wz_network_ui_2(LiteWindow *, wizard_page **);
extern int wz_network_ui_3(LiteWindow *, wizard_page **);
extern int wz_network_ui_4(LiteWindow *, wizard_page **);
extern int wz_network_evt_1(LiteWindow *, wizard_page **);
extern int wz_network_evt_2(LiteWindow *, wizard_page **);
extern int wz_network_evt_3(LiteWindow *, wizard_page **);
extern int wz_network_evt_4(LiteWindow *, wizard_page **);

/* wireless.c */
extern int wz_wireless_ui_1(LiteWindow *, wizard_page **);
extern int wz_wireless_ui_2(LiteWindow *, wizard_page **);
extern int wz_wireless_ui_3(LiteWindow *, wizard_page **);
extern int wz_wireless_ui_4(LiteWindow *, wizard_page **);
extern int wz_wireless_ui_5(LiteWindow *, wizard_page **);
extern int wz_wireless_evt_1(LiteWindow *, wizard_page **);
extern int wz_wireless_evt_2(LiteWindow *, wizard_page **);
extern int wz_wireless_evt_3(LiteWindow *, wizard_page **);
extern int wz_wireless_evt_4(LiteWindow *, wizard_page **);
extern int wz_wireless_evt_5(LiteWindow *, wizard_page **);

/* timezone.c */
extern int wz_timezone_ui(LiteWindow *, wizard_page **);
extern int wz_timezone_area_ui(LiteWindow *, wizard_page **);
extern int wz_timezone_evt(LiteWindow *, wizard_page **);
extern int wz_timezone_area_evt(LiteWindow *, wizard_page **);

/* final.c */
extern int wz_summary_ui(LiteWindow *, wizard_page **);
extern int wz_final_ui(LiteWindow *, wizard_page **);
extern int wz_summary_evt(LiteWindow *, wizard_page **);
extern int wz_final_evt(LiteWindow *, wizard_page **);

/* dhcpc_list.c, copy from wizard/ */
extern char *get_local_lan(char *, int *);

/* nvram.c */
extern int wz_nvram_set(const char *, char *);
extern int wz_nvram_reset();
extern int wz_nvram_commit();
extern char *wz_nvram_get(const char *);
extern char *wz_nvram_safe_get(const char *);
extern char *wz_nvram_get_local(const char *);

/* page.c */
extern int do_page_visible(LiteWindow *, int);
extern int do_page_next(wizard_page **);
extern int do_page_back(wizard_page **);
extern int do_page_distroy_all(wizard_page *);
extern int do_page_preload_all(wizard_page *);

/* info.c */
extern int info_save(const char *, const char *);
extern const char *info_load(const char *);

#endif
