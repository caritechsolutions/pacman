#include "SDL.h"
#include "gui_private.h"

void *screen = NULL;
void *spp_screen = NULL;
void *spp_surface = NULL;
int irr_fd = 0;
int panel_fd = 0;
int g_device_handle = 0;

int hd_osd_enable(int flag)
{
	return (0);
}

int hd_init(void)
{
	int ret = 0;

	if (SDL_Init(SDL_INIT_VIDEO) < 0) {
		gxloge ( "[GUI]Couldn't initialize SDL: %s\n",SDL_GetError());
		return (1);
	}

	screen = SDL_SetVideoMode(gui.config.width, gui.config.height, gui.config.bpp, SDL_HWSURFACE);
	if (!screen) {
		gxloge ( "[GUI]Couldn't set %dx%d video mode: %s\n", gui.config.width, gui.config.height,SDL_GetError());
		return (1);
	}

	if(gui.config.clut)
	{
		ret = gui_set_clut(screen, gui.config.clut->palette, 0, gui.config.clut->num);
		if (0 == ret)
		{
			return (1);
		}
	}

	return (0);
}

int hd_flip(void)
{
	int ret = 0;

	ret = SDL_Flip((SDL_Surface *)screen);

	return (ret);
}

int hd_quit(void)
{
	SDL_Quit();

	return (0);
}

int hd_lock(void)
{
	return (0);
}

int hd_unlock(void)
{
	return (0);
}

