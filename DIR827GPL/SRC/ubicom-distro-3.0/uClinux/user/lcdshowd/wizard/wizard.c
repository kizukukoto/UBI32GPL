#include <lite/window.h>
#include <leck/image.h>
#include <leck/progressbar.h>
#include <leck/label.h>

#include "wizard.h"

#include "misc.h"
#include "select.h"
#include "image.h"
#include "input.h"
#include "timer.h"

#define WIZARD_CANCEL -2
#define WIZARD_PRE_STEP -1
#define WIZARD_NEXT_STEP 1
#define WIZARD_SAVE_SETTING_AND_REBOOT 4
#define WIZARD_START_WIZARD_OVER 5

struct _record record;
static int current;

extern int layer_width, layer_height;
extern void wizard_step1();
extern void wizard_step2();
extern void wizard_install_router1();
extern void wizard_install_router2();
extern void wizard_install_router3();
extern void wizard_install_router4();
extern void wizard_install_router5();
extern void wizard_auto_detect();
extern void auto_detect_processbar();
extern void wizard_dhcp_check_cable();
extern void wizard_DHCP_Settings();
extern void wizard_PPPoE_Settings();
extern void wizard_24G_Set_SSID();
extern void wizard_24G_Set_PWD();
extern void wizard_5G_Set_SSID();
extern void wizard_5G_Set_PWD();
extern void wizard_optimize_channel_band();
extern void wizard_Time_Zone();
extern void wizard_Summary_internet();
extern void wizard_Set_and_Conn();
extern void wizard_save();
extern void wizard_clear();
extern void wizard_init();

static void btn_nothanks_begin(int x, int y, int *ret, int *flag)
{
	static DFBRegion btr[] = {
		{ 216, 190, 292, 218 }, //0: Begin
		{  36, 190, 112, 218 }, //1: No Thanks
	};

	printf("%s(%d) :: (%d, %d) \n", __func__, __LINE__, *ret, *flag);
	switch (bt_ripple_handler(x, y, btr, 2)) {
		case 0: *ret =  1; *flag = 1; break;
		case 1: *ret = -2; *flag = 1; break;
		default: return;
	}
	printf("%s(%d) :: (%d, %d) \n", __func__, __LINE__, *ret, *flag);
}

static void btn_pre_cancel(int x, int y, int *ret, int *flag)
{
	static DFBRegion btr[] = {
		{  36, 190, 112, 218 }, //0: Prev
		{ 122, 190, 206, 218 }, //1: Cancel
	};
	switch (bt_ripple_handler(x, y, btr, 2)) {
		case 0: *ret = -1; break;
		case 1: *ret = -2; break;
		default: return;
	}
	*flag = 1;
}

static void btn_next_cancel(int x, int y, int *ret, int *flag)
{
	static DFBRegion btr[] = {
		{ 216, 190, 292, 218 }, //0: Next
		{ 122, 190, 206, 218 }, //1: Cancel
        };
	switch (bt_ripple_handler(x, y, btr, 2)) {
		case 0: *ret =  1; break;
		case 1: *ret = -2; break;
		default: return;
	}
	*flag = 1;
}

static void btn_pre_next_cancel(int x, int y, int *ret, int *flag)
{
	static DFBRegion btr[] = {
		{ 216, 190, 292, 218 }, //0: Next
		{  36, 190, 112, 218 }, //1: Prev
		{ 122, 190, 206, 218 }, //2: Cancel
	};
	switch (bt_ripple_handler(x, y, btr, 3)) {
		case 0: *ret =  1; break;
		case 1: *ret = -1; break;
		case 2: *ret = -2; break;
		default: return;
	}
	*flag = 1;
}

