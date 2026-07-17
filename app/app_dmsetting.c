#include "app.h"
#include "app_data_type.h"
#include "app_send_msg.h"
#include <gxcore.h>
#include "app_utility.h"
#include "app_default_params.h"
#include "app_pop.h"
#include "app_wnd_system_setting_opt.h"
#include "app_book.h"
#include "app_windows.h"
#include "module/app_pvr.h"

#define BTN_TITLE  "setting_title_dm"
#define BTN_NAME_DM  "btn_name_dm"
#define BTN_USED_SIZE_DM  "btn_used_size_dm"
#define BTN_FREE_SIZE_DM  "btn_free_size_dm"
#define BTN_TOTAL_SIZE_DM  "btn_total_size_dm"
#define BTN_TYPE_DM  "btn_type_dm"
#define LIST_VIEW_DM "list_view_dm"
#define IMG_FORMAT_TIP "img_format_dm"
#define TEXT_FORMAT_TIP "txt_format_dm"
#define MAX_FORMAT_USB_SIZE (1024*32)

#define IMG_MARK		"s_choice"
#define MAX_FORMAT_FLAG 4

static HotplugPartitionList* partition_list = NULL;
static HotplugPartition s_opt_partition;
extern  status_t HotplugPartitionDestroy(struct gxfs_hot_device* device);

void system_setting_set_pvr_partition(char *partition)
{
    if(partition == NULL)
        return;
    GxBus_ConfigSet(PVR_PARTITION_KEY, partition);
}

void system_pvr_partition_init(char *pvr_partition)
{
    if(NULL == pvr_partition)
        return;
    GxBus_ConfigGet(PVR_PARTITION_KEY, pvr_partition, PVR_DEV_STR_LEN,  PVR_PARTITION);
}

#ifdef FILE_EDIT_VALID
static HotplugPartition s_opt_partition;
static int s_format_status[PARTITION_NUM_MAX] = {0}; // 0:success 1:failed
static int thiz_list_sel = 0;

static int  _format_pop_cb(int index, int disk_id)
{
	#ifdef LINUX_OS
	char cmd_buf[128];
	sprintf(cmd_buf, "usb_format %s", s_opt_partition.dev_name);
	system(cmd_buf);
	#else
	GxMkfsOpts opts;

	opts.cluster_size = CLUSTER_4096;
	opts.label = NULL;
	if(GXCORE_ERROR == GxCore_MkFs(s_opt_partition.dev_name, s_opt_partition.fs_type, &opts))
    {
        s_format_status[index] = disk_id;
        return -1;
    }
    else
    {
        s_format_status[index] = 0;
    }
	#endif
    memset(&s_opt_partition, 0, sizeof(HotplugPartition));
	return 0;
}

static int _format_status_pop_cb(PopDlgRet ret)
{
    int act = -1, sel = 0;
    GUI_SetProperty(LIST_VIEW_DM, "update_all", NULL);
    sel = thiz_list_sel;
    GUI_SetProperty(LIST_VIEW_DM, "active", &act);
    GUI_SetProperty(LIST_VIEW_DM, "select", &sel);
    return 0;
}

static int _format_finish_pop_cb(PopDlgRet ret)
{
	PopDlg pop;
	memset(&pop, 0, sizeof(PopDlg));
	if(0 == s_format_status[thiz_list_sel])
	{
		pop.str = "Complete format!";
	}
	else
	{
		pop.str = "Format failed!";
	}
	pop.type = POP_TYPE_NO_BTN;
	pop.mode = POP_MODE_UNBLOCK;
	pop.format = POP_FORMAT_DLG;
	pop.timeout_sec = 1;
	pop.exit_cb = _format_status_pop_cb;
	popdlg_create(&pop);
	GUI_SetInterface("flush",NULL);

    return 0;
}

