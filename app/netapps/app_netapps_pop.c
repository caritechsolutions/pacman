#include "app.h"
#if NETAPPS_SUPPORT
#include "app_module.h"
#include "app_windows.h"
#include "app_pop.h"
#include "app_pop_qr.h"
#include "app_common_view.h"
#include "app_netapps_pop.h"

typedef struct
{
	char *title;
	char *content;
}PopText;

static PopCreateCb netapps_pdlg_create_cb = NULL;
static PopExitCb netapps_pdlg_exit_cb = NULL;
static PoplistExitCb netapps_plist_exit_cb = NULL;
static PwExitCb netapps_pw_exit_cb = NULL;
#if QR_POP_SUPPORT
static PqrExitCb netapps_pqr_exit_cb = NULL;
#endif
static KbChangeCb netapps_kb_change_cb = NULL;
static KbReleaseCb netapps_kb_release_cb = NULL;

unsigned int app_netapps_popdlg_malloc(void)
{
	PopDlg *dlg = NULL;

	dlg = (PopDlg *)GxCore_Calloc(1, sizeof(PopDlg));
	if(dlg)
	{
		memset(dlg, 0 ,sizeof(PopDlg));
		dlg->mode = POP_MODE_UNBLOCK;
		dlg->format = POP_FORMAT_DLG;
		return (unsigned int)dlg;
	}

	return 0;
}

void app_netapps_popdlg_free(unsigned int handle)
{
	PopDlg *dlg = NULL;

	dlg = (PopDlg *)handle;
	if(dlg)
	{
		GxCore_Free(dlg);
		dlg = NULL;
	}
}

void app_netapps_popdlg_set_button_type(unsigned int handle, PdlgBtnType type)
{
	PopDlg *dlg = NULL;

	dlg = (PopDlg *)handle;
	if(dlg)
	{
		if(type == PDLG_BTN_YES_NO)
		{
			dlg->type = POP_TYPE_YES_NO;
		}
		else if(type == PDLG_BTN_OK)
		{
			dlg->type = POP_TYPE_OK;
		}
		else if(type == PDLG_BTN_NONE)
		{
			dlg->type = POP_TYPE_NO_BTN;
		}
	}
}

void app_netapps_popdlg_set_title(unsigned int handle, char *title)
{
	PopDlg *dlg = NULL;

	dlg = (PopDlg *)handle;
	if(dlg)
	{
		dlg->title = title;
	}
}

void app_netapps_popdlg_set_content(unsigned int handle, char *content)
{
	PopDlg *dlg = NULL;

	dlg = (PopDlg *)handle;
	if(dlg)
	{
		dlg->str = content;
	}
}

void app_netapps_popdlg_set_position(unsigned int handle, int pos_x, int pos_y)
{
	PopDlg *dlg = NULL;

	dlg = (PopDlg *)handle;
	if(dlg&&(pos_x>=0)&&(pos_y>=0))
	{
		dlg->pos.x = pos_x;
		dlg->pos.y = pos_y;
	}
}

void app_netapps_popdlg_set_timeout(unsigned int handle, int sec)
{
	PopDlg *dlg = NULL;

	dlg = (PopDlg *)handle;
	if(dlg&&(sec>=0))
	{
		dlg->timeout_sec = sec;
	}
}

void app_netapps_popdlg_set_default_ret(unsigned int handle, PopRetType ret)
{
	PopDlg *dlg = NULL;

	dlg = (PopDlg *)handle;
	if(dlg)
	{
		if(ret == POP_RET_OK)
		{
			dlg->default_ret = POP_VAL_OK;
		}
		else if(ret == POP_RET_CANCEL)
		{
			dlg->default_ret = POP_VAL_CANCEL;
		}
	}
}

void app_netapps_popdlg_set_time_display(unsigned int handle, int enable)
{
	PopDlg *dlg = NULL;

	dlg = (PopDlg *)handle;
	if(dlg)
	{
		if(enable>0)
		{
			dlg->show_time = true;
		}
		else
		{
			dlg->show_time = false;
		}
	}
}

