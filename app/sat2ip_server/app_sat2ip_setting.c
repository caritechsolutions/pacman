#include "sat2ip_server_platform.h"
#if SAT2IP_SERVER_SUPPORT
#include "app.h"
#include "app_wnd_system_setting_opt.h"

typedef enum {
	ITEM_SAT2IP_MODE = 0,
	ITEM_SAT2IP_PRIO,
#if SAT2IP_CONFIGURE_SUPPORT
	ITEM_SAT2IP_SWITCH,
	ITEM_SAT2IP_RTSP,
	ITEM_SAT2IP_CLIENT,
#endif
	ITEM_SAT2IP_SAVE,
	ITEM_SAT2IP_TOTAL
} ITEM_SAT2IP;


static char *s_sat2ip_item_title[ITEM_SAT2IP_TOTAL] = {
	STR_ID_SAT2IP_MODE,
	STR_ID_SAT2IP_PRI,
#if SAT2IP_CONFIGURE_SUPPORT
	STR_ID_SAT2IP_CONFLICT_OPERAT,
	STR_ID_SAT2IP_DMS_RTSP_SUP,
	STR_ID_SAT2IP_SAME_IP_CLIENT,
#endif
	STR_ID_SAVE
};

static char *s_sat2ip_item_content[ITEM_SAT2IP_TOTAL] = {
	"[Off,On]",
	"[Low,High]",
#if SAT2IP_CONFIGURE_SUPPORT
	"[Disable,Enable]",
	"[Off,On]",
	"[Simple,Multiple]",
#endif
	"Press OK"
};

static SystemSettingOpt  s_Sat2IpOpt;
static SystemSettingItem s_Sat2IpItem[ITEM_SAT2IP_TOTAL];
static int s_sat2ip_mode = 0;
static int s_sat2ip_prio = 0;
#if SAT2IP_CONFIGURE_SUPPORT
static int s_sat2ip_switch = 0;
static int s_sat2ip_rtsp = 0;
static int s_sat2ip_client = 0;
#endif

static int app_sat2ip_ok_item_press_cb(unsigned short key)
{
	if(key == STBK_OK)
	{
		app_system_set_item_data_update(ITEM_SAT2IP_MODE);
		app_system_set_item_data_update(ITEM_SAT2IP_PRIO);
#if SAT2IP_CONFIGURE_SUPPORT
		app_system_set_item_data_update(ITEM_SAT2IP_SWITCH);
		app_system_set_item_data_update(ITEM_SAT2IP_RTSP);
		app_system_set_item_data_update(ITEM_SAT2IP_CLIENT);
#endif

		int tmp_mode = s_Sat2IpItem[ITEM_SAT2IP_MODE].itemProperty.itemPropertyCmb.sel;
		int tmp_prio = s_Sat2IpItem[ITEM_SAT2IP_PRIO].itemProperty.itemPropertyCmb.sel;
#if SAT2IP_CONFIGURE_SUPPORT
		int tmp_switch = s_Sat2IpItem[ITEM_SAT2IP_SWITCH].itemProperty.itemPropertyCmb.sel;
		int tmp_rtsp = s_Sat2IpItem[ITEM_SAT2IP_RTSP].itemProperty.itemPropertyCmb.sel;
		int tmp_client = s_Sat2IpItem[ITEM_SAT2IP_CLIENT].itemProperty.itemPropertyCmb.sel;
#endif

		if (tmp_mode != s_sat2ip_mode
				|| tmp_prio != s_sat2ip_prio
#if SAT2IP_CONFIGURE_SUPPORT
				|| tmp_switch != s_sat2ip_switch
				|| tmp_rtsp != s_sat2ip_rtsp
				|| tmp_client != s_sat2ip_client
#endif
				)
		{
			PopDlg pop = {0};

			popdlg_destroy();
			pop.type = POP_TYPE_NO_BTN;
			pop.format = POP_FORMAT_DLG;
			pop.str = STR_ID_SAVING;
			pop.mode = POP_MODE_UNBLOCK;
			pop.timeout_sec = 3;
			pop.exit_cb = NULL;
			popdlg_create(&pop);

#if SAT2IP_CONFIGURE_SUPPORT
			s_sat2ip_client = tmp_client;
			app_sat2ip_client_flag_set(tmp_client);
			s_sat2ip_switch = tmp_switch;
			app_sat2ip_switch_flag_set(tmp_switch);
			s_sat2ip_rtsp = tmp_rtsp;
			app_sat2ip_rtsp_flag_set(tmp_rtsp);
#endif
			s_sat2ip_prio = tmp_prio;
			app_sat2ip_prio_flag_set(tmp_prio);
			s_sat2ip_mode = tmp_mode;
			app_sat2ip_use_flag_set(tmp_mode);
		}

		return EVENT_TRANSFER_STOP;
	}

	return EVENT_TRANSFER_KEEPON;
}

