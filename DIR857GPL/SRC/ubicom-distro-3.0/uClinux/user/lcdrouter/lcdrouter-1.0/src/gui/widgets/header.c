/*
 *  Header GUI Component
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

#include <gui/widgets/header.h>

struct header_instance {
	LiteBox box;//this must be in first place

	LiteLabel	 *text;
	IDirectFBSurface *bg;
	struct statusbar_instance *sbi;

	int refs;

#ifdef DEBUG
	int	magic;
#endif
};

#if defined(DEBUG)
#define HEADER_MAGIC 0xCCBE
#define HEADER_VERIFY_MAGIC(hi) header_verify_magic(hi)

void header_verify_magic(struct header_instance *hi)
{
	DEBUG_ASSERT(hi->magic == HEADER_MAGIC, "%p: bad magic", hi);
}
#else
#define HEADER_VERIFY_MAGIC(hi)
#endif

static void header_instance_destroy(struct header_instance *hi)
{
	DEBUG_ASSERT(hi == NULL, " headet destroy with NULL instance ");
	//free other internal members
#ifdef DEBUG
	hi->magic = 0;
#endif
	free(hi);	
	hi = NULL;
}

int  header_instance_ref(struct header_instance *hi)
{
	HEADER_VERIFY_MAGIC(hi);
	
	return ++hi->refs;
}

int  header_instance_deref(struct header_instance *hi)
{
	HEADER_VERIFY_MAGIC(hi);
	DEBUG_ASSERT(hi->refs, "%p: menu has no references", hi);

	if ( --hi->refs == 0 ) {
		header_instance_destroy(hi);
		return 0;
	}
	else {
		return hi->refs;
	}
}

DFBResult header_instance_set_text(struct header_instance *hi, char *text)
{
	HEADER_VERIFY_MAGIC(hi);
	return lite_set_label_text( hi->text, text);
}

static DFBResult header_instance_box_destroy(LiteBox *box)
{
	struct header_instance *hi = (struct header_instance*)(box);
	//do more here.
	(void)hi;

	return lite_destroy_box(box);
}

static DFBResult header_instance_draw(LiteBox *box, const DFBRegion *region, DFBBoolean clear)
{
	struct header_instance *hi = (struct header_instance*)(box);
	IDirectFBSurface *surface = box->surface;

	DEBUG_ERROR("header_instance_draw called...");

#if 1
	IDirectFBFont *font_interface = NULL;
	LiteFont  *font = NULL;
	DFBResult  res;

	res = lite_get_font("default", LITE_FONT_PLAIN, 16, 
		         DEFAULT_FONT_ATTRIBUTE, &font);
	if (res != DFB_OK) {
		return res;
	}

	res = lite_font(font, &font_interface);

	if (res != DFB_OK) {
	  return res;
	}

	surface->SetFont(surface, font_interface);
#endif

	surface->SetClip(surface, region);

	if ( clear ) {
		lite_clear_box(box, region);
	}

	surface->SetDrawingFlags (surface, DSDRAW_NOFX);

	//draw extras here , check DirectFB docs.
	DFBRectangle      rect;
	rect = hi->box.rect;

	surface->SetBlittingFlags(surface, DSBLIT_BLEND_ALPHACHANNEL);

#if 0
	surface->SetColor(surface, 0xff, 0, 0, 255);
	surface->DrawRectangle(surface, 0, 0, rect.w, rect.h);
#endif
	rect.x = rect.y = 0;
	
	rect.w = rect.w - ICON3_W;
	surface->Blit(surface, statusbar_instance_get_icon(hi->sbi, 3), NULL, rect.w, rect.y);
	rect.w = rect.w - ICON2_W;
	surface->Blit(surface, statusbar_instance_get_icon(hi->sbi, 2), NULL, rect.w, rect.y);
	rect.w = rect.w - ICON1_W;
	surface->Blit(surface, statusbar_instance_get_icon(hi->sbi, 1), NULL, rect.w, rect.y);

	return res;
}

DFBResult header_instance_alloc(LiteBox *parent, DFBRectangle *rect, struct statusbar_instance *sbi, struct header_instance **ret_hi)
{
	DFBResult   res = DFB_FAILURE;

	struct header_instance *hi = (struct header_instance*) calloc(1, sizeof(struct header_instance));
	if ( !hi ) {
		DEBUG_ERROR("struct header_instance instance  create failed...");
		goto error;
	}

	hi->refs = 1;	

#ifdef DEBUG
	hi->magic = HEADER_MAGIC ;
#endif

	hi->box.parent = parent;
	hi->box.rect   = *rect;
	
	res = lite_init_box(LITE_BOX(hi));
	if (res != DFB_OK) {
		DEBUG_ERROR("Lite box for header instance init failed...");
		goto error;
	}

	hi->box.type    = 0x6000;
	hi->box.Draw    = header_instance_draw;
	hi->box.Destroy = header_instance_box_destroy;

	hi->sbi = sbi;

	/* setup label */
	DFBRectangle text_rect;
	text_rect.x = 5; text_rect.y = (40-22)/2 ; text_rect.w = 200; text_rect.h= 22;
	res = lite_new_label(LITE_BOX(hi), &text_rect, liteNoLabelTheme, 18, &hi->text );

	lite_set_label_text( hi->text, "Menu");

	lite_update_box(LITE_BOX(hi), NULL);
	
	*ret_hi = hi;
	return DFB_OK;
error:
	//free unneeded values here
	if ( hi != NULL) {
		header_instance_destroy( hi);
	}
	return res;
}

