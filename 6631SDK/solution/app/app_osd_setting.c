#include "app.h"
#include "app_module.h"
#include "app_send_msg.h"
#include "app_wnd_system_setting_opt.h"
#include "app_default_params.h"
#include "module/config/gxconfig.h"
#include "app_pop.h"

/*
#define OSD_TRANS_30PERCENT 78*127/255
#define OSD_TRANS_40PERCENT 102*127/255
#define OSD_TRANS_50PERCENT 128*127/255
#define OSD_TRANS_60PERCENT 153*127/255
#define OSD_TRANS_70PERCENT 178*127/255
#define OSD_TRANS_80PERCENT 204*127/255
#define OSD_TRANS_90PERCENT 230*127/255
#define OSD_TRANS_100PERCENT 127
*/
enum OSD_ITEM_NAME
{
	ITEM_TRANS = 0,
	ITEM_BAR_TIMEOUT,
	//ITEM_VOL_TIMEOUT,
#if !OTT_SUPPORT
	ITEM_FREEZE,
#endif
	ITEM_OSD_TOTAL
};

typedef enum
{
	OSD_TRANS_40 = 0,
	OSD_TRANS_50,
	OSD_TRANS_60,
	OSD_TRANS_70,
	OSD_TRANS_80,
	OSD_TRANS_90,
	OSD_TRANS_100
}OsdTransSel;

typedef enum
{
	OSD_TIMEOUT_3S,
	OSD_TIMEOUT_5S,
	OSD_TIMEOUT_8S
}OsdTimeOutSel;

typedef struct
{
	int osd_tran;
	int bar_timeout;
	int vol_timeout;
	int freeze_switch;
}SysOsdPara;

static SystemSettingOpt s_osd_setting_opt;
static SystemSettingItem s_osd_item[ITEM_OSD_TOTAL];
static SysOsdPara s_sys_osd_para;
static int s_osd_menu_flag = 0;

#if WEBAPI_SUPPORT
static char* osd_tran_str[] = {
	"0%",
	"10%",
	"20%",
	"30%",
	"40%",
	"50%",
	"60%"
};
#define OSD_TRAN_TOTAL (sizeof(osd_tran_str)/sizeof(char *))
static char* osd_timeout_str[] = {
	"3s",
	"5s",
	"8s"
};
#define OSD_TIMEOUT_TOTAL (sizeof(osd_timeout_str)/sizeof(char *))
static char* osd_freeze_str[] = {
	"Black",
	"Freeze"
};
#define OSD_FREEZE_TOTAL (sizeof(osd_freeze_str)/sizeof(char *))
#endif

static OsdTransSel app_osd_trans_to_sel(int trans)
{
	OsdTransSel sel = OSD_TRANS_40;

	switch(trans)
	{
		case OSD_TRANS_40PERCENT:
			sel = OSD_TRANS_100;
			break;

		case OSD_TRANS_50PERCENT:
			sel = OSD_TRANS_90;
			break;

		case OSD_TRANS_60PERCENT:
			sel = OSD_TRANS_80;
			break;

		case OSD_TRANS_70PERCENT:
			sel = OSD_TRANS_70;
			break;

		case OSD_TRANS_80PERCENT:
			sel = OSD_TRANS_60;
			break;

		case OSD_TRANS_90PERCENT:
			sel = OSD_TRANS_50;
			break;

		case OSD_TRANS_100PERCENT:
			sel = OSD_TRANS_40;
			break;

		default:
			break;
	}

	return sel;
}

static int app_sel_to_osd_trans(OsdTransSel sel)
{
	int trans = OSD_TRANS_100PERCENT;

	switch(sel)
	{
		case OSD_TRANS_40:
			trans = OSD_TRANS_100PERCENT;
			break;

		case OSD_TRANS_50:
			trans = OSD_TRANS_90PERCENT;
			break;

		case OSD_TRANS_60:
			trans = OSD_TRANS_80PERCENT;
			break;

		case OSD_TRANS_70:
			trans = OSD_TRANS_70PERCENT;
			break;

		case OSD_TRANS_80:
			trans = OSD_TRANS_60PERCENT;
			break;

		case OSD_TRANS_90:
			trans = OSD_TRANS_50PERCENT;
			break;

		case OSD_TRANS_100:
			trans = OSD_TRANS_40PERCENT;
			break;

		default:
			break;
	}

	return trans;
}

