#include "app.h"
#include "app_wnd_system_setting_opt.h"
#if START_CHANNEL_SUPPORT
#include "channel_common.h"
#endif

typedef enum {
    ITEM_AUDIO_REMEMBER = 0,
    ITEM_HOH_SUBTITLE,
#if SHUTDOWN_MODE_SUPPORT
    ITEM_SHUTDOWN_MODE,
#endif
#if BOOT_AD_USER_CONTROL_SUPPORT
    ITEM_BOOT_VIDEO,
    ITEM_BOOT_LOGO,
#endif
#if START_CHANNEL_SUPPORT
    ITEM_START_CHANNEL,
#endif
#if AUTO_UPDATE_SUPPORT
    ITEM_AUTO_UPDATE,
#endif
#if PLAYBACK_SPEED_SUPPORT
    ITEM_AUDIO_SPEEDMODE,
#endif
    ITEM_ADVANCE_TOTAL,
} AdvancedPrintMode;

typedef enum {
    MODE_OFF,
    MODE_ON,
}AdavancedMode;

typedef struct {
    AdavancedMode adreme_mode;
    AdavancedMode hoh_mode;
#if SHUTDOWN_MODE_SUPPORT
    AdavancedMode shutdown_mode;
#endif
#if BOOT_AD_USER_CONTROL_SUPPORT
    AdavancedMode boot_video_mode;
    AdavancedMode boot_logo_mode;
#endif
#if START_CHANNEL_SUPPORT
    int channel_num;
#endif
#if AUTO_UPDATE_SUPPORT
    AdavancedMode acu_mode;
#endif
#if PLAYBACK_SPEED_SUPPORT
    int speed_mode;
#endif
}AdvancedSettingPara;

extern void app_main_menu_listview_update(void);

static SystemSettingOpt  s_AdvanceSetOpt;
static SystemSettingItem s_AdvanceSettingItem[ITEM_ADVANCE_TOTAL];
static AdvancedSettingPara s_old_para;
#if START_CHANNEL_SUPPORT
static int *thiz_buffer_id = NULL;
static int _start_channel_save(int sel)
{
    int32_t prog_id = 0;
    if (thiz_buffer_id != NULL)
    {
        prog_id = thiz_buffer_id[sel];
        GxBus_ConfigSetInt(START_CHANNEL_ID,prog_id);

        GxCore_Free(thiz_buffer_id);
        thiz_buffer_id = NULL;
    }
    return 0;
}
#endif

