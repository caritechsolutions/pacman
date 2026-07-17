#ifndef NO_OS
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "gxavdev.h"
#include "gui_private.h"
#include "gui.h"
#else
#include "mini_gui_private.h"
#include "common/gxmalloc.h"
#endif /*NO_OS*/

#include "gui_private.h"
#include "av/avapi.h"
#include "av/gxav.h"
#include "av/gxav_vpu_propertytypes.h"
#include "hd_gal.h"
#include "image_cache.h"
#include "av/hal/gxav_hal_fb.h"

#ifdef LINUX_OS
#include <unistd.h> //for mmap

#if (_FILE_OFFSET_BITS == 64)
#undef __USE_FILE_OFFSET64
#include <sys/mman.h> //for mmap

#define __USE_FILE_OFFSET64
#else
#include <sys/mman.h> //for mmap
#endif

#endif


#ifndef NO_OS
GuiSurface *screen = NULL;
GuiSurface *back_screen = NULL;
GuiSurface *spp_screen = NULL;
GuiSurface *frame_screen = NULL;

int g_device_handle = 0;
int vpu_handle = 0;
int jpeg_handle = 0;
int vdec_handle = 0;

static handle_t gui_mutex = -1;

extern int g_opacity;
extern int s_pal_index;

static int hd_create_lock(void)
{
	int ret = 0;

	if(gui_mutex == -1)
	{
		ret = GxCore_MutexCreate(&gui_mutex);
	}

	return (ret);
}

static int hd_destroy_lock(void)
{
	int ret = 0;

	return (ret);
}

int hd_lock(void)
{
	int ret = 0;

	ret = GxCore_MutexLock(gui_mutex);

	return (ret);
}

int hd_unlock(void)
{
	int ret = 0;

	ret = GxCore_MutexUnlock(gui_mutex);

	return (ret);
}

int hd_osd_enable(int flag)
{
	return (hd_enable_osd(flag));
}

//#define Q_MALLOC_INIT vqm_malloc_init
//#define Q_MALLOC vqm_malloc
//#define Q_FREE vqm_free

int hd_gxavdev_init(void)
{
	int ret;

	/*------------------ Create the mutex ------------------*/
	ret = hd_create_lock();
	if(0 != ret)
		gxlogd("[GUI]Create GUI lock failed.\n");

	/*------------------Obtain VPU and JPEG module handle------------------*/
	g_device_handle = GxAvdev_CreateDevice(0);
	if(g_device_handle <= 0) {
		gxlogd("[GUI]Cannot open device, %d!\n", g_device_handle);
		return (1);
	}

	vpu_handle = GxAvdev_OpenModule(g_device_handle, GXAV_MOD_VPU, 0);
	if(0 == vpu_handle) {
		gxlogd("[GUI]Open VPU module failed!\n");
		return (1);
	}

	vdec_handle = GxAvdev_OpenModule(g_device_handle, GXAV_MOD_VIDEO_DECODE, 0);
	if(0 == vdec_handle) {
		gxlogd("[GUI]Open Video Decoder module failed!\n");
		return (1);
	}

	jpeg_handle = GxAvdev_OpenModule(g_device_handle, GXAV_MOD_JPEG_DECODER, 0);
	if(0 == jpeg_handle) {
		gxlogd("[GUI]Open JPEG module failed!\n");
		return (1);
	}

	gui.config.chip_id = GxChipDetect(g_device_handle);
	gui.config.is_align = GxFBHAL_GetSurfaceAlign();

	/*------------------------------ init frame buffer memory ----------------------*/
	fb_init(&gui.fb);

	return 0;
}