static void proc_kboard_input(int x, int y, int *ret, int *flag,const char *fn, void *data)
{
	char *str = NULL;

	if (strcmp(fn, "D3") == 0) {

		struct dhcp_struct *dhcp = (struct dhcp_struct *)data;
		static DFBRegion btr = { 140, 95, 265, 115 };

		if (bt_ripple_handler(x, y, &btr, 1) == 0) {
		        *ret = 0; *flag = 1;
			keyboard_input(dhcp->text1, dhcp->txtline);
                        lite_get_textline_text(dhcp->txtline, &str);
			if (str && strlen(str) != 0) {
				memset(record.pc, '\0', sizeof(record.pc));
				strcpy(record.pc, str);
			}
			lite_set_textline_text(dhcp->txtline, str);
                } 
	}

	if (strcmp(fn, "D4") == 0) {

		struct pppoe_struct *pppoe = (struct pppoe_struct *)data;
		static DFBRegion btr[] = {
			{ 100, 140, 250, 160 }, //0: Username
			{ 100, 160, 250, 180 }, //1: Password
		};

		switch (bt_ripple_handler(x, y, btr, 2)) {
			case 0:
				*ret = 0; *flag = 1;
				//lite_set_textline_text(pppoe->txtline[0], record.username);
				keyboard_input(pppoe->text1, pppoe->txtline[0]);
				lite_get_textline_text(pppoe->txtline[0], &pppoe->str);
				memset(record.username, '\0', sizeof(record.username));
				strcpy(record.username, pppoe->str);
				break;
			case 1:
				*ret = 0; *flag = 1;
				//lite_set_textline_text(pppoe->txtline[1], record.password);
				keyboard_input(pppoe->text2, pppoe->txtline[1]);
				lite_get_textline_text(pppoe->txtline[1], &pppoe->str);
				memset(record.password, '\0', sizeof(record.password));
				strcpy(record.password, pppoe->str);
				break;
		}
	}

	if (strcmp(fn, "E1") == 0) {
		
		struct wizard_wlan_struct *wlan = (struct wizard_wlan_struct *)data;
		static DFBRegion btr = { 100, 100, 250, 120 };

		if (bt_ripple_handler(x, y, &btr, 1) == 0) {
			*ret = 0; *flag = 1;
			//lite_set_textline_text(wlan->txtline, record.wlan0_ssid);
			keyboard_input(wlan->text1, wlan->txtline);
			lite_get_textline_text(wlan->txtline, &wlan->str);
			memset(record.wlan0_ssid, '\0', sizeof(record.wlan0_ssid));
			strcpy(record.wlan0_ssid, wlan->str);
		}
	}

	if (strcmp(fn, "E2") == 0) {

		struct wizard_wlan_struct *wlan = (struct wizard_wlan_struct *)data;
		static DFBRegion btr = { 150, 100, 250, 120 };

		if (bt_ripple_handler(x, y, &btr, 1) == 0) {
			*ret = 0; *flag = 1;
			//lite_set_textline_text(wlan->txtline, record.wlan0_pwd);
			keyboard_input(wlan->text1, wlan->txtline);
			lite_get_textline_text(wlan->txtline, &wlan->str);
			memset(record.wlan0_pwd, '\0', sizeof(record.wlan0_pwd));
			strcpy(record.wlan0_pwd, wlan->str);
		}
	}

	if (strcmp(fn, "E3") == 0) {

		struct wizard_wlan_struct *wlan = (struct wizard_wlan_struct *)data;
		static DFBRegion btr = { 100, 100, 250, 120 };

		if (bt_ripple_handler(x, y, &btr, 1) == 0) { 
			*ret = 0; *flag = 1;
			//lite_set_textline_text(wlan->txtline, record.wlan1_ssid);
			keyboard_input(wlan->text1, wlan->txtline);
			lite_get_textline_text(wlan->txtline, &wlan->str);
			memset(record.wlan1_ssid, '\0', sizeof(record.wlan1_ssid));
			strcpy(record.wlan1_ssid, wlan->str);
		}
	}

	if (strcmp(fn, "E4") == 0) {

		struct wizard_wlan_struct *wlan = (struct wizard_wlan_struct *)data;
		static DFBRegion btr = { 100, 100, 250, 120 };

		if (bt_ripple_handler(x, y, &btr, 1) == 0) { 
			*ret = 0; *flag = 1;
			//lite_set_textline_text(wlan->txtline, record.wlan1_pwd);
			keyboard_input(wlan->text1, wlan->txtline);
			lite_get_textline_text(wlan->txtline, &wlan->str);
			memset(record.wlan1_pwd, '\0', sizeof(record.wlan1_pwd));
			strcpy(record.wlan1_pwd, wlan->str);
		}
	}
}

