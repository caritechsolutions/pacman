#include "app.h"
#if NETAPPS_SUPPORT
#include "app_module.h"
#include "app_wnd_system_setting_opt.h"
#include "app_netapps_setting.h"

#define TEXT_SYS_SET_TIMEZONE_UTC	"text_system_setting_timezone"
#define IMG_SYS_SET_TIMEZONE_UTC_BAK	"img_system_setting_timezone_back"

static SetExitCb netapps_exit_cb = NULL;

unsigned int app_netapps_setting_malloc(int item_num)
{
	SystemSettingOpt *setting_opt = NULL;
	SystemSettingItem *setting_item = NULL;

	if(item_num>0)
	{
		setting_opt = (SystemSettingOpt *)GxCore_Calloc(1, sizeof(SystemSettingOpt));
		if(setting_opt)
		{
			setting_item = (SystemSettingItem *)GxCore_Calloc(1, sizeof(SystemSettingItem)*item_num);
			if(setting_item)
			{
				memset(setting_opt, 0 ,sizeof(SystemSettingOpt));
				memset(setting_item, 0 ,sizeof(SystemSettingItem)*item_num);
				setting_opt->itemNum = item_num;
				setting_opt->item = setting_item;
				return (unsigned int)setting_opt;
			}
			else
			{
				GxCore_Free(setting_opt);
			}
		}
	}

	return 0;
}

void app_netapps_setting_free(unsigned int handle)
{
	SystemSettingOpt *setting_opt = NULL;

	setting_opt = (SystemSettingOpt *)handle;
	if(setting_opt)
	{
		if(setting_opt->item)
		{
			GxCore_Free(setting_opt->item);
			setting_opt->item = NULL;
		}
		GxCore_Free(setting_opt);
		setting_opt = NULL;
	}
}

void app_netapps_setting_set_title(unsigned int handle, char *title)
{
	SystemSettingOpt *setting_opt = NULL;

	setting_opt = (SystemSettingOpt *)handle;
	if(setting_opt)
	{
		setting_opt->menuTitle = title;
	}
}

void app_netapps_setting_set_logo(unsigned int handle, char *logo)
{
	SystemSettingOpt *setting_opt = NULL;

	setting_opt = (SystemSettingOpt *)handle;
	if(setting_opt)
	{
		setting_opt->titleImage = logo;
	}
}

static int app_netapps_setting_exit_cb(ExitType Type)
{
	SetExitType e_type = SET_ETYPE_NORMAL;
	SetExitRet e_ret = SET_ERET_NORMAL;
	int ret = EXIT_RET_DEFAULT;

	if(Type == EXIT_ABANDON)
	{
		e_type = SET_ETYPE_ABANDON;
	}
	else
	{
		e_type = SET_ETYPE_NORMAL;
	}
	if(netapps_exit_cb)
	{
		e_ret = netapps_exit_cb(e_type);
		if(e_ret == SET_ERET_ABANDON)
		{
			ret = EXIT_RET_POP;
		}
		else
		{
			ret = EXIT_RET_DEFAULT;
		}
	}

	return ret;
}

void app_netapps_setting_set_exit_cb(unsigned int handle, SetExitCb cb)
{
	SystemSettingOpt *setting_opt = NULL;

	setting_opt = (SystemSettingOpt *)handle;
	if(setting_opt)
	{
		setting_opt->exit = app_netapps_setting_exit_cb;
		netapps_exit_cb = cb;
	}
}

void app_netapps_setting_set_item_title(unsigned int handle, int index, char *title)
{
	SystemSettingOpt *setting_opt = NULL;

	setting_opt = (SystemSettingOpt *)handle;
	if(setting_opt&&setting_opt->item&&(index>=0)&&(index<setting_opt->itemNum))
	{
		setting_opt->item[index].itemTitle = title;
	}
}

void app_netapps_setting_set_item_type(unsigned int handle, int index, SetItemType type)
{
	SystemSettingOpt *setting_opt = NULL;
	ItemSettingType itemType = ITEM_CHOICE;

	setting_opt = (SystemSettingOpt *)handle;
	if(setting_opt&&setting_opt->item&&(index>=0)&&(index<setting_opt->itemNum))
	{
		if(type == SET_ITEM_EDIT)
		{
			itemType = ITEM_EDIT;
		}
		else if(type == SET_ITEM_PUSH)
		{
			itemType = ITEM_PUSH;
		}
		else
		{
			itemType = ITEM_CHOICE;
		}
		setting_opt->item[index].itemType = itemType;
	}
}

