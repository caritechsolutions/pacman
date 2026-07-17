#include "app.h"
#if TRANSMISSION_SUPPORT
#include "app_module.h"
#include <gx_apps.h>
#include "app_wnd_system_setting_opt.h"
#include "app_default_params.h"
#include "app_utility.h"
#include "app_wnd_file_list.h"
#include <gx_transmission.h>
#include <signal.h>
#include <sys/wait.h>

#define TEXT_TITLE           "text_transmission_title"
#define IMG_LOGO             "img_transmission_title"
#define LIST_GROUP           "list_spps_transmission_list"
#define BOX_GROUP "box_transmission_group"
#define TXT_POPUP            "txt_transmission_popup"
#define IMG_POPUP            "img_transmission_popup"
#define IMG_RED_KEY          "img_transmission_red"
#define TEXT_RED_KEY         "text_transmission_red"
#define TXT_DATE             "text_transmission_date"
#define TXT_TIME             "text_transmission_time"
#define IXT_INFO_1   "txt_apps_transmission_info_1"
#define IXT_INFO_2   "txt_apps_transmission_info_2"
#define IXT_INFO_3    "txt_apps_transmission_info_3"
#define IXT_INFO_4    "txt_apps_transmission_info_4"
#define IXT_USB    "txt_apps_transmission_usb_info"
#define IMG_USB 	"img_apps_transmission_usb"
#define IMG_RED     "img_transmission_red"
#define TXT_RED     "text_transmission_red"
#define IMG_GREEN   "img_transmission_green"
#define TXT_GREEN   "text_transmission_green"
#define IMG_YELLOW   "img_transmission_yellow"
#define TXT_YELLOW   "text_transmission_yellow"
#define IMG_BLUE    "img_transmission_blue"
#define TXT_BLUE    "text_transmission_blue"
#define IXT_TITLE_1     "txt_apps_transmission_title_1"
#define IXT_TITLE_2     "txt_apps_transmission_title_2"
#define IXT_TITLE_3   "txt_apps_transmission_title_3"
#define IXT_TITLE_4   "txt_apps_transmission_title_4"

#define MAX_TORRENT_ITEM 7
#define MIN_USB_FREE_SPACE 50
#define TRANSMISSION_PARTITION "/media/sda1"
#define TRANSMISSION_BIN_PATH "/usr/local/bin/transmission-daemon"
#define TRANSMISSION_CONFIG_DIR "/media/sda1/transmission/cfg"
#define TRANSMISSION_DOWNLOAD_DIR "/media/sda1/transmission/downloads"
#define TRANSMISSION_FILE_SUFFFIX    "torrent"

#define TRANSMISSION_LIMIT_UP_KEY	"netapps>transmission_speed_limit_up"
#define TRANSMISSION_LIMIT_DOWN_KEY	"netapps>transmission_speed_limit_down"
#define TRANSMISSION_DEFAULT_SPEED_LIMIT_UP 50
#define TRANSMISSION_DEFAULT_SPEED_LIMIT_DOWN 200

typedef enum
{
	TRANSMISSION_SETTING_LIMIT_UP=0,
	TRANSMISSION_SETTING_LIMIT_DOWN,
	TRANSMISSION_SETTING_MAX,
}transmission_setting_itm;

typedef enum transmission_list_type_e{
	TRANSMISSION_LIST_ALL = 0,
	TRANSMISSION_LIST_DOWNLOADING,
	TRANSMISSION_LIST_FINISH,
	TRANSMISSION_LIST_MAX,
}transmission_list_type_e;

typedef struct transmission_setting_s
{
	int speed_limit_up;
	int speed_limit_down;
}transmission_setting_t;

static bool pop_msg_on = false;
static char *s_file_path_bak = NULL;
static char *s_file_path = NULL;
static event_list* s_transmission_popup_timer = NULL;
static event_list* s_transmission_fresh_timer = NULL;
static event_list* s_transmission_time_timer = NULL;
static int thiz_del_torrent_id = 0;

static gx_transmission_torrentlist_t * s_transmission_list =NULL;
static int *s_transmission_list_map=NULL;
static SystemSettingOpt  *transmission_setting_opt = NULL;
static SystemSettingItem *transmission_setting_item = NULL;
static transmission_setting_t transmission_setting;
static transmission_list_type_e s_transmission_list_type = TRANSMISSION_LIST_ALL;
static bool s_transmission_task_begin = false;
static bool s_transmission_fresh_finish = false;
static void app_transmission_stop_all_task(void);
static int transmission_usb_check(void);


static void _transmission_display_size_str(uint64_t size, char* str)
{
	double display_size = 0.00;

	if(size >= SIZE_UNIT_T)
	{
		display_size = (double)size / (SIZE_UNIT_T);
		sprintf(str, "%3.2fGB", display_size);
	}
	else if(size >= SIZE_UNIT_G)
	{
		display_size = (double)size / (SIZE_UNIT_G);
		sprintf(str, "%3.2fGB", display_size);
	}
	else if(size >= SIZE_UNIT_M)
	{
		display_size = (double)size / (SIZE_UNIT_M);
		sprintf(str, "%3.2fMB", display_size);
	}
	else if(size >= SIZE_UNIT_K)
	{
		display_size = (double)size / (SIZE_UNIT_K);
		sprintf(str, "%3.2fKB", display_size);
	}
	else
	{
		display_size = (double)size;
		sprintf(str, "%3.2fB", display_size);
	}
}

static void _transmission_display_speed_str(uint64_t speed, char* str)
{
	double display_speed = 0.00;

	if(speed >= SIZE_UNIT_G)
	{
		display_speed = (double)speed / (SIZE_UNIT_G);
		sprintf(str, "%3.2fGB/S", display_speed);
	}
	else if(speed >= SIZE_UNIT_M)
	{
		display_speed = (double)speed / (SIZE_UNIT_M);
		sprintf(str, "%3.2fMB/S", display_speed);
	}
	else if(speed >= SIZE_UNIT_K)
	{
		display_speed = (double)speed / (SIZE_UNIT_K);
		sprintf(str, "%3.2fKB/S", display_speed);
	}
	else
	{
		display_speed = (double)speed;
		sprintf(str, "%3.2fB/S", display_speed);
	}
}

