#ifndef NO_OS
#include "gal_blit.h"
#else
#include "gal.h"
#include "hd_gal.h"
#endif


void *gal_get_surface(GAL_Rect *rect, int bpp)
{
#ifdef NO_OS
	return hd_get_surface(rect, bpp, NULL, 0);
#else /*NO_OS*/
	return hd_get_surface0(rect, bpp, gui.config.color_format, NULL, 0);
#endif /*NO_OS*/
}

int gal_copy_surface(void *src, GAL_Rect *src_rect, void *dst, GAL_Rect *dst_rect)
{
	return (hd_copy_surface(src, src_rect, dst, dst_rect));
}

int gal_mirror_image_surface(GuiSurface *src, GAL_Rect *src_rect, GuiSurface *dst, GAL_Rect *dst_rect, GxReverseMode mode)
{
	return (hd_mirror_image_surface(src, src_rect, dst, dst_rect, mode));
}

void gal_free_surface(void *surface)
{
	hd_free_surface(surface);
}

