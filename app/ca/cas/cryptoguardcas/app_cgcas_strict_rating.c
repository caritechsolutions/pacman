/*****************************************************************************
*                          CONFIDENTIAL
*        Hangzhou GuoXin Science and Technology Co., Ltd.
*                      (C)2015, All right reserved
******************************************************************************

******************************************************************************
* File Name :   app_cgcas_strict_rating.c
* Author    :   huangwf
* Project   :   CA Menu
* Type      :   Source Code
******************************************************************************
* Purpose   :
******************************************************************************
* Release History:
 VERSION      Date            AUTHOR         Description
  1.0      2021.03.23         huangwf             Creation
*****************************************************************************/

#include "app.h"
#include "app_module.h"
#ifdef CASCAM_CGCAS_SUPPORT
#include "cascam.h"
#include "cascam_ui_common.h"
#include "cascam_cryptoguardcas_data.h"
#include "app_cgcas_config.h"

#define CG_CAS_MIN_MATURITY_LEVEL   5
#define CG_CAS_MAX_MATURITY_LEVEL   20

#define POP_DIALOG_X_START          340
#define POP_DIALOG_Y_START          210
#define WND_STRICT_PARENTAL_CTRL    "wnd_cryptoguard_strict_rating"
#define XML_STRICT_PARENTAL_CTRL    "/ca/cryptoguardcas/wnd_cryptoguard_strict_rating.xml"
#define BOX_STRICT_PARENTAL_CTRL    "box_cg_strict_rating"
#define CMB_STRICT_PARENTAL_CTRL    "cmb_cg_strict_rating_level"
#define EDIT_STRICT_PARENTAL_CTRL   "edit_cg_strict_rating_pin"


#define BMP_BAR_LONG_FOCUS      "s_bar_long_focus"
#define BMP_BAR_LONG_UNFOCUS    ""
static char *s_cg_strict_rating_boxitem[] = {
	"bxi_cg_strict_rating_1",
	"bxi_cg_strict_rating_2",
};

static Cascam_CGSetStrictRating s_strict_rating_info;

static int _get_strict_rating_rating(int *rating)
{
    uint32_t sub_id = 0;
    unsigned char strict_rating = 0;

	if (NULL == rating)
	{
		printf("\nERROR, %s, %d\n",__func__, __LINE__);
		return -1;
	}
    
    sub_id = CASCAM_MSG_CGCAS_BASEID + CASCAM_CGCAS_GET_STRICT_RATING;
    if (0 == cascam_get(sub_id, (void*)&strict_rating))
	{
		*rating = strict_rating;

		return 0;
	}

	return -1;
}

static int _set_strict_rating_rating(Cascam_CGSetStrictRating *rating_info)
{
    uint32_t sub_id = 0;

	if (rating_info == NULL)
	{
		printf("\nERROR, %s, %d\n", __func__, __LINE__);
		return -1;
	}

    sub_id = CASCAM_MSG_CGCAS_BASEID + CASCAM_CGCAS_SET_STRICT_RATING;
    if(0 == cascam_set(sub_id, (void*)rating_info))
	{
		return 0;
	}

	return -1;
}

static bool _strict_rating_passwd_check_cb(void)
{
	int sel = 0;
	bool ret = true;
	int passwd_len = 0;
	char *passwd_data = NULL;

    GUI_GetProperty(EDIT_STRICT_PARENTAL_CTRL, "string", &passwd_data);
	passwd_len = strlen(passwd_data);
	if (passwd_data == NULL || passwd_len > sizeof(s_strict_rating_info.pin))
	{
		return false;
	}

	memset(&s_strict_rating_info, 0, sizeof(Cascam_CGSetStrictRating));
	GUI_GetProperty(CMB_STRICT_PARENTAL_CTRL, "select", &sel);
	s_strict_rating_info.enable = sel;
	memcpy(s_strict_rating_info.pin, passwd_data, passwd_len);

	if (_set_strict_rating_rating(&s_strict_rating_info) < 0)
	{
		ret = false;
	}
	else
	{
		ret = true;
	}

	return ret;
}

static int _strict_rating_passwd_exit_cb(bool ret)
{
	char err_notice[40] = {0};

	if(ret ==  true)
	{
		PopDlg  pop;
		memset(&pop, 0, sizeof(PopDlg));
		pop.type = POP_TYPE_NO_BTN;
		pop.str = STR_ID_SUCCESS;
		pop.timeout_sec = 1;
		pop.pos.x=POP_DIALOG_X_START;
		pop.pos.y=POP_DIALOG_Y_START;
		popdlg_create(&pop);
	}
	else if(ret ==  false)
	{
	//	app_cg_err_str_get(err_notice, sizeof(err_notice), s_strict_rating_info.ret, s_strict_rating_info.reset_num);

		PopDlg  pop;
		memset(&pop, 0, sizeof(PopDlg));
		pop.mode = POP_MODE_BLOCK;
		pop.type = POP_TYPE_OK;
		pop.str = err_notice;
		pop.pos.x=POP_DIALOG_X_START;
		pop.pos.y=POP_DIALOG_Y_START;
		popdlg_create(&pop);
		return -1;
	}

	return 0;
}

