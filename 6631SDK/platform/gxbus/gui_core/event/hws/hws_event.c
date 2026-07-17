#include "gui_private.h"

unsigned char hd_poll_event(GUI_Event *data)
{
	unsigned char type = 0;
	SDL_Event event = {0};

	if(NULL == data)
	{
		return (GUI_NOEVENT);
	}

	if (0 == SDL_PollEvent((SDL_Event *)&event))
	{
		return (GUI_NOEVENT);
	}

	type = event.type;
	switch(type)
	{
		case GUI_MOUSEBUTTONDOWN:
			memcpy(&data->button, &event.button, sizeof(GUI_MouseButton));
			break;
		case GUI_KEYDOWN:
			data->key.type = type;
			data->key.scancode = event.key.keysym.scancode;
			data->key.sym = event.key.keysym.sym;
			break;
		case GUI_QUIT:
			break;
		default:
			break;
	}

	return (type);
}