void app_netapps_setting_set_item_status(unsigned int handle, int index, SetItemStatus status)
{
	SystemSettingOpt *setting_opt = NULL;
	ItemDisplayStatus itemStatus = ITEM_NORMAL;

	setting_opt = (SystemSettingOpt *)handle;
	if(setting_opt&&setting_opt->item&&(index>=0)&&(index<setting_opt->itemNum))
	{
		if(status == SET_ITEM_HIDE)
		{
			itemStatus = ITEM_HIDE;
		}
		else if(status == SET_ITEM_DISABLE)
		{
			itemStatus = ITEM_DISABLE;
		}
		else
		{
			itemStatus = ITEM_NORMAL;
		}
		setting_opt->item[index].itemStatus = itemStatus;
	}
}

void app_netapps_setting_set_item_content(unsigned int handle, int index, char *content)
{
	SystemSettingOpt *setting_opt = NULL;

	setting_opt = (SystemSettingOpt *)handle;
	if(setting_opt&&setting_opt->item&&(index>=0)&&(index<setting_opt->itemNum))
	{
		if(setting_opt->item[index].itemType == ITEM_CHOICE)
		{
			setting_opt->item[index].itemProperty.itemPropertyCmb.content = content;
		}
		else if(setting_opt->item[index].itemType == ITEM_EDIT)
		{
			setting_opt->item[index].itemProperty.itemPropertyEdit.string = content;
		}
		else if(setting_opt->item[index].itemType == ITEM_PUSH)
		{
			setting_opt->item[index].itemProperty.itemPropertyBtn.string = content;
		}
	}
}

void app_netapps_setting_set_choice_sel(unsigned int handle, int index, int sel)
{
	SystemSettingOpt *setting_opt = NULL;

	setting_opt = (SystemSettingOpt *)handle;
	if(setting_opt&&setting_opt->item&&(index>=0)&&(index<setting_opt->itemNum))
	{
		if(setting_opt->item[index].itemType == ITEM_CHOICE)
		{
			setting_opt->item[index].itemProperty.itemPropertyCmb.sel = sel;
		}
	}
}

int app_netapps_setting_get_choice_sel(unsigned int handle, int index)
{
	SystemSettingOpt *setting_opt = NULL;
	int sel = -1;

	setting_opt = (SystemSettingOpt *)handle;
	if(setting_opt&&setting_opt->item&&(index>=0)&&(index<setting_opt->itemNum))
	{
		if(setting_opt->item[index].itemType == ITEM_CHOICE)
		{
			sel = setting_opt->item[index].itemProperty.itemPropertyCmb.sel;
		}
	}

	return sel;
}

void app_netapps_setting_set_edit_maxlen(unsigned int handle, int index, char *maxlen)
{
	SystemSettingOpt *setting_opt = NULL;

	setting_opt = (SystemSettingOpt *)handle;
	if(setting_opt&&setting_opt->item&&(index>=0)&&(index<setting_opt->itemNum)&&maxlen)
	{
		if(setting_opt->item[index].itemType == ITEM_EDIT)
		{
			setting_opt->item[index].itemProperty.itemPropertyEdit.maxlen = maxlen;
		}
	}
}

void app_netapps_setting_set_edit_format(unsigned int handle, int index, SetEditFormat format)
{
	SystemSettingOpt *setting_opt = NULL;

	setting_opt = (SystemSettingOpt *)handle;
	if(setting_opt&&setting_opt->item&&(index>=0)&&(index<setting_opt->itemNum))
	{
		if(setting_opt->item[index].itemType == ITEM_EDIT)
		{
			if(format == SET_EDIT_STRING)
			{
				setting_opt->item[index].itemProperty.itemPropertyEdit.format = "edit_string";
			}
			else if(format == SET_EDIT_DIGIT)
			{
				setting_opt->item[index].itemProperty.itemPropertyEdit.format = "edit_digit";
			}
			else if(format == SET_EDIT_IP)
			{
				setting_opt->item[index].itemProperty.itemPropertyEdit.format = "edit_ip";
			}
			else if(format == SET_EDIT_TIME)
			{
				setting_opt->item[index].itemProperty.itemPropertyEdit.format = "edit_time";
			}
			else if(format == SET_EDIT_DATE)
			{
				setting_opt->item[index].itemProperty.itemPropertyEdit.format = "edit_date";
			}
			else if(format == SET_EDIT_FLOAT)
			{
				setting_opt->item[index].itemProperty.itemPropertyEdit.format = "edit_float";
			}
		}
	}
}

