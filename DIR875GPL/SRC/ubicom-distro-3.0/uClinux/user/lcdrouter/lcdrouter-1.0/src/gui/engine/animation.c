/*
 * Ubicom Network Player
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
#include <gui/engine/animation.h>

static
IDirectFBSurface* create_animation_surface(IDirectFBSurface  *source)
{
	IDirectFB               *dfb;
	IDirectFBSurface        *dest;
	DFBSurfaceDescription   desc;
	int                     width, height;

	source->GetSize(source , &width, &height);

	dfb = lite_get_dfb_interface();

	desc.flags       = DSDESC_WIDTH | DSDESC_HEIGHT | DSDESC_PIXELFORMAT;
	desc.width       = width * 2;
	desc.height      = height;
	desc.pixelformat = DSPF_ARGB; //FIXME

	int ret;
	ret = dfb->CreateSurface( dfb, &desc, &dest );
	if (ret) {
		D_DERROR( ret, "DFBTest/Blit: IDirectFB::CreateSurface() failed!\n" );
		return NULL;
	}
	return dest;
}

void render_to_animation_surface(SlideAnimation* animation, IDirectFBSurface  *source, int side)
{
	int width, height;
	DEBUG_TRACE("render to animation  dest= %p  source = %p  side=%d", animation->animation_surface, source, side);
	if(side == 1) {
		animation->animation_surface->Blit(animation->animation_surface, source, 0, 0, 0);
	}

	if(side == 2) {
		source->GetSize(source, &width, &height);
		animation->animation_surface->Blit(animation->animation_surface, source, 0, width, 0);
	}
	//source->Dump(source, "/dump","shot");
}

void do_animation(SlideAnimation* animation)
{
	DFBRectangle rect;
	int width, height;

	animation->dest->GetSize(animation->dest, &width, &height);
	DEBUG_TRACE("Do animation dest = %p  w = %d  h = %d", animation->dest, width, height);

	rect.y = 0;
	rect.w = width * 2;
	rect.h = 280;

	int i = width / animation->step;

	DEBUG_TRACE("Do animation = %p  loop = %d", animation, i);
	int move;
	if (animation->direction == LEFT2RIGHT) {
		move = 0;
	}
	else {
		move = width;
	}

	animation->running = 1;
	while( i && animation->running ) {
		//printf("animation  dest= %p  source = %p  \n", animation->dest, animation->animation_surface);
		
		if (animation->direction == LEFT2RIGHT) {
			move += animation->step;
		}
		else {
			move -= animation->step;
		}
		rect.x = move;
		//animation->dest->SetBlittingFlags(animation->dest, DSBLIT_SRC_COLORKEY);
		//animation->dest->SetSrcColorKey(animation->dest, 0xff,0xff,0xff);
		
		animation->animation_surface->SetBlittingFlags(animation->animation_surface, DSBLIT_SRC_COLORKEY);
		animation->animation_surface->SetSrcColorKey(animation->animation_surface, 0xff,0xff,0xff);
		
		animation->dest->Blit(animation->dest, animation->animation_surface, &rect, 0, 0);
		animation->dest->Flip(animation->dest, 0, 0);
		usleep( animation->speed * 5000 );
		i--;
	}
	animation->running = 0;
}

void start_animation(SlideAnimation* animation)
{
	//check assert if animation is NULL
	animation->running = 1;
}

void stop_animation(SlideAnimation* animation)
{
	//check assert if animation is NULL
	animation->running = 0;
}

#define SPEED 5
#define STEP 40 

SlideAnimation* create_animation(IDirectFBSurface  *dest, IDirectFBSurface  *source1, IDirectFBSurface  *source2, int direction)
{
	SlideAnimation *animation = malloc( sizeof(SlideAnimation) );
	if( animation == NULL) {
		DEBUG_ERROR("Animation instance malloc failed");
		return NULL; 
	}
	
	animation->dest       = dest;
	animation->surface1   = source1;
	animation->surface2   = source2;
	animation->direction  = direction;
	animation->speed      = SPEED;
	animation->step       = STEP;

	animation->animation_surface = create_animation_surface(dest);

	DEBUG_INFO("animation direction = %d", animation->direction);

	if(animation->direction == LEFT2RIGHT) {

		render_to_animation_surface(animation, source2, 1);
	}

	if(animation->direction == RIGHT2LEFT) {
		render_to_animation_surface(animation, source2, 2);
	}

	DEBUG_TRACE("Create animation = %p", animation);

	return animation;
}

void destroy_animation(SlideAnimation* animation)
{
	if (animation->animation_surface) {
		animation->animation_surface->Release( animation->animation_surface );
		animation->animation_surface = NULL;
	}
	free(animation);
}