static int _advance_setting_save_pop_cb(PopDlgRet ret)
{
    int sel = 0;
    if(POP_VAL_OK == ret)
    {
        if(s_old_para.adreme_mode != s_AdvanceSettingItem[ITEM_AUDIO_REMEMBER].itemProperty.itemPropertyCmb.sel)
        {
            sel =  s_AdvanceSettingItem[ITEM_AUDIO_REMEMBER].itemProperty.itemPropertyCmb.sel;
            GxBus_ConfigSetInt(ADVANCED_AUDIO_REMEMBER_KEY, sel);
        }
        if(s_old_para.hoh_mode != s_AdvanceSettingItem[ITEM_HOH_SUBTITLE].itemProperty.itemPropertyCmb.sel)
        {
            sel =  s_AdvanceSettingItem[ITEM_HOH_SUBTITLE].itemProperty.itemPropertyCmb.sel;
            GxBus_ConfigSetInt(SUBTITLE_HOH_KEY, sel);
        }
#if SHUTDOWN_MODE_SUPPORT
        if(s_old_para.shutdown_mode != s_AdvanceSettingItem[ITEM_SHUTDOWN_MODE].itemProperty.itemPropertyCmb.sel)
        {
            sel =  s_AdvanceSettingItem[ITEM_SHUTDOWN_MODE].itemProperty.itemPropertyCmb.sel;
        }
#endif
#if BOOT_AD_USER_CONTROL_SUPPORT
        if(s_old_para.boot_video_mode != s_AdvanceSettingItem[ITEM_BOOT_VIDEO].itemProperty.itemPropertyCmb.sel)
        {
            sel =  s_AdvanceSettingItem[ITEM_BOOT_VIDEO].itemProperty.itemPropertyCmb.sel;
            GxBus_ConfigSetInt(WIDGET_TEXT_VIDEO_AD_KEY,(int32_t)sel);
        }
        if(s_old_para.boot_logo_mode != s_AdvanceSettingItem[ITEM_BOOT_LOGO].itemProperty.itemPropertyCmb.sel)
        {
            sel =  s_AdvanceSettingItem[ITEM_BOOT_LOGO].itemProperty.itemPropertyCmb.sel;
            GxBus_ConfigSetInt(WIDGET_TEXT_LOGO_AD_KEY,(int32_t)sel);
        }
#endif
#if START_CHANNEL_SUPPORT
        if(s_old_para.channel_num != s_AdvanceSettingItem[ITEM_START_CHANNEL].itemProperty.itemPropertyCmb.sel)
        {
            sel =  s_AdvanceSettingItem[ITEM_START_CHANNEL].itemProperty.itemPropertyCmb.sel;
            _start_channel_save(sel);
        }
#endif
#if AUTO_UPDATE_SUPPORT
        if(s_old_para.acu_mode != s_AdvanceSettingItem[ITEM_AUTO_UPDATE].itemProperty.itemPropertyCmb.sel)
        {
            sel =  s_AdvanceSettingItem[ITEM_AUTO_UPDATE].itemProperty.itemPropertyCmb.sel;
            GxBus_ConfigSetInt(T2_ACU_KEY,(int32_t)sel);
        }
#endif
#if PLAYBACK_SPEED_SUPPORT
        if(s_old_para.speed_mode != s_AdvanceSettingItem[ITEM_AUDIO_SPEEDMODE].itemProperty.itemPropertyCmb.sel)
        {
            sel =  s_AdvanceSettingItem[ITEM_AUDIO_SPEEDMODE].itemProperty.itemPropertyCmb.sel;
            GxBus_ConfigSetInt(AUDIO_SPEED_MODE_KEY,(int32_t)sel);
            GxPlayer_SetAudioSpeedMode(sel);
        }
#endif
    }
    app_system_reset_unfocus_image();
    GUI_EndDialog(WND_SYSTEM_SETTING);
    GUI_SetProperty(WND_SYSTEM_SETTING,"draw_now",NULL);
    return 0;
}

static bool _advance_setting_check_cb(void)
{
    if (s_old_para.adreme_mode != s_AdvanceSettingItem[ITEM_AUDIO_REMEMBER].itemProperty.itemPropertyCmb.sel)
    {
        return true;
    }
    if (s_old_para.hoh_mode != s_AdvanceSettingItem[ITEM_HOH_SUBTITLE].itemProperty.itemPropertyCmb.sel)
    {
        return true;
    }
#if START_CHANNEL_SUPPORT
    if(s_old_para.channel_num != s_AdvanceSettingItem[ITEM_START_CHANNEL].itemProperty.itemPropertyCmb.sel)
    {
        return true;
    }
    else
    {
        if (thiz_buffer_id != NULL)
        {
            GxCore_Free(thiz_buffer_id);
            thiz_buffer_id = NULL;
        }

    }
#endif
#if AUTO_UPDATE_SUPPORT
    if (s_old_para.acu_mode != s_AdvanceSettingItem[ITEM_AUTO_UPDATE].itemProperty.itemPropertyCmb.sel)
    {
        return true;
    }
#endif
#if PLAYBACK_SPEED_SUPPORT
    if (s_old_para.speed_mode != s_AdvanceSettingItem[ITEM_AUDIO_SPEEDMODE].itemProperty.itemPropertyCmb.sel)
    {
        return true;
    }
#endif
    return false;
}

static int app_advance_choice_exit_cb(ExitType exit_type)
{
    int ret = EXIT_RET_DEFAULT;

    if(exit_type != EXIT_ABANDON)
    {
        app_system_set_callback_handler(_advance_setting_check_cb, _advance_setting_save_pop_cb);
        ret = EXIT_RET_POP;
    }
    return ret;
}

static void app_advance_audio_remember_init(void)
{
    s_AdvanceSettingItem[ITEM_AUDIO_REMEMBER].itemTitle = STR_ID_AUDIO_REMEMBER;
    s_AdvanceSettingItem[ITEM_AUDIO_REMEMBER].itemType = ITEM_CHOICE;
    s_AdvanceSettingItem[ITEM_AUDIO_REMEMBER].itemProperty.itemPropertyCmb.content = "[Off,On]";
    s_AdvanceSettingItem[ITEM_AUDIO_REMEMBER].itemProperty.itemPropertyCmb.sel = s_old_para.adreme_mode;
    s_AdvanceSettingItem[ITEM_AUDIO_REMEMBER].itemCallback.cmbCallback.CmbChange = NULL;
    s_AdvanceSettingItem[ITEM_AUDIO_REMEMBER].itemStatus = ITEM_NORMAL;
}

