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

#include <gui/widgets/statusbar.h>

struct statusbar_instance {
	IDirectFBSurface *icon[3];

	int refs;

#ifdef DEBUG
	int	magic;
#endif
};

#if defined(DEBUG)
#define STATUSBAR_MAGIC 0xCCBE
#define STATUSBAR_VERIFY_MAGIC(sbi) statusbar_verify_magic(sbi)

void statusbar_verify_magic(struct statusbar_instance *sbi)
{
	DEBUG_ASSERT(sbi->magic == STATUSBAR_MAGIC, "%p: bad magic", sbi);
}
#else
#define STATUSBAR_VERIFY_MAGIC(hi)
#endif

static void statusbar_instance_destroy(struct statusbar_instance *sbi)
{
	DEBUG_ASSERT(sbi == NULL, " statusbar destroy with NULL instance ");
	//free other internal members
#ifdef DEBUG
	sbi->magic = 0;
#endif

	if (sbi->icon[0]) {
		sbi->icon[0]->Release( sbi->icon[0] );
		sbi->icon[0] = NULL;
	}
	if (sbi->icon[1]) {
		sbi->icon[1]->Release( sbi->icon[1] );
		sbi->icon[1] = NULL;
	}
	if (sbi->icon[2]) {
		sbi->icon[2]->Release( sbi->icon[2] );
		sbi->icon[2] = NULL;
	}

	free(sbi);	
	sbi = NULL;
}

int  statusbar_instance_ref(struct statusbar_instance *sbi)
{
	STATUSBAR_VERIFY_MAGIC(sbi);
	
	return ++sbi->refs;
}

int  statusbar_instance_deref(struct statusbar_instance *sbi)
{
	STATUSBAR_VERIFY_MAGIC(sbi);
	DEBUG_ASSERT(sbi->refs, "%p: statusbar has no references", sbi);

	if ( --sbi->refs == 0 ) {
		statusbar_instance_destroy(sbi);
		return 0;
	}
	else {
		return sbi->refs;
	}
}

DFBResult statusbar_instance_set_icon(struct statusbar_instance *sbi, char *filename, int id)
{
	STATUSBAR_VERIFY_MAGIC(sbi);

	DFBResult ret;
	IDirectFB               *dfb;
	IDirectFBImageProvider *provider = NULL;

	dfb = lite_get_dfb_interface();

	ret = dfb->CreateImageProvider(dfb, filename, &provider);

	if (ret) {
		DEBUG_ERROR("CreateImageProvider for '%s' failed\n", filename);
		return ret;
	}

	ret = provider->RenderTo(provider, sbi->icon[id-1], NULL);
	if (ret) {
		DEBUG_ERROR("statusbar_instance_set_icon for '%s': %s\n", filename, DirectFBErrorString(ret));
		provider->Release(provider);
		return ret;
	}

	return ret;
}

IDirectFBSurface* statusbar_instance_get_icon(struct statusbar_instance *sbi, int id)
{
	STATUSBAR_VERIFY_MAGIC(sbi);
	return sbi->icon[id-1];
}

static
IDirectFBSurface* statusbar_instance_create_surface(int w, int h)
{
	IDirectFB               *dfb;
	IDirectFBSurface        *surface;
	DFBSurfaceDescription   desc;


	dfb = lite_get_dfb_interface();

	desc.flags       = DSDESC_WIDTH | DSDESC_HEIGHT | DSDESC_PIXELFORMAT;
	desc.width       = w;
	desc.height      = h;
	desc.pixelformat = DSPF_ARGB; //FIXME

	int ret;
	ret = dfb->CreateSurface( dfb, &desc, &surface );
	if (ret) {
		DEBUG_ERROR("statusbar_instance_create_surface failed!\n" );
		return NULL;
	}
	return surface;
}

struct statusbar_instance* statusbar_instance_alloc()
{

	struct statusbar_instance *sbi = (struct statusbar_instance*) calloc(1, sizeof(struct statusbar_instance));
	if ( !sbi ) {
		DEBUG_ERROR("struct header_instance instance create failed...");
		goto error;
	}

	sbi->refs = 1;	

#ifdef DEBUG
	sbi->magic = STATUSBAR_MAGIC ;
#endif

        sbi->icon[0] = statusbar_instance_create_surface(ICON1_W, ICONS_H);
	if ( !sbi->icon[0] ) {
		DEBUG_ERROR("sbi->icon1 create failed...");
		goto error;
	}

        sbi->icon[1] = statusbar_instance_create_surface(ICON2_W, ICONS_H);
	if ( !sbi->icon[1] ) {
		DEBUG_ERROR("sbi->icon2 create failed...");
		goto error;
	}

        sbi->icon[2] = statusbar_instance_create_surface(ICON3_W, ICONS_H);
	if ( !sbi->icon[2]) {
		DEBUG_ERROR("sbi->icon3 create failed...");
		goto error;
	}

	return sbi;
error:
	if ( sbi != NULL) {
		statusbar_instance_destroy( sbi );
	}
	return NULL;
}