static void app_transmission_exit(void)
{
	GUI_EndDialog(WND_TRANSMISSION);
}

void app_create_transmission_menu(void)
{
	if(check_usb_status() == false)
	{
		PopDlg	pop;
		memset(&pop, 0, sizeof(PopDlg));
		pop.type = POP_TYPE_OK;
		pop.str = STR_ID_INSERT_USB;
		pop.mode = POP_MODE_UNBLOCK;
		popdlg_create(&pop);
		return;
	}

	if( transmission_usb_check() < 0)
	{
		PopDlg	pop;
		memset(&pop, 0, sizeof(PopDlg));
		pop.type = POP_TYPE_OK;
		pop.str = STR_ID_NO_DEVICE;
		pop.mode = POP_MODE_UNBLOCK;
		popdlg_create(&pop);
		return;
	}
	GUI_CreateDialog(WND_TRANSMISSION);
}

void app_transmission_begin(void)
{
	char cmd[256];
	__sighandler_t old_sig = 0;
	int ret = 0;

	if(s_transmission_task_begin)
		return;

	memset(cmd, 0, sizeof(cmd));
	snprintf(cmd, sizeof(cmd), "%s -g %s -w %s",
		TRANSMISSION_BIN_PATH,
		TRANSMISSION_CONFIG_DIR,
		TRANSMISSION_DOWNLOAD_DIR);

	old_sig = signal(SIGCHLD,SIG_DFL);
	ret = system(cmd);
	app_log_debug("cmd: %s  return:%d old_sig = %d\n",cmd,ret,(int)old_sig);

   if(ret == -1){
        app_log_error("system error");
    }
    else{
        if(WIFEXITED(ret))
        {
            if(WEXITSTATUS(ret) == 0)
                app_log_info("run command successful\n");
            else
                app_log_error("run command fail and exit code is %d\n",WEXITSTATUS(ret));
        }
        else
            app_log_info("exit status = %d\n",WEXITSTATUS(ret));
    }
    	signal(SIGCHLD,old_sig);
	GxCore_ThreadDelay(2000);
	s_transmission_task_begin = true;
}
static void app_transmission_hide_popup_msg(void)
{
	if(s_transmission_popup_timer)
	{
		remove_timer(s_transmission_popup_timer);
		s_transmission_popup_timer = NULL;
	}

	pop_msg_on = false;
	GUI_SetProperty(TXT_POPUP, "state", "hide");
	GUI_SetProperty(IMG_POPUP, "state", "hide");
}
static int _transmission_popup_timer_timeout(void *userdata)
{
    s_transmission_popup_timer = NULL;
	app_transmission_hide_popup_msg();
	return 0;
}

static void app_transmission_show_popup_msg(char* pMsg, int time_out)
{
	pop_msg_on = true;
	GUI_SetProperty(TXT_POPUP, "string", pMsg);
	GUI_SetProperty(IMG_POPUP, "state", "show");
	GUI_SetProperty(TXT_POPUP, "state", "show");

	if(time_out > 0)
	{
		if (reset_timer(s_transmission_popup_timer) != 0)
		{
			s_transmission_popup_timer = create_timer(_transmission_popup_timer_timeout, time_out, NULL, TIMER_ONCE);
		}
	}
}
static void app_transmission_show_info(char *info1, char *info2, char *info3,char *info4,char *usbinfo)
{
	GUI_SetProperty(IXT_INFO_1, "string", info1);
	GUI_SetProperty(IXT_INFO_2, "string", info2);
	GUI_SetProperty(IXT_INFO_3, "string", info3);
	GUI_SetProperty(IXT_INFO_4, "string", info4);
	if(usbinfo == NULL)
	{
		GUI_SetProperty(IXT_USB, "state", "hide");
		GUI_SetProperty(IMG_USB, "state", "hide");
	}
	else
	{
		GUI_SetProperty(IMG_USB, "state", "show");
		GUI_SetProperty(IXT_USB, "string", usbinfo);
		GUI_SetProperty(IXT_USB, "state", "show");
	}
}
static void app_transmission_show_color_tip(char *red, char *green, char *yellow, char *blue)
{
	if(0 == strcasecmp(LIST_GROUP, GUI_GetFocusWidget()))
	{
		GUI_SetProperty(IXT_TITLE_1, "string", "Up-Speed:");
		GUI_SetProperty(IXT_TITLE_2, "string", "Download:");
		GUI_SetProperty(IXT_TITLE_3, "string", "Upload:");
		GUI_SetProperty(IXT_TITLE_4, "string", "Left Time:");
	}
	else
	{
		GUI_SetProperty(IXT_TITLE_1, "string", "Total Count:");
		GUI_SetProperty(IXT_TITLE_2, "string", "Total Dload:");
		GUI_SetProperty(IXT_TITLE_3, "string", "Total Upload:");
		GUI_SetProperty(IXT_TITLE_4, "string", "Total Speed:");
	}

	if(red == NULL)
	{
		GUI_SetProperty(IMG_RED, "state", "hide");
		GUI_SetProperty(TXT_RED, "state", "hide");
	}
	else
	{
		GUI_SetProperty(IMG_RED, "state", "show");
		GUI_SetProperty(TXT_RED, "string", red);
		GUI_SetProperty(TXT_RED, "state", "show");
	}

	if(green == NULL)
	{
		GUI_SetProperty(IMG_GREEN, "state", "hide");
		GUI_SetProperty(TXT_GREEN, "state", "hide");
	}
	else
	{
		GUI_SetProperty(IMG_GREEN, "state", "show");
		GUI_SetProperty(TXT_GREEN, "string", green);
		GUI_SetProperty(TXT_GREEN, "state", "show");
	}

	if(yellow == NULL)
	{
		GUI_SetProperty(IMG_YELLOW, "state", "hide");
		GUI_SetProperty(TXT_YELLOW, "state", "hide");
	}
	else
	{
		GUI_SetProperty(IMG_YELLOW, "state", "show");
		GUI_SetProperty(TXT_YELLOW, "string", yellow);
		GUI_SetProperty(TXT_YELLOW, "state", "show");
	}

	if(blue == NULL)
	{
		GUI_SetProperty(IMG_BLUE, "state", "hide");
		GUI_SetProperty(TXT_BLUE, "state", "hide");
	}
	else
	{
		GUI_SetProperty(IMG_BLUE, "state", "show");
		GUI_SetProperty(TXT_BLUE, "string", blue);
		GUI_SetProperty(TXT_BLUE, "state", "show");
	}
}

