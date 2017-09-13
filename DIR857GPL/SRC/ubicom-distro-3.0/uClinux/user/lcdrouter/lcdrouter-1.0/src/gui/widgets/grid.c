/*
 *  Grid Menu GUI Component
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

#include <gui/widgets/grid.h>

struct grid_instance {
	LiteBox box;//this must be in first place
	struct menu_data_instance *menu_data;

	LiteButton *button[ITEM_PER_PAGE_FOR_GRID];

	struct scroll_instance *si;
	struct header_instance *hi;

	grid_instance_on_item_select on_item_select;
	void *on_item_select_data;
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
#define GRID_MAGIC 0xBABE
#define GRID_VERIFY_MAGIC(gi) grid_verify_magic(gi)

void grid_verify_magic(struct grid_instance *gi)
{
	DEBUG_ASSERT(gi->magic == GRID_MAGIC, "%p: bad magic", gi);
}
#else
#define GRID_VERIFY_MAGIC(gi)
#endif

static void grid_instance_destroy(struct grid_instance *gi)
{
	DEBUG_ASSERT(gi == NULL, " grid destroy with NULL instance ");
	//free other internal members
#ifdef DEBUG
	gi->magic = 0;
#endif
	free(gi);	
	gi = NULL;
}

int  grid_instance_ref(struct grid_instance *gi)
{
	GRID_VERIFY_MAGIC(gi);
	//needs pthread llock
	
	return ++gi->refs;
}

int  grid_instance_deref(struct grid_instance *gi)
{
	GRID_VERIFY_MAGIC(gi);
	DEBUG_ASSERT(gi->refs, "%p: grid has no references", gi);
	//needs pthread llock
	if ( --gi->refs == 0 ) {
		grid_instance_destroy(gi);
		return 0;
	}
	else {
		return gi->refs;
	}
}

static DFBResult grid_instance_box_destroy(LiteBox *box)
{
	struct grid_instance *gi = (struct grid_instance*)(box);
	//do more here.
	(void)gi;

	return lite_destroy_box(box);
}

void grid_instance_set_header_instance(struct grid_instance *gi, struct header_instance *hi)
{
	GRID_VERIFY_MAGIC(gi);
	gi->hi = hi;
}

void grid_instance_set_menu_data(struct grid_instance *gi, struct menu_data_instance *menu_data)
{
	GRID_VERIFY_MAGIC(gi);
	gi->menu_data = menu_data;

	gi->item_count = menu_data_instance_get_list_item_count(gi->menu_data) - 1;

	if ( gi->item_count > 0 ) {
		gi->current_item = 1;
		gi->last_id = -1;
		DEBUG_ERROR("menu_data added to menu instance with %d items...", gi->item_count);
		lite_update_box(LITE_BOX(gi), NULL);
	}
}

struct menu_data_instance* grid_instance_get_menu_data(struct grid_instance *gi)
{
	GRID_VERIFY_MAGIC(gi);
	return gi->menu_data;
}

void grid_instance_set_on_item_select(struct grid_instance *gi, grid_instance_on_item_select func , void *app_data)
{
	GRID_VERIFY_MAGIC(gi);
	gi->on_item_select = func;
	gi->on_item_select_data = app_data;
}

int grid_instance_get_current_item(struct grid_instance *gi)
{
	GRID_VERIFY_MAGIC(gi);
	return gi->current_item;
}

static DFBResult grid_instance_draw_grid(struct grid_instance *gi, int id, DFBRectangle *rect)
{
	IDirectFBSurface *surface = gi->box.surface;
	//DFBResult  res;
	int tmp = id - 1;//adjust for array
	(void)surface;

	char *text = menu_data_instance_get_metadata(menu_data_instance_get_list_item(gi->menu_data, id), LIST_TEXT1);
	if( text != NULL) {
		DEBUG_ERROR("item draw id=%d %s", id, text);

		char str[255];
		sprintf(str, "/share/LiTE/lcdrouter/theme/sky/menu/%s.png", text);
		lite_set_box_visible(LITE_BOX(gi->button[( tmp % gi->item_per_page)]), 1);
		lite_set_button_image( gi->button[ ( tmp % gi->item_per_page) ], LITE_BS_NORMAL, str );
		lite_set_button_image( gi->button[ ( tmp % gi->item_per_page) ], LITE_BS_PRESSED, str );
		lite_set_button_image( gi->button[ ( tmp % gi->item_per_page) ], LITE_BS_HILITE, str );
	}
	else {
		lite_set_box_visible(LITE_BOX(gi->button[( tmp % gi->item_per_page)]), 0);
	}

	return DFB_OK;
}

static DFBResult grid_instance_draw_items(struct grid_instance *gi, const DFBRegion *region)
{
	IDirectFBSurface *surface = gi->box.surface;

	DFBRectangle item_rect;
	item_rect.x = gi->c2b_space;
	item_rect.y = gi->c2b_space;
	item_rect.w = gi->item_w;
	item_rect.h = gi->item_h;

	if (gi->current_item > 0) {
		surface->SetColor(surface, 0xff, 0, 0, 255);
		surface->DrawRectangle(surface, LITE_BOX(gi->button[ ( (gi->current_item-1) % gi->item_per_page) ])->rect.x+1,
						LITE_BOX(gi->button[ ( (gi->current_item-1) % gi->item_per_page) ])->rect.y+1,
						LITE_BOX(gi->button[ ( (gi->current_item-1) % gi->item_per_page) ])->rect.w-2,
						LITE_BOX(gi->button[ ( (gi->current_item-1) % gi->item_per_page) ])->rect.h-2);
	}

	int first_id = ((gi->current_page - 1) * gi->item_per_page ) + 1;

	if( gi->last_id == first_id ){
		return DFB_OK;
	}

	char *header_text = menu_data_instance_get_metadata(menu_data_instance_get_list_item(gi->menu_data, 0), HEADER_TEXT);
	if( header_text != NULL && gi->hi != NULL) {
		header_instance_set_text(gi->hi, header_text); 
	}

	int id;

	for ( id = first_id ; id < first_id + gi->item_per_page ; id++ ) {
		grid_instance_draw_grid(gi, id, &item_rect);
	}
	
	gi->last_id = first_id;

	return DFB_OK;
}

static DFBResult grid_instance_draw(LiteBox *box, const DFBRegion *region, DFBBoolean clear)
{
	struct grid_instance *gi = (struct grid_instance*)(box);
	IDirectFBSurface *surface = box->surface;

	DEBUG_TRACE("grid_instance_draw called...");

	surface->SetClip(surface, region);

	if ( clear ) {
		lite_clear_box(box, region);
	}

	surface->SetDrawingFlags (surface, DSDRAW_NOFX);

	if( gi->item_count < 1 ) {
		DEBUG_WARN("No item in list...");
		return DFB_OK;
	}

	//draw items
	grid_instance_draw_items(gi, region);

	DEBUG_TRACE("grid_instance_draw success...");

	return DFB_OK;
}

int grid_instance_next_page(struct grid_instance *gi)
{
	int page_count = ceil((float)gi->item_count / (float)gi->item_per_page);

	if ( page_count > gi->current_page) {
#ifdef ANIMATION_ENABLED
		if (gi->si) {
			lite_set_box_visible(LITE_BOX(gi->si), 0);
		}
		lite_draw_box(LITE_BOX(gi)->parent, NULL, 0);
#endif
		gi->current_page++;
		gi->current_item = ((gi->current_page - 1) * gi->item_per_page ) + 1;

#ifdef ANIMATION_ENABLED
		SlideAnimation *animation;
		animation = create_animation(LITE_BOX(gi)->parent->surface, NULL, LITE_BOX(gi)->parent->surface, LEFT2RIGHT);
#endif

#ifdef ANIMATION_ENABLED
		//lite_draw_box(LITE_BOX(list), NULL, 1);
		lite_draw_box(LITE_BOX(gi)->parent, NULL, 0);
#else
		lite_update_box(LITE_BOX(gi), NULL);
#endif

#ifdef ANIMATION_ENABLED
		render_to_animation_surface(animation, LITE_BOX(gi)->parent->surface, 2);
	        do_animation(animation);
		destroy_animation(animation);
		if (gi->si) {
			lite_set_box_visible(LITE_BOX(gi->si), 1);
		}
		lite_update_box(LITE_BOX(gi), NULL);
#endif
		return 1;
	}
	return 0;
}

int grid_instance_prev_page(struct grid_instance *gi)
{
	int page_count = ceil((float)gi->item_count / (float)gi->item_per_page);

	if ( page_count >= gi->current_page && gi->current_page != 1  ) {
#ifdef ANIMATION_ENABLED
		if (gi->si) {
			lite_set_box_visible(LITE_BOX(gi->si), 0);
		}
		lite_draw_box(LITE_BOX(gi)->parent, NULL, 0);
#endif
		gi->current_page--;
		gi->current_item = ((gi->current_page - 1) * gi->item_per_page ) + 1;

#ifdef ANIMATION_ENABLED
		SlideAnimation *animation;
		animation = create_animation(LITE_BOX(gi)->parent->surface, NULL, LITE_BOX(gi)->parent->surface, RIGHT2LEFT);
#endif	
#ifdef ANIMATION_ENABLED
		//lite_draw_box(LITE_BOX(list), NULL, 1);
		lite_draw_box(LITE_BOX(gi)->parent, NULL, 0);
#else
		lite_update_box(LITE_BOX(gi), NULL);
#endif

#ifdef ANIMATION_ENABLED
		render_to_animation_surface(animation, LITE_BOX(gi)->parent->surface, 1);
	        do_animation(animation);
		destroy_animation(animation);
		if (gi->si) {
			lite_set_box_visible(LITE_BOX(gi->si), 1);
		}
		lite_update_box(LITE_BOX(gi), NULL);
#endif
		return 1;
	}
	return 0;
}

static void grid_instance_scroll_up_pressed( LiteButton *button, void *ctx )
{
	struct grid_instance *gi = (struct grid_instance*)(ctx);
	grid_instance_prev_page( gi );
}

static void grid_instance_scroll_down_pressed( LiteButton *button, void *ctx )
{
	struct grid_instance *gi = (struct grid_instance*)(ctx);
	grid_instance_next_page( gi );
}

void grid_instance_attach_scrollbar(struct grid_instance *gi, struct scroll_instance *si)
{
	gi->si = si;
	lite_on_button_press( scroll_instance_get_button(si, 1), grid_instance_scroll_up_pressed, gi);
	lite_on_button_press( scroll_instance_get_button(si, 0), grid_instance_scroll_down_pressed, gi);
}

static void button_pressed( LiteButton *button, void *ctx )
{
	struct grid_instance *gi = (struct grid_instance*)(ctx);

	int i;
	for ( i = 0 ; i < gi->item_per_page; i++ ) {
		if (gi->button[i] == button ) {
			break;
		}
	}
	DEBUG_ERROR("%d. grid button pressed ", i);

	gi->current_item = ((gi->current_page-1) * gi->item_per_page) + i + 1;

	if(gi->on_item_select) {
		gi->on_item_select(gi, gi->on_item_select_data);
	}
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
	struct grid_instance *gi = (struct grid_instance*)(box);
	DEBUG_ERROR("on_key_down() called. key_symbol : %d\n", evt->key_symbol);

	if ( gi->item_count < 1 ) {
        	return 1;
	}

	int first_id = ((gi->current_page - 1) * gi->item_per_page ) + 1;

	if ( evt->key_symbol == 61441 ) { //right
		grid_instance_next_page( gi );
	}

	if ( evt->key_symbol == 61440 ) { //left
		grid_instance_prev_page( gi );
	}

	if ( evt->key_symbol == 61442 ) { //up
		if ( gi->current_item > first_id) {
			gi->current_item--;
			lite_update_box(LITE_BOX(gi), NULL);
		}
	}

	if ( evt->key_symbol == 61443 ) { //down
		int item_count_tmp = (gi->current_page * gi->item_per_page > gi->item_count ?  
					gi->item_count % gi->item_per_page : gi->item_per_page);//item count for current page

		if ( gi->current_item < first_id + item_count_tmp -1) {
			gi->current_item++;
			lite_update_box(LITE_BOX(gi), NULL);
		}
	}

	if ( evt->key_symbol == 13 ) {
		if(gi->on_item_select) {
			gi->on_item_select(gi, gi->on_item_select_data);
		}
	}

	if ( evt->key_symbol == 27) { //back
		//for now for grid we don't need.		
	}

	return 1;
}

DFBResult grid_instance_alloc(LiteBox *parent, DFBRectangle *rect, struct grid_instance **ret_gi)
{
	DFBResult   res = DFB_FAILURE;

	struct grid_instance *gi = (struct grid_instance*) calloc(1, sizeof(struct grid_instance));
	if ( !gi ) {
		DEBUG_ERROR("struct grid_instance instance  create failed...");
		goto error;
	}

	gi->refs = 1;	

#ifdef DEBUG
	gi->magic = GRID_MAGIC ;
#endif

	gi->box.parent = parent;
	gi->box.rect   = *rect;
	
	res = lite_init_box(LITE_BOX(gi));
	if (res != DFB_OK) {
		DEBUG_ERROR("Lite box for menu instance init failed...");
		goto error;
	}

	gi->box.type    = 0x5500;
	gi->box.Draw    = grid_instance_draw;
	gi->box.Destroy = grid_instance_box_destroy;

	gi->box.OnEnter      = on_enter;
	gi->box.OnLeave      = on_leave;
	gi->box.OnFocusIn    = on_focus_in;
	gi->box.OnFocusOut   = on_focus_out;
	gi->box.OnButtonDown = on_button_down;
	gi->box.OnButtonUp   = on_button_up;
	gi->box.OnMotion     = on_motion;
	gi->box.OnKeyDown    = on_key_down;

	gi->item_count = 0;
	gi->current_item = -1;
	gi->current_page = 1;
	gi->last_id = -1;

	gi->c2c_space = GRID_C2C_SPACE;// pixels between two components
	gi->c2b_space = GRID_C2B_SPACE;// pixels between component and box edge

	gi->item_h = ITEM_HEIGHT_FOR_GRID;
	gi->item_w = ITEM_WIDTH_FOR_GRID;
	gi->item_per_page = ITEM_PER_PAGE_FOR_GRID;
	
	DEBUG_ERROR("item_h = %d  item_w = %d",gi->item_h, gi->item_w);

	DFBRectangle item_rect;
	item_rect.x = gi->c2b_space;
	item_rect.y = gi->c2b_space;
	item_rect.w = gi->item_w;
	item_rect.h = gi->item_h;

	int i;
	int col_num = ( gi->item_per_page / 2);
	for ( i = 0 ; i < gi->item_per_page ; i++) {
		DEBUG_TRACE("col=%d i=%d x=%d y=%d w=%d h=%d",col_num, i, item_rect.x , item_rect.y, item_rect.w, item_rect.h);
		res = lite_new_button(LITE_BOX(gi), &item_rect, NULL, &gi->button[i]);
		lite_set_box_visible(LITE_BOX(gi->button[i]), 0);
		lite_on_button_press( gi->button[i], button_pressed, gi);
	
		int x_step = gi->item_w + gi->c2c_space ;

		if ( ((i+1) % col_num) == 0 ) {
			item_rect.x -= col_num  * (x_step);
			item_rect.y += gi->item_h + gi->c2c_space;
		}

		item_rect.x += x_step;
	}

	lite_update_box(LITE_BOX(gi), NULL);
	
	*ret_gi = gi;
	return DFB_OK;
error:
	//free unneeded values here
	if ( gi != NULL) {
		grid_instance_destroy( gi);
	}
	return res;
}

