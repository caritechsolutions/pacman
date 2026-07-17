#include "app.h"
#include "app_module.h"
#include "app_send_msg.h"
#include "app_wnd_system_setting_opt.h"
#include "app_utility.h"
#include "app_default_params.h"
#include "module/config/gxconfig.h"
#include "app_pop.h"
#include "app_book.h"

enum TIME_ITEM_NAME
{
	ITEM_GMT = 0,
	ITEM_DATE,
	ITEM_TIME,
	ITEM_OFFSET,
	ITEM_SUMMER,
	ITEM_TIME_TOTAL
};

typedef enum
{
	GMT_ON,
	GMT_OFF,
    GMT_NET
}TimeGMTSel;

typedef enum
{
	SUMMER_OFF,
	SUMMER_ON
}TimeSummerSel;

typedef struct
{
	enum _gmt_local gmt;
	time_t tm_sec;
	int zone;
	enum _summer summer;
    int32_t halfzone;
}SysTimePara;

static SystemSettingOpt s_time_setting_opt;
static SystemSettingItem s_time_item[ITEM_TIME_TOTAL];
static SysTimePara s_sys_time_para;

static char *s_time_date_str = NULL;
static char *s_time_hourmin_str = NULL;
static bool s_time_date_valid = false;
static int s_zone_sel = 0;

static time_t time_sec;
static int zone_change = 0 ;
static int zone_change_bak = 0;
static int summer_change = 0;
static int summer_change_bak = 0;


static void _time_date_change(void)
{
	char *str_temp = NULL;
	int  str_len = 0;

	if(s_time_hourmin_str != NULL)
	{
		GxCore_Free(s_time_hourmin_str);
		s_time_hourmin_str = NULL;
	}
    if(s_time_date_str != NULL)
    {
        GxCore_Free(s_time_date_str);
        s_time_date_str = NULL;
    }
	// change time_sec
	time_sec += app_time_zone_interval_sec_get(zone_change - zone_change_bak, summer_change - summer_change_bak);

	// get and set date for edit2
	str_temp = app_time_to_date_edit_str(time_sec);
	if(str_temp != NULL)
	{
		str_len = strlen(str_temp);
		s_time_date_str = (char*)GxCore_Malloc(str_len + 1);
		if(s_time_date_str != NULL)
		{
			memset(s_time_date_str, 0 , str_len + 1);
			memcpy(s_time_date_str, str_temp, str_len);
		}
		GxCore_Free(str_temp);
		str_temp = NULL;
	}
	s_time_item[ITEM_DATE].itemProperty.itemPropertyEdit.string = s_time_date_str;
        app_system_set_item_property(ITEM_DATE, &s_time_item[ITEM_DATE].itemProperty);



	//   get and set time for edit3
	str_temp = app_time_to_hourmin_edit_str(time_sec);
	if(str_temp != NULL)
	{
		str_len = strlen(str_temp);
		s_time_hourmin_str = (char*)GxCore_Malloc(str_len + 1);
		if(s_time_hourmin_str != NULL)
		{
			memset(s_time_hourmin_str, 0 , str_len + 1);
			memcpy(s_time_hourmin_str, str_temp, str_len);
		}
		GxCore_Free(str_temp);
		str_temp =NULL;
	}
	s_time_item[ITEM_TIME].itemProperty.itemPropertyEdit.string = s_time_hourmin_str;
    app_system_set_item_property(ITEM_TIME, &s_time_item[ITEM_TIME].itemProperty);

}
static TimeGMTSel app_gmt_to_sel(enum _gmt_local gmt)
{
	TimeGMTSel ret;
	switch(gmt)
	{
		case TIME_LOCAL:
			ret = GMT_OFF;
			break;

		case TIME_GMT:
			ret = GMT_ON;
			break;
        case TIME_NET:
			ret = GMT_NET;
			break;

		default:
			ret = GMT_OFF;
			break;
	}
	return ret;
}

