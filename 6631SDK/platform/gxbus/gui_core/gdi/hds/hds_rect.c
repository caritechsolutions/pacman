#include "SDL_video.h"
#include "gal.h"
#include "gui_core.h"
extern GuiCore gui;

int hd_fillrect(void *screen, GAL_Rect * dstrect, unsigned int color)
{
	SDL_Surface *dst = NULL;
	int ret = 0;

	dst = (SDL_Surface *) screen;

	if (NULL == dst || NULL == dstrect) {
		return (1);
	}

	//hd_add_blit_element(screen);

	/*if (8 == gui.config.bpp) {
	  color = gui.config.clut->palette[color];
	  }*/

	if(gui.config.clut)
	{
		if (gui.config.clut->palette[color] != gal_get_gui_transparent()) {
			ret = SDL_FillRect(dst, (SDL_Rect *) dstrect, color);
		}
	}
	else
	{
		ret = SDL_FillRect(dst, (SDL_Rect *) dstrect, color);
	}

	return (ret);
}

int hd_image_rect(void *surface, GAL_Rect * dstrect, unsigned int color)
{
	return (0);
}


int hd_enable_video(int flag)
{
	return (0);
}

int hd_set_video_size(int x, int y, int x_size, int y_size)
{

	return (0);
}

int hd_set_image_size(int x, int y, int x_size, int y_size)
{
	return (0);
}

int hd_set_interface_size(int x, int y, int x_size, int y_size)
{
	return (0);
}

int hd_resolution(GAL_Rect *rect)
{
	return (0);
}


