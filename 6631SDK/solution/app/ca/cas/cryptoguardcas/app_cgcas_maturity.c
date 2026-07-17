/*****************************************************************************
*                          CONFIDENTIAL
*        Hangzhou GuoXin Science and Technology Co., Ltd.
*                      (C)2015, All right reserved
******************************************************************************

******************************************************************************
* File Name :   app_cgcas_maturity.c
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
#include "cascam_ui_pop.h"
#include "cascam_ui_porting.h"

#define CG_CAS_MIN_MATURITY_LEVEL   5
#define CG_CAS_MAX_MATURITY_LEVEL   20

#define POP_DIALOG_X_START      340
#define POP_DIALOG_Y_START      210
#define WND_MATURITY            "wnd_cryptoguard_maturity"
#define XML_MATURITY            "/ca/cryptoguardcas/wnd_cryptoguard_maturity.xml"
#define BOX_MATURITY            "box_cg_maturity"
#define CMB_MATURITY_LEVEL      "cmb_cg_maturity_level"
#define EDIT_MATURITY_PIN       "edit_cg_maturity_pin"


#define BMP_BAR_LONG_FOCUS      "s_bar_long_focus"
#define BMP_BAR_LONG_UNFOCUS    ""
static char *s_cg_maturity_boxitem[] = {
	"bxi_cg_maturity_1",
	"bxi_cg_maturity_2",
};

static void _set_maturity_rating_content(void)
{
#define TEMP_LEN 16

	int i = 0;
	char buffer[TEMP_LEN];
	static char content[256];
	memset(content, 0, sizeof(content));

	strcat(content, "[");
	for(i = CG_CAS_MIN_MATURITY_LEVEL; i <= CG_CAS_MAX_MATURITY_LEVEL; i++)
	{
		memset(buffer, 0, TEMP_LEN);
		snprintf(buffer, TEMP_LEN, "%d Age,", i);
		strcat(content, buffer);
	}
	//strcat(content, ABVCAS_RES_STR_NO_RESTRICTION);
	strcat(content, "]");
	GUI_SetProperty(CMB_MATURITY_LEVEL, "content", content);
}


static Cascam_CGChangeRating s_rating_info;

static int _get_maturity_rating(int *rating)
{
    uint32_t sub_id = 0;
    unsigned char maturity_rating = 0;

	if (NULL == rating)
	{
		printf("\nERROR, %s, %d\n",__func__, __LINE__);
		return -1;
	}

    sub_id = CASCAM_MSG_CGCAS_BASEID + CASCAM_CGCAS_GET_MATURITY_RATING;
    if (0 == cascam_get(sub_id, (void*)&maturity_rating))
	{
		*rating = maturity_rating;
		if (*rating < CG_CAS_MIN_MATURITY_LEVEL || *rating > CG_CAS_MAX_MATURITY_LEVEL)
			*rating = CG_CAS_MAX_MATURITY_LEVEL + 1;

		return 0;
	}

	return -1;
}

static int _set_maturity_rating(Cascam_CGChangeRating *rating_info)
{
    uint32_t sub_id = 0;

	if (rating_info == NULL)
	{
		printf("\nERROR, %s, %d\n", __func__, __LINE__);
		return -1;
	}

    sub_id = CASCAM_MSG_CGCAS_BASEID + CASCAM_CGCAS_SET_MATURITY_RATING;
    if(0 == cascam_set(sub_id, (void*)rating_info))
	{
		return 0;
	}

	return -1;
}

static bool _maturity_passwd_check_cb(void)
{
	int sel = 0;
	bool ret = true;
	int passwd_len = 0;
	char *passwd_data = NULL;

    GUI_GetProperty(EDIT_MATURITY_PIN, "string", &passwd_data);
	passwd_len = strlen(passwd_data);
	if (passwd_data == NULL || passwd_len != sizeof(s_rating_info.pin))
	{
		return false;
	}

	memset(&s_rating_info, 0, sizeof(Cascam_CGChangeRating));
	GUI_GetProperty(CMB_MATURITY_LEVEL, "select", &sel);
	s_rating_info.rating = sel + CG_CAS_MIN_MATURITY_LEVEL;
	memcpy(s_rating_info.pin, passwd_data, passwd_len);

	if (_set_maturity_rating(&s_rating_info) < 0)
	{
		ret = false;
	}
	else
	{
		ret = true;
	}

	return ret;
}

static int _maturity_passwd_exit_cb(bool ret)
{
//	char err_notice[40] = {0};

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
	//	app_cg_err_str_get(err_notice, sizeof(err_notice), s_rating_info.ret, s_rating_info.reset_num);

		PopDlg  pop;
		memset(&pop, 0, sizeof(PopDlg));
		pop.mode = POP_MODE_BLOCK;
		pop.type = POP_TYPE_OK;
		pop.str = "Password Error";
		pop.pos.x=POP_DIALOG_X_START;
		pop.pos.y=POP_DIALOG_Y_START;
		popdlg_create(&pop);
		return -1;
	}

	return 0;
}

void _cg_maturity_passwd_dlg(void)
{
	bool ret = 0;

	ret = _maturity_passwd_check_cb();
	_maturity_passwd_exit_cb(ret);
}

static int _maturity_save_exit(PopDlgRet ret)
{
	if(POP_VAL_OK == ret)
	{// need to save
		_cg_maturity_passwd_dlg();
	}

	return 0;
}

static int _cg_maturity_check_save(void)
{
	int level  = 0;
	int value1 = 0;

	_get_maturity_rating(&level);
	GUI_GetProperty(CMB_MATURITY_LEVEL, "select", &value1);

	if(level != value1 + CG_CAS_MIN_MATURITY_LEVEL)
	{// need to save
	#if 0
		PopDlg  pop;
		memset(&pop, 0, sizeof(PopDlg));
		pop.mode = POP_MODE_BLOCK;
		pop.type = POP_TYPE_YES_NO;
		pop.str = STR_ID_SAVE_INFO;
		pop.exit_cb = _maturity_save_exit;
		pop.pos.x=POP_DIALOG_X_START;
		pop.pos.y=POP_DIALOG_Y_START;
		popdlg_create(&pop);
		return 1;
	#endif
		_maturity_save_exit(POP_VAL_OK);
	}
	return 0;
}

void app_cg_maturity_exec(void)
{
	static char init_flag = 0;
	if(0 == init_flag)
	{
		GUI_LinkDialog(WND_MATURITY, XML_MATURITY);
		init_flag = 1;
	}
	GUI_CreateDialog(WND_MATURITY);

	_set_maturity_rating_content();

	int value = 0;
	_get_maturity_rating(&value);
	value -= CG_CAS_MIN_MATURITY_LEVEL;
	if(value >= 0)
	{
		GUI_SetProperty(CMB_MATURITY_LEVEL, "select", &value);
	}
	else
	{
		printf("\nCASCAM, error, %s, %d\n", __func__, __LINE__);
	}

	GUI_SetProperty(EDIT_MATURITY_PIN, "default_intaglio", "-");
	GUI_SetProperty(EDIT_MATURITY_PIN, "intaglio", "*");
}

SIGNAL_HANDLER int On_wnd_cg_maturity_destroy(GuiWidget *widget, void *usrdata)
{
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int On_wnd_cg_maturity_got_focus(GuiWidget *widget, void *usrdata)
{
	int box_sel = 0;
	GUI_GetProperty(BOX_MATURITY, "select", &box_sel);
	GUI_SetProperty(s_cg_maturity_boxitem[box_sel], "unfocus_image", BMP_BAR_LONG_UNFOCUS);

	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int On_wnd_cg_maturity_lost_focus(GuiWidget *widget, void *usrdata)
{
	int box_sel = 0;
	GUI_GetProperty(BOX_MATURITY, "select", &box_sel);
	GUI_SetProperty(s_cg_maturity_boxitem[box_sel], "unfocus_image", BMP_BAR_LONG_FOCUS);

	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int On_wnd_cg_maturity_keypress(GuiWidget *widget, void *usrdata)
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
					GUI_EndDialog(WND_MATURITY);
					break;
				case STBK_OK:
					_cg_maturity_check_save();
					break;
				default:
					break;
			}
		default:
			break;
	}
	return ret;
}

/*************************************************************************************************/
static bool maturity_passwd_check(char* passwd_buf,int len)
{
    bool ret = false;
    uint32_t sub_id = 0;
    sub_id = CASCAM_MSG_CGCAS_BASEID + CASCAM_CGCAS_UNLOCK_PIN;
    if(1 == cascam_set(sub_id, (void*)passwd_buf))
    {
        return true;
    }
    return ret;
}
static int maturity_passwd_cb(CascamPopPwdRet *ret, void *usr_data)
{
    if(ret->result == CASCAM_PASSWD_OK)
    {
		printf("CASCAM_PASSWD_OK [%s][%d]\n",__FUNCTION__,__LINE__);
    }
    else if (ret->result == CASCAM_PASSWD_ERROR)
    {
        CascamPasswdDlg passwd_dlg;
        memset(&passwd_dlg, 0, sizeof(CascamPasswdDlg));
        passwd_dlg.str = STR_ID_PASSWD;
        passwd_dlg.passwd_check_cb = maturity_passwd_check;
        passwd_dlg.usr_data = usr_data;
        passwd_dlg.passwd_key_msg = KEY_MSG_STOP;
        passwd_dlg.passwd_exit_cb = maturity_passwd_cb;
        passwd_dlg.pos.x = POP_PASSWD_POS_X;
        passwd_dlg.pos.y = POP_PASSWD_POS_Y;
        flexible_passwd_dlg_create(&passwd_dlg);
    }
    return 0;
}

