/*****************************************************************************
*                          CONFIDENTIAL
*        Hangzhou GuoXin Science and Technology Co., Ltd.
*                      (C)2015, All right reserved
******************************************************************************

******************************************************************************
* File Name :   app_cgcas_pin.c
* Author    :   huangwf
* Project   :   CA Menu
* Type      :   Source Code
******************************************************************************
* Purpose   :
******************************************************************************
* Release History:
 VERSION      Date            AUTHOR         Description
  1.0      2021.03.19         huangwf             Creation
*****************************************************************************/

#include "app.h"
#include "app_module.h"
#ifdef CASCAM_CGCAS_SUPPORT
#include "cascam.h"
#include "cascam_ui_common.h"
#include "cascam_cryptoguardcas_data.h"
#include "app_cgcas_config.h"

#define PIN_MAX_LEN                 4

#define POP_DIALOG_X_START          340
#define POP_DIALOG_Y_START          210

#define WID_PIN                     "wnd_cryptoguard_pin"
#define XML_PIN                     "ca/cryptoguardcas/wnd_cryptoguard_pin.xml"

#define BOX_PIN                     "box_cg_pin"
#define EDIT_PIN_OLD                "edit_cg_pin_old"
#define EDIT_PIN_NEW                "edit_cg_pin_new"
#define EDIT_PIN_CONFIRM            "edit_cg_pin_confirm"

#define BMP_BAR_LONG_FOCUS          "s_bar_long_focus"
#define BMP_BAR_LONG_UNFOCUS        ""
static char *s_cg_pin_boxitem[] = {
	"bxi_cg_pin_1",
	"bxi_cg_pin_2",
	"bxi_cg_pin_3",
};

//static CascamAbvcasChangePin s_pin_info;
/*
static int _set_pin(char *pin_info)
{
	if(pin_info == NULL)
	{
		printf("ERROR, pin_info NULL. %s[%d]\n", __func__, __LINE__);
		return -1;
	}

	return 0;//app_cg_set_pin(pin_info);
}
*/
static int _passwd_save(void)
{
	int ret = 0;
	char err_notice[40] = {0};
	int err_len = sizeof(err_notice);
    uint32_t sub_id = 0;

	char *old_pin = NULL;
	char *new_pin = NULL;
	char *confirm_pin = NULL;

	GUI_GetProperty(EDIT_PIN_OLD, "string", &old_pin);
	GUI_GetProperty(EDIT_PIN_NEW, "string", &new_pin);
	GUI_GetProperty(EDIT_PIN_CONFIRM, "string", &confirm_pin);

	int old_len = strlen(old_pin);
	int new_len = strlen(new_pin);
	int confirm_len = strlen(confirm_pin);

	if(old_pin == NULL || new_pin == NULL || confirm_pin == NULL
			|| old_len == 0 || new_len == 0 || confirm_len == 0
			|| new_len != confirm_len || strncmp(new_pin, confirm_pin, new_len) != 0
			|| old_len > PIN_MAX_LEN || new_len > PIN_MAX_LEN)
	{
		ret = -1;
		snprintf(err_notice, err_len, STR_ID_ERR_PASSWD);
	}
	else
	{
	    Cascam_CGChangePin s_pin_info;

        memset(&s_pin_info, 0, sizeof(s_pin_info));
        memcpy(s_pin_info.old_pin, old_pin, old_len);
        memcpy(s_pin_info.new_pin, new_pin, new_len);

        sub_id = CASCAM_MSG_CGCAS_BASEID + CASCAM_CGCAS_SET_PIN;
		ret = cascam_set(sub_id, (void *)&s_pin_info);
		if(ret == 0)
		{
			snprintf(err_notice, err_len, STR_ID_SUCCESS);
		}
		else
		{
	//		app_cg_err_str_get(err_notice, sizeof(err_notice), s_pin_info.ret, s_pin_info.reset_num);
		}
	}

	GUI_SetProperty(EDIT_PIN_OLD, "clear", NULL);
	GUI_SetProperty(EDIT_PIN_NEW, "clear", NULL);
	GUI_SetProperty(EDIT_PIN_CONFIRM, "clear", NULL);

	PopDlg pop;
	memset(&pop, 0, sizeof(PopDlg));
	pop.type = POP_TYPE_NO_BTN;
	pop.format = POP_FORMAT_DLG;
	pop.mode = POP_MODE_UNBLOCK;
	pop.timeout_sec = 3;
	pop.str = err_notice;
	pop.pos.x = POP_DIALOG_X_START;
	pop.pos.y = POP_DIALOG_Y_START;
	popdlg_create(&pop);

	return ret;
}