static int app_netapps_pdlg_create_cb(void)
{
	if(netapps_pdlg_create_cb)
	{
		netapps_pdlg_create_cb();
	}

	return 0;
}

void app_netapps_popdlg_set_create_cb(unsigned int handle, PopCreateCb cb)
{
	PopDlg *dlg = NULL;

	dlg = (PopDlg *)handle;
	if(dlg)
	{
		dlg->create_cb = app_netapps_pdlg_create_cb;
		netapps_pdlg_create_cb = cb;
	}
}

static int app_netapps_pdlg_exit_cb(PopDlgRet ret)
{
	PopRetType pdlg_ret = POP_RET_OK;

	if(ret == POP_VAL_OK)
	{
		pdlg_ret = POP_RET_OK;
	}
	else
	{
		pdlg_ret = POP_RET_CANCEL;
	}
	if(netapps_pdlg_exit_cb)
	{
		netapps_pdlg_exit_cb(pdlg_ret);
	}

	return 0;
}

void app_netapps_popdlg_set_exit_cb(unsigned int handle, PopExitCb cb)
{
	PopDlg *dlg = NULL;

	dlg = (PopDlg *)handle;
	if(dlg)
	{
		dlg->exit_cb = app_netapps_pdlg_exit_cb;
		netapps_pdlg_exit_cb = cb;
	}
}

PopRetType app_netapps_popdlg_create(unsigned int handle)
{
	PopDlg *dlg = NULL;
	PopRetType ret  = POP_RET_CANCEL;

	dlg = (PopDlg *)handle;
	if(dlg)
	{
		if(popdlg_create(dlg) == POP_VAL_OK)
		{
			ret = POP_RET_OK;
		}
		else
		{
			ret = POP_RET_CANCEL;
		}
	}

	return ret;
}

void app_netapps_popdlg_destroy(unsigned int handle)
{
	popdlg_destroy();
}

unsigned int app_netapps_keyboard_malloc(int max_num)
{
	PopKeyboard *kb = NULL;

	kb = (PopKeyboard *)GxCore_Calloc(1, sizeof(PopKeyboard));
	if(kb)
	{
		memset(kb, 0 ,sizeof(PopKeyboard));
		kb->max_num = max_num;
		return (unsigned int)kb;
	}

	return 0;
}

void app_netapps_keyboard_free(unsigned int handle)
{
	PopKeyboard *kb = NULL;

	kb = (PopKeyboard *)handle;
	if(kb)
	{
		GxCore_Free(kb);
		kb = NULL;
	}
}

void app_netapps_keyboard_set_instr(unsigned int handle, char *instr)
{
	PopKeyboard *kb = NULL;

	kb = (PopKeyboard *)handle;
	if(kb)
	{
		kb->in_name = instr;
	}
}

void app_netapps_keyboard_set_position(unsigned int handle, int pos_x, int pos_y)
{
	PopKeyboard *kb = NULL;

	kb = (PopKeyboard *)handle;
	if(kb)
	{
		if(pos_x>=0)
		{
			kb->pos.x = pos_x;
		}
		if(pos_y>=0)
		{
			kb->pos.y = pos_y;
		}
	}
}

static void app_netapps_keyboard_change_cb(PopKeyboard *data)
{
	if(netapps_kb_change_cb&&data)
	{
		netapps_kb_change_cb(data->out_name);
	}
}

void app_netapps_keyboard_set_change_cb(unsigned int handle, KbChangeCb cb)
{
	PopKeyboard *kb = NULL;

	kb = (PopKeyboard *)handle;
	if(kb)
	{
		kb->change_cb = app_netapps_keyboard_change_cb;
		netapps_kb_change_cb = cb;
	}
}

static void app_netapps_keyboard_release_cb(PopKeyboard *data)
{
	PopRetType ret = POP_RET_OK;

	if(netapps_kb_release_cb&&data)
	{
		if(data->in_ret == POP_VAL_OK)
		{
			ret = POP_RET_OK;
		}
		else
		{
			ret = POP_RET_CANCEL;
		}
		netapps_kb_release_cb(ret, data->out_name);
	}
}