static int _format_info_pop_cb(PopDlgRet ret)
{
    int i;
    int act = -1;
	GxDiskInfo disk_info;
	uint64_t temp_size = 0;

	if (POP_VAL_OK == ret)
	{
		PopDlg pop;

		for(i = 0; i < partition_list->partition_num; i++)
		{
			GxCore_DiskInfo(s_opt_partition.partition_entry, &disk_info);
			temp_size =  disk_info.total_size/1048576;
			memset(&pop, 0, sizeof(PopDlg));
			if(temp_size > MAX_FORMAT_USB_SIZE)
			{
                thiz_list_sel = i;
				pop.type = POP_TYPE_NO_BTN;
				pop.format = POP_FORMAT_DLG;
				pop.mode = POP_MODE_UNBLOCK;
				pop.str = "USB too large to format";
				pop.timeout_sec = 1;
                pop.exit_cb = _format_status_pop_cb;
				popdlg_create(&pop);
				return 0;
			}
			else
			{
				if(strcmp(s_opt_partition.dev_name, partition_list->partition[i].dev_name) == 0)
				{
					thiz_list_sel = i;
					pop.type = POP_TYPE_NO_BTN;
					pop.str = STR_ID_FORMATING;
					pop.format = POP_FORMAT_DLG;
					pop.mode = POP_MODE_UNBLOCK;
					pop.timeout_sec = 100;
					pop.exit_cb = _format_finish_pop_cb;
					popdlg_create(&pop);
					GUI_SetInterface("flush",NULL);
					_format_pop_cb(i,(int)partition_list->partition[i].disk_id);
					char *txt_pop_tip = NULL;
					GUI_GetProperty("txt_pop_tip", "string", &txt_pop_tip);
					if ((txt_pop_tip == NULL) || (strcmp(txt_pop_tip, STR_ID_FORMATING) == 0))
					{
                        GUI_SetProperty(LIST_VIEW_DM,"update_all",NULL);
						popdlg_destroy();
					}
					return 0;
				}
			}
		}
		if(i >= partition_list->partition_num)
		{
			GUI_SetProperty(LIST_VIEW_DM, "active", &act);
            memset(&s_opt_partition, 0, sizeof(HotplugPartition));

            memset(&pop, 0, sizeof(PopDlg));
            pop.type = POP_TYPE_OK;
            pop.format = POP_FORMAT_DLG;
            pop.str = STR_ID_INSERT_USB;
            pop.mode = POP_MODE_UNBLOCK;
            popdlg_create(&pop);
        }
    }
    else
    {
		GUI_SetProperty(LIST_VIEW_DM, "active", &act);
        memset(&s_opt_partition, 0, sizeof(HotplugPartition));
    }

    return 0;
}
#endif

static int _pvr_set_pop_cb(PopDlgRet ret)
{
    int i;
    int sel = 0;
    int act = -1;

    GUI_SetProperty(LIST_VIEW_DM, "active", &act);

    if (POP_VAL_OK == ret)
    {
        for(i = 0; i < partition_list->partition_num; i++)
        {
            if(strcmp(s_opt_partition.dev_name, partition_list->partition[i].dev_name) == 0)
            {
                system_setting_set_pvr_partition(partition_list->partition[i].dev_name);

                GUI_SetProperty(LIST_VIEW_DM, "update_all", NULL);
                sel = i;
                GUI_SetProperty(LIST_VIEW_DM, "select", &sel);

                break;
            }
        }

        if(i >= partition_list->partition_num)
        {
            PopDlg pop;

            memset(&pop, 0, sizeof(PopDlg));
            pop.type = POP_TYPE_OK;
            pop.format = POP_FORMAT_DLG;
            pop.str = STR_ID_INSERT_USB;
            pop.mode = POP_MODE_UNBLOCK;
            popdlg_create(&pop);
        }
    }

    memset(&s_opt_partition, 0, sizeof(HotplugPartition));

    return 0;
}

