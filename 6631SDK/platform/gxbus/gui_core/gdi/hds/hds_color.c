#include "SDL.h"
#include "gui_core.h"


int hd_set_clut(void *surface, Color * colors, int firstcolor, int ncolors)
{
	int ret = 0, i = 0;
	SDL_Surface *screen = (SDL_Surface *) (surface);
	SDL_Color *pal_color = NULL;

	if(NULL == surface || NULL == colors)
	{
		return (1);
	}

	pal_color = (SDL_Color *)GxCore_Malloc((ncolors - firstcolor + 1) * sizeof(SDL_Color));
	if(NULL == pal_color)
	{
		return (1);
	}

	for(i = firstcolor; i < ncolors; i++)
	{
		pal_color[i].b = colors[i] & 0x0000FF;
		pal_color[i].g = (colors[i] >> 8) & 0x0000FF;
		pal_color[i].r = (colors[i] >> 16) & 0x0000FF;
	}
	ret = SDL_SetColors(screen, pal_color, firstcolor, ncolors);

	GUI_FREE(pal_color);
	pal_color = NULL;

	return (ret);
}

int hd_set_alpha(void *surface, int alpha)
{
	return (0);
}

void *hd_get_clut(void)
{
	return NULL;
}


