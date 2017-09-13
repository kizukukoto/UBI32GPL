/*
 *  Scrollbar GUI Component
 *
 * (C) Copyright 2009, Ubicom, Inc.
 * Celil URGAN curgan@ubicom.com
 *
 * The Network Player Reference Code is free software: you can redistribute
 * it and/or modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation, either version 2 of the
 * License, or (at your option) any later version.
 *
 * The Network Player Reference Code is distributed in the hope that it
 * will be useful, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
 * the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with the Network Audio Device Reference Code.  If not,
 * see <http://www.gnu.org/licenses/>.
 *
 */

#include <gui/widgets/scroll.h>

struct scroll_instance {
	LiteBox box;//this must be in first place

	LiteButton *button[BUTTON_COUNT];
	int button_count;

	int refs;

#ifdef DEBUG
	int	magic;
#endif
};

#if defined(DEBUG)
#define SCROLL_MAGIC 0xCCBE
#define SCROLL_VERIFY_MAGIC(si) scroll_verify_magic(si)

void scroll_verify_magic(struct scroll_instance *si)
{
	DEBUG_ASSERT(si->magic == SCROLL_MAGIC, "%p: bad magic", si);
}
#else
#define SCROLL_VERIFY_MAGIC(si)
#endif

static void scroll_instance_destroy(struct scroll_instance *si)
{
	DEBUG_ASSERT(si != NULL, " header destroy with NULL instance ");
	//free other internal members
#ifdef DEBUG
	si->magic = 0;
#endif
	lite_destroy_box(LITE_BOX(si));
	//free(si);
	si = NULL;
}

int  scroll_instance_ref(struct scroll_instance *si)
{
	SCROLL_VERIFY_MAGIC(si);
	
	return ++si->refs;
}

int  scroll_instance_deref(struct scroll_instance *si)
{
	SCROLL_VERIFY_MAGIC(si);
	DEBUG_ASSERT(si->refs, "%p: menu has no references", si);

	if ( --si->refs == 0 ) {
		scroll_instance_destroy(si);
		return 0;
	}
	else {
		return si->refs;
	}
}

static DFBResult scroll_instance_box_destroy(LiteBox *box)
{
	struct scroll_instance *si = (struct scroll_instance*)(box);
	//do more here.
	(void)si;

	return lite_destroy_box(box);
}

static DFBResult scroll_instance_draw(LiteBox *box, const DFBRegion *region, DFBBoolean clear)
{
	struct scroll_instance *si = (struct scroll_instance*)(box);
	IDirectFBSurface *surface = box->surface;

	DEBUG_ERROR("scroll_instance_draw called...");

	surface->SetClip(surface, region);

	if ( clear ) {
		lite_clear_box(box, region);
	}

	surface->SetDrawingFlags (surface, DSDRAW_NOFX);

	return DFB_OK;
}

LiteButton* scroll_instance_get_button(struct scroll_instance *si, int id)
{
	SCROLL_VERIFY_MAGIC(si);
	return si->button[id];
}

static void button_pressed( LiteButton *button, void *ctx )
{
	struct scroll_instance *si = (struct scroll_instance*)(ctx);

	int i;
	for ( i = 0 ; i < si->button_count; i++ ) {
		if (si->button[i] == button ) {
			break;
		}
	}
	DEBUG_ERROR("%d. Scroll button pressed ", i);
	//use int i for to trigger action with 
}

int scroll_instance_set_button_icon(struct scroll_instance *si, int id, char *url1, char *url2, char *url3)
{
	SCROLL_VERIFY_MAGIC(si);
	if (url2 == NULL) {
		url2 = url1;
	}
	if (url3 == NULL) {
		url3 = url1;
	}
	lite_set_button_image( si->button[id], LITE_BS_NORMAL, url1);
	lite_set_button_image( si->button[id], LITE_BS_PRESSED, url2);
	lite_set_button_image( si->button[id], LITE_BS_HILITE, url3);
	return 1;
}

DFBResult scroll_instance_alloc(LiteBox *parent, DFBRectangle *rect, struct scroll_instance **ret_si)
{
	DFBResult   res = DFB_FAILURE;

	struct scroll_instance *si = (struct scroll_instance*) calloc(1, sizeof(struct scroll_instance));
	if ( !si ) {
		DEBUG_ERROR("struct scroll_instance instance  create failed...");
		goto error;
	}

	si->refs = 1;	

#ifdef DEBUG
	si->magic = SCROLL_MAGIC ;
#endif

	si->box.parent = parent;
	si->box.rect   = *rect;
	
	res = lite_init_box(LITE_BOX(si));
	if (res != DFB_OK) {
		DEBUG_ERROR("Lite box for scroll instance init failed...");
		goto error;
	}

	si->box.type    = 0x7000;
	si->box.Draw    = scroll_instance_draw;
	si->box.Destroy = scroll_instance_box_destroy;

	si->button_count = BUTTON_COUNT;

	DFBRectangle item_rect;
	item_rect.x = 0;
	item_rect.y = 0;
	item_rect.w = 45;
	item_rect.h = 45;

	int i;
	for ( i = 0 ; i < BUTTON_COUNT ; i++) {
		res = lite_new_button(LITE_BOX(si), &item_rect, NULL, &si->button[i]);
		if (res != DFB_OK) {
			DEBUG_ERROR("scroll box button create failed...");
			goto error;
		}
		lite_on_button_press( si->button[i], button_pressed, si);
		item_rect.y += item_rect.h + 5;
	}

	lite_set_button_image( si->button[0], LITE_BS_NORMAL, "/share/LiTE/lcdrouter/theme/sky/right2.png" );
	lite_set_button_image( si->button[0], LITE_BS_PRESSED, "/share/LiTE/lcdrouter/theme/sky/right2.png" );
	lite_set_button_image( si->button[0], LITE_BS_HILITE, "/share/LiTE/lcdrouter/theme/sky/right2_h.png" );

	lite_set_button_image( si->button[1], LITE_BS_NORMAL, "/share/LiTE/lcdrouter/theme/sky/left2.png" );
	lite_set_button_image( si->button[1], LITE_BS_PRESSED, "/share/LiTE/lcdrouter/theme/sky/left2.png" );
	lite_set_button_image( si->button[1], LITE_BS_HILITE, "/share/LiTE/lcdrouter/theme/sky/left2_h.png" );

	lite_set_button_image( si->button[2], LITE_BS_NORMAL, "/share/LiTE/lcdrouter/theme/sky/up1.png" );
	lite_set_button_image( si->button[2], LITE_BS_PRESSED, "/share/LiTE/lcdrouter/theme/sky/up1.png" );
	lite_set_button_image( si->button[2], LITE_BS_HILITE, "/share/LiTE/lcdrouter/theme/sky/up1_h.png" );
/*
	lite_set_button_image( si->button[3], LITE_BS_NORMAL, "/share/LiTE/lcdrouter/theme/sky/down1.png" );
	lite_set_button_image( si->button[3], LITE_BS_PRESSED, "/share/LiTE/lcdrouter/theme/sky/down1.png" );
	lite_set_button_image( si->button[3], LITE_BS_HILITE, "/share/LiTE/lcdrouter/theme/sky/down1_h.png" );
*/
	lite_update_box(LITE_BOX(si), NULL);
	
	*ret_si = si;
	return DFB_OK;
error:
	//free unneeded values here
	if ( si != NULL) {
		scroll_instance_destroy( si);
	}
	return res;
}

