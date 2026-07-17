#include "hd_gal.h"
#include "gui_core.h"
#ifdef NO_OS
#include "mini_gui_private.h"
#include "hd_gal.h"
#endif

/*
 * Set the pixel at (x, y) to the given value
 * NOTE: The surface must be locked before calling this!
 */
void gal_putpixel(void *screen, int x, int y, unsigned int pixel)
{
#ifndef NO_OS
	hd_putpixel(screen, x, y, pixel);
#endif
}

int gal_color2index(void *screen, int color)
{
	return (hd_color2index(screen, color));
}

int gal_color2index0(void *screen, int color)
{
	return (hd_color2index0(screen, color));
}

void gal_set_gui_transparent(unsigned int color)
{
	hd_set_gui_transparent(color);
}

int gal_get_gui_transparent(void)
{
	return (hd_get_gui_transparent());
}

int gal_set_osd_transparent(unsigned int color, unsigned short opacity)
{
	return (hd_set_osd_transparent(color, opacity));
}

int gdi_setpixel(int x, int y, int color)
{
#ifndef NO_OS
	gal_putpixel(screen, x, y, (unsigned int)color);
#endif

	return 0;
}

int gdi_image_pixel(int x, int y, int color)
{
#ifndef NO_OS
	hd_image_pixel(spp_screen, x, y, (unsigned int)color);
#endif

	return 0;
}