static int error_detect(const char *fn)
{
        if ( strcmp(fn, "D3") == 0 ) {
                if ( strlen(record.pc) < 15) {
                        return -1;
                }
        } else if ( strcmp(fn, "D4") == 0 ) {
                if ( strlen(record.username) < 1 ||
                     strlen(record.password) < 1) {
                        return -1;
                }
        } else if ( strcmp(fn, "E1") == 0 ) {
                if ( strlen(record.wlan0_ssid) < 0) {
                        return -1;
                }
        } else if ( strcmp(fn, "E2") == 0 ) {
                if ( strlen(record.wlan0_pwd) < 0) {
                        return -1;
                }

        } else if ( strcmp(fn, "E3") == 0 ) {
                if ( strlen(record.wlan1_ssid) < 0) {
                        return -1;
                }
        } else if ( strcmp(fn, "E4") == 0 ){
                if ( strlen(record.wlan1_pwd) < 0) {
                        return -1;
		}
        }
        return 0;
}

static int wizard_page_loop(LiteWindow *win, int step, void *list, const char *fn, void *data)
{
	int ret = -2;
	lite_set_window_opacity(win, 0xFF);
	while (true) {
		int x, y;
		int flag = -1;

		if (strcmp(fn , "D1") == 0) {
			struct detect_struct *detect = (struct detect_struct *)data;
			if(detect->degree >= 1.0) {
				ret = 1; flag = 1;
				break;
			}else {
				DFBWindowEventType type = mouse_handler(win, &x, &y, 1);
				if (type == DWET_BUTTONDOWN) {
					static DFBRegion btr = { 122, 190, 206, 245 }; //Cancel
					if (bt_ripple_handler(x, y, &btr, 1) == 0) {
						ret = -2; flag = 1;
						break;
					}
				}
			}
			//lite_window_event_loop(win, 100);
			continue;
		} else {
			ret = -2; flag = -1;

			DFBWindowEventType type = mouse_handler(win, &x, &y, 0);
			switch(step) {
				case -1:
					if (type = DWET_BUTTONUP) {
						btn_nothanks_begin(x, y, &ret, &flag);
					}
					break;
				case 0:
					if (type = DWET_BUTTONUP)
						btn_next_cancel(x, y, &ret, &flag);
					break;
				case 1:
					if (type = DWET_BUTTONUP)
						btn_pre_next_cancel(x, y, &ret, &flag);
					break;
				case 2:
					if (type == DWET_BUTTONUP) {
						btn_pre_cancel(x, y, &ret, &flag);
						static DFBRegion btr[] = {
							{  55, 107, 135, 130 }, //Cable User
							{ 195, 107, 275, 130 }, //No Cable User
						};
						switch (bt_ripple_handler(x, y, btr, 2)) {
							case 0: ret = 2; flag = 1; break;
							case 1: ret = 3; flag = 1; break;
						}
					}
					break;
				case 3:
					fflush(stdout);
					if (type == DWET_BUTTONUP) {
						if (strcmp(fn, "D3") == 0) {
							if (x >= 140 && y >= 95 && x <= 320 && y <= 115) {
								select_input(data, "Select Your PC:", (LiteTextLine *)list, EDIT_ENABLE);
								continue;
							}
						} else {
							proc_kboard_input(x, y, &ret, &flag, fn, data);
							if (ret == 0 && flag == 1) {
								ret = -2;
								continue;
							}
						}
						btn_pre_next_cancel(x, y, &ret, &flag);
					}
					break;
				case 4:
					if (type == DWET_BUTTONUP) {
						static DFBRegion btr[] = {
							{ 122, 190, 206, 245 }, //Cancel
							{  50,  77, 275, 102 },
							{  50, 128, 275, 150 },
						};
						switch (bt_ripple_handler(x, y, btr, 3)) {
							case 0: ret = -2; flag = 1; break;
							case 1: ret =  4; flag = 1; break;
							case 2: ret =  5; flag = 1; break;
						}
					}
					break;
				case 5:	{
					/*
					struct detect_struct *detect = (struct detect_struct *)data;
					if(detect->degree >= 1.0) {
						ret = 1; flag = 1;
						if (type == DWET_BUTTONDOWN) {
							if (y > 190 && y < 245) {
								if (x > 122 && x < 206 ) { //cancel
									ret = -2; flag = 1;
								}
							}
						}
					}
					*/
					break;
				}
				case 6:
					if (type == DWET_BUTTONUP) {
						if (strcmp(fn, "F") == 0) {
							if (x >= 140 && y >= 95 && x <= 320 && y <= 115) {
								select_input(data, "Time Zone:", (LiteTextLine *)list, EDIT_DISABLE);
								continue;
							} else {
								btn_next_cancel(x, y, &ret, &flag);
								static DFBRegion btr = { 300, 100, 320, 120 };

								//if (bt_ripple_handler(x, y, &btr, 1) == 0)
								//	ret = -3; flag = -1;
							}
						} else {
							btn_next_cancel(x, y, &ret, &flag);
							static DFBRegion btr = { 300, 100, 320, 120 };

							if (bt_ripple_handler(x, y, &btr, 1) == 0) { 
								ret = -3; flag = -1;
							}
						}
					}
					break;
				default :
					ret = 0; flag = -1;
					break;
			}
		}

		if ((ret == -1 || ret == -2) && flag == 1)
			break;
		if (error_detect(fn) == -1)
			flag = -1;
		if (flag != -1) break;
	}
	lite_set_window_opacity(win, 0x00);
	return ret;
}

