#include "app.h"
#include "app_module.h"
#include "app_send_msg.h"
#include "app_wnd_system_setting_opt.h"
#include "app_default_params.h"
#include "module/config/gxconfig.h"
#include "app_pop.h"
#include "app_utility.h"

enum TIME_AV_NAME
{
#if !INDEPEND_RECORD
	ITEM_TMS_FLAG = 0,
#endif
	ITEM_DURATION,
#if !INDEPEND_RECORD
	ITEM_SECTION_RECORD,
#endif
	ITEM_DISK_INFO,
    ITEM_PVR_TOTAL
};

typedef enum
{
	TMS_OFF,
	TMS_ON
}PvrTmsFlag;

typedef enum
{
	SRECORD_OFF,
	SRECORD_ON
}PvrSectionFlag;

typedef struct
{
	PvrTmsFlag      tms_flag;
    uint32_t        duration;   // seconds
	PvrSectionFlag  section_flag;
}PvrSetPara;

static SystemSettingOpt s_PvrSetOpt;
static SystemSettingItem s_PvrItem[ITEM_PVR_TOTAL];
static PvrSetPara s_PvrSetPara;

static void app_pvr_set_result_para_get(PvrSetPara *ret_para)
{
#if !INDEPEND_RECORD
	ret_para->tms_flag  = s_PvrItem[ITEM_TMS_FLAG].itemProperty.itemPropertyCmb.sel;
	ret_para->section_flag = s_PvrItem[ITEM_SECTION_RECORD].itemProperty.itemPropertyCmb.sel;
#endif
    ret_para->duration = s_PvrItem[ITEM_DURATION].itemProperty.itemPropertyCmb.sel;
}

static bool app_pvr_set_para_change(PvrSetPara *ret_para)
{
	bool ret = FALSE;

	if(s_PvrSetPara.tms_flag != ret_para->tms_flag)
	{
		ret = TRUE;
	}

	if(s_PvrSetPara.duration != ret_para->duration)
	{
		ret = TRUE;
	}
	if(s_PvrSetPara.section_flag != ret_para->section_flag)
	{
		ret = TRUE;
	}

	return ret;
}

static int app_pvr_set_tms_flag(int32_t tms)
{
    GxBus_ConfigSetInt(PVR_TIMESHIFT_KEY, tms);
    return 0;
}

static int app_pvr_set_duration_flag(uint32_t duration)
{
    GxBus_ConfigSetInt(PVR_DURATION_KEY, duration);

    // TODO: re-calculate pvr stop time

    return 0;
}

static int app_pvr_set_section_flag(int32_t section)
{
    GxBus_ConfigSetInt(PVR_SECTIONRECORD_KEY, section);
    return 0;
}

static int app_pvr_set_para_save(PvrSetPara *ret_para)
{
    app_pvr_set_tms_flag(ret_para->tms_flag);
    app_pvr_set_duration_flag(ret_para->duration);
	app_pvr_set_section_flag(ret_para->section_flag);
    return 0;
}

static int app_pvr_disk_info_press_callback(unsigned short key)
{
	if(key == STBK_OK)
	{
	#if 0
		if(FALSE == check_usb_status())
		{
			PopDlg  pop;
			memset(&pop, 0, sizeof(PopDlg));
        	pop.type = POP_TYPE_OK;
       		pop.str = STR_ID_NO_DEVICE;
       		pop.mode = POP_MODE_UNBLOCK;
			popdlg_create(&pop);

			return EVENT_TRANSFER_STOP;
		}
	#endif
		GUI_CreateDialog(WND_DISK_MANAGE);
	}
	return EVENT_TRANSFER_KEEPON;
}