static int transmission_usb_index_get(void)
{
	int res = -1;
	int i = 0;
	HotplugPartitionList *phpl = NULL;

	phpl = GxHotplugPartitionGet(HOTPLUG_TYPE_USB);
	if (phpl->partition_num < 1)
	{
		return -1;
	}

	for(i = 0; i < phpl->partition_num; i++)
	{
		if(strcmp(TRANSMISSION_PARTITION, phpl->partition[i].partition_entry) == 0)
		{
			res = i;
			break;
		}
	}

	return res;
}

static int transmission_usb_check(void)
{
	GxDiskInfo disk_info;
	HotplugPartitionList* partition_list = NULL;
	int i = -1;
	int free_space =0;

	if(check_usb_status() == false)
	{
		return -1;
	}

	i = transmission_usb_index_get();
	if (i<0)
	{
		return -2;
	}

	partition_list = GxHotplugPartitionGet(HOTPLUG_TYPE_USB);
	GxCore_DiskInfo(partition_list->partition[i].partition_entry, &disk_info);

  	free_space = disk_info.freed_size/SIZE_UNIT_M;
	if(free_space < MIN_USB_FREE_SPACE)
	{
		app_log_error("!!!!NO SPACE:%d!!!!!!\n",free_space);
		return -3;
	}
	return free_space;
}

static int app_transmission_get_list_map(void)
{
	int i=0,j=0;
	int res = 0;
	if(s_transmission_list_map)
	{
		GxCore_Free(s_transmission_list_map);
		s_transmission_list_map = NULL;
	}

	switch(s_transmission_list_type)
	{
		case TRANSMISSION_LIST_ALL:
			if(s_transmission_list)
			{
				res = s_transmission_list->torrent_num;
			}
			else
			{
				res = 0;
			}
			break;
		case TRANSMISSION_LIST_DOWNLOADING:
			if(s_transmission_list)
			{
				s_transmission_list_map = GxCore_Malloc((s_transmission_list->torrent_num+1)*sizeof(int));
				if(s_transmission_list_map)
				{
					for(i = 0,j = 0;i<s_transmission_list->torrent_num;i++)
					{
						if(4 == s_transmission_list->torrents[i].torrent_status)
						{
							j++;
							s_transmission_list_map[j]=i;
						}
					}
					s_transmission_list_map[0] = j;
					res = j;
				}
			}
			break;
		case TRANSMISSION_LIST_FINISH:
			if(s_transmission_list)
			{
				s_transmission_list_map = GxCore_Malloc((s_transmission_list->torrent_num+1)*sizeof(int));
				if(s_transmission_list_map)
				{
					for(i = 0,j = 0;i<s_transmission_list->torrent_num;i++)
					{
						if(1 == s_transmission_list->torrents[i].is_finish)
						{
							j++;
							s_transmission_list_map[j]=i;
						}
					}
					s_transmission_list_map[0] = j;
					res = j;
				}
			}
			break;
		default:
			break;
	}
	return res;
}

static gxapps_ret_t app_transmission_get_list(void)
{
	gxapps_ret_t ret= GXAPPS_SUCCESS;
	if(NULL != s_transmission_list)
	{
		gx_transmission_free_torrents(s_transmission_list);
		s_transmission_list = NULL;
	}

	ret = gx_transmission_get_torrents(&s_transmission_list);
	if(GXAPPS_SUCCESS != ret)
		return ret;
	if((TRANSMISSION_LIST_DOWNLOADING == s_transmission_list_type)||(TRANSMISSION_LIST_FINISH == s_transmission_list_type))
	{
		app_transmission_get_list_map();
	}
	return ret;
}


static int timer_transmission_info_show(void)
{
	uint32_t box_sel;
	static char info4[10];
	static char info1[20];
	static char info2[20];
	static char info3[20];
	static char subinfo[50]={0};
	int transmission_free_space =-1;

	char * usb=NULL;

	uint32_t totaltime;
	int h,m,s;
	if(NULL == s_transmission_list)
		return -1;

	transmission_free_space = transmission_usb_check();
	if(0 == strcasecmp(LIST_GROUP, GUI_GetFocusWidget()))
	{

		GUI_GetProperty(LIST_GROUP, "select", &box_sel);

		if(0 == s_transmission_list->torrent_num)
			return -1;
		if(box_sel > (s_transmission_list->torrent_num-1))
			return -2;
		if(0 == s_transmission_list->torrents[box_sel].download_speed)
		{
			strcpy(info4,"--:--:--");
		}
		else
		{
			totaltime = (s_transmission_list->torrents[box_sel].total_size-s_transmission_list->torrents[box_sel].download_size)/s_transmission_list->torrents[box_sel].download_speed;
			h = totaltime/3600;
			m = (totaltime - h*3600)/60;
			s = totaltime - h*3600 - m*60;
			sprintf(info4,"%d:%d:%d",h,m,s);
		}
		_transmission_display_speed_str(s_transmission_list->torrents[box_sel].upload_speed,info1);
		_transmission_display_size_str(s_transmission_list->torrents[box_sel].download_size,info2);
		_transmission_display_size_str(0,info3);

	}
	else if(0 == strcasecmp(BOX_GROUP, GUI_GetFocusWidget()))
	{
		int i = 0;
		long long total_dload=0;
		uint64_t total_speed = 0;
		for(i=0;i<s_transmission_list->torrent_num;i++)
			total_dload +=s_transmission_list->torrents[i].download_size;

		sprintf(info1,"%d",s_transmission_list->torrent_num);
		_transmission_display_size_str(total_dload,info2);
		_transmission_display_size_str(0,info3);
		for(i=0;i<s_transmission_list->torrent_num;i++)
			total_speed +=s_transmission_list->torrents[i].download_speed;
		_transmission_display_speed_str(total_speed,info4);

	}
	if(transmission_free_space > 0)
	{

		sprintf(subinfo,"%u M",transmission_free_space);
		usb = subinfo;

	}else if(transmission_free_space == -3)
	{
		sprintf(subinfo,"%s","No space");
		usb = subinfo;
	}
	else
	{
		usb = NULL;
	}
	app_transmission_show_info(info1,info2,info3,info4,usb);
	return 0;

}
static int timer_transmission_fresh(void)
{

	gxapps_ret_t ret= GXAPPS_SUCCESS;
	if(pop_msg_on)
		return -1;


	ret = app_transmission_get_list();
	if(ret < 0)
	{
		app_log_error("get list error,ret =%d\n",ret);
		return -1;
	}
	timer_transmission_info_show();

	GUI_SetProperty(LIST_GROUP, "update_all", NULL);
	GUI_SetInterface("flush",NULL);
	return 1;
}
static int timer_transmission_fresh_timeout(void *userdata)
{

	if(s_transmission_fresh_finish)
		timer_transmission_fresh();
	return 1;

}
static void timer_transmission_fresh_start(void)
{
	if( reset_timer(s_transmission_fresh_timer) != 0 )
	{
		s_transmission_fresh_timer = create_timer(timer_transmission_fresh_timeout,5000,NULL,TIMER_REPEAT);
	}
}