static int _dm_setting_update(void)
{
	int i = 0;
    char partition[PVR_DEV_STR_LEN + 1] = {0};

#ifdef FILE_EDIT_VALID
    int j = 0;
    int num = 0;
    char format_usb[MAX_FORMAT_FLAG][PVR_DEV_STR_LEN + 1];
    memset(format_usb,0,sizeof(format_usb));

    if(partition_list != NULL)
    {
        for(i = 0; i < partition_list->partition_num; i++)
        {
            //s_format_status 比较找出损坏分区
            if(partition_list->partition[i].disk_id == s_format_status[i] && num < MAX_FORMAT_FLAG)
            {
                strcpy(format_usb[num],partition_list->partition[i].dev_name);
                num++;
            }
        }
        memset(s_format_status,0,sizeof(int)*PARTITION_NUM_MAX);
    }
#endif

    partition_list = GxHotplugPartitionGet(HOTPLUG_TYPE_USB);
	if(partition_list != NULL)
	{
        system_pvr_partition_init(partition);
        for(i = 0; i < partition_list->partition_num; i++)
        {
            if(strcmp(partition, partition_list->partition[i].dev_name) == 0)
            {
                break;
            }
        }

		if((partition_list->partition_num > 0) && (i >= partition_list->partition_num))
		{
			system_setting_set_pvr_partition(partition_list->partition[0].dev_name);
		}
	}
#ifdef FILE_EDIT_VALID
    if(partition_list != NULL)
    {
        for(j=0; j < num ; j++)
        {
            for(i = 0; i < partition_list->partition_num; i++)
            {
                if(strcmp(format_usb[j], partition_list->partition[i].dev_name) == 0)
                {
                    s_format_status[i] = (int)partition_list->partition[i].disk_id;
                }
            }
        }
    }
#endif

	return 0;
}

SIGNAL_HANDLER int app_dm_setting_create(const char* widgetname, void *usrdata)
{
    int sel = -1;

    _dm_setting_update();
    memset(s_format_status,0,sizeof(int)*PARTITION_NUM_MAX);
    if(partition_list && partition_list->partition_num > 0)
    {
        sel = 0;
    }

    GUI_SetProperty(LIST_VIEW_DM, "select", &sel);
#ifdef DISK_FORMAT_UNVALID
return EVENT_TRANSFER_STOP;
#endif
#ifdef FILE_EDIT_VALID
	GUI_SetProperty(IMG_FORMAT_TIP, "state","show");
	GUI_SetProperty(TEXT_FORMAT_TIP, "state","show");
#endif

	return EVENT_TRANSFER_STOP;
}


SIGNAL_HANDLER int app_dm_destroy(const char* widgetname, void *usrdata)
{
	return EVENT_TRANSFER_STOP;
}


SIGNAL_HANDLER int app_dm_keypress(const char* widgetname, void *usrdata)
{
	GUI_Event *event = NULL;
	uint32_t sel = 0;

	event = (GUI_Event *)usrdata;
	if(GUI_KEYDOWN ==  event->type)
	{
		switch(find_virtualkey_ex(event->key.scancode,event->key.sym))
		{
				 case VK_BOOK_TRIGGER:
					app_system_reset_unfocus_image();
					GUI_EndDialog("after wnd_full_screen");
					break;

			case STBK_MENU:
			case STBK_EXIT:
			{
				GUI_EndDialog(WND_DISK_MANAGE);
				if( APP_THEME_CLASSIC_PURPLE ){
					GUI_SetProperty(WND_MAIN_MENU, "update", NULL);
				}
				return EVENT_TRANSFER_STOP;
			}
			case STBK_RED:
			{
				#ifdef DISK_FORMAT_UNVALID
				return EVENT_TRANSFER_STOP;
				#endif
#ifdef FILE_EDIT_VALID
				PopDlg pop;
				if(NULL == partition_list||0 == partition_list->partition_num)
				{
					return EVENT_TRANSFER_STOP;
				}
				GUI_GetProperty(LIST_VIEW_DM, "select", &sel);
                GUI_SetProperty(LIST_VIEW_DM, "active", &sel);
                memset(&s_opt_partition, 0, sizeof(HotplugPartition));
                memcpy(&s_opt_partition, &partition_list->partition[sel], sizeof(HotplugPartition));

                memset(&pop, 0, sizeof(PopDlg));
                pop.type = POP_TYPE_YES_NO;
                pop.format = POP_FORMAT_DLG;
                pop.mode = POP_MODE_UNBLOCK;
                pop.str = STR_ID_FORMAT_INFO;
                pop.exit_cb = _format_info_pop_cb;
                popdlg_create(&pop);
#endif
				return EVENT_TRANSFER_STOP;
			}

			case STBK_GREEN:
			{
				PopDlg pop;

				if(NULL == partition_list||0 == partition_list->partition_num)
				{
					return EVENT_TRANSFER_STOP;
				}

				GUI_GetProperty(LIST_VIEW_DM, "select", &sel);
                GUI_SetProperty(LIST_VIEW_DM, "active", &sel);
                memset(&s_opt_partition, 0, sizeof(s_opt_partition));
                memcpy(&s_opt_partition, &partition_list->partition[sel], sizeof(s_opt_partition));
                memset(&pop, 0, sizeof(PopDlg));
                pop.type = POP_TYPE_YES_NO;
                pop.format = POP_FORMAT_DLG;
                pop.mode = POP_MODE_UNBLOCK;
                pop.str = STR_ID_SAVE_INFO;
                pop.exit_cb = _pvr_set_pop_cb;
                popdlg_create(&pop);

                return EVENT_TRANSFER_STOP;
			}

			#ifdef LINUX_OS
			// usb speed test
			case STBK_YELLOW:
			{
				if(NULL == partition_list||0 == partition_list->partition_num)
				{
					return EVENT_TRANSFER_STOP;
				}

				GUI_GetProperty(LIST_VIEW_DM, "select", &sel);

				extern void app_usb_speed_menu_exec(void *usb_dev,uint32_t sel);
				app_usb_speed_menu_exec(partition_list->partition[sel].dev_name,sel);
			}
			return EVENT_TRANSFER_STOP;
			#endif

			case STBK_LEFT:
			case STBK_RIGHT:
				return EVENT_TRANSFER_STOP;

			default:
				break;
		}
	}
	return EVENT_TRANSFER_KEEPON;
}