#if !INDEPEND_RECORD
static int app_pvr_tms_flag_change_callbak(int flag)
{
    if(flag == TMS_OFF)
    {
        s_PvrItem[ITEM_SECTION_RECORD].itemProperty.itemPropertyCmb.sel = 0;
        app_system_set_item_property(ITEM_SECTION_RECORD,&(s_PvrItem[ITEM_SECTION_RECORD].itemProperty));
        app_system_set_item_state_update(ITEM_SECTION_RECORD,ITEM_DISABLE);
    }
    else
    {
        s_PvrItem[ITEM_SECTION_RECORD].itemProperty.itemPropertyCmb.sel = s_PvrSetPara.section_flag;
        app_system_set_item_property(ITEM_SECTION_RECORD,&(s_PvrItem[ITEM_SECTION_RECORD].itemProperty));
        app_system_set_item_state_update(ITEM_SECTION_RECORD,ITEM_NORMAL);
    }
    return EVENT_TRANSFER_KEEPON;

}

static void app_pvr_set_tms_flag_item_init(void)
{
    int32_t tms_flag=0;

	s_PvrItem[ITEM_TMS_FLAG].itemTitle = STR_ID_TMS;
	s_PvrItem[ITEM_TMS_FLAG].itemType = ITEM_CHOICE;
	s_PvrItem[ITEM_TMS_FLAG].itemProperty.itemPropertyCmb.content="[Off,On]";

	GxBus_ConfigGetInt(PVR_TIMESHIFT_KEY, &tms_flag, PVR_TIMESHIFT_FLAG);
	s_PvrItem[ITEM_TMS_FLAG].itemProperty.itemPropertyCmb.sel = tms_flag;
	s_PvrItem[ITEM_TMS_FLAG].itemCallback.cmbCallback.CmbChange= app_pvr_tms_flag_change_callbak;
	s_PvrItem[ITEM_TMS_FLAG].itemStatus = ITEM_NORMAL;

	s_PvrSetPara.tms_flag = tms_flag;
}

static void app_pvr_section_record_init(void)
{
	int32_t section_flag=0;
	s_PvrItem[ITEM_SECTION_RECORD].itemTitle = STR_ID_SECTION_RECORD;
	s_PvrItem[ITEM_SECTION_RECORD].itemType = ITEM_CHOICE;
	s_PvrItem[ITEM_SECTION_RECORD].itemProperty.itemPropertyCmb.content="[Off,On]";
	GxBus_ConfigGetInt(PVR_SECTIONRECORD_KEY, &section_flag, PVR_SECTIONRECORD_FLAG);
	s_PvrItem[ITEM_SECTION_RECORD].itemProperty.itemPropertyCmb.sel = section_flag;
	s_PvrItem[ITEM_SECTION_RECORD].itemCallback.cmbCallback.CmbChange= NULL;
    if(s_PvrSetPara.tms_flag == TMS_OFF)
    {
        s_PvrItem[ITEM_SECTION_RECORD].itemStatus = ITEM_DISABLE;
    }
    else
    {
        s_PvrItem[ITEM_SECTION_RECORD].itemStatus = ITEM_NORMAL;
    }
	s_PvrSetPara.section_flag = section_flag;
}
#endif

static void app_pvr_set_duration_item_init(void)
{
#define SHOW_TIME_LEN   (8)
    int32_t duration =0;

	s_PvrItem[ITEM_DURATION].itemTitle = STR_ID_REC_DURA;
	s_PvrItem[ITEM_DURATION].itemType = ITEM_CHOICE;
	s_PvrItem[ITEM_DURATION].itemProperty.itemPropertyCmb.content= "[2h00min,2h30min,3h00min,\
3h30min,4h00min,4h30min,5h00min,5h30min,6h00min,6h30min,7h00min,\
7h30min,8h00min,8h30min,9h00min,9h30min,10h00min,10h30min,11h00min,\
11h30min,12h00min]";

	GxBus_ConfigGetInt(PVR_DURATION_KEY, &duration, PVR_DURATION_VALUE);
	s_PvrItem[ITEM_DURATION].itemProperty.itemPropertyCmb.sel = duration;
	s_PvrItem[ITEM_DURATION].itemCallback.cmbCallback.CmbChange= NULL;
	s_PvrItem[ITEM_DURATION].itemStatus = ITEM_NORMAL;

	s_PvrSetPara.duration = (uint32_t)duration;
}

