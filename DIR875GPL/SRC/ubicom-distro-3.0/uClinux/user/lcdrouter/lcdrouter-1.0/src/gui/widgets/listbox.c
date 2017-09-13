/*
 *  Listbox GUI Component
 *
 * (C) Copyright 2009, Ubicom, Inc.
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

#include <gui/widgets/listbox.h>

struct listbox_instance {
	LiteBox box;//this must be in first place
	struct menu_data_instance *menu_data;

	LiteTextButton *button[ITEM_PER_PAGE];
	LiteTextButtonTheme *textButtonTheme;

	struct scroll_instance *si;
	struct header_instance *hi;

	listbox_instance_on_item_select on_item_select;
	void *on_item_select_data;

	listbox_instance_on_go_back on_go_back;
	void *on_go_back_data;

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
#define LISTBOX_MAGIC 0xBABE
#define LISTBOX_VERIFY_MAGIC(li) listbox_verify_magic(li)

void listbox_verify_magic(struct listbox_instance *li)
{
	DEBUG_ASSERT(li->magic == LISTBOX_MAGIC, "%p: bad magic", li);
}
#else
#define LISTBOX_VERIFY_MAGIC(li)
#endif

static void listbox_instance_destroy(struct listbox_instance *li)
{
	DEBUG_ASSERT(li == NULL, " menu destroy with NULL instance ");
	//free other internal members
#ifdef DEBUG
	li->magic = 0;
#endif
	free(li);	
	li = NULL;
}

int  listbox_instance_ref(struct listbox_instance *li)
{
	LISTBOX_VERIFY_MAGIC(li);
	//needs pthread llock
	
	return ++li->refs;
}

int  listbox_instance_deref(struct listbox_instance *li)
{
	LISTBOX_VERIFY_MAGIC(li);
	DEBUG_ASSERT(li->refs, "%p: menu has no references", li);
	//needs pthread llock
	if ( --li->refs == 0 ) {
		listbox_instance_destroy(li);
		return 0;
	}
	else {
		return li->refs;
	}
}

static DFBResult listbox_instance_box_destroy(LiteBox *box)
{
	struct listbox_instance *li = (struct listbox_instance*)(box);
	//do more here.
	(void)li;

	return lite_destroy_box(box);
}

void listbox_instance_set_header_instance(struct listbox_instance *li, struct header_instance *hi)
{
	LISTBOX_VERIFY_MAGIC(li);
	li->hi = hi;
}
void listbox_instance_set_menu_data(struct listbox_instance *li, struct menu_data_instance *menu_data)
{
	LISTBOX_VERIFY_MAGIC(li);
	li->menu_data = menu_data;

	li->item_count = menu_data_instance_get_list_item_count(li->menu_data) - 1;
	
	if (li->item_count > 0) { //FIXME: if menudata has just header than refresh not occurs.
		li->current_item = 1;
		li->last_id = 0;
		DEBUG_ERROR("menu_data added to menu instance with %d items...", li->item_count);
		lite_update_box(LITE_BOX(li), NULL);
	}
}

struct menu_data_instance* listbox_instance_get_menu_data(struct listbox_instance *li)
{
	LISTBOX_VERIFY_MAGIC(li);
	return li->menu_data;
}

void listbox_instance_set_on_item_select(struct listbox_instance *li, listbox_instance_on_item_select func , void *app_data)
{
	LISTBOX_VERIFY_MAGIC(li);
	li->on_item_select = func;
	li->on_item_select_data = app_data;
}

void listbox_instance_set_on_go_back(struct listbox_instance *li, listbox_instance_on_go_back func , void *app_data)
{
	LISTBOX_VERIFY_MAGIC(li);
	li->on_go_back = func;
	li->on_go_back_data = app_data;
}

int listbox_instance_get_current_item(struct listbox_instance *li)
{
	LISTBOX_VERIFY_MAGIC(li);
	return li->current_item;
}

void listbox_instance_set_current_item(struct listbox_instance *li, int id)
{
	LISTBOX_VERIFY_MAGIC(li);
	li->current_item = id;
}

static DFBResult listbox_instance_draw_list_without_images(struct listbox_instance *li, int id, DFBRectangle *rect)
{
	IDirectFBSurface *surface = li->box.surface;	
	DFBResult   res;
	int tmp = id - 1;//adjust for array
	(void)surface;

	char *text = menu_data_instance_get_metadata(menu_data_instance_get_list_item(li->menu_data, id), LIST_TEXT1);
	if (text != NULL) {

		DEBUG_ERROR("item draw id=%d %s", id, text);
		lite_set_box_visible(LITE_BOX(li->button[(tmp % li->item_per_page)]), 1);
		res = lite_set_text_button_caption(li->button[(tmp % li->item_per_page)], text);
	}
	else {
		lite_set_box_visible(LITE_BOX(li->button[(tmp % li->item_per_page)]), 0);
	}

	rect->y += li->item_h + li->c2c_space;

	return DFB_OK;
}

static DFBResult listbox_instance_draw_items(struct listbox_instance *li, const DFBRegion *region)
{
	IDirectFBSurface *surface = li->box.surface;

	DFBRectangle item_rect;
	item_rect.x = li->c2b_space;
	item_rect.y = li->c2b_space;
	item_rect.w = li->item_w;
	item_rect.h = li->item_h;

	if (li->current_item > 0) {
		surface->SetColor(surface, 0xff, 0, 0, 255);
		surface->DrawRectangle(surface, LITE_BOX(li->button[ ( (li->current_item-1) % li->item_per_page) ])->rect.x-1, 
						LITE_BOX(li->button[ ( (li->current_item-1) % li->item_per_page) ])->rect.y-1, 
						LITE_BOX(li->button[ ( (li->current_item-1) % li->item_per_page) ])->rect.w+2,
						LITE_BOX(li->button[ ( (li->current_item-1) % li->item_per_page) ])->rect.h+2);
	}

	int first_id = ((li->current_page - 1) * li->item_per_page ) + 1;

	if (li->last_id == first_id) {
		return DFB_OK;
	}

	char *header_text = menu_data_instance_get_metadata(menu_data_instance_get_list_item(li->menu_data, 0), HEADER_TEXT);
	if (header_text != NULL && li->hi != NULL) {
		header_instance_set_text(li->hi, header_text); 
	}

	int id;

	for (id = first_id; id < first_id + li->item_per_page; id++) {
		listbox_instance_draw_list_without_images(li, id, &item_rect);
	}

	li->last_id = first_id;

	return DFB_OK;
}

static DFBResult listbox_instance_draw(LiteBox *box, const DFBRegion *region, DFBBoolean clear)
{
	struct listbox_instance *li = (struct listbox_instance*)(box);
	IDirectFBSurface *surface = box->surface;

	DEBUG_TRACE("listbox_instance_draw called...");

	surface->SetClip(surface, region);

	if (clear) {
		lite_clear_box(box, region);
	}

	surface->SetDrawingFlags (surface, DSDRAW_NOFX);

	if (li->item_count < 1) {
		DEBUG_WARN("No item in list...");
		return DFB_OK;
	}

	//draw items
	listbox_instance_draw_items(li, region);

	DEBUG_TRACE("listbox_instance_draw success...");

	return DFB_OK;
}

int listbox_instance_next_page(struct listbox_instance *li)
{
	int page_count = ceil((float)li->item_count / (float)li->item_per_page);

	if (page_count > li->current_page) {
#ifdef ANIMATION_ENABLED
		if (li->si) {
			lite_set_box_visible(LITE_BOX(li->si), 0);
		}
		lite_draw_box(LITE_BOX(li)->parent, NULL, 0);
#endif
		li->current_page++;
		li->current_item = ((li->current_page - 1) * li->item_per_page ) + 1;

#ifdef ANIMATION_ENABLED
		SlideAnimation *animation;
		animation = create_animation(LITE_BOX(li)->parent->surface, NULL, LITE_BOX(li)->parent->surface, LEFT2RIGHT);
#endif

#ifdef ANIMATION_ENABLED
		//lite_draw_box(LITE_BOX(list), NULL, 1);
		lite_draw_box(LITE_BOX(li)->parent, NULL, 0);
#else
		lite_update_box(LITE_BOX(li), NULL);
#endif

#ifdef ANIMATION_ENABLED
		render_to_animation_surface(animation, LITE_BOX(li)->parent->surface, 2);
	        do_animation(animation);
		destroy_animation(animation);
		if (li->si) {
			lite_set_box_visible(LITE_BOX(li->si), 1);
		}
		lite_update_box(LITE_BOX(li), NULL);
#endif
		return 1;
	}
	return 0;
}

int listbox_instance_prev_page(struct listbox_instance *li)
{
	int page_count = ceil((float)li->item_count / (float)li->item_per_page);

	if (page_count >= li->current_page && li->current_page != 1) {
#ifdef ANIMATION_ENABLED
		if (li->si) {
			lite_set_box_visible(LITE_BOX(li->si), 0);
		}
		lite_draw_box(LITE_BOX(li)->parent, NULL, 0);
#endif
		li->current_page--;
		li->current_item = ((li->current_page - 1) * li->item_per_page ) + 1;

#ifdef ANIMATION_ENABLED
		SlideAnimation *animation;
		animation = create_animation(LITE_BOX(li)->parent->surface, NULL, LITE_BOX(li)->parent->surface, RIGHT2LEFT);
#endif	
#ifdef ANIMATION_ENABLED
		//lite_draw_box(LITE_BOX(list), NULL, 1);
		lite_draw_box(LITE_BOX(li)->parent, NULL, 0);
#else
		lite_update_box(LITE_BOX(li), NULL);
#endif

#ifdef ANIMATION_ENABLED
		render_to_animation_surface(animation, LITE_BOX(li)->parent->surface, 1);
	        do_animation(animation);
		destroy_animation(animation);
		if (li->si) {
			lite_set_box_visible(LITE_BOX(li->si), 1);
		}
		lite_update_box(LITE_BOX(li), NULL);
#endif
		return 1;
	}
	return 0;
}

static void listbox_instance_go_back(struct listbox_instance *li)
{
	if (menu_data_instance_get_prev_menu(li->menu_data) != NULL) {
	
		menu_data_instance_set_focused_item(li->menu_data, li->current_item);
		
		struct menu_data_instance *tmp = menu_data_instance_get_prev_menu(li->menu_data);
	
		if (!tmp) {
			DEBUG_ERROR("Prev menu not exist for = %p", li->menu_data);
			return 1;
		}

		struct list_item *lip = menu_data_instance_get_list_item(tmp, menu_data_instance_get_focused_item(tmp));

		if (!lip) {
			DEBUG_ERROR("list item not exist for = %p", li->menu_data);
			return 1;
		}

		if (menu_data_instance_get_listitem_flag(lip) == MAY_HAVE_SUBMENU) {
			menu_data_instance_deref(li->menu_data);
			DEBUG_ERROR("Submenu destroyed = %p", li->menu_data);
			menu_data_instance_set_listitem_submenu(menu_data_instance_get_list_item(tmp, menu_data_instance_get_focused_item(tmp)), NULL);
		}

		listbox_instance_set_menu_data(li, tmp);
		li->current_item = 1;
		li->current_page = 1;

#ifdef ANIMATION_ENABLED
		SlideAnimation *animation;
		animation = create_animation(LITE_BOX(li)->parent->surface, NULL, LITE_BOX(li)->parent->surface, RIGHT2LEFT);
#endif	
#ifdef ANIMATION_ENABLED
		lite_draw_box(LITE_BOX(li)->parent, NULL, 0);
#else
		lite_update_box(LITE_BOX(li), NULL);
#endif
#ifdef ANIMATION_ENABLED
		render_to_animation_surface(animation, LITE_BOX(li)->parent->surface, 1);
	        do_animation(animation);
		destroy_animation(animation);
		lite_update_box(LITE_BOX(li), NULL);
#endif
	}
	else {
		if (li->on_go_back) {
			li->on_go_back(li, li->on_go_back_data);
		}
	}
}

static void listbox_instance_scroll_up_pressed( LiteButton *button, void *ctx )
{
	struct listbox_instance *li = (struct listbox_instance*)(ctx);
	listbox_instance_prev_page(li);
}

static void listbox_instance_scroll_down_pressed( LiteButton *button, void *ctx )
{
	struct listbox_instance *li = (struct listbox_instance*)(ctx);
	listbox_instance_next_page(li);
}

static void listbox_instance_go_back_pressed( LiteButton *button, void *ctx )
{
	struct listbox_instance *li = (struct listbox_instance*)(ctx);
	listbox_instance_go_back(li);
}

void listbox_instance_attach_scrollbar(struct listbox_instance *li, struct scroll_instance *si)
{
	li->si = si;
	lite_on_button_press( scroll_instance_get_button(si, 1), listbox_instance_scroll_up_pressed, li);
	lite_on_button_press( scroll_instance_get_button(si, 0), listbox_instance_scroll_down_pressed, li);
	lite_on_button_press( scroll_instance_get_button(si, 2), listbox_instance_go_back_pressed, li);
}

static void listbox_instance_item_select(struct listbox_instance *li)
{
	if (li->on_item_select) {
		li->on_item_select(li, li->on_item_select_data);
	}

	struct menu_data_instance *mdi = NULL;
	struct list_item *lip = NULL;

	lip = menu_data_instance_get_list_item(li->menu_data, li->current_item);

	if (lip) {
		mdi = menu_data_instance_get_listitem_submenu(lip);
	} 			

	if ((void*)mdi == NULL) {
		menu_data_instance_callback_t callback = (menu_data_instance_callback_t)menu_data_instance_get_listitem_callback(lip);
		if( callback != NULL) {
			void *callback_data = menu_data_instance_get_listitem_callbackdata(lip);
			callback(li->menu_data, li->current_item, callback_data);
		}
	}

	// for to understand late binding happened or not
	if (lip) {
		mdi = menu_data_instance_get_listitem_submenu(lip);
	} 

	if ((void*)mdi != NULL) {

		menu_data_instance_set_focused_item(li->menu_data, li->current_item);

#ifdef ANIMATION_ENABLED
		SlideAnimation *animation;
		animation = create_animation(LITE_BOX(li)->parent->surface, NULL, LITE_BOX(li)->parent->surface, LEFT2RIGHT);
#endif	
		listbox_instance_set_menu_data(li, mdi);
		
		li->current_item = 1;
		li->current_page = 1;

#ifdef ANIMATION_ENABLED
		lite_draw_box(LITE_BOX(li)->parent, NULL, 0);
#else
		lite_update_box(LITE_BOX(li), NULL);
#endif
#ifdef ANIMATION_ENABLED
		render_to_animation_surface(animation, LITE_BOX(li)->parent->surface, 2);
	        do_animation(animation);
		destroy_animation(animation);
		lite_update_box(LITE_BOX(li), NULL);
#endif
	}

}

static void button_pressed( LiteButton *button, void *ctx )
{
	struct listbox_instance *li = (struct listbox_instance*)(ctx);

	int i;
	for ( i = 0 ; i < li->item_per_page; i++ ) {
		if (li->button[i] == button ) {
			break;
		}
	}
	DEBUG_ERROR("%d. list button pressed ", i);

	li->current_item = ((li->current_page-1) * li->item_per_page) + i + 1;

	listbox_instance_item_select(li);
	//use int i for to trigger action with 
}

static int on_enter(LiteBox *box, int x, int y)
{
     D_ASSERT(box != NULL);
     LITE_BOX_TYPE_PARAMETER_CHECK(LITE_BOX(box), LITE_TYPE_LIST);

     return 1;
}

static int on_leave(LiteBox *box, int x, int y)
{
     D_ASSERT(box != NULL);
     LITE_BOX_TYPE_PARAMETER_CHECK(LITE_BOX(box), LITE_TYPE_LIST);
     
     return 1;
}

static int on_focus_in(LiteBox *box)
{
     D_ASSERT(box != NULL);
     LITE_BOX_TYPE_PARAMETER_CHECK(LITE_BOX(box), LITE_TYPE_LIST);

     lite_update_box(box, NULL);

     return 1;
}

static int on_focus_out(LiteBox *box)
{
     D_ASSERT(box != NULL);
     LITE_BOX_TYPE_PARAMETER_CHECK(LITE_BOX(box), LITE_TYPE_LIST);

     lite_update_box(box, NULL);

     return 1;
}

static int on_button_down(LiteBox                       *box, 
                 int                            x, 
                 int                            y,
                 DFBInputDeviceButtonIdentifier button)
{
	D_ASSERT(box != NULL);
	LITE_BOX_TYPE_PARAMETER_CHECK(LITE_BOX(box), LITE_TYPE_LIST);
	DEBUG_ERROR("on_button_down x = %d  y = %d \n", x, y);
	return 1;
}

static int on_button_up(LiteBox                       *box, 
             int                            x, 
             int                            y,
             DFBInputDeviceButtonIdentifier button)
{
	D_ASSERT(box != NULL);
	LITE_BOX_TYPE_PARAMETER_CHECK(LITE_BOX(box), LITE_TYPE_LIST);
	DEBUG_ERROR("on_button_up x = %d  y = %d \n", x, y);
	return 1;
}

static int on_motion(LiteBox                 *box, 
          int                      x, 
          int                      y,
          DFBInputDeviceButtonMask buttons)
{
	D_ASSERT(box != NULL);
	LITE_BOX_TYPE_PARAMETER_CHECK(LITE_BOX(box), LITE_TYPE_LIST);

	DEBUG_ERROR("on_motion x = %d  y = %d \n", x, y);
	return 1;
}

static int on_key_down(LiteBox *box, DFBWindowEvent *evt)
{
	struct listbox_instance *li = (struct listbox_instance*)(box);
	DEBUG_ERROR("on_key_down() called. key_symbol : %d\n", evt->key_symbol);

	if (li->item_count < 1) {
        	return 1;
	}

	int first_id = ((li->current_page - 1) * li->item_per_page ) + 1;

	if (evt->key_symbol == 61441) { //right
		listbox_instance_next_page(li);
	}

	if (evt->key_symbol == 61440) { //left
		listbox_instance_prev_page(li);
	}

	if (evt->key_symbol == 61442) { //up
		if ( li->current_item > first_id) {
			li->current_item--;
			lite_update_box(LITE_BOX(li), NULL);
		}
	}

	if (evt->key_symbol == 61443) { //down
		int item_count_tmp = (li->current_page * li->item_per_page > li->item_count ?  
					li->item_count % li->item_per_page : li->item_per_page);//item count for current page

		if ( li->current_item < first_id + item_count_tmp -1) {
			li->current_item++;
			lite_update_box(LITE_BOX(li), NULL);
		}
	}

	if (evt->key_symbol == 13) { //enter
		listbox_instance_item_select(li);
	}

	if (evt->key_symbol == 27) { //back
		listbox_instance_go_back(li);
	}

	return 1;
}

DFBResult listbox_instance_alloc(LiteBox *parent, DFBRectangle *rect, struct listbox_instance **ret_li)
{
	DFBResult   res = DFB_FAILURE;

	struct listbox_instance *li = (struct listbox_instance*) calloc(1, sizeof(struct listbox_instance));
	if (!li) {
		DEBUG_ERROR("struct listbox_instance instance  create failed...");
		goto error;
	}

	li->refs = 1;	

#ifdef DEBUG
	li->magic = LISTBOX_MAGIC ;
#endif

	li->box.parent = parent;
	li->box.rect   = *rect;
	
	res = lite_init_box(LITE_BOX(li));
	if (res != DFB_OK) {
		DEBUG_ERROR("Lite box for menu instance init failed...");
		goto error;
	}

	li->box.type    = 0x5000;
	li->box.Draw    = listbox_instance_draw;
	li->box.Destroy = listbox_instance_box_destroy;

	li->box.OnEnter      = on_enter;
	li->box.OnLeave      = on_leave;
	li->box.OnFocusIn    = on_focus_in;
	li->box.OnFocusOut   = on_focus_out;
	li->box.OnButtonDown = on_button_down;
	li->box.OnButtonUp   = on_button_up;
	li->box.OnMotion     = on_motion;
	li->box.OnKeyDown    = on_key_down;

	li->last_id = 0;
	li->item_count = 0;
	li->current_item = -1;
	li->current_page = 1;

	li->c2c_space = C2C_SPACE;// pixels between two components
	li->c2b_space = C2B_SPACE;// pixels between component and box edge

	li->item_h = ITEM_HEIGHT;
	li->item_w = (rect->w - (2 * C2B_SPACE) );
	li->item_per_page = ITEM_PER_PAGE;

	DEBUG_ERROR("item_h = %d  item_w = %d",li->item_h, li->item_w);

	res = lite_new_text_button_theme(
		"/share/LiTE/textbuttonbgnd.png",
		&li->textButtonTheme);

	if (res != DFB_OK) {
		DEBUG_ERROR("menu button theme init failed...");
		goto error;
	}

	DFBRectangle item_rect;
	item_rect.x = li->c2b_space;
	item_rect.y = li->c2b_space;
	item_rect.w = li->item_w;
	item_rect.h = li->item_h;

	int i;
	for (i = 0; i < li->item_per_page; i++) {
		res = lite_new_text_button(LITE_BOX(li), &item_rect, "", li->textButtonTheme, &li->button[i]);
		lite_set_box_visible(LITE_BOX(li->button[i]), 0);
		lite_on_text_button_press(li->button[i], button_pressed, li);
		if (res != DFB_OK) {
			DEBUG_ERROR("menu button create failed...");
			goto error;
		}
		item_rect.y += li->item_h + li->c2c_space;
	}

	lite_update_box(LITE_BOX(li), NULL);
	
	*ret_li = li;
	return DFB_OK;
error:
	//free unneeded values here
	if (li != NULL) {
		listbox_instance_destroy( li);
	}
	return res;
}

