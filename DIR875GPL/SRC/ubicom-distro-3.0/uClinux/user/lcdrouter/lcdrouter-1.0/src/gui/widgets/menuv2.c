/*
 *  Menu GUI Component
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

#include <gui/widgets/menuv2.h>

struct menu_instance {
	LiteBox box;//this must be in first place
	struct menu_data_instance *menu_data;

	LiteTextButton *button[ITEM_PER_PAGE_FOR_LIST_WITH_IMAGES];
	LiteImage    *image[ITEM_PER_PAGE_FOR_LIST_WITH_IMAGES];
	LiteTextButtonTheme *textButtonTheme;

	struct scroll_instance *si;
	struct header_instance *hi;

	menu_instance_on_item_select on_item_select;
	void *on_item_select_data;

	menu_instance_on_prev_menu on_prev_menu;
	void *on_prev_menu_data;
	int last_id;
	int item_count;
	int current_item;
	int current_page;
	int item_per_page;

	int item_h;
	int item_w;

	int c2c_space;// pixels between two components
	int c2b_space;// pixels between component and box edge

	int refs;

#ifdef DEBUG
	int	magic;
#endif
};

#if defined(DEBUG)
#define MENU_MAGIC 0xBABE
#define MENU_VERIFY_MAGIC(mi) menu_verify_magic(mi)

void menu_verify_magic(struct menu_instance *mi)
{
	DEBUG_ASSERT(mi->magic == MENU_MAGIC, "%p: bad magic", mi);
}
#else
#define MENU_VERIFY_MAGIC(mi)
#endif

static void menu_instance_destroy(struct menu_instance *mi)
{
	DEBUG_ASSERT(mi == NULL, " menu destroy with NULL instance ");
	//free other internal members
#ifdef DEBUG
	mi->magic = 0;
#endif
	free(mi);	
	mi = NULL;
}

int  menu_instance_ref(struct menu_instance *mi)
{
	MENU_VERIFY_MAGIC(mi);
	//needs pthread llock
	
	return ++mi->refs;
}

int  menu_instance_deref(struct menu_instance *mi)
{
	MENU_VERIFY_MAGIC(mi);
	DEBUG_ASSERT(mi->refs, "%p: menu has no references", mi);
	//needs pthread llock
	if ( --mi->refs == 0 ) {
		menu_instance_destroy(mi);
		return 0;
	}
	else {
		return mi->refs;
	}
}

static DFBResult menu_instance_box_destroy(LiteBox *box)
{
	struct menu_instance *mi = (struct menu_instance*)(box);
	//do more here.
	(void)mi;

	return lite_destroy_box(box);
}

void menu_instance_set_header_instance(struct menu_instance *mi, struct header_instance *hi)
{
	MENU_VERIFY_MAGIC(mi);
	mi->hi = hi;
}

void menu_instance_set_menu_data(struct menu_instance *mi, struct menu_data_instance *menu_data)
{
	MENU_VERIFY_MAGIC(mi);
	mi->menu_data = menu_data;

	mi->item_count = menu_data_instance_get_list_item_count(mi->menu_data) - 1;
	
	if ( mi->item_count > 0 ) {
		mi->current_item = 1;
		mi->last_id = -1;
		DEBUG_ERROR("menu_data added to menu instance with %d items...", mi->item_count);
		lite_update_box(LITE_BOX(mi), NULL);
	}
}

struct menu_data_instance* menu_instance_get_menu_data(struct menu_instance *mi)
{
	MENU_VERIFY_MAGIC(mi);
	return mi->menu_data;
}

void menu_instance_set_on_item_select(struct menu_instance *mi, menu_instance_on_item_select func , void *app_data)
{
	MENU_VERIFY_MAGIC(mi);
	mi->on_item_select = func;
	mi->on_item_select_data = app_data;
}

void menu_instance_set_on_prev_menu(struct menu_instance *mi, menu_instance_on_prev_menu func, void *app_data)
{
	MENU_VERIFY_MAGIC(mi);
	mi->on_prev_menu = func;
	mi->on_prev_menu_data = app_data;
}

int menu_instance_get_current_item(struct menu_instance *mi)
{
	MENU_VERIFY_MAGIC(mi);
	return mi->current_item;
}

void menu_instance_set_current_item(struct menu_instance *mi, int id)
{
	MENU_VERIFY_MAGIC(mi);
	mi->current_item = id;
}

static DFBResult menu_instance_draw_list_with_images(struct menu_instance *mi, int id, DFBRectangle *rect)
{
	IDirectFBSurface *surface = mi->box.surface;	
	DFBResult   res;
	int tmp = id - 1;//adjust for array

	(void)surface;

	char *text = menu_data_instance_get_metadata(menu_data_instance_get_list_item(mi->menu_data, id), LIST_TEXT1);
	if( text != NULL) {
		DEBUG_ERROR("item draw id=%d %s", id, text);
		lite_set_box_visible(LITE_BOX(mi->button[ ( tmp % mi->item_per_page) ]), 1);
		lite_set_box_visible(LITE_BOX(mi->image[ ( tmp % mi->item_per_page) ]), 1);
		res = lite_set_text_button_caption( mi->button[ ( tmp % mi->item_per_page) ], text);
		char *icon = menu_data_instance_get_metadata(menu_data_instance_get_list_item(mi->menu_data, id), LIST_RIGHT_ICON);
		if( icon != NULL) {
			res = lite_load_image( mi->image[( tmp % mi->item_per_page)], icon);
		}
	}
	else {
		lite_set_box_visible(LITE_BOX(mi->button[ ( tmp % mi->item_per_page) ]), 0);
		lite_set_box_visible(LITE_BOX(mi->image[ ( tmp % mi->item_per_page) ]), 0);
	}

	rect->y += mi->item_h + mi->c2c_space;

	return DFB_OK;
}

static DFBResult menu_instance_draw_items(struct menu_instance *mi, const DFBRegion *region)
{
	IDirectFBSurface *surface = mi->box.surface;

	DFBRectangle item_rect;
	item_rect.x = mi->c2b_space;
	item_rect.y = mi->c2b_space;
	item_rect.w = mi->item_w;
	item_rect.h = mi->item_h;

	if (mi->current_item > 0) {
		surface->SetColor(surface, 0xff, 0, 0, 255);
		surface->DrawRectangle(surface, LITE_BOX(mi->button[ ( (mi->current_item-1) % mi->item_per_page) ])->rect.x-1, 
						LITE_BOX(mi->button[ ( (mi->current_item-1) % mi->item_per_page) ])->rect.y-1, 
						LITE_BOX(mi->button[ ( (mi->current_item-1) % mi->item_per_page) ])->rect.w+2,
						LITE_BOX(mi->button[ ( (mi->current_item-1) % mi->item_per_page) ])->rect.h+2);
	}

	int id;

	int first_id = ((mi->current_page - 1) * mi->item_per_page ) + 1;

	if ( mi->last_id == first_id ){
		return DFB_OK;
	}

	char *header_text = menu_data_instance_get_metadata(menu_data_instance_get_list_item(mi->menu_data, 0), HEADER_TEXT);
	if ( header_text != NULL && mi->hi != NULL) {
		header_instance_set_text(mi->hi, header_text); 
	}

	for ( id = first_id ; id < first_id + mi->item_per_page ; id++ ) {
		menu_instance_draw_list_with_images(mi, id, &item_rect);
	}

	mi->last_id = first_id;

	return DFB_OK;
}

static DFBResult menu_instance_draw(LiteBox *box, const DFBRegion *region, DFBBoolean clear)
{
	struct menu_instance *mi = (struct menu_instance*)(box);
	IDirectFBSurface *surface = box->surface;

	DEBUG_ERROR("menu_instance_draw called...");

	surface->SetClip(surface, region);

	if ( clear ) {
		lite_clear_box(box, region);
	}

	surface->SetDrawingFlags (surface, DSDRAW_NOFX);

	if( mi->item_count < 1 ) {
		DEBUG_WARN("No item in list...");
		return DFB_OK;
	}

	//draw items
	menu_instance_draw_items(mi, region);

	DEBUG_ERROR("menu_instance_draw success...");

	return DFB_OK;
}

int menu_instance_next_page(struct menu_instance *mi)
{

	int page_count = ceil((float)mi->item_count / (float)mi->item_per_page);

	if ( page_count > mi->current_page) {
#ifdef ANIMATION_ENABLED
		if (mi->si) {
			lite_set_box_visible(LITE_BOX(mi->si), 0);
		}
		lite_draw_box(LITE_BOX(mi)->parent, NULL, 0);
#endif
		mi->current_page++;
		mi->current_item = ((mi->current_page - 1) * mi->item_per_page ) + 1;

#ifdef ANIMATION_ENABLED
		SlideAnimation *animation;
		animation = create_animation(LITE_BOX(mi)->parent->surface, NULL, LITE_BOX(mi)->parent->surface, LEFT2RIGHT);
#endif

#ifdef ANIMATION_ENABLED
		//lite_draw_box(LITE_BOX(list), NULL, 1);
		lite_draw_box(LITE_BOX(mi)->parent, NULL, 0);
#else
		lite_update_box(LITE_BOX(mi), NULL);
#endif

#ifdef ANIMATION_ENABLED
		render_to_animation_surface(animation, LITE_BOX(mi)->parent->surface, 2);
	        do_animation(animation);
		destroy_animation(animation);
		if (mi->si) {
			lite_set_box_visible(LITE_BOX(mi->si), 1);
		}
		lite_update_box(LITE_BOX(mi), NULL);
#endif
		return 1;
	}
	return 0;
}

int menu_instance_prev_page(struct menu_instance *mi)
{
	int page_count = ceil((float)mi->item_count / (float)mi->item_per_page);

	if ( page_count >= mi->current_page && mi->current_page != 1  ) {
#ifdef ANIMATION_ENABLED
		if (mi->si) {
			lite_set_box_visible(LITE_BOX(mi->si), 0);
		}
		lite_draw_box(LITE_BOX(mi)->parent, NULL, 0);
#endif
		mi->current_page--;
		mi->current_item = ((mi->current_page - 1) * mi->item_per_page ) + 1;

#ifdef ANIMATION_ENABLED
		SlideAnimation *animation;
		animation = create_animation(LITE_BOX(mi)->parent->surface, NULL, LITE_BOX(mi)->parent->surface, RIGHT2LEFT);
#endif	
#ifdef ANIMATION_ENABLED
		//lite_draw_box(LITE_BOX(list), NULL, 1);
		lite_draw_box(LITE_BOX(mi)->parent, NULL, 0);
#else
		lite_update_box(LITE_BOX(mi), NULL);
#endif

#ifdef ANIMATION_ENABLED
		render_to_animation_surface(animation, LITE_BOX(mi)->parent->surface, 1);
	        do_animation(animation);
		destroy_animation(animation);
		if (mi->si) {
			lite_set_box_visible(LITE_BOX(mi->si), 1);
		}
		lite_update_box(LITE_BOX(mi), NULL);
#endif
		return 1;
	}
	return 0;
}

static void menu_instance_prev_menu( LiteButton *button, void *ctx )
{
	struct menu_instance *mi = (struct menu_instance*)(ctx);

	if( mi->menu_data && menu_data_instance_get_prev_menu(mi->menu_data) != NULL) {
	
		menu_data_instance_set_focused_item(mi->menu_data, mi->current_item);
		
		struct menu_data_instance *tmp = menu_data_instance_get_prev_menu(mi->menu_data);

		if (menu_data_instance_get_listitem_flag(menu_data_instance_get_list_item(tmp, menu_data_instance_get_focused_item(tmp))) == MAY_HAVE_SUBMENU) {
//			menu_data_instance_deref(mi->menu_data);
//			DEBUG_ERROR("Submenu destroyed = %p", mi->menu_data);
//			menu_data_instance_set_listitem_submenu(menu_data_instance_get_list_item(tmp, menu_data_instance_get_focused_item(tmp)), NULL);
			menu_data_instance_set_listitem_submenu(menu_data_instance_get_list_item(tmp, menu_data_instance_get_focused_item(tmp)), mi->menu_data);
		}

		menu_instance_set_menu_data(mi, tmp);
		mi->current_item = 1;
		mi->current_page = 1;

#ifdef ANIMATION_ENABLED
		SlideAnimation *animation;
		animation = create_animation(LITE_BOX(mi)->parent->surface, NULL, LITE_BOX(mi)->parent->surface, RIGHT2LEFT);
#endif	
#ifdef ANIMATION_ENABLED
		lite_draw_box(LITE_BOX(mi)->parent, NULL, 1);
#else
		lite_update_box(LITE_BOX(mi), NULL);
#endif
#ifdef ANIMATION_ENABLED
		render_to_animation_surface(animation, LITE_BOX(mi)->parent->surface, 1);
	        do_animation(animation);
		destroy_animation(animation);
		lite_update_box(LITE_BOX(mi), NULL);
#endif

	}
	else {
		if (mi->on_prev_menu) {
			mi->on_prev_menu(mi, mi->on_prev_menu_data);
		}
	}
}

static void menu_instance_scroll_up_pressed( LiteButton *button, void *ctx )
{
	struct menu_instance *mi = (struct menu_instance*)(ctx);
	menu_instance_prev_page( mi );
}

static void menu_instance_scroll_down_pressed( LiteButton *button, void *ctx )
{
	struct menu_instance *mi = (struct menu_instance*)(ctx);
	menu_instance_next_page( mi );
}

void menu_instance_attach_scrollbar(struct menu_instance *mi, struct scroll_instance *si)
{
	mi->si = si;
	lite_on_button_press( scroll_instance_get_button(si, 1), menu_instance_scroll_up_pressed, mi);
	lite_on_button_press( scroll_instance_get_button(si, 0), menu_instance_scroll_down_pressed, mi);
	lite_on_button_press( scroll_instance_get_button(si, 2), menu_instance_prev_menu, mi);
}

static void menu_instance_item_select(struct menu_instance *mi)
{
	if (mi->on_item_select) {
		mi->on_item_select(mi, mi->on_item_select_data);
	}

	struct menu_data_instance *mdi = NULL;
	struct list_item *lip = NULL;

	lip = menu_data_instance_get_list_item(mi->menu_data, mi->current_item);

	if (lip) {
		mdi = menu_data_instance_get_listitem_submenu(lip);
	} 

	if((void*)mdi == NULL) {
		menu_data_instance_callback_t callback = (menu_data_instance_callback_t)menu_data_instance_get_listitem_callback(lip);
		if( callback != NULL) {
			void *callback_data = menu_data_instance_get_listitem_callbackdata(lip);
			callback(mi->menu_data, mi->current_item, callback_data);
		}
	}

	// for to understand late binding happened or not
	if (lip) {
		mdi = menu_data_instance_get_listitem_submenu(lip);
	} 

	if((void*)mdi != NULL) {

		menu_data_instance_set_focused_item(mi->menu_data, mi->current_item);
		
		menu_instance_set_menu_data(mi, mdi);
		mi->current_item = 1;
		mi->current_page = 1;

#ifdef ANIMATION_ENABLED
		SlideAnimation *animation;
		animation = create_animation(LITE_BOX(mi)->parent->surface, NULL, LITE_BOX(mi)->parent->surface, LEFT2RIGHT);
#endif	
#ifdef ANIMATION_ENABLED
		lite_draw_box(LITE_BOX(mi)->parent, NULL, 1);
#else
		lite_update_box(LITE_BOX(mi), NULL);
#endif
#ifdef ANIMATION_ENABLED
		render_to_animation_surface(animation, LITE_BOX(mi)->parent->surface, 2);
	        do_animation(animation);
		destroy_animation(animation);
		lite_update_box(LITE_BOX(mi), NULL);
#endif
	}
}

static void button_pressed( LiteButton *button, void *ctx )
{
	struct menu_instance *mi = (struct menu_instance*)(ctx);

	int i;
	for ( i = 0 ; i < mi->item_per_page; i++ ) {
		if (mi->button[i] == button ) {
			break;
		}
	}
	DEBUG_ERROR("%d. menuv2 button pressed ", i);

	mi->current_item = ((mi->current_page-1) * mi->item_per_page) + i + 1;

	menu_instance_item_select(mi);
	//use int i for to trigger action with 
}

static int 
on_enter(LiteBox *box, int x, int y)
{
     D_ASSERT(box != NULL);
     LITE_BOX_TYPE_PARAMETER_CHECK(LITE_BOX(box), LITE_TYPE_LIST);

     return 1;
}

static int
on_leave(LiteBox *box, int x, int y)
{
     D_ASSERT(box != NULL);
     LITE_BOX_TYPE_PARAMETER_CHECK(LITE_BOX(box), LITE_TYPE_LIST);
     
     return 1;
}

static int 
on_focus_in(LiteBox *box)
{
     D_ASSERT(box != NULL);
     LITE_BOX_TYPE_PARAMETER_CHECK(LITE_BOX(box), LITE_TYPE_LIST);

     lite_update_box(box, NULL);

     return 1;
}

static int 
on_focus_out(LiteBox *box)
{
     D_ASSERT(box != NULL);
     LITE_BOX_TYPE_PARAMETER_CHECK(LITE_BOX(box), LITE_TYPE_LIST);

     lite_update_box(box, NULL);

     return 1;
}

static int 
on_button_down(LiteBox                       *box, 
                 int                            x, 
                 int                            y,
                 DFBInputDeviceButtonIdentifier button)
{
	D_ASSERT(box != NULL);
	LITE_BOX_TYPE_PARAMETER_CHECK(LITE_BOX(box), LITE_TYPE_LIST);
	DEBUG_ERROR("on_button_down x = %d  y = %d \n", x, y);
	return 1;
}

static int 
on_button_up(LiteBox                       *box, 
             int                            x, 
             int                            y,
             DFBInputDeviceButtonIdentifier button)
{
	D_ASSERT(box != NULL);
	LITE_BOX_TYPE_PARAMETER_CHECK(LITE_BOX(box), LITE_TYPE_LIST);
	DEBUG_ERROR("on_button_up x = %d  y = %d \n", x, y);
	return 1;
}

static int 
on_motion(LiteBox                 *box, 
          int                      x, 
          int                      y,
          DFBInputDeviceButtonMask buttons)
{
	D_ASSERT(box != NULL);
	LITE_BOX_TYPE_PARAMETER_CHECK(LITE_BOX(box), LITE_TYPE_LIST);

	DEBUG_ERROR("on_motion x = %d  y = %d \n", x, y);
	return 1;
}

static int
on_key_down(LiteBox *box, DFBWindowEvent *evt)
{
	struct menu_instance *mi = (struct menu_instance*)(box);
	DEBUG_ERROR("on_key_down() called. key_symbol : %d\n", evt->key_symbol);

	if ( mi->item_count < 1 ) {
        	return 1;
	}

	int first_id = ((mi->current_page - 1) * mi->item_per_page ) + 1;

	if ( evt->key_symbol == 61441 ) { //right
		menu_instance_next_page(mi);
	}

	if ( evt->key_symbol == 61440 ) { //left
		menu_instance_prev_page(mi);
	}

	if ( evt->key_symbol == 61442 ) { //up
		if ( mi->current_item > first_id) {
			mi->current_item--;
			lite_update_box(LITE_BOX(mi), NULL);
		}
	}

	if ( evt->key_symbol == 61443 ) { //down
		int item_count_tmp = (mi->current_page * mi->item_per_page > mi->item_count ?  
					mi->item_count % mi->item_per_page : mi->item_per_page);//item count for current page

		if ( mi->current_item < first_id + item_count_tmp -1) {
			mi->current_item++;
			lite_update_box(LITE_BOX(mi), NULL);
		}
	}

	if ( evt->key_symbol == 13 ) { //enter
		menu_instance_item_select(mi);
	}

	if ( evt->key_symbol == 27) { //back
		menu_instance_prev_menu( NULL, mi );
	}

	return 1;
}

DFBResult menu_instance_alloc(LiteBox *parent, DFBRectangle *rect, struct menu_instance **ret_mi)
{
	DFBResult   res = DFB_FAILURE;

	struct menu_instance *mi = (struct menu_instance*) calloc(1, sizeof(struct menu_instance));
	if ( !mi ) {
		DEBUG_ERROR("struct menu_instance instance  create failed...");
		goto error;
	}

	mi->refs = 1;	

#ifdef DEBUG
	mi->magic = MENU_MAGIC ;
#endif

	mi->box.parent = parent;
	mi->box.rect   = *rect;
	
	res = lite_init_box(LITE_BOX(mi));
	if (res != DFB_OK) {
		DEBUG_ERROR("Lite box for menu instance init failed...");
		goto error;
	}

	mi->box.type    = 0x5000;
	mi->box.Draw    = menu_instance_draw;
	mi->box.Destroy = menu_instance_box_destroy;

	mi->box.OnEnter      = on_enter;
	mi->box.OnLeave      = on_leave;
	mi->box.OnFocusIn    = on_focus_in;
	mi->box.OnFocusOut   = on_focus_out;
	mi->box.OnButtonDown = on_button_down;
	mi->box.OnButtonUp   = on_button_up;
	mi->box.OnMotion     = on_motion;
	mi->box.OnKeyDown    = on_key_down;

	mi->item_count = 0;
	mi->current_item = -1;
	mi->current_page = 1;
	mi->last_id = -1;

	mi->c2c_space = MENU_C2C_SPACE;// pixels between two components
	mi->c2b_space = MENU_C2B_SPACE;// pixels between component and box edge

	mi->item_h = ITEM_HEIGHT_FOR_LIST_WITH_IMAGES;
	mi->item_w = (rect->w - (2 * MENU_C2B_SPACE) );
	mi->item_per_page = ITEM_PER_PAGE_FOR_LIST_WITH_IMAGES;

	DEBUG_ERROR("item_h = %d  item_w = %d",mi->item_h, mi->item_w);

	res = lite_new_text_button_theme(
		"/share/LiTE/textbuttonbgnd.png",
		&mi->textButtonTheme);

	if (res != DFB_OK) {
		DEBUG_ERROR("menu button theme init failed...");
		goto error;
	}

	DFBRectangle item_rect;
	item_rect.x = mi->c2b_space;
	item_rect.y = mi->c2b_space;
	item_rect.w = mi->item_w;
	item_rect.h = mi->item_h;

	DFBRectangle image_area_rect;
	image_area_rect.x = item_rect.x;
	image_area_rect.y = item_rect.y;
	image_area_rect.w = ITEM_IMAGE_WIDTH;
	image_area_rect.h = ITEM_IMAGE_HEIGHT;

	int i;
	for ( i = 0 ; i < mi->item_per_page ; i++) {
		res = lite_new_text_button(LITE_BOX(mi), &item_rect, "", mi->textButtonTheme, &mi->button[i]);

		if (res != DFB_OK) {
			DEBUG_ERROR("menu button create failed...");
			goto error;
		}
		res = lite_new_image(LITE_BOX(mi), &image_area_rect, liteNoImageTheme, &mi->image[i]);
		if (res != DFB_OK) {
			DEBUG_ERROR("menu image create failed...");
			goto error;
		}

		lite_set_box_visible(LITE_BOX(mi->button[i]), 0);
		lite_on_text_button_press(mi->button[i], button_pressed, mi);

		image_area_rect.y += mi->item_h + mi->c2c_space;
		item_rect.y += mi->item_h + mi->c2c_space;
	}

	lite_update_box(LITE_BOX(mi), NULL);
	
	*ret_mi = mi;
	return DFB_OK;
error:
	//free unneeded values here
	if ( mi != NULL) {
		menu_instance_destroy( mi);
	}
	return res;
}