static void app_advance_hoh_subtitle_init(void)
{
    s_AdvanceSettingItem[ITEM_HOH_SUBTITLE].itemTitle = STR_ID_HOH_SUB;
    s_AdvanceSettingItem[ITEM_HOH_SUBTITLE].itemType = ITEM_CHOICE;
    s_AdvanceSettingItem[ITEM_HOH_SUBTITLE].itemProperty.itemPropertyCmb.content = "[Off,On]";
    s_AdvanceSettingItem[ITEM_HOH_SUBTITLE].itemProperty.itemPropertyCmb.sel = s_old_para.hoh_mode;
    s_AdvanceSettingItem[ITEM_HOH_SUBTITLE].itemCallback.cmbCallback.CmbChange = NULL;
    s_AdvanceSettingItem[ITEM_HOH_SUBTITLE].itemStatus = ITEM_NORMAL;
}
#if SHUTDOWN_MODE_SUPPORT
static int app_shutdown_mode_change(int sel)
{
	GxBus_ConfigSetInt(WIDGET_TEXT_SHUTDOWN_MODE_KEY,(int32_t)sel);
	return 0;
}

static void app_advance_shutdown_mode_item_init(void)
{
    int shutdown_mode_sel = 0;
	GxBus_ConfigGetInt(WIDGET_TEXT_SHUTDOWN_MODE_KEY, &shutdown_mode_sel, WIDGET_TEXT_SHUTDOWN_MODE_DEFAULT_KEY);

	s_AdvanceSettingItem[ITEM_SHUTDOWN_MODE].itemTitle = STR_ID_SHUTDOWN_MODE;
	s_AdvanceSettingItem[ITEM_SHUTDOWN_MODE].itemType = ITEM_CHOICE;
	s_AdvanceSettingItem[ITEM_SHUTDOWN_MODE].itemProperty.itemPropertyCmb.content = "[Off, On]";
	s_AdvanceSettingItem[ITEM_SHUTDOWN_MODE].itemProperty.itemPropertyCmb.sel = shutdown_mode_sel;
	s_AdvanceSettingItem[ITEM_SHUTDOWN_MODE].itemCallback.cmbCallback.CmbChange= app_shutdown_mode_change;
	s_AdvanceSettingItem[ITEM_SHUTDOWN_MODE].itemStatus = ITEM_NORMAL;
}
extern int top_stbk_halt(uint32_t scan_code);
static int _shutdown_mode_cb(PopDlgRet ret)
{
    if(POP_VAL_OK == ret)
    {
        app_reboot();
    }
    else
    {
        top_stbk_halt(0);
    }
    return 0;
}

int app_select_shutdow_mode_popup(void)
{
    PopDlg  pop;

    popdlg_destroy();
    memset(&pop, 0, sizeof(PopDlg));
    pop.type = POP_TYPE_YES_NO;
    pop.format = POP_FORMAT_DLG;
    pop.str = STR_ID_SHUTDOWN_REBOOT_INFO;
    pop.mode = POP_MODE_UNBLOCK;
    pop.exit_cb = _shutdown_mode_cb;
    popdlg_create(&pop);

    return 0;
}
#endif

#if BOOT_AD_USER_CONTROL_SUPPORT
static void app_video_ad_item_init(void)
{
    int video_ad_sel = 0;
	GxBus_ConfigGetInt(WIDGET_TEXT_VIDEO_AD_KEY, &video_ad_sel, WIDGET_TEXT_VIDEO_AD_DEFAULT_KEY);
	s_AdvanceSettingItem[ITEM_BOOT_VIDEO].itemTitle = STR_ID_BOOT_VIDEO_AD;
	s_AdvanceSettingItem[ITEM_BOOT_VIDEO].itemType = ITEM_CHOICE;
	s_AdvanceSettingItem[ITEM_BOOT_VIDEO].itemProperty.itemPropertyCmb.content = "[Off, On]";
	s_AdvanceSettingItem[ITEM_BOOT_VIDEO].itemProperty.itemPropertyCmb.sel = video_ad_sel;
	s_AdvanceSettingItem[ITEM_BOOT_VIDEO].itemCallback.cmbCallback.CmbChange= NULL;
	s_AdvanceSettingItem[ITEM_BOOT_VIDEO].itemStatus = ITEM_NORMAL;
}