static void app_sat2ip_mode_item_init(void)
{
 	s_Sat2IpItem[ITEM_SAT2IP_MODE].itemTitle = s_sat2ip_item_title[ITEM_SAT2IP_MODE];
	s_Sat2IpItem[ITEM_SAT2IP_MODE].itemType = ITEM_CHOICE;
	s_Sat2IpItem[ITEM_SAT2IP_MODE].itemProperty.itemPropertyCmb.content= s_sat2ip_item_content[ITEM_SAT2IP_MODE];
	s_Sat2IpItem[ITEM_SAT2IP_MODE].itemProperty.itemPropertyCmb.sel = s_sat2ip_mode;
	s_Sat2IpItem[ITEM_SAT2IP_MODE].itemCallback.cmbCallback.CmbChange = NULL;
	s_Sat2IpItem[ITEM_SAT2IP_MODE].itemStatus = ITEM_NORMAL;
}

static void app_sat2ip_prio_item_init(void)
{
	s_Sat2IpItem[ITEM_SAT2IP_PRIO].itemTitle = s_sat2ip_item_title[ITEM_SAT2IP_PRIO];
	s_Sat2IpItem[ITEM_SAT2IP_PRIO].itemType = ITEM_CHOICE;
	s_Sat2IpItem[ITEM_SAT2IP_PRIO].itemProperty.itemPropertyCmb.content= s_sat2ip_item_content[ITEM_SAT2IP_PRIO];
	s_Sat2IpItem[ITEM_SAT2IP_PRIO].itemProperty.itemPropertyCmb.sel = s_sat2ip_prio;
	s_Sat2IpItem[ITEM_SAT2IP_PRIO].itemCallback.cmbCallback.CmbChange = NULL;
	s_Sat2IpItem[ITEM_SAT2IP_PRIO].itemStatus = ITEM_NORMAL;
}

#if SAT2IP_CONFIGURE_SUPPORT
static void app_sat2ip_switch_item_init(void)
{
 	s_Sat2IpItem[ITEM_SAT2IP_SWITCH].itemTitle = s_sat2ip_item_title[ITEM_SAT2IP_SWITCH];
	s_Sat2IpItem[ITEM_SAT2IP_SWITCH].itemType = ITEM_CHOICE;
	s_Sat2IpItem[ITEM_SAT2IP_SWITCH].itemProperty.itemPropertyCmb.content= s_sat2ip_item_content[ITEM_SAT2IP_SWITCH];
	s_Sat2IpItem[ITEM_SAT2IP_SWITCH].itemProperty.itemPropertyCmb.sel = s_sat2ip_switch;
	s_Sat2IpItem[ITEM_SAT2IP_SWITCH].itemCallback.cmbCallback.CmbChange = NULL;
	s_Sat2IpItem[ITEM_SAT2IP_SWITCH].itemStatus = ITEM_NORMAL;
}

static void app_sat2ip_rtsp_item_init(void)
{
 	s_Sat2IpItem[ITEM_SAT2IP_RTSP].itemTitle = s_sat2ip_item_title[ITEM_SAT2IP_RTSP];
	s_Sat2IpItem[ITEM_SAT2IP_RTSP].itemType = ITEM_CHOICE;
	s_Sat2IpItem[ITEM_SAT2IP_RTSP].itemProperty.itemPropertyCmb.content= s_sat2ip_item_content[ITEM_SAT2IP_RTSP];
	s_Sat2IpItem[ITEM_SAT2IP_RTSP].itemProperty.itemPropertyCmb.sel = s_sat2ip_rtsp;
	s_Sat2IpItem[ITEM_SAT2IP_RTSP].itemCallback.cmbCallback.CmbChange = NULL;
	s_Sat2IpItem[ITEM_SAT2IP_RTSP].itemStatus = ITEM_NORMAL;
}