static void timer_transmission_refresh_stop(void)
{
	s_transmission_fresh_finish = false;
}

static void timer_transmission_refresh_start(void)
{
	s_transmission_fresh_finish = true;
}
void timer_transmission_fresh_stop(void)
{
	if(s_transmission_fresh_timer)
	{
		remove_timer(s_transmission_fresh_timer);
		s_transmission_fresh_timer = NULL;
	}
}

static int timer_transmission_time_update(void* userdata)
{
	char sys_time_str[24] = {0};
	char time_str_temp[24] = {0};

	g_AppTime.time_get(&g_AppTime, sys_time_str, sizeof(sys_time_str));
	// time
	if((strlen(sys_time_str) > 24) || (strlen(sys_time_str) < 5))
	{
		app_log_error("\nlenth error\n");
		return 1;
	}
	memcpy(time_str_temp,&sys_time_str[strlen(sys_time_str)-5],5);// 00:00
	GUI_SetProperty(TXT_TIME, "string", time_str_temp);

	// ymd
	memcpy(time_str_temp,sys_time_str,strlen(sys_time_str));// 0000/00/00
	time_str_temp[strlen(sys_time_str)-5] = '\0';
	GUI_SetProperty(TXT_DATE, "string", time_str_temp);
	return 0;
}

void timer_transmission_time_reset(void)
{
	if( reset_timer(s_transmission_time_timer) != 0 )
	{
		s_transmission_time_timer = create_timer(timer_transmission_time_update,1000,NULL,TIMER_REPEAT);
	}
}
void timer_transmission_time_stop(void)
{
	if(s_transmission_time_timer)
	{
		remove_timer(s_transmission_time_timer);
		s_transmission_time_timer = NULL;
	}
}

static void _transmission_setting_init(void)
{
	transmission_setting_opt=(SystemSettingOpt *)GxCore_Calloc(1, sizeof(SystemSettingOpt));
	transmission_setting_item=(SystemSettingItem *)GxCore_Calloc(1, sizeof(SystemSettingItem)*TRANSMISSION_SETTING_MAX);
}

static void _transmission_setting_destroy(void)
{
	if(transmission_setting_opt)
	{
		GxCore_Free(transmission_setting_opt);
		transmission_setting_opt=NULL;
	}
	if(transmission_setting_item)
	{
		GxCore_Free(transmission_setting_item);
		transmission_setting_item=NULL;
	}
}
static void _transmission_setting_get(void)
{
	int value = 0;

	GxBus_ConfigGetInt(TRANSMISSION_LIMIT_UP_KEY, &value, TRANSMISSION_DEFAULT_SPEED_LIMIT_UP);
	if(value<0)
	{
		value = 0;
	}
	transmission_setting.speed_limit_up= value;

	GxBus_ConfigGetInt(TRANSMISSION_LIMIT_DOWN_KEY, &value, TRANSMISSION_DEFAULT_SPEED_LIMIT_DOWN);
	if(value<0)
	{
		value = 0;
	}
	transmission_setting.speed_limit_down= value;
}

static void _transmission_setting_save(transmission_setting_t *ret_para)
{
	int value = 0;

	value = ret_para->speed_limit_up;
	if(value<0)
	{
		value = 0;
	}
	GxBus_ConfigSetInt(TRANSMISSION_LIMIT_UP_KEY,value);
	transmission_setting.speed_limit_up= value;

	value = ret_para->speed_limit_down;
	if(value<0)
	{
		value = 0;
	}
	GxBus_ConfigSetInt(TRANSMISSION_LIMIT_DOWN_KEY,value);
	transmission_setting.speed_limit_down= value;
}

static void _transmission_setting_all(void)
{
	gx_transmission_set_upspeed_limit(transmission_setting.speed_limit_up);
	gx_transmission_set_downspeed_limit(transmission_setting.speed_limit_down);
}

static void _transmission_setting_result_get(transmission_setting_t *para)
{
	sscanf(transmission_setting_item[TRANSMISSION_SETTING_LIMIT_UP].itemProperty.itemPropertyEdit.string,"%d",&para->speed_limit_up);
	sscanf(transmission_setting_item[TRANSMISSION_SETTING_LIMIT_DOWN].itemProperty.itemPropertyEdit.string,"%d",&para->speed_limit_down);
}

static int _transmission_setting_change(transmission_setting_t *para)
{
	if((transmission_setting.speed_limit_up == para->speed_limit_up)
		&&(transmission_setting.speed_limit_down == para->speed_limit_down))
	{
		return 0;
	}
	else
	{
		return 1;
	}
}