void app_netapps_setting_set_edit_intaglio(unsigned int handle, int index, char *intaglio)
{
	SystemSettingOpt *setting_opt = NULL;

	setting_opt = (SystemSettingOpt *)handle;
	if(setting_opt&&setting_opt->item&&(index>=0)&&(index<setting_opt->itemNum))
	{
		if(setting_opt->item[index].itemType == ITEM_EDIT)
		{
			setting_opt->item[index].itemProperty.itemPropertyEdit.intaglio = intaglio;
		}
	}
}

void app_netapps_setting_set_edit_default_intaglio(unsigned int handle, int index, char *default_intaglio)
{
	SystemSettingOpt *setting_opt = NULL;

	setting_opt = (SystemSettingOpt *)handle;
	if(setting_opt&&setting_opt->item&&(index>=0)&&(index<setting_opt->itemNum))
	{
		if(setting_opt->item[index].itemType == ITEM_EDIT)
		{
			setting_opt->item[index].itemProperty.itemPropertyEdit.default_intaglio = default_intaglio;
		}
	}
}

char *app_netapps_setting_get_edit_content(unsigned int handle, int index)
{
	SystemSettingOpt *setting_opt = NULL;
	char *content = NULL;

	setting_opt = (SystemSettingOpt *)handle;
	if(setting_opt&&setting_opt->item&&(index>=0)&&(index<setting_opt->itemNum))
	{
		if(setting_opt->item[index].itemType == ITEM_EDIT)
		{
			content = setting_opt->item[index].itemProperty.itemPropertyEdit.string;
		}
	}

	return content;
}

void app_netapps_setting_set_button_roll(unsigned int handle, int index, int onoff)
{
	SystemSettingOpt *setting_opt = NULL;

	setting_opt = (SystemSettingOpt *)handle;
	if(setting_opt&&setting_opt->item&&(index>=0)&&(index<setting_opt->itemNum))
	{
		if(setting_opt->item[index].itemType == ITEM_PUSH)
		{
			if(onoff>0)
			{
				setting_opt->item[index].itemProperty.itemPropertyBtn.roll_format = true;
			}
			else
			{
				setting_opt->item[index].itemProperty.itemPropertyBtn.roll_format = false;
			}
		}
	}
}

void app_netapps_setting_set_choice_change_cb(unsigned int handle, int index, SetChoiceChange cb)
{
	SystemSettingOpt *setting_opt = NULL;

	setting_opt = (SystemSettingOpt *)handle;
	if(setting_opt&&setting_opt->item&&(index>=0)&&(index<setting_opt->itemNum))
	{
		if(setting_opt->item[index].itemType == ITEM_CHOICE)
		{
			setting_opt->item[index].itemCallback.cmbCallback.CmbChange = cb;
		}
	}
}

void app_netapps_setting_set_choice_press_cb(unsigned int handle, int index, SetChoicePress cb)
{
	SystemSettingOpt *setting_opt = NULL;

	setting_opt = (SystemSettingOpt *)handle;
	if(setting_opt&&setting_opt->item&&(index>=0)&&(index<setting_opt->itemNum))
	{
		if(setting_opt->item[index].itemType == ITEM_CHOICE)
		{
			setting_opt->item[index].itemCallback.cmbCallback.CmbPress = cb;
		}
	}
}

void app_netapps_setting_set_edit_reachend_cb(unsigned int handle, int index, SetEditEnd cb)
{
	SystemSettingOpt *setting_opt = NULL;

	setting_opt = (SystemSettingOpt *)handle;
	if(setting_opt&&setting_opt->item&&(index>=0)&&(index<setting_opt->itemNum))
	{
		if(setting_opt->item[index].itemType == ITEM_EDIT)
		{
			setting_opt->item[index].itemCallback.editCallback.EditReachEnd = cb;
		}
	}
}

