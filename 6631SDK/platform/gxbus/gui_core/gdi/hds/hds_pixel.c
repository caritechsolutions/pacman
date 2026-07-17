#include "gal.h"
#include "SDL.h"
#include "gui_core.h"

#define  SQUARE(Dist) ((unsigned short)Dist) * ((unsigned short)Dist)
static int gui_trans_color;

extern GuiCore gui;

/*
 * Set the pixel at (x, y) to the given value
 * NOTE: The surface must be locked before calling this!
 */
void hd_putpixel(void *screen, int x, int y, unsigned int pixel)
{
	SDL_Surface *surface = NULL;
	int bpp = 0;
	/* Here p is the address to the pixel we want to set */
	Uint8 *p = NULL;

	surface = (SDL_Surface *) screen;
	if (NULL == surface) {
		return;
	}
	bpp = surface->format->BytesPerPixel;
	p = (Uint8 *) surface->pixels + y * surface->pitch + x * bpp + surface->offset;

	if (pixel == gal_color2index(surface, gui.config.gui_trans)) {
		return;
	}

	switch (bpp) {
		case 1:
			*p = pixel;
			break;

		case 2:
			*(Uint16 *) p = pixel;
			break;

		case 3:
			if (SDL_BYTEORDER == SDL_BIG_ENDIAN) {
				p[0] = (pixel >> 16) & 0xff;
				p[1] = (pixel >> 8) & 0xff;
				p[2] = pixel & 0xff;
			} else {
				p[0] = pixel & 0xff;
				p[1] = (pixel >> 8) & 0xff;
				p[2] = (pixel >> 16) & 0xff;
			}
			break;

		case 4:
			*(Uint32 *) p = pixel;
			break;
	}
}

void hd_image_pixel(void *screen, int x, int y, unsigned int pixel)
{
	return;
}

static int hds_calcolor_dif(int palcolor, int color)
{
	short dist;
	int sum;

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

int hd_color2index(void *screen, int color)
{
	int bpp = 0, new_color = 0;
	int i = 0;
	int *pal = NULL;
	unsigned int ncolors = 0, bestdiff = 0xFFFFFFFF, bestindex = 0;

	if (NULL != gui.config.clut) {
		ncolors = gui.config.clut->num;
		if (gui.config.clut->palette) {
			pal = gui.config.clut->palette;
		} else {
			return (0);
		}
	}

	bpp = gui.config.bpp;

	if (8 == bpp) {

		for (i = 0; i < ncolors; i++) {
			if (color == (int)(pal[i]))
				return (i);
		}

		i = 0;
		for (i = 0; i < ncolors; i++) {
			unsigned int diff;

			if((pal[i] == gui.config.gui_trans) || (pal[i] == gui.config.osd_trans))
			{
				continue;
			}

			diff = hds_calcolor_dif((int)color, (int)pal[i]);
			if (diff < bestdiff) {
				bestdiff = diff;
				bestindex = i;
			}
		}
		return (bestindex);
	}
	else if(16 == bpp)
	{
		new_color = (((color >> 16) & 0xF0) << 8) |
			(((color >> 8) & 0xF0) << 8) |
			(color & 0xF0) |
			(((gui.config.osd_alpha) & 0xF0) >> 4);
		color = new_color;
	}
	else {
		return (color);
	}
	return (color);
}


void hd_set_gui_transparent(unsigned int color)
{
	gui_trans_color = color;

	return;
}

int hd_get_gui_transparent(void)
{
	return (gui_trans_color);
}


int hd_set_osd_transparent(unsigned int color, unsigned short opacity)
{
	return (0);
}
