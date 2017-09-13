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
#ifndef ANIMATION_H
#define ANIMATION_H
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

#include <lite/lite.h>
#include <lite/box.h>
#include <lite/window.h>
#include <lite/util.h>

#include <string.h>

#include <debug.h>

enum {
	LEFT2RIGHT,
	RIGHT2LEFT
};

struct slide_animation {
	int	direction;
	int	speed;
	int 	step;
	int	running;
	IDirectFBSurface  *surface1;
	IDirectFBSurface  *surface2;
	IDirectFBSurface  *dest;
	IDirectFBSurface  *animation_surface;
};
typedef struct slide_animation SlideAnimation;

void do_animation(SlideAnimation* animation);
void render_to_animation_surface(SlideAnimation* animation, IDirectFBSurface  *source, int side);
SlideAnimation* create_animation(IDirectFBSurface  *dest, IDirectFBSurface  *source1, IDirectFBSurface  *source2, int direction);
void destroy_animation(SlideAnimation* animation);

#endif