void app_cg_pin_exec(void)
{
	static char init_flag = 0;
	if(0 == init_flag)
	{
		GUI_LinkDialog(WID_PIN, XML_PIN);
		init_flag = 1;
	}
	GUI_CreateDialog(WID_PIN);

	static char edit_len[PIN_MAX_LEN] = {0};
	snprintf(edit_len , sizeof(edit_len), "%d", PIN_MAX_LEN);

	GUI_SetProperty(EDIT_PIN_OLD, "maxlen", edit_len);
	GUI_SetProperty(EDIT_PIN_OLD, "default_intaglio", "-");
	GUI_SetProperty(EDIT_PIN_OLD, "intaglio", "*");

	GUI_SetProperty(EDIT_PIN_NEW, "maxlen", edit_len);
	GUI_SetProperty(EDIT_PIN_NEW, "default_intaglio", "-");
	GUI_SetProperty(EDIT_PIN_NEW, "intaglio", "*");

	GUI_SetProperty(EDIT_PIN_CONFIRM, "maxlen", edit_len);
	GUI_SetProperty(EDIT_PIN_CONFIRM, "default_intaglio", "-");
	GUI_SetProperty(EDIT_PIN_CONFIRM, "intaglio", "*");
}

// for ui
SIGNAL_HANDLER int On_wnd_cg_pin_destroy(GuiWidget *widget, void *usrdata)
{
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int On_wnd_cg_pin_got_focus(GuiWidget *widget, void *usrdata)
{
	int box_sel = 0;
	GUI_GetProperty(BOX_PIN, "select", &box_sel);
	GUI_SetProperty(s_cg_pin_boxitem[box_sel], "unfocus_image", BMP_BAR_LONG_UNFOCUS);

	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int On_wnd_cg_pin_lost_focus(GuiWidget *widget, void *usrdata)
{
	int box_sel = 0;
	GUI_GetProperty(BOX_PIN, "select", &box_sel);
	GUI_SetProperty(s_cg_pin_boxitem[box_sel], "unfocus_image", BMP_BAR_LONG_FOCUS);
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int On_wnd_cg_pin_keypress(GuiWidget *widget, void *usrdata)
{
	int ret = EVENT_TRANSFER_STOP;
	GUI_Event *event = NULL;

	event = (GUI_Event *)usrdata;
	switch(event->type)
	{
		case GUI_SERVICE_MSG:
			break;
		case GUI_KEYDOWN:
			switch(find_virtualkey_ex(event->key.scancode,event->key.sym))
			{
				case VK_BOOK_TRIGGER:
					GUI_EndDialog("after wnd_full_screen");
					break;
				case STBK_EXIT:
				case STBK_MENU:
					GUI_EndDialog(WID_PIN);
					break;
				case STBK_OK:
					_passwd_save();
					break;

				default:
					break;
			}
		default:
			break;
	}
	return ret;
}

SIGNAL_HANDLER int On_wnd_cg_pin_old_reach_end(GuiWidget *widget, void *usrdata)
{
	int box_sel = 1;
	GUI_SetProperty(BOX_PIN, "select", &box_sel);
	return EVENT_TRANSFER_KEEPON;
}

SIGNAL_HANDLER int On_wnd_cg_pin_new_reach_end(GuiWidget *widget, void *usrdata)
{
	int box_sel = 2;
	GUI_SetProperty(BOX_PIN, "select", &box_sel);
	return EVENT_TRANSFER_KEEPON;
}

SIGNAL_HANDLER int On_wnd_cg_pin_confirmpasswd_reach_end(GuiWidget *widget, void *usrdata)
{
	return EVENT_TRANSFER_KEEPON;
}

SIGNAL_HANDLER int On_wnd_cg_pin_confirmpasswd_keypress(GuiWidget *widget, void *usrdata)
{
	return EVENT_TRANSFER_KEEPON;
}
#endif
