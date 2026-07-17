#include "app.h"
#include "app_book.h"
#include "app_module.h"
#include "app_wnd_system_setting_opt.h"
#include "sys_setting_manage.h"

#if AUTO_STANDBY_SUPPORT
#define AUTO_STANDBY_INFO_SEC (120)
#define HOUR_PER_SEC  3600
#define HALF_HOUR_SEC 1800
#define HALF_SLEEP_CON "[Off,0.5,1,1.5,2,2.5,3,3.5,4,4.5,5,5.5,6,6.5,7,7.5,8,8.5,9,9.5,10,10.5,11,11.5,12]"
#define TIME_SLEEP_CON "[Off,1,2,3,4,5,6,7,8,9,10,11,12]"

enum SLEEP_ITEM_NAME
{
    ITEM_THREEHOUR_STANDBY = 0,
    ITEM_SLEEP_TIME_SET,
    ITEM_SLEEP_TOTAL,
};

enum _auto_sleep
{
	AUTO_STANDBY_OFF,
	AUTO_STANDBY_ON
};

enum _half_hour
{
    HALF_HOUR_OFF,
    HALF_HOUR_ON
};

typedef struct
{
    enum _auto_sleep threesleep;
    enum _auto_sleep sleep;
}SysSleepPara;

static char *s_sleep_time_content[25] = {"Off","0.5","1","1.5","2","2.5","3","3.5","4","4.5","5","5.5","6","6.5","7","7.5","8","8.5","9","9.5","10","10.5","11","11.5","12"};

static SystemSettingOpt s_sleep_setting_opt;
static SystemSettingItem s_sleep_item[ITEM_SLEEP_TOTAL];
static SysSleepPara s_sys_sleep_para;
static event_list * sp_AutoStandbyExec = NULL;



extern void app_low_power(time_t sleep_time, uint32_t scan_code);
extern void app_main_menu_listview_update(void);
void app_set_auto_sleep_time(void);

static void app_sleep_system_para_get(void)
{
    int init_value = 0;
    GxBus_ConfigGetInt(TIME_AUTO_STANDBY, &init_value, TIME_AUTO_STANDBY_VALUE);
    s_sys_sleep_para.threesleep = init_value;
    GxBus_ConfigGetInt(SLEEP_KEY, &init_value, SLEEP_VALUE);
    s_sys_sleep_para.sleep = init_value;
}

static int app_sleep_setting_exit_callback(ExitType exit_type)
{
    bool reset_flag = false;
    if(s_sleep_item[ITEM_THREEHOUR_STANDBY].itemProperty.itemPropertyCmb.sel != s_sys_sleep_para.threesleep)
    {
        s_sys_sleep_para.threesleep = s_sleep_item[ITEM_THREEHOUR_STANDBY].itemProperty.itemPropertyCmb.sel;
        GxBus_ConfigSetInt(TIME_AUTO_STANDBY, s_sys_sleep_para.threesleep);
        reset_flag = true;
    }
    if(s_sleep_item[ITEM_SLEEP_TIME_SET].itemProperty.itemPropertyCmb.sel != s_sys_sleep_para.sleep)
    {
        s_sys_sleep_para.sleep = s_sleep_item[ITEM_SLEEP_TIME_SET].itemProperty.itemPropertyCmb.sel;
        GxBus_ConfigSetInt(SLEEP_KEY, s_sys_sleep_para.sleep);
        reset_flag = true;
    }
    if(reset_flag)
    {
        app_set_auto_sleep_time();
    }
    return 0;
}

static int app_sleep_change_threesleep_cb(int sel)
{
    if(sel == AUTO_STANDBY_ON)
    {
        s_sleep_item[ITEM_SLEEP_TIME_SET].itemProperty.itemPropertyCmb.sel=3;
        app_system_set_item_property(ITEM_SLEEP_TIME_SET, &s_sleep_item[ITEM_SLEEP_TIME_SET].itemProperty);
        app_system_set_item_state_update(ITEM_SLEEP_TIME_SET, ITEM_DISABLE);
    }
    else
    {
        s_sleep_item[ITEM_SLEEP_TIME_SET].itemProperty.itemPropertyCmb.sel=0;
        app_system_set_item_property(ITEM_SLEEP_TIME_SET, &s_sleep_item[ITEM_SLEEP_TIME_SET].itemProperty);
        app_system_set_item_state_update(ITEM_SLEEP_TIME_SET, ITEM_NORMAL);
    }
    return EVENT_TRANSFER_KEEPON;
}


