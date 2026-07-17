#include "SDL.h"

unsigned int hd_get_tick(void)
{
	return (SDL_GetTicks());
}

void hd_delay(unsigned int ms)
{
	SDL_Delay(ms);
}

