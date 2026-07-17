#include "hd_gal.h"
#ifndef NO_OS
#include "gal.h"
#include "gui_core.h"
#include "av/avapi.h"
#include "av/gxav.h"
#include "gxavdev.h"
#include "av/gxav_vpu_propertytypes.h"
#include "gui_private.h"

int hd_image_rect(GuiSurface *surface, GAL_Rect * dstrect, unsigned int color)
{
	GxVpuProperty_FillRect FillRect = {0};
	int ret = 0;

	if((NULL == surface) || (NULL == dstrect))
	    return 1;

	FillRect.color.y = (color >> 16) & 0x0000FF;
	FillRect.color.cb = (color >> 8) & 0x0000FF;
	FillRect.color.cr = color & 0x0000FF;
	FillRect.color.alpha = 0xFF;
	FillRect.rect.x = dstrect->x;
	FillRect.rect.y = dstrect->y;
	FillRect.rect.width = dstrect->w;
	FillRect.rect.height = dstrect->h;
	FillRect.surface = surface;
	FillRect.is_ksurface = is_ksurface(surface);

	ret = GxAVSetProperty(g_device_handle,
			vpu_handle,
			GxVpuPropertyID_FillRect,
			&FillRect,
			sizeof(GxVpuProperty_FillRect));

	if(0 != ret)
	{
		gxlogd("[GUI]Fill SPP Rectangle failed\n");
		return (1);
	}

	hd_add_blit_element(NULL);
	//hd_add_update(g_header, NULL);

	return (ret);
}



int hd_enable_video(int flag)
{
	int ret;

	flag = 1 ^ flag;
	ret = GxAvdev_SetLayerTop(g_device_handle, vpu_handle, GX_LAYER_SPP, flag);
	if(0 != ret) {
		gxlogd("[GUI]Set SPP top or bottom failed!\n");
		return 1;
	}

	return 0;
}

int hd_set_video_size(int x, int y, int x_size, int y_size)
{
	int ret;
	GxAvRect viewport_rect;

	viewport_rect.x = x;
	viewport_rect.y = y;
	viewport_rect.width = x_size;
	viewport_rect.height = y_size;

	ret = GxAvdev_SetLayerViewport(g_device_handle, vpu_handle, GX_LAYER_VPP, &viewport_rect);
	if(0 != ret) {
		gxlogd("[GUI]Set VPP size failed!\n");
		return (1);
	}

	return (ret);
}

int hd_set_image_size(int x, int y, int x_size, int y_size)
{
	int ret;
	GxAvRect viewport_rect;

	viewport_rect.x = x;
	viewport_rect.y = y;
	viewport_rect.width = x_size;
	viewport_rect.height = y_size;

	ret = GxAvdev_SetLayerViewport(g_device_handle, vpu_handle, GX_LAYER_SPP, &viewport_rect);
	if(0 != ret) {
		gxlogd("[GUI]Set SPP size failed!\n");
		return (1);
	}

	return (0);
}

int hd_set_interface_size(int x, int y, int x_size, int y_size)
{
	int ret;
	GxAvRect viewport_rect;

	viewport_rect.x = x;
	viewport_rect.y = y;
	viewport_rect.width = x_size;
	viewport_rect.height = y_size;

	ret = GxAvdev_SetLayerViewport(g_device_handle, vpu_handle, GX_LAYER_OSD, &viewport_rect);
	if(0 != ret) {
		gxlogd("[GUI]Set OSD size failed!\n");
		return (1);
	}

	return (ret);
}

int hd_resolution(GAL_Rect *rect)
{
	return GxAvdev_SetVirtualResolution(g_device_handle, vpu_handle, rect->w, rect->h);
}
#else /*NO_OS*/
#include "gdi_core.h"
#include "gui_core.h"
#include "hd_gal.h"
#include "gal.h"
#include "gal_blit.h"
#include "av/gxav.h"
#include "av/avapi.h"
#include "mini_gui_private.h"

extern int g_device_handle;
extern int vpu_handle;
extern GuiCore gui;
#endif /*NO_OS*/

int hd_fillrect(GuiSurface *surface, GAL_Rect * dstrect, unsigned int color)
{
	GuiCore *gui_core = NULL;
	GxVpuProperty_FillRect FillRect;
	GxColorFormat color_format = 0;
	int ret = 0;

	if((NULL == surface) || (NULL == dstrect))
	    return 1;

	if((0 == dstrect->w) || (0 == dstrect->h))
	{
		return (1);
	}

	gui_core = GUI_GetCurrent();
	surface = hd_get_idle_surface(surface);

	memset(&FillRect, 0, sizeof(GxVpuProperty_FillRect));
	color_format = surface->sf.color_format;
	hd_color2formatcolor(surface, color, &(FillRect.color));
	if (GX_COLOR_FMT_ARGB1555 == color_format) {
		FillRect.color.a = 1 ? 0xFF : 0;
	}

	FillRect.rect.x = dstrect->x;
	FillRect.rect.y = dstrect->y;
	FillRect.rect.width = dstrect->w;
	FillRect.rect.height = dstrect->h;
	FillRect.is_ksurface = is_ksurface(surface);
	FillRect.surface = surface->hw_surface;
	ret = GxAVSetProperty(g_device_handle,
			vpu_handle,
			GxVpuPropertyID_FillRect,
			&FillRect,
			sizeof(GxVpuProperty_FillRect));

	if(0 != ret)
	{
		gxlogd("[GUI]Filled Rectangle failed!\n");
		return (1);
	}

	hd_add_blit_element(NULL);

	return (ret);
}