static void app_logo_ad_item_init(void)
{
    int logo_ad_sel = 0;
	GxBus_ConfigGetInt(WIDGET_TEXT_LOGO_AD_KEY, &logo_ad_sel, WIDGET_TEXT_LOGO_AD_DEFAULT_KEY);
	s_AdvanceSettingItem[ITEM_BOOT_LOGO].itemTitle = STR_ID_BOOT_LOGO_AD;
	s_AdvanceSettingItem[ITEM_BOOT_LOGO].itemType = ITEM_CHOICE;
	s_AdvanceSettingItem[ITEM_BOOT_LOGO].itemProperty.itemPropertyCmb.content = "[Off, On]";
	s_AdvanceSettingItem[ITEM_BOOT_LOGO].itemProperty.itemPropertyCmb.sel = logo_ad_sel;
	s_AdvanceSettingItem[ITEM_BOOT_LOGO].itemCallback.cmbCallback.CmbChange= NULL;
	s_AdvanceSettingItem[ITEM_BOOT_LOGO].itemStatus = ITEM_NORMAL;
}
#endif

#if START_CHANNEL_SUPPORT
static char *g_buffer_all = NULL;
static uint32_t thiz_total = 0;
static int _start_channel_rebuild(char **buffer_all)
{
    int i = 0;
    int prog_id = 0;
    int find_id = 0;
    uint32_t total = 0;
    GxMsgProperty_NodeByPosGet node = {0};
    char temp[16] = {0};
    GxBusPmViewInfo change_pmview = {0};
    GxBusPmViewInfo viewinfo_bak = {0};
    GxMsgProperty_NodeNumGet node_num_get;

    app_send_msg_exec(GXMSG_PM_VIEWINFO_GET, (void *)(&change_pmview));
    memcpy(&viewinfo_bak, &change_pmview, sizeof(GxBusPmViewInfo));
    change_pmview.group_mode = GROUP_MODE_ALL;
    change_pmview.taxis_mode = TAXIS_MODE_NON;
    GxBus_PmViewInfoMemSet(&change_pmview);
    node_num_get.node_type = NODE_PROG;
    if(GXCORE_SUCCESS == app_send_msg_exec(GXMSG_PM_NODE_NUM_GET, (void *)(&node_num_get))) //cur group
    {
        g_AppChOps.create(node_num_get.node_num);
    }

    total = g_AppChOps.get_list_total() + 1;
    *buffer_all = GxCore_Calloc(total, MAX_PROG_NAME);
    if(*buffer_all == NULL)
    {
        GxBus_PmViewInfoMemSet(&viewinfo_bak);
        return -1;
    }

    if (thiz_buffer_id != NULL)
    {
        GxCore_Free(thiz_buffer_id);
        thiz_buffer_id = NULL;
    }
    thiz_buffer_id = (int *)GxCore_Calloc(total, sizeof(int));
    if(thiz_buffer_id == NULL)
    {
        GxBus_PmViewInfoMemSet(&viewinfo_bak);
        return -1;
    }
    thiz_total = total;
    GxBus_ConfigGetInt(START_CHANNEL_ID, &prog_id, START_CHANNEL_DEFAULT_KEY);
    strcat(*buffer_all, "[");

    for(i = 0; i < total; i++)
    {
        if (i == 0)
        {
            strcat(*buffer_all, "Off");
            strcat(*buffer_all, ",");
            thiz_buffer_id[i] = i;
            continue;
        }
        memset(&node,0,sizeof(GxMsgProperty_NodeByPosGet));
        node.node_type = NODE_PROG;
        node.id = g_AppChOps.get_id(i-1);
        app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &node);

        memset(temp,0,sizeof(temp));
        sprintf(temp,"%d ", i);
        strcat(*buffer_all, temp);
        strcat(*buffer_all, (char*)node.prog_data.prog_name);
        strcat(*buffer_all, ",");
        thiz_buffer_id[i] = node.prog_data.id;
        if (node.prog_data.id == prog_id)
        {
            s_old_para.channel_num = i;
            find_id++;
        }
    }
    strcat(*buffer_all, "]");
    if(find_id == 0)
    {
       s_old_para.channel_num = 0;
       prog_id = 0;
       GxBus_ConfigSetInt(START_CHANNEL_ID,prog_id);;
    }
    GxBus_PmViewInfoMemSet(&viewinfo_bak);
    return 0;
}