SIGNAL_HANDLER int app_dm_list_get_total(const char* widgetname, void *usrdata)
{
	int num = 0;

    _dm_setting_update();

	if(partition_list != NULL)
		num = partition_list->partition_num;
	return num;
}


SIGNAL_HANDLER int app_dm_list_get_data(const char* widgetname, void *usrdata)
{
	GxDiskInfo disk_info;
	ListItemPara *item_para;
	char partition[PVR_DEV_STR_LEN + 1] = {0};
	uint64_t temp_size = 0;
	static char pArry0[20] = {0};
	static char pArry1[20] = {0};
	static char pArry2[20] = {0};

	if((NULL == widgetname || NULL == usrdata) || (partition_list == NULL) || (partition_list->partition_num == 0))
	{
		return (1);
	}

    item_para = (ListItemPara *)(usrdata);

    if(partition_list->partition_num <= item_para->sel)
    {
        item_para->image = NULL;

        item_para= item_para->next;
        item_para->string = " ";

        item_para= item_para->next;
        item_para->string = " ";

        item_para= item_para->next;
        item_para->string = " ";

        item_para= item_para->next;
        item_para->string = " ";

        item_para= item_para->next;
        item_para->string = " ";

        return EVENT_TRANSFER_STOP;
    }
	system_pvr_partition_init(partition);
    if(strcmp(partition, partition_list->partition[item_para->sel].dev_name) == 0)
	{
		item_para->image = IMG_MARK;
	}
	else
	{
		item_para->image = NULL;
	}

	item_para= item_para->next;
	item_para->string = partition_list->partition[item_para->sel].partition_name;

    if(1 == s_format_status[item_para->sel])//Format error
    {
        item_para= item_para->next;
        item_para->string = "N/A M";

        item_para= item_para->next;
        item_para->string = "N/A M";

        item_para= item_para->next;
        item_para->string = "N/A M";

        item_para= item_para->next;
        item_para->string = "Unknown";

        return EVENT_TRANSFER_STOP;
    }

	GxCore_DiskInfo(partition_list->partition[item_para->sel].partition_entry, &disk_info);
	item_para= item_para->next;
	memset(pArry0, 0 ,sizeof(pArry0));
	temp_size = disk_info.used_size/1048576;
	sprintf((void*)pArry0,"%llu M", temp_size);
	item_para->string = pArry0;

	item_para= item_para->next;
	memset(pArry1, 0 ,sizeof(pArry1));
	temp_size = disk_info.freed_size/1048576;
	sprintf((void*)pArry1,"%llu M", temp_size);
	item_para->string = pArry1;

	item_para= item_para->next;
	memset(pArry2, 0 ,sizeof(pArry2));
	temp_size = disk_info.total_size/1048576;
	sprintf((void*)pArry2,"%llu M", temp_size);
	item_para->string = pArry2;

	item_para= item_para->next;
	item_para->string = partition_list->partition[item_para->sel].fs_type;

	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_dm_list_keypress(GuiWidget *widget, void *usrdata)
{
    int ret = EVENT_TRANSFER_KEEPON;
    GUI_Event *event = NULL;
    uint32_t sel = 0;

    event = (GUI_Event *)usrdata;
    switch(event->type)
    {
        case GUI_SERVICE_MSG:
            break;

        case GUI_KEYDOWN:
            switch(find_virtualkey_ex(event->key.scancode,event->key.sym))
            {
                case STBK_PAGE_UP:
                    GUI_GetProperty(LIST_VIEW_DM, "select", &sel);
                    if ((partition_list != NULL) && (sel == 0))
                    {
                        sel = partition_list->partition_num - 1;
                        GUI_SetProperty(LIST_VIEW_DM, "select", &sel);
                        ret = EVENT_TRANSFER_STOP;
                    }
                    break;

                case STBK_PAGE_DOWN:
                    GUI_GetProperty(LIST_VIEW_DM, "select", &sel);
                    if ((partition_list != NULL) && (sel == partition_list->partition_num - 1))
                    {
                        sel = 0;
                        GUI_SetProperty(LIST_VIEW_DM, "select", &sel);
                        ret = EVENT_TRANSFER_STOP;
                    }
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

SIGNAL_HANDLER int app_dm_lost_focus(GuiWidget *widget, void *usrdata)
{
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_dm_got_focus(GuiWidget *widget, void *usrdata)
{

	return EVENT_TRANSFER_STOP;
}


void app_dm_list_hotplug_proc(GxMessage* msg)
{
    char partition[PVR_DEV_STR_LEN + 1] = {0};
    int sel = -1;
    int i;
    if(GUI_CheckDialog(WND_USB_SPEED) == GXCORE_SUCCESS)
    {
        GUI_EndDialog(WND_USB_SPEED);
    }

    GxBus_ConfigGet(PVR_PARTITION_KEY, partition, PVR_DEV_STR_LEN, PVR_PARTITION);

    if(msg->msg_id == GXMSG_HOTPLUG_OUT) //wait GXMSG_HOTPLUG_CLEAN
    {
        return;
    }
    if(msg->msg_id == GXMSG_HOTPLUG_CLEAN)
	{
		 popdlg_destroy();
	}

    if(GUI_CheckDialog(WND_POP_TIP) == GXCORE_SUCCESS)
    {
        for(i = 0; i < partition_list->partition_num; i++)
        {
            if(strcmp(s_opt_partition.dev_name, partition_list->partition[i].dev_name) == 0)
            {
                sel = i;
                break;
            }
        }

        if((i >= partition_list->partition_num) && (strlen(s_opt_partition.dev_name) > 0))
        {
            popdlg_destroy();
        }

        GUI_SetProperty(LIST_VIEW_DM,"update_all",NULL);

        GUI_SetProperty(LIST_VIEW_DM, "active", &sel);
        if(partition_list->partition_num > 0)
        {
            if(i >= partition_list->partition_num)
            {
                sel = 0;
            }
        }
        GUI_SetProperty(LIST_VIEW_DM, "select", &sel);
        GUI_SetProperty(WND_POP_TIP,"update",NULL);
    }
    else
    {
        GUI_SetProperty(LIST_VIEW_DM,"update_all",NULL);
        if(partition_list->partition_num > 0)
        {
            GUI_GetProperty(LIST_VIEW_DM, "select", &sel);

            //if(sel >= partition_list->partition_num)
            //{
            //    sel = 0;
            //}
            for(i=0;i<partition_list->partition_num;i++)
			{
				if(strcmp((char*)partition,partition_list->partition[i].dev_name)==0)
				{
					sel=i;
					break;
				}
			}
            if(i >= partition_list->partition_num)
				sel=0;

        }
        GUI_SetProperty(LIST_VIEW_DM, "select", &sel);
        GUI_SetProperty(LIST_VIEW_DM,"update_all",NULL);
        app_update_pop_dlg(WND_POP_BOOK);
    }
}

