/*
   (c) Copyright 2001-2002  convergence integrated media GmbH.
   (c) Copyright 2002-2005  convergence GmbH.

   Written by Denis Oliver Kropp <dok@directfb.org>
              
   This file is subject to the terms and conditions of the MIT License:

   Permission is hereby granted, free of charge, to any person
   obtaining a copy of this software and associated documentation
   files (the "Software"), to deal in the Software without restriction,
   including without limitation the rights to use, copy, modify, merge,
   publish, distribute, sublicense, and/or sell copies of the Software,
   and to permit persons to whom the Software is furnished to do so,
   subject to the following conditions:

   The above copyright notice and this permission notice shall be
   included in all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
   MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
   IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
   CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
   TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
   SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include "global.h"
#include <stddef.h>

#include <lite/lite.h>
#include <lite/window.h>
#include <lite/util.h>

#include <leck/textbutton.h>
#include <leck/textline.h>
#include "clist.h"
#include "select.h"
#include "input.h"
#include <string.h>

static char *select_name;
static DFBColor list_bg;

static void on_clist_draw_item(CLiteList *l, ListDrawItem *draw_item, void *ctx)
{
	char buf[64];

	sprintf(buf, "%s", draw_item->item_data);

	draw_item->surface->SetColor(
		draw_item->surface,
		list_bg.r, list_bg.g, list_bg.b, list_bg.a );

	draw_item->surface->FillRectangle(
		draw_item->surface,
		draw_item->rc_item.x,
		draw_item->rc_item.y,
		draw_item->rc_item.w,
		draw_item->rc_item.h);

	if(draw_item->disabled)
		draw_item->surface->SetColor(
			draw_item->surface,
			100, 100, 100, 255);
	else
		draw_item->surface->SetColor(
			draw_item->surface,
			0, 0, 0, 255);

	draw_item->surface->DrawString(
		draw_item->surface,
		buf,
		strlen(buf),
		draw_item->rc_item.x + 2,
		draw_item->rc_item.y + 2,
		DSTF_LEFT|DSTF_TOP);

	if(draw_item->selected) {
		draw_item->surface->SetColor(
			draw_item->surface,
			0, 0, 0, 255);
		draw_item->surface->DrawRectangle(
			draw_item->surface,
			draw_item->rc_item.x + 1,
			draw_item->rc_item.y + 1,
			draw_item->rc_item.w - 2,
			draw_item->rc_item.h - 2);
	}
}

static void list_item_changed(CLiteList *list, int new_item, void *ctx)
{
	char *buf;
	ComponentList *lcomp = (ComponentList *)ctx;

	printf("%s(%d)\n", __func__, __LINE__);

	lite_clist_get_item(list, new_item, &buf);
	lite_set_textline_text(lcomp->text, buf);
	lite_set_box_visible(LITE_BOX(lcomp->list), false);
	lcomp->list_visible = false;
}

static void on_clist_popup_pressed(LiteTextButton *button, void *ctx)
{
	ComponentList *lcomp= (ComponentList *)ctx;

	lcomp->list_visible = !lcomp->list_visible;
	lite_set_box_visible(LITE_BOX(lcomp->list), lcomp->list_visible);
}

static int text_on_button_down(
	struct _LiteBox *self,
	int x,
	int y,
	DFBInputDeviceButtonIdentifier button)
{
	keyboard_input(select_name, LITE_TEXTLINE(self));
	return 0;
}

ComponentList *select_create(
	LiteWindow *win,
	DFBRectangle rect,
	DFBRectangle list_rect,
	const char *btn_image,
	const char *scr_image,
	DFBColor color,
	bool editable)
{
	ComponentList *lcomp = malloc(sizeof(ComponentList));
	DFBRectangle btn_rect = { rect.x + (rect.w - 20), rect.y, 20, 20 };
	DFBRectangle text_rect = { rect.x, rect.y, rect.w - 20, 20 };
	char btn_gnd[MAX_FILENAME];
	char scr_gnd[MAX_FILENAME];

	if (lcomp == NULL)
		return NULL;

	list_bg = color;
	sprintf(btn_gnd, "%s%s", IMAGE_DIR, btn_image);
	sprintf(scr_gnd, "%s%s", IMAGE_DIR, scr_image);

	lcomp->list_visible = false;
	lcomp->list_editable = editable;
	lite_get_font("default", LITE_FONT_PLAIN, 12, DEFAULT_FONT_ATTRIBUTE, &lcomp->font);
	lite_font(lcomp->font, &lcomp->font_interface);

	lite_new_text_button_theme(btn_gnd, &liteDefaultTextButtonTheme);
	lite_new_text_button(LITE_BOX(win), &btn_rect, "", liteDefaultTextButtonTheme, &lcomp->btn);
	lite_new_textline(LITE_BOX(win), &text_rect, liteNoTextLineTheme, &lcomp->text);
	lite_new_clist_theme(scr_gnd, 3, &liteDefaultListTheme);
	lite_new_clist(LITE_BOX(win), &list_rect, liteDefaultListTheme, &lcomp->list);
	lite_set_box_visible(LITE_BOX(lcomp->list), lcomp->list_visible);

	lite_clist_set_row_height(lcomp->list, 20);
	(LITE_BOX(lcomp->list))->surface->SetFont(LITE_BOX(lcomp->list)->surface, lcomp->font_interface);
	lite_clist_on_draw_item(lcomp->list, on_clist_draw_item, 0);
	lite_clist_on_sel_change(lcomp->list, list_item_changed, lcomp);
	lite_on_text_button_press(lcomp->btn, on_clist_popup_pressed, lcomp);

	/* // It cause double click event
	if (lcomp->list_editable)
		LITE_BOX(lcomp->text)->OnButtonUp = text_on_button_down;
	*/
	select_name = NULL;
	return lcomp;
}