static OsdTimeOutSel app_widget_timeout_to_sel(int sec)
{
	OsdTimeOutSel sel = OSD_TIMEOUT_3S;

	switch(sec)
	{
		case 3:
			sel = OSD_TIMEOUT_3S;
			break;

		case 5:
			sel = OSD_TIMEOUT_5S;
			break;

		case 8:
			sel = OSD_TIMEOUT_8S;
			break;

		default:
			break;
	}

	return sel;
}

static int app_sel_to_widget_timeout(OsdTimeOutSel sel)
{
	int sec = 3;

	switch(sel)
	{
		case OSD_TIMEOUT_3S:
			sec = 3;
			break;

		case OSD_TIMEOUT_5S:
			sec = 5;
			break;

		case OSD_TIMEOUT_8S:
			sec = 8;
			break;

		default:
			break;
	}

	return sec;
}

static void app_osd_system_para_get(void)
{
	int init_value = 0;

    	GxBus_ConfigGetInt(TRANS_LEVEL_KEY, &init_value, TRANS_LEVEL);
    	s_sys_osd_para.osd_tran= init_value;

    	GxBus_ConfigGetInt(INFOBAR_TIMEOUT_KEY, &init_value, INFOBAR_TIMEOUT);
    	s_sys_osd_para.bar_timeout= init_value;

    	GxBus_ConfigGetInt(VOLUMEBAR_TIMEOUT_KEY, &init_value, VOLUMEBAR_TIMEOUT);
    	s_sys_osd_para.vol_timeout= init_value;
#if !OTT_SUPPORT
	GxBus_ConfigGetInt(FREEZE_SWITCH, &init_value, FREEZE_SWITCH_VALUE);
	s_sys_osd_para.freeze_switch= init_value;
#endif

}

static void app_osd_result_para_get(SysOsdPara *ret_para)
{
    	ret_para->osd_tran = app_sel_to_osd_trans(s_osd_item[ITEM_TRANS].itemProperty.itemPropertyCmb.sel);
    	ret_para->bar_timeout = app_sel_to_widget_timeout(s_osd_item[ITEM_BAR_TIMEOUT].itemProperty.itemPropertyCmb.sel);
    	ret_para->vol_timeout = s_sys_osd_para.vol_timeout;
#if !OTT_SUPPORT
    	ret_para->freeze_switch= s_osd_item[ITEM_FREEZE].itemProperty.itemPropertyCmb.sel;
#endif
}

static bool app_osd_setting_para_change(SysOsdPara *ret_para)
{
	bool ret = FALSE;

	if(s_sys_osd_para.osd_tran!= ret_para->osd_tran)
	{
		ret = TRUE;
	}

	if(s_sys_osd_para.bar_timeout!= ret_para->bar_timeout)
	{
		ret = TRUE;
	}

	if(s_sys_osd_para.vol_timeout!= ret_para->vol_timeout)
	{
		ret = TRUE;
	}
#if !OTT_SUPPORT
	if(s_sys_osd_para.freeze_switch!= ret_para->freeze_switch)
	{
		ret = TRUE;
	}
#endif
	return ret;
}

static int app_osd_setting_para_save(SysOsdPara *ret_para)
{
	int init_value = 0;

	init_value = ret_para->osd_tran;
    	GxBus_ConfigSetInt(TRANS_LEVEL_KEY, init_value);

	init_value = ret_para->bar_timeout;
    	GxBus_ConfigSetInt(INFOBAR_TIMEOUT_KEY,init_value);

    	init_value = ret_para->vol_timeout;
    	GxBus_ConfigSetInt(VOLUMEBAR_TIMEOUT_KEY,init_value);
#if !OTT_SUPPORT
    	init_value = ret_para->freeze_switch;
    	GxBus_ConfigSetInt(FREEZE_SWITCH,init_value);
        app_send_msg_exec(GXMSG_PLAYER_FREEZE_FRAME_SWITCH, &init_value);
#endif

	return 0;
}