static int _transmission_setting_save_pop_cb(PopDlgRet ret)
{
    transmission_setting_t para;

    if(POP_VAL_OK == ret)
    {
        _transmission_setting_result_get(&para);
        _transmission_setting_save(&para);
        _transmission_setting_all();
        gx_transmission_setting_exec();
    }

    app_system_reset_unfocus_image();
    GUI_EndDialog(WND_SYSTEM_SETTING);
    GUI_SetProperty(WND_SYSTEM_SETTING,"draw_now",NULL);
    timer_transmission_time_reset();
    timer_transmission_refresh_start();

    return 0;
}

static bool _transmission_setting_check_cb(void)
{
    bool ret = false;
    transmission_setting_t para;

    _transmission_setting_result_get(&para);
    ret = _transmission_setting_change(&para);

    return ret;
}

static int _transmission_setting_exit_cb(ExitType Type)
{
    int ret = EXIT_RET_DEFAULT;

    if(EXIT_ABANDON == Type)
    {
        timer_transmission_time_reset();
        timer_transmission_refresh_start();
    }
    else
    {
        if(false == app_system_set_callback_handler(_transmission_setting_check_cb, _transmission_setting_save_pop_cb))
        {
            timer_transmission_time_reset();
            timer_transmission_refresh_start();
        }
        ret = EXIT_RET_POP;
    }

    return ret;
}

static void _transmission_setting_item_init(void)
{
	int len = 6;

	int value = 0;
	static char string1[10];
	static char string2[10];
	static char edit_len[2] = {0};
	sprintf(edit_len , "%d", len);
	GxBus_ConfigGetInt(TRANSMISSION_LIMIT_UP_KEY, &value, TRANSMISSION_DEFAULT_SPEED_LIMIT_UP);
	sprintf(string1,"%d",value);
	GxBus_ConfigGetInt(TRANSMISSION_LIMIT_DOWN_KEY, &value, TRANSMISSION_DEFAULT_SPEED_LIMIT_DOWN);
	sprintf(string2,"%d",value);

	transmission_setting_item[TRANSMISSION_SETTING_LIMIT_UP].itemTitle = "Limit up";
	transmission_setting_item[TRANSMISSION_SETTING_LIMIT_UP].itemType = ITEM_EDIT;
	transmission_setting_item[TRANSMISSION_SETTING_LIMIT_UP].itemProperty.itemPropertyEdit.string= string1;
	transmission_setting_item[TRANSMISSION_SETTING_LIMIT_UP].itemProperty.itemPropertyEdit.maxlen = edit_len;
	transmission_setting_item[TRANSMISSION_SETTING_LIMIT_UP].itemProperty.itemPropertyEdit.format = "edit_digit";
	transmission_setting_item[TRANSMISSION_SETTING_LIMIT_UP].itemProperty.itemPropertyEdit.default_intaglio = "-";
	transmission_setting_item[TRANSMISSION_SETTING_LIMIT_UP].itemStatus = ITEM_NORMAL;

	transmission_setting_item[TRANSMISSION_SETTING_LIMIT_DOWN].itemTitle = "Limit down";
	transmission_setting_item[TRANSMISSION_SETTING_LIMIT_DOWN].itemType = ITEM_EDIT;
	transmission_setting_item[TRANSMISSION_SETTING_LIMIT_DOWN].itemProperty.itemPropertyEdit.string= string2;
	transmission_setting_item[TRANSMISSION_SETTING_LIMIT_DOWN].itemProperty.itemPropertyEdit.maxlen = edit_len;
	transmission_setting_item[TRANSMISSION_SETTING_LIMIT_DOWN].itemProperty.itemPropertyEdit.format = "edit_digit";
	transmission_setting_item[TRANSMISSION_SETTING_LIMIT_DOWN].itemProperty.itemPropertyEdit.default_intaglio = "-";
	transmission_setting_item[TRANSMISSION_SETTING_LIMIT_DOWN].itemStatus = ITEM_NORMAL;

}

static void app_transmission_create_setting_menu(void)
{
	memset(&transmission_setting, 0, sizeof(transmission_setting_t));
	memset(transmission_setting_opt, 0 ,sizeof(SystemSettingOpt));
	memset(transmission_setting_item, 0 ,sizeof(SystemSettingItem)*TRANSMISSION_SETTING_MAX);

	_transmission_setting_get();
	_transmission_setting_item_init();
	transmission_setting_opt->menuTitle = "Transmission Setting";
	transmission_setting_opt->itemNum = TRANSMISSION_SETTING_MAX;
	transmission_setting_opt->item = transmission_setting_item;
	transmission_setting_opt->exit = _transmission_setting_exit_cb;
	transmission_setting_opt->timeDisplay = TIP_HIDE;
	app_system_set_create(transmission_setting_opt);
}

static gxapps_ret_t app_transmission_login(void)
{
	gxapps_ret_t ret = -1;
	app_transmission_begin();
	ret = gx_transmission_login();
	return ret;
}
void app_transmission_init(void)
{
	gxapps_ret_t ret = -1;

	GUI_SetFocusWidget(BOX_GROUP);
	app_transmission_show_color_tip(NULL,NULL,NULL,NULL);
	timer_transmission_time_update(NULL);
	timer_transmission_fresh_start();
	timer_transmission_time_reset();
	s_transmission_list_type = TRANSMISSION_LIST_ALL;
	app_transmission_show_popup_msg(STR_ID_LOGIN_WAIT, 0);
	GUI_SetInterface("flush",NULL);

	ret = app_transmission_login();
	app_transmission_hide_popup_msg();
	if(GXAPPS_SUCCESS != ret)
	{
		app_transmission_show_popup_msg("Login fail,Please try again", 1000);
	}
	else
	{
		_transmission_setting_all();
		gx_transmission_setting_exec();
		timer_transmission_fresh();
		timer_transmission_refresh_start();
	}
}

static void _transmission_box_group_ok_keypress(void)
{
	uint32_t box_sel;
	GUI_GetProperty(BOX_GROUP, "select", &box_sel);
	timer_transmission_refresh_stop();
	timer_transmission_time_stop();
	if(TRANSMISSION_LIST_MAX == box_sel)
	{
		app_transmission_create_setting_menu();
	}
	else
	{
		s_transmission_list_type = (transmission_list_type_e)box_sel;
		timer_transmission_fresh();
		timer_transmission_refresh_start();
	}
}