static void app_threesleep_item_init(void)
{
    s_sleep_item[ITEM_THREEHOUR_STANDBY].itemTitle = STR_ID_AUTO_STANDBY;
    s_sleep_item[ITEM_THREEHOUR_STANDBY].itemType = ITEM_CHOICE;
    s_sleep_item[ITEM_THREEHOUR_STANDBY].itemProperty.itemPropertyCmb.content = "[Off,On]";
    s_sleep_item[ITEM_THREEHOUR_STANDBY].itemProperty.itemPropertyCmb.sel = s_sys_sleep_para.threesleep;
    s_sleep_item[ITEM_THREEHOUR_STANDBY].itemCallback.cmbCallback.CmbChange= app_sleep_change_threesleep_cb;
    s_sleep_item[ITEM_THREEHOUR_STANDBY].itemStatus = ITEM_NORMAL;
}

static void app_sleep_item_init(void)
{
    int init_value = 0;
    s_sleep_item[ITEM_SLEEP_TIME_SET].itemTitle = STR_ID_SLEEP_SET;
    s_sleep_item[ITEM_SLEEP_TIME_SET].itemType = ITEM_CHOICE;
    GxBus_ConfigGetInt(TIME_HALF_HOUR, &init_value, TIME_HALF_HOUR_VALUE);
    if(init_value == HALF_HOUR_ON)
    {
        s_sleep_item[ITEM_SLEEP_TIME_SET].itemProperty.itemPropertyCmb.content = HALF_SLEEP_CON;
    }
    else
    {
        s_sleep_item[ITEM_SLEEP_TIME_SET].itemProperty.itemPropertyCmb.content = TIME_SLEEP_CON;
    }
    s_sleep_item[ITEM_SLEEP_TIME_SET].itemProperty.itemPropertyCmb.sel = s_sys_sleep_para.sleep;
    s_sleep_item[ITEM_SLEEP_TIME_SET].itemCallback.cmbCallback.CmbChange= NULL;
    if(s_sys_sleep_para.threesleep == AUTO_STANDBY_ON)
    {
        s_sleep_item[ITEM_SLEEP_TIME_SET].itemStatus = ITEM_DISABLE;
    }
    else
    {
        s_sleep_item[ITEM_SLEEP_TIME_SET].itemStatus = ITEM_NORMAL;
    }
}

void app_sleep_setting_menu_exec(void)
{
    memset(&s_sleep_setting_opt, 0 ,sizeof(s_sleep_setting_opt));
    app_sleep_system_para_get();

    s_sleep_setting_opt.menuTitle = STR_ID_SLEEP_SET;
    s_sleep_setting_opt.itemNum = ITEM_SLEEP_TOTAL;
    s_sleep_setting_opt.timeDisplay = TIP_HIDE;
    s_sleep_setting_opt.item = s_sleep_item;

    app_threesleep_item_init();
    app_sleep_item_init();
    s_sleep_setting_opt.exit = app_sleep_setting_exit_callback;
    app_system_set_create(&s_sleep_setting_opt);

}




static int _standy_exec(PopDlgRet ret)
{
	time_t timerDur;
	time_t cur_time;

	if(ret == POP_VAL_OK)
	{
		cur_time = g_AppTime.utc_get(&g_AppTime);
		timerDur =  g_AppBook.get_sleep_time(cur_time);
		GxCore_ThreadDelay(1000);
		app_low_power(timerDur, 0);
	}

	return 0;
}