static void app_pvr_disk_info_item_init(void)
{
	s_PvrItem[ITEM_DISK_INFO].itemTitle = STR_ID_DISK_INFO;
	s_PvrItem[ITEM_DISK_INFO].itemType = ITEM_PUSH;
	s_PvrItem[ITEM_DISK_INFO].itemProperty.itemPropertyBtn.string= STR_ID_PRESS_OK;
	s_PvrItem[ITEM_DISK_INFO].itemCallback.btnCallback.BtnPress= app_pvr_disk_info_press_callback;
	s_PvrItem[ITEM_DISK_INFO].itemStatus = ITEM_NORMAL;
}

static int _pvr_set_save_pop_cb(PopDlgRet ret)
{
    PvrSetPara para_ret;

    if(POP_VAL_OK == ret)
    {
        app_pvr_set_result_para_get(&para_ret);
        app_pvr_set_para_save(&para_ret);
    }

    app_system_reset_unfocus_image();
    GUI_EndDialog(WND_SYSTEM_SETTING);
    GUI_SetProperty(WND_SYSTEM_SETTING,"draw_now",NULL);

    return 0;
}

static bool _pvr_set_check_cb(void)
{
    bool ret = false;
    PvrSetPara para_ret;

    app_pvr_set_result_para_get(&para_ret);
    ret = app_pvr_set_para_change(&para_ret);

    return ret;
}

static int app_pvr_set_exit_callback(ExitType exit_type)
{
    int ret = EXIT_RET_DEFAULT;

    if(EXIT_ABANDON == exit_type)
    {
        ;
    }
    else
    {
        app_system_set_callback_handler(_pvr_set_check_cb, _pvr_set_save_pop_cb);
        ret = EXIT_RET_POP;
    }

    return ret;
}

void app_pvr_set_menu_exec(void)
{
	memset(&s_PvrSetOpt, 0 ,sizeof(SystemSettingOpt));
	s_PvrSetOpt.menuTitle = STR_ID_PVR_MANAGE;
	if ( APP_THEME_CLASSIC_DARK ){
		s_PvrSetOpt.titleImage = "s_menu_icon_media3";
	}else{
		s_PvrSetOpt.titleImage = "s_title_left_media";
	}
	s_PvrSetOpt.itemNum = ITEM_PVR_TOTAL;
	s_PvrSetOpt.timeDisplay = TIP_HIDE;
	s_PvrSetOpt.item = s_PvrItem;
#if !INDEPEND_RECORD
	app_pvr_set_tms_flag_item_init();
	app_pvr_section_record_init();
#endif
	app_pvr_disk_info_item_init();
    app_pvr_set_duration_item_init();
	s_PvrSetOpt.exit = app_pvr_set_exit_callback;

	app_system_set_create(&s_PvrSetOpt);
}
void app_dvbt_pvr_set_menu_exec(void)
{
	memset(&s_PvrSetOpt, 0 ,sizeof(SystemSettingOpt));
	s_PvrSetOpt.menuTitle = STR_ID_PVR_MANAGE;
	s_PvrSetOpt.itemNum = ITEM_PVR_TOTAL;
    s_PvrSetOpt.topTipImgDisplay = TIP_HIDE;
    s_PvrSetOpt.bottmTipDisplay = TIP_HIDE;
	s_PvrSetOpt.timeDisplay = TIP_HIDE;
	s_PvrSetOpt.item = s_PvrItem;

#if !INDEPEND_RECORD
	app_pvr_set_tms_flag_item_init();
	app_pvr_section_record_init();
#endif
    app_pvr_set_duration_item_init();
	app_pvr_disk_info_item_init();

	s_PvrSetOpt.exit = app_pvr_set_exit_callback;

	app_system_set_create(&s_PvrSetOpt);
}