void app_netapps_setting_set_edit_press_cb(unsigned int handle, int index, SetEditPress cb)
{
	SystemSettingOpt *setting_opt = NULL;

	setting_opt = (SystemSettingOpt *)handle;
	if(setting_opt&&setting_opt->item&&(index>=0)&&(index<setting_opt->itemNum))
	{
		if(setting_opt->item[index].itemType == ITEM_EDIT)
		{
			setting_opt->item[index].itemCallback.editCallback.EditPress = cb;
		}
	}
}

void app_netapps_setting_set_button_press_cb(unsigned int handle, int index, SetBtnPress cb)
{
	SystemSettingOpt *setting_opt = NULL;

	setting_opt = (SystemSettingOpt *)handle;
	if(setting_opt&&setting_opt->item&&(index>=0)&&(index<setting_opt->itemNum))
	{
		if(setting_opt->item[index].itemType == ITEM_PUSH)
		{
			setting_opt->item[index].itemCallback.btnCallback.BtnPress = cb;
		}
	}
}

int app_netapps_setting_create(unsigned int handle)
{
	SystemSettingOpt *setting_opt = NULL;
	int ret  = 0;

	setting_opt = (SystemSettingOpt *)handle;
	if(setting_opt)
	{
		if(app_system_set_create(setting_opt) == WND_OK)
		{
			ret = 1;
		}
	}

	return ret;
}

void app_netapps_setting_destroy(unsigned int handle, SetExitType type)
{
	ExitType etype = EXIT_NORMAL;

	if(type == SET_ETYPE_ABANDON)
	{
		etype = EXIT_ABANDON;
	}
	app_system_set_destroy(etype);
}

void app_netapps_setting_close(unsigned int handle)
{
    app_system_reset_unfocus_image();
    GUI_EndDialog(WND_SYSTEM_SETTING);
    GUI_SetProperty(WND_SYSTEM_SETTING,"draw_now",NULL);
}

void app_netapps_setting_sync_item_data(unsigned int handle, int index)
{
	SystemSettingOpt *setting_opt = NULL;

	setting_opt = (SystemSettingOpt *)handle;
	if(setting_opt&&setting_opt->item&&(index>=0)&&(index<setting_opt->itemNum))
	{
		app_system_set_item_data_update(index);
	}
}

void app_netapps_setting_update_item_state(unsigned int handle, int index, SetItemStatus status)
{
	SystemSettingOpt *setting_opt = NULL;

	setting_opt = (SystemSettingOpt *)handle;
	if(setting_opt&&setting_opt->item&&(index>=0)&&(index<setting_opt->itemNum))
	{
		app_system_set_item_state_update(index, status);
	}
}

void app_netapps_setting_update_item_content(unsigned int handle, int index, char *content)
{
	SystemSettingOpt *setting_opt = NULL;

	setting_opt = (SystemSettingOpt *)handle;
	if(setting_opt&&setting_opt->item&&(index>=0)&&(index<setting_opt->itemNum))
	{
		if(setting_opt->item[index].itemType == ITEM_CHOICE)
		{
			GUI_SetProperty(app_system_get_combobox(index), "content", content);
			setting_opt->item[index].itemProperty.itemPropertyCmb.content = content;
		}
		else if(setting_opt->item[index].itemType == ITEM_EDIT)
		{
			GUI_SetProperty(app_system_get_edit(index), "string", content);
			setting_opt->item[index].itemProperty.itemPropertyEdit.string = content;
		}
		else if(setting_opt->item[index].itemType == ITEM_PUSH)
		{
			GUI_SetProperty(app_system_get_button(index), "string", content);
			setting_opt->item[index].itemProperty.itemPropertyBtn.string = content;
		}
	}
}

void app_netapps_setting_update_choice_sel(unsigned int handle, int index, int sel)
{
	SystemSettingOpt *setting_opt = NULL;

	setting_opt = (SystemSettingOpt *)handle;
	if(setting_opt&&setting_opt->item&&(index>=0)&&(index<setting_opt->itemNum))
	{
		if(setting_opt->item[index].itemType == ITEM_CHOICE)
		{
			GUI_SetProperty(app_system_get_combobox(index), "select", &sel);
			setting_opt->item[index].itemProperty.itemPropertyCmb.sel = sel;
		}
	}
}