void hd_set_layer_surface(LayerSurface layer)
{
	GxAvRect viewport_rect;
	GAL_Rect dstrect = {0};
	int ret = 0, color = 0;
	GuiSurface *surface = NULL;
	GxVpuProperty_ColorKey ColorKey = { 0 };
	GxVpuProperty_Alpha Alpha;
	GxVpuProperty_RegistSurface RegisterSurface = {0};

	dstrect.x = 0;
	dstrect.y = 0;
	dstrect.w = gui.config.width;
	dstrect.h = gui.config.height;
	if(gui.config.enable_double_buffer)
		surface = back_screen;
	else
		surface = screen;
	switch(layer)
	{
	case LAYER_MAIN_SPP_SURFACE:
		if(spp_screen)
		{
			if(GxAvdev_GetLayerMainSurface(g_device_handle, vpu_handle, GX_LAYER_SPP) != spp_screen->hw_surface)
			{
				ret |= GxAvdev_SetLayerMainSurface(g_device_handle, vpu_handle, GX_LAYER_SPP, spp_screen->hw_surface);
				if(0 != ret) {
					gxlogd("[GUI]Set SPP main surface failed!\n");
					return;
				}
			}
			return;
		}

		gal_enable_img(GUI_FALSE);
		/*------------------ Create SPP YCbCr surface------------------*/
		spp_screen = hd_get_surface(&dstrect, 0, NULL, 1); // ?
		if(NULL == spp_screen)
			return;
		ret |= GxAvdev_SetLayerMainSurface(g_device_handle, vpu_handle, GX_LAYER_SPP, spp_screen->hw_surface);
		if(0 != ret) {
			gxlogd("[GUI]Set SPP main surface failed!\n");
			return;
		}
		hd_img_clear();
		viewport_rect.x = viewport_rect.y = 0;
		viewport_rect.width = gui.config.width;
		viewport_rect.height = gui.config.height;
		ret |= GxAvdev_SetLayerViewport(g_device_handle, vpu_handle, GX_LAYER_SPP, &viewport_rect);
		if(0 != ret) {
			gxlogd("Set SPP view port failed\n");
		}
		break;
	case LAYER_MAIN_OSD_SURFACE:
		if(surface)
			return;
		/*------------------ Create OSD RGB surface------------------*/
		hd_osd_enable(GUI_FALSE);
		surface = hd_get_surface(&dstrect, gui.config.bpp, NULL, 1);
		if(NULL == surface)
			return;

		if(gui.config.enable_double_buffer)
			back_screen = surface;
		else {
			gui.osd_surface = screen = surface;
		}

		ColorKey.enable = 1;
		color = gui.config.osd_trans;
		color = hd_color2index(surface, color);
		hd_color2formatcolor(surface, color, &(ColorKey.color));
		ColorKey.color.a = g_opacity;
		if(GX_COLOR_FMT_ARGB4444 == gui.config.color_format ||
			GX_COLOR_FMT_ARGB1555 == gui.config.color_format ||			
			GX_COLOR_FMT_RGBA8888 == gui.config.color_format ||
			GX_COLOR_FMT_ARGB8888 == gui.config.color_format){
			ColorKey.color.a = 0;
		}
		ColorKey.surface = surface->hw_surface;
		ret = GxAVSetProperty(g_device_handle, vpu_handle,
				GxVpuPropertyID_ColorKey, &ColorKey, sizeof(GxVpuProperty_ColorKey));

		if(0 != ret) {
			gxlogd("[GUI]Set OSD Color Key failed!\n");
			return;
		}

		Alpha.surface = surface->hw_surface;
		if((GX_COLOR_FMT_RGB565 == get_color_format(0, gui.config.bpp)) &&
		   (GX_COLOR_FMT_RGB565 == gui.config.color_format))
		{
			Alpha.alpha.type = GX_ALPHA_GLOBAL;
		}
		else
		{
			Alpha.alpha.type = GX_ALPHA_PIXEL;
		}
		Alpha.alpha.value = (gui.config.osd_alpha  * 0x7F) / 0xFF;

		ret |= GxAVSetProperty(g_device_handle, vpu_handle, GxVpuPropertyID_Alpha, &Alpha, sizeof(GxVpuProperty_Alpha));
		if(0 != ret) {
			gxlogd("[GUI]Set OSD Alpha failed!\n");
			return;
		}

		if(gui.config.clut) {
			gui.config.clut->create_pal = NULL;
			gui_set_clut(surface, gui.config.clut->palette, 0, gui.config.clut->num);
			s_pal_index = 0;
		}
		else if(8 == gui.config.bpp) {
			Color temp_clut[256] = {0};

			gui_set_clut(surface, temp_clut, 0, 256);
			s_pal_index = -1;
		}

		color = gui.config.osd_trans;
		color = hd_color2index(surface, color);
		ret = hd_fillrect(surface, &dstrect, color);
		if(8 != gui.config.bpp)
			hd_set_alpha(surface, (unsigned int)gui.config.osd_alpha);

		gdi_commit();
		if(!gui.config.enable_double_buffer)
		{
			ret |= GxAvdev_SetLayerMainSurface(g_device_handle, vpu_handle, GX_LAYER_OSD, surface->hw_surface);
			if(0 != ret) {
				gxlogd("[GUI]Set OSD Main surface failed!\n");
				return;
			}
		}
		else
		{
			if(screen && back_screen)
			{
				RegisterSurface.layer = GX_LAYER_OSD;
				RegisterSurface.surfaces[0] = screen->hw_surface;
				RegisterSurface.surfaces[1] = back_screen->hw_surface;

				ret |= GxAVSetProperty(g_device_handle,
						vpu_handle,
						GxVpuPropertyID_RegistSurface,
						&RegisterSurface,
						sizeof(GxVpuProperty_RegistSurface));
				if(0 != ret){
					gxlogd("[GUI]Register Surface Failed!\n");
					return;
				}
				/*For bug 38823*/
				ret |= GxAvdev_SetLayerMainSurface(g_device_handle, vpu_handle, GX_LAYER_OSD, screen->hw_surface);

				if(0 != ret) {
					gxlogd("[GUI]Set OSD Main surface failed!\n");
					return;
				}
			}
		}
		hd_osd_enable(GUI_TRUE);
		break;
	case LAYER_BACK_OSD_SURFACE:
		if(screen)
			return;

		if(gui.config.enable_double_buffer)
		{
			hd_osd_enable(GUI_FALSE);
			gui.osd_surface = screen = hd_get_surface(&dstrect, gui.config.bpp, NULL, 1);
			if(NULL == screen)
				return;
			ColorKey.enable = 1;
			color = gui.config.osd_trans;
			color = hd_color2index(screen, color);
			hd_color2formatcolor(surface, color, &(ColorKey.color));
			ColorKey.color.a = g_opacity;
			ColorKey.surface = screen->hw_surface;
			ret = GxAVSetProperty(g_device_handle, vpu_handle,
					GxVpuPropertyID_ColorKey, &ColorKey, sizeof(GxVpuProperty_ColorKey));

			if(0 != ret) {
				gxlogd("[GUI]Set OSD Color Key failed!\n");
				return;
			}

			Alpha.surface = screen->hw_surface;
			if((GX_COLOR_FMT_RGB565 == get_color_format(0, gui.config.bpp)) &&
			   (GX_COLOR_FMT_RGB565 == gui.config.color_format))
			{
				Alpha.alpha.type = GX_ALPHA_GLOBAL;
			}
			else
			{
				Alpha.alpha.type = GX_ALPHA_PIXEL;
			}
			Alpha.alpha.value = (gui.config.osd_alpha  * 0x7F) / 0xFF;

			ret |= GxAVSetProperty(g_device_handle, vpu_handle, GxVpuPropertyID_Alpha, &Alpha, sizeof(GxVpuProperty_Alpha));

			if(0 != ret) {
				gxlogd("[GUI]Set OSD Alpha failed!\n");
				return;
			}

			color = gui.config.osd_trans;
			color = hd_color2index(screen, color);
			ret = hd_fillrect(screen, &dstrect, color);
			if(8 != gui.config.bpp)
				hd_set_alpha(screen, (unsigned int)gui.config.osd_alpha);

			if(screen && back_screen)
			{
				RegisterSurface.layer = GX_LAYER_OSD;
				RegisterSurface.surfaces[0] = screen->hw_surface;
				RegisterSurface.surfaces[1] = back_screen->hw_surface;

				ret |= GxAVSetProperty(g_device_handle,
						vpu_handle,
						GxVpuPropertyID_RegistSurface,
						&RegisterSurface,
						sizeof(GxVpuProperty_RegistSurface));
				if(0 != ret){
					gxlogd("[GUI]Register Surface Failed!\n");
					return;
				}
				/*For bug 38823*/
				ret |= GxAvdev_SetLayerMainSurface(g_device_handle, vpu_handle, GX_LAYER_OSD, screen->hw_surface);

				if(0 != ret) {
					gxlogd("[GUI]Set OSD Main surface failed!\n");
					return;
				}
			}
			gdi_commit();
			hd_osd_enable(GUI_TRUE);
		}
		break;
	default:
		break;
	}
}

