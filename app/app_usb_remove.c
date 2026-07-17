#include "app.h"
#if USB_SAFELY_REMOVE_SUPPORT
#include "app_module.h"
#include "app_send_msg.h"
#include "app_wnd_system_setting_opt.h"
#include "app_utility.h"
#include "app_default_params.h"
#include "module/config/gxconfig.h"
#include "app_pop.h"
#include "app_book.h"

#define RUSB_DISKSTR_LEN 2048

enum RUSB_ITEM_NAME
{
	ITEM_RUSB_DISK,
	ITEM_RUSB_REMOVE,
	ITEM_RUSB_TOTAL
};

static SystemSettingOpt s_rusb_opt;
static SystemSettingItem s_rusb_item[ITEM_RUSB_TOTAL];
static HotplugPartitionList* rusb_partition_list = NULL;
static char *rusb_disk_array = NULL;

static int app_rusb_save_press_callback(unsigned short key)
{
	int index = 0;
	static GUI_Event event;
	int32_t ret = EVENT_TRANSFER_KEEPON;

	if(key == STBK_OK)
	{
		GUI_GetProperty(app_system_get_combobox(ITEM_RUSB_DISK), "select", &index);
		if(index<rusb_partition_list->partition_num)
		{
			if(HotplugPartitionUmount(&rusb_partition_list->partition[index], HOTPLUG_TYPE_USB) != GXCORE_SUCCESS)
			{
				PopDlg  pop;
				memset(&pop, 0, sizeof(PopDlg));
				pop.type = POP_TYPE_NO_BTN;
				pop.format = POP_FORMAT_DLG;
				pop.mode = POP_MODE_UNBLOCK;
				pop.timeout_sec= 3;
				pop.str = STR_ID_USB_BUSY;
				popdlg_create(&pop);
				return ret;
			}
			event.type = GUI_KEYDOWN;
			event.key.sym = STBK_EXIT;
			GUI_SendEvent(WND_SYSTEM_SETTING, &event);
			ret = EVENT_TRANSFER_STOP;
		}
	}
	return ret;
}

static int app_rusb_disk_change_callback(int sel)
{
	return EVENT_TRANSFER_KEEPON;
}

static void app_rusb_setting_disk_item_init(void)
{
	s_rusb_item[ITEM_RUSB_DISK].itemTitle = STR_ID_DISK_SELECT;
	s_rusb_item[ITEM_RUSB_DISK].itemType = ITEM_CHOICE;
	s_rusb_item[ITEM_RUSB_DISK].itemProperty.itemPropertyCmb.content= rusb_disk_array;
	s_rusb_item[ITEM_RUSB_DISK].itemProperty.itemPropertyCmb.sel = 0;
	s_rusb_item[ITEM_RUSB_DISK].itemCallback.cmbCallback.CmbChange = app_rusb_disk_change_callback;
	s_rusb_item[ITEM_RUSB_DISK].itemStatus = ITEM_NORMAL;
}

static void app_rusb_setting_save_item_init(void)
{
	s_rusb_item[ITEM_RUSB_REMOVE].itemTitle = STR_ID_REMOVE;
	s_rusb_item[ITEM_RUSB_REMOVE].itemType = ITEM_PUSH;
	s_rusb_item[ITEM_RUSB_REMOVE].itemProperty.itemPropertyBtn.string= STR_ID_PRESS_OK;
	s_rusb_item[ITEM_RUSB_REMOVE].itemCallback.btnCallback.BtnPress = app_rusb_save_press_callback;
}

static int rusb_setting_exit_cb(ExitType Type)
{
	if(rusb_disk_array)
	{
		GxCore_Free(rusb_disk_array);
		rusb_disk_array=NULL;
	}

	return EXIT_RET_DEFAULT;
}