static int wizard_control_step(int ret, char *fn, void *data)
{
        if(ret == 0) return 0 ;
        if(ret == -2) return WIZARD_CANCEL;
        if(strcmp(fn, "D1") == 0) {
		struct detect_struct *detect = (struct detect_struct *)data;
                if (strcmp(detect->text1, STR_2_GUI_WAN_PROTO_DHCPC) == 0) {
                        strcpy(record.wan_type, STR_2_GUI_WAN_PROTO_DHCPC);
                        current++;
                        return 0;
                } else if(strcmp(detect->text1, STR_2_GUI_WAN_PROTO_PPPOE) == 0) {
                        strcpy(record.wan_type, STR_2_GUI_WAN_PROTO_PPPOE);
                        current+=3;
                        return 0;
                }
        }

        if (strcmp(fn, "D2") == 0) {
                if (ret == 2) {//cable user
                        current++;
                        return 0;
                } else if (ret == 3) {// not cable user
                        current+=3;
                        return 0;
                }
        }
        if (strcmp(fn, "D3") == 0 && ret == 1){
                current+=2;
                return 0;
        }

        if (strcmp(fn, "D4") == 0 && ret == -1) {
                current-=3;
                return 0;
        }

        if (strcmp(fn, "E1") == 0 && ret == -1) {
                current-=4;
                return 0;
        }

        if (strcmp(fn, "E5") == 0) {
                if (ret == 2) {
                        record.optimize = 1; current++;
                        return 0;
                } else if (ret == 3) {
                        record.optimize = 0; current++;
                        return 0;
                }
        }

        if (strcmp(fn, "G2") == 0) {
                if (ret == 4) {
                        record.save_or_start = 0; // save settings and reboot
                        wizard_save(&record);
                        return WIZARD_SAVE_SETTING_AND_REBOOT;
                } else if (ret == 5) {
                        record.save_or_start = 1; // start the wizard OVer
                        wizard_clear(&record);
			current = 0;
                        return WIZARD_START_WIZARD_OVER;
                }
        }

        if (ret == 1) current++; // next
        if (ret == -1) current--; // prev
        return 0;
}

