#include "gui_core.h"
#include "av/avapi.h"
#include "av/gxav.h"
#include "av/gxav_vpu_propertytypes.h"
#ifndef NO_OS
#include "gui_private.h"

#ifdef LINUX_OS
#include <unistd.h> //for mmap

#if (_FILE_OFFSET_BITS==64)
#undef __USE_FILE_OFFSET64
#include <sys/mman.h> //for mmap

#define __USE_FILE_OFFSET64

#else
#include <sys/mman.h> //for mmap
#endif

#endif

int hd_set_alpha(GuiSurface *surface, int alpha)
{
    GxVpuProperty_Alpha Alpha;

    Alpha.surface = surface->hw_surface;
    if(16 == gui.config.bpp)
    {
		Alpha.alpha.type = GX_ALPHA_GLOBAL;
		Alpha.alpha.value = (alpha * 0x7F) / 0xFF;
    }
    else
    {
		Alpha.alpha.type = GX_ALPHA_PIXEL;
		Alpha.alpha.value = alpha;
    }

    return GxAVSetProperty(g_device_handle,
                           vpu_handle,
                           GxVpuPropertyID_Alpha,
                           &Alpha,
                           sizeof(GxVpuProperty_Alpha));
}
#else
#include "mini_gui_private.h"
extern GuiCore gui;
#endif /*NO_OS*/
static int _add_pal_node(GuiClut *clut_list, void *pal)
{
	GuiClut *node = NULL;

	if((NULL == clut_list) || (NULL == pal))
	{
		return (1);
	}

	node = clut_list;

	while(node)
	{
		if(NULL == node->create_pal)
		{
			break;
		}
		node = node->next;
	}

	if(NULL == node)
	{
		return (1);
	}

	if(NULL == node->create_pal)
	{
		node->create_pal = pal;
	}

	return (0);
}

void *g_back_palette = NULL;
static void *g_palette = NULL;

void *hd_get_clut(void)
{
    return (g_palette);
}

int hd_set_clut(GuiSurface *surface, Color * colors, int firstcolor, int ncolors)
{
	GxVpuProperty_CreatePalette CreatePalette = {0};
	GxColor entries[256];
	GxPalette  palette = {0};
	GxVpuProperty_SurfaceBindPalette SurfaceBindPalette = {0};
	GxVpuProperty_RWPalette RWPalette = {0};
	GxVpuProperty_DestroyPalette DestroyPalette = {0};
	int i = 0, ret = 0;
	int value = 0;
	char key[10] = {0};

	if(NULL == surface || NULL == colors)
	{
		return (1);
	}

	if(8 != surface->bpp)
	{
		return (0);
	}

	if(g_back_palette)
	{
		DestroyPalette.palette = g_back_palette;
		ret = GxAVSetProperty(g_device_handle,
				vpu_handle,
				GxVpuPropertyID_DestroyPalette,
				&DestroyPalette,
				sizeof(GxVpuProperty_DestroyPalette));
	}

	CreatePalette.num_entries = ncolors;
	CreatePalette.palette_num = 1;
	ret = GxAVGetProperty(g_device_handle,
			vpu_handle,
			GxVpuPropertyID_CreatePalette,
			&CreatePalette,
			sizeof(GxVpuProperty_CreatePalette));

	g_back_palette = CreatePalette.palette;

	//hd_enable_osd(GUI_FALSE);

	for (i = firstcolor; i < ncolors; i++) {
		entries[i].b = colors[i]&0xff;
		entries[i].g = (colors[i]&0xff00)>>8;
		entries[i].r = (colors[i]&0xff0000)>>16;

		if((colors[i]&0xffffff) == gui.config.osd_trans)//osd_transparent alpha 0
		{
			entries[i].a = 0;
		}
		else if(0x00000000 == (colors[i]&0xff000000))//global alpha
		{
			sprintf(key, "%d", colors[i]&0x00FFFFFF);
			if(gui.config.color_hash)
			{
				value = (int)hash_get(gui.config.color_hash, key);
				if(value)
				{
					entries[i].a = (char)value;
				}
				else
				{
					entries[i].a = (char)gui.config.osd_alpha;
				}
				value = 0;
			}
			else
			{
				entries[i].a = (char)gui.config.osd_alpha;
			}
		}
		else//color self alpha
		{
			entries[i].a = (colors[i]&0xff000000)>>24;
		}
	}

	if(CreatePalette.palette)
	{
		RWPalette.k_palette = CreatePalette.palette;
		RWPalette.palette_id = 0;
		palette.entries = entries;
		palette.num_entries = ncolors;
		RWPalette.u_palette = &palette;
	}

	ret = GxAVSetProperty(g_device_handle,
			vpu_handle,
			GxVpuPropertyID_RWPalette,
			&RWPalette,
			sizeof(GxVpuProperty_RWPalette));

	SurfaceBindPalette.surface = surface->hw_surface;
	SurfaceBindPalette.palette = CreatePalette.palette;
	g_palette = CreatePalette.palette;
	ret = GxAVSetProperty(g_device_handle,
			vpu_handle,
			GxVpuPropertyID_SurfaceBindPalette,
			&SurfaceBindPalette,
			sizeof(GxVpuProperty_SurfaceBindPalette));
	//hd_enable_osd(GUI_TRUE);

	/*Add create palette node to record for destroy in quit GUI*/
	_add_pal_node(gui.config.firstclut, (void *)(CreatePalette.palette));

	return (ret);
}