static int _app_start_channel_poplist_cb(int32_t ret_sel, unsigned short key)
{
    GUI_SetProperty(app_system_get_combobox(ITEM_START_CHANNEL), "select", &ret_sel);
    poplist_destory_cb_free_item_content();
    return 0;
}

static int _app_start_channel_poplist_get_info(char **p_get_info,uint32_t *count)
{
    int i = 0;
    char **channel_info = NULL;
    GxMsgProperty_NodeByPosGet node ={0};

    if((NULL == p_get_info)||(thiz_buffer_id == NULL))
    {
        app_log_error("\n_app_av_standard_poplist_get_info failed, param...%s,%d\n",__FILE__,__LINE__);
        return -1;
    }

    *count = thiz_total;//g_AppChOps.get_list_total() + 1;
    channel_info = (char**)GxCore_Calloc(thiz_total, sizeof(char*));
    if(NULL == channel_info)
    {
        app_log_error("\033[31m!!!!!!!!!![%s, %d]:GxCore_Calloc failed!!!!!!!!!!!\033[0m\n", __func__, __LINE__);
        return -1;
    }

    for(i = 0; i < thiz_total; i++)
    {
        channel_info[i] = (char*)GxCore_Malloc(MAX_PROG_NAME);
        if(NULL == channel_info[i])
        {
            app_log_error("\033[31m!!!!!!!!!![%s, %d]:GxCore_Calloc failed!!!!!!!!!!!\033[0m\n", __func__, __LINE__);
            goto FREE_DATA;
        }
        if (i == 0)
        {
            memcpy(channel_info[i],"Off", MAX_PROG_NAME);
            continue;
        }
        memset(&node,0,sizeof(GxMsgProperty_NodeByPosGet));
        node.node_type = NODE_PROG;
        node.id = thiz_buffer_id[i];
        app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &node);
        memcpy(channel_info[i],node.prog_data.prog_name, MAX_PROG_NAME);
    }

    *p_get_info = (char*)channel_info;
    return thiz_total;
FREE_DATA:
    if(channel_info != NULL)
    {
        for(i = 0; i < thiz_total; i++)
        {
            if(channel_info[i] != NULL)
                GxCore_Free(channel_info[i]);
        }
        GxCore_Free(channel_info);
    }

    return 0;
}

static int app_start_channel_cmb_press_callback(int key)
{
    PopList pop_list;
    uint32_t total = 0;
    char *p_channel_info = NULL ;

    app_system_set_item_data_update(ITEM_START_CHANNEL);
    if(key == STBK_OK)
    {
       memset(&pop_list, 0, sizeof(PopList));
        if ( _app_start_channel_poplist_get_info(&p_channel_info,&total) < 0 )
            return EVENT_TRANSFER_STOP ;
         pop_list.title = STR_ID_START_CHANNEL;
         pop_list.item_num = total;
         pop_list.item_content = (char **)p_channel_info;
         pop_list.sel = s_AdvanceSettingItem[ITEM_START_CHANNEL].itemProperty.itemPropertyCmb.sel;
         pop_list.show_num= true;
         pop_list.mode= POP_MODE_UNBLOCK;
         pop_list.exit_cb = _app_start_channel_poplist_cb;
         poplist_create(&pop_list);
     }

     return EVENT_TRANSFER_KEEPON;
}

