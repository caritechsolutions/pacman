#include "gui_private.h"
#include "hd_system.h"

unsigned char gui_poll_event(GUI_Event *data)
{
	unsigned char type = 0;

	if((gui.key_process) && (gui.key_process->handler))
	{
	    type = gui.key_process->handler(NULL, data);
	    return (type);
	}

	type = hd_poll_event(data);

	return (type);
}


