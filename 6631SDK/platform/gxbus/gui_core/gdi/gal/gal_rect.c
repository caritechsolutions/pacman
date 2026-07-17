#include "hd_gal.h"
#include "gui_core.h"
#ifndef NO_OS
extern GuiCore gui;
extern int s_pal_index;

int gal_fillrect(void *screen, GAL_Rect * dstrect, unsigned int color)
{
    int ret = 0;
    GAL_Rect rect = {0};
	GuiCore *gui_core = NULL;
    GuiSurface *surface = NULL;

	gui_core = GUI_GetCurrent();

    if((0 > dstrect->x) ||
       (0 > dstrect->y) ||
       (0 > dstrect->w) ||
       (0 > dstrect->h)) {
	    return (-1);
    }

    if((8 == gui_core->config.bpp) && (NULL != gui_core->config.clut))
    {
	if(gui_core->config.gui_trans == (gui_core->config.clut->palette[color]&0x00ffffff))
	{
	    return (0);
	}
    }
    else if(16 == gui_core->config.bpp)
    {
	if(gal_color2index(screen, gui_core->config.gui_trans) == (0x00FFFF & color))
	{
	    return (0);
	}
    }
    else
    {
	if(gal_color2index(screen, gui_core->config.gui_trans) == color)
	{
	    return (0);
	}
    }

    if((gui_core->config.clut) && (0 > s_pal_index))
    {
	hd_set_clut(screen, gui_core->config.clut->palette, 0, 256);
	s_pal_index = 0;
    }

    rect = *dstrect;
    surface = screen;

    if(surface)
    {
	ret = hd_fillrect(surface, &rect, color);
    }

    return (ret);
}

int gdi_fillrect(void *rect, unsigned int color)
{
	if(NULL == rect)
	{
		return (1);
	}

	return gal_fillrect(screen, (GAL_Rect *)rect, color);
}

int gdi_image_rect(void *rect, int color)
{
	GAL_Rect *image_rect = NULL;

	if(NULL == rect)
	{
		return (1);
	}

	image_rect = (GAL_Rect *)rect;

	return hd_image_rect(spp_screen, image_rect, color);
}

int gdi_resolution(GAL_Rect *rect)
{
	if(NULL == rect)
	{
		return (1);
	}

	return hd_resolution(rect);
}

#endif /*NO_OS*/
int gal_stretch_surface(void *src, GAL_Rect *src_rect, void *dst, GAL_Rect *dst_rect)
{
	int ret = 0;

	ret = hd_stretch_surface(src, src_rect, dst, dst_rect);
	ret = hd_add_blit_element(NULL);

	return (ret);
}