void hd_free_layer_surface(LayerSurface layer)
{
	GuiSurface *surface = NULL;
	void *hw_surface = NULL;
	GxVpuProperty_UnregistSurface UnRegistSurface = {0};
	int ret = 0;

	if(gui.config.enable_double_buffer)
		surface = back_screen;
	else
		surface = screen;
	switch(layer)
	{
	case LAYER_MAIN_SPP_SURFACE:
		if(NULL == spp_screen)
			return;

		gal_set_default_desc(NULL);
		gal_enable_img(GUI_FALSE);
		GxAvdev_UnSetLayerMainSurface(g_device_handle, vpu_handle, GX_LAYER_SPP, spp_screen->hw_surface);
		hd_free_surface(spp_screen);
		spp_screen = NULL;
		break;
	case LAYER_MAIN_OSD_SURFACE:
		if(NULL == surface)
			return;

		if(gui.config.enable_double_buffer &&
		   screen &&
		   back_screen)
		{
			UnRegistSurface.layer = GX_LAYER_OSD;
			UnRegistSurface.surfaces[0] = screen->hw_surface;
			UnRegistSurface.surfaces[1] = back_screen->hw_surface;

			ret |= GxAVSetProperty(g_device_handle,
					vpu_handle,
					GxVpuPropertyID_UnregistSurface,
					&UnRegistSurface,
					sizeof(GxVpuProperty_UnregistSurface));
			if(0 != ret){
				gxlogd("[GUI]UnRegister Surface Failed!\n");
				return;
			}
		}

		hw_surface = GxAvdev_GetLayerMainSurface(g_device_handle, vpu_handle, GX_LAYER_OSD);
		if(hw_surface)
			GxAvdev_UnSetLayerMainSurface(g_device_handle, vpu_handle, GX_LAYER_OSD, hw_surface);
		hd_osd_enable(GUI_FALSE);
		hd_free_surface(surface);
		if(gui.config.enable_double_buffer) {
			back_screen = NULL;
		}
		else {
			screen = NULL;
		}
		gui.osd_surface = NULL;
		break;
	case LAYER_BACK_OSD_SURFACE:
		if(NULL == screen)
			return;

		if(gui.config.enable_double_buffer)
		{
			hd_osd_enable(GUI_FALSE);
			if(screen &&
			   back_screen)
			{
				UnRegistSurface.layer = GX_LAYER_OSD;
				UnRegistSurface.surfaces[0] = screen->hw_surface;
				UnRegistSurface.surfaces[1] = back_screen->hw_surface;

				ret |= GxAVSetProperty(g_device_handle,
						vpu_handle,
						GxVpuPropertyID_UnregistSurface,
						&UnRegistSurface,
						sizeof(GxVpuProperty_UnregistSurface));
				if(0 != ret){
					gxlogd("[GUI]UnRegister Surface Failed!\n");
					return;
				}
			}
			hw_surface = GxAvdev_GetLayerMainSurface(g_device_handle, vpu_handle, GX_LAYER_OSD);
			if(hw_surface)
				GxAvdev_UnSetLayerMainSurface(g_device_handle, vpu_handle, GX_LAYER_OSD, hw_surface);
			hd_free_surface(screen);
			screen = NULL;
			gui.osd_surface = NULL;
		}
		break;
	default:
		break;
	}
}