void app_rusb_update(void)
{
	int i = 0;
	static GUI_Event event;
	if(rusb_disk_array)
	{
		GxCore_Free(rusb_disk_array);
		rusb_disk_array=NULL;
	}

	rusb_partition_list = GxHotplugPartitionGet(HOTPLUG_TYPE_USB);
	if(rusb_partition_list && (rusb_partition_list->partition_num>0))
	{
		rusb_disk_array = GxCore_Calloc(1, RUSB_DISKSTR_LEN);
		memset(rusb_disk_array, 0, RUSB_DISKSTR_LEN);
		strcpy(rusb_disk_array, "[");
		for(i=0; i<rusb_partition_list->partition_num; i++)
		{
			strcat(rusb_disk_array, rusb_partition_list->partition[i].partition_name);
			if(i == (rusb_partition_list->partition_num-1))
			{
				strcat(rusb_disk_array, "]");
			}
			else
			{
				strcat(rusb_disk_array, ",");
			}
		}

	}
    else
    {
        event.type = GUI_KEYDOWN;
        event.key.sym = STBK_EXIT;
        GUI_SendEvent(WND_SYSTEM_SETTING, &event);
        return;
    }

	memset(&s_rusb_opt, 0 ,sizeof(SystemSettingOpt));
	memset(s_rusb_item, 0 ,sizeof(SystemSettingItem)*ITEM_RUSB_TOTAL);
	s_rusb_opt.menuTitle = STR_ID_USB_REMOVE;
	s_rusb_opt.titleImage = "s_title_left_media";
	s_rusb_opt.itemNum = ITEM_RUSB_TOTAL;
	s_rusb_opt.timeDisplay = TIP_HIDE;
	s_rusb_opt.item = s_rusb_item;
	s_rusb_opt.exit = rusb_setting_exit_cb;

	app_rusb_setting_disk_item_init();
	app_rusb_setting_save_item_init();

	app_system_set_create(&s_rusb_opt);
    return;
}

void app_create_rusb_menu(void)
{
	int i = 0;
	rusb_partition_list = GxHotplugPartitionGet(HOTPLUG_TYPE_USB);
	if(rusb_partition_list && (rusb_partition_list->partition_num>0))
	{
		rusb_disk_array = GxCore_Calloc(1, RUSB_DISKSTR_LEN);
		memset(rusb_disk_array, 0, RUSB_DISKSTR_LEN);
		strcpy(rusb_disk_array, "[");
		for(i=0; i<rusb_partition_list->partition_num; i++)
		{
			strcat(rusb_disk_array, rusb_partition_list->partition[i].partition_name);
			if(i == (rusb_partition_list->partition_num-1))
			{
				strcat(rusb_disk_array, "]");
			}
			else
			{
				strcat(rusb_disk_array, ",");
			}
		}

	}
	else
	{
		PopDlg  pop;
		memset(&pop, 0, sizeof(PopDlg));
		pop.type = POP_TYPE_OK;
		pop.str = STR_ID_INSERT_USB;
		pop.mode = POP_MODE_UNBLOCK;
		popdlg_create(&pop);
		return;
	}
	memset(&s_rusb_opt, 0 ,sizeof(SystemSettingOpt));
	memset(s_rusb_item, 0 ,sizeof(SystemSettingItem)*ITEM_RUSB_TOTAL);

	s_rusb_opt.menuTitle = STR_ID_USB_REMOVE;
	if (APP_THEME_CLASSIC_DARK){
		s_rusb_opt.titleImage  = "s_menu_icon_media3";
	}else if (APP_THEME_CLASSIC_BLUE){
		s_rusb_opt.titleImage = "s_title_left_media";
	}
	else{
		s_rusb_opt.topTipImgDisplay = TIP_HIDE;
		s_rusb_opt.bottmTipDisplay = TIP_HIDE;
	}
	s_rusb_opt.itemNum = ITEM_RUSB_TOTAL;
	s_rusb_opt.timeDisplay = TIP_HIDE;
	s_rusb_opt.item = s_rusb_item;
	s_rusb_opt.exit = rusb_setting_exit_cb;

	app_rusb_setting_disk_item_init();
	app_rusb_setting_save_item_init();

	app_system_set_create(&s_rusb_opt);

}
#endif