static void app_advanced_start_channel_item_init(void)
{
   if (g_buffer_all != NULL)
    {
        GxCore_Free(g_buffer_all);
        g_buffer_all = NULL;
    }
    _start_channel_rebuild(&g_buffer_all);

    s_AdvanceSettingItem[ITEM_START_CHANNEL].itemTitle = STR_ID_START_CHANNEL;
    s_AdvanceSettingItem[ITEM_START_CHANNEL].itemType = ITEM_CHOICE;
    s_AdvanceSettingItem[ITEM_START_CHANNEL].itemProperty.itemPropertyCmb.content = g_buffer_all;
    s_AdvanceSettingItem[ITEM_START_CHANNEL].itemProperty.itemPropertyCmb.sel = s_old_para.channel_num;
    s_AdvanceSettingItem[ITEM_START_CHANNEL].itemCallback.cmbCallback.CmbChange= NULL;
    s_AdvanceSettingItem[ITEM_START_CHANNEL].itemCallback.cmbCallback.CmbPress= app_start_channel_cmb_press_callback;
    s_AdvanceSettingItem[ITEM_START_CHANNEL].itemStatus = ITEM_NORMAL;
}
extern uint32_t app_play_id_set(uint32_t id);
int32_t app_get_start_channel_number(int32_t pos)
{
    int prog_id = 0;
    int32_t ret_value = pos;
    GxBusPmViewInfo pm_view_cur = {0};
    GxBusPmViewInfo pm_old_view = {0};
    GxMsgProperty_NodeByPosGet node = {0};

    GxBus_ConfigGetInt(START_CHANNEL_ID, &prog_id, START_CHANNEL_DEFAULT_KEY);
    if (prog_id <= 0) {
        return ret_value;
    }

    pos = GxBus_PmProgGetIndexPosById(prog_id);
    if(pos != -1){
        memset(&node,0,sizeof(GxMsgProperty_NodeByPosGet));
        node.node_type = NODE_PROG;
        node.pos = pos;
        app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &node);
    }

    if((pos == -1) || (node.prog_data.id != prog_id)) {
        app_send_msg_exec(GXMSG_PM_VIEWINFO_GET, (void *)(&pm_view_cur));
        memcpy(&pm_old_view, &pm_view_cur, sizeof(GxBusPmViewInfo));
        pm_view_cur.group_mode = GROUP_MODE_ALL;
        pm_view_cur.taxis_mode = TAXIS_MODE_NON;
        app_send_msg_exec(GXMSG_PM_VIEWINFO_SET, (void *)(&pm_view_cur));
        pos = GxBus_PmProgGetIndexPosById(prog_id);
        if(pos == -1){
            app_send_msg_exec(GXMSG_PM_VIEWINFO_SET, (void *)(&pm_old_view));
            return ret_value;
        }
    }
    app_play_id_set(prog_id);
    return ret_value;
}
#endif

#if AUTO_UPDATE_SUPPORT
static void app_advanced_acu_init(void)
{
	s_AdvanceSettingItem[ITEM_AUTO_UPDATE].itemTitle = STR_ID_ACU;
	s_AdvanceSettingItem[ITEM_AUTO_UPDATE].itemType = ITEM_CHOICE;
	s_AdvanceSettingItem[ITEM_AUTO_UPDATE].itemProperty.itemPropertyCmb.content = "[Off, On]";
	s_AdvanceSettingItem[ITEM_AUTO_UPDATE].itemProperty.itemPropertyCmb.sel = s_old_para.acu_mode;
	s_AdvanceSettingItem[ITEM_AUTO_UPDATE].itemCallback.cmbCallback.CmbChange= NULL;
	s_AdvanceSettingItem[ITEM_AUTO_UPDATE].itemStatus = ITEM_NORMAL;
}
#endif

#if PLAYBACK_SPEED_SUPPORT
static void app_advanced_audio_speedmode_init(void)
{
    s_AdvanceSettingItem[ITEM_AUDIO_SPEEDMODE].itemTitle = STR_ID_AUDIO_SPEEDMODE;
    s_AdvanceSettingItem[ITEM_AUDIO_SPEEDMODE].itemType = ITEM_CHOICE;
    s_AdvanceSettingItem[ITEM_AUDIO_SPEEDMODE].itemProperty.itemPropertyCmb.content = "[Normal,Advanced]";
    s_AdvanceSettingItem[ITEM_AUDIO_SPEEDMODE].itemProperty.itemPropertyCmb.sel = s_old_para.speed_mode;
    s_AdvanceSettingItem[ITEM_AUDIO_SPEEDMODE].itemCallback.cmbCallback.CmbChange= NULL;
    s_AdvanceSettingItem[ITEM_AUDIO_SPEEDMODE].itemStatus = ITEM_NORMAL;
}
#endif