static enum _gmt_local app_sel_to_gmt(TimeGMTSel sel)
{
	enum _gmt_local ret;
	switch(sel)
	{
		case GMT_OFF:
			ret = TIME_LOCAL;
			break;

		case GMT_ON:
			ret = TIME_GMT;
			break;
    	case GMT_NET:
			ret = TIME_NET;
			break;
		default:
			ret = TIME_LOCAL;
			break;
	}
	return ret;
}

static TimeSummerSel app_summer_to_sel(enum _summer summer)
{
	TimeGMTSel ret;
	switch(summer)
	{
		case TIME_SUMMER_OFF:
			ret = SUMMER_OFF;
			break;

		case TIME_SUMMER_ON:
			ret = SUMMER_ON;
			break;

		default:
			ret = SUMMER_OFF;
			break;
	}
	return ret;
}

static enum _gmt_local app_sel_to_summer(TimeSummerSel sel)
{
	enum _summer ret;
	switch(sel)
	{
		case SUMMER_OFF:
			ret = TIME_SUMMER_OFF;
			break;

		case SUMMER_ON:
			ret = TIME_SUMMER_ON;
			break;

		default:
			ret = TIME_SUMMER_OFF;
			break;
	}
	return ret;
}

static int app_time_zone_set_utc_string(int utc_mode)
{
#define TEXT_SYS_SET_TIMEZONE_UTC 	"text_system_setting_timezone"
#define IMG_SYS_SET_TIMEZONE_UTC_BAK 	"img_system_setting_timezone_back"
	char *country_str = app_time_zone_country_get_by_sel(utc_mode);

	if (country_str)
	{
		GUI_SetProperty(IMG_SYS_SET_TIMEZONE_UTC_BAK, "state", "show");
		GUI_SetProperty(TEXT_SYS_SET_TIMEZONE_UTC, "state", "show");
		GUI_SetProperty(TEXT_SYS_SET_TIMEZONE_UTC, "string", country_str);
	}
	else
	{
		GUI_SetProperty(TEXT_SYS_SET_TIMEZONE_UTC, "state", "hide");
		GUI_SetProperty(IMG_SYS_SET_TIMEZONE_UTC_BAK, "state", "hide");
	}
	zone_change = utc_mode - s_zone_sel;
	_time_date_change();
	zone_change_bak = zone_change;
	return EVENT_TRANSFER_KEEPON;
}

static int _app_time_zone_poplist_cb(int32_t ret_sel, unsigned short key)
{
    GUI_SetProperty(app_system_get_combobox(ITEM_OFFSET), "select", &ret_sel);
    return 0;
}

static int app_time_zone_press_cb(int key)
{
	PopList pop_list;

	if (key == STBK_OK) {
		app_system_set_item_data_update(ITEM_OFFSET);
		memset(&pop_list, 0, sizeof(PopList));
		pop_list.title = STR_ID_TIME_ZONE;
		pop_list.item_num = app_time_zone_name_array_size_get();
		pop_list.item_content = app_time_zone_name_array_get();
		pop_list.sel = s_time_item[ITEM_OFFSET].itemProperty.itemPropertyCmb.sel;
		pop_list.show_num = false;
		pop_list.mode = POP_MODE_UNBLOCK;
		pop_list.exit_cb = _app_time_zone_poplist_cb;
		poplist_create(&pop_list);
		return EVENT_TRANSFER_STOP;
	}
	return EVENT_TRANSFER_KEEPON;
}

static void app_time_system_para_get(void)
{
	int init_value = 0;
	time_t sec;
	time_t temp_sec;

	sec = g_AppTime.utc_get(&g_AppTime);
	temp_sec = get_display_time_by_timezone(sec);
	s_sys_time_para.tm_sec = temp_sec - temp_sec % 60;

	GxBus_ConfigGetInt(TIME_MODE, &init_value, TIME_MODE_VALUE);
	s_sys_time_para.gmt = init_value;
	app_time_zone_config_get(&s_sys_time_para.zone, &s_sys_time_para.halfzone, &init_value);
	s_sys_time_para.summer = init_value;

	// get the start time_sec
	time_sec = s_sys_time_para.tm_sec;
}

