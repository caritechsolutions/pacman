#ifndef NO_OS
#include "gal.h"
#include "gui_core.h"
#include "av/avapi.h"
#include "av/gxav.h"
#include "hd_gal.h"
#include "av/gxav_vpu_propertytypes.h"
#include "gui_private.h"

/*
 * Set the pixel at (x, y) to the given value
 * NOTE: The surface must be locked before calling this!
 */
void hd_putpixel(GuiSurface *surface, int x, int y, unsigned int pixel)
{
	GxVpuProperty_Point Point;
	int ret = 0;

	hd_color2formatcolor(surface, pixel, &(Point.color));
	if (GX_COLOR_FMT_ARGB1555 == surface->sf.color_format) {
		Point.color.a = 1 ? 0xFF : 0;
	}

	Point.surface = surface->hw_surface;

	Point.point.x = x;
	Point.point.y = y;

	ret = GxAVSetProperty(g_device_handle, vpu_handle, GxVpuPropertyID_Point, &Point, sizeof(GxVpuProperty_Point));

	if(0 != ret)
	{
		gxlogd("[GUI]Set OSD point failed!\n");
		return;
	}

}

void hd_image_pixel(GuiSurface *surface, int x, int y, unsigned int pixel)
{
	GxVpuProperty_Point Point;
	int ret = 0;

	Point.color.y = (pixel >> 16) & 0x0000FF;
	Point.color.cb = (pixel >> 8) & 0x0000FF;
	Point.color.cr = pixel & 0x0000FF;
	Point.color.alpha = 0xFF;
	Point.surface = surface->hw_surface;
	Point.point.x = x;
	Point.point.y = y;

	ret = GxAVSetProperty(g_device_handle,
			vpu_handle,
			GxVpuPropertyID_Point,
			&Point,
			sizeof(GxVpuProperty_Point));

	if(0 != ret)
	{
		gxlogd("[GUI]Set SPP point failed!\n");
		return;
	}
}

/*static int _el2eb(int val)
  {
  int ret = 0;
  int temp[4] = { 0 };

  temp[0] = val & 0xFF;
  temp[1] = (val & 0xFF00) >> 8;
  temp[2] = (val & 0xFF0000) >> 16;
  temp[3] = (val & 0xFF000000) >> 24;

  ret = (temp[0] << 16) | (temp[1] << 8) | (temp[2]);

  return (ret);
  }*/


#else /*NO_OS*/
#include "gdi_core.h"
#include "gui_core.h"
#include "mini_gui_private.h"

extern GuiCore gui;;

#endif /*NO_OS*/
int g_opacity = 0;
static int gui_trans_color;

#define  SQUARE(Dist) ((unsigned short)Dist) * ((unsigned short)Dist)
static int hda_calcolor_dif(int palcolor, int color)
{
	short dist;
	int sum;

	//palcolor = palcolor >> 8;
	//color = color >> 8;

	dist = ((palcolor)&0xFF) - ((color)&0xFF);
	if(dist < 0)
		dist = -dist;

	sum = SQUARE(dist);
	dist = (((palcolor)>>8)&0xFF) - (((color)>>8)&0xFF);
	if(dist<0)
		dist = -dist;
	sum += SQUARE(dist);
	dist = ((palcolor)>>16) - ((color)>>16);
	if(dist<0)
		dist = -dist;
	return (sum + SQUARE(dist));
}

int hd_color2index0(GuiSurface *surface, int color)
{
	GuiCore *gui_core = NULL;
	int bpp = 0, new_color = 0;
	unsigned int i = 0;
	int *pal = NULL;
	unsigned int ncolors = 0, bestdiff = 0xFFFFFFFF, bestindex = 0;
	GxColorFormat color_format = 0;

	if(NULL == surface)
		return (1);

	gui_core = GUI_GetCurrent();
	if (NULL != gui_core->config.clut) {
		ncolors = gui_core->config.clut->num;
		if (gui_core->config.clut->palette) {
			pal = gui_core->config.clut->palette;
		} else {
			return (0);
		}
	}

	bpp = surface->bpp;
	color_format = surface->sf.color_format;

	//color = _el2eb(color);

	if (8 == bpp) {

		for (i = 0; i < ncolors; i++) {
			if (color == (int)(pal[i]&0x00ffffff))
				return (i);
		}

		i = 0;
		for (i = 0; i < ncolors; i++) {
			unsigned int diff;

			if((pal[i] == gui.config.gui_trans) || (pal[i] == gui.config.osd_trans))
			{
				continue;
			}

			diff = hda_calcolor_dif((int)color, (int)pal[i]);
			if (diff < bestdiff) {
				bestdiff = diff;
				bestindex = i;
			}
		}
		return (bestindex);
	}
	else if(16 == bpp)
	{
		if(GX_COLOR_FMT_ARGB4444 == color_format) {
			new_color = ((color & 0xF0000000) >> 16) |
				((color & 0x00F00000) >> 12) |
				((color & 0x0000F000) >> 8) |
				((color & 0x000000F0) >> 4);
			return (new_color);
		}
		else if(GX_COLOR_FMT_ARGB1555 == color_format) {
			new_color = ((color & 0x80000000) >> 16) |
				((color & 0x00F80000) >> 9) |
				((color & 0x0000F800) >> 6) |
				((color & 0x000000F8) >> 3);
			return (new_color);
		}else {
			new_color = ((((color >> 16) & 0x0000FF) >> 3)<<11) |
				((((color >> 8) & 0x0000FF) >>2)<<5) |
				((color & 0x0000FF)>>3);
			color = new_color;
		}
	}
	else if(GX_COLOR_FMT_RGBA8888 == color_format)
	{
		return (0xFFFFFFFF & color);
	}
	else if(GX_COLOR_FMT_ARGB8888 == color_format)
	{
		if((gui.config.osd_trans == color) ||
		   (gui.config.gui_trans == color)) {
			return (0x00000000 | color);
		} else {
			return (0xFFFFFFFF & color);
		}
	}
	else {
		return (color);
	}
	return (color);
}