static int _osd_setting_save_pop_cb(PopDlgRet ret)
{
	SysOsdPara para_ret;

    if(POP_VAL_OK == ret)
    {
		app_osd_result_para_get(&para_ret);
        app_osd_setting_para_save(&para_ret);
    }
    else
    {
        GUI_SetInterface("osd_alpha_global", &s_sys_osd_para.osd_tran);
    }

    app_system_reset_unfocus_image();
    GUI_EndDialog(WND_SYSTEM_SETTING);
    GUI_SetProperty(WND_SYSTEM_SETTING,"draw_now",NULL);
    s_osd_menu_flag = 0;

    return 0;
}

static bool _osd_setting_check_cb(void)
{
    bool ret = false;
    SysOsdPara para_ret = {0};

    app_osd_result_para_get(&para_ret);
    ret = app_osd_setting_para_change(&para_ret);

    return ret;
}

static int app_osd_setting_exit_callback(ExitType exit_type)
{
    int ret = EXIT_RET_DEFAULT;

    if(EXIT_ABANDON == exit_type)
    {
        GUI_SetInterface("osd_alpha_global", &s_sys_osd_para.osd_tran);
        s_osd_menu_flag = 0;
    }
    else
    {
        if(false == app_system_set_callback_handler(_osd_setting_check_cb, _osd_setting_save_pop_cb))
        {
            s_osd_menu_flag = 0;
        }
        ret = EXIT_RET_POP;
    }

	return ret;
}

static int app_osd_trans_change_callback(int sel)
{
	int trans = OSD_TRANS_100PERCENT;

	trans = app_sel_to_osd_trans(sel);
	GUI_SetInterface("osd_alpha_global", &trans);

	return EVENT_TRANSFER_KEEPON;
}

static void app_osd_setting_trans_item_init(void)
{
	s_osd_item[ITEM_TRANS].itemTitle = STR_ID_TRAN;
	s_osd_item[ITEM_TRANS].itemType = ITEM_CHOICE;
	s_osd_item[ITEM_TRANS].itemProperty.itemPropertyCmb.content= "[0%,10%,20%,30%,40%,50%,60%]";//"[40%,50%,60%,70%,80%,90%,100%]";
	s_osd_item[ITEM_TRANS].itemProperty.itemPropertyCmb.sel = app_osd_trans_to_sel(s_sys_osd_para.osd_tran);
	s_osd_item[ITEM_TRANS].itemCallback.cmbCallback.CmbChange= app_osd_trans_change_callback;
	s_osd_item[ITEM_TRANS].itemStatus = ITEM_NORMAL;
}

static void app_osd_setting_bar_timeout_item_init(void)
{
	s_osd_item[ITEM_BAR_TIMEOUT].itemTitle = STR_ID_OSD_TIMEOUT;
	s_osd_item[ITEM_BAR_TIMEOUT].itemType = ITEM_CHOICE;
	s_osd_item[ITEM_BAR_TIMEOUT].itemProperty.itemPropertyCmb.content= "[3s,5s,8s]";
	s_osd_item[ITEM_BAR_TIMEOUT].itemProperty.itemPropertyCmb.sel = app_widget_timeout_to_sel(s_sys_osd_para.bar_timeout);
	s_osd_item[ITEM_BAR_TIMEOUT].itemCallback.cmbCallback.CmbChange= NULL;
	s_osd_item[ITEM_BAR_TIMEOUT].itemStatus = ITEM_NORMAL;
}