void hd_check_layer(void)
{
	if(NULL == gui.current) {
		return;
	}

	hd_set_layer_surface(LAYER_MAIN_OSD_SURFACE);

	if(gui.config.enable_double_buffer)
		hd_set_layer_surface(LAYER_BACK_OSD_SURFACE);
}

int hd_init(void)
{
	GxVpuProperty_ColorKey ColorKey = { 0 };
	GxVpuProperty_RegistSurface RegisterSurface = {0};
	GxVpuProperty_FlipSurface FlipSurface = {0};

	GxVpuProperty_Alpha Alpha;
	GAL_Rect dstrect = {0};
	GxAvRect viewport_rect;

	int ret = 0, color = 0;

	dstrect.x = 0;
	dstrect.y = 0;
	dstrect.w = gui.config.width;
	dstrect.h = gui.config.height;

	/*------------------ Create SPP YCbCr surface------------------*/
	//spp_screen = hd_get_surface(&dstrect, 0, NULL, 1); // ?

	/*------------------ Create OSD RGB surface------------------*/
	if(gui.config.color_format){
		screen = hd_get_surface0(&dstrect, gui.config.bpp, gui.config.color_format, NULL, 1);
	}else{
		screen = hd_get_surface(&dstrect, gui.config.bpp, NULL, GUI_TRUE);
		gui.config.color_format = screen->sf.color_format;
	}

	if(gui.config.enable_double_buffer)
	{
		if(gui.config.color_format)
			back_screen = hd_get_surface0(&dstrect, gui.config.bpp, gui.config.color_format, NULL, 1);
		else
			back_screen = hd_get_surface(&dstrect, gui.config.bpp, NULL, GUI_TRUE);
	}

	/*------Close down the SPP surface, which ensure wild data cannot be displayed------*/
	/*
	ret = GxAvdev_LayerEnable(g_device_handle, vpu_handle, GX_LAYER_SPP, FALSE);

	if(0 != ret) {
		gxlogd("[GUI]Set SPP main surface enable failed!\n");
		return (1);
	}*/

	/*-----------Set SPP surface which created previously as main surface-----------*/
	/*
	ret |= GxAvdev_SetLayerMainSurface(g_device_handle, vpu_handle, GX_LAYER_SPP, spp_screen->hw_surface);

	if(0 != ret) {
		gxlogd("[GUI]Set SPP main surface failed!\n");
		return (1);
	}
	*/

	viewport_rect.x = viewport_rect.y = 0;
	viewport_rect.width = gui.config.width;
	viewport_rect.height = gui.config.height;
	ret |= GxAvdev_SetLayerViewport(g_device_handle, vpu_handle, GX_LAYER_SPP, &viewport_rect);
	if(0 != ret) {
		gxlogd("Set SPP view port failed\n");
		ret = 0;
	}

	Alpha.surface = screen->hw_surface;
	if((GX_COLOR_FMT_RGB565 == get_color_format(0, gui.config.bpp)) &&
	   (GX_COLOR_FMT_RGB565 == gui.config.color_format))
	{
		Alpha.alpha.type = GX_ALPHA_GLOBAL;
		Alpha.alpha.value = (gui.config.osd_alpha  * 0x7F) / 0xFF;
	}
	else if(GX_COLOR_FMT_ARGB1555 == gui.config.color_format)
	{
		Alpha.alpha.type = GX_ALPHA_PIXEL;
		Alpha.alpha.value = (gui.config.osd_alpha  * 0x7F) / 0xFF;
	}
	else if(GX_COLOR_FMT_ARGB4444 == gui.config.color_format)
	{
		Alpha.alpha.type = GX_ALPHA_PIXEL;
		Alpha.alpha.value = (gui.config.osd_alpha  * 0x7F) / 0xFF;
	}
	else if(GX_COLOR_FMT_RGBA8888 == gui.config.color_format)
	{
		Alpha.alpha.type = GX_ALPHA_PIXEL;
		Alpha.alpha.value = (gui.config.osd_alpha  * 0x7F) / 0xFF;
	}
	else
	{
		Alpha.alpha.type = GX_ALPHA_PIXEL;
		Alpha.alpha.value = (gui.config.osd_alpha  * 0x7F) / 0xFF;
	}

	ret |= GxAVSetProperty(g_device_handle, vpu_handle, GxVpuPropertyID_Alpha, &Alpha, sizeof(GxVpuProperty_Alpha));

	if(0 != ret) {
		gxlogd("[GUI]Set OSD Alpha failed!\n");
		return (1);
	}

	if(gui.config.enable_double_buffer) {
		Alpha.surface = back_screen->hw_surface;
		ret |= GxAVSetProperty(g_device_handle,
				vpu_handle,
				GxVpuPropertyID_Alpha,
				&Alpha,
				sizeof(GxVpuProperty_Alpha));

		if(0 != ret) {
			gxlogd("[GUI]Set Back screen OSD Alpha failed!\n");
			return (1);
		}
	}

	ColorKey.enable = 1;
	color = gui.config.osd_trans;
	color = hd_color2index(screen, color);
	hd_color2formatcolor(screen, color, &(ColorKey.color));

	ColorKey.color.a = g_opacity;
	ColorKey.surface = screen->hw_surface;
	ret = GxAVSetProperty(g_device_handle, vpu_handle,
			GxVpuPropertyID_ColorKey, &ColorKey, sizeof(GxVpuProperty_ColorKey));

	if(0 != ret) {
		gxlogd("[GUI]Set OSD Color Key failed!\n");
		return (1);
	}

	if(gui.config.enable_double_buffer) {
		ColorKey.surface = back_screen->hw_surface;
		ret = GxAVSetProperty(g_device_handle,
				vpu_handle,
				GxVpuPropertyID_ColorKey,
				&ColorKey,
				sizeof(GxVpuProperty_ColorKey));

		if(0 != ret) {
			gxlogd("[GUI]Set Back Screen OSD Color Key failed!\n");
			return (1);
		}
	}

	hd_new_blit_list();
	ret = GxAvdev_LayerEnable(g_device_handle, vpu_handle, GX_LAYER_OSD, FALSE);
	if(0 != ret) {
		gxlogd("[GUI]Set OSD surface enable failed!\n");
		return (1);
	}

	if(gui.config.enable_double_buffer)
	{
		RegisterSurface.layer = GX_LAYER_OSD;
		RegisterSurface.surfaces[0] = screen->hw_surface;
		RegisterSurface.surfaces[1] = back_screen->hw_surface;

		ret |= GxAVSetProperty(g_device_handle,
				vpu_handle,
				GxVpuPropertyID_RegistSurface,
				&RegisterSurface,
				sizeof(GxVpuProperty_RegistSurface));
		if(0 != ret){
			gxlogd("[GUI]Register Surface Failed!\n");
			return (1);
		}
	}

	/*For bug 38823*/
	ret |= GxAvdev_SetLayerMainSurface(g_device_handle, vpu_handle, GX_LAYER_OSD, screen->hw_surface);
	/*Full Screen Rectangle*/
	color = gui.config.osd_trans;
	color = hd_color2index(screen, color);
	gdi_begin();
	ret = hd_fillrect(screen, &dstrect, color);

	if(gui.config.enable_double_buffer) {
		ret = hd_fillrect(back_screen, &dstrect, color);
	}
	gdi_end();

	if(0 != ret) {
		gxlogd("[GUI]Set OSD Main surface failed!\n");
		return (1);
	}

	/*Turn on the Anti-flicker in case User configured*/
	if (gui.config.enable_antiflicker) {
		ret = GxAvdev_SetLayerAntiFlicker(g_device_handle, vpu_handle, GX_LAYER_OSD, GUI_TRUE);
		if(0 != ret) {
			gxlogd("[GUI]Set Anti-flicker enable failed!\n");
		}
	}

	if(gui.config.enable_double_buffer)
	{
		FlipSurface.layer = GX_LAYER_OSD;
		FlipSurface.surface = screen->hw_surface;
		ret = GxAVSetProperty(g_device_handle,
				vpu_handle,
				GxVpuPropertyID_FlipSurface,
				&FlipSurface,
				sizeof(GxVpuProperty_FlipSurface));
		if(ret)
		{
			gxlogd("[GUI] First flip failed.\n");
		}
		gdi_change_surface((void**)&screen, (void**)&back_screen);
	}

	ret |= GxAvdev_SetLayerViewport(g_device_handle, vpu_handle, GX_LAYER_OSD, &viewport_rect);
	if(0 != ret) {
		gxlogd("[GUI]Set OSD view port failed\n");
		ret = 0;
	}

	if(gui.config.clut) {
		gui.config.clut->create_pal = NULL;
		gui_set_clut(screen, gui.config.clut->palette, 0, gui.config.clut->num);
		if(gui.config.enable_double_buffer)
			gui_set_clut(back_screen, gui.config.clut->palette, 0, gui.config.clut->num);
		s_pal_index = 0;
	}
	else if(8 == gui.config.bpp) {
		Color temp_clut[256] = {0};

		gui_set_clut(screen, temp_clut, 0, 256);
		if(gui.config.enable_double_buffer)
			gui_set_clut(back_screen, temp_clut, 0, 256);
		s_pal_index = -1;
	}
	hd_enable_osd(GUI_FALSE);
	gui.osd_surface = screen;

	return (ret);
}