void app_cgcas_pin_required_notify(void* p)
{
    Cascam_CGPinRequired cgcas_message;

    if (NULL == p)
    {
        printf("%s,%d, data is NULL!\n", __func__, __LINE__);
        return ;
    }

    memset(&cgcas_message, 0, sizeof(Cascam_CGPinRequired));
    memcpy(&cgcas_message, p, sizeof(Cascam_CGPinRequired));
    int value = 0;
    _get_maturity_rating(&value);

#if 0
    if(cgcas_message.rating != 0) // timeout
    {
        PopDlg pop;
        memset(&pop, 0, sizeof(PopDlg));
        pop.type = POP_TYPE_NO_BTN;
        pop.format = POP_FORMAT_DLG;
        pop.str = cgcas_message.text;
        pop.mode = POP_MODE_UNBLOCK;
        pop.create_cb = NULL;
        pop.exit_cb = NULL;
        pop.timeout_sec = cgcas_message.rating;
        popdlg_create(&pop);
    }
    else
    {
        PopDlg pop;
        memset(&pop, 0, sizeof(PopDlg));
        pop.type = POP_TYPE_OK;
        pop.format = POP_FORMAT_DLG;
        pop.str = cgcas_message.text;
        pop.mode = POP_MODE_BLOCK;
        pop.create_cb = NULL;
        pop.exit_cb = NULL;
        popdlg_create(&pop);
    }
#else
    CascamPasswdDlg passwd_dlg;
    memset(&passwd_dlg, 0, sizeof(CascamPasswdDlg));
    passwd_dlg.str = STR_ID_PASSWD;
    passwd_dlg.passwd_check_cb = maturity_passwd_check;
    passwd_dlg.passwd_key_msg = KEY_MSG_STOP;
    passwd_dlg.passwd_exit_cb = maturity_passwd_cb;
    passwd_dlg.pos.x = POP_PASSWD_POS_X;
    passwd_dlg.pos.y = POP_PASSWD_POS_Y;
    flexible_passwd_dlg_create(&passwd_dlg);
#endif
}
#else
SIGNAL_HANDLER int On_wnd_cg_maturity_destroy(GuiWidget *widget, void *usrdata)
{
	return EVENT_TRANSFER_STOP;
}
SIGNAL_HANDLER int On_wnd_cg_maturity_got_focus(GuiWidget *widget, void *usrdata)
{
	return EVENT_TRANSFER_STOP;
}
SIGNAL_HANDLER int On_wnd_cg_maturity_lost_focus(GuiWidget *widget, void *usrdata)
{
	return EVENT_TRANSFER_STOP;
}
SIGNAL_HANDLER int On_wnd_cg_maturity_keypress(GuiWidget *widget, void *usrdata)
{
	return EVENT_TRANSFER_STOP;
}
#endif