static void app_time_result_para_get(SysTimePara *ret_para)
{
	time_t date;
	time_t time;
	char* datestring;
	GUI_GetProperty(app_system_get_edit(ITEM_DATE),"string",&datestring);
	s_time_item[ITEM_DATE].itemProperty.itemPropertyEdit.string = datestring;

	if(app_edit_str_to_date_sec(s_time_item[ITEM_DATE].itemProperty.itemPropertyEdit.string, &date) == GXCORE_SUCCESS)
	{
		time = app_edit_str_to_hourmin_sec(s_time_item[ITEM_TIME].itemProperty.itemPropertyEdit.string);
		ret_para->tm_sec = date + time;
		ret_para->gmt = app_sel_to_gmt(s_time_item[ITEM_GMT].itemProperty.itemPropertyCmb.sel);
		ret_para->zone =app_time_sel_to_zone(s_time_item[ITEM_OFFSET].itemProperty.itemPropertyCmb.sel);
		ret_para->halfzone = app_time_sel_to_halfzone(s_time_item[ITEM_OFFSET].itemProperty.itemPropertyCmb.sel);
		ret_para->summer = app_sel_to_summer(s_time_item[ITEM_SUMMER].itemProperty.itemPropertyCmb.sel);

		if(ret_para->gmt == TIME_LOCAL)
		{
			time = app_time_to_utc_time(ret_para->tm_sec, ret_para->zone, ret_para->halfzone, ret_para->summer);
			s_time_date_valid = app_time_range_valid(time);
		}
		else
		{
			s_time_date_valid = true;
		}
	}
	else
	{
		s_time_date_valid = false;
	}
}

static bool app_time_setting_para_change(SysTimePara *ret_para)
{
	bool ret = FALSE;

	if(s_time_date_valid == false)
		return ret;

	if(s_sys_time_para.gmt != ret_para->gmt)
	{
		ret = TRUE;
	}

	if(s_zone_sel != app_time_zone_to_sel(ret_para->zone,ret_para->halfzone))
	{
		ret = TRUE;
	}

	if(s_sys_time_para.summer!= ret_para->summer)
	{
		ret = TRUE;
	}

	if(ret_para->gmt == TIME_LOCAL)
	{
		if(s_sys_time_para.tm_sec!= ret_para->tm_sec)
		{
			ret = TRUE;
		}
	}
	return ret;
}

static int app_time_setting_para_save(SysTimePara *ret_para)
{
	time_cfg time = {0};

	time.ymd = TIME_YMD;
	time.gmt_local = ret_para->gmt;
	time.zone = ret_para->zone;
	time.half_zone = ret_para->halfzone;
	time.summer = ret_para->summer;
	g_AppTime.mode_set(&g_AppTime, &time);

	if(TIME_LOCAL == ret_para->gmt)
	{
		time_t sec;

		sec = get_local_time_by_timezone(ret_para->tm_sec);
		g_AppTime.sync(&g_AppTime, sec+3);
	}

	g_AppTime.start(&g_AppTime);

	return 0;
}

static int _time_setting_save_pop_cb(PopDlgRet ret)
{
    SysTimePara para_ret = {0};

    if(POP_VAL_OK == ret)
    {
        app_time_result_para_get(&para_ret);
        app_time_setting_para_save(&para_ret);
    }

    app_system_reset_unfocus_image();
    GUI_EndDialog(WND_SYSTEM_SETTING);
    GUI_SetProperty(WND_SYSTEM_SETTING,"draw_now",NULL);

    return 0;
}

static bool _time_setting_check_cb(void)
{
    bool ret = false;
    SysTimePara para_ret = {0};

    app_time_result_para_get(&para_ret);
    ret = app_time_setting_para_change(&para_ret);

    return ret;
}