int hd_flip(void)
{
	int ret = 0;

	return (ret);
}

int hd_quit(void)
{
	GxVpuProperty_DestroyPalette DestroyPal = {0};
	GuiClut *clut = NULL;
	void *pal = NULL;
	int ret = 0;

	if(0 == g_device_handle)
	{
		return (1);
	}

	clut = gui.config.firstclut;
	if(NULL == clut)
	{
		return (1);
	}

	while(clut)
	{
		pal = clut->create_pal;

		if(pal)
		{
			DestroyPal.palette = (GxPalette *)pal;

			ret = GxAVSetProperty(g_device_handle,
					vpu_handle,
					GxVpuPropertyID_DestroyPalette,
					&DestroyPal,
					sizeof(GxVpuProperty_DestroyPalette));
		}
		clut = clut->next;
	}
	hd_free_surface(spp_screen);
	hd_free_surface(screen);
	hd_free_surface(back_screen);

	ret = hd_destroy_lock();

	g_device_handle = 0;

	return (ret);
}

GxHwMallocObj *hd_malloc(GxHwMallocObj **p, size_t size)
{
	if(NULL == *p)
	{
		*p = GUI_MALLOCZ(sizeof(GxHwMallocObj));
		if(NULL == *p)
			return (NULL);

		(*p)->size = size;
	}

	*p = fb_malloc(&gui.fb, *p);

	return *p;
}