void app_netapps_keyboard_set_release_cb(unsigned int handle, KbReleaseCb cb)
{
	PopKeyboard *kb = NULL;

	kb = (PopKeyboard *)handle;
	if(kb)
	{
		kb->release_cb = app_netapps_keyboard_release_cb;
		netapps_kb_release_cb = cb;
	}
}

void app_netapps_keyboard_disable_chars(unsigned int handle, char *chars)
{
	PopKeyboard *kb = NULL;

	kb = (PopKeyboard *)handle;
	if(kb&&chars)
	{
		kb->usr_data = chars;
	}
}

PopRetType app_netapps_keyboard_create(unsigned int handle)
{
	PopKeyboard *kb = NULL;

	kb = (PopKeyboard *)handle;
	if(kb)
	{
		if(multi_language_keyboard_create(kb) == GXCORE_SUCCESS)
		{
			return POP_RET_OK;
		}
	}

	return POP_RET_CANCEL;
}

void app_netapps_keyboard_destroy(unsigned int handle)
{
	if(GUI_CheckDialog(WND_KEYBOARD_LANGUAGE) == GXCORE_SUCCESS)
	{
		GUI_EndDialog(WND_KEYBOARD_LANGUAGE);
	}
}

unsigned int app_netapps_poplist_malloc(int item_num)
{
	PopList *list = NULL;

	list = (PopList *)GxCore_Calloc(1, sizeof(PopList));
	if(list)
	{
		memset(list, 0 ,sizeof(PopList));
		list->pos.x = APP_NETAPP_SETTING_POP_LIST_X;
		list->pos.y = APP_NETAPP_SETTING_POP_LIST_Y;
		list->mode = POP_MODE_UNBLOCK;
		list->item_num = item_num;
		return (unsigned int)list;
	}

	return 0;
}

void app_netapps_poplist_free(unsigned int handle)
{
	PopList *list = NULL;

	list = (PopList *)handle;
	if(list)
	{
		GxCore_Free(list);
		list = NULL;
	}
}

void app_netapps_poplist_set_title(unsigned int handle, char *title)
{
	PopList *list = NULL;

	list = (PopList *)handle;
	if(list)
	{
		list->title = title;
	}
}

void app_netapps_poplist_set_content(unsigned int handle, char **content)
{
	PopList *list = NULL;

	list = (PopList *)handle;
	if(list)
	{
		list->item_content = content;
	}
}

void app_netapps_poplist_set_position(unsigned int handle, int pos_x, int pos_y)
{
	PopList *list = NULL;

	list = (PopList *)handle;
	if(list)
	{
		if(pos_x>=0)
		{
			list->pos.x = pos_x;
		}
		if(pos_y>=0)
		{
			list->pos.y = pos_y;
		}
	}
}

void app_netapps_poplist_set_sel(unsigned int handle, int sel)
{
	PopList *list = NULL;

	list = (PopList *)handle;
	if(list)
	{
		list->sel = sel;
	}
}

void app_netapps_poplist_set_index_display(unsigned int handle, int enable)
{
	PopList *list = NULL;

	list = (PopList *)handle;
	if(list)
	{
		if(enable>0)
		{
			list->show_num = true;
		}
		else
		{
			list->show_num = false;
		}
	}
}

static int app_netapps_plist_exit_cb(int32_t ret_sel, unsigned short key)
{
	PopRetType ret = POP_RET_OK;

	if(key == STBK_OK)
	{
		ret = POP_RET_OK;
	}
	else
	{
		ret = POP_RET_CANCEL;
	}
	if(netapps_plist_exit_cb)
	{
		netapps_plist_exit_cb(ret, ret_sel);
	}

	return 0;
}

void app_netapps_poplist_set_exit_cb(unsigned int handle, PoplistExitCb cb)
{
	PopList *list = NULL;

	list = (PopList *)handle;
	if(list)
	{
		list->exit_cb = app_netapps_plist_exit_cb;
		netapps_plist_exit_cb = cb;
	}
}

PopRetType app_netapps_poplist_create(unsigned int handle)
{
	PopList *list = NULL;

	list = (PopList *)handle;
	if(list)
	{
		poplist_create(list);
	}

	return POP_RET_OK;
}