static int _app_transmission_filedlg_exit_cb(WndStatus ret)
{
	int str_len = 0;

	if(ret == WND_OK)
	{
		app_get_file_real_path(&s_file_path);
		if(s_file_path != NULL)
		{
			if(s_file_path_bak != NULL)
			{
				GxCore_Free(s_file_path_bak);
				s_file_path_bak = NULL;
			}

			str_len = strlen(s_file_path);
			s_file_path_bak = (char *)GxCore_Malloc(str_len + 1);
			if(s_file_path_bak != NULL)
			{
				memcpy(s_file_path_bak, s_file_path, str_len);
				s_file_path_bak[str_len] = '\0';
			}
			gx_transmission_add_torrent(s_file_path);
		}
	}
	pop_msg_on = false;

	timer_transmission_fresh();
	timer_transmission_refresh_start();

	return 0 ;
}

static status_t app_transmission_add(void)
{
	if(check_usb_status() == false)
	{
		PopDlg  pop;
		memset(&pop, 0, sizeof(PopDlg));
		pop.type = POP_TYPE_OK;
		pop.str = STR_ID_INSERT_USB;
		pop.mode = POP_MODE_UNBLOCK;
		popdlg_create(&pop);
	}
	else
	{
		FileListParam file_para;
		memset(&file_para, 0, sizeof(file_para));
		pop_msg_on = true;
		file_para.cur_path = s_file_path_bak;
		file_para.dest_path = &s_file_path;
		file_para.suffix = TRANSMISSION_FILE_SUFFFIX;
		file_para.dest_mode = DEST_MODE_FILE;
		file_para.exit_cb = _app_transmission_filedlg_exit_cb;
		app_get_file_path_dlg(&file_para);
	}
	return GXCORE_SUCCESS;
}

static void app_transmission_start(void)
{
	gxapps_ret_t ret = -1;

	uint32_t box_sel;
	int torrent_id=0;
	int id = 0;
	if(check_usb_status() == false)
	{

		app_transmission_show_popup_msg(STR_ID_INSERT_USB, 1000);
		return;
	}
	if(NULL == s_transmission_list)
		return;

	timer_transmission_refresh_stop();
	GUI_GetProperty(LIST_GROUP, "select", &box_sel);
	switch(s_transmission_list_type)
	{
		case TRANSMISSION_LIST_ALL:
			id = box_sel;
			torrent_id = s_transmission_list->torrents[id].torrent_id;
			break;
		case TRANSMISSION_LIST_DOWNLOADING:
		case TRANSMISSION_LIST_FINISH:
			if(NULL!=s_transmission_list_map&&s_transmission_list_map[0]>=box_sel)
			{
				id = s_transmission_list_map[box_sel];
				torrent_id = s_transmission_list->torrents[id].torrent_id;
			}
			else
				torrent_id = 0;
			break;
		default:
			torrent_id = 0;
			break;
	}

	if(torrent_id<=0)
		goto END;
	ret = gx_transmission_start_torrent(torrent_id);

	if(ret == GXAPPS_SUCCESS)
	{
		timer_transmission_fresh();

		if(torrent_id<=0)
			goto END;
		if(s_transmission_list->torrents[id].error != 0)
			app_transmission_show_popup_msg(s_transmission_list->torrents[id].err_str,1000);
	}
	else
		app_transmission_show_popup_msg("Please try again later",500);
END:
	timer_transmission_refresh_start();
	app_log_debug("app_transmission_start, ret=%d,torrent_id=%d,id=%d\n",ret,torrent_id,id);


}
static void app_transmission_stop(void)
{
	gxapps_ret_t ret = -1;

	uint32_t box_sel;
	int torrent_id=0;
	int id = 0;
	if(NULL == s_transmission_list)
		return;

	timer_transmission_refresh_stop();
	GUI_GetProperty(LIST_GROUP, "select", &box_sel);
	switch(s_transmission_list_type)
	{
		case TRANSMISSION_LIST_ALL:
			id = box_sel;
			torrent_id = s_transmission_list->torrents[id].torrent_id;
			break;
		case TRANSMISSION_LIST_DOWNLOADING:
		case TRANSMISSION_LIST_FINISH:
			if(NULL!=s_transmission_list_map&&s_transmission_list_map[0]>=box_sel)
			{
				id = s_transmission_list_map[box_sel];
				torrent_id = s_transmission_list->torrents[id].torrent_id;
			}
			else
				torrent_id = 0;
			break;
		default:
			torrent_id = 0;
			break;
	}

	if(torrent_id<=0)
		goto END;

	ret = gx_transmission_stop_torrent(torrent_id);

	if(ret == GXAPPS_SUCCESS)
	{
		timer_transmission_fresh();

		if(torrent_id<=0)
			goto END;
		if(s_transmission_list->torrents[id].error != 0)
			app_transmission_show_popup_msg(s_transmission_list->torrents[id].err_str,1000);
	}
	else
		app_transmission_show_popup_msg("Please try again later",500);
END:
	timer_transmission_refresh_start();
	app_log_debug("app_transmission_stop, ret=%d,torrent_id=%d,id=%d\n",ret,torrent_id,id);

}

static int _trans_del_cb2(PopDlgRet ret)
{
    int del_ret;
    int box_sel;

    if(POP_VAL_OK == ret)
        del_ret = gx_transmission_remove_torrent(thiz_del_torrent_id, 0);
    else
        del_ret = gx_transmission_remove_torrent(thiz_del_torrent_id, 1);

    pop_msg_on = false;
	if(del_ret == GXAPPS_SUCCESS)
	{
		box_sel = 0;
		GUI_SetProperty(LIST_GROUP, "select", &box_sel);
		timer_transmission_fresh();
	}
	else
		app_transmission_show_popup_msg("Please try again later",1000);
    return 0;
}