static int _auto_standy_exec_timeout(void* data)
{
	PopDlg standy_pop;
    APP_TIMER_REMOVE(sp_AutoStandbyExec);
	if((GUI_CheckDialog(WND_SEARCH) == GXCORE_SUCCESS)
		|| (GUI_CheckDialog(WND_UPGRADE_PROCESS)== GXCORE_SUCCESS)
		|| (GUI_CheckDialog(WND_OTA_UPGRADE)== GXCORE_SUCCESS)
        || (GUI_CheckDialog(WND_TKGS_UPGRADE_PROCESS)==GXCORE_SUCCESS))
	{
		return 0;
	}


	{
		g_AppTtxSubt.ttx_magz_opt(&g_AppTtxSubt, STBK_EXIT);
        if (GUI_CheckDialog(WND_CHANNEL_INFO_SIGNAL) == GXCORE_SUCCESS)
        {
            GUI_EndDialog("after wnd_full_screen");
            GUI_SetInterface("flush", NULL);
        }

		memset(&standy_pop, 0, sizeof(PopDlg));
		standy_pop.type = POP_TYPE_YES_NO;
		standy_pop.mode = POP_MODE_UNBLOCK;
		standy_pop.str = STR_ID_POWER_OFF;
		standy_pop.timeout_sec = AUTO_STANDBY_INFO_SEC;
		standy_pop.default_ret = POP_VAL_OK;
		standy_pop.show_time = true;
		standy_pop.exit_cb = _standy_exec;
		popdlg_create(&standy_pop);
	}
	return 0;
}

void app_init_sleep_set(void)
{
    APP_TIMER_REMOVE(sp_AutoStandbyExec);
}

void app_set_auto_sleep_time(void)
{
    int half_value = 0;
	time_t standy_exec_sec;

    APP_TIMER_REMOVE(sp_AutoStandbyExec);
    int init_value = 0;
    GxBus_ConfigGetInt(TIME_HALF_HOUR, &half_value, TIME_HALF_HOUR_VALUE);
    GxBus_ConfigGetInt(SLEEP_KEY, &init_value, SLEEP_VALUE);
    if(init_value != 0)
    {
        if(half_value == HALF_HOUR_ON)
        {
            standy_exec_sec = init_value * HALF_HOUR_SEC  -  AUTO_STANDBY_INFO_SEC;

        }
        else
        {
            standy_exec_sec = init_value * HOUR_PER_SEC -  AUTO_STANDBY_INFO_SEC;
        }
        APP_TIMER_ADD(sp_AutoStandbyExec, _auto_standy_exec_timeout, standy_exec_sec*1000, TIMER_REPEAT);
    }

	return;
}

void app_sleep_set(int flag)
{
    int init_value = 0;
    int32_t *first = NULL;
    int32_t end = 0;
    int32_t sleep_sel = 0;

    GxBus_ConfigGetInt(SLEEP_KEY, &sleep_sel, SLEEP_VALUE);
    GxBus_ConfigGetInt(TIME_HALF_HOUR, &init_value, TIME_HALF_HOUR_VALUE);

    first = &sleep_sel;
    if(init_value == HALF_HOUR_ON)
    {
        end = 25;
    }
    else
    {
        end = 13 ;
    }

    if (flag == 0)
    {
        if (*first == 0)
            (*first) = (end - 1);
        else
            (*first)--;
    }
    else if (flag == 1)
    {
        if ((*first) == (end - 1))
            *first = 0;
        else
            (*first)++;
    }

    GxBus_ConfigSetInt(SLEEP_KEY, sleep_sel);
    app_set_auto_sleep_time();
}

int app_sleep_setting_keypress(int key)
{
    if(STBK_LEFT != key && STBK_RIGHT != key)
        return EVENT_TRANSFER_KEEPON;

    if(STBK_LEFT == key)
        app_sleep_set(0);
    else if(STBK_RIGHT == key)
        app_sleep_set(1);

    app_main_menu_listview_update();

    return EVENT_TRANSFER_KEEPON;
}

char *app_sleep_setting_content_get(void)
{
    int init_value = 0;
    int32_t timesleep;

    GxBus_ConfigGetInt(SLEEP_KEY, &timesleep, SLEEP_VALUE);
    GxBus_ConfigGetInt(TIME_HALF_HOUR, &init_value, TIME_HALF_HOUR_VALUE);
    if(init_value == HALF_HOUR_ON)
    {
        return s_sleep_time_content[timesleep];

    }
    else
    {
        return s_sleep_time_content[2*timesleep];
    }
}
#endif