static int _time_setting_date_valid_pop_cb(PopDlgRet ret)
{
    app_system_reset_unfocus_image();
    GUI_EndDialog(WND_SYSTEM_SETTING);
    GUI_SetProperty(WND_SYSTEM_SETTING,"draw_now",NULL);

    return 0;
}

static int app_time_setting_exit_callback(ExitType exit_type)
{
    int ret = EXIT_RET_DEFAULT;
    SysTimePara para_ret = {0};
    PopDlg  pop;

    if(exit_type == EXIT_ABANDON)
    {
        ;
    }
    else
    {
        app_time_result_para_get(&para_ret);
        if(s_time_date_valid == false)
        {
            memset(&pop, 0, sizeof(PopDlg));
            pop.type = POP_TYPE_OK;
            pop.str = STR_ID_ERR_DATE;
            pop.mode = POP_MODE_UNBLOCK;
            pop.exit_cb = _time_setting_date_valid_pop_cb;
            popdlg_create(&pop);
        }
        else
        {
            app_system_set_callback_handler(_time_setting_check_cb, _time_setting_save_pop_cb);
        }
        ret = EXIT_RET_POP;
    }

    if(s_time_date_str != NULL)
    {
        GxCore_Free(s_time_date_str);
        s_time_date_str = NULL;
    }

    if(s_time_hourmin_str != NULL)
    {
        GxCore_Free(s_time_hourmin_str);
        s_time_hourmin_str = NULL;
    }

    return ret;
}

static int app_time_gmt_change_callback(int sel)
{
	if((sel == GMT_ON)||(sel == GMT_NET))
	{
		app_system_set_item_state_update(ITEM_DATE, ITEM_DISABLE);
		app_system_set_item_state_update(ITEM_TIME, ITEM_DISABLE);
		app_system_set_item_state_update(ITEM_OFFSET, ITEM_NORMAL);//20140113
		app_system_set_item_state_update(ITEM_SUMMER, ITEM_NORMAL);//20140113
	}
	else
	{
		app_system_set_item_state_update(ITEM_DATE, ITEM_NORMAL);
		app_system_set_item_state_update(ITEM_TIME, ITEM_NORMAL);
		app_system_set_item_state_update(ITEM_OFFSET, ITEM_DISABLE);//20140113
		app_system_set_item_state_update(ITEM_SUMMER, ITEM_DISABLE);//20140113
	}

	return EVENT_TRANSFER_KEEPON;
}

static int app_time_summer_change_callback(int sel)
{
	if(s_sys_time_para.summer == TIME_SUMMER_OFF)
	{
		if(sel == SUMMER_ON)
		{
			summer_change = 1;
			_time_date_change();
			summer_change_bak = summer_change;
		}
		else
		{
			summer_change = 0;
			_time_date_change();
			summer_change_bak = summer_change;
		}
	}
	else if(s_sys_time_para.summer == TIME_SUMMER_ON)
	{
		if(sel == SUMMER_OFF)
		{
			summer_change = -1;
			_time_date_change();
			summer_change_bak = summer_change;
		}
		else
		{
			summer_change = 0;
			_time_date_change();
			summer_change_bak = summer_change;
		}
	}

	return EVENT_TRANSFER_KEEPON;
}

static void app_time_setting_gmt_item_init(void)
{
	s_time_item[ITEM_GMT].itemTitle = STR_ID_MODE;
	s_time_item[ITEM_GMT].itemType = ITEM_CHOICE;
#if GET_NET_TIME_SUPPORT || OTT_SUPPORT
	s_time_item[ITEM_GMT].itemProperty.itemPropertyCmb.content= "[DVB,Manual,Net]";
#else
    s_time_item[ITEM_GMT].itemProperty.itemPropertyCmb.content= "[DVB,Manual]";
#endif
	s_time_item[ITEM_GMT].itemProperty.itemPropertyCmb.sel = app_gmt_to_sel(s_sys_time_para.gmt);
	s_time_item[ITEM_GMT].itemCallback.cmbCallback.CmbChange= app_time_gmt_change_callback;
#if OTT_SUPPORT
    s_time_item[ITEM_GMT].itemStatus = ITEM_DISABLE;
#else
	s_time_item[ITEM_GMT].itemStatus = ITEM_NORMAL;
#endif
}