void app_advanced_setting_menu_exec(void)
{
    int sel = 0;
    int hoh_value = 0;
    GxBus_ConfigGetInt(ADVANCED_AUDIO_REMEMBER_KEY, &sel, ADVANCED_AUDIO_REMEMBER_VALUE);
    s_old_para.adreme_mode = sel;
    GxBus_ConfigGetInt(SUBTITLE_HOH_KEY, &hoh_value, SUBTITLE_HOH_VALUE);
    s_old_para.hoh_mode = hoh_value;

    memset(&s_AdvanceSetOpt, 0, sizeof(SystemSettingOpt));
    s_AdvanceSetOpt.menuTitle = STR_ID_ADVANCED_SET;
    s_AdvanceSetOpt.itemNum = ITEM_ADVANCE_TOTAL;
    s_AdvanceSetOpt.item = s_AdvanceSettingItem;
    s_AdvanceSetOpt.timeDisplay = TIP_HIDE;

    app_advance_audio_remember_init();
    app_advance_hoh_subtitle_init();
#if SHUTDOWN_MODE_SUPPORT
    app_advance_shutdown_mode_item_init();
#endif

#if BOOT_AD_USER_CONTROL_SUPPORT
    GxBus_ConfigGetInt(WIDGET_TEXT_VIDEO_AD_KEY, &sel, WIDGET_TEXT_VIDEO_AD_DEFAULT_KEY);
    s_old_para.boot_video_mode = sel;
    GxBus_ConfigGetInt(WIDGET_TEXT_LOGO_AD_KEY, &sel, WIDGET_TEXT_LOGO_AD_DEFAULT_KEY);
    s_old_para.boot_logo_mode = sel;
    app_video_ad_item_init();
    app_logo_ad_item_init();
#endif
#if START_CHANNEL_SUPPORT
    app_advanced_start_channel_item_init();
#endif
#if AUTO_UPDATE_SUPPORT
    GxBus_ConfigGetInt(T2_ACU_KEY, &sel, T2_ACU_VALUE);
    s_old_para.acu_mode = sel;
    app_advanced_acu_init();
#endif
#if PLAYBACK_SPEED_SUPPORT
    GxBus_ConfigGetInt(AUDIO_SPEED_MODE_KEY, &sel, AUDIO_SPEED_MODE_VALUE);
    s_old_para.speed_mode = sel;
    app_advanced_audio_speedmode_init();
#endif
    s_AdvanceSetOpt.exit = app_advance_choice_exit_cb;

    app_system_set_create(&s_AdvanceSetOpt);
#if START_CHANNEL_SUPPORT
    if (g_buffer_all != NULL)
    {
        GxCore_Free(g_buffer_all);
        g_buffer_all = NULL;
    }
#endif
}

int app_audio_remember_setting_keypress(int key)
{
    int sel = 0, value = 0;
     if(STBK_LEFT != key && STBK_RIGHT != key)
         return EVENT_TRANSFER_KEEPON;
     GxBus_ConfigGetInt(ADVANCED_AUDIO_REMEMBER_KEY, &sel, ADVANCED_AUDIO_REMEMBER_VALUE);
     value = (++sel )% 2;
     GxBus_ConfigSetInt(ADVANCED_AUDIO_REMEMBER_KEY, value);
     app_main_menu_listview_update();
     return EVENT_TRANSFER_KEEPON;
}

char *app_audio_remember_setting_content_get(void)
{
    char *string = NULL;
    int sel = 0;
    GxBus_ConfigGetInt(ADVANCED_AUDIO_REMEMBER_KEY, &sel, ADVANCED_AUDIO_REMEMBER_VALUE);
    if(0 == sel)
    {
        string = STR_ID_OFF;
    }
    else
    {
        string = STR_ID_ON;
    }
    return string;
}

int app_subt_hoh_keypress(int key)
{
    //Hard-Of-Hearing subtitle setting
    int hoh_value = 0, value = 0;

    if(STBK_LEFT != key && STBK_RIGHT != key)
        return EVENT_TRANSFER_KEEPON;

    GxBus_ConfigGetInt(SUBTITLE_HOH_KEY, &hoh_value, SUBTITLE_HOH_VALUE);
    value = (++hoh_value) % 2;
    GxBus_ConfigSetInt(SUBTITLE_HOH_KEY, value);
    app_main_menu_listview_update();

    return EVENT_TRANSFER_KEEPON;
}

char *app_subt_hoh_content_get(void)
{
    char *string = NULL;
    int hoh_value = 0;

    GxBus_ConfigGetInt(SUBTITLE_HOH_KEY, &hoh_value, SUBTITLE_HOH_VALUE);
    if(0 == hoh_value)
        string = STR_ID_OFF;
    else
        string = STR_ID_ON;

    return string;
}
