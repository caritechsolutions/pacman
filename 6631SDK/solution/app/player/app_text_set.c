#include "app.h"
#include "app_windows.h"
#if THEME_CLASSIC_BLUE
//widget
#define NOTEPAD					"text_view_notepad"
#define BOX_TEXT_SET 	               "text_set_box"
#define COMBOBOX_TEXT_ROLL_LINES 	 "text_set_combo_roll_lines"
#define COMBOBOX_TEXT_AUTO_ROLL 	 "text_set_combo_auto_roll"

event_list* text_auto_roll_timer = NULL;
extern uint32_t text_roll_line_num;


 int text_roll_start(void* usrdata)
{
	if(0 == strcasecmp(WIN_TEXT_VIEW, GUI_GetFocusWindow()))
		GUI_SetProperty(NOTEPAD, "line_down", &text_roll_line_num);
	return 0;
}

SIGNAL_HANDLER  int text_set_service(const char* widgetname, void *usrdata)
{
	GUI_Event *event = NULL;

	event = (GUI_Event *)usrdata;
	GUI_SendEvent(WIN_TEXT_VIEW, event);

	return EVENT_TRANSFER_STOP;
}

static status_t key_left_right(void)
{
	uint32_t item_sel = 0;
	uint32_t value_sel = 0;
	char rows[]={1,3,5};

	GUI_GetProperty(BOX_TEXT_SET, "select", (void*)&item_sel);

	switch(item_sel)
	{
		case 0:
			GUI_GetProperty(COMBOBOX_TEXT_ROLL_LINES, "select", (void*)&value_sel);
			pmpset_set_int(PMPSET_TEXT_ROLL_LINES, value_sel);
			text_roll_line_num=rows[value_sel];
			break;

		case 1:
			GUI_GetProperty(COMBOBOX_TEXT_AUTO_ROLL, "select", (void*)&value_sel);
			pmpset_set_int(PMPSET_TEXT_AUTO_ROLL, value_sel);
			break;

		default:
			break;
	}

	app_log_debug("[TEXT] keypress_text_view_set_ok item:%d, value:%d\n", item_sel, value_sel);

	return GXCORE_SUCCESS;
}

SIGNAL_HANDLER int text_set_init(const char* widgetname, void *usrdata)
{
	int32_t value = 0;

	value = pmpset_get_int(PMPSET_TEXT_ROLL_LINES);
	GUI_SetProperty(COMBOBOX_TEXT_ROLL_LINES, "select", (void*)&value);


       value = pmpset_get_int(PMPSET_TEXT_AUTO_ROLL);
	GUI_SetProperty(COMBOBOX_TEXT_AUTO_ROLL, "select", (void*)&value);

	if(text_auto_roll_timer)
	{
		remove_timer(text_auto_roll_timer);
		text_auto_roll_timer = NULL;
	}
	return 0;
}

SIGNAL_HANDLER int text_set_destroy(const char* widgetname, void *usrdata)
{
	uint32_t value=0;

	value= pmpset_get_int(PMPSET_TEXT_AUTO_ROLL);
	if((value==PMPSET_TONE_ON) && (GUI_CheckDialog(WIN_TEXT_VIEW) == GXCORE_SUCCESS))
	{
		text_auto_roll_timer = create_timer(text_roll_start, 5000, 0, TIMER_REPEAT);
	}
      return 0;
}


SIGNAL_HANDLER int text_set_keypress(const char* widgetname, void *usrdata)
{
	GUI_Event *event = NULL;

	if(NULL == usrdata) return EVENT_TRANSFER_STOP;

	event = (GUI_Event *)usrdata;
	switch(find_virtualkey_ex(event->key.scancode,event->key.sym))
	{
		case VK_BOOK_TRIGGER:
			GUI_EndDialog("after wnd_full_screen");
			break;
		case APPK_LEFT:
		case APPK_RIGHT:
			key_left_right();
			break;

		case APPK_OK:
			//key_ok();
			break;

		default:
			break;
	}
	return EVENT_TRANSFER_KEEPON;
}
#endif