static void app_sat2ip_client_item_init(void)
{
	s_Sat2IpItem[ITEM_SAT2IP_CLIENT].itemTitle = s_sat2ip_item_title[ITEM_SAT2IP_CLIENT];
	s_Sat2IpItem[ITEM_SAT2IP_CLIENT].itemType = ITEM_CHOICE;
	s_Sat2IpItem[ITEM_SAT2IP_CLIENT].itemProperty.itemPropertyCmb.content= s_sat2ip_item_content[ITEM_SAT2IP_CLIENT];
	s_Sat2IpItem[ITEM_SAT2IP_CLIENT].itemProperty.itemPropertyCmb.sel = s_sat2ip_client;
	s_Sat2IpItem[ITEM_SAT2IP_CLIENT].itemCallback.cmbCallback.CmbChange = NULL;
	s_Sat2IpItem[ITEM_SAT2IP_CLIENT].itemStatus = ITEM_NORMAL;
}

#endif

static void app_sat2ip_save_item_init(void)
{
 	s_Sat2IpItem[ITEM_SAT2IP_SAVE].itemTitle = s_sat2ip_item_title[ITEM_SAT2IP_SAVE];
	s_Sat2IpItem[ITEM_SAT2IP_SAVE].itemType = ITEM_PUSH;
	s_Sat2IpItem[ITEM_SAT2IP_SAVE].itemProperty.itemPropertyBtn.string= s_sat2ip_item_content[ITEM_SAT2IP_SAVE];
	s_Sat2IpItem[ITEM_SAT2IP_SAVE].itemCallback.btnCallback.BtnPress = app_sat2ip_ok_item_press_cb;
	s_Sat2IpItem[ITEM_SAT2IP_SAVE].itemStatus = ITEM_NORMAL;
}


static bool _sat2ip_check_cb(void)
{
    bool ret = false;

	app_system_set_item_data_update(ITEM_SAT2IP_MODE);
	app_system_set_item_data_update(ITEM_SAT2IP_PRIO);
#if SAT2IP_CONFIGURE_SUPPORT
	app_system_set_item_data_update(ITEM_SAT2IP_SWITCH);
	app_system_set_item_data_update(ITEM_SAT2IP_RTSP);
	app_system_set_item_data_update(ITEM_SAT2IP_CLIENT);
#endif

	int tmp_mode = s_Sat2IpItem[ITEM_SAT2IP_MODE].itemProperty.itemPropertyCmb.sel;
	int tmp_prio = s_Sat2IpItem[ITEM_SAT2IP_PRIO].itemProperty.itemPropertyCmb.sel;
#if SAT2IP_CONFIGURE_SUPPORT
	int tmp_switch = s_Sat2IpItem[ITEM_SAT2IP_SWITCH].itemProperty.itemPropertyCmb.sel;
	int tmp_rtsp = s_Sat2IpItem[ITEM_SAT2IP_RTSP].itemProperty.itemPropertyCmb.sel;
	int tmp_client = s_Sat2IpItem[ITEM_SAT2IP_CLIENT].itemProperty.itemPropertyCmb.sel;
#endif

	if (tmp_mode != s_sat2ip_mode
			|| tmp_prio != s_sat2ip_prio
#if SAT2IP_CONFIGURE_SUPPORT
			|| tmp_switch != s_sat2ip_switch
			|| tmp_rtsp != s_sat2ip_rtsp
			|| tmp_client != s_sat2ip_client
#endif
			)
	{

		ret = true;
	}

    return ret;
}