GxHwMallocObj *hd_malloc_main(GxHwMallocObj **p, size_t size, int flag)
{
	if(NULL == *p)
	{
		*p = GUI_MALLOCZ(sizeof(GxHwMallocObj));
		if(NULL == *p)
			return (NULL);

		(*p)->size = size;
	}

	*p = fb_malloc_main(&gui.fb, *p, flag);

	return *p;
}

GxHwMallocObj *hd_malloc_nocache(GxHwMallocObj **p, size_t size)
{
	if(NULL == *p)
	{
		*p = GUI_MALLOCZ(sizeof(GxHwMallocObj));
		if(NULL == *p)
			return (NULL);

		(*p)->size = size;
	}

	*p = fb_malloc_nocache(&gui.fb, *p);

	return *p;
}

void hd_check(GxHwMallocObj *p)
{
	if(p && p->usr_p && p->kernel_p)
	{
		fb_check(&gui.fb, p);
	}
}

#endif /*NO_OS*/

void hd_free(GxHwMallocObj *p)
{
	fb_free(&gui.fb, p);
}

GuiSurface *image_desc_surface(const image_desc *desc)
{
	GAL_Rect rect;
	int bpp;
	GuiSurface *surface;

	rect.x = 0;
	rect.y = 0;
	rect.w = desc->width;
	rect.h = desc->height;

	if(JPEG_TYPE == desc->type)
		bpp = 0;
	else
		bpp = desc->bpp;

	if (desc->hw_accel) {
		if(desc->fmt)
			surface = hd_get_surface0(&rect, bpp, desc->fmt, desc->data, GUI_FALSE);
		else
			surface = hd_get_surface(&rect, bpp, desc->data, 0);
		if (surface->buffer_size != desc->size)
			while(1);
	}
	else {
		if(desc->fmt)
			surface = hd_get_surface0(&rect, bpp, desc->fmt, NULL, GUI_FALSE);
		else
			surface = hd_get_surface(&rect, bpp, NULL, 0);
		if (surface)
			memcpy(surface->data, desc->data, desc->size);
	}

	if (surface)
		hd_check_surface(surface);

	return surface;
}