DFBResult select_destroy(ComponentList *lcomp)
{
#if 0
	lite_destroy_box(LITE_BOX(lcomp->btn));
	lite_destroy_box(LITE_BOX(lcomp->text));
	lite_destroy_box(LITE_BOX(lcomp->list));
#endif
	if (select_name != NULL)
		D_FREE(select_name);

	lite_destroy_clist_theme(liteDefaultListTheme);
	lite_destroy_text_button_theme(liteDefaultTextButtonTheme);
	lite_release_font(lcomp->font);
	free(lcomp);

	return DFB_OK;
}

DFBResult select_set_name(ComponentList *lcomp, char *name)
{
	if (name == NULL)
		return DFB_OK;

	if (select_name == NULL)
		select_name = D_STRDUP(name);
	return DFB_OK;
}

DFBResult select_add_item(ComponentList *lcomp, char *str)
{
	int count;

	lite_clist_get_item_count(lcomp->list, &count);
	return lite_clist_insert_item(lcomp->list, count, str);
}

DFBResult select_set_list_visible(ComponentList *lcomp, bool visible)
{
	lcomp->list_visible = visible;
	return lite_set_box_visible(LITE_BOX(lcomp->list), visible);
}

DFBResult select_set_button_text(ComponentList *lcomp, const char *str)
{
	return lite_set_text_button_caption(lcomp->btn, str);
}

DFBResult select_set_default_item(ComponentList *lcomp, int idx)
{
	int count;
	char *buf;

	lite_clist_get_item_count(lcomp->list, &count);
	if (idx > count || idx < 0)
		return ~DFB_OK;

	lite_clist_get_item(lcomp->list, idx, &buf);
	return lite_set_textline_text(lcomp->text, buf);
}

int select_get_item_count(ComponentList *lcomp, int *count)
{
	DFBResult res = lite_clist_get_item_count(lcomp->list, count);

	if (res != DFB_OK)
		return -1;

	return *count;
}

DFBResult select_get_selected_item(ComponentList *lcomp, char **buf)
{
	return lite_get_textline_text(lcomp->text, buf);
}

DFBResult select_get_item(ComponentList *lcomp, int idx, char **buf)
{
	int nc;

	select_get_item_count(lcomp, &nc);
	if (idx > nc || idx < 0)
		return ~DFB_OK;

	return lite_clist_get_item(lcomp->list, idx, buf);
}

int select_find_item_idx(ComponentList *lcomp, const char *str)
{
	char *buf;
	int i = 0, total_count;

	select_get_item_count(lcomp, &total_count);

	for (; i < total_count; i++) {
		select_get_item(lcomp, i, &buf);
		if (strcmp(buf, str) == 0)
			goto out;
	}

	i = -1;
out:
	return i;
}

int select_update_selected_item(ComponentList *lcomp, char *str)
{
	return lite_set_textline_text(lcomp->text, str);
}
