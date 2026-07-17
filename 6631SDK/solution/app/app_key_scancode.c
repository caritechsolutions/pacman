#include "app.h"
#include "app_default_params.h"
#include "app_module.h"
#include "app_send_msg.h"
#include "app_windows.h"
#if KEY_SCANCODE_SUPPORT

SIGNAL_HANDLER  int app_key_scancode_create(const char* widgetname, void *usrdata)
{
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER  int app_key_scancode_destroy(const char* widgetname, void *usrdata)
{
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER  int app_key_scancode_keypress(const char* widgetname, void *usrdata)
{

	char key_buffer[20] = {0};
	uint32_t box_sel;
	GUI_Event *event = NULL;
	event = (GUI_Event *)usrdata;

	if(event->type == GUI_KEYDOWN)
	{
		switch(event->key.sym)
		{
			case STBK_MENU:
			case STBK_EXIT:
				GUI_EndDialog(WND_KEY_SCANCODE);
				break;

			default:
				GUI_GetProperty("box_key_scancode", "select", &box_sel);
				if(box_sel == 0)
				{
					sprintf(key_buffer,"0x%x",event->key.scancode);
					GUI_SetProperty("text_key_scancode_name","string",key_buffer);
				}
				break;
		}
	}

	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_key_scancode_box_keypress(GuiWidget *widget, void *usrdata)
{
	int ret = EVENT_TRANSFER_KEEPON;
	GUI_Event *event = NULL;
	uint32_t box_sel;

	event = (GUI_Event *)usrdata;
	if(event->type == GUI_KEYDOWN)
	{
		switch(event->key.sym)
		{
			case STBK_UP:
			case STBK_DOWN:
				{
					GUI_GetProperty("box_key_scancode", "select", &box_sel);
					if(box_sel == 1)
					{
						box_sel = 0;
						GUI_SetProperty("box_key_scancode", "select", &box_sel);
					}
					else
					{
						box_sel = 1;
						GUI_SetProperty("box_key_scancode", "select", &box_sel);
					}
					ret = EVENT_TRANSFER_STOP;
				}
				break;

			case STBK_OK:
				{
					GUI_GetProperty("box_key_scancode", "select", &box_sel);
					if(box_sel == 1)
					{
						event->key.sym = STBK_EXIT;
					}
					break;
				}
			default:
				break;
		}
	}

	return ret;
}

#endif