static int app_time_setting_date_edit_reach_end(char *str)
{
    uint32_t sel = 0;
    uint32_t cur_sel = 0;
#define TIME_SETTING_DATE_LEN 10
    GUI_GetProperty("edit_system_setting_opt2", "select", &cur_sel);
    sel = (cur_sel == 0 ? TIME_SETTING_DATE_LEN - 1 : 0);
    GUI_SetProperty("edit_system_setting_opt2", "select", &sel);
    return EVENT_TRANSFER_STOP;
}

static int app_time_setting_time_edit_reach_end(char *str)
{
    uint32_t sel = 0;
    uint32_t cur_sel = 0;
#define TIME_SETTING_TIME_LEN 5
    GUI_GetProperty("edit_system_setting_opt3", "select", &cur_sel);
    sel = (cur_sel == 0 ? TIME_SETTING_TIME_LEN - 1 : 0);
    GUI_SetProperty("edit_system_setting_opt3", "select", &sel);
    return EVENT_TRANSFER_STOP;
}

static void app_time_setting_date_item_init(void)
{
	char *str_temp = NULL;
	int str_len = 0;

	s_time_item[ITEM_DATE].itemTitle = STR_ID_DATE;
	s_time_item[ITEM_DATE].itemType = ITEM_EDIT;
	s_time_item[ITEM_DATE].itemProperty.itemPropertyEdit.format = app_date_edit_format_get();
	s_time_item[ITEM_DATE].itemProperty.itemPropertyEdit.maxlen = "10";
	s_time_item[ITEM_DATE].itemProperty.itemPropertyEdit.intaglio = NULL;
	s_time_item[ITEM_DATE].itemProperty.itemPropertyEdit.default_intaglio = NULL;

	if(s_time_date_str != NULL)
	{
		GxCore_Free(s_time_date_str);
		s_time_date_str = NULL;
	}
	str_temp = app_time_to_date_edit_str(s_sys_time_para.tm_sec);
	if(str_temp != NULL)
	{
		str_len = strlen(str_temp);
		s_time_date_str = (char*)GxCore_Malloc(str_len + 1);
		if(s_time_date_str != NULL)
		{
			memset(s_time_date_str, 0 , str_len + 1);
			memcpy(s_time_date_str, str_temp, str_len);
		}
		GxCore_Free(str_temp);
		str_temp =NULL;
	}
	s_time_item[ITEM_DATE].itemProperty.itemPropertyEdit.string = s_time_date_str;

	s_time_item[ITEM_DATE].itemCallback.editCallback.EditReachEnd =  app_time_setting_date_edit_reach_end;
	s_time_item[ITEM_DATE].itemCallback.editCallback.EditPress= NULL;
	if((s_sys_time_para.gmt == TIME_GMT)||(s_sys_time_para.gmt == TIME_NET))

	{
		s_time_item[ITEM_DATE].itemStatus = ITEM_DISABLE;
	}
	else
	{
		s_time_item[ITEM_DATE].itemStatus = ITEM_NORMAL;
	}
}