void _cg_strict_rating_passwd_dlg(void)
{
	bool ret = 0;

	ret = _strict_rating_passwd_check_cb();
	_strict_rating_passwd_exit_cb(ret);
}

static int _strict_rating_save_exit(PopDlgRet ret)
{
	if (POP_VAL_OK == ret)
	{
		_cg_strict_rating_passwd_dlg();
	}

	return 0;
}

static int _cg_strict_rating_check_save(void)
{
	int level  = 0;
	int value1 = 0;

	_get_strict_rating_rating(&level);
	GUI_GetProperty(CMB_STRICT_PARENTAL_CTRL, "select", &value1);

	if (level != value1)
	{
		PopDlg  pop;
		memset(&pop, 0, sizeof(PopDlg));
		pop.mode = POP_MODE_BLOCK;
		pop.type = POP_TYPE_YES_NO;
		pop.str = STR_ID_SAVE_INFO;
		pop.exit_cb = _strict_rating_save_exit;
		pop.pos.x=POP_DIALOG_X_START;
		pop.pos.y=POP_DIALOG_Y_START;
		popdlg_create(&pop);
		return 1;
	}
	return 0;
}

void app_cg_strict_rating_exec(void)
{
	static char init_flag = 0;
	if(0 == init_flag)
	{
		GUI_LinkDialog(WND_STRICT_PARENTAL_CTRL, XML_STRICT_PARENTAL_CTRL);
		init_flag = 1;
	}
	GUI_CreateDialog(WND_STRICT_PARENTAL_CTRL);

	int value = 0;
	_get_strict_rating_rating(&value);

	if (value == 0)
	{
		GUI_SetProperty(CMB_STRICT_PARENTAL_CTRL, "select", &value);
	}
	else
	{
		GUI_SetProperty(CMB_STRICT_PARENTAL_CTRL, "select", &value);
	}

	GUI_SetProperty(EDIT_STRICT_PARENTAL_CTRL, "default_intaglio", "-");
	GUI_SetProperty(EDIT_STRICT_PARENTAL_CTRL, "intaglio", "*");
}

SIGNAL_HANDLER int On_wnd_cg_strict_rating_destroy(GuiWidget *widget, void *usrdata)
{
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int On_wnd_cg_strict_rating_got_focus(GuiWidget *widget, void *usrdata)
{
	int box_sel = 0;
	GUI_GetProperty(BOX_STRICT_PARENTAL_CTRL, "select", &box_sel);
	GUI_SetProperty(s_cg_strict_rating_boxitem[box_sel], "unfocus_image", BMP_BAR_LONG_UNFOCUS);

	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int On_wnd_cg_strict_rating_lost_focus(GuiWidget *widget, void *usrdata)
{
	int box_sel = 0;
	GUI_GetProperty(BOX_STRICT_PARENTAL_CTRL, "select", &box_sel);
	GUI_SetProperty(s_cg_strict_rating_boxitem[box_sel], "unfocus_image", BMP_BAR_LONG_FOCUS);

	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int On_wnd_cg_strict_rating_keypress(GuiWidget *widget, void *usrdata)
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
					GUI_EndDialog(WND_STRICT_PARENTAL_CTRL);
					break;
				case STBK_OK:
					_cg_strict_rating_check_save();
					break;
				default:
					break;
			}
		default:
			break;
	}
	return ret;
}

#else
SIGNAL_HANDLER int On_wnd_cg_strict_rating_destroy(GuiWidget *widget, void *usrdata)
{
	return EVENT_TRANSFER_STOP;
}
SIGNAL_HANDLER int On_wnd_cg_strict_rating_got_focus(GuiWidget *widget, void *usrdata)
{
	return EVENT_TRANSFER_STOP;
}
SIGNAL_HANDLER int On_wnd_cg_strict_rating_lost_focus(GuiWidget *widget, void *usrdata)
{
	return EVENT_TRANSFER_STOP;
}
SIGNAL_HANDLER int On_wnd_cg_strict_rating_keypress(GuiWidget *widget, void *usrdata)
{
	return EVENT_TRANSFER_STOP;
}
#endif