static int _trans_del_cb1(PopDlgRet ret)
{
	PopDlg	pop;

    if(POP_VAL_OK == ret)
	{
		memset(&pop, 0, sizeof(PopDlg));
		pop.type = POP_TYPE_YES_NO;
        pop.mode = POP_MODE_UNBLOCK;
		pop.str = STR_ID_DELETE_TORRENT_FILE;
        pop.exit_cb = _trans_del_cb2;
        popdlg_create(&pop);
	}
    else
    {
	    pop_msg_on = false;
    }

    return 0;
}

static void app_transmission_delete(void)
{
	PopDlg	pop;

	uint32_t box_sel;
	int id = 0;
	if(NULL == s_transmission_list)
		return;

    thiz_del_torrent_id = 0;
	timer_transmission_refresh_stop();
	GUI_GetProperty(LIST_GROUP, "select", &box_sel);
	switch(s_transmission_list_type)
	{
		case TRANSMISSION_LIST_ALL:
			id = box_sel;
			thiz_del_torrent_id = s_transmission_list->torrents[id].torrent_id;
			break;
		case TRANSMISSION_LIST_DOWNLOADING:
		case TRANSMISSION_LIST_FINISH:
			if(NULL!=s_transmission_list_map&&s_transmission_list_map[0]>=box_sel)
			{
				id = s_transmission_list_map[box_sel];
				thiz_del_torrent_id = s_transmission_list->torrents[id].torrent_id;
			}
			else
				thiz_del_torrent_id = 0;
			break;
		default:
			thiz_del_torrent_id = 0;
			break;
	}

	if(thiz_del_torrent_id <= 0)
		goto END;


	pop_msg_on = true;
    memset(&pop, 0, sizeof(PopDlg));
    pop.type = POP_TYPE_YES_NO;
    pop.mode = POP_MODE_UNBLOCK;
    pop.exit_cb = _trans_del_cb1;
    pop.str = STR_ID_SURE_DELETE;
    popdlg_create(&pop);
END:
	timer_transmission_refresh_start();
	app_log_debug("app_transmission_delete, torrent_id=%d\n", thiz_del_torrent_id);

}

static void app_transmission_stop_all_task(void)
{
	int i = 0;
	gxapps_ret_t ret = -1;
	gx_transmission_torrentlist_t * s_transmission_downoading_list =NULL;

	timer_transmission_refresh_stop();
	ret = gx_transmission_get_torrents(&s_transmission_downoading_list);

	if((ret == GXAPPS_SUCCESS)&&s_transmission_downoading_list&&(s_transmission_downoading_list->torrent_num>0))
	{
		for(i=0;i<s_transmission_downoading_list->torrent_num;i++)
		{
			gx_transmission_stop_torrent(s_transmission_downoading_list->torrents[i].torrent_id);
		}
	}

	if(s_transmission_downoading_list)
	{
		gx_transmission_free_torrents(s_transmission_downoading_list);
		s_transmission_downoading_list =NULL;
	}

	timer_transmission_fresh();
	timer_transmission_refresh_start();
	app_log_debug("%s\n",__FUNCTION__);
}

void app_transmission_msg_proc(GxMessage *msg)
{
	if(msg == NULL)
		return;

	switch(msg->msg_id)
	{
		case GXMSG_HOTPLUG_OUT:
			app_transmission_stop_all_task();
			break;
		default:
			break;
	}
}