void app_netapps_poplist_destroy(unsigned int handle)
{
	if(GUI_CheckDialog(WND_POP_OPTION_LIST) == GXCORE_SUCCESS)
	{
		GUI_EndDialog(WND_POP_OPTION_LIST);
	}
}

unsigned int app_netapps_pwdlg_malloc(void)
{
	PasswdDlg *pwdlg = NULL;

	pwdlg = (PasswdDlg *)GxCore_Calloc(1, sizeof(PasswdDlg));
	if(pwdlg)
	{
		memset(pwdlg, 0 ,sizeof(PasswdDlg));
		pwdlg->pos.x = APP_POP_PASSWD_POS_X;
		pwdlg->pos.y = APP_POP_PASSWD_POS_Y;
		return (unsigned int)pwdlg;
	}

	return 0;
}

void app_netapps_pwdlg_free(unsigned int handle)
{
	PasswdDlg *pwdlg = NULL;

	pwdlg = (PasswdDlg *)handle;
	if(pwdlg)
	{
		GxCore_Free(pwdlg);
		pwdlg = NULL;
	}
}

void app_netapps_pwdlg_set_content(unsigned int handle, char *content)
{
	PasswdDlg *pwdlg = NULL;

	pwdlg = (PasswdDlg *)handle;
	if(pwdlg)
	{
		pwdlg->str = content;
	}
}

void app_netapps_pwdlg_set_position(unsigned int handle, int pos_x, int pos_y)
{
	PasswdDlg *pwdlg = NULL;

	pwdlg = (PasswdDlg *)handle;
	if(pwdlg)
	{
		if(pos_x>=0)
		{
			pwdlg->pos.x = pos_x;
		}
		if(pos_y>=0)
		{
			pwdlg->pos.y = pos_y;
		}
	}
}

static int app_netapps_pwdlg_exit_cb(PopPwdRet *ret, void *arg)
{
	PwRetType pw_ret = PW_RET_OK;

	if(ret->result ==  PASSWD_OK)
	{
		pw_ret = PW_RET_OK;
	}
	else if(ret->result ==  PASSWD_ERROR)
	{
		pw_ret = PW_RET_ERROR;
	}
	else
	{
		pw_ret = PW_RET_CANCEL;
	}

	if(netapps_pw_exit_cb)
	{
		netapps_pw_exit_cb(pw_ret, arg);
	}

	return 0;
}

void app_netapps_pwdlg_set_exit_cb(unsigned int handle, PwExitCb cb)
{
	PasswdDlg *pwdlg = NULL;

	pwdlg = (PasswdDlg *)handle;
	if(pwdlg)
	{
		pwdlg->passwd_exit_cb = app_netapps_pwdlg_exit_cb;
		netapps_pw_exit_cb = cb;
	}
}

PopRetType app_netapps_pwdlg_create(unsigned int handle)
{
	PasswdDlg *pwdlg = NULL;
	PopRetType ret = POP_RET_CANCEL;

	pwdlg = (PasswdDlg *)handle;
	if(pwdlg)
	{
		if(passwd_dlg_create(pwdlg) == GXCORE_SUCCESS)
		{
			ret = POP_RET_OK;
		}
	}

	return ret;
}

void app_netapps_pwdlg_destroy(unsigned int handle)
{
	if(GUI_CheckDialog(WND_PASSWD) == GXCORE_SUCCESS)
	{
		GUI_EndDialog(WND_PASSWD);
	}
}

#if QR_POP_SUPPORT
unsigned int app_netapps_popqr_malloc(void)
{
	PopQrParas *popqr = NULL;

	popqr = (PopQrParas *)GxCore_Calloc(1, sizeof(PopQrParas));
	if(popqr)
	{
		memset(popqr, 0 ,sizeof(PopQrParas));
		popqr->pos.x = APP_WND_POP_QR_POS_X;
		popqr->pos.y = APP_WND_POP_QR_POS_Y;
		return (unsigned int)popqr;
	}

	return 0;
}