#if !OTT_SUPPORT
static void app_osd_setting_freeze_switch_item_init(void)
{
	s_osd_item[ITEM_FREEZE].itemTitle = STR_ID_CHANNEL_SWITCH;
	s_osd_item[ITEM_FREEZE].itemType = ITEM_CHOICE;
	s_osd_item[ITEM_FREEZE].itemProperty.itemPropertyCmb.content= "[Black,Freeze]";
	s_osd_item[ITEM_FREEZE].itemProperty.itemPropertyCmb.sel = s_sys_osd_para.freeze_switch;
	s_osd_item[ITEM_FREEZE].itemCallback.cmbCallback.CmbChange= NULL;
	s_osd_item[ITEM_FREEZE].itemStatus = ITEM_NORMAL;
}
#endif
void app_osd_setting_menu_exec(void)
{
	s_osd_menu_flag = 1;
	memset(&s_osd_setting_opt, 0 ,sizeof(s_osd_setting_opt));

	app_osd_system_para_get();

	s_osd_setting_opt.menuTitle = STR_ID_OSD_SET;
	s_osd_setting_opt.itemNum = ITEM_OSD_TOTAL;
	s_osd_setting_opt.timeDisplay = TIP_HIDE;
	s_osd_setting_opt.item = s_osd_item;

	app_osd_setting_trans_item_init();
	app_osd_setting_bar_timeout_item_init();
#if !OTT_SUPPORT
	app_osd_setting_freeze_switch_item_init();
#endif
	s_osd_setting_opt.exit = app_osd_setting_exit_callback;

	app_system_set_create(&s_osd_setting_opt);
}

#if WEBAPI_SUPPORT
#include <cJSON.h>

int app_webapi_osd_setting_busy(void)
{
	return s_osd_menu_flag;
}

void app_webapi_get_osd_settings(int *trans, int *timeout, int *freeze)
{
	int init_value = 0;

	GxBus_ConfigGetInt(TRANS_LEVEL_KEY, &init_value, TRANS_LEVEL);
	*trans = app_osd_trans_to_sel(init_value);

	GxBus_ConfigGetInt(INFOBAR_TIMEOUT_KEY, &init_value, INFOBAR_TIMEOUT);
	*timeout = app_widget_timeout_to_sel(init_value);

	GxBus_ConfigGetInt(FREEZE_SWITCH, &init_value, FREEZE_SWITCH_VALUE);
	*freeze = init_value;
}

void app_webapi_make_osd_data(cJSON *trans, cJSON *timeout, cJSON *freeze)
{
	int i = 0;

	for(i=0; i<OSD_TRAN_TOTAL; i++)
	{
		cJSON_AddItemToArray(trans, cJSON_CreateString(osd_tran_str[i]));
	}

	for(i=0; i<OSD_TIMEOUT_TOTAL; i++)
	{
		cJSON_AddItemToArray(timeout, cJSON_CreateString(osd_timeout_str[i]));
	}

	for(i=0; i<OSD_FREEZE_TOTAL; i++)
	{
		cJSON_AddItemToArray(freeze, cJSON_CreateString(osd_freeze_str[i]));
	}

}

void app_webapi_save_osd_settings(int trans, int timeout, int freeze)
{
	int init_value = 0;
	int set_value = 0;

	if((trans>=0)&&trans<OSD_TRAN_TOTAL)
	{
		GxBus_ConfigGetInt(TRANS_LEVEL_KEY, &init_value, TRANS_LEVEL);
		if(init_value != app_sel_to_osd_trans(trans))
		{
			set_value = app_sel_to_osd_trans(trans);
			GUI_SetInterface("osd_alpha_global", &set_value);
			GxBus_ConfigSetInt(TRANS_LEVEL_KEY, set_value);
		}
	}

	if((timeout>=0)&&timeout<OSD_TIMEOUT_TOTAL)
	{
    		GxBus_ConfigGetInt(INFOBAR_TIMEOUT_KEY, &init_value, INFOBAR_TIMEOUT);
		if(timeout != app_widget_timeout_to_sel(init_value))
		{
			set_value = app_sel_to_widget_timeout(timeout);
			GxBus_ConfigSetInt(INFOBAR_TIMEOUT_KEY,set_value);
		}
	}

	if((freeze>=0)&&freeze<OSD_FREEZE_TOTAL)
	{
		GxBus_ConfigGetInt(FREEZE_SWITCH, &init_value, FREEZE_SWITCH_VALUE);
		if(freeze != init_value)
		{
			set_value = freeze;
    			GxBus_ConfigSetInt(FREEZE_SWITCH,set_value);
			app_send_msg_exec(GXMSG_PLAYER_FREEZE_FRAME_SWITCH, &set_value);
		}
	}
}

#endif



