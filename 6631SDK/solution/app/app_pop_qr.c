#include "app.h"
#include "app_module.h"
#include "app_pop.h"
#include "full_screen.h"
#include "app_windows.h"
#include "app_channel_info.h"
#include "app_qrencode.h"
#include "app_pop_qr.h"

#if QR_POP_SUPPORT
#define TEXT_POP_QR_TITLE "txt_pop_qr_title"
#define IMG_POP_QR_QRCODE "img_pop_qr_qrcode"

static PopQrParas s_qr_params = {0};
static event_list* pop_qr_timer = NULL;
static unsigned int pop_qr_handle = 0;

static int _timer_out(void *userdata)
{
	if(pop_qr_handle>0)
	{
		qren_encode_show(pop_qr_handle, IMG_POP_QR_QRCODE);
	}
	GUI_SetInterface("flush", NULL);
	pop_qr_timer = NULL;
	return 0;
}

static void _timer_stop(void)
{
	if(pop_qr_timer!= NULL)
		timer_stop(pop_qr_timer);
}

static void _timer_reset(void)
{
	if(pop_qr_timer != NULL)
		reset_timer(pop_qr_timer);
}

SIGNAL_HANDLER int app_pop_qr_create(GuiWidget *widget, void *usrdata)
{
	int32_t pos_x = 0;
	int32_t pos_y = 0;

	if(s_qr_params.str_title)
	{
		GUI_SetProperty(TEXT_POP_QR_TITLE, "string", s_qr_params.str_title);
		GUI_SetProperty(TEXT_POP_QR_TITLE, "state", "show");
	}
	else
	{
		GUI_SetProperty(TEXT_POP_QR_TITLE, "state", "hide");
	}
	if(s_qr_params.str_content)
	{
		pop_qr_handle = qren_encode_init(s_qr_params.str_content, 16);
		pop_qr_timer = create_timer(_timer_out, 100, NULL, TIMER_ONCE);
	}
	if((s_qr_params.pos.x == 0) && (s_qr_params.pos.y == 0))
	{
		pos_x = APP_WND_POP_QR_POS_X;
		pos_y = APP_WND_POP_QR_POS_Y;
	}
	else
	{
		pos_x = s_qr_params.pos.x;
		pos_y = s_qr_params.pos.y;
	}
	GUI_SetProperty(WND_POP_QR, "move_window_x", &pos_x);
	GUI_SetProperty(WND_POP_QR, "move_window_y", &pos_y);

	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_pop_qr_destroy(GuiWidget *widget, void *usrdata)
{
	APP_TIMER_REMOVE(pop_qr_timer);
	if(pop_qr_handle>0)
	{
		qren_encode_close(pop_qr_handle);
		pop_qr_handle = 0;
	}
	memset(&s_qr_params, 0, sizeof(PopQrParas));
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_pop_qr_got_focus(GuiWidget *widget, void *usrdata)
{
	_timer_reset();
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_pop_qr_lost_focus(GuiWidget *widget, void *usrdata)
{
	_timer_stop();
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_pop_qr_keypress(GuiWidget *widget, void *usrdata)
{
	int ret = EVENT_TRANSFER_STOP;
	GUI_Event *event = NULL;

	event = (GUI_Event *)usrdata;
	switch(event->type)
	{
		case GUI_SERVICE_MSG:
			break;

		case GUI_MOUSEBUTTONDOWN:
			break;

		case GUI_KEYDOWN:
			switch(event->key.sym)
			{
				case VK_BOOK_TRIGGER:
					GUI_EndDialog("after wnd_full_screen");
					if(g_AppFullArb.state.tv == STATE_OFF)
					{
						app_channel_info_create_dialog();
					}
					break;

				case STBK_EXIT:
				case STBK_MENU:
					if(s_qr_params.forbidExitKey>0)
						break;
					QrExitCb exit_cb = s_qr_params.exit_cb;
					GUI_EndDialog(WND_POP_QR);
					if(exit_cb)
					{
						exit_cb();
					}
					if ((app_channel_info_check_dialog() != GXCORE_SUCCESS && GUI_CheckDialog(WND_MAIN_MENU) != GXCORE_SUCCESS)
					&&(g_AppPlayOps.normal_play.play_total != 0))
						app_channel_info_create_dialog();
					break;

				default:
					break;
			}
			break;

		default:
			break;
	}

	return ret;
}

void app_pop_create_qr(PopQrParas *paras)
{
	if(GUI_CheckDialog(WND_POP_QR) == GXCORE_ERROR)
	{
		memcpy(&s_qr_params, paras, sizeof(PopQrParas));
		GUI_CreateDialog(WND_POP_QR);
	}
}

void app_pop_end_qr(void)
{
	if(GUI_CheckDialog(WND_POP_QR) == GXCORE_SUCCESS)
	{
		GUI_EndDialog(WND_POP_QR);
	}
}

#endif