void app_netapps_setting_set_red_key(unsigned int handle, char *name, SetKeyFun func)
{
	SystemSettingOpt *setting_opt = NULL;

	setting_opt = (SystemSettingOpt *)handle;
	if(setting_opt)
	{
		setting_opt->menuFuncKey.redKey.keyName = name;
		setting_opt->menuFuncKey.redKey.keyPressFun = func;
	}
}

void app_netapps_setting_set_green_key(unsigned int handle, char *name, SetKeyFun func)
{
	SystemSettingOpt *setting_opt = NULL;

	setting_opt = (SystemSettingOpt *)handle;
	if(setting_opt)
	{
		setting_opt->menuFuncKey.greenKey.keyName = name;
		setting_opt->menuFuncKey.greenKey.keyPressFun = func;
	}
}

void app_netapps_setting_set_blue_key(unsigned int handle, char *name, SetKeyFun func)
{
	SystemSettingOpt *setting_opt = NULL;

	setting_opt = (SystemSettingOpt *)handle;
	if(setting_opt)
	{
		setting_opt->menuFuncKey.blueKey.keyName = name;
		setting_opt->menuFuncKey.blueKey.keyPressFun = func;
	}
}

void app_netapps_setting_set_yellow_key(unsigned int handle, char *name, SetKeyFun func)
{
	SystemSettingOpt *setting_opt = NULL;

	setting_opt = (SystemSettingOpt *)handle;
	if(setting_opt)
	{
		setting_opt->menuFuncKey.yellowKey.keyName = name;
		setting_opt->menuFuncKey.yellowKey.keyPressFun = func;
	}
}

void app_netapps_setting_exit_key_set(unsigned int handle, SetStatus status)
{
	SystemSettingOpt *setting_opt = NULL;

	setting_opt = (SystemSettingOpt *)handle;
	if(setting_opt)
	{
		if(status == SET_OFF)
		{
			setting_opt->forbidExitKey = true;
		}
		else
		{
			setting_opt->forbidExitKey = false;
		}
	}
}

void app_netapps_setting_top_tip_set(unsigned int handle, SetStatus status)
{
	SystemSettingOpt *setting_opt = NULL;

	setting_opt = (SystemSettingOpt *)handle;
	if(setting_opt)
	{
		if(status == SET_OFF)
		{
			setting_opt->topTipDisplay = TIP_HIDE;
		}
		else
		{
			setting_opt->topTipDisplay = TIP_SHOW;
		}
	}
}

void app_netapps_setting_top_tipimg_set(unsigned int handle, SetStatus status)
{
	SystemSettingOpt *setting_opt = NULL;

	setting_opt = (SystemSettingOpt *)handle;
	if(setting_opt)
	{
		if(status == SET_OFF)
		{
			setting_opt->topTipImgDisplay = TIP_HIDE;
		}
		else
		{
			setting_opt->topTipImgDisplay = TIP_SHOW;
		}
	}
}

void app_netapps_setting_buttom_tip_set(unsigned int handle, SetStatus status)
{
	SystemSettingOpt *setting_opt = NULL;

	setting_opt = (SystemSettingOpt *)handle;
	if(setting_opt)
	{
		if(status == SET_OFF)
		{
			setting_opt->bottmTipDisplay = TIP_HIDE;
		}
		else
		{
			setting_opt->bottmTipDisplay = TIP_SHOW;
		}
	}
}

void app_netapps_setting_time_set(unsigned int handle, SetStatus status)
{
	SystemSettingOpt *setting_opt = NULL;

	setting_opt = (SystemSettingOpt *)handle;
	if(setting_opt)
	{
		if(status == SET_OFF)
		{
			setting_opt->timeDisplay = TIP_HIDE;
		}
		else
		{
			setting_opt->timeDisplay = TIP_SHOW;
		}
	}
}

void app_netapps_setting_show_msg(unsigned int handle, char *msg)
{
	SystemSettingOpt *setting_opt = NULL;

	setting_opt = (SystemSettingOpt *)handle;
	if(setting_opt)
	{
		GUI_SetProperty(TEXT_SYS_SET_TIMEZONE_UTC, "state", "show");
		GUI_SetProperty(IMG_SYS_SET_TIMEZONE_UTC_BAK, "state", "show");
		GUI_SetProperty(TEXT_SYS_SET_TIMEZONE_UTC, "string", msg);
	}
}


#endif
