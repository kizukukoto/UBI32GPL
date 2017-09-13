#ifndef __SELECT_H__
#define __SELECT_H__
#include <leck/textline.h>
#include <leck/textbutton.h>
#include "clist.h"

typedef int (*OnButtonDown)(struct _LiteBox *, int, int, DFBInputDeviceButtonIdentifier);

typedef struct _ComponentList {
	LiteTextButton *btn;
	LiteTextLine *text;
	CLiteList *list;
	LiteFont *font;
	IDirectFBFont *font_interface;
	bool list_visible;
	bool list_editable;
} ComponentList;

extern ComponentList *select_create(
	LiteWindow *win,
	DFBRectangle rect,
	DFBRectangle list_rect,
	const char *btn_image,
	const char *scr_image,
	DFBColor list_bgcolor,
	bool editable);
extern DFBResult select_destroy(ComponentList *);
extern DFBResult select_set_name(ComponentList *, char *);
extern DFBResult select_add_item(ComponentList *, char *);
extern DFBResult select_set_list_visible(ComponentList *, bool);
extern DFBResult select_set_button_text(ComponentList *, const char *);
extern DFBResult select_set_default_item(ComponentList *, int);
extern DFBResult select_set_list_editable(ComponentList *, bool);
extern DFBResult select_get_selected_item(ComponentList *, char **);
extern DFBResult select_get_item(ComponentList *, int , char **);
extern int select_get_item_count(ComponentList *, int *);
extern int select_find_item_idx(ComponentList *, const char *);
extern int select_update_selected_item(ComponentList *, char *);
#endif