static int draw_window(LiteWindow *win, int *ret)
{
	switch (current) {
		case 0 :
			wizard_init(&record);
			wizard_step1(win);
			*ret = wizard_page_loop(win, -1, NULL, "A", NULL);
			*ret = wizard_control_step(*ret, "A", NULL);
			break;
		case 1 :
			wizard_step2(win);
			*ret = wizard_page_loop(win, 0, NULL, "B", NULL);
			*ret = wizard_control_step(*ret, "B", NULL);
			break;
		case 2 :
			wizard_install_router1(win);
			*ret = wizard_page_loop(win, 0, NULL, "C1", NULL);
			*ret = wizard_control_step(*ret, "C1", NULL);
			break;
		case 3 :
			wizard_install_router2(win);
			*ret = wizard_page_loop(win, 1, NULL, "C2", NULL);
			*ret = wizard_control_step(*ret, "C2", NULL);
			break;
		case 4 :
			wizard_install_router3(win);
			*ret = wizard_page_loop(win, 1, NULL, "C3", NULL);
			*ret = wizard_control_step(*ret, "C3", NULL);
			break;
		case 5 :
			wizard_install_router4(win);
			*ret = wizard_page_loop(win, 1, NULL, "C4", NULL);
			*ret = wizard_control_step(*ret, "C4", NULL);
			break;
		case 6 :
			wizard_install_router5(win);
			*ret = wizard_page_loop(win, 1, NULL, "C5", NULL);
			*ret = wizard_control_step(*ret, "C5", NULL);
			break;
		case 7 :
			{
				LiteProgressBar *progress;
				struct detect_struct my_detect;
				memset(my_detect.text1, '\0', sizeof(my_detect.text1));
				my_detect.degree = 0.0;
				timer_option opt_proc = { .func = auto_detect_processbar,
                                			.timeout = 1000,
			                                .timeout_id = 10006, .data = &my_detect};
				wizard_auto_detect(win, &my_detect);
				timer_start(&opt_proc);
				*ret = wizard_page_loop(win, 5, NULL, "D1", &my_detect);
				timer_stop(&opt_proc);
				*ret = wizard_control_step(*ret, "D1", &my_detect);
			}
			break;
		case 8 :
			
			wizard_dhcp_check_cable(win);
			*ret = wizard_page_loop(win, 2, NULL, "D2", NULL);
			*ret = wizard_control_step(*ret, "D2", NULL);
			break;
		case 9 :
			{
				char *tmp;
				struct dhcp_struct my_dhcp;
				my_dhcp.str = NULL;
				memset(my_dhcp.client, '\0', sizeof(my_dhcp.client));
				memset(my_dhcp.text1, '\0', sizeof(my_dhcp.text1));
#ifdef SELECT_UI
				int opt_i = 0;
				char *opt[256] = { NULL };
				LiteTextLine *text;
#else
				ComponentList *list;
#endif

				if (strlen(record.pc) != 0) {
		                        my_dhcp.str = record.pc;
                		}
#ifdef SELECT_UI
				strcpy(record.pc, nvram_safe_get("wan_mac"));
				wizard_DHCP_Settings(win, &my_dhcp, &text, opt);
				*ret = wizard_page_loop(win, 3, text, "D3", opt);
				lite_get_textline_text(text, &tmp);
				bzero(record.pc, sizeof(record.pc));
				strcpy(record.pc, tmp);

				while (opt[opt_i])
					free(opt[opt_i++]);
#else
				wizard_DHCP_Settings(win, &my_dhcp, &list);
				select_set_default_item(list, 0);
				*ret = wizard_page_loop(win, 3, list, "D3", &my_dhcp);

				select_get_selected_item(list, &tmp);
				if (strlen(tmp) != 0) {
					memset(record.pc, '\0', sizeof(record.pc));
					strcpy(record.pc, tmp);
				}
#endif
				*ret = wizard_control_step(*ret, "D3", NULL);
			}
			break;
		case 10 :
			{
				struct pppoe_struct my_pppoe;
				char *ptr, pwd[128];
				int i = 0 ;
				my_pppoe.str = NULL;
				memset(my_pppoe.text1, '\0', sizeof(my_pppoe.text1));
				memset(my_pppoe.text2, '\0', sizeof(my_pppoe.text2));
				wizard_PPPoE_Settings(win, &my_pppoe);
				if (strlen(record.username) != 0) {
 		                       lite_set_textline_text(my_pppoe.txtline[0], record.username);
                		}
		                if (strlen(record.password) != 0) {
                		        ptr = pwd;
		                        for ( i = 0; i < strlen(record.password) ; i++)
                		                strcat(pwd, "*");
		                        lite_set_textline_text(my_pppoe.txtline[1], pwd);
                		}
				*ret = wizard_page_loop(win, 3, NULL, "D4", &my_pppoe);
				*ret = wizard_control_step(*ret, "D4", NULL);
			}
			break;
		case 11 :
			{
				struct wizard_wlan_struct my_wlan;
				my_wlan.str = NULL;
				memset(my_wlan.text1, '\0', sizeof(my_wlan.text1));
				wizard_24G_Set_SSID(win, &my_wlan);
			        if (strlen(record.wlan0_ssid) != 0) {
                        		lite_set_textline_text(my_wlan.txtline, record.wlan0_ssid);
                		}
				*ret = wizard_page_loop(win, 3, NULL, "E1", &my_wlan);
				*ret = wizard_control_step(*ret, "E1", NULL);
			}
			break;
		case 12 :
			{
				struct wizard_wlan_struct my_wlan;
				my_wlan.str = NULL;
				memset(my_wlan.text1, '\0', sizeof(my_wlan.text1));
				wizard_24G_Set_PWD(win, &my_wlan);
				if (strlen(record.wlan0_pwd) != 0) {
 		                       lite_set_textline_text(my_wlan.txtline, record.wlan0_pwd);
                		}
				*ret = wizard_page_loop(win, 3, NULL, "E2", &my_wlan);
				*ret = wizard_control_step(*ret, "E2", NULL);
			}
			break;
		case 13 :
			{	
				struct wizard_wlan_struct my_wlan;
				my_wlan.str = NULL;
				memset(my_wlan.text1, '\0', sizeof(my_wlan.text1));
				wizard_5G_Set_SSID(win, &my_wlan);
				if (strlen(record.wlan1_ssid) != 0) {
		                        lite_set_textline_text(my_wlan.txtline, record.wlan1_ssid);
               			}
				*ret = wizard_page_loop(win, 3, NULL, "E3", &my_wlan);
				*ret = wizard_control_step(*ret, "E3", NULL);
			}
			break;
		case 14 :
			{
				struct wizard_wlan_struct my_wlan;
				my_wlan.str = NULL;
				memset(my_wlan.text1, '\0', sizeof(my_wlan.text1));
				wizard_5G_Set_PWD(win, &my_wlan);
				if (strlen(record.wlan1_pwd) != 0) {
 	                	       lite_set_textline_text(my_wlan.txtline, record.wlan1_pwd);
        		        }
				*ret = wizard_page_loop(win, 3, NULL, "E4", &my_wlan);
				*ret = wizard_control_step(*ret, "E4", NULL);
			}
			break;
		case 15 :
				wizard_optimize_channel_band(win);
				*ret = wizard_page_loop(win, 2, NULL, "E5", NULL);
				*ret = wizard_control_step(*ret, "E5", NULL);
			break;
		case 16 :
			{
				struct __timezone my_tz;
				char *tmp;
#ifndef PC
				my_tz.tz_value = record.tz_value;
#else
				my_tz.tz_value = -192;
#endif
				my_tz.tz = NULL;
#ifdef SELECT_UI
				int opt_i = 0;
				char *opt[256];
				LiteTextLine *text;
				wizard_Time_Zone(win, &my_tz, &text, opt);
				*ret = wizard_page_loop(win, 6, text, "F", opt);
				lite_get_textline_text(text, &tmp);
				while (opt[opt_i])
					free(opt[opt_i++]);
#else
				ComponentList *list;
				wizard_Time_Zone(win, &my_tz, &list);
				*ret = wizard_page_loop(win, 6, list, "F", &my_tz);
				select_get_selected_item(list, &tmp);
#endif
				sprintf(record.timezone, "%s", tmp);
				struct _time_zone *tz = my_tz.tzone;
				for(;tz != NULL; tz++){
					if (strcmp(tz->txt, tmp) == 0) {
						record.tz_value = tz->value;
						break;
					}
				}
				*ret = wizard_control_step(*ret, "F", NULL);
			}
			break;
		case 17 :
			{
				wizard_Summary_internet(win, record);
				*ret = wizard_page_loop(win, 1, NULL, "G1", NULL);
				*ret = wizard_control_step(*ret, "G1", NULL);
			}
			break;
		case 18 :
				wizard_Set_and_Conn(win);
				*ret = wizard_page_loop(win, 4, NULL, "G2", NULL);
				*ret = wizard_control_step(*ret, "G2", NULL);
			break;
		default :
			printf("error");
			break;
	}
	return *ret;
}

static int run() 
{
	int ret = 0;
	LiteWindow *win; 
	DFBRectangle win_rect = { 0, 0, layer_width, layer_height };
	lite_new_window(NULL, &win_rect, DWCAPS_ALPHACHANNEL, NULL, NULL, &win);
	LiteImage *background = image_create(win, win_rect, "wizard_bg.png", 0);
	ret = draw_window(win, &ret);
	lite_destroy_window(win);
	return ret;
}

void wizard_start()
{
	int res = 0;
	current = 0;
	//wizard_init(&record);
	while (1) {
		res = run();
		if (res == WIZARD_CANCEL) //cancel
			break;
		if( res == WIZARD_START_WIZARD_OVER ){
                        // we should clean any previous setting or vaule first ??
                        current = 0;
                        continue ;
                }
                if (res == WIZARD_SAVE_SETTING_AND_REBOOT) { // finish
                        break;
                }
	}
}