int hd_color2index(GuiSurface *surface, int color)
{
	GuiCore *gui_core = NULL;
	int bpp = 0, new_color = 0;
	unsigned int i = 0;
	int *pal = NULL;
	unsigned int ncolors = 0, bestdiff = 0xFFFFFFFF, bestindex = 0;
	GxColorFormat color_format = 0;

	if(NULL == surface)
		return (1);

	gui_core = GUI_GetCurrent();
	if (NULL != gui_core->config.clut) {
		ncolors = gui_core->config.clut->num;
		if (gui_core->config.clut->palette) {
			pal = gui_core->config.clut->palette;
		} else {
			return (0);
		}
	}

	bpp = surface->bpp;
	color_format = surface->sf.color_format;

	//color = _el2eb(color);

	if (8 == bpp) {

		for (i = 0; i < ncolors; i++) {
			if (color == (int)(pal[i]&0x00ffffff))
				return (i);
		}

		i = 0;
		for (i = 0; i < ncolors; i++) {
			unsigned int diff;

			if((pal[i] == gui.config.gui_trans) || (pal[i] == gui.config.osd_trans))
			{
				continue;
			}

			diff = hda_calcolor_dif((int)color, (int)pal[i]);
			if (diff < bestdiff) {
				bestdiff = diff;
				bestindex = i;
			}
		}
		return (bestindex);
	}
	else if(16 == bpp)
	{
		if(GX_COLOR_FMT_ARGB4444 == color_format) {
			new_color = ((color & 0xF0000000) >> 16) |
				((color & 0x00F00000) >> 12) |
				((color & 0x0000F000) >> 8) |
				((color & 0x000000F0) >> 4);
			return (new_color);
		}
		else if(GX_COLOR_FMT_ARGB1555 == color_format) {
			new_color = ((color & 0x80000000) >> 16) |
				((color & 0x00F80000) >> 9) |
				((color & 0x0000F800) >> 6) |
				((color & 0x000000F8) >> 3);
			return (new_color);
		}else {
			new_color = ((((color >> 16) & 0x0000FF) >> 3)<<11) |
				((((color >> 8) & 0x0000FF) >>2)<<5) |
				((color & 0x0000FF)>>3);
			color = new_color;
		}
	}
	else if(GX_COLOR_FMT_RGBA8888 == color_format)
	{
		if((gui.config.osd_trans == color) ||
		   (gui.config.gui_trans == color)) {
			return (0x00000000 | color);
		} else {
			return (0x000000FF | color);
		}
	}
	else if(GX_COLOR_FMT_ARGB8888 == color_format)
	{
		if((gui.config.osd_trans == color) ||
		   (gui.config.gui_trans == color)) {
			return (0x00000000 | color);
		} else {
			if((color & 0xFF000000) == 0)
				return (0xFF000000 | color);
		}
	}
	else {
		return (color);
	}
	return (color);
}

void hd_color2formatcolor(GuiSurface *surface, int color, GxColor *value){
	if((NULL == surface) || (NULL == value)){
		return;
	}

	switch(surface->sf.color_format){
		case GX_COLOR_FMT_CLUT8:
			value->entry = color;
			break;
		case GX_COLOR_FMT_RGB565:
			value->r = (color & 0xF800) >> 8;
			value->g = (color & 0x07E0) >> 3;
			value->b = (color & 0x001F) << 3;
			value->a = 0xFF;
			break;
		case GX_COLOR_FMT_ARGB1555:
			value->r = (color & 0x7C00) >> 7;
			value->g = (color & 0x03E0) >> 2;
			value->b = (color & 0x001F) << 3;
			value->a = (color & 0x8000) >> 15;
			break;
		case GX_COLOR_FMT_RGB888:
		case GX_COLOR_FMT_YCBCR422:
			value->r = (color >> 16) & 0xFF;
			value->g = (color >> 8) & 0xFF;
			value->b = color & 0xFF;
			value->a = 0xFF;
			break;
		case GX_COLOR_FMT_ARGB4444:
			value->r = (color & 0x0F00) >> 4;
			value->g = (color & 0x00F0);
			value->b = (color & 0x000F) << 4;
			value->a = (color & 0xF000) >> 8;
			break;
		case GX_COLOR_FMT_RGBA8888:
		case GX_COLOR_FMT_YCBCRA6442:
			value->r = (color >> 24) & 0xFF;
			value->g = (color >> 16) & 0xFF;
			value->b = (color >> 8) & 0xFF;
			value->a = color & 0xFF;
			break;
		case GX_COLOR_FMT_ARGB8888:
			value->a = (color >> 24) & 0xFF;
			value->r = (color >> 16) & 0xFF;
			value->g = (color >> 8) & 0xFF;
			value->b = color & 0xFF;
			break;
		default:
			break;
	}
}


void hd_set_gui_transparent(unsigned int color)
{
	gui.config.gui_trans = gui_trans_color = color;

	return;
}

int hd_get_gui_transparent(void)
{
	return (gui_trans_color);
}

int hd_set_osd_transparent(unsigned int color, unsigned short opacity)
{
	gui.config.osd_trans = color;
	g_opacity = opacity;

	return (0);
}