static void app_time_setting_time_item_init(void)
{
	char *str_temp = NULL;
	int str_len = 0;

	s_time_item[ITEM_TIME].itemTitle = STR_ID_TIME;
	s_time_item[ITEM_TIME].itemType = ITEM_EDIT;
	s_time_item[ITEM_TIME].itemProperty.itemPropertyEdit.format = "edit_time";
	s_time_item[ITEM_TIME].itemProperty.itemPropertyEdit.maxlen = "5";
	s_time_item[ITEM_TIME].itemProperty.itemPropertyEdit.intaglio = NULL;
	s_time_item[ITEM_TIME].itemProperty.itemPropertyEdit.default_intaglio = NULL;

	if(s_time_hourmin_str != NULL)
	{
		GxCore_Free(s_time_hourmin_str);
		s_time_hourmin_str = NULL;
	}
	str_temp = app_time_to_hourmin_edit_str(s_sys_time_para.tm_sec);
	if(str_temp != NULL)
	{
		str_len = strlen(str_temp);
		s_time_hourmin_str = (char*)GxCore_Malloc(str_len + 1);
		if(s_time_hourmin_str != NULL)
		{
			memset(s_time_hourmin_str, 0 , str_len + 1);
			memcpy(s_time_hourmin_str, str_temp, str_len);
		}
		GxCore_Free(str_temp);
		str_temp =NULL;
	}
	s_time_item[ITEM_TIME].itemProperty.itemPropertyEdit.string = s_time_hourmin_str;

	s_time_item[ITEM_TIME].itemCallback.editCallback.EditReachEnd= app_time_setting_time_edit_reach_end;
	s_time_item[ITEM_TIME].itemCallback.editCallback.EditPress= NULL;
	if((s_sys_time_para.gmt == TIME_GMT)||(s_sys_time_para.gmt == TIME_NET))

	{
		s_time_item[ITEM_TIME].itemStatus = ITEM_DISABLE;
	}
	else
	{
		s_time_item[ITEM_TIME].itemStatus = ITEM_NORMAL;
	}
}

static void app_time_setting_zone_item_init(void)
{
	s_zone_sel = app_time_zone_to_sel(s_sys_time_para.zone, s_sys_time_para.halfzone);
	s_time_item[ITEM_OFFSET].itemTitle = STR_ID_TIME_ZONE;
	s_time_item[ITEM_OFFSET].itemType = ITEM_CHOICE;
	s_time_item[ITEM_OFFSET].itemProperty.itemPropertyCmb.content= app_time_zone_content_get();
	s_time_item[ITEM_OFFSET].itemProperty.itemPropertyCmb.sel = s_zone_sel;
	s_time_item[ITEM_OFFSET].itemCallback.cmbCallback.CmbChange= app_time_zone_set_utc_string;
	s_time_item[ITEM_OFFSET].itemCallback.cmbCallback.CmbPress = app_time_zone_press_cb;
	s_time_item[ITEM_OFFSET].itemStatus = ITEM_NORMAL;
}

static void app_time_setting_summer_item_init(void)
{
	s_time_item[ITEM_SUMMER].itemTitle = STR_ID_TIME_SUMMER;
	s_time_item[ITEM_SUMMER].itemType = ITEM_CHOICE;
	s_time_item[ITEM_SUMMER].itemProperty.itemPropertyCmb.content= "[Off,On]";
	s_time_item[ITEM_SUMMER].itemProperty.itemPropertyCmb.sel = app_summer_to_sel(s_sys_time_para.summer);
	s_time_item[ITEM_SUMMER].itemCallback.cmbCallback.CmbChange = app_time_summer_change_callback;
	s_time_item[ITEM_SUMMER].itemStatus = ITEM_NORMAL;
}


void app_time_setting_menu_exec(void)
{
	zone_change = 0 ;
	zone_change_bak = 0;
	summer_change = 0;
	summer_change_bak = 0;

	memset(&s_time_setting_opt, 0 ,sizeof(s_time_setting_opt));
	app_time_system_para_get();

	s_time_setting_opt.menuTitle = STR_ID_TIME_SET;
	s_time_setting_opt.itemNum = ITEM_TIME_TOTAL;
	s_time_setting_opt.timeDisplay = TIP_HIDE;
	s_time_setting_opt.item = s_time_item;

	app_time_setting_gmt_item_init();
	app_time_setting_date_item_init();
	app_time_setting_time_item_init();
	app_time_setting_zone_item_init();
	app_time_setting_summer_item_init();

	s_time_setting_opt.exit = app_time_setting_exit_callback;

	app_system_set_create(&s_time_setting_opt);

	GUI_SetProperty("text_system_setting_timezone","state","show");
	GUI_SetProperty("img_system_setting_timezone_back","state","show");
}

