#ifndef NO_OS
#include "av/gxav_vpu_propertytypes.h"
#include "gui.h"
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

#else /*NO_OS*/

#include "mini_gui_private.h"
#include "gal.h"
#include "hd_gal.h"
#include "gui_core.h"

extern GuiCore gui;

#endif /*NO_OS*/


int hd_char(GuiSurface *surface, char *pData, int x, int y, int xsize, int ysize, int byteperline, int color)
{
	GuiCore *gui_core = NULL;
	GxVpuProperty_Blit Blit;
	//GxVpuProperty_ColorKey ColorKey = { 0 };
	GxPalette palette = {0};
	GxColor entries[2];
	int ret = 0, tran_color = 0;
	GuiSurface *src_surface = NULL;
	GAL_Rect rect = {0};
	GxColorFormat color_format = surface->sf.color_format;

	if(pData == NULL ||  surface == NULL)
		return (1);

	gui_core = GUI_GetCurrent();
	surface = hd_get_idle_surface(surface);

	if(x < 0)
	    x = 0;
	if(y < 0)
	    y = 0;
	if((y + ysize) > surface->sf.height)
	{
	    ysize = surface->sf.height - y;
	}

	rect.x = rect.y = 0;
	rect.w = byteperline * 8;
	rect.h = ysize;

	src_surface = hd_get_surface(&rect, 1, NULL, 0);
	if(NULL == src_surface)
	{
		gxlogd("[GUI]Create char surface failed!\n");
		return (1);
	}

	memcpy(src_surface->data, pData, ysize * byteperline);

	//ret = 0;
	tran_color = gal_color2index(surface, gui.config.gui_trans);
	if(8 == gui_core->config.bpp) {
		entries[0].entry = tran_color;//ret;
		entries[0].a = 0;//(gui.config.gui_trans & 0x000F) << 4;
		entries[1].a = 0;// (color & 0x000F) << 4;
		entries[1].entry = color;
	}
	else{
		hd_color2formatcolor(surface, tran_color, &(entries[0]));
		hd_color2formatcolor(surface, color, &(entries[1]));
	}

	palette.num_entries = 2;
	palette.entries = entries;
	memset(&Blit, 0, sizeof(GxVpuProperty_Blit));

	//Blit.has_clut_conversion = 1;
	Blit.srca.cct = (void *)(&palette);
	Blit.srca.is_kcct = GUI_FALSE;
	Blit.srca.surface = src_surface->hw_surface;
	Blit.srca.is_ksurface              = is_ksurface(src_surface);
	Blit.srca.rect.x                   = 0;
	Blit.srca.rect.y                   = 0;
	Blit.srca.rect.width               = byteperline * 8/*gui.config.bpp*/;
	Blit.srca.rect.height              = ysize;
	//Blit.srca.alpha                  = 0;
	Blit.srca.dst_format               = color_format;//get_color_format(0, gui.config.bpp);

	Blit.srcb.surface                  = surface->hw_surface;
	Blit.srcb.is_ksurface              = is_ksurface(surface);
	Blit.srcb.rect                     = Blit.srca.rect;
	Blit.srcb.rect.x                   = x;
	Blit.srcb.rect.y                   = y;

	Blit.dst                           = Blit.srcb;
	Blit.mode                          = GX_ALU_ROP_COPY;//GX_ALU_MIX_B_TOP_A;
	Blit.colorkey_info.src_colorkey_en = GUI_TRUE;
	Blit.colorkey_info.src_colorkey    = 0;
	Blit.colorkey_info.mode            = GX_BLIT_COLORKEY_BASIC_MODE;

	ret = GxAVSetProperty(g_device_handle, vpu_handle,
			GxVpuPropertyID_Blit, &Blit, sizeof(GxVpuProperty_Blit));

	if(ret != 0) {
		hd_free_surface(src_surface);
		gxlogd("[GUI]Char Blit failed!\n");
		return (1);
	}

	hd_add_blit_element(src_surface);

	return (ret);
}