SIGNAL_HANDLER int app_transmission_create(GuiWidget *widget, void *usrdata)
{
	_transmission_setting_get();
	app_transmission_init();
	_transmission_setting_init();
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_transmission_destroy(GuiWidget *widget, void *usrdata)
{
	timer_transmission_fresh_stop();
	timer_transmission_time_stop();
	timer_transmission_refresh_stop();
	pop_msg_on = false;
	gx_transmission_logout();
	_transmission_setting_destroy();

	if(s_transmission_list)
	{
		gx_transmission_free_torrents(s_transmission_list);
		s_transmission_list = NULL;
	}
	if(s_transmission_list_map)
	{
		GxCore_Free(s_transmission_list_map);
		s_transmission_list_map = NULL;
	}

	s_transmission_list_type = TRANSMISSION_LIST_ALL;
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_transmission_keypress(GuiWidget *widget, void *usrdata)
{
	GUI_Event *event = NULL;

	event = (GUI_Event *)usrdata;
	if(GUI_KEYDOWN ==  event->type)
	{
		switch(find_virtualkey_ex(event->key.scancode,event->key.sym))
		{
			case VK_BOOK_TRIGGER:
				GUI_EndDialog("after wnd_full_screen");
				break;
			case STBK_EXIT:
				if(pop_msg_on)
				{
					app_transmission_hide_popup_msg();
				}
				else
				{
					app_transmission_exit();
				}
				break;
			case STBK_0:
				if(!pop_msg_on)
				{
					timer_transmission_refresh_stop();
					app_transmission_add();
				}
				break;
			default:
				break;
		}
	}
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_transmission_list_keypress(GuiWidget *widget, void *usrdata)
{
	int ret = EVENT_TRANSFER_KEEPON;
	GUI_Event *event = NULL;

	event = (GUI_Event *)usrdata;
	switch(event->type)
	{
		case GUI_SERVICE_MSG:
			break;

		case GUI_MOUSEBUTTONDOWN:
			break;

		case GUI_KEYDOWN:
			switch(find_virtualkey_ex(event->key.scancode,event->key.sym))
			{
				case STBK_EXIT:
				case STBK_MENU:
					break;
				case STBK_OK:
					break;
				case STBK_LEFT:
					if(!pop_msg_on)
					{
						timer_transmission_refresh_stop();
						GUI_SetFocusWidget(BOX_GROUP);
						app_transmission_show_color_tip(NULL,NULL,NULL,NULL);
						timer_transmission_info_show();
						timer_transmission_fresh();
						timer_transmission_refresh_start();
					}
					break;
				case STBK_RED:
					if(!pop_msg_on)
					{
						app_transmission_start();
					}
					break;

				case STBK_GREEN:
					if(!pop_msg_on)
					{
						app_transmission_stop();
					}
					break;
				case STBK_YELLOW:
					if(!pop_msg_on)
					{
						app_transmission_stop_all_task();
					}
					break;

				case STBK_BLUE:
					if(!pop_msg_on)
					{
						app_transmission_delete();
					}
					break;
				case STBK_UP:
				case STBK_DOWN:
					if(pop_msg_on)
						ret = EVENT_TRANSFER_STOP;
					break;
				default:
					break;
			}
		default:
			break;
	}

	return ret;
}

SIGNAL_HANDLER int app_transmission_list_get_total(GuiWidget *widget, void *usrdata)
{
	int num = 0;
	switch(s_transmission_list_type)
	{
		case TRANSMISSION_LIST_ALL:
			if(NULL != s_transmission_list)
			{
				num = s_transmission_list->torrent_num;
			}
			else
				num = 0;
			break;
		case TRANSMISSION_LIST_DOWNLOADING:
		case TRANSMISSION_LIST_FINISH:
			if(NULL != s_transmission_list&&NULL!=s_transmission_list_map)
			{
				num = s_transmission_list_map[0];
			}
			else
				num = 0;
			break;
		default:
			num = 0;
			break;
	}
	return num;
}

SIGNAL_HANDLER int app_transmission_list_get_data(GuiWidget *widget, void *usrdata)
{
	#define MAX_LEN	30
	ListItemPara* item = NULL;
	static char buffer1[MAX_LEN];
	static char buffer2[MAX_LEN];
	static char buffer3[MAX_LEN];
	static char buffer4[MAX_LEN];
	int sel = 0;

	item = (ListItemPara*)usrdata;
	if((NULL == item) ||(0 > item->sel)||NULL ==s_transmission_list)
		return GXCORE_ERROR;
	sel = item->sel;
	if(TRANSMISSION_LIST_DOWNLOADING==s_transmission_list_type||TRANSMISSION_LIST_FINISH ==s_transmission_list_type)
	{
		if(NULL ==s_transmission_list_map)
			return GXCORE_ERROR;
		sel = s_transmission_list_map[item->sel+1];

	}
	//col-0: choice
	item->x_offset = 0;
	if(s_transmission_list->torrents[sel].torrent_status == 0)
	{
		item->image = "s_play2";
	}
	else
	{
		item->image = "s_pause2";
	}
	item->string = NULL;

	//col-1: NAME
	item = item->next;
	if(NULL == item) return GXCORE_ERROR;
	item->x_offset = 0;
	item->image = NULL;
	item->string = s_transmission_list->torrents[sel].torrent_name;

	//col-2: SIZE
	item = item->next;
	if(NULL == item) return GXCORE_ERROR;
	item->x_offset = 0;
	memset(buffer1, 0, MAX_LEN);
	item->image = NULL;
	_transmission_display_size_str(s_transmission_list->torrents[sel].total_size,buffer1);

	item->string = buffer1;
	//col-3: seed
	item = item->next;
	if(NULL == item) return GXCORE_ERROR;
	item->x_offset = 0;
	item->image = NULL;
	memset(buffer2, 0, MAX_LEN);
	sprintf(buffer2, "%d/%d",  s_transmission_list->torrents[sel].active_peers_num,s_transmission_list->torrents[sel].connected_peers_num);
	item->string = buffer2;
	//col-4: speed
	item = item->next;
	if(NULL == item) return GXCORE_ERROR;
	item->x_offset = 0;
	item->image = NULL;
	memset(buffer3, 0, MAX_LEN);
	_transmission_display_speed_str(s_transmission_list->torrents[sel].download_speed,buffer3);
	item->string = buffer3;
	//col-5: percent
	item = item->next;
	if(NULL == item) return GXCORE_ERROR;
	item->x_offset = 0;
	item->image = NULL;
	memset(buffer4, 0, MAX_LEN);
	sprintf(buffer4, "%2.2f%%",s_transmission_list->torrents[sel].download_percent * 100);
	item->string = buffer4;

	//app_log_debug("%d,%s,%s,%s,%s,%s\n",sel,s_transmission_list->torrents[sel].torrent_name,buffer1,buffer2,buffer3,buffer4);
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_transmission_list_change(GuiWidget *widget, void *usrdata)
{
	timer_transmission_info_show();
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_transmission_box_keypress(GuiWidget *widget, void *usrdata)
{
	GUI_Event *event = NULL;
	int ret = EVENT_TRANSFER_KEEPON;
	uint32_t box_sel;
	event = (GUI_Event *)usrdata;
	if(GUI_KEYDOWN ==  event->type)
	{
		switch(find_virtualkey_ex(event->key.scancode,event->key.sym))
		{
			case VK_BOOK_TRIGGER:
				break;

			case STBK_EXIT:
				break;
			case STBK_OK:
				if(!pop_msg_on)
				{
					_transmission_box_group_ok_keypress();
					ret = EVENT_TRANSFER_STOP;
				}
				break;
			case STBK_UP:
				if(!pop_msg_on)
				{
					GUI_GetProperty(BOX_GROUP, "select", &box_sel);
					if(TRANSMISSION_LIST_ALL == box_sel)
						box_sel = TRANSMISSION_LIST_MAX;
					else
						box_sel--;
					GUI_SetProperty(BOX_GROUP, "select", &box_sel);
					ret = EVENT_TRANSFER_STOP;
				}
				break;

			case STBK_DOWN:
				if(!pop_msg_on)
				{
					GUI_GetProperty(BOX_GROUP, "select", &box_sel);
					if(TRANSMISSION_LIST_MAX == box_sel)
						box_sel = TRANSMISSION_LIST_ALL;
					else
						box_sel++;
					GUI_SetProperty(BOX_GROUP, "select", &box_sel);
					ret = EVENT_TRANSFER_STOP;
				}
				break;

			case STBK_RIGHT:
				ret = EVENT_TRANSFER_STOP;
				if((!pop_msg_on)&&s_transmission_list&&(s_transmission_list->torrent_num>0))
				{
					GUI_SetFocusWidget(LIST_GROUP);
					app_transmission_show_color_tip("Start","Pause","Stop All","Delete");
					timer_transmission_info_show();
				}
				break;
			default:
				break;
		}
	}
	return ret;
}

#endif