void app_netapps_popqr_free(unsigned int handle)
{
	PopQrParas *popqr = NULL;

	popqr = (PopQrParas *)handle;
	if(popqr)
	{
		GxCore_Free(popqr);
		popqr = NULL;
	}
}

void app_netapps_popqr_set_title(unsigned int handle, char *title)
{
	PopQrParas *popqr = NULL;

	popqr = (PopQrParas *)handle;
	if(popqr)
	{
		popqr->str_title = title;
	}
}

void app_netapps_popqr_set_content(unsigned int handle, char *content)
{
	PopQrParas *popqr = NULL;

	popqr = (PopQrParas *)handle;
	if(popqr)
	{
		popqr->str_content = content;
	}
}

void app_netapps_popqr_set_position(unsigned int handle, int pos_x, int pos_y)
{
	PopQrParas *popqr = NULL;

	popqr = (PopQrParas *)handle;
	if(popqr)
	{
		if(pos_x>=0)
		{
			popqr->pos.x = pos_x;
		}
		if(pos_y>=0)
		{
			popqr->pos.y = pos_y;
		}
	}
}

static void app_netapps_pqr_exit_cb(void)
{
	if(netapps_pqr_exit_cb)
	{
		netapps_pqr_exit_cb();
	}
}

void app_netapps_popqr_set_exit_cb(unsigned int handle, PqrExitCb cb)
{
	PopQrParas *popqr = NULL;

	popqr = (PopQrParas *)handle;
	if(popqr)
	{
		popqr->exit_cb = app_netapps_pqr_exit_cb;
		netapps_pqr_exit_cb = cb;
	}
}

void app_netapps_popqr_exit_key_set(unsigned int handle, int disable)
{
	PopQrParas *popqr = NULL;

	popqr = (PopQrParas *)handle;
	if(popqr)
	{
		if(disable>0)
		{
			popqr->forbidExitKey = 1;
		}
		else
		{
			popqr->forbidExitKey = 0;
		}
	}
}

PopRetType app_netapps_popqr_create(unsigned int handle)
{
	PopQrParas *popqr = NULL;
	PopRetType ret = POP_RET_CANCEL;

	popqr = (PopQrParas *)handle;
	if(popqr)
	{
		app_pop_create_qr(popqr);
		ret = POP_RET_OK;
	}

	return ret;
}

void app_netapps_popqr_destroy(unsigned int handle)
{
	app_pop_end_qr();
}
#endif

unsigned int app_netapps_poptext_malloc(void)
{
	PopText *ptext = NULL;

	ptext = (PopText *)GxCore_Calloc(1, sizeof(PopText));
	if(ptext)
	{
		memset(ptext, 0 ,sizeof(PopText));
		return (unsigned int)ptext;
	}

	return 0;
}

void app_netapps_poptext_free(unsigned int handle)
{
	PopText *ptext = NULL;

	ptext = (PopText *)handle;
	if(ptext)
	{
		GxCore_Free(ptext);
		ptext = NULL;
	}
}

void app_netapps_poptext_set_title(unsigned int handle, char *title)
{
	PopText *ptext = NULL;

	ptext = (PopText *)handle;
	if(ptext)
	{
		ptext->title = title;
	}
}

void app_netapps_poptext_set_content(unsigned int handle, char *content)
{
	PopText *ptext = NULL;

	ptext = (PopText *)handle;
	if(ptext)
	{
		ptext->content = content;
	}
}

PopRetType app_netapps_poptext_create(unsigned int handle)
{
	PopText *ptext = NULL;
	PopRetType ret = POP_RET_CANCEL;

	ptext = (PopText *)handle;
	if(ptext)
	{
		GUI_CreateDialog(WND_COMMON_TEXT);
		app_common_text_set_title(ptext->title);
		app_common_text_set_content(ptext->content);
		ret = POP_RET_OK;
	}

	return ret;
}

void app_netapps_poptext_destroy(unsigned int handle)
{
	if(GUI_CheckDialog(WND_COMMON_TEXT) == GXCORE_SUCCESS)
	{
		GUI_EndDialog(WND_COMMON_TEXT);
	}
}

#endif