static int _sat2ip_save_pop_cb(PopDlgRet ret)
{

    if(POP_VAL_OK == ret)
    {
		int tmp_mode = s_Sat2IpItem[ITEM_SAT2IP_MODE].itemProperty.itemPropertyCmb.sel;
		int tmp_prio = s_Sat2IpItem[ITEM_SAT2IP_PRIO].itemProperty.itemPropertyCmb.sel;
#if SAT2IP_CONFIGURE_SUPPORT
		int tmp_switch = s_Sat2IpItem[ITEM_SAT2IP_SWITCH].itemProperty.itemPropertyCmb.sel;
		int tmp_rtsp = s_Sat2IpItem[ITEM_SAT2IP_RTSP].itemProperty.itemPropertyCmb.sel;
		int tmp_client = s_Sat2IpItem[ITEM_SAT2IP_CLIENT].itemProperty.itemPropertyCmb.sel;
#endif

		if (tmp_mode != s_sat2ip_mode
				|| tmp_prio != s_sat2ip_prio
#if SAT2IP_CONFIGURE_SUPPORT
				|| tmp_switch != s_sat2ip_switch
				|| tmp_rtsp != s_sat2ip_rtsp
				|| tmp_client != s_sat2ip_client
#endif
				)
		{
			PopDlg pop = {0};

			popdlg_destroy();
			pop.type = POP_TYPE_NO_BTN;
			pop.format = POP_FORMAT_DLG;
			pop.str = STR_ID_SAVING;
			pop.mode = POP_MODE_UNBLOCK;
			pop.timeout_sec = 3;
			pop.exit_cb = NULL;
			popdlg_create(&pop);

#if SAT2IP_CONFIGURE_SUPPORT
			s_sat2ip_client = tmp_client;
			app_sat2ip_client_flag_set(tmp_client);
			s_sat2ip_switch = tmp_switch;
			app_sat2ip_switch_flag_set(tmp_switch);
			s_sat2ip_rtsp = tmp_rtsp;
			app_sat2ip_rtsp_flag_set(tmp_rtsp);
#endif
			s_sat2ip_prio = tmp_prio;
			app_sat2ip_prio_flag_set(tmp_prio);
			s_sat2ip_mode = tmp_mode;
			app_sat2ip_use_flag_set(tmp_mode);
		}
    }

    app_system_reset_unfocus_image();
    GUI_EndDialog(WND_SYSTEM_SETTING);
    GUI_SetProperty(WND_SYSTEM_SETTING,"draw_now",NULL);
    return 0;
}

static int app_sat2ip_exit_callback(ExitType exit_type)
{
    int ret = EXIT_RET_DEFAULT;

    if(EXIT_ABANDON == exit_type)
    {
			//do nothing
    }
    else
    {
        if(false == app_system_set_callback_handler(_sat2ip_check_cb, _sat2ip_save_pop_cb))
        {
        	//do nothing
        }
        ret = EXIT_RET_POP;
    }

    return ret;
}

void app_sat2ip_setting_menu_exec(void)
{
	s_sat2ip_mode = app_sat2ip_use_flag_get();
	s_sat2ip_prio = app_sat2ip_prio_flag_get();
#if SAT2IP_CONFIGURE_SUPPORT
	s_sat2ip_switch = app_sat2ip_switch_flag_get();
	s_sat2ip_rtsp = app_sat2ip_rtsp_flag_get();
	s_sat2ip_client = app_sat2ip_client_flag_get();
#endif

	memset(&s_Sat2IpOpt, 0 ,sizeof(SystemSettingOpt));
	s_Sat2IpOpt.menuTitle = STR_ID_SAT2IP_SETTING;
	//s_Sat2IpOpt.titleImage = "s_title_left_utility";
	s_Sat2IpOpt.itemNum = ITEM_SAT2IP_TOTAL;
	s_Sat2IpOpt.item = s_Sat2IpItem;
	s_Sat2IpOpt.timeDisplay = TIP_HIDE;

	app_sat2ip_mode_item_init();
	app_sat2ip_prio_item_init();
#if SAT2IP_CONFIGURE_SUPPORT
	app_sat2ip_switch_item_init();
	app_sat2ip_rtsp_item_init();
	app_sat2ip_client_item_init();
#endif
	app_sat2ip_save_item_init();

	s_Sat2IpOpt.exit = app_sat2ip_exit_callback;//NULL;

	app_system_set_create(&s_Sat2IpOpt);
}
#endif
